// Compute shader to build table for indexing  Bs <= (state%M) < Bs+1 
//

#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
layout(std430) buffer; //sets the default layout for SSBO

#define ANS_TABLE_SIZE_LOG 11
#define MAX_NUM_SYMBOLS 256

struct AnsTableEntry {
  uint freq;
  uint cum_freq;
  uint symbol;
};

layout(binding = 0) buffer in_frequencies {

  uint frequencies [];
}; 

layout(binding = 1) writeonly buffer out_table {

  AnsTableEntry table [];
};

shared uint cumulative_frequencies[MAX_NUM_SYMBOLS]; 
void main() {

  // Copy all the frequencies to shared memory 
  // gl_WorkGroupID; gl_GlobalInvocationID; gl_LocalInvocationID
  // gl_WorkGroupSize, gl_NumWorkGroups
  const uint num_symbols = MAX_NUM_SYMBOLS; 
  uint lid = gl_LocalInvocationID.x;

  if(lid < num_symbols) {
    uint gidx = lid + gl_WorkGroupSize.x * gl_GlobalInvocationID.y; // !!Could be wrong! 
    cumulative_frequencies[lid] = frequencies[gidx];
  }

  groupMemoryBarrier();

  // Scan to compute in-memory sum of cumulative frequencies 

  const int n = MAX_NUM_SYMBOLS;
  int offset = 1;
  for(int d = n >> 1; d > 0; d >>= 1) {
    groupMemoryBarrier();
    if(lid < d) {
      uint ai = offset * (2 * lid + 1) - 1;
      uint bi = offset * (2 * lid + 2) - 1;
      cumulative_frequencies[bi] += cumulative_frequencies[ai];
    }
    offset *= 2;
  }

  if(lid == 0) {
    cumulative_frequencies[n-1] = 0;
  }

  for(int d = 1; d < n; d *= 2) {
    offset >>= 1;
    groupMemoryBarrier();
    if(lid < d) {
      uint ai = offset * (2 * lid + 1) - 1;
      uint bi = offset * (2 * lid + 2) - 1;
      
      uint t = cumulative_frequencies[ai];
      cumulative_frequencies[ai] = cumulative_frequencies[bi];
      cumulative_frequencies[bi] += t;
    }
  }

  groupMemoryBarrier();

  uint id = gl_GlobalInvocationID.x;
  //Binary Search
  uint low = 0;
  uint high = num_symbols - 1;
  uint mid = (high + low) / 2;

  // condition
  // Cumulative_frequencies[x] <= id < cumulative_frequencies[x + 1] 
  for(int i = 0; i < 11; i++) {
    uint too_high = int(id < cumulative_frequencies[mid]);
    uint too_low =  int( (mid < num_symbols - 1)  && (cumulative_frequencies[mid + 1] <= id) );

    low = (too_low) * max(low + 1 , mid) + ((1 - too_low) * low);
    high = (too_high) * min(high - 1, mid) + ((1 - too_high) * high);

    mid = (low + high)/2;
  }

  uint gid = id + (gl_WorkGroupSize.x * gl_NumWorkGroups.x * gl_GlobalInvocationID.y); 
  uint gx = mid + (gl_LocalInvocationID.x * gl_GlobalInvocationID.y);
  table[gid].freq = frequencies[gx];
  table[gid].cum_freq = cumulative_frequencies[mid];
  table[gid].symbol = mid;

}




