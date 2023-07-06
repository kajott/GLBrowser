#include <cstdio>

#include "glad.h"

#include "app.h"

bool GLMenuApp::init() {
    glClearColor(0.125f, 0.25f, 0.375f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (!m_renderer.init()) { return false; }
    m_geometry.update(m_renderer.viewportWidth(), m_renderer.viewportHeight());
    m_dirView.navigate("");
    return true;
}

void GLMenuApp::shutdown() {
    m_renderer.shutdown();
}

void GLMenuApp::draw(double dt) {
    // process animations
    if (m_framesRequested > 0) { --m_framesRequested; }
    m_geometry.setTimeDelta(float(dt));
    if (m_dirView.animate()) { requestFrame(); }

    // clear screen and draw main view
    glClear(GL_COLOR_BUFFER_BIT);
    m_dirView.draw();

    // draw title and status bar background
    constexpr uint32_t barBackTrans = 0x404040;
    constexpr uint32_t barBackOpaque = barBackTrans | 0xFF000000;
    int y = 2 * m_geometry.outerMarginY + m_geometry.textSize;
    m_renderer.box(0, 0, m_geometry.screenWidth, y, barBackOpaque);
    m_renderer.box(0, y, m_geometry.screenWidth, y + m_geometry.gradientHeight, barBackOpaque, barBackTrans);
    y = m_geometry.screenHeight - y;
    m_renderer.box(0, y - m_geometry.gradientHeight, m_geometry.screenWidth, y, barBackTrans, barBackOpaque);
    m_renderer.box(0, y, m_geometry.screenWidth, m_geometry.screenHeight, barBackOpaque);
    y += m_geometry.outerMarginY;  // move to upper end of controls line, used below

    // draw title contents
    const char* title = m_dirView.path().c_str();
    if (!title || !title[0]) {
        #ifdef _WIN32
            title = "drive selection";
        #else
            title = "root directory";
        #endif
    }
    m_renderer.text(
        std::min(float(m_geometry.outerMarginX),
                 float(m_geometry.screenWidth - m_geometry.outerMarginX)
               - float(m_geometry.textSize) * m_renderer.textWidth(title)),
        float(m_geometry.outerMarginY), float(m_geometry.textSize), title, 0);

    // draw control bar contents
    constexpr uint32_t controlBarColor = 0xFFAAAAAA;
    int x = m_geometry.outerMarginX;
    if (m_haveController) {
        x = m_renderer.control(x, y, m_geometry.textSize, 0, false, "A", "Select", controlBarColor, barBackOpaque);
        if (!m_dirView.path().empty()) { x = m_renderer.control(x, y, m_geometry.textSize, 0, false, "B", "Parent Directory", controlBarColor, barBackOpaque); }
        //x = m_renderer.control(x, y, m_geometry.textSize, 0, false, "START", "Menu", controlBarColor, barBackOpaque);
    } else {
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Enter", "Select", controlBarColor, barBackOpaque);
        if (!m_dirView.path().empty()) { x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Backspace", "Parent Directory", controlBarColor, barBackOpaque); }
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Q", "Quit", controlBarColor, barBackOpaque);
    }

    m_renderer.flush();
}

void GLMenuApp::handleEvent(AppEvent ev) {
    switch (ev) {
        case AppEvent::Up:       m_dirView.moveCursor(-1, true); break;
        case AppEvent::Down:     m_dirView.moveCursor(+1, true); break;
        case AppEvent::PageUp:   m_dirView.moveCursor(-m_geometry.itemsPerPage, true); break;
        case AppEvent::PageDown: m_dirView.moveCursor(+m_geometry.itemsPerPage, true); break;
        case AppEvent::Home:     m_dirView.moveCursor(0, false); break;
        case AppEvent::End:      m_dirView.moveCursor(999999, false); break;
        case AppEvent::B:        m_dirView.pop(); break;
        case AppEvent::LS:       m_dirView.pop(); break;
        case AppEvent::RS:       m_dirView.push(); break;
        case AppEvent::A:
            if (m_dirView.currentItem().isdir) {
                m_dirView.push();
            }  // else: pop up file dialog (TODO)
            break;
        default: break;
    }
}
