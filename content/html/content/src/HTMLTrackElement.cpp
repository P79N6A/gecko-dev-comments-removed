




#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/HTMLTrackElement.h"
#include "mozilla/dom/HTMLTrackElementBinding.h"
#include "mozilla/dom/HTMLUnknownElement.h"
#include "nsAttrValueInlines.h"
#include "nsCOMPtr.h"
#include "nsContentPolicyUtils.h"
#include "nsContentUtils.h"
#include "nsCycleCollectionParticipant.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsICachingChannel.h"
#include "nsIChannelEventSink.h"
#include "nsIChannelPolicy.h"
#include "nsIContentPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMHTMLMediaElement.h"
#include "nsIFrame.h"
#include "nsIHttpChannel.h"
#include "nsIInterfaceRequestor.h"
#include "nsILoadGroup.h"
#include "nsIObserver.h"
#include "nsIStreamListener.h"
#include "nsISupportsImpl.h"
#include "nsMappedAttributes.h"
#include "nsNetUtil.h"
#include "nsRuleData.h"
#include "nsStyleConsts.h"
#include "nsThreadUtils.h"
#include "nsVideoFrame.h"
#include "webvtt/parser.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gTrackElementLog;
#define LOG(type, msg) PR_LOG(gTrackElementLog, type, msg)
#else
#define LOG(type, msg)
#endif



nsGenericHTMLElement*
NS_NewHTMLTrackElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                       mozilla::dom::FromParser aFromParser)
{
  if (!mozilla::dom::HTMLTrackElement::IsWebVTTEnabled()) {
    return mozilla::dom::NewHTMLElementHelper::Create<nsHTMLUnknownElement,
           mozilla::dom::HTMLUnknownElement>(aNodeInfo);
  }

  return mozilla::dom::NewHTMLElementHelper::Create<nsHTMLTrackElement,
         mozilla::dom::HTMLTrackElement>(aNodeInfo);
}

namespace mozilla {
namespace dom {


HTMLTrackElement::HTMLTrackElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
  , mReadyState(NONE)
{
#ifdef PR_LOGGING
  if (!gTrackElementLog) {
    gTrackElementLog = PR_NewLogModule("nsTrackElement");
  }
#endif

  SetIsDOMBinding();
}

HTMLTrackElement::~HTMLTrackElement()
{
}

NS_IMPL_ELEMENT_CLONE(HTMLTrackElement)

NS_IMPL_ADDREF_INHERITED(HTMLTrackElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLTrackElement, Element)

NS_IMPL_CYCLE_COLLECTION_INHERITED_3(HTMLTrackElement, nsGenericHTMLElement,
                                     mTrack, mChannel, mMediaParent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(HTMLTrackElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLElement)
NS_INTERFACE_MAP_END_INHERITING(nsGenericHTMLElement)

JSObject*
HTMLTrackElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return HTMLTrackElementBinding::Wrap(aCx, aScope, this);
}

bool
HTMLTrackElement::IsWebVTTEnabled()
{
  return HTMLTrackElementBinding::PrefEnabled();
}

TextTrack*
HTMLTrackElement::Track()
{
  if (!mTrack) {
    
    
    mTrack = new TextTrack(OwnerDoc()->GetParentObject());
  }

  return mTrack;
}

void
HTMLTrackElement::DisplayCueText(webvtt_node* head)
{
  
}

void
HTMLTrackElement::CreateTextTrack()
{
  DOMString label, srcLang;
  GetSrclang(srcLang);
  GetLabel(label);
  mTrack = new TextTrack(OwnerDoc()->GetParentObject(), Kind(), label, srcLang);

  if (mMediaParent) {
    mMediaParent->AddTextTrack(mTrack);
  }
}

TextTrackKind
HTMLTrackElement::Kind() const
{
  const nsAttrValue* value = GetParsedAttr(nsGkAtoms::kind);
  if (!value) {
    return TextTrackKind::Subtitles;
  }
  return static_cast<TextTrackKind>(value->GetEnumValue());
}

static EnumEntry
StringFromKind(TextTrackKind aKind)
{
  return TextTrackKindValues::strings[static_cast<int>(aKind)];
}

void
HTMLTrackElement::SetKind(TextTrackKind aKind, ErrorResult& aError)
{
  const EnumEntry& string = StringFromKind(aKind);
  nsAutoString kind;

  kind.AssignASCII(string.value, string.length);
  SetHTMLAttr(nsGkAtoms::kind, kind, aError);
}

bool
HTMLTrackElement::ParseAttribute(int32_t aNamespaceID,
                                 nsIAtom* aAttribute,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  
  static const nsAttrValue::EnumTable kKindTable[] = {
    { "subtitles", static_cast<int16_t>(TextTrackKind::Subtitles) },
    { "captions", static_cast<int16_t>(TextTrackKind::Captions) },
    { "descriptions", static_cast<int16_t>(TextTrackKind::Descriptions) },
    { "chapters", static_cast<int16_t>(TextTrackKind::Chapters) },
    { "metadata", static_cast<int16_t>(TextTrackKind::Metadata) },
    { 0 }
  };

  if (aNamespaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::kind) {
    
    return aResult.ParseEnumValue(aValue, kKindTable, false, kKindTable);
  }

  
  return nsGenericHTMLElement::ParseAttribute(aNamespaceID,
                                              aAttribute,
                                              aValue,
                                              aResult);
}

nsresult
HTMLTrackElement::BindToTree(nsIDocument* aDocument,
                             nsIContent* aParent,
                             nsIContent* aBindingParent,
                             bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument,
                                                 aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aDocument) {
    return NS_OK;
  }

  LOG(PR_LOG_DEBUG, ("Track Element bound to tree."));
  if (!aParent || !aParent->IsNodeOfType(nsINode::eMEDIA)) {
    return NS_OK;
  }

  
  if (!mMediaParent) {
    mMediaParent = static_cast<HTMLMediaElement*>(aParent);

    HTMLMediaElement* media = static_cast<HTMLMediaElement*>(aParent);
    
    media->NotifyAddedSource();
    LOG(PR_LOG_DEBUG, ("Track element sent notification to parent."));

    
    

    
    nsAutoString src;

    
    
    if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
      nsCOMPtr<nsIURI> uri;
      nsresult rvTwo = NewURIFromString(src, getter_AddRefs(uri));
      if (NS_SUCCEEDED(rvTwo)) {
        LOG(PR_LOG_ALWAYS, ("%p Trying to load from src=%s", this,
        NS_ConvertUTF16toUTF8(src).get()));
        
      }
    }
  }

  return NS_OK;
}

void
HTMLTrackElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  if (mMediaParent && aNullParent) {
    mMediaParent = nullptr;
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

} 
} 
