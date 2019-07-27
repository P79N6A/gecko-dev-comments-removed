









#ifndef mozilla_VideoToolbox_VideoToolbox_h
#define mozilla_VideoToolbox_VideoToolbox_h





#include <CoreFoundation/CoreFoundation.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CVPixelBuffer.h>

typedef uint32_t VTDecodeFrameFlags;
typedef uint32_t VTDecodeInfoFlags;
enum {
  kVTDecodeInfo_Asynchronous = 1UL << 0,
  kVTDecodeInfo_FrameDropped = 1UL << 1,
};

typedef struct OpaqueVTDecompressionSession* VTDecompressionSessionRef;
typedef void (*VTDecompressionOutputCallback)(
    void*,
    void*,
    OSStatus,
    VTDecodeInfoFlags,
    CVImageBufferRef,
    CMTime,
    CMTime
);
typedef struct VTDecompressionOutputCallbackRecord {
    VTDecompressionOutputCallback decompressionOutputCallback;
    void*                         decompressionOutputRefCon;
} VTDecompressionOutputCallbackRecord;

OSStatus
VTDecompressionSessionCreate(
    CFAllocatorRef,
    CMVideoFormatDescriptionRef,
    CFDictionaryRef,
    CFDictionaryRef,
    const VTDecompressionOutputCallbackRecord*,
    VTDecompressionSessionRef*
);

OSStatus
VTDecompressionSessionDecodeFrame(
    VTDecompressionSessionRef,
    CMSampleBufferRef,
    VTDecodeFrameFlags,
    void*,
    VTDecodeInfoFlags*
);

OSStatus
VTDecompressionSessionWaitForAsynchronousFrames(
    VTDecompressionSessionRef
);

void
VTDecompressionSessionInvalidate(
    VTDecompressionSessionRef
);

#endif 
