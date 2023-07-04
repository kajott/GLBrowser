#pragma once

#include <cstdint>

#include "glad.h"

#include "font_data.h"

//! text alignment constants
namespace Align {
    constexpr uint8_t Left     = 0x00;  //!< horizontally left-aligned
    constexpr uint8_t Center   = 0x01;  //!< horizontally centered
    constexpr uint8_t Right    = 0x02;  //!< horizontally right-aligned
    constexpr uint8_t Top      = 0x00;  //!< vertically top-aligned (text is *below* indicated point)
    constexpr uint8_t Middle   = 0x10;  //!< vertically centered
    constexpr uint8_t Bottom   = 0x20;  //!< vertically bottom-aligned (text is *above* indicated point)
    constexpr uint8_t Baseline = 0x30;  //!< vertically baseline-aligned (indicated point is at text baseline)
    constexpr uint8_t HMask    = 0x0F;  //!< \private horizontal alignment mask
    constexpr uint8_t VMask    = 0xF0;  //!< \private vertical alignment mask
};

//! a renderer that can draw two things: MSDF text, or rounded boxes
class TextBoxRenderer {
    float m_vpScaleX, m_vpScaleY;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ibo;
    GLuint m_prog;
    GLuint m_tex;
    int m_quadCount;
    int* m_glyphCache;

    struct Vertex {
        float pos[2];    // screen position (already transformed into NDC)
        float tc[2];     // texture coordinate | half-size coordinate (goes from -x/2 to x/2, with x=width or x=height)
        float size[3];   // not used | xy = half size, z = border radius
        float br[2];     // blend range: x = distance to outline (in pixels) that corresponds to middle gray, y = reciprocal of range
        uint32_t color;  // color to draw in
        uint32_t mode;   // 0 = box, 1 = text
    };

    Vertex* m_vertices;

    Vertex* newVertices();
    Vertex* newVertices(uint8_t mode, float x0, float y0, float x1, float y1);
    Vertex* newVertices(uint8_t mode, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1);

    const FontData::Glyph* getGlyph(uint32_t codepoint);
    static uint32_t nextCodepoint(const char* &utf8string);
    void alignText(float &x, float &y, float size, const char* text, uint8_t align);

public:
    bool init();
    void shutdown();
    void viewportChanged();
    void flush();

    void box(int x0, int y0, int x1, int y1,
             uint32_t colorUpper, uint32_t colorLower,
             int borderRadius=0,
             float blur=1.0f, float offset=0.0f);
    inline void box(int x0, int y0, int x1, int y1, uint32_t color)
        { box(x0, y0, x1, y1, color, color); }

    void outlineBox(int x0, int y0, int x1, int y1,
                    uint32_t colorUpper, uint32_t colorLower,  // all colors are forced to fully opaque!
                    uint32_t colorOutline=0xFFFFFFFF,
                    int outlineWidth=0,  // positive: outline *outside* the box coords, negative: outline *inside* the box coords
                    int borderRadius=0,
                    int shadowOffset=0, float shadowBlur=0.0f, float shadowAlpha=1.0f, int shadowGrow=0);

    inline void circle(int x, int y, int r, uint32_t color, float blur=1.0f, float offset=0.0f)
        { box(x - r, y - r, x + r, y + r, color, color, r, blur, offset); }

    float textWidth(const char* text);
    void text(float x, float y, float size, const char* text,
              uint8_t align = Align::Left + Align::Top,
              uint32_t colorUpper=0xFFFFFFFF, uint32_t colorLower=0xFFFFFFFF,
              float blur=1.0f, float offset=0.0f);
    void outlineText(float x, float y, float size, const char* text,
                     uint8_t align = Align::Left + Align::Top,
                     uint32_t colorUpper=0xFFFFFFFF, uint32_t colorLower=0xFFFFFFFF,
                     uint32_t colorOutline=0xFF000000,
                     float outlineWidth=0.0f,
                     int shadowOffset=0, float shadowBlur=0.0f, float shadowAlpha=1.0f, float shadowGrow=0.0f);
};
