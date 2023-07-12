// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cmath>

#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#include "renderer.h"
#include "geometry.h"
#include "event.h"

#include "menu.h"

void ModalMenu::clear() {
    m_mainTitle.clear();
    m_boxTitle.clear();
    m_items.clear();
    m_controls.clear();
    m_active = m_layoutFinished = false;
    m_confirmed = m_dismissed = false;
}

void ModalMenu::addItem(int id, const std::string& text) {
    if (m_layoutFinished) { return; }
    m_items.push_back(MenuItem(id, text));
}

void ModalMenu::addSeparator() {
    if (m_layoutFinished || m_items.empty()) { return; }
    m_items.back().separatorFollows = true;
}

void ModalMenu::setBoxTitle(const std::string& title) {
    if (m_layoutFinished) { return; }
    m_boxTitle = title;
}

void ModalMenu::addControl(bool keyboard, const std::string& control, const std::string& label, int enableMask, int enableRef) {
    m_controls.push_back(ControlItem(keyboard, control, label, enableMask, enableRef));
}

void ModalMenu::finishLayout() {
    if (m_layoutFinished) { return; }

    // determine box size
    if (m_boxTitle.empty()) {
        m_width = m_height = 0;
    } else {
        m_boxTitleTextX = m_renderer.textWidth(m_boxTitle.c_str()) * float(m_geometry.textSize);
        m_width = int(std::ceil(m_boxTitleTextX));
        m_height = m_geometry.itemHeight + m_geometry.itemSeparatorDistance;
    }
    m_targetY0 = m_height;
    for (auto& item : m_items) {
        item.y = m_height;
        item.textX = m_renderer.textWidth(item.text.c_str()) * float(m_geometry.textSize);
        m_width = std::max(m_width, int(std::ceil(item.textX)));
        m_height += m_geometry.itemHeight;
        if (item.separatorFollows) { m_height += m_geometry.itemSeparatorDistance; }
    }

    // position on screen
    m_x0 = (m_geometry.screenWidth  - m_width)  >> 1;
    m_y0 = (m_geometry.screenHeight - m_height) >> 1;
    m_layoutFinished = true;
}

void ModalMenu::avoid(int y, int x0, int x1) {
    if (m_active) { return; }
    finishLayout();
    int farMargin  = m_geometry.itemMarginX + m_geometry.panelMarginX;
    int nearMargin = farMargin + m_geometry.contextMenuDistance;
    int allMargin  = nearMargin + farMargin + m_geometry.outerMarginX;
    // try to fit the menu to the right, or to the left if that fails
    if ((m_geometry.screenWidth - m_width - x1 - allMargin) >= 0) { m_x0 = x1 + nearMargin; }
    else if                               ((x0 - allMargin) >= 0) { m_x0 = x0 - nearMargin - m_width; }
    else { return;  /* fits neither left nor right -> keep at screen center */ }
    m_y0 = std::min(std::max(y - m_targetY0, m_geometry.dirViewY0), m_geometry.dirViewY1 - m_height);
}

void ModalMenu::avoidCurrentItem(const DirView& dirView) {
    avoid(dirView.currentPanel().cursorY(),
          dirView.currentPanel().startX() - dirView.xScroll(),
          dirView.currentPanel().endX()   - dirView.xScroll());
}

void ModalMenu::setCursor(int pos) {
    m_cursor = std::min(std::max(pos, 0), int(m_items.size()) - 1);
    m_resultID = m_items[m_cursor].id;
}

void ModalMenu::activate() {
    if (m_active) { return; }
    finishLayout();
    setCursor(0);
    m_animCursorY = float(m_items[m_cursor].y + m_y0);
    float cx = float(m_x0) + 0.5f * float(m_width);
    m_boxTitleTextX = cx - 0.5f * m_boxTitleTextX;
    for (auto& item : m_items) {
        item.textX = cx - 0.5f * item.textX;
    }
    m_active = true;
}

int ModalMenu::animate() {
    if (!m_active) { return 0; }
    return m_geometry.animUpdate(m_animCursorY, float(m_items[m_cursor].y + m_y0));
}

void ModalMenu::draw() {
    if (!m_active) { return; }

    m_renderer.outlineBox(
        m_x0            - m_geometry.itemMarginX - m_geometry.panelMarginX - m_geometry.itemOutlineOffset,
        m_y0                                     - m_geometry.panelMarginY - m_geometry.itemOutlineOffset,
        m_x0 + m_width  + m_geometry.itemMarginX + m_geometry.panelMarginX + m_geometry.itemOutlineOffset,
        m_y0 + m_height                          + m_geometry.panelMarginY + m_geometry.itemOutlineOffset,
        0x554433, 0x554433, 0xFF987654, m_geometry.itemOutlineWidth,
        m_geometry.itemBorderRadius + m_geometry.panelMarginX,
        m_geometry.itemShadowOffset, float(m_geometry.menuBoxShadowSize), 0.5f, m_geometry.menuBoxShadowSize);

    if (!m_boxTitle.empty()) {
        m_renderer.outlineBox(
            m_x0 - m_geometry.itemMarginX - m_geometry.itemOutlineOffset,
            m_y0 - m_geometry.itemOutlineOffset,
            m_x0 + m_width + m_geometry.itemMarginX + m_geometry.itemOutlineOffset,
            m_y0 + m_geometry.itemHeight + m_geometry.itemOutlineOffset,
            0x404040, 0x404040, 0xFF808080, -m_geometry.itemOutlineWidth,
            m_geometry.itemBorderRadius, m_geometry.itemShadowOffset, 0.0f, 0.125f);
        m_renderer.text(
            m_boxTitleTextX, float(m_y0 + m_geometry.itemMarginY),
            float(m_geometry.textSize), m_boxTitle.c_str());
    }

    m_renderer.outlineBox(
        m_x0           - m_geometry.itemMarginX           - m_geometry.itemOutlineOffset,
        int(m_animCursorY + 0.5f)                         - m_geometry.itemOutlineOffset,
        m_x0 + m_width + m_geometry.itemMarginX           + m_geometry.itemOutlineOffset,
        int(m_animCursorY + 0.5f) + m_geometry.itemHeight + m_geometry.itemOutlineOffset,
        0xA98765, 0x876543, 0xFFFFFFFF, -m_geometry.itemOutlineWidth,
        m_geometry.itemBorderRadius, m_geometry.itemShadowOffset, 0.0f, 0.125f);

    for (auto& item : m_items) {
        m_renderer.text(
            item.textX, float(m_y0 + item.y + m_geometry.itemMarginY),
            float(m_geometry.textSize), item.text.c_str());
    }
}

void ModalMenu::controls(std::function<void(bool keyboard, const std::string& control, const std::string& label)> callback) {
    if (!m_active) { return; }
    callback(false, "A", "Select");  callback(true, "Enter", "Select");
    callback(false, "B", "Cancel");  callback(true, "Esc",   "Cancel");
    for (const auto& control : m_controls) {
        if ((m_resultID & control.enableMask) == control.enableRef) {
            callback(control.keyboard, control.control, control.label);
        }
    }
}

ModalMenu::EventType ModalMenu::handleEvent(AppEvent ev) {
    if (!m_active) { return EventType::Inactive; }
    switch (ev) {
        case AppEvent::Up:       setCursor(m_cursor - 1); return EventType::Cursor;
        case AppEvent::Down:     setCursor(m_cursor + 1); return EventType::Cursor;
        case AppEvent::LT:
        case AppEvent::PageUp:
        case AppEvent::Home:     setCursor(0); return EventType::Cursor;
        case AppEvent::RT:
        case AppEvent::PageDown:
        case AppEvent::End:      setCursor(9999); return EventType::Cursor;
        case AppEvent::A:        m_confirmed = true; m_active = false; return EventType::Confirm;
        case AppEvent::B:
        case AppEvent::Start:    m_dismissed = true; m_active = false; return EventType::Dismiss;
        default:                 return EventType::Other;
    }
}
