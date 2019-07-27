




#include "XULTabAccessible.h"

#include "nsAccUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"


#include "nsIAccessibleRelation.h"
#include "nsIDocument.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULRelatedElement.h"

using namespace mozilla::a11y;





XULTabAccessible::
  XULTabAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}




uint8_t
XULTabAccessible::ActionCount()
{
  return 1;
}


void
XULTabAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  if (aIndex == eAction_Switch)
    aName.AssignLiteral("switch");
}

bool
XULTabAccessible::DoAction(uint8_t index)
{
  if (index == eAction_Switch) {
    nsCOMPtr<nsIDOMXULElement> tab(do_QueryInterface(mContent));
    if (tab) {
      tab->Click();
      return true;
    }
  }
  return false;
}




role
XULTabAccessible::NativeRole()
{
  return roles::PAGETAB;
}

uint64_t
XULTabAccessible::NativeState()
{
  

  
  uint64_t state = AccessibleWrap::NativeState();

  
  nsCOMPtr<nsIDOMXULSelectControlItemElement> tab(do_QueryInterface(mContent));
  if (tab) {
    bool selected = false;
    if (NS_SUCCEEDED(tab->GetSelected(&selected)) && selected)
      state |= states::SELECTED;

    if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::pinned,
                              nsGkAtoms::_true, eCaseMatters))
      state |= states::PINNED;

  }

  return state;
}

uint64_t
XULTabAccessible::NativeInteractiveState() const
{
  uint64_t state = Accessible::NativeInteractiveState();
  return (state & states::UNAVAILABLE) ? state : state | states::SELECTABLE;
}


Relation
XULTabAccessible::RelationByType(RelationType aType)
{
  Relation rel = AccessibleWrap::RelationByType(aType);
  if (aType != RelationType::LABEL_FOR)
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






XULTabsAccessible::
  XULTabsAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  XULSelectControlAccessible(aContent, aDoc)
{
}

role
XULTabsAccessible::NativeRole()
{
  return roles::PAGETABLIST;
}

uint8_t
XULTabsAccessible::ActionCount()
{
  return 0;
}

void
XULTabsAccessible::Value(nsString& aValue)
{
  aValue.Truncate();
}

ENameValueFlag
XULTabsAccessible::NativeName(nsString& aName)
{
  
  return eNameOK;
}






role
XULTabpanelsAccessible::NativeRole()
{
  return roles::PANE;
}





XULTabpanelAccessible::
  XULTabpanelAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}

role
XULTabpanelAccessible::NativeRole()
{
  return roles::PROPERTYPAGE;
}

Relation
XULTabpanelAccessible::RelationByType(RelationType aType)
{
  Relation rel = AccessibleWrap::RelationByType(aType);
  if (aType != RelationType::LABELLED_BY)
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
