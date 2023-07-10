#pragma once

#include <string>
#include <vector>

#include "renderer.h"
#include "geometry.h"
#include "sysutil.h"

class DirView;

///////////////////////////////////////////////////////////////////////////////

struct DirItem {
    std::string name;
    uint32_t extCode;
    bool isDir;
    bool isExec;
    std::string display;
    bool operator< (const DirItem& other) const;
    bool operator== (const std::string& other) const;
    inline const std::string& displayText() const { return display.empty() ? name : display; }
    inline DirItem(const std::string& name_, bool isDir_, bool isExec_)
        : name(name_), extCode(isDir_ ? '/' : extractExtCode(name_)), isDir(isDir_), isExec(isExec_), display(isDir_ ? (name_ + " \xE2\x96\xBA") : "") {}
    inline DirItem(const std::string& name_, bool isDir_, bool isExec_, const std::string& display_)
        : name(name_), extCode(isDir_ ? '/' : extractExtCode(name_)), isDir(isDir_), isExec(isExec_), display(display_) {}
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
    explicit DirPanel(DirView& parent, const std::string& path, int x0, bool active=true, const std::string& preselect="");

    inline int cursorY()                const { return m_y0 + m_cursor * m_geometry.itemHeight; }
    inline int startX()                 const { return m_x0; }
    inline int endX()                   const { return m_x0 + m_width; }
    inline const std::string& path()    const { return m_path; }
    inline const DirItem& currentItem() const { return m_items[m_cursor]; }
    inline void deactivate()                  { m_active = false; }
    inline void activate()                    { m_active = true; }

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

    inline const bool atRoot()             const { return (m_panels.size() < 2u); }
    inline const int xScroll()             const { return m_xScroll; }
    inline const DirPanel& currentPanel()  const { return m_panels.back(); }
    inline const DirItem& currentItem()    const { return currentPanel().currentItem(); }
    inline const std::string& currentDir() const { return currentPanel().path(); }
    std::string currentItemFullPath() const;

    inline void deactivate() { m_panels.back().deactivate(); }
    inline void activate()   { m_panels.back().activate(); }

    void navigate(const std::string& path);

    int animate();
    void draw();

    void moveCursor(int target, bool relative);
    void push();
    void pop();
};
