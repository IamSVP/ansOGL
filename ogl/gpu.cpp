#include "gpu.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <cassert>
#include <vector>
#include <cstdio>

namespace ogl {


  std::unique_ptr<GPUCompute> GPUCompute::InitializeGL() {

    if(!glfwInit()) {
      fprintf(stderr, "Error Initializing glfw\n");
      assert(false);
      exit(-1);
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window;
	// Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "Tutorial 01", NULL, NULL);
    if( window == NULL ){
      fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
      getchar();
      glfwTerminate();
      exit(-1);
    }
      glfwMakeContextCurrent(window); 

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if(err != GLEW_OK) {
      std::cerr << "Error msg:: " << "GLEW Initialization failed" << std::endl;
      fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      assert(false);
      exit(-1);
    }
    GLenum err2 = GL_NO_ERROR;
    while((err2 = glGetError()) != GL_NO_ERROR)
    {
  //Process/log the error.
    }

    std::unique_ptr<GPUCompute> gpu = 
      std::unique_ptr<GPUCompute>(new GPUCompute);
    return std::move(gpu);
    
  }
 
  GPUProgram GPUCompute::GetShader(std::string &file_name) {
    std::unordered_map<std::string, GPUProgram>::const_iterator it = _gpu_programs.find(file_name);
    if(it != _gpu_programs.end() ) {
      return it->second;
    }
    else {
      std::cerr << "Error finding the file_name" << std::endl;
      assert(false);
      exit(-1);
    }
  }

  void GPUCompute::SetArgMemory(const GLuint &ssbo_id, GLenum target, GLsizeiptr size,
                                const GLvoid *data, GLenum usage) {
    CHECK_GL(glBindBuffer, target, ssbo_id);
    CHECK_GL(glBufferData, target, size, data, usage);
    CHECK_GL(glBindBuffer, target, 0);

  }

  void GPUCompute::SetShaderArg(const GLuint &ssbo_id, GLuint binding, GLenum target) {
    CHECK_GL(glBindBuffer, target, ssbo_id);
    CHECK_GL(glBindBufferBase, target, binding, ssbo_id);
    CHECK_GL(glBindBuffer, target, 0);
  }

  GPUProgram GPUCompute::CompileAndLinkShader(std::string &file_name, EShaderType eType) { 
    GLuint prog_id  = CHECK_GL(glCreateProgram);
    return CompileAndLinkShader(file_name, eType, prog_id);
  }

  GPUProgram GPUCompute::CompileAndLinkShader(std::string &file_name, 
                                               EShaderType eType, GLuint prog_id) {

    std::unordered_map<std::string, GPUProgram>::const_iterator it = _gpu_programs.find(file_name);
    if(it != _gpu_programs.end() ) {
      return it->second;
    }
    else {
      std::ifstream shdr_fs(file_name.c_str(), std::ifstream::in);
      if(!shdr_fs) {
        assert(!"Error opening file");
        exit(-1);
      }

      std::string shdr_str((std::istreambuf_iterator<char>(shdr_fs)),
	                    std::istreambuf_iterator<char>());

      if(shdr_str.empty()) {
        assert(false && "Error reading into string from file");
      
        exit(-1);
      }

      const char *shdr_Cstr = shdr_str.c_str();

      GLuint shdr_id;
      switch(eType) {
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

      // Compile shaders 
      CHECK_GL(glShaderSource, shdr_id, 1, &shdr_Cstr, NULL);
      CHECK_GL(glCompileShader, shdr_id);

      int result, log_length;
      CHECK_GL(glGetShaderiv, shdr_id, GL_COMPILE_STATUS, &result);

      if(result != GL_TRUE) {

        CHECK_GL(glGetShaderiv, shdr_id, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> shader_err_msg(log_length);
        CHECK_GL(glGetShaderInfoLog, shdr_id, log_length, NULL, shader_err_msg.data());
	std::cout << "################Shader  " << file_name  <<  "  Compilation Information ###############" << std::endl;
        for(auto &c: shader_err_msg)
	  std::cout << c;
	std::cout << "##############End of shader compile Informatin#####"<< std::endl;
        assert(false);
        exit(-1);
      }


      // Attach and link 
      CHECK_GL(glAttachShader, prog_id, shdr_id);
      CHECK_GL(glLinkProgram, prog_id);


      CHECK_GL(glGetProgramiv, prog_id, GL_LINK_STATUS,&result);

      if(result != GL_TRUE) {

        CHECK_GL(glGetProgramiv, prog_id, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> program_err_msg(log_length);
        CHECK_GL(glGetProgramInfoLog, prog_id, log_length, NULL, program_err_msg.data());
        std::cout << "################Shader Linking  Information ###############" << std::endl;
        for(auto &c: program_err_msg)
	  std::cout << c;
	std::cout << "##############End of shader Linking Informatin#####"<< std::endl;
        assert(false);
        exit(-1);
      }

      GPUProgram gpu_prog; 
      gpu_prog._prog_id = prog_id;
      gpu_prog._shader = std::make_tuple(eType, shdr_id);
    
      std::pair<std::string, GPUProgram> temp(file_name, gpu_prog);
      _gpu_programs.insert(temp);

      return gpu_prog;
    } // end of else     
  }

  GPUCompute::~GPUCompute() {
    for(auto it = _gpu_programs.cbegin(); it != _gpu_programs.cend(); it++)
      CHECK_GL(glDeleteProgram, it->second._prog_id);
    _gpu_programs.clear();
  }

} // namespace ogl
