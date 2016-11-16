#include "gtest/gtest.h"

#include <numeric>
#include <vector>
#include <ctime>

#include "bits.h"
#include "ans_ogl.h"

using ans_ogl::OpenGLDecoder;
using ans_ogl::kANSTableSize;
using ans_ogl::kNumEncodedSymbols;

/*class OpenGLEnvironment : public ::testing::Environment {
public:
  static OpenGLEnvironment* Initialize() { 

    if(_ptr == NULL) {
       _ptr = new OpenGLEnvironment();
    }
      return _ptr;
  
  }
  virtual ~OpenGLEnvironment() { }
  
  virtual void SetUp() { }
  virtual void TearDown() {
    _gpu = nullptr;
    is_setup = false;
  }

  const std::unique_ptr<ogl::GPUCompute> &GetContext() const { assert(is_setup);  return _gpu; }

private:
  bool is_setup;
  OpenGLEnvironment() {
    _gpu = std::move(ogl::GPUCompute::InitializeGL());
    // Make sure to always initialize random number generator for
    // deterministic tests
    srand(0);
    is_setup = true;

  };
  std::unique_ptr<ogl::GPUCompute> _gpu;
  static OpenGLEnvironment *_ptr;

} ;

OpenGLEnvironment *gTestEnv;*/


static std::vector<uint8_t> GenerateSymbols(const std::vector<uint32_t> &freqs, size_t num_symbols) {
  assert(freqs.size() < 256);
  std::vector<uint8_t> symbols;
  symbols.reserve(num_symbols);
  srand(time(NULL));
  for (size_t i = 0; i < num_symbols; ++i) {
    uint32_t M = std::accumulate(freqs.begin(), freqs.end(), 0);
    int r = rand() % M;
    uint8_t symbol = 0;
    int freq = 0;
    for (auto f : freqs) {
      freq += f;
      if (r < freq) {
        break;
      }
      symbol++;
    }
    assert(symbol < freqs.size());
    symbols.push_back(symbol);
  }

  return std::move(symbols);
}


/*TEST(ANS_OGL, Initialization) {

  std::vector<uint32_t> F = { 3, 2, 1, 4, 3, 4, 6, 4, 11,15,9,5,3,1 };
  std::unique_ptr<ogl::GPUCompute> gpu = ogl::GPUCompute::InitializeGL();
  OpenGLDecoder decoder(gpu, F, 1);

  std::vector<uint32_t> normalized_F = ans::GenerateHistogram(F, kANSTableSize);

  std::vector<uint8_t> expected_symbols(kANSTableSize, 0);
  std::vector<uint16_t> expected_frequencies(kANSTableSize, 0);
  std::vector<uint16_t> expected_cumulative_frequencies(kANSTableSize, 0);

  int sum = 0;
  for (size_t i = 0; i < normalized_F.size(); ++i) {
    for (uint32_t j = 0; j < normalized_F[i]; ++j) {
      expected_symbols[sum + j] = static_cast<uint8_t>(i);
	    assert(normalized_F[i] < std::numeric_limits<uint16_t>::max());
      expected_frequencies[sum + j] = static_cast<uint16_t>(normalized_F[i]);
      expected_cumulative_frequencies[sum + j] = sum;
    }
    sum += normalized_F[i];
  }
  ASSERT_EQ(sum, kANSTableSize);

  std::vector<uint8_t> symbols = std::move(decoder.GetSymbols());
  std::vector<uint16_t> frequencies = std::move(decoder.GetFrequencies());
  std::vector<uint16_t> cumulative_frequencies = std::move(decoder.GetCumulativeFrequencies());

  ASSERT_EQ(symbols.size(), expected_symbols.size());
  for (size_t i = 0; i < symbols.size(); ++i) {
    EXPECT_EQ(expected_symbols[i], symbols[i]);   }

  ASSERT_EQ(frequencies.size(), expected_frequencies.size());
  for (size_t i = 0; i < frequencies.size(); ++i) {
   EXPECT_EQ(expected_frequencies[i], frequencies[i]);   }

  ASSERT_EQ(cumulative_frequencies.size(), expected_cumulative_frequencies.size());
 for (size_t i = 0; i < cumulative_frequencies.size(); ++i) {
  EXPECT_EQ(expected_cumulative_frequencies[i], cumulative_frequencies[i]);
  }

} */

/*TEST(ANS_OpenGL, DecodeSingleStream) {
  std::vector<uint32_t> F = { 12, 14, 17, 1, 1, 2, 372 };

  std::vector<uint8_t> symbols = std::move(GenerateSymbols(F, kNumEncodedSymbols));
  ASSERT_EQ(symbols.size(), kNumEncodedSymbols);

  std::unique_ptr<ans::Encoder> encoder = ans_ogl::CreateCPUEncoder(F);
  std::vector<uint8_t> stream(10);

  size_t bytes_written = 0;
  for (auto symbol : symbols) {
    ans::BitWriter w(stream.data() + bytes_written);
    encoder->Encode(symbol, &w);

    bytes_written += w.BytesWritten();
    if (bytes_written > (stream.size() / 2)) {
      stream.resize(stream.size() * 2);
    }
  }

  stream.resize(bytes_written);
  ASSERT_EQ(bytes_written % 2, 0);

  // First, make sure we can CPU decode it.
  std::vector<uint16_t> short_stream;
  short_stream.reserve(bytes_written / 2);
  for (size_t i = 0; i < bytes_written; i += 2) {
    uint16_t x = (static_cast<uint16_t>(stream[i + 1]) << 8) | stream[i];
    short_stream.push_back(x);
  }
  std::reverse(short_stream.begin(), short_stream.end());

  std::vector<uint8_t> cpu_decoded_symbols;
  cpu_decoded_symbols.reserve(kNumEncodedSymbols);

  ans::BitReader r(reinterpret_cast<const uint8_t *>(short_stream.data()));
  std::unique_ptr<ans::Decoder> cpu_decoder = ans_ogl::CreateCPUDecoder(encoder->GetState(), F);
  for (size_t i = 0; i < kNumEncodedSymbols; ++i) {
    cpu_decoded_symbols.push_back(static_cast<uint8_t>(cpu_decoder->Decode(&r)));
  }
  std::reverse(cpu_decoded_symbols.begin(), cpu_decoded_symbols.end()); // Decode in reverse
  ASSERT_EQ(cpu_decoded_symbols.size(), kNumEncodedSymbols);
  for (size_t i = 0; i < kNumEncodedSymbols; ++i) {
    EXPECT_EQ(cpu_decoded_symbols[i], symbols[i]) << "Symbols differ at index " << i;
  }

  // Now make sure we can GPU decode it
  std::unique_ptr<ogl::GPUCompute> gpu = ogl::GPUCompute::InitializeGL();

  ans_ogl::OpenGLDecoder decoder(gpu, F, 1);
  std::vector<uint8_t> decoded_symbols = std::move(decoder.Decode(encoder->GetState(), stream));
  ASSERT_EQ(decoded_symbols.size(), kNumEncodedSymbols);
  for (size_t i = 0; i < kNumEncodedSymbols; ++i) {
    EXPECT_EQ(decoded_symbols[i], symbols[i]) << "Symbols differ at index " << i;
  } 

  
  // Make sure we can decode it twice...
  ans_ogl::OpenGLDecoder decoder2(gpu, F, 1);
  decoded_symbols = std::move(decoder2.Decode(encoder->GetState(), stream));
  ASSERT_EQ(decoded_symbols.size(), kNumEncodedSymbols);
  for (size_t i = 0; i < kNumEncodedSymbols; ++i) {
    EXPECT_EQ(decoded_symbols[i], symbols[i]) << "Symbols differ at index " << i;
  }
}*/


TEST(ANS_OpenGL, TableRebuilding) {
  std::unique_ptr<ogl::GPUCompute> gpu = ogl::GPUCompute::InitializeGL();

  std::vector<uint32_t> F = { 3, 2, 1, 4, 3, 406 };
  OpenGLDecoder decoder(gpu, F, 1);

  std::vector<uint32_t> new_F = { 80, 300, 2, 14, 1, 1, 1, 20 };
  decoder.RebuildTable(new_F);

  std::vector<uint32_t> normalized_F = ans::GenerateHistogram(new_F, kANSTableSize);

  std::vector<uint8_t> expected_symbols(kANSTableSize, 0);
  std::vector<uint16_t> expected_frequencies(kANSTableSize, 0);
  std::vector<uint16_t> expected_cumulative_frequencies(kANSTableSize, 0);

  int sum = 0;
  for (size_t i = 0; i < normalized_F.size(); ++i) {
    for (uint32_t j = 0; j < normalized_F[i]; ++j) {
      expected_symbols[sum + j] = static_cast<uint8_t>(i);
	  assert(normalized_F[i] < std::numeric_limits<uint16_t>::max());
	  expected_frequencies[sum + j] = static_cast<uint16_t>(normalized_F[i]);
      expected_cumulative_frequencies[sum + j] = sum;
    }
    sum += normalized_F[i];
  }
  ASSERT_EQ(sum, kANSTableSize);

  ASSERT_EQ(expected_symbols.size(), kANSTableSize);
  ASSERT_EQ(expected_frequencies.size(), kANSTableSize);
  ASSERT_EQ(expected_cumulative_frequencies.size(), kANSTableSize);

  std::vector<uint8_t> symbols = std::move(decoder.GetSymbols());
  std::vector<uint16_t> frequencies = std::move(decoder.GetFrequencies());
  std::vector<uint16_t> cumulative_frequencies = std::move(decoder.GetCumulativeFrequencies());

  ASSERT_EQ(symbols.size(), expected_symbols.size());
  for (size_t i = 0; i < symbols.size(); ++i) {
    EXPECT_EQ(expected_symbols[i], symbols[i]) << "Symbols differ at index " << i;
  }

  ASSERT_EQ(frequencies.size(), expected_frequencies.size());
  for (size_t i = 0; i < frequencies.size(); ++i) {
    EXPECT_EQ(expected_frequencies[i], frequencies[i]) << "Frequencies differ at index " << i;
  }

  ASSERT_EQ(cumulative_frequencies.size(), expected_cumulative_frequencies.size());
  for (size_t i = 0; i < cumulative_frequencies.size(); ++i) {
   EXPECT_EQ(expected_cumulative_frequencies[i], cumulative_frequencies[i])
      << "Cumulative frequencies differ at index " << i;
  }
}

/*TEST(ANS_OpenGL, DecodeInterleavedStreams) {
  std::vector<uint32_t> F = { 32, 186, 54, 8, 94, 35, 13, 21, 456, 789, 33, 215, 6, 54, 987, 54, 65, 13, 2, 1 };
  const size_t num_interleaved = 16;

  std::vector<uint8_t> symbols[num_interleaved];
  for (size_t i = 0; i < num_interleaved; ++i) {
    symbols[i] = std::move(GenerateSymbols(F, kNumEncodedSymbols));
    ASSERT_EQ(symbols[i].size(), kNumEncodedSymbols);
  }

  std::vector<std::unique_ptr<ans::Encoder> > encoders;
  encoders.reserve(num_interleaved);
  for (size_t i = 0; i < num_interleaved; ++i) {
    encoders.push_back(std::move(ans_ogl::CreateCPUEncoder(F)));
  }

  std::vector<uint8_t> stream(10);

  size_t bytes_written = 0;
  for (size_t sym_idx = 0; sym_idx < kNumEncodedSymbols; ++sym_idx) {
    for (size_t strm_idx = 0; strm_idx < num_interleaved; ++strm_idx) {
      ans::BitWriter w(stream.data() + bytes_written);
      encoders[strm_idx]->Encode(symbols[strm_idx][sym_idx], &w);

      bytes_written += w.BytesWritten();
      if (bytes_written >(stream.size() / 2)) {
        stream.resize(stream.size() * 2);
      }
    }
  }
  stream.resize(bytes_written);
  ASSERT_EQ(bytes_written % 2, 0);

  // Let's get our states...
  std::vector<uint32_t> states(num_interleaved, 0);
  std::transform(encoders.begin(), encoders.end(), states.begin(),
    [](const std::unique_ptr<ans::Encoder> &enc) { return enc->GetState(); }
  );

  // Now decode it!
  std::unique_ptr<ogl::GPUCompute> gpu = ogl::GPUCompute::InitializeGL();

  ans_ogl::OpenGLDecoder decoder(gpu, F, num_interleaved);
  std::vector<std::vector<uint8_t> > decoded_symbols = std::move(decoder.Decode(states, stream));
  ASSERT_EQ(decoded_symbols.size(), num_interleaved);
  for (size_t strm_idx = 0; strm_idx < num_interleaved; ++strm_idx) {
    ASSERT_EQ(decoded_symbols[strm_idx].size(), kNumEncodedSymbols)
      << "Issue with decoded stream at index: " << strm_idx;
  }

  for (size_t sym_idx = 0; sym_idx < kNumEncodedSymbols; ++sym_idx) {
    for (size_t strm_idx = 0; strm_idx < num_interleaved; ++strm_idx) {
      EXPECT_EQ(decoded_symbols[strm_idx][sym_idx], symbols[strm_idx][sym_idx])
        << "Symbols differ at stream and index: (" << strm_idx
        << ", " << sym_idx << ")";
    }
  }
}*/

TEST(ANS_OpenGL, DecodeMultipleInterleavedStreams) {
  std::vector<uint32_t> F = { 65, 4, 6, 132, 135, 64, 879, 87, 456, 13, 2, 12, 33, 16, 546, 987, 98, 74, 65, 43, 21, 32, 1 };
  const size_t num_interleaved = 32;
  const size_t num_independent_data_streams = 8;
  const size_t total_num_streams = num_interleaved * num_independent_data_streams;

  std::vector<uint8_t> symbols[total_num_streams];
  std::vector<std::unique_ptr<ans::Encoder> > encoders;
  encoders.reserve(total_num_streams);

  for (size_t i = 0; i < total_num_streams; ++i) {
    symbols[i] = std::move(GenerateSymbols(F, kNumEncodedSymbols));
    ASSERT_EQ(symbols[i].size(), kNumEncodedSymbols);
    encoders.push_back(std::move(ans_ogl::CreateCPUEncoder(F)));
  }

  std::vector<std::vector<uint8_t> > data_streams =
    std::vector<std::vector<uint8_t> >(
      num_independent_data_streams, std::vector<uint8_t>(10, 0));

  for (size_t grp_idx = 0; grp_idx < num_independent_data_streams; ++grp_idx) {

    std::vector<uint8_t> &stream = data_streams[grp_idx];

    size_t bytes_written = 0;
    const size_t enc_idx = grp_idx * num_interleaved;
    for (size_t sym_idx = 0; sym_idx < kNumEncodedSymbols; ++sym_idx) {
      for (size_t strm_idx = 0; strm_idx < num_interleaved; ++strm_idx) {
        ans::BitWriter w(stream.data() + bytes_written);
        encoders[enc_idx + strm_idx]->Encode(symbols[enc_idx + strm_idx][sym_idx], &w);

        bytes_written += w.BytesWritten();
        if (bytes_written >(stream.size() / 2)) {
          stream.resize(stream.size() * 2);
        }
      }
    }

    stream.resize(bytes_written);
    ASSERT_EQ(bytes_written % 2, 0);
  }

  // Let's get our states...
  std::vector<uint32_t> states(total_num_streams, 0);
  std::transform(encoders.begin(), encoders.end(), states.begin(),
    [](const std::unique_ptr<ans::Encoder> &enc) { return enc->GetState(); }
  );

  // Now decode it!
  std::unique_ptr<ogl::GPUCompute> gpu = ogl::GPUCompute::InitializeGL();

  ans_ogl::OpenGLDecoder decoder(gpu, F, num_interleaved);
  std::vector<std::vector<uint8_t> > decoded_symbols =
    std::move(decoder.Decode(states, data_streams));

  ASSERT_EQ(decoded_symbols.size(), total_num_streams);
  for (size_t strm_idx = 0; strm_idx < total_num_streams; ++strm_idx) {
    ASSERT_EQ(decoded_symbols[strm_idx].size(), kNumEncodedSymbols)
      << "Issue with decoded stream at index: ("
      << (strm_idx / num_interleaved) << ", "
      << (strm_idx % num_interleaved) << ")";

    for (size_t sym_idx = 0; sym_idx < kNumEncodedSymbols; ++sym_idx) {
      EXPECT_EQ(decoded_symbols[strm_idx][sym_idx], symbols[strm_idx][sym_idx])
        << "Symbols differ at stream and index: ("
        << (strm_idx / num_interleaved) << ", "
        << (strm_idx % num_interleaved) << "): " << sym_idx;
    }
  }
}

