



#include "AudioChannelAgent.h"
#include "AudioChannelCommon.h"
#include "AudioChannelService.h"

using namespace mozilla::dom;

NS_IMPL_ISUPPORTS1(AudioChannelAgent, nsIAudioChannelAgent)

static nsRefPtr<AudioChannelService> gAudioChannelService;

AudioChannelAgent::AudioChannelAgent()
  : mCallback(nullptr)
  , mAudioChannelType(AUDIO_AGENT_CHANNEL_ERROR)
  , mIsRegToService(false)
  , mVisible(true)
{
  gAudioChannelService = AudioChannelService::GetAudioChannelService();
}

AudioChannelAgent::~AudioChannelAgent()
{
  gAudioChannelService = nullptr;
}


NS_IMETHODIMP AudioChannelAgent::GetAudioChannelType(int32_t *aAudioChannelType)
{
  *aAudioChannelType = mAudioChannelType;
  return NS_OK;
}


NS_IMETHODIMP AudioChannelAgent::Init(int32_t channelType, nsIAudioChannelAgentCallback *callback)
{
  
  
  MOZ_STATIC_ASSERT(static_cast<AudioChannelType>(AUDIO_AGENT_CHANNEL_NORMAL) ==
                    AUDIO_CHANNEL_NORMAL &&
                    static_cast<AudioChannelType>(AUDIO_AGENT_CHANNEL_CONTENT) ==
                    AUDIO_CHANNEL_CONTENT &&
                    static_cast<AudioChannelType>(AUDIO_AGENT_CHANNEL_NOTIFICATION) ==
                    AUDIO_CHANNEL_NOTIFICATION &&
                    static_cast<AudioChannelType>(AUDIO_AGENT_CHANNEL_ALARM) ==
                    AUDIO_CHANNEL_ALARM &&
                    static_cast<AudioChannelType>(AUDIO_AGENT_CHANNEL_TELEPHONY) ==
                    AUDIO_CHANNEL_TELEPHONY &&
                    static_cast<AudioChannelType>(AUDIO_AGENT_CHANNEL_RINGER) ==
                    AUDIO_CHANNEL_RINGER &&
                    static_cast<AudioChannelType>(AUDIO_AGENT_CHANNEL_PUBLICNOTIFICATION) ==
                    AUDIO_CHANNEL_PUBLICNOTIFICATION,
                    "Enum of channel on nsIAudioChannelAgent.idl should be the same with AudioChannelCommon.h");

  if (mAudioChannelType != AUDIO_AGENT_CHANNEL_ERROR ||
      channelType > AUDIO_AGENT_CHANNEL_PUBLICNOTIFICATION ||
      channelType < AUDIO_AGENT_CHANNEL_NORMAL) {
    return NS_ERROR_FAILURE;
  }

  mAudioChannelType = channelType;
  mCallback = callback;
  return NS_OK;
}


NS_IMETHODIMP AudioChannelAgent::StartPlaying(bool *_retval)
{
  if (mAudioChannelType == AUDIO_AGENT_CHANNEL_ERROR ||
      gAudioChannelService == nullptr) {
    return NS_ERROR_FAILURE;
  }

  gAudioChannelService->RegisterAudioChannelAgent(this,
    static_cast<AudioChannelType>(mAudioChannelType));
  *_retval = !gAudioChannelService->GetMuted(static_cast<AudioChannelType>(mAudioChannelType), !mVisible);
  mIsRegToService = true;
  return NS_OK;
}


NS_IMETHODIMP AudioChannelAgent::StopPlaying(void)
{
  if (mAudioChannelType == AUDIO_AGENT_CHANNEL_ERROR ||
      mIsRegToService == false) {
    return NS_ERROR_FAILURE;
  }

  gAudioChannelService->UnregisterAudioChannelAgent(this);
  mIsRegToService = false;
  return NS_OK;
}


NS_IMETHODIMP AudioChannelAgent::SetVisibilityState(bool visible)
{
  bool oldVisibility = mVisible;

  mVisible = visible;
  if (mIsRegToService && oldVisibility != mVisible && mCallback != nullptr) {
    mCallback->CanPlayChanged(!gAudioChannelService->GetMuted(static_cast<AudioChannelType>(mAudioChannelType),
       !mVisible));
  }
  return NS_OK;
}

void AudioChannelAgent::NotifyAudioChannelStateChanged()
{
  if (mCallback != nullptr) {
    mCallback->CanPlayChanged(!gAudioChannelService->GetMuted(static_cast<AudioChannelType>(mAudioChannelType),
      !mVisible));
  }
}

