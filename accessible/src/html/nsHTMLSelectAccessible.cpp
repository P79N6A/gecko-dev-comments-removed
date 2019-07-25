




#include "nsHTMLSelectAccessible.h"

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





nsHTMLSelectListAccessible::
  nsHTMLSelectListAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
  mFlags |= eListControlAccessible;
}




PRUint64
nsHTMLSelectListAccessible::NativeState()
{
  PRUint64 state = AccessibleWrap::NativeState();
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple))
    state |= states::MULTISELECTABLE | states::EXTSELECTABLE;

  return state;
}

role
nsHTMLSelectListAccessible::NativeRole()
{
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
    AccessibleWrap::SelectAll() : false;
}

bool
nsHTMLSelectListAccessible::UnselectAll()
{
  return mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple) ?
    AccessibleWrap::UnselectAll() : false;
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

Accessible*
nsHTMLSelectListAccessible::CurrentItem()
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
  return nsnull;
}

void
nsHTMLSelectListAccessible::SetCurrentItem(Accessible* aItem)
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






nsHTMLSelectOptionAccessible::
  nsHTMLSelectOptionAccessible(nsIContent* aContent, DocAccessible* aDoc) :
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

PRUint64
nsHTMLSelectOptionAccessible::NativeState()
{
  
  
  
  
  PRUint64 state = Accessible::NativeState();

  Accessible* select = GetSelect();
  if (!select)
    return state;

  PRUint64 selectState = select->State();
  if (selectState & states::INVISIBLE)
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

void
nsHTMLSelectOptionAccessible::GetBoundsRect(nsRect& aTotalBounds,
                                            nsIFrame** aBoundingFrame)
{
  Accessible* combobox = GetCombobox();
  if (combobox && (combobox->State() & states::COLLAPSED))
    combobox->GetBoundsRect(aTotalBounds, aBoundingFrame);
  else
    nsHyperTextAccessibleWrap::GetBoundsRect(aTotalBounds, aBoundingFrame);
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




Accessible*
nsHTMLSelectOptionAccessible::ContainerWidget() const
{
  return mParent && mParent->IsListControl() ? mParent : nsnull;
}





nsHTMLSelectOptGroupAccessible::
  nsHTMLSelectOptGroupAccessible(nsIContent* aContent,
                                 DocAccessible* aDoc) :
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
  nsHTMLComboboxAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
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
  AccessibleWrap::InvalidateChildren();

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
  AccessibleWrap::Shutdown();

  if (mListAccessible) {
    mListAccessible->Shutdown();
    mListAccessible = nsnull;
  }
}

PRUint64
nsHTMLComboboxAccessible::NativeState()
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
nsHTMLComboboxAccessible::Description(nsString& aDescription)
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
nsHTMLComboboxAccessible::Value(nsString& aValue)
{
  
  Accessible* option = SelectedOption();
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

Accessible*
nsHTMLComboboxAccessible::CurrentItem()
{
  return AreItemsOperable() ? mListAccessible->CurrentItem() : nsnull;
}

void
nsHTMLComboboxAccessible::SetCurrentItem(Accessible* aItem)
{
  if (AreItemsOperable())
    mListAccessible->SetCurrentItem(aItem);
}




Accessible*
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
      DocAccessible* document = Document();
      if (document)
        return document->GetAccessible(activeOptionNode);
    }
  }

  return nsnull;
}






nsHTMLComboboxListAccessible::
  nsHTMLComboboxListAccessible(nsIAccessible* aParent, nsIContent* aContent,
                               DocAccessible* aDoc) :
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




role
nsHTMLComboboxListAccessible::NativeRole()
{
  return roles::COMBOBOX_LIST;
}

PRUint64
nsHTMLComboboxListAccessible::NativeState()
{
  
  
  
  PRUint64 state = Accessible::NativeState();

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

  Accessible* comboAcc = Parent();
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

