





#include "mozilla/dom/HTMLAudioElement.h"
#include "mozilla/dom/HTMLAudioElementBinding.h"
#include "nsError.h"
#include "nsIDOMHTMLAudioElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "jsfriendapi.h"
#include "nsContentUtils.h"
#include "nsJSUtils.h"
#include "AudioSampleFormat.h"
#include "AudioChannelCommon.h"
#include <algorithm>
#include "mozilla/Preferences.h"
#include "mozilla/dom/EnableWebAudioCheck.h"

static bool
IsAudioAPIEnabled()
{
  return mozilla::Preferences::GetBool("media.audio_data.enabled", true);
}

NS_IMPL_NS_NEW_HTML_ELEMENT(Audio)

namespace mozilla {
namespace dom {

NS_IMPL_ADDREF_INHERITED(HTMLAudioElement, HTMLMediaElement)
NS_IMPL_RELEASE_INHERITED(HTMLAudioElement, HTMLMediaElement)

NS_INTERFACE_TABLE_HEAD(HTMLAudioElement)
NS_HTML_CONTENT_INTERFACE_TABLE4(HTMLAudioElement, nsIDOMHTMLMediaElement,
                                 nsIDOMHTMLAudioElement, nsITimerCallback,
                                 nsIAudioChannelAgentCallback)
NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLAudioElement,
                                             HTMLMediaElement)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE(HTMLAudioElement)


HTMLAudioElement::HTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : HTMLMediaElement(aNodeInfo),
    mTimerActivated(false)
{
  SetIsDOMBinding();
}

HTMLAudioElement::~HTMLAudioElement()
{
}


already_AddRefed<HTMLAudioElement>
HTMLAudioElement::Audio(const GlobalObject& aGlobal,
                        const Optional<nsAString>& aSrc,
                        ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aGlobal.Get());
  nsIDocument* doc;
  if (!win || !(doc = win->GetExtantDoc())) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsCOMPtr<nsINodeInfo> nodeInfo =
    doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::audio, nullptr,
                                        kNameSpaceID_XHTML,
                                        nsIDOMNode::ELEMENT_NODE);

  nsRefPtr<HTMLAudioElement> audio = new HTMLAudioElement(nodeInfo.forget());
  audio->SetHTMLAttr(nsGkAtoms::preload, NS_LITERAL_STRING("auto"), aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  if (aSrc.WasPassed()) {
    aRv = audio->SetSrc(aSrc.Value());
  }

  return audio.forget();
}

void
HTMLAudioElement::MozSetup(uint32_t aChannels, uint32_t aRate, ErrorResult& aRv)
{
  if (!IsAudioAPIEnabled()) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  if (dom::EnableWebAudioCheck::PrefEnabled()) {
    OwnerDoc()->WarnOnceAbout(nsIDocument::eMozAudioData);
  }

  
  if (mDecoder) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  
  if (0 == aChannels) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  if (mAudioStream) {
    mAudioStream->Shutdown();
  }

#ifdef MOZ_B2G
  if (mTimerActivated) {
    mDeferStopPlayTimer->Cancel();
    mTimerActivated = false;
    UpdateAudioChannelPlayingState();
  }
#endif

  mAudioStream = AudioStream::AllocateStream();
  aRv = mAudioStream->Init(aChannels, aRate, mAudioChannelType);
  if (aRv.Failed()) {
    mAudioStream->Shutdown();
    mAudioStream = nullptr;
    return;
  }

  MetadataLoaded(aChannels, aRate, true, false, nullptr);
  mAudioStream->SetVolume(mVolume);
}

uint32_t
HTMLAudioElement::MozWriteAudio(const float* aData, uint32_t aLength,
                                ErrorResult& aRv)
{
  if (!IsAudioAPIEnabled()) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return 0;
  }

  if (!mAudioStream) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return 0;
  }

  
  
  if (aLength % mChannels != 0) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return 0;
  }

#ifdef MOZ_B2G
  if (!mDeferStopPlayTimer) {
    mDeferStopPlayTimer = do_CreateInstance("@mozilla.org/timer;1");
  }

  if (mTimerActivated) {
    mDeferStopPlayTimer->Cancel();
  }
  
  
  mDeferStopPlayTimer->InitWithCallback(this, 1000, nsITimer::TYPE_ONE_SHOT);
  mTimerActivated = true;
  UpdateAudioChannelPlayingState();
#endif

  
  uint32_t writeLen = std::min(mAudioStream->Available(), aLength / mChannels);

  
  
  
  
  nsAutoArrayPtr<AudioDataValue> audioData(new AudioDataValue[writeLen * mChannels]);
  ConvertAudioSamples(aData, audioData.get(), writeLen * mChannels);
  aRv = mAudioStream->Write(audioData.get(), writeLen);
  if (aRv.Failed()) {
    return 0;
  }
  mAudioStream->Start();

  
  return writeLen * mChannels;
}

uint64_t
HTMLAudioElement::MozCurrentSampleOffset(ErrorResult& aRv)
{
  if (!IsAudioAPIEnabled()) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return 0;
  }

  if (!mAudioStream) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return 0;
  }

  int64_t position = mAudioStream->GetPositionInFrames();
  if (position < 0) {
    return 0;
  }

  return position * mChannels;
}

nsresult HTMLAudioElement::SetAcceptHeader(nsIHttpChannel* aChannel)
{
    nsAutoCString value(
#ifdef MOZ_WEBM
      "audio/webm,"
#endif
#ifdef MOZ_OGG
      "audio/ogg,"
#endif
#ifdef MOZ_WAVE
      "audio/wav,"
#endif
      "audio/*;q=0.9,"
#ifdef MOZ_OGG
      "application/ogg;q=0.7,"
#endif
      "video/*;q=0.6,*/*;q=0.5");

    return aChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
                                      value,
                                      false);
}

JSObject*
HTMLAudioElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return HTMLAudioElementBinding::Wrap(aCx, aScope, this);
}


NS_IMETHODIMP
HTMLAudioElement::CanPlayChanged(bool canPlay)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);
  
  
  if (!mAudioStream) {
    return HTMLMediaElement::CanPlayChanged(canPlay);
  }
#ifdef MOZ_B2G
  if (mChannelSuspended == !canPlay) {
    return NS_OK;
  }
  mChannelSuspended = !canPlay;
  SetMutedInternal(mChannelSuspended);
#endif
  return NS_OK;
}

NS_IMETHODIMP
HTMLAudioElement::Notify(nsITimer* aTimer)
{
#ifdef MOZ_B2G
  mTimerActivated = false;
  UpdateAudioChannelPlayingState();
#endif
  return NS_OK;
}

void
HTMLAudioElement::UpdateAudioChannelPlayingState()
{
  if (!mAudioStream) {
    HTMLMediaElement::UpdateAudioChannelPlayingState();
    return;
  }
  
#ifdef MOZ_B2G
  if (mTimerActivated != mPlayingThroughTheAudioChannel) {
    mPlayingThroughTheAudioChannel = mTimerActivated;

    if (!mAudioChannelAgent) {
      nsresult rv;
      mAudioChannelAgent = do_CreateInstance("@mozilla.org/audiochannelagent;1", &rv);
      if (!mAudioChannelAgent) {
        return;
      }
      
      mAudioChannelAgent->InitWithWeakCallback(mAudioChannelType, this);

      nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(OwnerDoc());
      if (domDoc) {
        bool hidden = false;
        domDoc->GetHidden(&hidden);
        mAudioChannelAgent->SetVisibilityState(!hidden);
      }
    }

    if (mPlayingThroughTheAudioChannel) {
      bool canPlay;
      mAudioChannelAgent->StartPlaying(&canPlay);
    } else {
      mAudioChannelAgent->StopPlaying();
      mAudioChannelAgent = nullptr;
    }
  }
#endif
}
} 
} 
