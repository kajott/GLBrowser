// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstring>

#include "glad.h"

#include "event.h"
#include "renderer.h"
#include "dirview.h"
#include "menu.h"
#include "file_assoc.h"
#include "sysutil.h"

#include "app.h"

constexpr const char* favFileName = "glbrowser.fav";

namespace MenuItemID {
    constexpr int Dismiss         =  0;
    constexpr int QuitApplication = -1;
    constexpr int OpenWithDefault = -2;
    constexpr int RunExecutable   = -3;
    constexpr int ShowFavMenu     = -4;
    constexpr int AddFav          = -5;
    constexpr int FavBase         = 0x10000;
    constexpr int FavMask         = 0xF0000;
    inline bool IsFileAssoc(int id) { return (id > 0) && (id <  FavBase); }
    inline bool IsFav(int id)       { return             (id >= FavBase); }
    inline int  GetFav(int id)      { return              id -  FavBase;  }
};

bool GLBrowserApp::isValidFavID(int id) {
    return MenuItemID::IsFav(id) && (MenuItemID::GetFav(id) < int(m_favs.size()));
}

bool GLBrowserApp::init(const char *initial) {
    glClearColor(0.125f, 0.25f, 0.375f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (!m_renderer.init()) { return false; }
    m_geometry.update(m_renderer.viewportWidth(), m_renderer.viewportHeight());
    m_dirView.navigate(initial ? initial : GetCurrentDir());
    FileAssocInit(m_argv0);
    m_favFile = PathJoin(GetConfigDir(), favFileName);
    return true;
}

void GLBrowserApp::shutdown() {
    m_renderer.shutdown();
}

void GLBrowserApp::draw(double dt) {
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
        x = m_renderer.control(x, y, m_geometry.textSize, 0, false, "Y", "Favorites", controlBarColor, barBackOpaque);
        x = m_renderer.control(x, y, m_geometry.textSize, 0, false, "START", "Menu", controlBarColor, barBackOpaque);
    } else {
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Enter", "Select", controlBarColor, barBackOpaque);
        if (!m_dirView.atRoot()) { x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Backspace", "Parent Directory", controlBarColor, barBackOpaque); }
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Space", "Open With", controlBarColor, barBackOpaque);
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "RShift", "Favorites", controlBarColor, barBackOpaque);
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Esc", "Menu", controlBarColor, barBackOpaque);
        x = m_renderer.control(x, y, m_geometry.textSize, 0, true, "Q", "Quit", controlBarColor, barBackOpaque);
    }

    m_renderer.flush();
}

void GLBrowserApp::loadFavs() {
    m_favs.clear();
    FILE *f = fopen(m_favFile.c_str(), "r");
    if (!f) { return; }
    constexpr int lineBufferSize = 128;
    char buffer[lineBufferSize];
    std::string line;
    auto flushLine = [&] () {
        // strip trailing whitespace and comments
        int validLen = 0;
        for (int i = 0;  i < int(line.size());  ++i) {
            if (line[i] == '#') { break; }  // comment
            if (!my_isspace(line[i])) { validLen = i + 1; }
        }
        line.resize(validLen);
        // add line (if not empty) and start over with a new one
        if (!line.empty()) { m_favs.emplace_back(line); }
        line.clear();
    };
    while (fgets(buffer, lineBufferSize, f) != nullptr) {
        char *bufPos = buffer;
        if (line.empty()) {  // strip leading whitespace (but at line start only!)
            while (*bufPos && my_isspace(*bufPos)) { ++bufPos; }
        }
        line.append(bufPos);
        int len = int(strlen(bufPos));
        if ((len > 0) && (bufPos[len-1] == '\n')) {
            flushLine();
        }
    }
    flushLine();
    fclose(f);
}

void GLBrowserApp::saveFavs() {
    FILE *f = fopen(m_favFile.c_str(), "w");
    if (!f) { return; }
    fprintf(f, "# GLBrowser favorites file\n");
    for (const auto& fav : m_favs) {
        fprintf(f, "%s\n", fav.c_str());
    }
    fclose(f);
}

void GLBrowserApp::addFav() {
    std::string newFav = m_dirView.currentItemFullPath();
    for (const auto& fav : m_favs) {
        if (fav == newFav) { return; }
    }
    m_favs.emplace_back(newFav);
}

void GLBrowserApp::showMainMenu() {
    m_menu.clear();
    m_menu.setBoxTitle("Main Menu");
    m_menu.addItem(MenuItemID::QuitApplication, "Quit");
    m_menu.addSeparator();
    m_menu.addItem(MenuItemID::ShowFavMenu, "Favorites");
    m_menu.addSeparator();
    m_menu.addItem(0, "Cancel");
    m_menu.activate();
    m_dirView.deactivate();
}

void GLBrowserApp::showOpenWithMenu() {
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

void GLBrowserApp::showFavMenu() {
    loadFavs();
    std::string path = m_dirView.currentItemFullPath();
    m_menu.clear();
    m_menu.setMainTitle(path);
    m_menu.setBoxTitle("Favorites");
    int selectID = 0;
    for (int i = 0;  i < int(m_favs.size());  ++i) {
        m_menu.addItem(MenuItemID::FavBase + i, m_favs[i]);
        if (m_favs[i] == path) { selectID = i; }
    }
    m_menu.addSeparator();
    m_menu.addItem(MenuItemID::AddFav, "Add Favorite");
    m_menu.addSeparator();
    m_menu.addItem(0, "Cancel");
    m_menu.addControl(true, "Space", "Remove Favorite", MenuItemID::FavMask, MenuItemID::FavBase);
    m_menu.addControl(false, "X",    "Remove Favorite", MenuItemID::FavMask, MenuItemID::FavBase);
    m_menu.activate(MenuItemID::FavBase + selectID);
    m_dirView.deactivate();
}

void GLBrowserApp::itemSelected() {
    if (m_dirView.currentItem().isDir) {
        m_dirView.push();
        return;
    }
    int assocIndex = 0;
    FileAssocLookup(m_dirView.currentItem().extCode, [&] (const FileAssociation& assoc) -> bool {
        assocIndex = assoc.index;
        return false;
    });
    const FileAssociation& assoc = GetFileAssoc(assocIndex);
    if (m_dirView.currentItem().isExec && assoc.allowExec) {
        runProgramWrapper(m_dirView.currentItemFullPath().c_str());
    } else {
        runProgramWrapper(assoc.executablePath.c_str(),
                          m_dirView.currentItemFullPath().c_str());
    }
}

void GLBrowserApp::runProgramWrapper(const char* program, const char* argument) {
    m_actionCallback(AppAction::Minimize);
    RunProgram(program, argument);
    m_actionCallback(AppAction::Restore);
}

void GLBrowserApp::handleEvent(AppEvent ev) {
    // handle modal menu events first
    ModalMenu::EventType me = m_menu.handleEvent(ev);
    if (me != ModalMenu::EventType::Inactive) {
        if (me == ModalMenu::EventType::Confirm) {
            switch (m_menu.result()) {
                case MenuItemID::QuitApplication: m_actionCallback(AppAction::Quit); break;
                case MenuItemID::RunExecutable:   runProgramWrapper(m_dirView.currentItemFullPath().c_str()); break;
                case MenuItemID::OpenWithDefault: runProgramWrapper(nullptr, m_dirView.currentItemFullPath().c_str()); break;
                case MenuItemID::ShowFavMenu:     showFavMenu(); break;
                case MenuItemID::AddFav:          addFav(); saveFavs(); showFavMenu(); break;
                default:
                    if (MenuItemID::IsFileAssoc(m_menu.result())) {
                        runProgramWrapper(GetFileAssoc(m_menu.result()).executablePath.c_str(),
                                          m_dirView.currentItemFullPath().c_str());
                    } else if (isValidFavID(m_menu.result())) {
                        m_dirView.navigate(m_favs[MenuItemID::GetFav(m_menu.result())]);
                    }
                    break;
            }
        } else if ((ev == AppEvent::X) && isValidFavID(m_menu.result())) {
            m_favs.erase(m_favs.begin() + MenuItemID::GetFav(m_menu.result()));
            saveFavs(); showFavMenu();
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
        case AppEvent::LT:
        case AppEvent::PageUp:   m_dirView.moveCursor(-m_geometry.itemsPerPage, true); break;
        case AppEvent::RT:
        case AppEvent::PageDown: m_dirView.moveCursor(+m_geometry.itemsPerPage, true); break;
        case AppEvent::Home:     m_dirView.moveCursor(0, false); break;
        case AppEvent::End:      m_dirView.moveCursor(999999, false); break;
        case AppEvent::B:
        case AppEvent::LS:       m_dirView.pop(); break;
        case AppEvent::RS:       m_dirView.push(); break;
        case AppEvent::A:        itemSelected(); break;
        case AppEvent::X:        showOpenWithMenu(); break;
        case AppEvent::Y:        showFavMenu(); break;
        case AppEvent::Start:    showMainMenu(); break;
        default: break;
    }
}
