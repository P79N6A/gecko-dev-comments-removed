





#include "WMFReader.h"
#include "WMFDecoder.h"
#include "WMFUtils.h"
#include "WMFByteStream.h"
#include "WMFSourceReaderCallback.h"
#include "mozilla/dom/TimeRanges.h"
#include "mozilla/Preferences.h"
#include "DXVA2Manager.h"
#include "ImageContainer.h"
#include "Layers.h"
#include "mozilla/layers/LayersTypes.h"

#ifndef MOZ_SAMPLE_TYPE_FLOAT32
#error We expect 32bit float audio samples on desktop for the Windows Media Foundation media backend.
#endif

#include "MediaDecoder.h"
#include "VideoUtils.h"

using mozilla::layers::Image;
using mozilla::layers::LayerManager;
using mozilla::layers::LayersBackend;

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define LOG(...) PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif




WMFReader::WMFReader(AbstractMediaDecoder* aDecoder)
  : MediaDecoderReader(aDecoder),
    mSourceReader(nullptr),
    mAudioChannels(0),
    mAudioBytesPerSample(0),
    mAudioRate(0),
    mVideoWidth(0),
    mVideoHeight(0),
    mVideoStride(0),
    mAudioFrameSum(0),
    mAudioFrameOffset(0),
    mHasAudio(false),
    mHasVideo(false),
    mCanSeek(false),
    mUseHwAccel(false),
    mMustRecaptureAudioPosition(true),
    mIsMP3Enabled(WMFDecoder::IsMP3Supported())
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");
  MOZ_COUNT_CTOR(WMFReader);
}

WMFReader::~WMFReader()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");

  
  
  if (mByteStream) {
    DebugOnly<nsresult> rv = mByteStream->Shutdown();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to shutdown WMFByteStream");
  }
  DebugOnly<HRESULT> hr = wmf::MFShutdown();
  NS_ASSERTION(SUCCEEDED(hr), "MFShutdown failed");
  MOZ_COUNT_DTOR(WMFReader);
}

void
WMFReader::OnDecodeThreadStart()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  NS_ENSURE_TRUE_VOID(SUCCEEDED(hr));
}

void
WMFReader::OnDecodeThreadFinish()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  CoUninitialize();
}

bool
WMFReader::InitializeDXVA()
{
  if (!Preferences::GetBool("media.windows-media-foundation.use-dxva", false)) {
    return false;
  }

  
  
  
  
  
  MediaDecoderOwner* owner = mDecoder->GetOwner();
  NS_ENSURE_TRUE(owner, false);

  HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE(element, false);

  nsIDocument* doc = element->GetOwnerDocument();
  NS_ENSURE_TRUE(doc, false);

  nsRefPtr<LayerManager> layerManager = nsContentUtils::LayerManagerForDocument(doc);
  NS_ENSURE_TRUE(layerManager, false);

  if (layerManager->GetBackendType() != LayersBackend::LAYERS_D3D9 &&
      layerManager->GetBackendType() != LayersBackend::LAYERS_D3D10) {
    return false;
  }

  mDXVA2Manager = DXVA2Manager::Create();
  NS_ENSURE_TRUE(mDXVA2Manager, false);

  return true;
}

nsresult
WMFReader::Init(MediaDecoderReader* aCloneDonor)
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");

  nsresult rv = WMFDecoder::LoadDLLs();
  NS_ENSURE_SUCCESS(rv, rv);

  if (FAILED(wmf::MFStartup())) {
    NS_WARNING("Failed to initialize Windows Media Foundation");
    return NS_ERROR_FAILURE;
  }

  mSourceReaderCallback = new WMFSourceReaderCallback();

  
  mByteStream = new WMFByteStream(mDecoder->GetResource(), mSourceReaderCallback);
  rv = mByteStream->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mUseHwAccel = InitializeDXVA();

  return NS_OK;
}

bool
WMFReader::HasAudio()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  return mHasAudio;
}

bool
WMFReader::HasVideo()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  return mHasVideo;
}

static HRESULT
ConfigureSourceReaderStream(IMFSourceReader *aReader,
                            const DWORD aStreamIndex,
                            const GUID& aOutputSubType,
                            const GUID* aAllowedInSubTypes,
                            const uint32_t aNumAllowedInSubTypes)
{
  NS_ENSURE_TRUE(aReader, E_POINTER);
  NS_ENSURE_TRUE(aAllowedInSubTypes, E_POINTER);

  RefPtr<IMFMediaType> nativeType;
  RefPtr<IMFMediaType> type;
  HRESULT hr;

  
  hr = aReader->GetNativeMediaType(aStreamIndex, 0, byRef(nativeType));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  
  GUID subType;
  hr = nativeType->GetGUID(MF_MT_SUBTYPE, &subType);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  bool isSubTypeAllowed = false;
  for (uint32_t i = 0; i < aNumAllowedInSubTypes; i++) {
    if (aAllowedInSubTypes[i] == subType) {
      isSubTypeAllowed = true;
      break;
    }
  }
  if (!isSubTypeAllowed) {
    nsCString name = GetGUIDName(subType);
    LOG("ConfigureSourceReaderStream subType=%s is not allowed to be decoded", name.get());
    return E_FAIL;
  }

  
  GUID majorType;
  hr = nativeType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  hr = wmf::MFCreateMediaType(byRef(type));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = type->SetGUID(MF_MT_MAJOR_TYPE, majorType);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = type->SetGUID(MF_MT_SUBTYPE, aOutputSubType);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  
  return aReader->SetCurrentMediaType(aStreamIndex, NULL, type);
}


HRESULT
GetSourceReaderDuration(IMFSourceReader *aReader,
                        int64_t& aOutDuration)
{
  AutoPropVar var;
  HRESULT hr = aReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE,
                                                 MF_PD_DURATION,
                                                 &var);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  int64_t duration_hns = 0;
  hr = wmf::PropVariantToInt64(var, &duration_hns);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  aOutDuration = HNsToUsecs(duration_hns);

  return S_OK;
}

HRESULT
GetSourceReaderCanSeek(IMFSourceReader* aReader, bool& aOutCanSeek)
{
  NS_ENSURE_TRUE(aReader, E_FAIL);

  HRESULT hr;
  AutoPropVar var;
  hr = aReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE,
                                         MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS,
                                         &var);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  ULONG flags = 0;
  hr = wmf::PropVariantToUInt32(var, &flags);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  aOutCanSeek = ((flags & MFMEDIASOURCE_CAN_SEEK) == MFMEDIASOURCE_CAN_SEEK);

  return S_OK;
}

static HRESULT
GetDefaultStride(IMFMediaType *aType, uint32_t* aOutStride)
{
  
  HRESULT hr = aType->GetUINT32(MF_MT_DEFAULT_STRIDE, aOutStride);
  if (SUCCEEDED(hr)) {
    return S_OK;
  }

  
  GUID subtype = GUID_NULL;
  uint32_t width = 0;
  uint32_t height = 0;

  hr = aType->GetGUID(MF_MT_SUBTYPE, &subtype);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = MFGetAttributeSize(aType, MF_MT_FRAME_SIZE, &width, &height);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = wmf::MFGetStrideForBitmapInfoHeader(subtype.Data1, width, (LONG*)(aOutStride));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  return hr;
}

static int32_t
MFOffsetToInt32(const MFOffset& aOffset)
{
  return int32_t(aOffset.value + (aOffset.fract / 65536.0f));
}



static HRESULT
GetPictureRegion(IMFMediaType* aMediaType, nsIntRect& aOutPictureRegion)
{
  
  
  BOOL panScan = MFGetAttributeUINT32(aMediaType, MF_MT_PAN_SCAN_ENABLED, FALSE);

  
  HRESULT hr = E_FAIL;
  MFVideoArea videoArea;
  memset(&videoArea, 0, sizeof(MFVideoArea));
  if (panScan) {
    hr = aMediaType->GetBlob(MF_MT_PAN_SCAN_APERTURE,
                             (UINT8*)&videoArea,
                             sizeof(MFVideoArea),
                             NULL);
  }

  
  
  if (!panScan || hr == MF_E_ATTRIBUTENOTFOUND) {
    hr = aMediaType->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE,
                             (UINT8*)&videoArea,
                             sizeof(MFVideoArea),
                             NULL);
  }

  if (hr == MF_E_ATTRIBUTENOTFOUND) {
    
    
    hr = aMediaType->GetBlob(MF_MT_GEOMETRIC_APERTURE,
                             (UINT8*)&videoArea,
                             sizeof(MFVideoArea),
                             NULL);
  }

  if (SUCCEEDED(hr)) {
    
    aOutPictureRegion = nsIntRect(MFOffsetToInt32(videoArea.OffsetX),
                                  MFOffsetToInt32(videoArea.OffsetY),
                                  videoArea.Area.cx,
                                  videoArea.Area.cy);
    return S_OK;
  }

  
  UINT32 width = 0, height = 0;
  hr = MFGetAttributeSize(aMediaType, MF_MT_FRAME_SIZE, &width, &height);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
  aOutPictureRegion = nsIntRect(0, 0, width, height);
  return S_OK;
}

HRESULT
WMFReader::ConfigureVideoFrameGeometry(IMFMediaType* aMediaType)
{
  NS_ENSURE_TRUE(aMediaType != nullptr, E_POINTER);
  HRESULT hr;

  
  
  
  
  
  
  GUID videoFormat;
  hr = aMediaType->GetGUID(MF_MT_SUBTYPE, &videoFormat);
  NS_ENSURE_TRUE(videoFormat == MFVideoFormat_NV12 || !mUseHwAccel, E_FAIL);
  NS_ENSURE_TRUE(videoFormat == MFVideoFormat_YV12 || mUseHwAccel, E_FAIL);

  nsIntRect pictureRegion;
  hr = GetPictureRegion(aMediaType, pictureRegion);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  UINT32 width = 0, height = 0;
  hr = MFGetAttributeSize(aMediaType, MF_MT_FRAME_SIZE, &width, &height);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  uint32_t aspectNum = 0, aspectDenom = 0;
  hr = MFGetAttributeRatio(aMediaType,
                           MF_MT_PIXEL_ASPECT_RATIO,
                           &aspectNum,
                           &aspectDenom);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  
  nsIntSize frameSize = nsIntSize(width, height);
  nsIntSize displaySize = nsIntSize(pictureRegion.width, pictureRegion.height);
  ScaleDisplayByAspectRatio(displaySize, float(aspectNum) / float(aspectDenom));
  if (!VideoInfo::ValidateVideoRegion(frameSize, pictureRegion, displaySize)) {
    
    return E_FAIL;
  }

  
  mInfo.mDisplay = displaySize;
  GetDefaultStride(aMediaType, &mVideoStride);
  mVideoWidth = width;
  mVideoHeight = height;
  mPictureRegion = pictureRegion;

  LOG("WMFReader frame geometry frame=(%u,%u) stride=%u picture=(%d, %d, %d, %d) display=(%d,%d) PAR=%d:%d",
      width, height,
      mVideoStride,
      mPictureRegion.x, mPictureRegion.y, mPictureRegion.width, mPictureRegion.height,
      displaySize.width, displaySize.height,
      aspectNum, aspectDenom);

  return S_OK;
}

HRESULT
WMFReader::ConfigureVideoDecoder()
{
  NS_ASSERTION(mSourceReader, "Must have a SourceReader before configuring decoders!");

  
  if (!mSourceReader ||
      !SourceReaderHasStream(mSourceReader, MF_SOURCE_READER_FIRST_VIDEO_STREAM)) {
    
    return S_OK;
  }

  static const GUID MP4VideoTypes[] = {
    MFVideoFormat_H264
  };
  HRESULT hr = ConfigureSourceReaderStream(mSourceReader,
                                           MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                           mUseHwAccel ? MFVideoFormat_NV12 : MFVideoFormat_YV12,
                                           MP4VideoTypes,
                                           NS_ARRAY_LENGTH(MP4VideoTypes));
  if (FAILED(hr)) {
    LOG("Failed to configured video output");
    return hr;
  }

  RefPtr<IMFMediaType> mediaType;
  hr = mSourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                          byRef(mediaType));
  if (FAILED(hr)) {
    NS_WARNING("Failed to get configured video media type");
    return hr;
  }

  if (FAILED(ConfigureVideoFrameGeometry(mediaType))) {
    NS_WARNING("Failed configured video frame dimensions");
    return hr;
  }

  LOG("Successfully configured video stream");

  mHasVideo = mInfo.mHasVideo = true;

  return S_OK;
}

void
WMFReader::GetSupportedAudioCodecs(const GUID** aCodecs, uint32_t* aNumCodecs)
{
  MOZ_ASSERT(aCodecs);
  MOZ_ASSERT(aNumCodecs);

  if (mIsMP3Enabled) {
    static const GUID codecs[] = {
      MFAudioFormat_AAC,
      MFAudioFormat_MP3
    };
    *aCodecs = codecs;
    *aNumCodecs = NS_ARRAY_LENGTH(codecs);
  } else {
    static const GUID codecs[] = {
      MFAudioFormat_AAC
    };
    *aCodecs = codecs;
    *aNumCodecs = NS_ARRAY_LENGTH(codecs);
  }
}

HRESULT
WMFReader::ConfigureAudioDecoder()
{
  NS_ASSERTION(mSourceReader, "Must have a SourceReader before configuring decoders!");

  if (!mSourceReader ||
      !SourceReaderHasStream(mSourceReader, MF_SOURCE_READER_FIRST_AUDIO_STREAM)) {
    
    return S_OK;
  }

  const GUID* codecs;
  uint32_t numCodecs = 0;
  GetSupportedAudioCodecs(&codecs, &numCodecs);

  HRESULT hr = ConfigureSourceReaderStream(mSourceReader,
                                           MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                           MFAudioFormat_Float,
                                           codecs,
                                           numCodecs);
  if (FAILED(hr)) {
    NS_WARNING("Failed to configure WMF Audio decoder for PCM output");
    return hr;
  }

  RefPtr<IMFMediaType> mediaType;
  hr = mSourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                          byRef(mediaType));
  if (FAILED(hr)) {
    NS_WARNING("Failed to get configured audio media type");
    return hr;
  }

  mAudioRate = MFGetAttributeUINT32(mediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
  mAudioChannels = MFGetAttributeUINT32(mediaType, MF_MT_AUDIO_NUM_CHANNELS, 0);
  mAudioBytesPerSample = MFGetAttributeUINT32(mediaType, MF_MT_AUDIO_BITS_PER_SAMPLE, 16) / 8;

  mInfo.mAudioChannels = mAudioChannels;
  mInfo.mAudioRate = mAudioRate;
  mHasAudio = mInfo.mHasAudio = true;

  LOG("Successfully configured audio stream. rate=%u channels=%u bitsPerSample=%u",
      mAudioRate, mAudioChannels, mAudioBytesPerSample);

  return S_OK;
}

nsresult
WMFReader::ReadMetadata(VideoInfo* aInfo,
                        MetadataTags** aTags)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  LOG("WMFReader::ReadMetadata()");
  HRESULT hr;

  RefPtr<IMFAttributes> attr;
  hr = wmf::MFCreateAttributes(byRef(attr), 1);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = attr->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, mSourceReaderCallback);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  if (mUseHwAccel) {
    hr = attr->SetUnknown(MF_SOURCE_READER_D3D_MANAGER,
                          mDXVA2Manager->GetDXVADeviceManager());
    if (FAILED(hr)) {
      LOG("Failed to set DXVA2 D3D Device manager on source reader attributes");
      mUseHwAccel = false;
    }
  }

  hr = wmf::MFCreateSourceReaderFromByteStream(mByteStream, attr, byRef(mSourceReader));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = ConfigureVideoDecoder();
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = ConfigureAudioDecoder();
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  if (mUseHwAccel && mInfo.mHasVideo) {
    RefPtr<IMFTransform> videoDecoder;
    hr = mSourceReader->GetServiceForStream(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                            GUID_NULL,
                                            IID_IMFTransform,
                                            (void**)(IMFTransform**)(byRef(videoDecoder)));

    if (SUCCEEDED(hr)) {
      ULONG_PTR manager = ULONG_PTR(mDXVA2Manager->GetDXVADeviceManager());
      hr = videoDecoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER,
                                        manager);
    }
    if (FAILED(hr)) {
      LOG("Failed to set DXVA2 D3D Device manager on decoder");
      mUseHwAccel = false;
    }
  }
  if (mInfo.mHasVideo) {
    LOG("Using DXVA: %s", (mUseHwAccel ? "Yes" : "No"));
  }

  
  NS_ENSURE_TRUE(mInfo.mHasAudio || mInfo.mHasVideo, NS_ERROR_FAILURE);

  int64_t duration = 0;
  if (SUCCEEDED(GetSourceReaderDuration(mSourceReader, duration))) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(duration);
  }

  hr = GetSourceReaderCanSeek(mSourceReader, mCanSeek);
  NS_ASSERTION(SUCCEEDED(hr), "Can't determine if resource is seekable");

  *aInfo = mInfo;
  *aTags = nullptr;
  
  

  return NS_OK;
}

static int64_t
GetSampleDuration(IMFSample* aSample)
{
  int64_t duration = 0;
  aSample->GetSampleDuration(&duration);
  return HNsToUsecs(duration);
}

HRESULT
HNsToFrames(int64_t aHNs, uint32_t aRate, int64_t* aOutFrames)
{
  MOZ_ASSERT(aOutFrames);
  const int64_t HNS_PER_S = USECS_PER_S * 10;
  CheckedInt<int64_t> i = aHNs;
  i *= aRate;
  i /= HNS_PER_S;
  NS_ENSURE_TRUE(i.isValid(), E_FAIL);
  *aOutFrames = i.value();
  return S_OK;
}

HRESULT
FramesToUsecs(int64_t aSamples, uint32_t aRate, int64_t* aOutUsecs)
{
  MOZ_ASSERT(aOutUsecs);
  CheckedInt<int64_t> i = aSamples;
  i *= USECS_PER_S;
  i /= aRate;
  NS_ENSURE_TRUE(i.isValid(), E_FAIL);
  *aOutUsecs = i.value();
  return S_OK;
}

bool
WMFReader::DecodeAudioData()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  HRESULT hr;
  hr = mSourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                 0, 
                                 0, 
                                 nullptr,
                                 nullptr,
                                 nullptr);

  if (FAILED(hr)) {
    LOG("WMFReader::DecodeAudioData() ReadSample failed with hr=0x%x", hr);
    
    mAudioQueue.Finish();
    return false;
  }

  DWORD flags = 0;
  LONGLONG timestampHns = 0;
  RefPtr<IMFSample> sample;
  hr = mSourceReaderCallback->Wait(&flags, &timestampHns, byRef(sample));
  if (FAILED(hr) ||
      (flags & MF_SOURCE_READERF_ERROR) ||
      (flags & MF_SOURCE_READERF_ENDOFSTREAM) ||
      (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)) {
    LOG("WMFReader::DecodeAudioData() ReadSample failed with hr=0x%x flags=0x%x",
        hr, flags);
    
    mAudioQueue.Finish();
    return false;
  }

  if (!sample) {
    
    return true;
  }

  RefPtr<IMFMediaBuffer> buffer;
  hr = sample->ConvertToContiguousBuffer(byRef(buffer));
  NS_ENSURE_TRUE(SUCCEEDED(hr), false);

  BYTE* data = nullptr; 
  DWORD maxLength = 0, currentLength = 0;
  hr = buffer->Lock(&data, &maxLength, &currentLength);
  NS_ENSURE_TRUE(SUCCEEDED(hr), false);

  uint32_t numFrames = currentLength / mAudioBytesPerSample / mAudioChannels;
  NS_ASSERTION(sizeof(AudioDataValue) == mAudioBytesPerSample, "Size calculation is wrong");
  nsAutoArrayPtr<AudioDataValue> pcmSamples(new AudioDataValue[numFrames * mAudioChannels]);
  memcpy(pcmSamples.get(), data, currentLength);
  buffer->Unlock();

  
  
  

  
  
  
  UINT32 discontinuity = false;
  sample->GetUINT32(MFSampleExtension_Discontinuity, &discontinuity);
  if (mMustRecaptureAudioPosition || discontinuity) {
    mAudioFrameSum = 0;
    hr = HNsToFrames(timestampHns, mAudioRate, &mAudioFrameOffset);
    NS_ENSURE_TRUE(SUCCEEDED(hr), false);
    mMustRecaptureAudioPosition = false;
  }

  int64_t timestamp;
  hr = FramesToUsecs(mAudioFrameOffset + mAudioFrameSum, mAudioRate, &timestamp);
  NS_ENSURE_TRUE(SUCCEEDED(hr), false);

  mAudioFrameSum += numFrames;

  int64_t duration;
  hr = FramesToUsecs(numFrames, mAudioRate, &duration);
  NS_ENSURE_TRUE(SUCCEEDED(hr), false);

  mAudioQueue.Push(new AudioData(mDecoder->GetResource()->Tell(),
                                 timestamp,
                                 duration,
                                 numFrames,
                                 pcmSamples.forget(),
                                 mAudioChannels));

  #ifdef LOG_SAMPLE_DECODE
  LOG("Decoded audio sample! timestamp=%lld duration=%lld currentLength=%u",
      timestamp, duration, currentLength);
  #endif

  return true;
}

HRESULT
WMFReader::CreateBasicVideoFrame(IMFSample* aSample,
                                 int64_t aTimestampUsecs,
                                 int64_t aDurationUsecs,
                                 int64_t aOffsetBytes,
                                 VideoData** aOutVideoData)
{
  NS_ENSURE_TRUE(aSample, E_POINTER);
  NS_ENSURE_TRUE(aOutVideoData, E_POINTER);

  *aOutVideoData = nullptr;

  HRESULT hr;
  RefPtr<IMFMediaBuffer> buffer;

  
  hr = aSample->ConvertToContiguousBuffer(byRef(buffer));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  
  
  BYTE* data = nullptr;
  LONG stride = 0;
  RefPtr<IMF2DBuffer> twoDBuffer;
  hr = buffer->QueryInterface(static_cast<IMF2DBuffer**>(byRef(twoDBuffer)));
  if (SUCCEEDED(hr)) {
    hr = twoDBuffer->Lock2D(&data, &stride);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
  } else {
    hr = buffer->Lock(&data, NULL, NULL);
    NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
    stride = mVideoStride;
  }

  
  
  VideoData::YCbCrBuffer b;

  
  b.mPlanes[0].mData = data;
  b.mPlanes[0].mStride = stride;
  b.mPlanes[0].mHeight = mVideoHeight;
  b.mPlanes[0].mWidth = mVideoWidth;
  b.mPlanes[0].mOffset = 0;
  b.mPlanes[0].mSkip = 0;

  
  
  uint32_t padding = 0;
  if (mVideoHeight % 16 != 0) {
    padding = 16 - (mVideoHeight % 16);
  }
  uint32_t y_size = stride * (mVideoHeight + padding);
  uint32_t v_size = stride * (mVideoHeight + padding) / 4;
  uint32_t halfStride = (stride + 1) / 2;
  uint32_t halfHeight = (mVideoHeight + 1) / 2;
  uint32_t halfWidth = (mVideoWidth + 1) / 2;

  
  b.mPlanes[1].mData = data + y_size + v_size;
  b.mPlanes[1].mStride = halfStride;
  b.mPlanes[1].mHeight = halfHeight;
  b.mPlanes[1].mWidth = halfWidth;
  b.mPlanes[1].mOffset = 0;
  b.mPlanes[1].mSkip = 0;

  
  b.mPlanes[2].mData = data + y_size;
  b.mPlanes[2].mStride = halfStride;
  b.mPlanes[2].mHeight = halfHeight;
  b.mPlanes[2].mWidth = halfWidth;
  b.mPlanes[2].mOffset = 0;
  b.mPlanes[2].mSkip = 0;

  VideoData *v = VideoData::Create(mInfo,
                                   mDecoder->GetImageContainer(),
                                   aOffsetBytes,
                                   aTimestampUsecs,
                                   aTimestampUsecs + aDurationUsecs,
                                   b,
                                   false,
                                   -1,
                                   mPictureRegion);
  if (twoDBuffer) {
    twoDBuffer->Unlock2D();
  } else {
    buffer->Unlock();
  }

  *aOutVideoData = v;

  return S_OK;
}

HRESULT
WMFReader::CreateD3DVideoFrame(IMFSample* aSample,
                               int64_t aTimestampUsecs,
                               int64_t aDurationUsecs,
                               int64_t aOffsetBytes,
                               VideoData** aOutVideoData)
{
  NS_ENSURE_TRUE(aSample, E_POINTER);
  NS_ENSURE_TRUE(aOutVideoData, E_POINTER);
  NS_ENSURE_TRUE(mDXVA2Manager, E_ABORT);
  NS_ENSURE_TRUE(mUseHwAccel, E_ABORT);

  *aOutVideoData = nullptr;
  HRESULT hr;

  nsRefPtr<Image> image;
  hr = mDXVA2Manager->CopyToImage(aSample,
                                  mPictureRegion,
                                  mDecoder->GetImageContainer(),
                                  getter_AddRefs(image));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
  NS_ENSURE_TRUE(image, E_FAIL);

  VideoData *v = VideoData::CreateFromImage(mInfo,
                                            mDecoder->GetImageContainer(),
                                            aOffsetBytes,
                                            aTimestampUsecs,
                                            aTimestampUsecs + aDurationUsecs,
                                            image.forget(),
                                            false,
                                            -1,
                                            mPictureRegion);

  NS_ENSURE_TRUE(v, E_FAIL);
  *aOutVideoData = v;

  return S_OK;
}

bool
WMFReader::DecodeVideoFrame(bool &aKeyframeSkip,
                            int64_t aTimeThreshold)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  
  
  uint32_t parsed = 0, decoded = 0;
  AbstractMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  HRESULT hr;

  hr = mSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                 0, 
                                 0, 
                                 nullptr,
                                 nullptr,
                                 nullptr);
  if (FAILED(hr)) {
    LOG("WMFReader::DecodeVideoData() ReadSample failed with hr=0x%x", hr);
    
    mVideoQueue.Finish();
    return false;
  }

  DWORD flags = 0;
  LONGLONG timestampHns = 0;
  RefPtr<IMFSample> sample;
  hr = mSourceReaderCallback->Wait(&flags, &timestampHns, byRef(sample));

  if (flags & MF_SOURCE_READERF_ERROR) {
    NS_WARNING("WMFReader: Catastrophic failure reading video sample");
    
    mVideoQueue.Finish();
    return false;
  }

  if (FAILED(hr)) {
    
    return true;
  }

  if (!sample) {
    if ((flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
      LOG("WMFReader; Null sample after video decode, at end of stream");
      
      mVideoQueue.Finish();
      return false;
    }
    LOG("WMFReader; Null sample after video decode. Maybe insufficient data...");
    return true;
  }

  if ((flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)) {
    LOG("WMFReader: Video media type changed!");
    RefPtr<IMFMediaType> mediaType;
    hr = mSourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                            byRef(mediaType));
    if (FAILED(hr) ||
        FAILED(ConfigureVideoFrameGeometry(mediaType))) {
      NS_WARNING("Failed to reconfigure video media type");
      mVideoQueue.Finish();
      return false;
    }
  }

  int64_t timestamp = HNsToUsecs(timestampHns);
  if (timestamp < aTimeThreshold) {
    return true;
  }
  int64_t offset = mDecoder->GetResource()->Tell();
  int64_t duration = GetSampleDuration(sample);

  VideoData* v = nullptr;
  if (mUseHwAccel) {
    hr = CreateD3DVideoFrame(sample, timestamp, duration, offset, &v);
  } else {
    hr = CreateBasicVideoFrame(sample, timestamp, duration, offset, &v);
  }
  NS_ENSURE_TRUE(SUCCEEDED(hr) && v, false);

  parsed++;
  decoded++;
  mVideoQueue.Push(v);

  #ifdef LOG_SAMPLE_DECODE
  LOG("Decoded video sample timestamp=%lld duration=%lld stride=%d height=%u flags=%u",
      timestamp, duration, mVideoStride, mVideoHeight, flags);
  #endif

  if ((flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
    
    mVideoQueue.Finish();
    LOG("End of video stream");
    return false;
  }

  return true;
}

nsresult
WMFReader::Seek(int64_t aTargetUs,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime)
{
  LOG("WMFReader::Seek() %lld", aTargetUs);

  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  if (!mCanSeek) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = ResetDecode();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  mMustRecaptureAudioPosition = true;

  AutoPropVar var;
  HRESULT hr = InitPropVariantFromInt64(UsecsToHNs(aTargetUs), &var);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  hr = mSourceReader->SetCurrentPosition(GUID_NULL, var);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  return DecodeToTarget(aTargetUs);
}

nsresult
WMFReader::GetBuffered(mozilla::dom::TimeRanges* aBuffered, int64_t aStartTime)
{
  MediaResource* stream = mDecoder->GetResource();
  int64_t durationUs = 0;
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    durationUs = mDecoder->GetMediaDuration();
  }
  GetEstimatedBufferedTimeRanges(stream, durationUs, aBuffered);
  return NS_OK;
}

} 
