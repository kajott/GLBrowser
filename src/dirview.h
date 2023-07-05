#pragma once

#include <string>
#include <vector>

#include "renderer.h"
#include "geometry.h"

class DirView;

///////////////////////////////////////////////////////////////////////////////

struct DirItem {
    std::string name;
    bool isdir;
    std::string display;
    bool operator< (const DirItem& other) const;
    inline const std::string& displayText() const { return display.empty() ? name : display; }
    inline DirItem(const std::string& name_, bool isdir_) : name(name_), isdir(isdir_), display(isdir_ ? (name_ + " \xC2\xBB") : "") {}
    inline DirItem(const std::string& name_, bool isdir_, const std::string& display_) : name(name_), isdir(isdir_), display(display_) {}
};

///////////////////////////////////////////////////////////////////////////////

class DirPanel {
    DirView& m_parent;
    const Geometry& m_geometry;
    std::string m_path;
    std::vector<DirItem> m_items;
    bool m_active;
    int m_cursor;
    int m_x0;
    int m_width;
    int m_y0;
    float m_animY0;
    float m_animActive;
    float m_animCursorY;

public:
    explicit DirPanel(DirView& parent, const std::string& path, int x0, bool active=true);

    int startX()                 const { return m_x0; }
    int endX()                   const { return m_x0 + m_width; }
    const std::string& path()    const { return m_path; }
    const DirItem& currentItem() const { return m_items[m_cursor]; }
    void deactivate()                  { m_active = false; }
    void activate()                    { m_active = true; }

    int animate();
    void draw(float xOffset=0.0f);
    void moveCursor(int target, bool relative);
};

///////////////////////////////////////////////////////////////////////////////

class DirView {
    friend class DirPanel;

protected:
    TextBoxRenderer& m_renderer;
    const Geometry& m_geometry;

    std::vector<DirPanel> m_panels;

    int m_xScroll = 0;
    float m_animXOffset = 0.0f;
    void updateScroll();

public:
    inline DirView(TextBoxRenderer& renderer, const Geometry& geometry)
        : m_renderer(renderer), m_geometry(geometry) {}

    const std::string& path()    const { return m_panels.back().path(); }
    const DirItem& currentItem() const { return m_panels.back().currentItem(); }

    void navigate(const std::string& path);

    int animate();
    void draw();

    void moveCursor(int target, bool relative);
    void push();
    void pop();
};
