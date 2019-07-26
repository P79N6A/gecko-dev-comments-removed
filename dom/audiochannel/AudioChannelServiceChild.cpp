





#include "AudioChannelServiceChild.h"

#include "base/basictypes.h"

#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozilla/Util.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "nsIObserverService.h"
#include "nsThreadUtils.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::hal;

StaticRefPtr<AudioChannelServiceChild> gAudioChannelServiceChild;


AudioChannelService*
AudioChannelServiceChild::GetAudioChannelService()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (gAudioChannelServiceChild) {
    return gAudioChannelServiceChild;
  }

  
  nsRefPtr<AudioChannelServiceChild> service = new AudioChannelServiceChild();
  NS_ENSURE_TRUE(service, nullptr);

  gAudioChannelServiceChild = service;
  return gAudioChannelServiceChild;
}

void
AudioChannelServiceChild::Shutdown()
{
  if (gAudioChannelServiceChild) {
    gAudioChannelServiceChild = nullptr;
  }
}

AudioChannelServiceChild::AudioChannelServiceChild()
{
}

AudioChannelServiceChild::~AudioChannelServiceChild()
{
}

AudioChannelState
AudioChannelServiceChild::GetState(AudioChannelAgent* aAgent, bool aElementHidden)
{
  AudioChannelAgentData* data;
  if (!mAgents.Get(aAgent, &data)) {
    return AUDIO_CHANNEL_STATE_MUTED;
  }

  ContentChild *cc = ContentChild::GetSingleton();
  AudioChannelState state = AUDIO_CHANNEL_STATE_MUTED;
  bool oldElementHidden = data->mElementHidden;

  UpdateChannelType(data->mType, CONTENT_PROCESS_ID_MAIN, aElementHidden, oldElementHidden);

  
  data->mElementHidden = aElementHidden;

  if (cc) {
    cc->SendAudioChannelGetState(data->mType, aElementHidden, oldElementHidden, &state);
  }
  data->mState = state;

  if (cc) {
    cc->SendAudioChannelChangedNotification();
  }

  return state;
}

void
AudioChannelServiceChild::RegisterAudioChannelAgent(AudioChannelAgent* aAgent,
                                                    AudioChannelType aType)
{
  MOZ_ASSERT(aType != AUDIO_CHANNEL_DEFAULT);

  AudioChannelService::RegisterAudioChannelAgent(aAgent, aType);

  ContentChild *cc = ContentChild::GetSingleton();
  if (cc) {
    cc->SendAudioChannelRegisterType(aType);
  }

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->NotifyObservers(nullptr, "audio-channel-agent-changed", nullptr);
  }
}

void
AudioChannelServiceChild::UnregisterAudioChannelAgent(AudioChannelAgent* aAgent)
{
  AudioChannelAgentData *pData;
  if (!mAgents.Get(aAgent, &pData)) {
    return;
  }

  
  
  AudioChannelAgentData data(*pData);

  AudioChannelService::UnregisterAudioChannelAgent(aAgent);

  ContentChild *cc = ContentChild::GetSingleton();
  if (cc) {
    cc->SendAudioChannelUnregisterType(data.mType, data.mElementHidden);
  }

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->NotifyObservers(nullptr, "audio-channel-agent-changed", nullptr);
  }
}

void
AudioChannelServiceChild::SetDefaultVolumeControlChannel(
  AudioChannelType aType, bool aHidden)
{
  ContentChild *cc = ContentChild::GetSingleton();
  if (cc) {
    cc->SendAudioChannelChangeDefVolChannel(aType, aHidden);
  }
}
