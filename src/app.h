#pragma once

#include "renderer.h"
#include "dirview.h"


enum class AppEvent {
    Left, Right, Up, Down,
    PageUp, PageDown, Home, End,
    A, B, X, Y,
    RS, LS, RT, LT,
    Select, Start, Logo
};


class GLMenuApp {
    bool m_active = true;
    int m_framesRequested = 1;
    bool m_haveController = false;
    TextBoxRenderer m_renderer;
    Geometry m_geometry;
    DirView m_dirView;

public:
    inline GLMenuApp() : m_dirView(m_renderer, m_geometry) {}

    inline void haveController() { m_haveController = true; }

    bool init(const char* initial);
    void shutdown();
    void draw(double dt);
    void handleEvent(AppEvent ev);
    inline void quit() { m_active = false; }
    inline bool active() const { return m_active; }
    inline int framesRequested() const { return m_framesRequested; }
    inline void requestFrame(int frames=1) { if (frames > m_framesRequested) { m_framesRequested = frames; } }
};
