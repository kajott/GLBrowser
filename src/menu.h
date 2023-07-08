#pragma once

#include "renderer.h"
#include "geometry.h"
#include "dirview.h"
#include "event.h"

#include <string>
#include <vector>
#include <functional>

class ModalMenu {
    TextBoxRenderer& m_renderer;
    const Geometry& m_geometry;
    bool m_active = false;
    std::string m_mainTitle;
    std::string m_boxTitle;
    
    struct MenuItem {
        int id;
        std::string text;
        bool separatorFollows = false;
        float textX;
        int y;
        inline MenuItem(int id_, const std::string& text_) : id(id_), text(text_) {}
    };
    std::vector<MenuItem> m_items;

    struct ControlItem {
        bool keyboard;
        std::string control;
        std::string caption;
        int enableMask;
        int enableRef;
        inline ControlItem(bool keyboard_, const std::string& control_, const std::string& caption_, int enableMask_=0, int enableRef_=0)
            : keyboard(keyboard_), control(control_), caption(caption_), enableMask(enableMask_), enableRef(enableRef_) {}
    };
    std::vector<ControlItem> m_controls;

    bool m_layoutFinished = false;
    int m_width;
    int m_height;
    int m_targetY0;
    int m_x0;
    int m_y0;
    int m_cursor;
    int m_resultID;
    float m_boxTitleTextX;
    float m_animCursorY;
    bool m_confirmed = false;
    bool m_dismissed = false;

    void setCursor(int pos);
    void finishLayout();

public:
    inline ModalMenu(TextBoxRenderer& renderer, const Geometry& geometry)
        : m_renderer(renderer), m_geometry(geometry) {}

    inline const std::string& mainTitle() const  { return m_mainTitle; }
    inline void setMainTitle(std::string& title) { m_mainTitle = title; }

    inline const std::string& boxTitle() const   { return m_boxTitle; }
    void setBoxTitle(const std::string& title);

    inline bool active()    const { return m_active; }
    inline int  result()    const { return m_resultID; }
    inline bool confirmed() const { return m_confirmed; }
    inline bool dismissed() const { return m_dismissed; }

    void clear();
    void addItem(int id, const std::string& text);
    void addSeparator();
    void addControl(bool keyboard, const std::string& control, const std::string& caption, int enableMask=0, int enableRef=0);

    void avoid(int y, int x0, int x1);
    void avoidCurrentItem(const DirView& dirView);
    void activate();

    int animate();
    void draw();
    void controls(std::function<void(bool keyboard, const std::string& control, const std::string& caption)> callback);

    enum class EventType {
        Confirm,  //!< menu item has been selected and confirmed (and menu has been deactivated)
        Dismiss,  //!< menu has been dismissed (and has been deactivated)
        Cursor,   //!< some kind of cursor movement has been processed
        Other,    //!< some other button has been pressed (the app may want to investigate further)
        Inactive  //!< menu isn't active, no event processing done
    };
    EventType handleEvent(AppEvent ev);
};
