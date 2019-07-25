





































#include "nsHTMLSelectAccessible.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsDocAccessible.h"
#include "nsEventShell.h"
#include "nsIAccessibleEvent.h"
#include "nsTextEquivUtils.h"
#include "States.h"

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

using namespace mozilla::a11y;





nsHTMLSelectListAccessible::
  nsHTMLSelectListAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
  mFlags |= eListControlAccessible;
}




PRUint64
nsHTMLSelectListAccessible::NativeState()
{
  PRUint64 state = nsAccessibleWrap::NativeState();
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple))
    state |= states::MULTISELECTABLE | states::EXTSELECTABLE;

  return state;
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
  return mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple) ?
           nsAccessibleWrap::SelectAll() : false;
}

bool
nsHTMLSelectListAccessible::UnselectAll()
{
  return mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple) ?
           nsAccessibleWrap::UnselectAll() : false;
}




bool
nsHTMLSelectListAccessible::IsWidget() const
{
  return true;
}

bool
nsHTMLSelectListAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
nsHTMLSelectListAccessible::AreItemsOperable() const
{
  return true;
}

nsAccessible*
nsHTMLSelectListAccessible::CurrentItem()
{
  nsIListControlFrame* listControlFrame = do_QueryFrame(GetFrame());
  if (listControlFrame) {
    nsCOMPtr<nsIContent> activeOptionNode = listControlFrame->GetCurrentOption();
    if (activeOptionNode) {
      nsDocAccessible* document = GetDocAccessible();
      if (document)
        return document->GetAccessible(activeOptionNode);
    }
  }
  return nsnull;
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
    if (tag == nsGkAtoms::option ||
        tag == nsGkAtoms::optgroup) {

      
      nsRefPtr<nsAccessible> accessible =
        GetAccService()->GetOrCreateAccessible(childContent, presShell,
                                               mWeakShell);
      if (accessible)
        AppendChild(accessible);

      
      if (tag == nsGkAtoms::optgroup)
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
  
  
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, aName);
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
  PRUint64 state = 0;
  nsIContent* content = GetSelectState(&state);
  if (state & states::COLLAPSED) {
    if (content) {
      return content->GetPrimaryFrame();
    }

    return nsnull;
  }

  return nsAccessible::GetBoundsFrame();
}

PRUint64
nsHTMLSelectOptionAccessible::NativeState()
{
  
  
  
  
  PRUint64 state = nsAccessible::NativeState();

  PRUint64 selectState = 0;
  nsIContent* selectContent = GetSelectState(&selectState);
  if (!selectContent || selectState & states::INVISIBLE)
    return state;

  
  if (!(state & states::UNAVAILABLE))
    state |= (states::FOCUSABLE | states::SELECTABLE);

  
  bool isSelected = false;
  nsCOMPtr<nsIDOMHTMLOptionElement> option(do_QueryInterface(mContent));
  if (option) {
    option->GetSelected(&isSelected);
    if (isSelected)
      state |= states::SELECTED;
  }

  if (selectState & states::OFFSCREEN) {
    state |= states::OFFSCREEN;
  }
  else if (selectState & states::COLLAPSED) {
    
    
    if (!isSelected) {
      state |= states::OFFSCREEN;
    }
    else {
      
      state &= ~(states::OFFSCREEN | states::INVISIBLE);
      state |= selectState & states::OPAQUE1;
    }
  }
  else {
    
    
    state &= ~states::OFFSCREEN;
    
    nsAccessible* listAcc = Parent();
    if (listAcc) {
      PRInt32 optionX, optionY, optionWidth, optionHeight;
      PRInt32 listX, listY, listWidth, listHeight;
      GetBounds(&optionX, &optionY, &optionWidth, &optionHeight);
      listAcc->GetBounds(&listX, &listY, &listWidth, &listHeight);
      if (optionY < listY || optionY + optionHeight > listY + listHeight) {
        state |= states::OFFSCREEN;
      }
    }
  }
 
  return state;
}

PRInt32
nsHTMLSelectOptionAccessible::GetLevelInternal()
{
  nsIContent *parentContent = mContent->GetParent();

  PRInt32 level =
    parentContent->NodeInfo()->Equals(nsGkAtoms::optgroup) ? 2 : 1;

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
  bool isContentFound = false;

  PRUint32 childCount = parentContent->GetChildCount();
  for (PRUint32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsIContent *childContent = parentContent->GetChildAt(childIdx);
    if (childContent->NodeInfo()->Equals(mContent->NodeInfo())) {
      if (!isContentFound) {
        if (childContent == mContent)
          isContentFound = true;

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

PRUint8
nsHTMLSelectOptionAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
nsHTMLSelectOptionAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Select)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  DoCommand();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSelectOptionAccessible::SetSelected(bool aSelect)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMHTMLOptionElement> optionElm(do_QueryInterface(mContent));
  return optionElm->SetSelected(aSelect);
}




nsAccessible*
nsHTMLSelectOptionAccessible::ContainerWidget() const
{
  return mParent && mParent->IsListControl() ? mParent : nsnull;
}




nsIContent*
nsHTMLSelectOptionAccessible::GetSelectState(PRUint64* aState)
{
  *aState = 0;

  nsIContent *content = mContent;
  while (content && content->Tag() != nsGkAtoms::select) {
    content = content->GetParent();
  }

  if (content) {
    nsAccessible* selAcc = GetAccService()->GetAccessible(content);
    if (selAcc) {
      *aState = selAcc->State();
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

PRUint64
nsHTMLSelectOptGroupAccessible::NativeState()
{
  PRUint64 state = nsHTMLSelectOptionAccessible::NativeState();

  state &= ~(states::FOCUSABLE | states::SELECTABLE);

  return state;
}

NS_IMETHODIMP nsHTMLSelectOptGroupAccessible::DoAction(PRUint8 index)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsHTMLSelectOptGroupAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

PRUint8
nsHTMLSelectOptGroupAccessible::ActionCount()
{
  return 0;
}




void
nsHTMLSelectOptGroupAccessible::CacheChildren()
{
  
  
  
  
  
}






nsHTMLComboboxAccessible::
  nsHTMLComboboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
  mFlags |= eComboboxAccessible;
}




PRUint32
nsHTMLComboboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_COMBOBOX;
}

void
nsHTMLComboboxAccessible::InvalidateChildren()
{
  nsAccessibleWrap::InvalidateChildren();

  if (mListAccessible)
    mListAccessible->InvalidateChildren();
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

    
    if (!GetDocAccessible()->BindToDocument(mListAccessible, nsnull))
      return;
  }

  if (AppendChild(mListAccessible)) {
    
    
    mListAccessible->EnsureChildren();
  }
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



PRUint64
nsHTMLComboboxAccessible::NativeState()
{
  
  
  
  PRUint64 state = nsAccessible::NativeState();

  nsIFrame *frame = GetBoundsFrame();
  nsIComboboxControlFrame *comboFrame = do_QueryFrame(frame);
  if (comboFrame && comboFrame->IsDroppedDown())
    state |= states::EXPANDED;
  else
    state |= states::COLLAPSED;

  state |= states::HASPOPUP;
  return state;
}

void
nsHTMLComboboxAccessible::Description(nsString& aDescription)
{
  aDescription.Truncate();
  
  
  nsAccessible::Description(aDescription);
  if (!aDescription.IsEmpty())
    return;

  
  nsAccessible* option = SelectedOption();
  if (option)
    option->Description(aDescription);
}

NS_IMETHODIMP nsHTMLComboboxAccessible::GetValue(nsAString& aValue)
{
  
  nsAccessible* option = SelectedOption();
  return option ? option->GetName(aValue) : NS_OK;
}

PRUint8
nsHTMLComboboxAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
nsHTMLComboboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  DoCommand();
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




bool
nsHTMLComboboxAccessible::IsWidget() const
{
  return true;
}

bool
nsHTMLComboboxAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
nsHTMLComboboxAccessible::AreItemsOperable() const
{
  nsIComboboxControlFrame* comboboxFrame = do_QueryFrame(GetFrame());
  return comboboxFrame && comboboxFrame->IsDroppedDown();
}

nsAccessible*
nsHTMLComboboxAccessible::CurrentItem()
{
  return AreItemsOperable() ? mListAccessible->CurrentItem() : nsnull;
}




nsAccessible*
nsHTMLComboboxAccessible::SelectedOption() const
{
  nsIFrame* frame = GetFrame();
  nsIComboboxControlFrame* comboboxFrame = do_QueryFrame(frame);
  if (!comboboxFrame)
    return nsnull;

  nsIListControlFrame* listControlFrame =
    do_QueryFrame(comboboxFrame->GetDropDown());
  if (listControlFrame) {
    nsCOMPtr<nsIContent> activeOptionNode = listControlFrame->GetCurrentOption();
    if (activeOptionNode) {
      nsDocAccessible* document = GetDocAccessible();
      if (document)
        return document->GetAccessible(activeOptionNode);
    }
  }

  return nsnull;
}






nsHTMLComboboxListAccessible::
  nsHTMLComboboxListAccessible(nsIAccessible *aParent, nsIContent *aContent,
                               nsIWeakReference *aShell) :
  nsHTMLSelectListAccessible(aContent, aShell)
{
}




nsIFrame*
nsHTMLComboboxListAccessible::GetFrame() const
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

bool
nsHTMLComboboxListAccessible::IsPrimaryForNode() const
{
  return false;
}




PRUint64
nsHTMLComboboxListAccessible::NativeState()
{
  
  
  
  PRUint64 state = nsAccessible::NativeState();

  nsIFrame *boundsFrame = GetBoundsFrame();
  nsIComboboxControlFrame* comboFrame = do_QueryFrame(boundsFrame);
  if (comboFrame && comboFrame->IsDroppedDown())
    state |= states::FLOATING;
  else
    state |= states::INVISIBLE;

  return state;
}





void nsHTMLComboboxListAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame)
{
  *aBoundingFrame = nsnull;

  nsAccessible* comboAcc = Parent();
  if (!comboAcc)
    return;

  if (0 == (comboAcc->State() & states::COLLAPSED)) {
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




bool
nsHTMLComboboxListAccessible::IsActiveWidget() const
{
  return mParent && mParent->IsActiveWidget();
}

bool
nsHTMLComboboxListAccessible::AreItemsOperable() const
{
  return mParent && mParent->AreItemsOperable();
}

