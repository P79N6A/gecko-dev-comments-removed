



































#include "nsHTMLFormElement.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsEventStateManager.h"
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


#include "nsIDOMHTMLInputElement.h"
#include "nsIRadioControlElement.h"
#include "nsIRadioVisitor.h"

#include "nsLayoutUtils.h"

#include "nsEventDispatcher.h"

#include "mozAutoDocUpdate.h"
#include "nsIHTMLCollection.h"

static const int NS_FORM_CONTROL_LIST_HASHTABLE_SIZE = 16;



PRBool nsHTMLFormElement::gFirstFormSubmitted = PR_FALSE;
PRBool nsHTMLFormElement::gPasswordManagerInitialized = PR_FALSE;



class nsFormControlList : public nsIHTMLCollection
{
public:
  nsFormControlList(nsHTMLFormElement* aForm);
  virtual ~nsFormControlList();

  nsresult Init();

  void DropFormReference();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMHTMLCOLLECTION

  virtual nsISupports* GetNodeAt(PRUint32 aIndex, nsresult* aResult)
  {
    FlushPendingNotifications();

    *aResult = NS_OK;

    
    return static_cast<nsIFormControl*>(mElements.SafeElementAt(aIndex, nsnull));
  }
  virtual nsISupports* GetNamedItem(const nsAString& aName, nsresult* aResult)
  {
    *aResult = NS_OK;

    return NamedItemInternal(aName, PR_TRUE);
  }

  nsresult AddElementToTable(nsGenericHTMLFormElement* aChild,
                             const nsAString& aName);
  nsresult RemoveElementFromTable(nsGenericHTMLFormElement* aChild,
                                  const nsAString& aName);
  nsresult IndexOfControl(nsIFormControl* aControl,
                          PRInt32* aIndex);

  nsISupports* NamedItemInternal(const nsAString& aName, PRBool aFlushContent);
  
  








  nsresult GetSortedControls(nsTArray<nsGenericHTMLFormElement*>& aControls) const;

  nsHTMLFormElement* mForm;  

  nsTArray<nsGenericHTMLFormElement*> mElements;  

  
  
  
  

  nsTArray<nsGenericHTMLFormElement*> mNotInElements; 

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFormControlList, nsIHTMLCollection)

protected:
  
  void Clear();

  
  void FlushPendingNotifications();
  
  
  
  
  
  

  nsInterfaceHashtable<nsStringHashKey,nsISupports> mNameLookupTable;
};

static PRBool
ShouldBeInElements(nsIFormControl* aFormControl)
{
  
  
  

  switch (aFormControl->GetType()) {
  case NS_FORM_BUTTON_BUTTON :
  case NS_FORM_BUTTON_RESET :
  case NS_FORM_BUTTON_SUBMIT :
  case NS_FORM_INPUT_BUTTON :
  case NS_FORM_INPUT_CHECKBOX :
  case NS_FORM_INPUT_FILE :
  case NS_FORM_INPUT_HIDDEN :
  case NS_FORM_INPUT_RESET :
  case NS_FORM_INPUT_PASSWORD :
  case NS_FORM_INPUT_RADIO :
  case NS_FORM_INPUT_SEARCH :
  case NS_FORM_INPUT_SUBMIT :
  case NS_FORM_INPUT_TEXT :
  case NS_FORM_INPUT_TEL :
  case NS_FORM_SELECT :
  case NS_FORM_TEXTAREA :
  case NS_FORM_FIELDSET :
  case NS_FORM_OBJECT :
  case NS_FORM_OUTPUT :
    return PR_TRUE;
  }

  
  
  
  
  
  

  return PR_FALSE;
}




nsGenericHTMLElement*
NS_NewHTMLFormElement(nsINodeInfo *aNodeInfo, PRBool aFromParser)
{
  nsHTMLFormElement* it = new nsHTMLFormElement(aNodeInfo);
  if (!it) {
    return nsnull;
  }

  nsresult rv = it->Init();

  if (NS_FAILED(rv)) {
    delete it;
    return nsnull;
  }

  return it;
}

nsHTMLFormElement::nsHTMLFormElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mGeneratingSubmit(PR_FALSE),
    mGeneratingReset(PR_FALSE),
    mIsSubmitting(PR_FALSE),
    mDeferSubmission(PR_FALSE),
    mNotifiedObservers(PR_FALSE),
    mNotifiedObserversResult(PR_FALSE),
    mSubmitPopupState(openAbused),
    mSubmitInitiatedFromUserInput(PR_FALSE),
    mPendingSubmission(nsnull),
    mSubmittingRequest(nsnull),
    mDefaultSubmitElement(nsnull),
    mFirstSubmitInElements(nsnull),
    mFirstSubmitNotInElements(nsnull)
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
  
  NS_ENSURE_TRUE(mSelectedRadioButtons.Init(4),
                 NS_ERROR_OUT_OF_MEMORY);

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


DOMCI_DATA(HTMLFormElement, nsHTMLFormElement)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLFormElement)
  NS_HTML_CONTENT_INTERFACE_TABLE5(nsHTMLFormElement,
                                   nsIDOMHTMLFormElement,
                                   nsIDOMNSHTMLFormElement,
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
                           PRBool aNotify)
{
  if ((aName == nsGkAtoms::action || aName == nsGkAtoms::target) &&
      aNameSpaceID == kNameSpaceID_None) {
    if (mPendingSubmission) {
      
      
      
      
      FlushPendingSubmission();
    }
    
    
    PRBool notifiedObservers = mNotifiedObservers;
    ForgetCurrentSubmission();
    mNotifiedObservers = notifiedObservers;
  }
  return nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                       aNotify);
}

NS_IMPL_STRING_ATTR(nsHTMLFormElement, AcceptCharset, acceptcharset)
NS_IMPL_STRING_ATTR(nsHTMLFormElement, Action, action)
NS_IMPL_STRING_ATTR(nsHTMLFormElement, Enctype, enctype)
NS_IMPL_STRING_ATTR(nsHTMLFormElement, Method, method)
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
  nsFormEvent event(PR_TRUE, NS_FORM_RESET);
  nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), nsnull,
                              &event);
  return NS_OK;
}

static const nsAttrValue::EnumTable kFormMethodTable[] = {
  { "get", NS_FORM_METHOD_GET },
  { "post", NS_FORM_METHOD_POST },
  { 0 }
};

static const nsAttrValue::EnumTable kFormEnctypeTable[] = {
  { "multipart/form-data", NS_FORM_ENCTYPE_MULTIPART },
  { "application/x-www-form-urlencoded", NS_FORM_ENCTYPE_URLENCODED },
  { "text/plain", NS_FORM_ENCTYPE_TEXTPLAIN },
  { 0 }
};

PRBool
nsHTMLFormElement::ParseAttribute(PRInt32 aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::method) {
      return aResult.ParseEnumValue(aValue, kFormMethodTable, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::enctype) {
      return aResult.ParseEnumValue(aValue, kFormEnctypeTable, PR_FALSE);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsresult
nsHTMLFormElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers)
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
  
  PRUint32 length = aArray.Length();
  for (PRUint32 i = length; i > 0; --i) {
    nsGenericHTMLFormElement* node = aArray[i-1];

    
    
    
    
    
#ifdef DEBUG
    PRBool removed = PR_FALSE;
#endif
    if (node->HasFlag(MAYBE_ORPHAN_FORM_ELEMENT)) {
      node->UnsetFlags(MAYBE_ORPHAN_FORM_ELEMENT);
      if (!nsContentUtils::ContentIsDescendantOf(node, aRemovalRoot)) {
        node->ClearForm(PR_TRUE, PR_TRUE);
#ifdef DEBUG
        removed = PR_TRUE;
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
nsHTMLFormElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
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
  aVisitor.mWantsWillHandleEvent = PR_TRUE;
  if (aVisitor.mEvent->originalTarget == static_cast<nsIContent*>(this)) {
    PRUint32 msg = aVisitor.mEvent->message;
    if (msg == NS_FORM_SUBMIT) {
      if (mGeneratingSubmit) {
        aVisitor.mCanHandle = PR_FALSE;
        return NS_OK;
      }
      mGeneratingSubmit = PR_TRUE;

      
      
      
      mDeferSubmission = PR_TRUE;
    }
    else if (msg == NS_FORM_RESET) {
      if (mGeneratingReset) {
        aVisitor.mCanHandle = PR_FALSE;
        return NS_OK;
      }
      mGeneratingReset = PR_TRUE;
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
      
      mDeferSubmission = PR_FALSE;
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
      mGeneratingSubmit = PR_FALSE;
    }
    else if (msg == NS_FORM_RESET) {
      mGeneratingReset = PR_FALSE;
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

  

  
  nsresult rv = NS_OK;
  if (NS_FORM_RESET == aMessage) {
    rv = DoReset();
  }
  else if (NS_FORM_SUBMIT == aMessage) {
    
    if (doc) {
      rv = DoSubmit(aEvent);
    }
  }
  return rv;
}

nsresult
nsHTMLFormElement::DoReset()
{
  
  PRUint32 numElements = GetElementCount();
  for (PRUint32 elementX = 0; (elementX < numElements); elementX++) {
    
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

  
  mIsSubmitting = PR_TRUE;
  NS_ASSERTION(!mWebProgress && !mSubmittingRequest, "Web progress / submitting request should not exist here!");

  nsAutoPtr<nsFormSubmission> submission;

  
  
  
  BuildSubmission(getter_Transfers(submission), aEvent); 

  
  
  nsPIDOMWindow *window = GetOwnerDoc()->GetWindow();

  if (window) {
    mSubmitPopupState = window->GetPopupControlState();
  } else {
    mSubmitPopupState = openAbused;
  }

  mSubmitInitiatedFromUserInput = nsEventStateManager::IsHandlingUserInput();

  if(mDeferSubmission) { 
    
    
    
    mPendingSubmission = submission;
    
    mIsSubmitting = PR_FALSE;
    return NS_OK; 
  } 
  
  
  
  
  return SubmitSubmission(submission); 
}

nsresult
nsHTMLFormElement::BuildSubmission(nsFormSubmission** aFormSubmission, 
                                   nsEvent* aEvent)
{
  NS_ASSERTION(!mPendingSubmission, "tried to build two submissions!");

  
  nsIContent *originatingElement = nsnull;
  if (aEvent) {
    if (NS_FORM_EVENT == aEvent->eventStructType) {
      originatingElement = ((nsFormEvent *)aEvent)->originator;
    }
  }

  nsresult rv;

  
  
  
  rv = GetSubmissionFromForm(this, aFormSubmission);
  NS_ENSURE_SUBMIT_SUCCESS(rv);

  
  
  
  rv = WalkFormElements(*aFormSubmission, originatingElement);
  NS_ENSURE_SUBMIT_SUCCESS(rv);

  return NS_OK;
}

nsresult
nsHTMLFormElement::SubmitSubmission(nsFormSubmission* aFormSubmission)
{
  nsresult rv;
  
  
  
  nsCOMPtr<nsIURI> actionURI;
  rv = GetActionURL(getter_AddRefs(actionURI));
  NS_ENSURE_SUBMIT_SUCCESS(rv);

  if (!actionURI) {
    mIsSubmitting = PR_FALSE;
    return NS_OK;
  }

  
  nsIDocument* doc = GetCurrentDoc();
  nsCOMPtr<nsISupports> container = doc ? doc->GetContainer() : nsnull;
  nsCOMPtr<nsILinkHandler> linkHandler(do_QueryInterface(container));
  if (!linkHandler || IsEditable()) {
    mIsSubmitting = PR_FALSE;
    return NS_OK;
  }

  
  
  
  
  
  
  
  
  
  
  
  PRBool schemeIsJavaScript = PR_FALSE;
  if (NS_SUCCEEDED(actionURI->SchemeIs("javascript", &schemeIsJavaScript)) &&
      schemeIsJavaScript) {
    mIsSubmitting = PR_FALSE;
  }

  nsAutoString target;
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::target, target)) {
    GetBaseTarget(target);
  }

  
  
  
  PRBool cancelSubmit = PR_FALSE;
  if (mNotifiedObservers) {
    cancelSubmit = mNotifiedObserversResult;
  } else {
    rv = NotifySubmitObservers(actionURI, &cancelSubmit, PR_TRUE);
    NS_ENSURE_SUBMIT_SUCCESS(rv);
  }

  if (cancelSubmit) {
    mIsSubmitting = PR_FALSE;
    return NS_OK;
  }

  cancelSubmit = PR_FALSE;
  rv = NotifySubmitObservers(actionURI, &cancelSubmit, PR_FALSE);
  NS_ENSURE_SUBMIT_SUCCESS(rv);

  if (cancelSubmit) {
    mIsSubmitting = PR_FALSE;
    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIDocShell> docShell;

  {
    nsAutoPopupStatePusher popupStatePusher(mSubmitPopupState);

    nsAutoHandlingUserInputStatePusher userInpStatePusher(mSubmitInitiatedFromUserInput, PR_FALSE);

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
    
    PRBool pending = PR_FALSE;
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
                                         PRBool* aCancelSubmit,
                                         PRBool  aEarlyNotify)
{
  
  
  if (!gFirstFormSubmitted) {
    gFirstFormSubmitted = PR_TRUE;
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
    *aCancelSubmit = PR_FALSE;

    
    
    
    nsCOMPtr<nsPIDOMWindow> window = GetOwnerDoc()->GetWindow();

    PRBool loop = PR_TRUE;
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
nsHTMLFormElement::WalkFormElements(nsFormSubmission* aFormSubmission,
                                    nsIContent* aSubmitElement)
{
  nsTArray<nsGenericHTMLFormElement*> sortedControls;
  nsresult rv = mControls->GetSortedControls(sortedControls);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  PRUint32 len = sortedControls.Length();
  for (PRUint32 i = 0; i < len; ++i) {
    
    sortedControls[i]->SubmitNamesValues(aFormSubmission, aSubmitElement);
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

  NS_ASSERTION(aControl1->GetParent() && aControl2->GetParent(),
               "Form controls should always have parents");
 
  return nsLayoutUtils::CompareTreePosition(aControl1, aControl2, aForm);
}
 
#ifdef DEBUG







static void
AssertDocumentOrder(const nsTArray<nsGenericHTMLFormElement*>& aControls,
                    nsIContent* aForm)
{
  
  
  
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
                              PRBool aNotify)
{
  NS_ASSERTION(aChild->GetParent(), "Form control should have a parent");

  
  
  PRBool childInElements = ShouldBeInElements(aChild);
  nsTArray<nsGenericHTMLFormElement*>& controlList = childInElements ?
      mControls->mElements : mControls->mNotInElements;
  
  NS_ASSERTION(controlList.IndexOf(aChild) == controlList.NoIndex,
               "Form control already in form");

  PRUint32 count = controlList.Length();
  nsGenericHTMLFormElement* element;
  
  
  PRBool lastElement = PR_FALSE;
  PRInt32 position = -1;
  if (count > 0) {
    element = controlList[count - 1];
    position = CompareFormControlPosition(aChild, element, this);
  }

  
  
  
  if (position >= 0 || count == 0) {
    
    controlList.AppendElement(aChild);
    lastElement = PR_TRUE;
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
  if (type == NS_FORM_INPUT_RADIO) {
    nsCOMPtr<nsIRadioControlElement> radio;
    CallQueryInterface(aChild, getter_AddRefs(radio));
    nsresult rv = radio->AddedToRadioGroup();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  if (!gPasswordManagerInitialized && type == NS_FORM_INPUT_PASSWORD) {
    
    gPasswordManagerInitialized = PR_TRUE;
    NS_CreateServicesFromCategory(NS_PASSWORDMANAGER_CATEGORY,
                                  nsnull,
                                  NS_PASSWORDMANAGER_CATEGORY);
  }
 
  
  if (aChild->IsSubmitControl()) {
    
    

    nsGenericHTMLFormElement** firstSubmitSlot =
      childInElements ? &mFirstSubmitInElements : &mFirstSubmitNotInElements;
    
    
    
    
    
    
    
    
    
    nsIFormControl* oldDefaultSubmit = mDefaultSubmitElement;
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

    
    
    
    
    if (aNotify && oldDefaultSubmit &&
        oldDefaultSubmit != mDefaultSubmitElement) {
      nsIDocument* document = GetCurrentDoc();
      if (document) {
        MOZ_AUTO_DOC_UPDATE(document, UPDATE_CONTENT_STATE, PR_TRUE);
        nsCOMPtr<nsIContent> oldElement(do_QueryInterface(oldDefaultSubmit));
        document->ContentStatesChanged(oldElement, nsnull,
                                       NS_EVENT_STATE_DEFAULT);
      }
    }
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
                                 PRBool aNotify) 
{
  
  
  
  nsresult rv = NS_OK;
  if (aChild->GetType() == NS_FORM_INPUT_RADIO) {
    nsCOMPtr<nsIRadioControlElement> radio;
    CallQueryInterface(aChild, getter_AddRefs(radio));
    rv = radio->WillRemoveFromRadioGroup();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  PRBool childInElements = ShouldBeInElements(aChild);
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
    nsContentUtils::AddScriptRunner(new RemoveElementRunnable(this, aNotify));

    
    
    
    
  }

  return rv;
}

void
nsHTMLFormElement::HandleDefaultSubmitRemoval(PRBool aNotify)
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

  
  if (aNotify && mDefaultSubmitElement) {
    nsIDocument* document = GetCurrentDoc();
    if (document) {
      MOZ_AUTO_DOC_UPDATE(document, UPDATE_CONTENT_STATE, PR_TRUE);
      document->ContentStatesChanged(mDefaultSubmitElement, nsnull,
                                     NS_EVENT_STATE_DEFAULT);
    }
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
  return DoResolveName(aName, PR_TRUE);
}

already_AddRefed<nsISupports>
nsHTMLFormElement::DoResolveName(const nsAString& aName,
                                 PRBool aFlushContent)
{
  nsISupports *result;
  NS_IF_ADDREF(result = mControls->NamedItemInternal(aName, aFlushContent));
  return result;
}

void
nsHTMLFormElement::OnSubmitClickBegin()
{
  mDeferSubmission = PR_TRUE;

  
  
  
  nsCOMPtr<nsIURI> actionURI;
  nsresult rv;

  rv = GetActionURL(getter_AddRefs(actionURI));
  if (NS_FAILED(rv) || !actionURI)
    return;

  
  
  
  PRBool cancelSubmit = PR_FALSE;
  rv = NotifySubmitObservers(actionURI, &cancelSubmit, PR_TRUE);
  if (NS_SUCCEEDED(rv)) {
    mNotifiedObservers = PR_TRUE;
    mNotifiedObserversResult = cancelSubmit;
  }
}

void
nsHTMLFormElement::OnSubmitClickEnd()
{
  mDeferSubmission = PR_FALSE;
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
nsHTMLFormElement::GetActionURL(nsIURI** aActionURL)
{
  nsresult rv = NS_OK;

  *aActionURL = nsnull;

  
  
  
  nsAutoString action;
  GetAttr(kNameSpaceID_None, nsGkAtoms::action, action);
  
  if (!action.IsEmpty()) {
    GetURIAttr(nsGkAtoms::action, nsnull, action);
  }

  
  
  

  
  
  
  if (!IsInDoc()) {
    return NS_OK; 
  }

  
  nsIDocument *document = GetOwnerDoc();
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

PRBool
nsHTMLFormElement::IsDefaultSubmitElement(const nsIFormControl* aControl) const
{
  NS_PRECONDITION(aControl, "Unexpected call");

  if (aControl == mDefaultSubmitElement) {
    
    return PR_TRUE;
  }

  if (mDefaultSubmitElement ||
      (aControl != mFirstSubmitInElements &&
       aControl != mFirstSubmitNotInElements)) {
    
    return PR_FALSE;
  }

  
  
  
  
  
  if (!mFirstSubmitInElements || !mFirstSubmitNotInElements) {
    
    return PR_TRUE;
  }

  
  nsIFormControl* defaultSubmit =
    CompareFormControlPosition(mFirstSubmitInElements,
                               mFirstSubmitNotInElements, this) < 0 ?
      mFirstSubmitInElements : mFirstSubmitNotInElements;
  return aControl == defaultSubmit;
}

PRBool
nsHTMLFormElement::HasSingleTextControl() const
{
  
  PRUint32 numTextControlsFound = 0;
  PRUint32 length = mControls->mElements.Length();
  for (PRUint32 i = 0; i < length && numTextControlsFound < 2; ++i) {
    if (mControls->mElements[i]->IsSingleLineTextControl(PR_FALSE)) {
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
nsHTMLFormElement::GetFormData(nsIDOMFormData** aFormData)
{
  nsRefPtr<nsFormData> fd = new nsFormData();

  nsresult rv = WalkFormElements(fd, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  *aFormData = fd.forget().get();

  return NS_OK;
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
  mNotifiedObservers = PR_FALSE;
  mIsSubmitting = PR_FALSE;
  mSubmittingRequest = nsnull;
  nsCOMPtr<nsIWebProgress> webProgress = do_QueryReferent(mWebProgress);
  if (webProgress) {
    webProgress->RemoveProgressListener(this);
  }
  mWebProgress = nsnull;
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
                                    nsIURI* location)
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
  NS_ENSURE_TRUE(mSelectedRadioButtons.Put(aName, aRadio),
                 NS_ERROR_OUT_OF_MEMORY);

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
nsHTMLFormElement::GetPositionInGroup(nsIDOMHTMLInputElement *aRadio,
                                      PRInt32 *aPositionIndex,
                                      PRInt32 *aItemsInGroup)
{
  *aPositionIndex = 0;
  *aItemsInGroup = 1;

  nsAutoString name;
  aRadio->GetName(name);
  if (name.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<nsISupports> itemWithName;
  itemWithName = ResolveName(name);
  NS_ENSURE_TRUE(itemWithName, NS_ERROR_FAILURE);
  nsCOMPtr<nsINodeList> radioGroup(do_QueryInterface(itemWithName));

  NS_ASSERTION(radioGroup, "No such radio group in this container");
  if (!radioGroup) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> currentRadioNode(do_QueryInterface(aRadio));
  NS_ASSERTION(currentRadioNode, "No nsIContent for current radio button");
  *aPositionIndex = radioGroup->IndexOf(currentRadioNode);
  NS_ASSERTION(*aPositionIndex >= 0, "Radio button not found in its own group");
  PRUint32 itemsInGroup;
  radioGroup->GetLength(&itemsInGroup);
  *aItemsInGroup = itemsInGroup;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::GetNextRadioButton(const nsAString& aName,
                                      const PRBool aPrevious,
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
  PRBool disabled = PR_TRUE;
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
                                  PRBool aFlushContent)
{
  nsresult rv = NS_OK;

  PRBool stopIterating = PR_FALSE;

  if (aName.IsEmpty()) {
    
    
    
    
    nsCOMPtr<nsIFormControl> control;
    PRUint32 len = GetElementCount();
    for (PRUint32 i=0; i<len; i++) {
      control = GetElementAt(i);
      if (control->GetType() == NS_FORM_INPUT_RADIO) {
        nsCOMPtr<nsIContent> controlContent(do_QueryInterface(control));
        if (controlContent) {
          if (controlContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                                          EmptyString(), eCaseMatters)) {
            aVisitor->Visit(control, &stopIterating);
            if (stopIterating) {
              break;
            }
          }
        }
      }
    }
  } else {
    
    
    
    nsCOMPtr<nsISupports> item;
    item = DoResolveName(aName, aFlushContent);
    rv = item ? NS_OK : NS_ERROR_FAILURE;

    if (item) {
      
      
      
      nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(item));
      if (formControl) {
        if (formControl->GetType() == NS_FORM_INPUT_RADIO) {
          aVisitor->Visit(formControl, &stopIterating);
        }
      } else {
        nsCOMPtr<nsIDOMNodeList> nodeList(do_QueryInterface(item));
        if (nodeList) {
          PRUint32 length = 0;
          nodeList->GetLength(&length);
          for (PRUint32 i=0; i<length; i++) {
            nsCOMPtr<nsIDOMNode> node;
            nodeList->Item(i, getter_AddRefs(node));
            nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(node));
            if (formControl) {
              if (formControl->GetType() == NS_FORM_INPUT_RADIO) {
                aVisitor->Visit(formControl, &stopIterating);
                if (stopIterating) {
                  break;
                }
              }
            }
          }
        }
      }
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLFormElement::AddToRadioGroup(const nsAString& aName,
                                   nsIFormControl* aRadio)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElement::RemoveFromRadioGroup(const nsAString& aName,
                                        nsIFormControl* aRadio)
{
  return NS_OK;
}





nsFormControlList::nsFormControlList(nsHTMLFormElement* aForm) :
  mForm(aForm),
  
  
  mElements(8)
{
}

nsFormControlList::~nsFormControlList()
{
  mForm = nsnull;
  Clear();
}

nsresult nsFormControlList::Init()
{
  NS_ENSURE_TRUE(
    mNameLookupTable.Init(NS_FORM_CONTROL_LIST_HASHTABLE_SIZE),
    NS_ERROR_OUT_OF_MEMORY);

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
  
  PRInt32 i;
  for (i = mElements.Length()-1; i >= 0; i--) {
    mElements[i]->ClearForm(PR_FALSE, PR_TRUE);
  }
  mElements.Clear();

  for (i = mNotInElements.Length()-1; i >= 0; i--) {
    mNotInElements[i]->ClearForm(PR_FALSE, PR_TRUE);
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
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFormControlList)
  tmp->mNameLookupTable.EnumerateRead(ControlTraverser, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(HTMLCollection, nsFormControlList)


NS_INTERFACE_TABLE_HEAD(nsFormControlList)
  NS_INTERFACE_TABLE2(nsFormControlList,
                      nsIHTMLCollection,
                      nsIDOMHTMLCollection)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsFormControlList)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLCollection)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsFormControlList, nsIHTMLCollection)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsFormControlList, nsIHTMLCollection)




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
  nsresult rv;
  nsISupports* item = GetNodeAt(aIndex, &rv);
  if (!item) {
    *aReturn = nsnull;

    return rv;
  }

  return CallQueryInterface(item, aReturn);
}

NS_IMETHODIMP 
nsFormControlList::NamedItem(const nsAString& aName,
                             nsIDOMNode** aReturn)
{
  FlushPendingNotifications();

  *aReturn = nsnull;

  nsresult rv = NS_OK;

  nsCOMPtr<nsISupports> supports;
  
  if (!mNameLookupTable.Get(aName, getter_AddRefs(supports))) 
     return rv;

  if (supports) {
    
    CallQueryInterface(supports, aReturn);

    if (!*aReturn) {
      
      nsCOMPtr<nsIDOMNodeList> nodeList(do_QueryInterface(supports));
      NS_ASSERTION(nodeList, "Huh, what's going one here?");

      if (nodeList) {
        
        
        rv = nodeList->Item(0, aReturn);
      }
    }
  }

  return rv;
}

nsISupports*
nsFormControlList::NamedItemInternal(const nsAString& aName,
                                     PRBool aFlushContent)
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
    
    NS_ENSURE_TRUE( mNameLookupTable.Put(aName,
                                         NS_ISUPPORTS_CAST(nsIContent*, aChild)),
                    NS_ERROR_FAILURE );
  } else {
    
    nsCOMPtr<nsIContent> content(do_QueryInterface(supports));

    if (content) {
      
      
      
      
      if (content == aChild) {
        return NS_OK;
      }

      
      
      nsBaseContentList *list = new nsBaseContentList();
      NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

      NS_ASSERTION(content->GetParent(), "Item in list without parent");

      
      PRBool newFirst = nsContentUtils::PositionIsBefore(aChild, content);

      list->AppendElement(newFirst ? aChild : content);
      list->AppendElement(newFirst ? content : aChild);


      nsCOMPtr<nsISupports> listSupports =
        do_QueryInterface(static_cast<nsIDOMNodeList*>(list));

      
      NS_ENSURE_TRUE(mNameLookupTable.Put(aName, listSupports),
                     NS_ERROR_FAILURE);
    } else {
      
      nsCOMPtr<nsIDOMNodeList> nodeList(do_QueryInterface(supports));
      NS_ENSURE_TRUE(nodeList, NS_ERROR_FAILURE);

      
      nsBaseContentList *list = static_cast<nsBaseContentList *>
                                           ((nsIDOMNodeList *)nodeList.get());

      NS_ASSERTION(list->Length() > 1,
                   "List should have been converted back to a single element");

      
      
      
      
      if(nsContentUtils::PositionIsBefore(list->GetNodeAt(list->Length() - 1), aChild)) {
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

  
  nsBaseContentList *list = static_cast<nsBaseContentList *>
                                       ((nsIDOMNodeList *)nodeList.get());

  list->RemoveElement(aChild);

  PRUint32 length = 0;
  list->GetLength(&length);

  if (!length) {
    
    
    mNameLookupTable.Remove(aName);
  } else if (length == 1) {
    
    
    nsIContent* node = list->GetNodeAt(0);
    if (node) {
      NS_ENSURE_TRUE(mNameLookupTable.Put(aName, node),NS_ERROR_FAILURE);
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
