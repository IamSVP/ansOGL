#version 430 core
// single decoder local_size_x = 1, local_size_y = 1, local_size_z = 1
layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(std430) buffer;

#define ANS_TABLE_SIZE_LOG  11
#define ANS_TABLE_SIZE      (1 << ANS_TABLE_SIZE_LOG)
#define NUM_ENCODED_SYMBOLS 784
#define ANS_DECODER_K       (1 << 4)
#define ANS_DECODER_L       (ANS_DECODER_K * ANS_TABLE_SIZE)



struct AnsTableEntry {
  uint freq;
  uint cum_freq;
  uint symbol;
};

layout(binding = 0) readonly buffer in_table {
  AnsTableEntry table [];
};

layout(binding = 1) readonly buffer in_data_stream {
  uint data_stream []; 
};

layout(binding = 2) writeonly buffer out_symbols {
  uint symbols [];
};

layout(binding = 3 ) writeonly buffer out_dummy {
  uint dummy [];
};

shared uint normalization_mask;
// Copy all the frequencies to shared memory 
// gl_WorkGroupID; gl_GlobalInvocationID; gl_LocalInvocationID
// gl_WorkGroupSize, gl_NumWorkGroups

// main fucntion decodes the whole thing
void main() {


  if(0 == gl_LocalInvocationID.x) {
    normalization_mask = 0;
  }
 
  uint stream_group_id = gl_WorkGroupID.x;
  uint offset_val = data_stream[stream_group_id];
  uint state = data_stream[offset_val/4 - gl_WorkGroupSize.x + gl_LocalInvocationID.x];
  // Have to be careful the bytes count to word count conversion
  // 
  uint next_to_read = (offset_val - gl_WorkGroupSize.x * 4) / 4; 

  groupMemoryBarrier();

  uint prev_read_offset = 0; 
  for(int i = 0; i < NUM_ENCODED_SYMBOLS; i++) {
    const uint symbol = state & (ANS_TABLE_SIZE - 1);
    const AnsTableEntry entry = table[symbol];
    state = (state >> ANS_TABLE_SIZE_LOG) * entry.freq 
            - entry.cum_freq + symbol;

    const uint normalization_bit = 
      ( uint(state < ANS_DECODER_L) << gl_LocalInvocationID.x );
    atomicOr(normalization_mask, normalization_bit);

    groupMemoryBarrier();

    uint total_to_read = bitCount(normalization_mask);
    if(normalization_bit != 0) {

      const uint up_to_me_mask = normalization_bit - 1 ;
      uint num_to_skip = total_to_read;
      num_to_skip = num_to_skip - bitCount(normalization_mask & up_to_me_mask) + 1 + prev_read_offset;


      uint shift_mask = ((num_to_skip ^ 1) & 1) * 16;
      num_to_skip = num_to_skip >> 1;

      uint read_data = data_stream[next_to_read - num_to_skip];
      read_data = (read_data >> shift_mask);
      read_data = read_data  & 0x0000FFFF;

      state = (state << 16) | read_data;

    } // end of if condition

    //memoryBarrierShared();
    groupMemoryBarrier();

    atomicAnd(normalization_mask, ~normalization_bit);
    
    total_to_read = total_to_read + prev_read_offset;
    prev_read_offset = total_to_read & 1;
    total_to_read = total_to_read >> 1;

    next_to_read = next_to_read - total_to_read;

    const uint gidx = (gl_LocalInvocationID.x + stream_group_id * gl_WorkGroupSize.x) * NUM_ENCODED_SYMBOLS;

    symbols[gidx + NUM_ENCODED_SYMBOLS - 1 - i] = entry.symbol;

  } // end of for 

}
