#include <cstdio>

#include "glad.h"

#include "event.h"
#include "renderer.h"
#include "dirview.h"
#include "menu.h"
#include "file_assoc.h"
#include "sysutil.h"

#include "app.h"

namespace MenuItemID {
    constexpr int Dismiss         =  0;
    constexpr int QuitApplication = -1;
    constexpr int OpenWithDefault = -2;
    constexpr int RunExecutable   = -3;
    inline bool IsFileAssoc(int id) { return (id > 0); }
};

bool GLMenuApp::init(const char *initial) {
    glClearColor(0.125f, 0.25f, 0.375f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (!m_renderer.init()) { return false; }
    m_geometry.update(m_renderer.viewportWidth(), m_renderer.viewportHeight());
    m_dirView.navigate(initial ? initial : GetCurrentDir());
    FileAssocInit(m_argv0);
    return true;
}

void GLMenuApp::shutdown() {
    m_renderer.shutdown();
}

void GLMenuApp::draw(double dt) {
    // process animations
    if (m_framesRequested > 0) { --m_framesRequested; }
    m_geometry.setTimeDelta(float(dt));
    if (m_dirView.animate() + m_menu.animate()) { requestFrame(); }

    // clear screen and draw main views
    glClear(GL_COLOR_BUFFER_BIT);
    m_dirView.draw();
    m_menu.draw();

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
    const char* title = (m_menu.active() && !m_menu.mainTitle().empty())
                      ?  m_menu.mainTitle().c_str()
                      :  m_dirView.currentDir().c_str();
    if (!title || !title[0]) { title = "drive selection"; }
    m_renderer.text(
        std::min(float(m_geometry.outerMarginX),
                 float(m_geometry.screenWidth - m_geometry.outerMarginX)
               - float(m_geometry.textSize) * m_renderer.textWidth(title)),
        float(m_geometry.outerMarginY), float(m_geometry.textSize), title, 0);

    // draw control bar contents
    constexpr uint32_t controlBarColor = 0xFFAAAAAA;
    int x = m_geometry.outerMarginX;
    if (m_menu.active()) {
        m_menu.controls([&] (bool keyboard, const std::string& control, const std::string& label) {
            if (keyboard != m_haveController) {
                x = m_renderer.control(x, y, m_geometry.textSize, 0, keyboard, control.c_str(), label.c_str(), controlBarColor, barBackOpaque);
            }
        });
    } else if (m_haveController) {
        x = m_renderer.control(x, y, m_geometry.textSize, 0, false, "A", "Select", controlBarColor, barBackOpaque);
        if (!m_dirView.atRoot()) { x = m_renderer.control(x, y, m_geometry.textSize, 0, false, "B", "Parent Directory", controlBarColor, barBackOpaque); }
        x = m_renderer.control(x, y, m_geometry.textSize, 0, false, "X", "Open With", controlBarColor, barBackOpaque);
        x = m_renderer.control(x, y, m_geometry.textSize, 0, false, "START", "Menu", controlBarColor, barBackOpaque);
    } else {
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Enter", "Select", controlBarColor, barBackOpaque);
        if (!m_dirView.atRoot()) { x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Backspace", "Parent Directory", controlBarColor, barBackOpaque); }
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Space", "Open With", controlBarColor, barBackOpaque);
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Q", "Quit", controlBarColor, barBackOpaque);
    }

    m_renderer.flush();
}

void GLMenuApp::showMainMenu() {
    m_menu.clear();
    m_menu.setBoxTitle("Main Menu");
    m_menu.addItem(MenuItemID::QuitApplication, "Quit");
    m_menu.addSeparator();
    m_menu.addItem(0, "Cancel");
    m_menu.activate();
    m_dirView.deactivate();
}

void GLMenuApp::showOpenWithMenu() {
    m_menu.clear();
    m_menu.setMainTitle(m_dirView.currentItemFullPath());
    m_menu.setBoxTitle("Open With");
    if (m_dirView.currentItem().isExec) {
        m_menu.addItem(MenuItemID::RunExecutable, "Run");
    }
    m_menu.addSeparator();
    FileAssocLookup(m_dirView.currentItem().extCode, [&] (const FileAssociation& assoc) -> bool {
        m_menu.addItem(assoc.index, assoc.displayName);
        return true;
    });
    m_menu.addSeparator();
    m_menu.addItem(MenuItemID::OpenWithDefault, "System Default");
    m_menu.addSeparator();
    m_menu.addItem(0, "Cancel");
    m_menu.avoidCurrentItem(m_dirView);
    m_menu.activate();
    m_dirView.deactivate();
}

void GLMenuApp::handleEvent(AppEvent ev) {
    // handle modal menu's events first
    ModalMenu::EventType me = m_menu.handleEvent(ev);
    if (me != ModalMenu::EventType::Inactive) {
        if (me == ModalMenu::EventType::Confirm) {
            switch (m_menu.result()) {
                case MenuItemID::QuitApplication: m_active = false; break;
                default: break;
            }
        }
        if (!m_menu.active()) {
            m_dirView.activate();
        }
        return;
    }

    // handle main directory view events
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
            if (m_dirView.currentItem().isDir) {
                m_dirView.push();
            }  // else: open with default application (TODO)
            break;
        case AppEvent::Start:   showMainMenu(); break;
        case AppEvent::X:       showOpenWithMenu(); break;
        default: break;
    }
}
