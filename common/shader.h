#ifndef __OGL_SHADER_H__
#define __OGL_SHADER_H__

#include <GL/glew.h>


#include <glm/glm.hpp>

#include <iostream>

#include "gl_gaurds.h"

enum  EShaderType {
  eCompute_shader,
  eVertex_shader,
  eFragment_shader
};

GLuint CompileShader(const char *src, EShaderType type) {

  GLuint prog_id;
  switch(type){
    case eCompute_shader:
      prog_id = CHECK_GL(glCreateShader, GL_COMPUTE_SHADER);
      break;
    case eVertex_shader:
      prog_id = CHECK_GL(glCreateShader, GL_VERTEX_SHADER);
      break;
    case 
  }
}


#endif // __OGL_SHADER_H__
