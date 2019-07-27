
















#ifndef BROTLI_DEC_STREAMS_H_
#define BROTLI_DEC_STREAMS_H_

#include <stdio.h>
#include "./types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif



typedef int (*BrotliInputFunction)(void* data, uint8_t* buf, size_t len);


typedef struct {
  BrotliInputFunction cb_;
  void* data_;
} BrotliInput;


static BROTLI_INLINE int BrotliRead(BrotliInput in, uint8_t* buf, size_t len) {
  return in.cb_(in.data_, buf, len);
}



typedef int (*BrotliOutputFunction)(void* data, const uint8_t* buf, size_t len);


typedef struct {
  BrotliOutputFunction cb_;
  void* data_;
} BrotliOutput;


static BROTLI_INLINE int BrotliWrite(BrotliOutput out,
                                     const uint8_t* buf, size_t len) {
  return out.cb_(out.data_, buf, len);
}


typedef struct {
  const uint8_t* buffer;
  size_t length;
  size_t pos;
} BrotliMemInput;


int BrotliMemInputFunction(void* data, uint8_t* buf, size_t count);


BrotliInput BrotliInitMemInput(const uint8_t* buffer, size_t length,
                               BrotliMemInput* mem_input);


typedef struct {
  uint8_t* buffer;
  size_t length;
  size_t pos;
} BrotliMemOutput;


int BrotliMemOutputFunction(void* data, const uint8_t* buf, size_t count);


BrotliOutput BrotliInitMemOutput(uint8_t* buffer, size_t length,
                                 BrotliMemOutput* mem_output);


int BrotliStdinInputFunction(void* data, uint8_t* buf, size_t count);
BrotliInput BrotliStdinInput();


int BrotliStdoutOutputFunction(void* data, const uint8_t* buf, size_t count);
BrotliOutput BrotliStdoutOutput();


int BrotliFileOutputFunction(void* data, const uint8_t* buf, size_t count);
BrotliOutput BrotliFileOutput(FILE* f);

#if defined(__cplusplus) || defined(c_plusplus)
}    
#endif

#endif
