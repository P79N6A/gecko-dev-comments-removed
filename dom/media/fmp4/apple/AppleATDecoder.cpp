





#include "AppleUtils.h"
#include "MP4Reader.h"
#include "MP4Decoder.h"
#include "mp4_demuxer/DecoderData.h"
#include "AppleATDecoder.h"
#include "prlog.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetAppleMediaLog();
#define LOG(...) PR_LOG(GetAppleMediaLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

namespace mozilla {

AppleATDecoder::AppleATDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                               MediaTaskQueue* aAudioTaskQueue,
                               MediaDataDecoderCallback* aCallback)
  : mConfig(aConfig)
  , mTaskQueue(aAudioTaskQueue)
  , mCallback(aCallback)
  , mConverter(nullptr)
{
  MOZ_COUNT_CTOR(AppleATDecoder);
  LOG("Creating Apple AudioToolbox decoder");
  LOG("Audio Decoder configuration: %s %d Hz %d channels %d bits per channel",
      mConfig.mime_type,
      mConfig.samples_per_second,
      mConfig.channel_count,
      mConfig.bits_per_sample);

  if (!strcmp(mConfig.mime_type, "audio/mpeg")) {
    mFormatID = kAudioFormatMPEGLayer3;
  } else if (!strcmp(mConfig.mime_type, "audio/mp4a-latm")) {
    mFormatID = kAudioFormatMPEG4AAC;
  } else {
    mFormatID = 0;
  }
}

AppleATDecoder::~AppleATDecoder()
{
  MOZ_COUNT_DTOR(AppleATDecoder);
  MOZ_ASSERT(!mConverter);
}

nsresult
AppleATDecoder::Init()
{
  if (!mFormatID) {
    NS_ERROR("Non recognised format");
    return NS_ERROR_FAILURE;
  }
  LOG("Initializing Apple AudioToolbox decoder");
  AudioStreamBasicDescription inputFormat;
  PodZero(&inputFormat);

  if (NS_FAILED(GetInputAudioDescription(inputFormat))) {
    return NS_ERROR_FAILURE;
  }
  
  PodZero(&mOutputFormat);
  mOutputFormat.mFormatID = kAudioFormatLinearPCM;
  mOutputFormat.mSampleRate = inputFormat.mSampleRate;
  mOutputFormat.mChannelsPerFrame = inputFormat.mChannelsPerFrame;
#if defined(MOZ_SAMPLE_TYPE_FLOAT32)
  mOutputFormat.mBitsPerChannel = 32;
  mOutputFormat.mFormatFlags =
    kLinearPCMFormatFlagIsFloat |
    0;
#else
# error Unknown audio sample type
#endif
  
  mOutputFormat.mFramesPerPacket = 1;
  mOutputFormat.mBytesPerPacket = mOutputFormat.mBytesPerFrame
        = mOutputFormat.mChannelsPerFrame * mOutputFormat.mBitsPerChannel / 8;

  OSStatus rv = AudioConverterNew(&inputFormat, &mOutputFormat, &mConverter);
  if (rv) {
    LOG("Error %d constructing AudioConverter", rv);
    mConverter = nullptr;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
AppleATDecoder::Input(mp4_demuxer::MP4Sample* aSample)
{
  LOG("mp4 input sample %p %lld us %lld pts%s %llu bytes audio",
      aSample,
      aSample->duration,
      aSample->composition_timestamp,
      aSample->is_sync_point ? " keyframe" : "",
      (unsigned long long)aSample->size);

  
  mTaskQueue->Dispatch(
      NS_NewRunnableMethodWithArg<nsAutoPtr<mp4_demuxer::MP4Sample>>(
        this,
        &AppleATDecoder::SubmitSample,
        nsAutoPtr<mp4_demuxer::MP4Sample>(aSample)));

  return NS_OK;
}

nsresult
AppleATDecoder::Flush()
{
  LOG("Flushing AudioToolbox AAC decoder");
  mTaskQueue->Flush();
  OSStatus rv = AudioConverterReset(mConverter);
  if (rv) {
    LOG("Error %d resetting AudioConverter", rv);
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
AppleATDecoder::Drain()
{
  LOG("Draining AudioToolbox AAC decoder");
  mTaskQueue->AwaitIdle();
  mCallback->DrainComplete();
  return Flush();
}

nsresult
AppleATDecoder::Shutdown()
{
  LOG("Shutdown: Apple AudioToolbox AAC decoder");
  OSStatus rv = AudioConverterDispose(mConverter);
  if (rv) {
    LOG("error %d disposing of AudioConverter", rv);
    return NS_ERROR_FAILURE;
  }
  mConverter = nullptr;
  return NS_OK;
}

struct PassthroughUserData {
  UInt32 mChannels;
  UInt32 mDataSize;
  const void* mData;
  AudioStreamPacketDescription mPacket;
};



const uint32_t kNoMoreDataErr = 'MOAR';

static OSStatus
_PassthroughInputDataCallback(AudioConverterRef aAudioConverter,
                              UInt32* aNumDataPackets ,
                              AudioBufferList* aData ,
                              AudioStreamPacketDescription** aPacketDesc,
                              void* aUserData)
{
  PassthroughUserData* userData = (PassthroughUserData*)aUserData;
  if (!userData->mDataSize) {
    *aNumDataPackets = 0;
    return kNoMoreDataErr;
  }

  LOG("AudioConverter wants %u packets of audio data\n", *aNumDataPackets);

  if (aPacketDesc) {
    userData->mPacket.mStartOffset = 0;
    userData->mPacket.mVariableFramesInPacket = 0;
    userData->mPacket.mDataByteSize = userData->mDataSize;
    *aPacketDesc = &userData->mPacket;
  }

  aData->mBuffers[0].mNumberChannels = userData->mChannels;
  aData->mBuffers[0].mDataByteSize = userData->mDataSize;
  aData->mBuffers[0].mData = const_cast<void*>(userData->mData);

  
  userData->mDataSize = 0;

  return noErr;
}

void
AppleATDecoder::SubmitSample(nsAutoPtr<mp4_demuxer::MP4Sample> aSample)
{
  
  nsTArray<AudioDataValue> outputData;
  UInt32 channels = mOutputFormat.mChannelsPerFrame;
  
  
  const uint32_t MAX_AUDIO_FRAMES = 128;
  const uint32_t maxDecodedSamples = MAX_AUDIO_FRAMES * channels;

  
  nsAutoArrayPtr<AudioStreamPacketDescription>
    packets(new AudioStreamPacketDescription[MAX_AUDIO_FRAMES]);

  
  
  PassthroughUserData userData =
    { channels, (UInt32)aSample->size, aSample->data };

  
  nsAutoArrayPtr<AudioDataValue> decoded(new AudioDataValue[maxDecodedSamples]);

  do {
    AudioBufferList decBuffer;
    decBuffer.mNumberBuffers = 1;
    decBuffer.mBuffers[0].mNumberChannels = channels;
    decBuffer.mBuffers[0].mDataByteSize =
      maxDecodedSamples * sizeof(AudioDataValue);
    decBuffer.mBuffers[0].mData = decoded.get();

    
    
    UInt32 numFrames = MAX_AUDIO_FRAMES;

    OSStatus rv = AudioConverterFillComplexBuffer(mConverter,
                                                  _PassthroughInputDataCallback,
                                                  &userData,
                                                  &numFrames ,
                                                  &decBuffer,
                                                  packets.get());

    if (rv && rv != kNoMoreDataErr) {
      LOG("Error decoding audio stream: %d\n", rv);
      mCallback->Error();
      return;
    }

    if (numFrames) {
      outputData.AppendElements(decoded.get(), numFrames * channels);
      LOG("%d frames decoded", numFrames);
    }

    if (rv == kNoMoreDataErr) {
      LOG("done processing compressed packet");
      break;
    }
  } while (true);

  if (!outputData.IsEmpty()) {
    size_t numFrames = outputData.Length() / channels;
    int rate = mOutputFormat.mSampleRate;
    CheckedInt<Microseconds> duration = FramesToUsecs(numFrames, rate);
    if (!duration.isValid()) {
      NS_WARNING("Invalid count of accumulated audio samples");
      mCallback->Error();
      return;
    }

    LOG("pushed audio at time %lfs; duration %lfs\n",
        (double)aSample->composition_timestamp / USECS_PER_S,
        (double)duration.value() / USECS_PER_S);

    nsAutoArrayPtr<AudioDataValue>
      data(new AudioDataValue[outputData.Length()]);
    PodCopy(data.get(), &outputData[0], outputData.Length());
    AudioData* audio = new AudioData(aSample->byte_offset,
                                     aSample->composition_timestamp,
                                     duration.value(),
                                     numFrames,
                                     data.forget(),
                                     channels,
                                     rate);
    mCallback->Output(audio);
  }

  if (mTaskQueue->IsEmpty()) {
    mCallback->InputExhausted();
  }
}

nsresult
AppleATDecoder::GetInputAudioDescription(AudioStreamBasicDescription& aDesc)
{
  
  AudioFormatInfo formatInfo;
  PodZero(&formatInfo.mASBD);
  formatInfo.mASBD.mFormatID = mFormatID;
  if (mFormatID == kAudioFormatMPEG4AAC) {
    formatInfo.mASBD.mFormatFlags = mConfig.extended_profile;
  }
  formatInfo.mMagicCookieSize = mConfig.extra_data.length();
  formatInfo.mMagicCookie = mConfig.extra_data.begin();

  UInt32 formatListSize;
  
  
  
  
  aDesc.mFormatID = mFormatID;
  aDesc.mChannelsPerFrame = mConfig.channel_count;
  aDesc.mSampleRate = mConfig.samples_per_second;
  UInt32 inputFormatSize = sizeof(aDesc);
  OSStatus rv = AudioFormatGetProperty(kAudioFormatProperty_FormatInfo,
                                       0,
                                       NULL,
                                       &inputFormatSize,
                                       &aDesc);
  if (NS_WARN_IF(rv)) {
    return NS_ERROR_FAILURE;
  }

  
  
  rv = AudioFormatGetPropertyInfo(kAudioFormatProperty_FormatList,
                                  sizeof(formatInfo),
                                  &formatInfo,
                                  &formatListSize);
  if (rv || (formatListSize % sizeof(AudioFormatListItem))) {
    return NS_OK;
  }
  size_t listCount = formatListSize / sizeof(AudioFormatListItem);
  nsAutoArrayPtr<AudioFormatListItem> formatList(
    new AudioFormatListItem[listCount]);

  rv = AudioFormatGetProperty(kAudioFormatProperty_FormatList,
                              sizeof(formatInfo),
                              &formatInfo,
                              &formatListSize,
                              formatList);
  if (rv) {
    return NS_OK;
  }
  LOG("found %u available audio stream(s)",
      formatListSize / sizeof(AudioFormatListItem));
  
  
  
  UInt32 itemIndex;
  UInt32 indexSize = sizeof(itemIndex);
  rv = AudioFormatGetProperty(kAudioFormatProperty_FirstPlayableFormatFromList,
                              formatListSize,
                              formatList,
                              &indexSize,
                              &itemIndex);
  if (rv) {
    return NS_OK;
  }

  aDesc = formatList[itemIndex].mASBD;

  return NS_OK;
}

} 
