/*
 * @Author: RetliveAdore lizaterop@gmail.com
 * @Date: 2024-07-08 12:33:11
 * @LastEditors: RetliveAdore lizaterop@gmail.com
 * @LastEditTime: 2024-08-18 12:59:02
 * @FilePath: \CrystalGraphic\src\crgl.h
 * @Description: 
 * Coptright (c) 2024 by RetliveAdore-lizaterop@gmail.com, All Rights Reserved. 
 */
#ifndef _INCLUDE_CRGL_H_
#define _INCLUDE_CRGL_H_

#include <GraphicDfs.h>
#include "glDfs.h"

#ifdef CR_WINDOWS
#  include <libloaderapi.h>
#  include <Windows.h>
#  define CR_GLAPI
#elif defined CR_LINUX
#  include <dlfcn.h>
#  include <GL/glx.h>
#  define CR_GLAPI
#endif

/**
 * 用于对象池管理，申请新的GPU对象很慢，所以说能复用就复用
 */
typedef struct cr_gl_public_pool
{
    CRSTRUCTURE vaoPool;
    CRSTRUCTURE vboPool;
    CRSTRUCTURE texPool;
    //一些会用到的接口
    void (*glGenVertexArrays)(GLsizei n, GLuint* arrays);
    void (*glDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
    void (*glGenBuffers)(GLsizei n, GLuint* buffers);
    void (*glDeleteBuffers)(GLsizei n, const GLuint* buffers);
    void (*glGenTextures)(GLsizei n, GLuint* textures);
    void (*glDeleteTextures)(GLsizei n, const GLuint* textures);
}CR_GL_PUBPOOL, *PCR_GL_PUBPOOL;

typedef struct cr_gl
{
    struct cr_gl* pShared;
    #ifdef CR_WINDOWS
    HDC hdc;
    HGLRC hrc;
    #elif defined CR_LINUX
    Display* dpy;
    Window wd;
    GLXContext context;
    #endif
    CRUINT64 w, h;
    float ratio;
    float aspx, aspy;
    float dx, dy;
    //
    PCR_GL_PUBPOOL pubPool;
    //
    CRUINT32 VertexShader;
    CRUINT32 FragmentShader;
    CRUINT32 shaderProgram;
    CRINT32 colorLocation;
    CRINT32 aspLocation;
    CRINT32 texture0;
    CRUINT32 publicTexture;
    CRCOLORU whiteColor;  //1x1纹理，主打一个节省空间
    //
    CRUINT8 *VertexSource;
    CRUINT8 *FragmentSource;
    //
    CRLOCK renderLock;
    //ui
    GLuint fboUI;
    GLuint texUI1, texUI2, texUIcurrent;
    //
    const GLubyte* (*glGetString)(GLenum name);
    void (*glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void (*glClear)(GLbitfield mask);
    void (*glLoadIdentity)(void);
    void (*glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
    void (*glOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
    void (*glDisable)(GLenum cap);
    void (*glEnable)(GLenum cap);
    void (*glBlendFunc)(GLenum sfactor, GLenum dfactor);
    void (*glBegin)(GLenum mode);
    void (*glEnd)(void);
    void (*glColor3f)(GLfloat red, GLfloat green, GLfloat blue);
    void (*glColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void (*glVertex3f)(GLfloat x, GLfloat y, GLfloat z);
    void (*glGenVertexArrays)(GLsizei n, GLuint* arrays);
    void (*glDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
    void (*glGenBuffers)(GLsizei n, GLuint* buffers);
    void (*glDeleteBuffers)(GLsizei n, const GLuint* buffers);
    void (*glGenTextures)(GLsizei n, GLuint* textures);
    void (*glDeleteTextures)(GLsizei n, const GLuint* textures);
    void (*glGenRenderBuffers)(GLsizei n, GLuint* renderbuffers);
    void (*glDeleteRenderBuffers)(GLsizei n, GLuint* renderbuffers); 
    void (*glGenFrameBuffers)(GLsizei n, GLuint* framebuffers);
    void (*glDeleteFrameBuffers)(GLsizei n, GLuint* framebuffers);
    void (*glBindVertexArray)(GLuint arr);
    void (*glBindBuffer)(GLenum target, GLuint buffer);
    void (*glBindTexture)(GLenum target, GLuint texture);
    void (*glBindRenderBuffer)(GLenum target, GLuint renderbuffer);
    void (*glBindFrameBuffer)(GLenum target, GLuint framebuffer);
    void (*glVertexAttribPointer)(GLuint index, GLuint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
    void (*glBufferData)(GLenum target, GLsizei ptrSize, const GLvoid* data, GLenum usage);
    void (*glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLint format, GLenum type, const GLvoid* pixels);
    GLuint (*glCreateShader)(GLenum shaderType);
    void (*glDeleteShader)(GLuint shader);
    void (*glShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
    void (*glCompileShader)(GLuint shader);
    void (*glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
    void (*glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
    void (*glAttachShader)(GLuint program, GLuint shader);
    void (*glDetachShader)(GLuint program, GLuint shader);
    GLuint (*glCreateProgram)(void);
    void (*glDeleteProgram)(GLuint program);
    void (*glLinkProgram)(GLuint program);
    void (*glUseProgram)(GLuint program);
    void (*glEnableVertexAttribArray)(GLuint index);
    void (*glDisableVertexAttribArray)(GLuint index);
    void (*glDrawArrays)(GLenum mode, GLint first, GLsizei count);
    void (*glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void* indicies);
    void (*glPolygonMode)(GLenum face, GLenum mode);
    void (*glUniform1i)(GLint location, GLint v0);
    void (*glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
    void (*glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    GLuint (*glGetUniformLocation)(GLuint program, const GLchar* name);
    void (*glTexParameteri)(GLenum target, GLenum pname, GLint param);
    void (*glTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params);
    void (*glGenerateMipmap)(GLenum target);
    void (*glActiveTexture)(GLenum texture);
    GLenum (*glCheckFrameBufferStatus)(GLenum target);
}CR_GL;

#ifdef CR_WINDOWS
CR_GL* _inner_create_cr_gl_(CR_GL* shared, HDC hDc);
CR_GL* _inner_create_cr_gl_ui_(HDC hDc);
#elif defined CR_LINUX
CR_GL* _inner_create_cr_gl_(CR_GL* shared, Display* pDisplay, XVisualInfo* vi, Window win);
CR_GL* _inner_create_cr_gl_ui_(Display* pDisplay, XVisualInfo* vi, Window win);
#endif
void _inner_delete_cr_gl_(CR_GL* pgl);

void _inner_cr_gl_resize_(CR_GL* pgl);
void _inner_cr_gl_paint_ui_(CR_GL* pgl);
void _inner_cr_gl_paint_(CR_GL* pgl);
void _inner_set_size_(CR_GL* pgl, CRUINT64 w, CRUINT64 h);

//
PCR_GL_PUBPOOL _inner_cr_glpubpool_create_(CR_GL* pgl);
void _inner_cr_glpubpool_release_(PCR_GL_PUBPOOL pool);
CRUINT32 _inner_vao_(PCR_GL_PUBPOOL pool);
CRUINT32 _inner_vbo_(PCR_GL_PUBPOOL pool);
CRUINT32 _inner_tex_(PCR_GL_PUBPOOL pool);

#endif
