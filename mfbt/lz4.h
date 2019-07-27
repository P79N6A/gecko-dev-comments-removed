
































#pragma once

#if defined (__cplusplus)
extern "C" {
#endif





#define LZ4_VERSION_MAJOR    1    /* for major interface/format changes  */
#define LZ4_VERSION_MINOR    2    /* for minor interface/format changes  */
#define LZ4_VERSION_RELEASE  0    /* for tweaks, bug-fixes, or development */












#define LZ4_MEMORY_USAGE 14






int LZ4_compress        (const char* source, char* dest, int inputSize);
int LZ4_decompress_safe (const char* source, char* dest, int compressedSize, int maxOutputSize);

































#define LZ4_MAX_INPUT_SIZE        0x7E000000   /* 2 113 929 216 bytes */
#define LZ4_COMPRESSBOUND(isize)  ((unsigned int)(isize) > (unsigned int)LZ4_MAX_INPUT_SIZE ? 0 : (isize) + ((isize)/255) + 16)











int LZ4_compressBound(int isize);













int LZ4_compress_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize);













int LZ4_decompress_fast (const char* source, char* dest, int originalSize);














int LZ4_decompress_safe_partial (const char* source, char* dest, int compressedSize, int targetOutputSize, int maxOutputSize);






#define LZ4_STREAMSIZE_U32 ((1 << (LZ4_MEMORY_USAGE-2)) + 8)
#define LZ4_STREAMSIZE     (LZ4_STREAMSIZE_U32 * sizeof(unsigned int))





typedef struct { unsigned int table[LZ4_STREAMSIZE_U32]; } LZ4_stream_t;







void* LZ4_createStream();
int   LZ4_free (void* LZ4_stream);









int LZ4_loadDict (void* LZ4_stream, const char* dictionary, int dictSize);






int LZ4_compress_continue (void* LZ4_stream, const char* source, char* dest, int inputSize);






int LZ4_compress_limitedOutput_continue (void* LZ4_stream, const char* source, char* dest, int inputSize, int maxOutputSize);










int LZ4_saveDict (void* LZ4_stream, char* safeBuffer, int dictSize);






#define LZ4_STREAMDECODESIZE_U32 4
#define LZ4_STREAMDECODESIZE     (LZ4_STREAMDECODESIZE_U32 * sizeof(unsigned int))





typedef struct { unsigned int table[LZ4_STREAMDECODESIZE_U32]; } LZ4_streamDecode_t;







void* LZ4_createStreamDecode();
int   LZ4_free (void* LZ4_stream);   








int LZ4_decompress_safe_continue (void* LZ4_streamDecode, const char* source, char* dest, int compressedSize, int maxOutputSize);
int LZ4_decompress_fast_continue (void* LZ4_streamDecode, const char* source, char* dest, int originalSize);









int LZ4_setDictDecode (void* LZ4_streamDecode, const char* dictionary, int dictSize);










int LZ4_decompress_safe_usingDict (const char* source, char* dest, int compressedSize, int maxOutputSize, const char* dictStart, int dictSize);
int LZ4_decompress_fast_usingDict (const char* source, char* dest, int originalSize, const char* dictStart, int dictSize);



#if defined (__cplusplus)
}
#endif
