




#include "HTMLSelectAccessible.h"

#include "Accessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "DocAccessible.h"
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





HTMLSelectListAccessible::
  HTMLSelectListAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
  mFlags |= eListControlAccessible;
}




PRUint64
HTMLSelectListAccessible::NativeState()
{
  PRUint64 state = AccessibleWrap::NativeState();
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple))
    state |= states::MULTISELECTABLE | states::EXTSELECTABLE;

  return state;
}

role
HTMLSelectListAccessible::NativeRole()
{
  return roles::LISTBOX;
}




bool
HTMLSelectListAccessible::IsSelect()
{
  return true;
}

bool
HTMLSelectListAccessible::SelectAll()
{
  return mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple) ?
    AccessibleWrap::SelectAll() : false;
}

bool
HTMLSelectListAccessible::UnselectAll()
{
  return mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple) ?
    AccessibleWrap::UnselectAll() : false;
}




bool
HTMLSelectListAccessible::IsWidget() const
{
  return true;
}

bool
HTMLSelectListAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
HTMLSelectListAccessible::AreItemsOperable() const
{
  return true;
}

Accessible*
HTMLSelectListAccessible::CurrentItem()
{
  nsIListControlFrame* listControlFrame = do_QueryFrame(GetFrame());
  if (listControlFrame) {
    nsCOMPtr<nsIContent> activeOptionNode = listControlFrame->GetCurrentOption();
    if (activeOptionNode) {
      DocAccessible* document = Document();
      if (document)
        return document->GetAccessible(activeOptionNode);
    }
  }
  return nullptr;
}

void
HTMLSelectListAccessible::SetCurrentItem(Accessible* aItem)
{
  aItem->GetContent()->SetAttr(kNameSpaceID_None,
                               nsGkAtoms::selected, NS_LITERAL_STRING("true"),
                               true);
}




void
HTMLSelectListAccessible::CacheChildren()
{
  
  
  
  
  CacheOptSiblings(mContent);
}




void
HTMLSelectListAccessible::CacheOptSiblings(nsIContent* aParentContent)
{
  for (nsIContent* childContent = aParentContent->GetFirstChild(); childContent;
       childContent = childContent->GetNextSibling()) {
    if (!childContent->IsHTML()) {
      continue;
    }

    nsIAtom* tag = childContent->Tag();
    if (tag == nsGkAtoms::option ||
        tag == nsGkAtoms::optgroup) {

      
      nsRefPtr<Accessible> accessible =
        GetAccService()->GetOrCreateAccessible(childContent, mDoc);
      if (accessible)
        AppendChild(accessible);

      
      if (tag == nsGkAtoms::optgroup)
        CacheOptSiblings(childContent);
    }
  }
}






HTMLSelectOptionAccessible::
  HTMLSelectOptionAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}




role
HTMLSelectOptionAccessible::NativeRole()
{
  if (mParent && mParent->Role() == roles::COMBOBOX_LIST)
    return roles::COMBOBOX_OPTION;

  return roles::OPTION;
}

nsresult
HTMLSelectOptionAccessible::GetNameInternal(nsAString& aName)
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

PRUint64
HTMLSelectOptionAccessible::NativeState()
{
  
  
  
  
  PRUint64 state = Accessible::NativeState();

  Accessible* select = GetSelect();
  if (!select)
    return state;

  PRUint64 selectState = select->State();
  if (selectState & states::INVISIBLE)
    return state;

  
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
    
    Accessible* listAcc = Parent();
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

PRUint64
HTMLSelectOptionAccessible::NativeInteractiveState() const
{
  return NativelyUnavailable() ?
    states::UNAVAILABLE : states::FOCUSABLE | states::SELECTABLE;
}

PRInt32
HTMLSelectOptionAccessible::GetLevelInternal()
{
  nsIContent *parentContent = mContent->GetParent();

  PRInt32 level =
    parentContent->NodeInfo()->Equals(nsGkAtoms::optgroup) ? 2 : 1;

  if (level == 1 && Role() != roles::HEADING)
    level = 0; 

  return level;
}

void
HTMLSelectOptionAccessible::GetBoundsRect(nsRect& aTotalBounds,
                                          nsIFrame** aBoundingFrame)
{
  Accessible* combobox = GetCombobox();
  if (combobox && (combobox->State() & states::COLLAPSED))
    combobox->GetBoundsRect(aTotalBounds, aBoundingFrame);
  else
    HyperTextAccessibleWrap::GetBoundsRect(aTotalBounds, aBoundingFrame);
}

NS_IMETHODIMP
HTMLSelectOptionAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Select) {
    aName.AssignLiteral("select"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

PRUint8
HTMLSelectOptionAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
HTMLSelectOptionAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Select)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  DoCommand();
  return NS_OK;
}

NS_IMETHODIMP
HTMLSelectOptionAccessible::SetSelected(bool aSelect)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMHTMLOptionElement> optionElm(do_QueryInterface(mContent));
  return optionElm->SetSelected(aSelect);
}




Accessible*
HTMLSelectOptionAccessible::ContainerWidget() const
{
  return mParent && mParent->IsListControl() ? mParent : nullptr;
}





HTMLSelectOptGroupAccessible::
  HTMLSelectOptGroupAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HTMLSelectOptionAccessible(aContent, aDoc)
{
}

role
HTMLSelectOptGroupAccessible::NativeRole()
{
  return roles::HEADING;
}

PRUint64
HTMLSelectOptGroupAccessible::NativeInteractiveState() const
{
  return NativelyUnavailable() ? states::UNAVAILABLE : 0;
}

NS_IMETHODIMP
HTMLSelectOptGroupAccessible::DoAction(PRUint8 index)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
HTMLSelectOptGroupAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

PRUint8
HTMLSelectOptGroupAccessible::ActionCount()
{
  return 0;
}




void
HTMLSelectOptGroupAccessible::CacheChildren()
{
  
  
  
  
  
}






HTMLComboboxAccessible::
  HTMLComboboxAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
  mFlags |= eComboboxAccessible;
}




role
HTMLComboboxAccessible::NativeRole()
{
  return roles::COMBOBOX;
}

void
HTMLComboboxAccessible::InvalidateChildren()
{
  AccessibleWrap::InvalidateChildren();

  if (mListAccessible)
    mListAccessible->InvalidateChildren();
}

void
HTMLComboboxAccessible::CacheChildren()
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
      new HTMLComboboxListAccessible(mParent, mContent, mDoc);

    
    if (!Document()->BindToDocument(mListAccessible, nullptr))
      return;
  }

  if (AppendChild(mListAccessible)) {
    
    
    mListAccessible->EnsureChildren();
  }
}

void
HTMLComboboxAccessible::Shutdown()
{
  AccessibleWrap::Shutdown();

  if (mListAccessible) {
    mListAccessible->Shutdown();
    mListAccessible = nullptr;
  }
}

PRUint64
HTMLComboboxAccessible::NativeState()
{
  
  
  
  PRUint64 state = Accessible::NativeState();

  nsIComboboxControlFrame* comboFrame = do_QueryFrame(GetFrame());
  if (comboFrame && comboFrame->IsDroppedDown())
    state |= states::EXPANDED;
  else
    state |= states::COLLAPSED;

  state |= states::HASPOPUP;
  return state;
}

void
HTMLComboboxAccessible::Description(nsString& aDescription)
{
  aDescription.Truncate();
  
  
  Accessible::Description(aDescription);
  if (!aDescription.IsEmpty())
    return;

  
  Accessible* option = SelectedOption();
  if (option)
    option->Description(aDescription);
}

void
HTMLComboboxAccessible::Value(nsString& aValue)
{
  
  Accessible* option = SelectedOption();
  if (option)
    option->Name(aValue);
}

PRUint8
HTMLComboboxAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
HTMLComboboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  DoCommand();
  return NS_OK;
}







NS_IMETHODIMP
HTMLComboboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != HTMLComboboxAccessible::eAction_Click) {
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
HTMLComboboxAccessible::IsWidget() const
{
  return true;
}

bool
HTMLComboboxAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
HTMLComboboxAccessible::AreItemsOperable() const
{
  nsIComboboxControlFrame* comboboxFrame = do_QueryFrame(GetFrame());
  return comboboxFrame && comboboxFrame->IsDroppedDown();
}

Accessible*
HTMLComboboxAccessible::CurrentItem()
{
  return AreItemsOperable() ? mListAccessible->CurrentItem() : nullptr;
}

void
HTMLComboboxAccessible::SetCurrentItem(Accessible* aItem)
{
  if (AreItemsOperable())
    mListAccessible->SetCurrentItem(aItem);
}




Accessible*
HTMLComboboxAccessible::SelectedOption() const
{
  nsIFrame* frame = GetFrame();
  nsIComboboxControlFrame* comboboxFrame = do_QueryFrame(frame);
  if (!comboboxFrame)
    return nullptr;

  nsIListControlFrame* listControlFrame =
    do_QueryFrame(comboboxFrame->GetDropDown());
  if (listControlFrame) {
    nsCOMPtr<nsIContent> activeOptionNode = listControlFrame->GetCurrentOption();
    if (activeOptionNode) {
      DocAccessible* document = Document();
      if (document)
        return document->GetAccessible(activeOptionNode);
    }
  }

  return nullptr;
}






HTMLComboboxListAccessible::
  HTMLComboboxListAccessible(nsIAccessible* aParent, nsIContent* aContent,
                             DocAccessible* aDoc) :
  HTMLSelectListAccessible(aContent, aDoc)
{
}




nsIFrame*
HTMLComboboxListAccessible::GetFrame() const
{
  nsIFrame* frame = HTMLSelectListAccessible::GetFrame();

  if (frame) {
    nsIComboboxControlFrame* comboBox = do_QueryFrame(frame);
    if (comboBox) {
      return comboBox->GetDropDown();
    }
  }

  return nullptr;
}

bool
HTMLComboboxListAccessible::IsPrimaryForNode() const
{
  return false;
}




role
HTMLComboboxListAccessible::NativeRole()
{
  return roles::COMBOBOX_LIST;
}

PRUint64
HTMLComboboxListAccessible::NativeState()
{
  
  
  
  PRUint64 state = Accessible::NativeState();

  nsIComboboxControlFrame* comboFrame = do_QueryFrame(mParent->GetFrame());
  if (comboFrame && comboFrame->IsDroppedDown())
    state |= states::FLOATING;
  else
    state |= states::INVISIBLE;

  return state;
}





void
HTMLComboboxListAccessible::GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame)
{
  *aBoundingFrame = nullptr;

  Accessible* comboAcc = Parent();
  if (!comboAcc)
    return;

  if (0 == (comboAcc->State() & states::COLLAPSED)) {
    HTMLSelectListAccessible::GetBoundsRect(aBounds, aBoundingFrame);
    return;
  }

  
  nsIContent* content = mContent->GetFirstChild();
  if (!content) {
    return;
  }
  nsIFrame* frame = content->GetPrimaryFrame();
  if (!frame) {
    *aBoundingFrame = nullptr;
    return;
  }

  *aBoundingFrame = frame->GetParent();
  aBounds = (*aBoundingFrame)->GetRect();
}




bool
HTMLComboboxListAccessible::IsActiveWidget() const
{
  return mParent && mParent->IsActiveWidget();
}

bool
HTMLComboboxListAccessible::AreItemsOperable() const
{
  return mParent && mParent->AreItemsOperable();
}

