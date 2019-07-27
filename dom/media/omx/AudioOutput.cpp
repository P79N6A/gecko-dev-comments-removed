


















#include <stagefright/foundation/ADebug.h>
#include "AudioOutput.h"

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gAudioOffloadPlayerLog;
#define AUDIO_OFFLOAD_LOG(type, msg) \
  PR_LOG(gAudioOffloadPlayerLog, type, msg)
#else
#define AUDIO_OFFLOAD_LOG(type, msg)
#endif

using namespace android;

AudioOutput::AudioOutput(int aSessionId, int aUid) :
  mCallback(nullptr),
  mCallbackCookie(nullptr),
  mCallbackData(nullptr),
  mSessionId(aSessionId),
  mUid(aUid)
{
#ifdef PR_LOGGING
  if (!gAudioOffloadPlayerLog) {
    gAudioOffloadPlayerLog = PR_NewLogModule("AudioOffloadPlayer");
  }
#endif
}

AudioOutput::~AudioOutput()
{
  Close();
}

ssize_t AudioOutput::FrameSize() const
{
  if (!mTrack.get()) {
    return NO_INIT;
  }
  return mTrack->frameSize();
}

status_t AudioOutput::GetPosition(uint32_t *aPosition) const
{
  if (!mTrack.get()) {
    return NO_INIT;
  }
  return mTrack->getPosition(aPosition);
}

status_t AudioOutput::SetVolume(float aVolume) const
{
  if (!mTrack.get()) {
    return NO_INIT;
  }
  return mTrack->setVolume(aVolume);
}

status_t AudioOutput::SetParameters(const String8& aKeyValuePairs)
{
  if (!mTrack.get()) {
    return NO_INIT;
  }
  return mTrack->setParameters(aKeyValuePairs);
}

status_t AudioOutput::Open(uint32_t aSampleRate,
                           int aChannelCount,
                           audio_channel_mask_t aChannelMask,
                           audio_format_t aFormat,
                           AudioCallback aCb,
                           void* aCookie,
                           audio_output_flags_t aFlags,
                           const audio_offload_info_t *aOffloadInfo)
{
  mCallback = aCb;
  mCallbackCookie = aCookie;

  if (((aFlags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) == 0) || !aCb ||
      !aOffloadInfo) {
    return BAD_VALUE;
  }

  AUDIO_OFFLOAD_LOG(PR_LOG_DEBUG, ("open(%u, %d, 0x%x, 0x%x, %d 0x%x)",
      aSampleRate, aChannelCount, aChannelMask, aFormat, mSessionId, aFlags));

  if (aChannelMask == CHANNEL_MASK_USE_CHANNEL_ORDER) {
    aChannelMask = audio_channel_out_mask_from_count(aChannelCount);
    if (0 == aChannelMask) {
      AUDIO_OFFLOAD_LOG(PR_LOG_ERROR, ("open() error, can\'t derive mask for"
          " %d audio channels", aChannelCount));
      return NO_INIT;
    }
  }

  sp<AudioTrack> t;
  CallbackData* newcbd = new CallbackData(this);

  t = new AudioTrack(
      AUDIO_STREAM_MUSIC,
      aSampleRate,
      aFormat,
      aChannelMask,
      0,  
      aFlags,
      CallbackWrapper,
      newcbd,
      0,  
      mSessionId,
      AudioTrack::TRANSFER_CALLBACK,
      aOffloadInfo,
      mUid);

  if ((!t.get()) || (t->initCheck() != NO_ERROR)) {
    AUDIO_OFFLOAD_LOG(PR_LOG_ERROR, ("Unable to create audio track"));
    delete newcbd;
    return NO_INIT;
  }

  mCallbackData = newcbd;
  t->setVolume(1.0);

  mTrack = t;
  return NO_ERROR;
}

status_t AudioOutput::Start()
{
  AUDIO_OFFLOAD_LOG(PR_LOG_DEBUG, ("%s", __PRETTY_FUNCTION__));
  if (!mTrack.get()) {
    return NO_INIT;
  }
  mTrack->setVolume(1.0);
  return mTrack->start();
}

void AudioOutput::Stop()
{
  AUDIO_OFFLOAD_LOG(PR_LOG_DEBUG, ("%s", __PRETTY_FUNCTION__));
  if (mTrack.get()) {
    mTrack->stop();
  }
}

void AudioOutput::Flush()
{
  if (mTrack.get()) {
    mTrack->flush();
  }
}

void AudioOutput::Pause()
{
  if (mTrack.get()) {
    mTrack->pause();
  }
}

void AudioOutput::Close()
{
  AUDIO_OFFLOAD_LOG(PR_LOG_DEBUG, ("%s", __PRETTY_FUNCTION__));
  mTrack.clear();

  delete mCallbackData;
  mCallbackData = nullptr;
}


void AudioOutput::CallbackWrapper(int aEvent, void* aCookie, void* aInfo)
{
  CallbackData* data = (CallbackData*) aCookie;
  data->Lock();
  AudioOutput* me = data->GetOutput();
  AudioTrack::Buffer* buffer = (AudioTrack::Buffer*) aInfo;
  if (!me) {
    
    
    data->Unlock();
    if (buffer) {
      buffer->size = 0;
    }
    return;
  }

  switch(aEvent) {

    case AudioTrack::EVENT_MORE_DATA: {

      size_t actualSize = (*me->mCallback)(me, buffer->raw, buffer->size,
          me->mCallbackCookie, CB_EVENT_FILL_BUFFER);

      if (actualSize == 0 && buffer->size > 0) {
        
        
        memset(buffer->raw, 0, buffer->size);
        actualSize = buffer->size;
      }

      buffer->size = actualSize;
    } break;

    case AudioTrack::EVENT_STREAM_END:
      AUDIO_OFFLOAD_LOG(PR_LOG_DEBUG, ("Callback wrapper: EVENT_STREAM_END"));
      (*me->mCallback)(me, nullptr , 0 ,
          me->mCallbackCookie, CB_EVENT_STREAM_END);
      break;

    case AudioTrack::EVENT_NEW_IAUDIOTRACK :
      AUDIO_OFFLOAD_LOG(PR_LOG_DEBUG, ("Callback wrapper: EVENT_TEAR_DOWN"));
      (*me->mCallback)(me,  nullptr , 0 ,
          me->mCallbackCookie, CB_EVENT_TEAR_DOWN);
      break;

    default:
      AUDIO_OFFLOAD_LOG(PR_LOG_DEBUG, ("received unknown event type: %d in"
          " Callback wrapper!", aEvent));
      break;
  }

  data->Unlock();
}

} 
