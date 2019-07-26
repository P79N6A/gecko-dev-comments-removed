




#include "mozilla/dom/HTMLTrackElement.h"
#include "mozilla/dom/TextTrackCue.h"
#include "mozilla/dom/TextTrackCueBinding.h"
#include "mozilla/dom/ProcessingInstruction.h"
#include "nsIFrame.h"
#include "nsTextNode.h"
#include "nsVideoFrame.h"
#include "webvtt/cue.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_4(TextTrackCue,
                                        mGlobal,
                                        mTrack,
                                        mTrackElement,
                                        mCueDiv)

NS_IMPL_ADDREF_INHERITED(TextTrackCue, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TextTrackCue, nsDOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TextTrackCue)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)



void
TextTrackCue::SetDefaultCueSettings()
{
  mPosition = 50;
  mSize = 100;
  mPauseOnExit = false;
  mSnapToLines = true;
  mLine = WEBVTT_AUTO;
  mAlign = TextTrackCueAlign::Middle;
}

TextTrackCue::TextTrackCue(nsISupports* aGlobal,
                           double aStartTime,
                           double aEndTime,
                           const nsAString& aText)
  : mGlobal(aGlobal)
  , mText(aText)
  , mStartTime(aStartTime)
  , mEndTime(aEndTime)
  , mHead(nullptr)
{
  SetDefaultCueSettings();
  MOZ_ASSERT(aGlobal);
  SetIsDOMBinding();
}

TextTrackCue::TextTrackCue(nsISupports* aGlobal,
                           double aStartTime,
                           double aEndTime,
                           const nsAString& aText,
                           HTMLTrackElement* aTrackElement,
                           webvtt_node* head)
  : mGlobal(aGlobal)
  , mText(aText)
  , mStartTime(aStartTime)
  , mEndTime(aEndTime)
  , mTrackElement(aTrackElement)
  , mHead(head)
{
  
  webvtt_ref_node(mHead);
  SetDefaultCueSettings();
  MOZ_ASSERT(aGlobal);
  SetIsDOMBinding();
}

TextTrackCue::~TextTrackCue()
{
  if (mHead) {
    
    webvtt_release_node(&mHead);
  }
}

void
TextTrackCue::CreateCueOverlay()
{
  mTrackElement->OwnerDoc()->CreateElem(NS_LITERAL_STRING("div"), nullptr,
                                        kNameSpaceID_XHTML,
                                        getter_AddRefs(mCueDiv));
  nsGenericHTMLElement* cueDiv =
    static_cast<nsGenericHTMLElement*>(mCueDiv.get());
  cueDiv->SetClassName(NS_LITERAL_STRING("caption-text"));
}

void
TextTrackCue::RenderCue()
{
  nsRefPtr<DocumentFragment> frag = GetCueAsHTML();
  if (!frag || !mTrackElement) {
    return;
  }

  if (!mCueDiv) {
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
  nsContentUtils::SetNodeTextContent(mCueDiv, EmptyString(), true);

  mCueDiv->AppendChild(*frag, rv);
  overlay->AppendChild(*mCueDiv, rv);
}

already_AddRefed<DocumentFragment>
TextTrackCue::GetCueAsHTML()
{
  nsRefPtr<DocumentFragment> frag =
    mTrackElement->OwnerDoc()->CreateDocumentFragment();

  ConvertNodeTreeToDOMTree(frag);

  return frag.forget();
}

struct WebVTTNodeParentPair
{
  webvtt_node* mNode;
  nsIContent* mParent;

  WebVTTNodeParentPair(webvtt_node* aNode, nsIContent* aParent)
    : mNode(aNode)
    , mParent(aParent)
  {}
};

static void
PushChildren(nsTArray<WebVTTNodeParentPair> &aNodeParentPairStack,
             webvtt_node* aNode, nsIContent* aParentContent)
{
  
  
  for (int i = aNode->data.internal_data->length; i > 0; i--) {
    WebVTTNodeParentPair nodeParentPair(
      aNode->data.internal_data->children[i - 1],
      aParentContent);
    aNodeParentPairStack.AppendElement(nodeParentPair);
  }
}

static WebVTTNodeParentPair
PopChild(nsTArray<WebVTTNodeParentPair> &aNodeParentPairStack) {
  WebVTTNodeParentPair temp =
    aNodeParentPairStack.LastElement();
  aNodeParentPairStack.RemoveElementAt(aNodeParentPairStack.Length() - 1);
  return temp;
}

void
TextTrackCue::ConvertNodeTreeToDOMTree(nsIContent* aParentContent)
{
  nsTArray<WebVTTNodeParentPair> nodeParentPairStack;

  
  if (mHead->kind != WEBVTT_HEAD_NODE) {
    return;
  }
  
  PushChildren(nodeParentPairStack, mHead, aParentContent);

  while (!nodeParentPairStack.IsEmpty()) {
    WebVTTNodeParentPair nodeParentPair = PopChild(nodeParentPairStack);
    nsCOMPtr<nsIContent> content;
    if (WEBVTT_IS_VALID_LEAF_NODE(nodeParentPair.mNode->kind)) {
      content = ConvertLeafNodeToContent(nodeParentPair.mNode);
    } else if (WEBVTT_IS_VALID_INTERNAL_NODE(nodeParentPair.mNode->kind)) {
      content = ConvertInternalNodeToContent(nodeParentPair.mNode);
      
      PushChildren(nodeParentPairStack, nodeParentPair.mNode, content);
    }
    if (content && nodeParentPair.mParent) {
      ErrorResult rv;
      nodeParentPair.mParent->AppendChild(*content, rv);
    }
  }
}

already_AddRefed<nsIContent>
TextTrackCue::ConvertInternalNodeToContent(const webvtt_node* aWebVTTNode)
{
  nsIAtom* atom;

  switch (aWebVTTNode->kind) {
    case WEBVTT_BOLD:
      atom = nsGkAtoms::b;
      break;
    case WEBVTT_ITALIC:
      atom = nsGkAtoms::i;
      break;
    case WEBVTT_UNDERLINE:
      atom = nsGkAtoms::u;
      break;
    case WEBVTT_RUBY:
      atom = nsGkAtoms::ruby;
      break;
    case WEBVTT_RUBY_TEXT:
      atom = nsGkAtoms::rt;
      break;
    case WEBVTT_VOICE:
      atom = nsGkAtoms::span;
      break;
    case WEBVTT_CLASS:
      atom = nsGkAtoms::span;
      break;
    default:
      return nullptr;
      break;
  }

  nsCOMPtr<nsIContent> cueTextContent;
  mTrackElement->OwnerDoc()->CreateElem(nsDependentAtomString(atom), nullptr,
                                        kNameSpaceID_XHTML,
                                        getter_AddRefs(cueTextContent));

  if (aWebVTTNode->kind == WEBVTT_VOICE) {
    const char* text =
      webvtt_string_text(&aWebVTTNode->data.internal_data->annotation);
    if (text) {
      nsGenericHTMLElement* genericHtmlElement =
        static_cast<nsGenericHTMLElement*>(cueTextContent.get());
      genericHtmlElement->SetTitle(NS_ConvertUTF8toUTF16(text));
    }
  }

  webvtt_stringlist* classes = aWebVTTNode->data.internal_data->css_classes;
  if (classes && classes->items && classes->length > 0) {
    nsAutoString classString;

    const char *text = webvtt_string_text(classes->items);
    if (text) {
      AppendUTF8toUTF16(text, classString);
      for (uint32_t i = 1; i < classes->length; i++) {
        text = webvtt_string_text(classes->items + i);
        if (text) {
          classString.Append(' ');
          AppendUTF8toUTF16(text, classString);
        }
      }
    }

    nsGenericHTMLElement* genericHtmlElement =
      static_cast<nsGenericHTMLElement*>(cueTextContent.get());
    genericHtmlElement->SetClassName(classString);
  }
  return cueTextContent.forget();
}

already_AddRefed<nsIContent>
TextTrackCue::ConvertLeafNodeToContent(const webvtt_node* aWebVTTNode)
{
  nsCOMPtr<nsIContent> cueTextContent;
  nsNodeInfoManager* nimgr = mTrackElement->NodeInfo()->NodeInfoManager();
  switch (aWebVTTNode->kind) {
    case WEBVTT_TEXT:
    {
      cueTextContent = new nsTextNode(nimgr);
      const char* text = webvtt_string_text(&aWebVTTNode->data.text);
      if (text) {
        cueTextContent->SetText(NS_ConvertUTF8toUTF16(text), false);
      }
      break;
    }
    case WEBVTT_TIME_STAMP:
    {
      nsAutoString timeStamp;
      timeStamp.AppendInt(aWebVTTNode->data.timestamp);
      cueTextContent =
          NS_NewXMLProcessingInstruction(nimgr, NS_LITERAL_STRING("timestamp"),
                                         timeStamp);
      break;
    }
    default:
      return nullptr;
      break;
  }
  return cueTextContent.forget();
}

JSObject*
TextTrackCue::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return TextTrackCueBinding::Wrap(aCx, aScope, this);
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
