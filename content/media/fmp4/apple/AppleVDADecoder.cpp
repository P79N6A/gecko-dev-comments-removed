





#include <CoreFoundation/CFString.h>

#include "AppleUtils.h"
#include "AppleVDADecoder.h"
#include "AppleVDALinker.h"
#include "mp4_demuxer/DecoderData.h"
#include "MP4Decoder.h"
#include "MediaData.h"
#include "MacIOSurfaceImage.h"
#include "mozilla/ArrayUtils.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "prlog.h"
#include "VideoUtils.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetAppleMediaLog();
#define LOG(...) PR_LOG(GetAppleMediaLog(), PR_LOG_DEBUG, (__VA_ARGS__))

#else
#define LOG(...)
#endif

namespace mozilla {

AppleVDADecoder::AppleVDADecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                               MediaTaskQueue* aVideoTaskQueue,
                               MediaDataDecoderCallback* aCallback,
                               layers::ImageContainer* aImageContainer)
  : mConfig(aConfig)
  , mTaskQueue(aVideoTaskQueue)
  , mCallback(aCallback)
  , mImageContainer(aImageContainer)
  , mDecoder(nullptr)
{
  MOZ_COUNT_CTOR(AppleVDADecoder);
  
  LOG("Creating AppleVDADecoder for %dx%d h.264 video",
      mConfig.display_width,
      mConfig.display_height
     );
}

AppleVDADecoder::~AppleVDADecoder()
{
  MOZ_COUNT_DTOR(AppleVDADecoder);
}

nsresult
AppleVDADecoder::Init()
{
  if (mDecoder) {
    return NS_OK;
  }
  nsresult rv = InitializeSession();
  return rv;
}

nsresult
AppleVDADecoder::Shutdown()
{
  if (mDecoder) {
    LOG("%s: cleaning up decoder %p", __func__, mDecoder);
    VDADecoderDestroy(mDecoder);
    mDecoder = nullptr;
  }
  return NS_OK;
}

nsresult
AppleVDADecoder::Input(mp4_demuxer::MP4Sample* aSample)
{
  LOG("mp4 input sample %p pts %lld duration %lld us%s %d bytes",
      aSample,
      aSample->composition_timestamp,
      aSample->duration,
      aSample->is_sync_point ? " keyframe" : "",
      aSample->size);

  mTaskQueue->Dispatch(
      NS_NewRunnableMethodWithArg<nsAutoPtr<mp4_demuxer::MP4Sample>>(
          this,
          &AppleVDADecoder::SubmitFrame,
          nsAutoPtr<mp4_demuxer::MP4Sample>(aSample)));
  return NS_OK;
}

nsresult
AppleVDADecoder::Flush()
{
  mTaskQueue->Flush();
  OSStatus rv = VDADecoderFlush(mDecoder, 0 );
  if (rv != noErr) {
    LOG("AppleVDADecoder::Flush failed waiting for platform decoder.");
  }
  ClearReorderedFrames();

  return NS_OK;
}

nsresult
AppleVDADecoder::Drain()
{
  mTaskQueue->AwaitIdle();
  OSStatus rv = VDADecoderFlush(mDecoder, kVDADecoderFlush_EmitFrames);
  if (rv != noErr) {
    LOG("AppleVDADecoder::Drain failed waiting for platform decoder.");
  }
  DrainReorderedFrames();
  mCallback->DrainComplete();
  return NS_OK;
}





class AppleFrameRef {
public:
  Microseconds decode_timestamp;
  Microseconds composition_timestamp;
  Microseconds duration;
  int64_t byte_offset;
  bool is_sync_point;

  AppleFrameRef(Microseconds aDts,
                Microseconds aPts,
                Microseconds aDuration,
                int64_t aByte_offset,
                bool aIs_sync_point)
  : decode_timestamp(aDts)
  , composition_timestamp(aPts)
  , duration(aDuration)
  , byte_offset(aByte_offset)
  , is_sync_point(aIs_sync_point)
  {
  }
};





static void
PlatformCallback(void* decompressionOutputRefCon,
                 CFDictionaryRef frameInfo,
                 OSStatus status,
                 VDADecodeInfoFlags infoFlags,
                 CVImageBufferRef image)
{
  LOG("AppleVDADecoder %s status %d flags %d", __func__, status, infoFlags);

  
  
  
  
  
  if (status != noErr || !image) {
    NS_WARNING("AppleVDADecoder decoder returned no data");
    return;
  }
  MOZ_ASSERT(CFGetTypeID(image) == CVPixelBufferGetTypeID(),
             "AppleVDADecoder returned an unexpected image type");

  if (infoFlags & kVDADecodeInfo_FrameDropped)
  {
    NS_WARNING("  ...frame dropped...");
    return;
  }

  AppleVDADecoder* decoder =
    static_cast<AppleVDADecoder*>(decompressionOutputRefCon);

  AutoCFRelease<CFNumberRef> ptsref =
    (CFNumberRef)CFDictionaryGetValue(frameInfo, CFSTR("FRAME_PTS"));
  AutoCFRelease<CFNumberRef> dtsref =
    (CFNumberRef)CFDictionaryGetValue(frameInfo, CFSTR("FRAME_DTS"));
  AutoCFRelease<CFNumberRef> durref =
    (CFNumberRef)CFDictionaryGetValue(frameInfo, CFSTR("FRAME_DURATION"));
  AutoCFRelease<CFNumberRef> boref =
    (CFNumberRef)CFDictionaryGetValue(frameInfo, CFSTR("FRAME_OFFSET"));
  AutoCFRelease<CFNumberRef> kfref =
    (CFNumberRef)CFDictionaryGetValue(frameInfo, CFSTR("FRAME_KEYFRAME"));

  Microseconds dts;
  Microseconds pts;
  Microseconds duration;
  int64_t byte_offset;
  char is_sync_point;

  CFNumberGetValue(ptsref, kCFNumberSInt64Type, &pts);
  CFNumberGetValue(dtsref, kCFNumberSInt64Type, &dts);
  CFNumberGetValue(durref, kCFNumberSInt64Type, &duration);
  CFNumberGetValue(boref, kCFNumberSInt64Type, &byte_offset);
  CFNumberGetValue(kfref, kCFNumberSInt8Type, &is_sync_point);

  nsAutoPtr<AppleFrameRef> frameRef(new AppleFrameRef(dts,
                                                      pts,
                                                      duration,
                                                      byte_offset,
                                                      is_sync_point == 1));

  
  
  decoder->OutputFrame(image, frameRef);
}

void
AppleVDADecoder::DrainReorderedFrames()
{
  while (!mReorderQueue.IsEmpty()) {
    mCallback->Output(mReorderQueue.Pop());
  }
}

void
AppleVDADecoder::ClearReorderedFrames()
{
  while (!mReorderQueue.IsEmpty()) {
    delete mReorderQueue.Pop();
  }
}


nsresult
AppleVDADecoder::OutputFrame(CVPixelBufferRef aImage,
                             nsAutoPtr<AppleFrameRef> aFrameRef)
{
  IOSurfacePtr surface = MacIOSurfaceLib::CVPixelBufferGetIOSurface(aImage);
  MOZ_ASSERT(surface, "Decoder didn't return an IOSurface backed buffer");

  LOG("mp4 output frame %lld dts %lld pts %lld duration %lld us%s",
    aFrameRef->byte_offset,
    aFrameRef->decode_timestamp,
    aFrameRef->composition_timestamp,
    aFrameRef->duration,
    aFrameRef->is_sync_point ? " keyframe" : ""
  );

  nsRefPtr<MacIOSurface> macSurface = new MacIOSurface(surface);
  
  VideoInfo info;
  info.mDisplay = nsIntSize(macSurface->GetWidth(), macSurface->GetHeight());
  info.mHasVideo = true;
  gfx::IntRect visible = gfx::IntRect(0,
                                      0,
                                      mConfig.display_width,
                                      mConfig.display_height);

  nsRefPtr<layers::Image> image =
    mImageContainer->CreateImage(ImageFormat::MAC_IOSURFACE);
  layers::MacIOSurfaceImage* videoImage =
    static_cast<layers::MacIOSurfaceImage*>(image.get());
  videoImage->SetSurface(macSurface);

  nsAutoPtr<VideoData> data;
  data = VideoData::CreateFromImage(info,
                                    mImageContainer,
                                    aFrameRef->byte_offset,
                                    aFrameRef->composition_timestamp,
                                    aFrameRef->duration, image.forget(),
                                    aFrameRef->is_sync_point,
                                    aFrameRef->decode_timestamp,
                                    visible);

  if (!data) {
    NS_ERROR("Couldn't create VideoData for frame");
    mCallback->Error();
    return NS_ERROR_FAILURE;
  }

  
  
  mReorderQueue.Push(data.forget());
  
  while (mReorderQueue.Length() > 0) {
    VideoData* readyData = mReorderQueue.Pop();
    if (readyData->mTime <= aFrameRef->decode_timestamp) {
      LOG("returning queued frame with pts %lld", readyData->mTime);
      mCallback->Output(readyData);
    } else {
      LOG("requeued frame with pts %lld > %lld",
          readyData->mTime, aFrameRef->decode_timestamp);
      mReorderQueue.Push(readyData);
      break;
    }
  }
  LOG("%llu decoded frames queued",
      static_cast<unsigned long long>(mReorderQueue.Length()));

  return NS_OK;
}

nsresult
AppleVDADecoder::SubmitFrame(mp4_demuxer::MP4Sample* aSample)
{
  AutoCFRelease<CFDataRef> block =
    CFDataCreate(kCFAllocatorDefault, aSample->data, aSample->size);
  if (!block) {
    NS_ERROR("Couldn't create CFData");
    return NS_ERROR_FAILURE;
  }

  AutoCFRelease<CFNumberRef> pts =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt64Type,
                   &aSample->composition_timestamp);
  AutoCFRelease<CFNumberRef> dts =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt64Type,
                   &aSample->decode_timestamp);
  AutoCFRelease<CFNumberRef> duration =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt64Type,
                   &aSample->duration);
  AutoCFRelease<CFNumberRef> byte_offset =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt64Type,
                   &aSample->byte_offset);
  char keyframe = aSample->is_sync_point ? 1 : 0;
  AutoCFRelease<CFNumberRef> cfkeyframe =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt8Type,
                   &keyframe);

  const void* keys[] = { CFSTR("FRAME_PTS"),
                         CFSTR("FRAME_DTS"),
                         CFSTR("FRAME_DURATION"),
                         CFSTR("FRAME_OFFSET"),
                         CFSTR("FRAME_KEYFRAME") };
  const void* values[] = { pts,
                           dts,
                           duration,
                           byte_offset,
                           cfkeyframe };
  static_assert(ArrayLength(keys) == ArrayLength(values),
                "Non matching keys/values array size");

  AutoCFRelease<CFDictionaryRef> params =
    CFDictionaryCreate(kCFAllocatorDefault,
                       keys,
                       values,
                       ArrayLength(keys),
                       &kCFTypeDictionaryKeyCallBacks,
                       &kCFTypeDictionaryValueCallBacks);

  OSStatus rv = VDADecoderDecode(mDecoder,
                                 0,
                                 block,
                                 params);
  if (rv != noErr) {
    NS_ERROR("AppleVDADecoder: Couldn't pass frame to decoder");
    return NS_ERROR_FAILURE;
  }

  
  if (mTaskQueue->IsEmpty()) {
    LOG("AppleVDADecoder task queue empty; requesting more data");
    mCallback->InputExhausted();
  }

  return NS_OK;
}

nsresult
AppleVDADecoder::InitializeSession()
{
  OSStatus rv;

  AutoCFRelease<CFDictionaryRef> decoderConfig =
    CreateDecoderSpecification();

  AutoCFRelease<CFDictionaryRef> outputConfiguration =
    CreateOutputConfiguration();

  rv =
    VDADecoderCreate(decoderConfig,
                     outputConfiguration,
                     (VDADecoderOutputCallback*)PlatformCallback,
                     this,
                     &mDecoder);

  if (rv != noErr) {
    NS_ERROR("AppleVDADecoder: Couldn't create decoder!");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

CFDictionaryRef
AppleVDADecoder::CreateDecoderSpecification()
{
  const uint8_t* extradata = mConfig.extra_data.begin();
  int extrasize = mConfig.extra_data.length();

  OSType format = 'avc1';
  AutoCFRelease<CFNumberRef> avc_width  =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt32Type,
                   &mConfig.display_width);
  AutoCFRelease<CFNumberRef> avc_height =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt32Type,
                   &mConfig.display_height);
  AutoCFRelease<CFNumberRef> avc_format =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt32Type,
                   &format);
  AutoCFRelease<CFDataRef> avc_data =
    CFDataCreate(kCFAllocatorDefault,
                 extradata,
                 extrasize);

  const void* decoderKeys[] = { AppleVDALinker::skPropWidth,
                                AppleVDALinker::skPropHeight,
                                AppleVDALinker::skPropSourceFormat,
                                AppleVDALinker::skPropAVCCData };
  const void* decoderValue[] = { avc_width,
                                 avc_height,
                                 avc_format,
                                 avc_data };
  static_assert(ArrayLength(decoderKeys) == ArrayLength(decoderValue),
                "Non matching keys/values array size");

  return CFDictionaryCreate(kCFAllocatorDefault,
                            decoderKeys,
                            decoderValue,
                            ArrayLength(decoderKeys),
                            &kCFTypeDictionaryKeyCallBacks,
                            &kCFTypeDictionaryValueCallBacks);
}

CFDictionaryRef
AppleVDADecoder::CreateOutputConfiguration()
{
  
  const void* IOSurfaceKeys[] = { MacIOSurfaceLib::kPropIsGlobal };
  const void* IOSurfaceValues[] = { kCFBooleanTrue };
  static_assert(ArrayLength(IOSurfaceKeys) == ArrayLength(IOSurfaceValues),
                "Non matching keys/values array size");

  
  AutoCFRelease<CFDictionaryRef> IOSurfaceProperties =
    CFDictionaryCreate(kCFAllocatorDefault,
                       IOSurfaceKeys,
                       IOSurfaceValues,
                       ArrayLength(IOSurfaceKeys),
                       &kCFTypeDictionaryKeyCallBacks,
                       &kCFTypeDictionaryValueCallBacks);

  SInt32 PixelFormatTypeValue = kCVPixelFormatType_32BGRA;
  AutoCFRelease<CFNumberRef> PixelFormatTypeNumber =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt32Type,
                   &PixelFormatTypeValue);

  const void* outputKeys[] = { kCVPixelBufferIOSurfacePropertiesKey,
                               kCVPixelBufferPixelFormatTypeKey,
                               kCVPixelBufferOpenGLCompatibilityKey };
  const void* outputValues[] = { IOSurfaceProperties,
                                 PixelFormatTypeNumber,
                                 kCFBooleanTrue };
  static_assert(ArrayLength(outputKeys) == ArrayLength(outputValues),
                "Non matching keys/values array size");

  return CFDictionaryCreate(kCFAllocatorDefault,
                            outputKeys,
                            outputValues,
                            ArrayLength(outputKeys),
                            &kCFTypeDictionaryKeyCallBacks,
                            &kCFTypeDictionaryValueCallBacks);
}


already_AddRefed<AppleVDADecoder>
AppleVDADecoder::CreateVDADecoder(
  const mp4_demuxer::VideoDecoderConfig& aConfig,
  MediaTaskQueue* aVideoTaskQueue,
  MediaDataDecoderCallback* aCallback,
  layers::ImageContainer* aImageContainer)
{
  nsRefPtr<AppleVDADecoder> decoder =
    new AppleVDADecoder(aConfig, aVideoTaskQueue, aCallback, aImageContainer);
  if (NS_FAILED(decoder->Init())) {
    NS_ERROR("AppleVDADecoder an error occurred");
    return nullptr;
  }
  return decoder.forget();
}

} 
