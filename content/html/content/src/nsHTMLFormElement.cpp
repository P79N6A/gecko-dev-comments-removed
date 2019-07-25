



#include "nsHTMLFormElement.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsEventStateManager.h"
#include "nsEventStates.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIFormControlFrame.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"
#include "nsInterfaceHashtable.h"
#include "nsContentList.h"
#include "nsGUIEvent.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIMutableArray.h"


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


#include "nsIDOMHTMLInputElement.h"
#include "nsHTMLInputElement.h"
#include "nsIRadioVisitor.h"

#include "nsLayoutUtils.h"

#include "nsEventDispatcher.h"

#include "mozAutoDocUpdate.h"
#include "nsIHTMLCollection.h"

#include "nsIConstraintValidation.h"

#include "nsIDOMHTMLButtonElement.h"
#include "dombindings.h"

using namespace mozilla::dom;

static const int NS_FORM_CONTROL_LIST_HASHTABLE_SIZE = 16;

static const PRUint8 NS_FORM_AUTOCOMPLETE_ON  = 1;
static const PRUint8 NS_FORM_AUTOCOMPLETE_OFF = 0;

static const nsAttrValue::EnumTable kFormAutocompleteTable[] = {
  { "on",  NS_FORM_AUTOCOMPLETE_ON },
  { "off", NS_FORM_AUTOCOMPLETE_OFF },
  { 0 }
};

static const nsAttrValue::EnumTable* kFormDefaultAutocomplete = &kFormAutocompleteTable[0];



bool nsHTMLFormElement::gFirstFormSubmitted = false;
bool nsHTMLFormElement::gPasswordManagerInitialized = false;



class nsFormControlList : public nsIHTMLCollection,
                          public nsWrapperCache
{
public:
  nsFormControlList(nsHTMLFormElement* aForm);
  virtual ~nsFormControlList();

  nsresult Init();

  void DropFormReference();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMHTMLCOLLECTION

  virtual nsINode* GetParentObject()
  {
    return mForm;
  }

  nsresult AddElementToTable(nsGenericHTMLFormElement* aChild,
                             const nsAString& aName);
  nsresult RemoveElementFromTable(nsGenericHTMLFormElement* aChild,
                                  const nsAString& aName);
  nsresult IndexOfControl(nsIFormControl* aControl,
                          PRInt32* aIndex);

  nsISupports* NamedItemInternal(const nsAString& aName, bool aFlushContent);
  
  








  nsresult GetSortedControls(nsTArray<nsGenericHTMLFormElement*>& aControls) const;

  
  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap)
  {
    return mozilla::dom::binding::HTMLCollection::create(cx, scope, this,
                                                         triedToWrap);
  }

  nsHTMLFormElement* mForm;  

  nsTArray<nsGenericHTMLFormElement*> mElements;  

  
  
  
  

  nsTArray<nsGenericHTMLFormElement*> mNotInElements; 

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsFormControlList)

protected:
  
  void Clear();

  
  void FlushPendingNotifications();
  
  
  
  
  
  

  nsInterfaceHashtable<nsStringHashKey,nsISupports> mNameLookupTable;
};

static bool
ShouldBeInElements(nsIFormControl* aFormControl)
{
  
  
  

  switch (aFormControl->GetType()) {
  case NS_FORM_BUTTON_BUTTON :
  case NS_FORM_BUTTON_RESET :
  case NS_FORM_BUTTON_SUBMIT :
  case NS_FORM_INPUT_BUTTON :
  case NS_FORM_INPUT_CHECKBOX :
  case NS_FORM_INPUT_EMAIL :
  case NS_FORM_INPUT_FILE :
  case NS_FORM_INPUT_HIDDEN :
  case NS_FORM_INPUT_RESET :
  case NS_FORM_INPUT_PASSWORD :
  case NS_FORM_INPUT_RADIO :
  case NS_FORM_INPUT_SEARCH :
  case NS_FORM_INPUT_SUBMIT :
  case NS_FORM_INPUT_TEXT :
  case NS_FORM_INPUT_TEL :
  case NS_FORM_INPUT_URL :
  case NS_FORM_SELECT :
  case NS_FORM_TEXTAREA :
  case NS_FORM_FIELDSET :
  case NS_FORM_OBJECT :
  case NS_FORM_OUTPUT :
    return true;
  }

  
  
  
  
  
  

  return false;
}




nsGenericHTMLElement*
NS_NewHTMLFormElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      FromParser aFromParser)
{
  nsHTMLFormElement* it = new nsHTMLFormElement(aNodeInfo);

  nsresult rv = it->Init();

  if (NS_FAILED(rv)) {
    delete it;
    return nsnull;
  }

  return it;
}

nsHTMLFormElement::nsHTMLFormElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mGeneratingSubmit(false),
    mGeneratingReset(false),
    mIsSubmitting(false),
    mDeferSubmission(false),
    mNotifiedObservers(false),
    mNotifiedObserversResult(false),
    mSubmitPopupState(openAbused),
    mSubmitInitiatedFromUserInput(false),
    mPendingSubmission(nsnull),
    mSubmittingRequest(nsnull),
    mDefaultSubmitElement(nsnull),
    mFirstSubmitInElements(nsnull),
    mFirstSubmitNotInElements(nsnull),
    mInvalidElementsCount(0),
    mEverTriedInvalidSubmit(false)
{
}

nsHTMLFormElement::~nsHTMLFormElement()
{
  if (mControls) {
    mControls->DropFormReference();
  }
}

nsresult
nsHTMLFormElement::Init()
{
  mControls = new nsFormControlList(this);
  if (!mControls) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = mControls->Init();
  
  if (NS_FAILED(rv))
  {
    mControls = nsnull;
    return rv;
  }
  
  mSelectedRadioButtons.Init(4);
  mRequiredRadioButtonCounts.Init(4);
  mValueMissingRadioGroups.Init(4);

  return NS_OK;
}




static PLDHashOperator
ElementTraverser(const nsAString& key, nsIDOMHTMLInputElement* element,
                 void* userArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(userArg);
 
  cb->NoteXPCOMChild(element);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLFormElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLFormElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mControls,
                                                       nsIDOMHTMLCollection)
  tmp->mSelectedRadioButtons.EnumerateRead(ElementTraverser, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLFormElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLFormElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLFormElement, nsHTMLFormElement)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLFormElement)
  NS_HTML_CONTENT_INTERFACE_TABLE4(nsHTMLFormElement,
                                   nsIDOMHTMLFormElement,
                                   nsIForm,
                                   nsIWebProgressListener,
                                   nsIRadioGroupContainer)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLFormElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLFormElement)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsHTMLFormElement)

NS_IMETHODIMP
nsHTMLFormElement::GetElements(nsIDOMHTMLCollection** aElements)
{
  *aElements = mControls;
  NS_ADDREF(*aElements);
  return NS_OK;
}

nsresult
nsHTMLFormElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
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
nsHTMLFormElement::AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify)
{
  if (aName == nsGkAtoms::novalidate && aNameSpaceID == kNameSpaceID_None) {
    
    
    for (PRUint32 i = 0, length = mControls->mElements.Length();
         i < length; ++i) {
      mControls->mElements[i]->UpdateState(true);
    }

    for (PRUint32 i = 0, length = mControls->mNotInElements.Length();
         i < length; ++i) {
      mControls->mNotInElements[i]->UpdateState(true);
    }
  }

  return nsGenericHTMLElement::AfterSetAttr(aNameSpaceID, aName, aValue, aNotify);
}

NS_IMPL_STRING_ATTR(nsHTMLFormElement, AcceptCharset, acceptcharset)
NS_IMPL_ACTION_ATTR(nsHTMLFormElement, Action, action)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLFormElement, Autocomplete, autocomplete,
                                kFormDefaultAutocomplete->tag)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLFormElement, Enctype, enctype,
                                kFormDefaultEnctype->tag)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLFormElement, Method, method,
                                kFormDefaultMethod->tag)
NS_IMPL_BOOL_ATTR(nsHTMLFormElement, NoValidate, novalidate)
NS_IMPL_STRING_ATTR(nsHTMLFormElement, Name, name)
NS_IMPL_STRING_ATTR(nsHTMLFormElement, Target, target)

NS_IMETHODIMP
nsHTMLFormElement::Submit()
{
  
  nsresult rv = NS_OK;
  nsRefPtr<nsPresContext> presContext = GetPresContext();
  if (mPendingSubmission) {
    
    
    
    
    mPendingSubmission = nsnull;
  }

  rv = DoSubmitOrReset(nsnull, NS_FORM_SUBMIT);
  return rv;
}

NS_IMETHODIMP
nsHTMLFormElement::Reset()
{
  nsFormEvent event(true, NS_FORM_RESET);
  nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), nsnull,
                              &event);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::CheckValidity(bool* retVal)
{
  *retVal = CheckFormValidity(nsnull);
  return NS_OK;
}

bool
nsHTMLFormElement::ParseAttribute(PRInt32 aNamespaceID,
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
nsHTMLFormElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
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

static void
MarkOrphans(const nsTArray<nsGenericHTMLFormElement*> aArray)
{
  PRUint32 length = aArray.Length();
  for (PRUint32 i = 0; i < length; ++i) {
    aArray[i]->SetFlags(MAYBE_ORPHAN_FORM_ELEMENT);
  }
}

static void
CollectOrphans(nsINode* aRemovalRoot, nsTArray<nsGenericHTMLFormElement*> aArray
#ifdef DEBUG
               , nsIDOMHTMLFormElement* aThisForm
#endif
               )
{
  
  nsAutoScriptBlocker scriptBlocker;

  
  PRUint32 length = aArray.Length();
  for (PRUint32 i = length; i > 0; --i) {
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

void
nsHTMLFormElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsCOMPtr<nsIHTMLDocument> oldDocument = do_QueryInterface(GetCurrentDoc());

  
  MarkOrphans(mControls->mElements);
  MarkOrphans(mControls->mNotInElements);

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);

  nsINode* ancestor = this;
  nsINode* cur;
  do {
    cur = ancestor->GetNodeParent();
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

  if (oldDocument) {
    oldDocument->RemovedForm();
  }     
  ForgetCurrentSubmission();
}

nsresult
nsHTMLFormElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mWantsWillHandleEvent = true;
  if (aVisitor.mEvent->originalTarget == static_cast<nsIContent*>(this)) {
    PRUint32 msg = aVisitor.mEvent->message;
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
nsHTMLFormElement::WillHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  
  
  
  if ((aVisitor.mEvent->message == NS_FORM_SUBMIT ||
       aVisitor.mEvent->message == NS_FORM_RESET) &&
      aVisitor.mEvent->flags & NS_EVENT_FLAG_BUBBLE &&
      aVisitor.mEvent->originalTarget != static_cast<nsIContent*>(this)) {
    aVisitor.mEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
  }
  return NS_OK;
}

nsresult
nsHTMLFormElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (aVisitor.mEvent->originalTarget == static_cast<nsIContent*>(this)) {
    PRUint32 msg = aVisitor.mEvent->message;
    if (msg == NS_FORM_SUBMIT) {
      
      mDeferSubmission = false;
    }

    if (aVisitor.mEventStatus == nsEventStatus_eIgnore) {
      switch (msg) {
        case NS_FORM_RESET:
        case NS_FORM_SUBMIT:
        {
          if (mPendingSubmission && msg == NS_FORM_SUBMIT) {
            
            
            
            
            
            mPendingSubmission = nsnull;
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
nsHTMLFormElement::DoSubmitOrReset(nsEvent* aEvent,
                                   PRInt32 aMessage)
{
  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    doc->FlushPendingNotifications(Flush_ContentAndNotify);
  }

  

  
  if (NS_FORM_RESET == aMessage) {
    return DoReset();
  }

  if (NS_FORM_SUBMIT == aMessage) {
    
    if (!doc) {
      return NS_OK;
    }
    return DoSubmit(aEvent);
  }

  MOZ_ASSERT(false);
  return NS_OK;
}

nsresult
nsHTMLFormElement::DoReset()
{
  
  PRUint32 numElements = GetElementCount();
  for (PRUint32 elementX = 0; elementX < numElements; ++elementX) {
    
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
nsHTMLFormElement::DoSubmit(nsEvent* aEvent)
{
  NS_ASSERTION(GetCurrentDoc(), "Should never get here without a current doc");

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

  mSubmitInitiatedFromUserInput = nsEventStateManager::IsHandlingUserInput();

  if(mDeferSubmission) { 
    
    
    
    mPendingSubmission = submission;
    
    mIsSubmitting = false;
    return NS_OK; 
  } 
  
  
  
  
  return SubmitSubmission(submission); 
}

nsresult
nsHTMLFormElement::BuildSubmission(nsFormSubmission** aFormSubmission, 
                                   nsEvent* aEvent)
{
  NS_ASSERTION(!mPendingSubmission, "tried to build two submissions!");

  
  nsGenericHTMLElement* originatingElement = nsnull;
  if (aEvent) {
    if (NS_FORM_EVENT == aEvent->eventStructType) {
      nsIContent* originator = ((nsFormEvent *)aEvent)->originator;
      if (originator) {
        if (!originator->IsHTML()) {
          return NS_ERROR_UNEXPECTED;
        }
        originatingElement =
          static_cast<nsGenericHTMLElement*>(((nsFormEvent *)aEvent)->originator);
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
nsHTMLFormElement::SubmitSubmission(nsFormSubmission* aFormSubmission)
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

  
  nsIDocument* doc = GetCurrentDoc();
  nsCOMPtr<nsISupports> container = doc ? doc->GetContainer() : nsnull;
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

    nsAutoHandlingUserInputStatePusher userInpStatePusher(
                                         mSubmitInitiatedFromUserInput,
                                         nsnull, doc);

    nsCOMPtr<nsIInputStream> postDataStream;
    rv = aFormSubmission->GetEncodedSubmission(actionURI,
                                               getter_AddRefs(postDataStream));
    NS_ENSURE_SUBMIT_SUCCESS(rv);

    rv = linkHandler->OnLinkClickSync(this, actionURI,
                                      target.get(),
                                      postDataStream, nsnull,
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
nsHTMLFormElement::NotifySubmitObservers(nsIURI* aActionURL,
                                         bool* aCancelSubmit,
                                         bool    aEarlyNotify)
{
  
  
  if (!gFirstFormSubmitted) {
    gFirstFormSubmitted = true;
    NS_CreateServicesFromCategory(NS_FIRST_FORMSUBMIT_CATEGORY,
                                  nsnull,
                                  NS_FIRST_FORMSUBMIT_CATEGORY);
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
nsHTMLFormElement::WalkFormElements(nsFormSubmission* aFormSubmission)
{
  nsTArray<nsGenericHTMLFormElement*> sortedControls;
  nsresult rv = mControls->GetSortedControls(sortedControls);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  PRUint32 len = sortedControls.Length();
  for (PRUint32 i = 0; i < len; ++i) {
    
    sortedControls[i]->SubmitNamesValues(aFormSubmission);
  }

  return NS_OK;
}



NS_IMETHODIMP_(PRUint32)
nsHTMLFormElement::GetElementCount() const 
{
  PRUint32 count = nsnull;
  mControls->GetLength(&count); 
  return count;
}

NS_IMETHODIMP_(nsIFormControl*)
nsHTMLFormElement::GetElementAt(PRInt32 aIndex) const
{
  return mControls->mElements.SafeElementAt(aIndex, nsnull);
}










static inline PRInt32
CompareFormControlPosition(nsGenericHTMLFormElement *aControl1,
                           nsGenericHTMLFormElement *aControl2,
                           const nsIContent* aForm)
{
  NS_ASSERTION(aControl1 != aControl2, "Comparing a form control to itself");

  
  
  NS_ASSERTION((aControl1->HasAttr(kNameSpaceID_None, nsGkAtoms::form) ||
                aControl1->GetParent()) &&
               (aControl2->HasAttr(kNameSpaceID_None, nsGkAtoms::form) ||
                aControl2->GetParent()),
               "Form controls should always have parents");

  
  
  
  
#ifdef DEBUG
  nsLayoutUtils::gPreventAssertInCompareTreePosition = true;
  PRInt32 rVal = nsLayoutUtils::CompareTreePosition(aControl1, aControl2, aForm);
  nsLayoutUtils::gPreventAssertInCompareTreePosition = false;

  return rVal;
#else 
  return nsLayoutUtils::CompareTreePosition(aControl1, aControl2, aForm);
#endif 
}
 
#ifdef DEBUG







static void
AssertDocumentOrder(const nsTArray<nsGenericHTMLFormElement*>& aControls,
                    nsIContent* aForm)
{
  
  
  return;

  
  
  
  if (!aControls.IsEmpty()) {
    for (PRUint32 i = 0; i < aControls.Length() - 1; ++i) {
      NS_ASSERTION(CompareFormControlPosition(aControls[i], aControls[i + 1],
                                              aForm) < 0,
                   "Form controls not ordered correctly");
    }
  }
}
#endif

nsresult
nsHTMLFormElement::AddElement(nsGenericHTMLFormElement* aChild,
                              bool aUpdateValidity, bool aNotify)
{
  
  
  NS_ASSERTION(aChild->HasAttr(kNameSpaceID_None, nsGkAtoms::form) ||
               aChild->GetParent(),
               "Form control should have a parent");

  
  
  bool childInElements = ShouldBeInElements(aChild);
  nsTArray<nsGenericHTMLFormElement*>& controlList = childInElements ?
      mControls->mElements : mControls->mNotInElements;
  
  NS_ASSERTION(controlList.IndexOf(aChild) == controlList.NoIndex,
               "Form control already in form");

  PRUint32 count = controlList.Length();
  nsGenericHTMLFormElement* element;
  
  
  bool lastElement = false;
  PRInt32 position = -1;
  if (count > 0) {
    element = controlList[count - 1];
    position = CompareFormControlPosition(aChild, element, this);
  }

  
  
  
  if (position >= 0 || count == 0) {
    
    controlList.AppendElement(aChild);
    lastElement = true;
  }
  else {
    PRInt32 low = 0, mid, high;
    high = count - 1;
      
    while (low <= high) {
      mid = (low + high) / 2;
        
      element = controlList[mid];
      position = CompareFormControlPosition(aChild, element, this);
      if (position >= 0)
        low = mid + 1;
      else
        high = mid - 1;
    }
      
    
    controlList.InsertElementAt(low, aChild);
  }

#ifdef DEBUG
  AssertDocumentOrder(controlList, this);
#endif

  PRInt32 type = aChild->GetType();

  
  
  
  
  if (!gPasswordManagerInitialized && type == NS_FORM_INPUT_PASSWORD) {
    
    gPasswordManagerInitialized = true;
    NS_CreateServicesFromCategory(NS_PASSWORDMANAGER_CATEGORY,
                                  nsnull,
                                  NS_PASSWORDMANAGER_CATEGORY);
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
    nsRefPtr<nsHTMLInputElement> radio =
      static_cast<nsHTMLInputElement*>(aChild);
    radio->AddedToRadioGroup();
  }

  return NS_OK;
}

nsresult
nsHTMLFormElement::AddElementToTable(nsGenericHTMLFormElement* aChild,
                                     const nsAString& aName)
{
  return mControls->AddElementToTable(aChild, aName);  
}


nsresult
nsHTMLFormElement::RemoveElement(nsGenericHTMLFormElement* aChild,
                                 bool aUpdateValidity)
{
  
  
  
  nsresult rv = NS_OK;
  if (aChild->GetType() == NS_FORM_INPUT_RADIO) {
    nsRefPtr<nsHTMLInputElement> radio =
      static_cast<nsHTMLInputElement*>(aChild);
    radio->WillRemoveFromRadioGroup();
  }

  
  
  bool childInElements = ShouldBeInElements(aChild);
  nsTArray<nsGenericHTMLFormElement*>& controls = childInElements ?
      mControls->mElements :  mControls->mNotInElements;
  
  
  
  PRUint32 index = controls.IndexOf(aChild);
  NS_ENSURE_STATE(index != controls.NoIndex);

  controls.RemoveElementAt(index);

  
  nsGenericHTMLFormElement** firstSubmitSlot =
    childInElements ? &mFirstSubmitInElements : &mFirstSubmitNotInElements;
  if (aChild == *firstSubmitSlot) {
    *firstSubmitSlot = nsnull;

    
    PRUint32 length = controls.Length();
    for (PRUint32 i = index; i < length; ++i) {
      nsGenericHTMLFormElement* currentControl = controls[i];
      if (currentControl->IsSubmitControl()) {
        *firstSubmitSlot = currentControl;
        break;
      }
    }
  }

  if (aChild == mDefaultSubmitElement) {
    
    
    mDefaultSubmitElement = nsnull;
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
nsHTMLFormElement::HandleDefaultSubmitRemoval()
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
nsHTMLFormElement::RemoveElementFromTable(nsGenericHTMLFormElement* aElement,
                                          const nsAString& aName)
{
  return mControls->RemoveElementFromTable(aElement, aName);
}

NS_IMETHODIMP_(already_AddRefed<nsISupports>)
nsHTMLFormElement::ResolveName(const nsAString& aName)
{
  return DoResolveName(aName, true);
}

already_AddRefed<nsISupports>
nsHTMLFormElement::DoResolveName(const nsAString& aName,
                                 bool aFlushContent)
{
  nsISupports *result;
  NS_IF_ADDREF(result = mControls->NamedItemInternal(aName, aFlushContent));
  return result;
}

void
nsHTMLFormElement::OnSubmitClickBegin(nsIContent* aOriginatingElement)
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
nsHTMLFormElement::OnSubmitClickEnd()
{
  mDeferSubmission = false;
}

void
nsHTMLFormElement::FlushPendingSubmission()
{
  if (mPendingSubmission) {
    
    
    nsAutoPtr<nsFormSubmission> submission = mPendingSubmission;

    SubmitSubmission(submission);
  }
}

nsresult
nsHTMLFormElement::GetActionURL(nsIURI** aActionURL,
                                nsIContent* aOriginatingElement)
{
  nsresult rv = NS_OK;

  *aActionURL = nsnull;

  
  
  
  
  
  
  
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
    rv = NS_NewURI(getter_AddRefs(actionURL), action, nsnull, baseURL);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  nsIScriptSecurityManager *securityManager =
      nsContentUtils::GetSecurityManager();
  rv = securityManager->
    CheckLoadURIWithPrincipal(NodePrincipal(), actionURL,
                              nsIScriptSecurityManager::STANDARD);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  *aActionURL = actionURL;
  NS_ADDREF(*aActionURL);

  return rv;
}

NS_IMETHODIMP_(nsIFormControl*)
nsHTMLFormElement::GetDefaultSubmitElement() const
{
  NS_PRECONDITION(mDefaultSubmitElement == mFirstSubmitInElements ||
                  mDefaultSubmitElement == mFirstSubmitNotInElements,
                  "What happened here?");
  
  return mDefaultSubmitElement;
}

bool
nsHTMLFormElement::IsDefaultSubmitElement(const nsIFormControl* aControl) const
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
nsHTMLFormElement::HasSingleTextControl() const
{
  
  PRUint32 numTextControlsFound = 0;
  PRUint32 length = mControls->mElements.Length();
  for (PRUint32 i = 0; i < length && numTextControlsFound < 2; ++i) {
    if (mControls->mElements[i]->IsSingleLineTextControl(false)) {
      numTextControlsFound++;
    }
  }
  return numTextControlsFound == 1;
}

NS_IMETHODIMP
nsHTMLFormElement::GetEncoding(nsAString& aEncoding)
{
  return GetEnctype(aEncoding);
}
 
NS_IMETHODIMP
nsHTMLFormElement::SetEncoding(const nsAString& aEncoding)
{
  return SetEnctype(aEncoding);
}

NS_IMETHODIMP    
nsHTMLFormElement::GetLength(PRInt32* aLength)
{
  PRUint32 length;
  nsresult rv = mControls->GetLength(&length);
  *aLength = length;
  return rv;
}

void
nsHTMLFormElement::ForgetCurrentSubmission()
{
  mNotifiedObservers = false;
  mIsSubmitting = false;
  mSubmittingRequest = nsnull;
  nsCOMPtr<nsIWebProgress> webProgress = do_QueryReferent(mWebProgress);
  if (webProgress) {
    webProgress->RemoveProgressListener(this);
  }
  mWebProgress = nsnull;
}

bool
nsHTMLFormElement::CheckFormValidity(nsIMutableArray* aInvalidElements) const
{
  bool ret = true;

  nsTArray<nsGenericHTMLFormElement*> sortedControls;
  if (NS_FAILED(mControls->GetSortedControls(sortedControls))) {
    return false;
  }

  PRUint32 len = sortedControls.Length();

  
  
  for (PRUint32 i = 0; i < len; ++i) {
    static_cast<nsGenericHTMLElement*>(sortedControls[i])->AddRef();
  }

  for (PRUint32 i = 0; i < len; ++i) {
    nsCOMPtr<nsIConstraintValidation> cvElmt =
      do_QueryInterface((nsGenericHTMLElement*)sortedControls[i]);
    if (cvElmt && cvElmt->IsCandidateForConstraintValidation() &&
        !cvElmt->IsValid()) {
      ret = false;
      bool defaultAction = true;
      nsContentUtils::DispatchTrustedEvent(sortedControls[i]->OwnerDoc(),
                                           static_cast<nsIContent*>(sortedControls[i]),
                                           NS_LITERAL_STRING("invalid"),
                                           false, true, &defaultAction);

      
      
      if (defaultAction && aInvalidElements) {
        aInvalidElements->AppendElement((nsGenericHTMLElement*)sortedControls[i],
                                        false);
      }
    }
  }

  
  for (PRUint32 i = 0; i < len; ++i) {
    static_cast<nsGenericHTMLElement*>(sortedControls[i])->Release();
  }

  return ret;
}

bool
nsHTMLFormElement::CheckValidFormSubmission()
{
  













  NS_ASSERTION(!HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate),
               "We shouldn't be there if novalidate is set!");

  
  
  nsCOMPtr<nsIObserverService> service =
    mozilla::services::GetObserverService();
  if (!service) {
    NS_WARNING("No observer service available!");
    return true;
  }

  nsCOMPtr<nsISimpleEnumerator> theEnum;
  nsresult rv = service->EnumerateObservers(NS_INVALIDFORMSUBMIT_SUBJECT,
                                            getter_AddRefs(theEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasObserver = false;
  rv = theEnum->HasMoreElements(&hasObserver);

  
  
  if (NS_SUCCEEDED(rv) && hasObserver) {
    nsCOMPtr<nsIMutableArray> invalidElements =
      do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!CheckFormValidity(invalidElements.get())) {
      
      
      
      if (!mEverTriedInvalidSubmit) {
        mEverTriedInvalidSubmit = true;

        





        nsAutoScriptBlocker scriptBlocker;

        for (PRUint32 i = 0, length = mControls->mElements.Length();
             i < length; ++i) {
          
          
          if (mControls->mElements[i]->IsHTML(nsGkAtoms::input) &&
              nsContentUtils::IsFocusedContent(mControls->mElements[i])) {
            static_cast<nsHTMLInputElement*>(mControls->mElements[i])
              ->UpdateValidityUIBits(true);
          }

          mControls->mElements[i]->UpdateState(true);
        }

        
        
        
        for (PRUint32 i = 0, length = mControls->mNotInElements.Length();
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
nsHTMLFormElement::UpdateValidity(bool aElementValidity)
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

  
  for (PRUint32 i = 0, length = mControls->mElements.Length();
       i < length; ++i) {
    if (mControls->mElements[i]->IsSubmitControl()) {
      mControls->mElements[i]->UpdateState(true);
    }
  }

  
  
  PRUint32 length = mControls->mNotInElements.Length();
  for (PRUint32 i = 0; i < length; ++i) {
    if (mControls->mNotInElements[i]->IsSubmitControl()) {
      mControls->mNotInElements[i]->UpdateState(true);
    }
  }

  UpdateState(true);
}


NS_IMETHODIMP
nsHTMLFormElement::OnStateChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 PRUint32 aStateFlags,
                                 PRUint32 aStatus)
{
  
  
  
  
  if (aRequest == mSubmittingRequest &&
      aStateFlags & nsIWebProgressListener::STATE_STOP) {
    ForgetCurrentSubmission();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::OnProgressChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    PRInt32 aCurSelfProgress,
                                    PRInt32 aMaxSelfProgress,
                                    PRInt32 aCurTotalProgress,
                                    PRInt32 aMaxTotalProgress)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::OnLocationChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    nsIURI* location,
                                    PRUint32 aFlags)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::OnStatusChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsresult aStatus,
                                  const PRUnichar* aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::OnSecurityChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    PRUint32 state)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}
 
NS_IMETHODIMP_(PRInt32)
nsHTMLFormElement::IndexOfControl(nsIFormControl* aControl)
{
  PRInt32 index = nsnull;
  return mControls->IndexOfControl(aControl, &index) == NS_OK ? index : nsnull;
}

NS_IMETHODIMP
nsHTMLFormElement::SetCurrentRadioButton(const nsAString& aName,
                                         nsIDOMHTMLInputElement* aRadio)
{
  mSelectedRadioButtons.Put(aName, aRadio);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::GetCurrentRadioButton(const nsAString& aName,
                                         nsIDOMHTMLInputElement** aRadio)
{
  mSelectedRadioButtons.Get(aName, aRadio);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::GetNextRadioButton(const nsAString& aName,
                                      const bool aPrevious,
                                      nsIDOMHTMLInputElement*  aFocusedRadio,
                                      nsIDOMHTMLInputElement** aRadioOut)
{
  
  
  *aRadioOut = nsnull;

  nsCOMPtr<nsIDOMHTMLInputElement> currentRadio;
  if (aFocusedRadio) {
    currentRadio = aFocusedRadio;
  }
  else {
    mSelectedRadioButtons.Get(aName, getter_AddRefs(currentRadio));
  }

  nsCOMPtr<nsISupports> itemWithName = ResolveName(aName);
  nsCOMPtr<nsINodeList> radioGroup(do_QueryInterface(itemWithName));

  if (!radioGroup) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIContent> currentRadioNode(do_QueryInterface(currentRadio));
  NS_ASSERTION(currentRadioNode, "No nsIContent for current radio button");
  PRInt32 index = radioGroup->IndexOf(currentRadioNode);
  if (index < 0) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 numRadios;
  radioGroup->GetLength(&numRadios);
  bool disabled = true;
  nsCOMPtr<nsIDOMHTMLInputElement> radio;
  nsCOMPtr<nsIFormControl> formControl;

  do {
    if (aPrevious) {
      if (--index < 0) {
        index = numRadios -1;
      }
    }
    else if (++index >= (PRInt32)numRadios) {
      index = 0;
    }
    radio = do_QueryInterface(radioGroup->GetNodeAt(index));
    if (!radio)
      continue;

    
    formControl = do_QueryInterface(radio);
    if (!formControl || formControl->GetType() != NS_FORM_INPUT_RADIO)
      continue;

    radio->GetDisabled(&disabled);
  } while (disabled && radio != currentRadio);

  NS_IF_ADDREF(*aRadioOut = radio);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::WalkRadioGroup(const nsAString& aName,
                                  nsIRadioVisitor* aVisitor,
                                  bool aFlushContent)
{
  if (aName.IsEmpty()) {
    
    
    
    
    nsCOMPtr<nsIFormControl> control;
    PRUint32 len = GetElementCount();
    for (PRUint32 i = 0; i < len; i++) {
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
  PRUint32 length = 0;
  nodeList->GetLength(&length);
  for (PRUint32 i = 0; i < length; i++) {
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

NS_IMETHODIMP
nsHTMLFormElement::AddToRadioGroup(const nsAString& aName,
                                   nsIFormControl* aRadio)
{
  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements!");

  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    mRequiredRadioButtonCounts.Put(aName,
                                   mRequiredRadioButtonCounts.Get(aName)+1);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::RemoveFromRadioGroup(const nsAString& aName,
                                        nsIFormControl* aRadio)
{
  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements!");

  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    PRUint32 requiredNb = mRequiredRadioButtonCounts.Get(aName);
    NS_ASSERTION(requiredNb >= 1,
                 "At least one radio button has to be required!");

    if (requiredNb == 1) {
      mRequiredRadioButtonCounts.Remove(aName);
    } else {
      mRequiredRadioButtonCounts.Put(aName, requiredNb-1);
    }
  }

  return NS_OK;
}

PRUint32
nsHTMLFormElement::GetRequiredRadioCount(const nsAString& aName) const
{
  return mRequiredRadioButtonCounts.Get(aName);
}

void
nsHTMLFormElement::RadioRequiredChanged(const nsAString& aName,
                                        nsIFormControl* aRadio)
{
  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements!");

  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    mRequiredRadioButtonCounts.Put(aName,
                                   mRequiredRadioButtonCounts.Get(aName)+1);
  } else {
    PRUint32 requiredNb = mRequiredRadioButtonCounts.Get(aName);
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
nsHTMLFormElement::GetValueMissingState(const nsAString& aName) const
{
  return mValueMissingRadioGroups.Get(aName);
}

void
nsHTMLFormElement::SetValueMissingState(const nsAString& aName, bool aValue)
{
  mValueMissingRadioGroups.Put(aName, aValue);
}

nsEventStates
nsHTMLFormElement::IntrinsicState() const
{
  nsEventStates state = nsGenericHTMLElement::IntrinsicState();

  if (mInvalidElementsCount) {
    state |= NS_EVENT_STATE_INVALID;
  } else {
      state |= NS_EVENT_STATE_VALID;
  }

  return state;
}





nsFormControlList::nsFormControlList(nsHTMLFormElement* aForm) :
  mForm(aForm),
  
  
  mElements(8)
{
  SetIsDOMBinding();
}

nsFormControlList::~nsFormControlList()
{
  mForm = nsnull;
  Clear();
}

nsresult nsFormControlList::Init()
{
  mNameLookupTable.Init(NS_FORM_CONTROL_LIST_HASHTABLE_SIZE);
  return NS_OK;
}

void
nsFormControlList::DropFormReference()
{
  mForm = nsnull;
  Clear();
}

void
nsFormControlList::Clear()
{
  
  for (PRInt32 i = mElements.Length() - 1; i >= 0; i--) {
    mElements[i]->ClearForm(false);
  }
  mElements.Clear();

  for (PRInt32 i = mNotInElements.Length() - 1; i >= 0; i--) {
    mNotInElements[i]->ClearForm(false);
  }
  mNotInElements.Clear();

  mNameLookupTable.Clear();
}

void
nsFormControlList::FlushPendingNotifications()
{
  if (mForm) {
    nsIDocument* doc = mForm->GetCurrentDoc();
    if (doc) {
      doc->FlushPendingNotifications(Flush_Content);
    }
  }
}

static PLDHashOperator
ControlTraverser(const nsAString& key, nsISupports* control, void* userArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(userArg);
 
  cb->NoteXPCOMChild(control);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsFormControlList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsFormControlList)
  tmp->Clear();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFormControlList)
  tmp->mNameLookupTable.EnumerateRead(ControlTraverser, &cb);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsFormControlList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

DOMCI_DATA(HTMLCollection, nsFormControlList)


NS_INTERFACE_TABLE_HEAD(nsFormControlList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_TABLE2(nsFormControlList,
                      nsIHTMLCollection,
                      nsIDOMHTMLCollection)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsFormControlList)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLCollection)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFormControlList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFormControlList)




NS_IMETHODIMP    
nsFormControlList::GetLength(PRUint32* aLength)
{
  FlushPendingNotifications();
  *aLength = mElements.Length();
  return NS_OK;
}

NS_IMETHODIMP
nsFormControlList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsISupports* item = GetNodeAt(aIndex);
  if (!item) {
    *aReturn = nsnull;

    return NS_OK;
  }

  return CallQueryInterface(item, aReturn);
}

NS_IMETHODIMP 
nsFormControlList::NamedItem(const nsAString& aName,
                             nsIDOMNode** aReturn)
{
  FlushPendingNotifications();

  *aReturn = nsnull;

  nsCOMPtr<nsISupports> supports;
  
  if (!mNameLookupTable.Get(aName, getter_AddRefs(supports))) {
    
    return NS_OK;
  }

  if (!supports) {
    return NS_OK;
  }

  
  CallQueryInterface(supports, aReturn);
  if (*aReturn) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNodeList> nodeList = do_QueryInterface(supports);
  NS_ASSERTION(nodeList, "Huh, what's going one here?");
  if (!nodeList) {
    return NS_OK;
  }

  
  
  return nodeList->Item(0, aReturn);
}

nsISupports*
nsFormControlList::NamedItemInternal(const nsAString& aName,
                                     bool aFlushContent)
{
  if (aFlushContent) {
    FlushPendingNotifications();
  }

  return mNameLookupTable.GetWeak(aName);
}

nsresult
nsFormControlList::AddElementToTable(nsGenericHTMLFormElement* aChild,
                                     const nsAString& aName)
{
  if (!ShouldBeInElements(aChild)) {
    return NS_OK;
  }

  nsCOMPtr<nsISupports> supports;
  mNameLookupTable.Get(aName, getter_AddRefs(supports));

  if (!supports) {
    
    mNameLookupTable.Put(aName, NS_ISUPPORTS_CAST(nsIContent*, aChild));
  } else {
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(supports);

    if (content) {
      
      
      
      
      if (content == aChild) {
        return NS_OK;
      }

      
      
      nsSimpleContentList *list = new nsSimpleContentList(mForm);

      
      
      NS_ASSERTION(content->HasAttr(kNameSpaceID_None, nsGkAtoms::form) ||
                   content->GetParent(), "Item in list without parent");

      
      bool newFirst = nsContentUtils::PositionIsBefore(aChild, content);

      list->AppendElement(newFirst ? aChild : content);
      list->AppendElement(newFirst ? content : aChild);


      nsCOMPtr<nsISupports> listSupports = do_QueryObject(list);

      
      mNameLookupTable.Put(aName, listSupports);
    } else {
      
      nsCOMPtr<nsIDOMNodeList> nodeList = do_QueryInterface(supports);
      NS_ENSURE_TRUE(nodeList, NS_ERROR_FAILURE);

      
      nsSimpleContentList *list =
        static_cast<nsSimpleContentList*>(nodeList.get());

      NS_ASSERTION(list->Length() > 1,
                   "List should have been converted back to a single element");

      
      
      
      
      if (nsContentUtils::PositionIsBefore(list->GetNodeAt(list->Length() - 1), aChild)) {
        list->AppendElement(aChild);
        return NS_OK;
      }

      
      
      if (list->IndexOf(aChild) != -1) {
        return NS_OK;
      }
      
      
      
      PRUint32 first = 0;
      PRUint32 last = list->Length() - 1;
      PRUint32 mid;
      
      
      while (last != first) {
        mid = (first + last) / 2;
          
        if (nsContentUtils::PositionIsBefore(aChild, list->GetNodeAt(mid)))
          last = mid;
        else
          first = mid + 1;
      }

      list->InsertElementAt(aChild, first);
    }
  }

  return NS_OK;
}

nsresult
nsFormControlList::IndexOfControl(nsIFormControl* aControl,
                                  PRInt32* aIndex)
{
  
  
  NS_ENSURE_ARG_POINTER(aIndex);

  *aIndex = mElements.IndexOf(aControl);

  return NS_OK;
}

nsresult
nsFormControlList::RemoveElementFromTable(nsGenericHTMLFormElement* aChild,
                                          const nsAString& aName)
{
  if (!ShouldBeInElements(aChild)) {
    return NS_OK;
  }

  nsCOMPtr<nsISupports> supports;

  if (!mNameLookupTable.Get(aName, getter_AddRefs(supports)))
    return NS_OK;

  nsCOMPtr<nsIFormControl> fctrl(do_QueryInterface(supports));

  if (fctrl) {
    
    
    if (fctrl == aChild) {
      mNameLookupTable.Remove(aName);
    }

    return NS_OK;
  }

  nsCOMPtr<nsIDOMNodeList> nodeList(do_QueryInterface(supports));
  NS_ENSURE_TRUE(nodeList, NS_ERROR_FAILURE);

  
  nsBaseContentList *list = static_cast<nsBaseContentList*>(nodeList.get());

  list->RemoveElement(aChild);

  PRUint32 length = 0;
  list->GetLength(&length);

  if (!length) {
    
    
    mNameLookupTable.Remove(aName);
  } else if (length == 1) {
    
    
    nsIContent* node = list->GetNodeAt(0);
    if (node) {
      mNameLookupTable.Put(aName, node);
    }
  }

  return NS_OK;
}

nsresult
nsFormControlList::GetSortedControls(nsTArray<nsGenericHTMLFormElement*>& aControls) const
{
#ifdef DEBUG
  AssertDocumentOrder(mElements, mForm);
  AssertDocumentOrder(mNotInElements, mForm);
#endif

  aControls.Clear();

  
  
  PRUint32 elementsLen = mElements.Length();
  PRUint32 notInElementsLen = mNotInElements.Length();
  aControls.SetCapacity(elementsLen + notInElementsLen);

  PRUint32 elementsIdx = 0;
  PRUint32 notInElementsIdx = 0;

  while (elementsIdx < elementsLen || notInElementsIdx < notInElementsLen) {
    
    if (elementsIdx == elementsLen) {
      NS_ASSERTION(notInElementsIdx < notInElementsLen,
                   "Should have remaining not-in-elements");
      
      if (!aControls.AppendElements(mNotInElements.Elements() +
                                      notInElementsIdx,
                                    notInElementsLen -
                                      notInElementsIdx)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      break;
    }
    
    if (notInElementsIdx == notInElementsLen) {
      NS_ASSERTION(elementsIdx < elementsLen,
                   "Should have remaining in-elements");
      
      if (!aControls.AppendElements(mElements.Elements() +
                                      elementsIdx,
                                    elementsLen -
                                      elementsIdx)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      break;
    }
    
    NS_ASSERTION(mElements[elementsIdx] &&
                 mNotInElements[notInElementsIdx],
                 "Should have remaining elements");
    
    
    nsGenericHTMLFormElement* elementToAdd;
    if (CompareFormControlPosition(mElements[elementsIdx],
                                   mNotInElements[notInElementsIdx],
                                   mForm) < 0) {
      elementToAdd = mElements[elementsIdx];
      ++elementsIdx;
    } else {
      elementToAdd = mNotInElements[notInElementsIdx];
      ++notInElementsIdx;
    }
    
    if (!aControls.AppendElement(elementToAdd)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ASSERTION(aControls.Length() == elementsLen + notInElementsLen,
               "Not all form controls were added to the sorted list");
#ifdef DEBUG
  AssertDocumentOrder(aControls, mForm);
#endif

  return NS_OK;
}

nsIContent*
nsFormControlList::GetNodeAt(PRUint32 aIndex)
{
  FlushPendingNotifications();

  return mElements.SafeElementAt(aIndex, nsnull);
}

nsISupports*
nsFormControlList::GetNamedItem(const nsAString& aName, nsWrapperCache **aCache)
{
  nsISupports *item = NamedItemInternal(aName, true);
  *aCache = nsnull;
  return item;
}
