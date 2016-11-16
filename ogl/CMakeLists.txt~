include_directories("${ansOGL_SOURCE_DIR}/ogl")

include_directories("${ansOGL_SORUCE_DIR}/external/glm")
include_directories("${ansOGL_SOURCE_DIR}/external/glew-1.13.0/include")
include_directories("${ansOGL_SOURCE_DIR}/external/glfw-3.1/include/GLFW")


set(ALL_GL_LIBS
	${OPENGL_LIBRARY}
	glfw
	GLEW_1130
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)



set( HEADERS
  "gpu.h"
  "gl_guards.h"
  )

set( SOURCES
  "gpu.cpp"
  )


add_library(gpu_ogl ${HEADERS} ${SOURCES})
target_link_libraries(gpu_ogl ${ALL_GL_LIBS})

