





































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
    aSelectedAccessibles->AppendElement(NS_STATIC_CAST(nsISupports*, tempAccess), PR_FALSE);
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






NS_IMETHODIMP nsHTMLSelectListAccessible::GetState(PRUint32 *_retval)
{
  nsHTMLSelectableAccessible::GetState(_retval);
  nsCOMPtr<nsIDOMHTMLSelectElement> select (do_QueryInterface(mDOMNode));
  if ( select ) {
    PRBool multiple;
    select->GetMultiple(&multiple);
    if ( multiple )
      *_retval |= nsIAccessibleStates::STATE_MULTISELECTABLE |
                  nsIAccessibleStates::STATE_EXTSELECTABLE;
  }

  return NS_OK;
}

NS_IMETHODIMP nsHTMLSelectListAccessible::GetRole(PRUint32 *aRole)
{
  if (mParent && Role(mParent) == nsIAccessibleRole::ROLE_COMBOBOX) {
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LIST;
  }
  else {
    *aRole = nsIAccessibleRole::ROLE_LIST;
  }
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
  nsCOMPtr<nsPIAccessible> privateAccessible(do_QueryInterface(accessible));
  if (!privateAccessible) {
    return nsnull;
  }

  ++ *aChildCount;
  privateAccessible->SetParent(this);
  nsCOMPtr<nsPIAccessible> privatePrevAccessible(do_QueryInterface(aLastGoodAccessible));
  if (privatePrevAccessible) {
    privatePrevAccessible->SetNextSibling(accessible);
  }
  if (!mFirstChild) {
    mFirstChild = accessible;
  }
  nsIAccessible *returnAccessible = accessible;
  NS_ADDREF(returnAccessible);
  return returnAccessible;
}

already_AddRefed<nsIAccessible>
nsHTMLSelectListAccessible::CacheOptSiblings(nsIAccessibilityService *aAccService,
                                             nsIContent *aParentContent,
                                             nsIAccessible *aLastGoodAccessible,
                                             PRInt32 *aChildCount)
{
  

  PRUint32 numChildren = aParentContent->GetChildCount();
  nsCOMPtr<nsIAccessible> lastGoodAccessible(aLastGoodAccessible);

  for (PRUint32 count = 0; count < numChildren; count ++) {
    nsIContent *childContent = aParentContent->GetChildAt(count);
    if (!childContent->IsNodeOfType(nsINode::eHTML)) {
      continue;
    }
    nsCOMPtr<nsIAtom> tag = childContent->Tag();
    if (tag == nsAccessibilityAtoms::option || tag == nsAccessibilityAtoms::optgroup) {
      lastGoodAccessible = AccessibleForOption(aAccService,
                                               childContent,
                                               lastGoodAccessible,
                                               aChildCount);
      if (tag == nsAccessibilityAtoms::optgroup) {
        lastGoodAccessible = CacheOptSiblings(aAccService, childContent,
                                              lastGoodAccessible,
                                              aChildCount);
      }
    }
  }
  if (lastGoodAccessible) {
    nsCOMPtr<nsPIAccessible> privateLastAcc =
      do_QueryInterface(lastGoodAccessible);
    privateLastAcc->SetNextSibling(nsnull);
    NS_ADDREF(aLastGoodAccessible = lastGoodAccessible);
  }
  return aLastGoodAccessible;
}







void nsHTMLSelectListAccessible::CacheChildren()
{
  
  

  nsCOMPtr<nsIContent> selectContent(do_QueryInterface(mDOMNode));
  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  if (!selectContent || !accService) {
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  PRInt32 childCount = 0;
  nsCOMPtr<nsIAccessible> lastGoodAccessible =
    CacheOptSiblings(accService, selectContent, nsnull, &childCount);
  mAccChildCount = childCount;
}




nsHTMLSelectOptionAccessible::nsHTMLSelectOptionAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsHyperTextAccessible(aDOMNode, aShell)
{
  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  nsCOMPtr<nsIDOMNode> parentNode;
  aDOMNode->GetParentNode(getter_AddRefs(parentNode));
  nsCOMPtr<nsIAccessible> parentAccessible;
  if (parentNode) {
    
    
    
    
    
    accService->GetAccessibleInWeakShell(parentNode, mWeakShell, getter_AddRefs(parentAccessible));
    if (parentAccessible) {
      PRUint32 role;
      parentAccessible->GetRole(&role);
      if (role == nsIAccessibleRole::ROLE_COMBOBOX) {
        nsCOMPtr<nsIAccessible> comboAccessible(parentAccessible);
        comboAccessible->GetLastChild(getter_AddRefs(parentAccessible));
      }
    }
  }
  SetParent(parentAccessible);
}


NS_IMETHODIMP nsHTMLSelectOptionAccessible::GetRole(PRUint32 *aRole)
{
  if (mParent && Role(mParent) == nsIAccessibleRole::ROLE_COMBOBOX_LIST) {
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LISTITEM;
  }
  else {
    *aRole = nsIAccessibleRole::ROLE_COMBOBOX_LISTITEM;
  }
  return NS_OK;
}




NS_IMETHODIMP nsHTMLSelectOptionAccessible::GetName(nsAString& aName)
{
  
  
  nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(mDOMNode));
  NS_ASSERTION(domElement, "No domElement for accessible DOM node!");
  nsresult rv = domElement->GetAttribute(NS_LITERAL_STRING("label"), aName) ;

  if (NS_SUCCEEDED(rv) && !aName.IsEmpty() ) {
    return NS_OK;
  }
  
  
  
  nsCOMPtr<nsIDOMNode> child;
  mDOMNode->GetFirstChild(getter_AddRefs(child));

  if (child) {
    nsCOMPtr<nsIContent> text = do_QueryInterface(child);
    if (text && text->IsNodeOfType(nsINode::eTEXT)) {
      nsAutoString txtValue;
      rv = AppendFlatStringFromContentNode(text, &txtValue);
      if (NS_SUCCEEDED(rv)) {
        
        txtValue.CompressWhitespace();
        aName.Assign(txtValue);
        return NS_OK;
      }
    }
  }
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsHTMLSelectOptionAccessible::GetAttributes(nsIPersistentProperties **aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  NS_ENSURE_TRUE(mDOMNode, NS_ERROR_FAILURE);

  nsresult rv = nsHyperTextAccessible::GetAttributes(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  nsCOMPtr<nsIContent> parentContent = content->GetParent();
  NS_ENSURE_TRUE(parentContent, NS_ERROR_FAILURE);

  PRUint32 level =
    parentContent->NodeInfo()->Equals(nsAccessibilityAtoms::optgroup) ? 2 : 1;
  PRUint32 childCount = parentContent->GetChildCount();
  PRUint32 indexOf = parentContent->IndexOf(content);

  nsAccessibilityUtils::
    SetAccGroupAttrs(*aAttributes, level, indexOf + 1, childCount);
  return  NS_OK;
}

nsIFrame* nsHTMLSelectOptionAccessible::GetBoundsFrame()
{
  nsCOMPtr<nsIContent> selectContent(do_QueryInterface(mDOMNode));

  while (selectContent && selectContent->Tag() != nsAccessibilityAtoms::select) {
    selectContent = selectContent->GetParent();
  }

  nsCOMPtr<nsIDOMNode> selectNode(do_QueryInterface(selectContent));
  if (selectNode) {
    nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
    nsCOMPtr<nsIAccessible> selAcc;
    accService->GetAccessibleFor(selectNode, getter_AddRefs(selAcc));
    if (selAcc) {
      PRUint32 state;
      selAcc->GetFinalState(&state);
      if (state & nsIAccessibleStates::STATE_COLLAPSED) {
        nsCOMPtr<nsIPresShell> presShell(GetPresShell());
        if (!presShell) {
          return nsnull;
        }
        return presShell->GetPrimaryFrameFor(selectContent);
      }
    }
  }

  return nsAccessible::GetBoundsFrame();
}









NS_IMETHODIMP nsHTMLSelectOptionAccessible::GetState(PRUint32 *_retval)
{
  *_retval = 0;
  nsCOMPtr<nsIDOMNode> focusedOptionNode, parentNode;
  
  nsCOMPtr<nsIDOMNode> thisNode(do_QueryInterface(mDOMNode));
  do {
    thisNode->GetParentNode(getter_AddRefs(parentNode));
    nsCOMPtr<nsIDOMHTMLSelectElement> selectControl(do_QueryInterface(parentNode));
    if (selectControl) {
      break;
    }
    thisNode = parentNode;
  } while (parentNode);
  if (!parentNode) {
    return NS_ERROR_FAILURE;
  }
  
  
  GetFocusedOptionNode(parentNode, getter_AddRefs(focusedOptionNode));
  if (focusedOptionNode == mDOMNode)
    *_retval |= nsIAccessibleStates::STATE_FOCUSED;

  
  nsCOMPtr<nsIDOMHTMLOptionElement> option (do_QueryInterface(mDOMNode));
  if ( option ) {
    PRBool isSelected = PR_FALSE;
    option->GetSelected(&isSelected);
    if ( isSelected ) 
      *_retval |= nsIAccessibleStates::STATE_SELECTED;
  }

  *_retval |= nsIAccessibleStates::STATE_SELECTABLE |
              nsIAccessibleStates::STATE_FOCUSABLE;

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
    nsIComboboxControlFrame *comboBoxFrame = nsnull;
    CallQueryInterface(selectFrame, &comboBoxFrame);
    if (comboBoxFrame) {
      nsIFrame *listFrame = comboBoxFrame->GetDropDown();
      if (comboBoxFrame->IsDroppedDown() && listFrame) {
        
        nsIListControlFrame *listControlFrame = nsnull;
        listFrame->QueryInterface(NS_GET_IID(nsIListControlFrame), (void**)&listControlFrame);
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
    shell = document->GetShellAt(0);
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
    nsIListControlFrame *listFrame = nsnull;
    frame->QueryInterface(NS_GET_IID(nsIListControlFrame), (void**)&listFrame);
    if (listFrame) {
      
      
      
      
      focusedOptionIndex = listFrame->GetSelectedIndex();
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

  nsCOMPtr<nsIAccessible> multiSelect = GetMultiSelectFor(optionNode);
  nsCOMPtr<nsPIAccessible> privateMultiSelect = do_QueryInterface(multiSelect);
  if (!privateMultiSelect) {
    return;
  }

  nsCOMPtr<nsIAccessibilityService> accService = 
    do_GetService("@mozilla.org/accessibilityService;1");
  nsCOMPtr<nsIAccessible> optionAccessible;
  accService->GetAccessibleFor(optionNode, getter_AddRefs(optionAccessible));
  if (!optionAccessible) {
    return;
  }

  privateMultiSelect->FireToolkitEvent(nsIAccessibleEvent::EVENT_SELECTION_WITHIN,
                      multiSelect, nsnull);
  PRUint32 state;
  optionAccessible->GetFinalState(&state);
  PRUint32 eventType = (state & nsIAccessibleStates::STATE_SELECTED) ?
                       nsIAccessibleEvent::EVENT_SELECTION_ADD :
                       nsIAccessibleEvent::EVENT_SELECTION_REMOVE; 
  privateMultiSelect->FireToolkitEvent(eventType, optionAccessible, nsnull);
}




nsHTMLSelectOptGroupAccessible::nsHTMLSelectOptGroupAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsHTMLSelectOptionAccessible(aDOMNode, aShell)
{
}






NS_IMETHODIMP nsHTMLSelectOptGroupAccessible::GetState(PRUint32 *_retval)
{
  nsHTMLSelectOptionAccessible::GetState(_retval);
  *_retval &= ~(nsIAccessibleStates::STATE_FOCUSABLE |
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







nsHTMLComboboxAccessible::nsHTMLComboboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsAccessibleWrap(aDOMNode, aShell)
{
}


NS_IMETHODIMP nsHTMLComboboxAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_COMBOBOX;
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
    nsIComboboxControlFrame *comboFrame = nsnull;
    frame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);
    if (!comboFrame) {
      return;
    }
    nsIFrame *listFrame = comboFrame->GetDropDown();
    if (!listFrame) {
      return;
    }

    nsHTMLComboboxListAccessible* listAccessible = 
      new nsHTMLComboboxListAccessible(mParent, mDOMNode, listFrame, mWeakShell);
#ifdef COMBO_BOX_WITH_THREE_CHILDREN
    buttonAccessible->SetNextSibling(listAccessible);
#else
    SetFirstChild(listAccessible);
#endif
    if (!listAccessible) {
      return;
    }

    listAccessible->SetParent(this);
    listAccessible->SetNextSibling(nsnull);
    listAccessible->Init();

    ++ mAccChildCount;  
  }
}










NS_IMETHODIMP nsHTMLComboboxAccessible::GetState(PRUint32 *_retval)
{
  
  nsAccessible::GetState(_retval);

  nsIFrame *frame = GetBoundsFrame();
  nsIComboboxControlFrame *comboFrame = nsnull;
  frame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);

  if (comboFrame && comboFrame->IsDroppedDown())
    *_retval |= nsIAccessibleStates::STATE_EXPANDED;
  else
    *_retval |= nsIAccessibleStates::STATE_COLLAPSED;

  *_retval |= nsIAccessibleStates::STATE_HASPOPUP |
              nsIAccessibleStates::STATE_READONLY |
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
  nsIComboboxControlFrame *comboFrame = nsnull;
  frame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);
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
  nsIComboboxControlFrame *comboFrame = nsnull;
  frame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);
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
  
  *aUniqueID = NS_STATIC_CAST(void*, this);
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
    nsAccessibleTreeWalker walker(mWeakShell, mDOMNode, PR_TRUE);
    
    
    
    walker.mState.frame = GetFrame();

    walker.GetFirstChild();
    SetFirstChild(walker.mState.accessible);
    nsCOMPtr<nsPIAccessible> privateChild = 
      do_QueryInterface(walker.mState.accessible);
    privateChild->SetParent(this);
    privateChild->SetNextSibling(nsnull);
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
  
  *aUniqueID = NS_STATIC_CAST(void*, this);
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


NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
}


NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetParent(nsIAccessible **aParent)
{   
  NS_IF_ADDREF(*aParent = mParent);
  return NS_OK;
}




NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetName(nsAString& aName)
{
  return GetActionName(eAction_Click, aName);
}







NS_IMETHODIMP nsHTMLComboboxButtonAccessible::GetState(PRUint32 *_retval)
{
  
  nsAccessible::GetState(_retval);

  nsIFrame *boundsFrame = GetBoundsFrame();
  nsIComboboxControlFrame* comboFrame;
  boundsFrame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);
  if (!comboFrame)
    return NS_ERROR_FAILURE;

  if (comboFrame->IsDroppedDown())
    *_retval |= nsIAccessibleStates::STATE_PRESSED;

  *_retval |= nsIAccessibleStates::STATE_FOCUSABLE;

  return NS_OK;
}
#endif



nsHTMLComboboxListAccessible::nsHTMLComboboxListAccessible(nsIAccessible *aParent,
                                                           nsIDOMNode* aDOMNode,
                                                           nsIFrame *aListFrame,
                                                           nsIWeakReference* aShell):
nsHTMLSelectListAccessible(aDOMNode, aShell), mListFrame(aListFrame)
{
}

nsIFrame *nsHTMLComboboxListAccessible::GetFrame()
{
  return mListFrame;
}








NS_IMETHODIMP nsHTMLComboboxListAccessible::GetState(PRUint32 *aState)
{
  
  nsAccessible::GetState(aState);

  nsIFrame *boundsFrame = GetBoundsFrame();
  nsIComboboxControlFrame* comboFrame = nsnull;
  boundsFrame->QueryInterface(NS_GET_IID(nsIComboboxControlFrame), (void**)&comboFrame);
  if (!comboFrame)
    return NS_ERROR_FAILURE;

  if (comboFrame->IsDroppedDown())
    *aState |= nsIAccessibleStates::STATE_FLOATING |
               nsIAccessibleStates::STATE_FOCUSABLE;
  else
    *aState |= nsIAccessibleStates::STATE_INVISIBLE |
               nsIAccessibleStates::STATE_FOCUSABLE;

  return NS_OK;
}


NS_IMETHODIMP nsHTMLComboboxListAccessible::GetParent(nsIAccessible **aParent)
{
  NS_IF_ADDREF(*aParent = mParent);
  return NS_OK;
}

NS_IMETHODIMP nsHTMLComboboxListAccessible::GetUniqueID(void **aUniqueID)
{
  
  *aUniqueID = NS_STATIC_CAST(void*, this);
  return NS_OK;
}





void nsHTMLComboboxListAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame)
{
   
  nsCOMPtr<nsIDOMNode> child;
  mDOMNode->GetFirstChild(getter_AddRefs(child));

  
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));
  if (!shell) {
    *aBoundingFrame = nsnull;
    return;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(child));
  nsIFrame* frame = shell->GetPrimaryFrameFor(content);
  if (!frame) {
    *aBoundingFrame = nsnull;
    return;
  }

  *aBoundingFrame = frame->GetParent();
  aBounds = (*aBoundingFrame)->GetRect();
}
