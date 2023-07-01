#include <cstdio>

#include "glad.h"

#include "app.h"

bool GLMenuApp::init() {
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    return m_renderer.init();
}

void GLMenuApp::shutdown() {
    m_renderer.shutdown();
}

void GLMenuApp::draw(double t) {
    if (m_framesRequested > 0) { --m_framesRequested; }

    glClear(GL_COLOR_BUFFER_BIT);

    m_renderer.box(100, 200, 700, 400, 0xFF987654, 0xFFFEDCBA);
    m_renderer.flush();
}

void GLMenuApp::handleEvent(AppEvent ev) {
printf("EVENT:%d\n", static_cast<int>(ev));
    switch (ev) {
        default:
            break;
    }
}
