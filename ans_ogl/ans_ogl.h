#ifndef __ANS_OGL_H__
#define __ANS_OGL_H__

#include <cstdlib>
#include <cassert>
#include <stdint.h>
#include <vector>

// Include opengl wrapper 
#include <GL/glew.h>

// Include Window management library
#include <glfw3.h>

// Include glm 
#include <glm/glm.hpp>

#include "ans.h"
#include "histogram.h"
#include "gpu.h"


// ASSUMES THE FOLLOWING 
// 1. OpenGL context is already created by the usage class 
// 2. Encoded Stream already exists 
// 3. 


// ToDo 
// 1. Takes input the state and data-stream and decoded
// 2. Create compute program
// 3. Compile and link the program to shader program
// 4. build_table shader and decode shader
// 5. Can not write multiple shaders and disptach them 
// 6. Figure out data-tyes that can be used
// 7. Reuse the files and structure from GST(GenTC), don't waste time on figuring that again!
// !!!! Need to add copy opengl mem flags !! 
namespace ans_ogl {

static const size_t kANSTableSize = (1 << 11);
static const size_t kNumEncodedSymbols = 784;
static const size_t kThreadsPerEncodingGroup = 32;

std::vector<uint32_t> NormalizeFrequencies( const std::vector<uint32_t> &F);
ans::Options GetOpenGLOptions( const std::vector<uint32_t> &F);
std::unique_ptr<ans::Encoder> CreateCPUEncoder(const std::vector<uint32_t> &F);
std::unique_ptr<ans::Decoder> CreateCPUDecoder(uint32_t state, const std::vector<uint32_t> &F);

class OpenGLDecoder {
 public:
   OpenGLDecoder(
     const std::unique_ptr<ogl::GPUCompute> &gpu,
     const std::vector<uint32_t> &F,
     const int num_interleaved);
   ~OpenGLDecoder();

   std::vector<uint8_t> Decode(const uint32_t &state,
                                    const std::vector<uint8_t> &stream);
   std::vector< std::vector<uint8_t> > Decode(const std::vector<uint32_t> &states,
                                const std::vector<uint8_t> &stream);
   std::vector< std::vector<uint8_t> > Decode(const std::vector<uint32_t> &states,
                                              const std::vector< std::vector<uint8_t> > &stream);


   void RebuildTable(const std::vector<uint32_t> &F);


   std::vector<uint8_t> GetSymbols() const;
   std::vector<uint16_t> GetFrequencies() const;
   std::vector<uint16_t> GetCumulativeFrequencies() const;

 private:
   OpenGLDecoder(const OpenGLDecoder &); // Disallow copy constructor
   
   const int _num_interleaved;
   const size_t _M;
const std::unique_ptr<ogl::GPUCompute> &_gpu;

   GLuint _ssbo_table;
   bool _build_table;

 
};

} // namespace ans_ogl


#endif // __ANS_OGL_H__ 
