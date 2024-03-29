#include "player.h"

#include <SDL_video.h>
#include <cstdio>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>
#include <string>
#include <sys/types.h>
#include <wayland-client.h>

#include <cstdint>

#include "SDL_error.h"
#include "SDL_hints.h"
#include "SDL_stdinc.h"
#include "SDL_syswm.h"
#include "SDL_timer.h"
#include "SDL_version.h"
#include "config.h"
#include "util.h"
namespace sendprotest {

static uint32_t wakeup_on_mpv_render_update;
static uint32_t wakeup_on_mpv_events;

static void *get_proc_address_mpv(void *fn_ctx, const char *name) {
    return SDL_GL_GetProcAddress(name);
}

static void on_mpv_events(void *) {
    SDL_Event event = {.type = wakeup_on_mpv_events};
    SDL_PushEvent(&event);
}

static void on_mpv_render_update(void *) {
    SDL_Event event = {.type = wakeup_on_mpv_render_update};
    SDL_PushEvent(&event);
}

Player::Player(struct window_ctx *ctx, const std::string &media_path, Api &api,
               int category, int playback_duration)
    : api(api),
      category(category),
      file_loaded(false),
      media_path(media_path) {
    mpv = mpv_create();
    //mpv_request_log_messages(mpv, "trace");
    if (!mpv) die("context init failed");
    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0) die("mpv init failed");

    mpv_set_property_string(mpv, "hwdec", "h264-drm-copy");
    mpv_set_property_string(mpv, "shuffle", "");

    // Jesus Christ SDL, you suck!
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "no");
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) die("SDL init failed");

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_FLAGS,
        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);  // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
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
    SDL_GL_SetSwapInterval(1);

    gladLoadGLES2Loader(SDL_GL_GetProcAddress);
    mpv_opengl_init_params opengl_params = {
        .get_proc_address = get_proc_address_mpv,
    };
    int advanced_control_param = 1;

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);

    if (!SDL_GetWindowWMInfo(ctx->window, &info)) {
        die("Failed to get WM info: %s\n", SDL_GetError());
    }
    struct wl_display *display = info.info.wl.display;

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, (void *)MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &opengl_params},
        // Tell libmpv that you will call mpv_render_context_update() on
        // render context update callbacks, and that you will _not_ block
        // on the core ever (see <libmpv/render.h> "Threading" section for
        // what libmpv functions you can call at all when this is active).
        // In particular, this means you must call e.g. mpv_command_async()
        // instead of mpv_command().
        // If you want to use synchronous calls, either make them on a
        // separate thread, or remove the option below (this will disable
        // features like DR and is not recommended anyway).
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advanced_control_param},
        {MPV_RENDER_PARAM_WL_DISPLAY, display},
        {MPV_RENDER_PARAM_INVALID, NULL}};

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

    const auto duration_str = to_string(playback_duration);
    mpv_set_property_string(mpv, "image-display-duration",
                            duration_str.c_str());
}

Player::~Player() {
    mpv_render_context_free(mpv_gl);
    mpv_destroy(mpv);
}

void Player::play_file(const std::string &file_name) {
    auto absolute_path =
        media_path + "/" + to_string(category) + "/" + file_name;
    if (category == COMBINED_CATEGORY) {
        absolute_path =
            media_path + "/" + file_name;
    }
    const char *loadfile_cmd[] = {"loadfile", absolute_path.c_str(), "replace",
                                  NULL};
    mpv_command_async(mpv, 0, loadfile_cmd);
}

void Player::set_category(int new_category) {
    category = new_category;
    // The stop command will empty the playlist. Once this is done, mpv will
    // enter the idle state, which will cause run() to load a playlist with the
    // new category
    const char *stop_cmd[] = {"stop", NULL};
    mpv_command_async(mpv, 0, stop_cmd);
}

void Player::run(struct window_ctx *ctx, SDL_Event event, unsigned int fbo,
                 uint64_t &draw_target_tick, int playback_duration,
                 bool &playlist_next) {
    // Happens when at least 1 new event is in the mpv event
    // queue.
    if (event.type == wakeup_on_mpv_events) {
        // Handle all remaining mpv events.
        while (1) {
            mpv_event *mp_event = mpv_wait_event(mpv, 0);
            if (mp_event->event_id == MPV_EVENT_NONE) break;
            if (mp_event->event_id == MPV_EVENT_LOG_MESSAGE) {
                mpv_event_log_message *msg =
                    (mpv_event_log_message *)mp_event->data;
                printf("[mpv] %s\n", msg->text);
                continue;
            }
            if (mp_event->event_id == MPV_EVENT_IDLE) {
                auto playback_path = media_path + "/" + to_string(category);
                if (category == 9) {
                    // special category to load all categories
                    playback_path = media_path + "/";
                }
                const char *load_cmd[] = {"loadfile", playback_path.c_str(), "replace",
                    NULL};
                mpv_command_async(mpv, (uint64_t)Command::Loadfile, load_cmd);
            }
            if (mp_event->event_id == MPV_EVENT_FILE_LOADED) {
                mpv_get_property_async(mpv, (uint64_t)Command::Duration,
                                       "duration", MPV_FORMAT_INT64);
                mpv_get_property_async(mpv, (uint64_t)Command::Filename,
                                       "filename", MPV_FORMAT_STRING);
                file_loaded = true;
            }
            if (mp_event->event_id == MPV_EVENT_PLAYBACK_RESTART) {
                if (file_loaded) {
                    file_loaded = false;
                    playlist_next = true;
                }
            }
            if (mp_event->event_id == MPV_EVENT_COMMAND_REPLY) {
                if (mp_event->reply_userdata == (uint64_t)Command::Loadfile) {
                    const char *shuffle_cmd[] = {"playlist-shuffle", NULL};
                    mpv_command_async(mpv, (uint64_t)Command::Shuffle,
                                      shuffle_cmd);
                }
                if (mp_event->reply_userdata == (uint64_t)Command::Shuffle) {
                    const char *next_cmd[] = {"playlist-next", NULL};
                    mpv_command_async(mpv, 0, next_cmd);
                }
            }
            if (mp_event->event_id == MPV_EVENT_GET_PROPERTY_REPLY) {
                auto event_property =
                    static_cast<mpv_event_property *>(mp_event->data);
                if (mp_event->reply_userdata == (uint64_t)Command::Duration) {
                    if (event_property->format != MPV_FORMAT_INT64 ||
                        strcmp(event_property->name, "duration")) {
                        die("Unexpected property reply\n");
                    }
                    auto video_duration =
                        *static_cast<int64_t *>(event_property->data);
                    int64_t loops = 0;
                    if (video_duration > 0 &&
                        video_duration != playback_duration) {
                        loops = (playback_duration / video_duration);
                        loops = loops < 0 ? 0 : loops;
                    }
                    printf("video_duration: %ld loop: %ld loops\n",
                           video_duration, loops);
                    mpv_set_property_async(mpv, 0, "loop", MPV_FORMAT_INT64,
                                           &loops);
                } else if (mp_event->reply_userdata ==
                           (uint64_t)Command::Filename) {
                    if (event_property->format != MPV_FORMAT_STRING ||
                        strcmp(event_property->name, "filename")) {
                        die("Unexpected property reply\n");
                    }
                    auto filename = std::string(
                        *static_cast<char **>(event_property->data));
                    printf("playing %s\n", filename.c_str());
                    api.set_play_cmd(filename);
                    mpv_free(event_property->data);
                } else {
                    die("Unknown reply_userdata %lu\n",
                        mp_event->reply_userdata);
                }
            }
            printf("[mpv] event: %s\n", mpv_event_name(mp_event->event_id));
        }
        return;
    }
    if (event.type == wakeup_on_mpv_render_update) {
        // Happens when there is new work for the render thread
        // (such as rendering a new video frame or redrawing it).
        uint64_t flags = mpv_render_context_update(mpv_gl);
        if (!(flags & MPV_RENDER_UPDATE_FRAME)) {
            return;
        }
        int w, h;
        SDL_GetWindowSize(ctx->window, &w, &h);

        mpv_opengl_fbo mpv_fbo = {
            .fbo = (int)fbo,
            .w = w,
            .h = h,
        };
        int mpv_flip_y = 1;
        mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &mpv_fbo},
                                     // Flip rendering (needed due to
                                     // flipped GL coordinate system).
                                     {MPV_RENDER_PARAM_FLIP_Y, &mpv_flip_y},
                                     {MPV_RENDER_PARAM_INVALID, NULL}};

        mpv_render_context_render(mpv_gl, params);
        draw_target_tick = SDL_GetTicks64();
        return;
    }
}
}  // namespace sendprotest
