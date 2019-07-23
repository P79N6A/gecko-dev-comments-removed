





































#include "nsCOMPtr.h"
#include "nsHTMLSelectAccessible.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessibleEvent.h"
#include "nsIFrame.h"
#include "nsIComboboxControlFrame.h"
#include "nsIDocument.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIListControlFrame.h"
#include "nsIServiceManager.h"
#include "nsIMutableArray.h"




























nsHTMLSelectableAccessible::iterator::iterator(nsHTMLSelectableAccessible *aParent, nsIWeakReference *aWeakShell): 
  mWeakShell(aWeakShell), mParentSelect(aParent)
{
  mLength = mIndex = 0;
  mSelCount = 0;

  nsCOMPtr<nsIDOMHTMLSelectElement> htmlSelect(do_QueryInterface(mParentSelect->mDOMNode));
  if (htmlSelect) {
    htmlSelect->GetOptions(getter_AddRefs(mOptions));
    if (mOptions)
      mOptions->GetLength(&mLength);
  }
}

PRBool nsHTMLSelectableAccessible::iterator::Advance() 
{
  if (mIndex < mLength) {
    nsCOMPtr<nsIDOMNode> tempNode;
    if (mOptions) {
      mOptions->Item(mIndex, getter_AddRefs(tempNode));
      mOption = do_QueryInterface(tempNode);
    }
    mIndex++;
    return PR_TRUE;
  }
  return PR_FALSE;
}

void nsHTMLSelectableAccessible::iterator::CalcSelectionCount(PRInt32 *aSelectionCount)
{
  PRBool isSelected = PR_FALSE;

  if (mOption)
    mOption->GetSelected(&isSelected);

  if (isSelected)
    (*aSelectionCount)++;
}

void nsHTMLSelectableAccessible::iterator::AddAccessibleIfSelected(nsIAccessibilityService *aAccService, 
                                                                   nsIMutableArray *aSelectedAccessibles, 
                                                                   nsPresContext *aContext)
{
  PRBool isSelected = PR_FALSE;
  nsCOMPtr<nsIAccessible> tempAccess;

  if (mOption) {
    mOption->GetSelected(&isSelected);
    if (isSelected) {
      nsCOMPtr<nsIDOMNode> optionNode(do_QueryInterface(mOption));
      aAccService->GetAccessibleInWeakShell(optionNode, mWeakShell, getter_AddRefs(tempAccess));
    }
  }

  if (tempAccess)
    aSelectedAccessibles->AppendElement(static_cast<nsISupports*>(tempAccess), PR_FALSE);
}

PRBool nsHTMLSelectableAccessible::iterator::GetAccessibleIfSelected(PRInt32 aIndex, 
                                                                     nsIAccessibilityService *aAccService, 
                                                                     nsPresContext *aContext, 
                                                                     nsIAccessible **aAccessible)
{
  PRBool isSelected = PR_FALSE;

  *aAccessible = nsnull;

  if (mOption) {
    mOption->GetSelected(&isSelected);
    if (isSelected) {
      if (mSelCount == aIndex) {
        nsCOMPtr<nsIDOMNode> optionNode(do_QueryInterface(mOption));
        aAccService->GetAccessibleInWeakShell(optionNode, mWeakShell, aAccessible);
        return PR_TRUE;
      }
      mSelCount++;
    }
  }

  return PR_FALSE;
}

void nsHTMLSelectableAccessible::iterator::Select(PRBool aSelect)
{
  if (mOption)
    mOption->SetSelected(aSelect);
}

nsHTMLSelectableAccessible::nsHTMLSelectableAccessible(nsIDOMNode* aDOMNode, 
                                                       nsIWeakReference* aShell):
nsAccessibleWrap(aDOMNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsHTMLSelectableAccessible, nsAccessible, nsIAccessibleSelectable)


NS_IMETHODIMP nsHTMLSelectableAccessible::ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState)
{
  *aSelState = PR_FALSE;

  nsCOMPtr<nsIDOMHTMLSelectElement> htmlSelect(do_QueryInterface(mDOMNode));
  if (!htmlSelect)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMHTMLOptionsCollection> options;
  htmlSelect->GetOptions(getter_AddRefs(options));
  if (!options)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> tempNode;
  options->Item(aIndex, getter_AddRefs(tempNode));
  nsCOMPtr<nsIDOMHTMLOptionElement> tempOption(do_QueryInterface(tempNode));
  if (!tempOption)
    return NS_ERROR_FAILURE;

  tempOption->GetSelected(aSelState);
  nsresult rv = NS_OK;
  if (eSelection_Add == aMethod && !(*aSelState))
    rv = tempOption->SetSelected(PR_TRUE);
  else if (eSelection_Remove == aMethod && (*aSelState))
    rv = tempOption->SetSelected(PR_FALSE);
  return rv;
}


NS_IMETHODIMP nsHTMLSelectableAccessible::GetSelectedChildren(nsIArray **_retval)
{
  *_retval = nsnull;

  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  if (!accService)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIMutableArray> selectedAccessibles =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_STATE(selectedAccessibles);
  
  nsPresContext *context = GetPresContext();
  if (!context)
    return NS_ERROR_FAILURE;

  nsHTMLSelectableAccessible::iterator iter(this, mWeakShell);
  while (iter.Advance())
    iter.AddAccessibleIfSelected(accService, selectedAccessibles, context);

  PRUint32 uLength = 0;
  selectedAccessibles->GetLength(&uLength); 
  if (uLength != 0) { 
    *_retval = selectedAccessibles;
    NS_ADDREF(*_retval);
  }
  return NS_OK;
}


NS_IMETHODIMP nsHTMLSelectableAccessible::RefSelection(PRInt32 aIndex, nsIAccessible **_retval)
{
  *_retval = nsnull;

  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  if (!accService)
    return NS_ERROR_FAILURE;

  nsPresContext *context = GetPresContext();
  if (!context)
    return NS_ERROR_FAILURE;

  nsHTMLSelectableAccessible::iterator iter(this, mWeakShell);
  while (iter.Advance())
    if (iter.GetAccessibleIfSelected(aIndex, accService, context, _retval))
      return NS_OK;
  
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHTMLSelectableAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  *aSelectionCount = 0;

  nsHTMLSelectableAccessible::iterator iter(this, mWeakShell);
  while (iter.Advance())
    iter.CalcSelectionCount(aSelectionCount);
  return NS_OK;
}

NS_IMETHODIMP nsHTMLSelectableAccessible::AddChildToSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Add, &isSelected);
}

NS_IMETHODIMP nsHTMLSelectableAccessible::RemoveChildFromSelection(PRInt32 aIndex)
{
  PRBool isSelected;
  return ChangeSelection(aIndex, eSelection_Remove, &isSelected);
}

NS_IMETHODIMP nsHTMLSelectableAccessible::IsChildSelected(PRInt32 aIndex, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return ChangeSelection(aIndex, eSelection_GetState, _retval);
}

NS_IMETHODIMP nsHTMLSelectableAccessible::ClearSelection()
{
  nsHTMLSelectableAccessible::iterator iter(this, mWeakShell);
  while (iter.Advance())
    iter.Select(PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP nsHTMLSelectableAccessible::SelectAllSelection(PRBool *_retval)
{
  *_retval = PR_FALSE;
  
  nsCOMPtr<nsIDOMHTMLSelectElement> htmlSelect(do_QueryInterface(mDOMNode));
  if (!htmlSelect)
    return NS_ERROR_FAILURE;

  htmlSelect->GetMultiple(_retval);
  if (*_retval) {
    nsHTMLSelectableAccessible::iterator iter(this, mWeakShell);
    while (iter.Advance())
      iter.Select(PR_TRUE);
  }
  return NS_OK;
}








nsHTMLSelectListAccessible::nsHTMLSelectListAccessible(nsIDOMNode* aDOMNode, 
                                                       nsIWeakReference* aShell)
:nsHTMLSelectableAccessible(aDOMNode, aShell)
{
}






nsresult
nsHTMLSelectListAccessible::GetStateInternal(PRUint32 *aState,
                                             PRUint32 *aExtraState)
{
  nsresult rv = nsHTMLSelectableAccessible::GetStateInternal(aState,
                                                             aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMHTMLSelectElement> select (do_QueryInterface(mDOMNode));
  if (select) {
    if (*aState & nsIAccessibleStates::STATE_FOCUSED) {
      
      
      nsCOMPtr<nsIDOMNode> focusedOption;
      nsHTMLSelectOptionAccessible::GetFocusedOptionNode(mDOMNode, 
                                                         getter_AddRefs(focusedOption));
      if (focusedOption) { 
        *aState &= ~nsIAccessibleStates::STATE_FOCUSED;
      }
    }
    PRBool multiple;
    select->GetMultiple(&multiple);
    if ( multiple )
      *aState |= nsIAccessibleStates::STATE_MULTISELECTABLE |
                 nsIAccessibleStates::STATE_EXTSELECTABLE;
  }

  return NS_OK;
}

nsresult
nsHTMLSelectListAccessible::GetRoleInternal(PRUint32 *aRole)
{
  if (nsAccUtils::Role(mParent) == nsIAccessibleRole::ROLE_COMBOBOX)
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
  else
    *aRole = nsIAccessibleRole::ROLE_LISTBOX;

  return NS_OK;
}

already_AddRefed<nsIAccessible>
nsHTMLSelectListAccessible::AccessibleForOption(nsIAccessibilityService *aAccService,
                                                nsIContent *aContent,
                                                nsIAccessible *aLastGoodAccessible,
                                                PRInt32 *aChildCount)
{
  nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(aContent));
  NS_ASSERTION(domNode, "DOM node is null");
  
  nsCOMPtr<nsIAccessible> accessible;
  aAccService->GetAccessibleInWeakShell(domNode, mWeakShell, getter_AddRefs(accessible));
  nsRefPtr<nsAccessible> acc(nsAccUtils::QueryAccessible(accessible));
  if (!acc)
    return nsnull;

  ++ *aChildCount;
  acc->SetParent(this);
  nsRefPtr<nsAccessible> prevAcc =
    nsAccUtils::QueryAccessible(aLastGoodAccessible);
  if (prevAcc)
    prevAcc->SetNextSibling(accessible);

  if (!mFirstChild)
    mFirstChild = accessible;

  return accessible.forget();
}

already_AddRefed<nsIAccessible>
nsHTMLSelectListAccessible::CacheOptSiblings(nsIAccessibilityService *aAccService,
                                             nsIContent *aParentContent,
                                             nsIAccessible *aLastGoodAccessible,
                                             PRInt32 *aChildCount)
{
  

  PRUint32 numChildren = aParentContent->GetChildCount();
  nsCOMPtr<nsIAccessible> lastGoodAccessible(aLastGoodAccessible);
  nsCOMPtr<nsIAccessible> newAccessible;

  for (PRUint32 count = 0; count < numChildren; count ++) {
    nsIContent *childContent = aParentContent->GetChildAt(count);
    if (!childContent->IsNodeOfType(nsINode::eHTML)) {
      continue;
    }
    nsCOMPtr<nsIAtom> tag = childContent->Tag();
    if (tag == nsAccessibilityAtoms::option || tag == nsAccessibilityAtoms::optgroup) {
      newAccessible = AccessibleForOption(aAccService,
                                           childContent,
                                           lastGoodAccessible,
                                           aChildCount);
      if (newAccessible) {
        lastGoodAccessible = newAccessible;
      }
      if (tag == nsAccessibilityAtoms::optgroup) {
        newAccessible = CacheOptSiblings(aAccService, childContent,
                                         lastGoodAccessible, aChildCount);
        if (newAccessible) {
          lastGoodAccessible = newAccessible;
        }
      }
    }
  }

  if (lastGoodAccessible) {
    nsRefPtr<nsAccessible> lastAcc =
      nsAccUtils::QueryAccessible(lastGoodAccessible);
    lastAcc->SetNextSibling(nsnull);
  }

  return lastGoodAccessible.forget();
}







void nsHTMLSelectListAccessible::CacheChildren()
{
  
  

  nsCOMPtr<nsIContent> selectContent(do_QueryInterface(mDOMNode));
  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  if (!selectContent || !accService) {
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  if (mAccChildCount != eChildCountUninitialized) {
    return;
  }

  mAccChildCount = 0; 
  PRInt32 childCount = 0;
  nsCOMPtr<nsIAccessible> lastGoodAccessible =
    CacheOptSiblings(accService, selectContent, nsnull, &childCount);
  mAccChildCount = childCount;
}




nsHTMLSelectOptionAccessible::nsHTMLSelectOptionAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsHyperTextAccessibleWrap(aDOMNode, aShell)
{
  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  nsCOMPtr<nsIDOMNode> parentNode;
  aDOMNode->GetParentNode(getter_AddRefs(parentNode));
  nsCOMPtr<nsIAccessible> parentAccessible;
  if (parentNode) {
    
    
    
    
    
    accService->GetAccessibleInWeakShell(parentNode, mWeakShell, getter_AddRefs(parentAccessible));
    if (parentAccessible) {
      if (nsAccUtils::RoleInternal(parentAccessible) ==
          nsIAccessibleRole::ROLE_COMBOBOX) {
        nsCOMPtr<nsIAccessible> comboAccessible(parentAccessible);
        comboAccessible->GetLastChild(getter_AddRefs(parentAccessible));
      }
    }
  }
  SetParent(parentAccessible);
}


nsresult
nsHTMLSelectOptionAccessible::GetRoleInternal(PRUint32 *aRole)
{
  if (nsAccUtils::Role(mParent) == nsIAccessibleRole::ROLE_COMBOBOX_LIST)
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_OPTION;
  else
    *aRole = nsIAccessibleRole::ROLE_OPTION;

  return NS_OK;
}

nsresult
nsHTMLSelectOptionAccessible::GetNameInternal(nsAString& aName)
{
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::label, aName);
  if (!aName.IsEmpty())
    return NS_OK;
  
  
  
  nsCOMPtr<nsIContent> text = content->GetChildAt(0);
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

nsresult
nsHTMLSelectOptionAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;  
  }

  nsresult rv = nsHyperTextAccessibleWrap::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> parentNode;
  mDOMNode->GetParentNode(getter_AddRefs(parentNode));
  nsCOMPtr<nsIDOMElement> parentElement(do_QueryInterface(parentNode));
  NS_ENSURE_TRUE(parentElement, NS_ERROR_FAILURE);
  nsAutoString parentTagName;
  parentNode->GetLocalName(parentTagName);

  PRInt32 level = parentTagName.LowerCaseEqualsLiteral("optgroup") ? 2 : 1;
  if (level == 1 && nsAccUtils::Role(this) != nsIAccessibleRole::ROLE_HEADING) {
    level = 0; 
  }

  nsAutoString tagName;
  mDOMNode->GetLocalName(tagName);  
  nsCOMPtr<nsIDOMNodeList> siblings;
  parentElement->GetElementsByTagName(tagName, getter_AddRefs(siblings));
  PRInt32 posInSet = 0;
  PRUint32 setSize = 0;
  if (siblings) {
    siblings->GetLength(&setSize);
    nsCOMPtr<nsIDOMNode> itemNode;
    while (NS_SUCCEEDED(siblings->Item(posInSet ++, getter_AddRefs(itemNode))) &&
           itemNode != mDOMNode) {
      
    }
  }

  nsAccUtils::SetAccGroupAttrs(aAttributes, level, posInSet,
                               static_cast<PRInt32>(setSize));
  return  NS_OK;
}

nsIFrame* nsHTMLSelectOptionAccessible::GetBoundsFrame()
{
  PRUint32 state;
  nsCOMPtr<nsIContent> content = GetSelectState(&state);
  if (state & nsIAccessibleStates::STATE_COLLAPSED) {
    nsCOMPtr<nsIPresShell> presShell(GetPresShell());
    if (!presShell) {
      return nsnull;
    }
    return presShell->GetPrimaryFrameFor(content);
  }

  return nsAccessible::GetBoundsFrame();
}









nsresult
nsHTMLSelectOptionAccessible::GetStateInternal(PRUint32 *aState,
                                               PRUint32 *aExtraState)
{
  
  
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  PRUint32 selectState, selectExtState;
  nsCOMPtr<nsIContent> selectContent = GetSelectState(&selectState,
                                                      &selectExtState);
  if (selectState & nsIAccessibleStates::STATE_INVISIBLE) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> selectNode = do_QueryInterface(selectContent); 
  NS_ENSURE_TRUE(selectNode, NS_ERROR_FAILURE);

  
  if (0 == (*aState & nsIAccessibleStates::STATE_UNAVAILABLE)) {
    *aState |= (nsIAccessibleStates::STATE_FOCUSABLE |
                nsIAccessibleStates::STATE_SELECTABLE);
    
    
    
    
    
    
    
    nsCOMPtr<nsIDOMNode> focusedOptionNode;
    GetFocusedOptionNode(selectNode, getter_AddRefs(focusedOptionNode));
    if (focusedOptionNode == mDOMNode) {
      *aState |= nsIAccessibleStates::STATE_FOCUSED;
    }
  }

  
  PRBool isSelected = PR_FALSE;
  nsCOMPtr<nsIDOMHTMLOptionElement> option (do_QueryInterface(mDOMNode));
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
    
    nsCOMPtr<nsIAccessible> listAccessible = GetParent();
    if (listAccessible) {
      PRInt32 optionX, optionY, optionWidth, optionHeight;
      PRInt32 listX, listY, listWidth, listHeight;
      GetBounds(&optionX, &optionY, &optionWidth, &optionHeight);
      listAccessible->GetBounds(&listX, &listY, &listWidth, &listHeight);
      if (optionY < listY || optionY + optionHeight > listY + listHeight) {
        *aState |= nsIAccessibleStates::STATE_OFFSCREEN;
      }
    }
  }
 
  return NS_OK;
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
    nsCOMPtr<nsIDOMHTMLOptionElement> newHTMLOption(do_QueryInterface(mDOMNode));
    if (!newHTMLOption) 
      return NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIDOMNode> oldHTMLOptionNode, selectNode;
    nsCOMPtr<nsIAccessible> parent(GetParent());
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(parent));
    NS_ASSERTION(accessNode, "Unable to QI to nsIAccessNode");
    accessNode->GetDOMNode(getter_AddRefs(selectNode));
    GetFocusedOptionNode(selectNode, getter_AddRefs(oldHTMLOptionNode));
    nsCOMPtr<nsIDOMHTMLOptionElement> oldHTMLOption(do_QueryInterface(oldHTMLOptionNode));
    if (oldHTMLOption)
      oldHTMLOption->SetSelected(PR_FALSE);
    
    newHTMLOption->SetSelected(PR_TRUE);

    
    
    nsCOMPtr<nsIDOMNode> testSelectNode;
    nsCOMPtr<nsIDOMNode> thisNode(do_QueryInterface(mDOMNode));
    do {
      thisNode->GetParentNode(getter_AddRefs(testSelectNode));
      nsCOMPtr<nsIDOMHTMLSelectElement> selectControl(do_QueryInterface(testSelectNode));
      if (selectControl)
        break;
      thisNode = testSelectNode;
    } while (testSelectNode);

    nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
    nsCOMPtr<nsIContent> selectContent(do_QueryInterface(testSelectNode));
    nsCOMPtr<nsIDOMHTMLOptionElement> option(do_QueryInterface(mDOMNode));

    if (!testSelectNode || !selectContent || !presShell || !option) 
      return NS_ERROR_FAILURE;

    nsIFrame *selectFrame = presShell->GetPrimaryFrameFor(selectContent);
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






nsresult nsHTMLSelectOptionAccessible::GetFocusedOptionNode(nsIDOMNode *aListNode, 
                                                            nsIDOMNode **aFocusedOptionNode)
{
  *aFocusedOptionNode = nsnull;
  NS_ASSERTION(aListNode, "Called GetFocusedOptionNode without a valid list node");

  nsCOMPtr<nsIContent> content(do_QueryInterface(aListNode));
  nsCOMPtr<nsIDocument> document = content->GetDocument();
  nsIPresShell *shell = nsnull;
  if (document)
    shell = document->GetPrimaryShell();
  if (!shell)
    return NS_ERROR_FAILURE;

  nsIFrame *frame = shell->GetPrimaryFrameFor(content);
  if (!frame)
    return NS_ERROR_FAILURE;

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
    rv = options->Item(focusedOptionIndex, aFocusedOptionNode);
  }

  return rv;
}

void nsHTMLSelectOptionAccessible::SelectionChangedIfOption(nsIContent *aPossibleOption)
{
  if (!aPossibleOption || aPossibleOption->Tag() != nsAccessibilityAtoms::option ||
      !aPossibleOption->IsNodeOfType(nsINode::eHTML)) {
    return;
  }

  nsCOMPtr<nsIDOMNode> optionNode(do_QueryInterface(aPossibleOption));
  NS_ASSERTION(optionNode, "No option node for nsIContent with option tag!");

  nsCOMPtr<nsIAccessible> multiSelect =
    nsAccUtils::GetMultiSelectFor(optionNode);
  if (!multiSelect)
    return;

  nsCOMPtr<nsIAccessible> optionAccessible;
  GetAccService()->GetAccessibleFor(optionNode,
                                    getter_AddRefs(optionAccessible));
  if (!optionAccessible)
    return;

  nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_SELECTION_WITHIN,
                           multiSelect);

  PRUint32 state = nsAccUtils::State(optionAccessible);
  PRUint32 eventType;
  if (state & nsIAccessibleStates::STATE_SELECTED) {
    eventType = nsIAccessibleEvent::EVENT_SELECTION_ADD;
  }
  else {
    eventType = nsIAccessibleEvent::EVENT_SELECTION_REMOVE;
  }

  nsAccUtils::FireAccEvent(eventType, optionAccessible);
}

nsIContent* nsHTMLSelectOptionAccessible::GetSelectState(PRUint32* aState,
                                                         PRUint32* aExtraState)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  while (content && content->Tag() != nsAccessibilityAtoms::select) {
    content = content->GetParent();
  }

  nsCOMPtr<nsIDOMNode> selectNode(do_QueryInterface(content));
  if (selectNode) {
    nsCOMPtr<nsIAccessibilityService> accService = GetAccService();
    if (accService) {
      nsCOMPtr<nsIAccessible> selAcc;
      accService->GetAccessibleFor(selectNode, getter_AddRefs(selAcc));
      if (selAcc) {
        selAcc->GetState(aState, aExtraState);
        return content;
      }
    }
  }
  return nsnull; 
}




nsHTMLSelectOptGroupAccessible::nsHTMLSelectOptGroupAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsHTMLSelectOptionAccessible(aDOMNode, aShell)
{
}

nsresult
nsHTMLSelectOptGroupAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_HEADING;
  return NS_OK;
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

void nsHTMLSelectOptGroupAccessible::CacheChildren()
{
  if (!mWeakShell) {
    
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  if (mAccChildCount == eChildCountUninitialized) {
    
    
    
    
    mAccChildCount = 0;
    SetFirstChild(nsnull);
  }
}







nsHTMLComboboxAccessible::nsHTMLComboboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsAccessibleWrap(aDOMNode, aShell)
{
}


nsresult
nsHTMLComboboxAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_COMBOBOX;
  return NS_OK;
}

void nsHTMLComboboxAccessible::CacheChildren()
{
  if (!mWeakShell) {
    
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  if (mAccChildCount == eChildCountUninitialized) {
    mAccChildCount = 0;
#ifdef COMBO_BOX_WITH_THREE_CHILDREN
    
    
    nsHTMLComboboxTextFieldAccessible* textFieldAccessible = 
      new nsHTMLComboboxTextFieldAccessible(this, mDOMNode, mWeakShell);
    SetFirstChild(textFieldAccessible);
    if (!textFieldAccessible) {
      return;
    }
    textFieldAccessible->SetParent(this);
    textFieldAccessible->Init();
    mAccChildCount = 1;  

    nsHTMLComboboxButtonAccessible* buttonAccessible =
      new nsHTMLComboboxButtonAccessible(mParent, mDOMNode, mWeakShell);
    textFieldAccessible->SetNextSibling(buttonAccessible);
    if (!buttonAccessible) {
      return;
    }

    buttonAccessible->SetParent(this);
    buttonAccessible->Init();
    mAccChildCount = 2; 
#endif

    nsIFrame *frame = GetFrame();
    if (!frame) {
      return;
    }
    nsIComboboxControlFrame *comboFrame = do_QueryFrame(frame);
    if (!comboFrame) {
      return;
    }
    nsIFrame *listFrame = comboFrame->GetDropDown();
    if (!listFrame) {
      return;
    }

    if (!mListAccessible) {
      mListAccessible = 
        new nsHTMLComboboxListAccessible(mParent, mDOMNode, mWeakShell);
      if (!mListAccessible)
        return;

      mListAccessible->Init();
    }

#ifdef COMBO_BOX_WITH_THREE_CHILDREN
    buttonAccessible->SetNextSibling(mListAccessible);
#else
    SetFirstChild(mListAccessible);
#endif

    mListAccessible->SetParent(this);
    mListAccessible->SetNextSibling(nsnull);

    ++ mAccChildCount;  
  }
}

nsresult
nsHTMLComboboxAccessible::Shutdown()
{
  nsAccessibleWrap::Shutdown();

  if (mListAccessible) {
    mListAccessible->Shutdown();
    mListAccessible = nsnull;
  }
  return NS_OK;
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
  
  nsCOMPtr<nsIAccessible> optionAccessible = GetFocusedOptionAccessible();
  return optionAccessible ? optionAccessible->GetDescription(aDescription) : NS_OK;
}

already_AddRefed<nsIAccessible>
nsHTMLComboboxAccessible::GetFocusedOptionAccessible()
{
  if (!mWeakShell) {
    return nsnull;  
  }
  nsCOMPtr<nsIDOMNode> focusedOptionNode;
  nsHTMLSelectOptionAccessible::GetFocusedOptionNode(mDOMNode, getter_AddRefs(focusedOptionNode));
  nsIAccessibilityService *accService = GetAccService();
  if (!focusedOptionNode || !accService) {
    return nsnull;
  }

  nsIAccessible *optionAccessible;
  accService->GetAccessibleInWeakShell(focusedOptionNode, mWeakShell, 
                                       &optionAccessible);
  return optionAccessible;
}






NS_IMETHODIMP nsHTMLComboboxAccessible::GetValue(nsAString& aValue)
{
  
  nsCOMPtr<nsIAccessible> optionAccessible = GetFocusedOptionAccessible();
  NS_ENSURE_TRUE(optionAccessible, NS_ERROR_FAILURE);
  return optionAccessible->GetName(aValue);
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


#ifdef COMBO_BOX_WITH_THREE_CHILDREN



nsHTMLComboboxTextFieldAccessible::nsHTMLComboboxTextFieldAccessible(nsIAccessible* aParent, 
                                                                     nsIDOMNode* aDOMNode, 
                                                                     nsIWeakReference* aShell):
nsHTMLTextFieldAccessible(aDOMNode, aShell)
{
}

NS_IMETHODIMP nsHTMLComboboxTextFieldAccessible::GetUniqueID(void **aUniqueID)
{
  
  *aUniqueID = static_cast<void*>(this);
  return NS_OK;
}





void nsHTMLComboboxTextFieldAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame)
{
  
  nsIFrame* frame = nsAccessible::GetBoundsFrame();
  if (!frame)
    return;

  frame = frame->GetFirstChild(nsnull);
  *aBoundingFrame = frame;

  aBounds = frame->GetRect();
}

void nsHTMLComboboxTextFieldAccessible::CacheChildren()
{
  
  
  
  if (!mWeakShell) {
    
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  
  if (mAccChildCount == eChildCountUninitialized) {
    mAccChildCount = 0; 
    nsAccessibleTreeWalker walker(mWeakShell, mDOMNode, PR_TRUE);
    
    
    
    walker.mState.frame = GetFrame();

    walker.GetFirstChild();
    SetFirstChild(walker.mState.accessible);
    nsRefPtr<nsAccessible> child =
      nsAccUtils::QueryAccessible(walker.mState.accessible);
    child->SetParent(this);
    child->SetNextSibling(nsnull);
    mAccChildCount = 1;
  }
}




nsHTMLComboboxButtonAccessible::nsHTMLComboboxButtonAccessible(nsIAccessible* aParent, 
                                                           nsIDOMNode* aDOMNode, 
                                                           nsIWeakReference* aShell):
nsLeafAccessible(aDOMNode, aShell)
{
}


NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetNumActions(PRUint8 *aNumActions)
{
  *aNumActions = 1;
  return NS_OK;
}






NS_IMETHODIMP nsHTMLComboboxButtonAccessible::DoAction(PRUint8 aIndex)
{
  nsIFrame* frame = nsAccessible::GetBoundsFrame();
  nsPresContext *context = GetPresContext();
  if (!frame || !context)
    return NS_ERROR_FAILURE;

  frame = frame->GetFirstChild(nsnull)->GetNextSibling();

  
  if (aIndex == eAction_Click) {
    nsCOMPtr<nsIDOMHTMLInputElement>
      element(do_QueryInterface(frame->GetContent()));
    if (element)
    {
       element->Click();
       return NS_OK;
    }
    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_INVALID_ARG;
}







NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  nsIFrame *boundsFrame = GetBoundsFrame();
  nsIComboboxControlFrame* comboFrame;
  boundsFrame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);
  if (!comboFrame)
    return NS_ERROR_FAILURE;

  if (comboFrame->IsDroppedDown())
    aName.AssignLiteral("close"); 
  else
    aName.AssignLiteral("open");

  return NS_OK;
}

NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetUniqueID(void **aUniqueID)
{
  
  *aUniqueID = static_cast<void*>(this);
  return NS_OK;
}





void nsHTMLComboboxButtonAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame)
{
  
  
  nsIFrame *frame = nsAccessible::GetBoundsFrame();
  *aBoundingFrame = frame;
  nsPresContext *context = GetPresContext();
  if (!frame || !context)
    return;

  aBounds = frame->GetFirstChild(nsnull)->GetNextSibling()->GetRect();
    
}


nsresult
nsHTMLComboboxButtonAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
}


NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetParent(nsIAccessible **aParent)
{   
  NS_IF_ADDREF(*aParent = mParent);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLComboboxButtonAccessible::GetName(nsAString& aName)
{
  
  aName.Truncate();
  return GetActionName(eAction_Click, aName);
}








nsresult
nsHTMLComboboxButtonAccessible::GetStateInternal(PRUint32 *aState,
                                                 PRUint32 *aExtraState)
{
  
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsIFrame *boundsFrame = GetBoundsFrame();
  nsIComboboxControlFrame* comboFrame = nsnull;
  if (boundsFrame)
    boundsFrame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);

  if (!comboFrame) {
    *aState |= nsIAccessibleStates::STATE_INVISIBLE;
  }
  else {
    *aState |= nsIAccessibleStates::STATE_FOCUSABLE;
    if (comboFrame->IsDroppedDown()) {
      *aState |= nsIAccessibleStates::STATE_PRESSED;
    }
  }
 
  return NS_OK;
}
#endif



nsHTMLComboboxListAccessible::nsHTMLComboboxListAccessible(nsIAccessible *aParent,
                                                           nsIDOMNode* aDOMNode,
                                                           nsIWeakReference* aShell):
nsHTMLSelectListAccessible(aDOMNode, aShell)
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


NS_IMETHODIMP nsHTMLComboboxListAccessible::GetParent(nsIAccessible **aParent)
{
  NS_IF_ADDREF(*aParent = mParent);
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

  nsCOMPtr<nsIAccessible> comboAccessible;
  GetParent(getter_AddRefs(comboAccessible));
  if (!comboAccessible) {
    return;
  }
  if (0 == (nsAccUtils::State(comboAccessible) & nsIAccessibleStates::STATE_COLLAPSED)) {
    nsHTMLSelectListAccessible::GetBoundsRect(aBounds, aBoundingFrame);
    return;
  }
   
  nsCOMPtr<nsIDOMNode> child;
  mDOMNode->GetFirstChild(getter_AddRefs(child));

  
  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  if (!shell) {
    return;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(child));
  if (!content) {
    return;
  }
  nsIFrame* frame = shell->GetPrimaryFrameFor(content);
  if (!frame) {
    *aBoundingFrame = nsnull;
    return;
  }

  *aBoundingFrame = frame->GetParent();
  aBounds = (*aBoundingFrame)->GetRect();
}
