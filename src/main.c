#include <errno.h>
#include <glad/glad.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// clang-format off
// must be included after glad
#include <SDL.h>
// clang-format on

static Uint32 wakeup_on_mpv_render_update, wakeup_on_mpv_events;

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

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

const int compile_shader(const char *path, int type) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        die("Could not open shader");
    }
    char shader_src[4096 * 1024] = {0};
    size_t src_len = fread(shader_src, 1, sizeof(shader_src), fp);
    if (src_len >= sizeof(shader_src)) {
        die("Shader too large");
    }
    const char *shader_arg[2] = {shader_src, NULL};

    unsigned int shader = glCreateShader(type);
    int success;
    char info[512];
    glShaderSource(shader, 1, shader_arg, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info);
        fprintf(stderr, "shader compilation failed\n%s\n", info);
        exit(1);
    }
    return shader;
}

const int init_shader_program(const int vert_shader, const int frag_shader) {
    int success;
    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vert_shader);
    glAttachShader(shader_program, frag_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(shader_program, 512, NULL, info);
        fprintf(stderr, "shader linking failed\n\n%s", info);
        exit(1);
    }
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    return shader_program;
}

float quadVertices[] = {  // vertex attributes for a quad that fills the entire
                          // screen in Normalized Device Coordinates.
    // positions   // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

int main(int argc, char *argv[]) {
    if (argc != 2) die("pass a single media file as argument");

    mpv_handle *mpv = mpv_create();
    if (!mpv) die("context init failed");

    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0) die("mpv init failed");

    mpv_request_log_messages(mpv, "debug");

    // Jesus Christ SDL, you suck!
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "no");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) die("SDL init failed");

    SDL_Window *window =
        SDL_CreateWindow("hi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) die("failed to create SDL window");

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    if (!glcontext) die("failed to create SDL GL context");

    gladLoadGL();

    // shader configuration
    const int vert_shader =
        compile_shader("shaders/vert.glsl", GL_VERTEX_SHADER);
    const int frag_shader =
        compile_shader("shaders/frag.glsl", GL_FRAGMENT_SHADER);
    const int shader_program = init_shader_program(vert_shader, frag_shader);

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));

    glUniform1i(glGetUniformLocation(frag_shader, "movieTexture"), 0);

    // framebuffer configuration
    unsigned int mpv_fbo;
    glGenFramebuffers(1, &mpv_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mpv_fbo);
    unsigned int mpv_texture;
    glGenTextures(1, &mpv_texture);
    glBindTexture(GL_TEXTURE_2D, mpv_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           mpv_texture, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1920, 1080);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        die("Framebuffer incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
    mpv_render_context *mpv_gl;
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

    // Play this file.
    const char *cmd[] = {"loadfile", argv[1], NULL};
    mpv_command_async(mpv, 0, cmd);

    while (1) {
        SDL_Event event;
        if (SDL_WaitEvent(&event) != 1) die("event loop error");
        int redraw = 0;
        switch (event.type) {
            case SDL_QUIT:
                goto done;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_EXPOSED) redraw = 1;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_SPACE) {
                    const char *cmd_pause[] = {"cycle", "pause", NULL};
                    mpv_command_async(mpv, 0, cmd_pause);
                }
                if (event.key.keysym.sym == SDLK_s) {
                    // Also requires MPV_RENDER_PARAM_ADVANCED_CONTROL if
                    // you want screenshots to be rendered on GPU (like
                    // --vo=gpu would do).
                    const char *cmd_scr[] = {"screenshot-to-file",
                                             "screenshot.png", "window", NULL};
                    printf("attempting to save screenshot to %s\n", cmd_scr[1]);
                    mpv_command_async(mpv, 0, cmd_scr);
                }
                break;
            default:
                // Happens when there is new work for the render thread
                // (such as rendering a new video frame or redrawing it).
                if (event.type == wakeup_on_mpv_render_update) {
                    uint64_t flags = mpv_render_context_update(mpv_gl);
                    if (flags & MPV_RENDER_UPDATE_FRAME) redraw = 1;
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
                            if (strstr(msg->text, "DR image"))
                                printf("log: %s", msg->text);
                            continue;
                        }
                        printf("event: %s\n",
                               mpv_event_name(mp_event->event_id));
                    }
                }
        }
        if (redraw) {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            mpv_render_param params[] = {
                // Specify the default framebuffer (0) as target. This will
                // render onto the entire screen. If you want to show the
                // video
                // in a smaller rectangle or apply fancy transformations,
                // you'll
                // need to render into a separate FBO and draw it manually.
                {MPV_RENDER_PARAM_OPENGL_FBO,
                 &(mpv_opengl_fbo){
                     .fbo = mpv_fbo,
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
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(shader_program);
            glad_glBindVertexArray(quadVAO);
            glBindTexture(GL_TEXTURE_2D, mpv_texture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            SDL_GL_SwapWindow(window);
        }
    }
done:

    // Destroy the GL renderer and all of the GL objects it allocated. If video
    // is still running, the video track will be deselected.
    mpv_render_context_free(mpv_gl);

    mpv_detach_destroy(mpv);

    glDeleteFramebuffers(1, &mpv_fbo);

    printf("properly terminated\n");
    return 0;
}
