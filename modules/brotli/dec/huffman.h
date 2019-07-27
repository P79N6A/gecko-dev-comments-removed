
















#ifndef BROTLI_DEC_HUFFMAN_H_
#define BROTLI_DEC_HUFFMAN_H_

#include <assert.h>
#include "./types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct {
  uint8_t bits;     
  uint16_t value;   
} HuffmanCode;



int BrotliBuildHuffmanTable(HuffmanCode* root_table,
                            int root_bits,
                            const uint8_t* const code_lengths,
                            int code_lengths_size);

#if defined(__cplusplus) || defined(c_plusplus)
}    
#endif

#endif
