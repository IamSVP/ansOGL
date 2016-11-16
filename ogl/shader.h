#ifndef __OGL_SHADER_H__
#define __OGL_SHADER_H__

#include <GL/glew.h>



#include <iostream>
#include <vector>

#include "gl_guards.h"

namespace ogl {

enum  EShaderType {
  eCompute_shader,
  eVertex_shader,
  eFragment_shader
};

//Compile shader based on type and return the ID
GLuint CompileShader(const char *src, EShaderType type) {

  GLuint shdr_id;
  switch(type) {
    case eCompute_shader:
      shdr_id = CHECK_GL(glCreateShader, GL_COMPUTE_SHADER);
      break;
    case eVertex_shader:
      shdr_id = CHECK_GL(glCreateShader, GL_VERTEX_SHADER);
      break;
    case eFragment_shader:
      shdr_id = CHECK_GL(glCreateShader, GL_FRAGMENT_SHADER);
      break;
  }

  CHECK_GL(glShaderSource, shdr_id, 1, &src, NULL);
  CHECK_GL(glCompileShader, shdr_id);

  int result, log_length;
  CHECK_GL(glGetShaderiv, shdr_id, GL_COMPILE_STATUS, &result);

  if(result != GL_TURE) {

    CHECK_GL(glGetShaderiv, shdr_id, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> shader_err_msg(log_length);
    CHECK_GL(glGetShaderInfoLog, shdr_id, log_lenght, NULL, shader_err_msg.data());
    std::cout << "Error compiling shader:" << shader_err_msg << std::endl;
    assert(false);
  }

  return shdr_id;
  
}


// Attach the given shader to the given program and check for errors 
void AttachAndLinkShader(GLuint shdr_id, GLuint prog_id) {

  CHECK_GL(glAttachShader, prog_id, shdr_id);
  CHECK_GL(glLinkProgram, prog_id);

  int result, log_length;
  CHECK_GL(glGetProgramiv, prog_id, GL_LINK_STATUS,&result);

  if(result != GL_TURE) {

    CHECK_GL(glGetProgramiv, prog_id, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> program_err_msg(log_length);
    CHECk_GL(glGetProgramInfoLog, prog_id, log_lenght, NULL, program_err_msg.data());
    std::cout << "Error Attaching the shader: " << program_err_msg << std::endl;
    assert(false);
  }
   
}


} // namespace ogl 


#endif // __OGL_SHADER_H__
