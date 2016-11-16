#ifndef __OGL_GPU_H__
#define __OGL_GPU_H__


#include <GL/glew.h>
// Window Management file
#include <glfw3.h> 
    

#include "gl_guards.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <tuple>
#include <memory>
#include <vector>

namespace ogl {

enum  EShaderType {
  eCompute_shader,
  eVertex_shader,
  eFragment_shader
};


typedef struct _GPUProgram {

  GLuint _prog_id;
  std::tuple<EShaderType, GLuint> _shader;

} GPUProgram;


// !!Does't handle VBO creations or FBO creations yet but can be modified!!
//  Just for shader compilations and invocations 
class GPUCompute {
 public:
   ~GPUCompute();
   static std::unique_ptr<GPUCompute> InitializeGL();
   void InitializeGLFW(); 

   GPUProgram CompileAndLinkShader(std::string &file_name, EShaderType eType, GLuint prog_id);

   GPUProgram CompileAndLinkShader(std::string &file_name, EShaderType eType);

   GPUProgram GetShader(std::string &file_name);

   void SetArgMemory(const GLuint &ssbo_id, GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage);
   void SetShaderArg( const GLuint &ssbo_id, GLuint binding, GLenum target); 
   template<typename T>
   std::vector<T> GetShaderArg(const GLuint &ssbo_id, GLenum target, uint32_t count, GLenum usage) {
     CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, ssbo_id);
     GLvoid *ptr = CHECK_GL(glMapBuffer, target, usage);
     std::vector<T> result(count);
     memcpy(result.data(), ptr, result.size()*sizeof(T));
     CHECK_GL(glUnmapBuffer, GL_SHADER_STORAGE_BUFFER);
     CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, 0);
     return std::move(result);
   }



 private:
   GPUCompute() { };
   std::unordered_map<std::string, GPUProgram> _gpu_programs;
   std::unique_ptr<GPUProgram> _curr_program;
   GPUCompute(const GPUCompute &) { }



};


} // namespace ogl 


#endif // __OGL_GPU_H__
