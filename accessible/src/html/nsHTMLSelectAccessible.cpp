





































#include "nsHTMLSelectAccessible.h"

#include "Accessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsDocAccessible.h"
#include "nsEventShell.h"
#include "nsIAccessibleEvent.h"
#include "nsTextEquivUtils.h"
#include "Role.h"
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
  nsHTMLSelectListAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
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

role
nsHTMLSelectListAccessible::NativeRole()
{
  if (mParent && mParent->Role() == roles::COMBOBOX)
    return roles::COMBOBOX_LIST;

  return roles::LISTBOX;
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
      nsDocAccessible* document = Document();
      if (document)
        return document->GetAccessible(activeOptionNode);
    }
  }
  return nsnull;
}

void
nsHTMLSelectListAccessible::SetCurrentItem(nsAccessible* aItem)
{
  aItem->GetContent()->SetAttr(kNameSpaceID_None,
                               nsGkAtoms::selected, NS_LITERAL_STRING("true"),
                               true);
}




void
nsHTMLSelectListAccessible::CacheChildren()
{
  
  
  
  
  CacheOptSiblings(mContent);
}




void
nsHTMLSelectListAccessible::CacheOptSiblings(nsIContent *aParentContent)
{
  for (nsIContent* childContent = aParentContent->GetFirstChild(); childContent;
       childContent = childContent->GetNextSibling()) {
    if (!childContent->IsHTML()) {
      continue;
    }

    nsCOMPtr<nsIAtom> tag = childContent->Tag();
    if (tag == nsGkAtoms::option ||
        tag == nsGkAtoms::optgroup) {

      
      nsRefPtr<nsAccessible> accessible =
        GetAccService()->GetOrCreateAccessible(childContent, mDoc);
      if (accessible)
        AppendChild(accessible);

      
      if (tag == nsGkAtoms::optgroup)
        CacheOptSiblings(childContent);
    }
  }
}






nsHTMLSelectOptionAccessible::
  nsHTMLSelectOptionAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsHyperTextAccessibleWrap(aContent, aDoc)
{
}




role
nsHTMLSelectOptionAccessible::NativeRole()
{
  if (mParent && mParent->Role() == roles::COMBOBOX_LIST)
    return roles::COMBOBOX_OPTION;

  return roles::OPTION;
}

nsresult
nsHTMLSelectOptionAccessible::GetNameInternal(nsAString& aName)
{
  
  
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, aName);
  if (!aName.IsEmpty())
    return NS_OK;

  
  
  nsIContent* text = mContent->GetFirstChild();
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

  if (level == 1 && Role() != roles::HEADING)
    level = 0; 

  return level;
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

  nsIContent* selectNode = mContent;
  while (selectNode && selectNode->Tag() != nsGkAtoms::select) {
    selectNode = selectNode->GetParent();
  }

  if (selectNode) {
    nsAccessible* select = mDoc->GetAccessible(selectNode);
    if (select) {
      *aState = select->State();
      return selectNode;
    }
  }
  return nsnull; 
}






nsHTMLSelectOptGroupAccessible::
  nsHTMLSelectOptGroupAccessible(nsIContent* aContent,
                                 nsDocAccessible* aDoc) :
  nsHTMLSelectOptionAccessible(aContent, aDoc)
{
}

role
nsHTMLSelectOptGroupAccessible::NativeRole()
{
  return roles::HEADING;
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
  nsHTMLComboboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
  mFlags |= eComboboxAccessible;
}




role
nsHTMLComboboxAccessible::NativeRole()
{
  return roles::COMBOBOX;
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
      new nsHTMLComboboxListAccessible(mParent, mContent, mDoc);

    
    if (!Document()->BindToDocument(mListAccessible, nsnull))
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

  nsIComboboxControlFrame* comboFrame = do_QueryFrame(GetFrame());
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

void
nsHTMLComboboxAccessible::Value(nsString& aValue)
{
  
  nsAccessible* option = SelectedOption();
  if (option)
    option->Name(aValue);
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

void
nsHTMLComboboxAccessible::SetCurrentItem(nsAccessible* aItem)
{
  if (AreItemsOperable())
    mListAccessible->SetCurrentItem(aItem);
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
      nsDocAccessible* document = Document();
      if (document)
        return document->GetAccessible(activeOptionNode);
    }
  }

  return nsnull;
}






nsHTMLComboboxListAccessible::
  nsHTMLComboboxListAccessible(nsIAccessible* aParent, nsIContent* aContent,
                               nsDocAccessible* aDoc) :
  nsHTMLSelectListAccessible(aContent, aDoc)
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

  nsIComboboxControlFrame* comboFrame = do_QueryFrame(mParent->GetFrame());
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

  
  nsIContent* content = mContent->GetFirstChild();
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

