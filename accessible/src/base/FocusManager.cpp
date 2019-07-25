




































#include "FocusManager.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsRootAccessible.h"

#include "nsEventStateManager.h"
#include "nsFocusManager.h"

namespace dom = mozilla::dom;
using namespace mozilla::a11y;

FocusManager::FocusManager()
{
}

FocusManager::~FocusManager()
{
}

nsAccessible*
FocusManager::FocusedAccessible() const
{
  if (mActiveItem)
    return mActiveItem;

  nsINode* focusedNode = FocusedDOMNode();
  if (focusedNode)
    return GetAccService()->GetAccessibleOrContainer(focusedNode, nsnull);

  return nsnull;
}

bool
FocusManager::IsFocused(const nsAccessible* aAccessible) const
{
  if (mActiveItem)
    return mActiveItem == aAccessible;

  nsINode* focusedNode = FocusedDOMNode();
  if (focusedNode) {
    
    
    
    
    
    
    if (focusedNode->OwnerDoc() == aAccessible->GetNode()->OwnerDoc()) {
      return aAccessible ==
        GetAccService()->GetAccessibleOrContainer(focusedNode, nsnull);
    }
  }
  return false;
}

bool
FocusManager::IsFocusWithin(const nsAccessible* aContainer) const
{
  nsAccessible* child = FocusedAccessible();
  while (child) {
    if (child == aContainer)
      return true;

    child = child->Parent();
  }
  return false;
}

FocusManager::FocusDisposition
FocusManager::IsInOrContainsFocus(const nsAccessible* aAccessible) const
{
  nsAccessible* focus = FocusedAccessible();
  if (!focus)
    return eNone;

  
  if (focus == aAccessible)
    return eFocused;

  
  nsAccessible* child = focus->Parent();
  while (child) {
    if (child == aAccessible)
      return eContainsFocus;

    child = child->Parent();
  }

  
  child = aAccessible->Parent();
  while (child) {
    if (child == focus)
      return eContainedByFocus;

    child = child->Parent();
  }

  return eNone;
}

void
FocusManager::NotifyOfDOMFocus(nsISupports* aTarget)
{
  A11YDEBUG_FOCUS_NOTIFICATION_SUPPORTSTARGET("DOM focus", "DOM focus target",
                                              aTarget)

  mActiveItem = nsnull;

  nsCOMPtr<nsINode> targetNode(do_QueryInterface(aTarget));
  if (targetNode) {
    nsDocAccessible* document =
      GetAccService()->GetDocAccessible(targetNode->OwnerDoc());
    if (document) {
      
      if (targetNode->IsElement()) {
        nsRootAccessible* root = document->RootAccessible();
        nsCaretAccessible* caretAcc = root->GetCaretAccessible();
        caretAcc->SetControlSelectionListener(targetNode->AsElement());
      }

      document->HandleNotification<FocusManager, nsINode>
        (this, &FocusManager::ProcessDOMFocus, targetNode);
    }
  }
}

void
FocusManager::NotifyOfDOMBlur(nsISupports* aTarget)
{
  A11YDEBUG_FOCUS_NOTIFICATION_SUPPORTSTARGET("DOM blur", "DOM blur target",
                                              aTarget)

  mActiveItem = nsnull;

  
  
  nsCOMPtr<nsINode> targetNode(do_QueryInterface(aTarget));
  if (targetNode && targetNode->OwnerDoc() == FocusedDOMDocument()) {
    nsIDocument* DOMDoc = targetNode->OwnerDoc();
    nsDocAccessible* document =
      GetAccService()->GetDocAccessible(DOMDoc);
    if (document) {
      document->HandleNotification<FocusManager, nsINode>
        (this, &FocusManager::ProcessDOMFocus, DOMDoc);
    }
  }
}

void
FocusManager::ActiveItemChanged(nsAccessible* aItem, bool aCheckIfActive)
{
  A11YDEBUG_FOCUS_NOTIFICATION_ACCTARGET("active item changed",
                                         "Active item", aItem)

  
  if (aItem && aItem == mActiveItem)
    return;

  mActiveItem = nsnull;

  if (aItem && aCheckIfActive) {
    nsAccessible* widget = aItem->ContainerWidget();
    A11YDEBUG_FOCUS_LOG_WIDGET("Active item widget", widget)
    if (!widget || !widget->IsActiveWidget() || !widget->AreItemsOperable())
      return;
  }
  mActiveItem = aItem;

  
  
  
  nsAccessible* target = FocusedAccessible();
  if (target)
    DispatchFocusEvent(target->GetDocAccessible(), target);
}

void
FocusManager::ForceFocusEvent()
{
  nsINode* focusedNode = FocusedDOMNode();
  if (focusedNode) {
    nsDocAccessible* document =
      GetAccService()->GetDocAccessible(focusedNode->OwnerDoc());
    if (document) {
      document->HandleNotification<FocusManager, nsINode>
        (this, &FocusManager::ProcessDOMFocus, focusedNode);
    }
  }
}

void
FocusManager::DispatchFocusEvent(nsDocAccessible* aDocument,
                                 nsAccessible* aTarget)
{
  NS_PRECONDITION(aDocument, "No document for focused accessible!");
  if (aDocument) {
    nsRefPtr<AccEvent> event =
      new AccEvent(nsIAccessibleEvent::EVENT_FOCUS, aTarget,
                   eAutoDetect, AccEvent::eCoalesceOfSameType);
    aDocument->FireDelayedAccessibleEvent(event);

    A11YDEBUG_FOCUS_LOG_ACCTARGET("Focus notification", aTarget)
  }
}

void
FocusManager::ProcessDOMFocus(nsINode* aTarget)
{
  A11YDEBUG_FOCUS_NOTIFICATION_DOMTARGET("Process DOM focus",
                                         "Notification target", aTarget)

  nsDocAccessible* document =
    GetAccService()->GetDocAccessible(aTarget->OwnerDoc());

  nsAccessible* target = document->GetAccessibleOrContainer(aTarget);
  if (target) {
    
    
    nsAccessible* DOMFocus =
      GetAccService()->GetAccessibleOrContainer(FocusedDOMNode(), nsnull);
    if (target != DOMFocus)
      return;

    nsAccessible* activeItem = target->CurrentItem();
    if (activeItem) {
      mActiveItem = activeItem;
      target = activeItem;
    }

    DispatchFocusEvent(document, target);
  }
}

void
FocusManager::ProcessFocusEvent(AccEvent* aEvent)
{
  NS_PRECONDITION(aEvent->GetEventType() == nsIAccessibleEvent::EVENT_FOCUS,
                  "Focus event is expected!");

  EIsFromUserInput fromUserInputFlag = aEvent->IsFromUserInput() ?
    eFromUserInput : eNoUserInput;

  
  
  nsAccessible* target = aEvent->GetAccessible();
  if (target != mActiveItem) {
    
    
    nsAccessible* DOMFocus =
      GetAccService()->GetAccessibleOrContainer(FocusedDOMNode(), nsnull);
    if (target != DOMFocus)
      return;

    nsAccessible* activeItem = target->CurrentItem();
    if (activeItem) {
      mActiveItem = activeItem;
      target = activeItem;
    }
  }

  
  if (target->ARIARole() == nsIAccessibleRole::ROLE_MENUITEM) {
    
    nsAccessible* ARIAMenubar =
      nsAccUtils::GetAncestorWithRole(target, nsIAccessibleRole::ROLE_MENUBAR);

    if (ARIAMenubar != mActiveARIAMenubar) {
      
      if (mActiveARIAMenubar) {
        nsRefPtr<AccEvent> menuEndEvent =
          new AccEvent(nsIAccessibleEvent::EVENT_MENU_END, mActiveARIAMenubar,
                       fromUserInputFlag);
        nsEventShell::FireEvent(menuEndEvent);
      }

      mActiveARIAMenubar = ARIAMenubar;

      
      if (mActiveARIAMenubar) {
        nsRefPtr<AccEvent> menuStartEvent =
          new AccEvent(nsIAccessibleEvent::EVENT_MENU_START,
                       mActiveARIAMenubar, fromUserInputFlag);
        nsEventShell::FireEvent(menuStartEvent);
      }
    }
  } else if (mActiveARIAMenubar) {
    
    nsRefPtr<AccEvent> menuEndEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_MENU_END, mActiveARIAMenubar,
                   fromUserInputFlag);
    nsEventShell::FireEvent(menuEndEvent);

    mActiveARIAMenubar = nsnull;
  }

  A11YDEBUG_FOCUS_NOTIFICATION_ACCTARGET("FIRE FOCUS EVENT", "Focus target",
                                         target)

  nsRefPtr<AccEvent> focusEvent =
    new AccEvent(nsIAccessibleEvent::EVENT_FOCUS, target, fromUserInputFlag);
  nsEventShell::FireEvent(focusEvent);

  
  
  
  nsDocAccessible* targetDocument = target->GetDocAccessible();
  nsAccessible* anchorJump = targetDocument->AnchorJump();
  if (anchorJump) {
    if (target == targetDocument) {
      
      
      nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SCROLLING_START,
                              anchorJump, fromUserInputFlag);
    }
    targetDocument->SetAnchorJump(nsnull);
  }
}

nsINode*
FocusManager::FocusedDOMNode() const
{
  nsFocusManager* DOMFocusManager = nsFocusManager::GetFocusManager();
  nsIContent* focusedElm = DOMFocusManager->GetFocusedContent();

  
  
  
  if (focusedElm) {
    if (nsEventStateManager::IsRemoteTarget(focusedElm))
      return nsnull;
    return focusedElm;
  }

  
  nsCOMPtr<nsIDOMWindow> focusedWnd;
  DOMFocusManager->GetFocusedWindow(getter_AddRefs(focusedWnd));
  if (focusedWnd) {
    nsCOMPtr<nsIDOMDocument> DOMDoc;
    focusedWnd->GetDocument(getter_AddRefs(DOMDoc));
    nsCOMPtr<nsIDocument> DOMDocNode(do_QueryInterface(DOMDoc));
    return DOMDocNode;
  }
  return nsnull;
}

nsIDocument*
FocusManager::FocusedDOMDocument() const
{
  nsINode* focusedNode = FocusedDOMNode();
  return focusedNode ? focusedNode->OwnerDoc() : nsnull;
}
