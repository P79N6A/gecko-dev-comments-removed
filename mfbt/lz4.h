
































#pragma once

#if defined (__cplusplus)
extern "C" {
#endif





#if defined(_MSC_VER) && !defined(__cplusplus)   
#  define inline __forceinline           // Visual C is not C99, but supports some kind of inline. Note : we *do* want to force inline
#endif






int LZ4_compress        (const char* source, char* dest, int inputSize);
int LZ4_decompress_safe (const char* source, char* dest, int inputSize, int maxOutputSize);























static inline int LZ4_compressBound(int isize)   { return ((isize) + ((isize)/255) + 16); }
#define           LZ4_COMPRESSBOUND(    isize)            ((isize) + ((isize)/255) + 16)














int LZ4_compress_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize);














int LZ4_decompress_fast (const char* source, char* dest, int outputSize);












int LZ4_decompress_safe_partial (const char* source, char* dest, int inputSize, int targetOutputSize, int maxOutputSize);















int LZ4_decompress_safe_withPrefix64k (const char* source, char* dest, int inputSize, int maxOutputSize);
int LZ4_decompress_fast_withPrefix64k (const char* source, char* dest, int outputSize);













static inline int LZ4_uncompress (const char* source, char* dest, int outputSize) { return LZ4_decompress_fast(source, dest, outputSize); }
static inline int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize) { return LZ4_decompress_safe(source, dest, isize, maxOutputSize); }








#if defined (__cplusplus)
}
#endif
