




#include "nsXULTabAccessible.h"

#include "nsAccUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"


#include "nsIAccessibleRelation.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULRelatedElement.h"

using namespace mozilla::a11y;





nsXULTabAccessible::
  nsXULTabAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}




PRUint8
nsXULTabAccessible::ActionCount()
{
  return 1;
}


NS_IMETHODIMP nsXULTabAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Switch) {
    aName.AssignLiteral("switch"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP nsXULTabAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Switch) {
    nsCOMPtr<nsIDOMXULElement> tab(do_QueryInterface(mContent));
    if ( tab )
    {
      tab->Click();
      return NS_OK;
    }
    return NS_ERROR_FAILURE;
  }
  return NS_ERROR_INVALID_ARG;
}




role
nsXULTabAccessible::NativeRole()
{
  return roles::PAGETAB;
}

PRUint64
nsXULTabAccessible::NativeState()
{
  

  
  PRUint64 state = AccessibleWrap::NativeState();

  
  
  
  state &= ~states::FOCUSABLE;

  nsIFrame *frame = mContent->GetPrimaryFrame();
  if (frame) {
    const nsStyleUserInterface* ui = frame->GetStyleUserInterface();
    if (ui->mUserFocus == NS_STYLE_USER_FOCUS_NORMAL)
      state |= states::FOCUSABLE;
  }

  
  state |= states::SELECTABLE;
  state &= ~states::SELECTED;
  nsCOMPtr<nsIDOMXULSelectControlItemElement> tab(do_QueryInterface(mContent));
  if (tab) {
    bool selected = false;
    if (NS_SUCCEEDED(tab->GetSelected(&selected)) && selected)
      state |= states::SELECTED;
  }
  return state;
}


Relation
nsXULTabAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = AccessibleWrap::RelationByType(aType);
  if (aType != nsIAccessibleRelation::RELATION_LABEL_FOR)
    return rel;

  
  nsCOMPtr<nsIDOMXULRelatedElement> tabsElm =
    do_QueryInterface(mContent->GetParent());
  if (!tabsElm)
    return rel;

  nsCOMPtr<nsIDOMNode> domNode(DOMNode());
  nsCOMPtr<nsIDOMNode> tabpanelNode;
  tabsElm->GetRelatedElement(domNode, getter_AddRefs(tabpanelNode));
  if (!tabpanelNode)
    return rel;

  nsCOMPtr<nsIContent> tabpanelContent(do_QueryInterface(tabpanelNode));
  rel.AppendTarget(mDoc, tabpanelContent);
  return rel;
}






nsXULTabsAccessible::
  nsXULTabsAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  XULSelectControlAccessible(aContent, aDoc)
{
}

role
nsXULTabsAccessible::NativeRole()
{
  return roles::PAGETABLIST;
}

PRUint8
nsXULTabsAccessible::ActionCount()
{
  return 0;
}

void
nsXULTabsAccessible::Value(nsString& aValue)
{
  aValue.Truncate();
}

nsresult
nsXULTabsAccessible::GetNameInternal(nsAString& aName)
{
  
  return NS_OK;
}






nsXULTabpanelsAccessible::
  nsXULTabpanelsAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}

role
nsXULTabpanelsAccessible::NativeRole()
{
  return roles::PANE;
}






nsXULTabpanelAccessible::
  nsXULTabpanelAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}

role
nsXULTabpanelAccessible::NativeRole()
{
  return roles::PROPERTYPAGE;
}

Relation
nsXULTabpanelAccessible::RelationByType(PRUint32 aType)
{
  Relation rel = AccessibleWrap::RelationByType(aType);
  if (aType != nsIAccessibleRelation::RELATION_LABELLED_BY)
    return rel;

  
  nsCOMPtr<nsIDOMXULRelatedElement> tabpanelsElm =
    do_QueryInterface(mContent->GetParent());
  if (!tabpanelsElm)
    return rel;

  nsCOMPtr<nsIDOMNode> domNode(DOMNode());
  nsCOMPtr<nsIDOMNode> tabNode;
  tabpanelsElm->GetRelatedElement(domNode, getter_AddRefs(tabNode));
  if (!tabNode)
    return rel;

  nsCOMPtr<nsIContent> tabContent(do_QueryInterface(tabNode));
  rel.AppendTarget(mDoc, tabContent);
  return rel;
}
