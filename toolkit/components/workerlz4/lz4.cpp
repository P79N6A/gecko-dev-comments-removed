



#include "mozilla/Compression.h"












using namespace mozilla::Compression;










extern "C" MOZ_EXPORT size_t
workerlz4_compress(const char* source, size_t inputSize, char* dest) {
  return LZ4::compress(source, inputSize, dest);
}














extern "C" MOZ_EXPORT int
workerlz4_decompress(const char* source, size_t inputSize,
                     char* dest, size_t maxOutputSize,
                     size_t *bytesOutput) {
  return LZ4::decompress(source, inputSize,
                         dest, maxOutputSize,
                         bytesOutput);
}











extern "C" MOZ_EXPORT size_t
workerlz4_maxCompressedSize(size_t inputSize)
{
  return LZ4::maxCompressedSize(inputSize);
}



