






































#include "nsHTMLSelectElement.h"

#include "nsHTMLOptionElement.h"
#include "nsIDOMEventTarget.h"
#include "nsContentCreatorFunctions.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsLayoutUtils.h"
#include "nsMappedAttributes.h"
#include "nsIForm.h"
#include "nsFormSubmission.h"
#include "nsIFormProcessor.h"
#include "nsContentCreatorFunctions.h"

#include "nsIDOMHTMLOptGroupElement.h"
#include "nsEventStates.h"
#include "nsGUIEvent.h"
#include "nsIPrivateDOMEvent.h"


#include "nsIDocument.h"
#include "nsIFormControlFrame.h"
#include "nsIComboboxControlFrame.h"
#include "nsIListControlFrame.h"
#include "nsIFrame.h"

#include "nsDOMError.h"
#include "nsServiceManagerUtils.h"
#include "nsRuleData.h"
#include "nsEventDispatcher.h"
#include "mozilla/dom/Element.h"
#include "mozAutoDocUpdate.h"

using namespace mozilla::dom;

NS_IMPL_ISUPPORTS1(nsSelectState, nsSelectState)
NS_DEFINE_STATIC_IID_ACCESSOR(nsSelectState, NS_SELECT_STATE_IID)






nsSafeOptionListMutation::nsSafeOptionListMutation(nsIContent* aSelect,
                                                   nsIContent* aParent,
                                                   nsIContent* aKid,
                                                   PRUint32 aIndex,
                                                   PRBool aNotify)
  : mSelect(nsHTMLSelectElement::FromContent(aSelect))
  , mTopLevelMutation(PR_FALSE)
  , mNeedsRebuild(PR_FALSE)
{
  if (mSelect) {
    mTopLevelMutation = !mSelect->mMutating;
    if (mTopLevelMutation) {
      mSelect->mMutating = PR_TRUE;
    } else {
      
      
      
      
      mSelect->RebuildOptionsArray(aNotify);
    }
    nsresult rv;
    if (aKid) {
      rv = mSelect->WillAddOptions(aKid, aParent, aIndex, aNotify);
    } else {
      rv = mSelect->WillRemoveOptions(aParent, aIndex, aNotify);
    }
    mNeedsRebuild = NS_FAILED(rv);
  }
}

nsSafeOptionListMutation::~nsSafeOptionListMutation()
{
  if (mSelect) {
    if (mNeedsRebuild || (mTopLevelMutation && mGuard.Mutated(1))) {
      mSelect->RebuildOptionsArray(PR_TRUE);
    }
    if (mTopLevelMutation) {
      mSelect->mMutating = PR_FALSE;
    }
#ifdef DEBUG
    mSelect->VerifyOptionsArray();
#endif
  }
}









NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Select)

nsHTMLSelectElement::nsHTMLSelectElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                         FromParser aFromParser)
  : nsGenericHTMLFormElement(aNodeInfo),
    mOptions(new nsHTMLOptionCollection(this)),
    mIsDoneAddingChildren(!aFromParser),
    mDisabledChanged(PR_FALSE),
    mMutating(PR_FALSE),
    mInhibitStateRestoration(!!(aFromParser & FROM_PARSER_FRAGMENT)),
    mSelectionHasChanged(PR_FALSE),
    mDefaultSelectionSet(PR_FALSE),
    mCanShowInvalidUI(PR_TRUE),
    mCanShowValidUI(PR_TRUE),
    mNonOptionChildren(0),
    mOptGroupCount(0),
    mSelectedIndex(-1)
{
  
  

  
  
}

nsHTMLSelectElement::~nsHTMLSelectElement()
{
  if (mOptions) {
    mOptions->DropReference();
  }
}



NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLSelectElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLSelectElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mOptions,
                                                       nsIDOMHTMLCollection)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLSelectElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLSelectElement, nsGenericElement)


DOMCI_NODE_DATA(HTMLSelectElement, nsHTMLSelectElement)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLSelectElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLSelectElement,
                                   nsIDOMHTMLSelectElement,
                                   nsIConstraintValidation)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLSelectElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLSelectElement)





NS_IMPL_ELEMENT_CLONE(nsHTMLSelectElement)


NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(nsHTMLSelectElement)

NS_IMETHODIMP
nsHTMLSelectElement::SetCustomValidity(const nsAString& aError)
{
  nsIConstraintValidation::SetCustomValidity(aError);

  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    nsAutoScriptBlocker scriptBlocker;
    doc->ContentStateChanged(this, NS_EVENT_STATE_INVALID |
                                   NS_EVENT_STATE_VALID |
                                   NS_EVENT_STATE_MOZ_UI_INVALID |
                                   NS_EVENT_STATE_MOZ_UI_VALID);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSelectElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

nsresult
nsHTMLSelectElement::InsertChildAt(nsIContent* aKid,
                                   PRUint32 aIndex,
                                   PRBool aNotify)
{
  nsSafeOptionListMutation safeMutation(this, this, aKid, aIndex, aNotify);
  nsresult rv = nsGenericHTMLFormElement::InsertChildAt(aKid, aIndex, aNotify);
  if (NS_FAILED(rv)) {
    safeMutation.MutationFailed();
  }
  return rv;
}

nsresult
nsHTMLSelectElement::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
  nsSafeOptionListMutation safeMutation(this, this, nsnull, aIndex, aNotify);
  nsresult rv = nsGenericHTMLFormElement::RemoveChildAt(aIndex, aNotify);
  if (NS_FAILED(rv)) {
    safeMutation.MutationFailed();
  }
  return rv;
}




nsresult
nsHTMLSelectElement::InsertOptionsIntoList(nsIContent* aOptions,
                                           PRInt32 aListIndex,
                                           PRInt32 aDepth,
                                           PRBool aNotify)
{
  PRInt32 insertIndex = aListIndex;
  nsresult rv = InsertOptionsIntoListRecurse(aOptions, &insertIndex, aDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (insertIndex - aListIndex) {
    
    if (aListIndex <= mSelectedIndex) {
      mSelectedIndex += (insertIndex - aListIndex);
      SetSelectionChanged(PR_TRUE, aNotify);
    }

    
    
    
    nsISelectControlFrame* selectFrame = nsnull;
    nsWeakFrame weakSelectFrame;
    PRBool didGetFrame = PR_FALSE;

    
    nsCOMPtr<nsIDOMNode> optionNode;
    nsCOMPtr<nsIDOMHTMLOptionElement> option;
    for (PRInt32 i=aListIndex;i<insertIndex;i++) {
      
      if (!didGetFrame || (selectFrame && !weakSelectFrame.IsAlive())) {
        selectFrame = GetSelectFrame();
        weakSelectFrame = do_QueryFrame(selectFrame);
        didGetFrame = PR_TRUE;
      }

      if (selectFrame) {
        selectFrame->AddOption(i);
      }

      Item(i, getter_AddRefs(optionNode));
      option = do_QueryInterface(optionNode);
      if (option) {
        PRBool selected;
        option->GetSelected(&selected);
        if (selected) {
          
          if (!HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)) {
            SetOptionsSelectedByIndex(i, i, PR_TRUE, PR_TRUE, PR_TRUE, PR_TRUE, nsnull);
          }

          
          
          
          OnOptionSelected(selectFrame, i, PR_TRUE, PR_FALSE, PR_FALSE);
        }
      }
    }

    CheckSelectSomething(aNotify);
  }

  return NS_OK;
}

nsresult
nsHTMLSelectElement::RemoveOptionsFromList(nsIContent* aOptions,
                                           PRInt32 aListIndex,
                                           PRInt32 aDepth,
                                           PRBool aNotify)
{
  PRInt32 numRemoved = 0;
  nsresult rv = RemoveOptionsFromListRecurse(aOptions, aListIndex, &numRemoved,
                                             aDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  if (numRemoved) {
    
    nsISelectControlFrame* selectFrame = GetSelectFrame();
    if (selectFrame) {
      nsAutoScriptBlocker scriptBlocker;
      for (int i = aListIndex; i < aListIndex + numRemoved; ++i) {
        selectFrame->RemoveOption(i);
      }
    }

    
    if (aListIndex <= mSelectedIndex) {
      if (mSelectedIndex < (aListIndex+numRemoved)) {
        
        
        FindSelectedIndex(aListIndex, aNotify);
      } else {
        
        
        mSelectedIndex -= numRemoved;
        SetSelectionChanged(PR_TRUE, aNotify);
      }
    }

    
    
    if (!CheckSelectSomething(aNotify) && mSelectedIndex == -1) {
      
      
      UpdateValueMissingValidityState();

      if (aNotify) {
        nsIDocument* doc = GetCurrentDoc();
        if (doc) {
          nsAutoScriptBlocker scriptBlocker;
          doc->ContentStateChanged(this, NS_EVENT_STATE_VALID |
                                         NS_EVENT_STATE_INVALID |
                                         NS_EVENT_STATE_MOZ_UI_INVALID |
                                         NS_EVENT_STATE_MOZ_UI_VALID);
        }
      }
    }
  }

  return NS_OK;
}

static PRBool IsOptGroup(nsIContent *aContent)
{
  return (aContent->NodeInfo()->Equals(nsGkAtoms::optgroup) &&
          aContent->IsHTML());
}




nsresult
nsHTMLSelectElement::InsertOptionsIntoListRecurse(nsIContent* aOptions,
                                                  PRInt32* aInsertIndex,
                                                  PRInt32 aDepth)
{
  
  
  
  

  nsHTMLOptionElement *optElement = nsHTMLOptionElement::FromContent(aOptions);
  if (optElement) {
    nsresult rv = mOptions->InsertOptionAt(optElement, *aInsertIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    (*aInsertIndex)++;
    return NS_OK;
  }

  
  
  if (aDepth == 0) {
    mNonOptionChildren++;
  }

  
  if (IsOptGroup(aOptions)) {
    mOptGroupCount++;

    PRUint32 numChildren = aOptions->GetChildCount();
    for (PRUint32 i = 0; i < numChildren; ++i) {
      nsresult rv = InsertOptionsIntoListRecurse(aOptions->GetChildAt(i),
                                                 aInsertIndex, aDepth+1);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}



nsresult
nsHTMLSelectElement::RemoveOptionsFromListRecurse(nsIContent* aOptions,
                                                  PRInt32 aRemoveIndex,
                                                  PRInt32* aNumRemoved,
                                                  PRInt32 aDepth)
{
  
  
  
  

  nsCOMPtr<nsIDOMHTMLOptionElement> optElement(do_QueryInterface(aOptions));
  if (optElement) {
    if (mOptions->ItemAsOption(aRemoveIndex) != optElement) {
      NS_ERROR("wrong option at index");
      return NS_ERROR_UNEXPECTED;
    }
    mOptions->RemoveOptionAt(aRemoveIndex);
    (*aNumRemoved)++;
    return NS_OK;
  }

  
  if (aDepth == 0) {
    mNonOptionChildren--;
  }

  
  if (mOptGroupCount && IsOptGroup(aOptions)) {
    mOptGroupCount--;

    PRUint32 numChildren = aOptions->GetChildCount();
    for (PRUint32 i = 0; i < numChildren; ++i) {
      nsresult rv = RemoveOptionsFromListRecurse(aOptions->GetChildAt(i),
                                                 aRemoveIndex,
                                                 aNumRemoved,
                                                 aDepth + 1);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}






NS_IMETHODIMP
nsHTMLSelectElement::WillAddOptions(nsIContent* aOptions,
                                    nsIContent* aParent,
                                    PRInt32 aContentIndex,
                                    PRBool aNotify)
{
  PRInt32 level = GetContentDepth(aParent);
  if (level == -1) {
    return NS_ERROR_FAILURE;
  }

  
  PRInt32 ind = -1;
  if (!mNonOptionChildren) {
    
    ind = aContentIndex;
  } else {
    
    
    PRInt32 children = aParent->GetChildCount();

    if (aContentIndex >= children) {
      
      
      ind = GetOptionIndexAfter(aParent);
    } else {
      
      
      
      nsIContent *currentKid = aParent->GetChildAt(aContentIndex);
      NS_ASSERTION(currentKid, "Child not found!");
      if (currentKid) {
        ind = GetOptionIndexAt(currentKid);
      } else {
        ind = -1;
      }
    }
  }

  return InsertOptionsIntoList(aOptions, ind, level, aNotify);
}

NS_IMETHODIMP
nsHTMLSelectElement::WillRemoveOptions(nsIContent* aParent,
                                       PRInt32 aContentIndex,
                                       PRBool aNotify)
{
  PRInt32 level = GetContentDepth(aParent);
  NS_ASSERTION(level >= 0, "getting notified by unexpected content");
  if (level == -1) {
    return NS_ERROR_FAILURE;
  }

  
  nsIContent *currentKid = aParent->GetChildAt(aContentIndex);
  if (currentKid) {
    PRInt32 ind;
    if (!mNonOptionChildren) {
      
      ind = aContentIndex;
    } else {
      
      
      ind = GetFirstOptionIndex(currentKid);
    }
    if (ind != -1) {
      nsresult rv = RemoveOptionsFromList(currentKid, ind, level, aNotify);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

PRInt32
nsHTMLSelectElement::GetContentDepth(nsIContent* aContent)
{
  nsIContent* content = aContent;

  PRInt32 retval = 0;
  while (content != this) {
    retval++;
    content = content->GetParent();
    if (!content) {
      retval = -1;
      break;
    }
  }

  return retval;
}

PRInt32
nsHTMLSelectElement::GetOptionIndexAt(nsIContent* aOptions)
{
  
  
  PRInt32 retval = GetFirstOptionIndex(aOptions);
  if (retval == -1) {
    retval = GetOptionIndexAfter(aOptions);
  }

  return retval;
}

PRInt32
nsHTMLSelectElement::GetOptionIndexAfter(nsIContent* aOptions)
{
  
  
  
  
  if (aOptions == this) {
    PRUint32 len;
    GetLength(&len);
    return len;
  }

  PRInt32 retval = -1;

  nsCOMPtr<nsIContent> parent = aOptions->GetParent();

  if (parent) {
    PRInt32 index = parent->IndexOf(aOptions);
    PRInt32 count = parent->GetChildCount();

    retval = GetFirstChildOptionIndex(parent, index+1, count);

    if (retval == -1) {
      retval = GetOptionIndexAfter(parent);
    }
  }

  return retval;
}

PRInt32
nsHTMLSelectElement::GetFirstOptionIndex(nsIContent* aOptions)
{
  PRInt32 listIndex = -1;
  nsHTMLOptionElement *optElement = nsHTMLOptionElement::FromContent(aOptions);
  if (optElement) {
    GetOptionIndex(optElement, 0, PR_TRUE, &listIndex);
    
    
    return listIndex;
  }

  listIndex = GetFirstChildOptionIndex(aOptions, 0, aOptions->GetChildCount());

  return listIndex;
}

PRInt32
nsHTMLSelectElement::GetFirstChildOptionIndex(nsIContent* aOptions,
                                              PRInt32 aStartIndex,
                                              PRInt32 aEndIndex)
{
  PRInt32 retval = -1;

  for (PRInt32 i = aStartIndex; i < aEndIndex; ++i) {
    retval = GetFirstOptionIndex(aOptions->GetChildAt(i));
    if (retval != -1) {
      break;
    }
  }

  return retval;
}

nsISelectControlFrame *
nsHTMLSelectElement::GetSelectFrame()
{
  nsIFormControlFrame* form_control_frame = GetFormControlFrame(PR_FALSE);

  nsISelectControlFrame *select_frame = nsnull;

  if (form_control_frame) {
    select_frame = do_QueryFrame(form_control_frame);
  }

  return select_frame;
}

NS_IMETHODIMP
nsHTMLSelectElement::Add(nsIDOMHTMLElement* aElement,
                         nsIDOMHTMLElement* aBefore)
{
  nsCOMPtr<nsIDOMNode> added;
  if (!aBefore) {
    return AppendChild(aElement, getter_AddRefs(added));
  }

  
  
  nsCOMPtr<nsIDOMNode> parent;
  aBefore->GetParentNode(getter_AddRefs(parent));
  if (!parent) {
    
    
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  nsCOMPtr<nsIDOMNode> ancestor(parent);
  nsCOMPtr<nsIDOMNode> temp;
  while (ancestor != static_cast<nsIDOMNode*>(this)) {
    ancestor->GetParentNode(getter_AddRefs(temp));
    if (!temp) {
      
      
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }
    temp.swap(ancestor);
  }

  
  
  return parent->InsertBefore(aElement, aBefore, getter_AddRefs(added));
}

NS_IMETHODIMP
nsHTMLSelectElement::Remove(PRInt32 aIndex)
{
  nsCOMPtr<nsIDOMNode> option;
  Item(aIndex, getter_AddRefs(option));

  if (option) {
    nsCOMPtr<nsIDOMNode> parent;

    option->GetParentNode(getter_AddRefs(parent));
    if (parent) {
      nsCOMPtr<nsIDOMNode> ret;
      parent->RemoveChild(option, getter_AddRefs(ret));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSelectElement::GetOptions(nsIDOMHTMLOptionsCollection** aValue)
{
  NS_IF_ADDREF(*aValue = GetOptions());

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSelectElement::GetType(nsAString& aType)
{
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)) {
    aType.AssignLiteral("select-multiple");
  }
  else {
    aType.AssignLiteral("select-one");
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSelectElement::GetLength(PRUint32* aLength)
{
  return mOptions->GetLength(aLength);
}

#define MAX_DYNAMIC_SELECT_LENGTH 10000

NS_IMETHODIMP
nsHTMLSelectElement::SetLength(PRUint32 aLength)
{
  nsresult rv=NS_OK;

  PRUint32 curlen;
  PRUint32 i;

  rv = GetLength(&curlen);
  if (NS_FAILED(rv)) {
    curlen = 0;
  }

  if (curlen > aLength) { 
    for (i = curlen; i > aLength && NS_SUCCEEDED(rv); --i) {
      rv = Remove(i-1);
    }
  } else if (aLength > curlen) {
    if (aLength > MAX_DYNAMIC_SELECT_LENGTH) {
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
    
    
    nsCOMPtr<nsINodeInfo> nodeInfo;

    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::option,
                                getter_AddRefs(nodeInfo));

    nsCOMPtr<nsIContent> element = NS_NewHTMLOptionElement(nodeInfo.forget());
    if (!element) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIContent> text;
    rv = NS_NewTextNode(getter_AddRefs(text), mNodeInfo->NodeInfoManager());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = element->AppendChildTo(text, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(element));

    for (i = curlen; i < aLength; i++) {
      nsCOMPtr<nsIDOMNode> tmpNode;

      rv = AppendChild(node, getter_AddRefs(tmpNode));
      NS_ENSURE_SUCCESS(rv, rv);

      if (i + 1 < aLength) {
        nsCOMPtr<nsIDOMNode> newNode;

        rv = node->CloneNode(PR_TRUE, getter_AddRefs(newNode));
        NS_ENSURE_SUCCESS(rv, rv);

        node = newNode;
      }
    }
  }

  return NS_OK;
}



NS_IMETHODIMP
nsHTMLSelectElement::GetSelectedIndex(PRInt32* aValue)
{
  *aValue = mSelectedIndex;

  return NS_OK;
}

nsresult
nsHTMLSelectElement::SetSelectedIndexInternal(PRInt32 aIndex, PRBool aNotify)
{
  PRInt32 oldSelectedIndex = mSelectedIndex;

  nsresult rv = SetOptionsSelectedByIndex(aIndex, aIndex, PR_TRUE,
                                          PR_TRUE, PR_TRUE, aNotify, nsnull);

  if (NS_SUCCEEDED(rv)) {
    nsISelectControlFrame* selectFrame = GetSelectFrame();
    if (selectFrame) {
      rv = selectFrame->OnSetSelectedIndex(oldSelectedIndex, mSelectedIndex);
    }
  }

  SetSelectionChanged(PR_TRUE, aNotify);

  return rv;
}

NS_IMETHODIMP
nsHTMLSelectElement::SetSelectedIndex(PRInt32 aIndex)
{
  return SetSelectedIndexInternal(aIndex, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLSelectElement::GetOptionIndex(nsIDOMHTMLOptionElement* aOption,
                                    PRInt32 aStartIndex, PRBool aForward,
                                    PRInt32* aIndex)
{
  nsCOMPtr<nsINode> option = do_QueryInterface(aOption);
  return mOptions->GetOptionIndex(option->AsElement(), aStartIndex, aForward, aIndex);
}

PRBool
nsHTMLSelectElement::IsOptionSelectedByIndex(PRInt32 aIndex)
{
  nsIDOMHTMLOptionElement *option = mOptions->ItemAsOption(aIndex);
  PRBool isSelected = PR_FALSE;
  if (option) {
    option->GetSelected(&isSelected);
  }
  return isSelected;
}

void
nsHTMLSelectElement::OnOptionSelected(nsISelectControlFrame* aSelectFrame,
                                      PRInt32 aIndex,
                                      PRBool aSelected,
                                      PRBool aChangeOptionState,
                                      PRBool aNotify)
{
  
  if (aSelected && (aIndex < mSelectedIndex || mSelectedIndex < 0)) {
    mSelectedIndex = aIndex;
    SetSelectionChanged(PR_TRUE, aNotify);
  } else if (!aSelected && aIndex == mSelectedIndex) {
    FindSelectedIndex(aIndex+1, aNotify);
  }

  if (aChangeOptionState) {
    
    nsCOMPtr<nsIDOMNode> option;
    Item(aIndex, getter_AddRefs(option));
    if (option) {
      nsRefPtr<nsHTMLOptionElement> optionElement = 
        static_cast<nsHTMLOptionElement*>(option.get());
      optionElement->SetSelectedInternal(aSelected, aNotify);
    }
  }

  
  if (aSelectFrame) {
    aSelectFrame->OnOptionSelected(aIndex, aSelected);
  }

  UpdateValueMissingValidityState();
  if (aNotify) {
    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      nsAutoScriptBlocker scriptBlocker;
      doc->ContentStateChanged(this, NS_EVENT_STATE_VALID |
                                     NS_EVENT_STATE_INVALID |
                                     NS_EVENT_STATE_MOZ_UI_INVALID |
                                     NS_EVENT_STATE_MOZ_UI_VALID);
    }
  }
}

void
nsHTMLSelectElement::FindSelectedIndex(PRInt32 aStartIndex, PRBool aNotify)
{
  mSelectedIndex = -1;
  SetSelectionChanged(PR_TRUE, aNotify);
  PRUint32 len;
  GetLength(&len);
  for (PRInt32 i=aStartIndex; i<(PRInt32)len; i++) {
    if (IsOptionSelectedByIndex(i)) {
      mSelectedIndex = i;
      SetSelectionChanged(PR_TRUE, aNotify);
      break;
    }
  }
}

























NS_IMETHODIMP
nsHTMLSelectElement::SetOptionsSelectedByIndex(PRInt32 aStartIndex,
                                               PRInt32 aEndIndex,
                                               PRBool aIsSelected,
                                               PRBool aClearAll,
                                               PRBool aSetDisabled,
                                               PRBool aNotify,
                                               PRBool* aChangedSomething)
{
#if 0
  printf("SetOption(%d-%d, %c, ClearAll=%c)\n", aStartIndex, aEndIndex,
                                       (aIsSelected ? 'Y' : 'N'),
                                       (aClearAll ? 'Y' : 'N'));
#endif
  if (aChangedSomething) {
    *aChangedSomething = PR_FALSE;
  }

  
  if (!aSetDisabled && IsDisabled()) {
    return NS_OK;
  }

  
  PRUint32 numItems = 0;
  GetLength(&numItems);
  if (numItems == 0) {
    return NS_OK;
  }

  
  PRBool isMultiple = HasAttr(kNameSpaceID_None, nsGkAtoms::multiple);

  
  
  PRBool optionsSelected = PR_FALSE;
  PRBool optionsDeselected = PR_FALSE;

  nsISelectControlFrame *selectFrame = nsnull;
  PRBool didGetFrame = PR_FALSE;
  nsWeakFrame weakSelectFrame;

  if (aIsSelected) {
    
    if (aStartIndex >= (PRInt32)numItems || aStartIndex < 0 ||
        aEndIndex >= (PRInt32)numItems || aEndIndex < 0) {
      aStartIndex = -1;
      aEndIndex = -1;
    }

    
    if (!isMultiple) {
      aEndIndex = aStartIndex;
    }

    
    
    
    
    PRBool allDisabled = !aSetDisabled;

    
    
    
    PRInt32 previousSelectedIndex = mSelectedIndex;

    
    
    
    
    if (aStartIndex != -1) {
      
      
      for (PRInt32 optIndex = aStartIndex; optIndex <= aEndIndex; optIndex++) {

        
        if (!aSetDisabled) {
          PRBool isDisabled;
          IsOptionDisabled(optIndex, &isDisabled);

          if (isDisabled) {
            continue;
          } else {
            allDisabled = PR_FALSE;
          }
        }

        nsIDOMHTMLOptionElement *option = mOptions->ItemAsOption(optIndex);
        if (option) {
          
          PRBool isSelected = PR_FALSE;
          option->GetSelected(&isSelected);
          if (!isSelected) {
            
            
            
            
            selectFrame = GetSelectFrame();
            weakSelectFrame = do_QueryFrame(selectFrame);
            didGetFrame = PR_TRUE;

            OnOptionSelected(selectFrame, optIndex, PR_TRUE, PR_TRUE, aNotify);
            optionsSelected = PR_TRUE;
          }
        }
      }
    }

    
    
    if (((!isMultiple && optionsSelected)
       || (aClearAll && !allDisabled)
       || aStartIndex == -1)
       && previousSelectedIndex != -1) {
      for (PRInt32 optIndex = previousSelectedIndex;
           optIndex < (PRInt32)numItems;
           optIndex++) {
        if (optIndex < aStartIndex || optIndex > aEndIndex) {
          nsIDOMHTMLOptionElement *option = mOptions->ItemAsOption(optIndex);
          if (option) {
            
            PRBool isSelected = PR_FALSE;
            option->GetSelected(&isSelected);
            if (isSelected) {
              if (!didGetFrame || (selectFrame && !weakSelectFrame.IsAlive())) {
                
                
                
                selectFrame = GetSelectFrame();
                weakSelectFrame = do_QueryFrame(selectFrame);

                didGetFrame = PR_TRUE;
              }

              OnOptionSelected(selectFrame, optIndex, PR_FALSE, PR_TRUE,
                               aNotify);
              optionsDeselected = PR_TRUE;

              
              if (!isMultiple) {
                break;
              }
            }
          }
        }
      }
    }

  } else {

    
    
    for (PRInt32 optIndex = aStartIndex; optIndex <= aEndIndex; optIndex++) {
      if (!aSetDisabled) {
        PRBool isDisabled;
        IsOptionDisabled(optIndex, &isDisabled);
        if (isDisabled) {
          continue;
        }
      }

      nsIDOMHTMLOptionElement *option = mOptions->ItemAsOption(optIndex);
      if (option) {
        
        PRBool isSelected = PR_FALSE;
        option->GetSelected(&isSelected);
        if (isSelected) {
          if (!didGetFrame || (selectFrame && !weakSelectFrame.IsAlive())) {
            
            
            
            selectFrame = GetSelectFrame();
            weakSelectFrame = do_QueryFrame(selectFrame);

            didGetFrame = PR_TRUE;
          }

          OnOptionSelected(selectFrame, optIndex, PR_FALSE, PR_TRUE, aNotify);
          optionsDeselected = PR_TRUE;
        }
      }
    }
  }

  
  if (optionsDeselected && aStartIndex != -1) {
    optionsSelected = CheckSelectSomething(aNotify) || optionsSelected;
  }

  
  if (optionsSelected || optionsDeselected) {
    if (aChangedSomething)
      *aChangedSomething = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSelectElement::IsOptionDisabled(PRInt32 aIndex, PRBool* aIsDisabled)
{
  *aIsDisabled = PR_FALSE;
  nsCOMPtr<nsIDOMNode> optionNode;
  Item(aIndex, getter_AddRefs(optionNode));
  NS_ENSURE_TRUE(optionNode, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMHTMLOptionElement> option = do_QueryInterface(optionNode);
  if (option) {
    PRBool isDisabled;
    option->GetDisabled(&isDisabled);
    if (isDisabled) {
      *aIsDisabled = PR_TRUE;
      return NS_OK;
    }
  }

  
  
  if (mNonOptionChildren) {
    nsCOMPtr<nsIDOMNode> parent;
    while (1) {
      optionNode->GetParentNode(getter_AddRefs(parent));

      
      if (!parent) {
        break;
      }

      
      nsCOMPtr<nsIDOMHTMLSelectElement> selectElement =
        do_QueryInterface(parent);
      if (selectElement) {
        break;
      }

      nsCOMPtr<nsIDOMHTMLOptGroupElement> optGroupElement =
        do_QueryInterface(parent);

      if (optGroupElement) {
        PRBool isDisabled;
        optGroupElement->GetDisabled(&isDisabled);

        if (isDisabled) {
          *aIsDisabled = PR_TRUE;
          return NS_OK;
        }
      } else {
        
        
        break;
      }

      optionNode = parent;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSelectElement::GetValue(nsAString& aValue)
{
  PRInt32 selectedIndex;

  nsresult rv = GetSelectedIndex(&selectedIndex);

  if (NS_SUCCEEDED(rv) && selectedIndex > -1) {
    nsCOMPtr<nsIDOMNode> node;

    rv = Item(selectedIndex, getter_AddRefs(node));

    nsCOMPtr<nsIDOMHTMLOptionElement> option = do_QueryInterface(node);
    if (NS_SUCCEEDED(rv) && option) {
      return option->GetValue(aValue);
    }
  }

  aValue.Truncate(0);
  return rv;
}

NS_IMETHODIMP
nsHTMLSelectElement::SetValue(const nsAString& aValue)
{
  nsresult rv = NS_OK;

  PRUint32 length;
  rv = GetLength(&length);
  if (NS_SUCCEEDED(rv)) {
    PRUint32 i;
    for (i = 0; i < length; i++) {
      nsCOMPtr<nsIDOMNode> node;

      rv = Item(i, getter_AddRefs(node));

      if (NS_SUCCEEDED(rv) && node) {
        nsCOMPtr<nsIDOMHTMLOptionElement> option = do_QueryInterface(node);

        if (option) {
          nsAutoString optionVal;

          option->GetValue(optionVal);

          if (optionVal.Equals(aValue)) {
            SetSelectedIndexInternal((PRInt32)i, PR_TRUE);

            break;
          }
        }
      }
    }
  }

  return rv;
}


NS_IMPL_BOOL_ATTR(nsHTMLSelectElement, Autofocus, autofocus)
NS_IMPL_BOOL_ATTR(nsHTMLSelectElement, Disabled, disabled)
NS_IMPL_BOOL_ATTR(nsHTMLSelectElement, Multiple, multiple)
NS_IMPL_STRING_ATTR(nsHTMLSelectElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLSelectElement, Required, required)
NS_IMPL_NON_NEGATIVE_INT_ATTR_DEFAULT_VALUE(nsHTMLSelectElement, Size, size, 0)
NS_IMPL_INT_ATTR(nsHTMLSelectElement, TabIndex, tabindex)

PRBool
nsHTMLSelectElement::IsHTMLFocusable(PRBool aWithMouse,
                                     PRBool *aIsFocusable, PRInt32 *aTabIndex)
{
  if (nsGenericHTMLFormElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
    return PR_TRUE;
  }

  *aIsFocusable = !IsDisabled();

  return PR_FALSE;
}

NS_IMETHODIMP
nsHTMLSelectElement::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  return mOptions->Item(aIndex, aReturn);
}

NS_IMETHODIMP
nsHTMLSelectElement::NamedItem(const nsAString& aName,
                               nsIDOMNode** aReturn)
{
  return mOptions->NamedItem(aName, aReturn);
}

PRBool
nsHTMLSelectElement::CheckSelectSomething(PRBool aNotify)
{
  if (mIsDoneAddingChildren) {
    if (mSelectedIndex < 0 && IsCombobox()) {
      return SelectSomething(aNotify);
    }
  }
  return PR_FALSE;
}

PRBool
nsHTMLSelectElement::SelectSomething(PRBool aNotify)
{
  
  if (!mIsDoneAddingChildren) {
    return PR_FALSE;
  }

  PRUint32 count;
  GetLength(&count);
  for (PRUint32 i=0; i<count; i++) {
    PRBool disabled;
    nsresult rv = IsOptionDisabled(i, &disabled);

    if (NS_FAILED(rv) || !disabled) {
      rv = SetSelectedIndexInternal(i, aNotify);
      NS_ENSURE_SUCCESS(rv, PR_FALSE);

      UpdateValueMissingValidityState();
      if (aNotify) {
        nsIDocument* doc = GetCurrentDoc();
        if (doc) {
          nsAutoScriptBlocker scriptBlocker;
          doc->ContentStateChanged(this, NS_EVENT_STATE_VALID |
                                         NS_EVENT_STATE_INVALID |
                                         NS_EVENT_STATE_MOZ_UI_INVALID |
                                         NS_EVENT_STATE_MOZ_UI_VALID);
        }
      }

      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsresult
nsHTMLSelectElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElement::BindToTree(aDocument, aParent,
                                                     aBindingParent,
                                                     aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  UpdateBarredFromConstraintValidation();

  return rv;
}

void
nsHTMLSelectElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);

  
  UpdateBarredFromConstraintValidation();
}

nsresult
nsHTMLSelectElement::BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                   const nsAString* aValue, PRBool aNotify)
{
  if (aNotify && aName == nsGkAtoms::disabled &&
      aNameSpaceID == kNameSpaceID_None) {
    mDisabledChanged = PR_TRUE;
  }

  return nsGenericHTMLFormElement::BeforeSetAttr(aNameSpaceID, aName,
                                                 aValue, aNotify);
}

nsresult
nsHTMLSelectElement::AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                  const nsAString* aValue, PRBool aNotify)
{
  nsEventStates states;

  if (aNameSpaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::disabled) {
      UpdateBarredFromConstraintValidation();
      states |= NS_EVENT_STATE_VALID | NS_EVENT_STATE_INVALID |
                NS_EVENT_STATE_MOZ_UI_VALID | NS_EVENT_STATE_MOZ_UI_INVALID;
    } else if (aName == nsGkAtoms::required) {
      UpdateValueMissingValidityState();
      states |= NS_EVENT_STATE_VALID | NS_EVENT_STATE_INVALID |
                NS_EVENT_STATE_MOZ_UI_VALID | NS_EVENT_STATE_MOZ_UI_INVALID;
    }
  }

  if (aNotify && !states.IsEmpty()) {
    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      nsAutoScriptBlocker scriptBlocker;
      doc->ContentStateChanged(this, states);
    }
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName,
                                                aValue, aNotify);
}

nsresult
nsHTMLSelectElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                               PRBool aNotify)
{
  if (aNotify && aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::multiple) {
    
    
    
    
    
    
    if (mSelectedIndex >= 0) {
      SetSelectedIndexInternal(mSelectedIndex, aNotify);
    }
  }

  nsresult rv = nsGenericHTMLFormElement::UnsetAttr(aNameSpaceID, aAttribute,
                                                    aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNotify && aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::multiple) {
    
    
    CheckSelectSomething(aNotify);
  }

  return rv;
}

nsresult
nsHTMLSelectElement::DoneAddingChildren(PRBool aHaveNotified)
{
  mIsDoneAddingChildren = PR_TRUE;

  nsISelectControlFrame* selectFrame = GetSelectFrame();

  
  
  if (mRestoreState) {
    RestoreStateTo(mRestoreState);
    mRestoreState = nsnull;
  }

  
  if (selectFrame) {
    selectFrame->DoneAddingChildren(PR_TRUE);
  }

  
  if (!mInhibitStateRestoration) {
    RestoreFormControlState(this, this);
  }

  
  
  if (!CheckSelectSomething(PR_FALSE)) {
    
    
    
    UpdateValueMissingValidityState();
  }

  mDefaultSelectionSet = PR_TRUE;

  return NS_OK;
}

PRBool
nsHTMLSelectElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aAttribute == nsGkAtoms::size && kNameSpaceID_None == aNamespaceID) {
    return aResult.ParsePositiveIntValue(aValue);
  }
  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  nsGenericHTMLFormElement::MapImageAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLFormElement::MapCommonAttributesInto(aAttributes, aData);
}

nsChangeHint
nsHTMLSelectElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                            PRInt32 aModType) const
{
  nsChangeHint retval =
      nsGenericHTMLFormElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::multiple ||
      aAttribute == nsGkAtoms::size) {
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
  }
  return retval;
}

NS_IMETHODIMP_(PRBool)
nsHTMLSelectElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sCommonAttributeMap,
    sImageAlignAttributeMap
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

nsMapRuleToAttributesFunc
nsHTMLSelectElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}


nsresult
nsHTMLSelectElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = PR_FALSE;
  
  
  if (IsDisabled()) {
    return NS_OK;
  }

  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);
  nsIFrame* formFrame = nsnull;

  if (formControlFrame &&
      (formFrame = do_QueryFrame(formControlFrame))) {
    const nsStyleUserInterface* uiStyle = formFrame->GetStyleUserInterface();

    if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
        uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED) {
      return NS_OK;
    }
  }

  return nsGenericHTMLFormElement::PreHandleEvent(aVisitor);
}

nsresult
nsHTMLSelectElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (aVisitor.mEvent->message == NS_FOCUS_CONTENT) {
    
    
    mCanShowInvalidUI = !IsValid() && ShouldShowValidityUI();

    
    
    mCanShowValidUI = ShouldShowValidityUI();

    
    
  } else if (aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    mCanShowInvalidUI = PR_TRUE;
    mCanShowValidUI = PR_TRUE;

    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      nsAutoScriptBlocker scriptBlocker;
      doc->ContentStateChanged(this, NS_EVENT_STATE_MOZ_UI_VALID |
                                     NS_EVENT_STATE_MOZ_UI_INVALID);
    }
  }

  return nsGenericHTMLFormElement::PostHandleEvent(aVisitor);
}

nsEventStates
nsHTMLSelectElement::IntrinsicState() const
{
  nsEventStates state = nsGenericHTMLFormElement::IntrinsicState();

  if (IsCandidateForConstraintValidation()) {
    if (IsValid()) {
      state |= NS_EVENT_STATE_VALID;
    } else {
      state |= NS_EVENT_STATE_INVALID;

      if ((!mForm || !mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate)) &&
          (GetValidityState(VALIDITY_STATE_CUSTOM_ERROR) ||
           (mCanShowInvalidUI && ShouldShowValidityUI()))) {
        state |= NS_EVENT_STATE_MOZ_UI_INVALID;
      }
    }

    
    
    
    
    
    
    
    
    
    if ((!mForm || !mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate)) &&
        (mCanShowValidUI && ShouldShowValidityUI() &&
         (IsValid() || (state.HasState(NS_EVENT_STATE_MOZ_UI_INVALID) &&
                        !mCanShowInvalidUI)))) {
      state |= NS_EVENT_STATE_MOZ_UI_VALID;
    }
  }

  if (HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    state |= NS_EVENT_STATE_REQUIRED;
  } else {
    state |= NS_EVENT_STATE_OPTIONAL;
  }

  return state;
}



NS_IMETHODIMP
nsHTMLSelectElement::SaveState()
{
  nsRefPtr<nsSelectState> state = new nsSelectState();
  if (!state) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRUint32 len;
  GetLength(&len);

  for (PRUint32 optIndex = 0; optIndex < len; optIndex++) {
    nsIDOMHTMLOptionElement *option = mOptions->ItemAsOption(optIndex);
    if (option) {
      PRBool isSelected;
      option->GetSelected(&isSelected);
      if (isSelected) {
        nsAutoString value;
        option->GetValue(value);
        state->PutOption(optIndex, value);
      }
    }
  }

  nsPresState *presState = nsnull;
  nsresult rv = GetPrimaryPresState(this, &presState);
  if (presState) {
    presState->SetStateProperty(state);

    if (mDisabledChanged) {
      
      
      presState->SetDisabled(HasAttr(kNameSpaceID_None, nsGkAtoms::disabled));
    }
  }

  return rv;
}

PRBool
nsHTMLSelectElement::RestoreState(nsPresState* aState)
{
  
  nsCOMPtr<nsSelectState> state(
    do_QueryInterface(aState->GetStateProperty()));

  if (state) {
    RestoreStateTo(state);

    
    
    DispatchContentReset();
  }

  if (aState->IsDisabledSet()) {
    SetDisabled(aState->GetDisabled());
  }

  return PR_FALSE;
}

void
nsHTMLSelectElement::RestoreStateTo(nsSelectState* aNewSelected)
{
  if (!mIsDoneAddingChildren) {
    mRestoreState = aNewSelected;
    return;
  }

  PRUint32 len;
  GetLength(&len);

  
  SetOptionsSelectedByIndex(-1, -1, PR_TRUE, PR_TRUE, PR_TRUE, PR_TRUE, nsnull);

  
  for (PRInt32 i = 0; i < (PRInt32)len; i++) {
    nsIDOMHTMLOptionElement *option = mOptions->ItemAsOption(i);
    if (option) {
      nsAutoString value;
      nsresult rv = option->GetValue(value);
      if (NS_SUCCEEDED(rv) && aNewSelected->ContainsOption(i, value)) {
        SetOptionsSelectedByIndex(i, i, PR_TRUE, PR_FALSE, PR_TRUE, PR_TRUE, nsnull);
      }
    }
  }

  
}

NS_IMETHODIMP
nsHTMLSelectElement::Reset()
{
  PRUint32 numSelected = 0;

  
  
  
  PRUint32 numOptions;
  nsresult rv = GetLength(&numOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < numOptions; i++) {
    nsCOMPtr<nsIDOMNode> node;
    rv = Item(i, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMHTMLOptionElement> option(do_QueryInterface(node));

    NS_ASSERTION(option, "option not an OptionElement");
    if (option) {
      
      
      
      PRBool selected = PR_FALSE;
      option->GetDefaultSelected(&selected);
      SetOptionsSelectedByIndex(i, i, selected,
                                PR_FALSE, PR_TRUE, PR_TRUE, nsnull);
      if (selected) {
        numSelected++;
      }
    }
  }

  
  
  
  if (numSelected == 0 && IsCombobox()) {
    SelectSomething(PR_TRUE);
  }

  SetSelectionChanged(PR_FALSE, PR_TRUE);

  
  
  
  
  
  
  DispatchContentReset();

  return NS_OK;
}

static NS_DEFINE_CID(kFormProcessorCID, NS_FORMPROCESSOR_CID);

NS_IMETHODIMP
nsHTMLSelectElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  nsresult rv = NS_OK;

  
  if (IsDisabled()) {
    return NS_OK;
  }

  
  
  
  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
  if (name.IsEmpty()) {
    return NS_OK;
  }

  
  
  
  PRUint32 len;
  GetLength(&len);

  nsAutoString mozType;
  nsCOMPtr<nsIFormProcessor> keyGenProcessor;
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::_moz_type, mozType) &&
      mozType.EqualsLiteral("-mozilla-keygen")) {
    keyGenProcessor = do_GetService(kFormProcessorCID, &rv);
  }

  for (PRUint32 optIndex = 0; optIndex < len; optIndex++) {
    
    PRBool disabled;
    rv = IsOptionDisabled(optIndex, &disabled);
    if (NS_FAILED(rv) || disabled) {
      continue;
    }

    nsIDOMHTMLOptionElement *option = mOptions->ItemAsOption(optIndex);
    NS_ENSURE_TRUE(option, NS_ERROR_UNEXPECTED);

    PRBool isSelected;
    rv = option->GetSelected(&isSelected);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!isSelected) {
      continue;
    }

    nsCOMPtr<nsIDOMHTMLOptionElement> optionElement = do_QueryInterface(option);
    NS_ENSURE_TRUE(optionElement, NS_ERROR_UNEXPECTED);

    nsAutoString value;
    rv = optionElement->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    if (keyGenProcessor) {
      nsAutoString tmp(value);
      rv = keyGenProcessor->ProcessValue(this, name, tmp);
      if (NS_SUCCEEDED(rv)) {
        value = tmp;
      }
    }

    rv = aFormSubmission->AddNameValuePair(name, value);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSelectElement::GetHasOptGroups(PRBool* aHasGroups)
{
  *aHasGroups = (mOptGroupCount > 0);
  return NS_OK;
}

void nsHTMLSelectElement::DispatchContentReset() {
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);
  if (formControlFrame) {
    
    
    if (IsCombobox()) {
      nsIComboboxControlFrame* comboFrame = do_QueryFrame(formControlFrame);
      if (comboFrame) {
        comboFrame->OnContentReset();
      }
    } else {
      nsIListControlFrame* listFrame = do_QueryFrame(formControlFrame);
      if (listFrame) {
        listFrame->OnContentReset();
      }
    }
  }
}

static void
AddOptionsRecurse(nsIContent* aRoot, nsHTMLOptionCollection* aArray)
{
  nsIContent* child;
  for(PRUint32 i = 0; (child = aRoot->GetChildAt(i)); ++i) {
    nsHTMLOptionElement *opt = nsHTMLOptionElement::FromContent(child);
    if (opt) {
      
      aArray->AppendOption(opt);
    }
    else if (IsOptGroup(child)) {
      AddOptionsRecurse(child, aArray);
    }
  }
}

void
nsHTMLSelectElement::RebuildOptionsArray(PRBool aNotify)
{
  mOptions->Clear();
  AddOptionsRecurse(this, mOptions);
  FindSelectedIndex(0, aNotify);
}

bool
nsHTMLSelectElement::IsValueMissing()
{
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    return false;
  }

  PRUint32 length;
  nsIDOMHTMLOptionElement* option = nsnull;
  PRBool disabled;
  PRBool selected;

  mOptions->GetLength(&length);

  for (PRUint32 i=0; i<length; ++i) {
    option = mOptions->ItemAsOption(i);
    NS_ENSURE_SUCCESS(option->GetSelected(&selected), false);

    if (!selected) {
      continue;
    }

    IsOptionDisabled(i, &disabled);
    if (disabled) {
      continue;
    }

    nsAutoString value;
    NS_ENSURE_SUCCESS(option->GetValue(value), false);
    if (!value.IsEmpty()) {
      return false;
    }
  }

  return true;
}

void
nsHTMLSelectElement::UpdateValueMissingValidityState()
{
  SetValidityState(VALIDITY_STATE_VALUE_MISSING, IsValueMissing());
}

nsresult
nsHTMLSelectElement::GetValidationMessage(nsAString& aValidationMessage,
                                            ValidityStateType aType)
{
  nsresult rv = NS_OK;

  switch (aType)
  {
    case VALIDITY_STATE_VALUE_MISSING:
      {
        nsXPIDLString message;

        rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                "FormValidationSelectMissing",
                                                message);

        aValidationMessage = message;
      }
      break;
    default:
      rv = nsIConstraintValidation::GetValidationMessage(aValidationMessage, aType);
  }

  return rv;
}

#ifdef DEBUG

static void
VerifyOptionsRecurse(nsIContent* aRoot, PRInt32& aIndex,
                     nsHTMLOptionCollection* aArray)
{
  nsIContent* child;
  for(PRUint32 i = 0; (child = aRoot->GetChildAt(i)); ++i) {
    nsCOMPtr<nsIDOMHTMLOptionElement> opt = do_QueryInterface(child);
    if (opt) {
      NS_ASSERTION(opt == aArray->ItemAsOption(aIndex++),
                   "Options collection broken");
    }
    else if (IsOptGroup(child)) {
      VerifyOptionsRecurse(child, aIndex, aArray);
    }
  }
}

void
nsHTMLSelectElement::VerifyOptionsArray()
{
  PRInt32 aIndex = 0;
  VerifyOptionsRecurse(this, aIndex, mOptions);
}

#endif






nsHTMLOptionCollection::nsHTMLOptionCollection(nsHTMLSelectElement* aSelect)
{
  
  
  mSelect = aSelect;
}

nsHTMLOptionCollection::~nsHTMLOptionCollection()
{
  DropReference();
}

void
nsHTMLOptionCollection::DropReference()
{
  
  mSelect = nsnull;
}

nsresult
nsHTMLOptionCollection::GetOptionIndex(mozilla::dom::Element* aOption,
                                       PRInt32 aStartIndex,
                                       PRBool aForward,
                                       PRInt32* aIndex)
{
  PRInt32 index;

  
  if (aStartIndex == 0 && aForward) {
    index = mElements.IndexOf(aOption);
    if (index == -1) {
      return NS_ERROR_FAILURE;
    }
    
    *aIndex = index;
    return NS_OK;
  }

  PRInt32 high = mElements.Length();
  PRInt32 step = aForward ? 1 : -1;

  for (index = aStartIndex; index < high && index > -1; index += step) {
    if (mElements[index] == aOption) {
      *aIndex = index;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLOptionCollection)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsHTMLOptionCollection)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mElements)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsHTMLOptionCollection)
    {
      PRUint32 i;
      for (i = 0; i < tmp->mElements.Length(); ++i) {
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mElements[i]");
        cb.NoteXPCOMChild(static_cast<Element*>(tmp->mElements[i]));
      }
    }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END



DOMCI_DATA(HTMLOptionsCollection, nsHTMLOptionCollection)


NS_INTERFACE_TABLE_HEAD(nsHTMLOptionCollection)
  NS_INTERFACE_TABLE4(nsHTMLOptionCollection,
                      nsIHTMLCollection,
                      nsIDOMNSHTMLOptionCollection,
                      nsIDOMHTMLOptionsCollection,
                      nsIDOMHTMLCollection)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsHTMLOptionCollection)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLOptionsCollection)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsHTMLOptionCollection)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsHTMLOptionCollection)




NS_IMETHODIMP
nsHTMLOptionCollection::GetLength(PRUint32* aLength)
{
  *aLength = mElements.Length();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionCollection::SetLength(PRUint32 aLength)
{
  if (!mSelect) {
    return NS_ERROR_UNEXPECTED;
  }

  return mSelect->SetLength(aLength);
}

NS_IMETHODIMP
nsHTMLOptionCollection::SetOption(PRInt32 aIndex,
                                  nsIDOMHTMLOptionElement *aOption)
{
  if (aIndex < 0 || !mSelect) {
    return NS_OK;
  }

  
  
  if (!aOption) {
    mSelect->Remove(aIndex);

    
    return NS_OK;
  }

  nsresult rv = NS_OK;

  PRUint32 index = PRUint32(aIndex);

  
  if (index > mElements.Length()) {
    
    
    rv = SetLength(index);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(index <= mElements.Length(), "SetLength lied");
  
  nsCOMPtr<nsIDOMNode> ret;
  if (index == mElements.Length()) {
    rv = mSelect->AppendChild(aOption, getter_AddRefs(ret));
  } else {
    
    
    nsCOMPtr<nsIDOMHTMLOptionElement> refChild = ItemAsOption(index);
    NS_ENSURE_TRUE(refChild, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDOMNode> parent;
    refChild->GetParentNode(getter_AddRefs(parent));
    if (parent) {
      rv = parent->ReplaceChild(aOption, refChild, getter_AddRefs(ret));
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLOptionCollection::GetSelectedIndex(PRInt32 *aSelectedIndex)
{
  NS_ENSURE_TRUE(mSelect, NS_ERROR_UNEXPECTED);

  return mSelect->GetSelectedIndex(aSelectedIndex);
}

NS_IMETHODIMP
nsHTMLOptionCollection::SetSelectedIndex(PRInt32 aSelectedIndex)
{
  NS_ENSURE_TRUE(mSelect, NS_ERROR_UNEXPECTED);

  return mSelect->SetSelectedIndex(aSelectedIndex);
}

NS_IMETHODIMP
nsHTMLOptionCollection::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsISupports* item = GetNodeAt(aIndex);
  if (!item) {
    *aReturn = nsnull;

    return NS_OK;
  }

  return CallQueryInterface(item, aReturn);
}

nsIContent*
nsHTMLOptionCollection::GetNodeAt(PRUint32 aIndex)
{
  return static_cast<nsIContent*>(ItemAsOption(aIndex));
}

static nsHTMLOptionElement*
GetNamedItemHelper(nsTArray<nsRefPtr<nsHTMLOptionElement> > &aElements,
                   const nsAString& aName)
{
  PRUint32 count = aElements.Length();
  for (PRUint32 i = 0; i < count; i++) {
    nsHTMLOptionElement *content = aElements.ElementAt(i);
    if (content &&
        (content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name, aName,
                              eCaseMatters) ||
         content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::id, aName,
                              eCaseMatters))) {
      return content;
    }
  }

  return nsnull;
}

nsISupports*
nsHTMLOptionCollection::GetNamedItem(const nsAString& aName,
                                     nsWrapperCache **aCache)
{
  nsINode *item = GetNamedItemHelper(mElements, aName);
  *aCache = item;
  return item;
}

NS_IMETHODIMP
nsHTMLOptionCollection::NamedItem(const nsAString& aName,
                                  nsIDOMNode** aReturn)
{
  NS_IF_ADDREF(*aReturn = GetNamedItemHelper(mElements, aName));

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionCollection::GetSelect(nsIDOMHTMLSelectElement **aReturn)
{
  NS_IF_ADDREF(*aReturn = mSelect);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionCollection::Add(nsIDOMHTMLOptionElement *aOption,
                            PRInt32 aIndex, PRUint8 optional_argc)
{
  if (!aOption) {
    return NS_ERROR_INVALID_ARG;
  }

  if (aIndex < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  if (!mSelect) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  PRUint32 length;
  GetLength(&length);

  if (optional_argc == 0 || aIndex == -1 || aIndex > (PRInt32)length) {
    
    aIndex = length;
  }

  nsCOMPtr<nsIDOMNode> beforeNode;
  Item(aIndex, getter_AddRefs(beforeNode));

  nsCOMPtr<nsIDOMHTMLOptionElement> beforeElement =
    do_QueryInterface(beforeNode);

  return mSelect->Add(aOption, beforeElement);
}

NS_IMETHODIMP
nsHTMLOptionCollection::Remove(PRInt32 aIndex)
{
  NS_ENSURE_TRUE(mSelect, NS_ERROR_UNEXPECTED);

  PRUint32 len = 0;
  mSelect->GetLength(&len);
  if (aIndex < 0 || (PRUint32)aIndex >= len)
    aIndex = 0;

  return mSelect->Remove(aIndex);
}

void
nsHTMLSelectElement::UpdateBarredFromConstraintValidation()
{
  SetBarredFromConstraintValidation(IsDisabled());
}

void
nsHTMLSelectElement::FieldSetDisabledChanged(nsEventStates aStates, PRBool aNotify)
{
  UpdateBarredFromConstraintValidation();

  aStates |= NS_EVENT_STATE_VALID | NS_EVENT_STATE_INVALID |
             NS_EVENT_STATE_MOZ_UI_VALID | NS_EVENT_STATE_MOZ_UI_INVALID;
  nsGenericHTMLFormElement::FieldSetDisabledChanged(aStates, aNotify);
}

void
nsHTMLSelectElement::SetSelectionChanged(PRBool aValue, PRBool aNotify)
{
  if (!mDefaultSelectionSet) {
    return;
  }

  PRBool previousSelectionChangedValue = mSelectionHasChanged;
  mSelectionHasChanged = aValue;

  if (aNotify && mSelectionHasChanged != previousSelectionChangedValue) {
    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      nsAutoScriptBlocker scriptBlocker;
      doc->ContentStateChanged(this, NS_EVENT_STATE_MOZ_UI_INVALID |
                                     NS_EVENT_STATE_MOZ_UI_VALID);
    }
  }
}

