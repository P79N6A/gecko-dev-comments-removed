





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
    gAudioChannelService = nullptr;
  }
}

NS_IMPL_ISUPPORTS0(AudioChannelService)

AudioChannelService::AudioChannelService()
: mCurrentHigherChannel(AUDIO_CHANNEL_NORMAL)
{
  
  mAgents.Init();

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      obs->AddObserver(this, "ipc:content-shutdown", false);
    }
  }
}

AudioChannelService::~AudioChannelService()
{
}

void
AudioChannelService::RegisterAudioChannelAgent(AudioChannelAgent* aAgent,
                                               AudioChannelType aType)
{
  mAgents.Put(aAgent, aType);
  RegisterType(aType, CONTENT_PARENT_UNKNOWN_CHILD_ID);
}

void
AudioChannelService::RegisterType(AudioChannelType aType, uint64_t aChildID)
{
  mChannelCounters[aType].AppendElement(aChildID);

  
  
  Notify();
}

void
AudioChannelService::UnregisterAudioChannelAgent(AudioChannelAgent* aAgent)
{
  AudioChannelType type;
  if (!mAgents.Get(aAgent, &type)) {
    return;
  }

  mAgents.Remove(aAgent);
  UnregisterType(type, CONTENT_PARENT_UNKNOWN_CHILD_ID);
}

void
AudioChannelService::UnregisterType(AudioChannelType aType, uint64_t aChildID)
{
  
  
  mChannelCounters[aType].RemoveElement(aChildID);

  bool isNoChannelUsed = true;
  for (int32_t type = AUDIO_CHANNEL_NORMAL;
         type <= AUDIO_CHANNEL_PUBLICNOTIFICATION;
         ++type) {
    if (!mChannelCounters[type].IsEmpty()) {
      isNoChannelUsed = false;
      break;
    }
  }

  if (isNoChannelUsed) {
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    obs->NotifyObservers(nullptr, "audio-channel-changed", NS_LITERAL_STRING("default").get());
    mCurrentHigherChannel = AUDIO_CHANNEL_NORMAL;
    return;
  }

  
  
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
      {
        
        
        uint32_t childId = CONTENT_PARENT_UNKNOWN_CHILD_ID;
        bool empty = true;
        for (uint32_t i = 0;
             i < mChannelCounters[AUDIO_CHANNEL_CONTENT].Length();
             ++i) {
          if (empty) {
            childId = mChannelCounters[AUDIO_CHANNEL_CONTENT][i];
            empty = false;
          }
          else if (childId != mChannelCounters[AUDIO_CHANNEL_CONTENT][i])
            return true;
        }
        break;
      }

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
      muted = !mChannelCounters[AUDIO_CHANNEL_NOTIFICATION].IsEmpty() ||
              !mChannelCounters[AUDIO_CHANNEL_ALARM].IsEmpty() ||
              !mChannelCounters[AUDIO_CHANNEL_TELEPHONY].IsEmpty() ||
              !mChannelCounters[AUDIO_CHANNEL_RINGER].IsEmpty() ||
              !mChannelCounters[AUDIO_CHANNEL_PUBLICNOTIFICATION].IsEmpty();
      break;

    case AUDIO_CHANNEL_NOTIFICATION:
    case AUDIO_CHANNEL_ALARM:
    case AUDIO_CHANNEL_TELEPHONY:
    case AUDIO_CHANNEL_RINGER:
      muted = ChannelsActiveWithHigherPriorityThan(aType);
      break;

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
      if (!mChannelCounters[type].IsEmpty()) {
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
NotifyEnumerator(AudioChannelAgent* aAgent,
                 AudioChannelType aType, void* aData)
{
  if (aAgent) {
    aAgent->NotifyAudioChannelStateChanged();
  }
  return PL_DHASH_NEXT;
}

void
AudioChannelService::Notify()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  mAgents.EnumerateRead(NotifyEnumerator, nullptr);

  
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

    if (!mChannelCounters[i].IsEmpty()) {
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

NS_IMETHODIMP
AudioChannelService::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* data)
{
  MOZ_ASSERT(!strcmp(aTopic, "ipc:content-shutdown"));

  nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(aSubject);
  if (!props) {
    NS_WARNING("ipc:content-shutdown message without property bag as subject");
    return NS_OK;
  }

  uint64_t childID = 0;
  nsresult rv = props->GetPropertyAsUint64(NS_LITERAL_STRING("childID"),
                                           &childID);
  if (NS_SUCCEEDED(rv)) {
    for (int32_t type = AUDIO_CHANNEL_NORMAL;
         type <= AUDIO_CHANNEL_PUBLICNOTIFICATION;
         ++type) {
      int32_t index;
      while ((index = mChannelCounters[type].IndexOf(childID)) != -1) {
        mChannelCounters[type].RemoveElementAt(index);
      }
    }

    
    

    Notify();
  } else {
    NS_WARNING("ipc:content-shutdown message without childID property");
  }

  return NS_OK;
}
