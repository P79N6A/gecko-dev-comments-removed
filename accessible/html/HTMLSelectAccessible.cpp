




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
#include "mozilla/dom/HTMLOptionElement.h"
#include "nsIComboboxControlFrame.h"
#include "nsContainerFrame.h"
#include "nsIListControlFrame.h"

using namespace mozilla::a11y;
using namespace mozilla::dom;





HTMLSelectListAccessible::
  HTMLSelectListAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
  mGenericTypes |= eListControl | eSelect;
}




uint64_t
HTMLSelectListAccessible::NativeState()
{
  uint64_t state = AccessibleWrap::NativeState();
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
  
  
  
  
  for (nsIContent* childContent = mContent->GetFirstChild(); childContent;
       childContent = childContent->GetNextSibling()) {
    if (!childContent->IsHTML()) {
      continue;
    }

    nsIAtom* tag = childContent->Tag();
    if (tag == nsGkAtoms::option ||
        tag == nsGkAtoms::optgroup) {

      
      nsRefPtr<Accessible> accessible =
        GetAccService()->GetOrCreateAccessible(childContent, this);
      if (accessible)
        AppendChild(accessible);
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
  if (GetCombobox())
    return roles::COMBOBOX_OPTION;

  return roles::OPTION;
}

ENameValueFlag
HTMLSelectOptionAccessible::NativeName(nsString& aName)
{
  
  
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, aName);
  if (!aName.IsEmpty())
    return eNameOK;

  
  
  nsIContent* text = mContent->GetFirstChild();
  if (text && text->IsNodeOfType(nsINode::eTEXT)) {
    nsTextEquivUtils::AppendTextEquivFromTextContent(text, &aName);
    aName.CompressWhitespace();
    return aName.IsEmpty() ? eNameOK : eNameFromSubtree;
  }

  return eNameOK;
}

uint64_t
HTMLSelectOptionAccessible::NativeState()
{
  
  
  
  
  uint64_t state = Accessible::NativeState();

  Accessible* select = GetSelect();
  if (!select)
    return state;

  uint64_t selectState = select->State();
  if (selectState & states::INVISIBLE)
    return state;

  
  HTMLOptionElement* option = HTMLOptionElement::FromContent(mContent);
  bool selected = option && option->Selected();
  if (selected)
    state |= states::SELECTED;

  if (selectState & states::OFFSCREEN) {
    state |= states::OFFSCREEN;
  } else if (selectState & states::COLLAPSED) {
    
    
    if (!selected) {
      state |= states::OFFSCREEN;
      state ^= states::INVISIBLE;
    } else {
      
      state &= ~(states::OFFSCREEN | states::INVISIBLE);
      state |= selectState & states::OPAQUE1;
    }
  } else {
    
    
    state &= ~states::OFFSCREEN;
    
    Accessible* listAcc = Parent();
    if (listAcc) {
      nsIntRect optionRect = Bounds();
      nsIntRect listRect = listAcc->Bounds();
      if (optionRect.y < listRect.y ||
          optionRect.y + optionRect.height > listRect.y + listRect.height) {
        state |= states::OFFSCREEN;
      }
    }
  }

  return state;
}

uint64_t
HTMLSelectOptionAccessible::NativeInteractiveState() const
{
  return NativelyUnavailable() ?
    states::UNAVAILABLE : states::FOCUSABLE | states::SELECTABLE;
}

int32_t
HTMLSelectOptionAccessible::GetLevelInternal()
{
  nsIContent* parentContent = mContent->GetParent();

  int32_t level =
    parentContent->NodeInfo()->Equals(nsGkAtoms::optgroup) ? 2 : 1;

  if (level == 1 && Role() != roles::HEADING)
    level = 0; 

  return level;
}

nsRect
HTMLSelectOptionAccessible::RelativeBounds(nsIFrame** aBoundingFrame) const
{
  Accessible* combobox = GetCombobox();
  if (combobox && (combobox->State() & states::COLLAPSED))
    return combobox->RelativeBounds(aBoundingFrame);

  return HyperTextAccessibleWrap::RelativeBounds(aBoundingFrame);
}

void
HTMLSelectOptionAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  if (aIndex == eAction_Select)
    aName.AssignLiteral("select");
}

uint8_t
HTMLSelectOptionAccessible::ActionCount()
{
  return 1;
}

bool
HTMLSelectOptionAccessible::DoAction(uint8_t aIndex)
{
  if (aIndex != eAction_Select)
    return false;

  DoCommand();
  return true;
}

void
HTMLSelectOptionAccessible::SetSelected(bool aSelect)
{
  HTMLOptionElement* option = HTMLOptionElement::FromContent(mContent);
  if (option)
    option->SetSelected(aSelect);
}




Accessible*
HTMLSelectOptionAccessible::ContainerWidget() const
{
  Accessible* parent = Parent();
  if (parent && parent->IsHTMLOptGroup())
    parent = parent->Parent();

  return parent && parent->IsListControl() ? parent : nullptr;
}





role
HTMLSelectOptGroupAccessible::NativeRole()
{
  return roles::GROUPING;
}

uint64_t
HTMLSelectOptGroupAccessible::NativeInteractiveState() const
{
  return NativelyUnavailable() ? states::UNAVAILABLE : 0;
}

uint8_t
HTMLSelectOptGroupAccessible::ActionCount()
{
  return 0;
}

void
HTMLSelectOptGroupAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  aName.Truncate();
}

bool
HTMLSelectOptGroupAccessible::DoAction(uint8_t aIndex)
{
  return false;
}





HTMLComboboxAccessible::
  HTMLComboboxAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
  mGenericTypes |= eCombobox;
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
  nsIComboboxControlFrame* comboFrame = do_QueryFrame(GetFrame());
  if (!comboFrame)
    return;

  nsIFrame* listFrame = comboFrame->GetDropDown();
  if (!listFrame)
    return;

  if (!mListAccessible) {
    mListAccessible = 
      new HTMLComboboxListAccessible(mParent, mContent, mDoc);

    
    Document()->BindToDocument(mListAccessible, nullptr);
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

uint64_t
HTMLComboboxAccessible::NativeState()
{
  
  
  
  uint64_t state = Accessible::NativeState();

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

uint8_t
HTMLComboboxAccessible::ActionCount()
{
  return 1;
}

bool
HTMLComboboxAccessible::DoAction(uint8_t aIndex)
{
  if (aIndex != eAction_Click)
    return false;

  DoCommand();
  return true;
}

void
HTMLComboboxAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  if (aIndex != HTMLComboboxAccessible::eAction_Click)
    return;

  nsIComboboxControlFrame* comboFrame = do_QueryFrame(GetFrame());
  if (!comboFrame)
    return;

  if (comboFrame->IsDroppedDown())
    aName.AssignLiteral("close");
  else
    aName.AssignLiteral("open");
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
  mStateFlags |= eSharedNode;
}




nsIFrame*
HTMLComboboxListAccessible::GetFrame() const
{
  nsIFrame* frame = HTMLSelectListAccessible::GetFrame();
  nsIComboboxControlFrame* comboBox = do_QueryFrame(frame);
  if (comboBox) {
    return comboBox->GetDropDown();
  }

  return nullptr;
}

role
HTMLComboboxListAccessible::NativeRole()
{
  return roles::COMBOBOX_LIST;
}

uint64_t
HTMLComboboxListAccessible::NativeState()
{
  
  
  
  uint64_t state = Accessible::NativeState();

  nsIComboboxControlFrame* comboFrame = do_QueryFrame(mParent->GetFrame());
  if (comboFrame && comboFrame->IsDroppedDown())
    state |= states::FLOATING;
  else
    state |= states::INVISIBLE;

  return state;
}

nsRect
HTMLComboboxListAccessible::RelativeBounds(nsIFrame** aBoundingFrame) const
{
  *aBoundingFrame = nullptr;

  Accessible* comboAcc = Parent();
  if (!comboAcc)
    return nsRect();

  if (0 == (comboAcc->State() & states::COLLAPSED)) {
    return HTMLSelectListAccessible::RelativeBounds(aBoundingFrame);
  }

  
  nsIContent* content = mContent->GetFirstChild();
  if (!content)
    return nsRect();

  nsIFrame* frame = content->GetPrimaryFrame();
  if (!frame) {
    *aBoundingFrame = nullptr;
    return nsRect();
  }

  *aBoundingFrame = frame->GetParent();
  return (*aBoundingFrame)->GetRect();
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

