#include <cstdio>

#include <new>
#include <algorithm>

#include "glad.h"

#include "renderer.h"

///////////////////////////////////////////////////////////////////////////////

constexpr int BatchSize = 1024;  // must be 16384 or less

static const char* vsSrc =
     "#version 330"
"\n" "layout(location=0) in vec2 aPos;"
"\n" "layout(location=1) in vec2 aTC;         out vec2 vTC;"
"\n" "layout(location=2) in vec3 aSize;  flat out vec3 vSize;"
"\n" "layout(location=3) in vec2 aBR;    flat out vec2 vBR;"
"\n" "layout(location=4) in vec4 aColor;      out vec4 vColor;"
"\n" "void main() {"
"\n" "    gl_Position = vec4(aPos, 0., 1.);"
"\n" "    vTC    = aTC;"
"\n" "    vSize  = aSize;"
"\n" "    vBR    = aBR;"
"\n" "    vColor = aColor;"
"\n" "}"
"\n";

static const char* fsSrc =
     "#version 330"
"\n" "     in vec2 vTC;"
"\n" "flat in vec3 vSize;"
"\n" "flat in vec2 vBR;"
"\n" "     in vec4 vColor;"
"\n" "layout(location=0) out vec4 outColor;"
"\n" "void main() {"
"\n" "    float d;"
"\n" "    if (true) {  // box mode"
"\n" "        vec2 p = abs(vTC) - vSize.xy;"
"\n" "        d = (min(p.x, p.y) > (-vSize.z))"
"\n" "          ? (vSize.z - length(p + vec2(vSize.z)))"
"\n" "          : min(-p.x, -p.y);"
"\n" "    }"
"\n" "    outColor = vec4(vColor.rgb, vColor.a * clamp((d - vBR.x) * vBR.y + 0.5, 0.0, 1.0));"
"\n" "}"
"\n";

bool TextBoxRenderer::init() {
    GLint res;

    viewportChanged();

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, BatchSize * 4 * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
    m_vertices = nullptr;

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    // GL_ARRAY_BUFFER is still bound
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(0, 2, GL_FLOAT,        GL_FALSE, sizeof(Vertex), &(static_cast<Vertex*>(0)->pos[0]));
    glVertexAttribPointer(1, 2, GL_FLOAT,        GL_FALSE, sizeof(Vertex), &(static_cast<Vertex*>(0)->tc[0]));
    glVertexAttribPointer(2, 3, GL_FLOAT,        GL_FALSE, sizeof(Vertex), &(static_cast<Vertex*>(0)->size[0]));
    glVertexAttribPointer(3, 2, GL_FLOAT,        GL_FALSE, sizeof(Vertex), &(static_cast<Vertex*>(0)->br[0]));
    glVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &(static_cast<Vertex*>(0)->color));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    auto iboData = new(std::nothrow) uint16_t[BatchSize * 6];
    if (!iboData) { return false; }
    auto iboPtr = iboData;
    for (int base = 0;  base < (BatchSize * 4);  base += 4) {
        *iboPtr++ = uint16_t(base + 0);
        *iboPtr++ = uint16_t(base + 2);
        *iboPtr++ = uint16_t(base + 1);
        *iboPtr++ = uint16_t(base + 1);
        *iboPtr++ = uint16_t(base + 2);
        *iboPtr++ = uint16_t(base + 3);
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, BatchSize * 6 * sizeof(uint16_t), (const void*) iboData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glFlush(); glFinish();
    delete[] iboData;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &res);
    if (res != GL_TRUE) {
        #ifdef _DEBUG
            ::puts(vsSrc);
            printf("Vertex Shader compilation failed.\n");
            glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &res);
            char* msg = new(std::nothrow) char[res];
            if (msg) {
                glGetShaderInfoLog(vs, res, nullptr, msg);
                puts(msg);
                delete[] msg;
            }
        #endif
        return false;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &res);
    if (res != GL_TRUE) {
        #ifdef _DEBUG
            ::puts(fsSrc);
            printf("Fragment Shader compilation failed.\n");
            glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &res);
            char* msg = new(std::nothrow) char[res];
            if (msg) {
                glGetShaderInfoLog(fs, res, nullptr, msg);
                ::puts(msg);
                delete[] msg;
            }
        #endif
        return false;
    }

    m_prog = glCreateProgram();
    glAttachShader(m_prog, vs);
    glAttachShader(m_prog, fs);
    glLinkProgram(m_prog);
    glGetProgramiv(fs, GL_LINK_STATUS, &res);
    if (res != GL_TRUE) {
        #ifdef _DEBUG
            printf("Shader Program linking failed.\n");
            glGetProgramiv(fs, GL_INFO_LOG_LENGTH, &res);
            char* msg = new(std::nothrow) char[res];
            if (msg) {
                glGetProgramInfoLog(fs, res, nullptr, msg);
                ::puts(msg);
                delete[] msg;
            }
        #endif
        return false;
    }
    glDeleteShader(fs);
    glDeleteShader(vs);

    m_quadCount = 0;
    return true;
}

void TextBoxRenderer::viewportChanged() {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    m_vpScaleX =  2.0f / float(vp[2]);
    m_vpScaleY = -2.0f / float(vp[3]);
}

void TextBoxRenderer::flush() {
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(m_vao);
    glUseProgram(m_prog);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glDrawElements(GL_TRIANGLES, m_quadCount * 6, GL_UNSIGNED_SHORT, nullptr);
    m_quadCount = 0;
}

void TextBoxRenderer::shutdown() {
    glBindVertexArray(0);                      glDeleteVertexArrays(1, &m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, 0);          glDeleteBuffers(1, &m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  glDeleteBuffers(1, &m_ibo);
    glUseProgram(0);                           glDeleteProgram(m_prog);
}

///////////////////////////////////////////////////////////////////////////////

TextBoxRenderer::Vertex* TextBoxRenderer::newVertices() {
    if (m_quadCount >= BatchSize) { flush(); }
    if (!m_vertices) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        m_vertices = (Vertex*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    return &m_vertices[4 * (m_quadCount++)];
}

TextBoxRenderer::Vertex* TextBoxRenderer::newVertices(float x0, float y0, float x1, float y1) {
    x0 = x0 * m_vpScaleX - 1.0f;
    y0 = y0 * m_vpScaleY + 1.0f;
    x1 = x1 * m_vpScaleX - 1.0f;
    y1 = y1 * m_vpScaleY + 1.0f;
    Vertex* v = newVertices();
    v[0].pos[0] = x0;  v[0].pos[1] = y0;
    v[1].pos[0] = x1;  v[1].pos[1] = y0;
    v[2].pos[0] = x0;  v[2].pos[1] = y1;
    v[3].pos[0] = x1;  v[3].pos[1] = y1;
    return v;
}

TextBoxRenderer::Vertex* TextBoxRenderer::newVertices(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1) {
    Vertex* v = newVertices(x0, y0, x1, y1);
    v[0].tc[0] = u0;  v[0].tc[1] = v0;
    v[1].tc[0] = u1;  v[1].tc[1] = v0;
    v[2].tc[0] = u0;  v[2].tc[1] = v1;
    v[3].tc[0] = u1;  v[3].tc[1] = v1;
    return v;
}

void TextBoxRenderer::box(int x0, int y0, int x1, int y1, uint32_t colorUpper, uint32_t colorLower, int borderRadius, float blur, float offset) {
    float w = 0.5f * (float(x1) - float(x0));
    float h = 0.5f * (float(y1) - float(y0));
    Vertex* v = newVertices(float(x0), float(y0), float(x1), float(y1), -w, -h, w, h);
    v[0].color = v[1].color = colorUpper;
    v[2].color = v[3].color = colorLower;
    for (int i = 4;  i;  --i, ++v) {
        v->size[0] = w;  v->size[1] = h;
        v->size[2] = std::min(std::min(w, h), float(borderRadius));  // clamp border radius to half size
        v->br[0] = offset;
        v->br[1] = 1.0f / std::max(blur, 1.0f/256);
    }
}

void TextBoxRenderer::contourBox(int x0, int y0, int x1, int y1, uint32_t colorUpper, uint32_t colorLower, uint32_t colorContour, int contourWidth, int borderRadius, int shadowOffset, float shadowBlur, float shadowAlpha, int shadowGrow) {
    int cOuter = std::max(0,  contourWidth);
    int cInner = std::max(0, -contourWidth);
    if ((shadowOffset || shadowGrow) && (shadowAlpha > 0.0f)) {
        uint32_t shadowColor = uint32_t(std::min(1.0f, std::max(0.0f, shadowAlpha)) * 255.f + .5f) << 24;
        box(x0 - cOuter + shadowOffset - shadowGrow,
            y0 - cOuter + shadowOffset - shadowGrow,
            x1 + cOuter + shadowOffset + shadowGrow,
            y1 + cOuter + shadowOffset + shadowGrow,
            shadowColor, shadowColor,
            borderRadius + cOuter + shadowGrow,
            shadowBlur + 1.0f, shadowBlur);
    }
    if (contourWidth) {
        box(x0 - cOuter, y0 - cOuter, x1 + cOuter, y1 + cOuter,
            colorContour | 0xFF000000u, colorContour | 0xFF000000u, borderRadius + cOuter);
    }
    box(x0 + cInner, y0 + cInner, x1 - cInner, y1 - cInner,
        colorUpper | 0xFF000000u, colorLower | 0xFF000000u, borderRadius - cInner);
}
