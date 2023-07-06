#include <cmath>

#include <algorithm>

#include "geometry.h"

void Geometry::update(int screenWidth_, int screenHeight_) {
    screenWidth  = screenWidth_;
    screenHeight = screenHeight_;

    itemsPerPage = 20;
    itemHeight = screenHeight / itemsPerPage;

    itemMarginY = itemHeight / 8;
    itemMarginX = itemMarginY;
    panelMarginX = itemMarginX;

    textSize = itemHeight - 2 * itemMarginY;

    itemOutlineWidth = std::max(1, itemHeight / 16);
    itemOutlineOffset = itemOutlineWidth / 2;
    itemBorderRadius = itemMarginY;
    itemShadowOffset = itemOutlineWidth * 2;

    outerMarginX = itemHeight / 4;
    outerMarginY = itemHeight / 6;
    gradientHeight = itemHeight / 4;

    dirViewY0 = textSize + 3 * outerMarginY + gradientHeight;
    dirViewY1 = screenHeight - dirViewY0;
}

void Geometry::setTimeDelta(float dt) {
    _filterCoeff = std::exp2(dt * (-1.0f / 0.05f));
}

int Geometry::animUpdate(float &value, float newValue) const {
    float prev = value;
    value = newValue + _filterCoeff * (value - newValue);
    bool running = (std::abs(value - newValue) > 1E-3f);
    if (!running) { value = newValue; }
    return running ? 1 : 0;
}
