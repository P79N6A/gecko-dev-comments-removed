





#include "SpeakerManagerService.h"
#include "SpeakerManagerServiceChild.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozilla/dom/ContentParent.h"
#include "nsIPropertyBag2.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "AudioChannelService.h"
#include <cutils/properties.h>

#define NS_AUDIOMANAGER_CONTRACTID "@mozilla.org/telephony/audiomanager;1"
#include "nsIAudioManager.h"

using namespace mozilla;
using namespace mozilla::dom;

StaticRefPtr<SpeakerManagerService> gSpeakerManagerService;


SpeakerManagerService*
SpeakerManagerService::GetOrCreateSpeakerManagerService()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return SpeakerManagerServiceChild::GetOrCreateSpeakerManagerService();
  }

  
  if (gSpeakerManagerService) {
    return gSpeakerManagerService;
  }

  
  nsRefPtr<SpeakerManagerService> service = new SpeakerManagerService();

  gSpeakerManagerService = service;

  return gSpeakerManagerService;
}

SpeakerManagerService*
SpeakerManagerService::GetSpeakerManagerService()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return SpeakerManagerServiceChild::GetSpeakerManagerService();
  }

  return gSpeakerManagerService;
}

void
SpeakerManagerService::Shutdown()
{
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return SpeakerManagerServiceChild::Shutdown();
  }

  if (gSpeakerManagerService) {
    gSpeakerManagerService = nullptr;
  }
}

NS_IMPL_ISUPPORTS(SpeakerManagerService, nsIObserver)

void
SpeakerManagerService::ForceSpeaker(bool aEnable, uint64_t aChildId)
{
  TurnOnSpeaker(aEnable);
  if (aEnable) {
    mSpeakerStatusSet.Put(aChildId);
  }
  Notify();
  return;
}

void
SpeakerManagerService::ForceSpeaker(bool aEnable, bool aVisible)
{
  
  TurnOnSpeaker(aEnable && aVisible);
  mVisible = aVisible;
  mOrgSpeakerStatus = aEnable;
  Notify();
}

void
SpeakerManagerService::TurnOnSpeaker(bool aOn)
{
  nsCOMPtr<nsIAudioManager> audioManager = do_GetService(NS_AUDIOMANAGER_CONTRACTID);
  NS_ENSURE_TRUE_VOID(audioManager);
  int32_t phoneState;
  audioManager->GetPhoneState(&phoneState);
  int32_t forceuse = (phoneState == nsIAudioManager::PHONE_STATE_IN_CALL ||
                      phoneState == nsIAudioManager::PHONE_STATE_IN_COMMUNICATION)
                        ? nsIAudioManager::USE_COMMUNICATION : nsIAudioManager::USE_MEDIA;
  if (aOn) {
    audioManager->SetForceForUse(forceuse, nsIAudioManager::FORCE_SPEAKER);
  } else {
    audioManager->SetForceForUse(forceuse, nsIAudioManager::FORCE_NONE);
  }
}

bool
SpeakerManagerService::GetSpeakerStatus()
{
  char propQemu[PROPERTY_VALUE_MAX];
  property_get("ro.kernel.qemu", propQemu, "");
  if (!strncmp(propQemu, "1", 1)) {
    return mOrgSpeakerStatus;
  }
  nsCOMPtr<nsIAudioManager> audioManager = do_GetService(NS_AUDIOMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(audioManager, false);
  int32_t usage;
  audioManager->GetForceForUse(nsIAudioManager::USE_MEDIA, &usage);
  return usage == nsIAudioManager::FORCE_SPEAKER;
}

void
SpeakerManagerService::Notify()
{
  
  nsTArray<ContentParent*> children;
  ContentParent::GetAll(children);
  for (uint32_t i = 0; i < children.Length(); i++) {
    unused << children[i]->SendSpeakerManagerNotify();
  }

  for (uint32_t i = 0; i < mRegisteredSpeakerManagers.Length(); i++) {
    mRegisteredSpeakerManagers[i]->
      DispatchSimpleEvent(NS_LITERAL_STRING("speakerforcedchange"));
  }
}

void
SpeakerManagerService::SetAudioChannelActive(bool aIsActive)
{
  if (!aIsActive && !mVisible) {
    ForceSpeaker(!mOrgSpeakerStatus, mVisible);
  }
}

NS_IMETHODIMP
SpeakerManagerService::Observe(nsISupports* aSubject,
                               const char* aTopic,
                               const char16_t* aData)
{
  if (!strcmp(aTopic, "ipc:content-shutdown")) {
    nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(aSubject);
    if (!props) {
      NS_WARNING("ipc:content-shutdown message without property bag as subject");
      return NS_OK;
    }

    uint64_t childID = 0;
    nsresult rv = props->GetPropertyAsUint64(NS_LITERAL_STRING("childID"),
                                             &childID);
    if (NS_SUCCEEDED(rv)) {
        
        
        if (mSpeakerStatusSet.Contains(childID)) {
          TurnOnSpeaker(false);
          mSpeakerStatusSet.Remove(childID);
        }
        if (mOrgSpeakerStatus) {
          TurnOnSpeaker(!mOrgSpeakerStatus);
          mOrgSpeakerStatus = false;
        }
    } else {
      NS_WARNING("ipc:content-shutdown message without childID property");
    }
  }
  return NS_OK;
}

SpeakerManagerService::SpeakerManagerService()
  : mOrgSpeakerStatus(false),
    mVisible(false)
{
  MOZ_COUNT_CTOR(SpeakerManagerService);
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      obs->AddObserver(this, "ipc:content-shutdown", false);
    }
  }
  AudioChannelService* audioChannelService =
    AudioChannelService::GetOrCreateAudioChannelService();
  if (audioChannelService) {
    audioChannelService->RegisterSpeakerManager(this);
  }
}

SpeakerManagerService::~SpeakerManagerService()
{
  MOZ_COUNT_DTOR(SpeakerManagerService);
  AudioChannelService* audioChannelService =
    AudioChannelService::GetOrCreateAudioChannelService();
  if (audioChannelService)
    audioChannelService->UnregisterSpeakerManager(this);
}
