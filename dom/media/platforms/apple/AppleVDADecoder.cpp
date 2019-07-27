





#include <CoreFoundation/CFString.h>

#include "AppleUtils.h"
#include "AppleVDADecoder.h"
#include "AppleVDALinker.h"
#include "MediaInfo.h"
#include "mp4_demuxer/H264.h"
#include "MP4Decoder.h"
#include "MediaData.h"
#include "MacIOSurfaceImage.h"
#include "mozilla/ArrayUtils.h"
#include "nsAutoPtr.h"
#include "nsCocoaFeatures.h"
#include "nsThreadUtils.h"
#include "mozilla/Logging.h"
#include "VideoUtils.h"
#include <algorithm>
#include "gfxPlatform.h"

PRLogModuleInfo* GetAppleMediaLog();
#define LOG(...) MOZ_LOG(GetAppleMediaLog(), mozilla::LogLevel::Debug, (__VA_ARGS__))


namespace mozilla {

AppleVDADecoder::AppleVDADecoder(const VideoInfo& aConfig,
                               FlushableTaskQueue* aVideoTaskQueue,
                               MediaDataDecoderCallback* aCallback,
                               layers::ImageContainer* aImageContainer)
  : mTaskQueue(aVideoTaskQueue)
  , mCallback(aCallback)
  , mImageContainer(aImageContainer)
  , mPictureWidth(aConfig.mImage.width)
  , mPictureHeight(aConfig.mImage.height)
  , mDisplayWidth(aConfig.mDisplay.width)
  , mDisplayHeight(aConfig.mDisplay.height)
  , mDecoder(nullptr)
  , mIs106(!nsCocoaFeatures::OnLionOrLater())
{
  MOZ_COUNT_CTOR(AppleVDADecoder);
  

  mExtraData = aConfig.mExtraData;
  mMaxRefFrames = 4;
  
  mp4_demuxer::SPSData spsdata;
  if (mp4_demuxer::H264::DecodeSPSFromExtraData(mExtraData, spsdata)) {
    
    
    
    
    mMaxRefFrames =
      std::min(std::max(mMaxRefFrames, spsdata.max_num_ref_frames + 1), 16u);
  }

  LOG("Creating AppleVDADecoder for %dx%d (%dx%d) h.264 video",
      mPictureWidth,
      mPictureHeight,
      mDisplayWidth,
      mDisplayHeight
     );
}

AppleVDADecoder::~AppleVDADecoder()
{
  MOZ_COUNT_DTOR(AppleVDADecoder);
}

nsresult
AppleVDADecoder::Init()
{
  if (!gfxPlatform::GetPlatform()->CanUseHardwareVideoDecoding()) {
    
    return NS_ERROR_FAILURE;
  }

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
AppleVDADecoder::Input(MediaRawData* aSample)
{
  LOG("mp4 input sample %p pts %lld duration %lld us%s %d bytes",
      aSample,
      aSample->mTime,
      aSample->mDuration,
      aSample->mKeyframe ? " keyframe" : "",
      aSample->mSize);

  nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethodWithArg<nsRefPtr<MediaRawData>>(
          this,
          &AppleVDADecoder::SubmitFrame,
          nsRefPtr<MediaRawData>(aSample));
  mTaskQueue->Dispatch(runnable.forget());
  return NS_OK;
}

nsresult
AppleVDADecoder::Flush()
{
  mTaskQueue->Flush();
  OSStatus rv = VDADecoderFlush(mDecoder, 0 );
  if (rv != noErr) {
    LOG("AppleVDADecoder::Flush failed waiting for platform decoder "
        "with error:%d.", rv);
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
    LOG("AppleVDADecoder::Drain failed waiting for platform decoder "
        "with error:%d.", rv);
  }
  DrainReorderedFrames();
  mCallback->DrainComplete();
  return NS_OK;
}









static void
PlatformCallback(void* decompressionOutputRefCon,
                 CFDictionaryRef frameInfo,
                 OSStatus status,
                 VDADecodeInfoFlags infoFlags,
                 CVImageBufferRef image)
{
  LOG("AppleVDADecoder[%s] status %d flags %d retainCount %ld",
      __func__, status, infoFlags, CFGetRetainCount(frameInfo));

  
  
  
  
  
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

  int64_t dts;
  int64_t pts;
  int64_t duration;
  int64_t byte_offset;
  char is_sync_point;

  CFNumberGetValue(ptsref, kCFNumberSInt64Type, &pts);
  CFNumberGetValue(dtsref, kCFNumberSInt64Type, &dts);
  CFNumberGetValue(durref, kCFNumberSInt64Type, &duration);
  CFNumberGetValue(boref, kCFNumberSInt64Type, &byte_offset);
  CFNumberGetValue(kfref, kCFNumberSInt8Type, &is_sync_point);

  nsAutoPtr<AppleVDADecoder::AppleFrameRef> frameRef(
    new AppleVDADecoder::AppleFrameRef(
      media::TimeUnit::FromMicroseconds(dts),
      media::TimeUnit::FromMicroseconds(pts),
      media::TimeUnit::FromMicroseconds(duration),
      byte_offset,
      is_sync_point == 1));

  
  
  decoder->OutputFrame(image, frameRef);
}

AppleVDADecoder::AppleFrameRef*
AppleVDADecoder::CreateAppleFrameRef(const MediaRawData* aSample)
{
  MOZ_ASSERT(aSample);
  return new AppleFrameRef(*aSample);
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
    mReorderQueue.Pop();
  }
}


nsresult
AppleVDADecoder::OutputFrame(CVPixelBufferRef aImage,
                             nsAutoPtr<AppleVDADecoder::AppleFrameRef> aFrameRef)
{
  IOSurfacePtr surface = MacIOSurfaceLib::CVPixelBufferGetIOSurface(aImage);
  MOZ_ASSERT(surface, "Decoder didn't return an IOSurface backed buffer");

  LOG("mp4 output frame %lld dts %lld pts %lld duration %lld us%s",
    aFrameRef->byte_offset,
    aFrameRef->decode_timestamp.ToMicroseconds(),
    aFrameRef->composition_timestamp.ToMicroseconds(),
    aFrameRef->duration.ToMicroseconds(),
    aFrameRef->is_sync_point ? " keyframe" : ""
  );

  nsRefPtr<MacIOSurface> macSurface = new MacIOSurface(surface);
  
  VideoInfo info;
  info.mDisplay = nsIntSize(mDisplayWidth, mDisplayHeight);
  gfx::IntRect visible = gfx::IntRect(0,
                                      0,
                                      mPictureWidth,
                                      mPictureHeight);

  nsRefPtr<layers::Image> image =
    mImageContainer->CreateImage(ImageFormat::MAC_IOSURFACE);
  layers::MacIOSurfaceImage* videoImage =
    static_cast<layers::MacIOSurfaceImage*>(image.get());
  videoImage->SetSurface(macSurface);

  nsRefPtr<VideoData> data;
  data = VideoData::CreateFromImage(info,
                                    mImageContainer,
                                    aFrameRef->byte_offset,
                                    aFrameRef->composition_timestamp.ToMicroseconds(),
                                    aFrameRef->duration.ToMicroseconds(),
                                    image.forget(),
                                    aFrameRef->is_sync_point,
                                    aFrameRef->decode_timestamp.ToMicroseconds(),
                                    visible);

  if (!data) {
    NS_ERROR("Couldn't create VideoData for frame");
    mCallback->Error();
    return NS_ERROR_FAILURE;
  }

  
  
  mReorderQueue.Push(data);
  while (mReorderQueue.Length() > mMaxRefFrames) {
    mCallback->Output(mReorderQueue.Pop());
  }
  LOG("%llu decoded frames queued",
      static_cast<unsigned long long>(mReorderQueue.Length()));

  return NS_OK;
}

nsresult
AppleVDADecoder::SubmitFrame(MediaRawData* aSample)
{
  AutoCFRelease<CFDataRef> block =
    CFDataCreate(kCFAllocatorDefault, aSample->mData, aSample->mSize);
  if (!block) {
    NS_ERROR("Couldn't create CFData");
    return NS_ERROR_FAILURE;
  }

  AutoCFRelease<CFNumberRef> pts =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt64Type,
                   &aSample->mTime);
  AutoCFRelease<CFNumberRef> dts =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt64Type,
                   &aSample->mTimecode);
  AutoCFRelease<CFNumberRef> duration =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt64Type,
                   &aSample->mDuration);
  AutoCFRelease<CFNumberRef> byte_offset =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt64Type,
                   &aSample->mOffset);
  char keyframe = aSample->mKeyframe ? 1 : 0;
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

  AutoCFRelease<CFDictionaryRef> frameInfo =
    CFDictionaryCreate(kCFAllocatorDefault,
                       keys,
                       values,
                       ArrayLength(keys),
                       &kCFTypeDictionaryKeyCallBacks,
                       &kCFTypeDictionaryValueCallBacks);

  OSStatus rv = VDADecoderDecode(mDecoder,
                                 0,
                                 block,
                                 frameInfo);

  if (rv != noErr) {
    NS_WARNING("AppleVDADecoder: Couldn't pass frame to decoder");
    mCallback->Error();
    return NS_ERROR_FAILURE;
  }

  if (mIs106) {
    
    
    
    
    
    
    
    CFRetain(frameInfo);
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
    NS_WARNING("AppleVDADecoder: Couldn't create hardware VDA decoder");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

CFDictionaryRef
AppleVDADecoder::CreateDecoderSpecification()
{
  const uint8_t* extradata = mExtraData->Elements();
  int extrasize = mExtraData->Length();

  OSType format = 'avc1';
  AutoCFRelease<CFNumberRef> avc_width  =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt32Type,
                   &mPictureWidth);
  AutoCFRelease<CFNumberRef> avc_height =
    CFNumberCreate(kCFAllocatorDefault,
                   kCFNumberSInt32Type,
                   &mPictureHeight);
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
  const VideoInfo& aConfig,
  FlushableTaskQueue* aVideoTaskQueue,
  MediaDataDecoderCallback* aCallback,
  layers::ImageContainer* aImageContainer)
{
  nsRefPtr<AppleVDADecoder> decoder =
    new AppleVDADecoder(aConfig, aVideoTaskQueue, aCallback, aImageContainer);
  if (NS_FAILED(decoder->Init())) {
    return nullptr;
  }
  return decoder.forget();
}

} 
