#include "ans_ogl.h"
#include "gpu.h"

#include <numeric>
#include <iostream>

#include "histogram.h"
#include "cs_path_config.h"

struct AnsTableEntry {
  uint32_t freq;
  uint32_t cum_freq;
  uint32_t symbol;
};


namespace ans_ogl {

  std::vector<uint32_t> NormalizeFrequencies(const std::vector<uint32_t> &F) {
    return std::move(ans::GenerateHistogram(F, kANSTableSize));
  }

  ans::Options GetOpenGLOptions(const std::vector<uint32_t> &F) {
    ans::Options opts;
    opts.b = 1 << 16;
    opts.k = 1 << 4;
    opts.M = kANSTableSize;
    opts.Fs = F;
    opts.type = ans::eType_rANS;

    return opts;
  }

  std::unique_ptr<ans::Encoder> CreateCPUEncoder(const std::vector<uint32_t> &F) {
    return ans::Encoder::Create(GetOpenGLOptions(F));
  }

  std::unique_ptr<ans::Decoder> CreateCPUDecoder(uint32_t state, const std::vector<uint32_t> &F) {
    return ans::Decoder::Create(state, GetOpenGLOptions(F));
  }


  template<typename T>
  static inline T NextMultipleOfFour(T x) {
    return ((x + 3) >> 2) << 2;
  }

  
  // Constructor 
  OpenGLDecoder::OpenGLDecoder(
      const std::unique_ptr<ogl::GPUCompute> &gpu,
      const std::vector<uint32_t> &F,
      const int num_interleaved)
    : _num_interleaved(num_interleaved)
    , _gpu(gpu)
    , _M(kANSTableSize)
    , _build_table(false)
  {
    // generate buffers for table 

     CHECK_GL(glGenBuffers, 1, &_ssbo_table);
     RebuildTable(F); 
     _build_table = true;
     
  }

  OpenGLDecoder::~OpenGLDecoder() {
    CHECK_GL(glDeleteBuffers, 1, &_ssbo_table);
  }

  void OpenGLDecoder::RebuildTable( const std::vector<uint32_t> &F) {
    
    std::vector<uint32_t> freqs = std::move(NormalizeFrequencies(F));

    assert(_M == std::accumulate(freqs.begin(), freqs.end(), 0U));
    assert(freqs.size() <= 256);
    freqs.resize(256, 0);

    GLuint work_group_size_x = 256;
    
    std::string build_table_cs_path(kANSOpenGLShaders[eANSOpenGLCS_BuildTable]);

    ogl::GPUProgram curr_prog = _gpu->CompileAndLinkShader(build_table_cs_path, ogl::eCompute_shader);

    _gpu->SetArgMemory(_ssbo_table, GL_SHADER_STORAGE_BUFFER, 
	               _M*sizeof(AnsTableEntry), NULL, GL_DYNAMIC_READ);
		     
    GLuint ssbo_freqs;
    CHECK_GL(glGenBuffers, 1, &ssbo_freqs);
    _gpu->SetArgMemory(ssbo_freqs,GL_SHADER_STORAGE_BUFFER,
	               freqs.size()*sizeof(uint32_t), freqs.data(), GL_STREAM_READ);
    //CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, ssbo_freqs);
    //CHECK_GL(glBufferData, GL_SHADER_STORAGE_BUFFER, freqs.size()*sizeof(uint32_t), &freqs[0], GL_DYNAMIC_COPY);
    //CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, 0);



    CHECK_GL(glUseProgram, curr_prog._prog_id);


    _gpu->SetShaderArg(ssbo_freqs, 0, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(_ssbo_table, 1, GL_SHADER_STORAGE_BUFFER);

    

    CHECK_GL(glDispatchCompute, _M/work_group_size_x, 1, 1); 
    // Probaby a barrier not sure yet but a barrier 
    CHECK_GL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);

     CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, ssbo_freqs);
    GLvoid *ptr = CHECK_GL(glMapBuffer, GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    assert(ptr != NULL);

    std::vector<uint32_t> out_table(256);

    memcpy(out_table.data(), ptr, out_table.size() * sizeof(uint32_t));
    CHECK_GL(glUnmapBuffer, GL_SHADER_STORAGE_BUFFER);
    CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, 0);
    CHECK_GL(glDeleteBuffers, 1, &ssbo_freqs);    
  }




  std::vector<uint8_t> OpenGLDecoder::Decode(const uint32_t &state, 
                                             const std::vector<uint8_t> &stream) {
    std::string ans_decode_cs_path(kANSOpenGLShaders[eANSOpenGLCS_ANSDecode]);

    ogl::GPUProgram curr_prog = _gpu->CompileAndLinkShader(ans_decode_cs_path, ogl::eCompute_shader);

    uint32_t offset = NextMultipleOfFour(static_cast<uint32_t>(stream.size() + 8));
    std::vector<uint8_t> all_data(offset, 0);
    memcpy(all_data.data(), &offset, sizeof(offset));
    memcpy(all_data.data() + all_data.size() - stream.size() - 4, stream.data(), stream.size());
    memcpy(all_data.data() + all_data.size() - 4, &state, sizeof(state));

    GLuint ssbo_all_data;
    CHECK_GL(glGenBuffers, 1, &ssbo_all_data);

    _gpu->SetArgMemory(ssbo_all_data, GL_SHADER_STORAGE_BUFFER,
	               all_data.size(), all_data.data(), GL_STREAM_READ);
                                                     
    GLuint ssbo_dummy_debug;
    CHECK_GL(glGenBuffers, 1, &ssbo_dummy_debug);
    _gpu->SetArgMemory(ssbo_dummy_debug, GL_SHADER_STORAGE_BUFFER, 10*sizeof(uint32_t),NULL, GL_STREAM_READ);

    GLuint ssbo_out_symbols;
    CHECK_GL(glGenBuffers, 1, &ssbo_out_symbols);
    _gpu->SetArgMemory(ssbo_out_symbols, GL_SHADER_STORAGE_BUFFER,
	               kNumEncodedSymbols*sizeof(uint32_t), NULL, GL_STREAM_READ);

    //GO WRITE THE SHADER
    //
    CHECK_GL(glUseProgram, curr_prog._prog_id);

    _gpu->SetShaderArg(_ssbo_table, 0, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(ssbo_all_data, 1, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(ssbo_out_symbols, 2, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(ssbo_dummy_debug, 3, GL_SHADER_STORAGE_BUFFER);

    CHECK_GL(glDispatchCompute, 1, 1,1);
    


    CHECK_GL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);
    
    //copyt the symbols 
    std::vector<uint32_t> out_symbols = std::move(_gpu->GetShaderArg<uint32_t>(ssbo_out_symbols, GL_SHADER_STORAGE_BUFFER, kNumEncodedSymbols, GL_READ_ONLY));

    std::vector<uint32_t> debug_data = std::move(_gpu->GetShaderArg<uint32_t>(ssbo_dummy_debug, GL_SHADER_STORAGE_BUFFER, 10, GL_READ_ONLY));
    std::vector<uint8_t> result;
    result.reserve(kNumEncodedSymbols);

    for( auto a : out_symbols)
      result.push_back(static_cast<uint8_t>(a));

    // Delete the buffers
    CHECK_GL(glDeleteBuffers, 1, &ssbo_all_data);
    CHECK_GL(glDeleteBuffers, 1, &ssbo_out_symbols);
  
    return std::move(result);

  }

  std::vector< std::vector<uint8_t> > OpenGLDecoder::Decode(const std::vector<uint32_t> &states,
                                             const std::vector<uint8_t> &stream) {

    std::string ans_decode_cs_path(kANSOpenGLShaders[eANSOpenGLCS_ANSDecode]);

    ogl::GPUProgram curr_prog = _gpu->CompileAndLinkShader(ans_decode_cs_path, ogl::eCompute_shader);
    uint32_t offset = NextMultipleOfFour(static_cast<uint32_t>(stream.size() + 4 + states.size()*4));
    std::vector<uint8_t> all_data(offset,0);
    memcpy(all_data.data(), &offset, sizeof(offset));
    memcpy(all_data.data() + all_data.size() - 4 * states.size() - stream.size(), stream.data(), stream.size());
    memcpy(all_data.data() + all_data.size() - 4*states.size(), states.data(), 4*states.size());


    const size_t total_symbols = kNumEncodedSymbols * states.size();
    const size_t num_streams = states.size();
    const size_t streams_per_work_group = num_streams;

    GLuint ssbo_all_data;
    CHECK_GL(glGenBuffers, 1, &ssbo_all_data);

    _gpu->SetArgMemory(ssbo_all_data, GL_SHADER_STORAGE_BUFFER,
	               all_data.size(), all_data.data(), GL_STREAM_READ);
                                                     
    GLuint ssbo_dummy_debug;
    CHECK_GL(glGenBuffers, 1, &ssbo_dummy_debug);
    _gpu->SetArgMemory(ssbo_dummy_debug, GL_SHADER_STORAGE_BUFFER, 10*sizeof(uint32_t),NULL, GL_STREAM_READ);

    GLuint ssbo_out_symbols;
    CHECK_GL(glGenBuffers, 1, &ssbo_out_symbols);
    _gpu->SetArgMemory(ssbo_out_symbols, GL_SHADER_STORAGE_BUFFER,
	              total_symbols*sizeof(uint32_t), NULL, GL_STREAM_READ);

    //GO WRITE THE SHADER
    //
    CHECK_GL(glUseProgram, curr_prog._prog_id);

    _gpu->SetShaderArg(_ssbo_table, 0, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(ssbo_all_data, 1, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(ssbo_out_symbols, 2, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(ssbo_dummy_debug, 3, GL_SHADER_STORAGE_BUFFER);

    CHECK_GL(glDispatchCompute, 1, 1,1);
    


    CHECK_GL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);
    std::vector<uint32_t> out_symbols = std::move(_gpu->GetShaderArg<uint32_t>(ssbo_out_symbols, GL_SHADER_STORAGE_BUFFER, total_symbols, GL_READ_ONLY));

    std::vector<uint32_t> debug_data = std::move(_gpu->GetShaderArg<uint32_t>(ssbo_dummy_debug, GL_SHADER_STORAGE_BUFFER, 10, GL_READ_ONLY));
    std::vector<uint8_t> result;
    result.reserve(total_symbols);

    for( auto a : out_symbols)
      result.push_back(static_cast<uint8_t>(a));

    // Delete the buffers
    CHECK_GL(glDeleteBuffers, 1, &ssbo_all_data);
    CHECK_GL(glDeleteBuffers, 1, &ssbo_out_symbols);

    std::vector< std::vector<uint8_t> > out;
    out.reserve(num_streams);
    for(size_t i = 0; i < num_streams; i++) {
      std::vector<uint8_t> st;
      size_t stream_start = i*kNumEncodedSymbols;
      size_t stream_end = stream_start + kNumEncodedSymbols;
      st.insert(st.begin(), result.begin() +stream_start, result.begin() + stream_end);
      out.push_back(std::move(st));
    }
    // Delete the buffers
    CHECK_GL(glDeleteBuffers, 1, &ssbo_all_data);
    CHECK_GL(glDeleteBuffers, 1, &ssbo_out_symbols);

    return std::move(out);
 
  } 
  std::vector< std::vector<uint8_t> > OpenGLDecoder::Decode(const std::vector<uint32_t> &states,
                                                            const std::vector< std::vector<uint8_t> > &stream) {

    const size_t total_streams = states.size();
    const size_t streams_per_work_group = _num_interleaved;
    const size_t total_work_groups = total_streams / streams_per_work_group;
    std::string ans_decode_cs_path(kANSOpenGLShaders[eANSOpenGLCS_ANSDecode]);
    ogl::GPUProgram curr_prog = _gpu->CompileAndLinkShader(ans_decode_cs_path, ogl::eCompute_shader);

    std::vector<uint32_t> offsets;
    std::vector<uint8_t> all_data;

    size_t states_offset = 0;

    for( const auto &strm : stream) {
      size_t last = all_data.size();
      size_t sz_for_strm = NextMultipleOfFour(strm.size()) + 4*streams_per_work_group;
      all_data.resize(last + sz_for_strm);

      uint8_t *ptr = all_data.data() + last + sz_for_strm;
      ptr -= 4 * streams_per_work_group;
      memcpy(ptr, &states[states_offset], 4 * streams_per_work_group);
      ptr -= strm.size();
      memcpy(ptr, strm.data(), strm.size());
      states_offset += streams_per_work_group;

      offsets.push_back(static_cast<uint32_t>(last + sz_for_strm));
    }

    for(auto &off : offsets) {
      off += static_cast<uint32_t>(4 * offsets.size());
    }

    const uint8_t *offsets_start = reinterpret_cast<const uint8_t*>(offsets.data());
    const uint8_t *offsets_end = offsets_start + 4 * offsets.size();
    all_data.insert(all_data.begin(), offsets_start, offsets_end);

    size_t total_symbols = total_streams * kNumEncodedSymbols;

    GLuint ssbo_all_data;
    CHECK_GL(glGenBuffers, 1, &ssbo_all_data);

    _gpu->SetArgMemory(ssbo_all_data, GL_SHADER_STORAGE_BUFFER,
	               all_data.size(), all_data.data(), GL_STREAM_READ);
                                                     
    GLuint ssbo_dummy_debug;
    CHECK_GL(glGenBuffers, 1, &ssbo_dummy_debug);
    _gpu->SetArgMemory(ssbo_dummy_debug, GL_SHADER_STORAGE_BUFFER, 10*sizeof(uint32_t),NULL, GL_STREAM_READ);

    GLuint ssbo_out_symbols;
    CHECK_GL(glGenBuffers, 1, &ssbo_out_symbols);
    _gpu->SetArgMemory(ssbo_out_symbols, GL_SHADER_STORAGE_BUFFER,
	              total_symbols*sizeof(uint32_t), NULL, GL_STREAM_READ);

    //GO WRITE THE SHADER
    //
    CHECK_GL(glUseProgram, curr_prog._prog_id);

    _gpu->SetShaderArg(_ssbo_table, 0, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(ssbo_all_data, 1, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(ssbo_out_symbols, 2, GL_SHADER_STORAGE_BUFFER);
    _gpu->SetShaderArg(ssbo_dummy_debug, 3, GL_SHADER_STORAGE_BUFFER);

    CHECK_GL(glDispatchCompute, total_work_groups, 1,1);
    


    //CHECK_GL(glMemoryBarrier, GL_SHADER_STORAGE_BARRIER_BIT);
    std::vector<uint32_t> out_symbols = std::move(_gpu->GetShaderArg<uint32_t>(ssbo_out_symbols, GL_SHADER_STORAGE_BUFFER, total_symbols, GL_READ_ONLY));

    std::vector<uint32_t> debug_data = std::move(_gpu->GetShaderArg<uint32_t>(ssbo_dummy_debug, GL_SHADER_STORAGE_BUFFER, 10, GL_READ_ONLY));

    std::vector<uint8_t> result;
    result.reserve(total_symbols);

    for( auto a : out_symbols)
      result.push_back(static_cast<uint8_t>(a));

    // Delete the buffers
    CHECK_GL(glDeleteBuffers, 1, &ssbo_all_data);
    CHECK_GL(glDeleteBuffers, 1, &ssbo_out_symbols);

    std::vector< std::vector<uint8_t> > out;
    out.reserve(total_streams);

    for(size_t i = 0; i < total_streams; i++) {

      std::vector<uint8_t> st;
      size_t stream_start = i*kNumEncodedSymbols;
      size_t stream_end = stream_start + kNumEncodedSymbols;
      st.insert(st.begin(), result.begin() +stream_start, result.begin() + stream_end);
      out.push_back(std::move(st));

    }
    // Delete the buffers
    CHECK_GL(glDeleteBuffers, 1, &ssbo_all_data);
    CHECK_GL(glDeleteBuffers, 1, &ssbo_out_symbols);

    return std::move(out);
   
  }


  std::vector<uint8_t> OpenGLDecoder::GetSymbols() const {

    CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, _ssbo_table);
    GLvoid *ptr = CHECK_GL(glMapBuffer, GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    assert(ptr != NULL);

    std::vector<AnsTableEntry> out_table(_M);

    memcpy(out_table.data(), ptr,out_table.size() * sizeof(AnsTableEntry));
    CHECK_GL(glUnmapBuffer, GL_SHADER_STORAGE_BUFFER);

    std::vector<uint8_t> result;
    result.reserve(out_table.size());

    for(auto entry : out_table) {
      result.push_back(static_cast<uint8_t>(entry.symbol));
    }

    CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, 0);

    return std::move(result);
  }

  std::vector<uint16_t> OpenGLDecoder::GetFrequencies() const {

    CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, _ssbo_table);
    GLvoid *ptr = CHECK_GL(glMapBuffer, GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    assert(ptr != NULL);

    std::vector<AnsTableEntry> out_table(_M);

    memcpy(out_table.data(), ptr, out_table.size() * sizeof(AnsTableEntry));
    CHECK_GL(glUnmapBuffer, GL_SHADER_STORAGE_BUFFER);

    std::vector<uint16_t> result;
    result.reserve(out_table.size());

    for(auto entry : out_table) {
      result.push_back(static_cast<uint16_t>(entry.freq));
    }

    CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, 0);

    return std::move(result);

  }

  std::vector<uint16_t> OpenGLDecoder::GetCumulativeFrequencies() const {
    CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, _ssbo_table);
    GLvoid *ptr = CHECK_GL(glMapBuffer, GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    assert(ptr != NULL);
    std::vector<AnsTableEntry> out_table(_M);

    memcpy(out_table.data(), ptr, out_table.size() * sizeof(AnsTableEntry));

    std::vector<uint16_t> result;
    result.reserve(out_table.size());

    for(auto entry : out_table) {
      result.push_back(static_cast<uint16_t>(entry.cum_freq));
    }

    CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, 0);

    return std::move(result);

  }

  

} // namespace ans_ogl 
