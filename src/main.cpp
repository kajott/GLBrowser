#include <cstdint>
#include <cstdio>

#include <SDL.h>

#include "glad.h"

#include "app.h"

#ifndef NDEBUG
    #define IFRELEASE(a,b) (b)
#else
    #define IFRELEASE(a,b) (a)
#endif

///////////////////////////////////////////////////////////////////////////////

static constexpr Uint64 TypematicDelay    =  250u;
static constexpr Sint16 AnalogSensitivity = 8192;

namespace FTDirection {
    constexpr int Left  = 0;
    constexpr int Right = 1;
    constexpr int Up    = 2;
    constexpr int Down  = 3;
    constexpr int Mask  = 3;
    constexpr int XAxis = Left;
    constexpr int YAxis = Up;
};

namespace FTSource {
    constexpr int DPad       = 0;
    constexpr int LeftStick  = 4;
    constexpr int RightStick = 8;
    constexpr int Trigger    = 12;
    constexpr int Mask       = 0x0C;
};

class FakeTypematic {
    GLBrowserApp& m_app;
    uint16_t m_buttons;
    Uint64 m_timeouts[14];
    void fireEvent(int button);
public:
    inline FakeTypematic(GLBrowserApp& app) : m_app(app), m_buttons(0u) {}
    inline bool buttonsPressed() { return (m_buttons != 0u); }
    void setState(int button, bool state);
    inline void setAxis(int axis, Sint16 value) {
        setState(axis,     (value < -AnalogSensitivity));
        setState(axis + 1, (value > +AnalogSensitivity));
    }
    bool update();
};

void FakeTypematic::fireEvent(int button) {
    if (button >= FTSource::Trigger) {
        switch (button) {
            case FTSource::Trigger + FTDirection::Left:  m_app.handleEvent(AppEvent::LT); break;
            case FTSource::Trigger + FTDirection::Right: m_app.handleEvent(AppEvent::RT); break;
            default: break;
        }
    } else {
        switch (button & FTDirection::Mask) {
            case FTDirection::Left:  m_app.handleEvent(AppEvent::Left);  break;
            case FTDirection::Right: m_app.handleEvent(AppEvent::Right); break;
            case FTDirection::Up:    m_app.handleEvent(AppEvent::Up);    break;
            case FTDirection::Down:  m_app.handleEvent(AppEvent::Down);  break;
            default: break;
        }
    }
    m_timeouts[button] = SDL_GetTicks64() + TypematicDelay;
}

void FakeTypematic::setState(int button, bool state) {
    uint16_t mask = 1u << button;
//printf("state %2d %d -> %d\n", button, (m_buttons >> button) & 1, state);
    if (!state) {
        m_buttons &= ~mask;
    } else if (!(m_buttons & mask)) {
        m_buttons |= mask;
        fireEvent(button);
    }
}

bool FakeTypematic::update() {
    Uint64 now = SDL_GetTicks64();
    bool res = false;
    // only up to 12 -- no actual typematic for the triggers, just state tracking!
    for (int button = 0;  button < 12;  ++button) {
        if ((m_buttons & (1u << button)) && (now >= m_timeouts[button])) {
            fireEvent(button);
            res = true;
        }
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "FATAL: SDL initialization failed - %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow(
        "GL Launcher",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        IFRELEASE(0, 1280),
        IFRELEASE(0,  720),
        IFRELEASE(SDL_WINDOW_FULLSCREEN_DESKTOP, 0) | SDL_WINDOW_OPENGL);
    if (!win) {
        fprintf(stderr, "FATAL: failed to create window - %s\n", SDL_GetError());
        return 1;
    }
    #ifdef NDEBUG
        SDL_ShowCursor(SDL_DISABLE);
    #endif

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,   8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GLContext glctx = SDL_GL_CreateContext(win);
    if (!glctx) {
        fprintf(stderr, "FATAL: failed to create OpenGL context - %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_MakeCurrent(win, glctx);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGL()) {
        fprintf(stderr, "FATAL: failed to import OpenGL functions - %s\n", SDL_GetError());
        return 1;
    }
    #ifdef _DEBUG
        printf("OpenGL version:  %s\n", glGetString(GL_VERSION));
        printf("OpenGL vendor:   %s\n", glGetString(GL_VENDOR));
        printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    #endif

    bool active = true;
    auto actionCallback = [&] (AppAction action) {
        switch (action) {
            case AppAction::Quit:     active = false;          break;
            case AppAction::Minimize: SDL_MinimizeWindow(win); break;
            case AppAction::Restore:  SDL_RestoreWindow(win);  break;
            default: break;
        }
    };

    static GLBrowserApp app(actionCallback, argv[0]);
    if (!app.init((argc > 1) ? argv[1] : nullptr)) {
        return 1;
    }

    for (int i = 0;  i < SDL_NumJoysticks();  ++i) {
        if (SDL_IsGameController(i)) {
            if (SDL_GameControllerOpen(i)) { app.haveController(); }
        }
    }
    SDL_GameControllerEventState(SDL_ENABLE);
    FakeTypematic typematic(app);

    Uint64 prevTime = SDL_GetPerformanceCounter();
    while (active) {
        // wait for events, if we need to
        bool wait = !app.framesRequested();
        if (wait) {
            if (typematic.buttonsPressed()) {
                do {
                    SDL_Delay(10);
                } while (!SDL_PollEvent(nullptr) && typematic.buttonsPressed() && !typematic.update());
            } else { SDL_WaitEvent(nullptr); }
        }
        if (wait) { app.requestFrame(); }

        // event processing loop
        typematic.update();
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_KEYDOWN:
                    switch (ev.key.keysym.sym) {
                        case SDLK_LEFT:      app.handleEvent(AppEvent::Left);     break;
                        case SDLK_RIGHT:     app.handleEvent(AppEvent::Right);    break;
                        case SDLK_UP:        app.handleEvent(AppEvent::Up);       break;
                        case SDLK_DOWN:      app.handleEvent(AppEvent::Down);     break;
                        case SDLK_PAGEUP:    app.handleEvent(AppEvent::PageUp);   break;
                        case SDLK_PAGEDOWN:  app.handleEvent(AppEvent::PageDown); break;
                        case SDLK_HOME:      app.handleEvent(AppEvent::Home);     break;
                        case SDLK_END:       app.handleEvent(AppEvent::End);      break;
                        case SDLK_RETURN:
                        case SDLK_a:         app.handleEvent(AppEvent::A);        break;
                        case SDLK_BACKSPACE:
                        case SDLK_b:         app.handleEvent(AppEvent::B);        break;
                        case SDLK_SPACE:
                        case SDLK_x:         app.handleEvent(AppEvent::X);        break;
                        case SDLK_RSHIFT:
                        case SDLK_z:
                        case SDLK_y:         app.handleEvent(AppEvent::Y);        break;
                        case SDLK_TAB:       app.handleEvent(AppEvent::Select);   break;
                        case SDLK_ESCAPE:    app.handleEvent(AppEvent::Start);    break;
                        case SDLK_q:         active = false;                      break;
                        default: break;
                    }
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    switch (ev.cbutton.button) {
                        case SDL_CONTROLLER_BUTTON_A:             app.handleEvent(AppEvent::A);      break;
                        case SDL_CONTROLLER_BUTTON_B:             app.handleEvent(AppEvent::B);      break;
                        case SDL_CONTROLLER_BUTTON_X:             app.handleEvent(AppEvent::X);      break;
                        case SDL_CONTROLLER_BUTTON_Y:             app.handleEvent(AppEvent::Y);      break;
                        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  app.handleEvent(AppEvent::LS);     break;
                        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: app.handleEvent(AppEvent::RS);     break;
                        case SDL_CONTROLLER_BUTTON_BACK:          app.handleEvent(AppEvent::Select); break;
                        case SDL_CONTROLLER_BUTTON_START:         app.handleEvent(AppEvent::Start);  break;
                        case SDL_CONTROLLER_BUTTON_GUIDE:         app.handleEvent(AppEvent::Logo);   break;
                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     typematic.setState(FTSource::DPad + FTDirection::Left,  true); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    typematic.setState(FTSource::DPad + FTDirection::Right, true); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_UP:       typematic.setState(FTSource::DPad + FTDirection::Up,    true); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     typematic.setState(FTSource::DPad + FTDirection::Down,  true); break;
                        default: break;
                    }
                    break;
                case SDL_CONTROLLERBUTTONUP:
                    switch (ev.cbutton.button) {
                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     typematic.setState(FTSource::DPad + FTDirection::Left,  false); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    typematic.setState(FTSource::DPad + FTDirection::Right, false); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_UP:       typematic.setState(FTSource::DPad + FTDirection::Up,    false); break;
                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     typematic.setState(FTSource::DPad + FTDirection::Down,  false); break;
                        default: break;
                    }
                    break;
                case SDL_CONTROLLERAXISMOTION:
                    switch (ev.caxis.axis) {
                        case SDL_CONTROLLER_AXIS_LEFTX:        typematic.setAxis(FTSource::LeftStick  + FTDirection::XAxis, ev.caxis.value); break;
                        case SDL_CONTROLLER_AXIS_LEFTY:        typematic.setAxis(FTSource::LeftStick  + FTDirection::YAxis, ev.caxis.value); break;
                        case SDL_CONTROLLER_AXIS_RIGHTX:       typematic.setAxis(FTSource::RightStick + FTDirection::XAxis, ev.caxis.value); break;
                        case SDL_CONTROLLER_AXIS_RIGHTY:       typematic.setAxis(FTSource::RightStick + FTDirection::YAxis, ev.caxis.value); break;
                        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:  typematic.setState(FTSource::Trigger + FTDirection::Left,  (ev.caxis.value > AnalogSensitivity)); break;
                        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: typematic.setState(FTSource::Trigger + FTDirection::Right, (ev.caxis.value > AnalogSensitivity)); break;
                        default: break;
                    }
                    break;
                case SDL_QUIT:
                    active = false;
                    break;
                default:
                    break;
            }   // END switch (ev.type)
        }   // END while (SDL_PollEvent())

        // finally, draw the app
        Uint64 now = SDL_GetPerformanceCounter();
        app.draw(wait ? 0.0 : (double(now - prevTime) / double(SDL_GetPerformanceFrequency())));
        prevTime = now;
        SDL_GL_SwapWindow(win);
    }

    app.shutdown();
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(glctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
