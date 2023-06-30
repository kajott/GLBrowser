#pragma once


enum class AppEvent {
    Left, Right, Up, Down,
    A, B, X, Y,
    RS, LS, RT, LT,
    Select, Start, Logo
};


class GLMenuApp {
    bool m_active = true;

public:
    bool init();
    void shutdown();
    void draw();
    void handleEvent(AppEvent ev);
    inline void quit()   { m_active = false; }
    inline bool active() { return m_active; }
};
