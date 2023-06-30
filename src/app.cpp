#include <cstdio>

#include "glad.h"

#include "app.h"

bool GLMenuApp::init() {
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    return true;
}

void GLMenuApp::shutdown() {
}

void GLMenuApp::draw() {
    printf("--draw--\n");
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLMenuApp::handleEvent(AppEvent ev) {
printf("EVENT:%d\n", static_cast<int>(ev));
    switch (ev) {
        default:
            break;
    }
}
