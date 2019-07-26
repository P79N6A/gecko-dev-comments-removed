





#include "AudioChannelService.h"
#include "AudioChannelServiceChild.h"

#include "base/basictypes.h"

#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozilla/Util.h"

#include "mozilla/dom/ContentParent.h"

#include "base/basictypes.h"

#include "nsThreadUtils.h"

#ifdef MOZ_WIDGET_GONK
#include "nsIAudioManager.h"
#endif
using namespace mozilla;
using namespace mozilla::dom;

StaticRefPtr<AudioChannelService> gAudioChannelService;


AudioChannelService*
AudioChannelService::GetAudioChannelService()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return AudioChannelServiceChild::GetAudioChannelService();
  }

  
  if (gAudioChannelService) {
    return gAudioChannelService;
  }

  
  nsRefPtr<AudioChannelService> service = new AudioChannelService();
  NS_ENSURE_TRUE(service, nullptr);

  gAudioChannelService = service;
  return gAudioChannelService;
}

void
AudioChannelService::Shutdown()
{
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return AudioChannelServiceChild::Shutdown();
  }

  if (gAudioChannelService) {
    delete gAudioChannelService;
    gAudioChannelService = nullptr;
  }
}

NS_IMPL_ISUPPORTS0(AudioChannelService)

AudioChannelService::AudioChannelService()
: mCurrentHigherChannel(AUDIO_CHANNEL_NORMAL)
{
  mChannelCounters = new int32_t[AUDIO_CHANNEL_PUBLICNOTIFICATION+1];

  for (int i = AUDIO_CHANNEL_NORMAL;
       i <= AUDIO_CHANNEL_PUBLICNOTIFICATION;
       ++i) {
    mChannelCounters[i] = 0;
  }

  
  mMediaElements.Init();
}

AudioChannelService::~AudioChannelService()
{
  delete [] mChannelCounters;
}

void
AudioChannelService::RegisterMediaElement(nsHTMLMediaElement* aMediaElement,
                                          AudioChannelType aType)
{
  mMediaElements.Put(aMediaElement, aType);
  RegisterType(aType);
}

void
AudioChannelService::RegisterType(AudioChannelType aType)
{
  mChannelCounters[aType]++;

  
  
  Notify();
}

void
AudioChannelService::UnregisterMediaElement(nsHTMLMediaElement* aMediaElement)
{
  AudioChannelType type;
  if (!mMediaElements.Get(aMediaElement, &type)) {
    return;
  }

  mMediaElements.Remove(aMediaElement);
  UnregisterType(type);
}

void
AudioChannelService::UnregisterType(AudioChannelType aType)
{
  mChannelCounters[aType]--;
  MOZ_ASSERT(mChannelCounters[aType] >= 0);

  
  
  Notify();
}

bool
AudioChannelService::GetMuted(AudioChannelType aType, bool aElementHidden)
{
  
  if (aElementHidden) {
    switch (aType) {
      case AUDIO_CHANNEL_NORMAL:
        return true;

      case AUDIO_CHANNEL_CONTENT:
        
        if (mChannelCounters[AUDIO_CHANNEL_CONTENT] > 1)
          return true;
        break;

      case AUDIO_CHANNEL_NOTIFICATION:
      case AUDIO_CHANNEL_ALARM:
      case AUDIO_CHANNEL_TELEPHONY:
      case AUDIO_CHANNEL_RINGER:
      case AUDIO_CHANNEL_PUBLICNOTIFICATION:
        
        break;

      case AUDIO_CHANNEL_LAST:
        MOZ_NOT_REACHED();
        return false;
    }
  }

  bool muted = false;

  
  switch (aType) {
    case AUDIO_CHANNEL_NORMAL:
    case AUDIO_CHANNEL_CONTENT:
      muted = !!mChannelCounters[AUDIO_CHANNEL_NOTIFICATION] ||
              !!mChannelCounters[AUDIO_CHANNEL_ALARM] ||
              !!mChannelCounters[AUDIO_CHANNEL_TELEPHONY] ||
              !!mChannelCounters[AUDIO_CHANNEL_RINGER] ||
              !!mChannelCounters[AUDIO_CHANNEL_PUBLICNOTIFICATION];

    case AUDIO_CHANNEL_NOTIFICATION:
    case AUDIO_CHANNEL_ALARM:
    case AUDIO_CHANNEL_TELEPHONY:
    case AUDIO_CHANNEL_RINGER:
      muted = ChannelsActiveWithHigherPriorityThan(aType);

    case AUDIO_CHANNEL_PUBLICNOTIFICATION:
      break;

    case AUDIO_CHANNEL_LAST:
      MOZ_NOT_REACHED();
      return false;
  }

  
  if (!muted) {

    
    AudioChannelType higher = AUDIO_CHANNEL_NORMAL;
    for (int32_t type = AUDIO_CHANNEL_NORMAL;
         type <= AUDIO_CHANNEL_PUBLICNOTIFICATION;
         ++type) {
      if (mChannelCounters[type]) {
        higher = (AudioChannelType)type;
      }
    }

    if (higher != mCurrentHigherChannel) {
      mCurrentHigherChannel = higher;

      nsString channelName;
      channelName.AssignASCII(ChannelName(mCurrentHigherChannel));

      nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
      obs->NotifyObservers(nullptr, "audio-channel-changed", channelName.get());
    }
  }

  return muted;
}


static PLDHashOperator
NotifyEnumerator(nsHTMLMediaElement* aElement,
                 AudioChannelType aType, void* aData)
{
  if (aElement) {
    aElement->NotifyAudioChannelStateChanged();
  }
  return PL_DHASH_NEXT;
}

void
AudioChannelService::Notify()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  mMediaElements.EnumerateRead(NotifyEnumerator, nullptr);

  
  nsTArray<ContentParent*> children;
  ContentParent::GetAll(children);
  for (uint32_t i = 0; i < children.Length(); i++) {
    unused << children[i]->SendAudioChannelNotify();
  }
}

bool
AudioChannelService::ChannelsActiveWithHigherPriorityThan(AudioChannelType aType)
{
  for (int i = AUDIO_CHANNEL_PUBLICNOTIFICATION;
       i != AUDIO_CHANNEL_CONTENT; --i) {
    if (i == aType) {
      return false;
    }

    if (mChannelCounters[i]) {
      return true;
    }
  }

  return false;
}

const char*
AudioChannelService::ChannelName(AudioChannelType aType)
{
  static struct {
    int32_t type;
    const char* value;
  } ChannelNameTable[] = {
    { AUDIO_CHANNEL_NORMAL,             "normal" },
    { AUDIO_CHANNEL_CONTENT,            "normal" },
    { AUDIO_CHANNEL_NOTIFICATION,       "notification" },
    { AUDIO_CHANNEL_ALARM,              "alarm" },
    { AUDIO_CHANNEL_TELEPHONY,          "telephony" },
    { AUDIO_CHANNEL_RINGER,             "ringer" },
    { AUDIO_CHANNEL_PUBLICNOTIFICATION, "publicnotification" },
    { -1,                               "unknown" }
  };

  for (int i = AUDIO_CHANNEL_NORMAL; ; ++i) {
    if (ChannelNameTable[i].type == aType ||
        ChannelNameTable[i].type == -1) {
      return ChannelNameTable[i].value;
    }
  }

  NS_NOTREACHED("Execution should not reach here!");
  return nullptr;
}

#ifdef MOZ_WIDGET_GONK
void
AudioChannelService::SetPhoneInCall(bool aActive)
{
  
  if (aActive) {
    mChannelCounters[AUDIO_CHANNEL_TELEPHONY] = 1;
  } else {
    mChannelCounters[AUDIO_CHANNEL_TELEPHONY] = 0;
  }
  Notify();
}
#endif

