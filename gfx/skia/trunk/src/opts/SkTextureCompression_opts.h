






#ifndef SkTextureCompression_opts_DEFINED
#define SkTextureCompression_opts_DEFINED

#include "SkTextureCompressor.h"
#include "SkImageInfo.h"

SkTextureCompressor::CompressionProc
SkTextureCompressorGetPlatformProc(SkColorType colorType, SkTextureCompressor::Format fmt);

#endif  
