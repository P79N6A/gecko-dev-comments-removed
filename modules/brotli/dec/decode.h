
















#ifndef BROTLI_DEC_DECODE_H_
#define BROTLI_DEC_DECODE_H_

#include "./streams.h"
#include "./types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif





int BrotliDecompressedSize(size_t encoded_size,
                           const uint8_t* encoded_buffer,
                           size_t* decoded_size);






int BrotliDecompressBuffer(size_t encoded_size,
                           const uint8_t* encoded_buffer,
                           size_t* decoded_size,
                           uint8_t* decoded_buffer);



int BrotliDecompress(BrotliInput input, BrotliOutput output);

#if defined(__cplusplus) || defined(c_plusplus)
}    
#endif

#endif
