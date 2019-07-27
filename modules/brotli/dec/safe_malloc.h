
















#ifndef BROTLI_DEC_SAFE_MALLOC_H_
#define BROTLI_DEC_SAFE_MALLOC_H_

#include <assert.h>

#include "./types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


#define BROTLI_MAX_ALLOCABLE_MEMORY (1 << 30)








void* BrotliSafeMalloc(uint64_t nmemb, size_t size);

#if defined(__cplusplus) || defined(c_plusplus)
}    
#endif

#endif
