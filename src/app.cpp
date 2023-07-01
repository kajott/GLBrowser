#include <cstdio>

#include "glad.h"

#include "app.h"

bool GLMenuApp::init() {
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    return true;
}

void GLMenuApp::shutdown() {
}

void GLMenuApp::draw(double t) {
    if (m_framesRequested > 0) { --m_framesRequested; }
    printf("-- draw t=%.2f --\n", t);

    glClear(GL_COLOR_BUFFER_BIT);
}

void GLMenuApp::handleEvent(AppEvent ev) {
printf("EVENT:%d\n", static_cast<int>(ev));
    switch (ev) {
        default:
            break;
    }
}
