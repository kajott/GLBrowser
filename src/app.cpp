#include <cstdio>

#include "glad.h"

#include "app.h"

bool GLMenuApp::init() {
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return m_renderer.init();
}

void GLMenuApp::shutdown() {
    m_renderer.shutdown();
}

void GLMenuApp::draw(double t) {
    if (m_framesRequested > 0) { --m_framesRequested; }

    glClear(GL_COLOR_BUFFER_BIT);

    m_renderer.box(700, 100, 1000, 200, 0xFFC0C0C0);
    m_renderer.box(50, 100, 400, 200, 0xFF987654, 0xFFFEDCBA, 20);
    m_renderer.circle(550, 150, 50, 0xFF0369CF);
    m_renderer.box(100, 300, 300, 500, 0x80000000, 0x80000000, 30, 15.f, 15.f);
    m_renderer.contourBox(100, 600, 1200, 700, 0xA98765, 0x876543, 0xFFFFFF, 3, 3*4, 3, 0.f, .5f);
    m_renderer.flush();
}

void GLMenuApp::handleEvent(AppEvent ev) {
printf("EVENT:%d\n", static_cast<int>(ev));
    switch (ev) {
        default:
            break;
    }
}
