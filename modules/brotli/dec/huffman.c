
















#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "./huffman.h"
#include "./safe_malloc.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define MAX_LENGTH 15



static BROTLI_INLINE int GetNextKey(int key, int len) {
  int step = 1 << (len - 1);
  while (key & step) {
    step >>= 1;
  }
  return (key & (step - 1)) + step;
}



static BROTLI_INLINE void ReplicateValue(HuffmanCode* table,
                                         int step, int end,
                                         HuffmanCode code) {
  do {
    end -= step;
    table[end] = code;
  } while (end > 0);
}




static BROTLI_INLINE int NextTableBitSize(const int* const count,
                                          int len, int root_bits) {
  int left = 1 << (len - root_bits);
  while (len < MAX_LENGTH) {
    left -= count[len];
    if (left <= 0) break;
    ++len;
    left <<= 1;
  }
  return len - root_bits;
}

int BrotliBuildHuffmanTable(HuffmanCode* root_table,
                            int root_bits,
                            const uint8_t* const code_lengths,
                            int code_lengths_size) {
  HuffmanCode code;    
  HuffmanCode* table;  
  int len;             
  int symbol;          
  int key;             
  int step;            
  int low;             
  int mask;            
  int table_bits;      
  int table_size;      
  int total_size;      
  int* sorted;         
  int count[MAX_LENGTH + 1] = { 0 };  
  int offset[MAX_LENGTH + 1];  

  sorted = (int*)malloc((size_t)code_lengths_size * sizeof(*sorted));
  if (sorted == NULL) {
    return 0;
  }

  
  for (symbol = 0; symbol < code_lengths_size; symbol++) {
    count[code_lengths[symbol]]++;
  }

  
  offset[1] = 0;
  for (len = 1; len < MAX_LENGTH; len++) {
    offset[len + 1] = offset[len] + count[len];
  }

  
  for (symbol = 0; symbol < code_lengths_size; symbol++) {
    if (code_lengths[symbol] != 0) {
      sorted[offset[code_lengths[symbol]]++] = symbol;
    }
  }

  table = root_table;
  table_bits = root_bits;
  table_size = 1 << table_bits;
  total_size = table_size;

  
  if (offset[MAX_LENGTH] == 1) {
    code.bits = 0;
    code.value = (uint16_t)sorted[0];
    for (key = 0; key < total_size; ++key) {
      table[key] = code;
    }
    free(sorted);
    return total_size;
  }

  
  key = 0;
  symbol = 0;
  for (len = 1, step = 2; len <= root_bits; ++len, step <<= 1) {
    for (; count[len] > 0; --count[len]) {
      code.bits = (uint8_t)(len);
      code.value = (uint16_t)sorted[symbol++];
      ReplicateValue(&table[key], step, table_size, code);
      key = GetNextKey(key, len);
    }
  }

  
  mask = total_size - 1;
  low = -1;
  for (len = root_bits + 1, step = 2; len <= MAX_LENGTH; ++len, step <<= 1) {
    for (; count[len] > 0; --count[len]) {
      if ((key & mask) != low) {
        table += table_size;
        table_bits = NextTableBitSize(count, len, root_bits);
        table_size = 1 << table_bits;
        total_size += table_size;
        low = key & mask;
        root_table[low].bits = (uint8_t)(table_bits + root_bits);
        root_table[low].value = (uint16_t)((table - root_table) - low);
      }
      code.bits = (uint8_t)(len - root_bits);
      code.value = (uint16_t)sorted[symbol++];
      ReplicateValue(&table[key >> root_bits], step, table_size, code);
      key = GetNextKey(key, len);
    }
  }

  free(sorted);
  return total_size;
}

#if defined(__cplusplus) || defined(c_plusplus)
}    
#endif
