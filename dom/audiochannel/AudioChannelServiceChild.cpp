





#include "AudioChannelServiceChild.h"

#include "base/basictypes.h"

#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozilla/Util.h"

#include "mozilla/dom/ContentChild.h"

#include "base/basictypes.h"

#include "nsThreadUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

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
    delete gAudioChannelServiceChild;
    gAudioChannelServiceChild = nullptr;
  }
}

AudioChannelServiceChild::AudioChannelServiceChild()
{
}

AudioChannelServiceChild::~AudioChannelServiceChild()
{
}

bool
AudioChannelServiceChild::GetMuted(AudioChannelType aType, bool aMozHidden)
{
  ContentChild *cc = ContentChild::GetSingleton();
  bool muted = false;

  if (cc) {
    cc->SendAudioChannelGetMuted(aType, aMozHidden, &muted);
  }

  return muted;
}

void
AudioChannelServiceChild::RegisterMediaElement(nsHTMLMediaElement* aMediaElement,
                                               AudioChannelType aType)
{
  AudioChannelService::RegisterMediaElement(aMediaElement, aType);

  ContentChild *cc = ContentChild::GetSingleton();
  if (cc) {
    cc->SendAudioChannelRegisterType(aType);
  }
}

void
AudioChannelServiceChild::UnregisterMediaElement(nsHTMLMediaElement* aMediaElement)
{
  AudioChannelType type;
  if (!mMediaElements.Get(aMediaElement, &type)) {
    return;
  }

  AudioChannelService::UnregisterMediaElement(aMediaElement);

  ContentChild *cc = ContentChild::GetSingleton();
  if (cc) {
    cc->SendAudioChannelUnregisterType(type);
  }
}

