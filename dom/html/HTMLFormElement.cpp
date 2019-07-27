




#include "mozilla/dom/HTMLFormElement.h"

#include "jsapi.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/dom/AutocompleteErrorEvent.h"
#include "mozilla/dom/HTMLFormControlsCollection.h"
#include "mozilla/dom/HTMLFormElementBinding.h"
#include "mozilla/Move.h"
#include "nsIHTMLDocument.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIDocument.h"
#include "nsIFormControlFrame.h"
#include "nsError.h"
#include "nsContentUtils.h"
#include "nsInterfaceHashtable.h"
#include "nsContentList.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIMutableArray.h"
#include "nsIFormAutofillContentService.h"
#include "mozilla/BinarySearch.h"
#include "nsQueryObject.h"


#include "mozilla/Telemetry.h"
#include "nsIFormSubmitObserver.h"
#include "nsIObserverService.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"
#include "nsISimpleEnumerator.h"
#include "nsRange.h"
#include "nsIScriptSecurityManager.h"
#include "nsNetUtil.h"
#include "nsIWebProgress.h"
#include "nsIDocShell.h"
#include "nsFormData.h"
#include "nsFormSubmissionConstants.h"
#include "nsIPromptService.h"
#include "nsISecurityUITelemetry.h"
#include "nsIStringBundle.h"


#include "mozilla/dom/HTMLInputElement.h"
#include "nsIRadioVisitor.h"
#include "RadioNodeList.h"

#include "nsLayoutUtils.h"

#include "mozAutoDocUpdate.h"
#include "nsIHTMLCollection.h"

#include "nsIConstraintValidation.h"

#include "nsIDOMHTMLButtonElement.h"
#include "nsSandboxFlags.h"

#include "nsIContentSecurityPolicy.h"


#include "mozilla/dom/HTMLImageElement.h"


NS_IMPL_NS_NEW_HTML_ELEMENT(Form)

namespace mozilla {
namespace dom {

static const uint8_t NS_FORM_AUTOCOMPLETE_ON  = 1;
static const uint8_t NS_FORM_AUTOCOMPLETE_OFF = 0;

static const nsAttrValue::EnumTable kFormAutocompleteTable[] = {
  { "on",  NS_FORM_AUTOCOMPLETE_ON },
  { "off", NS_FORM_AUTOCOMPLETE_OFF },
  { 0 }
};

static const nsAttrValue::EnumTable* kFormDefaultAutocomplete = &kFormAutocompleteTable[0];

bool HTMLFormElement::gFirstFormSubmitted = false;
bool HTMLFormElement::gPasswordManagerInitialized = false;

HTMLFormElement::HTMLFormElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mControls(new HTMLFormControlsCollection(this)),
    mSelectedRadioButtons(2),
    mRequiredRadioButtonCounts(2),
    mValueMissingRadioGroups(2),
    mGeneratingSubmit(false),
    mGeneratingReset(false),
    mIsSubmitting(false),
    mDeferSubmission(false),
    mNotifiedObservers(false),
    mNotifiedObserversResult(false),
    mSubmitPopupState(openAbused),
    mSubmitInitiatedFromUserInput(false),
    mPendingSubmission(nullptr),
    mSubmittingRequest(nullptr),
    mDefaultSubmitElement(nullptr),
    mFirstSubmitInElements(nullptr),
    mFirstSubmitNotInElements(nullptr),
    mImageNameLookupTable(FORM_CONTROL_LIST_HASHTABLE_LENGTH),
    mPastNameLookupTable(FORM_CONTROL_LIST_HASHTABLE_LENGTH),
    mInvalidElementsCount(0),
    mEverTriedInvalidSubmit(false)
{
}

HTMLFormElement::~HTMLFormElement()
{
  if (mControls) {
    mControls->DropFormReference();
  }

  Clear();
}



static PLDHashOperator
ElementTraverser(const nsAString& key, HTMLInputElement* element,
                 void* userArg)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(userArg);

  cb->NoteXPCOMChild(ToSupports(element));
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLFormElement)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLFormElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mControls)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mImageNameLookupTable)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPastNameLookupTable)
  tmp->mSelectedRadioButtons.EnumerateRead(ElementTraverser, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLFormElement,
                                                nsGenericHTMLElement)
  tmp->Clear();
  tmp->mExpandoAndGeneration.Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(HTMLFormElement,
                                               nsGenericHTMLElement)
  if (tmp->PreservingWrapper()) {
    NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mExpandoAndGeneration.expando);
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_ADDREF_INHERITED(HTMLFormElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLFormElement, Element)



NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLFormElement)
  NS_INTERFACE_TABLE_INHERITED(HTMLFormElement,
                               nsIDOMHTMLFormElement,
                               nsIForm,
                               nsIWebProgressListener,
                               nsIRadioGroupContainer)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsGenericHTMLElement)




NS_IMPL_ELEMENT_CLONE(HTMLFormElement)

nsIHTMLCollection*
HTMLFormElement::Elements()
{
  return mControls;
}

NS_IMETHODIMP
HTMLFormElement::GetElements(nsIDOMHTMLCollection** aElements)
{
  *aElements = Elements();
  NS_ADDREF(*aElements);
  return NS_OK;
}

nsresult
HTMLFormElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                         nsIAtom* aPrefix, const nsAString& aValue,
                         bool aNotify)
{
  if ((aName == nsGkAtoms::action || aName == nsGkAtoms::target) &&
      aNameSpaceID == kNameSpaceID_None) {
    if (mPendingSubmission) {
      
      
      
      
      FlushPendingSubmission();
    }
    
    
    bool notifiedObservers = mNotifiedObservers;
    ForgetCurrentSubmission();
    mNotifiedObservers = notifiedObservers;
  }
  return nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                       aNotify);
}

nsresult
HTMLFormElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                              const nsAttrValue* aValue, bool aNotify)
{
  if (aName == nsGkAtoms::novalidate && aNameSpaceID == kNameSpaceID_None) {
    
    
    for (uint32_t i = 0, length = mControls->mElements.Length();
         i < length; ++i) {
      mControls->mElements[i]->UpdateState(true);
    }

    for (uint32_t i = 0, length = mControls->mNotInElements.Length();
         i < length; ++i) {
      mControls->mNotInElements[i]->UpdateState(true);
    }
  }

  return nsGenericHTMLElement::AfterSetAttr(aNameSpaceID, aName, aValue, aNotify);
}

NS_IMPL_STRING_ATTR(HTMLFormElement, AcceptCharset, acceptcharset)
NS_IMPL_ACTION_ATTR(HTMLFormElement, Action, action)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(HTMLFormElement, Autocomplete, autocomplete,
                                kFormDefaultAutocomplete->tag)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(HTMLFormElement, Enctype, enctype,
                                kFormDefaultEnctype->tag)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(HTMLFormElement, Method, method,
                                kFormDefaultMethod->tag)
NS_IMPL_BOOL_ATTR(HTMLFormElement, NoValidate, novalidate)
NS_IMPL_STRING_ATTR(HTMLFormElement, Name, name)
NS_IMPL_STRING_ATTR(HTMLFormElement, Target, target)

void
HTMLFormElement::Submit(ErrorResult& aRv)
{
  
  if (mPendingSubmission) {
    
    
    
    
    mPendingSubmission = nullptr;
  }

  aRv = DoSubmitOrReset(nullptr, NS_FORM_SUBMIT);
}

NS_IMETHODIMP
HTMLFormElement::Submit()
{
  ErrorResult rv;
  Submit(rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLFormElement::Reset()
{
  InternalFormEvent event(true, NS_FORM_RESET);
  EventDispatcher::Dispatch(static_cast<nsIContent*>(this), nullptr, &event);
  return NS_OK;
}

NS_IMETHODIMP
HTMLFormElement::CheckValidity(bool* retVal)
{
  *retVal = CheckValidity();
  return NS_OK;
}

void
HTMLFormElement::RequestAutocomplete()
{
  bool dummy;
  nsCOMPtr<nsIDOMWindow> window =
    do_QueryInterface(OwnerDoc()->GetScriptHandlingObject(dummy));
  nsCOMPtr<nsIFormAutofillContentService> formAutofillContentService =
    do_GetService("@mozilla.org/formautofill/content-service;1");

  if (!formAutofillContentService || !window) {
    AutocompleteErrorEventInit init;
    init.mBubbles = true;
    init.mCancelable = false;
    init.mReason = AutoCompleteErrorReason::Disabled;

    nsRefPtr<AutocompleteErrorEvent> event =
      AutocompleteErrorEvent::Constructor(this, NS_LITERAL_STRING("autocompleteerror"), init);

    (new AsyncEventDispatcher(this, event))->PostDOMEvent();
    return;
  }

  formAutofillContentService->RequestAutocomplete(this, window);
}

bool
HTMLFormElement::ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::method) {
      return aResult.ParseEnumValue(aValue, kFormMethodTable, false);
    }
    if (aAttribute == nsGkAtoms::enctype) {
      return aResult.ParseEnumValue(aValue, kFormEnctypeTable, false);
    }
    if (aAttribute == nsGkAtoms::autocomplete) {
      return aResult.ParseEnumValue(aValue, kFormAutocompleteTable, false);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsresult
HTMLFormElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                            nsIContent* aBindingParent,
                            bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHTMLDocument> htmlDoc(do_QueryInterface(aDocument));
  if (htmlDoc) {
    htmlDoc->AddedForm();
  }

  return rv;
}

template<typename T>
static void
MarkOrphans(const nsTArray<T*>& aArray)
{
  uint32_t length = aArray.Length();
  for (uint32_t i = 0; i < length; ++i) {
    aArray[i]->SetFlags(MAYBE_ORPHAN_FORM_ELEMENT);
  }
}

static void
CollectOrphans(nsINode* aRemovalRoot,
               const nsTArray<nsGenericHTMLFormElement*>& aArray
#ifdef DEBUG
               , nsIDOMHTMLFormElement* aThisForm
#endif
               )
{
  
  nsAutoScriptBlocker scriptBlocker;

  
  uint32_t length = aArray.Length();
  for (uint32_t i = length; i > 0; --i) {
    nsGenericHTMLFormElement* node = aArray[i-1];

    
    
    
    
#ifdef DEBUG
    bool removed = false;
#endif
    if (node->HasFlag(MAYBE_ORPHAN_FORM_ELEMENT)) {
      node->UnsetFlags(MAYBE_ORPHAN_FORM_ELEMENT);
      if (!nsContentUtils::ContentIsDescendantOf(node, aRemovalRoot)) {
        node->ClearForm(true);

        
        node->UpdateState(true);
#ifdef DEBUG
        removed = true;
#endif
      }
    }

#ifdef DEBUG
    if (!removed) {
      nsCOMPtr<nsIDOMHTMLFormElement> form;
      node->GetForm(getter_AddRefs(form));
      NS_ASSERTION(form == aThisForm, "How did that happen?");
    }
#endif 
  }
}

static void
CollectOrphans(nsINode* aRemovalRoot,
               const nsTArray<HTMLImageElement*>& aArray
#ifdef DEBUG
               , nsIDOMHTMLFormElement* aThisForm
#endif
               )
{
  
  uint32_t length = aArray.Length();
  for (uint32_t i = length; i > 0; --i) {
    HTMLImageElement* node = aArray[i-1];

    
    
    
    
#ifdef DEBUG
    bool removed = false;
#endif
    if (node->HasFlag(MAYBE_ORPHAN_FORM_ELEMENT)) {
      node->UnsetFlags(MAYBE_ORPHAN_FORM_ELEMENT);
      if (!nsContentUtils::ContentIsDescendantOf(node, aRemovalRoot)) {
        node->ClearForm(true);

#ifdef DEBUG
        removed = true;
#endif
      }
    }

#ifdef DEBUG
    if (!removed) {
      nsCOMPtr<nsIDOMHTMLFormElement> form = node->GetForm();
      NS_ASSERTION(form == aThisForm, "How did that happen?");
    }
#endif 
  }
}

void
HTMLFormElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsCOMPtr<nsIHTMLDocument> oldDocument = do_QueryInterface(GetUncomposedDoc());

  
  MarkOrphans(mControls->mElements);
  MarkOrphans(mControls->mNotInElements);
  MarkOrphans(mImageElements);

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);

  nsINode* ancestor = this;
  nsINode* cur;
  do {
    cur = ancestor->GetParentNode();
    if (!cur) {
      break;
    }
    ancestor = cur;
  } while (1);
  
  CollectOrphans(ancestor, mControls->mElements
#ifdef DEBUG
                 , this
#endif
                 );
  CollectOrphans(ancestor, mControls->mNotInElements
#ifdef DEBUG
                 , this
#endif
                 );
  CollectOrphans(ancestor, mImageElements
#ifdef DEBUG
                 , this
#endif
                 );

  if (oldDocument) {
    oldDocument->RemovedForm();
  }
  ForgetCurrentSubmission();
}

nsresult
HTMLFormElement::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  aVisitor.mWantsWillHandleEvent = true;
  if (aVisitor.mEvent->originalTarget == static_cast<nsIContent*>(this)) {
    uint32_t msg = aVisitor.mEvent->message;
    if (msg == NS_FORM_SUBMIT) {
      if (mGeneratingSubmit) {
        aVisitor.mCanHandle = false;
        return NS_OK;
      }
      mGeneratingSubmit = true;

      
      
      
      mDeferSubmission = true;
    }
    else if (msg == NS_FORM_RESET) {
      if (mGeneratingReset) {
        aVisitor.mCanHandle = false;
        return NS_OK;
      }
      mGeneratingReset = true;
    }
  }
  return nsGenericHTMLElement::PreHandleEvent(aVisitor);
}

nsresult
HTMLFormElement::WillHandleEvent(EventChainPostVisitor& aVisitor)
{
  
  
  
  if ((aVisitor.mEvent->message == NS_FORM_SUBMIT ||
       aVisitor.mEvent->message == NS_FORM_RESET) &&
      aVisitor.mEvent->mFlags.mInBubblingPhase &&
      aVisitor.mEvent->originalTarget != static_cast<nsIContent*>(this)) {
    aVisitor.mEvent->mFlags.mPropagationStopped = true;
  }
  return NS_OK;
}

nsresult
HTMLFormElement::PostHandleEvent(EventChainPostVisitor& aVisitor)
{
  if (aVisitor.mEvent->originalTarget == static_cast<nsIContent*>(this)) {
    uint32_t msg = aVisitor.mEvent->message;
    if (msg == NS_FORM_SUBMIT) {
      
      mDeferSubmission = false;
    }

    if (aVisitor.mEventStatus == nsEventStatus_eIgnore) {
      switch (msg) {
        case NS_FORM_RESET:
        case NS_FORM_SUBMIT:
        {
          if (mPendingSubmission && msg == NS_FORM_SUBMIT) {
            
            
            
            
            
            mPendingSubmission = nullptr;
          }
          DoSubmitOrReset(aVisitor.mEvent, msg);
        }
        break;
      }
    } else {
      if (msg == NS_FORM_SUBMIT) {
        
        
        
        
        FlushPendingSubmission();
      }
    }

    if (msg == NS_FORM_SUBMIT) {
      mGeneratingSubmit = false;
    }
    else if (msg == NS_FORM_RESET) {
      mGeneratingReset = false;
    }
  }
  return NS_OK;
}

nsresult
HTMLFormElement::DoSubmitOrReset(WidgetEvent* aEvent,
                                 int32_t aMessage)
{
  
  nsIDocument* doc = GetComposedDoc();
  if (doc) {
    doc->FlushPendingNotifications(Flush_ContentAndNotify);
  }

  

  
  if (NS_FORM_RESET == aMessage) {
    return DoReset();
  }

  if (NS_FORM_SUBMIT == aMessage) {
    
    
    if (!doc || (doc->GetSandboxFlags() & SANDBOXED_FORMS)) {
      return NS_OK;
    }
    return DoSubmit(aEvent);
  }

  MOZ_ASSERT(false);
  return NS_OK;
}

nsresult
HTMLFormElement::DoReset()
{
  
  uint32_t numElements = GetElementCount();
  for (uint32_t elementX = 0; elementX < numElements; ++elementX) {
    
    nsCOMPtr<nsIFormControl> controlNode = GetElementAt(elementX);
    if (controlNode) {
      controlNode->Reset();
    }
  }

  return NS_OK;
}

#define NS_ENSURE_SUBMIT_SUCCESS(rv)                                          \
  if (NS_FAILED(rv)) {                                                        \
    ForgetCurrentSubmission();                                                \
    return rv;                                                                \
  }

nsresult
HTMLFormElement::DoSubmit(WidgetEvent* aEvent)
{
  NS_ASSERTION(GetComposedDoc(), "Should never get here without a current doc");

  if (mIsSubmitting) {
    NS_WARNING("Preventing double form submission");
    
    return NS_OK;
  }

  
  mIsSubmitting = true;
  NS_ASSERTION(!mWebProgress && !mSubmittingRequest, "Web progress / submitting request should not exist here!");

  nsAutoPtr<nsFormSubmission> submission;

  
  
  
  nsresult rv = BuildSubmission(getter_Transfers(submission), aEvent);
  if (NS_FAILED(rv)) {
    mIsSubmitting = false;
    return rv;
  }

  
  
  nsPIDOMWindow *window = OwnerDoc()->GetWindow();

  if (window) {
    mSubmitPopupState = window->GetPopupControlState();
  } else {
    mSubmitPopupState = openAbused;
  }

  mSubmitInitiatedFromUserInput = EventStateManager::IsHandlingUserInput();

  if(mDeferSubmission) { 
    
    
    
    mPendingSubmission = submission;
    
    mIsSubmitting = false;
    return NS_OK; 
  } 
  
  
  
  
  return SubmitSubmission(submission); 
}

nsresult
HTMLFormElement::BuildSubmission(nsFormSubmission** aFormSubmission, 
                                 WidgetEvent* aEvent)
{
  NS_ASSERTION(!mPendingSubmission, "tried to build two submissions!");

  
  nsGenericHTMLElement* originatingElement = nullptr;
  if (aEvent) {
    InternalFormEvent* formEvent = aEvent->AsFormEvent();
    if (formEvent) {
      nsIContent* originator = formEvent->originator;
      if (originator) {
        if (!originator->IsHTMLElement()) {
          return NS_ERROR_UNEXPECTED;
        }
        originatingElement = static_cast<nsGenericHTMLElement*>(originator);
      }
    }
  }

  nsresult rv;

  
  
  
  rv = GetSubmissionFromForm(this, originatingElement, aFormSubmission);
  NS_ENSURE_SUBMIT_SUCCESS(rv);

  
  
  
  rv = WalkFormElements(*aFormSubmission);
  NS_ENSURE_SUBMIT_SUCCESS(rv);

  return NS_OK;
}

nsresult
HTMLFormElement::SubmitSubmission(nsFormSubmission* aFormSubmission)
{
  nsresult rv;
  nsIContent* originatingElement = aFormSubmission->GetOriginatingElement();

  
  
  
  nsCOMPtr<nsIURI> actionURI;
  rv = GetActionURL(getter_AddRefs(actionURI), originatingElement);
  NS_ENSURE_SUBMIT_SUCCESS(rv);

  if (!actionURI) {
    mIsSubmitting = false;
    return NS_OK;
  }

  
  nsIDocument* doc = GetComposedDoc();
  nsCOMPtr<nsISupports> container = doc ? doc->GetContainer() : nullptr;
  nsCOMPtr<nsILinkHandler> linkHandler(do_QueryInterface(container));
  if (!linkHandler || IsEditable()) {
    mIsSubmitting = false;
    return NS_OK;
  }

  
  
  
  
  
  
  
  
  
  
  
  bool schemeIsJavaScript = false;
  if (NS_SUCCEEDED(actionURI->SchemeIs("javascript", &schemeIsJavaScript)) &&
      schemeIsJavaScript) {
    mIsSubmitting = false;
  }

  
  
  
  
  
  
  
  nsAutoString target;
  if (!(originatingElement && originatingElement->GetAttr(kNameSpaceID_None,
                                                          nsGkAtoms::formtarget,
                                                          target)) &&
      !GetAttr(kNameSpaceID_None, nsGkAtoms::target, target)) {
    GetBaseTarget(target);
  }

  
  
  
  bool cancelSubmit = false;
  if (mNotifiedObservers) {
    cancelSubmit = mNotifiedObserversResult;
  } else {
    rv = NotifySubmitObservers(actionURI, &cancelSubmit, true);
    NS_ENSURE_SUBMIT_SUCCESS(rv);
  }

  if (cancelSubmit) {
    mIsSubmitting = false;
    return NS_OK;
  }

  cancelSubmit = false;
  rv = NotifySubmitObservers(actionURI, &cancelSubmit, false);
  NS_ENSURE_SUBMIT_SUCCESS(rv);

  if (cancelSubmit) {
    mIsSubmitting = false;
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIDocShell> docShell;

  {
    nsAutoPopupStatePusher popupStatePusher(mSubmitPopupState);

    AutoHandlingUserInputStatePusher userInpStatePusher(
                                       mSubmitInitiatedFromUserInput,
                                       nullptr, doc);

    nsCOMPtr<nsIInputStream> postDataStream;
    rv = aFormSubmission->GetEncodedSubmission(actionURI,
                                               getter_AddRefs(postDataStream));
    NS_ENSURE_SUBMIT_SUCCESS(rv);

    rv = linkHandler->OnLinkClickSync(this, actionURI,
                                      target.get(),
                                      NullString(),
                                      postDataStream, nullptr,
                                      getter_AddRefs(docShell),
                                      getter_AddRefs(mSubmittingRequest));
    NS_ENSURE_SUBMIT_SUCCESS(rv);
  }

  
  
  
  if (docShell) {
    
    bool pending = false;
    mSubmittingRequest->IsPending(&pending);
    if (pending && !schemeIsJavaScript) {
      nsCOMPtr<nsIWebProgress> webProgress = do_GetInterface(docShell);
      NS_ASSERTION(webProgress, "nsIDocShell not converted to nsIWebProgress!");
      rv = webProgress->AddProgressListener(this, nsIWebProgress::NOTIFY_STATE_ALL);
      NS_ENSURE_SUBMIT_SUCCESS(rv);
      mWebProgress = do_GetWeakReference(webProgress);
      NS_ASSERTION(mWebProgress, "can't hold weak ref to webprogress!");
    } else {
      ForgetCurrentSubmission();
    }
  } else {
    ForgetCurrentSubmission();
  }

  return rv;
}

nsresult
HTMLFormElement::DoSecureToInsecureSubmitCheck(nsIURI* aActionURL,
                                               bool* aCancelSubmit)
{
  *aCancelSubmit = false;

  
  
  
  nsIDocument* parent = OwnerDoc()->GetParentDocument();
  bool isRootDocument = (!parent || nsContentUtils::IsChromeDoc(parent));
  if (!isRootDocument) {
    return NS_OK;
  }

  nsIPrincipal* principal = NodePrincipal();
  if (!principal) {
    *aCancelSubmit = true;
    return NS_OK;
  }
  nsCOMPtr<nsIURI> principalURI;
  nsresult rv = principal->GetURI(getter_AddRefs(principalURI));
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!principalURI) {
    principalURI = OwnerDoc()->GetDocumentURI();
  }
  bool formIsHTTPS;
  rv = principalURI->SchemeIs("https", &formIsHTTPS);
  if (NS_FAILED(rv)) {
    return rv;
  }
  bool actionIsHTTPS;
  rv = aActionURL->SchemeIs("https", &actionIsHTTPS);
  if (NS_FAILED(rv)) {
    return rv;
  }
  bool actionIsJS;
  rv = aActionURL->SchemeIs("javascript", &actionIsJS);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!formIsHTTPS || actionIsHTTPS || actionIsJS) {
    return NS_OK;
  }

  nsCOMPtr<nsIPromptService> promptSvc =
    do_GetService("@mozilla.org/embedcomp/prompt-service;1");
  if (!promptSvc) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIStringBundle> stringBundle;
  nsCOMPtr<nsIStringBundleService> stringBundleService =
    mozilla::services::GetStringBundleService();
  if (!stringBundleService) {
    return NS_ERROR_FAILURE;
  }
  rv = stringBundleService->CreateBundle(
    "chrome://global/locale/browser.properties",
    getter_AddRefs(stringBundle));
  if (NS_FAILED(rv)) {
    return rv;
  }
  nsAutoString title;
  nsAutoString message;
  nsAutoString cont;
  stringBundle->GetStringFromName(
    MOZ_UTF16("formPostSecureToInsecureWarning.title"), getter_Copies(title));
  stringBundle->GetStringFromName(
    MOZ_UTF16("formPostSecureToInsecureWarning.message"),
    getter_Copies(message));
  stringBundle->GetStringFromName(
    MOZ_UTF16("formPostSecureToInsecureWarning.continue"),
    getter_Copies(cont));
  int32_t buttonPressed;
  bool checkState = false; 
  nsCOMPtr<nsPIDOMWindow> window = OwnerDoc()->GetWindow();
  rv = promptSvc->ConfirmEx(window, title.get(), message.get(),
                            (nsIPromptService::BUTTON_TITLE_IS_STRING *
                             nsIPromptService::BUTTON_POS_0) +
                            (nsIPromptService::BUTTON_TITLE_CANCEL *
                             nsIPromptService::BUTTON_POS_1),
                            cont.get(), nullptr, nullptr, nullptr,
                            &checkState, &buttonPressed);
  if (NS_FAILED(rv)) {
    return rv;
  }
  *aCancelSubmit = (buttonPressed == 1);
  uint32_t telemetryBucket =
    nsISecurityUITelemetry::WARNING_CONFIRM_POST_TO_INSECURE_FROM_SECURE;
  mozilla::Telemetry::Accumulate(mozilla::Telemetry::SECURITY_UI,
                                 telemetryBucket);
  if (!*aCancelSubmit) {
    
    mozilla::Telemetry::Accumulate(mozilla::Telemetry::SECURITY_UI,
                                   telemetryBucket + 1);
  }
  return NS_OK;
}

nsresult
HTMLFormElement::NotifySubmitObservers(nsIURI* aActionURL,
                                       bool* aCancelSubmit,
                                       bool    aEarlyNotify)
{
  
  
  if (!gFirstFormSubmitted) {
    gFirstFormSubmitted = true;
    NS_CreateServicesFromCategory(NS_FIRST_FORMSUBMIT_CATEGORY,
                                  nullptr,
                                  NS_FIRST_FORMSUBMIT_CATEGORY);
  }

  if (!aEarlyNotify) {
    nsresult rv = DoSecureToInsecureSubmitCheck(aActionURL, aCancelSubmit);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (*aCancelSubmit) {
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIObserverService> service =
    mozilla::services::GetObserverService();
  if (!service)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISimpleEnumerator> theEnum;
  nsresult rv = service->EnumerateObservers(aEarlyNotify ?
                                            NS_EARLYFORMSUBMIT_SUBJECT :
                                            NS_FORMSUBMIT_SUBJECT,
                                            getter_AddRefs(theEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  if (theEnum) {
    nsCOMPtr<nsISupports> inst;
    *aCancelSubmit = false;

    
    
    
    nsCOMPtr<nsPIDOMWindow> window = OwnerDoc()->GetWindow();

    bool loop = true;
    while (NS_SUCCEEDED(theEnum->HasMoreElements(&loop)) && loop) {
      theEnum->GetNext(getter_AddRefs(inst));

      nsCOMPtr<nsIFormSubmitObserver> formSubmitObserver(
                      do_QueryInterface(inst));
      if (formSubmitObserver) {
        rv = formSubmitObserver->Notify(this,
                                        window,
                                        aActionURL,
                                        aCancelSubmit);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      if (*aCancelSubmit) {
        return NS_OK;
      }
    }
  }

  return rv;
}


nsresult
HTMLFormElement::WalkFormElements(nsFormSubmission* aFormSubmission)
{
  nsTArray<nsGenericHTMLFormElement*> sortedControls;
  nsresult rv = mControls->GetSortedControls(sortedControls);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t len = sortedControls.Length();

  
  
  for (uint32_t i = 0; i < len; ++i) {
    static_cast<nsGenericHTMLElement*>(sortedControls[i])->AddRef();
  }

  
  
  
  for (uint32_t i = 0; i < len; ++i) {
    
    sortedControls[i]->SubmitNamesValues(aFormSubmission);
  }

  
  for (uint32_t i = 0; i < len; ++i) {
    static_cast<nsGenericHTMLElement*>(sortedControls[i])->Release();
  }

  return NS_OK;
}



NS_IMETHODIMP_(uint32_t)
HTMLFormElement::GetElementCount() const 
{
  uint32_t count = 0;
  mControls->GetLength(&count); 
  return count;
}

Element*
HTMLFormElement::IndexedGetter(uint32_t aIndex, bool &aFound)
{
  Element* element = mControls->mElements.SafeElementAt(aIndex, nullptr);
  aFound = element != nullptr;
  return element;
}

NS_IMETHODIMP_(nsIFormControl*)
HTMLFormElement::GetElementAt(int32_t aIndex) const
{
  return mControls->mElements.SafeElementAt(aIndex, nullptr);
}










 int32_t
HTMLFormElement::CompareFormControlPosition(Element* aElement1,
                                            Element* aElement2,
                                            const nsIContent* aForm)
{
  NS_ASSERTION(aElement1 != aElement2, "Comparing a form control to itself");

  
  
  NS_ASSERTION((aElement1->HasAttr(kNameSpaceID_None, nsGkAtoms::form) ||
                aElement1->GetParent()) &&
               (aElement2->HasAttr(kNameSpaceID_None, nsGkAtoms::form) ||
                aElement2->GetParent()),
               "Form controls should always have parents");

  
  
  
  
#ifdef DEBUG
  nsLayoutUtils::gPreventAssertInCompareTreePosition = true;
  int32_t rVal = nsLayoutUtils::CompareTreePosition(aElement1, aElement2, aForm);
  nsLayoutUtils::gPreventAssertInCompareTreePosition = false;

  return rVal;
#else 
  return nsLayoutUtils::CompareTreePosition(aElement1, aElement2, aForm);
#endif 
}

#ifdef DEBUG







 void
HTMLFormElement::AssertDocumentOrder(
  const nsTArray<nsGenericHTMLFormElement*>& aControls, nsIContent* aForm)
{
  
  
  return;

  
  
  
  if (!aControls.IsEmpty()) {
    for (uint32_t i = 0; i < aControls.Length() - 1; ++i) {
      NS_ASSERTION(CompareFormControlPosition(aControls[i], aControls[i + 1],
                                              aForm) < 0,
                   "Form controls not ordered correctly");
    }
  }
}
#endif

void
HTMLFormElement::PostPasswordEvent()
{
  
  if (mFormPasswordEventDispatcher.get()) {
    return;
  }

  mFormPasswordEventDispatcher =
    new FormPasswordEventDispatcher(this,
                                    NS_LITERAL_STRING("DOMFormHasPassword"));
  mFormPasswordEventDispatcher->PostDOMEvent();
}

namespace {

struct FormComparator
{
  Element* const mChild;
  HTMLFormElement* const mForm;
  FormComparator(Element* aChild, HTMLFormElement* aForm)
    : mChild(aChild), mForm(aForm) {}
  int operator()(Element* aElement) const {
    return HTMLFormElement::CompareFormControlPosition(mChild, aElement, mForm);
  }
};

} 



template<typename ElementType>
static bool
AddElementToList(nsTArray<ElementType*>& aList, ElementType* aChild,
                 HTMLFormElement* aForm)
{
  NS_ASSERTION(aList.IndexOf(aChild) == aList.NoIndex,
               "aChild already in aList");

  const uint32_t count = aList.Length();
  ElementType* element;
  bool lastElement = false;

  
  int32_t position = -1;
  if (count > 0) {
    element = aList[count - 1];
    position =
      HTMLFormElement::CompareFormControlPosition(aChild, element, aForm);
  }

  
  
  
  if (position >= 0 || count == 0) {
    
    aList.AppendElement(aChild);
    lastElement = true;
  }
  else {
    size_t idx;
    BinarySearchIf(aList, 0, count, FormComparator(aChild, aForm), &idx);

    
    aList.InsertElementAt(idx, aChild);
  }

  return lastElement;
}

nsresult
HTMLFormElement::AddElement(nsGenericHTMLFormElement* aChild,
                            bool aUpdateValidity, bool aNotify)
{
  
  
  NS_ASSERTION(aChild->HasAttr(kNameSpaceID_None, nsGkAtoms::form) ||
               aChild->GetParent(),
               "Form control should have a parent");

  
  
  bool childInElements = HTMLFormControlsCollection::ShouldBeInElements(aChild);
  nsTArray<nsGenericHTMLFormElement*>& controlList = childInElements ?
      mControls->mElements : mControls->mNotInElements;

  bool lastElement = AddElementToList(controlList, aChild, this);

#ifdef DEBUG
  AssertDocumentOrder(controlList, this);
#endif

  int32_t type = aChild->GetType();

  
  
  
  
  if (type == NS_FORM_INPUT_PASSWORD) {
    if (!gPasswordManagerInitialized) {
      gPasswordManagerInitialized = true;
      NS_CreateServicesFromCategory(NS_PASSWORDMANAGER_CATEGORY,
                                    nullptr,
                                    NS_PASSWORDMANAGER_CATEGORY);
    }
    PostPasswordEvent();
  }
 
  
  if (aChild->IsSubmitControl()) {
    
    

    nsGenericHTMLFormElement** firstSubmitSlot =
      childInElements ? &mFirstSubmitInElements : &mFirstSubmitNotInElements;
    
    
    
    
    
    
    
    
    
    nsGenericHTMLFormElement* oldDefaultSubmit = mDefaultSubmitElement;
    if (!*firstSubmitSlot ||
        (!lastElement &&
         CompareFormControlPosition(aChild, *firstSubmitSlot, this) < 0)) {
      
      
      
      if ((mDefaultSubmitElement ||
           (!mFirstSubmitInElements && !mFirstSubmitNotInElements)) &&
          (*firstSubmitSlot == mDefaultSubmitElement ||
           CompareFormControlPosition(aChild,
                                      mDefaultSubmitElement, this) < 0)) {
        mDefaultSubmitElement = aChild;
      }
      *firstSubmitSlot = aChild;
    }
    NS_POSTCONDITION(mDefaultSubmitElement == mFirstSubmitInElements ||
                     mDefaultSubmitElement == mFirstSubmitNotInElements ||
                     !mDefaultSubmitElement,
                     "What happened here?");

    
    
    
    if (oldDefaultSubmit && oldDefaultSubmit != mDefaultSubmitElement) {
      oldDefaultSubmit->UpdateState(aNotify);
    }
  }

  
  
  if (aUpdateValidity) {
    nsCOMPtr<nsIConstraintValidation> cvElmt = do_QueryObject(aChild);
    if (cvElmt &&
        cvElmt->IsCandidateForConstraintValidation() && !cvElmt->IsValid()) {
      UpdateValidity(false);
    }
  }

  
  
  
  if (type == NS_FORM_INPUT_RADIO) {
    nsRefPtr<HTMLInputElement> radio =
      static_cast<HTMLInputElement*>(aChild);
    radio->AddedToRadioGroup();
  }

  return NS_OK;
}

nsresult
HTMLFormElement::AddElementToTable(nsGenericHTMLFormElement* aChild,
                                   const nsAString& aName)
{
  return mControls->AddElementToTable(aChild, aName);  
}


nsresult
HTMLFormElement::RemoveElement(nsGenericHTMLFormElement* aChild,
                               bool aUpdateValidity)
{
  
  
  
  nsresult rv = NS_OK;
  if (aChild->GetType() == NS_FORM_INPUT_RADIO) {
    nsRefPtr<HTMLInputElement> radio =
      static_cast<HTMLInputElement*>(aChild);
    radio->WillRemoveFromRadioGroup();
  }

  
  
  bool childInElements = HTMLFormControlsCollection::ShouldBeInElements(aChild);
  nsTArray<nsGenericHTMLFormElement*>& controls = childInElements ?
      mControls->mElements :  mControls->mNotInElements;
  
  
  
  size_t index = controls.IndexOf(aChild);
  NS_ENSURE_STATE(index != controls.NoIndex);

  controls.RemoveElementAt(index);

  
  nsGenericHTMLFormElement** firstSubmitSlot =
    childInElements ? &mFirstSubmitInElements : &mFirstSubmitNotInElements;
  if (aChild == *firstSubmitSlot) {
    *firstSubmitSlot = nullptr;

    
    uint32_t length = controls.Length();
    for (uint32_t i = index; i < length; ++i) {
      nsGenericHTMLFormElement* currentControl = controls[i];
      if (currentControl->IsSubmitControl()) {
        *firstSubmitSlot = currentControl;
        break;
      }
    }
  }

  if (aChild == mDefaultSubmitElement) {
    
    
    mDefaultSubmitElement = nullptr;
    nsContentUtils::AddScriptRunner(new RemoveElementRunnable(this));

    
    
    
    
  }

  
  
  if (aUpdateValidity) {
    nsCOMPtr<nsIConstraintValidation> cvElmt = do_QueryObject(aChild);
    if (cvElmt &&
        cvElmt->IsCandidateForConstraintValidation() && !cvElmt->IsValid()) {
      UpdateValidity(true);
    }
  }

  return rv;
}

void
HTMLFormElement::HandleDefaultSubmitRemoval()
{
  if (mDefaultSubmitElement) {
    
    return;
  }

  if (!mFirstSubmitNotInElements) {
    mDefaultSubmitElement = mFirstSubmitInElements;
  } else if (!mFirstSubmitInElements) {
    mDefaultSubmitElement = mFirstSubmitNotInElements;
  } else {
    NS_ASSERTION(mFirstSubmitInElements != mFirstSubmitNotInElements,
                 "How did that happen?");
    
    mDefaultSubmitElement =
      CompareFormControlPosition(mFirstSubmitInElements,
                                 mFirstSubmitNotInElements, this) < 0 ?
      mFirstSubmitInElements : mFirstSubmitNotInElements;
  }

  NS_POSTCONDITION(mDefaultSubmitElement == mFirstSubmitInElements ||
                   mDefaultSubmitElement == mFirstSubmitNotInElements,
                   "What happened here?");

  
  if (mDefaultSubmitElement) {
    mDefaultSubmitElement->UpdateState(true);
  }
}

nsresult
HTMLFormElement::RemoveElementFromTableInternal(
  nsInterfaceHashtable<nsStringHashKey,nsISupports>& aTable,
  nsIContent* aChild, const nsAString& aName)
{
  nsCOMPtr<nsISupports> supports;

  if (!aTable.Get(aName, getter_AddRefs(supports)))
    return NS_OK;

  
  
  if (supports == aChild) {
    aTable.Remove(aName);
    ++mExpandoAndGeneration.generation;
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(supports));
  if (content) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNodeList> nodeList(do_QueryInterface(supports));
  NS_ENSURE_TRUE(nodeList, NS_ERROR_FAILURE);

  
  nsBaseContentList *list = static_cast<nsBaseContentList*>(nodeList.get());

  list->RemoveElement(aChild);

  uint32_t length = 0;
  list->GetLength(&length);

  if (!length) {
    
    
    aTable.Remove(aName);
    ++mExpandoAndGeneration.generation;
  } else if (length == 1) {
    
    
    nsIContent* node = list->Item(0);
    if (node) {
      aTable.Put(aName, node);
    }
  }

  return NS_OK;
}

static PLDHashOperator
RemovePastNames(const nsAString& aName,
                nsCOMPtr<nsISupports>& aData,
                void* aClosure)
{
  return aClosure == aData ? PL_DHASH_REMOVE : PL_DHASH_NEXT;
}

nsresult
HTMLFormElement::RemoveElementFromTable(nsGenericHTMLFormElement* aElement,
                                        const nsAString& aName,
                                        RemoveElementReason aRemoveReason)
{
  
  
  if (aRemoveReason == ElementRemoved) {
    uint32_t oldCount = mPastNameLookupTable.Count();
    mPastNameLookupTable.Enumerate(RemovePastNames, aElement);
    if (oldCount != mPastNameLookupTable.Count()) {
      ++mExpandoAndGeneration.generation;
    }
  }

  return mControls->RemoveElementFromTable(aElement, aName);
}

already_AddRefed<nsISupports>
HTMLFormElement::NamedGetter(const nsAString& aName, bool &aFound)
{
  aFound = true;

  nsCOMPtr<nsISupports> result = DoResolveName(aName, true);
  if (result) {
    AddToPastNamesMap(aName, result);
    return result.forget();
  }

  result = mImageNameLookupTable.GetWeak(aName);
  if (result) {
    AddToPastNamesMap(aName, result);
    return result.forget();
  }

  result = mPastNameLookupTable.GetWeak(aName);
  if (result) {
    return result.forget();
  }

  aFound = false;
  return nullptr;
}

bool
HTMLFormElement::NameIsEnumerable(const nsAString& aName)
{
  return true;
}

void
HTMLFormElement::GetSupportedNames(unsigned, nsTArray<nsString >& aRetval)
{
  
}

already_AddRefed<nsISupports>
HTMLFormElement::FindNamedItem(const nsAString& aName,
                               nsWrapperCache** aCache)
{
  

  bool found;
  nsCOMPtr<nsISupports> result = NamedGetter(aName, found);
  if (result) {
    *aCache = nullptr;
    return result.forget();
  }

  return nullptr;
}

already_AddRefed<nsISupports>
HTMLFormElement::DoResolveName(const nsAString& aName,
                               bool aFlushContent)
{
  nsCOMPtr<nsISupports> result =
    mControls->NamedItemInternal(aName, aFlushContent);
  return result.forget();
}

void
HTMLFormElement::OnSubmitClickBegin(nsIContent* aOriginatingElement)
{
  mDeferSubmission = true;

  
  
  
  nsCOMPtr<nsIURI> actionURI;
  nsresult rv;

  rv = GetActionURL(getter_AddRefs(actionURI), aOriginatingElement);
  if (NS_FAILED(rv) || !actionURI)
    return;

  
  
  
  if (mInvalidElementsCount == 0) {
    bool cancelSubmit = false;
    rv = NotifySubmitObservers(actionURI, &cancelSubmit, true);
    if (NS_SUCCEEDED(rv)) {
      mNotifiedObservers = true;
      mNotifiedObserversResult = cancelSubmit;
    }
  }
}

void
HTMLFormElement::OnSubmitClickEnd()
{
  mDeferSubmission = false;
}

void
HTMLFormElement::FlushPendingSubmission()
{
  if (mPendingSubmission) {
    
    
    nsAutoPtr<nsFormSubmission> submission = Move(mPendingSubmission);

    SubmitSubmission(submission);
  }
}

nsresult
HTMLFormElement::GetActionURL(nsIURI** aActionURL,
                              nsIContent* aOriginatingElement)
{
  nsresult rv = NS_OK;

  *aActionURL = nullptr;

  
  
  
  
  
  
  
  nsAutoString action;

  if (aOriginatingElement &&
      aOriginatingElement->HasAttr(kNameSpaceID_None, nsGkAtoms::formaction)) {
#ifdef DEBUG
    nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(aOriginatingElement);
    NS_ASSERTION(formControl && formControl->IsSubmitControl(),
                 "The originating element must be a submit form control!");
#endif 

    nsCOMPtr<nsIDOMHTMLInputElement> inputElement = do_QueryInterface(aOriginatingElement);
    if (inputElement) {
      inputElement->GetFormAction(action);
    } else {
      nsCOMPtr<nsIDOMHTMLButtonElement> buttonElement = do_QueryInterface(aOriginatingElement);
      if (buttonElement) {
        buttonElement->GetFormAction(action);
      } else {
        NS_ERROR("Originating element must be an input or button element!");
        return NS_ERROR_UNEXPECTED;
      }
    }
  } else {
    GetAction(action);
  }

  
  
  

  
  
  
  if (!IsInDoc()) {
    return NS_OK; 
  }

  
  nsIDocument *document = OwnerDoc();
  nsIURI *docURI = document->GetDocumentURI();
  NS_ENSURE_TRUE(docURI, NS_ERROR_UNEXPECTED);

  
  
  
  
  
  

  nsCOMPtr<nsIURI> actionURL;
  if (action.IsEmpty()) {
    nsCOMPtr<nsIHTMLDocument> htmlDoc(do_QueryInterface(document));
    if (!htmlDoc) {
      
      
      return NS_OK;
    }

    rv = docURI->Clone(getter_AddRefs(actionURL));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    nsCOMPtr<nsIURI> baseURL = GetBaseURI();
    NS_ASSERTION(baseURL, "No Base URL found in Form Submit!\n");
    if (!baseURL) {
      return NS_OK; 
    }
    rv = NS_NewURI(getter_AddRefs(actionURL), action, nullptr, baseURL);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  nsIScriptSecurityManager *securityManager =
      nsContentUtils::GetSecurityManager();
  rv = securityManager->
    CheckLoadURIWithPrincipal(NodePrincipal(), actionURL,
                              nsIScriptSecurityManager::STANDARD);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);
  if (csp) {
    bool permitsFormAction = true;

    
    
    
    rv = csp->Permits(actionURL, nsIContentSecurityPolicy::FORM_ACTION_DIRECTIVE,
                      true, &permitsFormAction);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!permitsFormAction) {
      rv = NS_ERROR_CSP_FORM_ACTION_VIOLATION;
    }
  }

  
  
  
  actionURL.forget(aActionURL);

  return rv;
}

NS_IMETHODIMP_(nsIFormControl*)
HTMLFormElement::GetDefaultSubmitElement() const
{
  NS_PRECONDITION(mDefaultSubmitElement == mFirstSubmitInElements ||
                  mDefaultSubmitElement == mFirstSubmitNotInElements,
                  "What happened here?");
  
  return mDefaultSubmitElement;
}

bool
HTMLFormElement::IsDefaultSubmitElement(const nsIFormControl* aControl) const
{
  NS_PRECONDITION(aControl, "Unexpected call");

  if (aControl == mDefaultSubmitElement) {
    
    return true;
  }

  if (mDefaultSubmitElement ||
      (aControl != mFirstSubmitInElements &&
       aControl != mFirstSubmitNotInElements)) {
    
    return false;
  }

  
  
  
  
  
  if (!mFirstSubmitInElements || !mFirstSubmitNotInElements) {
    
    return true;
  }

  
  nsIFormControl* defaultSubmit =
    CompareFormControlPosition(mFirstSubmitInElements,
                               mFirstSubmitNotInElements, this) < 0 ?
      mFirstSubmitInElements : mFirstSubmitNotInElements;
  return aControl == defaultSubmit;
}

bool
HTMLFormElement::ImplicitSubmissionIsDisabled() const
{
  
  uint32_t numDisablingControlsFound = 0;
  uint32_t length = mControls->mElements.Length();
  for (uint32_t i = 0; i < length && numDisablingControlsFound < 2; ++i) {
    if (mControls->mElements[i]->IsSingleLineTextControl(false) ||
        mControls->mElements[i]->GetType() == NS_FORM_INPUT_NUMBER) {
      numDisablingControlsFound++;
    }
  }
  return numDisablingControlsFound != 1;
}

NS_IMETHODIMP
HTMLFormElement::GetEncoding(nsAString& aEncoding)
{
  return GetEnctype(aEncoding);
}
 
NS_IMETHODIMP
HTMLFormElement::SetEncoding(const nsAString& aEncoding)
{
  return SetEnctype(aEncoding);
}

int32_t
HTMLFormElement::Length()
{
  return mControls->Length();
}

NS_IMETHODIMP    
HTMLFormElement::GetLength(int32_t* aLength)
{
  *aLength = Length();
  return NS_OK;
}

void
HTMLFormElement::ForgetCurrentSubmission()
{
  mNotifiedObservers = false;
  mIsSubmitting = false;
  mSubmittingRequest = nullptr;
  nsCOMPtr<nsIWebProgress> webProgress = do_QueryReferent(mWebProgress);
  if (webProgress) {
    webProgress->RemoveProgressListener(this);
  }
  mWebProgress = nullptr;
}

bool
HTMLFormElement::CheckFormValidity(nsIMutableArray* aInvalidElements) const
{
  bool ret = true;

  nsTArray<nsGenericHTMLFormElement*> sortedControls;
  if (NS_FAILED(mControls->GetSortedControls(sortedControls))) {
    return false;
  }

  uint32_t len = sortedControls.Length();

  
  
  for (uint32_t i = 0; i < len; ++i) {
    sortedControls[i]->AddRef();
  }

  for (uint32_t i = 0; i < len; ++i) {
    nsCOMPtr<nsIConstraintValidation> cvElmt = do_QueryObject(sortedControls[i]);
    if (cvElmt && cvElmt->IsCandidateForConstraintValidation() &&
        !cvElmt->IsValid()) {
      ret = false;
      bool defaultAction = true;
      nsContentUtils::DispatchTrustedEvent(sortedControls[i]->OwnerDoc(),
                                           static_cast<nsIContent*>(sortedControls[i]),
                                           NS_LITERAL_STRING("invalid"),
                                           false, true, &defaultAction);

      
      
      if (defaultAction && aInvalidElements) {
        aInvalidElements->AppendElement(ToSupports(sortedControls[i]),
                                        false);
      }
    }
  }

  
  for (uint32_t i = 0; i < len; ++i) {
    static_cast<nsGenericHTMLElement*>(sortedControls[i])->Release();
  }

  return ret;
}

bool
HTMLFormElement::CheckValidFormSubmission()
{
  













  NS_ASSERTION(!HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate),
               "We shouldn't be there if novalidate is set!");

  
  
  
  nsIDocument* doc = GetComposedDoc();
  if (doc && (doc->GetSandboxFlags() & SANDBOXED_FORMS)) {
    return true;
  }

  
  
  nsCOMPtr<nsIObserverService> service =
    mozilla::services::GetObserverService();
  if (!service) {
    NS_WARNING("No observer service available!");
    return true;
  }

  nsCOMPtr<nsISimpleEnumerator> theEnum;
  nsresult rv = service->EnumerateObservers(NS_INVALIDFORMSUBMIT_SUBJECT,
                                            getter_AddRefs(theEnum));
  
  NS_ENSURE_SUCCESS(rv, true);

  bool hasObserver = false;
  rv = theEnum->HasMoreElements(&hasObserver);

  
  
  if (NS_SUCCEEDED(rv) && hasObserver) {
    nsCOMPtr<nsIMutableArray> invalidElements =
      do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    
    NS_ENSURE_SUCCESS(rv, true);

    if (!CheckFormValidity(invalidElements.get())) {
      
      
      
      if (!mEverTriedInvalidSubmit) {
        mEverTriedInvalidSubmit = true;

        





        nsAutoScriptBlocker scriptBlocker;

        for (uint32_t i = 0, length = mControls->mElements.Length();
             i < length; ++i) {
          
          
          if (mControls->mElements[i]->IsHTMLElement(nsGkAtoms::input) &&
              nsContentUtils::IsFocusedContent(mControls->mElements[i])) {
            static_cast<HTMLInputElement*>(mControls->mElements[i])
              ->UpdateValidityUIBits(true);
          }

          mControls->mElements[i]->UpdateState(true);
        }

        
        
        
        for (uint32_t i = 0, length = mControls->mNotInElements.Length();
             i < length; ++i) {
          mControls->mNotInElements[i]->UpdateState(true);
        }
      }

      nsCOMPtr<nsISupports> inst;
      nsCOMPtr<nsIFormSubmitObserver> observer;
      bool more = true;
      while (NS_SUCCEEDED(theEnum->HasMoreElements(&more)) && more) {
        theEnum->GetNext(getter_AddRefs(inst));
        observer = do_QueryInterface(inst);

        if (observer) {
          observer->NotifyInvalidSubmit(this,
                                        static_cast<nsIArray*>(invalidElements));
        }
      }

      
      return false;
    }
  } else {
    NS_WARNING("There is no observer for \"invalidformsubmit\". \
One should be implemented!");
  }

  return true;
}

void
HTMLFormElement::UpdateValidity(bool aElementValidity)
{
  if (aElementValidity) {
    --mInvalidElementsCount;
  } else {
    ++mInvalidElementsCount;
  }

  NS_ASSERTION(mInvalidElementsCount >= 0, "Something went seriously wrong!");

  
  
  
  
  if (mInvalidElementsCount &&
      (mInvalidElementsCount != 1 || aElementValidity)) {
    return;
  }

  






  nsAutoScriptBlocker scriptBlocker;

  
  for (uint32_t i = 0, length = mControls->mElements.Length();
       i < length; ++i) {
    if (mControls->mElements[i]->IsSubmitControl()) {
      mControls->mElements[i]->UpdateState(true);
    }
  }

  
  
  uint32_t length = mControls->mNotInElements.Length();
  for (uint32_t i = 0; i < length; ++i) {
    if (mControls->mNotInElements[i]->IsSubmitControl()) {
      mControls->mNotInElements[i]->UpdateState(true);
    }
  }

  UpdateState(true);
}


NS_IMETHODIMP
HTMLFormElement::OnStateChange(nsIWebProgress* aWebProgress,
                               nsIRequest* aRequest,
                               uint32_t aStateFlags,
                               nsresult aStatus)
{
  
  
  
  
  if (aRequest == mSubmittingRequest &&
      aStateFlags & nsIWebProgressListener::STATE_STOP) {
    ForgetCurrentSubmission();
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLFormElement::OnProgressChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  int32_t aCurSelfProgress,
                                  int32_t aMaxSelfProgress,
                                  int32_t aCurTotalProgress,
                                  int32_t aMaxTotalProgress)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
HTMLFormElement::OnLocationChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsIURI* location,
                                  uint32_t aFlags)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
HTMLFormElement::OnStatusChange(nsIWebProgress* aWebProgress,
                                nsIRequest* aRequest,
                                nsresult aStatus,
                                const char16_t* aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
HTMLFormElement::OnSecurityChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  uint32_t state)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP_(int32_t)
HTMLFormElement::IndexOfControl(nsIFormControl* aControl)
{
  int32_t index = 0;
  return mControls->IndexOfControl(aControl, &index) == NS_OK ? index : 0;
}

void
HTMLFormElement::SetCurrentRadioButton(const nsAString& aName,
                                       HTMLInputElement* aRadio)
{
  mSelectedRadioButtons.Put(aName, aRadio);
}

HTMLInputElement*
HTMLFormElement::GetCurrentRadioButton(const nsAString& aName)
{
  return mSelectedRadioButtons.GetWeak(aName);
}

NS_IMETHODIMP
HTMLFormElement::GetNextRadioButton(const nsAString& aName,
                                    const bool aPrevious,
                                    HTMLInputElement* aFocusedRadio,
                                    HTMLInputElement** aRadioOut)
{
  
  
  *aRadioOut = nullptr;

  nsRefPtr<HTMLInputElement> currentRadio;
  if (aFocusedRadio) {
    currentRadio = aFocusedRadio;
  }
  else {
    mSelectedRadioButtons.Get(aName, getter_AddRefs(currentRadio));
  }

  nsCOMPtr<nsISupports> itemWithName = DoResolveName(aName, true);
  nsCOMPtr<nsINodeList> radioGroup(do_QueryInterface(itemWithName));

  if (!radioGroup) {
    return NS_ERROR_FAILURE;
  }

  int32_t index = radioGroup->IndexOf(currentRadio);
  if (index < 0) {
    return NS_ERROR_FAILURE;
  }

  uint32_t numRadios;
  radioGroup->GetLength(&numRadios);
  nsRefPtr<HTMLInputElement> radio;

  bool isRadio = false;
  do {
    if (aPrevious) {
      if (--index < 0) {
        index = numRadios -1;
      }
    }
    else if (++index >= (int32_t)numRadios) {
      index = 0;
    }
    radio = HTMLInputElement::FromContentOrNull(radioGroup->Item(index));
    isRadio = radio && radio->GetType() == NS_FORM_INPUT_RADIO;
    if (!isRadio) {
      continue;
    }

    nsAutoString name;
    radio->GetName(name);
    isRadio = aName.Equals(name);
  } while (!isRadio || (radio->Disabled() && radio != currentRadio));

  NS_IF_ADDREF(*aRadioOut = radio);
  return NS_OK;
}

NS_IMETHODIMP
HTMLFormElement::WalkRadioGroup(const nsAString& aName,
                                nsIRadioVisitor* aVisitor,
                                bool aFlushContent)
{
  if (aName.IsEmpty()) {
    
    
    
    
    nsCOMPtr<nsIFormControl> control;
    uint32_t len = GetElementCount();
    for (uint32_t i = 0; i < len; i++) {
      control = GetElementAt(i);
      if (control->GetType() == NS_FORM_INPUT_RADIO) {
        nsCOMPtr<nsIContent> controlContent = do_QueryInterface(control);
        if (controlContent &&
            controlContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                                        EmptyString(), eCaseMatters) &&
            !aVisitor->Visit(control)) {
          break;
        }
      }
    }
    return NS_OK;
  }

  
  nsCOMPtr<nsISupports> item = DoResolveName(aName, aFlushContent);
  if (!item) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(item);
  if (formControl) {
    if (formControl->GetType() == NS_FORM_INPUT_RADIO) {
      aVisitor->Visit(formControl);
    }
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNodeList> nodeList = do_QueryInterface(item);
  if (!nodeList) {
    return NS_OK;
  }
  uint32_t length = 0;
  nodeList->GetLength(&length);
  for (uint32_t i = 0; i < length; i++) {
    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(i, getter_AddRefs(node));
    nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(node);
    if (formControl && formControl->GetType() == NS_FORM_INPUT_RADIO &&
        !aVisitor->Visit(formControl)) {
      break;
    }
  }
  return NS_OK;
}

void
HTMLFormElement::AddToRadioGroup(const nsAString& aName,
                                 nsIFormControl* aRadio)
{
  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements!");

  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    mRequiredRadioButtonCounts.Put(aName,
                                   mRequiredRadioButtonCounts.Get(aName)+1);
  }
}

void
HTMLFormElement::RemoveFromRadioGroup(const nsAString& aName,
                                      nsIFormControl* aRadio)
{
  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements!");

  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    uint32_t requiredNb = mRequiredRadioButtonCounts.Get(aName);
    NS_ASSERTION(requiredNb >= 1,
                 "At least one radio button has to be required!");

    if (requiredNb == 1) {
      mRequiredRadioButtonCounts.Remove(aName);
    } else {
      mRequiredRadioButtonCounts.Put(aName, requiredNb-1);
    }
  }
}

uint32_t
HTMLFormElement::GetRequiredRadioCount(const nsAString& aName) const
{
  return mRequiredRadioButtonCounts.Get(aName);
}

void
HTMLFormElement::RadioRequiredWillChange(const nsAString& aName,
                                         bool aRequiredAdded)
{
  if (aRequiredAdded) {
    mRequiredRadioButtonCounts.Put(aName,
                                   mRequiredRadioButtonCounts.Get(aName)+1);
  } else {
    uint32_t requiredNb = mRequiredRadioButtonCounts.Get(aName);
    NS_ASSERTION(requiredNb >= 1,
                 "At least one radio button has to be required!");
    if (requiredNb == 1) {
      mRequiredRadioButtonCounts.Remove(aName);
    } else {
      mRequiredRadioButtonCounts.Put(aName, requiredNb-1);
    }
  }
}

bool
HTMLFormElement::GetValueMissingState(const nsAString& aName) const
{
  return mValueMissingRadioGroups.Get(aName);
}

void
HTMLFormElement::SetValueMissingState(const nsAString& aName, bool aValue)
{
  mValueMissingRadioGroups.Put(aName, aValue);
}

EventStates
HTMLFormElement::IntrinsicState() const
{
  EventStates state = nsGenericHTMLElement::IntrinsicState();

  if (mInvalidElementsCount) {
    state |= NS_EVENT_STATE_INVALID;
  } else {
      state |= NS_EVENT_STATE_VALID;
  }

  return state;
}

void
HTMLFormElement::Clear()
{
  for (int32_t i = mImageElements.Length() - 1; i >= 0; i--) {
    mImageElements[i]->ClearForm(false);
  }
  mImageElements.Clear();
  mImageNameLookupTable.Clear();
  mPastNameLookupTable.Clear();
}

namespace {

struct PositionComparator
{
  nsIContent* const mElement;
  explicit PositionComparator(nsIContent* const aElement) : mElement(aElement) {}

  int operator()(nsIContent* aElement) const {
    if (mElement == aElement) {
      return 0;
    }
    if (nsContentUtils::PositionIsBefore(mElement, aElement)) {
      return -1;
    }
    return 1;
  }
};

struct NodeListAdaptor
{
  nsINodeList* const mList;
  explicit NodeListAdaptor(nsINodeList* aList) : mList(aList) {}
  nsIContent* operator[](size_t aIdx) const {
    return mList->Item(aIdx);
  }
};

} 

nsresult
HTMLFormElement::AddElementToTableInternal(
  nsInterfaceHashtable<nsStringHashKey,nsISupports>& aTable,
  nsIContent* aChild, const nsAString& aName)
{
  nsCOMPtr<nsISupports> supports;
  aTable.Get(aName, getter_AddRefs(supports));

  if (!supports) {
    
    aTable.Put(aName, aChild);
    ++mExpandoAndGeneration.generation;
  } else {
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(supports);

    if (content) {
      
      
      
      
      if (content == aChild) {
        return NS_OK;
      }

      
      
      RadioNodeList *list = new RadioNodeList(this);

      
      
      NS_ASSERTION(content->HasAttr(kNameSpaceID_None, nsGkAtoms::form) ||
                   content->GetParent(), "Item in list without parent");

      
      bool newFirst = nsContentUtils::PositionIsBefore(aChild, content);

      list->AppendElement(newFirst ? aChild : content.get());
      list->AppendElement(newFirst ? content.get() : aChild);


      nsCOMPtr<nsISupports> listSupports = do_QueryObject(list);

      
      aTable.Put(aName, listSupports);
    } else {
      
      nsCOMPtr<nsIDOMNodeList> nodeList = do_QueryInterface(supports);
      NS_ENSURE_TRUE(nodeList, NS_ERROR_FAILURE);

      
      RadioNodeList *list =
        static_cast<RadioNodeList*>(nodeList.get());

      NS_ASSERTION(list->Length() > 1,
                   "List should have been converted back to a single element");

      
      
      
      
      if (nsContentUtils::PositionIsBefore(list->Item(list->Length() - 1), aChild)) {
        list->AppendElement(aChild);
        return NS_OK;
      }

      
      
      if (list->IndexOf(aChild) != -1) {
        return NS_OK;
      }

      size_t idx;
      DebugOnly<bool> found = BinarySearchIf(NodeListAdaptor(list), 0, list->Length(),
                                             PositionComparator(aChild), &idx);
      MOZ_ASSERT(!found, "should not have found an element");

      list->InsertElementAt(aChild, idx);
    }
  }

  return NS_OK;
}

nsresult
HTMLFormElement::AddImageElement(HTMLImageElement* aChild)
{
  AddElementToList(mImageElements, aChild, this);
  return NS_OK;
}

nsresult
HTMLFormElement::AddImageElementToTable(HTMLImageElement* aChild,
                                        const nsAString& aName)
{
  return AddElementToTableInternal(mImageNameLookupTable, aChild, aName);
}

nsresult
HTMLFormElement::RemoveImageElement(HTMLImageElement* aChild)
{
  size_t index = mImageElements.IndexOf(aChild);
  NS_ENSURE_STATE(index != mImageElements.NoIndex);

  mImageElements.RemoveElementAt(index);
  return NS_OK;
}

nsresult
HTMLFormElement::RemoveImageElementFromTable(HTMLImageElement* aElement,
                                             const nsAString& aName,
                                             RemoveElementReason aRemoveReason)
{
  
  
  if (aRemoveReason == ElementRemoved) {
    mPastNameLookupTable.Enumerate(RemovePastNames, aElement);
  }

  return RemoveElementFromTableInternal(mImageNameLookupTable, aElement, aName);
}

void
HTMLFormElement::AddToPastNamesMap(const nsAString& aName,
                                   nsISupports* aChild)
{
  
  
  
  nsCOMPtr<nsIContent> node = do_QueryInterface(aChild);
  if (node) {
    mPastNameLookupTable.Put(aName, aChild);
  }
}
 
JSObject*
HTMLFormElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLFormElementBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
