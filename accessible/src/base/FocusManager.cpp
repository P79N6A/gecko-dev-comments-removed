



#include "FocusManager.h"

#include "Accessible-inl.h"
#include "AccIterator.h"
#include "DocAccessible-inl.h"
#include "HTMLImageMapAccessible.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsEventShell.h"
#include "nsImageFrame.h"
#include "Role.h"

#include "nsEventStateManager.h"
#include "nsFocusManager.h"
#include "mozilla/dom/Element.h"

namespace dom = mozilla::dom;
using namespace mozilla::a11y;

FocusManager::FocusManager()
{
}

FocusManager::~FocusManager()
{
}

Accessible*
FocusManager::FocusedAccessible() const
{
  if (mActiveItem)
    return mActiveItem;

  nsINode* focusedNode = FocusedDOMNode();
  if (focusedNode) {
    DocAccessible* doc = 
      GetAccService()->GetDocAccessible(focusedNode->OwnerDoc());
    return doc ? GetFocusableAccessibleFor(focusedNode, doc) : nullptr;
  }

  return nullptr;
}

bool
FocusManager::IsFocused(const Accessible* aAccessible) const
{
  if (mActiveItem)
    return mActiveItem == aAccessible;

  nsINode* focusedNode = FocusedDOMNode();
  if (focusedNode) {
    
    
    
    
    
    
    if (focusedNode->OwnerDoc() == aAccessible->GetNode()->OwnerDoc()) {
      DocAccessible* doc = 
        GetAccService()->GetDocAccessible(focusedNode->OwnerDoc());
      return aAccessible ==
        (doc ? GetFocusableAccessibleFor(focusedNode, doc) : nullptr);
    }
  }
  return false;
}

bool
FocusManager::IsFocusWithin(const Accessible* aContainer) const
{
  Accessible* child = FocusedAccessible();
  while (child) {
    if (child == aContainer)
      return true;

    child = child->Parent();
  }
  return false;
}

FocusManager::FocusDisposition
FocusManager::IsInOrContainsFocus(const Accessible* aAccessible) const
{
  Accessible* focus = FocusedAccessible();
  if (!focus)
    return eNone;

  
  if (focus == aAccessible)
    return eFocused;

  
  Accessible* child = focus->Parent();
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
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eFocus))
    logging::FocusNotificationTarget("DOM focus", "Target", aTarget);
#endif

  mActiveItem = nullptr;

  nsCOMPtr<nsINode> targetNode(do_QueryInterface(aTarget));
  if (targetNode) {
    DocAccessible* document =
      GetAccService()->GetDocAccessible(targetNode->OwnerDoc());
    if (document) {
      
      if (targetNode->IsElement())
        SelectionMgr()->SetControlSelectionListener(targetNode->AsElement());

      document->HandleNotification<FocusManager, nsINode>
        (this, &FocusManager::ProcessDOMFocus, targetNode);
    }
  }
}

void
FocusManager::NotifyOfDOMBlur(nsISupports* aTarget)
{
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eFocus))
    logging::FocusNotificationTarget("DOM blur", "Target", aTarget);
#endif

  mActiveItem = nullptr;

  
  
  nsCOMPtr<nsINode> targetNode(do_QueryInterface(aTarget));
  if (targetNode && targetNode->OwnerDoc() == FocusedDOMDocument()) {
    nsIDocument* DOMDoc = targetNode->OwnerDoc();
    DocAccessible* document =
      GetAccService()->GetDocAccessible(DOMDoc);
    if (document) {
      
      if (targetNode->IsElement())
        SelectionMgr()->ClearControlSelectionListener();

      document->HandleNotification<FocusManager, nsINode>
        (this, &FocusManager::ProcessDOMFocus, DOMDoc);
    }
  }
}

void
FocusManager::ActiveItemChanged(Accessible* aItem, bool aCheckIfActive)
{
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eFocus))
    logging::FocusNotificationTarget("active item changed", "Item", aItem);
#endif

  
  if (aItem && aItem == mActiveItem)
    return;

  mActiveItem = nullptr;

  if (aItem && aCheckIfActive) {
    Accessible* widget = aItem->ContainerWidget();
#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eFocus))
      logging::ActiveWidget(widget);
#endif
    if (!widget || !widget->IsActiveWidget() || !widget->AreItemsOperable())
      return;
  }
  mActiveItem = aItem;

  
  
  
  Accessible* target = FocusedAccessible();
  if (target)
    DispatchFocusEvent(target->Document(), target);
}

void
FocusManager::ForceFocusEvent()
{
  nsINode* focusedNode = FocusedDOMNode();
  if (focusedNode) {
    DocAccessible* document =
      GetAccService()->GetDocAccessible(focusedNode->OwnerDoc());
    if (document) {
      document->HandleNotification<FocusManager, nsINode>
        (this, &FocusManager::ProcessDOMFocus, focusedNode);
    }
  }
}

void
FocusManager::DispatchFocusEvent(DocAccessible* aDocument,
                                 Accessible* aTarget)
{
  NS_PRECONDITION(aDocument, "No document for focused accessible!");
  if (aDocument) {
    nsRefPtr<AccEvent> event =
      new AccEvent(nsIAccessibleEvent::EVENT_FOCUS, aTarget,
                   eAutoDetect, AccEvent::eCoalesceOfSameType);
    aDocument->FireDelayedEvent(event);

#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eFocus))
      logging::FocusDispatched(aTarget);
#endif
  }
}

void
FocusManager::ProcessDOMFocus(nsINode* aTarget)
{
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eFocus))
    logging::FocusNotificationTarget("process DOM focus", "Target", aTarget);
#endif

  DocAccessible* document =
    GetAccService()->GetDocAccessible(aTarget->OwnerDoc());

  Accessible* target = GetFocusableAccessibleFor(aTarget, document);
  if (target && document) {
    
    
    nsINode* focusedNode = FocusedDOMNode();
    if (!focusedNode)
      return;

    Accessible* DOMFocus = GetFocusableAccessibleFor(focusedNode, document);
    if (target != DOMFocus)
      return;

    Accessible* activeItem = target->CurrentItem();
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

  
  
  Accessible* target = aEvent->GetAccessible();
  if (target != mActiveItem) {

    
    
    DocAccessible* document = aEvent->GetDocAccessible();
    nsINode* focusedNode = FocusedDOMNode();
    if (!focusedNode)
      return;

    Accessible* DOMFocus = GetFocusableAccessibleFor(focusedNode, document);
    if (target != DOMFocus)
      return;

    Accessible* activeItem = target->CurrentItem();
    if (activeItem) {
      mActiveItem = activeItem;
      target = activeItem;
    }
  }

  
  if (target->IsARIARole(nsGkAtoms::menuitem)) {
    
    Accessible* ARIAMenubar = nullptr;
    Accessible* child = target;
    Accessible* parent = child->Parent();
    while (parent) {
      nsRoleMapEntry* roleMap = parent->ARIARoleMap();
      if (roleMap) {
        if (roleMap->Is(nsGkAtoms::menubar)) {
          ARIAMenubar = parent;
          break;
        }

        
        if (roleMap->Is(nsGkAtoms::menuitem) || roleMap->Is(nsGkAtoms::menu)) {
          child = parent;
          parent = child->Parent();
          continue;
        }
      }

      
      RelatedAccIterator iter(child->Document(), child->GetContent(),
                              nsGkAtoms::aria_owns);
      parent = iter.Next();
    }

    if (ARIAMenubar != mActiveARIAMenubar) {
      
      if (mActiveARIAMenubar) {
        nsRefPtr<AccEvent> menuEndEvent =
          new AccEvent(nsIAccessibleEvent::EVENT_MENU_END, mActiveARIAMenubar,
                       aEvent->FromUserInput());
        nsEventShell::FireEvent(menuEndEvent);
      }

      mActiveARIAMenubar = ARIAMenubar;

      
      if (mActiveARIAMenubar) {
        nsRefPtr<AccEvent> menuStartEvent =
          new AccEvent(nsIAccessibleEvent::EVENT_MENU_START,
                       mActiveARIAMenubar, aEvent->FromUserInput());
        nsEventShell::FireEvent(menuStartEvent);
      }
    }
  } else if (mActiveARIAMenubar) {
    
    nsRefPtr<AccEvent> menuEndEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_MENU_END, mActiveARIAMenubar,
                   aEvent->FromUserInput());
    nsEventShell::FireEvent(menuEndEvent);

    mActiveARIAMenubar = nullptr;
  }

#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eFocus))
    logging::FocusNotificationTarget("fire focus event", "Target", target);
#endif

  nsRefPtr<AccEvent> focusEvent =
    new AccEvent(nsIAccessibleEvent::EVENT_FOCUS, target, aEvent->FromUserInput());
  nsEventShell::FireEvent(focusEvent);

  
  
  
  DocAccessible* targetDocument = target->Document();
  Accessible* anchorJump = targetDocument->AnchorJump();
  if (anchorJump) {
    if (target == targetDocument) {
      
      
      nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SCROLLING_START,
                              anchorJump, aEvent->FromUserInput());
    }
    targetDocument->SetAnchorJump(nullptr);
  }
}

nsINode*
FocusManager::FocusedDOMNode() const
{
  nsFocusManager* DOMFocusManager = nsFocusManager::GetFocusManager();
  nsIContent* focusedElm = DOMFocusManager->GetFocusedContent();

  
  
  
  if (focusedElm) {
    if (nsEventStateManager::IsRemoteTarget(focusedElm))
      return nullptr;
    return focusedElm;
  }

  
  nsPIDOMWindow* focusedWnd = DOMFocusManager->GetFocusedWindow();
  return focusedWnd ? focusedWnd->GetExtantDoc() : nullptr;
}

nsIDocument*
FocusManager::FocusedDOMDocument() const
{
  nsINode* focusedNode = FocusedDOMNode();
  return focusedNode ? focusedNode->OwnerDoc() : nullptr;
}

Accessible*
FocusManager::GetFocusableAccessibleFor(nsINode* aNode,
                                        DocAccessible* aDoc) const
{
  if (!aNode->IsContent() || !aNode->AsContent()->IsHTML(nsGkAtoms::area))
    return aDoc->GetAccessibleOrContainer(aNode);

  
  nsIFrame* frame = aNode->AsContent()->GetPrimaryFrame();
  nsImageFrame* imageFrame = do_QueryFrame(frame);
  if (imageFrame) {
    Accessible* parent = aDoc->GetAccessible(imageFrame->GetContent());
    if (parent) {
      Accessible* area =
        parent->AsImageMap()->GetChildAccessibleFor(aNode);
      if (area)
        return area;

      return nullptr;
    }
  }

  return aDoc->GetAccessibleOrContainer(aNode);
}
