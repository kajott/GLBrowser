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
    if (m_framesRequested > 0) { --m_framesRequested; }

    m_geometry.setTimeDelta(float(dt));
    if (m_dirView.animate()) { requestFrame(); }

    glClear(GL_COLOR_BUFFER_BIT);

    m_dirView.draw();

#if 0
    m_renderer.box(700, 100, 1000, 200, 0xFFC0C0C0);
    m_renderer.box(50, 100, 400, 200, 0xFF987654, 0xFFFEDCBA, 20);
    m_renderer.circle(550, 150, 50, 0xFF0369CF);
    m_renderer.box(100, 300, 300, 500, 0x80000000, 0x80000000, 30, 15.f, 15.f);
    m_renderer.outlineBox(100, 600, 1200, 700, 0xA98765, 0x876543, 0xFFFFFF, 3, 3*4, 3, 0.f, .5f);
    m_renderer.text(640, 360, 100.0f, "Hello World!", Align::Center + Align::Middle);
    m_renderer.shadowText(120, 620, 40.f, "text with shadow", 0, 0xFFFFFFFF, 0xFFFFFFFF, 2, 3.0f, 0.5f);
#endif

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
