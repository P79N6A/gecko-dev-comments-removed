





#include "SpeakerManagerServiceChild.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozilla/Util.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "nsIObserverService.h"
#include "nsThreadUtils.h"
#include "AudioChannelService.h"
#include <cutils/properties.h>

using namespace mozilla;
using namespace mozilla::dom;

StaticRefPtr<SpeakerManagerServiceChild> gSpeakerManagerServiceChild;


SpeakerManagerService*
SpeakerManagerServiceChild::GetSpeakerManagerService()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (gSpeakerManagerServiceChild) {
    return gSpeakerManagerServiceChild;
  }

  
  nsRefPtr<SpeakerManagerServiceChild> service = new SpeakerManagerServiceChild();
  NS_ENSURE_TRUE(service, nullptr);

  gSpeakerManagerServiceChild = service;
  return gSpeakerManagerServiceChild;
}

void
SpeakerManagerServiceChild::ForceSpeaker(bool aEnable, bool aVisible)
{
  mVisible = aVisible;
  mOrgSpeakerStatus = aEnable;
  ContentChild *cc = ContentChild::GetSingleton();
  if (cc) {
    cc->SendSpeakerManagerForceSpeaker(aEnable && aVisible);
  }
}

bool
SpeakerManagerServiceChild::GetSpeakerStatus()
{
  ContentChild *cc = ContentChild::GetSingleton();
  bool status = false;
  if (cc) {
    cc->SendSpeakerManagerGetSpeakerStatus(&status);
  }
  char propQemu[PROPERTY_VALUE_MAX];
  property_get("ro.kernel.qemu", propQemu, "");
  if (!strncmp(propQemu, "1", 1)) {
    return mOrgSpeakerStatus;
  }
  return status;
}

void
SpeakerManagerServiceChild::Shutdown()
{
  if (gSpeakerManagerServiceChild) {
    gSpeakerManagerServiceChild = nullptr;
  }
}

void
SpeakerManagerServiceChild::SetAudioChannelActive(bool aIsActive)
{
  
  
  for (uint32_t i = 0; i < mRegisteredSpeakerManagers.Length(); i++) {
    mRegisteredSpeakerManagers[i]->SetAudioChannelActive(aIsActive);
  }
}

SpeakerManagerServiceChild::SpeakerManagerServiceChild()
{
  MOZ_ASSERT(NS_IsMainThread());
  AudioChannelService* audioChannelService = AudioChannelService::GetAudioChannelService();
  if (audioChannelService) {
    audioChannelService->RegisterSpeakerManager(this);
  }
  MOZ_COUNT_CTOR(SpeakerManagerServiceChild);
}

SpeakerManagerServiceChild::~SpeakerManagerServiceChild()
{
  AudioChannelService* audioChannelService = AudioChannelService::GetAudioChannelService();
  if (audioChannelService) {
    audioChannelService->UnregisterSpeakerManager(this);
  }
  MOZ_COUNT_DTOR(SpeakerManagerServiceChild);
}

void
SpeakerManagerServiceChild::Notify()
{
  for (uint32_t i = 0; i < mRegisteredSpeakerManagers.Length(); i++) {
    mRegisteredSpeakerManagers[i]->DispatchSimpleEvent(NS_LITERAL_STRING("speakerforcedchange"));
  }
}
