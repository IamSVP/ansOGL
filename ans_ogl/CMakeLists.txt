include_directories("${ansOGL_SOURCE_DIR}/ans_ogl")
include_directories("${ansOGL_BINARY_DIR}/ans_ogl")

include_directories("${ansOGL_SOURCE_DIR}/external/glm")
include_directories("${ansOGL_SOURCE_DIR}/external/glew-1.13.0/include")
include_directories("${ansOGL_SOURCE_DIR}/external/glfw-3.1.2/inclue/GLFW")

set( TEST_CS_PATH ${ansOGL_SOURCE_DIR}/ans_ogl/first.cs ) 

SET( SHADERS 
  ${TEST_CS_PATH}
)




