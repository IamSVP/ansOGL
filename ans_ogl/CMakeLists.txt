include_directories("${ansOGL_SOURCE_DIR}/ans_ogl")
include_directories("${ansOGL_BINARY_DIR}/ans_ogl")

include_directories("${ansOGL_SOURCE_DIR}/ogl")

include_directories("${ansOGL_SOURCE_DIR}/external/glm")
include_directories("${ansOGL_SOURCE_DIR}/external/glew-1.13.0/include")
include_directories("${ansOGL_SOURCE_DIR}/external/glfw-3.1/include/GLFW")

set(BUILD_TABLE_CS_PATH ${ansOGL_SOURCE_DIR}/ans_ogl/build_table.cs)
set(ANS_DECODE_CS_PATH ${ansOGL_SOURCE_DIR}/ans_ogl/ans_decode.cs)

CONFIGURE_FILE(
  "cs_path_config.h.in"
  "cs_path_config.h"
)

set(CS
  ${BUILD_TABLE_CS_PATH}
  ${ANS_DECODE_CS_PATH}
  )

set( HEADERS
  "ans.h"
  "cs_path_config.h"
  "ans_ogl.h"
  "ans_utils.h"
  "bits.h"
  "histogram.h"
)

set( SOURCES
  "ans_ogl.cpp"
  "decode.cpp"
  "encode.cpp"
  "histogram.cpp"
)

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


IF( MSVC )
  ADD_DEFINITIONS(/D _SCL_SECURE_NO_WARNINGS)
ENDIF()

source_group(OpenGL_CS FILES ${CS})
add_library(ans_ogl ${HEADERS} ${SOURCES} ${CS} )
target_link_libraries(ans_ogl gpu_ogl)
target_link_libraries(ans_ogl ${ALL_GL_LIBS})

INCLUDE_DIRECTORIES(${ansOGL_SOURCE_DIR}/googletest/include)

# Summary of tuple support for Microsoft Visual Studio:
# Compiler    version(MS)  version(cmake)  Support
# ----------  -----------  --------------  -----------------------------
# <= VS 2010  <= 10        <= 1600         Use Google Tests's own tuple.
# VS 2012     11           1700            std::tr1::tuple + _VARIADIC_MAX=10
# VS 2013     12           1800            std::tr1::tuple
if (MSVC AND MSVC_VERSION EQUAL 1700)
  add_definitions(/D _VARIADIC_MAX=10)
endif()

FOREACH(TEST "ans_ogl")
  ADD_EXECUTABLE(${TEST}_test ${TEST}_test.cpp)
  TARGET_LINK_LIBRARIES(${TEST}_test ans_ogl)
  TARGET_LINK_LIBRARIES(${TEST}_test gtest)

  TARGET_LINK_LIBRARIES(${TEST}_test gpu_ogl)
  TARGET_LINK_LIBRARIES(${TEST}_test gtest_main)

  ADD_TEST(Test_${TEST} ${TEST}_test)
ENDFOREACH()







