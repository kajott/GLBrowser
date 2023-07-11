// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>

#include "event.h"
#include "renderer.h"
#include "dirview.h"
#include "menu.h"

class GLBrowserApp {
    std::function<void(AppAction action)> m_actionCallback;
    const char* m_argv0;
    int m_framesRequested = 1;
    bool m_haveController = false;
    TextBoxRenderer m_renderer;
    Geometry m_geometry;
    DirView m_dirView;
    ModalMenu m_menu;

    void runProgramWrapper(const char* program=nullptr, const char* argument=nullptr);
    void itemSelected();
    void showMainMenu();
    void showOpenWithMenu();

public:
    explicit inline GLBrowserApp(std::function<void(AppAction action)> actionCallback, const char *argv0=nullptr)
        : m_actionCallback(actionCallback), m_argv0(argv0)
        , m_dirView(m_renderer, m_geometry)
        , m_menu   (m_renderer, m_geometry) {}

    inline void haveController() { m_haveController = true; }

    bool init(const char* initial);
    void shutdown();
    void draw(double dt);
    void handleEvent(AppEvent ev);
    inline int framesRequested() const { return m_framesRequested; }
    inline void requestFrame(int frames=1) { if (frames > m_framesRequested) { m_framesRequested = frames; } }
};
