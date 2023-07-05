#pragma once

#include <algorithm>

struct Geometry {
    int screenWidth;
    int screenHeight;

    int dirViewY0;
    int dirViewY1;

    int itemsPerPage;
    int itemHeight;
    int textSize;

    int itemMarginX;
    int itemMarginY;
    int panelMarginX;

    int itemOutlineWidth;
    int itemOutlineOffset;
    int itemBorderRadius;
    int itemShadowOffset;

    void update(int screenWidth, int screenHeight);

    // animation filter API
    float _filterCoeff;
    void setTimeDelta(float dt);
    int animUpdate(float &value, float newValue) const;
};
