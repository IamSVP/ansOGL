#ifndef __OGL_GL_GUARDS_H__
#define __OGL_GL_GUARDS_H__

#include <cstdio>
#include <cassert>
#ifndef NDEBUG

static inline const char* glErrMsg(GLenum err){
  const char *errString = "Unknown error";
  switch(err){
    case GL_INVALID_ENUM: errString = "Invalid enum"; break;
    case GL_INVALID_VALUE: errString = "Invalid value"; break;
    case GL_INVALID_OPERATION: errString = "Invalid operation"; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: errString = "Invalid Frame Buffer Operation"; break;
    case GL_OUT_OF_MEMORY: errString = "Out of Memory Operation"; break;
  }
  return errString;
}

static const char* getGLErrString(GLenum err){

    const char *errString = "Unknown error";
    switch(err){
        case GL_INVALID_ENUM: errString = "Invalid enum"; break;
        case GL_INVALID_VALUE: errString = "Invalid value"; break;
        case GL_INVALID_OPERATION: errString = "Invalid operation"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: errString = "Invalid Frame Buffer Operation"; break;
        case GL_OUT_OF_MEMORY: errString = "Out of Memory Operation"; break;
    }
    return errString;
}


#define CHECK_GL(fn, ...) fn(__VA_ARGS__);                                           \
  do {                                                                               \
    GLenum glErr = glGetError();                                                     \
    if(glErr != GL_NO_ERROR) {                                                       \
      const char *errString = getGLErrString(glErr);                                 \
      if(NULL != errString){                                                         \
        printf("OpenGL call Error (%s : %d): %s\n",__FILE__, __LINE__, errString);    \
      } else {                                                                       \
        printf("Unknown OpenGL call Error(%s : %d)", __FILE__, __LINE__);                 \
       }                                                                             \
      assert(false);                                                                 \
    }                                                                                \
  } while(0)
#else
#define CHECK_GL(fn, ...) fn(__VA_ARGS__);
#endif

#endif // __OGL_GL_GUARDS_H__
