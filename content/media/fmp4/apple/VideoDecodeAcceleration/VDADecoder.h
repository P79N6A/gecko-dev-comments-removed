











#ifndef mozilla_VideoDecodeAcceleration_VDADecoder_h
#define mozilla_VideoDecodeAcceleration_VDADecoder_h

#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CoreVideo.h>

typedef uint32_t VDADecodeFrameFlags;
typedef uint32_t VDADecodeInfoFlags;

enum {
  kVDADecodeInfo_Asynchronous = 1UL << 0,
  kVDADecodeInfo_FrameDropped = 1UL << 1
};

enum {
  kVDADecoderFlush_EmitFrames = 1 << 0
};

typedef struct OpaqueVDADecoder* VDADecoder;

typedef void (*VDADecoderOutputCallback)
  (void* decompressionOutputRefCon,
   CFDictionaryRef frameInfo,
   OSStatus status,
   uint32_t infoFlags,
   CVImageBufferRef imageBuffer);

OSStatus
VDADecoderCreate(
  CFDictionaryRef decoderConfiguration,
  CFDictionaryRef destinationImageBufferAttributes, 
  VDADecoderOutputCallback* outputCallback,
  void* decoderOutputCallbackRefcon,
  VDADecoder* decoderOut);

OSStatus
VDADecoderDecode(
  VDADecoder decoder,
  uint32_t decodeFlags,
  CFTypeRef compressedBuffer,
  CFDictionaryRef frameInfo); 

OSStatus
VDADecoderFlush(
  VDADecoder decoder,
  uint32_t flushFlags);

OSStatus
VDADecoderDestroy(VDADecoder decoder);

#endif 
