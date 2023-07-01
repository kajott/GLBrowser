#include <cstdio>

#include <new>

#include "glad.h"

#include "renderer.h"

///////////////////////////////////////////////////////////////////////////////

constexpr int BatchSize = 1024;  // must be 16384 or less

static const char* vsSrc =
     "#version 330"
"\n" "layout(location=0) in vec2 aPos;"
"\n" "layout(location=2) in vec4 aColor; out vec4 vColor;"
"\n" "void main() {"
"\n" "    gl_Position = vec4(aPos, 0., 1.);"
"\n" "    vColor = aColor;"
"\n" "}"
"\n";

static const char* fsSrc =
     "#version 330"
"\n" "in vec4 vColor;"
"\n" "layout(location=0) out vec4 outColor;"
"\n" "void main() {"
"\n" "    outColor = vColor;"
"\n" "}"
"\n";

bool TextBoxRenderer::init() {
    GLint res;

    viewportChanged();

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, BatchSize * 4 * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
    m_vertices = nullptr;

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

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    // GL_ARRAY_BUFFER is still bound here
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT,        GL_FALSE, sizeof(Vertex), &(static_cast<Vertex*>(0)->pos[0]));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &(static_cast<Vertex*>(0)->color));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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

void TextBoxRenderer::box(int x0, int y0, int x1, int y1, uint32_t colorUpper, uint32_t colorLower) {
    Vertex* v = newVertices(float(x0), float(y0), float(x1), float(y1));
    v[0].color = v[1].color = colorUpper;
    v[2].color = v[3].color = colorLower;
}
