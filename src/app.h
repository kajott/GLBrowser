#pragma once

#include "renderer.h"


enum class AppEvent {
    Left, Right, Up, Down,
    A, B, X, Y,
    RS, LS, RT, LT,
    Select, Start, Logo
};


class GLMenuApp {
    bool m_active = true;
    int m_framesRequested = 1;
    TextBoxRenderer m_renderer;

public:
    inline GLMenuApp() {}

    bool init();
    void shutdown();
    void draw(double t);
    void handleEvent(AppEvent ev);
    inline void quit() { m_active = false; }
    inline bool active() const { return m_active; }
    inline int framesRequested() const { return m_framesRequested; }
    inline void requestFrame(int frames=1) { if (frames > m_framesRequested) { m_framesRequested = frames; } }
};
