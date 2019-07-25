






































#include "nsXULFormControlAccessible.h"

#include "nsAccUtils.h"
#include "nsAccTreeWalker.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"
#include "Relation.h"
#include "States.h"


#include "nsHTMLFormControlAccessible.h"
#include "nsXULMenuAccessible.h"
#include "nsIAccessibleRelation.h"
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





nsXULButtonAccessible::
  nsXULButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
  if (ContainsMenu())
    mFlags |= eMenuButtonAccessible;
}




NS_IMPL_ISUPPORTS_INHERITED0(nsXULButtonAccessible, nsAccessible)




PRUint8
nsXULButtonAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
nsXULButtonAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("press"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsXULButtonAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}




PRUint32
nsXULButtonAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
}

PRUint64
nsXULButtonAccessible::NativeState()
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
nsXULButtonAccessible::IsWidget() const
{
  return true;
}

bool
nsXULButtonAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
nsXULButtonAccessible::AreItemsOperable() const
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
nsXULButtonAccessible::ContainerWidget() const
{
  if (IsMenuButton() && mParent && mParent->IsAutoComplete())
    return mParent;
  return nsnull;
}




void
nsXULButtonAccessible::CacheChildren()
{
  
  
  

  
  
  bool isMenu = mContent->AttrValueIs(kNameSpaceID_None,
                                       nsGkAtoms::type,
                                       nsGkAtoms::menu,
                                       eCaseMatters);

  bool isMenuButton = isMenu ?
    PR_FALSE :
    mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                          nsGkAtoms::menuButton, eCaseMatters);

  if (!isMenu && !isMenuButton)
    return;

  nsAccessible* menupopup = nsnull;
  nsAccessible* button = nsnull;

  nsAccTreeWalker walker(mWeakShell, mContent, PR_TRUE);

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild())) {
    PRUint32 role = child->Role();

    if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
      
      menupopup = child;

    } else if (isMenuButton && role == nsIAccessibleRole::ROLE_PUSHBUTTON) {
      
      
      button = child;
      break;

    } else {
      
      GetDocAccessible()->UnbindFromDocument(child);
    }
  }

  if (!menupopup)
    return;

  AppendChild(menupopup);
  if (button)
    AppendChild(button);
}




bool
nsXULButtonAccessible::ContainsMenu()
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::menu, &nsGkAtoms::menuButton, nsnull};

  return mContent->FindAttrValueIn(kNameSpaceID_None,
                                   nsGkAtoms::type,
                                   strings, eCaseMatters) >= 0;
}





nsXULDropmarkerAccessible::
  nsXULDropmarkerAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsFormControlAccessible(aContent, aShell)
{
}

PRUint8
nsXULDropmarkerAccessible::ActionCount()
{
  return 1;
}

bool nsXULDropmarkerAccessible::DropmarkerOpen(bool aToggleOpen)
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




NS_IMETHODIMP nsXULDropmarkerAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    if (DropmarkerOpen(PR_FALSE))
      aName.AssignLiteral("close");
    else
      aName.AssignLiteral("open");
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}




NS_IMETHODIMP nsXULDropmarkerAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Click) {
    DropmarkerOpen(PR_TRUE); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

PRUint32
nsXULDropmarkerAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
}

PRUint64
nsXULDropmarkerAccessible::NativeState()
{
  return DropmarkerOpen(PR_FALSE) ? states::PRESSED : 0;
}





nsXULCheckboxAccessible::
  nsXULCheckboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsFormControlAccessible(aContent, aShell)
{
}

PRUint32
nsXULCheckboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_CHECKBUTTON;
}

PRUint8
nsXULCheckboxAccessible::ActionCount()
{
  return 1;
}




NS_IMETHODIMP nsXULCheckboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
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
nsXULCheckboxAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

PRUint64
nsXULCheckboxAccessible::NativeState()
{
  
  
  PRUint64 state = nsFormControlAccessible::NativeState();
  
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





nsXULGroupboxAccessible::
  nsXULGroupboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsXULGroupboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_GROUPING;
}

nsresult
nsXULGroupboxAccessible::GetNameInternal(nsAString& aName)
{
  
  nsAccessible* label =
    RelationByType(nsIAccessibleRelation::RELATION_LABELLED_BY).Next();
  if (label)
    return label->GetName(aName);

  return NS_OK;
}

Relation
nsXULGroupboxAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = nsAccessibleWrap::RelationByType(aType);
  if (aType != nsIAccessibleRelation::RELATION_LABELLED_BY)
    return rel;

  
  
  
  PRInt32 childCount = GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible *childAcc = GetChildAt(childIdx);
    if (childAcc->Role() == nsIAccessibleRole::ROLE_LABEL) {
      
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





nsXULRadioButtonAccessible::
  nsXULRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsRadioButtonAccessible(aContent, aShell)
{
}

PRUint64
nsXULRadioButtonAccessible::NativeState()
{
  PRUint64 state = nsFormControlAccessible::NativeState();
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

void
nsXULRadioButtonAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                                       PRInt32 *aSetSize)
{
  nsAccUtils::GetPositionAndSizeForXULSelectControlItem(mContent, aPosInSet,
                                                        aSetSize);
}




nsAccessible*
nsXULRadioButtonAccessible::ContainerWidget() const
{
  return mParent;
}















nsXULRadioGroupAccessible::
  nsXULRadioGroupAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULSelectableAccessible(aContent, aShell)
{ 
}

PRUint32
nsXULRadioGroupAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_GROUPING;
}

PRUint64
nsXULRadioGroupAccessible::NativeState()
{
  
  
  
  return nsAccessible::NativeState() & ~(states::FOCUSABLE | states::FOCUSED);
}




bool
nsXULRadioGroupAccessible::IsWidget() const
{
  return true;
}

bool
nsXULRadioGroupAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
nsXULRadioGroupAccessible::AreItemsOperable() const
{
  return true;
}






nsXULStatusBarAccessible::
  nsXULStatusBarAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsXULStatusBarAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_STATUSBAR;
}






nsXULToolbarButtonAccessible::
  nsXULToolbarButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULButtonAccessible(aContent, aShell)
{
}

void
nsXULToolbarButtonAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                                         PRInt32 *aSetSize)
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
nsXULToolbarButtonAccessible::IsSeparator(nsAccessible *aAccessible)
{
  nsCOMPtr<nsIDOMNode> domNode;
  aAccessible->GetDOMNode(getter_AddRefs(domNode));
  nsCOMPtr<nsIContent> contentDomNode(do_QueryInterface(domNode));

  if (!contentDomNode)
    return PR_FALSE;

  return (contentDomNode->Tag() == nsGkAtoms::toolbarseparator) ||
         (contentDomNode->Tag() == nsGkAtoms::toolbarspacer) ||
         (contentDomNode->Tag() == nsGkAtoms::toolbarspring);
}






nsXULToolbarAccessible::
  nsXULToolbarAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsXULToolbarAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_TOOLBAR;
}

nsresult
nsXULToolbarAccessible::GetNameInternal(nsAString& aName)
{
  nsAutoString name;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::toolbarname, name)) {
    name.CompressWhitespace();
    aName = name;
  }

  return NS_OK;
}






nsXULToolbarSeparatorAccessible::
  nsXULToolbarSeparatorAccessible(nsIContent *aContent,
                                  nsIWeakReference *aShell) :
  nsLeafAccessible(aContent, aShell)
{
}

PRUint32
nsXULToolbarSeparatorAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_SEPARATOR;
}

PRUint64
nsXULToolbarSeparatorAccessible::NativeState()
{
  return 0;
}





nsXULTextFieldAccessible::
 nsXULTextFieldAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
 nsHyperTextAccessibleWrap(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED3(nsXULTextFieldAccessible, nsAccessible, nsHyperTextAccessible, nsIAccessibleText, nsIAccessibleEditableText)




NS_IMETHODIMP nsXULTextFieldAccessible::GetValue(nsAString& aValue)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint64 state = NativeState();

  if (state & states::PROTECTED)    
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMXULTextBoxElement> textBox(do_QueryInterface(mContent));
  if (textBox) {
    return textBox->GetValue(aValue);
  }
  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList) {
    return menuList->GetLabel(aValue);
  }
  return NS_ERROR_FAILURE;
}

void
nsXULTextFieldAccessible::ApplyARIAState(PRUint64* aState)
{
  nsHyperTextAccessibleWrap::ApplyARIAState(aState);

  nsStateMapEntry::MapToStates(mContent, aState, eARIAAutoComplete);

}

PRUint64
nsXULTextFieldAccessible::NativeState()
{
  PRUint64 state = nsHyperTextAccessibleWrap::NativeState();

  nsCOMPtr<nsIContent> inputField(GetInputField());
  NS_ENSURE_TRUE(inputField, state);

  
  
  nsRefPtr<nsHTMLTextFieldAccessible> tempAccessible =
    new nsHTMLTextFieldAccessible(inputField, mWeakShell);
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

PRUint32
nsXULTextFieldAccessible::NativeRole()
{
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            nsGkAtoms::password, eIgnoreCase))
    return nsIAccessibleRole::ROLE_PASSWORD_TEXT;
  return nsIAccessibleRole::ROLE_ENTRY;
}




PRUint8
nsXULTextFieldAccessible::ActionCount()
{
  return 1;
}




NS_IMETHODIMP nsXULTextFieldAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("activate"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}




NS_IMETHODIMP nsXULTextFieldAccessible::DoAction(PRUint8 index)
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
nsXULTextFieldAccessible::GetAllowsAnonChildAccessibles()
{
  return PR_FALSE;
}

NS_IMETHODIMP nsXULTextFieldAccessible::GetAssociatedEditor(nsIEditor **aEditor)
{
  *aEditor = nsnull;

  nsCOMPtr<nsIContent> inputField = GetInputField();
  nsCOMPtr<nsIDOMNSEditableElement> editableElt(do_QueryInterface(inputField));
  NS_ENSURE_TRUE(editableElt, NS_ERROR_FAILURE);
  return editableElt->GetEditor(aEditor);
}




void
nsXULTextFieldAccessible::CacheChildren()
{
  
  
  nsCOMPtr<nsIContent> inputContent(GetInputField());
  if (!inputContent)
    return;

  nsAccTreeWalker walker(mWeakShell, inputContent, PR_FALSE);

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild()) && AppendChild(child));
}




already_AddRefed<nsFrameSelection>
nsXULTextFieldAccessible::FrameSelection()
{
  nsCOMPtr<nsIContent> inputContent(GetInputField());
  nsIFrame* frame = inputContent->GetPrimaryFrame();
  return frame->GetFrameSelection();
}




already_AddRefed<nsIContent>
nsXULTextFieldAccessible::GetInputField() const
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

  NS_ASSERTION(inputFieldDOMNode, "No input field for nsXULTextFieldAccessible");

  nsIContent* inputField = nsnull;
  if (inputFieldDOMNode)
    CallQueryInterface(inputFieldDOMNode, &inputField);

  return inputField;
}
