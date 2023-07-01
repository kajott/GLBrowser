#pragma once

#include <cstdint>

#include "glad.h"

//! a renderer that can draw two things: MSDF text, or rounded boxes
class TextBoxRenderer {
    float m_vpScaleX, m_vpScaleY;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ibo;
    GLuint m_prog;
    int m_quadCount;

    struct Vertex {
        float pos[2];
        float texCoord[2];
        uint32_t color;
    };

    Vertex* m_vertices;

    Vertex* newVertices();
    Vertex* newVertices(float x0, float y0, float x1, float y1);

public:
    bool init();
    void shutdown();
    void viewportChanged();
    void flush();

    void box(int x0, int y0, int x1, int y1, uint32_t colorUpper, uint32_t colorLower);
    inline void box(int x0, int y0, int x1, int y1, uint32_t color)
        { box(x0, y0, x1, y1, color, color); }

};
