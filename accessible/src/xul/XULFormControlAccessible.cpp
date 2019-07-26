






































#include "XULFormControlAccessible.h"

#include "Accessible-inl.h"
#include "HTMLFormControlAccessible.h"
#include "nsAccUtils.h"
#include "nsAccTreeWalker.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"
#include "nsIAccessibleRelation.h"
#include "nsXULMenuAccessible.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"

#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIDOMXULButtonElement.h"
#include "nsIDOMXULCheckboxElement.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULTextboxElement.h"
#include "nsIEditor.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsITextControlFrame.h"
#include "nsMenuPopupFrame.h"

using namespace mozilla::a11y;





XULButtonAccessible::
  XULButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
  if (ContainsMenu())
    mFlags |= eMenuButtonAccessible;
}




NS_IMPL_ISUPPORTS_INHERITED0(XULButtonAccessible, nsAccessible)




PRUint8
XULButtonAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
XULButtonAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("press"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
XULButtonAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}




role
XULButtonAccessible::NativeRole()
{
  return roles::PUSHBUTTON;
}

PRUint64
XULButtonAccessible::NativeState()
{
  

  
  PRUint64 state = nsAccessible::NativeState();

  bool disabled = false;
  nsCOMPtr<nsIDOMXULControlElement> xulFormElement(do_QueryInterface(mContent));
  if (xulFormElement) {
    xulFormElement->GetDisabled(&disabled);
    if (disabled)
      state |= states::UNAVAILABLE;
    else 
      state |= states::FOCUSABLE;
  }

  
  nsCOMPtr<nsIDOMXULButtonElement> xulButtonElement(do_QueryInterface(mContent));
  if (xulButtonElement) {
    nsAutoString type;
    xulButtonElement->GetType(type);
    if (type.EqualsLiteral("checkbox") || type.EqualsLiteral("radio")) {
      state |= states::CHECKABLE;
      bool checked = false;
      PRInt32 checkState = 0;
      xulButtonElement->GetChecked(&checked);
      if (checked) {
        state |= states::PRESSED;
        xulButtonElement->GetCheckState(&checkState);
        if (checkState == nsIDOMXULButtonElement::CHECKSTATE_MIXED) { 
          state |= states::MIXED;
        }
      }
    }
  }

  if (ContainsMenu())
    state |= states::HASPOPUP;

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::_default))
    state |= states::DEFAULT;

  return state;
}




bool
XULButtonAccessible::IsWidget() const
{
  return true;
}

bool
XULButtonAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
XULButtonAccessible::AreItemsOperable() const
{
  if (IsMenuButton()) {
    nsAccessible* menuPopup = mChildren.SafeElementAt(0, nsnull);
    if (menuPopup) {
      nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(menuPopup->GetFrame());
      return menuPopupFrame->IsOpen();
    }
  }
  return false; 
}

nsAccessible*
XULButtonAccessible::ContainerWidget() const
{
  if (IsMenuButton() && mParent && mParent->IsAutoComplete())
    return mParent;
  return nsnull;
}




void
XULButtonAccessible::CacheChildren()
{
  
  
  

  
  
  bool isMenu = mContent->AttrValueIs(kNameSpaceID_None,
                                       nsGkAtoms::type,
                                       nsGkAtoms::menu,
                                       eCaseMatters);

  bool isMenuButton = isMenu ?
    false :
    mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                          nsGkAtoms::menuButton, eCaseMatters);

  NS_ENSURE_TRUE(mDoc,);
  if (!isMenu && !isMenuButton)
    return;

  nsAccessible* menupopup = nsnull;
  nsAccessible* button = nsnull;

  nsAccTreeWalker walker(mDoc, mContent, true);

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild())) {
    roles::Role role = child->Role();

    if (role == roles::MENUPOPUP) {
      
      menupopup = child;

    } else if (isMenuButton && role == roles::PUSHBUTTON) {
      
      
      button = child;
      break;

    } else {
      
      Document()->UnbindFromDocument(child);
    }
  }

  if (!menupopup)
    return;

  AppendChild(menupopup);
  if (button)
    AppendChild(button);
}




bool
XULButtonAccessible::ContainsMenu()
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::menu, &nsGkAtoms::menuButton, nsnull};

  return mContent->FindAttrValueIn(kNameSpaceID_None,
                                   nsGkAtoms::type,
                                   strings, eCaseMatters) >= 0;
}





XULDropmarkerAccessible::
  XULDropmarkerAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsLeafAccessible(aContent, aDoc)
{
}

PRUint8
XULDropmarkerAccessible::ActionCount()
{
  return 1;
}

bool
XULDropmarkerAccessible::DropmarkerOpen(bool aToggleOpen)
{
  bool isOpen = false;

  nsCOMPtr<nsIDOMXULButtonElement> parentButtonElement =
    do_QueryInterface(mContent->GetParent());

  if (parentButtonElement) {
    parentButtonElement->GetOpen(&isOpen);
    if (aToggleOpen)
      parentButtonElement->SetOpen(!isOpen);
  }
  else {
    nsCOMPtr<nsIDOMXULMenuListElement> parentMenuListElement =
      do_QueryInterface(parentButtonElement);
    if (parentMenuListElement) {
      parentMenuListElement->GetOpen(&isOpen);
      if (aToggleOpen)
        parentMenuListElement->SetOpen(!isOpen);
    }
  }

  return isOpen;
}




NS_IMETHODIMP
XULDropmarkerAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    if (DropmarkerOpen(false))
      aName.AssignLiteral("close");
    else
      aName.AssignLiteral("open");
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}




NS_IMETHODIMP
XULDropmarkerAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Click) {
    DropmarkerOpen(true); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

role
XULDropmarkerAccessible::NativeRole()
{
  return roles::PUSHBUTTON;
}

PRUint64
XULDropmarkerAccessible::NativeState()
{
  return DropmarkerOpen(false) ? states::PRESSED : 0;
}





XULCheckboxAccessible::
  XULCheckboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsLeafAccessible(aContent, aDoc)
{
}

role
XULCheckboxAccessible::NativeRole()
{
  return roles::CHECKBUTTON;
}

PRUint8
XULCheckboxAccessible::ActionCount()
{
  return 1;
}




NS_IMETHODIMP
XULCheckboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    

    if (NativeState() & states::CHECKED)
      aName.AssignLiteral("uncheck");
    else
      aName.AssignLiteral("check");

    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}




NS_IMETHODIMP
XULCheckboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

PRUint64
XULCheckboxAccessible::NativeState()
{
  
  
  PRUint64 state = nsLeafAccessible::NativeState();
  
  state |= states::CHECKABLE;
  
  
  nsCOMPtr<nsIDOMXULCheckboxElement> xulCheckboxElement =
    do_QueryInterface(mContent);
  if (xulCheckboxElement) {
    bool checked = false;
    xulCheckboxElement->GetChecked(&checked);
    if (checked) {
      state |= states::CHECKED;
      PRInt32 checkState = 0;
      xulCheckboxElement->GetCheckState(&checkState);
      if (checkState == nsIDOMXULCheckboxElement::CHECKSTATE_MIXED)
        state |= states::MIXED;
    }
  }

  return state;
}





XULGroupboxAccessible::
  XULGroupboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
}

role
XULGroupboxAccessible::NativeRole()
{
  return roles::GROUPING;
}

nsresult
XULGroupboxAccessible::GetNameInternal(nsAString& aName)
{
  
  nsAccessible* label =
    RelationByType(nsIAccessibleRelation::RELATION_LABELLED_BY).Next();
  if (label)
    return label->GetName(aName);

  return NS_OK;
}

Relation
XULGroupboxAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = nsAccessibleWrap::RelationByType(aType);
  if (aType != nsIAccessibleRelation::RELATION_LABELLED_BY)
    return rel;

  
  
  
  PRInt32 childCount = GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible *childAcc = GetChildAt(childIdx);
    if (childAcc->Role() == roles::LABEL) {
      
      Relation reverseRel =
        childAcc->RelationByType(nsIAccessibleRelation::RELATION_LABEL_FOR);
      nsAccessible* testGroupbox = nsnull;
      while ((testGroupbox = reverseRel.Next()))
        if (testGroupbox == this) {
          
          rel.AppendTarget(childAcc);
        }
    }
  }

  return rel;
}





XULRadioButtonAccessible::
  XULRadioButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  RadioButtonAccessible(aContent, aDoc)
{
}

PRUint64
XULRadioButtonAccessible::NativeState()
{
  PRUint64 state = nsLeafAccessible::NativeState();
  state |= states::CHECKABLE;

  if (!(state & states::UNAVAILABLE))
    state |= states::FOCUSABLE;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> radioButton =
    do_QueryInterface(mContent);
  if (radioButton) {
    bool selected = false;   
    radioButton->GetSelected(&selected);
    if (selected) {
      state |= states::CHECKED;
    }
  }

  return state;
}




nsAccessible*
XULRadioButtonAccessible::ContainerWidget() const
{
  return mParent;
}















XULRadioGroupAccessible::
  XULRadioGroupAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  XULSelectControlAccessible(aContent, aDoc)
{ 
}

role
XULRadioGroupAccessible::NativeRole()
{
  return roles::GROUPING;
}

PRUint64
XULRadioGroupAccessible::NativeState()
{
  
  
  
  return nsAccessible::NativeState() & ~(states::FOCUSABLE | states::FOCUSED);
}




bool
XULRadioGroupAccessible::IsWidget() const
{
  return true;
}

bool
XULRadioGroupAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
XULRadioGroupAccessible::AreItemsOperable() const
{
  return true;
}






XULStatusBarAccessible::
  XULStatusBarAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
}

role
XULStatusBarAccessible::NativeRole()
{
  return roles::STATUSBAR;
}






XULToolbarButtonAccessible::
  XULToolbarButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  XULButtonAccessible(aContent, aDoc)
{
}

void
XULToolbarButtonAccessible::GetPositionAndSizeInternal(PRInt32* aPosInSet,
                                                       PRInt32* aSetSize)
{
  PRInt32 setSize = 0;
  PRInt32 posInSet = 0;

  nsAccessible* parent = Parent();
  if (!parent)
    return;

  PRInt32 childCount = parent->GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible* child = parent->GetChildAt(childIdx);
    if (IsSeparator(child)) { 
      if (posInSet)
        break; 

      setSize = 0; 

    } else {
      setSize++; 

      if (child == this)
        posInSet = setSize; 
    }
  }

  *aPosInSet = posInSet;
  *aSetSize = setSize;
}

bool
XULToolbarButtonAccessible::IsSeparator(nsAccessible* aAccessible)
{
  nsIContent* content = aAccessible->GetContent();
  return content && ((content->Tag() == nsGkAtoms::toolbarseparator) ||
                     (content->Tag() == nsGkAtoms::toolbarspacer) ||
                     (content->Tag() == nsGkAtoms::toolbarspring)); }






XULToolbarAccessible::
  XULToolbarAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
}

role
XULToolbarAccessible::NativeRole()
{
  return roles::TOOLBAR;
}

nsresult
XULToolbarAccessible::GetNameInternal(nsAString& aName)
{
  nsAutoString name;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::toolbarname, name)) {
    name.CompressWhitespace();
    aName = name;
  }

  return NS_OK;
}






XULToolbarSeparatorAccessible::
  XULToolbarSeparatorAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsLeafAccessible(aContent, aDoc)
{
}

role
XULToolbarSeparatorAccessible::NativeRole()
{
  return roles::SEPARATOR;
}

PRUint64
XULToolbarSeparatorAccessible::NativeState()
{
  return 0;
}





XULTextFieldAccessible::
 XULTextFieldAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
 nsHyperTextAccessibleWrap(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED3(XULTextFieldAccessible,
                             nsAccessible,
                             nsHyperTextAccessible,
                             nsIAccessibleText,
                             nsIAccessibleEditableText)




void
XULTextFieldAccessible::Value(nsString& aValue)
{
  aValue.Truncate();
  if (NativeRole() == roles::PASSWORD_TEXT) 
    return;

  nsCOMPtr<nsIDOMXULTextBoxElement> textBox(do_QueryInterface(mContent));
  if (textBox) {
    textBox->GetValue(aValue);
    return;
  }

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList)
    menuList->GetLabel(aValue);
}

void
XULTextFieldAccessible::ApplyARIAState(PRUint64* aState)
{
  nsHyperTextAccessibleWrap::ApplyARIAState(aState);

  aria::MapToState(aria::eARIAAutoComplete, mContent->AsElement(), aState);

}

PRUint64
XULTextFieldAccessible::NativeState()
{
  PRUint64 state = nsHyperTextAccessibleWrap::NativeState();

  nsCOMPtr<nsIContent> inputField(GetInputField());
  NS_ENSURE_TRUE(inputField, state);

  
  
  nsRefPtr<HTMLTextFieldAccessible> tempAccessible =
    new HTMLTextFieldAccessible(inputField, mDoc);
  if (!tempAccessible)
    return state;

  state |= tempAccessible->NativeState();

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList) {
    
    if (!mContent->AttrValueIs(kNameSpaceID_None,
                               nsGkAtoms::editable,
                               nsGkAtoms::_true, eIgnoreCase)) {
      state |= states::READONLY;
    }
  }

  return state;
}

role
XULTextFieldAccessible::NativeRole()
{
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            nsGkAtoms::password, eIgnoreCase))
    return roles::PASSWORD_TEXT;
  
  return roles::ENTRY;
}




PRUint8
XULTextFieldAccessible::ActionCount()
{
  return 1;
}




NS_IMETHODIMP
XULTextFieldAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("activate"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}




NS_IMETHODIMP
XULTextFieldAccessible::DoAction(PRUint8 index)
{
  if (index == 0) {
    nsCOMPtr<nsIDOMXULElement> element(do_QueryInterface(mContent));
    if (element)
    {
      element->Focus();
      return NS_OK;
    }
    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_INVALID_ARG;
}

bool
XULTextFieldAccessible::CanHaveAnonChildren()
{
  return false;
}

already_AddRefed<nsIEditor>
XULTextFieldAccessible::GetEditor() const
{
  nsCOMPtr<nsIContent> inputField = GetInputField();
  nsCOMPtr<nsIDOMNSEditableElement> editableElt(do_QueryInterface(inputField));
  if (!editableElt)
    return nsnull;

  nsCOMPtr<nsIEditor> editor;
  editableElt->GetEditor(getter_AddRefs(editor));
  return editor.forget();
}




void
XULTextFieldAccessible::CacheChildren()
{
  NS_ENSURE_TRUE(mDoc,);
  
  
  nsCOMPtr<nsIContent> inputContent(GetInputField());
  if (!inputContent)
    return;

  nsAccTreeWalker walker(mDoc, inputContent, false);

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild()) && AppendChild(child));
}




already_AddRefed<nsFrameSelection>
XULTextFieldAccessible::FrameSelection()
{
  nsCOMPtr<nsIContent> inputContent(GetInputField());
  nsIFrame* frame = inputContent->GetPrimaryFrame();
  return frame ? frame->GetFrameSelection() : nsnull;
}




already_AddRefed<nsIContent>
XULTextFieldAccessible::GetInputField() const
{
  nsCOMPtr<nsIDOMNode> inputFieldDOMNode;
  nsCOMPtr<nsIDOMXULTextBoxElement> textBox = do_QueryInterface(mContent);
  if (textBox) {
    textBox->GetInputField(getter_AddRefs(inputFieldDOMNode));

  } else {
    
    nsCOMPtr<nsIDOMXULMenuListElement> menuList = do_QueryInterface(mContent);
    if (menuList)
      menuList->GetInputField(getter_AddRefs(inputFieldDOMNode));
  }

  NS_ASSERTION(inputFieldDOMNode, "No input field for XULTextFieldAccessible");

  nsIContent* inputField = nsnull;
  if (inputFieldDOMNode)
    CallQueryInterface(inputFieldDOMNode, &inputField);

  return inputField;
}
