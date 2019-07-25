





































#include "nsHTMLSelectAccessible.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsDocAccessible.h"
#include "nsEventShell.h"
#include "nsIAccessibleEvent.h"
#include "nsTextEquivUtils.h"

#include "nsCOMPtr.h"
#include "nsIFrame.h"
#include "nsIComboboxControlFrame.h"
#include "nsIDocument.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIListControlFrame.h"
#include "nsIServiceManager.h"
#include "nsIMutableArray.h"





nsHTMLSelectListAccessible::
  nsHTMLSelectListAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}




nsresult
nsHTMLSelectListAccessible::GetStateInternal(PRUint32 *aState,
                                             PRUint32 *aExtraState)
{
  nsresult rv = nsAccessibleWrap::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  
  

  if (*aState & nsIAccessibleStates::STATE_FOCUSED) {
    
    
    nsCOMPtr<nsIContent> focusedOption =
      nsHTMLSelectOptionAccessible::GetFocusedOption(mContent);
    if (focusedOption) { 
      *aState &= ~nsIAccessibleStates::STATE_FOCUSED;
    }
  }
  if (mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::multiple))
    *aState |= nsIAccessibleStates::STATE_MULTISELECTABLE |
               nsIAccessibleStates::STATE_EXTSELECTABLE;

  return NS_OK;
}

PRUint32
nsHTMLSelectListAccessible::NativeRole()
{
  if (mParent && mParent->Role() == nsIAccessibleRole::ROLE_COMBOBOX)
    return nsIAccessibleRole::ROLE_COMBOBOX_LIST;

  return nsIAccessibleRole::ROLE_LISTBOX;
}




bool
nsHTMLSelectListAccessible::IsSelect()
{
  return true;
}

bool
nsHTMLSelectListAccessible::SelectAll()
{
  return mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::multiple) ?
           nsAccessibleWrap::SelectAll() : false;
}

bool
nsHTMLSelectListAccessible::UnselectAll()
{
  return mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::multiple) ?
           nsAccessibleWrap::UnselectAll() : false;
}




void
nsHTMLSelectListAccessible::CacheChildren()
{
  
  
  
  
  CacheOptSiblings(mContent);
}




void
nsHTMLSelectListAccessible::CacheOptSiblings(nsIContent *aParentContent)
{
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
  PRUint32 numChildren = aParentContent->GetChildCount();
  for (PRUint32 count = 0; count < numChildren; count ++) {
    nsIContent *childContent = aParentContent->GetChildAt(count);
    if (!childContent->IsHTML()) {
      continue;
    }

    nsCOMPtr<nsIAtom> tag = childContent->Tag();
    if (tag == nsAccessibilityAtoms::option ||
        tag == nsAccessibilityAtoms::optgroup) {

      
      nsRefPtr<nsAccessible> accessible =
        GetAccService()->GetOrCreateAccessible(childContent, presShell,
                                               mWeakShell);
      if (accessible)
        AppendChild(accessible);

      
      if (tag == nsAccessibilityAtoms::optgroup)
        CacheOptSiblings(childContent);
    }
  }
}






nsHTMLSelectOptionAccessible::
  nsHTMLSelectOptionAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}




PRUint32
nsHTMLSelectOptionAccessible::NativeRole()
{
  if (mParent && mParent->Role() == nsIAccessibleRole::ROLE_COMBOBOX_LIST)
    return nsIAccessibleRole::ROLE_COMBOBOX_OPTION;

  return nsIAccessibleRole::ROLE_OPTION;
}

nsresult
nsHTMLSelectOptionAccessible::GetNameInternal(nsAString& aName)
{
  
  
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::label, aName);
  if (!aName.IsEmpty())
    return NS_OK;

  
  
  nsIContent *text = mContent->GetChildAt(0);
  if (!text)
    return NS_OK;

  if (text->IsNodeOfType(nsINode::eTEXT)) {
    nsAutoString txtValue;
    nsresult rv = nsTextEquivUtils::
      AppendTextEquivFromTextContent(text, &txtValue);
    NS_ENSURE_SUCCESS(rv, rv);

    
    txtValue.CompressWhitespace();
    aName.Assign(txtValue);
    return NS_OK;
  }

  return NS_OK;
}


nsIFrame* nsHTMLSelectOptionAccessible::GetBoundsFrame()
{
  PRUint32 state = 0;
  nsCOMPtr<nsIContent> content = GetSelectState(&state);
  if (state & nsIAccessibleStates::STATE_COLLAPSED) {
    if (content) {
      return content->GetPrimaryFrame();
    }

    return nsnull;
  }

  return nsAccessible::GetBoundsFrame();
}









nsresult
nsHTMLSelectOptionAccessible::GetStateInternal(PRUint32 *aState,
                                               PRUint32 *aExtraState)
{
  
  
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  PRUint32 selectState = 0, selectExtState = 0;
  nsCOMPtr<nsIContent> selectContent = GetSelectState(&selectState,
                                                      &selectExtState);
  if (selectState & nsIAccessibleStates::STATE_INVISIBLE) {
    return NS_OK;
  }

  NS_ENSURE_TRUE(selectContent, NS_ERROR_FAILURE);

  
  if (0 == (*aState & nsIAccessibleStates::STATE_UNAVAILABLE)) {
    *aState |= (nsIAccessibleStates::STATE_FOCUSABLE |
                nsIAccessibleStates::STATE_SELECTABLE);
    
    
    
    
    
    
    
    nsCOMPtr<nsIContent> focusedOption = GetFocusedOption(selectContent);
    if (focusedOption == mContent)
      *aState |= nsIAccessibleStates::STATE_FOCUSED;
  }

  
  PRBool isSelected = PR_FALSE;
  nsCOMPtr<nsIDOMHTMLOptionElement> option(do_QueryInterface(mContent));
  if (option) {
    option->GetSelected(&isSelected);
    if ( isSelected ) 
      *aState |= nsIAccessibleStates::STATE_SELECTED;
  }

  if (selectState & nsIAccessibleStates::STATE_OFFSCREEN) {
    *aState |= nsIAccessibleStates::STATE_OFFSCREEN;
  }
  else if (selectState & nsIAccessibleStates::STATE_COLLAPSED) {
    
    
    if (!isSelected) {
      *aState |= nsIAccessibleStates::STATE_OFFSCREEN;
    }
    else {
      
      *aState &= ~nsIAccessibleStates::STATE_OFFSCREEN;
      *aState &= ~nsIAccessibleStates::STATE_INVISIBLE;
       if (aExtraState) {
         *aExtraState |= selectExtState & nsIAccessibleStates::EXT_STATE_OPAQUE;
       }
    }
  }
  else {
    
    
    *aState &= ~nsIAccessibleStates::STATE_OFFSCREEN;
    
    nsAccessible* listAcc = GetParent();
    if (listAcc) {
      PRInt32 optionX, optionY, optionWidth, optionHeight;
      PRInt32 listX, listY, listWidth, listHeight;
      GetBounds(&optionX, &optionY, &optionWidth, &optionHeight);
      listAcc->GetBounds(&listX, &listY, &listWidth, &listHeight);
      if (optionY < listY || optionY + optionHeight > listY + listHeight) {
        *aState |= nsIAccessibleStates::STATE_OFFSCREEN;
      }
    }
  }
 
  return NS_OK;
}

PRInt32
nsHTMLSelectOptionAccessible::GetLevelInternal()
{
  nsIContent *parentContent = mContent->GetParent();

  PRInt32 level =
    parentContent->NodeInfo()->Equals(nsAccessibilityAtoms::optgroup) ? 2 : 1;

  if (level == 1 && Role() != nsIAccessibleRole::ROLE_HEADING)
    level = 0; 

  return level;
}

void
nsHTMLSelectOptionAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                                         PRInt32 *aSetSize)
{
  nsIContent *parentContent = mContent->GetParent();

  PRInt32 posInSet = 0, setSize = 0;
  PRBool isContentFound = PR_FALSE;

  PRUint32 childCount = parentContent->GetChildCount();
  for (PRUint32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsIContent *childContent = parentContent->GetChildAt(childIdx);
    if (childContent->NodeInfo()->Equals(mContent->NodeInfo())) {
      if (!isContentFound) {
        if (childContent == mContent)
          isContentFound = PR_TRUE;

        posInSet++;
      }
      setSize++;
    }
  }

  *aSetSize = setSize;
  *aPosInSet = posInSet;
}





NS_IMETHODIMP nsHTMLSelectOptionAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Select) {
    aName.AssignLiteral("select"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsHTMLSelectOptionAccessible::GetNumActions(PRUint8 *_retval)
{
  *_retval = 1;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLSelectOptionAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Select) {   
    nsCOMPtr<nsIDOMHTMLOptionElement> newHTMLOption(do_QueryInterface(mContent));
    if (!newHTMLOption) 
      return NS_ERROR_FAILURE;
    
    nsAccessible* parent = GetParent();
    NS_ASSERTION(parent, "No parent!");

    nsCOMPtr<nsIContent> oldHTMLOptionContent =
      GetFocusedOption(parent->GetContent());
    nsCOMPtr<nsIDOMHTMLOptionElement> oldHTMLOption =
      do_QueryInterface(oldHTMLOptionContent);
    if (oldHTMLOption)
      oldHTMLOption->SetSelected(PR_FALSE);
    
    newHTMLOption->SetSelected(PR_TRUE);

    
    
    nsIContent *selectContent = mContent;
    do {
      selectContent = selectContent->GetParent();
      nsCOMPtr<nsIDOMHTMLSelectElement> selectControl =
        do_QueryInterface(selectContent);
      if (selectControl)
        break;

    } while (selectContent);

    nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
    nsCOMPtr<nsIDOMHTMLOptionElement> option(do_QueryInterface(mContent));

    if (!selectContent || !presShell || !option)
      return NS_ERROR_FAILURE;

    nsIFrame *selectFrame = selectContent->GetPrimaryFrame();
    nsIComboboxControlFrame *comboBoxFrame = do_QueryFrame(selectFrame);
    if (comboBoxFrame) {
      nsIFrame *listFrame = comboBoxFrame->GetDropDown();
      if (comboBoxFrame->IsDroppedDown() && listFrame) {
        
        nsIListControlFrame *listControlFrame = do_QueryFrame(listFrame);
        if (listControlFrame) {
          PRInt32 newIndex = 0;
          option->GetIndex(&newIndex);
          listControlFrame->ComboboxFinish(newIndex);
        }
      }
    }
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsHTMLSelectOptionAccessible::SetSelected(PRBool aSelect)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMHTMLOptionElement> optionElm(do_QueryInterface(mContent));
  return optionElm->SetSelected(aSelect);
}









already_AddRefed<nsIContent>
nsHTMLSelectOptionAccessible::GetFocusedOption(nsIContent *aListNode)
{
  NS_ASSERTION(aListNode, "Called GetFocusedOptionNode without a valid list node");

  nsIFrame *frame = aListNode->GetPrimaryFrame();
  if (!frame)
    return nsnull;

  PRInt32 focusedOptionIndex = 0;

  
  nsCOMPtr<nsIDOMHTMLSelectElement> selectElement(do_QueryInterface(aListNode));
  NS_ASSERTION(selectElement, "No select element where it should be");

  nsCOMPtr<nsIDOMHTMLOptionsCollection> options;
  nsresult rv = selectElement->GetOptions(getter_AddRefs(options));
  
  if (NS_SUCCEEDED(rv)) {
    nsIListControlFrame *listFrame = do_QueryFrame(frame);
    if (listFrame) {
      
      
      
      
      focusedOptionIndex = listFrame->GetSelectedIndex();
      if (focusedOptionIndex == -1) {
        nsCOMPtr<nsIDOMNode> nextOption;
        while (PR_TRUE) {
          ++ focusedOptionIndex;
          options->Item(focusedOptionIndex, getter_AddRefs(nextOption));
          nsCOMPtr<nsIDOMHTMLOptionElement> optionElement = do_QueryInterface(nextOption);
          if (!optionElement) {
            break;
          }
          PRBool disabled;
          optionElement->GetDisabled(&disabled);
          if (!disabled) {
            break;
          }
        }
      }
    }
    else  
      rv = selectElement->GetSelectedIndex(&focusedOptionIndex);
  }

  
  if (NS_SUCCEEDED(rv) && options && focusedOptionIndex >= 0) {  
    nsCOMPtr<nsIDOMNode> focusedOptionNode;
    options->Item(focusedOptionIndex, getter_AddRefs(focusedOptionNode));
    nsIContent *focusedOption = nsnull;
    if (focusedOptionNode)
      CallQueryInterface(focusedOptionNode, &focusedOption);
    return focusedOption;
  }

  return nsnull;
}

void
nsHTMLSelectOptionAccessible::SelectionChangedIfOption(nsIContent *aPossibleOptionNode)
{
  if (!aPossibleOptionNode ||
      aPossibleOptionNode->Tag() != nsAccessibilityAtoms::option ||
      !aPossibleOptionNode->IsHTML()) {
    return;
  }

  nsAccessible *multiSelect =
    nsAccUtils::GetMultiSelectableContainer(aPossibleOptionNode);
  if (!multiSelect)
    return;

  nsAccessible *option = GetAccService()->GetAccessible(aPossibleOptionNode);
  if (!option)
    return;


  nsRefPtr<AccEvent> selWithinEvent =
    new AccEvent(nsIAccessibleEvent::EVENT_SELECTION_WITHIN, multiSelect);

  if (!selWithinEvent)
    return;

  option->GetDocAccessible()->FireDelayedAccessibleEvent(selWithinEvent);

  PRUint32 state = nsAccUtils::State(option);
  PRUint32 eventType;
  if (state & nsIAccessibleStates::STATE_SELECTED) {
    eventType = nsIAccessibleEvent::EVENT_SELECTION_ADD;
  }
  else {
    eventType = nsIAccessibleEvent::EVENT_SELECTION_REMOVE;
  }

  nsRefPtr<AccEvent> selAddRemoveEvent = new AccEvent(eventType, option);

  if (selAddRemoveEvent)
    option->GetDocAccessible()->FireDelayedAccessibleEvent(selAddRemoveEvent);
}




nsIContent* nsHTMLSelectOptionAccessible::GetSelectState(PRUint32* aState,
                                                         PRUint32* aExtraState)
{
  *aState = 0;

  if (aExtraState)
    *aExtraState = 0;

  nsIContent *content = mContent;
  while (content && content->Tag() != nsAccessibilityAtoms::select) {
    content = content->GetParent();
  }

  if (content) {
    nsAccessible* selAcc = GetAccService()->GetAccessible(content);
    if (selAcc) {
      selAcc->GetState(aState, aExtraState);
      return content;
    }
  }
  return nsnull; 
}






nsHTMLSelectOptGroupAccessible::
  nsHTMLSelectOptGroupAccessible(nsIContent *aContent,
                                 nsIWeakReference *aShell) :
  nsHTMLSelectOptionAccessible(aContent, aShell)
{
}

PRUint32
nsHTMLSelectOptGroupAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_HEADING;
}

nsresult
nsHTMLSelectOptGroupAccessible::GetStateInternal(PRUint32 *aState,
                                                 PRUint32 *aExtraState)
{
  nsresult rv = nsHTMLSelectOptionAccessible::GetStateInternal(aState,
                                                               aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState &= ~(nsIAccessibleStates::STATE_FOCUSABLE |
               nsIAccessibleStates::STATE_SELECTABLE);

  return NS_OK;
}

NS_IMETHODIMP nsHTMLSelectOptGroupAccessible::DoAction(PRUint8 index)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsHTMLSelectOptGroupAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsHTMLSelectOptGroupAccessible::GetNumActions(PRUint8 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}




void
nsHTMLSelectOptGroupAccessible::CacheChildren()
{
  
  
  
  
  
}






nsHTMLComboboxAccessible::
  nsHTMLComboboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsHTMLComboboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_COMBOBOX;
}

void
nsHTMLComboboxAccessible::CacheChildren()
{
  nsIFrame* frame = GetFrame();
  if (!frame)
    return;

  nsIComboboxControlFrame *comboFrame = do_QueryFrame(frame);
  if (!comboFrame)
    return;

  nsIFrame *listFrame = comboFrame->GetDropDown();
  if (!listFrame)
    return;

  if (!mListAccessible) {
    mListAccessible = 
      new nsHTMLComboboxListAccessible(mParent, mContent, mWeakShell);
    if (!mListAccessible)
      return;

    
    if (!mListAccessible->Init()) {
      mListAccessible->Shutdown();
      return;
    }
  }

  AppendChild(mListAccessible);

  
  
  mListAccessible->EnsureChildren();
}

void
nsHTMLComboboxAccessible::Shutdown()
{
  nsAccessibleWrap::Shutdown();

  if (mListAccessible) {
    mListAccessible->Shutdown();
    mListAccessible = nsnull;
  }
}









nsresult
nsHTMLComboboxAccessible::GetStateInternal(PRUint32 *aState,
                                           PRUint32 *aExtraState)
{
  
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsIFrame *frame = GetBoundsFrame();
  nsIComboboxControlFrame *comboFrame = do_QueryFrame(frame);
  if (comboFrame && comboFrame->IsDroppedDown()) {
    *aState |= nsIAccessibleStates::STATE_EXPANDED;
  }
  else {
    *aState &= ~nsIAccessibleStates::STATE_FOCUSED; 
    *aState |= nsIAccessibleStates::STATE_COLLAPSED;
  }

  *aState |= nsIAccessibleStates::STATE_HASPOPUP |
             nsIAccessibleStates::STATE_FOCUSABLE;

  return NS_OK;
}

NS_IMETHODIMP nsHTMLComboboxAccessible::GetDescription(nsAString& aDescription)
{
  aDescription.Truncate();
  
  
  nsAccessible::GetDescription(aDescription);
  if (!aDescription.IsEmpty()) {
    return NS_OK;
  }
  
  nsAccessible *option = GetFocusedOptionAccessible();
  return option ? option->GetDescription(aDescription) : NS_OK;
}

nsAccessible *
nsHTMLComboboxAccessible::GetFocusedOptionAccessible()
{
  if (IsDefunct())
    return nsnull;

  nsCOMPtr<nsIContent> focusedOption =
    nsHTMLSelectOptionAccessible::GetFocusedOption(mContent);
  if (!focusedOption) {
    return nsnull;
  }

  return GetAccService()->GetAccessibleInWeakShell(focusedOption,
                                                   mWeakShell);
}






NS_IMETHODIMP nsHTMLComboboxAccessible::GetValue(nsAString& aValue)
{
  
  nsAccessible *option = GetFocusedOptionAccessible();
  return option ? option->GetName(aValue) : NS_OK;
}


NS_IMETHODIMP nsHTMLComboboxAccessible::GetNumActions(PRUint8 *aNumActions)
{
  *aNumActions = 1;
  return NS_OK;
}




NS_IMETHODIMP nsHTMLComboboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != nsHTMLComboboxAccessible::eAction_Click) {
    return NS_ERROR_INVALID_ARG;
  }
  nsIFrame *frame = GetFrame();
  if (!frame) {
    return NS_ERROR_FAILURE;
  }
  nsIComboboxControlFrame *comboFrame = do_QueryFrame(frame);
  if (!comboFrame) {
    return NS_ERROR_FAILURE;
  }
  
  comboFrame->ShowDropDown(!comboFrame->IsDroppedDown());

  return NS_OK;
}







NS_IMETHODIMP nsHTMLComboboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != nsHTMLComboboxAccessible::eAction_Click) {
    return NS_ERROR_INVALID_ARG;
  }
  nsIFrame *frame = GetFrame();
  if (!frame) {
    return NS_ERROR_FAILURE;
  }
  nsIComboboxControlFrame *comboFrame = do_QueryFrame(frame);
  if (!comboFrame) {
    return NS_ERROR_FAILURE;
  }
  if (comboFrame->IsDroppedDown())
    aName.AssignLiteral("close"); 
  else
    aName.AssignLiteral("open"); 

  return NS_OK;
}






nsHTMLComboboxListAccessible::
  nsHTMLComboboxListAccessible(nsIAccessible *aParent, nsIContent *aContent,
                               nsIWeakReference *aShell) :
  nsHTMLSelectListAccessible(aContent, aShell)
{
}

nsIFrame*
nsHTMLComboboxListAccessible::GetFrame()
{
  nsIFrame* frame = nsHTMLSelectListAccessible::GetFrame();

  if (frame) {
    nsIComboboxControlFrame* comboBox = do_QueryFrame(frame);
    if (comboBox) {
      return comboBox->GetDropDown();
    }
  }

  return nsnull;
}








nsresult
nsHTMLComboboxListAccessible::GetStateInternal(PRUint32 *aState,
                                               PRUint32 *aExtraState)
{
  
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsIFrame *boundsFrame = GetBoundsFrame();
  nsIComboboxControlFrame* comboFrame = do_QueryFrame(boundsFrame);
  if (comboFrame && comboFrame->IsDroppedDown())
    *aState |= nsIAccessibleStates::STATE_FLOATING;
  else
    *aState |= nsIAccessibleStates::STATE_INVISIBLE;

  return NS_OK;
}

NS_IMETHODIMP nsHTMLComboboxListAccessible::GetUniqueID(void **aUniqueID)
{
  
  
  *aUniqueID = static_cast<void*>(this);
  return NS_OK;
}





void nsHTMLComboboxListAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame)
{
  *aBoundingFrame = nsnull;

  nsAccessible* comboAcc = GetParent();
  if (!comboAcc)
    return;

  if (0 == (nsAccUtils::State(comboAcc) & nsIAccessibleStates::STATE_COLLAPSED)) {
    nsHTMLSelectListAccessible::GetBoundsRect(aBounds, aBoundingFrame);
    return;
  }

  
  nsIContent* content = mContent->GetChildAt(0);
  if (!content) {
    return;
  }
  nsIFrame* frame = content->GetPrimaryFrame();
  if (!frame) {
    *aBoundingFrame = nsnull;
    return;
  }

  *aBoundingFrame = frame->GetParent();
  aBounds = (*aBoundingFrame)->GetRect();
}
