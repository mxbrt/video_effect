#include <SDL_video.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>

#include "util.h"
#include "player.h"

static Uint32 wakeup_on_mpv_render_update, wakeup_on_mpv_events;
static mpv_handle *mpv;
static mpv_render_context *mpv_gl;

static void *get_proc_address_mpv(void *fn_ctx, const char *name) {
    return SDL_GL_GetProcAddress(name);
}

static void on_mpv_events(void *ctx) {
    SDL_Event event = {.type = wakeup_on_mpv_events};
    SDL_PushEvent(&event);
}

static void on_mpv_render_update(void *ctx) {
    SDL_Event event = {.type = wakeup_on_mpv_render_update};
    SDL_PushEvent(&event);
}

void player_create(struct window_ctx *ctx) {
    mpv = mpv_create();
    if (!mpv) die("context init failed");

    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0) die("mpv init failed");

    mpv_request_log_messages(mpv, "debug");

    // Jesus Christ SDL, you suck!
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "no");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) die("SDL init failed");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    ctx->window =
        SDL_CreateWindow("hi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!ctx->window) die("failed to create SDL window");
    ctx->gl = SDL_GL_CreateContext(ctx->window);
    if (!ctx->gl) die("failed to create SDL GL context");

    SDL_GL_MakeCurrent(ctx->window, ctx->gl);
    SDL_GL_SetSwapInterval(1);  // Enable vsync

    gladLoadGL();

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS,
         &(mpv_opengl_init_params){
             .get_proc_address = get_proc_address_mpv,
         }},
        // Tell libmpv that you will call mpv_render_context_update() on
        // render context update callbacks, and that you will _not_ block
        // on the core ever (see <libmpv/render.h> "Threading" section for
        // what libmpv functions you can call at all when this is active).
        // In particular, this means you must call e.g. mpv_command_async()
        // instead of mpv_command().
        // If you want to use synchronous calls, either make them on a
        // separate thread, or remove the option below (this will disable
        // features like DR and is not recommended anyway).
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &(int){1}},
        {0}};

    // This makes mpv use the currently set GL context. It will use the
    // callback (passed via params) to resolve GL builtin functions, as well
    // as extensions.
    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
        die("failed to initialize mpv GL context");

    // We use events for thread-safe notification of the SDL main loop.
    // Generally, the wakeup callbacks (set further below) should do as
    // least work as possible, and merely wake up another thread to do
    // actual work. On SDL, waking up the mainloop is the ideal course of
    // action. SDL's SDL_PushEvent() is thread-safe, so we use that.
    wakeup_on_mpv_render_update = SDL_RegisterEvents(1);
    wakeup_on_mpv_events = SDL_RegisterEvents(1);
    if (wakeup_on_mpv_render_update == (Uint32)-1 ||
        wakeup_on_mpv_events == (Uint32)-1)
        die("could not register events");

    // When normal mpv events are available.
    mpv_set_wakeup_callback(mpv, on_mpv_events, NULL);

    // When there is a need to call mpv_render_context_update(), which can
    // request a new frame to be rendered.
    // (Separate from the normal event handling mechanism for the sake of
    //  users which run OpenGL on a different thread.)
    mpv_render_context_set_update_callback(mpv_gl, on_mpv_render_update, NULL);

    mpv_set_property_string(mpv, "image-display-duration", "15");
}

void player_cmd(const char *cmd[]) {
    mpv_command_async(mpv, 0, cmd);
}

enum player_event player_run(struct window_ctx *ctx, SDL_Event event,
                             unsigned int fbo) {
    enum player_event result = PLAYER_NO_EVENT;
    // Happens when there is new work for the render thread
    // (such as rendering a new video frame or redrawing it).
    if (event.type == wakeup_on_mpv_render_update) {
        uint64_t flags = mpv_render_context_update(mpv_gl);
        if (flags & MPV_RENDER_UPDATE_FRAME) result = PLAYER_REDRAW;
    }
    // Happens when at least 1 new event is in the mpv event
    // queue.
    if (event.type == wakeup_on_mpv_events) {
        // Handle all remaining mpv events.
        while (1) {
            mpv_event *mp_event = mpv_wait_event(mpv, 0);
            if (mp_event->event_id == MPV_EVENT_NONE) break;
            if (mp_event->event_id == MPV_EVENT_LOG_MESSAGE) {
                mpv_event_log_message *msg = mp_event->data;
                // Print log messages about DR allocations, just
                // to test whether it works. If there is more
                // than 1 of these, it works. (The log message
                // can actually change any time, so it's
                // possible this logging stops working in the
                // future.)
                if (strstr(msg->text, "DR image")) printf("log: %s", msg->text);
                continue;
            }
            if (mp_event->event_id == MPV_EVENT_IDLE) {
                result = PLAYER_IDLE;
            }
            printf("event: %s\n", mpv_event_name(mp_event->event_id));
        }
    }
    if (result == PLAYER_REDRAW) {
        int w, h;
        SDL_GetWindowSize(ctx->window, &w, &h);
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO,
             &(mpv_opengl_fbo){
                 .fbo = fbo,
                 .w = w,
                 .h = h,
             }},
            // Flip rendering (needed due to flipped GL coordinate
            // system).
            {MPV_RENDER_PARAM_FLIP_Y, &(int){1}},
            {0}};
        // See render_gl.h on what OpenGL environment mpv expects, and
        // other API details.
        mpv_render_context_render(mpv_gl, params);
    }
    return result;
}

void player_free() {
    // Destroy the GL renderer and all of the GL objects it allocated. If video
    // is still running, the video track will be deselected.
    mpv_render_context_free(mpv_gl);
    mpv_detach_destroy(mpv);
}

