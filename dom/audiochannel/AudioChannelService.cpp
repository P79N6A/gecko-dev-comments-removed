





#include "AudioChannelService.h"
#include "AudioChannelServiceChild.h"

#include "base/basictypes.h"

#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"

#include "mozilla/dom/ContentParent.h"

#include "nsThreadUtils.h"
#include "nsHashPropertyBag.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"

#ifdef MOZ_WIDGET_GONK
#include "nsJSUtils.h"
#include "nsCxPusher.h"
#include "nsIAudioManager.h"
#include "SpeakerManagerService.h"
#define NS_AUDIOMANAGER_CONTRACTID "@mozilla.org/telephony/audiomanager;1"
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::hal;

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

NS_IMPL_ISUPPORTS2(AudioChannelService, nsIObserver, nsITimerCallback)

AudioChannelService::AudioChannelService()
: mCurrentHigherChannel(AUDIO_CHANNEL_LAST)
, mCurrentVisibleHigherChannel(AUDIO_CHANNEL_LAST)
, mPlayableHiddenContentChildID(CONTENT_PROCESS_ID_UNKNOWN)
, mDisabled(false)
, mDefChannelChildID(CONTENT_PROCESS_ID_UNKNOWN)
{
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      obs->AddObserver(this, "ipc:content-shutdown", false);
      obs->AddObserver(this, "xpcom-shutdown", false);
#ifdef MOZ_WIDGET_GONK
      
      obs->AddObserver(this, "mozsettings-changed", false);
#endif
    }
  }
}

AudioChannelService::~AudioChannelService()
{
}

void
AudioChannelService::RegisterAudioChannelAgent(AudioChannelAgent* aAgent,
                                               AudioChannelType aType,
                                               bool aWithVideo)
{
  if (mDisabled) {
    return;
  }

  MOZ_ASSERT(aType != AUDIO_CHANNEL_DEFAULT);

  AudioChannelAgentData* data = new AudioChannelAgentData(aType,
                                true ,
                                AUDIO_CHANNEL_STATE_MUTED ,
                                aWithVideo);
  mAgents.Put(aAgent, data);
  RegisterType(aType, CONTENT_PROCESS_ID_MAIN, aWithVideo);
}

void
AudioChannelService::RegisterType(AudioChannelType aType, uint64_t aChildID, bool aWithVideo)
{
  if (mDisabled) {
    return;
  }

  AudioChannelInternalType type = GetInternalType(aType, true);
  mChannelCounters[type].AppendElement(aChildID);

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    
    
    if (mDeferTelChannelTimer && aType == AUDIO_CHANNEL_TELEPHONY) {
      mDeferTelChannelTimer->Cancel();
      mDeferTelChannelTimer = nullptr;
      UnregisterTypeInternal(aType, mTimerElementHidden, mTimerChildID, false);
    }

    if (aWithVideo) {
      mWithVideoChildIDs.AppendElement(aChildID);
    }

    
    
    
    if (type == AUDIO_CHANNEL_INT_CONTENT ||
        (type == AUDIO_CHANNEL_INT_NORMAL &&
         mWithVideoChildIDs.Contains(aChildID))) {
      mPlayableHiddenContentChildID = CONTENT_PROCESS_ID_UNKNOWN;
    }
    
    
    
    else if (type == AUDIO_CHANNEL_INT_CONTENT_HIDDEN &&
        mChannelCounters[AUDIO_CHANNEL_INT_CONTENT].IsEmpty()) {
      mPlayableHiddenContentChildID = aChildID;
    }

    
    
    SendAudioChannelChangedNotification(aChildID);
    Notify();
  }
}

void
AudioChannelService::UnregisterAudioChannelAgent(AudioChannelAgent* aAgent)
{
  if (mDisabled) {
    return;
  }

  nsAutoPtr<AudioChannelAgentData> data;
  mAgents.RemoveAndForget(aAgent, data);

  if (data) {
    UnregisterType(data->mType, data->mElementHidden,
                   CONTENT_PROCESS_ID_MAIN, data->mWithVideo);
  }
#ifdef MOZ_WIDGET_GONK
  bool active = AnyAudioChannelIsActive();
  for (uint32_t i = 0; i < mSpeakerManager.Length(); i++) {
    mSpeakerManager[i]->SetAudioChannelActive(active);
  }
#endif
}

void
AudioChannelService::UnregisterType(AudioChannelType aType,
                                    bool aElementHidden,
                                    uint64_t aChildID,
                                    bool aWithVideo)
{
  if (mDisabled) {
    return;
  }

  
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Default &&
      aType == AUDIO_CHANNEL_TELEPHONY &&
      (mChannelCounters[AUDIO_CHANNEL_INT_TELEPHONY_HIDDEN].Length() +
       mChannelCounters[AUDIO_CHANNEL_INT_TELEPHONY].Length()) == 1) {
    mTimerElementHidden = aElementHidden;
    mTimerChildID = aChildID;
    mDeferTelChannelTimer = do_CreateInstance("@mozilla.org/timer;1");
    mDeferTelChannelTimer->InitWithCallback(this, 1500, nsITimer::TYPE_ONE_SHOT);
    return;
  }

  UnregisterTypeInternal(aType, aElementHidden, aChildID, aWithVideo);
}

void
AudioChannelService::UnregisterTypeInternal(AudioChannelType aType,
                                            bool aElementHidden,
                                            uint64_t aChildID,
                                            bool aWithVideo)
{
  
  
  AudioChannelInternalType type = GetInternalType(aType, aElementHidden);
  MOZ_ASSERT(mChannelCounters[type].Contains(aChildID));
  mChannelCounters[type].RemoveElement(aChildID);

  
  
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    
    
    if (aType == AUDIO_CHANNEL_CONTENT &&
        mPlayableHiddenContentChildID == aChildID &&
        !mChannelCounters[AUDIO_CHANNEL_INT_CONTENT_HIDDEN].Contains(aChildID)) {
      mPlayableHiddenContentChildID = CONTENT_PROCESS_ID_UNKNOWN;
    }

    if (aWithVideo) {
      MOZ_ASSERT(mWithVideoChildIDs.Contains(aChildID));
      mWithVideoChildIDs.RemoveElement(aChildID);
    }

    SendAudioChannelChangedNotification(aChildID);
    Notify();
  }
}

void
AudioChannelService::UpdateChannelType(AudioChannelType aType,
                                       uint64_t aChildID,
                                       bool aElementHidden,
                                       bool aElementWasHidden)
{
  
  AudioChannelInternalType newType = GetInternalType(aType, aElementHidden);
  AudioChannelInternalType oldType = GetInternalType(aType, aElementWasHidden);

  if (newType != oldType) {
    mChannelCounters[newType].AppendElement(aChildID);
    MOZ_ASSERT(mChannelCounters[oldType].Contains(aChildID));
    mChannelCounters[oldType].RemoveElement(aChildID);
  }

  
  
  
  if (newType == AUDIO_CHANNEL_INT_CONTENT ||
      (newType == AUDIO_CHANNEL_INT_NORMAL &&
       mWithVideoChildIDs.Contains(aChildID))) {
    mPlayableHiddenContentChildID = CONTENT_PROCESS_ID_UNKNOWN;
  }
  
  
  
  else if (oldType == AUDIO_CHANNEL_INT_CONTENT &&
      newType == AUDIO_CHANNEL_INT_CONTENT_HIDDEN &&
      mChannelCounters[AUDIO_CHANNEL_INT_CONTENT].IsEmpty()) {
    mPlayableHiddenContentChildID = aChildID;
  }
}

AudioChannelState
AudioChannelService::GetState(AudioChannelAgent* aAgent, bool aElementHidden)
{
  AudioChannelAgentData* data;
  if (!mAgents.Get(aAgent, &data)) {
    return AUDIO_CHANNEL_STATE_MUTED;
  }

  bool oldElementHidden = data->mElementHidden;
  
  data->mElementHidden = aElementHidden;

  data->mState = GetStateInternal(data->mType, CONTENT_PROCESS_ID_MAIN,
                                aElementHidden, oldElementHidden);
  return data->mState;
}

AudioChannelState
AudioChannelService::GetStateInternal(AudioChannelType aType, uint64_t aChildID,
                                      bool aElementHidden, bool aElementWasHidden)
{
  UpdateChannelType(aType, aChildID, aElementHidden, aElementWasHidden);

  
  AudioChannelInternalType newType = GetInternalType(aType, aElementHidden);
  AudioChannelInternalType oldType = GetInternalType(aType, aElementWasHidden);

  if (newType != oldType &&
      (aType == AUDIO_CHANNEL_CONTENT ||
       (aType == AUDIO_CHANNEL_NORMAL &&
        mWithVideoChildIDs.Contains(aChildID)))) {
    Notify();
  }

  SendAudioChannelChangedNotification(aChildID);

  
  if (!aElementHidden) {
    if (CheckVolumeFadedCondition(newType, aElementHidden)) {
      return AUDIO_CHANNEL_STATE_FADED;
    }
    return AUDIO_CHANNEL_STATE_NORMAL;
  }

  
  if (newType == AUDIO_CHANNEL_INT_NORMAL_HIDDEN ||
      (newType == AUDIO_CHANNEL_INT_CONTENT_HIDDEN &&
       
       
       
       
       
       
       
       !(mChannelCounters[AUDIO_CHANNEL_INT_CONTENT].Contains(aChildID) ||
         (mChannelCounters[AUDIO_CHANNEL_INT_CONTENT].IsEmpty() &&
          mPlayableHiddenContentChildID == aChildID)))) {
    return AUDIO_CHANNEL_STATE_MUTED;
  }

  
  
  if (ChannelsActiveWithHigherPriorityThan(newType)) {
    MOZ_ASSERT(newType != AUDIO_CHANNEL_INT_NORMAL_HIDDEN);
    if (CheckVolumeFadedCondition(newType, aElementHidden)) {
      return AUDIO_CHANNEL_STATE_FADED;
    }
    return AUDIO_CHANNEL_STATE_MUTED;
  }

  return AUDIO_CHANNEL_STATE_NORMAL;
}

bool
AudioChannelService::CheckVolumeFadedCondition(AudioChannelInternalType aType,
                                               bool aElementHidden)
{
  
  if (aType > AUDIO_CHANNEL_INT_CONTENT_HIDDEN) {
    return false;
  }

  
  
  if (mChannelCounters[AUDIO_CHANNEL_INT_NOTIFICATION].IsEmpty() &&
      mChannelCounters[AUDIO_CHANNEL_INT_NOTIFICATION_HIDDEN].IsEmpty()) {
    return false;
  }

  
  
  if (aElementHidden == false) {
   return true;
  }

  
  
  for (int i = AUDIO_CHANNEL_INT_LAST - 1;
    i != AUDIO_CHANNEL_INT_NOTIFICATION_HIDDEN; --i) {
    if (!mChannelCounters[i].IsEmpty()) {
      return false;
    }
  }

  return true;
}

bool
AudioChannelService::ContentOrNormalChannelIsActive()
{
  return !mChannelCounters[AUDIO_CHANNEL_INT_CONTENT].IsEmpty() ||
         !mChannelCounters[AUDIO_CHANNEL_INT_CONTENT_HIDDEN].IsEmpty() ||
         !mChannelCounters[AUDIO_CHANNEL_INT_NORMAL].IsEmpty();
}

bool
AudioChannelService::ProcessContentOrNormalChannelIsActive(uint64_t aChildID)
{
  return mChannelCounters[AUDIO_CHANNEL_INT_CONTENT].Contains(aChildID) ||
         mChannelCounters[AUDIO_CHANNEL_INT_CONTENT_HIDDEN].Contains(aChildID) ||
         mChannelCounters[AUDIO_CHANNEL_INT_NORMAL].Contains(aChildID);
}

void
AudioChannelService::SetDefaultVolumeControlChannel(AudioChannelType aType,
                                                    bool aHidden)
{
  SetDefaultVolumeControlChannelInternal(aType, aHidden, CONTENT_PROCESS_ID_MAIN);
}

void
AudioChannelService::SetDefaultVolumeControlChannelInternal(
  AudioChannelType aType, bool aHidden, uint64_t aChildID)
{
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return;
  }

  
  
  
  if (!aHidden && mDefChannelChildID != aChildID) {
    return;
  }

  mDefChannelChildID = aChildID;
  nsString channelName;
  channelName.AssignASCII(ChannelName(aType));
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->NotifyObservers(nullptr, "default-volume-channel-changed",
                         channelName.get());
  }
}

void
AudioChannelService::SendAudioChannelChangedNotification(uint64_t aChildID)
{
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return;
  }

  nsRefPtr<nsHashPropertyBag> props = new nsHashPropertyBag();
  props->SetPropertyAsUint64(NS_LITERAL_STRING("childID"), aChildID);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->NotifyObservers(static_cast<nsIWritablePropertyBag*>(props),
                         "audio-channel-process-changed", nullptr);
  }

  
  AudioChannelType higher = AUDIO_CHANNEL_LAST;

  
  if (!mChannelCounters[AUDIO_CHANNEL_INT_PUBLICNOTIFICATION].IsEmpty()) {
    higher = AUDIO_CHANNEL_PUBLICNOTIFICATION;
  }

  else if (!mChannelCounters[AUDIO_CHANNEL_INT_RINGER].IsEmpty()) {
    higher = AUDIO_CHANNEL_RINGER;
  }

  else if (!mChannelCounters[AUDIO_CHANNEL_INT_TELEPHONY].IsEmpty()) {
    higher = AUDIO_CHANNEL_TELEPHONY;
  }

  else if (!mChannelCounters[AUDIO_CHANNEL_INT_ALARM].IsEmpty()) {
    higher = AUDIO_CHANNEL_ALARM;
  }

  else if (!mChannelCounters[AUDIO_CHANNEL_INT_NOTIFICATION].IsEmpty()) {
    higher = AUDIO_CHANNEL_NOTIFICATION;
  }

  else if (!mChannelCounters[AUDIO_CHANNEL_INT_CONTENT].IsEmpty()) {
    higher = AUDIO_CHANNEL_CONTENT;
  }

  else if (!mChannelCounters[AUDIO_CHANNEL_INT_NORMAL].IsEmpty()) {
    higher = AUDIO_CHANNEL_NORMAL;
  }

  AudioChannelType visibleHigher = higher;

  
  if (higher == AUDIO_CHANNEL_LAST) {
    if (!mChannelCounters[AUDIO_CHANNEL_INT_PUBLICNOTIFICATION_HIDDEN].IsEmpty()) {
      higher = AUDIO_CHANNEL_PUBLICNOTIFICATION;
    }

    else if (!mChannelCounters[AUDIO_CHANNEL_INT_RINGER_HIDDEN].IsEmpty()) {
      higher = AUDIO_CHANNEL_RINGER;
    }

    else if (!mChannelCounters[AUDIO_CHANNEL_INT_TELEPHONY_HIDDEN].IsEmpty()) {
      higher = AUDIO_CHANNEL_TELEPHONY;
    }

    else if (!mChannelCounters[AUDIO_CHANNEL_INT_ALARM_HIDDEN].IsEmpty()) {
      higher = AUDIO_CHANNEL_ALARM;
    }

    else if (!mChannelCounters[AUDIO_CHANNEL_INT_NOTIFICATION_HIDDEN].IsEmpty()) {
      higher = AUDIO_CHANNEL_NOTIFICATION;
    }

    
    else if (mPlayableHiddenContentChildID != CONTENT_PROCESS_ID_UNKNOWN) {
      higher = AUDIO_CHANNEL_CONTENT;
    }
  }

  if (higher != mCurrentHigherChannel) {
    mCurrentHigherChannel = higher;

    nsString channelName;
    if (mCurrentHigherChannel != AUDIO_CHANNEL_LAST) {
      channelName.AssignASCII(ChannelName(mCurrentHigherChannel));
    } else {
      channelName.AssignLiteral("none");
    }

    if (obs) {
      obs->NotifyObservers(nullptr, "audio-channel-changed", channelName.get());
    }
  }

  if (visibleHigher != mCurrentVisibleHigherChannel) {
    mCurrentVisibleHigherChannel = visibleHigher;

    nsString channelName;
    if (mCurrentVisibleHigherChannel != AUDIO_CHANNEL_LAST) {
      channelName.AssignASCII(ChannelName(mCurrentVisibleHigherChannel));
    } else {
      channelName.AssignLiteral("none");
    }

    if (obs) {
      obs->NotifyObservers(nullptr, "visible-audio-channel-changed", channelName.get());
    }
  }
}

PLDHashOperator
AudioChannelService::NotifyEnumerator(AudioChannelAgent* aAgent,
                                      AudioChannelAgentData* aData, void* aUnused)
{
  MOZ_ASSERT(aAgent);
  aAgent->NotifyAudioChannelStateChanged();
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

NS_IMETHODIMP
AudioChannelService::Notify(nsITimer* aTimer)
{
  UnregisterTypeInternal(AUDIO_CHANNEL_TELEPHONY, mTimerElementHidden, mTimerChildID, false);
  mDeferTelChannelTimer = nullptr;
  return NS_OK;
}

bool
AudioChannelService::AnyAudioChannelIsActive()
{
  for (int i = AUDIO_CHANNEL_INT_LAST - 1;
       i >= AUDIO_CHANNEL_INT_NORMAL; --i) {
    if (!mChannelCounters[i].IsEmpty()) {
      return true;
    }
  }

  return false;
}

bool
AudioChannelService::ChannelsActiveWithHigherPriorityThan(
  AudioChannelInternalType aType)
{
  for (int i = AUDIO_CHANNEL_INT_LAST - 1;
       i != AUDIO_CHANNEL_INT_CONTENT_HIDDEN; --i) {
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
    { AUDIO_CHANNEL_CONTENT,            "content" },
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
AudioChannelService::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
{
  if (!strcmp(aTopic, "xpcom-shutdown")) {
    mDisabled = true;
  }

  if (!strcmp(aTopic, "ipc:content-shutdown")) {
    nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(aSubject);
    if (!props) {
      NS_WARNING("ipc:content-shutdown message without property bag as subject");
      return NS_OK;
    }

    int32_t index;
    uint64_t childID = 0;
    nsresult rv = props->GetPropertyAsUint64(NS_LITERAL_STRING("childID"),
                                             &childID);
    if (NS_SUCCEEDED(rv)) {
      for (int32_t type = AUDIO_CHANNEL_INT_NORMAL;
           type < AUDIO_CHANNEL_INT_LAST;
           ++type) {

        while ((index = mChannelCounters[type].IndexOf(childID)) != -1) {
          mChannelCounters[type].RemoveElementAt(index);
        }
      }

      
      
      if (mPlayableHiddenContentChildID == childID) {
        mPlayableHiddenContentChildID = CONTENT_PROCESS_ID_UNKNOWN;
      }

      while ((index = mWithVideoChildIDs.IndexOf(childID)) != -1) {
        mWithVideoChildIDs.RemoveElementAt(index);
      }

      
      

      SendAudioChannelChangedNotification(childID);
      Notify();

      if (mDefChannelChildID == childID) {
        SetDefaultVolumeControlChannelInternal(AUDIO_CHANNEL_DEFAULT,
                                               false, childID);
        mDefChannelChildID = CONTENT_PROCESS_ID_UNKNOWN;
      }
    } else {
      NS_WARNING("ipc:content-shutdown message without childID property");
    }
  }
#ifdef MOZ_WIDGET_GONK
  
  
  else if (!strcmp(aTopic, "mozsettings-changed")) {
    AutoSafeJSContext cx;
    nsDependentString dataStr(aData);
    JS::Rooted<JS::Value> val(cx);
    if (!JS_ParseJSON(cx, dataStr.get(), dataStr.Length(), &val) ||
        !val.isObject()) {
      return NS_OK;
    }

    JS::Rooted<JSObject*> obj(cx, &val.toObject());
    JS::Rooted<JS::Value> key(cx);
    if (!JS_GetProperty(cx, obj, "key", &key) ||
        !key.isString()) {
      return NS_OK;
    }

    JS::Rooted<JSString*> jsKey(cx, JS::ToString(cx, key));
    if (!jsKey) {
      return NS_OK;
    }
    nsDependentJSString keyStr;
    if (!keyStr.init(cx, jsKey) || keyStr.Find("audio.volume.", 0, false)) {
      return NS_OK;
    }

    JS::Rooted<JS::Value> value(cx);
    if (!JS_GetProperty(cx, obj, "value", &value) || !value.isInt32()) {
      return NS_OK;
    }

    nsCOMPtr<nsIAudioManager> audioManager = do_GetService(NS_AUDIOMANAGER_CONTRACTID);
    NS_ENSURE_TRUE(audioManager, NS_OK);

    int32_t index = value.toInt32();
    if (keyStr.EqualsLiteral("audio.volume.content")) {
      audioManager->SetAudioChannelVolume(AUDIO_CHANNEL_CONTENT, index);
    } else if (keyStr.EqualsLiteral("audio.volume.notification")) {
      audioManager->SetAudioChannelVolume(AUDIO_CHANNEL_NOTIFICATION, index);
    } else if (keyStr.EqualsLiteral("audio.volume.alarm")) {
      audioManager->SetAudioChannelVolume(AUDIO_CHANNEL_ALARM, index);
    } else if (keyStr.EqualsLiteral("audio.volume.telephony")) {
      audioManager->SetAudioChannelVolume(AUDIO_CHANNEL_TELEPHONY, index);
    } else if (!keyStr.EqualsLiteral("audio.volume.bt_sco")) {
      
      
      
      
      
      NS_WARNING("unexpected audio channel for volume control");
    }
  }
#endif

  return NS_OK;
}

AudioChannelService::AudioChannelInternalType
AudioChannelService::GetInternalType(AudioChannelType aType,
                                     bool aElementHidden)
{
  switch (aType) {
    case AUDIO_CHANNEL_NORMAL:
      return aElementHidden
               ? AUDIO_CHANNEL_INT_NORMAL_HIDDEN
               : AUDIO_CHANNEL_INT_NORMAL;

    case AUDIO_CHANNEL_CONTENT:
      return aElementHidden
               ? AUDIO_CHANNEL_INT_CONTENT_HIDDEN
               : AUDIO_CHANNEL_INT_CONTENT;

    case AUDIO_CHANNEL_NOTIFICATION:
      return aElementHidden
               ? AUDIO_CHANNEL_INT_NOTIFICATION_HIDDEN
               : AUDIO_CHANNEL_INT_NOTIFICATION;

    case AUDIO_CHANNEL_ALARM:
      return aElementHidden
               ? AUDIO_CHANNEL_INT_ALARM_HIDDEN
               : AUDIO_CHANNEL_INT_ALARM;

    case AUDIO_CHANNEL_TELEPHONY:
      return aElementHidden
               ? AUDIO_CHANNEL_INT_TELEPHONY_HIDDEN
               : AUDIO_CHANNEL_INT_TELEPHONY;

    case AUDIO_CHANNEL_RINGER:
      return aElementHidden
               ? AUDIO_CHANNEL_INT_RINGER_HIDDEN
               : AUDIO_CHANNEL_INT_RINGER;

    case AUDIO_CHANNEL_PUBLICNOTIFICATION:
      return aElementHidden
               ? AUDIO_CHANNEL_INT_PUBLICNOTIFICATION_HIDDEN
               : AUDIO_CHANNEL_INT_PUBLICNOTIFICATION;

    case AUDIO_CHANNEL_LAST:
    default:
      break;
  }

  MOZ_CRASH("unexpected audio channel type");
}
