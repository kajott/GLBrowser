#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <cmath>

#include <string>
#include <vector>
#include <algorithm>

#include "renderer.h"
#include "dirview.h"

///////////////////////////////////////////////////////////////////////////////

bool DirItem::operator< (const DirItem& other) const {
    if (isdir && !other.isdir) { return true; }
    if (other.isdir && !isdir) { return false; }
    // case-insensitive string comparison; rolled fully by hand because
    // (a) C++ doesn't have that,
    // (b) neither does C (at least not universally available),
    // (c) MSVC's toupper()/tolower() implementations aren't even 8-bit safe
    const char* s1 = name.c_str();
    const char* s2 = other.name.c_str();
    while (*s1 && *s2) {
        uint8_t c1 = uint8_t(*s1++);
        uint8_t c2 = uint8_t(*s2++);
        if ((c1 >= 'a') && (c1 <= 'z')) { c1 -= 'a' - 'A'; }
        if ((c2 >= 'a') && (c2 <= 'z')) { c2 -= 'a' - 'A'; }
        if (c1 < c2) { return true; }
        if (c1 > c2) { return false; }
    }
    return (*s2 != 0);
}

///////////////////////////////////////////////////////////////////////////////

DirPanel::DirPanel(DirView& parent, const std::string& path, int x0, bool active)
    : m_parent(parent), m_geometry(parent.m_geometry), m_path(path), m_active(active), m_cursor(0), m_x0(x0)
{
    float w = 0.0f;
    bool isSubdir = !path.empty();
    if (isSubdir) { m_items.push_back(DirItem("", true, "\xC2\xAB back")); }

    int maxlen = (rand() % 32) + 2;
    for (int i = (rand() % 42);  i;  --i) {
        char name[64], *npos = name;
        static const char *gamut = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz +-_";
        for (int j = (rand() % maxlen) + 7;  j;  --j) {
            *npos++ = gamut[rand() % strlen(gamut)];
        }
        *npos = '\0';
        m_items.push_back(DirItem(name, ((rand() & 1) == 0)));
        w = std::max(w, m_parent.m_renderer.textWidth(m_items.back().displayText().c_str()));
    }

    std::sort(m_items.begin() + (isSubdir ? 1 : 0), m_items.end());

    m_width = 2 * m_geometry.panelMarginX
            + 2 * m_geometry.itemMarginX
            + int(std::ceil(w * float(m_geometry.textSize)));
    m_y0 = m_geometry.dirViewY0;
    m_animY0 = float(m_y0);
    m_animActive = (m_active ? 1.0f : 0.0f);
    m_animCursorY = 0.0f;
}

void DirPanel::draw(float xOffset) {
    float x = xOffset + float(m_x0 + m_geometry.panelMarginX + m_geometry.itemMarginX);
    int ix = int(std::floor(x + 0.5f));

    if (m_active) {
        int iy = int(std::floor(m_animY0 + m_animCursorY + 0.5f));
        m_parent.m_renderer.outlineBox(
            ix - m_geometry.itemMarginX - m_geometry.itemOutlineOffset,
            iy - m_geometry.itemOutlineOffset,
            ix - m_geometry.itemMarginX + m_width - 2 * m_geometry.panelMarginX + m_geometry.itemOutlineOffset,
            iy + m_geometry.itemHeight + m_geometry.itemOutlineOffset,
            0xA98765, 0x876543, 0xFFFFFFFF, -m_geometry.itemOutlineWidth,
            m_geometry.itemBorderRadius, m_geometry.itemShadowOffset, 0.0f, 0.125f);
    }

    for (int i = 0;  i < int(m_items.size());  ++i) {
        float y = m_animY0 + float(i * m_geometry.itemHeight + m_geometry.itemMarginY);
        float alpha = m_animActive + (1.0f - m_animActive) * ((i == m_cursor) ? 0.75f : 0.25f);
        m_parent.m_renderer.text(x, y, float(m_geometry.textSize),
            m_items[i].displayText().c_str(),
            Align::Left + Align::Top,
            TextBoxRenderer::makeAlpha(alpha) | 0xFFFFFF);
    }
}

void DirPanel::moveCursor(int target, bool relative) {
    if (relative) { target += m_cursor; }
    target = std::min(std::max(0, target), int(m_items.size()) - 1);
    m_cursor = target;
    int cursorScreenY = m_y0 + m_cursor * m_geometry.itemHeight;
    m_y0 += std::max(0, m_geometry.dirViewY0 - cursorScreenY)
          - std::max(0, cursorScreenY + m_geometry.itemHeight - m_geometry.dirViewY1);
}

int DirPanel::animate() {
    return m_geometry.animUpdate(m_animY0,      float(m_y0))
         + m_geometry.animUpdate(m_animActive,  m_active ? 1.f : 0.f)
         + m_geometry.animUpdate(m_animCursorY, float(m_cursor * m_geometry.itemHeight));
}

///////////////////////////////////////////////////////////////////////////////

void DirView::navigate(const std::string& path) {
    m_panels.clear();
    m_panels.push_back(DirPanel(*this, path, 0));
    m_xScroll = -m_geometry.outerMarginX;
    updateScroll();
    m_animXOffset = float(-m_xScroll);
}

void DirView::updateScroll() {
    // compute xScroll value for when the current panel is maximally left- and right-aligned
    int scrollL = m_panels.back().startX() - m_geometry.outerMarginX;
    int scrollR = m_panels.back().endX() - m_geometry.screenWidth + m_geometry.outerMarginX;
    // if we need to scroll left, try to center the current panel first
    if (m_xScroll > scrollL) {
        m_xScroll = (m_panels.back().startX() + m_panels.back().endX() - m_geometry.screenWidth) >> 1;
    }
    // if we still need to fit things, fit to right then left
    // (so that panels which are wider than the screen are shown left-aligned)
    if (m_xScroll < scrollR) { m_xScroll = scrollR; }
    if (m_xScroll > scrollL) { m_xScroll = scrollL; }
}

int DirView::animate() {
    int res = m_geometry.animUpdate(m_animXOffset, float(-m_xScroll));
    for (auto& panel : m_panels) {
        res += panel.animate();
    }
    return res;
}

void DirView::draw() {
    for (auto& panel : m_panels) {
        panel.draw(m_animXOffset);
    }
}

void DirView::moveCursor(int target, bool relative) {
    if (m_panels.empty()) { return; }
    m_panels.back().moveCursor(target, relative);
}

void DirView::push() {
    const DirItem& current = currentItem();
    if (!current.isdir) { return; }
    if (current.name.empty()) { pop(); return; }
    m_panels.back().deactivate();
    m_panels.push_back(DirPanel(*this, "dummy", m_panels.back().endX()));
    updateScroll();
}

void DirView::pop() {
    if (m_panels.size() <= 1) { return; }
    m_panels.pop_back();
    m_panels.back().activate();
    updateScroll();
}
