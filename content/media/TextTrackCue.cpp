




#include "mozilla/dom/HTMLTrackElement.h"
#include "mozilla/dom/TextTrackCue.h"
#include "nsIFrame.h"
#include "nsVideoFrame.h"
#include "nsComponentManagerUtils.h"
#include "mozilla/ClearOnShutdown.h"


#define WEBVTT_AUTO -1

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_4(TextTrackCue,
                                     nsDOMEventTargetHelper,
                                     mDocument,
                                     mTrack,
                                     mTrackElement,
                                     mDisplayState)

NS_IMPL_ADDREF_INHERITED(TextTrackCue, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TextTrackCue, nsDOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TextTrackCue)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

StaticRefPtr<nsIWebVTTParserWrapper> TextTrackCue::sParserWrapper;



void
TextTrackCue::SetDefaultCueSettings()
{
  mPosition = 50;
  mSize = 100;
  mPauseOnExit = false;
  mSnapToLines = true;
  mLine = WEBVTT_AUTO;
  mAlign = TextTrackCueAlign::Middle;
  mVertical = DirectionSetting::_empty;
}

TextTrackCue::TextTrackCue(nsISupports* aGlobal,
                           double aStartTime,
                           double aEndTime,
                           const nsAString& aText,
                           ErrorResult& aRv)
  : mText(aText)
  , mStartTime(aStartTime)
  , mEndTime(aEndTime)
  , mReset(false)
{
  SetDefaultCueSettings();
  MOZ_ASSERT(aGlobal);
  SetIsDOMBinding();
  if (NS_FAILED(StashDocument(aGlobal))) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
  }
}

TextTrackCue::TextTrackCue(nsISupports* aGlobal,
                           double aStartTime,
                           double aEndTime,
                           const nsAString& aText,
                           HTMLTrackElement* aTrackElement,
                           ErrorResult& aRv)
  : mText(aText)
  , mStartTime(aStartTime)
  , mEndTime(aEndTime)
  , mTrackElement(aTrackElement)
  , mReset(false)
{
  SetDefaultCueSettings();
  MOZ_ASSERT(aGlobal);
  SetIsDOMBinding();
  if (NS_FAILED(StashDocument(aGlobal))) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
  }
}




nsresult
TextTrackCue::StashDocument(nsISupports* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aGlobal));
  if (!window) {
    return NS_ERROR_NO_INTERFACE;
  }
  mDocument = window->GetDoc();
  if (!mDocument) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  return NS_OK;
}

void
TextTrackCue::CreateCueOverlay()
{
  mDocument->CreateElem(NS_LITERAL_STRING("div"), nullptr,
                        kNameSpaceID_XHTML,
                        getter_AddRefs(mDisplayState));
  nsGenericHTMLElement* cueDiv =
    static_cast<nsGenericHTMLElement*>(mDisplayState.get());
  cueDiv->SetClassName(NS_LITERAL_STRING("caption-text"));
}

void
TextTrackCue::RenderCue()
{
  nsRefPtr<DocumentFragment> frag = GetCueAsHTML();
  if (!frag || !mTrackElement) {
    return;
  }

  if (!mDisplayState) {
    CreateCueOverlay();
  }

  HTMLMediaElement* parent = mTrackElement->mMediaParent;
  if (!parent) {
    return;
  }

  nsIFrame* frame = parent->GetPrimaryFrame();
  if (!frame) {
    return;
  }

  nsVideoFrame* videoFrame = do_QueryFrame(frame);
  if (!videoFrame) {
    return;
  }

  nsIContent* overlay =  videoFrame->GetCaptionOverlay();
  if (!overlay) {
    return;
  }

  ErrorResult rv;
  nsContentUtils::SetNodeTextContent(overlay, EmptyString(), true);
  nsContentUtils::SetNodeTextContent(mDisplayState, EmptyString(), true);

  mDisplayState->AppendChild(*frag, rv);
  overlay->AppendChild(*mDisplayState, rv);
}

already_AddRefed<DocumentFragment>
TextTrackCue::GetCueAsHTML()
{
  MOZ_ASSERT(mDocument);

  if (!sParserWrapper) {
    nsresult rv;
    nsCOMPtr<nsIWebVTTParserWrapper> parserWrapper =
      do_CreateInstance(NS_WEBVTTPARSERWRAPPER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      return mDocument->CreateDocumentFragment();
    }
    sParserWrapper = parserWrapper;
    ClearOnShutdown(&sParserWrapper);
  }

  nsPIDOMWindow* window = mDocument->GetWindow();
  if (!window) {
    return mDocument->CreateDocumentFragment();
  }

  nsCOMPtr<nsIDOMHTMLElement> div;
  sParserWrapper->ConvertCueToDOMTree(window, this,
                                      getter_AddRefs(div));
  if (!div) {
    return mDocument->CreateDocumentFragment();
  }
  nsRefPtr<DocumentFragment> docFrag = mDocument->CreateDocumentFragment();
  nsCOMPtr<nsIDOMNode> throwAway;
  docFrag->AppendChild(div, getter_AddRefs(throwAway));

  return docFrag.forget();
}

void
TextTrackCue::SetTrackElement(HTMLTrackElement* aTrackElement)
{
  mTrackElement = aTrackElement;
}

JSObject*
TextTrackCue::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return VTTCueBinding::Wrap(aCx, aScope, this);
}

void
TextTrackCue::CueChanged()
{
  if (mTrack) {
    mTrack->CueChanged(*this);
  }
}
} 
} 
