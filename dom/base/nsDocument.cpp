









#include "nsDocument.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/BinarySearch.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Likely.h"
#include <algorithm>

#include "prlog.h"
#include "plstr.h"
#include "prprf.h"

#include "mozilla/Telemetry.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsILoadContext.h"
#include "nsUnicharUtils.h"
#include "nsContentList.h"
#include "nsIObserver.h"
#include "nsIBaseWindow.h"
#include "mozilla/css/Loader.h"
#include "mozilla/css/ImageLoader.h"
#include "nsDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsCOMArray.h"
#include "nsQueryObject.h"
#include "nsDOMClassInfo.h"
#include "mozilla/Services.h"

#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventListenerManager.h"
#include "mozilla/EventStateManager.h"
#include "nsIDOMNodeFilter.h"

#include "nsIDOMStyleSheet.h"
#include "mozilla/dom/Attr.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMDocumentXBL.h"
#include "mozilla/dom/Element.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/dom/CDATASection.h"
#include "mozilla/dom/ProcessingInstruction.h"
#include "nsDOMString.h"
#include "nsNodeUtils.h"
#include "nsLayoutUtils.h" 
#include "nsIFrame.h"
#include "nsITabChild.h"

#include "nsRange.h"
#include "nsIDOMText.h"
#include "nsIDOMComment.h"
#include "mozilla/dom/DocumentType.h"
#include "mozilla/dom/NodeIterator.h"
#include "mozilla/dom/TreeWalker.h"

#include "nsIServiceManager.h"
#include "nsIServiceWorkerManager.h"

#include "nsCanvasFrame.h"
#include "nsContentCID.h"
#include "nsError.h"
#include "nsPresShell.h"
#include "nsPresContext.h"
#include "nsIJSON.h"
#include "nsThreadUtils.h"
#include "nsNodeInfoManager.h"
#include "nsIFileChannel.h"
#include "nsIMultiPartChannel.h"
#include "nsIRefreshURI.h"
#include "nsIWebNavigation.h"
#include "nsIScriptError.h"
#include "nsStyleSheetService.h"

#include "nsNetUtil.h"     

#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"

#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsFocusManager.h"


#include "nsIDOMHTMLInputElement.h"
#include "nsIRadioVisitor.h"
#include "nsIFormControl.h"

#include "nsBidiUtils.h"

#include "nsIParserService.h"
#include "nsContentCreatorFunctions.h"

#include "nsIScriptContext.h"
#include "nsBindingManager.h"
#include "nsIDOMHTMLDocument.h"
#include "nsHTMLDocument.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIRequest.h"
#include "nsHostObjectProtocolHandler.h"

#include "nsCharsetSource.h"
#include "nsIParser.h"
#include "nsIContentSink.h"

#include "nsDateTimeFormatCID.h"
#include "nsIDateTimeFormat.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStates.h"
#include "mozilla/InternalMutationEvent.h"
#include "nsDOMCID.h"

#include "jsapi.h"
#include "nsIXPConnect.h"
#include "nsCCUncollectableMarker.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsICategoryManager.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIDocumentLoader.h"
#include "nsIContentViewer.h"
#include "nsIXMLContentSink.h"
#include "nsIXULDocument.h"
#include "nsIPrompt.h"
#include "nsIPropertyBag2.h"
#include "mozilla/dom/PageTransitionEvent.h"
#include "mozilla/dom/StyleRuleChangeEvent.h"
#include "mozilla/dom/StyleSheetChangeEvent.h"
#include "mozilla/dom/StyleSheetApplicableStateChangeEvent.h"
#include "nsJSUtils.h"
#include "nsFrameLoader.h"
#include "nsEscape.h"
#include "nsObjectLoadingContent.h"
#include "nsHtml5TreeOpExecutor.h"
#include "mozilla/dom/HTMLLinkElement.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/HTMLImageElement.h"
#include "mozilla/dom/MediaSource.h"

#include "mozAutoDocUpdate.h"
#include "nsGlobalWindow.h"
#include "mozilla/dom/EncodingUtils.h"
#include "nsDOMNavigationTiming.h"

#include "nsSMILAnimationController.h"
#include "imgIContainer.h"
#include "nsSVGUtils.h"
#include "SVGElementFactory.h"

#include "nsRefreshDriver.h"


#include "nsIContentSecurityPolicy.h"
#include "mozilla/dom/nsCSPService.h"
#include "nsHTMLStyleSheet.h"
#include "nsHTMLCSSStyleSheet.h"
#include "SVGAttrAnimationRuleProcessor.h"
#include "mozilla/dom/DOMImplementation.h"
#include "mozilla/dom/ShadowRoot.h"
#include "mozilla/dom/Comment.h"
#include "nsTextNode.h"
#include "mozilla/dom/Link.h"
#include "mozilla/dom/HTMLElementBinding.h"
#include "mozilla/dom/SVGElementBinding.h"
#include "nsXULAppAPI.h"
#include "mozilla/dom/Touch.h"
#include "mozilla/dom/TouchEvent.h"

#include "mozilla/Preferences.h"

#include "imgILoader.h"
#include "imgRequestProxy.h"
#include "nsWrapperCacheInlines.h"
#include "nsSandboxFlags.h"
#include "nsIAppsService.h"
#include "mozilla/dom/AnonymousContent.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DocumentFragment.h"
#include "mozilla/dom/DocumentTimeline.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/HTMLBodyElement.h"
#include "mozilla/dom/HTMLInputElement.h"
#include "mozilla/dom/MediaQueryList.h"
#include "mozilla/dom/NodeFilterBinding.h"
#include "mozilla/dom/OwningNonNull.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/dom/UndoManager.h"
#include "mozilla/dom/WebComponentsBinding.h"
#include "nsFrame.h"
#include "nsDOMCaretPosition.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsViewportInfo.h"
#include "nsIContentPermissionPrompt.h"
#include "mozilla/StaticPtr.h"
#include "nsITextControlElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIEditor.h"
#include "nsIDOMCSSStyleRule.h"
#include "mozilla/css/Rule.h"
#include "nsIDOMLocation.h"
#include "nsIHttpChannelInternal.h"
#include "nsISecurityConsoleMessage.h"
#include "nsCharSeparatedTokenizer.h"
#include "mozilla/dom/XPathEvaluator.h"
#include "mozilla/dom/XPathNSResolverBinding.h"
#include "mozilla/dom/XPathResult.h"
#include "nsIDocumentEncoder.h"
#include "nsIDocumentActivity.h"
#include "nsIStructuredCloneContainer.h"
#include "nsIMutableArray.h"
#include "nsContentPermissionHelper.h"
#include "mozilla/dom/DOMStringList.h"
#include "nsWindowMemoryReporter.h"
#include "nsLocation.h"
#include "mozilla/dom/FontFaceSet.h"
#include "mozilla/dom/BoxObject.h"
#include "gfxVR.h"

#ifdef MOZ_MEDIA_NAVIGATOR
#include "mozilla/MediaManager.h"
#endif 
#ifdef MOZ_WEBRTC
#include "IPeerConnection.h"
#endif 

using namespace mozilla;
using namespace mozilla::dom;

typedef nsTArray<Link*> LinkArray;

#ifdef PR_LOGGING
static PRLogModuleInfo* gDocumentLeakPRLog;
static PRLogModuleInfo* gCspPRLog;
#endif

#define NAME_NOT_VALID ((nsSimpleContentList*)1)

nsIdentifierMapEntry::~nsIdentifierMapEntry()
{
}

void
nsIdentifierMapEntry::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback,
                                     "mIdentifierMap mNameContentList");
  aCallback->NoteXPCOMChild(static_cast<nsIDOMNodeList*>(mNameContentList));

  if (mImageElement) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback,
                                       "mIdentifierMap mImageElement element");
    nsIContent* imageElement = mImageElement;
    aCallback->NoteXPCOMChild(imageElement);
  }
}

bool
nsIdentifierMapEntry::IsEmpty()
{
  return mIdContentList.Count() == 0 && !mNameContentList &&
         !mChangeCallbacks && !mImageElement;
}

Element*
nsIdentifierMapEntry::GetIdElement()
{
  return static_cast<Element*>(mIdContentList.SafeElementAt(0));
}

Element*
nsIdentifierMapEntry::GetImageIdElement()
{
  return mImageElement ? mImageElement.get() : GetIdElement();
}

void
nsIdentifierMapEntry::AppendAllIdContent(nsCOMArray<nsIContent>* aElements)
{
  for (int32_t i = 0; i < mIdContentList.Count(); ++i) {
    aElements->AppendObject(static_cast<Element*>(mIdContentList[i]));
  }
}

void
nsIdentifierMapEntry::AddContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                               void* aData, bool aForImage)
{
  if (!mChangeCallbacks) {
    mChangeCallbacks = new nsTHashtable<ChangeCallbackEntry>;
    if (!mChangeCallbacks)
      return;
  }

  ChangeCallback cc = { aCallback, aData, aForImage };
  mChangeCallbacks->PutEntry(cc);
}

void
nsIdentifierMapEntry::RemoveContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                                  void* aData, bool aForImage)
{
  if (!mChangeCallbacks)
    return;
  ChangeCallback cc = { aCallback, aData, aForImage };
  mChangeCallbacks->RemoveEntry(cc);
  if (mChangeCallbacks->Count() == 0) {
    mChangeCallbacks = nullptr;
  }
}

struct FireChangeArgs {
  Element* mFrom;
  Element* mTo;
  bool mImageOnly;
  bool mHaveImageOverride;
};


template<>
struct nsIStyleRule::COMTypeInfo<css::Rule, void> {
  static const nsIID kIID;
};
const nsIID nsIStyleRule::COMTypeInfo<css::Rule, void>::kIID = NS_ISTYLE_RULE_IID;

namespace mozilla {
namespace dom {

static PLDHashOperator
CustomDefinitionsTraverse(CustomElementHashKey* aKey,
                          CustomElementDefinition* aDefinition,
                          void* aArg)
{
  nsCycleCollectionTraversalCallback* cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aArg);

  nsAutoPtr<LifecycleCallbacks>& callbacks = aDefinition->mCallbacks;

  if (callbacks->mAttributeChangedCallback.WasPassed()) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
      "mCustomDefinitions->mCallbacks->mAttributeChangedCallback");
    cb->NoteXPCOMChild(aDefinition->mCallbacks->mAttributeChangedCallback.Value());
  }

  if (callbacks->mCreatedCallback.WasPassed()) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
      "mCustomDefinitions->mCallbacks->mCreatedCallback");
    cb->NoteXPCOMChild(aDefinition->mCallbacks->mCreatedCallback.Value());
  }

  if (callbacks->mAttachedCallback.WasPassed()) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
      "mCustomDefinitions->mCallbacks->mAttachedCallback");
    cb->NoteXPCOMChild(aDefinition->mCallbacks->mAttachedCallback.Value());
  }

  if (callbacks->mDetachedCallback.WasPassed()) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
      "mCustomDefinitions->mCallbacks->mDetachedCallback");
    cb->NoteXPCOMChild(aDefinition->mCallbacks->mDetachedCallback.Value());
  }

  return PL_DHASH_NEXT;
}

static PLDHashOperator
CandidatesTraverse(CustomElementHashKey* aKey,
                   nsTArray<nsRefPtr<Element>>* aData,
                   void* aArg)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aArg);
  for (size_t i = 0; i < aData->Length(); ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mCandidatesMap->Element");
    cb->NoteXPCOMChild(aData->ElementAt(i));
  }
  return PL_DHASH_NEXT;
}

struct CustomDefinitionTraceArgs
{
  const TraceCallbacks& callbacks;
  void* closure;
};

static PLDHashOperator
CustomDefinitionTrace(CustomElementHashKey *aKey,
                      CustomElementDefinition *aData,
                      void *aArg)
{
  CustomDefinitionTraceArgs* traceArgs = static_cast<CustomDefinitionTraceArgs*>(aArg);
  MOZ_ASSERT(aData, "Definition must not be null");
  traceArgs->callbacks.Trace(&aData->mPrototype, "mCustomDefinitions prototype",
                             traceArgs->closure);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(Registry)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(Registry)
  CustomDefinitionTraceArgs customDefinitionArgs = { aCallbacks, aClosure };
  tmp->mCustomDefinitions.EnumerateRead(CustomDefinitionTrace,
                                        &customDefinitionArgs);
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Registry)
  tmp->mCustomDefinitions.EnumerateRead(CustomDefinitionsTraverse, &cb);
  tmp->mCandidatesMap.EnumerateRead(CandidatesTraverse, &cb);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(Registry)
  tmp->mCustomDefinitions.Clear();
  tmp->mCandidatesMap.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Registry)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Registry)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Registry)

Registry::Registry()
{
  mozilla::HoldJSObjects(this);
}

Registry::~Registry()
{
  mozilla::DropJSObjects(this);
}

void
CustomElementCallback::Call()
{
  ErrorResult rv;
  switch (mType) {
    case nsIDocument::eCreated:
    {
      
      
      mOwnerData->mElementIsBeingCreated = true;

      
      
      
      mOwnerData->mCreatedCallbackInvoked = true;

      
      
      nsIDocument* document = mThisObject->GetComposedDoc();
      if (document && document->GetDocShell()) {
        document->EnqueueLifecycleCallback(nsIDocument::eAttached, mThisObject);
      }

      static_cast<LifecycleCreatedCallback *>(mCallback.get())->Call(mThisObject, rv);
      mOwnerData->mElementIsBeingCreated = false;
      break;
    }
    case nsIDocument::eAttached:
      static_cast<LifecycleAttachedCallback *>(mCallback.get())->Call(mThisObject, rv);
      break;
    case nsIDocument::eDetached:
      static_cast<LifecycleDetachedCallback *>(mCallback.get())->Call(mThisObject, rv);
      break;
    case nsIDocument::eAttributeChanged:
      static_cast<LifecycleAttributeChangedCallback *>(mCallback.get())->Call(mThisObject,
        mArgs.name, mArgs.oldValue, mArgs.newValue, rv);
      break;
  }
}

void
CustomElementCallback::Traverse(nsCycleCollectionTraversalCallback& aCb) const
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(aCb, "mThisObject");
  aCb.NoteXPCOMChild(mThisObject);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(aCb, "mCallback");
  aCb.NoteXPCOMChild(mCallback);
}

CustomElementCallback::CustomElementCallback(Element* aThisObject,
                                             nsIDocument::ElementCallbackType aCallbackType,
                                             mozilla::dom::CallbackFunction* aCallback,
                                             CustomElementData* aOwnerData)
  : mThisObject(aThisObject),
    mCallback(aCallback),
    mType(aCallbackType),
    mOwnerData(aOwnerData)
{
}

CustomElementDefinition::CustomElementDefinition(JSObject* aPrototype,
                                                 nsIAtom* aType,
                                                 nsIAtom* aLocalName,
                                                 LifecycleCallbacks* aCallbacks,
                                                 uint32_t aNamespaceID,
                                                 uint32_t aDocOrder)
  : mPrototype(aPrototype),
    mType(aType),
    mLocalName(aLocalName),
    mCallbacks(aCallbacks),
    mNamespaceID(aNamespaceID),
    mDocOrder(aDocOrder)
{
}

CustomElementData::CustomElementData(nsIAtom* aType)
  : mType(aType),
    mCurrentCallback(-1),
    mElementIsBeingCreated(false),
    mCreatedCallbackInvoked(true),
    mAssociatedMicroTask(-1)
{
}

void
CustomElementData::RunCallbackQueue()
{
  
  while (static_cast<uint32_t>(++mCurrentCallback) < mCallbackQueue.Length()) {
    mCallbackQueue[mCurrentCallback]->Call();
  }

  mCallbackQueue.Clear();
  mCurrentCallback = -1;
}

} 
} 

static PLDHashOperator
FireChangeEnumerator(nsIdentifierMapEntry::ChangeCallbackEntry *aEntry, void *aArg)
{
  FireChangeArgs* args = static_cast<FireChangeArgs*>(aArg);
  
  
  if (aEntry->mKey.mForImage ? (args->mHaveImageOverride && !args->mImageOnly) :
                               args->mImageOnly)
    return PL_DHASH_NEXT;
  return aEntry->mKey.mCallback(args->mFrom, args->mTo, aEntry->mKey.mData)
      ? PL_DHASH_NEXT : PL_DHASH_REMOVE;
}

void
nsIdentifierMapEntry::FireChangeCallbacks(Element* aOldElement,
                                          Element* aNewElement,
                                          bool aImageOnly)
{
  if (!mChangeCallbacks)
    return;

  FireChangeArgs args = { aOldElement, aNewElement, aImageOnly, !!mImageElement };
  mChangeCallbacks->EnumerateEntries(FireChangeEnumerator, &args);
}

namespace {

struct PositionComparator
{
  Element* const mElement;
  explicit PositionComparator(Element* const aElement) : mElement(aElement) {}

  int operator()(void* aElement) const {
    Element* curElement = static_cast<Element*>(aElement);
    if (mElement == curElement) {
      return 0;
    }
    if (nsContentUtils::PositionIsBefore(mElement, curElement)) {
      return -1;
    }
    return 1;
  }
};
} 

bool
nsIdentifierMapEntry::AddIdElement(Element* aElement)
{
  NS_PRECONDITION(aElement, "Must have element");
  NS_PRECONDITION(mIdContentList.IndexOf(nullptr) < 0,
                  "Why is null in our list?");

#ifdef DEBUG
  Element* currentElement =
    static_cast<Element*>(mIdContentList.SafeElementAt(0));
#endif

  
  if (mIdContentList.Count() == 0) {
    if (!mIdContentList.AppendElement(aElement))
      return false;
    NS_ASSERTION(currentElement == nullptr, "How did that happen?");
    FireChangeCallbacks(nullptr, aElement);
    return true;
  }

  
  

  size_t idx;
  if (BinarySearchIf(mIdContentList, 0, mIdContentList.Count(),
                     PositionComparator(aElement), &idx)) {
    
    
    
    return true;
  }

  if (!mIdContentList.InsertElementAt(aElement, idx))
    return false;

  if (idx == 0) {
    Element* oldElement =
      static_cast<Element*>(mIdContentList.SafeElementAt(1));
    NS_ASSERTION(currentElement == oldElement, "How did that happen?");
    FireChangeCallbacks(oldElement, aElement);
  }
  return true;
}

void
nsIdentifierMapEntry::RemoveIdElement(Element* aElement)
{
  NS_PRECONDITION(aElement, "Missing element");

  
  

  
  
  
  NS_ASSERTION(!aElement->OwnerDoc()->IsHTMLDocument() ||
               mIdContentList.IndexOf(aElement) >= 0,
               "Removing id entry that doesn't exist");

  
  
  Element* currentElement =
    static_cast<Element*>(mIdContentList.SafeElementAt(0));
  mIdContentList.RemoveElement(aElement);
  if (currentElement == aElement) {
    FireChangeCallbacks(currentElement,
                        static_cast<Element*>(mIdContentList.SafeElementAt(0)));
  }
}

void
nsIdentifierMapEntry::SetImageElement(Element* aElement)
{
  Element* oldElement = GetImageIdElement();
  mImageElement = aElement;
  Element* newElement = GetImageIdElement();
  if (oldElement != newElement) {
    FireChangeCallbacks(oldElement, newElement, true);
  }
}

void
nsIdentifierMapEntry::AddNameElement(nsINode* aNode, Element* aElement)
{
  if (!mNameContentList) {
    mNameContentList = new nsSimpleContentList(aNode);
  }

  mNameContentList->AppendElement(aElement);
}

void
nsIdentifierMapEntry::RemoveNameElement(Element* aElement)
{
  if (mNameContentList) {
    mNameContentList->RemoveElement(aElement);
  }
}

bool
nsIdentifierMapEntry::HasIdElementExposedAsHTMLDocumentProperty()
{
  Element* idElement = GetIdElement();
  return idElement &&
         nsGenericHTMLElement::ShouldExposeIdAsHTMLDocumentProperty(idElement);
}

size_t
nsIdentifierMapEntry::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  return nsStringHashKey::SizeOfExcludingThis(aMallocSizeOf);
}



class SubDocMapEntry : public PLDHashEntryHdr
{
public:
  
  Element *mKey; 
  nsIDocument *mSubDocument;
};

struct FindContentData
{
  explicit FindContentData(nsIDocument* aSubDoc)
    : mSubDocument(aSubDoc), mResult(nullptr)
  {
  }

  nsISupports *mSubDocument;
  Element *mResult;
};





struct nsRadioGroupStruct
{
  nsRadioGroupStruct()
    : mRequiredRadioCount(0)
    , mGroupSuffersFromValueMissing(false)
  {}

  


  nsRefPtr<HTMLInputElement> mSelectedRadioButton;
  nsCOMArray<nsIFormControl> mRadioButtons;
  uint32_t mRequiredRadioCount;
  bool mGroupSuffersFromValueMissing;
};


nsDOMStyleSheetList::nsDOMStyleSheetList(nsIDocument *aDocument)
{
  mLength = -1;
  
  
  mDocument = aDocument;
  mDocument->AddObserver(this);
}

nsDOMStyleSheetList::~nsDOMStyleSheetList()
{
  if (mDocument) {
    mDocument->RemoveObserver(this);
  }
}

NS_IMPL_ISUPPORTS_INHERITED(nsDOMStyleSheetList, StyleSheetList,
                            nsIDocumentObserver,
                            nsIMutationObserver)

uint32_t
nsDOMStyleSheetList::Length()
{
  if (!mDocument) {
    return 0;
  }

  
  
  
  if (-1 == mLength) {
    mLength = mDocument->GetNumberOfStyleSheets();

#ifdef DEBUG
    int32_t i;
    for (i = 0; i < mLength; i++) {
      nsIStyleSheet *sheet = mDocument->GetStyleSheetAt(i);
      nsCOMPtr<nsIDOMStyleSheet> domss(do_QueryInterface(sheet));
      NS_ASSERTION(domss, "All \"normal\" sheets implement nsIDOMStyleSheet");
    }
#endif
  }
  return mLength;
}

CSSStyleSheet*
nsDOMStyleSheetList::IndexedGetter(uint32_t aIndex, bool& aFound)
{
  if (!mDocument || aIndex >= (uint32_t)mDocument->GetNumberOfStyleSheets()) {
    aFound = false;
    return nullptr;
  }

  aFound = true;
  nsIStyleSheet *sheet = mDocument->GetStyleSheetAt(aIndex);
  NS_ASSERTION(sheet, "Must have a sheet");

  return static_cast<CSSStyleSheet*>(sheet);
}

void
nsDOMStyleSheetList::NodeWillBeDestroyed(const nsINode *aNode)
{
  mDocument = nullptr;
}

void
nsDOMStyleSheetList::StyleSheetAdded(nsIDocument *aDocument,
                                     nsIStyleSheet* aStyleSheet,
                                     bool aDocumentSheet)
{
  if (aDocumentSheet && -1 != mLength) {
    nsCOMPtr<nsIDOMStyleSheet> domss(do_QueryInterface(aStyleSheet));
    if (domss) {
      mLength++;
    }
  }
}

void
nsDOMStyleSheetList::StyleSheetRemoved(nsIDocument *aDocument,
                                       nsIStyleSheet* aStyleSheet,
                                       bool aDocumentSheet)
{
  if (aDocumentSheet && -1 != mLength) {
    nsCOMPtr<nsIDOMStyleSheet> domss(do_QueryInterface(aStyleSheet));
    if (domss) {
      mLength--;
    }
  }
}


NS_IMPL_ISUPPORTS(nsOnloadBlocker, nsIRequest)

NS_IMETHODIMP
nsOnloadBlocker::GetName(nsACString &aResult)
{
  aResult.AssignLiteral("about:document-onload-blocker");
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::IsPending(bool *_retval)
{
  *_retval = true;
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::GetStatus(nsresult *status)
{
  *status = NS_OK;
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::Cancel(nsresult status)
{
  return NS_OK;
}
NS_IMETHODIMP
nsOnloadBlocker::Suspend(void)
{
  return NS_OK;
}
NS_IMETHODIMP
nsOnloadBlocker::Resume(void)
{
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
  *aLoadGroup = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  *aLoadFlags = nsIRequest::LOAD_NORMAL;
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  return NS_OK;
}



nsExternalResourceMap::nsExternalResourceMap()
  : mHaveShutDown(false)
{
}

nsIDocument*
nsExternalResourceMap::RequestResource(nsIURI* aURI,
                                       nsINode* aRequestingNode,
                                       nsDocument* aDisplayDocument,
                                       ExternalResourceLoad** aPendingLoad)
{
  
  
  
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aRequestingNode, "Must have a node");
  *aPendingLoad = nullptr;
  if (mHaveShutDown) {
    return nullptr;
  }

  
  nsCOMPtr<nsIURI> clone;
  nsresult rv = aURI->CloneIgnoringRef(getter_AddRefs(clone));
  if (NS_FAILED(rv) || !clone) {
    return nullptr;
  }

  ExternalResource* resource;
  mMap.Get(clone, &resource);
  if (resource) {
    return resource->mDocument;
  }

  nsRefPtr<PendingLoad> load;
  mPendingLoads.Get(clone, getter_AddRefs(load));
  if (load) {
    load.forget(aPendingLoad);
    return nullptr;
  }

  load = new PendingLoad(aDisplayDocument);

  mPendingLoads.Put(clone, load);

  if (NS_FAILED(load->StartLoad(clone, aRequestingNode))) {
    
    
    AddExternalResource(clone, nullptr, nullptr, aDisplayDocument);
  } else {
    load.forget(aPendingLoad);
  }

  return nullptr;
}

struct
nsExternalResourceEnumArgs
{
  nsIDocument::nsSubDocEnumFunc callback;
  void *data;
};

static PLDHashOperator
ExternalResourceEnumerator(nsIURI* aKey,
                           nsExternalResourceMap::ExternalResource* aData,
                           void* aClosure)
{
  nsExternalResourceEnumArgs* args =
    static_cast<nsExternalResourceEnumArgs*>(aClosure);
  bool next =
    aData->mDocument ? args->callback(aData->mDocument, args->data) : true;
  return next ? PL_DHASH_NEXT : PL_DHASH_STOP;
}

void
nsExternalResourceMap::EnumerateResources(nsIDocument::nsSubDocEnumFunc aCallback,
                                          void* aData)
{
  nsExternalResourceEnumArgs args = { aCallback, aData };
  mMap.EnumerateRead(ExternalResourceEnumerator, &args);
}

static PLDHashOperator
ExternalResourceTraverser(nsIURI* aKey,
                          nsExternalResourceMap::ExternalResource* aData,
                          void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                     "mExternalResourceMap.mMap entry"
                                     "->mDocument");
  cb->NoteXPCOMChild(aData->mDocument);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                     "mExternalResourceMap.mMap entry"
                                     "->mViewer");
  cb->NoteXPCOMChild(aData->mViewer);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                     "mExternalResourceMap.mMap entry"
                                     "->mLoadGroup");
  cb->NoteXPCOMChild(aData->mLoadGroup);

  return PL_DHASH_NEXT;
}

void
nsExternalResourceMap::Traverse(nsCycleCollectionTraversalCallback* aCallback) const
{
  
  
  mMap.EnumerateRead(ExternalResourceTraverser, aCallback);
}

static PLDHashOperator
ExternalResourceHider(nsIURI* aKey,
                      nsExternalResourceMap::ExternalResource* aData,
                      void* aClosure)
{
  if (aData->mViewer) {
    aData->mViewer->Hide();
  }
  return PL_DHASH_NEXT;
}

void
nsExternalResourceMap::HideViewers()
{
  mMap.EnumerateRead(ExternalResourceHider, nullptr);
}

static PLDHashOperator
ExternalResourceShower(nsIURI* aKey,
                       nsExternalResourceMap::ExternalResource* aData,
                       void* aClosure)
{
  if (aData->mViewer) {
    aData->mViewer->Show();
  }
  return PL_DHASH_NEXT;
}

void
nsExternalResourceMap::ShowViewers()
{
  mMap.EnumerateRead(ExternalResourceShower, nullptr);
}

void
TransferZoomLevels(nsIDocument* aFromDoc,
                   nsIDocument* aToDoc)
{
  MOZ_ASSERT(aFromDoc && aToDoc,
             "transferring zoom levels from/to null doc");

  nsIPresShell* fromShell = aFromDoc->GetShell();
  if (!fromShell)
    return;

  nsPresContext* fromCtxt = fromShell->GetPresContext();
  if (!fromCtxt)
    return;

  nsIPresShell* toShell = aToDoc->GetShell();
  if (!toShell)
    return;

  nsPresContext* toCtxt = toShell->GetPresContext();
  if (!toCtxt)
    return;

  toCtxt->SetFullZoom(fromCtxt->GetFullZoom());
  toCtxt->SetBaseMinFontSize(fromCtxt->BaseMinFontSize());
  toCtxt->SetTextZoom(fromCtxt->TextZoom());
}

void
TransferShowingState(nsIDocument* aFromDoc, nsIDocument* aToDoc)
{
  MOZ_ASSERT(aFromDoc && aToDoc,
             "transferring showing state from/to null doc");

  if (aFromDoc->IsShowing()) {
    aToDoc->OnPageShow(true, nullptr);
  }
}

nsresult
nsExternalResourceMap::AddExternalResource(nsIURI* aURI,
                                           nsIContentViewer* aViewer,
                                           nsILoadGroup* aLoadGroup,
                                           nsIDocument* aDisplayDocument)
{
  NS_PRECONDITION(aURI, "Unexpected call");
  NS_PRECONDITION((aViewer && aLoadGroup) || (!aViewer && !aLoadGroup),
                  "Must have both or neither");

  nsRefPtr<PendingLoad> load;
  mPendingLoads.Get(aURI, getter_AddRefs(load));
  mPendingLoads.Remove(aURI);

  nsresult rv = NS_OK;

  nsCOMPtr<nsIDocument> doc;
  if (aViewer) {
    doc = aViewer->GetDocument();
    NS_ASSERTION(doc, "Must have a document");

    nsCOMPtr<nsIXULDocument> xulDoc = do_QueryInterface(doc);
    if (xulDoc) {
      
      rv = NS_ERROR_NOT_AVAILABLE;
    } else {
      doc->SetDisplayDocument(aDisplayDocument);

      
      aViewer->SetSticky(false);

      rv = aViewer->Init(nullptr, nsIntRect(0, 0, 0, 0));
      if (NS_SUCCEEDED(rv)) {
        rv = aViewer->Open(nullptr, nullptr);
      }
    }

    if (NS_FAILED(rv)) {
      doc = nullptr;
      aViewer = nullptr;
      aLoadGroup = nullptr;
    }
  }

  ExternalResource* newResource = new ExternalResource();
  mMap.Put(aURI, newResource);

  newResource->mDocument = doc;
  newResource->mViewer = aViewer;
  newResource->mLoadGroup = aLoadGroup;
  if (doc) {
    TransferZoomLevels(aDisplayDocument, doc);
    TransferShowingState(aDisplayDocument, doc);
  }

  const nsTArray< nsCOMPtr<nsIObserver> > & obs = load->Observers();
  for (uint32_t i = 0; i < obs.Length(); ++i) {
    obs[i]->Observe(doc, "external-resource-document-created", nullptr);
  }

  return rv;
}

NS_IMPL_ISUPPORTS(nsExternalResourceMap::PendingLoad,
                  nsIStreamListener,
                  nsIRequestObserver)

NS_IMETHODIMP
nsExternalResourceMap::PendingLoad::OnStartRequest(nsIRequest *aRequest,
                                                   nsISupports *aContext)
{
  nsExternalResourceMap& map = mDisplayDocument->ExternalResourceMap();
  if (map.HaveShutDown()) {
    return NS_BINDING_ABORTED;
  }

  nsCOMPtr<nsIContentViewer> viewer;
  nsCOMPtr<nsILoadGroup> loadGroup;
  nsresult rv = SetupViewer(aRequest, getter_AddRefs(viewer),
                            getter_AddRefs(loadGroup));

  
  nsresult rv2 = map.AddExternalResource(mURI, viewer, loadGroup,
                                         mDisplayDocument);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (NS_FAILED(rv2)) {
    mTargetListener = nullptr;
    return rv2;
  }

  return mTargetListener->OnStartRequest(aRequest, aContext);
}

nsresult
nsExternalResourceMap::PendingLoad::SetupViewer(nsIRequest* aRequest,
                                                nsIContentViewer** aViewer,
                                                nsILoadGroup** aLoadGroup)
{
  NS_PRECONDITION(!mTargetListener, "Unexpected call to OnStartRequest");
  *aViewer = nullptr;
  *aLoadGroup = nullptr;

  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  NS_ENSURE_TRUE(chan, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
  if (httpChannel) {
    bool requestSucceeded;
    if (NS_FAILED(httpChannel->GetRequestSucceeded(&requestSucceeded)) ||
        !requestSucceeded) {
      
      return NS_BINDING_ABORTED;
    }
  }

  nsAutoCString type;
  chan->GetContentType(type);

  nsCOMPtr<nsILoadGroup> loadGroup;
  chan->GetLoadGroup(getter_AddRefs(loadGroup));

  
  nsCOMPtr<nsILoadGroup> newLoadGroup =
        do_CreateInstance(NS_LOADGROUP_CONTRACTID);
  NS_ENSURE_TRUE(newLoadGroup, NS_ERROR_OUT_OF_MEMORY);
  newLoadGroup->SetLoadGroup(loadGroup);

  nsCOMPtr<nsIInterfaceRequestor> callbacks;
  loadGroup->GetNotificationCallbacks(getter_AddRefs(callbacks));

  nsCOMPtr<nsIInterfaceRequestor> newCallbacks =
    new LoadgroupCallbacks(callbacks);
  newLoadGroup->SetNotificationCallbacks(newCallbacks);

  
  nsCOMPtr<nsICategoryManager> catMan =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(catMan, NS_ERROR_NOT_AVAILABLE);
  nsXPIDLCString contractId;
  nsresult rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", type.get(),
                                         getter_Copies(contractId));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory =
    do_GetService(contractId);
  NS_ENSURE_TRUE(docLoaderFactory, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIContentViewer> viewer;
  nsCOMPtr<nsIStreamListener> listener;
  rv = docLoaderFactory->CreateInstance("external-resource", chan, newLoadGroup,
                                        type, nullptr, nullptr,
                                        getter_AddRefs(listener),
                                        getter_AddRefs(viewer));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(viewer, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIParser> parser = do_QueryInterface(listener);
  if (!parser) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  nsIContentSink* sink = parser->GetContentSink();
  nsCOMPtr<nsIXMLContentSink> xmlSink = do_QueryInterface(sink);
  if (!xmlSink) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  listener.swap(mTargetListener);
  viewer.forget(aViewer);
  newLoadGroup.forget(aLoadGroup);
  return NS_OK;
}

NS_IMETHODIMP
nsExternalResourceMap::PendingLoad::OnDataAvailable(nsIRequest* aRequest,
                                                    nsISupports* aContext,
                                                    nsIInputStream* aStream,
                                                    uint64_t aOffset,
                                                    uint32_t aCount)
{
  NS_PRECONDITION(mTargetListener, "Shouldn't be getting called!");
  if (mDisplayDocument->ExternalResourceMap().HaveShutDown()) {
    return NS_BINDING_ABORTED;
  }
  return mTargetListener->OnDataAvailable(aRequest, aContext, aStream, aOffset,
                                          aCount);
}

NS_IMETHODIMP
nsExternalResourceMap::PendingLoad::OnStopRequest(nsIRequest* aRequest,
                                                  nsISupports* aContext,
                                                  nsresult aStatus)
{
  
  if (mTargetListener) {
    nsCOMPtr<nsIStreamListener> listener;
    mTargetListener.swap(listener);
    return listener->OnStopRequest(aRequest, aContext, aStatus);
  }

  return NS_OK;
}

nsresult
nsExternalResourceMap::PendingLoad::StartLoad(nsIURI* aURI,
                                              nsINode* aRequestingNode)
{
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aRequestingNode, "Must have a node");

  

  nsIPrincipal* requestingPrincipal = aRequestingNode->NodePrincipal();

  nsresult rv = nsContentUtils::GetSecurityManager()->
    CheckLoadURIWithPrincipal(requestingPrincipal, aURI,
                              nsIScriptSecurityManager::STANDARD);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = requestingPrincipal->CheckMayLoad(aURI, true, true);
  NS_ENSURE_SUCCESS(rv, rv);

  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OTHER,
                                 aURI,
                                 requestingPrincipal,
                                 aRequestingNode,
                                 EmptyCString(), 
                                 nullptr,         
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv)) return rv;
  if (NS_CP_REJECTED(shouldLoad)) {
    
    return NS_ERROR_CONTENT_BLOCKED;
  }

  nsIDocument* doc = aRequestingNode->OwnerDoc();

  nsCOMPtr<nsIInterfaceRequestor> req = nsContentUtils::SameOriginChecker();

  nsCOMPtr<nsILoadGroup> loadGroup = doc->GetDocumentLoadGroup();
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     aURI,
                     aRequestingNode,
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_OTHER,
                     loadGroup,
                     req); 

  NS_ENSURE_SUCCESS(rv, rv);

  mURI = aURI;

  return channel->AsyncOpen(this, nullptr);
}

NS_IMPL_ISUPPORTS(nsExternalResourceMap::LoadgroupCallbacks,
                  nsIInterfaceRequestor)

#define IMPL_SHIM(_i) \
  NS_IMPL_ISUPPORTS(nsExternalResourceMap::LoadgroupCallbacks::_i##Shim, _i)

IMPL_SHIM(nsILoadContext)
IMPL_SHIM(nsIProgressEventSink)
IMPL_SHIM(nsIChannelEventSink)
IMPL_SHIM(nsISecurityEventSink)
IMPL_SHIM(nsIApplicationCacheContainer)

#undef IMPL_SHIM

#define IID_IS(_i) aIID.Equals(NS_GET_IID(_i))

#define TRY_SHIM(_i)                                                       \
  PR_BEGIN_MACRO                                                           \
    if (IID_IS(_i)) {                                                      \
      nsCOMPtr<_i> real = do_GetInterface(mCallbacks);                     \
      if (!real) {                                                         \
        return NS_NOINTERFACE;                                             \
      }                                                                    \
      nsCOMPtr<_i> shim = new _i##Shim(this, real);                        \
      if (!shim) {                                                         \
        return NS_ERROR_OUT_OF_MEMORY;                                     \
      }                                                                    \
      shim.forget(aSink);                                                  \
      return NS_OK;                                                        \
    }                                                                      \
  PR_END_MACRO

NS_IMETHODIMP
nsExternalResourceMap::LoadgroupCallbacks::GetInterface(const nsIID & aIID,
                                                        void **aSink)
{
  if (mCallbacks &&
      (IID_IS(nsIPrompt) || IID_IS(nsIAuthPrompt) || IID_IS(nsIAuthPrompt2) ||
       IID_IS(nsITabChild))) {
    return mCallbacks->GetInterface(aIID, aSink);
  }

  *aSink = nullptr;

  TRY_SHIM(nsILoadContext);
  TRY_SHIM(nsIProgressEventSink);
  TRY_SHIM(nsIChannelEventSink);
  TRY_SHIM(nsISecurityEventSink);
  TRY_SHIM(nsIApplicationCacheContainer);

  return NS_NOINTERFACE;
}

#undef TRY_SHIM
#undef IID_IS

nsExternalResourceMap::ExternalResource::~ExternalResource()
{
  if (mViewer) {
    mViewer->Close(nullptr);
    mViewer->Destroy();
  }
}








class nsDOMStyleSheetSetList final : public DOMStringList
{
public:
  explicit nsDOMStyleSheetSetList(nsIDocument* aDocument);

  void Disconnect()
  {
    mDocument = nullptr;
  }

  virtual void EnsureFresh() override;

protected:
  nsIDocument* mDocument;  
                           
};

nsDOMStyleSheetSetList::nsDOMStyleSheetSetList(nsIDocument* aDocument)
  : mDocument(aDocument)
{
  NS_ASSERTION(mDocument, "Must have document!");
}

void
nsDOMStyleSheetSetList::EnsureFresh()
{
  MOZ_ASSERT(NS_IsMainThread());

  mNames.Clear();

  if (!mDocument) {
    return; 
            
  }

  int32_t count = mDocument->GetNumberOfStyleSheets();
  nsAutoString title;
  for (int32_t index = 0; index < count; index++) {
    nsIStyleSheet* sheet = mDocument->GetStyleSheetAt(index);
    NS_ASSERTION(sheet, "Null sheet in sheet list!");
    sheet->GetTitle(title);
    if (!title.IsEmpty() && !mNames.Contains(title) && !Add(title)) {
      return;
    }
  }
}


nsIDocument::SelectorCache::SelectorCache()
  : nsExpirationTracker<SelectorCacheKey, 4>(1000) { }


void nsIDocument::SelectorCache::CacheList(const nsAString& aSelector,
                                           nsCSSSelectorList* aSelectorList)
{
  SelectorCacheKey* key = new SelectorCacheKey(aSelector);
  mTable.Put(key->mKey, aSelectorList);
  AddObject(key);
}

class nsIDocument::SelectorCacheKeyDeleter final : public nsRunnable
{
public:
  explicit SelectorCacheKeyDeleter(SelectorCacheKey* aToDelete)
    : mSelector(aToDelete)
  {
    MOZ_COUNT_CTOR(SelectorCacheKeyDeleter);
  }

protected:
  ~SelectorCacheKeyDeleter()
  {
    MOZ_COUNT_DTOR(SelectorCacheKeyDeleter);
  }

public:
  NS_IMETHOD Run()
  {
    return NS_OK;
  }

private:
  nsAutoPtr<SelectorCacheKey> mSelector;
};

void nsIDocument::SelectorCache::NotifyExpired(SelectorCacheKey* aSelector)
{
  RemoveObject(aSelector);
  mTable.Remove(aSelector->mKey);
  nsCOMPtr<nsIRunnable> runnable = new SelectorCacheKeyDeleter(aSelector);
  NS_DispatchToCurrentThread(runnable);
}


struct nsIDocument::FrameRequest
{
  FrameRequest(const FrameRequestCallbackHolder& aCallback,
               int32_t aHandle) :
    mCallback(aCallback),
    mHandle(aHandle)
  {}

  
  
  operator const FrameRequestCallbackHolder& () const {
    return mCallback;
  }

  
  
  bool operator==(int32_t aHandle) const {
    return mHandle == aHandle;
  }
  bool operator<(int32_t aHandle) const {
    return mHandle < aHandle;
  }

  FrameRequestCallbackHolder mCallback;
  int32_t mHandle;
};

static already_AddRefed<mozilla::dom::NodeInfo> nullNodeInfo;




nsIDocument::nsIDocument()
  : nsINode(nullNodeInfo),
    mReferrerPolicySet(false),
    mReferrerPolicy(mozilla::net::RP_Default),
    mCharacterSet(NS_LITERAL_CSTRING("ISO-8859-1")),
    mNodeInfoManager(nullptr),
    mCompatMode(eCompatibility_FullStandards),
    mVisibilityState(dom::VisibilityState::Hidden),
    mIsInitialDocumentInWindow(false),
    mMayStartLayout(true),
    mVisible(true),
    mRemovedFromDocShell(false),
    
    
    
    
    mAllowDNSPrefetch(true),
    mIsBeingUsedAsImage(false),
    mHasLinksToUpdate(false),
    mPartID(0),
    mDidFireDOMContentLoaded(true)
{
  SetInDocument();

  PR_INIT_CLIST(&mDOMMediaQueryLists);  
}




nsDocument::nsDocument(const char* aContentType)
  : nsIDocument()
  , mAnimatingImages(true)
  , mViewportType(Unknown)
{
  SetContentTypeInternal(nsDependentCString(aContentType));

#ifdef PR_LOGGING
  if (!gDocumentLeakPRLog)
    gDocumentLeakPRLog = PR_NewLogModule("DocumentLeak");

  if (gDocumentLeakPRLog)
    PR_LOG(gDocumentLeakPRLog, PR_LOG_DEBUG,
           ("DOCUMENT %p created", this));

  if (!gCspPRLog)
    gCspPRLog = PR_NewLogModule("CSP");
#endif

  
  SetDOMStringToNull(mLastStyleSheetSet);

  
  mPreloadPictureFoundSource.SetIsVoid(true);

  if (!sProcessingStack) {
    sProcessingStack.emplace();
    
    sProcessingStack->AppendElement((CustomElementData*) nullptr);
  }
}

static PLDHashOperator
ClearAllBoxObjects(nsIContent* aKey, nsPIBoxObject* aBoxObject, void* aUserArg)
{
  if (aBoxObject) {
    aBoxObject->Clear();
  }
  return PL_DHASH_NEXT;
}

nsIDocument::~nsIDocument()
{
  MOZ_ASSERT(PR_CLIST_IS_EMPTY(&mDOMMediaQueryLists),
             "must not have media query lists left");

  if (mNodeInfoManager) {
    mNodeInfoManager->DropDocumentReference();
  }
}


nsDocument::~nsDocument()
{
#ifdef PR_LOGGING
  if (gDocumentLeakPRLog)
    PR_LOG(gDocumentLeakPRLog, PR_LOG_DEBUG,
           ("DOCUMENT %p destroyed", this));
#endif

  NS_ASSERTION(!mIsShowing, "Destroying a currently-showing document");

  
  
  NS_ASSERTION(!mObservingAppThemeChanged,
               "Document leaked to shutdown, then the observer service dropped "
               "its ref to us so we were able to go away.");

  if (IsTopLevelContentDocument()) {
    
    nsCOMPtr<nsIPrincipal> principal = GetPrincipal();
    nsCOMPtr<nsIURI> uri;
    principal->GetURI(getter_AddRefs(uri));
    bool isAboutScheme = true;
    if (uri) {
      uri->SchemeIs("about", &isAboutScheme);
    }

    if (!isAboutScheme) {
      
      uint32_t pageLoaded = 1;
      Accumulate(Telemetry::MIXED_CONTENT_UNBLOCK_COUNTER, pageLoaded);
      
      enum {
        NO_MIXED_CONTENT = 0, 
        MIXED_DISPLAY_CONTENT = 1, 
        MIXED_ACTIVE_CONTENT = 2, 
        MIXED_DISPLAY_AND_ACTIVE_CONTENT = 3 
      };

      bool mixedActiveLoaded = GetHasMixedActiveContentLoaded();
      bool mixedActiveBlocked = GetHasMixedActiveContentBlocked();

      bool mixedDisplayLoaded = GetHasMixedDisplayContentLoaded();
      bool mixedDisplayBlocked = GetHasMixedDisplayContentBlocked();

      bool hasMixedDisplay = (mixedDisplayBlocked || mixedDisplayLoaded);
      bool hasMixedActive = (mixedActiveBlocked || mixedActiveLoaded);

      uint32_t mixedContentLevel = NO_MIXED_CONTENT;
      if (hasMixedDisplay && hasMixedActive) {
        mixedContentLevel = MIXED_DISPLAY_AND_ACTIVE_CONTENT;
      } else if (hasMixedActive){
        mixedContentLevel = MIXED_ACTIVE_CONTENT;
      } else if (hasMixedDisplay) {
        mixedContentLevel = MIXED_DISPLAY_CONTENT;
      }
      Accumulate(Telemetry::MIXED_CONTENT_PAGE_LOAD, mixedContentLevel);
    }
  }

  mInDestructor = true;
  mInUnlinkOrDeletion = true;

  mRegistry = nullptr;

  mozilla::DropJSObjects(this);

  
  mObservers.Clear();

  if (mStyleSheetSetList) {
    mStyleSheetSetList->Disconnect();
  }

  if (mAnimationController) {
    mAnimationController->Disconnect();
  }

  mParentDocument = nullptr;

  
  
  if (mSubDocuments) {
    PL_DHashTableDestroy(mSubDocuments);

    mSubDocuments = nullptr;
  }

  
  
  DestroyElementMaps();

  nsAutoScriptBlocker scriptBlocker;

  int32_t indx; 
  uint32_t count = mChildren.ChildCount();
  for (indx = int32_t(count) - 1; indx >= 0; --indx) {
    mChildren.ChildAt(indx)->UnbindFromTree();
    mChildren.RemoveChildAt(indx);
  }
  mFirstChild = nullptr;
  mCachedRootElement = nullptr;

  
  indx = mStyleSheets.Count();
  while (--indx >= 0) {
    mStyleSheets[indx]->SetOwningDocument(nullptr);
  }
  if (mAttrStyleSheet) {
    mAttrStyleSheet->SetOwningDocument(nullptr);
  }
  

  if (mListenerManager) {
    mListenerManager->Disconnect();
    UnsetFlags(NODE_HAS_LISTENERMANAGER);
  }

  if (mScriptLoader) {
    mScriptLoader->DropDocumentReference();
  }

  if (mCSSLoader) {
    
    mCSSLoader->DropDocumentReference();
  }

  if (mStyleImageLoader) {
    mStyleImageLoader->DropDocumentReference();
  }

  delete mHeaderData;

  if (mBoxObjectTable) {
    mBoxObjectTable->EnumerateRead(ClearAllBoxObjects, nullptr);
    delete mBoxObjectTable;
  }

  mPendingTitleChangeEvent.Revoke();

  for (uint32_t i = 0; i < mHostObjectURIs.Length(); ++i) {
    nsHostObjectProtocolHandler::RemoveDataEntry(mHostObjectURIs[i]);
  }

  
  
  SetImageLockingState(false);
  mImageTracker.Clear();

  mPlugins.Clear();
}

NS_INTERFACE_TABLE_HEAD(nsDocument)
  NS_WRAPPERCACHE_INTERFACE_TABLE_ENTRY
  NS_INTERFACE_TABLE_BEGIN
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(nsDocument, nsISupports, nsINode)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsINode)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDocument)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMDocument)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMNode)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMDocumentXBL)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIScriptObjectPrincipal)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMEventTarget)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, mozilla::dom::EventTarget)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsISupportsWeakReference)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIRadioGroupContainer)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIMutationObserver)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIApplicationCacheContainer)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIObserver)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMXPathEvaluator)
  NS_INTERFACE_TABLE_END
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsDocument)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDocument)
NS_IMETHODIMP_(MozExternalRefCountType)
nsDocument::Release()
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  NS_ASSERT_OWNINGTHREAD(nsDocument);
  nsISupports* base = NS_CYCLE_COLLECTION_CLASSNAME(nsDocument)::Upcast(this);
  bool shouldDelete = false;
  nsrefcnt count = mRefCnt.decr(base, &shouldDelete);
  NS_LOG_RELEASE(this, count, "nsDocument");
  if (count == 0) {
    if (mStackRefCnt && !mNeedsReleaseAfterStackRefCntRelease) {
      mNeedsReleaseAfterStackRefCntRelease = true;
      NS_ADDREF_THIS();
      return mRefCnt.get();
    }
    mRefCnt.incr(base);
    nsNodeUtils::LastRelease(this);
    mRefCnt.decr(base);
    if (shouldDelete) {
      mRefCnt.stabilizeForDeletion();
      DeleteCycleCollectable();
    }
  }
  return count;
}

NS_IMETHODIMP_(void)
nsDocument::DeleteCycleCollectable()
{
  delete this;
}

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsDocument)
  if (Element::CanSkip(tmp, aRemovingAllowed)) {
    EventListenerManager* elm = tmp->GetExistingListenerManager();
    if (elm) {
      elm->MarkForCC();
    }
    if (tmp->mExpandoAndGeneration.expando.isObject()) {
      JS::ExposeObjectToActiveJS(
        &(tmp->mExpandoAndGeneration.expando.toObject()));
    }
    return true;
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsDocument)
  return Element::CanSkipInCC(tmp);
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsDocument)
  return Element::CanSkipThis(tmp);
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

static PLDHashOperator
SubDocTraverser(PLDHashTable *table, PLDHashEntryHdr *hdr, uint32_t number,
                void *arg)
{
  SubDocMapEntry *entry = static_cast<SubDocMapEntry*>(hdr);
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(arg);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mSubDocuments entry->mKey");
  cb->NoteXPCOMChild(entry->mKey);
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mSubDocuments entry->mSubDocument");
  cb->NoteXPCOMChild(entry->mSubDocument);

  return PL_DHASH_NEXT;
}

static PLDHashOperator
RadioGroupsTraverser(const nsAString& aKey, nsRadioGroupStruct* aData,
                     void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                   "mRadioGroups entry->mSelectedRadioButton");
  cb->NoteXPCOMChild(ToSupports(aData->mSelectedRadioButton));

  uint32_t i, count = aData->mRadioButtons.Count();
  for (i = 0; i < count; ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                       "mRadioGroups entry->mRadioButtons[i]");
    cb->NoteXPCOMChild(aData->mRadioButtons[i]);
  }

  return PL_DHASH_NEXT;
}

static PLDHashOperator
BoxObjectTraverser(nsIContent* key, nsPIBoxObject* boxObject, void* userArg)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(userArg);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mBoxObjectTable entry");
  cb->NoteXPCOMChild(boxObject);

  return PL_DHASH_NEXT;
}

static PLDHashOperator
IdentifierMapEntryTraverse(nsIdentifierMapEntry *aEntry, void *aArg)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aArg);
  aEntry->Traverse(cb);
  return PL_DHASH_NEXT;
}

static const char* kNSURIs[] = {
  "([none])",
  "(xmlns)",
  "(xml)",
  "(xhtml)",
  "(XLink)",
  "(XSLT)",
  "(XBL)",
  "(MathML)",
  "(RDF)",
  "(XUL)"
};

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsDocument)
  if (MOZ_UNLIKELY(cb.WantDebugInfo())) {
    char name[512];
    nsAutoCString loadedAsData;
    if (tmp->IsLoadedAsData()) {
      loadedAsData.AssignLiteral("data");
    } else {
      loadedAsData.AssignLiteral("normal");
    }
    uint32_t nsid = tmp->GetDefaultNamespaceID();
    nsAutoCString uri;
    if (tmp->mDocumentURI)
      tmp->mDocumentURI->GetSpec(uri);
    if (nsid < ArrayLength(kNSURIs)) {
      PR_snprintf(name, sizeof(name), "nsDocument %s %s %s",
                  loadedAsData.get(), kNSURIs[nsid], uri.get());
    }
    else {
      PR_snprintf(name, sizeof(name), "nsDocument %s %s",
                  loadedAsData.get(), uri.get());
    }
    cb.DescribeRefCountedNode(tmp->mRefCnt.get(), name);
  }
  else {
    NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsDocument, tmp->mRefCnt.get())
  }

  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS

  if (!nsINode::Traverse(tmp, cb)) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }

  tmp->mIdentifierMap.EnumerateEntries(IdentifierMapEntryTraverse, &cb);

  tmp->mExternalResourceMap.Traverse(&cb);

  
  for (int32_t indx = int32_t(tmp->mChildren.ChildCount()); indx > 0; --indx) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mChildren[i]");
    cb.NoteXPCOMChild(tmp->mChildren.ChildAt(indx - 1));
  }

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSecurityInfo)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDisplayDocument)

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mScriptGlobalObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mListenerManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDOMStyleSheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStyleSheetSetList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mScriptLoader)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMasterDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mImportManager)

  tmp->mRadioGroups.EnumerateRead(RadioGroupsTraverser, &cb);

  
  
  if (tmp->mBoxObjectTable) {
    tmp->mBoxObjectTable->EnumerateRead(BoxObjectTraverser, &cb);
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChannel)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStyleAttrStyleSheet)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mXPathEvaluator)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mLayoutHistoryState)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOnloadBlocker)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFirstBaseNodeWithHref)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDOMImplementation)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mImageMaps)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOriginalDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCachedEncoder)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStateObjectCached)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mUndoManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocumentTimeline)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPendingAnimationTracker)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTemplateContentsOwner)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChildrenCollection)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRegistry)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAnonymousContents)

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStyleSheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOnDemandBuiltInUASheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPreloadingImages)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSubImportLinks)

  for (uint32_t i = 0; i < tmp->mFrameRequestCallbacks.Length(); ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mFrameRequestCallbacks[i]");
    cb.NoteXPCOMChild(tmp->mFrameRequestCallbacks[i].mCallback.GetISupports());
  }

  
  if (tmp->mAnimationController) {
    tmp->mAnimationController->Traverse(&cb);
  }

  if (tmp->mSubDocuments && tmp->mSubDocuments->IsInitialized()) {
    PL_DHashTableEnumerate(tmp->mSubDocuments, SubDocTraverser, &cb);
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCSSLoader)

  for (uint32_t i = 0; i < tmp->mHostObjectURIs.Length(); ++i) {
    nsHostObjectProtocolHandler::Traverse(tmp->mHostObjectURIs[i], cb);
  }

  
  
  
  for (PRCList *l = PR_LIST_HEAD(&tmp->mDOMMediaQueryLists);
       l != &tmp->mDOMMediaQueryLists; l = PR_NEXT_LINK(l)) {
    MediaQueryList *mql = static_cast<MediaQueryList*>(l);
    if (mql->HasListeners()) {
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mDOMMediaQueryLists item");
      cb.NoteXPCOMChild(mql);
    }
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDocument)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsDocument)
  if (tmp->PreservingWrapper()) {
    NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mExpandoAndGeneration.expando);
  }
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDocument)
  tmp->mInUnlinkOrDeletion = true;

  
  tmp->mExternalResourceMap.Shutdown();

  nsAutoScriptBlocker scriptBlocker;

  nsINode::Unlink(tmp);

  
  for (int32_t indx = int32_t(tmp->mChildren.ChildCount()) - 1;
       indx >= 0; --indx) {
    tmp->mChildren.ChildAt(indx)->UnbindFromTree();
    tmp->mChildren.RemoveChildAt(indx);
  }
  tmp->mFirstChild = nullptr;

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mXPathEvaluator)
  tmp->mCachedRootElement = nullptr; 
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDisplayDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mFirstBaseNodeWithHref)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDOMImplementation)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mImageMaps)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOriginalDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCachedEncoder)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mUndoManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocumentTimeline)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPendingAnimationTracker)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mTemplateContentsOwner)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mChildrenCollection)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRegistry)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mMasterDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mImportManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mSubImportLinks)

  tmp->mParentDocument = nullptr;

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPreloadingImages)


  if (tmp->mBoxObjectTable) {
   tmp->mBoxObjectTable->EnumerateRead(ClearAllBoxObjects, nullptr);
   delete tmp->mBoxObjectTable;
   tmp->mBoxObjectTable = nullptr;
 }

  if (tmp->mListenerManager) {
    tmp->mListenerManager->Disconnect();
    tmp->UnsetFlags(NODE_HAS_LISTENERMANAGER);
    tmp->mListenerManager = nullptr;
  }

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDOMStyleSheets)

  if (tmp->mStyleSheetSetList) {
    tmp->mStyleSheetSetList->Disconnect();
    tmp->mStyleSheetSetList = nullptr;
  }

  if (tmp->mSubDocuments) {
    PL_DHashTableDestroy(tmp->mSubDocuments);
    tmp->mSubDocuments = nullptr;
  }

  tmp->mFrameRequestCallbacks.Clear();

  tmp->mRadioGroups.Clear();

  
  
  

  tmp->mIdentifierMap.Clear();
  tmp->mExpandoAndGeneration.Unlink();

  if (tmp->mAnimationController) {
    tmp->mAnimationController->Unlink();
  }

  tmp->mPendingTitleChangeEvent.Revoke();

  if (tmp->mCSSLoader) {
    tmp->mCSSLoader->DropDocumentReference();
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mCSSLoader)
  }

  for (uint32_t i = 0; i < tmp->mHostObjectURIs.Length(); ++i) {
    nsHostObjectProtocolHandler::RemoveDataEntry(tmp->mHostObjectURIs[i]);
  }

  
  
  
  for (PRCList *l = PR_LIST_HEAD(&tmp->mDOMMediaQueryLists);
       l != &tmp->mDOMMediaQueryLists; ) {
    PRCList *next = PR_NEXT_LINK(l);
    MediaQueryList *mql = static_cast<MediaQueryList*>(l);
    mql->RemoveAllListeners();
    l = next;
  }

  tmp->mInUnlinkOrDeletion = false;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

static bool sPrefsInitialized = false;
static uint32_t sOnloadDecodeLimit = 0;

nsresult
nsDocument::Init()
{
  if (mCSSLoader || mStyleImageLoader || mNodeInfoManager || mScriptLoader) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  if (!sPrefsInitialized) {
    sPrefsInitialized = true;
    Preferences::AddUintVarCache(&sOnloadDecodeLimit, "image.onload.decode.limit", 0);
  }

  
  nsINode::nsSlots* slots = Slots();

  
  
  
  
  NS_ENSURE_TRUE(slots->mMutationObservers.PrependElementUnlessExists(static_cast<nsIMutationObserver*>(this)),
                 NS_ERROR_OUT_OF_MEMORY);


  mOnloadBlocker = new nsOnloadBlocker();
  mCSSLoader = new mozilla::css::Loader(this);
  
  mCSSLoader->SetCompatibilityMode(eCompatibility_FullStandards);

  mStyleImageLoader = new mozilla::css::ImageLoader(this);

  mNodeInfoManager = new nsNodeInfoManager();
  nsresult rv = mNodeInfoManager->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mNodeInfo = mNodeInfoManager->GetDocumentNodeInfo();
  NS_ENSURE_TRUE(mNodeInfo, NS_ERROR_OUT_OF_MEMORY);
  MOZ_ASSERT(mNodeInfo->NodeType() == nsIDOMNode::DOCUMENT_NODE,
             "Bad NodeType in aNodeInfo");

  NS_ASSERTION(OwnerDoc() == this, "Our nodeinfo is busted!");

  
  
  
  
  nsCOMPtr<nsIGlobalObject> global = xpc::NativeGlobal(xpc::PrivilegedJunkScope());
  NS_ENSURE_TRUE(global, NS_ERROR_FAILURE);
  mScopeObject = do_GetWeakReference(global);
  MOZ_ASSERT(mScopeObject);

  mScriptLoader = new nsScriptLoader(this);

  mozilla::HoldJSObjects(this);

  return NS_OK;
}

void
nsIDocument::DeleteAllProperties()
{
  for (uint32_t i = 0; i < GetPropertyTableCount(); ++i) {
    PropertyTable(i)->DeleteAllProperties();
  }
}

void
nsIDocument::DeleteAllPropertiesFor(nsINode* aNode)
{
  for (uint32_t i = 0; i < GetPropertyTableCount(); ++i) {
    PropertyTable(i)->DeleteAllPropertiesFor(aNode);
  }
}

nsPropertyTable*
nsIDocument::GetExtraPropertyTable(uint16_t aCategory)
{
  NS_ASSERTION(aCategory > 0, "Category 0 should have already been handled");
  while (aCategory >= mExtraPropertyTables.Length() + 1) {
    mExtraPropertyTables.AppendElement(new nsPropertyTable());
  }
  return mExtraPropertyTables[aCategory - 1];
}

bool
nsIDocument::IsVisibleConsideringAncestors() const
{
  const nsIDocument *parent = this;
  do {
    if (!parent->IsVisible()) {
      return false;
    }
  } while ((parent = parent->GetParentDocument()));

  return true;
      }

void
nsDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsIPrincipal> principal;
  if (aChannel) {
    
    
    
    NS_GetFinalChannelURI(aChannel, getter_AddRefs(uri));

    nsIScriptSecurityManager *securityManager =
      nsContentUtils::GetSecurityManager();
    if (securityManager) {
      securityManager->GetChannelResultPrincipal(aChannel,
                                                 getter_AddRefs(principal));
    }
  }

  ResetToURI(uri, aLoadGroup, principal);

  
  
  
  mDocumentTimeline = nullptr;

  nsCOMPtr<nsIPropertyBag2> bag = do_QueryInterface(aChannel);
  if (bag) {
    nsCOMPtr<nsIURI> baseURI;
    bag->GetPropertyAsInterface(NS_LITERAL_STRING("baseURI"),
                                NS_GET_IID(nsIURI), getter_AddRefs(baseURI));
    if (baseURI) {
      mDocumentBaseURI = baseURI;
      mChromeXHRDocBaseURI = baseURI;
    }
  }

  mChannel = aChannel;
}

void
nsDocument::ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                       nsIPrincipal* aPrincipal)
{
  NS_PRECONDITION(aURI, "Null URI passed to ResetToURI");

#ifdef PR_LOGGING
  if (gDocumentLeakPRLog && PR_LOG_TEST(gDocumentLeakPRLog, PR_LOG_DEBUG)) {
    nsAutoCString spec;
    aURI->GetSpec(spec);
    PR_LogPrint("DOCUMENT %p ResetToURI %s", this, spec.get());
  }
#endif

  mSecurityInfo = nullptr;

  mDocumentLoadGroup = nullptr;

  
  
  if (mSubDocuments) {
    PL_DHashTableDestroy(mSubDocuments);

    mSubDocuments = nullptr;
  }

  
  
  DestroyElementMaps();

  bool oldVal = mInUnlinkOrDeletion;
  mInUnlinkOrDeletion = true;
  uint32_t count = mChildren.ChildCount();
  { 
    MOZ_AUTO_DOC_UPDATE(this, UPDATE_CONTENT_MODEL, true);
    for (int32_t i = int32_t(count) - 1; i >= 0; i--) {
      nsCOMPtr<nsIContent> content = mChildren.ChildAt(i);

      nsIContent* previousSibling = content->GetPreviousSibling();

      if (nsINode::GetFirstChild() == content) {
        mFirstChild = content->GetNextSibling();
      }
      mChildren.RemoveChildAt(i);
      nsNodeUtils::ContentRemoved(this, content, i, previousSibling);
      content->UnbindFromTree();
    }
    mCachedRootElement = nullptr;
  }
  mInUnlinkOrDeletion = oldVal;

  if (!mMasterDocument) {
    
    
    
    mRegistry = nullptr;
  }

  
  ResetStylesheetsToURI(aURI);

  
  if (mListenerManager) {
    mListenerManager->Disconnect();
    mListenerManager = nullptr;
  }

  
  mDOMStyleSheets = nullptr;

  
  
  
  
  SetPrincipal(nullptr);

  
  mOriginalURI = nullptr;

  SetDocumentURI(aURI);
  mChromeXHRDocURI = aURI;
  
  
  mDocumentBaseURI = nullptr;
  mChromeXHRDocBaseURI = nullptr;

  if (aLoadGroup) {
    mDocumentLoadGroup = do_GetWeakReference(aLoadGroup);
    
    
    

    
    
  }

  mLastModified.Truncate();
  
  
  SetContentTypeInternal(EmptyCString());
  mContentLanguage.Truncate();
  mBaseTarget.Truncate();
  mReferrer.Truncate();

  mXMLDeclarationBits = 0;

  
  if (aPrincipal) {
    SetPrincipal(aPrincipal);
  } else {
    nsIScriptSecurityManager *securityManager =
      nsContentUtils::GetSecurityManager();
    if (securityManager) {
      nsCOMPtr<nsILoadContext> loadContext(mDocumentContainer);

      if (!loadContext && aLoadGroup) {
        nsCOMPtr<nsIInterfaceRequestor> cbs;
        aLoadGroup->GetNotificationCallbacks(getter_AddRefs(cbs));
        loadContext = do_GetInterface(cbs);
      }

      MOZ_ASSERT(loadContext,
                 "must have a load context or pass in an explicit principal");

      nsCOMPtr<nsIPrincipal> principal;
      nsresult rv = securityManager->
        GetLoadContextCodebasePrincipal(mDocumentURI, loadContext,
                                        getter_AddRefs(principal));
      if (NS_SUCCEEDED(rv)) {
        SetPrincipal(principal);
      }
    }
  }

  
  nsPIDOMWindow* win = GetInnerWindow();
  if (win) {
    win->RefreshCompartmentPrincipal();
  }
}

void
nsDocument::RemoveDocStyleSheetsFromStyleSets()
{
  
  int32_t indx = mStyleSheets.Count();
  while (--indx >= 0) {
    nsIStyleSheet* sheet = mStyleSheets[indx];
    sheet->SetOwningDocument(nullptr);

    if (sheet->IsApplicable()) {
      nsCOMPtr<nsIPresShell> shell = GetShell();
      if (shell) {
        shell->StyleSet()->RemoveDocStyleSheet(sheet);
      }
    }
    
  }
}

void
nsDocument::RemoveStyleSheetsFromStyleSets(nsCOMArray<nsIStyleSheet>& aSheets, nsStyleSet::sheetType aType)
{
  
  int32_t indx = aSheets.Count();
  while (--indx >= 0) {
    nsIStyleSheet* sheet = aSheets[indx];
    sheet->SetOwningDocument(nullptr);

    if (sheet->IsApplicable()) {
      nsCOMPtr<nsIPresShell> shell = GetShell();
      if (shell) {
        shell->StyleSet()->RemoveStyleSheet(aType, sheet);
      }
    }

    
  }

}

void
nsDocument::ResetStylesheetsToURI(nsIURI* aURI)
{
  MOZ_ASSERT(aURI);

  mozAutoDocUpdate upd(this, UPDATE_STYLE, true);
  RemoveDocStyleSheetsFromStyleSets();
  RemoveStyleSheetsFromStyleSets(mOnDemandBuiltInUASheets, nsStyleSet::eAgentSheet);
  RemoveStyleSheetsFromStyleSets(mAdditionalSheets[eAgentSheet], nsStyleSet::eAgentSheet);
  RemoveStyleSheetsFromStyleSets(mAdditionalSheets[eUserSheet], nsStyleSet::eUserSheet);
  RemoveStyleSheetsFromStyleSets(mAdditionalSheets[eAuthorSheet], nsStyleSet::eDocSheet);

  
  mStyleSheets.Clear();
  mOnDemandBuiltInUASheets.Clear();
  for (uint32_t i = 0; i < SheetTypeCount; ++i)
    mAdditionalSheets[i].Clear();

  
  
  

  
  if (mAttrStyleSheet) {
    mAttrStyleSheet->Reset();
    mAttrStyleSheet->SetOwningDocument(this);
  } else {
    mAttrStyleSheet = new nsHTMLStyleSheet(this);
  }

  if (!mStyleAttrStyleSheet) {
    mStyleAttrStyleSheet = new nsHTMLCSSStyleSheet();
  }

  if (!mSVGAttrAnimationRuleProcessor) {
    mSVGAttrAnimationRuleProcessor =
      new mozilla::SVGAttrAnimationRuleProcessor();
  }

  
  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    FillStyleSet(shell->StyleSet());
  }
}

static bool
AppendAuthorSheet(nsIStyleSheet *aSheet, void *aData)
{
  nsStyleSet *styleSet = static_cast<nsStyleSet*>(aData);
  styleSet->AppendStyleSheet(nsStyleSet::eDocSheet, aSheet);
  return true;
}

static void
AppendSheetsToStyleSet(nsStyleSet* aStyleSet,
                       const nsCOMArray<nsIStyleSheet>& aSheets,
                       nsStyleSet::sheetType aType)
{
  for (int32_t i = aSheets.Count() - 1; i >= 0; --i) {
    aStyleSet->AppendStyleSheet(aType, aSheets[i]);
  }
}


void
nsDocument::FillStyleSet(nsStyleSet* aStyleSet)
{
  NS_PRECONDITION(aStyleSet, "Must have a style set");
  NS_PRECONDITION(aStyleSet->SheetCount(nsStyleSet::eDocSheet) == 0,
                  "Style set already has document sheets?");

  
  
  aStyleSet->DirtyRuleProcessors(nsStyleSet::ePresHintSheet);
  aStyleSet->DirtyRuleProcessors(nsStyleSet::eStyleAttrSheet);

  int32_t i;
  for (i = mStyleSheets.Count() - 1; i >= 0; --i) {
    nsIStyleSheet* sheet = mStyleSheets[i];
    if (sheet->IsApplicable()) {
      aStyleSet->AddDocStyleSheet(sheet, this);
    }
  }

  nsStyleSheetService *sheetService = nsStyleSheetService::GetInstance();
  if (sheetService) {
    sheetService->AuthorStyleSheets()->EnumerateForwards(AppendAuthorSheet,
                                                         aStyleSet);
  }

  
  for (i = mOnDemandBuiltInUASheets.Count() - 1; i >= 0; --i) {
    nsIStyleSheet* sheet = mOnDemandBuiltInUASheets[i];
    if (sheet->IsApplicable()) {
      aStyleSet->PrependStyleSheet(nsStyleSet::eAgentSheet, sheet);
    }
  }

  AppendSheetsToStyleSet(aStyleSet, mAdditionalSheets[eAgentSheet],
                         nsStyleSet::eAgentSheet);
  AppendSheetsToStyleSet(aStyleSet, mAdditionalSheets[eUserSheet],
                         nsStyleSet::eUserSheet);
  AppendSheetsToStyleSet(aStyleSet, mAdditionalSheets[eAuthorSheet],
                         nsStyleSet::eDocSheet);
}

static void
WarnIfSandboxIneffective(nsIDocShell* aDocShell,
                         uint32_t aSandboxFlags,
                         nsIChannel* aChannel)
{
  
  
  
  
  
  if (aSandboxFlags & SANDBOXED_NAVIGATION &&
      !(aSandboxFlags & SANDBOXED_SCRIPTS) &&
      !(aSandboxFlags & SANDBOXED_ORIGIN)) {
    nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
    aDocShell->GetSameTypeParent(getter_AddRefs(parentAsItem));
    nsCOMPtr<nsIDocShell> parentDocShell = do_QueryInterface(parentAsItem);
    if (!parentDocShell) {
      return;
    }

    
    nsCOMPtr<nsIDocShellTreeItem> grandParentAsItem;
    parentDocShell->GetSameTypeParent(getter_AddRefs(grandParentAsItem));
    if (grandParentAsItem) {
      return;
    }

    nsCOMPtr<nsIChannel> parentChannel;
    parentDocShell->GetCurrentDocumentChannel(getter_AddRefs(parentChannel));
    if (!parentChannel) {
      return;
    }
    nsresult rv = nsContentUtils::CheckSameOrigin(aChannel, parentChannel);
    if (NS_FAILED(rv)) {
      return;
    }

    nsCOMPtr<nsIDocument> parentDocument = do_GetInterface(parentDocShell);
    nsCOMPtr<nsIURI> iframeUri;
    parentChannel->GetURI(getter_AddRefs(iframeUri));
    nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                    NS_LITERAL_CSTRING("Iframe Sandbox"),
                                    parentDocument,
                                    nsContentUtils::eSECURITY_PROPERTIES,
                                    "BothAllowScriptsAndSameOriginPresent",
                                    nullptr, 0, iframeUri);
  }
}

nsresult
nsDocument::StartDocumentLoad(const char* aCommand, nsIChannel* aChannel,
                              nsILoadGroup* aLoadGroup,
                              nsISupports* aContainer,
                              nsIStreamListener **aDocListener,
                              bool aReset, nsIContentSink* aSink)
{
#ifdef PR_LOGGING
  if (gDocumentLeakPRLog && PR_LOG_TEST(gDocumentLeakPRLog, PR_LOG_DEBUG)) {
    nsCOMPtr<nsIURI> uri;
    aChannel->GetURI(getter_AddRefs(uri));
    nsAutoCString spec;
    if (uri)
      uri->GetSpec(spec);
    PR_LogPrint("DOCUMENT %p StartDocumentLoad %s", this, spec.get());
  }
#endif

#ifdef DEBUG
  {
    uint32_t appId;
    nsresult rv = NodePrincipal()->GetAppId(&appId);
    NS_ENSURE_SUCCESS(rv, rv);
    MOZ_ASSERT(appId != nsIScriptSecurityManager::UNKNOWN_APP_ID,
               "Document should never have UNKNOWN_APP_ID");
  }
#endif

  MOZ_ASSERT(GetReadyStateEnum() == nsIDocument::READYSTATE_UNINITIALIZED,
             "Bad readyState");
  SetReadyStateInternal(READYSTATE_LOADING);

  if (nsCRT::strcmp(kLoadAsData, aCommand) == 0) {
    mLoadedAsData = true;
    
    
    

    
    ScriptLoader()->SetEnabled(false);

    
    CSSLoader()->SetEnabled(false); 
  } else if (nsCRT::strcmp("external-resource", aCommand) == 0) {
    
    ScriptLoader()->SetEnabled(false);
  }

  mMayStartLayout = false;

  mHaveInputEncoding = true;

  if (aReset) {
    Reset(aChannel, aLoadGroup);
  }

  nsAutoCString contentType;
  nsCOMPtr<nsIPropertyBag2> bag = do_QueryInterface(aChannel);
  if ((bag && NS_SUCCEEDED(bag->GetPropertyAsACString(
                NS_LITERAL_STRING("contentType"), contentType))) ||
      NS_SUCCEEDED(aChannel->GetContentType(contentType))) {
    
    nsACString::const_iterator start, end, semicolon;
    contentType.BeginReading(start);
    contentType.EndReading(end);
    semicolon = start;
    FindCharInReadable(';', semicolon, end);
    SetContentTypeInternal(Substring(start, semicolon));
  }

  RetrieveRelevantHeaders(aChannel);

  mChannel = aChannel;
  nsCOMPtr<nsIInputStreamChannel> inStrmChan = do_QueryInterface(mChannel);
  if (inStrmChan) {
    bool isSrcdocChannel;
    inStrmChan->GetIsSrcdocChannel(&isSrcdocChannel);
    if (isSrcdocChannel) {
      mIsSrcdocDocument = true;
    }
  }

  
  
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(aContainer);

  if (docShell) {
    nsresult rv = docShell->GetSandboxFlags(&mSandboxFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    WarnIfSandboxIneffective(docShell, mSandboxFlags, GetChannel());
  }

  
  if (!mLoadedAsData) {
    nsresult rv = InitCSP(aChannel);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
CSPErrorQueue::Add(const char* aMessageName)
{
  mErrors.AppendElement(aMessageName);
}

void
CSPErrorQueue::Flush(nsIDocument* aDocument)
{
  for (uint32_t i = 0; i < mErrors.Length(); i++) {
    nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
        NS_LITERAL_CSTRING("CSP"), aDocument,
        nsContentUtils::eSECURITY_PROPERTIES,
        mErrors[i]);
  }
  mErrors.Clear();
}

void
nsDocument::SendToConsole(nsCOMArray<nsISecurityConsoleMessage>& aMessages)
{
  for (uint32_t i = 0; i < aMessages.Length(); ++i) {
    nsAutoString messageTag;
    aMessages[i]->GetTag(messageTag);

    nsAutoString category;
    aMessages[i]->GetCategory(category);

    nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                    NS_ConvertUTF16toUTF8(category),
                                    this, nsContentUtils::eSECURITY_PROPERTIES,
                                    NS_ConvertUTF16toUTF8(messageTag).get());
  }
}

static nsresult
AppendCSPFromHeader(nsIContentSecurityPolicy* csp,
                    const nsAString& aHeaderValue,
                    bool aReportOnly)
{
  
  
  
  nsresult rv = NS_OK;
  nsCharSeparatedTokenizer tokenizer(aHeaderValue, ',');
  while (tokenizer.hasMoreTokens()) {
      const nsSubstring& policy = tokenizer.nextToken();
      rv = csp->AppendPolicy(policy, aReportOnly);
      NS_ENSURE_SUCCESS(rv, rv);
#ifdef PR_LOGGING
      {
        PR_LOG(gCspPRLog, PR_LOG_DEBUG,
                ("CSP refined with policy: \"%s\"",
                NS_ConvertUTF16toUTF8(policy).get()));
      }
#endif
  }
  return NS_OK;
}

bool
nsDocument::IsLoopDocument(nsIChannel *aChannel)
{
  nsCOMPtr<nsIURI> chanURI;
  nsresult rv = aChannel->GetOriginalURI(getter_AddRefs(chanURI));
  NS_ENSURE_SUCCESS(rv, false);

  bool isAbout = false;
  bool isLoop = false;
  rv = chanURI->SchemeIs("about", &isAbout);
  NS_ENSURE_SUCCESS(rv, false);
  if (isAbout) {
    nsCOMPtr<nsIURI> loopURI;
    rv = NS_NewURI(getter_AddRefs(loopURI), "about:loopconversation");
    NS_ENSURE_SUCCESS(rv, false);
    rv = chanURI->EqualsExceptRef(loopURI, &isLoop);
    NS_ENSURE_SUCCESS(rv, false);
    if (!isLoop) {
      rv = NS_NewURI(getter_AddRefs(loopURI), "about:looppanel");
      NS_ENSURE_SUCCESS(rv, false);
      rv = chanURI->EqualsExceptRef(loopURI, &isLoop);
      NS_ENSURE_SUCCESS(rv, false);
    }
  }
  return isLoop;
}

nsresult
nsDocument::InitCSP(nsIChannel* aChannel)
{
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  if (!CSPService::sCSPEnabled) {
#ifdef PR_LOGGING
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSP is disabled, skipping CSP init for document %p", this));
#endif
    return NS_OK;
  }

  nsAutoCString tCspHeaderValue, tCspROHeaderValue;

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);
  if (httpChannel) {
    httpChannel->GetResponseHeader(
        NS_LITERAL_CSTRING("content-security-policy"),
        tCspHeaderValue);

    httpChannel->GetResponseHeader(
        NS_LITERAL_CSTRING("content-security-policy-report-only"),
        tCspROHeaderValue);
  }
  NS_ConvertASCIItoUTF16 cspHeaderValue(tCspHeaderValue);
  NS_ConvertASCIItoUTF16 cspROHeaderValue(tCspROHeaderValue);

  
  nsIPrincipal* principal = NodePrincipal();

  uint16_t appStatus = principal->GetAppStatus();
  bool applyAppDefaultCSP = false;
  bool applyAppManifestCSP = false;

  nsAutoString appManifestCSP;
  nsAutoString appDefaultCSP;
  if (appStatus != nsIPrincipal::APP_STATUS_NOT_INSTALLED) {
    nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
    if (appsService) {
      uint32_t appId = 0;
      if (NS_SUCCEEDED(principal->GetAppId(&appId))) {
        appsService->GetManifestCSPByLocalId(appId, appManifestCSP);
        if (!appManifestCSP.IsEmpty()) {
          applyAppManifestCSP = true;
        }
        appsService->GetDefaultCSPByLocalId(appId, appDefaultCSP);
        if (!appDefaultCSP.IsEmpty()) {
          applyAppDefaultCSP = true;
        }
      }
    }
  }

 
 bool applyLoopCSP = IsLoopDocument(aChannel);

  
  if (!applyAppDefaultCSP &&
      !applyAppManifestCSP &&
      !applyLoopCSP &&
      cspHeaderValue.IsEmpty() &&
      cspROHeaderValue.IsEmpty()) {
#ifdef PR_LOGGING
    nsCOMPtr<nsIURI> chanURI;
    aChannel->GetURI(getter_AddRefs(chanURI));
    nsAutoCString aspec;
    chanURI->GetAsciiSpec(aspec);
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("no CSP for document, %s, %s",
            aspec.get(),
            applyAppDefaultCSP ? "is app" : "not an app"));
#endif
    return NS_OK;
  }

#ifdef PR_LOGGING
  PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("Document is an app or CSP header specified %p", this));
#endif

  nsresult rv;

  
  
  
  
  
  
  
  
  
  if (applyAppDefaultCSP || applyAppManifestCSP) {
    nsCOMPtr<nsIContentSecurityPolicy> csp;
    rv = principal->GetCsp(getter_AddRefs(csp));
    NS_ENSURE_SUCCESS(rv, rv);

    if (csp) {
#ifdef PR_LOGGING
      PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("%s %s %s",
           "This document is sharing principal with another document.",
           "Since the document is an app, CSP was already set.",
           "Skipping attempt to set CSP."));
#endif
      return NS_OK;
    }
  }

  csp = do_CreateInstance("@mozilla.org/cspcontext;1", &rv);

  if (NS_FAILED(rv)) {
#ifdef PR_LOGGING
    PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("Failed to create CSP object: %x", rv));
#endif
    return rv;
  }

  
  nsCOMPtr<nsIURI> selfURI;
  aChannel->GetURI(getter_AddRefs(selfURI));

  
  csp->SetRequestContext(nullptr, nullptr, aChannel);

  
  if (applyAppDefaultCSP) {
    csp->AppendPolicy(appDefaultCSP, false);
  }

  
  if (applyAppManifestCSP) {
    csp->AppendPolicy(appManifestCSP, false);
  }

  
  if (applyLoopCSP) {
    nsAdoptingString loopCSP;
    loopCSP = Preferences::GetString("loop.CSP");
    NS_ASSERTION(loopCSP, "Missing loop.CSP preference");
    
    if (loopCSP) {
      csp->AppendPolicy(loopCSP, false);
    }
  }

  
  if (!cspHeaderValue.IsEmpty()) {
    rv = AppendCSPFromHeader(csp, cspHeaderValue, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (!cspROHeaderValue.IsEmpty()) {
    rv = AppendCSPFromHeader(csp, cspROHeaderValue, true);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIDocShell> docShell(mDocumentContainer);
  if (docShell) {
    bool safeAncestry = false;

    
    rv = csp->PermitsAncestry(docShell, &safeAncestry);

    if (NS_FAILED(rv) || !safeAncestry) {
#ifdef PR_LOGGING
      PR_LOG(gCspPRLog, PR_LOG_DEBUG,
              ("CSP doesn't like frame's ancestry, not loading."));
#endif
      
      aChannel->Cancel(NS_ERROR_CSP_FRAME_ANCESTOR_VIOLATION);
    }
  }

  
  bool hasReferrerPolicy = false;
  uint32_t referrerPolicy = mozilla::net::RP_Default;
  rv = csp->GetReferrerPolicy(&referrerPolicy, &hasReferrerPolicy);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasReferrerPolicy) {
    
    
    if (!mReferrerPolicySet) {
      mReferrerPolicy = static_cast<ReferrerPolicy>(referrerPolicy);
      mReferrerPolicySet = true;
    } else if (mReferrerPolicy != referrerPolicy) {
      mReferrerPolicy = mozilla::net::RP_No_Referrer;
#ifdef PR_LOGGING
      {
        PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("%s %s",
                "CSP wants to set referrer, but nsDocument"
                "already has it set. No referrers will be sent"));
      }
#endif
    }

    
    
    
  }

  rv = principal->SetCsp(csp);
  NS_ENSURE_SUCCESS(rv, rv);
#ifdef PR_LOGGING
  PR_LOG(gCspPRLog, PR_LOG_DEBUG,
         ("Inserted CSP into principal %p", principal));
#endif

  return NS_OK;
}

void
nsDocument::StopDocumentLoad()
{
  if (mParser) {
    mParserAborted = true;
    mParser->Terminate();
  }
}

void
nsDocument::SetDocumentURI(nsIURI* aURI)
{
  nsCOMPtr<nsIURI> oldBase = GetDocBaseURI();
  mDocumentURI = NS_TryToMakeImmutable(aURI);
  nsIURI* newBase = GetDocBaseURI();

  bool equalBases = false;
  
  
  
  if (oldBase && newBase) {
    oldBase->EqualsExceptRef(newBase, &equalBases);
  }
  else {
    equalBases = !oldBase && !newBase;
  }

  
  
  if (!mOriginalURI)
    mOriginalURI = mDocumentURI;

  
  
  if (!equalBases) {
    RefreshLinkHrefs();
  }
}

void
nsDocument::SetChromeXHRDocURI(nsIURI* aURI)
{
  mChromeXHRDocURI = aURI;
}

void
nsDocument::SetChromeXHRDocBaseURI(nsIURI* aURI)
{
  mChromeXHRDocBaseURI = aURI;
}

NS_IMETHODIMP
nsDocument::GetLastModified(nsAString& aLastModified)
{
  nsIDocument::GetLastModified(aLastModified);
  return NS_OK;
}

static void
GetFormattedTimeString(PRTime aTime, nsAString& aFormattedTimeString)
{
  PRExplodedTime prtime;
  PR_ExplodeTime(aTime, PR_LocalTimeParameters, &prtime);
  
  char formatedTime[24];
  if (PR_snprintf(formatedTime, sizeof(formatedTime),
                  "%02ld/%02ld/%04hd %02ld:%02ld:%02ld",
                  prtime.tm_month + 1, prtime.tm_mday, prtime.tm_year,
                  prtime.tm_hour     ,  prtime.tm_min,  prtime.tm_sec)) {
    CopyASCIItoUTF16(nsDependentCString(formatedTime), aFormattedTimeString);
  } else {
    
    
    aFormattedTimeString.AssignLiteral(MOZ_UTF16("01/01/1970 00:00:00"));
  }
}

void
nsIDocument::GetLastModified(nsAString& aLastModified) const
{
  if (!mLastModified.IsEmpty()) {
    aLastModified.Assign(mLastModified);
  } else {
    GetFormattedTimeString(PR_Now(), aLastModified);
  }
}

void
nsDocument::AddToNameTable(Element *aElement, nsIAtom* aName)
{
  MOZ_ASSERT(nsGenericHTMLElement::ShouldExposeNameAsHTMLDocumentProperty(aElement),
             "Only put elements that need to be exposed as document['name'] in "
             "the named table.");

  nsIdentifierMapEntry *entry =
    mIdentifierMap.PutEntry(nsDependentAtomString(aName));

  
  if (entry) {
    if (!entry->HasNameElement() &&
        !entry->HasIdElementExposedAsHTMLDocumentProperty()) {
      ++mExpandoAndGeneration.generation;
    }
    entry->AddNameElement(this, aElement);
  }
}

void
nsDocument::RemoveFromNameTable(Element *aElement, nsIAtom* aName)
{
  
  if (mIdentifierMap.Count() == 0)
    return;

  nsIdentifierMapEntry *entry =
    mIdentifierMap.GetEntry(nsDependentAtomString(aName));
  if (!entry) 
    return;

  entry->RemoveNameElement(aElement);
  if (!entry->HasNameElement() &&
      !entry->HasIdElementExposedAsHTMLDocumentProperty()) {
    ++mExpandoAndGeneration.generation;
  }
}

void
nsDocument::AddToIdTable(Element *aElement, nsIAtom* aId)
{
  nsIdentifierMapEntry *entry =
    mIdentifierMap.PutEntry(nsDependentAtomString(aId));

  if (entry) { 
    if (nsGenericHTMLElement::ShouldExposeIdAsHTMLDocumentProperty(aElement) &&
        !entry->HasNameElement() &&
        !entry->HasIdElementExposedAsHTMLDocumentProperty()) {
      ++mExpandoAndGeneration.generation;
    }
    entry->AddIdElement(aElement);
  }
}

void
nsDocument::RemoveFromIdTable(Element *aElement, nsIAtom* aId)
{
  NS_ASSERTION(aId, "huhwhatnow?");

  
  if (mIdentifierMap.Count() == 0) {
    return;
  }

  nsIdentifierMapEntry *entry =
    mIdentifierMap.GetEntry(nsDependentAtomString(aId));
  if (!entry) 
    return;

  entry->RemoveIdElement(aElement);
  if (nsGenericHTMLElement::ShouldExposeIdAsHTMLDocumentProperty(aElement) &&
      !entry->HasNameElement() &&
      !entry->HasIdElementExposedAsHTMLDocumentProperty()) {
    ++mExpandoAndGeneration.generation;
  }
  if (entry->IsEmpty()) {
    mIdentifierMap.RawRemoveEntry(entry);
  }
}

nsIPrincipal*
nsDocument::GetPrincipal()
{
  return NodePrincipal();
}

extern bool sDisablePrefetchHTTPSPref;

void
nsDocument::SetPrincipal(nsIPrincipal *aNewPrincipal)
{
  if (aNewPrincipal && mAllowDNSPrefetch && sDisablePrefetchHTTPSPref) {
    nsCOMPtr<nsIURI> uri;
    aNewPrincipal->GetURI(getter_AddRefs(uri));
    bool isHTTPS;
    if (!uri || NS_FAILED(uri->SchemeIs("https", &isHTTPS)) ||
        isHTTPS) {
      mAllowDNSPrefetch = false;
    }
  }
  mNodeInfoManager->SetDocumentPrincipal(aNewPrincipal);
}

NS_IMETHODIMP
nsDocument::GetApplicationCache(nsIApplicationCache **aApplicationCache)
{
  NS_IF_ADDREF(*aApplicationCache = mApplicationCache);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetApplicationCache(nsIApplicationCache *aApplicationCache)
{
  mApplicationCache = aApplicationCache;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetContentType(nsAString& aContentType)
{
  CopyUTF8toUTF16(GetContentTypeInternal(), aContentType);

  return NS_OK;
}

void
nsDocument::SetContentType(const nsAString& aContentType)
{
  NS_ASSERTION(GetContentTypeInternal().IsEmpty() ||
               GetContentTypeInternal().Equals(NS_ConvertUTF16toUTF8(aContentType)),
               "Do you really want to change the content-type?");

  SetContentTypeInternal(NS_ConvertUTF16toUTF8(aContentType));
}

nsresult
nsDocument::GetAllowPlugins(bool * aAllowPlugins)
{
  
  nsCOMPtr<nsIDocShell> docShell(mDocumentContainer);

  if (docShell) {
    docShell->GetAllowPlugins(aAllowPlugins);

    
    
    if (*aAllowPlugins)
      *aAllowPlugins = !(mSandboxFlags & SANDBOXED_PLUGINS);
  }

  return NS_OK;
}

already_AddRefed<UndoManager>
nsDocument::GetUndoManager()
{
  Element* rootElement = GetRootElement();
  if (!rootElement) {
    return nullptr;
  }

  if (!mUndoManager) {
    mUndoManager = new UndoManager(rootElement);
  }

  nsRefPtr<UndoManager> undoManager = mUndoManager;
  return undoManager.forget();
}

bool
nsDocument::IsWebAnimationsEnabled(JSContext* , JSObject* )
{
  MOZ_ASSERT(NS_IsMainThread());

  return nsContentUtils::IsCallerChrome() ||
         Preferences::GetBool("dom.animations-api.core.enabled");
}

DocumentTimeline*
nsDocument::Timeline()
{
  if (!mDocumentTimeline) {
    mDocumentTimeline = new DocumentTimeline(this);
  }

  return mDocumentTimeline;
}



NS_IMETHODIMP
nsDocument::HasFocus(bool* aResult)
{
  ErrorResult rv;
  *aResult = nsIDocument::HasFocus(rv);
  return rv.StealNSResult();
}

bool
nsIDocument::HasFocus(ErrorResult& rv) const
{
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm) {
    rv.Throw(NS_ERROR_NOT_AVAILABLE);
    return false;
  }

  
  nsCOMPtr<nsIDOMWindow> focusedWindow;
  fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
  if (!focusedWindow) {
    return false;
  }

  
  nsCOMPtr<nsIDOMDocument> domDocument;
  focusedWindow->GetDocument(getter_AddRefs(domDocument));
  nsCOMPtr<nsIDocument> document = do_QueryInterface(domDocument);

  for (nsIDocument* currentDoc = document; currentDoc;
       currentDoc = currentDoc->GetParentDocument()) {
    if (currentDoc == this) {
      
      return true;
    }
  }

  return false;
}

NS_IMETHODIMP
nsDocument::GetReferrer(nsAString& aReferrer)
{
  nsIDocument::GetReferrer(aReferrer);
  return NS_OK;
}

void
nsIDocument::GetReferrer(nsAString& aReferrer) const
{
  if (mIsSrcdocDocument && mParentDocument)
      mParentDocument->GetReferrer(aReferrer);
  else
    CopyUTF8toUTF16(mReferrer, aReferrer);
}

nsresult
nsIDocument::GetSrcdocData(nsAString &aSrcdocData)
{
  if (mIsSrcdocDocument) {
    nsCOMPtr<nsIInputStreamChannel> inStrmChan = do_QueryInterface(mChannel);
    if (inStrmChan) {
      return inStrmChan->GetSrcdocData(aSrcdocData);
    }
  }
  aSrcdocData = NullString();
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetActiveElement(nsIDOMElement **aElement)
{
  nsCOMPtr<nsIDOMElement> el(do_QueryInterface(nsIDocument::GetActiveElement()));
  el.forget(aElement);
  return NS_OK;
}

Element*
nsIDocument::GetActiveElement()
{
  
  nsCOMPtr<nsPIDOMWindow> window = GetWindow();
  if (window) {
    nsCOMPtr<nsPIDOMWindow> focusedWindow;
    nsIContent* focusedContent =
      nsFocusManager::GetFocusedDescendant(window, false,
                                           getter_AddRefs(focusedWindow));
    
    if (focusedContent && focusedContent->OwnerDoc() == this) {
      if (focusedContent->ChromeOnlyAccess()) {
        focusedContent = focusedContent->FindFirstNonChromeOnlyAccessContent();
      }
      if (focusedContent) {
        return focusedContent->AsElement();
      }
    }
  }

  
  nsRefPtr<nsHTMLDocument> htmlDoc = AsHTMLDocument();
  if (htmlDoc) {
    
    
    return htmlDoc->GetBody();
  }

  
  return GetDocumentElement();
}

NS_IMETHODIMP
nsDocument::GetCurrentScript(nsIDOMElement **aElement)
{
  nsCOMPtr<nsIDOMElement> el(do_QueryInterface(nsIDocument::GetCurrentScript()));
  el.forget(aElement);
  return NS_OK;
}

Element*
nsIDocument::GetCurrentScript()
{
  nsCOMPtr<Element> el(do_QueryInterface(ScriptLoader()->GetCurrentScript()));
  return el;
}

NS_IMETHODIMP
nsDocument::ElementFromPoint(float aX, float aY, nsIDOMElement** aReturn)
{
  Element* el = nsIDocument::ElementFromPoint(aX, aY);
  nsCOMPtr<nsIDOMElement> retval = do_QueryInterface(el);
  retval.forget(aReturn);
  return NS_OK;
}

Element*
nsIDocument::ElementFromPoint(float aX, float aY)
{
  return ElementFromPointHelper(aX, aY, false, true);
}

Element*
nsDocument::ElementFromPointHelper(float aX, float aY,
                                   bool aIgnoreRootScrollFrame,
                                   bool aFlushLayout)
{
  
  if (!aIgnoreRootScrollFrame && (aX < 0 || aY < 0)) {
    return nullptr;
  }

  nscoord x = nsPresContext::CSSPixelsToAppUnits(aX);
  nscoord y = nsPresContext::CSSPixelsToAppUnits(aY);
  nsPoint pt(x, y);

  
  
  if (aFlushLayout)
    FlushPendingNotifications(Flush_Layout);

  nsIPresShell *ps = GetShell();
  if (!ps) {
    return nullptr;
  }
  nsIFrame *rootFrame = ps->GetRootFrame();

  
  if (!rootFrame) {
    return nullptr; 
  }

  nsIFrame *ptFrame = nsLayoutUtils::GetFrameForPoint(rootFrame, pt,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION | nsLayoutUtils::IGNORE_CROSS_DOC |
    (aIgnoreRootScrollFrame ? nsLayoutUtils::IGNORE_ROOT_SCROLL_FRAME : 0));
  if (!ptFrame) {
    return nullptr;
  }

  nsIContent* elem = GetContentInThisDocument(ptFrame);
  if (elem && !elem->IsElement()) {
    elem = elem->GetParent();
  }
  return elem ? elem->AsElement() : nullptr;
}

nsresult
nsDocument::NodesFromRectHelper(float aX, float aY,
                                float aTopSize, float aRightSize,
                                float aBottomSize, float aLeftSize,
                                bool aIgnoreRootScrollFrame,
                                bool aFlushLayout,
                                nsIDOMNodeList** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  nsSimpleContentList* elements = new nsSimpleContentList(this);
  NS_ADDREF(elements);
  *aReturn = elements;

  
  
  if (!aIgnoreRootScrollFrame && (aX < 0 || aY < 0))
    return NS_OK;

  nscoord x = nsPresContext::CSSPixelsToAppUnits(aX - aLeftSize);
  nscoord y = nsPresContext::CSSPixelsToAppUnits(aY - aTopSize);
  nscoord w = nsPresContext::CSSPixelsToAppUnits(aLeftSize + aRightSize) + 1;
  nscoord h = nsPresContext::CSSPixelsToAppUnits(aTopSize + aBottomSize) + 1;

  nsRect rect(x, y, w, h);

  
  
  if (aFlushLayout) {
    FlushPendingNotifications(Flush_Layout);
  }

  nsIPresShell *ps = GetShell();
  NS_ENSURE_STATE(ps);
  nsIFrame *rootFrame = ps->GetRootFrame();

  
  if (!rootFrame)
    return NS_OK; 

  nsAutoTArray<nsIFrame*,8> outFrames;
  nsLayoutUtils::GetFramesForArea(rootFrame, rect, outFrames,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION | nsLayoutUtils::IGNORE_CROSS_DOC |
    (aIgnoreRootScrollFrame ? nsLayoutUtils::IGNORE_ROOT_SCROLL_FRAME : 0));

  
  nsIContent* lastAdded = nullptr;

  for (uint32_t i = 0; i < outFrames.Length(); i++) {
    nsIContent* node = GetContentInThisDocument(outFrames[i]);

    if (node && !node->IsElement() && !node->IsNodeOfType(nsINode::eTEXT)) {
      
      
      node = node->GetParent();
    }
    if (node && node != lastAdded) {
      elements->AppendElement(node);
      lastAdded = node;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetElementsByClassName(const nsAString& aClasses,
                                   nsIDOMNodeList** aReturn)
{
  *aReturn = nsIDocument::GetElementsByClassName(aClasses).take();
  return NS_OK;
}

already_AddRefed<nsContentList>
nsIDocument::GetElementsByClassName(const nsAString& aClasses)
{
  return nsContentUtils::GetElementsByClassName(this, aClasses);
}

NS_IMETHODIMP
nsDocument::ReleaseCapture()
{
  nsIDocument::ReleaseCapture();
  return NS_OK;
}

void
nsIDocument::ReleaseCapture() const
{
  
  
  nsCOMPtr<nsINode> node = nsIPresShell::GetCapturingContent();
  if (node && nsContentUtils::CanCallerAccess(node)) {
    nsIPresShell::SetCapturingContent(nullptr, 0);
  }
}

already_AddRefed<nsIURI>
nsIDocument::GetBaseURI(bool aTryUseXHRDocBaseURI) const
{
  nsCOMPtr<nsIURI> uri;
  if (aTryUseXHRDocBaseURI && mChromeXHRDocBaseURI) {
    uri = mChromeXHRDocBaseURI;
  } else {
    uri = GetDocBaseURI();
  }

  return uri.forget();
}

nsresult
nsDocument::SetBaseURI(nsIURI* aURI)
{
  if (!aURI && !mDocumentBaseURI) {
    return NS_OK;
  }

  
  if (aURI && mDocumentBaseURI) {
    bool equalBases = false;
    mDocumentBaseURI->Equals(aURI, &equalBases);
    if (equalBases) {
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  nsresult rv = NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);
  if (csp && aURI) {
    bool permitsBaseURI = false;

    
    
    
    rv = csp->Permits(aURI, nsIContentSecurityPolicy::BASE_URI_DIRECTIVE,
                      true, &permitsBaseURI);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!permitsBaseURI) {
      return NS_OK;
    }
  }

  if (aURI) {
    mDocumentBaseURI = NS_TryToMakeImmutable(aURI);
  } else {
    mDocumentBaseURI = nullptr;
  }
  RefreshLinkHrefs();

  return NS_OK;
}

void
nsDocument::GetBaseTarget(nsAString &aBaseTarget)
{
  aBaseTarget = mBaseTarget;
}

void
nsDocument::SetDocumentCharacterSet(const nsACString& aCharSetID)
{
  
  
  
  if (!mCharacterSet.Equals(aCharSetID)) {
    if (mMasterDocument && !aCharSetID.EqualsLiteral("UTF-8")) {
      
      return;
    }
    mCharacterSet = aCharSetID;

    int32_t n = mCharSetObservers.Length();

    for (int32_t i = 0; i < n; i++) {
      nsIObserver* observer = mCharSetObservers.ElementAt(i);

      observer->Observe(static_cast<nsIDocument *>(this), "charset",
                        NS_ConvertASCIItoUTF16(aCharSetID).get());
    }
  }
}

nsresult
nsDocument::AddCharSetObserver(nsIObserver* aObserver)
{
  NS_ENSURE_ARG_POINTER(aObserver);

  NS_ENSURE_TRUE(mCharSetObservers.AppendElement(aObserver), NS_ERROR_FAILURE);

  return NS_OK;
}

void
nsDocument::RemoveCharSetObserver(nsIObserver* aObserver)
{
  mCharSetObservers.RemoveElement(aObserver);
}

void
nsDocument::GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const
{
  aData.Truncate();
  const nsDocHeaderData* data = mHeaderData;
  while (data) {
    if (data->mField == aHeaderField) {
      aData = data->mData;

      break;
    }
    data = data->mNext;
  }
}

void
nsDocument::SetHeaderData(nsIAtom* aHeaderField, const nsAString& aData)
{
  if (!aHeaderField) {
    NS_ERROR("null headerField");
    return;
  }

  if (!mHeaderData) {
    if (!aData.IsEmpty()) { 
      mHeaderData = new nsDocHeaderData(aHeaderField, aData);
    }
  }
  else {
    nsDocHeaderData* data = mHeaderData;
    nsDocHeaderData** lastPtr = &mHeaderData;
    bool found = false;
    do {  
      if (data->mField == aHeaderField) {
        if (!aData.IsEmpty()) {
          data->mData.Assign(aData);
        }
        else {  
          *lastPtr = data->mNext;
          data->mNext = nullptr;
          delete data;
        }
        found = true;

        break;
      }
      lastPtr = &(data->mNext);
      data = *lastPtr;
    } while (data);

    if (!aData.IsEmpty() && !found) {
      
      *lastPtr = new nsDocHeaderData(aHeaderField, aData);
    }
  }

  if (aHeaderField == nsGkAtoms::headerContentLanguage) {
    CopyUTF16toUTF8(aData, mContentLanguage);
  }

  if (aHeaderField == nsGkAtoms::headerDefaultStyle) {
    
    
    if (DOMStringIsNull(mLastStyleSheetSet)) {
      
      
      
      
      
      EnableStyleSheetsForSetInternal(aData, true);
    }
  }

  if (aHeaderField == nsGkAtoms::refresh) {
    
    
    nsCOMPtr<nsIRefreshURI> refresher(mDocumentContainer);
    if (refresher) {
      
      
      
      
      
      refresher->SetupRefreshURIFromHeader(mDocumentURI, NodePrincipal(),
                                           NS_ConvertUTF16toUTF8(aData));
    }
  }

  if (aHeaderField == nsGkAtoms::headerDNSPrefetchControl &&
      mAllowDNSPrefetch) {
    
    mAllowDNSPrefetch = aData.IsEmpty() || aData.LowerCaseEqualsLiteral("on");
  }

  if (aHeaderField == nsGkAtoms::viewport ||
      aHeaderField == nsGkAtoms::handheldFriendly ||
      aHeaderField == nsGkAtoms::viewport_minimum_scale ||
      aHeaderField == nsGkAtoms::viewport_maximum_scale ||
      aHeaderField == nsGkAtoms::viewport_initial_scale ||
      aHeaderField == nsGkAtoms::viewport_height ||
      aHeaderField == nsGkAtoms::viewport_width ||
      aHeaderField ==  nsGkAtoms::viewport_user_scalable) {
    mViewportType = Unknown;
  }

  
  if (aHeaderField == nsGkAtoms::referrer && !aData.IsEmpty()) {
    ReferrerPolicy policy = mozilla::net::ReferrerPolicyFromString(aData);

    
    
    if (!mReferrerPolicySet) {
      mReferrerPolicy = policy;
      mReferrerPolicySet = true;
    } else if (mReferrerPolicy != policy) {
      mReferrerPolicy = mozilla::net::RP_No_Referrer;
    }
  }
}

void
nsDocument::TryChannelCharset(nsIChannel *aChannel,
                              int32_t& aCharsetSource,
                              nsACString& aCharset,
                              nsHtml5TreeOpExecutor* aExecutor)
{
  if (aChannel) {
    nsAutoCString charsetVal;
    nsresult rv = aChannel->GetContentCharset(charsetVal);
    if (NS_SUCCEEDED(rv)) {
      nsAutoCString preferred;
      if(EncodingUtils::FindEncodingForLabel(charsetVal, preferred)) {
        aCharset = preferred;
        aCharsetSource = kCharsetFromChannel;
        return;
      } else if (aExecutor && !charsetVal.IsEmpty()) {
        aExecutor->ComplainAboutBogusProtocolCharset(this);
      }
    }
  }
}

already_AddRefed<nsIPresShell>
nsDocument::CreateShell(nsPresContext* aContext, nsViewManager* aViewManager,
                        nsStyleSet* aStyleSet)
{
  
  
  
  return doCreateShell(aContext, aViewManager, aStyleSet,
                       eCompatibility_FullStandards);
}

already_AddRefed<nsIPresShell>
nsDocument::doCreateShell(nsPresContext* aContext,
                          nsViewManager* aViewManager, nsStyleSet* aStyleSet,
                          nsCompatibility aCompatMode)
{
  NS_ASSERTION(!mPresShell, "We have a presshell already!");

  NS_ENSURE_FALSE(GetBFCacheEntry(), nullptr);

  FillStyleSet(aStyleSet);

  nsRefPtr<PresShell> shell = new PresShell;
  shell->Init(this, aContext, aViewManager, aStyleSet, aCompatMode);

  
  mPresShell = shell;

  
  nsCOMPtr<nsIDocShell> docShell(mDocumentContainer);
  if (docShell && docShell->IsInvisible())
    shell->SetNeverPainting(true);

  mExternalResourceMap.ShowViewers();

  MaybeRescheduleAnimationFrameNotifications();

  return shell.forget();
}

void
nsDocument::MaybeRescheduleAnimationFrameNotifications()
{
  if (!mPresShell || !IsEventHandlingEnabled()) {
    
    return;
  }

  nsRefreshDriver* rd = mPresShell->GetPresContext()->RefreshDriver();
  if (!mFrameRequestCallbacks.IsEmpty()) {
    rd->ScheduleFrameRequestCallbacks(this);
  }
}

void
nsIDocument::TakeFrameRequestCallbacks(FrameRequestCallbackList& aCallbacks)
{
  aCallbacks.AppendElements(mFrameRequestCallbacks);
  mFrameRequestCallbacks.Clear();
}

bool
nsIDocument::ShouldThrottleFrameRequests()
{
  if (!mIsShowing) {
    
    return true;
  }

  if (!mPresShell) {
    return false;  
  }

  nsIFrame* frame = mPresShell->GetRootFrame();
  if (!frame) {
    return false;  
  }

  nsIFrame* displayRootFrame = nsLayoutUtils::GetDisplayRootFrame(frame);
  if (!displayRootFrame) {
    return false;  
  }

  if (!displayRootFrame->DidPaintPresShell(mPresShell)) {
    
    
    
    
    
    
    return true;
  }

  
  return false;
}

PLDHashOperator RequestDiscardEnumerator(imgIRequest* aKey,
                                         uint32_t aData,
                                         void* userArg)
{
  aKey->RequestDiscard();
  return PL_DHASH_NEXT;
}

void
nsDocument::DeleteShell()
{
  mExternalResourceMap.HideViewers();
  if (IsEventHandlingEnabled()) {
    RevokeAnimationFrameNotifications();
  }

  
  
  
  mImageTracker.EnumerateRead(RequestDiscardEnumerator, nullptr);

  mPresShell = nullptr;
}

void
nsDocument::RevokeAnimationFrameNotifications()
{
  if (!mFrameRequestCallbacks.IsEmpty()) {
    mPresShell->GetPresContext()->RefreshDriver()->
      RevokeFrameRequestCallbacks(this);
  }
}

static void
SubDocClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  SubDocMapEntry *e = static_cast<SubDocMapEntry *>(entry);

  NS_RELEASE(e->mKey);
  if (e->mSubDocument) {
    e->mSubDocument->SetParentDocument(nullptr);
    NS_RELEASE(e->mSubDocument);
  }
}

static void
SubDocInitEntry(PLDHashEntryHdr *entry, const void *key)
{
  SubDocMapEntry *e =
    const_cast<SubDocMapEntry *>
              (static_cast<const SubDocMapEntry *>(entry));

  e->mKey = const_cast<Element*>(static_cast<const Element*>(key));
  NS_ADDREF(e->mKey);

  e->mSubDocument = nullptr;
}

nsresult
nsDocument::SetSubDocumentFor(Element* aElement, nsIDocument* aSubDoc)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_UNEXPECTED);

  if (!aSubDoc) {
    

    if (mSubDocuments) {
      SubDocMapEntry *entry =
        static_cast<SubDocMapEntry*>
                   (PL_DHashTableSearch(mSubDocuments, aElement));

      if (entry) {
        PL_DHashTableRawRemove(mSubDocuments, entry);
      }
    }
  } else {
    if (!mSubDocuments) {
      

      static const PLDHashTableOps hash_table_ops =
      {
        PL_DHashVoidPtrKeyStub,
        PL_DHashMatchEntryStub,
        PL_DHashMoveEntryStub,
        SubDocClearEntry,
        SubDocInitEntry
      };

      mSubDocuments = PL_NewDHashTable(&hash_table_ops, sizeof(SubDocMapEntry));
    }

    
    SubDocMapEntry *entry = static_cast<SubDocMapEntry*>
      (PL_DHashTableAdd(mSubDocuments, aElement, fallible));

    if (!entry) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (entry->mSubDocument) {
      entry->mSubDocument->SetParentDocument(nullptr);

      
      NS_RELEASE(entry->mSubDocument);
    }

    entry->mSubDocument = aSubDoc;
    NS_ADDREF(entry->mSubDocument);

    aSubDoc->SetParentDocument(this);
  }

  return NS_OK;
}

nsIDocument*
nsDocument::GetSubDocumentFor(nsIContent *aContent) const
{
  if (mSubDocuments && aContent->IsElement()) {
    SubDocMapEntry *entry =
      static_cast<SubDocMapEntry*>
                 (PL_DHashTableSearch(mSubDocuments, aContent->AsElement()));

    if (entry) {
      return entry->mSubDocument;
    }
  }

  return nullptr;
}

static PLDHashOperator
FindContentEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                      uint32_t number, void *arg)
{
  SubDocMapEntry *entry = static_cast<SubDocMapEntry*>(hdr);
  FindContentData *data = static_cast<FindContentData*>(arg);

  if (entry->mSubDocument == data->mSubDocument) {
    data->mResult = entry->mKey;

    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

Element*
nsDocument::FindContentForSubDocument(nsIDocument *aDocument) const
{
  NS_ENSURE_TRUE(aDocument, nullptr);

  if (!mSubDocuments) {
    return nullptr;
  }

  FindContentData data(aDocument);
  PL_DHashTableEnumerate(mSubDocuments, FindContentEnumerator, &data);

  return data.mResult;
}

bool
nsDocument::IsNodeOfType(uint32_t aFlags) const
{
    return !(aFlags & ~eDOCUMENT);
}

Element*
nsIDocument::GetRootElement() const
{
  return (mCachedRootElement && mCachedRootElement->GetParentNode() == this) ?
         mCachedRootElement : GetRootElementInternal();
}

Element*
nsDocument::GetRootElementInternal() const
{
  
  
  uint32_t i;
  for (i = mChildren.ChildCount(); i > 0; --i) {
    nsIContent* child = mChildren.ChildAt(i - 1);
    if (child->IsElement()) {
      const_cast<nsDocument*>(this)->mCachedRootElement = child->AsElement();
      return child->AsElement();
    }
  }

  const_cast<nsDocument*>(this)->mCachedRootElement = nullptr;
  return nullptr;
}

nsIContent *
nsDocument::GetChildAt(uint32_t aIndex) const
{
  return mChildren.GetSafeChildAt(aIndex);
}

int32_t
nsDocument::IndexOf(const nsINode* aPossibleChild) const
{
  return mChildren.IndexOfChild(aPossibleChild);
}

uint32_t
nsDocument::GetChildCount() const
{
  return mChildren.ChildCount();
}

nsIContent * const *
nsDocument::GetChildArray(uint32_t* aChildCount) const
{
  return mChildren.GetChildArray(aChildCount);
}


nsresult
nsDocument::InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                          bool aNotify)
{
  if (aKid->IsElement() && GetRootElement()) {
    NS_WARNING("Inserting root element when we already have one");
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  return doInsertChildAt(aKid, aIndex, aNotify, mChildren);
}

void
nsDocument::RemoveChildAt(uint32_t aIndex, bool aNotify)
{
  nsCOMPtr<nsIContent> oldKid = GetChildAt(aIndex);
  if (!oldKid) {
    return;
  }

  if (oldKid->IsElement()) {
    
    DestroyElementMaps();
  }

  doRemoveChildAt(aIndex, aNotify, oldKid, mChildren);
  mCachedRootElement = nullptr;
}

void
nsDocument::EnsureOnDemandBuiltInUASheet(CSSStyleSheet* aSheet)
{
  
  if (mOnDemandBuiltInUASheets.Contains(static_cast<nsIStyleSheet*>(aSheet))) {
    return;
  }
  BeginUpdate(UPDATE_STYLE);
  AddOnDemandBuiltInUASheet(aSheet);
  EndUpdate(UPDATE_STYLE);
}

void
nsDocument::AddOnDemandBuiltInUASheet(CSSStyleSheet* aSheet)
{
  
  MOZ_ASSERT(!mOnDemandBuiltInUASheets.Contains(static_cast<nsIStyleSheet*>(aSheet)));

  
  
  mOnDemandBuiltInUASheets.InsertElementAt(0, aSheet);

  if (aSheet->IsApplicable()) {
    
    nsCOMPtr<nsIPresShell> shell = GetShell();
    if (shell) {
      
      
      
      
      shell->StyleSet()->PrependStyleSheet(nsStyleSet::eAgentSheet, aSheet);
    }
  }

  NotifyStyleSheetAdded(aSheet, false);
}

int32_t
nsDocument::GetNumberOfStyleSheets() const
{
  return mStyleSheets.Count();
}

nsIStyleSheet*
nsDocument::GetStyleSheetAt(int32_t aIndex) const
{
  NS_ENSURE_TRUE(0 <= aIndex && aIndex < mStyleSheets.Count(), nullptr);
  return mStyleSheets[aIndex];
}

int32_t
nsDocument::GetIndexOfStyleSheet(nsIStyleSheet* aSheet) const
{
  return mStyleSheets.IndexOf(aSheet);
}

void
nsDocument::AddStyleSheetToStyleSets(nsIStyleSheet* aSheet)
{
  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    shell->StyleSet()->AddDocStyleSheet(aSheet, this);
  }
}

#define DO_STYLESHEET_NOTIFICATION(className, type, memberName, argName)      \
  do {                                                                        \
    nsRefPtr<CSSStyleSheet> cssSheet = do_QueryObject(aSheet);                \
    if (!cssSheet) {                                                          \
      return;                                                                 \
    }                                                                         \
                                                                              \
    className##Init init;                                                     \
    init.mBubbles = true;                                                     \
    init.mCancelable = true;                                                  \
    init.mStylesheet = cssSheet;                                              \
    init.memberName = argName;                                                \
                                                                              \
    nsRefPtr<className> event =                                               \
      className::Constructor(this, NS_LITERAL_STRING(type), init);            \
    event->SetTrusted(true);                                                  \
    event->SetTarget(this);                                                   \
    nsRefPtr<AsyncEventDispatcher> asyncDispatcher =                          \
      new AsyncEventDispatcher(this, event);                                  \
    asyncDispatcher->mDispatchChromeOnly = true;                              \
    asyncDispatcher->PostDOMEvent();                                          \
  } while (0);

void
nsDocument::NotifyStyleSheetAdded(nsIStyleSheet* aSheet, bool aDocumentSheet)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetAdded, (this, aSheet, aDocumentSheet));

  if (StyleSheetChangeEventsEnabled()) {
    DO_STYLESHEET_NOTIFICATION(StyleSheetChangeEvent,
                               "StyleSheetAdded",
                               mDocumentSheet,
                               aDocumentSheet);
  }
}

void
nsDocument::NotifyStyleSheetRemoved(nsIStyleSheet* aSheet, bool aDocumentSheet)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetRemoved, (this, aSheet, aDocumentSheet));

  if (StyleSheetChangeEventsEnabled()) {
    DO_STYLESHEET_NOTIFICATION(StyleSheetChangeEvent,
                               "StyleSheetRemoved",
                               mDocumentSheet,
                               aDocumentSheet);
  }
}

void
nsDocument::AddStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  mStyleSheets.AppendObject(aSheet);
  aSheet->SetOwningDocument(this);

  if (aSheet->IsApplicable()) {
    AddStyleSheetToStyleSets(aSheet);
  }

  NotifyStyleSheetAdded(aSheet, true);
}

void
nsDocument::RemoveStyleSheetFromStyleSets(nsIStyleSheet* aSheet)
{
  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    shell->StyleSet()->RemoveDocStyleSheet(aSheet);
  }
}

void
nsDocument::RemoveStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  nsCOMPtr<nsIStyleSheet> sheet = aSheet; 

  if (!mStyleSheets.RemoveObject(aSheet)) {
    NS_ASSERTION(mInUnlinkOrDeletion, "stylesheet not found");
    return;
  }

  if (!mIsGoingAway) {
    if (aSheet->IsApplicable()) {
      RemoveStyleSheetFromStyleSets(aSheet);
    }

    NotifyStyleSheetRemoved(aSheet, true);
  }

  aSheet->SetOwningDocument(nullptr);
}

void
nsDocument::UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                              nsCOMArray<nsIStyleSheet>& aNewSheets)
{
  BeginUpdate(UPDATE_STYLE);

  
  NS_PRECONDITION(aOldSheets.Count() == aNewSheets.Count(),
                  "The lists must be the same length!");
  int32_t count = aOldSheets.Count();

  nsCOMPtr<nsIStyleSheet> oldSheet;
  int32_t i;
  for (i = 0; i < count; ++i) {
    oldSheet = aOldSheets[i];

    
    NS_ASSERTION(oldSheet, "None of the old sheets should be null");
    int32_t oldIndex = mStyleSheets.IndexOf(oldSheet);
    RemoveStyleSheet(oldSheet);  

    
    nsIStyleSheet* newSheet = aNewSheets[i];
    if (newSheet) {
      mStyleSheets.InsertObjectAt(newSheet, oldIndex);
      newSheet->SetOwningDocument(this);
      if (newSheet->IsApplicable()) {
        AddStyleSheetToStyleSets(newSheet);
      }

      NotifyStyleSheetAdded(newSheet, true);
    }
  }

  EndUpdate(UPDATE_STYLE);
}

void
nsDocument::InsertStyleSheetAt(nsIStyleSheet* aSheet, int32_t aIndex)
{
  NS_PRECONDITION(aSheet, "null ptr");
  mStyleSheets.InsertObjectAt(aSheet, aIndex);

  aSheet->SetOwningDocument(this);

  if (aSheet->IsApplicable()) {
    AddStyleSheetToStyleSets(aSheet);
  }

  NotifyStyleSheetAdded(aSheet, true);
}


void
nsDocument::SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                         bool aApplicable)
{
  NS_PRECONDITION(aSheet, "null arg");

  
  if (-1 != mStyleSheets.IndexOf(aSheet)) {
    if (aApplicable) {
      AddStyleSheetToStyleSets(aSheet);
    } else {
      RemoveStyleSheetFromStyleSets(aSheet);
    }
  }

  
  
  

  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetApplicableStateChanged,
                               (this, aSheet, aApplicable));

  if (StyleSheetChangeEventsEnabled()) {
    DO_STYLESHEET_NOTIFICATION(StyleSheetApplicableStateChangeEvent,
                               "StyleSheetApplicableStateChanged",
                               mApplicable,
                               aApplicable);
  }

  if (!mSSApplicableStateNotificationPending) {
    nsCOMPtr<nsIRunnable> notification = NS_NewRunnableMethod(this,
      &nsDocument::NotifyStyleSheetApplicableStateChanged);
    mSSApplicableStateNotificationPending =
      NS_SUCCEEDED(NS_DispatchToCurrentThread(notification));
  }
}

void
nsDocument::NotifyStyleSheetApplicableStateChanged()
{
  mSSApplicableStateNotificationPending = false;
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->NotifyObservers(static_cast<nsIDocument*>(this),
                                     "style-sheet-applicable-state-changed",
                                     nullptr);
  }
}

static nsStyleSet::sheetType
ConvertAdditionalSheetType(nsIDocument::additionalSheetType aType)
{
  switch(aType) {
    case nsIDocument::eAgentSheet:
      return nsStyleSet::eAgentSheet;
    case nsIDocument::eUserSheet:
      return nsStyleSet::eUserSheet;
    case nsIDocument::eAuthorSheet:
      return nsStyleSet::eDocSheet;
    default:
      NS_ASSERTION(false, "wrong type");
      
      return nsStyleSet::eSheetTypeCount;
  }
}

static int32_t
FindSheet(const nsCOMArray<nsIStyleSheet>& aSheets, nsIURI* aSheetURI)
{
  for (int32_t i = aSheets.Count() - 1; i >= 0; i-- ) {
    bool bEqual;
    nsIURI* uri = aSheets[i]->GetSheetURI();

    if (uri && NS_SUCCEEDED(uri->Equals(aSheetURI, &bEqual)) && bEqual)
      return i;
  }

  return -1;
}

nsresult
nsDocument::LoadAdditionalStyleSheet(additionalSheetType aType, nsIURI* aSheetURI)
{
  NS_PRECONDITION(aSheetURI, "null arg");

  
  if (FindSheet(mAdditionalSheets[aType], aSheetURI) >= 0)
    return NS_ERROR_INVALID_ARG;

  
  nsRefPtr<mozilla::css::Loader> loader = new mozilla::css::Loader();

  nsRefPtr<CSSStyleSheet> sheet;
  nsresult rv = loader->LoadSheetSync(aSheetURI, aType == eAgentSheet,
    true, getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);

  sheet->SetOwningDocument(this);
  MOZ_ASSERT(sheet->IsApplicable());

  return AddAdditionalStyleSheet(aType, sheet);
}

nsresult
nsDocument::AddAdditionalStyleSheet(additionalSheetType aType, nsIStyleSheet* aSheet)
{
  if (mAdditionalSheets[aType].Contains(aSheet))
    return NS_ERROR_INVALID_ARG;

  if (!aSheet->IsApplicable())
    return NS_ERROR_INVALID_ARG;

  mAdditionalSheets[aType].AppendObject(aSheet);

  BeginUpdate(UPDATE_STYLE);
  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    nsStyleSet::sheetType type = ConvertAdditionalSheetType(aType);
    shell->StyleSet()->AppendStyleSheet(type, aSheet);
  }

  
  
  NotifyStyleSheetAdded(aSheet, false);
  EndUpdate(UPDATE_STYLE);
  return NS_OK;
}

void
nsDocument::RemoveAdditionalStyleSheet(additionalSheetType aType, nsIURI* aSheetURI)
{
  MOZ_ASSERT(aSheetURI);

  nsCOMArray<nsIStyleSheet>& sheets = mAdditionalSheets[aType];

  int32_t i = FindSheet(mAdditionalSheets[aType], aSheetURI);
  if (i >= 0) {
    nsCOMPtr<nsIStyleSheet> sheetRef = sheets[i];
    sheets.RemoveObjectAt(i);

    BeginUpdate(UPDATE_STYLE);
    if (!mIsGoingAway) {
      MOZ_ASSERT(sheetRef->IsApplicable());
      nsCOMPtr<nsIPresShell> shell = GetShell();
      if (shell) {
        nsStyleSet::sheetType type = ConvertAdditionalSheetType(aType);
        shell->StyleSet()->RemoveStyleSheet(type, sheetRef);
      }
    }

    
    
    NotifyStyleSheetRemoved(sheetRef, false);
    EndUpdate(UPDATE_STYLE);

    sheetRef->SetOwningDocument(nullptr);
  }
}

nsIStyleSheet*
nsDocument::FirstAdditionalAuthorSheet()
{
  return mAdditionalSheets[eAuthorSheet].SafeObjectAt(0);
}

nsIGlobalObject*
nsDocument::GetScopeObject() const
{
  nsCOMPtr<nsIGlobalObject> scope(do_QueryReferent(mScopeObject));
  return scope;
}

void
nsDocument::SetScopeObject(nsIGlobalObject* aGlobal)
{
  mScopeObject = do_GetWeakReference(aGlobal);
  if (aGlobal) {
    mHasHadScriptHandlingObject = true;
  }
}

#ifdef MOZ_EME
static void
CheckIfContainsEMEContent(nsISupports* aSupports, void* aContainsEME)
{
  nsCOMPtr<nsIDOMHTMLMediaElement> domMediaElem(do_QueryInterface(aSupports));
  if (domMediaElem) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(domMediaElem));
    MOZ_ASSERT(content, "aSupports is not a content");
    HTMLMediaElement* mediaElem = static_cast<HTMLMediaElement*>(content.get());
    bool* contains = static_cast<bool*>(aContainsEME);
    if (mediaElem->GetMediaKeys()) {
      *contains = true;
    }
  }
}

bool
nsDocument::ContainsEMEContent()
{
  bool containsEME = false;
  EnumerateActivityObservers(CheckIfContainsEMEContent,
                             static_cast<void*>(&containsEME));
  return containsEME;
}
#endif 

static void
CheckIfContainsMSEContent(nsISupports* aSupports, void* aContainsMSE)
{
  nsCOMPtr<nsIDOMHTMLMediaElement> domMediaElem(do_QueryInterface(aSupports));
  if (domMediaElem) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(domMediaElem));
    MOZ_ASSERT(content, "aSupports is not a content");
    HTMLMediaElement* mediaElem = static_cast<HTMLMediaElement*>(content.get());
    bool* contains = static_cast<bool*>(aContainsMSE);
    nsRefPtr<MediaSource> ms = mediaElem->GetMozMediaSourceObject();
    if (ms) {
      *contains = true;
    }
  }
}

bool
nsDocument::ContainsMSEContent()
{
  bool containsMSE = false;
  EnumerateActivityObservers(CheckIfContainsMSEContent,
                             static_cast<void*>(&containsMSE));
  return containsMSE;
}

static void
NotifyActivityChanged(nsISupports *aSupports, void *aUnused)
{
  nsCOMPtr<nsIDOMHTMLMediaElement> domMediaElem(do_QueryInterface(aSupports));
  if (domMediaElem) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(domMediaElem));
    MOZ_ASSERT(content, "aSupports is not a content");
    HTMLMediaElement* mediaElem = static_cast<HTMLMediaElement*>(content.get());
    mediaElem->NotifyOwnerDocumentActivityChanged();
  }
  nsCOMPtr<nsIObjectLoadingContent> objectLoadingContent(do_QueryInterface(aSupports));
  if (objectLoadingContent) {
    nsObjectLoadingContent* olc = static_cast<nsObjectLoadingContent*>(objectLoadingContent.get());
    olc->NotifyOwnerDocumentActivityChanged();
  }
  nsCOMPtr<nsIDocumentActivity> objectDocumentActivity(do_QueryInterface(aSupports));
  if (objectDocumentActivity) {
    objectDocumentActivity->NotifyOwnerDocumentActivityChanged();
  }
}

void
nsIDocument::SetContainer(nsDocShell* aContainer)
{
  if (aContainer) {
    mDocumentContainer = aContainer;
  } else {
    mDocumentContainer = WeakPtr<nsDocShell>();
  }

  EnumerateActivityObservers(NotifyActivityChanged, nullptr);
  if (!aContainer) {
    return;
  }

  
  if (aContainer->ItemType() == nsIDocShellTreeItem::typeContent) {
    
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    aContainer->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    NS_ASSERTION(sameTypeRoot, "No document shell root tree item from document shell tree item!");

    if (sameTypeRoot == aContainer) {
      static_cast<nsDocument*>(this)->SetIsTopLevelContentDocument(true);
    }
  }
}

nsISupports*
nsIDocument::GetContainer() const
{
  return static_cast<nsIDocShell*>(mDocumentContainer);
}

void
nsDocument::SetScriptGlobalObject(nsIScriptGlobalObject *aScriptGlobalObject)
{
#ifdef DEBUG
  {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(aScriptGlobalObject));

    NS_ASSERTION(!win || win->IsInnerWindow(),
                 "Script global object must be an inner window!");
  }
#endif
  MOZ_ASSERT(aScriptGlobalObject || !mAnimationController ||
             mAnimationController->IsPausedByType(
               nsSMILTimeContainer::PAUSE_PAGEHIDE |
               nsSMILTimeContainer::PAUSE_BEGIN),
             "Clearing window pointer while animations are unpaused");

  if (mScriptGlobalObject && !aScriptGlobalObject) {
    
    
    mLayoutHistoryState = GetLayoutHistoryState();

    if (mPresShell && !EventHandlingSuppressed()) {
      RevokeAnimationFrameNotifications();
    }

    
    if (mOnloadBlockCount != 0) {
      nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
      if (loadGroup) {
        loadGroup->RemoveRequest(mOnloadBlocker, nullptr, NS_OK);
      }
    }
  }

  mScriptGlobalObject = aScriptGlobalObject;

  if (aScriptGlobalObject) {
    mHasHadScriptHandlingObject = true;
    mHasHadDefaultView = true;
    
    mLayoutHistoryState = nullptr;
    mScopeObject = do_GetWeakReference(aScriptGlobalObject);
#ifdef DEBUG
    if (!mWillReparent) {
      
      
      JSObject *obj = GetWrapperPreserveColor();
      if (obj) {
        JSObject *newScope = aScriptGlobalObject->GetGlobalJSObject();
        NS_ASSERTION(js::GetGlobalForObjectCrossCompartment(obj) == newScope,
                     "Wrong scope, this is really bad!");
      }
    }
#endif

    if (mAllowDNSPrefetch) {
      nsCOMPtr<nsIDocShell> docShell(mDocumentContainer);
      if (docShell) {
#ifdef DEBUG
        nsCOMPtr<nsIWebNavigation> webNav =
          do_GetInterface(aScriptGlobalObject);
        NS_ASSERTION(SameCOMIdentity(webNav, docShell),
                     "Unexpected container or script global?");
#endif
        bool allowDNSPrefetch;
        docShell->GetAllowDNSPrefetch(&allowDNSPrefetch);
        mAllowDNSPrefetch = allowDNSPrefetch;
      }
    }

    MaybeRescheduleAnimationFrameNotifications();
    mRegistry = new Registry();
  }

  
  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mScriptGlobalObject);
  mWindow = window;

  
  
  
  FlushCSPWebConsoleErrorQueue();
  nsCOMPtr<nsIHttpChannelInternal> internalChannel =
    do_QueryInterface(GetChannel());
  if (internalChannel) {
    nsCOMArray<nsISecurityConsoleMessage> messages;
    internalChannel->TakeAllSecurityMessages(messages);
    SendToConsole(messages);
  }

  
  
  
  
  
  dom::VisibilityState oldState = mVisibilityState;
  mVisibilityState = GetVisibilityState();
  
  
  
  
  
  
  
  
  
  
  
  
  if (oldState != mVisibilityState) {
    EnumerateActivityObservers(NotifyActivityChanged, nullptr);
  }

  
  if (mTemplateContentsOwner && mTemplateContentsOwner != this) {
    mTemplateContentsOwner->SetScriptGlobalObject(aScriptGlobalObject);
  }

  nsCOMPtr<nsIChannel> channel = GetChannel();
  if (!mMaybeServiceWorkerControlled && channel) {
    nsLoadFlags loadFlags = 0;
    channel->GetLoadFlags(&loadFlags);
    
    if (loadFlags & nsIRequest::LOAD_BYPASS_CACHE) {
      NS_WARNING("Page was shift reloaded, skipping ServiceWorker control");
      return;
    }

    nsCOMPtr<nsIServiceWorkerManager> swm = mozilla::services::GetServiceWorkerManager();
    if (swm) {
      swm->MaybeStartControlling(this);
      mMaybeServiceWorkerControlled = true;
    }
  }
}

nsIScriptGlobalObject*
nsDocument::GetScriptHandlingObjectInternal() const
{
  MOZ_ASSERT(!mScriptGlobalObject,
             "Do not call this when mScriptGlobalObject is set!");
  if (mHasHadDefaultView) {
    return nullptr;
  }

  nsCOMPtr<nsIScriptGlobalObject> scriptHandlingObject =
    do_QueryReferent(mScopeObject);
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(scriptHandlingObject);
  if (win) {
    NS_ASSERTION(win->IsInnerWindow(), "Should have inner window here!");
    nsPIDOMWindow* outer = win->GetOuterWindow();
    if (!outer || outer->GetCurrentInnerWindow() != win) {
      NS_WARNING("Wrong inner/outer window combination!");
      return nullptr;
    }
  }
  return scriptHandlingObject;
}
void
nsDocument::SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject)
{
  NS_ASSERTION(!mScriptGlobalObject ||
               mScriptGlobalObject == aScriptObject,
               "Wrong script object!");
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aScriptObject);
  NS_ASSERTION(!win || win->IsInnerWindow(), "Should have inner window here!");
  if (aScriptObject) {
    mScopeObject = do_GetWeakReference(aScriptObject);
    mHasHadScriptHandlingObject = true;
    mHasHadDefaultView = false;
  }
}

bool
nsDocument::IsTopLevelContentDocument()
{
  return mIsTopLevelContentDocument;
}

void
nsDocument::SetIsTopLevelContentDocument(bool aIsTopLevelContentDocument)
{
  mIsTopLevelContentDocument = aIsTopLevelContentDocument;
}

nsPIDOMWindow *
nsDocument::GetWindowInternal() const
{
  MOZ_ASSERT(!mWindow, "This should not be called when mWindow is not null!");
  
  
  nsCOMPtr<nsPIDOMWindow> win;
  if (mRemovedFromDocShell) {
    
    nsCOMPtr<nsIDocShell> kungfuDeathGrip(mDocumentContainer);
    if (mDocumentContainer) {
      win = mDocumentContainer->GetWindow();
    }
  } else {
    win = do_QueryInterface(mScriptGlobalObject);
    if (win) {
      
      win = win->GetOuterWindow();
    }
  }

  return win;
}

nsScriptLoader*
nsDocument::ScriptLoader()
{
  return mScriptLoader;
}

bool
nsDocument::InternalAllowXULXBL()
{
  if (nsContentUtils::AllowXULXBLForPrincipal(NodePrincipal())) {
    mAllowXULXBL = eTriTrue;
    return true;
  }

  mAllowXULXBL = eTriFalse;
  return false;
}



void
nsDocument::AddObserver(nsIDocumentObserver* aObserver)
{
  NS_ASSERTION(mObservers.IndexOf(aObserver) == nsTArray<int>::NoIndex,
               "Observer already in the list");
  mObservers.AppendElement(aObserver);
  AddMutationObserver(aObserver);
}

bool
nsDocument::RemoveObserver(nsIDocumentObserver* aObserver)
{
  
  
  
  
  if (!mInDestructor) {
    RemoveMutationObserver(aObserver);
    return mObservers.RemoveElement(aObserver);
  }

  return mObservers.Contains(aObserver);
}

void
nsDocument::MaybeEndOutermostXBLUpdate()
{
  
  
  if (mUpdateNestLevel == 0 && mInXBLUpdate) {
    if (nsContentUtils::IsSafeToRunScript()) {
      mInXBLUpdate = false;
      BindingManager()->EndOutermostUpdate();
    } else if (!mInDestructor) {
      nsContentUtils::AddScriptRunner(
        NS_NewRunnableMethod(this, &nsDocument::MaybeEndOutermostXBLUpdate));
    }
  }
}

void
nsDocument::BeginUpdate(nsUpdateType aUpdateType)
{
  if (mUpdateNestLevel == 0 && !mInXBLUpdate) {
    mInXBLUpdate = true;
    BindingManager()->BeginOutermostUpdate();
  }

  ++mUpdateNestLevel;
  nsContentUtils::AddScriptBlocker();
  NS_DOCUMENT_NOTIFY_OBSERVERS(BeginUpdate, (this, aUpdateType));
}

void
nsDocument::EndUpdate(nsUpdateType aUpdateType)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(EndUpdate, (this, aUpdateType));

  nsContentUtils::RemoveScriptBlocker();

  --mUpdateNestLevel;

  
  
  MaybeEndOutermostXBLUpdate();

  MaybeInitializeFinalizeFrameLoaders();
}

void
nsDocument::BeginLoad()
{
  
  
  BlockOnload();
  mDidFireDOMContentLoaded = false;
  BlockDOMContentLoaded();

  if (mScriptLoader) {
    mScriptLoader->BeginDeferringScripts();
  }

  NS_DOCUMENT_NOTIFY_OBSERVERS(BeginLoad, (this));
}

void
nsDocument::ReportEmptyGetElementByIdArg()
{
  nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                  NS_LITERAL_CSTRING("DOM"), this,
                                  nsContentUtils::eDOM_PROPERTIES,
                                  "EmptyGetElementByIdParam");
}

Element*
nsDocument::GetElementById(const nsAString& aElementId)
{
  if (!CheckGetElementByIdArg(aElementId)) {
    return nullptr;
  }

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aElementId);
  return entry ? entry->GetIdElement() : nullptr;
}

const nsSmallVoidArray*
nsDocument::GetAllElementsForId(const nsAString& aElementId) const
{
  if (aElementId.IsEmpty()) {
    return nullptr;
  }

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aElementId);
  return entry ? entry->GetIdElements() : nullptr;
}

NS_IMETHODIMP
nsDocument::GetElementById(const nsAString& aId, nsIDOMElement** aReturn)
{
  Element *content = GetElementById(aId);
  if (content) {
    return CallQueryInterface(content, aReturn);
  }

  *aReturn = nullptr;

  return NS_OK;
}

Element*
nsDocument::AddIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                void* aData, bool aForImage)
{
  nsDependentAtomString id(aID);

  if (!CheckGetElementByIdArg(id))
    return nullptr;

  nsIdentifierMapEntry *entry = mIdentifierMap.PutEntry(id);
  NS_ENSURE_TRUE(entry, nullptr);

  entry->AddContentChangeCallback(aObserver, aData, aForImage);
  return aForImage ? entry->GetImageIdElement() : entry->GetIdElement();
}

void
nsDocument::RemoveIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                   void* aData, bool aForImage)
{
  nsDependentAtomString id(aID);

  if (!CheckGetElementByIdArg(id))
    return;

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(id);
  if (!entry) {
    return;
  }

  entry->RemoveContentChangeCallback(aObserver, aData, aForImage);
}

NS_IMETHODIMP
nsDocument::MozSetImageElement(const nsAString& aImageElementId,
                               nsIDOMElement* aImageElement)
{
  nsCOMPtr<Element> el = do_QueryInterface(aImageElement);
  MozSetImageElement(aImageElementId, el);
  return NS_OK;
}

void
nsDocument::MozSetImageElement(const nsAString& aImageElementId,
                               Element* aElement)
{
  if (aImageElementId.IsEmpty())
    return;

  
  
  nsAutoScriptBlocker scriptBlocker;

  nsIdentifierMapEntry *entry = mIdentifierMap.PutEntry(aImageElementId);
  if (entry) {
    entry->SetImageElement(aElement);
    if (entry->IsEmpty()) {
      mIdentifierMap.RemoveEntry(aImageElementId);
    }
  }
}

Element*
nsDocument::LookupImageElement(const nsAString& aId)
{
  if (aId.IsEmpty())
    return nullptr;

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aId);
  return entry ? entry->GetImageIdElement() : nullptr;
}

void
nsDocument::DispatchContentLoadedEvents()
{
  
  

  
  mPreloadingImages.Clear();

  if (mTiming) {
    mTiming->NotifyDOMContentLoadedStart(nsIDocument::GetDocumentURI());
  }

  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  nsIPrincipal *principal = GetPrincipal();
  os->NotifyObservers(static_cast<nsIDocument*>(this),
                      nsContentUtils::IsSystemPrincipal(principal) ?
                        "chrome-document-interactive" :
                        "content-document-interactive",
                      nullptr);

  
  
  
  nsContentUtils::DispatchTrustedEvent(this, static_cast<nsIDocument*>(this),
                                       NS_LITERAL_STRING("DOMContentLoaded"),
                                       true, false);

  if (mTiming) {
    mTiming->NotifyDOMContentLoadedEnd(nsIDocument::GetDocumentURI());
  }

  
  
  
  

  
  
  
  nsCOMPtr<EventTarget> target_frame;

  if (mParentDocument) {
    target_frame = mParentDocument->FindContentForSubDocument(this);
  }

  if (target_frame) {
    nsCOMPtr<nsIDocument> parent = mParentDocument;
    do {
      nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(parent);

      nsCOMPtr<nsIDOMEvent> event;
      if (domDoc) {
        domDoc->CreateEvent(NS_LITERAL_STRING("Events"),
                            getter_AddRefs(event));

      }

      if (event) {
        event->InitEvent(NS_LITERAL_STRING("DOMFrameContentLoaded"), true,
                         true);

        event->SetTarget(target_frame);
        event->SetTrusted(true);

        
        
        
        
        

        WidgetEvent* innerEvent = event->GetInternalNSEvent();
        if (innerEvent) {
          nsEventStatus status = nsEventStatus_eIgnore;

          nsIPresShell *shell = parent->GetShell();
          if (shell) {
            nsRefPtr<nsPresContext> context = shell->GetPresContext();

            if (context) {
              EventDispatcher::Dispatch(parent, context, innerEvent, event,
                                        &status);
            }
          }
        }
      }

      parent = parent->GetParentDocument();
    } while (parent);
  }

  
  
  Element* root = GetRootElement();
  if (root && root->HasAttr(kNameSpaceID_None, nsGkAtoms::manifest)) {
    nsContentUtils::DispatchChromeEvent(this, static_cast<nsIDocument*>(this),
                                        NS_LITERAL_STRING("MozApplicationManifest"),
                                        true, true);
  }

  UnblockOnload(true);
}

void
nsDocument::EndLoad()
{
  
  
  
  if (mParser) {
    mWeakSink = do_GetWeakReference(mParser->GetContentSink());
    mParser = nullptr;
  }

  NS_DOCUMENT_NOTIFY_OBSERVERS(EndLoad, (this));

  UnblockDOMContentLoaded();
}

void
nsDocument::UnblockDOMContentLoaded()
{
  MOZ_ASSERT(mBlockDOMContentLoaded);
  if (--mBlockDOMContentLoaded != 0 || mDidFireDOMContentLoaded) {
    return;
  }
  mDidFireDOMContentLoaded = true;

  MOZ_ASSERT(mReadyState == READYSTATE_INTERACTIVE);
  if (!mSynchronousDOMContentLoaded) {
    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &nsDocument::DispatchContentLoadedEvents);
    NS_DispatchToCurrentThread(ev);
  } else {
    DispatchContentLoadedEvents();
  }
}

void
nsDocument::ContentStateChanged(nsIContent* aContent, EventStates aStateMask)
{
  NS_PRECONDITION(!nsContentUtils::IsSafeToRunScript(),
                  "Someone forgot a scriptblocker");
  NS_DOCUMENT_NOTIFY_OBSERVERS(ContentStateChanged,
                               (this, aContent, aStateMask));
}

void
nsDocument::DocumentStatesChanged(EventStates aStateMask)
{
  
  mGotDocumentState &= ~aStateMask;
  mDocumentState &= ~aStateMask;

  NS_DOCUMENT_NOTIFY_OBSERVERS(DocumentStatesChanged, (this, aStateMask));
}

void
nsDocument::StyleRuleChanged(nsIStyleSheet* aSheet,
                             nsIStyleRule* aOldStyleRule,
                             nsIStyleRule* aNewStyleRule)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleRuleChanged,
                               (this, aSheet,
                                aOldStyleRule, aNewStyleRule));

  if (StyleSheetChangeEventsEnabled()) {
    nsCOMPtr<css::Rule> rule = do_QueryInterface(aNewStyleRule);
    DO_STYLESHEET_NOTIFICATION(StyleRuleChangeEvent,
                               "StyleRuleChanged",
                               mRule,
                               rule ? rule->GetDOMRule() : nullptr);
  }
}

void
nsDocument::StyleRuleAdded(nsIStyleSheet* aSheet,
                           nsIStyleRule* aStyleRule)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleRuleAdded,
                               (this, aSheet, aStyleRule));

  if (StyleSheetChangeEventsEnabled()) {
    nsCOMPtr<css::Rule> rule = do_QueryInterface(aStyleRule);
    DO_STYLESHEET_NOTIFICATION(StyleRuleChangeEvent,
                               "StyleRuleAdded",
                               mRule,
                               rule ? rule->GetDOMRule() : nullptr);
  }
}

void
nsDocument::StyleRuleRemoved(nsIStyleSheet* aSheet,
                             nsIStyleRule* aStyleRule)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleRuleRemoved,
                               (this, aSheet, aStyleRule));

  if (StyleSheetChangeEventsEnabled()) {
    nsCOMPtr<css::Rule> rule = do_QueryInterface(aStyleRule);
    DO_STYLESHEET_NOTIFICATION(StyleRuleChangeEvent,
                               "StyleRuleRemoved",
                               mRule,
                               rule ? rule->GetDOMRule() : nullptr);
  }
}

#undef DO_STYLESHEET_NOTIFICATION

already_AddRefed<AnonymousContent>
nsIDocument::InsertAnonymousContent(Element& aElement, ErrorResult& aRv)
{
  nsIPresShell* shell = GetShell();
  if (!shell || !shell->GetCanvasFrame()) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsAutoScriptBlocker scriptBlocker;
  nsCOMPtr<Element> container = shell->GetCanvasFrame()
                                     ->GetCustomContentContainer();
  if (!container) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  
  nsCOMPtr<nsINode> clonedElement = aElement.CloneNode(true, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  
  nsresult rv;
  rv = container->AppendChildTo(clonedElement->AsContent(), true);
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  nsRefPtr<AnonymousContent> anonymousContent =
    new AnonymousContent(clonedElement->AsElement());
  mAnonymousContents.AppendElement(anonymousContent);

  shell->GetCanvasFrame()->ShowCustomContentContainer();

  return anonymousContent.forget();
}

void
nsIDocument::RemoveAnonymousContent(AnonymousContent& aContent,
                                    ErrorResult& aRv)
{
  nsIPresShell* shell = GetShell();
  if (!shell || !shell->GetCanvasFrame()) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  nsAutoScriptBlocker scriptBlocker;
  nsCOMPtr<Element> container = shell->GetCanvasFrame()
                                     ->GetCustomContentContainer();
  if (!container) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  
  for (size_t i = 0, len = mAnonymousContents.Length(); i < len; ++i) {
    if (mAnonymousContents[i] == &aContent) {
      
      nsCOMPtr<Element> node = aContent.GetContentNode();

      
      mAnonymousContents.RemoveElementAt(i);

      
      container->RemoveChild(*node, aRv);
      if (aRv.Failed()) {
        return;
      }

      break;
    }
  }
  if (mAnonymousContents.IsEmpty()) {
    shell->GetCanvasFrame()->HideCustomContentContainer();
  }
}




DocumentType*
nsIDocument::GetDoctype() const
{
  for (nsIContent* child = GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->NodeType() == nsIDOMNode::DOCUMENT_TYPE_NODE) {
      return static_cast<DocumentType*>(child);
    }
  }
  return nullptr;
}

NS_IMETHODIMP
nsDocument::GetDoctype(nsIDOMDocumentType** aDoctype)
{
  MOZ_ASSERT(aDoctype);
  nsCOMPtr<nsIDOMDocumentType> doctype = nsIDocument::GetDoctype();
  doctype.forget(aDoctype);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetImplementation(nsIDOMDOMImplementation** aImplementation)
{
  ErrorResult rv;
  *aImplementation = GetImplementation(rv);
  if (rv.Failed()) {
    MOZ_ASSERT(!*aImplementation);
    return rv.StealNSResult();
  }
  NS_ADDREF(*aImplementation);
  return NS_OK;
}

DOMImplementation*
nsDocument::GetImplementation(ErrorResult& rv)
{
  if (!mDOMImplementation) {
    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), "about:blank");
    if (!uri) {
      rv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return nullptr;
    }
    bool hasHadScriptObject = true;
    nsIScriptGlobalObject* scriptObject =
      GetScriptHandlingObject(hasHadScriptObject);
    if (!scriptObject && hasHadScriptObject) {
      rv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }
    mDOMImplementation = new DOMImplementation(this,
      scriptObject ? scriptObject : GetScopeObject(), uri, uri);
  }

  return mDOMImplementation;
}

NS_IMETHODIMP
nsDocument::GetDocumentElement(nsIDOMElement** aDocumentElement)
{
  NS_ENSURE_ARG_POINTER(aDocumentElement);

  Element* root = GetRootElement();
  if (root) {
    return CallQueryInterface(root, aDocumentElement);
  }

  *aDocumentElement = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateElement(const nsAString& aTagName,
                          nsIDOMElement** aReturn)
{
  *aReturn = nullptr;
  ErrorResult rv;
  nsCOMPtr<Element> element = nsIDocument::CreateElement(aTagName, rv);
  NS_ENSURE_FALSE(rv.Failed(), rv.StealNSResult());
  return CallQueryInterface(element, aReturn);
}

bool IsLowercaseASCII(const nsAString& aValue)
{
  int32_t len = aValue.Length();
  for (int32_t i = 0; i < len; ++i) {
    char16_t c = aValue[i];
    if (!(0x0061 <= (c) && ((c) <= 0x007a))) {
      return false;
    }
  }
  return true;
}

already_AddRefed<Element>
nsIDocument::CreateElement(const nsAString& aTagName, ErrorResult& rv)
{
  rv = nsContentUtils::CheckQName(aTagName, false);
  if (rv.Failed()) {
    return nullptr;
  }

  bool needsLowercase = IsHTMLDocument() && !IsLowercaseASCII(aTagName);
  nsAutoString lcTagName;
  if (needsLowercase) {
    nsContentUtils::ASCIIToLower(aTagName, lcTagName);
  }

  nsCOMPtr<nsIContent> content;
  rv = CreateElem(needsLowercase ? lcTagName : aTagName,
                  nullptr, mDefaultElementType, getter_AddRefs(content));
  if (rv.Failed()) {
    return nullptr;
  }
  return dont_AddRef(content.forget().take()->AsElement());
}

void
nsDocument::SetupCustomElement(Element* aElement,
                               uint32_t aNamespaceID,
                               const nsAString* aTypeExtension)
{
  if (!mRegistry) {
    return;
  }

  nsCOMPtr<nsIAtom> tagAtom = aElement->NodeInfo()->NameAtom();
  nsCOMPtr<nsIAtom> typeAtom = aTypeExtension ?
    do_GetAtom(*aTypeExtension) : tagAtom;

  if (aTypeExtension && !aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::is)) {
    
    
    aElement->SetAttr(kNameSpaceID_None, nsGkAtoms::is, *aTypeExtension, true);
  }

  CustomElementDefinition* data;
  CustomElementHashKey key(aNamespaceID, typeAtom);
  if (!mRegistry->mCustomDefinitions.Get(&key, &data)) {
    
    
    
    RegisterUnresolvedElement(aElement, typeAtom);
    return;
  }

  if (data->mLocalName != tagAtom) {
    
    
    
    return;
  }

  
  
  EnqueueLifecycleCallback(nsIDocument::eCreated, aElement, nullptr, data);
}

already_AddRefed<Element>
nsDocument::CreateElement(const nsAString& aTagName,
                          const nsAString& aTypeExtension,
                          ErrorResult& rv)
{
  nsRefPtr<Element> elem = nsIDocument::CreateElement(aTagName, rv);
  if (rv.Failed()) {
    return nullptr;
  }

  if (!aTagName.Equals(aTypeExtension)) {
    
    SetupCustomElement(elem, GetDefaultNamespaceID(), &aTypeExtension);
  }

  return elem.forget();
}

NS_IMETHODIMP
nsDocument::CreateElementNS(const nsAString& aNamespaceURI,
                            const nsAString& aQualifiedName,
                            nsIDOMElement** aReturn)
{
  *aReturn = nullptr;
  ErrorResult rv;
  nsCOMPtr<Element> element =
    nsIDocument::CreateElementNS(aNamespaceURI, aQualifiedName, rv);
  NS_ENSURE_FALSE(rv.Failed(), rv.StealNSResult());
  return CallQueryInterface(element, aReturn);
}

already_AddRefed<Element>
nsIDocument::CreateElementNS(const nsAString& aNamespaceURI,
                             const nsAString& aQualifiedName,
                             ErrorResult& rv)
{
  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  rv = nsContentUtils::GetNodeInfoFromQName(aNamespaceURI,
                                            aQualifiedName,
                                            mNodeInfoManager,
                                            nsIDOMNode::ELEMENT_NODE,
                                            getter_AddRefs(nodeInfo));
  if (rv.Failed()) {
    return nullptr;
  }

  nsCOMPtr<Element> element;
  rv = NS_NewElement(getter_AddRefs(element), nodeInfo.forget(),
                     NOT_FROM_PARSER);
  if (rv.Failed()) {
    return nullptr;
  }
  return element.forget();
}

already_AddRefed<Element>
nsDocument::CreateElementNS(const nsAString& aNamespaceURI,
                            const nsAString& aQualifiedName,
                            const nsAString& aTypeExtension,
                            ErrorResult& rv)
{
  nsRefPtr<Element> elem = nsIDocument::CreateElementNS(aNamespaceURI,
                                                        aQualifiedName,
                                                        rv);
  if (rv.Failed()) {
    return nullptr;
  }

  int32_t nameSpaceId = kNameSpaceID_Wildcard;
  if (!aNamespaceURI.EqualsLiteral("*")) {
    rv = nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                               nameSpaceId);
    if (rv.Failed()) {
      return nullptr;
    }
  }

  if (!aQualifiedName.Equals(aTypeExtension)) {
    
    SetupCustomElement(elem, nameSpaceId, &aTypeExtension);
  }

  return elem.forget();
}

NS_IMETHODIMP
nsDocument::CreateTextNode(const nsAString& aData, nsIDOMText** aReturn)
{
  *aReturn = nsIDocument::CreateTextNode(aData).take();
  return NS_OK;
}

already_AddRefed<nsTextNode>
nsIDocument::CreateTextNode(const nsAString& aData) const
{
  nsRefPtr<nsTextNode> text = new nsTextNode(mNodeInfoManager);
  
  text->SetText(aData, false);
  return text.forget();
}

NS_IMETHODIMP
nsDocument::CreateDocumentFragment(nsIDOMDocumentFragment** aReturn)
{
  *aReturn = nsIDocument::CreateDocumentFragment().take();
  return NS_OK;
}

already_AddRefed<DocumentFragment>
nsIDocument::CreateDocumentFragment() const
{
  nsRefPtr<DocumentFragment> frag = new DocumentFragment(mNodeInfoManager);
  return frag.forget();
}

NS_IMETHODIMP
nsDocument::CreateComment(const nsAString& aData, nsIDOMComment** aReturn)
{
  *aReturn = nsIDocument::CreateComment(aData).take();
  return NS_OK;
}


already_AddRefed<dom::Comment>
nsIDocument::CreateComment(const nsAString& aData) const
{
  nsRefPtr<dom::Comment> comment = new dom::Comment(mNodeInfoManager);

  
  comment->SetText(aData, false);
  return comment.forget();
}

NS_IMETHODIMP
nsDocument::CreateCDATASection(const nsAString& aData,
                               nsIDOMCDATASection** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  ErrorResult rv;
  *aReturn = nsIDocument::CreateCDATASection(aData, rv).take();
  return rv.StealNSResult();
}

already_AddRefed<CDATASection>
nsIDocument::CreateCDATASection(const nsAString& aData,
                                ErrorResult& rv)
{
  if (IsHTMLDocument()) {
    rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  if (FindInReadable(NS_LITERAL_STRING("]]>"), aData)) {
    rv.Throw(NS_ERROR_DOM_INVALID_CHARACTER_ERR);
    return nullptr;
  }

  nsRefPtr<CDATASection> cdata = new CDATASection(mNodeInfoManager);

  
  cdata->SetText(aData, false);

  return cdata.forget();
}

NS_IMETHODIMP
nsDocument::CreateProcessingInstruction(const nsAString& aTarget,
                                        const nsAString& aData,
                                        nsIDOMProcessingInstruction** aReturn)
{
  ErrorResult rv;
  *aReturn =
    nsIDocument::CreateProcessingInstruction(aTarget, aData, rv).take();
  return rv.StealNSResult();
}

already_AddRefed<ProcessingInstruction>
nsIDocument::CreateProcessingInstruction(const nsAString& aTarget,
                                         const nsAString& aData,
                                         ErrorResult& rv) const
{
  nsresult res = nsContentUtils::CheckQName(aTarget, false);
  if (NS_FAILED(res)) {
    rv.Throw(res);
    return nullptr;
  }

  if (FindInReadable(NS_LITERAL_STRING("?>"), aData)) {
    rv.Throw(NS_ERROR_DOM_INVALID_CHARACTER_ERR);
    return nullptr;
  }

  nsRefPtr<ProcessingInstruction> pi =
    NS_NewXMLProcessingInstruction(mNodeInfoManager, aTarget, aData);

  return pi.forget();
}

NS_IMETHODIMP
nsDocument::CreateAttribute(const nsAString& aName,
                            nsIDOMAttr** aReturn)
{
  ErrorResult rv;
  *aReturn = nsIDocument::CreateAttribute(aName, rv).take();
  return rv.StealNSResult();
}

already_AddRefed<Attr>
nsIDocument::CreateAttribute(const nsAString& aName, ErrorResult& rv)
{
  WarnOnceAbout(eCreateAttribute);

  if (!mNodeInfoManager) {
    rv.Throw(NS_ERROR_NOT_INITIALIZED);
    return nullptr;
  }

  nsresult res = nsContentUtils::CheckQName(aName, false);
  if (NS_FAILED(res)) {
    rv.Throw(res);
    return nullptr;
  }

  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  res = mNodeInfoManager->GetNodeInfo(aName, nullptr, kNameSpaceID_None,
                                      nsIDOMNode::ATTRIBUTE_NODE,
                                      getter_AddRefs(nodeInfo));
  if (NS_FAILED(res)) {
    rv.Throw(res);
    return nullptr;
  }

  nsRefPtr<Attr> attribute = new Attr(nullptr, nodeInfo.forget(),
                                      EmptyString(), false);
  return attribute.forget();
}

NS_IMETHODIMP
nsDocument::CreateAttributeNS(const nsAString & aNamespaceURI,
                              const nsAString & aQualifiedName,
                              nsIDOMAttr **aResult)
{
  ErrorResult rv;
  *aResult =
    nsIDocument::CreateAttributeNS(aNamespaceURI, aQualifiedName, rv).take();
  return rv.StealNSResult();
}

already_AddRefed<Attr>
nsIDocument::CreateAttributeNS(const nsAString& aNamespaceURI,
                               const nsAString& aQualifiedName,
                               ErrorResult& rv)
{
  WarnOnceAbout(eCreateAttributeNS);

  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  rv = nsContentUtils::GetNodeInfoFromQName(aNamespaceURI,
                                            aQualifiedName,
                                            mNodeInfoManager,
                                            nsIDOMNode::ATTRIBUTE_NODE,
                                            getter_AddRefs(nodeInfo));
  if (rv.Failed()) {
    return nullptr;
  }

  nsRefPtr<Attr> attribute = new Attr(nullptr, nodeInfo.forget(),
                                      EmptyString(), true);
  return attribute.forget();
}

bool
nsDocument::CustomElementConstructor(JSContext* aCx, unsigned aArgc, JS::Value* aVp)
{
  JS::CallArgs args = JS::CallArgsFromVp(aArgc, aVp);

  JS::Rooted<JSObject*> global(aCx,
    JS_GetGlobalForObject(aCx, &args.callee()));
  nsCOMPtr<nsPIDOMWindow> window = do_QueryWrapper(aCx, global);
  MOZ_ASSERT(window, "Should have a non-null window");

  nsDocument* document = static_cast<nsDocument*>(window->GetDoc());

  
  JSString* jsFunName =
    JS_GetFunctionId(JS_ValueToFunction(aCx, args.calleev()));
  nsAutoJSString elemName;
  if (!elemName.init(aCx, jsFunName)) {
    return true;
  }

  nsCOMPtr<nsIAtom> typeAtom(do_GetAtom(elemName));
  CustomElementHashKey key(kNameSpaceID_Unknown, typeAtom);
  CustomElementDefinition* definition;
  if (!document->mRegistry ||
      !document->mRegistry->mCustomDefinitions.Get(&key, &definition)) {
    return true;
  }

  nsDependentAtomString localName(definition->mLocalName);

  nsCOMPtr<nsIContent> newElement;
  nsresult rv = document->CreateElem(localName, nullptr,
                                     definition->mNamespaceID,
                                     getter_AddRefs(newElement));
  NS_ENSURE_SUCCESS(rv, true);

  nsCOMPtr<Element> element = do_QueryInterface(newElement);
  if (definition->mLocalName != typeAtom) {
    
    
    
    document->SetupCustomElement(element, definition->mNamespaceID, &elemName);
  }

  rv = nsContentUtils::WrapNative(aCx, newElement, newElement, args.rval());
  NS_ENSURE_SUCCESS(rv, true);

  return true;
}

bool
nsDocument::IsWebComponentsEnabled(JSContext* aCx, JSObject* aObject)
{
  JS::Rooted<JSObject*> obj(aCx, aObject);
  return Preferences::GetBool("dom.webcomponents.enabled") ||
    IsInCertifiedApp(aCx, obj);
}

nsresult
nsDocument::RegisterUnresolvedElement(Element* aElement, nsIAtom* aTypeName)
{
  if (!mRegistry) {
    return NS_OK;
  }

  mozilla::dom::NodeInfo* info = aElement->NodeInfo();

  
  
  
  nsCOMPtr<nsIAtom> typeName = aTypeName;
  if (!typeName) {
    typeName = info->NameAtom();
  }

  CustomElementHashKey key(info->NamespaceID(), typeName);
  if (mRegistry->mCustomDefinitions.Get(&key)) {
    return NS_OK;
  }

  nsTArray<nsRefPtr<Element>>* unresolved;
  mRegistry->mCandidatesMap.Get(&key, &unresolved);
  if (!unresolved) {
    unresolved = new nsTArray<nsRefPtr<Element>>();
    
    mRegistry->mCandidatesMap.Put(&key, unresolved);
  }

  nsRefPtr<Element>* elem = unresolved->AppendElement();
  *elem = aElement;
  aElement->AddStates(NS_EVENT_STATE_UNRESOLVED);

  return NS_OK;
}

namespace {

class ProcessStackRunner final : public nsIRunnable
{
  ~ProcessStackRunner() {}
public:
  explicit ProcessStackRunner(bool aIsBaseQueue = false)
    : mIsBaseQueue(aIsBaseQueue)
  {
  }
  NS_DECL_ISUPPORTS
  NS_IMETHOD Run() override
  {
    nsDocument::ProcessTopElementQueue(mIsBaseQueue);
    return NS_OK;
  }
  bool mIsBaseQueue;
};

NS_IMPL_ISUPPORTS(ProcessStackRunner, nsIRunnable);

} 

void
nsDocument::EnqueueLifecycleCallback(nsIDocument::ElementCallbackType aType,
                                     Element* aCustomElement,
                                     LifecycleCallbackArgs* aArgs,
                                     CustomElementDefinition* aDefinition)
{
  if (!mRegistry) {
    
    
    return;
  }

  CustomElementData* elementData = aCustomElement->GetCustomElementData();

  
  CustomElementDefinition* definition = aDefinition;
  if (!definition) {
    mozilla::dom::NodeInfo* info = aCustomElement->NodeInfo();

    
    
    nsCOMPtr<nsIAtom> typeAtom = elementData ?
      elementData->mType.get() : info->NameAtom();

    CustomElementHashKey key(info->NamespaceID(), typeAtom);
    if (!mRegistry->mCustomDefinitions.Get(&key, &definition) ||
        definition->mLocalName != info->NameAtom()) {
      
      
      return;
    }
  }

  if (!elementData) {
    
    
    elementData = new CustomElementData(definition->mType);
    
    aCustomElement->SetCustomElementData(elementData);
    MOZ_ASSERT(aType == nsIDocument::eCreated,
               "First callback should be the created callback");
  }

  
  CallbackFunction* func = nullptr;
  switch (aType) {
    case nsIDocument::eCreated:
      if (definition->mCallbacks->mCreatedCallback.WasPassed()) {
        func = definition->mCallbacks->mCreatedCallback.Value();
      }
      break;

    case nsIDocument::eAttached:
      if (definition->mCallbacks->mAttachedCallback.WasPassed()) {
        func = definition->mCallbacks->mAttachedCallback.Value();
      }
      break;

    case nsIDocument::eDetached:
      if (definition->mCallbacks->mDetachedCallback.WasPassed()) {
        func = definition->mCallbacks->mDetachedCallback.Value();
      }
      break;

    case nsIDocument::eAttributeChanged:
      if (definition->mCallbacks->mAttributeChangedCallback.WasPassed()) {
        func = definition->mCallbacks->mAttributeChangedCallback.Value();
      }
      break;
  }

  
  if (!func) {
    return;
  }

  if (aType == nsIDocument::eCreated) {
    elementData->mCreatedCallbackInvoked = false;
  } else if (!elementData->mCreatedCallbackInvoked) {
    
    
    return;
  }

  
  CustomElementCallback* callback = new CustomElementCallback(aCustomElement,
                                                              aType,
                                                              func,
                                                              elementData);
  
  elementData->mCallbackQueue.AppendElement(callback);
  if (aArgs) {
    callback->SetArgs(*aArgs);
  }

  if (!elementData->mElementIsBeingCreated) {
    CustomElementData* lastData =
      sProcessingStack->SafeLastElement(nullptr);

    
    
    
    
    
    bool shouldPushElementQueue = nsContentUtils::MicroTaskLevel() > 0 &&
      (!lastData || lastData->mAssociatedMicroTask <
         static_cast<int32_t>(nsContentUtils::MicroTaskLevel()));

    
    
    if (shouldPushElementQueue) {
      
      
      sProcessingStack->AppendElement((CustomElementData*) nullptr);
    }

    sProcessingStack->AppendElement(elementData);
    elementData->mAssociatedMicroTask =
      static_cast<int32_t>(nsContentUtils::MicroTaskLevel());

    
    
    if (shouldPushElementQueue) {
      
      
      
      
      nsContentUtils::AddScriptRunner(new ProcessStackRunner());
    }
  }
}


void
nsDocument::ProcessBaseElementQueue()
{
  
  
  if (sProcessingBaseElementQueue || !sProcessingStack) {
    return;
  }

  MOZ_ASSERT(nsContentUtils::MicroTaskLevel() == 0);
  sProcessingBaseElementQueue = true;
  nsContentUtils::AddScriptRunner(new ProcessStackRunner(true));
}


void
nsDocument::ProcessTopElementQueue(bool aIsBaseQueue)
{
  MOZ_ASSERT(nsContentUtils::IsSafeToRunScript());

  nsTArray<nsRefPtr<CustomElementData>>& stack = *sProcessingStack;
  uint32_t firstQueue = stack.LastIndexOf((CustomElementData*) nullptr);

  if (aIsBaseQueue && firstQueue != 0) {
    return;
  }

  for (uint32_t i = firstQueue + 1; i < stack.Length(); ++i) {
    
    
    
    if (stack[i]->mAssociatedMicroTask != -1) {
      stack[i]->RunCallbackQueue();
      stack[i]->mAssociatedMicroTask = -1;
    }
  }

  
  
  if (firstQueue != 0) {
    stack.SetLength(firstQueue);
  } else {
    
    stack.SetLength(1);
    sProcessingBaseElementQueue = false;
  }
}

bool
nsDocument::RegisterEnabled()
{
  static bool sPrefValue =
    Preferences::GetBool("dom.webcomponents.enabled", false);
  return sPrefValue;
}


Maybe<nsTArray<nsRefPtr<mozilla::dom::CustomElementData>>>
nsDocument::sProcessingStack;


bool
nsDocument::sProcessingBaseElementQueue;

void
nsDocument::RegisterElement(JSContext* aCx, const nsAString& aType,
                            const ElementRegistrationOptions& aOptions,
                            JS::MutableHandle<JSObject*> aRetval,
                            ErrorResult& rv)
{
  if (!mRegistry) {
    rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  Registry::DefinitionMap& definitions = mRegistry->mCustomDefinitions;

  
  nsAutoString lcType;
  nsContentUtils::ASCIIToLower(aType, lcType);

  
  
  nsAutoString lcName;
  if (IsHTMLDocument()) {
    nsContentUtils::ASCIIToLower(aOptions.mExtends, lcName);
  } else {
    lcName.Assign(aOptions.mExtends);
  }

  nsCOMPtr<nsIAtom> typeAtom(do_GetAtom(lcType));
  if (!nsContentUtils::IsCustomElementName(typeAtom)) {
    rv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  
  
  
  CustomElementHashKey duplicateFinder(kNameSpaceID_Unknown, typeAtom);
  if (definitions.Get(&duplicateFinder)) {
    rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  nsIGlobalObject* sgo = GetScopeObject();
  if (!sgo) {
    rv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  JS::Rooted<JSObject*> global(aCx, sgo->GetGlobalJSObject());
  nsCOMPtr<nsIAtom> nameAtom;
  int32_t namespaceID = kNameSpaceID_XHTML;
  JS::Rooted<JSObject*> protoObject(aCx);
  {
    JS::Rooted<JSObject*> htmlProto(aCx);
    JS::Rooted<JSObject*> svgProto(aCx);
    {
      JSAutoCompartment ac(aCx, global);

      htmlProto = HTMLElementBinding::GetProtoObjectHandle(aCx, global);
      if (!htmlProto) {
        rv.Throw(NS_ERROR_OUT_OF_MEMORY);
        return;
      }

      svgProto = SVGElementBinding::GetProtoObjectHandle(aCx, global);
      if (!svgProto) {
        rv.Throw(NS_ERROR_OUT_OF_MEMORY);
        return;
      }
    }

    if (!aOptions.mPrototype) {
      if (!JS_WrapObject(aCx, &htmlProto)) {
        rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
        return;
      }

      protoObject = JS_NewObjectWithGivenProto(aCx, nullptr, htmlProto);
      if (!protoObject) {
        rv.Throw(NS_ERROR_UNEXPECTED);
        return;
      }
    } else {
      protoObject = aOptions.mPrototype;

      
      JS::Rooted<JSObject*> protoObjectUnwrapped(aCx, js::CheckedUnwrap(protoObject));
      if (!protoObjectUnwrapped) {
        
        
        rv.Throw(NS_ERROR_DOM_SECURITY_ERR);
        return;
      }

      
      
      
      const js::Class* clasp = js::GetObjectClass(protoObjectUnwrapped);
      if (IsDOMIfaceAndProtoClass(clasp)) {
        rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
        return;
      }

      JS::Rooted<JSPropertyDescriptor> descRoot(aCx);
      JS::MutableHandle<JSPropertyDescriptor> desc(&descRoot);
      
      
      
      if (!JS_GetPropertyDescriptor(aCx, protoObject, "constructor", desc)) {
        rv.Throw(NS_ERROR_UNEXPECTED);
        return;
      }

      if (!desc.configurable()) {
        rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
        return;
      }

      JS::Rooted<JSObject*> protoProto(aCx, protoObject);

      if (!JS_WrapObject(aCx, &htmlProto) || !JS_WrapObject(aCx, &svgProto)) {
        rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
        return;
      }

      
      
      while (protoProto) {
        if (protoProto == htmlProto) {
          break;
        }

        if (protoProto == svgProto) {
          namespaceID = kNameSpaceID_SVG;
          break;
        }

        if (!JS_GetPrototype(aCx, protoProto, &protoProto)) {
          rv.Throw(NS_ERROR_UNEXPECTED);
          return;
        }
      }
    } 

    
    if (!lcName.IsEmpty()) {
      
      bool known = false;
      nameAtom = do_GetAtom(lcName);
      if (namespaceID == kNameSpaceID_XHTML) {
        nsIParserService* ps = nsContentUtils::GetParserService();
        if (!ps) {
          rv.Throw(NS_ERROR_UNEXPECTED);
          return;
        }

        known =
          ps->HTMLCaseSensitiveAtomTagToId(nameAtom) != eHTMLTag_userdefined;
      } else {
        known = SVGElementFactory::Exists(nameAtom);
      }

      
      
      
      if (!known) {
        rv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
        return;
      }
    } else {
      
      if (namespaceID == kNameSpaceID_SVG) {
        rv.Throw(NS_ERROR_UNEXPECTED);
        return;
      }

      nameAtom = typeAtom;
    }
  } 

  JS::Rooted<JSObject*> wrappedProto(aCx, protoObject);
  if (!JS_WrapObject(aCx, &wrappedProto)) {
    rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  
  nsAutoPtr<LifecycleCallbacks> callbacksHolder(new LifecycleCallbacks());
  JS::RootedValue rootedv(aCx, JS::ObjectValue(*wrappedProto));
  if (!JS_WrapValue(aCx, &rootedv) || !callbacksHolder->Init(aCx, rootedv)) {
    rv.Throw(NS_ERROR_FAILURE);
    return;
  }

  
  CustomElementHashKey key(namespaceID, typeAtom);
  LifecycleCallbacks* callbacks = callbacksHolder.forget();
  CustomElementDefinition* definition =
    new CustomElementDefinition(wrappedProto,
                                typeAtom,
                                nameAtom,
                                callbacks,
                                namespaceID,
                                0 );
  definitions.Put(&key, definition);

  
  nsAutoPtr<nsTArray<nsRefPtr<Element>>> candidates;
  mRegistry->mCandidatesMap.RemoveAndForget(&key, candidates);
  if (candidates) {
    for (size_t i = 0; i < candidates->Length(); ++i) {
      Element *elem = candidates->ElementAt(i);

      elem->RemoveStates(NS_EVENT_STATE_UNRESOLVED);

      
      
      
      if (elem->NodeInfo()->NameAtom() != nameAtom) {
        
        continue;
      }

      MOZ_ASSERT(elem->IsHTMLElement(nameAtom));
      nsWrapperCache* cache;
      CallQueryInterface(elem, &cache);
      MOZ_ASSERT(cache, "Element doesn't support wrapper cache?");

      
      
      
      
      JS::RootedObject wrapper(aCx);
      if ((wrapper = cache->GetWrapper()) && JS_WrapObject(aCx, &wrapper)) {
        if (!JS_SetPrototype(aCx, wrapper, wrappedProto)) {
          continue;
        }
      }

      EnqueueLifecycleCallback(nsIDocument::eCreated, elem, nullptr, definition);
    }
  }

  JS::Rooted<JSFunction*> constructor(aCx);

  {
    
    
    
    JSAutoCompartment ac(aCx, global);

    
    
    constructor = JS_NewFunction(aCx, nsDocument::CustomElementConstructor, 0,
                                 JSFUN_CONSTRUCTOR,
                                 NS_ConvertUTF16toUTF8(lcType).get());
    if (!constructor) {
      rv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return;
    }
  }

  JS::Rooted<JSObject*> wrappedConstructor(aCx);
  wrappedConstructor = JS_GetFunctionObject(constructor);
  if (!JS_WrapObject(aCx, &wrappedConstructor)) {
    rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  if (!JS_LinkConstructorAndPrototype(aCx, wrappedConstructor, protoObject)) {
    rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  aRetval.set(wrappedConstructor);
}

void
nsDocument::UseRegistryFromDocument(nsIDocument* aDocument)
{
  nsDocument* doc = static_cast<nsDocument*>(aDocument);
  MOZ_ASSERT(!mRegistry, "There should be no existing registry.");
  mRegistry = doc->mRegistry;
}

NS_IMETHODIMP
nsDocument::GetElementsByTagName(const nsAString& aTagname,
                                 nsIDOMNodeList** aReturn)
{
  nsRefPtr<nsContentList> list = GetElementsByTagName(aTagname);
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  
  list.forget(aReturn);
  return NS_OK;
}

long
nsDocument::BlockedTrackingNodeCount() const
{
  return mBlockedTrackingNodes.Length();
}

already_AddRefed<nsSimpleContentList>
nsDocument::BlockedTrackingNodes() const
{
  nsRefPtr<nsSimpleContentList> list = new nsSimpleContentList(nullptr);

  nsTArray<nsWeakPtr> blockedTrackingNodes;
  blockedTrackingNodes = mBlockedTrackingNodes;

  for (unsigned long i = 0; i < blockedTrackingNodes.Length(); i++) {
    nsWeakPtr weakNode = blockedTrackingNodes[i];
    nsCOMPtr<nsIContent> node = do_QueryReferent(weakNode);
    
    
    
    if (node) {
      list->AppendElement(node);
    }
  }

  return list.forget();
}

already_AddRefed<nsContentList>
nsIDocument::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                    const nsAString& aLocalName,
                                    ErrorResult& aResult)
{
  int32_t nameSpaceId = kNameSpaceID_Wildcard;

  if (!aNamespaceURI.EqualsLiteral("*")) {
    aResult =
      nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                            nameSpaceId);
    if (aResult.Failed()) {
      return nullptr;
    }
  }

  NS_ASSERTION(nameSpaceId != kNameSpaceID_Unknown, "Unexpected namespace ID!");

  return NS_GetContentList(this, nameSpaceId, aLocalName);
}

NS_IMETHODIMP
nsDocument::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                   const nsAString& aLocalName,
                                   nsIDOMNodeList** aReturn)
{
  ErrorResult rv;
  nsRefPtr<nsContentList> list =
    nsIDocument::GetElementsByTagNameNS(aNamespaceURI, aLocalName, rv);
  if (rv.Failed()) {
    return rv.StealNSResult();
  }

  
  list.forget(aReturn);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetAsync(bool *aAsync)
{
  NS_ERROR("nsDocument::GetAsync() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::SetAsync(bool aAsync)
{
  NS_ERROR("nsDocument::SetAsync() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::Load(const nsAString& aUrl, bool *aReturn)
{
  NS_ERROR("nsDocument::Load() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetStyleSheets(nsIDOMStyleSheetList** aStyleSheets)
{
  NS_ADDREF(*aStyleSheets = StyleSheets());
  return NS_OK;
}

StyleSheetList*
nsDocument::StyleSheets()
{
  if (!mDOMStyleSheets) {
    mDOMStyleSheets = new nsDOMStyleSheetList(this);
  }
  return mDOMStyleSheets;
}

NS_IMETHODIMP
nsDocument::GetMozSelectedStyleSheetSet(nsAString& aSheetSet)
{
  nsIDocument::GetSelectedStyleSheetSet(aSheetSet);
  return NS_OK;
}

void
nsIDocument::GetSelectedStyleSheetSet(nsAString& aSheetSet)
{
  aSheetSet.Truncate();

  
  int32_t count = GetNumberOfStyleSheets();
  nsAutoString title;
  for (int32_t index = 0; index < count; index++) {
    nsIStyleSheet* sheet = GetStyleSheetAt(index);
    NS_ASSERTION(sheet, "Null sheet in sheet list!");

    nsCOMPtr<nsIDOMStyleSheet> domSheet = do_QueryInterface(sheet);
    NS_ASSERTION(domSheet, "Sheet must QI to nsIDOMStyleSheet");
    bool disabled;
    domSheet->GetDisabled(&disabled);
    if (disabled) {
      
      continue;
    }

    sheet->GetTitle(title);

    if (aSheetSet.IsEmpty()) {
      aSheetSet = title;
    } else if (!title.IsEmpty() && !aSheetSet.Equals(title)) {
      
      SetDOMStringToNull(aSheetSet);
      return;
    }
  }
}

NS_IMETHODIMP
nsDocument::SetMozSelectedStyleSheetSet(const nsAString& aSheetSet)
{
  SetSelectedStyleSheetSet(aSheetSet);
  return NS_OK;
}

void
nsDocument::SetSelectedStyleSheetSet(const nsAString& aSheetSet)
{
  if (DOMStringIsNull(aSheetSet)) {
    return;
  }

  
  
  mLastStyleSheetSet = aSheetSet;
  EnableStyleSheetsForSetInternal(aSheetSet, true);
}

NS_IMETHODIMP
nsDocument::GetLastStyleSheetSet(nsAString& aSheetSet)
{
  nsString sheetSet;
  GetLastStyleSheetSet(sheetSet);
  aSheetSet = sheetSet;
  return NS_OK;
}

void
nsDocument::GetLastStyleSheetSet(nsString& aSheetSet)
{
  aSheetSet = mLastStyleSheetSet;
}

NS_IMETHODIMP
nsDocument::GetPreferredStyleSheetSet(nsAString& aSheetSet)
{
  nsIDocument::GetPreferredStyleSheetSet(aSheetSet);
  return NS_OK;
}

void
nsIDocument::GetPreferredStyleSheetSet(nsAString& aSheetSet)
{
  GetHeaderData(nsGkAtoms::headerDefaultStyle, aSheetSet);
}

NS_IMETHODIMP
nsDocument::GetStyleSheetSets(nsISupports** aList)
{
  NS_ADDREF(*aList = StyleSheetSets());
  return NS_OK;
}

DOMStringList*
nsDocument::StyleSheetSets()
{
  if (!mStyleSheetSetList) {
    mStyleSheetSetList = new nsDOMStyleSheetSetList(this);
  }
  return mStyleSheetSetList;
}

NS_IMETHODIMP
nsDocument::MozEnableStyleSheetsForSet(const nsAString& aSheetSet)
{
  EnableStyleSheetsForSet(aSheetSet);
  return NS_OK;
}

void
nsDocument::EnableStyleSheetsForSet(const nsAString& aSheetSet)
{
  
  if (!DOMStringIsNull(aSheetSet)) {
    
    
    
    
    EnableStyleSheetsForSetInternal(aSheetSet, false);
  }
}

void
nsDocument::EnableStyleSheetsForSetInternal(const nsAString& aSheetSet,
                                            bool aUpdateCSSLoader)
{
  BeginUpdate(UPDATE_STYLE);
  int32_t count = GetNumberOfStyleSheets();
  nsAutoString title;
  for (int32_t index = 0; index < count; index++) {
    nsIStyleSheet* sheet = GetStyleSheetAt(index);
    NS_ASSERTION(sheet, "Null sheet in sheet list!");
    sheet->GetTitle(title);
    if (!title.IsEmpty()) {
      sheet->SetEnabled(title.Equals(aSheetSet));
    }
  }
  if (aUpdateCSSLoader) {
    CSSLoader()->SetPreferredSheet(aSheetSet);
  }
  EndUpdate(UPDATE_STYLE);
}

NS_IMETHODIMP
nsDocument::GetCharacterSet(nsAString& aCharacterSet)
{
  nsIDocument::GetCharacterSet(aCharacterSet);
  return NS_OK;
}

void
nsIDocument::GetCharacterSet(nsAString& aCharacterSet) const
{
  CopyASCIItoUTF16(GetDocumentCharacterSet(), aCharacterSet);
}

NS_IMETHODIMP
nsDocument::ImportNode(nsIDOMNode* aImportedNode,
                       bool aDeep,
                       uint8_t aArgc,
                       nsIDOMNode** aResult)
{
  if (aArgc == 0) {
    aDeep = true;
  }

  *aResult = nullptr;

  nsCOMPtr<nsINode> imported = do_QueryInterface(aImportedNode);
  NS_ENSURE_TRUE(imported, NS_ERROR_UNEXPECTED);

  ErrorResult rv;
  nsCOMPtr<nsINode> result = nsIDocument::ImportNode(*imported, aDeep, rv);
  if (rv.Failed()) {
    return rv.StealNSResult();
  }

  NS_ADDREF(*aResult = result->AsDOMNode());
  return NS_OK;
}

already_AddRefed<nsINode>
nsIDocument::ImportNode(nsINode& aNode, bool aDeep, ErrorResult& rv) const
{
  nsINode* imported = &aNode;

  switch (imported->NodeType()) {
    case nsIDOMNode::ATTRIBUTE_NODE:
    case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    case nsIDOMNode::ELEMENT_NODE:
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    case nsIDOMNode::TEXT_NODE:
    case nsIDOMNode::CDATA_SECTION_NODE:
    case nsIDOMNode::COMMENT_NODE:
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
    {
      nsCOMPtr<nsINode> newNode;
      nsCOMArray<nsINode> nodesWithProperties;
      rv = nsNodeUtils::Clone(imported, aDeep, mNodeInfoManager,
                              nodesWithProperties, getter_AddRefs(newNode));
      if (rv.Failed()) {
        return nullptr;
      }
      return newNode.forget();
    }
    default:
    {
      NS_WARNING("Don't know how to clone this nodetype for importNode.");

      rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    }
  }

  return nullptr;
}

NS_IMETHODIMP
nsDocument::LoadBindingDocument(const nsAString& aURI)
{
  ErrorResult rv;
  nsIDocument::LoadBindingDocument(aURI, rv);
  return rv.StealNSResult();
}

void
nsIDocument::LoadBindingDocument(const nsAString& aURI, ErrorResult& rv)
{
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURI,
                 mCharacterSet.get(),
                 GetDocBaseURI());
  if (rv.Failed()) {
    return;
  }

  
  
  
  nsCOMPtr<nsIPrincipal> subjectPrincipal =
    nsContentUtils::GetCurrentJSContext() ? nsContentUtils::SubjectPrincipal()
                                          : NodePrincipal();
  BindingManager()->LoadBindingDocument(this, uri, subjectPrincipal);
}

NS_IMETHODIMP
nsDocument::GetBindingParent(nsIDOMNode* aNode, nsIDOMElement** aResult)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_ARG_POINTER(node);

  Element* bindingParent = nsIDocument::GetBindingParent(*node);
  nsCOMPtr<nsIDOMElement> retval = do_QueryInterface(bindingParent);
  retval.forget(aResult);
  return NS_OK;
}

Element*
nsIDocument::GetBindingParent(nsINode& aNode)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(&aNode));
  if (!content)
    return nullptr;

  nsIContent* bindingParent = content->GetBindingParent();
  return bindingParent ? bindingParent->AsElement() : nullptr;
}

static Element*
GetElementByAttribute(nsIContent* aContent, nsIAtom* aAttrName,
                      const nsAString& aAttrValue, bool aUniversalMatch)
{
  if (aUniversalMatch ? aContent->HasAttr(kNameSpaceID_None, aAttrName) :
                        aContent->AttrValueIs(kNameSpaceID_None, aAttrName,
                                              aAttrValue, eCaseMatters)) {
    return aContent->AsElement();
  }

  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {

    Element* matchedElement =
      GetElementByAttribute(child, aAttrName, aAttrValue, aUniversalMatch);
    if (matchedElement)
      return matchedElement;
  }

  return nullptr;
}

Element*
nsDocument::GetAnonymousElementByAttribute(nsIContent* aElement,
                                           nsIAtom* aAttrName,
                                           const nsAString& aAttrValue) const
{
  nsINodeList* nodeList = BindingManager()->GetAnonymousNodesFor(aElement);
  if (!nodeList)
    return nullptr;

  uint32_t length = 0;
  nodeList->GetLength(&length);

  bool universalMatch = aAttrValue.EqualsLiteral("*");

  for (uint32_t i = 0; i < length; ++i) {
    nsIContent* current = nodeList->Item(i);
    Element* matchedElm =
      GetElementByAttribute(current, aAttrName, aAttrValue, universalMatch);
    if (matchedElm)
      return matchedElm;
  }

  return nullptr;
}

NS_IMETHODIMP
nsDocument::GetAnonymousElementByAttribute(nsIDOMElement* aElement,
                                           const nsAString& aAttrName,
                                           const nsAString& aAttrValue,
                                           nsIDOMElement** aResult)
{
  nsCOMPtr<Element> element = do_QueryInterface(aElement);
  NS_ENSURE_ARG_POINTER(element);

  Element* anonEl =
    nsIDocument::GetAnonymousElementByAttribute(*element, aAttrName,
                                                aAttrValue);
  nsCOMPtr<nsIDOMElement> retval = do_QueryInterface(anonEl);
  retval.forget(aResult);
  return NS_OK;
}

Element*
nsIDocument::GetAnonymousElementByAttribute(Element& aElement,
                                            const nsAString& aAttrName,
                                            const nsAString& aAttrValue)
{
  nsCOMPtr<nsIAtom> attribute = do_GetAtom(aAttrName);

  return GetAnonymousElementByAttribute(&aElement, attribute, aAttrValue);
}


NS_IMETHODIMP
nsDocument::GetAnonymousNodes(nsIDOMElement* aElement,
                              nsIDOMNodeList** aResult)
{
  *aResult = nullptr;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));
  return BindingManager()->GetAnonymousNodesFor(content, aResult);
}

nsINodeList*
nsIDocument::GetAnonymousNodes(Element& aElement)
{
  return BindingManager()->GetAnonymousNodesFor(&aElement);
}

NS_IMETHODIMP
nsDocument::CreateRange(nsIDOMRange** aReturn)
{
  ErrorResult rv;
  *aReturn = nsIDocument::CreateRange(rv).take();
  return rv.StealNSResult();
}

already_AddRefed<nsRange>
nsIDocument::CreateRange(ErrorResult& rv)
{
  nsRefPtr<nsRange> range = new nsRange(this);
  nsresult res = range->Set(this, 0, this, 0);
  if (NS_FAILED(res)) {
    rv.Throw(res);
    return nullptr;
  }

  return range.forget();
}

NS_IMETHODIMP
nsDocument::CreateNodeIterator(nsIDOMNode *aRoot,
                               uint32_t aWhatToShow,
                               nsIDOMNodeFilter *aFilter,
                               uint8_t aOptionalArgc,
                               nsIDOMNodeIterator **_retval)
{
  *_retval = nullptr;

  if (!aOptionalArgc) {
    aWhatToShow = nsIDOMNodeFilter::SHOW_ALL;
  }

  if (!aRoot) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  nsCOMPtr<nsINode> root = do_QueryInterface(aRoot);
  NS_ENSURE_TRUE(root, NS_ERROR_UNEXPECTED);

  ErrorResult rv;
  NodeFilterHolder holder(aFilter);
  *_retval = nsIDocument::CreateNodeIterator(*root, aWhatToShow, holder,
                                             rv).take();
  return rv.StealNSResult();
}

already_AddRefed<NodeIterator>
nsIDocument::CreateNodeIterator(nsINode& aRoot, uint32_t aWhatToShow,
                                NodeFilter* aFilter,
                                ErrorResult& rv) const
{
  NodeFilterHolder holder(aFilter);
  return CreateNodeIterator(aRoot, aWhatToShow, holder, rv);
}

already_AddRefed<NodeIterator>
nsIDocument::CreateNodeIterator(nsINode& aRoot, uint32_t aWhatToShow,
                                const NodeFilterHolder& aFilter,
                                ErrorResult& rv) const
{
  nsINode* root = &aRoot;
  nsRefPtr<NodeIterator> iterator = new NodeIterator(root, aWhatToShow,
                                                     aFilter);
  return iterator.forget();
}

NS_IMETHODIMP
nsDocument::CreateTreeWalker(nsIDOMNode *aRoot,
                             uint32_t aWhatToShow,
                             nsIDOMNodeFilter *aFilter,
                             uint8_t aOptionalArgc,
                             nsIDOMTreeWalker **_retval)
{
  *_retval = nullptr;

  if (!aOptionalArgc) {
    aWhatToShow = nsIDOMNodeFilter::SHOW_ALL;
  }

  nsCOMPtr<nsINode> root = do_QueryInterface(aRoot);
  NS_ENSURE_TRUE(root, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

  ErrorResult rv;
  NodeFilterHolder holder(aFilter);
  *_retval = nsIDocument::CreateTreeWalker(*root, aWhatToShow, holder,
                                           rv).take();
  return rv.StealNSResult();
}

already_AddRefed<TreeWalker>
nsIDocument::CreateTreeWalker(nsINode& aRoot, uint32_t aWhatToShow,
                              NodeFilter* aFilter,
                              ErrorResult& rv) const
{
  NodeFilterHolder holder(aFilter);
  return CreateTreeWalker(aRoot, aWhatToShow, holder, rv);
}

already_AddRefed<TreeWalker>
nsIDocument::CreateTreeWalker(nsINode& aRoot, uint32_t aWhatToShow,
                              const NodeFilterHolder& aFilter,
                              ErrorResult& rv) const
{
  nsINode* root = &aRoot;
  nsRefPtr<TreeWalker> walker = new TreeWalker(root, aWhatToShow, aFilter);
  return walker.forget();
}


NS_IMETHODIMP
nsDocument::GetDefaultView(nsIDOMWindow** aDefaultView)
{
  *aDefaultView = nullptr;
  nsCOMPtr<nsPIDOMWindow> win = GetWindow();
  win.forget(aDefaultView);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetLocation(nsIDOMLocation **_retval)
{
  *_retval = nsIDocument::GetLocation().take();
  return NS_OK;
}

already_AddRefed<nsLocation>
nsIDocument::GetLocation() const
{
  nsCOMPtr<nsIDOMWindow> w = do_QueryInterface(mScriptGlobalObject);

  if (!w) {
    return nullptr;
  }

  nsCOMPtr<nsIDOMLocation> loc;
  w->GetLocation(getter_AddRefs(loc));
  return loc.forget().downcast<nsLocation>();
}

Element*
nsIDocument::GetHtmlElement() const
{
  Element* rootElement = GetRootElement();
  if (rootElement && rootElement->IsHTMLElement(nsGkAtoms::html))
    return rootElement;
  return nullptr;
}

Element*
nsIDocument::GetHtmlChildElement(nsIAtom* aTag)
{
  Element* html = GetHtmlElement();
  if (!html)
    return nullptr;

  
  
  for (nsIContent* child = html->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsHTMLElement(aTag))
      return child->AsElement();
  }
  return nullptr;
}

nsIContent*
nsDocument::GetTitleContent(uint32_t aNamespace)
{
  
  
  
  
  
  if (!mMayHaveTitleElement)
    return nullptr;

  nsRefPtr<nsContentList> list =
    NS_GetContentList(this, aNamespace, NS_LITERAL_STRING("title"));

  return list->Item(0, false);
}

void
nsDocument::GetTitleFromElement(uint32_t aNamespace, nsAString& aTitle)
{
  nsIContent* title = GetTitleContent(aNamespace);
  if (!title)
    return;
  if(!nsContentUtils::GetNodeTextContent(title, false, aTitle))
    NS_RUNTIMEABORT("OOM");
}

NS_IMETHODIMP
nsDocument::GetTitle(nsAString& aTitle)
{
  nsString title;
  GetTitle(title);
  aTitle = title;
  return NS_OK;
}

void
nsDocument::GetTitle(nsString& aTitle)
{
  aTitle.Truncate();

  nsIContent *rootElement = GetRootElement();
  if (!rootElement)
    return;

  nsAutoString tmp;

  switch (rootElement->GetNameSpaceID()) {
#ifdef MOZ_XUL
    case kNameSpaceID_XUL:
      rootElement->GetAttr(kNameSpaceID_None, nsGkAtoms::title, tmp);
      break;
#endif
    case kNameSpaceID_SVG:
      if (rootElement->IsSVGElement(nsGkAtoms::svg)) {
        GetTitleFromElement(kNameSpaceID_SVG, tmp);
        break;
      } 
    default:
      GetTitleFromElement(kNameSpaceID_XHTML, tmp);
      break;
  }

  tmp.CompressWhitespace();
  aTitle = tmp;
}

NS_IMETHODIMP
nsDocument::SetTitle(const nsAString& aTitle)
{
  Element *rootElement = GetRootElement();
  if (!rootElement)
    return NS_OK;

  switch (rootElement->GetNameSpaceID()) {
    case kNameSpaceID_SVG:
      return NS_OK; 
#ifdef MOZ_XUL
    case kNameSpaceID_XUL:
      return rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::title,
                                  aTitle, true);
#endif
  }

  
  
  mozAutoDocUpdate updateBatch(this, UPDATE_CONTENT_MODEL, true);

  nsIContent* title = GetTitleContent(kNameSpaceID_XHTML);
  if (!title) {
    Element *head = GetHeadElement();
    if (!head)
      return NS_OK;

    {
      nsRefPtr<mozilla::dom::NodeInfo> titleInfo;
      titleInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::title, nullptr,
                                                kNameSpaceID_XHTML,
                                                nsIDOMNode::ELEMENT_NODE);
      title = NS_NewHTMLTitleElement(titleInfo.forget());
      if (!title)
        return NS_OK;
    }

    head->AppendChildTo(title, true);
  }

  return nsContentUtils::SetNodeTextContent(title, aTitle, false);
}

void
nsDocument::SetTitle(const nsAString& aTitle, ErrorResult& rv)
{
  rv = SetTitle(aTitle);
}

void
nsDocument::NotifyPossibleTitleChange(bool aBoundTitleElement)
{
  NS_ASSERTION(!mInUnlinkOrDeletion || !aBoundTitleElement,
               "Setting a title while unlinking or destroying the element?");
  if (mInUnlinkOrDeletion) {
    return;
  }

  if (aBoundTitleElement) {
    mMayHaveTitleElement = true;
  }
  if (mPendingTitleChangeEvent.IsPending())
    return;

  nsRefPtr<nsRunnableMethod<nsDocument, void, false> > event =
    NS_NewNonOwningRunnableMethod(this,
      &nsDocument::DoNotifyPossibleTitleChange);
  nsresult rv = NS_DispatchToCurrentThread(event);
  if (NS_SUCCEEDED(rv)) {
    mPendingTitleChangeEvent = event;
  }
}

void
nsDocument::DoNotifyPossibleTitleChange()
{
  mPendingTitleChangeEvent.Forget();
  mHaveFiredTitleChange = true;

  nsAutoString title;
  GetTitle(title);

  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    nsCOMPtr<nsISupports> container =
      shell->GetPresContext()->GetContainerWeak();
    if (container) {
      nsCOMPtr<nsIBaseWindow> docShellWin = do_QueryInterface(container);
      if (docShellWin) {
        docShellWin->SetTitle(title.get());
      }
    }
  }

  
  nsContentUtils::DispatchChromeEvent(this, static_cast<nsIDocument*>(this),
                                      NS_LITERAL_STRING("DOMTitleChanged"),
                                      true, true);
}

already_AddRefed<BoxObject>
nsDocument::GetBoxObjectFor(Element* aElement, ErrorResult& aRv)
{
  if (!aElement) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsIDocument* doc = aElement->OwnerDoc();
  if (doc != this) {
    aRv.Throw(NS_ERROR_DOM_WRONG_DOCUMENT_ERR);
    return nullptr;
  }

  if (!mHasWarnedAboutBoxObjects && !aElement->IsXULElement()) {
    mHasWarnedAboutBoxObjects = true;
    nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                    NS_LITERAL_CSTRING("BoxObjects"), this,
                                    nsContentUtils::eDOM_PROPERTIES,
                                    "UseOfGetBoxObjectForWarning");
  }

  if (!mBoxObjectTable) {
    mBoxObjectTable = new nsInterfaceHashtable<nsPtrHashKey<nsIContent>, nsPIBoxObject>(6);
  } else {
    nsCOMPtr<nsPIBoxObject> boxObject = mBoxObjectTable->Get(aElement);
    if (boxObject) {
      return boxObject.forget().downcast<BoxObject>();
    }
  }

  int32_t namespaceID;
  nsCOMPtr<nsIAtom> tag = BindingManager()->ResolveTag(aElement, &namespaceID);

  nsAutoCString contractID("@mozilla.org/layout/xul-boxobject");
  if (namespaceID == kNameSpaceID_XUL) {
    if (tag == nsGkAtoms::browser ||
        tag == nsGkAtoms::editor ||
        tag == nsGkAtoms::iframe)
      contractID += "-container";
    else if (tag == nsGkAtoms::menu)
      contractID += "-menu";
    else if (tag == nsGkAtoms::popup ||
             tag == nsGkAtoms::menupopup ||
             tag == nsGkAtoms::panel ||
             tag == nsGkAtoms::tooltip)
      contractID += "-popup";
    else if (tag == nsGkAtoms::tree)
      contractID += "-tree";
    else if (tag == nsGkAtoms::listbox)
      contractID += "-listbox";
    else if (tag == nsGkAtoms::scrollbox)
      contractID += "-scrollbox";
  }
  contractID += ";1";

  nsCOMPtr<nsPIBoxObject> boxObject(do_CreateInstance(contractID.get()));
  if (!boxObject) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  boxObject->Init(aElement);

  if (mBoxObjectTable) {
    mBoxObjectTable->Put(aElement, boxObject.get());
  }

  return boxObject.forget().downcast<BoxObject>();
}

void
nsDocument::ClearBoxObjectFor(nsIContent* aContent)
{
  if (mBoxObjectTable) {
    nsPIBoxObject *boxObject = mBoxObjectTable->GetWeak(aContent);
    if (boxObject) {
      boxObject->Clear();
      mBoxObjectTable->Remove(aContent);
    }
  }
}

already_AddRefed<MediaQueryList>
nsIDocument::MatchMedia(const nsAString& aMediaQueryList)
{
  nsRefPtr<MediaQueryList> result = new MediaQueryList(this, aMediaQueryList);

  
  PR_INSERT_BEFORE(result, &mDOMMediaQueryLists);

  return result.forget();
}

void
nsDocument::FlushSkinBindings()
{
  BindingManager()->FlushSkinBindings();
}

nsresult
nsDocument::InitializeFrameLoader(nsFrameLoader* aLoader)
{
  mInitializableFrameLoaders.RemoveElement(aLoader);
  
  if (mInDestructor) {
    NS_WARNING("Trying to initialize a frame loader while"
               "document is being deleted");
    return NS_ERROR_FAILURE;
  }

  mInitializableFrameLoaders.AppendElement(aLoader);
  if (!mFrameLoaderRunner) {
    mFrameLoaderRunner =
      NS_NewRunnableMethod(this, &nsDocument::MaybeInitializeFinalizeFrameLoaders);
    NS_ENSURE_TRUE(mFrameLoaderRunner, NS_ERROR_OUT_OF_MEMORY);
    nsContentUtils::AddScriptRunner(mFrameLoaderRunner);
  }
  return NS_OK;
}

nsresult
nsDocument::FinalizeFrameLoader(nsFrameLoader* aLoader, nsIRunnable* aFinalizer)
{
  mInitializableFrameLoaders.RemoveElement(aLoader);
  if (mInDestructor) {
    return NS_ERROR_FAILURE;
  }

  mFrameLoaderFinalizers.AppendElement(aFinalizer);
  if (!mFrameLoaderRunner) {
    mFrameLoaderRunner =
      NS_NewRunnableMethod(this, &nsDocument::MaybeInitializeFinalizeFrameLoaders);
    NS_ENSURE_TRUE(mFrameLoaderRunner, NS_ERROR_OUT_OF_MEMORY);
    nsContentUtils::AddScriptRunner(mFrameLoaderRunner);
  }
  return NS_OK;
}

void
nsDocument::MaybeInitializeFinalizeFrameLoaders()
{
  if (mDelayFrameLoaderInitialization || mUpdateNestLevel != 0) {
    
    
    mFrameLoaderRunner = nullptr;
    return;
  }

  
  
  if (!nsContentUtils::IsSafeToRunScript()) {
    if (!mInDestructor && !mFrameLoaderRunner &&
        (mInitializableFrameLoaders.Length() ||
         mFrameLoaderFinalizers.Length())) {
      mFrameLoaderRunner =
        NS_NewRunnableMethod(this, &nsDocument::MaybeInitializeFinalizeFrameLoaders);
      nsContentUtils::AddScriptRunner(mFrameLoaderRunner);
    }
    return;
  }
  mFrameLoaderRunner = nullptr;

  
  
  
  while (mInitializableFrameLoaders.Length()) {
    nsRefPtr<nsFrameLoader> loader = mInitializableFrameLoaders[0];
    mInitializableFrameLoaders.RemoveElementAt(0);
    NS_ASSERTION(loader, "null frameloader in the array?");
    loader->ReallyStartLoading();
  }

  uint32_t length = mFrameLoaderFinalizers.Length();
  if (length > 0) {
    nsTArray<nsCOMPtr<nsIRunnable> > finalizers;
    mFrameLoaderFinalizers.SwapElements(finalizers);
    for (uint32_t i = 0; i < length; ++i) {
      finalizers[i]->Run();
    }
  }
}

void
nsDocument::TryCancelFrameLoaderInitialization(nsIDocShell* aShell)
{
  uint32_t length = mInitializableFrameLoaders.Length();
  for (uint32_t i = 0; i < length; ++i) {
    if (mInitializableFrameLoaders[i]->GetExistingDocShell() == aShell) {
      mInitializableFrameLoaders.RemoveElementAt(i);
      return;
    }
  }
}

nsIDocument*
nsDocument::RequestExternalResource(nsIURI* aURI,
                                    nsINode* aRequestingNode,
                                    ExternalResourceLoad** aPendingLoad)
{
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aRequestingNode, "Must have a node");
  if (mDisplayDocument) {
    return mDisplayDocument->RequestExternalResource(aURI,
                                                     aRequestingNode,
                                                     aPendingLoad);
  }

  return mExternalResourceMap.RequestResource(aURI, aRequestingNode,
                                              this, aPendingLoad);
}

void
nsDocument::EnumerateExternalResources(nsSubDocEnumFunc aCallback, void* aData)
{
  mExternalResourceMap.EnumerateResources(aCallback, aData);
}

nsSMILAnimationController*
nsDocument::GetAnimationController()
{
  
  
  if (mAnimationController)
    return mAnimationController;
  
  if (mLoadedAsData || mLoadedAsInteractiveData)
    return nullptr;

  mAnimationController = new nsSMILAnimationController(this);

  
  
  nsIPresShell *shell = GetShell();
  if (mAnimationController && shell) {
    nsPresContext *context = shell->GetPresContext();
    if (context &&
        context->ImageAnimationMode() == imgIContainer::kDontAnimMode) {
      mAnimationController->Pause(nsSMILTimeContainer::PAUSE_USERPREF);
    }
  }

  
  
  
  if (!mIsShowing && !mIsBeingUsedAsImage) {
    mAnimationController->OnPageHide();
  }

  return mAnimationController;
}

PendingAnimationTracker*
nsDocument::GetOrCreatePendingAnimationTracker()
{
  if (!mPendingAnimationTracker) {
    mPendingAnimationTracker = new PendingAnimationTracker(this);
  }

  return mPendingAnimationTracker;
}






NS_IMETHODIMP
nsDocument::GetDir(nsAString& aDirection)
{
  nsIDocument::GetDir(aDirection);
  return NS_OK;
}

void
nsIDocument::GetDir(nsAString& aDirection) const
{
  aDirection.Truncate();
  Element* rootElement = GetHtmlElement();
  if (rootElement) {
    static_cast<nsGenericHTMLElement*>(rootElement)->GetDir(aDirection);
  }
}






NS_IMETHODIMP
nsDocument::SetDir(const nsAString& aDirection)
{
  nsIDocument::SetDir(aDirection);
  return NS_OK;
}

void
nsIDocument::SetDir(const nsAString& aDirection)
{
  Element* rootElement = GetHtmlElement();
  if (rootElement) {
    rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::dir,
                         aDirection, true);
  }
}

NS_IMETHODIMP
nsDocument::GetInputEncoding(nsAString& aInputEncoding)
{
  nsIDocument::GetInputEncoding(aInputEncoding);
  return NS_OK;
}

void
nsIDocument::GetInputEncoding(nsAString& aInputEncoding)
{
  
  WarnOnceAbout(eInputEncoding);
  if (mHaveInputEncoding) {
    return GetCharacterSet(aInputEncoding);
  }

  SetDOMStringToNull(aInputEncoding);
}

NS_IMETHODIMP
nsDocument::GetMozSyntheticDocument(bool *aSyntheticDocument)
{
  *aSyntheticDocument = mIsSyntheticDocument;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentURI(nsAString& aDocumentURI)
{
  nsString temp;
  nsIDocument::GetDocumentURI(temp);
  aDocumentURI = temp;
  return NS_OK;
}

void
nsIDocument::GetDocumentURI(nsString& aDocumentURI) const
{
  if (mDocumentURI) {
    nsAutoCString uri;
    mDocumentURI->GetSpec(uri);
    CopyUTF8toUTF16(uri, aDocumentURI);
  } else {
    aDocumentURI.Truncate();
  }
}


NS_IMETHODIMP
nsDocument::GetURL(nsAString& aURL)
{
  return GetDocumentURI(aURL);
}

void
nsIDocument::GetURL(nsString& aURL) const
{
  return GetDocumentURI(aURL);
}

void
nsIDocument::GetDocumentURIFromJS(nsString& aDocumentURI) const
{
  if (!mChromeXHRDocURI || !nsContentUtils::IsCallerChrome()) {
    return GetDocumentURI(aDocumentURI);
  }

  nsAutoCString uri;
  mChromeXHRDocURI->GetSpec(uri);
  CopyUTF8toUTF16(uri, aDocumentURI);
}

nsIURI*
nsIDocument::GetDocumentURIObject() const
{
  if (!mChromeXHRDocURI) {
    return GetDocumentURI();
  }

  return mChromeXHRDocURI;
}






NS_IMETHODIMP
nsDocument::GetCompatMode(nsAString& aCompatMode)
{
  nsString temp;
  nsIDocument::GetCompatMode(temp);
  aCompatMode = temp;
  return NS_OK;
}

void
nsIDocument::GetCompatMode(nsString& aCompatMode) const
{
  NS_ASSERTION(mCompatMode == eCompatibility_NavQuirks ||
               mCompatMode == eCompatibility_AlmostStandards ||
               mCompatMode == eCompatibility_FullStandards,
               "mCompatMode is neither quirks nor strict for this document");

  if (mCompatMode == eCompatibility_NavQuirks) {
    aCompatMode.AssignLiteral("BackCompat");
  } else {
    aCompatMode.AssignLiteral("CSS1Compat");
  }
}

static void BlastSubtreeToPieces(nsINode *aNode);

PLDHashOperator
BlastFunc(nsAttrHashKey::KeyType aKey, Attr *aData, void* aUserArg)
{
  nsCOMPtr<nsIAttribute> *attr =
    static_cast<nsCOMPtr<nsIAttribute>*>(aUserArg);

  *attr = aData;

  NS_ASSERTION(attr->get(),
               "non-nsIAttribute somehow made it into the hashmap?!");

  return PL_DHASH_STOP;
}

static void
BlastSubtreeToPieces(nsINode *aNode)
{
  if (aNode->IsElement()) {
    Element *element = aNode->AsElement();
    const nsDOMAttributeMap *map = element->GetAttributeMap();
    if (map) {
      nsCOMPtr<nsIAttribute> attr;
      while (map->Enumerate(BlastFunc, &attr) > 0) {
        BlastSubtreeToPieces(attr);

#ifdef DEBUG
        nsresult rv =
#endif
          element->UnsetAttr(attr->NodeInfo()->NamespaceID(),
                             attr->NodeInfo()->NameAtom(),
                             false);

        
        NS_ASSERTION(NS_SUCCEEDED(rv), "Uhoh, UnsetAttr shouldn't fail!");
      }
    }
  }

  uint32_t count = aNode->GetChildCount();
  for (uint32_t i = 0; i < count; ++i) {
    BlastSubtreeToPieces(aNode->GetFirstChild());
    aNode->RemoveChildAt(0, false);
  }
}

NS_IMETHODIMP
nsDocument::AdoptNode(nsIDOMNode *aAdoptedNode, nsIDOMNode **aResult)
{
  *aResult = nullptr;

  nsCOMPtr<nsINode> adoptedNode = do_QueryInterface(aAdoptedNode);
  NS_ENSURE_TRUE(adoptedNode, NS_ERROR_UNEXPECTED);

  ErrorResult rv;
  nsINode* result = nsIDocument::AdoptNode(*adoptedNode, rv);
  if (rv.Failed()) {
    return rv.StealNSResult();
  }

  NS_ADDREF(*aResult = result->AsDOMNode());
  return NS_OK;
}

nsINode*
nsIDocument::AdoptNode(nsINode& aAdoptedNode, ErrorResult& rv)
{
  nsINode* adoptedNode = &aAdoptedNode;

  
  
  {
    nsINode* parent = adoptedNode->GetParentNode();
    if (parent) {
      nsContentUtils::MaybeFireNodeRemoved(adoptedNode, parent,
                                           adoptedNode->OwnerDoc());
    }
  }

  nsAutoScriptBlocker scriptBlocker;

  switch (adoptedNode->NodeType()) {
    case nsIDOMNode::ATTRIBUTE_NODE:
    {
      
      nsRefPtr<Attr> adoptedAttr = static_cast<Attr*>(adoptedNode);

      nsCOMPtr<Element> ownerElement = adoptedAttr->GetOwnerElement(rv);
      if (rv.Failed()) {
        return nullptr;
      }

      if (ownerElement) {
        nsRefPtr<Attr> newAttr =
          ownerElement->RemoveAttributeNode(*adoptedAttr, rv);
        if (rv.Failed()) {
          return nullptr;
        }

        newAttr.swap(adoptedAttr);
      }

      break;
    }
    case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    case nsIDOMNode::ELEMENT_NODE:
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    case nsIDOMNode::TEXT_NODE:
    case nsIDOMNode::CDATA_SECTION_NODE:
    case nsIDOMNode::COMMENT_NODE:
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
    {
      
      if (adoptedNode->AsContent()->IsRootOfAnonymousSubtree()) {
        rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
        return nullptr;
      }

      
      
      
      
      nsIDocument *doc = this;
      do {
        nsPIDOMWindow *win = doc->GetWindow();
        if (win) {
          nsCOMPtr<nsINode> node = win->GetFrameElementInternal();
          if (node &&
              nsContentUtils::ContentIsDescendantOf(node, adoptedNode)) {
            rv.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
            return nullptr;
          }
        }
      } while ((doc = doc->GetParentDocument()));

      
      nsCOMPtr<nsINode> parent = adoptedNode->GetParentNode();
      if (parent) {
        int32_t idx = parent->IndexOf(adoptedNode);
        MOZ_ASSERT(idx >= 0);
        parent->RemoveChildAt(idx, true);
      } else {
        MOZ_ASSERT(!adoptedNode->IsInDoc());

        
        
        
        
        adoptedNode->AsContent()->SetXBLBinding(nullptr);
      }

      break;
    }
    case nsIDOMNode::DOCUMENT_NODE:
    {
      rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return nullptr;
    }
    default:
    {
      NS_WARNING("Don't know how to adopt this nodetype for adoptNode.");

      rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return nullptr;
    }
  }

  nsCOMPtr<nsIDocument> oldDocument = adoptedNode->OwnerDoc();
  bool sameDocument = oldDocument == this;

  AutoJSContext cx;
  JS::Rooted<JSObject*> newScope(cx, nullptr);
  if (!sameDocument) {
    newScope = GetWrapper();
    if (!newScope && GetScopeObject() && GetScopeObject()->GetGlobalJSObject()) {
      
      
      
      
      JSAutoCompartment ac(cx, GetScopeObject()->GetGlobalJSObject());
      JS::Rooted<JS::Value> v(cx);
      rv = nsContentUtils::WrapNative(cx, this, this, &v,
                                       false);
      if (rv.Failed())
        return nullptr;
      newScope = &v.toObject();
    }
  }

  nsCOMArray<nsINode> nodesWithProperties;
  rv = nsNodeUtils::Adopt(adoptedNode, sameDocument ? nullptr : mNodeInfoManager,
                          newScope, nodesWithProperties);
  if (rv.Failed()) {
    
    
    BlastSubtreeToPieces(adoptedNode);

    if (!sameDocument && oldDocument) {
      uint32_t count = nodesWithProperties.Count();
      for (uint32_t j = 0; j < oldDocument->GetPropertyTableCount(); ++j) {
        for (uint32_t i = 0; i < count; ++i) {
          
          oldDocument->PropertyTable(j)->
            DeleteAllPropertiesFor(nodesWithProperties[i]);
        }
      }
    }

    return nullptr;
  }

  uint32_t count = nodesWithProperties.Count();
  if (!sameDocument && oldDocument) {
    for (uint32_t j = 0; j < oldDocument->GetPropertyTableCount(); ++j) {
      nsPropertyTable *oldTable = oldDocument->PropertyTable(j);
      nsPropertyTable *newTable = PropertyTable(j);
      for (uint32_t i = 0; i < count; ++i) {
        rv = oldTable->TransferOrDeleteAllPropertiesFor(nodesWithProperties[i],
                                                        newTable);
      }
    }

    if (rv.Failed()) {
      
      BlastSubtreeToPieces(adoptedNode);

      return nullptr;
    }
  }

  NS_ASSERTION(adoptedNode->OwnerDoc() == this,
               "Should still be in the document we just got adopted into");

  return adoptedNode;
}

nsViewportInfo
nsDocument::GetViewportInfo(const ScreenIntSize& aDisplaySize)
{
  
  
  nsPresContext* context = mPresShell->GetPresContext();
  float fullZoom = context ? context->GetFullZoom() : 1.0;
  fullZoom = (fullZoom == 0.0) ? 1.0 : fullZoom;
  nsIWidget *widget = nsContentUtils::WidgetForDocument(this);
  float widgetScale = widget ? widget->GetDefaultScale().scale : 1.0f;
  CSSToLayoutDeviceScale layoutDeviceScale(widgetScale * fullZoom);

  CSSToScreenScale defaultScale = layoutDeviceScale
                                * LayoutDeviceToScreenScale(1.0);
  
  nsPIDOMWindow* win = GetWindow();
  if (win && win->IsDesktopModeViewport())
  {
    return nsViewportInfo(aDisplaySize,
                          defaultScale,
                          false,
                           true);
  }

  if (!Preferences::GetBool("dom.meta-viewport.enabled", false)) {
    return nsViewportInfo(aDisplaySize,
                          defaultScale,
                           false,
                           true);
  }

  
  
  

  switch (mViewportType) {
  case DisplayWidthHeight:
    return nsViewportInfo(aDisplaySize,
                          defaultScale,
                           true,
                           true);
  case DisplayWidthHeightNoZoom:
    return nsViewportInfo(aDisplaySize,
                          defaultScale,
                           false,
                           false);
  case Unknown:
  {
    nsAutoString viewport;
    GetHeaderData(nsGkAtoms::viewport, viewport);
    if (viewport.IsEmpty()) {
      
      
      nsCOMPtr<nsIDOMDocumentType> docType;
      nsresult rv = GetDoctype(getter_AddRefs(docType));
      if (NS_SUCCEEDED(rv) && docType) {
        nsAutoString docId;
        rv = docType->GetPublicId(docId);
        if (NS_SUCCEEDED(rv)) {
          if ((docId.Find("WAP") != -1) ||
              (docId.Find("Mobile") != -1) ||
              (docId.Find("WML") != -1))
          {
            
            mViewportType = DisplayWidthHeight;
            return nsViewportInfo(aDisplaySize,
                                  defaultScale,
                                  true,
                                  false);
          }
        }
      }

      nsAutoString handheldFriendly;
      GetHeaderData(nsGkAtoms::handheldFriendly, handheldFriendly);
      if (handheldFriendly.EqualsLiteral("true")) {
        mViewportType = DisplayWidthHeight;
        return nsViewportInfo(aDisplaySize,
                              defaultScale,
                              true,
                              false);
      }
    }

    nsAutoString minScaleStr;
    GetHeaderData(nsGkAtoms::viewport_minimum_scale, minScaleStr);

    nsresult errorCode;
    mScaleMinFloat = LayoutDeviceToScreenScale(minScaleStr.ToFloat(&errorCode));

    if (NS_FAILED(errorCode)) {
      mScaleMinFloat = kViewportMinScale;
    }

    mScaleMinFloat = mozilla::clamped(
        mScaleMinFloat, kViewportMinScale, kViewportMaxScale);

    nsAutoString maxScaleStr;
    GetHeaderData(nsGkAtoms::viewport_maximum_scale, maxScaleStr);

    
    
    nsresult scaleMaxErrorCode;
    mScaleMaxFloat = LayoutDeviceToScreenScale(maxScaleStr.ToFloat(&scaleMaxErrorCode));

    if (NS_FAILED(scaleMaxErrorCode)) {
      mScaleMaxFloat = kViewportMaxScale;
    }

    mScaleMaxFloat = mozilla::clamped(
        mScaleMaxFloat, kViewportMinScale, kViewportMaxScale);

    nsAutoString scaleStr;
    GetHeaderData(nsGkAtoms::viewport_initial_scale, scaleStr);

    nsresult scaleErrorCode;
    mScaleFloat = LayoutDeviceToScreenScale(scaleStr.ToFloat(&scaleErrorCode));

    nsAutoString widthStr, heightStr;

    GetHeaderData(nsGkAtoms::viewport_height, heightStr);
    GetHeaderData(nsGkAtoms::viewport_width, widthStr);

    mAutoSize = false;

    if (widthStr.EqualsLiteral("device-width")) {
      mAutoSize = true;
    }

    if (widthStr.IsEmpty() &&
        (heightStr.EqualsLiteral("device-height") ||
         (mScaleFloat.scale == 1.0)))
    {
      mAutoSize = true;
    }

    nsresult widthErrorCode, heightErrorCode;
    mViewportSize.width = widthStr.ToInteger(&widthErrorCode);
    mViewportSize.height = heightStr.ToInteger(&heightErrorCode);

    
    
    mValidWidth = (!widthStr.IsEmpty() && NS_SUCCEEDED(widthErrorCode) && mViewportSize.width > 0);
    mValidHeight = (!heightStr.IsEmpty() && NS_SUCCEEDED(heightErrorCode) && mViewportSize.height > 0);

    mAllowZoom = true;
    nsAutoString userScalable;
    GetHeaderData(nsGkAtoms::viewport_user_scalable, userScalable);

    if ((userScalable.EqualsLiteral("0")) ||
        (userScalable.EqualsLiteral("no")) ||
        (userScalable.EqualsLiteral("false"))) {
      mAllowZoom = false;
    }
    mAllowDoubleTapZoom = mAllowZoom;

    mScaleStrEmpty = scaleStr.IsEmpty();
    mWidthStrEmpty = widthStr.IsEmpty();
    mValidScaleFloat = !scaleStr.IsEmpty() && NS_SUCCEEDED(scaleErrorCode);
    mValidMaxScale = !maxScaleStr.IsEmpty() && NS_SUCCEEDED(scaleMaxErrorCode);

    mViewportType = Specified;
  }
  case Specified:
  default:
    CSSSize size = mViewportSize;

    if (!mValidWidth) {
      if (mValidHeight && !aDisplaySize.IsEmpty()) {
        size.width = size.height * aDisplaySize.width / aDisplaySize.height;
      } else {
        
        
        
        size.width = Preferences::GetInt("browser.viewport.desktopWidth",
                                         kViewportDefaultScreenWidth) / fullZoom;
      }
    }

    if (!mValidHeight) {
      if (!aDisplaySize.IsEmpty()) {
        size.height = size.width * aDisplaySize.height / aDisplaySize.width;
      } else {
        size.height = size.width;
      }
    }

    CSSToScreenScale scaleFloat = mScaleFloat * layoutDeviceScale;
    CSSToScreenScale scaleMinFloat = mScaleMinFloat * layoutDeviceScale;
    CSSToScreenScale scaleMaxFloat = mScaleMaxFloat * layoutDeviceScale;

    if (mAutoSize) {
      
      CSSToScreenScale defaultPixelScale = layoutDeviceScale * LayoutDeviceToScreenScale(1.0f);
      size = ScreenSize(aDisplaySize) / defaultPixelScale;
    }

    size.width = clamped(size.width, float(kViewportMinSize.width), float(kViewportMaxSize.width));

    
    
    if (mScaleStrEmpty && !mWidthStrEmpty) {
      CSSToScreenScale defaultScale(float(aDisplaySize.width) / size.width);
      scaleFloat = (scaleFloat > defaultScale) ? scaleFloat : defaultScale;
    }

    size.height = clamped(size.height, float(kViewportMinSize.height), float(kViewportMaxSize.height));

    
    
    if (mValidScaleFloat) {
      CSSSize displaySize = ScreenSize(aDisplaySize) / scaleFloat;
      size.width = std::max(size.width, displaySize.width);
      size.height = std::max(size.height, displaySize.height);
    } else if (mValidMaxScale) {
      CSSSize displaySize = ScreenSize(aDisplaySize) / scaleMaxFloat;
      size.width = std::max(size.width, displaySize.width);
      size.height = std::max(size.height, displaySize.height);
    }

    return nsViewportInfo(scaleFloat, scaleMinFloat, scaleMaxFloat, size,
                          mAutoSize, mAllowZoom, mAllowDoubleTapZoom);
  }
}

EventListenerManager*
nsDocument::GetOrCreateListenerManager()
{
  if (!mListenerManager) {
    mListenerManager =
      new EventListenerManager(static_cast<EventTarget*>(this));
    SetFlags(NODE_HAS_LISTENERMANAGER);
  }

  return mListenerManager;
}

EventListenerManager*
nsDocument::GetExistingListenerManager() const
{
  return mListenerManager;
}

nsresult
nsDocument::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = true;
   
   
  aVisitor.mForceContentDispatch = true;

  
  if (aVisitor.mEvent->message != NS_LOAD) {
    nsGlobalWindow* window = static_cast<nsGlobalWindow*>(GetWindow());
    aVisitor.mParentTarget =
      window ? window->GetTargetForEventTargetChain() : nullptr;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateEvent(const nsAString& aEventType, nsIDOMEvent** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  ErrorResult rv;
  *aReturn = nsIDocument::CreateEvent(aEventType, rv).take();
  return rv.StealNSResult();
}

already_AddRefed<Event>
nsIDocument::CreateEvent(const nsAString& aEventType, ErrorResult& rv) const
{
  nsIPresShell *shell = GetShell();

  nsPresContext *presContext = nullptr;

  if (shell) {
    
    presContext = shell->GetPresContext();
  }

  
  nsCOMPtr<nsIDOMEvent> ev;
  rv = EventDispatcher::CreateEvent(const_cast<nsIDocument*>(this),
                                    presContext, nullptr, aEventType,
                                    getter_AddRefs(ev));
  if (!ev) {
    return nullptr;
  }
  WidgetEvent* e = ev->GetInternalNSEvent();
  e->mFlags.mBubbles = false;
  e->mFlags.mCancelable = false;
  return dont_AddRef(ev.forget().take()->InternalDOMEvent());
}

void
nsDocument::FlushPendingNotifications(mozFlushType aType)
{
  nsDocumentOnStack dos(this);

  
  
  
  
  
  
  if ((!IsHTMLDocument() ||
       (aType > Flush_ContentAndNotify && mPresShell &&
        !mPresShell->DidInitialize())) &&
      (mParser || mWeakSink)) {
    nsCOMPtr<nsIContentSink> sink;
    if (mParser) {
      sink = mParser->GetContentSink();
    } else {
      sink = do_QueryReferent(mWeakSink);
      if (!sink) {
        mWeakSink = nullptr;
      }
    }
    
    
    if (sink && (aType == Flush_Content || IsSafeToFlush())) {
      sink->FlushPendingNotifications(aType);
    }
  }

  

  if (aType <= Flush_ContentAndNotify) {
    
    return;
  }

  
  
  
  
  
  
  
  
  
  if (mParentDocument && IsSafeToFlush()) {
    mozFlushType parentType = aType;
    if (aType >= Flush_Style)
      parentType = std::max(Flush_Layout, aType);
    mParentDocument->FlushPendingNotifications(parentType);
  }

  
  
  
  
  
  
  if (mNeedStyleFlush ||
      (mNeedLayoutFlush && aType >= Flush_InterruptibleLayout) ||
      aType >= Flush_Display ||
      mInFlush) {
    nsCOMPtr<nsIPresShell> shell = GetShell();
    if (shell) {
      mNeedStyleFlush = false;
      mNeedLayoutFlush = mNeedLayoutFlush && (aType < Flush_InterruptibleLayout);
      
      
      
      bool oldInFlush = mInFlush;
      mInFlush = true;
      shell->FlushPendingNotifications(aType);
      mInFlush = oldInFlush;
    }
  }
}

static bool
Copy(nsIDocument* aDocument, void* aData)
{
  nsTArray<nsCOMPtr<nsIDocument> >* resources =
    static_cast<nsTArray<nsCOMPtr<nsIDocument> >* >(aData);
  resources->AppendElement(aDocument);
  return true;
}

void
nsDocument::FlushExternalResources(mozFlushType aType)
{
  NS_ASSERTION(aType >= Flush_Style,
    "should only need to flush for style or higher in external resources");
  if (GetDisplayDocument()) {
    return;
  }
  nsTArray<nsCOMPtr<nsIDocument> > resources;
  EnumerateExternalResources(Copy, &resources);

  for (uint32_t i = 0; i < resources.Length(); i++) {
    resources[i]->FlushPendingNotifications(aType);
  }
}

void
nsDocument::SetXMLDeclaration(const char16_t *aVersion,
                              const char16_t *aEncoding,
                              const int32_t aStandalone)
{
  if (!aVersion || *aVersion == '\0') {
    mXMLDeclarationBits = 0;
    return;
  }

  mXMLDeclarationBits = XML_DECLARATION_BITS_DECLARATION_EXISTS;

  if (aEncoding && *aEncoding != '\0') {
    mXMLDeclarationBits |= XML_DECLARATION_BITS_ENCODING_EXISTS;
  }

  if (aStandalone == 1) {
    mXMLDeclarationBits |= XML_DECLARATION_BITS_STANDALONE_EXISTS |
                           XML_DECLARATION_BITS_STANDALONE_YES;
  }
  else if (aStandalone == 0) {
    mXMLDeclarationBits |= XML_DECLARATION_BITS_STANDALONE_EXISTS;
  }
}

void
nsDocument::GetXMLDeclaration(nsAString& aVersion, nsAString& aEncoding,
                              nsAString& aStandalone)
{
  aVersion.Truncate();
  aEncoding.Truncate();
  aStandalone.Truncate();

  if (!(mXMLDeclarationBits & XML_DECLARATION_BITS_DECLARATION_EXISTS)) {
    return;
  }

  
  aVersion.AssignLiteral("1.0");

  if (mXMLDeclarationBits & XML_DECLARATION_BITS_ENCODING_EXISTS) {
    
    
    GetCharacterSet(aEncoding);
  }

  if (mXMLDeclarationBits & XML_DECLARATION_BITS_STANDALONE_EXISTS) {
    if (mXMLDeclarationBits & XML_DECLARATION_BITS_STANDALONE_YES) {
      aStandalone.AssignLiteral("yes");
    } else {
      aStandalone.AssignLiteral("no");
    }
  }
}

bool
nsDocument::IsScriptEnabled()
{
  
  
  if (mSandboxFlags & SANDBOXED_SCRIPTS) {
    return false;
  }

  nsCOMPtr<nsIScriptSecurityManager> sm(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  NS_ENSURE_TRUE(sm, false);

  nsCOMPtr<nsIScriptGlobalObject> globalObject = do_QueryInterface(GetInnerWindow());
  if (!globalObject && mMasterDocument) {
    globalObject = do_QueryInterface(mMasterDocument->GetInnerWindow());
  }
  NS_ENSURE_TRUE(globalObject && globalObject->GetGlobalJSObject(), false);

  return sm->ScriptAllowed(globalObject->GetGlobalJSObject());
}

nsRadioGroupStruct*
nsDocument::GetRadioGroupInternal(const nsAString& aName) const
{
#ifdef DEBUG
  if (IsHTMLDocument()) {
    nsAutoString lcName;
    ToLowerCase(aName, lcName);
    MOZ_ASSERT(aName == lcName);
  }
#endif

  nsRadioGroupStruct* radioGroup;
  if (!mRadioGroups.Get(aName, &radioGroup)) {
    return nullptr;
  }

  return radioGroup;
}

nsRadioGroupStruct*
nsDocument::GetRadioGroup(const nsAString& aName) const
{
  nsAutoString tmKey(aName);
  if (IsHTMLDocument()) {
    ToLowerCase(tmKey); 
  }

  return GetRadioGroupInternal(tmKey);
}

nsRadioGroupStruct*
nsDocument::GetOrCreateRadioGroup(const nsAString& aName)
{
  nsAutoString tmKey(aName);
  if (IsHTMLDocument()) {
    ToLowerCase(tmKey); 
  }

  if (nsRadioGroupStruct* radioGroup = GetRadioGroupInternal(tmKey)) {
    return radioGroup;
  }

  nsAutoPtr<nsRadioGroupStruct> newRadioGroup(new nsRadioGroupStruct());
  mRadioGroups.Put(tmKey, newRadioGroup);

  return newRadioGroup.forget();
}

void
nsDocument::SetCurrentRadioButton(const nsAString& aName,
                                  HTMLInputElement* aRadio)
{
  nsRadioGroupStruct* radioGroup = GetOrCreateRadioGroup(aName);
  radioGroup->mSelectedRadioButton = aRadio;
}

HTMLInputElement*
nsDocument::GetCurrentRadioButton(const nsAString& aName)
{
  return GetOrCreateRadioGroup(aName)->mSelectedRadioButton;
}

NS_IMETHODIMP
nsDocument::GetNextRadioButton(const nsAString& aName,
                               const bool aPrevious,
                               HTMLInputElement* aFocusedRadio,
                               HTMLInputElement** aRadioOut)
{
  
  
  
  
  *aRadioOut = nullptr;

  nsRadioGroupStruct* radioGroup = GetOrCreateRadioGroup(aName);

  
  
  nsRefPtr<HTMLInputElement> currentRadio;
  if (aFocusedRadio) {
    currentRadio = aFocusedRadio;
  }
  else {
    currentRadio = radioGroup->mSelectedRadioButton;
    if (!currentRadio) {
      return NS_ERROR_FAILURE;
    }
  }
  int32_t index = radioGroup->mRadioButtons.IndexOf(currentRadio);
  if (index < 0) {
    return NS_ERROR_FAILURE;
  }

  int32_t numRadios = radioGroup->mRadioButtons.Count();
  nsRefPtr<HTMLInputElement> radio;
  do {
    if (aPrevious) {
      if (--index < 0) {
        index = numRadios -1;
      }
    }
    else if (++index >= numRadios) {
      index = 0;
    }
    NS_ASSERTION(static_cast<nsGenericHTMLFormElement*>(radioGroup->mRadioButtons[index])->IsHTMLElement(nsGkAtoms::input),
                 "mRadioButtons holding a non-radio button");
    radio = static_cast<HTMLInputElement*>(radioGroup->mRadioButtons[index]);
  } while (radio->Disabled() && radio != currentRadio);

  radio.forget(aRadioOut);
  return NS_OK;
}

void
nsDocument::AddToRadioGroup(const nsAString& aName,
                            nsIFormControl* aRadio)
{
  nsRadioGroupStruct* radioGroup = GetOrCreateRadioGroup(aName);
  radioGroup->mRadioButtons.AppendObject(aRadio);

  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements");
  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    radioGroup->mRequiredRadioCount++;
  }
}

void
nsDocument::RemoveFromRadioGroup(const nsAString& aName,
                                 nsIFormControl* aRadio)
{
  nsRadioGroupStruct* radioGroup = GetOrCreateRadioGroup(aName);
  radioGroup->mRadioButtons.RemoveObject(aRadio);

  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements");
  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    NS_ASSERTION(radioGroup->mRequiredRadioCount != 0,
                 "mRequiredRadioCount about to wrap below 0!");
    radioGroup->mRequiredRadioCount--;
  }
}

NS_IMETHODIMP
nsDocument::WalkRadioGroup(const nsAString& aName,
                           nsIRadioVisitor* aVisitor,
                           bool aFlushContent)
{
  nsRadioGroupStruct* radioGroup = GetOrCreateRadioGroup(aName);

  for (int i = 0; i < radioGroup->mRadioButtons.Count(); i++) {
    if (!aVisitor->Visit(radioGroup->mRadioButtons[i])) {
      return NS_OK;
    }
  }

  return NS_OK;
}

uint32_t
nsDocument::GetRequiredRadioCount(const nsAString& aName) const
{
  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);
  return radioGroup ? radioGroup->mRequiredRadioCount : 0;
}

void
nsDocument::RadioRequiredWillChange(const nsAString& aName, bool aRequiredAdded)
{
  nsRadioGroupStruct* radioGroup = GetOrCreateRadioGroup(aName);

  if (aRequiredAdded) {
    radioGroup->mRequiredRadioCount++;
  } else {
    NS_ASSERTION(radioGroup->mRequiredRadioCount != 0,
                 "mRequiredRadioCount about to wrap below 0!");
    radioGroup->mRequiredRadioCount--;
  }
}

bool
nsDocument::GetValueMissingState(const nsAString& aName) const
{
  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);
  return radioGroup && radioGroup->mGroupSuffersFromValueMissing;
}

void
nsDocument::SetValueMissingState(const nsAString& aName, bool aValue)
{
  nsRadioGroupStruct* radioGroup = GetOrCreateRadioGroup(aName);
  radioGroup->mGroupSuffersFromValueMissing = aValue;
}

void
nsDocument::RetrieveRelevantHeaders(nsIChannel *aChannel)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);
  PRTime modDate = 0;
  nsresult rv;

  if (httpChannel) {
    nsAutoCString tmp;
    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("last-modified"),
                                        tmp);

    if (NS_SUCCEEDED(rv)) {
      PRTime time;
      PRStatus st = PR_ParseTimeString(tmp.get(), true, &time);
      if (st == PR_SUCCESS) {
        modDate = time;
      }
    }

    
    rv = httpChannel->GetRequestHeader(NS_LITERAL_CSTRING("referer"),
                                       mReferrer);
    if (NS_FAILED(rv)) {
      mReferrer.Truncate();
    }

    static const char *const headers[] = {
      "default-style",
      "content-style-type",
      "content-language",
      "content-disposition",
      "refresh",
      "x-dns-prefetch-control",
      "x-frame-options",
      
      
      
      0
    };

    nsAutoCString headerVal;
    const char *const *name = headers;
    while (*name) {
      rv =
        httpChannel->GetResponseHeader(nsDependentCString(*name), headerVal);
      if (NS_SUCCEEDED(rv) && !headerVal.IsEmpty()) {
        nsCOMPtr<nsIAtom> key = do_GetAtom(*name);
        SetHeaderData(key, NS_ConvertASCIItoUTF16(headerVal));
      }
      ++name;
    }
  } else {
    nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(aChannel);
    if (fileChannel) {
      nsCOMPtr<nsIFile> file;
      fileChannel->GetFile(getter_AddRefs(file));
      if (file) {
        PRTime msecs;
        rv = file->GetLastModifiedTime(&msecs);

        if (NS_SUCCEEDED(rv)) {
          modDate = msecs * int64_t(PR_USEC_PER_MSEC);
        }
      }
    } else {
      nsAutoCString contentDisp;
      rv = aChannel->GetContentDispositionHeader(contentDisp);
      if (NS_SUCCEEDED(rv)) {
        SetHeaderData(nsGkAtoms::headerContentDisposition,
                      NS_ConvertASCIItoUTF16(contentDisp));
      }
    }
  }

  mLastModified.Truncate();
  if (modDate != 0) {
    GetFormattedTimeString(modDate, mLastModified);
  }
}

nsresult
nsDocument::CreateElem(const nsAString& aName, nsIAtom *aPrefix, int32_t aNamespaceID,
                       nsIContent **aResult)
{
#ifdef DEBUG
  nsAutoString qName;
  if (aPrefix) {
    aPrefix->ToString(qName);
    qName.Append(':');
  }
  qName.Append(aName);

  
  
  
  bool nsAware = aPrefix != nullptr || aNamespaceID != GetDefaultNamespaceID();
  NS_ASSERTION(NS_SUCCEEDED(nsContentUtils::CheckQName(qName, nsAware)),
               "Don't pass invalid prefixes to nsDocument::CreateElem, "
               "check caller.");
#endif

  *aResult = nullptr;

  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  mNodeInfoManager->GetNodeInfo(aName, aPrefix, aNamespaceID,
                                nsIDOMNode::ELEMENT_NODE,
                                getter_AddRefs(nodeInfo));
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<Element> element;
  nsresult rv = NS_NewElement(getter_AddRefs(element), nodeInfo.forget(),
                              NOT_FROM_PARSER);
  element.forget(aResult);
  return rv;
}

bool
nsDocument::IsSafeToFlush() const
{
  nsIPresShell* shell = GetShell();
  if (!shell)
    return true;

  return shell->IsSafeToFlush();
}

void
nsDocument::Sanitize()
{
  
  
  
  
  

  
  

  nsRefPtr<nsContentList> nodes = GetElementsByTagName(NS_LITERAL_STRING("input"));

  nsCOMPtr<nsIContent> item;
  nsAutoString value;

  uint32_t length = nodes->Length(true);
  for (uint32_t i = 0; i < length; ++i) {
    NS_ASSERTION(nodes->Item(i), "null item in node list!");

    nsRefPtr<HTMLInputElement> input = HTMLInputElement::FromContentOrNull(nodes->Item(i));
    if (!input)
      continue;

    bool resetValue = false;

    input->GetAttribute(NS_LITERAL_STRING("autocomplete"), value);
    if (value.LowerCaseEqualsLiteral("off")) {
      resetValue = true;
    } else {
      input->GetType(value);
      if (value.LowerCaseEqualsLiteral("password"))
        resetValue = true;
    }

    if (resetValue) {
      input->Reset();
    }
  }

  
  nodes = GetElementsByTagName(NS_LITERAL_STRING("form"));

  length = nodes->Length(true);
  for (uint32_t i = 0; i < length; ++i) {
    NS_ASSERTION(nodes->Item(i), "null item in nodelist");

    nsCOMPtr<nsIDOMHTMLFormElement> form = do_QueryInterface(nodes->Item(i));
    if (!form)
      continue;

    nodes->Item(i)->AsElement()->GetAttr(kNameSpaceID_None,
                                         nsGkAtoms::autocomplete, value);
    if (value.LowerCaseEqualsLiteral("off"))
      form->Reset();
  }
}

struct SubDocEnumArgs
{
  nsIDocument::nsSubDocEnumFunc callback;
  void *data;
};

static PLDHashOperator
SubDocHashEnum(PLDHashTable *table, PLDHashEntryHdr *hdr,
               uint32_t number, void *arg)
{
  SubDocMapEntry *entry = static_cast<SubDocMapEntry*>(hdr);
  SubDocEnumArgs *args = static_cast<SubDocEnumArgs*>(arg);

  nsIDocument *subdoc = entry->mSubDocument;
  bool next = subdoc ? args->callback(subdoc, args->data) : true;

  return next ? PL_DHASH_NEXT : PL_DHASH_STOP;
}

void
nsDocument::EnumerateSubDocuments(nsSubDocEnumFunc aCallback, void *aData)
{
  if (mSubDocuments) {
    SubDocEnumArgs args = { aCallback, aData };
    PL_DHashTableEnumerate(mSubDocuments, SubDocHashEnum, &args);
  }
}

static PLDHashOperator
CanCacheSubDocument(PLDHashTable *table, PLDHashEntryHdr *hdr,
                    uint32_t number, void *arg)
{
  SubDocMapEntry *entry = static_cast<SubDocMapEntry*>(hdr);
  bool *canCacheArg = static_cast<bool*>(arg);

  nsIDocument *subdoc = entry->mSubDocument;

  
  bool canCache = subdoc ? subdoc->CanSavePresentation(nullptr) : false;
  if (!canCache) {
    *canCacheArg = false;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

#ifdef DEBUG_bryner
#define DEBUG_PAGE_CACHE
#endif

bool
nsDocument::CanSavePresentation(nsIRequest *aNewRequest)
{
  if (EventHandlingSuppressed()) {
    return false;
  }

  nsPIDOMWindow* win = GetInnerWindow();
  if (win && win->TimeoutSuspendCount()) {
    return false;
  }

  
  nsCOMPtr<EventTarget> piTarget = do_QueryInterface(mScriptGlobalObject);
  if (piTarget) {
    EventListenerManager* manager = piTarget->GetExistingListenerManager();
    if (manager && manager->HasUnloadListeners()) {
      return false;
    }
  }

  
  nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
  if (loadGroup) {
    nsCOMPtr<nsISimpleEnumerator> requests;
    loadGroup->GetRequests(getter_AddRefs(requests));

    bool hasMore = false;

    
    
    
    nsCOMPtr<nsIChannel> baseChannel;
    nsCOMPtr<nsIMultiPartChannel> part(do_QueryInterface(aNewRequest));
    if (part) {
      part->GetBaseChannel(getter_AddRefs(baseChannel));
    }

    while (NS_SUCCEEDED(requests->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> elem;
      requests->GetNext(getter_AddRefs(elem));

      nsCOMPtr<nsIRequest> request = do_QueryInterface(elem);
      if (request && request != aNewRequest && request != baseChannel) {
#ifdef DEBUG_PAGE_CACHE
        nsAutoCString requestName, docSpec;
        request->GetName(requestName);
        if (mDocumentURI)
          mDocumentURI->GetSpec(docSpec);

        printf("document %s has request %s\n",
               docSpec.get(), requestName.get());
#endif
        return false;
      }
    }
  }

#ifdef MOZ_MEDIA_NAVIGATOR
  
  if (MediaManager::Exists() && win &&
      MediaManager::Get()->IsWindowStillActive(win->WindowID())) {
    return false;
  }
#endif 

#ifdef MOZ_WEBRTC
  
  nsCOMPtr<IPeerConnectionManager> pcManager =
    do_GetService(IPEERCONNECTION_MANAGER_CONTRACTID);

  if (pcManager && win) {
    bool active;
    pcManager->HasActivePeerConnection(win->WindowID(), &active);
    if (active) {
      return false;
    }
  }
#endif 

#ifdef MOZ_EME
  
  
  if (ContainsEMEContent()) {
    return false;
  }
#endif

  
  
  if (ContainsMSEContent()) {
    return false;
  }

  bool canCache = true;
  if (mSubDocuments)
    PL_DHashTableEnumerate(mSubDocuments, CanCacheSubDocument, &canCache);

  return canCache;
}

void
nsDocument::Destroy()
{
  
  
  if (mIsGoingAway)
    return;

  mIsGoingAway = true;

  RemovedFromDocShell();

  bool oldVal = mInUnlinkOrDeletion;
  mInUnlinkOrDeletion = true;
  uint32_t i, count = mChildren.ChildCount();
  for (i = 0; i < count; ++i) {
    mChildren.ChildAt(i)->DestroyContent();
  }
  mInUnlinkOrDeletion = oldVal;

  mLayoutHistoryState = nullptr;

  
  
  
  mExternalResourceMap.Shutdown();

  mRegistry = nullptr;

  nsCOMPtr<nsIServiceWorkerManager> swm = mozilla::services::GetServiceWorkerManager();
  if (swm) {
    swm->MaybeStopControlling(this);
  }

  
  
  ReleaseWrapper(static_cast<nsINode*>(this));
}

void
nsDocument::RemovedFromDocShell()
{
  if (mRemovedFromDocShell)
    return;

  mRemovedFromDocShell = true;
  EnumerateActivityObservers(NotifyActivityChanged, nullptr);

  uint32_t i, count = mChildren.ChildCount();
  for (i = 0; i < count; ++i) {
    mChildren.ChildAt(i)->SaveSubtreeState();
  }
}

already_AddRefed<nsILayoutHistoryState>
nsDocument::GetLayoutHistoryState() const
{
  nsCOMPtr<nsILayoutHistoryState> state;
  if (!mScriptGlobalObject) {
    state = mLayoutHistoryState;
  } else {
    nsCOMPtr<nsIDocShell> docShell(mDocumentContainer);
    if (docShell) {
      docShell->GetLayoutHistoryState(getter_AddRefs(state));
    }
  }

  return state.forget();
}

void
nsDocument::EnsureOnloadBlocker()
{
  
  
  if (mOnloadBlockCount != 0 && mScriptGlobalObject) {
    nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
    if (loadGroup) {
      
      nsCOMPtr<nsISimpleEnumerator> requests;
      loadGroup->GetRequests(getter_AddRefs(requests));

      bool hasMore = false;
      while (NS_SUCCEEDED(requests->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsISupports> elem;
        requests->GetNext(getter_AddRefs(elem));
        nsCOMPtr<nsIRequest> request = do_QueryInterface(elem);
        if (request && request == mOnloadBlocker) {
          return;
        }
      }

      
      loadGroup->AddRequest(mOnloadBlocker, nullptr);
    }
  }
}

void
nsDocument::AsyncBlockOnload()
{
  while (mAsyncOnloadBlockCount) {
    --mAsyncOnloadBlockCount;
    BlockOnload();
  }
}

void
nsDocument::BlockOnload()
{
  if (mDisplayDocument) {
    mDisplayDocument->BlockOnload();
    return;
  }

  
  
  if (mOnloadBlockCount == 0 && mScriptGlobalObject) {
    if (!nsContentUtils::IsSafeToRunScript()) {
      
      
      ++mAsyncOnloadBlockCount;
      if (mAsyncOnloadBlockCount == 1) {
        bool success = nsContentUtils::AddScriptRunner(
          NS_NewRunnableMethod(this, &nsDocument::AsyncBlockOnload));

        
        
        
        if (!success) {
          NS_WARNING("Disaster! Onload blocking script runner failed to add - expect bad things!");
          mAsyncOnloadBlockCount = 0;
        }
      }
      return;
    }
    nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
    if (loadGroup) {
      loadGroup->AddRequest(mOnloadBlocker, nullptr);
    }
  }
  ++mOnloadBlockCount;
}

void
nsDocument::UnblockOnload(bool aFireSync)
{
  if (mDisplayDocument) {
    mDisplayDocument->UnblockOnload(aFireSync);
    return;
  }

  if (mOnloadBlockCount == 0 && mAsyncOnloadBlockCount == 0) {
    NS_NOTREACHED("More UnblockOnload() calls than BlockOnload() calls; dropping call");
    return;
  }

  --mOnloadBlockCount;

  if (mOnloadBlockCount == 0) {
    if (mScriptGlobalObject) {
      
      
      if (aFireSync && mAsyncOnloadBlockCount == 0) {
        
        ++mOnloadBlockCount;
        DoUnblockOnload();
      } else {
        PostUnblockOnloadEvent();
      }
    } else if (mIsBeingUsedAsImage) {
      
      
      
      
      
      
      nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
        new AsyncEventDispatcher(this,
                                 NS_LITERAL_STRING("MozSVGAsImageDocumentLoad"),
                                 false,
                                 false);
      asyncDispatcher->PostDOMEvent();
    }
  }
}

class nsUnblockOnloadEvent : public nsRunnable {
public:
  explicit nsUnblockOnloadEvent(nsDocument* aDoc) : mDoc(aDoc) {}
  NS_IMETHOD Run() {
    mDoc->DoUnblockOnload();
    return NS_OK;
  }
private:
  nsRefPtr<nsDocument> mDoc;
};

void
nsDocument::PostUnblockOnloadEvent()
{
  nsCOMPtr<nsIRunnable> evt = new nsUnblockOnloadEvent(this);
  nsresult rv = NS_DispatchToCurrentThread(evt);
  if (NS_SUCCEEDED(rv)) {
    
    ++mOnloadBlockCount;
  } else {
    NS_WARNING("failed to dispatch nsUnblockOnloadEvent");
  }
}

void
nsDocument::DoUnblockOnload()
{
  NS_PRECONDITION(!mDisplayDocument,
                  "Shouldn't get here for resource document");
  NS_PRECONDITION(mOnloadBlockCount != 0,
                  "Shouldn't have a count of zero here, since we stabilized in "
                  "PostUnblockOnloadEvent");

  --mOnloadBlockCount;

  if (mOnloadBlockCount != 0) {
    
    
    return;
  }

  if (mAsyncOnloadBlockCount != 0) {
    
    PostUnblockOnloadEvent();
  }

  
  
  if (mScriptGlobalObject) {
    nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
    if (loadGroup) {
      loadGroup->RemoveRequest(mOnloadBlocker, nullptr, NS_OK);
    }
  }
}

nsIContent*
nsDocument::GetContentInThisDocument(nsIFrame* aFrame) const
{
  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetParentOrPlaceholderForCrossDoc(f)) {
    nsIContent* content = f->GetContent();
    if (!content || content->IsInAnonymousSubtree())
      continue;

    if (content->OwnerDoc() == this) {
      return content;
    }
    
    
    
    f = f->PresContext()->GetPresShell()->GetRootFrame();
  }

  return nullptr;
}

void
nsDocument::DispatchPageTransition(EventTarget* aDispatchTarget,
                                   const nsAString& aType,
                                   bool aPersisted)
{
  if (!aDispatchTarget) {
    return;
  }

  PageTransitionEventInit init;
  init.mBubbles = true;
  init.mCancelable = true;
  init.mPersisted = aPersisted;

  nsRefPtr<PageTransitionEvent> event =
    PageTransitionEvent::Constructor(this, aType, init);

  event->SetTrusted(true);
  event->SetTarget(this);
  EventDispatcher::DispatchDOMEvent(aDispatchTarget, nullptr, event,
                                    nullptr, nullptr);
}

static bool
NotifyPageShow(nsIDocument* aDocument, void* aData)
{
  const bool* aPersistedPtr = static_cast<const bool*>(aData);
  aDocument->OnPageShow(*aPersistedPtr, nullptr);
  return true;
}

void
nsDocument::OnPageShow(bool aPersisted,
                       EventTarget* aDispatchStartTarget)
{
  mVisible = true;

  EnumerateActivityObservers(NotifyActivityChanged, nullptr);
  EnumerateExternalResources(NotifyPageShow, &aPersisted);

  Element* root = GetRootElement();
  if (aPersisted && root) {
    
    nsRefPtr<nsContentList> links = NS_GetContentList(root,
                                                      kNameSpaceID_XHTML,
                                                      NS_LITERAL_STRING("link"));

    uint32_t linkCount = links->Length(true);
    for (uint32_t i = 0; i < linkCount; ++i) {
      static_cast<HTMLLinkElement*>(links->Item(i, false))->LinkAdded();
    }
  }

  
  if (!aDispatchStartTarget) {
    
    
    mIsShowing = true;
  }

  if (mAnimationController) {
    mAnimationController->OnPageShow();
  }

  if (aPersisted) {
    SetImagesNeedAnimating(true);
  }

  UpdateVisibilityState();

  nsCOMPtr<EventTarget> target = aDispatchStartTarget;
  if (!target) {
    target = do_QueryInterface(GetWindow());
  }

  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  nsIPrincipal *principal = GetPrincipal();
  os->NotifyObservers(static_cast<nsIDocument*>(this),
                      nsContentUtils::IsSystemPrincipal(principal) ?
                        "chrome-page-shown" :
                        "content-page-shown",
                      nullptr);
  if (!mObservingAppThemeChanged) {
    os->AddObserver(this, "app-theme-changed",  false);
    mObservingAppThemeChanged = true;
  }

  DispatchPageTransition(target, NS_LITERAL_STRING("pageshow"), aPersisted);
}

static bool
NotifyPageHide(nsIDocument* aDocument, void* aData)
{
  const bool* aPersistedPtr = static_cast<const bool*>(aData);
  aDocument->OnPageHide(*aPersistedPtr, nullptr);
  return true;
}

static void
DispatchFullScreenChange(nsIDocument* aTarget)
{
  nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
    new AsyncEventDispatcher(aTarget,
                             NS_LITERAL_STRING("mozfullscreenchange"),
                             true,
                             false);
  asyncDispatcher->PostDOMEvent();
}

void
nsDocument::OnPageHide(bool aPersisted,
                       EventTarget* aDispatchStartTarget)
{
  
  
  Element* root = GetRootElement();
  if (aPersisted && root) {
    nsRefPtr<nsContentList> links = NS_GetContentList(root,
                                                      kNameSpaceID_XHTML,
                                                      NS_LITERAL_STRING("link"));

    uint32_t linkCount = links->Length(true);
    for (uint32_t i = 0; i < linkCount; ++i) {
      static_cast<HTMLLinkElement*>(links->Item(i, false))->LinkRemoved();
    }
  }

  
  if (!aDispatchStartTarget) {
    
    
    mIsShowing = false;
  }

  if (mAnimationController) {
    mAnimationController->OnPageHide();
  }

  if (aPersisted) {
    SetImagesNeedAnimating(false);
  }

  MozExitPointerLock();

  
  nsCOMPtr<EventTarget> target = aDispatchStartTarget;
  if (!target) {
    target = do_QueryInterface(GetWindow());
  }

  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    nsIPrincipal* principal = GetPrincipal();
    os->NotifyObservers(static_cast<nsIDocument*>(this),
                        nsContentUtils::IsSystemPrincipal(principal) ?
                          "chrome-page-hidden" :
                          "content-page-hidden",
                        nullptr);

    os->RemoveObserver(this, "app-theme-changed");
    mObservingAppThemeChanged = false;
  }

  DispatchPageTransition(target, NS_LITERAL_STRING("pagehide"), aPersisted);

  mVisible = false;

  UpdateVisibilityState();

  EnumerateExternalResources(NotifyPageHide, &aPersisted);
  EnumerateActivityObservers(NotifyActivityChanged, nullptr);

  if (IsFullScreenDoc()) {
    
    
    
    
    
    
    
    
    
    nsIDocument::ExitFullscreen(this,  false);

    
    
    
    
    
    
    
    
    CleanupFullscreenState();

    
    
    DispatchFullScreenChange(this);
  }
}

void
nsDocument::WillDispatchMutationEvent(nsINode* aTarget)
{
  NS_ASSERTION(mSubtreeModifiedDepth != 0 ||
               mSubtreeModifiedTargets.Count() == 0,
               "mSubtreeModifiedTargets not cleared after dispatching?");
  ++mSubtreeModifiedDepth;
  if (aTarget) {
    
    
    int32_t count = mSubtreeModifiedTargets.Count();
    if (!count || mSubtreeModifiedTargets[count - 1] != aTarget) {
      mSubtreeModifiedTargets.AppendObject(aTarget);
    }
  }
}

void
nsDocument::MutationEventDispatched(nsINode* aTarget)
{
  --mSubtreeModifiedDepth;
  if (mSubtreeModifiedDepth == 0) {
    int32_t count = mSubtreeModifiedTargets.Count();
    if (!count) {
      return;
    }

    nsPIDOMWindow* window = GetInnerWindow();
    if (window &&
        !window->HasMutationListeners(NS_EVENT_BITS_MUTATION_SUBTREEMODIFIED)) {
      mSubtreeModifiedTargets.Clear();
      return;
    }

    nsCOMArray<nsINode> realTargets;
    for (int32_t i = 0; i < count; ++i) {
      nsINode* possibleTarget = mSubtreeModifiedTargets[i];
      nsCOMPtr<nsIContent> content = do_QueryInterface(possibleTarget);
      if (content && content->ChromeOnlyAccess()) {
        continue;
      }

      nsINode* commonAncestor = nullptr;
      int32_t realTargetCount = realTargets.Count();
      for (int32_t j = 0; j < realTargetCount; ++j) {
        commonAncestor =
          nsContentUtils::GetCommonAncestor(possibleTarget, realTargets[j]);
        if (commonAncestor) {
          realTargets.ReplaceObjectAt(commonAncestor, j);
          break;
        }
      }
      if (!commonAncestor) {
        realTargets.AppendObject(possibleTarget);
      }
    }

    mSubtreeModifiedTargets.Clear();

    int32_t realTargetCount = realTargets.Count();
    for (int32_t k = 0; k < realTargetCount; ++k) {
      InternalMutationEvent mutation(true, NS_MUTATION_SUBTREEMODIFIED);
      (new AsyncEventDispatcher(realTargets[k], mutation))->
        RunDOMEventWhenSafe();
    }
  }
}

void
nsDocument::AddStyleRelevantLink(Link* aLink)
{
  NS_ASSERTION(aLink, "Passing in a null link.  Expect crashes RSN!");
#ifdef DEBUG
  nsPtrHashKey<Link>* entry = mStyledLinks.GetEntry(aLink);
  NS_ASSERTION(!entry, "Document already knows about this Link!");
  mStyledLinksCleared = false;
#endif
  (void)mStyledLinks.PutEntry(aLink);
}

void
nsDocument::ForgetLink(Link* aLink)
{
  NS_ASSERTION(aLink, "Passing in a null link.  Expect crashes RSN!");
#ifdef DEBUG
  nsPtrHashKey<Link>* entry = mStyledLinks.GetEntry(aLink);
  NS_ASSERTION(entry || mStyledLinksCleared,
               "Document knows nothing about this Link!");
#endif
  (void)mStyledLinks.RemoveEntry(aLink);
}

void
nsDocument::DestroyElementMaps()
{
#ifdef DEBUG
  mStyledLinksCleared = true;
#endif
  mStyledLinks.Clear();
  mIdentifierMap.Clear();
  ++mExpandoAndGeneration.generation;
}

static
PLDHashOperator
EnumerateStyledLinks(nsPtrHashKey<Link>* aEntry, void* aArray)
{
  LinkArray* array = static_cast<LinkArray*>(aArray);
  (void)array->AppendElement(aEntry->GetKey());
  return PL_DHASH_NEXT;
}

void
nsDocument::RefreshLinkHrefs()
{
  
  
  
  LinkArray linksToNotify(mStyledLinks.Count());
  (void)mStyledLinks.EnumerateEntries(EnumerateStyledLinks, &linksToNotify);

  
  nsAutoScriptBlocker scriptBlocker;
  for (LinkArray::size_type i = 0; i < linksToNotify.Length(); i++) {
    linksToNotify[i]->ResetLinkState(true, linksToNotify[i]->ElementHasHref());
  }
}

nsresult
nsDocument::CloneDocHelper(nsDocument* clone) const
{
  clone->mIsStaticDocument = mCreatingStaticClone;

  
  nsresult rv = clone->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  if (mCreatingStaticClone) {
    nsCOMPtr<nsILoadGroup> loadGroup;

    
    
    nsCOMPtr<nsIDocumentLoader> docLoader(mDocumentContainer);
    if (docLoader) {
      docLoader->GetLoadGroup(getter_AddRefs(loadGroup));
    }
    nsCOMPtr<nsIChannel> channel = GetChannel();
    nsCOMPtr<nsIURI> uri;
    if (channel) {
      NS_GetFinalChannelURI(channel, getter_AddRefs(uri));
    } else {
      uri = nsIDocument::GetDocumentURI();
    }
    clone->mChannel = channel;
    if (uri) {
      clone->ResetToURI(uri, loadGroup, NodePrincipal());
    }

    clone->SetContainer(mDocumentContainer);
  }

  
  
  
  
  
  
  clone->nsDocument::SetDocumentURI(nsIDocument::GetDocumentURI());
  clone->SetChromeXHRDocURI(mChromeXHRDocURI);
  clone->SetPrincipal(NodePrincipal());
  clone->mDocumentBaseURI = mDocumentBaseURI;
  clone->SetChromeXHRDocBaseURI(mChromeXHRDocBaseURI);

  
  bool hasHadScriptObject = true;
  nsIScriptGlobalObject* scriptObject =
    GetScriptHandlingObject(hasHadScriptObject);
  NS_ENSURE_STATE(scriptObject || !hasHadScriptObject);
  if (scriptObject) {
    clone->SetScriptHandlingObject(scriptObject);
  } else {
    clone->SetScopeObject(GetScopeObject());
  }
  
  clone->SetLoadedAsData(true);

  

  
  clone->mCharacterSet = mCharacterSet;
  clone->mCharacterSetSource = mCharacterSetSource;
  clone->mCompatMode = mCompatMode;
  clone->mBidiOptions = mBidiOptions;
  clone->mContentLanguage = mContentLanguage;
  clone->SetContentTypeInternal(GetContentTypeInternal());
  clone->mSecurityInfo = mSecurityInfo;

  
  clone->mType = mType;
  clone->mXMLDeclarationBits = mXMLDeclarationBits;
  clone->mBaseTarget = mBaseTarget;
  return NS_OK;
}

void
nsDocument::SetReadyStateInternal(ReadyState rs)
{
  mReadyState = rs;
  if (rs == READYSTATE_UNINITIALIZED) {
    
    
    
    return;
  }
  if (mTiming) {
    switch (rs) {
      case READYSTATE_LOADING:
        mTiming->NotifyDOMLoading(nsIDocument::GetDocumentURI());
        break;
      case READYSTATE_INTERACTIVE:
        mTiming->NotifyDOMInteractive(nsIDocument::GetDocumentURI());
        break;
      case READYSTATE_COMPLETE:
        mTiming->NotifyDOMComplete(nsIDocument::GetDocumentURI());
        break;
      default:
        NS_WARNING("Unexpected ReadyState value");
        break;
    }
  }
  
  if (READYSTATE_LOADING == rs) {
    mLoadingTimeStamp = mozilla::TimeStamp::Now();
  }

  nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
    new AsyncEventDispatcher(this, NS_LITERAL_STRING("readystatechange"),
                             false, false);
  asyncDispatcher->RunDOMEventWhenSafe();
}

NS_IMETHODIMP
nsDocument::GetReadyState(nsAString& aReadyState)
{
  nsIDocument::GetReadyState(aReadyState);
  return NS_OK;
}

void
nsIDocument::GetReadyState(nsAString& aReadyState) const
{
  switch(mReadyState) {
  case READYSTATE_LOADING :
    aReadyState.AssignLiteral(MOZ_UTF16("loading"));
    break;
  case READYSTATE_INTERACTIVE :
    aReadyState.AssignLiteral(MOZ_UTF16("interactive"));
    break;
  case READYSTATE_COMPLETE :
    aReadyState.AssignLiteral(MOZ_UTF16("complete"));
    break;
  default:
    aReadyState.AssignLiteral(MOZ_UTF16("uninitialized"));
  }
}

namespace {

struct SuppressArgs
{
  nsIDocument::SuppressionType mWhat;
  uint32_t mIncrease;
};

}

static bool
SuppressEventHandlingInDocument(nsIDocument* aDocument, void* aData)
{
  SuppressArgs* args = static_cast<SuppressArgs*>(aData);
  aDocument->SuppressEventHandling(args->mWhat, args->mIncrease);
  return true;
}

void
nsDocument::SuppressEventHandling(nsIDocument::SuppressionType aWhat,
                                  uint32_t aIncrease)
{
  if (mEventsSuppressed == 0 && mAnimationsPaused == 0 &&
      aIncrease != 0 && mPresShell && mScriptGlobalObject) {
    RevokeAnimationFrameNotifications();
  }

  if (aWhat == eAnimationsOnly) {
    mAnimationsPaused += aIncrease;
  } else {
    mEventsSuppressed += aIncrease;
  }

  SuppressArgs args = { aWhat, aIncrease };
  EnumerateSubDocuments(SuppressEventHandlingInDocument, &args);
}

static void
FireOrClearDelayedEvents(nsTArray<nsCOMPtr<nsIDocument> >& aDocuments,
                         bool aFireEvents)
{
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm)
    return;

  for (uint32_t i = 0; i < aDocuments.Length(); ++i) {
    
    
    if (!aDocuments[i]->EventHandlingSuppressed()) {
      fm->FireDelayedEvents(aDocuments[i]);
      nsCOMPtr<nsIPresShell> shell = aDocuments[i]->GetShell();
      if (shell) {
        
        bool fire = aFireEvents &&
                    aDocuments[i]->GetInnerWindow() &&
                    aDocuments[i]->GetInnerWindow()->IsCurrentInnerWindow();
        shell->FireOrClearDelayedEvents(fire);
      }
    }
  }
}

void
nsDocument::PreloadPictureOpened()
{
  mPreloadPictureDepth++;
}

void
nsDocument::PreloadPictureClosed()
{
  mPreloadPictureDepth--;
  if (mPreloadPictureDepth == 0) {
    mPreloadPictureFoundSource.SetIsVoid(true);
  } else {
    MOZ_ASSERT(mPreloadPictureDepth >= 0);
  }
}

void
nsDocument::PreloadPictureImageSource(const nsAString& aSrcsetAttr,
                                      const nsAString& aSizesAttr,
                                      const nsAString& aTypeAttr,
                                      const nsAString& aMediaAttr)
{
  
  
  
  if (mPreloadPictureDepth == 1 && mPreloadPictureFoundSource.IsVoid()) {
    
    
    bool found =
      HTMLImageElement::SelectSourceForTagWithAttrs(this, true, NullString(),
                                                    aSrcsetAttr, aSizesAttr,
                                                    aTypeAttr, aMediaAttr,
                                                    mPreloadPictureFoundSource);
    if (found && mPreloadPictureFoundSource.IsVoid()) {
      
      mPreloadPictureFoundSource.SetIsVoid(false);
    }
  }
}

already_AddRefed<nsIURI>
nsDocument::ResolvePreloadImage(nsIURI *aBaseURI,
                                const nsAString& aSrcAttr,
                                const nsAString& aSrcsetAttr,
                                const nsAString& aSizesAttr)
{
  nsString sourceURL;
  if (mPreloadPictureDepth == 1 && !mPreloadPictureFoundSource.IsVoid()) {
    
    
    sourceURL = mPreloadPictureFoundSource;
  } else {
    
    HTMLImageElement::SelectSourceForTagWithAttrs(this, false, aSrcAttr,
                                                  aSrcsetAttr, aSizesAttr,
                                                  NullString(), NullString(),
                                                  sourceURL);
  }

  
  if (sourceURL.IsEmpty()) {
    return nullptr;
  }

  
  
  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri), sourceURL,
                                                 this, aBaseURI);
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  
  
  
  return uri.forget();
}

void
nsDocument::MaybePreLoadImage(nsIURI* uri, const nsAString &aCrossOriginAttr,
                              ReferrerPolicy aReferrerPolicy)
{
  
  
  
  int16_t blockingStatus;
  if (nsContentUtils::IsImageInCache(uri, static_cast<nsIDocument *>(this)) ||
      !nsContentUtils::CanLoadImage(uri, static_cast<nsIDocument *>(this),
                                    this, NodePrincipal(), &blockingStatus)) {
    return;
  }

  nsLoadFlags loadFlags = nsIRequest::LOAD_NORMAL;
  switch (Element::StringToCORSMode(aCrossOriginAttr)) {
  case CORS_NONE:
    
    break;
  case CORS_ANONYMOUS:
    loadFlags |= imgILoader::LOAD_CORS_ANONYMOUS;
    break;
  case CORS_USE_CREDENTIALS:
    loadFlags |= imgILoader::LOAD_CORS_USE_CREDENTIALS;
    break;
  default:
    MOZ_CRASH("Unknown CORS mode!");
  }

  
  nsRefPtr<imgRequestProxy> request;
  nsresult rv =
    nsContentUtils::LoadImage(uri,
                              this,
                              NodePrincipal(),
                              mDocumentURI, 
                              aReferrerPolicy,
                              nullptr,       
                              loadFlags,
                              NS_LITERAL_STRING("img"),
                              getter_AddRefs(request));

  
  
  
  if (NS_SUCCEEDED(rv)) {
    mPreloadingImages.Put(uri, request.forget());
  }
}

void
nsDocument::ForgetImagePreload(nsIURI* aURI)
{
  
  
  if (mPreloadingImages.Count() != 0) {
    nsCOMPtr<imgIRequest> req;
    mPreloadingImages.Remove(aURI, getter_AddRefs(req));
    if (req) {
      
      req->CancelAndForgetObserver(NS_BINDING_ABORTED);
    }
  }
}

EventStates
nsDocument::GetDocumentState()
{
  if (!mGotDocumentState.HasState(NS_DOCUMENT_STATE_RTL_LOCALE)) {
    if (IsDocumentRightToLeft()) {
      mDocumentState |= NS_DOCUMENT_STATE_RTL_LOCALE;
    }
    mGotDocumentState |= NS_DOCUMENT_STATE_RTL_LOCALE;
  }
  if (!mGotDocumentState.HasState(NS_DOCUMENT_STATE_WINDOW_INACTIVE)) {
    nsIPresShell* shell = GetShell();
    if (shell && shell->GetPresContext() &&
        shell->GetPresContext()->IsTopLevelWindowInactive()) {
      mDocumentState |= NS_DOCUMENT_STATE_WINDOW_INACTIVE;
    }
    mGotDocumentState |= NS_DOCUMENT_STATE_WINDOW_INACTIVE;
  }
  return mDocumentState;
}

namespace {





class StubCSSLoaderObserver final : public nsICSSLoaderObserver {
  ~StubCSSLoaderObserver() {}
public:
  NS_IMETHOD
  StyleSheetLoaded(CSSStyleSheet*, bool, nsresult) override
  {
    return NS_OK;
  }
  NS_DECL_ISUPPORTS
};
NS_IMPL_ISUPPORTS(StubCSSLoaderObserver, nsICSSLoaderObserver)

}

void
nsDocument::PreloadStyle(nsIURI* uri, const nsAString& charset,
                         const nsAString& aCrossOriginAttr,
                         const ReferrerPolicy aReferrerPolicy)
{
  
  nsCOMPtr<nsICSSLoaderObserver> obs = new StubCSSLoaderObserver();

  
  CSSLoader()->LoadSheet(uri, NodePrincipal(),
                         NS_LossyConvertUTF16toASCII(charset),
                         obs,
                         Element::StringToCORSMode(aCrossOriginAttr),
                         aReferrerPolicy);
}

nsresult
nsDocument::LoadChromeSheetSync(nsIURI* uri, bool isAgentSheet,
                                CSSStyleSheet** sheet)
{
  return CSSLoader()->LoadSheetSync(uri, isAgentSheet, isAgentSheet, sheet);
}

class nsDelayedEventDispatcher : public nsRunnable
{
public:
  explicit nsDelayedEventDispatcher(nsTArray<nsCOMPtr<nsIDocument>>& aDocuments)
  {
    mDocuments.SwapElements(aDocuments);
  }
  virtual ~nsDelayedEventDispatcher() {}

  NS_IMETHOD Run()
  {
    FireOrClearDelayedEvents(mDocuments, true);
    return NS_OK;
  }

private:
  nsTArray<nsCOMPtr<nsIDocument> > mDocuments;
};

namespace {

struct UnsuppressArgs
{
  explicit UnsuppressArgs(nsIDocument::SuppressionType aWhat)
    : mWhat(aWhat)
  {
  }

  nsIDocument::SuppressionType mWhat;
  nsTArray<nsCOMPtr<nsIDocument>> mDocs;
};

}

static bool
GetAndUnsuppressSubDocuments(nsIDocument* aDocument,
                             void* aData)
{
  UnsuppressArgs* args = static_cast<UnsuppressArgs*>(aData);
  if (args->mWhat != nsIDocument::eAnimationsOnly &&
      aDocument->EventHandlingSuppressed() > 0) {
    static_cast<nsDocument*>(aDocument)->DecreaseEventSuppression();
  } else if (args->mWhat == nsIDocument::eAnimationsOnly &&
             aDocument->AnimationsPaused()) {
    static_cast<nsDocument*>(aDocument)->ResumeAnimations();
  }

  if (args->mWhat != nsIDocument::eAnimationsOnly) {
    
    args->mDocs.AppendElement(aDocument);
  }

  aDocument->EnumerateSubDocuments(GetAndUnsuppressSubDocuments, aData);
  return true;
}

void
nsDocument::UnsuppressEventHandlingAndFireEvents(nsIDocument::SuppressionType aWhat,
                                                 bool aFireEvents)
{
  UnsuppressArgs args(aWhat);
  GetAndUnsuppressSubDocuments(this, &args);

  if (aWhat == nsIDocument::eAnimationsOnly) {
    
    return;
  }

  if (aFireEvents) {
    NS_DispatchToCurrentThread(new nsDelayedEventDispatcher(args.mDocs));
  } else {
    FireOrClearDelayedEvents(args.mDocs, false);
  }
}

nsISupports*
nsDocument::GetCurrentContentSink()
{
  return mParser ? mParser->GetContentSink() : nullptr;
}

nsIDocument*
nsDocument::GetTemplateContentsOwner()
{
  if (!mTemplateContentsOwner) {
    bool hasHadScriptObject = true;
    nsIScriptGlobalObject* scriptObject =
      GetScriptHandlingObject(hasHadScriptObject);

    nsCOMPtr<nsIDOMDocument> domDocument;
    nsresult rv = NS_NewDOMDocument(getter_AddRefs(domDocument),
                                    EmptyString(), 
                                    EmptyString(), 
                                    nullptr, 
                                    nsIDocument::GetDocumentURI(),
                                    nsIDocument::GetDocBaseURI(),
                                    NodePrincipal(),
                                    true, 
                                    scriptObject, 
                                    DocumentFlavorHTML);
    NS_ENSURE_SUCCESS(rv, nullptr);

    mTemplateContentsOwner = do_QueryInterface(domDocument);
    NS_ENSURE_TRUE(mTemplateContentsOwner, nullptr);

    nsDocument* doc = static_cast<nsDocument*>(mTemplateContentsOwner.get());

    if (!scriptObject) {
      mTemplateContentsOwner->SetScopeObject(GetScopeObject());
    }

    doc->mHasHadScriptHandlingObject = hasHadScriptObject;

    
    
    
    doc->mTemplateContentsOwner = doc;
  }

  return mTemplateContentsOwner;
}

void
nsDocument::RegisterHostObjectUri(const nsACString& aUri)
{
  mHostObjectURIs.AppendElement(aUri);
}

void
nsDocument::UnregisterHostObjectUri(const nsACString& aUri)
{
  mHostObjectURIs.RemoveElement(aUri);
}

void
nsDocument::SetScrollToRef(nsIURI *aDocumentURI)
{
  if (!aDocumentURI) {
    return;
  }

  nsAutoCString ref;

  
  
  
  

  aDocumentURI->GetSpec(ref);

  nsReadingIterator<char> start, end;

  ref.BeginReading(start);
  ref.EndReading(end);

  if (FindCharInReadable('#', start, end)) {
    ++start; 

    mScrollToRef = Substring(start, end);
  }
}

void
nsDocument::ScrollToRef()
{
  if (mScrolledToRefAlready) {
    nsCOMPtr<nsIPresShell> shell = GetShell();
    if (shell) {
      shell->ScrollToAnchor();
    }
    return;
  }

  if (mScrollToRef.IsEmpty()) {
    return;
  }

  char* tmpstr = ToNewCString(mScrollToRef);
  if (!tmpstr) {
    return;
  }

  nsUnescape(tmpstr);
  nsAutoCString unescapedRef;
  unescapedRef.Assign(tmpstr);
  free(tmpstr);

  nsresult rv = NS_ERROR_FAILURE;
  
  
  NS_ConvertUTF8toUTF16 ref(unescapedRef);

  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    
    if (!ref.IsEmpty()) {
      
      rv = shell->GoToAnchor(ref, mChangeScrollPosWhenScrollingToRef);
    } else {
      rv = NS_ERROR_FAILURE;
    }

    
    

    if (NS_FAILED(rv)) {
      const nsACString &docCharset = GetDocumentCharacterSet();

      rv = nsContentUtils::ConvertStringFromEncoding(docCharset,
                                                     unescapedRef,
                                                     ref);

      if (NS_SUCCEEDED(rv) && !ref.IsEmpty()) {
        rv = shell->GoToAnchor(ref, mChangeScrollPosWhenScrollingToRef);
      }
    }
    if (NS_SUCCEEDED(rv)) {
      mScrolledToRefAlready = true;
    }
  }
}

void
nsDocument::ResetScrolledToRefAlready()
{
  mScrolledToRefAlready = false;
}

void
nsDocument::SetChangeScrollPosWhenScrollingToRef(bool aValue)
{
  mChangeScrollPosWhenScrollingToRef = aValue;
}

void
nsIDocument::RegisterActivityObserver(nsISupports* aSupports)
{
  if (!mActivityObservers) {
    mActivityObservers = new nsTHashtable<nsPtrHashKey<nsISupports> >();
    if (!mActivityObservers)
      return;
  }
  mActivityObservers->PutEntry(aSupports);
}

bool
nsIDocument::UnregisterActivityObserver(nsISupports* aSupports)
{
  if (!mActivityObservers)
    return false;
  if (!mActivityObservers->GetEntry(aSupports))
    return false;
  mActivityObservers->RemoveEntry(aSupports);
  return true;
}

struct EnumerateActivityObserversData {
  nsIDocument::ActivityObserverEnumerator mEnumerator;
  void* mData;
};

static PLDHashOperator
EnumerateObservers(nsPtrHashKey<nsISupports>* aEntry, void* aData)
{
  EnumerateActivityObserversData* data = static_cast<EnumerateActivityObserversData*>(aData);
  data->mEnumerator(aEntry->GetKey(), data->mData);
  return PL_DHASH_NEXT;
}

void
nsIDocument::EnumerateActivityObservers(ActivityObserverEnumerator aEnumerator,
                                        void* aData)
{
  if (!mActivityObservers)
    return;
  EnumerateActivityObserversData data = { aEnumerator, aData };
  mActivityObservers->EnumerateEntries(EnumerateObservers, &data);
}

void
nsIDocument::RegisterPendingLinkUpdate(Link* aLink)
{
  MOZ_ASSERT(!mIsLinkUpdateRegistrationsForbidden);
  mLinksToUpdate.PutEntry(aLink);
  mHasLinksToUpdate = true;
}

void
nsIDocument::UnregisterPendingLinkUpdate(Link* aLink)
{
  MOZ_ASSERT(!mIsLinkUpdateRegistrationsForbidden);
  if (!mHasLinksToUpdate)
    return;

  mLinksToUpdate.RemoveEntry(aLink);
}

static PLDHashOperator
EnumeratePendingLinkUpdates(nsPtrHashKey<Link>* aEntry, void* aData)
{
  aEntry->GetKey()->GetElement()->UpdateLinkState(aEntry->GetKey()->LinkState());
  return PL_DHASH_NEXT;
}

void
nsIDocument::FlushPendingLinkUpdates()
{
  MOZ_ASSERT(!mIsLinkUpdateRegistrationsForbidden);
  if (!mHasLinksToUpdate)
    return;

#ifdef DEBUG
  AutoRestore<bool> saved(mIsLinkUpdateRegistrationsForbidden);
  mIsLinkUpdateRegistrationsForbidden = true;
#endif
  mLinksToUpdate.EnumerateEntries(EnumeratePendingLinkUpdates, nullptr);
  mLinksToUpdate.Clear();
  mHasLinksToUpdate = false;
}

already_AddRefed<nsIDocument>
nsIDocument::CreateStaticClone(nsIDocShell* aCloneContainer)
{
  nsDocument* thisAsDoc = static_cast<nsDocument*>(this);
  mCreatingStaticClone = true;

  
  nsRefPtr<nsDocShell> originalShell = mDocumentContainer.get();
  SetContainer(static_cast<nsDocShell*>(aCloneContainer));
  nsCOMPtr<nsIDOMNode> clonedNode;
  nsresult rv = thisAsDoc->CloneNode(true, 1, getter_AddRefs(clonedNode));
  SetContainer(originalShell);

  nsRefPtr<nsDocument> clonedDoc;
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDocument> tmp = do_QueryInterface(clonedNode);
    if (tmp) {
      clonedDoc = static_cast<nsDocument*>(tmp.get());
      if (IsStaticDocument()) {
        clonedDoc->mOriginalDocument = mOriginalDocument;
      } else {
        clonedDoc->mOriginalDocument = this;
      }
      int32_t sheetsCount = GetNumberOfStyleSheets();
      for (int32_t i = 0; i < sheetsCount; ++i) {
        nsRefPtr<CSSStyleSheet> sheet = do_QueryObject(GetStyleSheetAt(i));
        if (sheet) {
          if (sheet->IsApplicable()) {
            nsRefPtr<CSSStyleSheet> clonedSheet =
              sheet->Clone(nullptr, nullptr, clonedDoc, nullptr);
            NS_WARN_IF_FALSE(clonedSheet, "Cloning a stylesheet didn't work!");
            if (clonedSheet) {
              clonedDoc->AddStyleSheet(clonedSheet);
            }
          }
        }
      }

      sheetsCount = thisAsDoc->mOnDemandBuiltInUASheets.Count();
      
      for (int32_t i = sheetsCount - 1; i >= 0; --i) {
        nsRefPtr<CSSStyleSheet> sheet =
          do_QueryObject(thisAsDoc->mOnDemandBuiltInUASheets[i]);
        if (sheet) {
          if (sheet->IsApplicable()) {
            nsRefPtr<CSSStyleSheet> clonedSheet =
              sheet->Clone(nullptr, nullptr, clonedDoc, nullptr);
            NS_WARN_IF_FALSE(clonedSheet, "Cloning a stylesheet didn't work!");
            if (clonedSheet) {
              clonedDoc->AddOnDemandBuiltInUASheet(clonedSheet);
            }
          }
        }
      }
    }
  }
  mCreatingStaticClone = false;
  return clonedDoc.forget();
}

nsresult
nsIDocument::ScheduleFrameRequestCallback(const FrameRequestCallbackHolder& aCallback,
                                          int32_t *aHandle)
{
  if (mFrameRequestCallbackCounter == INT32_MAX) {
    
    return NS_ERROR_NOT_AVAILABLE;
  }
  int32_t newHandle = ++mFrameRequestCallbackCounter;

  bool alreadyRegistered = !mFrameRequestCallbacks.IsEmpty();
  DebugOnly<FrameRequest*> request =
    mFrameRequestCallbacks.AppendElement(FrameRequest(aCallback, newHandle));
  NS_ASSERTION(request, "This is supposed to be infallible!");
  if (!alreadyRegistered && mPresShell && IsEventHandlingEnabled()) {
    mPresShell->GetPresContext()->RefreshDriver()->
      ScheduleFrameRequestCallbacks(this);
  }

  *aHandle = newHandle;
  return NS_OK;
}

void
nsIDocument::CancelFrameRequestCallback(int32_t aHandle)
{
  
  if (mFrameRequestCallbacks.RemoveElementSorted(aHandle) &&
      mFrameRequestCallbacks.IsEmpty() &&
      mPresShell && IsEventHandlingEnabled()) {
    mPresShell->GetPresContext()->RefreshDriver()->
      RevokeFrameRequestCallbacks(this);
  }
}

nsresult
nsDocument::GetStateObject(nsIVariant** aState)
{
  
  
  
  
  

  nsCOMPtr<nsIVariant> stateObj;
  if (!mStateObjectCached && mStateObjectContainer) {
    AutoJSContext cx;
    nsIGlobalObject* sgo = GetScopeObject();
    NS_ENSURE_TRUE(sgo, NS_ERROR_UNEXPECTED);
    JS::Rooted<JSObject*> global(cx, sgo->GetGlobalJSObject());
    NS_ENSURE_TRUE(global, NS_ERROR_UNEXPECTED);
    JSAutoCompartment ac(cx, global);

    mStateObjectContainer->
      DeserializeToVariant(cx, getter_AddRefs(mStateObjectCached));
  }

  NS_IF_ADDREF(*aState = mStateObjectCached);

  return NS_OK;
}

nsDOMNavigationTiming*
nsDocument::GetNavigationTiming() const
{
  return mTiming;
}

nsresult
nsDocument::SetNavigationTiming(nsDOMNavigationTiming* aTiming)
{
  mTiming = aTiming;
  if (!mLoadingTimeStamp.IsNull() && mTiming) {
    mTiming->SetDOMLoadingTimeStamp(nsIDocument::GetDocumentURI(), mLoadingTimeStamp);
  }
  return NS_OK;
}

Element*
nsDocument::FindImageMap(const nsAString& aUseMapValue)
{
  if (aUseMapValue.IsEmpty()) {
    return nullptr;
  }

  nsAString::const_iterator start, end;
  aUseMapValue.BeginReading(start);
  aUseMapValue.EndReading(end);

  int32_t hash = aUseMapValue.FindChar('#');
  if (hash < 0) {
    return nullptr;
  }
  
  start.advance(hash + 1);

  if (start == end) {
    return nullptr; 
  }

  const nsAString& mapName = Substring(start, end);

  if (!mImageMaps) {
    mImageMaps = new nsContentList(this, kNameSpaceID_XHTML, nsGkAtoms::map, nsGkAtoms::map);
  }

  uint32_t i, n = mImageMaps->Length(true);
  nsString name;
  for (i = 0; i < n; ++i) {
    nsIContent* map = mImageMaps->Item(i);
    if (map->AttrValueIs(kNameSpaceID_None, nsGkAtoms::id, mapName,
                         eCaseMatters) ||
        (map->GetAttr(kNameSpaceID_None, nsGkAtoms::name, name) &&
         mapName.Equals(name, nsCaseInsensitiveStringComparator()))) {
      return map->AsElement();
    }
  }

  return nullptr;
}

#define DEPRECATED_OPERATION(_op) #_op "Warning",
static const char* kDeprecationWarnings[] = {
#include "nsDeprecatedOperationList.h"
  nullptr
};
#undef DEPRECATED_OPERATION

#define DOCUMENT_WARNING(_op) #_op "Warning",
static const char* kDocumentWarnings[] = {
#include "nsDocumentWarningList.h"
  nullptr
};
#undef DOCUMENT_WARNING

bool
nsIDocument::HasWarnedAbout(DeprecatedOperations aOperation)
{
  static_assert(eDeprecatedOperationCount <= 64,
                "Too many deprecated operations");
  return mDeprecationWarnedAbout & (1ull << aOperation);
}

void
nsIDocument::WarnOnceAbout(DeprecatedOperations aOperation,
                           bool asError )
{
  if (HasWarnedAbout(aOperation)) {
    return;
  }
  mDeprecationWarnedAbout |= (1ull << aOperation);
  uint32_t flags = asError ? nsIScriptError::errorFlag
                           : nsIScriptError::warningFlag;
  nsContentUtils::ReportToConsole(flags,
                                  NS_LITERAL_CSTRING("DOM Core"), this,
                                  nsContentUtils::eDOM_PROPERTIES,
                                  kDeprecationWarnings[aOperation]);
}

bool
nsIDocument::HasWarnedAbout(DocumentWarnings aWarning)
{
  static_assert(eDocumentWarningCount <= 64,
                "Too many document warnings");
  return mDocWarningWarnedAbout & (1ull << aWarning);
}

void
nsIDocument::WarnOnceAbout(DocumentWarnings aWarning,
                           bool asError ,
                           const char16_t **aParams ,
                           uint32_t aParamsLength )
{
  if (HasWarnedAbout(aWarning)) {
    return;
  }
  mDocWarningWarnedAbout |= (1ull << aWarning);
  uint32_t flags = asError ? nsIScriptError::errorFlag
                           : nsIScriptError::warningFlag;
  nsContentUtils::ReportToConsole(flags,
                                  NS_LITERAL_CSTRING("DOM Core"), this,
                                  nsContentUtils::eDOM_PROPERTIES,
                                  kDocumentWarnings[aWarning],
                                  aParams,
                                  aParamsLength);
}

nsresult
nsDocument::AddImage(imgIRequest* aImage)
{
  NS_ENSURE_ARG_POINTER(aImage);

  
  uint32_t oldCount = 0;
  mImageTracker.Get(aImage, &oldCount);

  
  mImageTracker.Put(aImage, oldCount + 1);

  nsresult rv = NS_OK;

  
  
  if (oldCount == 0) {
    if (mLockingImages)
      rv = aImage->LockImage();
    if (NS_SUCCEEDED(rv) && (!sOnloadDecodeLimit ||
                             mImageTracker.Count() < sOnloadDecodeLimit))
      rv = aImage->StartDecoding();
  }

  
  
  if (oldCount == 0 && mAnimatingImages) {
    nsresult rv2 = aImage->IncrementAnimationConsumers();
    rv = NS_SUCCEEDED(rv) ? rv2 : rv;
  }

  return rv;
}

nsresult
nsDocument::RemoveImage(imgIRequest* aImage, uint32_t aFlags)
{
  NS_ENSURE_ARG_POINTER(aImage);

  
  uint32_t count = 0;
  DebugOnly<bool> found = mImageTracker.Get(aImage, &count);
  MOZ_ASSERT(found, "Removing image that wasn't in the tracker!");
  MOZ_ASSERT(count > 0, "Entry in the cache tracker with count 0!");

  
  count--;

  
  
  if (count != 0) {
    mImageTracker.Put(aImage, count);
    return NS_OK;
  }

  mImageTracker.Remove(aImage);

  nsresult rv = NS_OK;

  
  
  if (mLockingImages) {
    rv = aImage->UnlockImage();
  }

  
  if (mAnimatingImages) {
    nsresult rv2 = aImage->DecrementAnimationConsumers();
    rv = NS_SUCCEEDED(rv) ? rv2 : rv;
  }

  if (aFlags & REQUEST_DISCARD) {
    
    
    
    aImage->RequestDiscard();
  }

  return rv;
}

nsresult
nsDocument::AddPlugin(nsIObjectLoadingContent* aPlugin)
{
  MOZ_ASSERT(aPlugin);
  if (!mPlugins.PutEntry(aPlugin)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

void
nsDocument::RemovePlugin(nsIObjectLoadingContent* aPlugin)
{
  MOZ_ASSERT(aPlugin);
  mPlugins.RemoveEntry(aPlugin);
}

static bool
AllSubDocumentPluginEnum(nsIDocument* aDocument, void* userArg)
{
  nsTArray<nsIObjectLoadingContent*>* plugins =
    reinterpret_cast< nsTArray<nsIObjectLoadingContent*>* >(userArg);
  MOZ_ASSERT(plugins);
  aDocument->GetPlugins(*plugins);
  return true;
}

static PLDHashOperator
AllPluginEnum(nsPtrHashKey<nsIObjectLoadingContent>* aPlugin, void* userArg)
{
  nsTArray<nsIObjectLoadingContent*>* allPlugins =
    reinterpret_cast< nsTArray<nsIObjectLoadingContent*>* >(userArg);
  MOZ_ASSERT(allPlugins);
  allPlugins->AppendElement(aPlugin->GetKey());
  return PL_DHASH_NEXT;
}

void
nsDocument::GetPlugins(nsTArray<nsIObjectLoadingContent*>& aPlugins)
{
  aPlugins.SetCapacity(aPlugins.Length() + mPlugins.Count());
  mPlugins.EnumerateEntries(AllPluginEnum, &aPlugins);
  EnumerateSubDocuments(AllSubDocumentPluginEnum, &aPlugins);
}

PLDHashOperator LockEnumerator(imgIRequest* aKey,
                               uint32_t aData,
                               void*    userArg)
{
  aKey->LockImage();
  aKey->RequestDecode();
  return PL_DHASH_NEXT;
}

PLDHashOperator UnlockEnumerator(imgIRequest* aKey,
                                 uint32_t aData,
                                 void*    userArg)
{
  aKey->UnlockImage();
  return PL_DHASH_NEXT;
}


nsresult
nsDocument::SetImageLockingState(bool aLocked)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content &&
      !Preferences::GetBool("image.mem.allow_locking_in_content_processes", true)) {
    return NS_OK;
  }

  
  if (mLockingImages == aLocked)
    return NS_OK;

  
  mImageTracker.EnumerateRead(aLocked ? LockEnumerator
                                      : UnlockEnumerator,
                              nullptr);

  
  mLockingImages = aLocked;

  return NS_OK;
}

PLDHashOperator IncrementAnimationEnumerator(imgIRequest* aKey,
                                             uint32_t aData,
                                             void*    userArg)
{
  aKey->IncrementAnimationConsumers();
  return PL_DHASH_NEXT;
}

PLDHashOperator DecrementAnimationEnumerator(imgIRequest* aKey,
                                             uint32_t aData,
                                             void*    userArg)
{
  aKey->DecrementAnimationConsumers();
  return PL_DHASH_NEXT;
}

void
nsDocument::SetImagesNeedAnimating(bool aAnimating)
{
  
  if (mAnimatingImages == aAnimating)
    return;

  
  mImageTracker.EnumerateRead(aAnimating ? IncrementAnimationEnumerator
                                         : DecrementAnimationEnumerator,
                              nullptr);

  
  mAnimatingImages = aAnimating;
}

already_AddRefed<Touch>
nsIDocument::CreateTouch(nsIDOMWindow* aView,
                         EventTarget* aTarget,
                         int32_t aIdentifier,
                         int32_t aPageX, int32_t aPageY,
                         int32_t aScreenX, int32_t aScreenY,
                         int32_t aClientX, int32_t aClientY,
                         int32_t aRadiusX, int32_t aRadiusY,
                         float aRotationAngle,
                         float aForce)
{
  nsRefPtr<Touch> touch = new Touch(aTarget,
                                    aIdentifier,
                                    aPageX, aPageY,
                                    aScreenX, aScreenY,
                                    aClientX, aClientY,
                                    aRadiusX, aRadiusY,
                                    aRotationAngle,
                                    aForce);
  return touch.forget();
}

already_AddRefed<TouchList>
nsIDocument::CreateTouchList()
{
  nsRefPtr<TouchList> retval = new TouchList(ToSupports(this));
  return retval.forget();
}

already_AddRefed<TouchList>
nsIDocument::CreateTouchList(Touch& aTouch,
                             const Sequence<OwningNonNull<Touch> >& aTouches)
{
  nsRefPtr<TouchList> retval = new TouchList(ToSupports(this));
  retval->Append(&aTouch);
  for (uint32_t i = 0; i < aTouches.Length(); ++i) {
    retval->Append(aTouches[i].get());
  }
  return retval.forget();
}

already_AddRefed<TouchList>
nsIDocument::CreateTouchList(const Sequence<OwningNonNull<Touch> >& aTouches)
{
  nsRefPtr<TouchList> retval = new TouchList(ToSupports(this));
  for (uint32_t i = 0; i < aTouches.Length(); ++i) {
    retval->Append(aTouches[i].get());
  }
  return retval.forget();
}

already_AddRefed<nsDOMCaretPosition>
nsIDocument::CaretPositionFromPoint(float aX, float aY)
{
  nscoord x = nsPresContext::CSSPixelsToAppUnits(aX);
  nscoord y = nsPresContext::CSSPixelsToAppUnits(aY);
  nsPoint pt(x, y);

  FlushPendingNotifications(Flush_Layout);

  nsIPresShell *ps = GetShell();
  if (!ps) {
    return nullptr;
  }

  nsIFrame *rootFrame = ps->GetRootFrame();

  
  if (!rootFrame) {
    return nullptr;
  }

  nsIFrame *ptFrame = nsLayoutUtils::GetFrameForPoint(rootFrame, pt,
      nsLayoutUtils::IGNORE_PAINT_SUPPRESSION | nsLayoutUtils::IGNORE_CROSS_DOC);
  if (!ptFrame) {
    return nullptr;
  }

  
  
  
  nsPoint adjustedPoint = pt - ptFrame->GetOffsetTo(rootFrame);

  nsFrame::ContentOffsets offsets =
    ptFrame->GetContentOffsetsFromPoint(adjustedPoint);

  nsCOMPtr<nsIContent> node = offsets.content;
  uint32_t offset = offsets.offset;
  nsCOMPtr<nsIContent> anonNode = node;
  bool nodeIsAnonymous = node && node->IsInNativeAnonymousSubtree();
  if (nodeIsAnonymous) {
    node = ptFrame->GetContent();
    nsIContent* nonanon = node->FindFirstNonChromeOnlyAccessContent();
    nsCOMPtr<nsIDOMHTMLInputElement> input = do_QueryInterface(nonanon);
    nsCOMPtr<nsIDOMHTMLTextAreaElement> textArea = do_QueryInterface(nonanon);
    bool isText;
    if (textArea || (input &&
                     NS_SUCCEEDED(input->MozIsTextField(false, &isText)) &&
                     isText)) {
      
      
      
      nsCOMPtr<nsIContent> firstChild = anonNode->GetFirstChild();
      if (firstChild) {
        anonNode = firstChild;
      }

      if (textArea) {
        offset = nsContentUtils::GetAdjustedOffsetInTextControl(ptFrame, offset);
      }

      node = nonanon;
    } else {
      node = nullptr;
      offset = 0;
    }
  }

  nsRefPtr<nsDOMCaretPosition> aCaretPos = new nsDOMCaretPosition(node, offset);
  if (nodeIsAnonymous) {
    aCaretPos->SetAnonymousContentNode(anonNode);
  }
  return aCaretPos.forget();
}

NS_IMETHODIMP
nsDocument::CaretPositionFromPoint(float aX, float aY, nsISupports** aCaretPos)
{
  NS_ENSURE_ARG_POINTER(aCaretPos);
  *aCaretPos = nsIDocument::CaretPositionFromPoint(aX, aY).take();
  return NS_OK;
}

void
nsIDocument::ObsoleteSheet(nsIURI *aSheetURI, ErrorResult& rv)
{
  nsresult res = CSSLoader()->ObsoleteSheet(aSheetURI);
  if (NS_FAILED(res)) {
    rv.Throw(res);
  }
}

void
nsIDocument::ObsoleteSheet(const nsAString& aSheetURI, ErrorResult& rv)
{
  nsCOMPtr<nsIURI> uri;
  nsresult res = NS_NewURI(getter_AddRefs(uri), aSheetURI);
  if (NS_FAILED(res)) {
    rv.Throw(res);
    return;
  }
  res = CSSLoader()->ObsoleteSheet(uri);
  if (NS_FAILED(res)) {
    rv.Throw(res);
  }
}

nsIHTMLCollection*
nsIDocument::Children()
{
  if (!mChildrenCollection) {
    mChildrenCollection = new nsContentList(this, kNameSpaceID_Wildcard,
                                            nsGkAtoms::_asterisk,
                                            nsGkAtoms::_asterisk,
                                            false);
  }

  return mChildrenCollection;
}

uint32_t
nsIDocument::ChildElementCount()
{
  return Children()->Length();
}

namespace mozilla {





class FullscreenRoots {
public:
  
  
  static void Add(nsIDocument* aRoot);

  
  
  
  
  
  static void ForEach(void(*aFunction)(nsIDocument* aDoc));

  
  static void Remove(nsIDocument* aRoot);

  
  static bool IsEmpty();

private:

  FullscreenRoots() {
    MOZ_COUNT_CTOR(FullscreenRoots);
  }
  ~FullscreenRoots() {
    MOZ_COUNT_DTOR(FullscreenRoots);
  }

  enum {
    NotFound = uint32_t(-1)
  };
  
  static uint32_t Find(nsIDocument* aRoot);

  
  static bool Contains(nsIDocument* aRoot);

  
  
  static FullscreenRoots* sInstance;

  
  nsTArray<nsWeakPtr> mRoots;
};

FullscreenRoots* FullscreenRoots::sInstance = nullptr;


void
FullscreenRoots::ForEach(void(*aFunction)(nsIDocument* aDoc))
{
  if (!sInstance) {
    return;
  }
  
  
  nsTArray<nsWeakPtr> roots(sInstance->mRoots);
  
  for (uint32_t i = 0; i < roots.Length(); i++) {
    nsCOMPtr<nsIDocument> root = do_QueryReferent(roots[i]);
    
    
    if (root && FullscreenRoots::Contains(root)) {
      aFunction(root);
    }
  }
}


bool
FullscreenRoots::Contains(nsIDocument* aRoot)
{
  return FullscreenRoots::Find(aRoot) != NotFound;
}


void
FullscreenRoots::Add(nsIDocument* aRoot)
{
  if (!FullscreenRoots::Contains(aRoot)) {
    if (!sInstance) {
      sInstance = new FullscreenRoots();
    }
    sInstance->mRoots.AppendElement(do_GetWeakReference(aRoot));
  }
}


uint32_t
FullscreenRoots::Find(nsIDocument* aRoot)
{
  if (!sInstance) {
    return NotFound;
  }
  nsTArray<nsWeakPtr>& roots = sInstance->mRoots;
  for (uint32_t i = 0; i < roots.Length(); i++) {
    nsCOMPtr<nsIDocument> otherRoot(do_QueryReferent(roots[i]));
    if (otherRoot == aRoot) {
      return i;
    }
  }
  return NotFound;
}


void
FullscreenRoots::Remove(nsIDocument* aRoot)
{
  uint32_t index = Find(aRoot);
  NS_ASSERTION(index != NotFound,
    "Should only try to remove roots which are still added!");
  if (index == NotFound || !sInstance) {
    return;
  }
  sInstance->mRoots.RemoveElementAt(index);
  if (sInstance->mRoots.IsEmpty()) {
    delete sInstance;
    sInstance = nullptr;
  }
}


bool
FullscreenRoots::IsEmpty()
{
  return !sInstance;
}

} 
using mozilla::FullscreenRoots;

nsIDocument*
nsDocument::GetFullscreenRoot()
{
  nsCOMPtr<nsIDocument> root = do_QueryReferent(mFullscreenRoot);
  return root;
}

void
nsDocument::SetFullscreenRoot(nsIDocument* aRoot)
{
  mFullscreenRoot = do_GetWeakReference(aRoot);
}

NS_IMETHODIMP
nsDocument::MozCancelFullScreen()
{
  nsIDocument::MozCancelFullScreen();
  return NS_OK;
}

void
nsIDocument::MozCancelFullScreen()
{
  RestorePreviousFullScreenState();
}






class nsSetWindowFullScreen : public nsRunnable {
public:
  nsSetWindowFullScreen(nsIDocument* aDoc, bool aValue, gfx::VRHMDInfo* aHMD = nullptr)
    : mDoc(aDoc), mValue(aValue), mHMD(aHMD) {}

  NS_IMETHOD Run()
  {
    if (mDoc->GetWindow()) {
      mDoc->GetWindow()->SetFullScreenInternal(mValue, false, mHMD);
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDocument> mDoc;
  bool mValue;
  nsRefPtr<gfx::VRHMDInfo> mHMD;
};

static nsIDocument*
GetFullscreenRootDocument(nsIDocument* aDoc)
{
  if (!aDoc) {
    return nullptr;
  }
  nsIDocument* doc = aDoc;
  nsIDocument* parent;
  while ((parent = doc->GetParentDocument()) &&
         (!nsContentUtils::IsFullscreenApiContentOnly() ||
          !nsContentUtils::IsChromeDoc(parent))) {
    doc = parent;
  }
  return doc;
}

static void
SetWindowFullScreen(nsIDocument* aDoc, bool aValue, gfx::VRHMDInfo *aVRHMD = nullptr)
{
  
  nsCOMPtr<nsIDocument> root = GetFullscreenRootDocument(aDoc);
  if (aValue) {
    FullscreenRoots::Add(root);
  } else {
    FullscreenRoots::Remove(root);
  }
  if (!nsContentUtils::IsFullscreenApiContentOnly()) {
    nsContentUtils::AddScriptRunner(new nsSetWindowFullScreen(aDoc, aValue, aVRHMD));
  }
}

class nsCallExitFullscreen : public nsRunnable {
public:
  explicit nsCallExitFullscreen(nsIDocument* aDoc)
    : mDoc(aDoc) {}
  NS_IMETHOD Run()
  {
    nsDocument::ExitFullscreen(mDoc);
    return NS_OK;
  }
private:
  nsCOMPtr<nsIDocument> mDoc;
};


void
nsIDocument::ExitFullscreen(nsIDocument* aDoc, bool aRunAsync)
{
  if (aDoc && !aDoc->IsFullScreenDoc()) {
    return;
  }
  if (aRunAsync) {
    NS_DispatchToCurrentThread(new nsCallExitFullscreen(aDoc));
    return;
  }
  nsDocument::ExitFullscreen(aDoc);
}





static bool
HasCrossProcessParent(nsIDocument* aDocument)
{
  if (XRE_GetProcessType() != GeckoProcessType_Content) {
    return false;
  }
  if (aDocument->GetParentDocument() != nullptr) {
    return false;
  }
  nsPIDOMWindow* win = aDocument->GetWindow();
  if (!win) {
    return false;
  }
  nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();
  if (!docShell) {
    return false;
  }
  TabChild* tabChild(TabChild::GetFrom(docShell));
  if (!tabChild) {
    return false;
  }

  return true;
}

static bool
CountFullscreenSubDocuments(nsIDocument* aDoc, void* aData)
{
  if (aDoc->IsFullScreenDoc()) {
    uint32_t* count = static_cast<uint32_t*>(aData);
    (*count)++;
  }
  return true;
}

static uint32_t
CountFullscreenSubDocuments(nsIDocument* aDoc)
{
  uint32_t count = 0;
  aDoc->EnumerateSubDocuments(CountFullscreenSubDocuments, &count);
  return count;
}

bool
nsDocument::IsFullscreenLeaf()
{
  
  
  if (!IsFullScreenDoc()) {
    return false;
  }
  return CountFullscreenSubDocuments(this) == 0;
}

static bool
ResetFullScreen(nsIDocument* aDocument, void* aData)
{
  if (aDocument->IsFullScreenDoc()) {
    NS_ASSERTION(CountFullscreenSubDocuments(aDocument) <= 1,
        "Should have at most 1 fullscreen subdocument.");
    static_cast<nsDocument*>(aDocument)->CleanupFullscreenState();
    NS_ASSERTION(!aDocument->IsFullScreenDoc(), "Should reset full-screen");
    nsTArray<nsIDocument*>* changed = reinterpret_cast<nsTArray<nsIDocument*>*>(aData);
    changed->AppendElement(aDocument);

    if (HasCrossProcessParent(aDocument)) {
      
      
      nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
      os->NotifyObservers(aDocument, "ask-parent-to-exit-fullscreen", nullptr);
    }

    
    
    
    
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    os->NotifyObservers(aDocument, "ask-children-to-exit-fullscreen", nullptr);

    aDocument->EnumerateSubDocuments(ResetFullScreen, aData);
  }
  return true;
}

static void
ExitFullscreenInDocTree(nsIDocument* aMaybeNotARootDoc)
{
  MOZ_ASSERT(aMaybeNotARootDoc);
  nsCOMPtr<nsIDocument> root = aMaybeNotARootDoc->GetFullscreenRoot();
  NS_ASSERTION(root, "Should have root when in fullscreen!");
  if (!root) {
    return;
  }
  NS_ASSERTION(root->IsFullScreenDoc(),
    "Fullscreen root should be a fullscreen doc...");

  
  
  
  
  
  nsAutoTArray<nsIDocument*, 8> changed;

  
  ResetFullScreen(root, static_cast<void*>(&changed));

  
  
  
  for (uint32_t i = 0; i < changed.Length(); ++i) {
    DispatchFullScreenChange(changed[changed.Length() - i - 1]);
  }

  NS_ASSERTION(!root->IsFullScreenDoc(),
    "Fullscreen root should no longer be a fullscreen doc...");

  
  SetWindowFullScreen(root, false);
}


void
nsDocument::ExitFullscreen(nsIDocument* aDoc)
{
  
  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(EventStateManager::sPointerLockedElement);
  if (pointerLockedElement) {
    UnlockPointer();
  }

  if (aDoc)  {
    ExitFullscreenInDocTree(aDoc);
    return;
  }

  
  FullscreenRoots::ForEach(&ExitFullscreenInDocTree);
  NS_ASSERTION(FullscreenRoots::IsEmpty(),
      "Should have exited all fullscreen roots from fullscreen");
}

bool
GetFullscreenLeaf(nsIDocument* aDoc, void* aData)
{
  if (aDoc->IsFullscreenLeaf()) {
    nsIDocument** result = static_cast<nsIDocument**>(aData);
    *result = aDoc;
    return false;
  } else if (aDoc->IsFullScreenDoc()) {
    aDoc->EnumerateSubDocuments(GetFullscreenLeaf, aData);
  }
  return true;
}

static nsIDocument*
GetFullscreenLeaf(nsIDocument* aDoc)
{
  nsIDocument* leaf = nullptr;
  GetFullscreenLeaf(aDoc, &leaf);
  if (leaf) {
    return leaf;
  }
  
  
  nsIDocument* root = GetFullscreenRootDocument(aDoc);
  
  
  if (!root->IsFullScreenDoc()) {
    return nullptr;
  }
  GetFullscreenLeaf(root, &leaf);
  return leaf;
}

void
nsDocument::RestorePreviousFullScreenState()
{
  NS_ASSERTION(!IsFullScreenDoc() || !FullscreenRoots::IsEmpty(),
    "Should have at least 1 fullscreen root when fullscreen!");
  NS_ASSERTION(!nsContentUtils::IsFullscreenApiContentOnly() ||
               !nsContentUtils::IsChromeDoc(this),
               "Should not run RestorePreviousFullScreenState() on "
               "chrome document when fullscreen is content only");

  if (!IsFullScreenDoc() || !GetWindow() || FullscreenRoots::IsEmpty()) {
    return;
  }

  
  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(EventStateManager::sPointerLockedElement);
  if (pointerLockedElement) {
    UnlockPointer();
  }

  nsCOMPtr<nsIDocument> fullScreenDoc = GetFullscreenLeaf(this);

  
  
  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  os->NotifyObservers(fullScreenDoc, "ask-children-to-exit-fullscreen", nullptr);

  
  nsIDocument* doc = fullScreenDoc;
  while (doc != this) {
    NS_ASSERTION(doc->IsFullScreenDoc(), "Should be full-screen doc");
    static_cast<nsDocument*>(doc)->CleanupFullscreenState();
    UnlockPointer();
    DispatchFullScreenChange(doc);
    doc = doc->GetParentDocument();
  }

  
  NS_ASSERTION(doc == this, "Must have reached this doc.");
  while (doc != nullptr) {
    static_cast<nsDocument*>(doc)->FullScreenStackPop();
    UnlockPointer();
    DispatchFullScreenChange(doc);
    if (static_cast<nsDocument*>(doc)->mFullScreenStack.IsEmpty()) {
      if (HasCrossProcessParent(doc)) {
        
        
        nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
        os->NotifyObservers(doc, "ask-parent-to-rollback-fullscreen", nullptr);
      }
      
      
      
      static_cast<nsDocument*>(doc)->CleanupFullscreenState();
      doc = doc->GetParentDocument();
    } else {
      
      
      if (fullScreenDoc != doc) {
        
        
        
        
        if (!nsContentUtils::HaveEqualPrincipals(fullScreenDoc, doc) ||
            (!nsContentUtils::IsSitePermAllow(doc->NodePrincipal(), "fullscreen") &&
             !static_cast<nsDocument*>(doc)->mIsApprovedForFullscreen)) {
          nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
            new AsyncEventDispatcher(doc,
                  NS_LITERAL_STRING("MozEnteredDomFullscreen"),
                  true,
                  true);
          asyncDispatcher->PostDOMEvent();
        }
      }

      if (!nsContentUtils::HaveEqualPrincipals(doc, fullScreenDoc)) {
        
        
        
        nsAutoString origin;
        nsContentUtils::GetUTFOrigin(doc->NodePrincipal(), origin);
        nsIDocument* root = GetFullscreenRootDocument(doc);
        nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
        os->NotifyObservers(root, "fullscreen-origin-change", origin.get());
      }

      break;
    }
  }

  if (doc == nullptr) {
    
    
    NS_ASSERTION(!GetFullscreenRootDocument(this)->IsFullScreenDoc(),
      "Should have cleared all docs' stacks");
    SetWindowFullScreen(this, false);
  }
}

bool
nsDocument::IsFullScreenDoc()
{
  return GetFullScreenElement() != nullptr;
}

FullScreenOptions::FullScreenOptions()
{
}

class nsCallRequestFullScreen : public nsRunnable
{
public:
  explicit nsCallRequestFullScreen(Element* aElement, FullScreenOptions& aOptions)
    : mElement(aElement),
      mDoc(aElement->OwnerDoc()),
      mWasCallerChrome(nsContentUtils::IsCallerChrome()),
      mHadRequestPending(static_cast<nsDocument*>(mDoc.get())->
                         mAsyncFullscreenPending),
      mOptions(aOptions)
  {
    static_cast<nsDocument*>(mDoc.get())->
      mAsyncFullscreenPending = true;
  }

  NS_IMETHOD Run()
  {
    static_cast<nsDocument*>(mDoc.get())->
      mAsyncFullscreenPending = mHadRequestPending;
    nsDocument* doc = static_cast<nsDocument*>(mDoc.get());
    doc->RequestFullScreen(mElement,
                           mOptions,
                           mWasCallerChrome,
                            true);
    return NS_OK;
  }

  nsRefPtr<Element> mElement;
  nsCOMPtr<nsIDocument> mDoc;
  bool mWasCallerChrome;
  bool mHadRequestPending;
  FullScreenOptions mOptions;
};

void
nsDocument::AsyncRequestFullScreen(Element* aElement,
                                   FullScreenOptions& aOptions)
{
  NS_ASSERTION(aElement,
    "Must pass non-null element to nsDocument::AsyncRequestFullScreen");
  if (!aElement) {
    return;
  }
  
  nsCOMPtr<nsIRunnable> event(new nsCallRequestFullScreen(aElement, aOptions));
  NS_DispatchToCurrentThread(event);
}

static void
LogFullScreenDenied(bool aLogFailure, const char* aMessage, nsIDocument* aDoc)
{
  if (!aLogFailure) {
    return;
  }
  nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
    new AsyncEventDispatcher(aDoc,
                             NS_LITERAL_STRING("mozfullscreenerror"),
                             true,
                             false);
  asyncDispatcher->PostDOMEvent();
  nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                  NS_LITERAL_CSTRING("DOM"), aDoc,
                                  nsContentUtils::eDOM_PROPERTIES,
                                  aMessage);
}

nsresult
nsDocument::AddFullscreenApprovedObserver()
{
  if (mHasFullscreenApprovedObserver ||
      !Preferences::GetBool("full-screen-api.approval-required")) {
    return NS_OK;
  }

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);

  nsresult res = os->AddObserver(this, "fullscreen-approved", true);
  NS_ENSURE_SUCCESS(res, res);

  mHasFullscreenApprovedObserver = true;

  return NS_OK;
}

nsresult
nsDocument::RemoveFullscreenApprovedObserver()
{
  if (!mHasFullscreenApprovedObserver) {
    return NS_OK;
  }
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);

  nsresult res = os->RemoveObserver(this, "fullscreen-approved");
  NS_ENSURE_SUCCESS(res, res);

  mHasFullscreenApprovedObserver = false;

  return NS_OK;
}

void
nsDocument::CleanupFullscreenState()
{
  if (!mFullScreenStack.IsEmpty()) {
    
    
    
    Element* top = FullScreenStackTop();
    NS_ASSERTION(top, "Should have a top when full-screen stack isn't empty");
    if (top) {
      
      top->DeleteProperty(nsGkAtoms::vr_state);

      EventStateManager::SetFullScreenState(top, false);
    }
    mFullScreenStack.Clear();
  }
  SetApprovedForFullscreen(false);
  RemoveFullscreenApprovedObserver();
  mFullscreenRoot = nullptr;
}

bool
nsDocument::FullScreenStackPush(Element* aElement)
{
  NS_ASSERTION(aElement, "Must pass non-null to FullScreenStackPush()");
  Element* top = FullScreenStackTop();
  if (top == aElement || !aElement) {
    return false;
  }
  if (top) {
    
    
    
    EventStateManager::SetFullScreenState(top, false);
  }
  EventStateManager::SetFullScreenState(aElement, true);
  mFullScreenStack.AppendElement(do_GetWeakReference(aElement));
  NS_ASSERTION(GetFullScreenElement() == aElement, "Should match");
  return true;
}

void
nsDocument::FullScreenStackPop()
{
  if (mFullScreenStack.IsEmpty()) {
    return;
  }

  Element* top = FullScreenStackTop();

  
  top->DeleteProperty(nsGkAtoms::vr_state);

  
  EventStateManager::SetFullScreenState(top, false);

  
  
  
  uint32_t last = mFullScreenStack.Length() - 1;
  mFullScreenStack.RemoveElementAt(last);

  
  
  
  while (!mFullScreenStack.IsEmpty()) {
    Element* element = FullScreenStackTop();
    if (!element || !element->IsInDoc() || element->OwnerDoc() != this) {
      NS_ASSERTION(!element->IsFullScreenAncestor(),
                   "Should have already removed full-screen styles");
      uint32_t last = mFullScreenStack.Length() - 1;
      mFullScreenStack.RemoveElementAt(last);
    } else {
      
      
      EventStateManager::SetFullScreenState(element, true);
      break;
    }
  }
}

Element*
nsDocument::FullScreenStackTop()
{
  if (mFullScreenStack.IsEmpty()) {
    return nullptr;
  }
  uint32_t last = mFullScreenStack.Length() - 1;
  nsCOMPtr<Element> element(do_QueryReferent(mFullScreenStack[last]));
  NS_ASSERTION(element, "Should have full-screen element!");
  NS_ASSERTION(element->IsInDoc(), "Full-screen element should be in doc");
  NS_ASSERTION(element->OwnerDoc() == this, "Full-screen element should be in this doc");
  return element;
}


static bool
IsInActiveTab(nsIDocument* aDoc)
{
  nsCOMPtr<nsIDocShell> docshell = aDoc->GetDocShell();
  if (!docshell) {
    return false;
  }

  bool isActive = false;
  docshell->GetIsActive(&isActive);
  if (!isActive) {
    return false;
  }

  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  docshell->GetRootTreeItem(getter_AddRefs(rootItem));
  if (!rootItem) {
    return false;
  }
  nsCOMPtr<nsIDOMWindow> rootWin = rootItem->GetWindow();
  if (!rootWin) {
    return false;
  }

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm) {
    return false;
  }

  nsCOMPtr<nsIDOMWindow> activeWindow;
  fm->GetActiveWindow(getter_AddRefs(activeWindow));
  if (!activeWindow) {
    return false;
  }

  return activeWindow == rootWin;
}

nsresult nsDocument::RemoteFrameFullscreenChanged(nsIDOMElement* aFrameElement,
                                                  const nsAString& aOrigin)
{
  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(aFrameElement));
  FullScreenOptions opts;
  RequestFullScreen(content->AsElement(),
                    opts,
                     false,
                     false);

  
  
  
  
  
  
  if (!aOrigin.IsEmpty()) {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    os->NotifyObservers(GetFullscreenRootDocument(this),
                        "fullscreen-origin-change",
                        PromiseFlatString(aOrigin).get());
  }

  return NS_OK;
}

nsresult nsDocument::RemoteFrameFullscreenReverted()
{
  RestorePreviousFullScreenState();
  return NS_OK;
}

static void
ReleaseHMDInfoRef(void *, nsIAtom*, void *aPropertyValue, void *)
{
  if (aPropertyValue) {
    static_cast<gfx::VRHMDInfo*>(aPropertyValue)->Release();
  }
}

void
nsDocument::RequestFullScreen(Element* aElement,
                              FullScreenOptions& aOptions,
                              bool aWasCallerChrome,
                              bool aNotifyOnOriginChange)
{
  NS_ASSERTION(aElement,
    "Must pass non-null element to nsDocument::RequestFullScreen");
  if (!aElement || aElement == GetFullScreenElement()) {
    return;
  }
  if (!aElement->IsInDoc()) {
    LogFullScreenDenied(true, "FullScreenDeniedNotInDocument", this);
    return;
  }
  if (aElement->OwnerDoc() != this) {
    LogFullScreenDenied(true, "FullScreenDeniedMovedDocument", this);
    return;
  }
  if (!GetWindow()) {
    LogFullScreenDenied(true, "FullScreenDeniedLostWindow", this);
    return;
  }
  if (nsContentUtils::IsFullscreenApiContentOnly() &&
      nsContentUtils::IsChromeDoc(this)) {
    
    
    LogFullScreenDenied(true, "FullScreenDeniedContentOnly", this);
    return;
  }
  if (!IsFullScreenEnabled(aWasCallerChrome, true)) {
    
    return;
  }
  if (GetFullScreenElement() &&
      !nsContentUtils::ContentIsDescendantOf(aElement, GetFullScreenElement())) {
    
    
    LogFullScreenDenied(true, "FullScreenDeniedNotDescendant", this);
    return;
  }
  if (!nsContentUtils::IsChromeDoc(this) && !IsInActiveTab(this)) {
    LogFullScreenDenied(true, "FullScreenDeniedNotFocusedTab", this);
    return;
  }
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm) {
    NS_WARNING("Failed to retrieve focus manager in full-screen request.");
    return;
  }
  nsCOMPtr<nsIDOMElement> focusedElement;
  fm->GetFocusedElement(getter_AddRefs(focusedElement));
  if (focusedElement) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(focusedElement);
    if (nsContentUtils::HasPluginWithUncontrolledEventDispatch(content)) {
      LogFullScreenDenied(true, "FullScreenDeniedFocusedPlugin", this);
      return;
    }
  }

  
  
  nsCOMPtr<nsIDocument> previousFullscreenDoc = GetFullscreenLeaf(this);

  AddFullscreenApprovedObserver();

  
  
  
  
  
  nsAutoTArray<nsIDocument*, 8> changed;

  
  
  nsIDocument* fullScreenRootDoc = GetFullscreenRootDocument(this);
  if (fullScreenRootDoc->IsFullScreenDoc()) {
    
    
    UnlockPointer();
  }

  
  
  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(EventStateManager::sPointerLockedElement);
  if (pointerLockedElement) {
    UnlockPointer();
  }

  
  if (aOptions.mVRHMDDevice) {
    nsRefPtr<gfx::VRHMDInfo> hmdRef = aOptions.mVRHMDDevice;
    aElement->SetProperty(nsGkAtoms::vr_state, hmdRef.forget().take(),
                          ReleaseHMDInfoRef,
                          true);
  }

  
  
  
  DebugOnly<bool> x = FullScreenStackPush(aElement);
  NS_ASSERTION(x, "Full-screen state of requesting doc should always change!");
  changed.AppendElement(this);

  
  
  
  
  
  nsIDocument* child = this;
  while (true) {
    child->SetFullscreenRoot(fullScreenRootDoc);
    NS_ASSERTION(child->GetFullscreenRoot() == fullScreenRootDoc,
        "Fullscreen root should be set!");
    if (child == fullScreenRootDoc) {
      break;
    }
    nsIDocument* parent = child->GetParentDocument();
    Element* element = parent->FindContentForSubDocument(child)->AsElement();
    if (static_cast<nsDocument*>(parent)->FullScreenStackPush(element)) {
      changed.AppendElement(parent);
      child = parent;
    } else {
      
      
      
      
      break;
    }
  }

  
  
  
  for (uint32_t i = 0; i < changed.Length(); ++i) {
    DispatchFullScreenChange(changed[changed.Length() - i - 1]);
  }

  
  
  
  
  
  if (!mIsApprovedForFullscreen) {
    mIsApprovedForFullscreen =
      !Preferences::GetBool("full-screen-api.approval-required") ||
      NodePrincipal()->GetAppStatus() >= nsIPrincipal::APP_STATUS_INSTALLED ||
      nsContentUtils::IsSitePermAllow(NodePrincipal(), "fullscreen");
  }

  
  
  
  
  
  
  if (!mIsApprovedForFullscreen ||
      !nsContentUtils::HaveEqualPrincipals(previousFullscreenDoc, this)) {
    nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
      new AsyncEventDispatcher(this,
                               NS_LITERAL_STRING("MozEnteredDomFullscreen"),
                               true,
                               true);
    asyncDispatcher->PostDOMEvent();
  }

#ifdef DEBUG
  
  
  NS_ASSERTION(GetFullScreenElement() == aElement,
               "Full-screen element should be the requested element!");
  NS_ASSERTION(IsFullScreenDoc(), "Should be full-screen doc");
  nsCOMPtr<nsIDOMElement> fse;
  GetMozFullScreenElement(getter_AddRefs(fse));
  nsCOMPtr<nsIContent> c(do_QueryInterface(fse));
  NS_ASSERTION(c->AsElement() == aElement,
    "GetMozFullScreenElement should match GetFullScreenElement()");
#endif

  
  
  
  
  
  
  if (aNotifyOnOriginChange &&
      !nsContentUtils::HaveEqualPrincipals(previousFullscreenDoc, this)) {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    nsIDocument* root = GetFullscreenRootDocument(this);
    nsAutoString origin;
    nsContentUtils::GetUTFOrigin(NodePrincipal(), origin);
    os->NotifyObservers(root, "fullscreen-origin-change", origin.get());
  }

  
  
  
  
  
  
  
  SetWindowFullScreen(this, true, aOptions.mVRHMDDevice);
}

NS_IMETHODIMP
nsDocument::GetMozFullScreenElement(nsIDOMElement **aFullScreenElement)
{
  ErrorResult rv;
  Element* el = GetMozFullScreenElement(rv);
  if (rv.Failed()) {
    return rv.StealNSResult();
  }
  nsCOMPtr<nsIDOMElement> retval = do_QueryInterface(el);
  retval.forget(aFullScreenElement);
  return NS_OK;
}

Element*
nsDocument::GetMozFullScreenElement(ErrorResult& rv)
{
  if (IsFullScreenDoc()) {
    
    Element* el = GetFullScreenElement();
    if (!el) {
      rv.Throw(NS_ERROR_UNEXPECTED);
    }
    return el;
  }
  return nullptr;
}

Element*
nsDocument::GetFullScreenElement()
{
  Element* element = FullScreenStackTop();
  NS_ASSERTION(!element ||
               element->IsFullScreenAncestor(),
    "Fullscreen element should have fullscreen styles applied");
  return element;
}

NS_IMETHODIMP
nsDocument::GetMozFullScreen(bool *aFullScreen)
{
  *aFullScreen = MozFullScreen();
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetMozFullScreenEnabled(bool *aFullScreen)
{
  NS_ENSURE_ARG_POINTER(aFullScreen);
  *aFullScreen = MozFullScreenEnabled();
  return NS_OK;
}

bool
nsDocument::MozFullScreenEnabled()
{
  return IsFullScreenEnabled(nsContentUtils::IsCallerChrome(), false);
}

static bool
HasFullScreenSubDocument(nsIDocument* aDoc)
{
  uint32_t count = CountFullscreenSubDocuments(aDoc);
  NS_ASSERTION(count <= 1, "Fullscreen docs should have at most 1 fullscreen child!");
  return count >= 1;
}

bool
nsDocument::IsFullScreenEnabled(bool aCallerIsChrome, bool aLogFailure)
{
  if (nsContentUtils::IsFullScreenApiEnabled() && aCallerIsChrome) {
    
    
    
    
    return true;
  }

  if (!nsContentUtils::IsFullScreenApiEnabled()) {
    LogFullScreenDenied(aLogFailure, "FullScreenDeniedDisabled", this);
    return false;
  }
  if (!IsVisible()) {
    LogFullScreenDenied(aLogFailure, "FullScreenDeniedHidden", this);
    return false;
  }
  if (HasFullScreenSubDocument(this)) {
    LogFullScreenDenied(aLogFailure, "FullScreenDeniedSubDocFullScreen", this);
    return false;
  }

  
  
  nsCOMPtr<nsIDocShell> docShell(mDocumentContainer);
  bool allowed = false;
  if (docShell) {
    docShell->GetFullscreenAllowed(&allowed);
  }
  if (!allowed) {
    LogFullScreenDenied(aLogFailure, "FullScreenDeniedIframeNotAllowed", this);
  }

  return allowed;
}

static void
DispatchPointerLockChange(nsIDocument* aTarget)
{
  if (!aTarget) {
    return;
  }

  nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
    new AsyncEventDispatcher(aTarget,
                             NS_LITERAL_STRING("mozpointerlockchange"),
                             true,
                             false);
  asyncDispatcher->PostDOMEvent();
}

static void
DispatchPointerLockError(nsIDocument* aTarget)
{
  if (!aTarget) {
    return;
  }

  nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
    new AsyncEventDispatcher(aTarget,
                             NS_LITERAL_STRING("mozpointerlockerror"),
                             true,
                             false);
  asyncDispatcher->PostDOMEvent();
}

mozilla::StaticRefPtr<nsPointerLockPermissionRequest> gPendingPointerLockRequest;

class nsPointerLockPermissionRequest : public nsRunnable,
                                       public nsIContentPermissionRequest
{
public:
  nsPointerLockPermissionRequest(Element* aElement, bool aUserInputOrChromeCaller)
  : mElement(do_GetWeakReference(aElement)),
    mDocument(do_GetWeakReference(aElement->OwnerDoc())),
    mUserInputOrChromeCaller(aUserInputOrChromeCaller)
  {
    nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument);
    if (doc) {
      mRequester = new nsContentPermissionRequester(doc->GetInnerWindow());
    }
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICONTENTPERMISSIONREQUEST

  NS_IMETHOD Run() override
  {
    nsCOMPtr<Element> e = do_QueryReferent(mElement);
    nsCOMPtr<nsIDocument> d = do_QueryReferent(mDocument);
    if (!e || !d || gPendingPointerLockRequest != this ||
        e->GetUncomposedDoc() != d) {
      Handled();
      DispatchPointerLockError(d);
      return NS_OK;
    }

    
    nsDocument* doc = static_cast<nsDocument*>(d.get());
    if (doc->mAsyncFullscreenPending ||
        (doc->mHasFullscreenApprovedObserver && !doc->mIsApprovedForFullscreen)) {
      
      return NS_OK;
    }

    if (doc->mIsApprovedForFullscreen || doc->mAllowRelocking) {
      Allow(JS::UndefinedHandleValue);
      return NS_OK;
    }

    
    
    if (!mUserInputOrChromeCaller ||
        doc->mCancelledPointerLockRequests > 2) {
      Handled();
      DispatchPointerLockError(d);
      return NS_OK;
    }

    
    
    nsCOMPtr<nsPIDOMWindow> window = doc->GetInnerWindow();
    nsContentPermissionUtils::AskPermission(this, window);
    return NS_OK;
  }

  void Handled()
  {
    mElement = nullptr;
    mDocument = nullptr;
    if (gPendingPointerLockRequest == this) {
      gPendingPointerLockRequest = nullptr;
    }
  }

  nsWeakPtr mElement;
  nsWeakPtr mDocument;
  bool mUserInputOrChromeCaller;

protected:
  virtual ~nsPointerLockPermissionRequest() {}
  nsCOMPtr<nsIContentPermissionRequester> mRequester;
};

NS_IMPL_ISUPPORTS_INHERITED(nsPointerLockPermissionRequest,
                            nsRunnable,
                            nsIContentPermissionRequest)

NS_IMETHODIMP
nsPointerLockPermissionRequest::GetTypes(nsIArray** aTypes)
{
  nsTArray<nsString> emptyOptions;
  return nsContentPermissionUtils::CreatePermissionArray(NS_LITERAL_CSTRING("pointerLock"),
                                                         NS_LITERAL_CSTRING("unused"),
                                                         emptyOptions,
                                                         aTypes);
}

NS_IMETHODIMP
nsPointerLockPermissionRequest::GetPrincipal(nsIPrincipal** aPrincipal)
{
  nsCOMPtr<nsIDocument> d = do_QueryReferent(mDocument);
  if (d) {
    NS_ADDREF(*aPrincipal = d->NodePrincipal());
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPointerLockPermissionRequest::GetWindow(nsIDOMWindow** aWindow)
{
  nsCOMPtr<nsIDocument> d = do_QueryReferent(mDocument);
  if (d) {
    NS_IF_ADDREF(*aWindow = d->GetInnerWindow());
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPointerLockPermissionRequest::GetElement(nsIDOMElement** aElement)
{
  
  *aElement = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsPointerLockPermissionRequest::Cancel()
{
  nsCOMPtr<nsIDocument> d = do_QueryReferent(mDocument);
  Handled();
  if (d) {
    static_cast<nsDocument*>(d.get())->mCancelledPointerLockRequests++;
    DispatchPointerLockError(d);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPointerLockPermissionRequest::Allow(JS::HandleValue aChoices)
{
  MOZ_ASSERT(aChoices.isUndefined());

  nsCOMPtr<Element> e = do_QueryReferent(mElement);
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument);
  nsDocument* d = static_cast<nsDocument*>(doc.get());
  if (!e || !d || gPendingPointerLockRequest != this ||
      e->GetUncomposedDoc() != d ||
      (!mUserInputOrChromeCaller && !d->mIsApprovedForFullscreen)) {
    Handled();
    DispatchPointerLockError(d);
    return NS_OK;
  }

  
  Handled();

  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(EventStateManager::sPointerLockedElement);
  if (e == pointerLockedElement) {
    DispatchPointerLockChange(d);
    return NS_OK;
  }

  
  if (!d->ShouldLockPointer(e, pointerLockedElement, true)) {
    DispatchPointerLockError(d);
    return NS_OK;
  }

  if (!d->SetPointerLock(e, NS_STYLE_CURSOR_NONE)) {
    DispatchPointerLockError(d);
    return NS_OK;
  }

  d->mCancelledPointerLockRequests = 0;
  e->SetPointerLock();
  EventStateManager::sPointerLockedElement = do_GetWeakReference(e);
  EventStateManager::sPointerLockedDoc = do_GetWeakReference(doc);
  NS_ASSERTION(EventStateManager::sPointerLockedElement &&
               EventStateManager::sPointerLockedDoc,
               "aElement and this should support weak references!");

  DispatchPointerLockChange(d);
  return NS_OK;
}

NS_IMETHODIMP
nsPointerLockPermissionRequest::GetRequester(nsIContentPermissionRequester** aRequester)
{
  NS_ENSURE_ARG_POINTER(aRequester);

  nsCOMPtr<nsIContentPermissionRequester> requester = mRequester;
  requester.forget(aRequester);
  return NS_OK;
}

void
nsDocument::SetApprovedForFullscreen(bool aIsApproved)
{
  mIsApprovedForFullscreen = aIsApproved;
}

nsresult
nsDocument::Observe(nsISupports *aSubject,
                    const char *aTopic,
                    const char16_t *aData)
{
  if (strcmp("fullscreen-approved", aTopic) == 0) {
    nsCOMPtr<nsIDocument> subject(do_QueryInterface(aSubject));
    if (subject != this) {
      return NS_OK;
    }
    SetApprovedForFullscreen(true);
    if (gPendingPointerLockRequest) {
      
      
      nsCOMPtr<Element> el =
        do_QueryReferent(gPendingPointerLockRequest->mElement);
      nsCOMPtr<nsIDocument> doc =
        do_QueryReferent(gPendingPointerLockRequest->mDocument);
      bool userInputOrChromeCaller =
        gPendingPointerLockRequest->mUserInputOrChromeCaller;
      gPendingPointerLockRequest->Handled();
      if (doc == this && el && el->GetUncomposedDoc() == doc) {
        nsPointerLockPermissionRequest* clone =
          new nsPointerLockPermissionRequest(el, userInputOrChromeCaller);
        gPendingPointerLockRequest = clone;
        nsCOMPtr<nsIRunnable> r = gPendingPointerLockRequest.get();
        NS_DispatchToMainThread(r);
      }
    }
  } else if (strcmp("app-theme-changed", aTopic) == 0) {
    if (!nsContentUtils::IsSystemPrincipal(NodePrincipal()) &&
        !IsUnstyledDocument()) {
      
      OnAppThemeChanged();
    }
  }
  return NS_OK;
}

void
nsDocument::OnAppThemeChanged()
{
  
  auto themeOrigin = Preferences::GetString("b2g.theme.origin");
  if (!themeOrigin || !Preferences::GetBool("dom.mozApps.themable")) {
    return;
  }

  for (int32_t i = 0; i < GetNumberOfStyleSheets(); i++) {
    nsRefPtr<CSSStyleSheet> sheet =  do_QueryObject(GetStyleSheetAt(i));
    if (!sheet) {
      continue;
    }

    nsINode* owningNode = sheet->GetOwnerNode();
    if (!owningNode) {
      continue;
    }
    
    nsIURI* sheetURI = sheet->GetOriginalURI();
    if (!sheetURI) {
      continue;
    }
    nsAutoString sheetOrigin;
    nsContentUtils::GetUTFOrigin(sheetURI, sheetOrigin);
    if (!sheetOrigin.Equals(themeOrigin)) {
      continue;
    }

    
    nsCOMPtr<nsIStyleSheetLinkingElement> link = do_QueryInterface(owningNode);
    if (!link) {
      continue;
    }
    bool willNotify;
    bool isAlternate;
    link->UpdateStyleSheet(nullptr, &willNotify, &isAlternate, true);
  }
}

void
nsDocument::RequestPointerLock(Element* aElement)
{
  NS_ASSERTION(aElement,
    "Must pass non-null element to nsDocument::RequestPointerLock");

  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(EventStateManager::sPointerLockedElement);
  if (aElement == pointerLockedElement) {
    DispatchPointerLockChange(this);
    return;
  }

  if (!ShouldLockPointer(aElement, pointerLockedElement)) {
    DispatchPointerLockError(this);
    return;
  }

  bool userInputOrChromeCaller = EventStateManager::IsHandlingUserInput() ||
                                 nsContentUtils::IsCallerChrome();

  gPendingPointerLockRequest =
    new nsPointerLockPermissionRequest(aElement, userInputOrChromeCaller);
  nsCOMPtr<nsIRunnable> r = gPendingPointerLockRequest.get();
  NS_DispatchToMainThread(r);
}

bool
nsDocument::ShouldLockPointer(Element* aElement, Element* aCurrentLock,
                              bool aNoFocusCheck)
{
  
  if (!Preferences::GetBool("full-screen-api.pointer-lock.enabled")) {
    NS_WARNING("ShouldLockPointer(): Pointer Lock pref not enabled");
    return false;
  }

  if (aCurrentLock && aCurrentLock->OwnerDoc() != aElement->OwnerDoc()) {
    NS_WARNING("ShouldLockPointer(): Existing pointer lock element in a different document");
    return false;
  }

  if (!aElement->IsInDoc()) {
    NS_WARNING("ShouldLockPointer(): Element without Document");
    return false;
  }

  if (mSandboxFlags & SANDBOXED_POINTER_LOCK) {
    NS_WARNING("ShouldLockPointer(): Document is sandboxed and doesn't allow pointer-lock");
    return false;
  }

  
  nsCOMPtr<nsIDocument> ownerDoc = aElement->OwnerDoc();
  if (!ownerDoc->GetContainer()) {
    return false;
  }
  nsCOMPtr<nsPIDOMWindow> ownerWindow = ownerDoc->GetWindow();
  if (!ownerWindow) {
    return false;
  }
  nsCOMPtr<nsPIDOMWindow> ownerInnerWindow = ownerDoc->GetInnerWindow();
  if (!ownerInnerWindow) {
    return false;
  }
  if (ownerWindow->GetCurrentInnerWindow() != ownerInnerWindow) {
    return false;
  }

  nsCOMPtr<nsIDOMWindow> top;
  ownerWindow->GetScriptableTop(getter_AddRefs(top));
  nsCOMPtr<nsPIDOMWindow> piTop = do_QueryInterface(top);
  if (!piTop || !piTop->GetExtantDoc() ||
      piTop->GetExtantDoc()->Hidden()) {
    NS_WARNING("ShouldLockPointer(): Top document isn't visible.");
    return false;
  }

  if (!aNoFocusCheck) {
    mozilla::ErrorResult rv;
    if (!piTop->GetExtantDoc()->HasFocus(rv)) {
      NS_WARNING("ShouldLockPointer(): Top document isn't focused.");
      return false;
    }
  }

  return true;
}

bool
nsDocument::SetPointerLock(Element* aElement, int aCursorStyle)
{
  
  nsCOMPtr<nsPIDOMWindow> window = GetWindow();
  if (!window) {
    NS_WARNING("SetPointerLock(): No Window");
    return false;
  }

  nsIDocShell *docShell = window->GetDocShell();
  if (!docShell) {
    NS_WARNING("SetPointerLock(): No DocShell (window already closed?)");
    return false;
  }

  nsRefPtr<nsPresContext> presContext;
  docShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) {
    NS_WARNING("SetPointerLock(): Unable to get presContext in \
                domWindow->GetDocShell()->GetPresContext()");
    return false;
  }

  nsCOMPtr<nsIPresShell> shell = presContext->PresShell();
  if (!shell) {
    NS_WARNING("SetPointerLock(): Unable to find presContext->PresShell()");
    return false;
  }

  nsIFrame* rootFrame = shell->GetRootFrame();
  if (!rootFrame) {
    NS_WARNING("SetPointerLock(): Unable to get root frame");
    return false;
  }

  nsCOMPtr<nsIWidget> widget = rootFrame->GetNearestWidget();
  if (!widget) {
    NS_WARNING("SetPointerLock(): Unable to find widget in \
                shell->GetRootFrame()->GetNearestWidget();");
    return false;
  }

  if (aElement && (aElement->OwnerDoc() != this)) {
    NS_WARNING("SetPointerLock(): Element not in this document.");
    return false;
  }

  
  nsRefPtr<EventStateManager> esm = presContext->EventStateManager();
  esm->SetCursor(aCursorStyle, nullptr, false,
                 0.0f, 0.0f, widget, true);
  esm->SetPointerLock(widget, aElement);

  return true;
}

void
nsDocument::UnlockPointer(nsIDocument* aDoc)
{
  if (!EventStateManager::sIsPointerLocked) {
    return;
  }

  nsCOMPtr<nsIDocument> pointerLockedDoc =
    do_QueryReferent(EventStateManager::sPointerLockedDoc);
  if (!pointerLockedDoc || (aDoc && aDoc != pointerLockedDoc)) {
    return;
  }
  nsDocument* doc = static_cast<nsDocument*>(pointerLockedDoc.get());
  if (!doc->SetPointerLock(nullptr, NS_STYLE_CURSOR_AUTO)) {
    return;
  }

  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(EventStateManager::sPointerLockedElement);
  if (pointerLockedElement) {
    pointerLockedElement->ClearPointerLock();
  }

  EventStateManager::sPointerLockedElement = nullptr;
  EventStateManager::sPointerLockedDoc = nullptr;
  static_cast<nsDocument*>(pointerLockedDoc.get())->mAllowRelocking = !!aDoc;
  gPendingPointerLockRequest = nullptr;
  DispatchPointerLockChange(pointerLockedDoc);
}

void
nsIDocument::UnlockPointer(nsIDocument* aDoc)
{
  nsDocument::UnlockPointer(aDoc);
}

NS_IMETHODIMP
nsDocument::MozExitPointerLock()
{
  nsIDocument::MozExitPointerLock();
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetMozPointerLockElement(nsIDOMElement** aPointerLockedElement)
{
  Element* el = nsIDocument::GetMozPointerLockElement();
  nsCOMPtr<nsIDOMElement> retval = do_QueryInterface(el);
  retval.forget(aPointerLockedElement);
  return NS_OK;
}

Element*
nsIDocument::GetMozPointerLockElement()
{
  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(EventStateManager::sPointerLockedElement);
  if (!pointerLockedElement) {
    return nullptr;
  }

  
  nsCOMPtr<nsIDocument> pointerLockedDoc =
    do_QueryReferent(EventStateManager::sPointerLockedDoc);
  if (pointerLockedDoc != this) {
    return nullptr;
  }

  return pointerLockedElement;
}

void
nsDocument::XPCOMShutdown()
{
  gPendingPointerLockRequest = nullptr;
  sProcessingStack.reset();
}

void
nsDocument::UpdateVisibilityState()
{
  dom::VisibilityState oldState = mVisibilityState;
  mVisibilityState = GetVisibilityState();
  if (oldState != mVisibilityState) {
    nsContentUtils::DispatchTrustedEvent(this, static_cast<nsIDocument*>(this),
                                         NS_LITERAL_STRING("visibilitychange"),
                                          true,
                                          false);
    nsContentUtils::DispatchTrustedEvent(this, static_cast<nsIDocument*>(this),
                                         NS_LITERAL_STRING("mozvisibilitychange"),
                                          true,
                                          false);

    EnumerateActivityObservers(NotifyActivityChanged, nullptr);
  }
}

VisibilityState
nsDocument::GetVisibilityState() const
{
  
  
  
  
  
  
  
  if (!IsVisible() || !mWindow || !mWindow->GetOuterWindow() ||
      mWindow->GetOuterWindow()->IsBackground()) {
    return dom::VisibilityState::Hidden;
  }

  return dom::VisibilityState::Visible;
}

 void
nsDocument::PostVisibilityUpdateEvent()
{
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &nsDocument::UpdateVisibilityState);
  NS_DispatchToMainThread(event);
}

NS_IMETHODIMP
nsDocument::GetMozHidden(bool* aHidden)
{
  *aHidden = MozHidden();
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetHidden(bool* aHidden)
{
  *aHidden = Hidden();
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetMozVisibilityState(nsAString& aState)
{
  WarnOnceAbout(ePrefixedVisibilityAPI);
  return GetVisibilityState(aState);
}

NS_IMETHODIMP
nsDocument::GetVisibilityState(nsAString& aState)
{
  const EnumEntry& entry =
    VisibilityStateValues::strings[static_cast<int>(mVisibilityState)];
  aState.AssignASCII(entry.value, entry.length);
  return NS_OK;
}

 void
nsIDocument::DocAddSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const
{
  aWindowSizes->mDOMOtherSize +=
    nsINode::SizeOfExcludingThis(aWindowSizes->mMallocSizeOf);

  if (mPresShell) {
    mPresShell->AddSizeOfIncludingThis(aWindowSizes->mMallocSizeOf,
                                       &aWindowSizes->mArenaStats,
                                       &aWindowSizes->mLayoutPresShellSize,
                                       &aWindowSizes->mLayoutStyleSetsSize,
                                       &aWindowSizes->mLayoutTextRunsSize,
                                       &aWindowSizes->mLayoutPresContextSize);
  }

  aWindowSizes->mPropertyTablesSize +=
    mPropertyTable.SizeOfExcludingThis(aWindowSizes->mMallocSizeOf);
  for (uint32_t i = 0, count = mExtraPropertyTables.Length();
       i < count; ++i) {
    aWindowSizes->mPropertyTablesSize +=
      mExtraPropertyTables[i]->SizeOfIncludingThis(aWindowSizes->mMallocSizeOf);
  }

  if (EventListenerManager* elm = GetExistingListenerManager()) {
    aWindowSizes->mDOMEventListenersCount += elm->ListenerCount();
  }

  
  
  
}

void
nsIDocument::DocAddSizeOfIncludingThis(nsWindowSizes* aWindowSizes) const
{
  aWindowSizes->mDOMOtherSize += aWindowSizes->mMallocSizeOf(this);
  DocAddSizeOfExcludingThis(aWindowSizes);
}

static size_t
SizeOfStyleSheetsElementIncludingThis(nsIStyleSheet* aStyleSheet,
                                      MallocSizeOf aMallocSizeOf,
                                      void* aData)
{
  if (!aStyleSheet->GetOwningDocument()) {
    
    return 0;
  }
  return aStyleSheet->SizeOfIncludingThis(aMallocSizeOf);
}

size_t
nsDocument::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  
  
  
  
  MOZ_CRASH();
}

void
nsDocument::DocAddSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const
{
  nsIDocument::DocAddSizeOfExcludingThis(aWindowSizes);

  for (nsIContent* node = nsINode::GetFirstChild();
       node;
       node = node->GetNextNode(this))
  {
    size_t nodeSize = node->SizeOfIncludingThis(aWindowSizes->mMallocSizeOf);
    size_t* p;

    switch (node->NodeType()) {
    case nsIDOMNode::ELEMENT_NODE:
      p = &aWindowSizes->mDOMElementNodesSize;
      break;
    case nsIDOMNode::TEXT_NODE:
      p = &aWindowSizes->mDOMTextNodesSize;
      break;
    case nsIDOMNode::CDATA_SECTION_NODE:
      p = &aWindowSizes->mDOMCDATANodesSize;
      break;
    case nsIDOMNode::COMMENT_NODE:
      p = &aWindowSizes->mDOMCommentNodesSize;
      break;
    default:
      p = &aWindowSizes->mDOMOtherSize;
      break;
    }

    *p += nodeSize;

    if (EventListenerManager* elm = node->GetExistingListenerManager()) {
      aWindowSizes->mDOMEventListenersCount += elm->ListenerCount();
    }
  }

  aWindowSizes->mStyleSheetsSize +=
    mStyleSheets.SizeOfExcludingThis(SizeOfStyleSheetsElementIncludingThis,
                                     aWindowSizes->mMallocSizeOf);
  
  
  
  aWindowSizes->mStyleSheetsSize +=
    mOnDemandBuiltInUASheets.SizeOfExcludingThis(nullptr,
                                                 aWindowSizes->mMallocSizeOf);
  aWindowSizes->mStyleSheetsSize +=
    mAdditionalSheets[eAgentSheet].
      SizeOfExcludingThis(SizeOfStyleSheetsElementIncludingThis,
                          aWindowSizes->mMallocSizeOf);
  aWindowSizes->mStyleSheetsSize +=
    mAdditionalSheets[eUserSheet].
      SizeOfExcludingThis(SizeOfStyleSheetsElementIncludingThis,
                          aWindowSizes->mMallocSizeOf);
  aWindowSizes->mStyleSheetsSize +=
    mAdditionalSheets[eAuthorSheet].
      SizeOfExcludingThis(SizeOfStyleSheetsElementIncludingThis,
                          aWindowSizes->mMallocSizeOf);
  
  
  
  aWindowSizes->mStyleSheetsSize +=
    CSSLoader()->SizeOfIncludingThis(aWindowSizes->mMallocSizeOf);

  aWindowSizes->mDOMOtherSize +=
    mAttrStyleSheet ?
    mAttrStyleSheet->DOMSizeOfIncludingThis(aWindowSizes->mMallocSizeOf) :
    0;

  aWindowSizes->mDOMOtherSize +=
    mSVGAttrAnimationRuleProcessor ?
    mSVGAttrAnimationRuleProcessor->DOMSizeOfIncludingThis(
                                      aWindowSizes->mMallocSizeOf) :
    0;

  aWindowSizes->mDOMOtherSize +=
    mStyledLinks.SizeOfExcludingThis(nullptr, aWindowSizes->mMallocSizeOf);

  aWindowSizes->mDOMOtherSize +=
    mIdentifierMap.SizeOfExcludingThis(aWindowSizes->mMallocSizeOf);

  
  
  
}

NS_IMETHODIMP
nsDocument::QuerySelector(const nsAString& aSelector, nsIDOMElement **aReturn)
{
  return nsINode::QuerySelector(aSelector, aReturn);
}

NS_IMETHODIMP
nsDocument::QuerySelectorAll(const nsAString& aSelector, nsIDOMNodeList **aReturn)
{
  return nsINode::QuerySelectorAll(aSelector, aReturn);
}

already_AddRefed<nsIDocument>
nsIDocument::Constructor(const GlobalObject& aGlobal,
                         ErrorResult& rv)
{
  nsCOMPtr<nsIScriptGlobalObject> global = do_QueryInterface(aGlobal.GetAsSupports());
  if (!global) {
    rv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsCOMPtr<nsIScriptObjectPrincipal> prin = do_QueryInterface(aGlobal.GetAsSupports());
  if (!prin) {
    rv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), "about:blank");
  if (!uri) {
    rv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  nsCOMPtr<nsIDOMDocument> document;
  nsresult res =
    NS_NewDOMDocument(getter_AddRefs(document),
                      NullString(),
                      EmptyString(),
                      nullptr,
                      uri,
                      uri,
                      prin->GetPrincipal(),
                      true,
                      global,
                      DocumentFlavorPlain);
  if (NS_FAILED(res)) {
    rv.Throw(res);
    return nullptr;
  }

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(document);
  doc->SetReadyStateInternal(nsIDocument::READYSTATE_COMPLETE);

  return doc.forget();
}

XPathExpression*
nsIDocument::CreateExpression(const nsAString& aExpression,
                              XPathNSResolver* aResolver,
                              ErrorResult& rv)
{
  return XPathEvaluator()->CreateExpression(aExpression, aResolver, rv);
}

nsINode*
nsIDocument::CreateNSResolver(nsINode& aNodeResolver)
{
  return XPathEvaluator()->CreateNSResolver(aNodeResolver);
}

already_AddRefed<XPathResult>
nsIDocument::Evaluate(JSContext* aCx, const nsAString& aExpression,
                      nsINode& aContextNode, XPathNSResolver* aResolver,
                      uint16_t aType, JS::Handle<JSObject*> aResult,
                      ErrorResult& rv)
{
  return XPathEvaluator()->Evaluate(aCx, aExpression, aContextNode, aResolver,
                                    aType, aResult, rv);
}

NS_IMETHODIMP
nsDocument::Evaluate(const nsAString& aExpression, nsIDOMNode* aContextNode,
                     nsIDOMNode* aResolver, uint16_t aType,
                     nsISupports* aInResult, nsISupports** aResult)
{
  return XPathEvaluator()->Evaluate(aExpression, aContextNode, aResolver, aType,
                                    aInResult, aResult);
}

XPathEvaluator*
nsIDocument::XPathEvaluator()
{
  if (!mXPathEvaluator) {
    mXPathEvaluator = new dom::XPathEvaluator(this);
  }
  return mXPathEvaluator;
}

already_AddRefed<nsIDocumentEncoder>
nsIDocument::GetCachedEncoder()
{
  return mCachedEncoder.forget();
}

void
nsIDocument::SetCachedEncoder(already_AddRefed<nsIDocumentEncoder> aEncoder)
{
  mCachedEncoder = aEncoder;
}

void
nsIDocument::SetContentTypeInternal(const nsACString& aType)
{
  mCachedEncoder = nullptr;
  mContentType = aType;
}

nsILoadContext*
nsIDocument::GetLoadContext() const
{
  return mDocumentContainer;
}

nsIDocShell*
nsIDocument::GetDocShell() const
{
  return mDocumentContainer;
}

void
nsIDocument::SetStateObject(nsIStructuredCloneContainer *scContainer)
{
  mStateObjectContainer = scContainer;
  mStateObjectCached = nullptr;
}

already_AddRefed<Element>
nsIDocument::CreateHTMLElement(nsIAtom* aTag)
{
  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(aTag, nullptr, kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  MOZ_ASSERT(nodeInfo, "GetNodeInfo should never fail");

  nsCOMPtr<Element> element;
  DebugOnly<nsresult> rv = NS_NewHTMLElement(getter_AddRefs(element),
                                             nodeInfo.forget(),
                                             mozilla::dom::NOT_FROM_PARSER);

  MOZ_ASSERT(NS_SUCCEEDED(rv), "NS_NewHTMLElement should never fail");
  return element.forget();
}

nsresult
nsIDocument::GetId(nsAString& aId)
{
  if (mId.IsEmpty()) {
    nsresult rv;
    nsCOMPtr<nsIUUIDGenerator> uuidgen = do_GetService("@mozilla.org/uuid-generator;1", &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsID id;
    rv = uuidgen->GenerateUUIDInPlace(&id);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    
    char buffer[NSID_LENGTH];
    id.ToProvidedString(buffer);
    NS_ConvertASCIItoUTF16 uuid(buffer);

    
    mId.Assign(Substring(uuid, 1, NSID_LENGTH - 3));
  }

  aId = mId;
  return NS_OK;
}

bool
MarkDocumentTreeToBeInSyncOperation(nsIDocument* aDoc, void* aData)
{
  nsCOMArray<nsIDocument>* documents =
    static_cast<nsCOMArray<nsIDocument>*>(aData);
  if (aDoc) {
    aDoc->SetIsInSyncOperation(true);
    documents->AppendObject(aDoc);
    aDoc->EnumerateSubDocuments(MarkDocumentTreeToBeInSyncOperation, aData);
  }
  return true;
}

nsAutoSyncOperation::nsAutoSyncOperation(nsIDocument* aDoc)
{
  mMicroTaskLevel = nsContentUtils::MicroTaskLevel();
  nsContentUtils::SetMicroTaskLevel(0);
  if (aDoc) {
    nsPIDOMWindow* win = aDoc->GetWindow();
    if (win) {
      nsCOMPtr<nsIDOMWindow> topWindow;
      win->GetTop(getter_AddRefs(topWindow));
      nsCOMPtr<nsPIDOMWindow> top = do_QueryInterface(topWindow);
      if (top) {
        nsCOMPtr<nsIDocument> doc = top->GetExtantDoc();
        MarkDocumentTreeToBeInSyncOperation(doc, &mDocuments);
      }
    }
  }
}

nsAutoSyncOperation::~nsAutoSyncOperation()
{
  for (int32_t i = 0; i < mDocuments.Count(); ++i) {
    mDocuments[i]->SetIsInSyncOperation(false);
  }
  nsContentUtils::SetMicroTaskLevel(mMicroTaskLevel);
}

FontFaceSet*
nsIDocument::GetFonts(ErrorResult& aRv)
{
  nsIPresShell* shell = GetShell();
  if (!shell) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsPresContext* presContext = shell->GetPresContext();
  if (!presContext) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return presContext->Fonts();
}
