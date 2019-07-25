




































#include "nsAccessibilityService.h"
#include "nsApplicationAccessibleWrap.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsRelUtils.h"

#include "mozilla/dom/Element.h"
#include "nsHTMLSelectAccessible.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMDataContainerEvent.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULPopupElement.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIFrame.h"
#include "nsIMenuFrame.h"
#include "nsIHTMLDocument.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsISelectionPrivate.h"
#include "nsIServiceManager.h"
#include "nsPIDOMWindow.h"
#include "nsIWebBrowserChrome.h"
#include "nsReadableUtils.h"
#include "nsRootAccessible.h"
#include "nsIDOMNSEventTarget.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsFocusManager.h"
#include "mozilla/dom/Element.h"


#ifdef MOZ_XUL
#include "nsXULTreeAccessible.h"
#include "nsIXULDocument.h"
#include "nsIXULWindow.h"
#endif

using namespace mozilla;






NS_IMPL_QUERY_HEAD(nsRootAccessible)
NS_IMPL_QUERY_BODY(nsIDOMEventListener)
if (aIID.Equals(NS_GET_IID(nsRootAccessible)))
  foundInterface = reinterpret_cast<nsISupports*>(this);
else
NS_IMPL_QUERY_TAIL_INHERITING(nsDocAccessible)

NS_IMPL_ADDREF_INHERITED(nsRootAccessible, nsDocAccessible) 
NS_IMPL_RELEASE_INHERITED(nsRootAccessible, nsDocAccessible)




nsRootAccessible::
  nsRootAccessible(nsIDocument *aDocument, nsIContent *aRootContent,
                   nsIWeakReference *aShell) :
  nsDocAccessibleWrap(aDocument, aRootContent, aShell)
{
}

nsRootAccessible::~nsRootAccessible()
{
}





NS_IMETHODIMP
nsRootAccessible::GetName(nsAString& aName)
{
  aName.Truncate();

  if (!mDocument) {
    return NS_ERROR_FAILURE;
  }

  if (mRoleMapEntry) {
    nsAccessible::GetName(aName);
    if (!aName.IsEmpty()) {
      return NS_OK;
    }
  }

  nsCOMPtr<nsIDOMNSDocument> document(do_QueryInterface(mDocument));
  return document->GetTitle(aName);
}

PRUint32
nsRootAccessible::NativeRole()
{
  
  dom::Element *root = mDocument->GetRootElement();
  if (root) {
    nsCOMPtr<nsIDOMElement> rootElement(do_QueryInterface(root));
    if (rootElement) {
      nsAutoString name;
      rootElement->GetLocalName(name);
      if (name.EqualsLiteral("dialog") || name.EqualsLiteral("wizard")) {
        return nsIAccessibleRole::ROLE_DIALOG; 
      }
    }
  }

  return nsDocAccessibleWrap::NativeRole();
}


#ifdef MOZ_XUL
PRUint32 nsRootAccessible::GetChromeFlags()
{
  
  
  
  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDocument);
  NS_ENSURE_TRUE(treeItem, 0);
  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  treeItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_TRUE(treeOwner, 0);
  nsCOMPtr<nsIXULWindow> xulWin(do_GetInterface(treeOwner));
  if (!xulWin) {
    return 0;
  }
  PRUint32 chromeFlags;
  xulWin->GetChromeFlags(&chromeFlags);
  return chromeFlags;
}
#endif

nsresult
nsRootAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsDocAccessibleWrap::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

#ifdef MOZ_XUL
  PRUint32 chromeFlags = GetChromeFlags();
  if (chromeFlags & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE) {
    *aState |= nsIAccessibleStates::STATE_SIZEABLE;
  }
  if (chromeFlags & nsIWebBrowserChrome::CHROME_TITLEBAR) {
    
    
    
    *aState |= nsIAccessibleStates::STATE_MOVEABLE;
  }
#endif

  if (!aExtraState)
    return NS_OK;

  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm) {
    nsCOMPtr<nsIDOMWindow> rootWindow;
    GetWindow(getter_AddRefs(rootWindow));

    nsCOMPtr<nsIDOMWindow> activeWindow;
    fm->GetActiveWindow(getter_AddRefs(activeWindow));
    if (activeWindow == rootWindow)
      *aExtraState |= nsIAccessibleStates::EXT_STATE_ACTIVE;
  }

#ifdef MOZ_XUL
  if (GetChromeFlags() & nsIWebBrowserChrome::CHROME_MODAL) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_MODAL;
  }
#endif

  return NS_OK;
}

const char* const docEvents[] = {
#ifdef DEBUG
  
  
  "mouseover",
#endif
  
  "focus",
  "blur",
  
  "select",
  
  "ValueChange",
  
  "AlertActive",
  
  "TreeViewChanged",
  "TreeRowCountChanged",
  "TreeInvalidated",
  
  "OpenStateChange",
  
  "CheckboxStateChange",
  
  "RadioStateChange",
  "popupshown",
  "popuphiding",
  "DOMMenuInactive",
  "DOMMenuItemActive",
  "DOMMenuBarActive",
  "DOMMenuBarInactive"
};

nsresult nsRootAccessible::AddEventListeners()
{
  
  
  
  
  nsCOMPtr<nsIDOMNSEventTarget> nstarget(do_QueryInterface(mDocument));

  if (nstarget) {
    for (const char* const* e = docEvents,
                   * const* e_end = docEvents + NS_ARRAY_LENGTH(docEvents);
         e < e_end; ++e) {
      nsresult rv = nstarget->AddEventListener(NS_ConvertASCIItoUTF16(*e),
                                               this, PR_TRUE, PR_TRUE, 1);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (!mCaretAccessible) {
    mCaretAccessible = new nsCaretAccessible(this);
  }

  return nsDocAccessible::AddEventListeners();
}

nsresult nsRootAccessible::RemoveEventListeners()
{
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mDocument));
  if (target) { 
    for (const char* const* e = docEvents,
                   * const* e_end = docEvents + NS_ARRAY_LENGTH(docEvents);
         e < e_end; ++e) {
      nsresult rv = target->RemoveEventListener(NS_ConvertASCIItoUTF16(*e), this, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  nsDocAccessible::RemoveEventListeners();

  if (mCaretAccessible) {
    mCaretAccessible->Shutdown();
    mCaretAccessible = nsnull;
  }

  return NS_OK;
}

nsCaretAccessible*
nsRootAccessible::GetCaretAccessible()
{
  return mCaretAccessible;
}

PRBool
nsRootAccessible::FireAccessibleFocusEvent(nsAccessible *aAccessible,
                                           nsINode *aNode,
                                           nsIDOMEvent *aFocusEvent,
                                           PRBool aForceEvent,
                                           EIsFromUserInput aIsFromUserInput)
{
  

  if (mCaretAccessible) {
    nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aFocusEvent));
    if (nsevent) {
      
      
      
      
      
      
      
      
      nsCOMPtr<nsIDOMEventTarget> domEventTarget;
      nsevent->GetOriginalTarget(getter_AddRefs(domEventTarget));
      nsCOMPtr<nsIContent> realFocusedNode(do_QueryInterface(domEventTarget));
      if (!realFocusedNode) {
        
        
        
        realFocusedNode = do_QueryInterface(aNode);
      }
      if (realFocusedNode) {
        mCaretAccessible->SetControlSelectionListener(realFocusedNode);
      }
    }
  }

  
  nsINode *finalFocusNode = aNode;
  nsAccessible *finalFocusAccessible = aAccessible;

  nsIContent *finalFocusContent = nsCoreUtils::GetRoleContent(finalFocusNode);
  if (finalFocusContent) {
    nsAutoString id;
    if (finalFocusContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_activedescendant, id)) {
      nsIDocument *doc = aNode->GetOwnerDoc();
      finalFocusNode = doc->GetElementById(id);
      if (!finalFocusNode) {
        
        
        finalFocusNode = aNode;
      }
      finalFocusAccessible = nsnull;
    }
  }

  
  if (gLastFocusedNode == finalFocusNode && !aForceEvent) {
    return PR_FALSE;
  }

  if (!finalFocusAccessible) {
    finalFocusAccessible = GetAccService()->GetAccessible(finalFocusNode);
    
    
    
    if (!finalFocusAccessible) {
      return PR_FALSE;
    }
  }

  gLastFocusedAccessiblesState = nsAccUtils::State(finalFocusAccessible);
  PRUint32 role = finalFocusAccessible->Role();
  if (role == nsIAccessibleRole::ROLE_MENUITEM) {
    if (!mCurrentARIAMenubar) {  
      
      if (role != finalFocusAccessible->NativeRole()) { 
        nsAccessible *menuBarAccessible =
          nsAccUtils::GetAncestorWithRole(finalFocusAccessible,
                                          nsIAccessibleRole::ROLE_MENUBAR);
        if (menuBarAccessible) {
          mCurrentARIAMenubar = menuBarAccessible->GetNode();
          if (mCurrentARIAMenubar) {
            nsRefPtr<AccEvent> menuStartEvent =
              new AccEvent(nsIAccessibleEvent::EVENT_MENU_START,
                           menuBarAccessible, aIsFromUserInput,
                           AccEvent::eAllowDupes);
            if (menuStartEvent) {
              FireDelayedAccessibleEvent(menuStartEvent);
            }
          }
        }
      }
    }
  }
  else if (mCurrentARIAMenubar) {
    nsRefPtr<AccEvent> menuEndEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_MENU_END, mCurrentARIAMenubar,
                   aIsFromUserInput, AccEvent::eAllowDupes);
    if (menuEndEvent) {
      FireDelayedAccessibleEvent(menuEndEvent);
    }
    mCurrentARIAMenubar = nsnull;
  }

  nsCOMPtr<nsIContent> focusContent = do_QueryInterface(finalFocusNode);
  nsIFrame *focusFrame = nsnull;
  if (focusContent) {
    nsIPresShell *shell = nsCoreUtils::GetPresShellFor(finalFocusNode);

    NS_ASSERTION(shell, "No pres shell for final focus node!");
    if (!shell)
      return PR_FALSE;

    focusFrame = focusContent->GetPrimaryFrame();
  }

  NS_IF_RELEASE(gLastFocusedNode);
  gLastFocusedNode = finalFocusNode;
  NS_IF_ADDREF(gLastFocusedNode);

  
  
  FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_FOCUS,
                             finalFocusNode, AccEvent::eCoalesceFromSameDocument,
                             aIsFromUserInput);

  return PR_TRUE;
}

void
nsRootAccessible::FireCurrentFocusEvent()
{
  if (IsDefunct())
    return;

  
  
  nsCOMPtr<nsINode> focusedNode = GetCurrentFocus();
  if (!focusedNode) {
    return; 
  }

  nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(mDocument);
  if (docEvent) {
    nsCOMPtr<nsIDOMEvent> event;
    if (NS_SUCCEEDED(docEvent->CreateEvent(NS_LITERAL_STRING("Events"),
                                           getter_AddRefs(event))) &&
        NS_SUCCEEDED(event->InitEvent(NS_LITERAL_STRING("focus"), PR_TRUE, PR_TRUE))) {

      nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
      nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(focusedNode));
      privateEvent->SetTarget(target);
      HandleEvent(event);
    }
  }
}




NS_IMETHODIMP
nsRootAccessible::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aEvent));
  NS_ENSURE_STATE(nsevent);

  nsCOMPtr<nsIDOMEventTarget> domEventTarget;
  nsevent->GetOriginalTarget(getter_AddRefs(domEventTarget));
  nsCOMPtr<nsINode> origTarget(do_QueryInterface(domEventTarget));
  NS_ENSURE_STATE(origTarget);

  nsAutoString eventType;
  aEvent->GetType(eventType);

  nsCOMPtr<nsIWeakReference> weakShell =
    nsCoreUtils::GetWeakShellFor(origTarget);
  if (!weakShell)
    return NS_OK;

  nsAccessible* accessible =
    GetAccService()->GetAccessibleOrContainer(origTarget, weakShell);

  if (eventType.EqualsLiteral("popuphiding"))
    return HandlePopupHidingEvent(origTarget, accessible);

  if (!accessible)
    return NS_OK;

  nsINode* targetNode = accessible->GetNode();
  nsIContent* targetContent = targetNode->IsElement() ?
    targetNode->AsElement() : nsnull;
#ifdef MOZ_XUL
  PRBool isTree = targetContent ?
    targetContent->NodeInfo()->Equals(nsAccessibilityAtoms::tree,
                                      kNameSpaceID_XUL) : PR_FALSE;

  if (isTree) {
    nsRefPtr<nsXULTreeAccessible> treeAcc = do_QueryObject(accessible);
    NS_ASSERTION(treeAcc,
                 "Accessible for xul:tree isn't nsXULTreeAccessible.");

    if (treeAcc) {
      if (eventType.EqualsLiteral("TreeViewChanged")) {
        treeAcc->TreeViewChanged();
        return NS_OK;
      }

      if (eventType.EqualsLiteral("TreeRowCountChanged"))
        return HandleTreeRowCountChangedEvent(aEvent, treeAcc);
      
      if (eventType.EqualsLiteral("TreeInvalidated"))
        return HandleTreeInvalidatedEvent(aEvent, treeAcc);
    }
  }
#endif

  if (eventType.EqualsLiteral("RadioStateChange")) {
    PRUint32 state = nsAccUtils::State(accessible);

    
    
    
    
    PRBool isEnabled = (state & (nsIAccessibleStates::STATE_CHECKED |
                        nsIAccessibleStates::STATE_SELECTED)) != 0;

    nsRefPtr<AccEvent> accEvent =
      new AccStateChangeEvent(accessible, nsIAccessibleStates::STATE_CHECKED,
                              PR_FALSE, isEnabled);
    nsEventShell::FireEvent(accEvent);

    if (isEnabled)
      FireAccessibleFocusEvent(accessible, targetNode, aEvent);

    return NS_OK;
  }

  if (eventType.EqualsLiteral("CheckboxStateChange")) {
    PRUint32 state = nsAccUtils::State(accessible);

    PRBool isEnabled = !!(state & nsIAccessibleStates::STATE_CHECKED);

    nsRefPtr<AccEvent> accEvent =
      new AccStateChangeEvent(accessible, nsIAccessibleStates::STATE_CHECKED,
                              PR_FALSE, isEnabled);

    nsEventShell::FireEvent(accEvent);
    return NS_OK;
  }

  nsAccessible *treeItemAccessible = nsnull;
#ifdef MOZ_XUL
  
  if (isTree) {
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
      do_QueryInterface(targetNode);
    if (multiSelect) {
      PRInt32 treeIndex = -1;
      multiSelect->GetCurrentIndex(&treeIndex);
      if (treeIndex >= 0) {
        nsRefPtr<nsXULTreeAccessible> treeAcc = do_QueryObject(accessible);
        if (treeAcc) {
          treeItemAccessible = treeAcc->GetTreeItemAccessible(treeIndex);
          if (treeItemAccessible)
            accessible = treeItemAccessible;
        }
      }
    }
  }
#endif

#ifdef MOZ_XUL
  if (treeItemAccessible && eventType.EqualsLiteral("OpenStateChange")) {
    PRUint32 state = nsAccUtils::State(accessible); 
    PRBool isEnabled = (state & nsIAccessibleStates::STATE_EXPANDED) != 0;

    nsRefPtr<AccEvent> accEvent =
      new AccStateChangeEvent(accessible, nsIAccessibleStates::STATE_EXPANDED,
                              PR_FALSE, isEnabled);
    nsEventShell::FireEvent(accEvent);
    return NS_OK;
  }

  if (treeItemAccessible && eventType.EqualsLiteral("select")) {
    
    if (gLastFocusedNode == targetNode) {
      nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSel =
        do_QueryInterface(targetNode);
      nsAutoString selType;
      multiSel->GetSelType(selType);
      if (selType.IsEmpty() || !selType.EqualsLiteral("single")) {
        
        
        
        
        nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SELECTION_WITHIN,
                                accessible);
        return NS_OK;
      }

      nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SELECTION,
                              treeItemAccessible);
      return NS_OK;
    }
  }
  else
#endif
  if (eventType.EqualsLiteral("focus")) {
    if (targetNode == mDocument && mDocument != gLastFocusedNode) {
      
      
      
      NS_DISPATCH_RUNNABLEMETHOD(FireCurrentFocusEvent, this)
    }

    
    
    
    nsCOMPtr<nsINode> focusedItem = targetNode;
    if (!treeItemAccessible) {
      nsCOMPtr<nsIDOMXULSelectControlElement> selectControl =
        do_QueryInterface(targetNode);
      if (selectControl) {
        nsCOMPtr<nsIDOMXULMenuListElement> menuList =
          do_QueryInterface(targetNode);
        if (!menuList) {
          
          
          nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
          selectControl->GetSelectedItem(getter_AddRefs(selectedItem));
          if (selectedItem)
            focusedItem = do_QueryInterface(selectedItem);

          if (!focusedItem)
            return NS_OK;

          accessible = GetAccService()->GetAccessibleInWeakShell(focusedItem,
                                                                 weakShell);
          if (!accessible)
            return NS_OK;
        }
      }
    }
    FireAccessibleFocusEvent(accessible, focusedItem, aEvent);
  }
  else if (eventType.EqualsLiteral("blur")) {
    NS_IF_RELEASE(gLastFocusedNode);
    gLastFocusedAccessiblesState = 0;
  }
  else if (eventType.EqualsLiteral("AlertActive")) { 
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_ALERT, accessible);
  }
  else if (eventType.EqualsLiteral("popupshown")) {
    HandlePopupShownEvent(accessible);
  }
  else if (eventType.EqualsLiteral("DOMMenuInactive")) {
    if (accessible->Role() == nsIAccessibleRole::ROLE_MENUPOPUP) {
      nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_END,
                              accessible);
    }
  }
  else if (eventType.EqualsLiteral("DOMMenuItemActive")) {
    PRBool fireFocus = PR_FALSE;
    if (!treeItemAccessible) {
#ifdef MOZ_XUL
      if (isTree) {
        return NS_OK; 
      }
#endif
      nsIFrame* menuFrame = accessible->GetFrame();
      NS_ENSURE_TRUE(menuFrame, NS_ERROR_FAILURE);

      nsIMenuFrame* imenuFrame = do_QueryFrame(menuFrame);
      if (imenuFrame)
        fireFocus = PR_TRUE;
      
      if (imenuFrame && imenuFrame->IsOnMenuBar() &&
                       !imenuFrame->IsOnActiveMenuBar()) {
        
        
        return NS_OK;
      } else {
        nsAccessible *containerAccessible = accessible->GetParent();
        NS_ENSURE_TRUE(containerAccessible, NS_ERROR_FAILURE);
        
        
        
        if (nsAccUtils::State(containerAccessible) & nsIAccessibleStates::STATE_COLLAPSED) {
          nsAccessible *containerParent = containerAccessible->GetParent();
          NS_ENSURE_TRUE(containerParent, NS_ERROR_FAILURE);
          if (containerParent->Role() != nsIAccessibleRole::ROLE_COMBOBOX) {
            return NS_OK;
          }
        }
      }
    }
    if (!fireFocus) {
      nsCOMPtr<nsINode> realFocusedNode = GetCurrentFocus();
      nsIContent* realFocusedContent =
        realFocusedNode->IsElement() ? realFocusedNode->AsElement() : nsnull;
      nsIContent* containerContent = targetContent;
      while (containerContent) {
        nsCOMPtr<nsIDOMXULPopupElement> popup = do_QueryInterface(containerContent);
        if (popup || containerContent == realFocusedContent) { 
          
          
          fireFocus = PR_TRUE;
          break;
        }
        containerContent = containerContent->GetParent();
      }
    }
    if (fireFocus) {
      
      FireAccessibleFocusEvent(accessible, targetNode, aEvent, PR_TRUE,
                               eFromUserInput);
    }
  }
  else if (eventType.EqualsLiteral("DOMMenuBarActive")) {  
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENU_START,
                            accessible, eFromUserInput);
  }
  else if (eventType.EqualsLiteral("DOMMenuBarInactive")) {  
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENU_END,
                            accessible, eFromUserInput);
    FireCurrentFocusEvent();
  }
  else if (eventType.EqualsLiteral("ValueChange")) {
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE,
                               targetNode, AccEvent::eRemoveDupes);
  }
#ifdef DEBUG
  else if (eventType.EqualsLiteral("mouseover")) {
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_DRAGDROP_START,
                            accessible);
  }
#endif
  return NS_OK;
}





void
nsRootAccessible::Shutdown()
{
  
  if (!mWeakShell)
    return;  

  mCurrentARIAMenubar = nsnull;

  nsDocAccessibleWrap::Shutdown();
}


already_AddRefed<nsIDocShellTreeItem>
nsRootAccessible::GetContentDocShell(nsIDocShellTreeItem *aStart)
{
  if (!aStart) {
    return nsnull;
  }

  PRInt32 itemType;
  aStart->GetItemType(&itemType);
  if (itemType == nsIDocShellTreeItem::typeContent) {
    nsDocAccessible *accDoc = nsAccUtils::GetDocAccessibleFor(aStart);

    
    
    if (!accDoc)
      return nsnull;

    
    
    
    nsAccessible *parent = accDoc->GetParent();
    while (parent) {
      if (nsAccUtils::State(parent) & nsIAccessibleStates::STATE_INVISIBLE)
        return nsnull;

      if (parent == this)
        break; 

      parent = parent->GetParent();
    }

    NS_ADDREF(aStart);
    return aStart;
  }
  nsCOMPtr<nsIDocShellTreeNode> treeNode(do_QueryInterface(aStart));
  if (treeNode) {
    PRInt32 subDocuments;
    treeNode->GetChildCount(&subDocuments);
    for (PRInt32 count = 0; count < subDocuments; count ++) {
      nsCOMPtr<nsIDocShellTreeItem> treeItemChild, contentTreeItem;
      treeNode->GetChildAt(count, getter_AddRefs(treeItemChild));
      NS_ENSURE_TRUE(treeItemChild, nsnull);
      contentTreeItem = GetContentDocShell(treeItemChild);
      if (contentTreeItem) {
        NS_ADDREF(aStart = contentTreeItem);
        return aStart;
      }
    }
  }
  return nsnull;
}


NS_IMETHODIMP
nsRootAccessible::GetRelationByType(PRUint32 aRelationType,
                                    nsIAccessibleRelation **aRelation)
{
  NS_ENSURE_ARG_POINTER(aRelation);
  *aRelation = nsnull;

  if (!mDocument || aRelationType != nsIAccessibleRelation::RELATION_EMBEDS) {
    return nsDocAccessibleWrap::GetRelationByType(aRelationType, aRelation);
  }

  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDocument);
  nsCOMPtr<nsIDocShellTreeItem> contentTreeItem = GetContentDocShell(treeItem);
  
  if (contentTreeItem) {
    nsDocAccessible *accDoc = nsAccUtils::GetDocAccessibleFor(contentTreeItem);
    return nsRelUtils::AddTarget(aRelationType, aRelation, accDoc);
  }

  return NS_OK;
}




nsresult
nsRootAccessible::HandlePopupShownEvent(nsAccessible *aAccessible)
{
  PRUint32 role = aAccessible->Role();

  if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
    
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START,
                            aAccessible);
    return NS_OK;
  }

  if (role == nsIAccessibleRole::ROLE_TOOLTIP) {
    
    
    
    
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SHOW, aAccessible);
    return NS_OK;
  }

  if (role == nsIAccessibleRole::ROLE_COMBOBOX_LIST) {
    
    nsAccessible* combobox = aAccessible->GetParent();
    NS_ENSURE_STATE(combobox);

    PRUint32 comboboxRole = combobox->Role();
    if (comboboxRole == nsIAccessibleRole::ROLE_COMBOBOX ||
        comboboxRole == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
      nsRefPtr<AccEvent> event =
        new AccStateChangeEvent(combobox,
                                nsIAccessibleStates::STATE_EXPANDED,
                                PR_FALSE, PR_TRUE);
      NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

      nsEventShell::FireEvent(event);
      return NS_OK;
    }
  }

  return NS_OK;
}

nsresult
nsRootAccessible::HandlePopupHidingEvent(nsINode *aNode,
                                         nsAccessible *aAccessible)
{
  
  
  
  
  

  if (gLastFocusedNode &&
      nsCoreUtils::IsAncestorOf(aNode, gLastFocusedNode)) {
    
    FireCurrentFocusEvent();
  }

  
  if (!aAccessible)
    return NS_OK;

  if (aAccessible->Role() != nsIAccessibleRole::ROLE_COMBOBOX_LIST)
    return NS_OK;

  nsAccessible* combobox = aAccessible->GetParent();
  NS_ENSURE_STATE(combobox);

  PRUint32 comboboxRole = combobox->Role();
  if (comboboxRole == nsIAccessibleRole::ROLE_COMBOBOX ||
      comboboxRole == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(combobox,
                              nsIAccessibleStates::STATE_EXPANDED,
                              PR_FALSE, PR_FALSE);
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

    nsEventShell::FireEvent(event);
    return NS_OK;
  }

  return NS_OK;
}

#ifdef MOZ_XUL
nsresult
nsRootAccessible::HandleTreeRowCountChangedEvent(nsIDOMEvent *aEvent,
                                                 nsXULTreeAccessible *aAccessible)
{
  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent(do_QueryInterface(aEvent));
  if (!dataEvent)
    return NS_OK;

  nsCOMPtr<nsIVariant> indexVariant;
  dataEvent->GetData(NS_LITERAL_STRING("index"),
                     getter_AddRefs(indexVariant));
  if (!indexVariant)
    return NS_OK;

  nsCOMPtr<nsIVariant> countVariant;
  dataEvent->GetData(NS_LITERAL_STRING("count"),
                     getter_AddRefs(countVariant));
  if (!countVariant)
    return NS_OK;

  PRInt32 index, count;
  indexVariant->GetAsInt32(&index);
  countVariant->GetAsInt32(&count);

  aAccessible->InvalidateCache(index, count);
  return NS_OK;
}

nsresult
nsRootAccessible::HandleTreeInvalidatedEvent(nsIDOMEvent *aEvent,
                                             nsXULTreeAccessible *aAccessible)
{
  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent(do_QueryInterface(aEvent));
  if (!dataEvent)
    return NS_OK;

  PRInt32 startRow = 0, endRow = -1, startCol = 0, endCol = -1;

  nsCOMPtr<nsIVariant> startRowVariant;
  dataEvent->GetData(NS_LITERAL_STRING("startrow"),
                     getter_AddRefs(startRowVariant));
  if (startRowVariant)
    startRowVariant->GetAsInt32(&startRow);

  nsCOMPtr<nsIVariant> endRowVariant;
  dataEvent->GetData(NS_LITERAL_STRING("endrow"),
                     getter_AddRefs(endRowVariant));
  if (endRowVariant)
    endRowVariant->GetAsInt32(&endRow);

  nsCOMPtr<nsIVariant> startColVariant;
  dataEvent->GetData(NS_LITERAL_STRING("startcolumn"),
                     getter_AddRefs(startColVariant));
  if (startColVariant)
    startColVariant->GetAsInt32(&startCol);

  nsCOMPtr<nsIVariant> endColVariant;
  dataEvent->GetData(NS_LITERAL_STRING("endcolumn"),
                     getter_AddRefs(endColVariant));
  if (endColVariant)
    endColVariant->GetAsInt32(&endCol);

  aAccessible->TreeViewInvalidated(startRow, endRow, startCol, endCol);
  return NS_OK;
}
#endif

