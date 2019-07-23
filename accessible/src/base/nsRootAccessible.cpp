





































#include "nsAccessibilityService.h"
#include "nsAccessibleEventData.h"
#include "nsApplicationAccessibleWrap.h"

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
#include "nsIScrollableView.h"
#include "nsISelectionPrivate.h"
#include "nsIServiceManager.h"
#include "nsIViewManager.h"
#include "nsPIDOMWindow.h"
#include "nsIWebBrowserChrome.h"
#include "nsReadableUtils.h"
#include "nsRootAccessible.h"
#include "nsIDOMNSEventTarget.h"
#include "nsIDOMDocumentEvent.h"
#include "nsFocusManager.h"

#ifdef MOZ_XUL
#include "nsXULTreeAccessible.h"
#include "nsIXULDocument.h"
#include "nsIXULWindow.h"
#endif



NS_IMPL_QUERY_HEAD(nsRootAccessible)
NS_IMPL_QUERY_BODY(nsIDOMEventListener)
if (aIID.Equals(NS_GET_IID(nsRootAccessible)))
  foundInterface = reinterpret_cast<nsISupports*>(this);
else
NS_IMPL_QUERY_TAIL_INHERITING(nsDocAccessible)

NS_IMPL_ADDREF_INHERITED(nsRootAccessible, nsDocAccessible) 
NS_IMPL_RELEASE_INHERITED(nsRootAccessible, nsDocAccessible)




nsRootAccessible::nsRootAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell):
  nsDocAccessibleWrap(aDOMNode, aShell)
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


NS_IMETHODIMP nsRootAccessible::GetParent(nsIAccessible * *aParent) 
{
  NS_ENSURE_ARG_POINTER(aParent);
  *aParent = nsnull;

  if (!mParent) {
    nsRefPtr<nsApplicationAccessibleWrap> root = GetApplicationAccessible();
    mParent = root;
  }

  NS_IF_ADDREF(*aParent = mParent);
  return NS_OK;
}


nsresult
nsRootAccessible::GetRoleInternal(PRUint32 *aRole) 
{ 
  if (!mDocument) {
    return NS_ERROR_FAILURE;
  }

  
  nsIContent *rootContent = mDocument->GetRootContent();
  if (rootContent) {
    nsCOMPtr<nsIDOMElement> rootElement(do_QueryInterface(rootContent));
    if (rootElement) {
      nsAutoString name;
      rootElement->GetLocalName(name);
      if (name.EqualsLiteral("dialog") || name.EqualsLiteral("wizard")) {
        *aRole = nsIAccessibleRole::ROLE_DIALOG; 
        return NS_OK;
      }
    }
  }

  return nsDocAccessibleWrap::GetRoleInternal(aRole);
}

#ifdef MOZ_XUL
PRUint32 nsRootAccessible::GetChromeFlags()
{
  
  
  
  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDOMNode);
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

  nsCOMPtr<nsIDOMWindow> domWin;
  GetWindow(getter_AddRefs(domWin));
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_GetInterface(domWin);
  if (dsti) {
    nsCOMPtr<nsIDocShellTreeItem> root;
    dsti->GetRootTreeItem(getter_AddRefs(root));
    nsCOMPtr<nsIDOMWindow> rootWindow = do_GetInterface(root);

    nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
    if (fm && rootWindow) {
      nsCOMPtr<nsIDOMWindow> activeWindow;
      fm->GetActiveWindow(getter_AddRefs(activeWindow));
      if (activeWindow == rootWindow)
        *aExtraState |= nsIAccessibleStates::EXT_STATE_ACTIVE;
    }
  }
#ifdef MOZ_XUL
  if (GetChromeFlags() & nsIWebBrowserChrome::CHROME_MODAL) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_MODAL;
  }
#endif

  return NS_OK;
}

void
nsRootAccessible::GetChromeEventHandler(nsIDOMEventTarget **aChromeTarget)
{
  nsCOMPtr<nsIDOMWindow> domWin;
  GetWindow(getter_AddRefs(domWin));
  nsCOMPtr<nsPIDOMWindow> privateDOMWindow(do_QueryInterface(domWin));
  nsCOMPtr<nsPIDOMEventTarget> chromeEventHandler;
  if (privateDOMWindow) {
    chromeEventHandler = privateDOMWindow->GetChromeEventHandler();
  }

  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(chromeEventHandler));

  *aChromeTarget = target;
  NS_IF_ADDREF(*aChromeTarget);
}

const char* const docEvents[] = {
#ifdef DEBUG
  
  
  "mouseover",
#endif
  
  "focus",
  
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
  "DOMMenuBarInactive",
  "DOMContentLoaded"
};

nsresult nsRootAccessible::AddEventListeners()
{
  
  
  
  
  nsCOMPtr<nsIDOMNSEventTarget> nstarget(do_QueryInterface(mDocument));

  if (nstarget) {
    for (const char* const* e = docEvents,
                   * const* e_end = docEvents + NS_ARRAY_LENGTH(docEvents);
         e < e_end; ++e) {
      nsresult rv = nstarget->AddEventListener(NS_ConvertASCIItoUTF16(*e),
                                               this, PR_TRUE, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsCOMPtr<nsIDOMEventTarget> target;
  GetChromeEventHandler(getter_AddRefs(target));
  if (target) {
    target->AddEventListener(NS_LITERAL_STRING("pagehide"), this, PR_TRUE);
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

  GetChromeEventHandler(getter_AddRefs(target));
  if (target) {
    target->RemoveEventListener(NS_LITERAL_STRING("pagehide"), this, PR_TRUE);
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

void nsRootAccessible::TryFireEarlyLoadEvent(nsIDOMNode *aDocNode)
{
  
  
  

  
  
  

  

  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsCoreUtils::GetDocShellTreeItemFor(aDocNode);
  NS_ASSERTION(treeItem, "No docshelltreeitem for aDocNode");
  if (!treeItem) {
    return;
  }
  PRInt32 itemType;
  treeItem->GetItemType(&itemType);
  if (itemType != nsIDocShellTreeItem::typeContent) {
    return;
  }

  
  
  nsCOMPtr<nsIDocShellTreeNode> treeNode(do_QueryInterface(treeItem));
  if (treeNode) {
    PRInt32 subDocuments;
    treeNode->GetChildCount(&subDocuments);
    if (subDocuments) {
      return;
    }
  }
  nsCOMPtr<nsIDocShellTreeItem> rootContentTreeItem;
  treeItem->GetSameTypeRootTreeItem(getter_AddRefs(rootContentTreeItem));
  NS_ASSERTION(rootContentTreeItem, "No root content tree item");
  if (rootContentTreeItem == treeItem) {
    
    FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_INTERNAL_LOAD, aDocNode);
  }
}

PRBool nsRootAccessible::FireAccessibleFocusEvent(nsIAccessible *aAccessible,
                                                  nsIDOMNode *aNode,
                                                  nsIDOMEvent *aFocusEvent,
                                                  PRBool aForceEvent,
                                                  PRBool aIsAsynch)
{
  if (mCaretAccessible) {
    nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aFocusEvent));
    if (nsevent) {
      
      
      
      
      
      
      
      
      nsCOMPtr<nsIDOMEventTarget> domEventTarget;
      nsevent->GetOriginalTarget(getter_AddRefs(domEventTarget));
      nsCOMPtr<nsIDOMNode> realFocusedNode(do_QueryInterface(domEventTarget));
      if (!realFocusedNode) {
        
        
        
        realFocusedNode = aNode;
      }
      if (realFocusedNode) {
        mCaretAccessible->SetControlSelectionListener(realFocusedNode);
      }
    }
  }

  
  nsCOMPtr<nsIDOMNode> finalFocusNode = aNode;
  nsCOMPtr<nsIAccessible> finalFocusAccessible = aAccessible;
  nsCOMPtr<nsIContent> finalFocusContent =
    nsCoreUtils::GetRoleContent(finalFocusNode);

  if (finalFocusContent) {
    nsAutoString id;
    if (finalFocusContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_activedescendant, id)) {
      nsCOMPtr<nsIDOMDocument> domDoc;
      aNode->GetOwnerDocument(getter_AddRefs(domDoc));
      if (!domDoc) {  
        domDoc = do_QueryInterface(aNode);
      }
      if (!domDoc) {
        return PR_FALSE;
      }
      nsCOMPtr<nsIDOMElement> relatedEl;
      domDoc->GetElementById(id, getter_AddRefs(relatedEl));
      finalFocusNode = do_QueryInterface(relatedEl);
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
    GetAccService()->GetAccessibleFor(finalFocusNode, getter_AddRefs(finalFocusAccessible));      
    
    
    
    if (!finalFocusAccessible) {
      return PR_FALSE;
    }
  }

  gLastFocusedAccessiblesState = nsAccUtils::State(finalFocusAccessible);
  PRUint32 role = nsAccUtils::Role(finalFocusAccessible);
  if (role == nsIAccessibleRole::ROLE_MENUITEM) {
    if (!mCurrentARIAMenubar) {  
      
      PRUint32 naturalRole = nsAccUtils::RoleInternal(finalFocusAccessible);
      if (role != naturalRole) { 
        nsCOMPtr<nsIAccessible> menuBarAccessible =
          nsAccUtils::GetAncestorWithRole(finalFocusAccessible,
                                          nsIAccessibleRole::ROLE_MENUBAR);
        nsCOMPtr<nsIAccessNode> menuBarAccessNode = do_QueryInterface(menuBarAccessible);
        if (menuBarAccessNode) {
          menuBarAccessNode->GetDOMNode(getter_AddRefs(mCurrentARIAMenubar));
          if (mCurrentARIAMenubar) {
            nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_MENU_START,
                                     menuBarAccessible);
          }
        }
      }
    }
  }
  else if (mCurrentARIAMenubar) {
    nsCOMPtr<nsIAccessibleEvent> menuEndEvent =
      new nsAccEvent(nsIAccessibleEvent::EVENT_MENU_END, mCurrentARIAMenubar,
                     PR_FALSE, nsAccEvent::eAllowDupes);
    if (menuEndEvent) {
      FireDelayedAccessibleEvent(menuEndEvent);
    }
    mCurrentARIAMenubar = nsnull;
  }

  nsCOMPtr<nsIContent> focusContent = do_QueryInterface(finalFocusNode);
  nsIFrame *focusFrame = nsnull;
  if (focusContent) {
    nsCOMPtr<nsIPresShell> shell =
      nsCoreUtils::GetPresShellFor(finalFocusNode);

    NS_ASSERTION(shell, "No pres shell for final focus node!");
    if (!shell)
      return PR_FALSE;

    focusFrame = shell->GetRealPrimaryFrameFor(focusContent);
  }

  NS_IF_RELEASE(gLastFocusedNode);
  gLastFocusedNode = finalFocusNode;
  NS_IF_ADDREF(gLastFocusedNode);

  gLastFocusedFrameType = (focusFrame && focusFrame->GetStyleVisibility()->IsVisible()) ? focusFrame->GetType() : 0;

  nsCOMPtr<nsIAccessibleDocument> docAccessible = do_QueryInterface(finalFocusAccessible);
  if (docAccessible) {
    
    nsCOMPtr<nsIDOMNode> realFocusedNode = GetCurrentFocus();
    if (realFocusedNode != aNode || realFocusedNode == mDOMNode) {
      
      
      
      return PR_FALSE;
    }
  }

  FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_FOCUS,
                          finalFocusNode, nsAccEvent::eRemoveDupes,
                          aIsAsynch);

  return PR_TRUE;
}

void nsRootAccessible::FireCurrentFocusEvent()
{
  nsCOMPtr<nsIDOMNode> focusedNode = GetCurrentFocus();
  if (!focusedNode) {
    return; 
  }

  
  nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(mDocument);
  if (docEvent) {
    nsCOMPtr<nsIDOMEvent> event;
    if (NS_SUCCEEDED(docEvent->CreateEvent(NS_LITERAL_STRING("Events"),
                                           getter_AddRefs(event))) &&
        NS_SUCCEEDED(event->InitEvent(NS_LITERAL_STRING("focus"), PR_TRUE, PR_TRUE))) {
      
      nsIAccessibilityService* accService = GetAccService();
      if (accService) {
        nsCOMPtr<nsIDOMNode> targetNode;
        accService->GetRelevantContentNodeFor(focusedNode,
                                            getter_AddRefs(targetNode));
        if (targetNode) {
          HandleEventWithTarget(event, targetNode);
        }
      }
    }
  }
}



NS_IMETHODIMP nsRootAccessible::HandleEvent(nsIDOMEvent* aEvent)
{
  
  
  nsCOMPtr<nsIDOMNode> targetNode;
  GetTargetNode(aEvent, getter_AddRefs(targetNode));
  if (!targetNode)
    return NS_ERROR_FAILURE;
  
  return HandleEventWithTarget(aEvent, targetNode);
}

nsresult nsRootAccessible::HandleEventWithTarget(nsIDOMEvent* aEvent,
                                                 nsIDOMNode* aTargetNode)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);
  nsAutoString localName;
  aTargetNode->GetLocalName(localName);
#ifdef MOZ_XUL
  PRBool isTree = localName.EqualsLiteral("tree");
#endif
#ifdef DEBUG_A11Y
  
  if (eventType.EqualsLiteral("AlertActive")) {
    printf("\ndebugging %s events for %s", NS_ConvertUTF16toUTF8(eventType).get(), NS_ConvertUTF16toUTF8(localName).get());
  }
  if (localName.LowerCaseEqualsLiteral("textbox")) {
    printf("\ndebugging %s events for %s", NS_ConvertUTF16toUTF8(eventType).get(), NS_ConvertUTF16toUTF8(localName).get());
  }
#endif

  nsIAccessibilityService *accService = GetAccService();
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);

  if (eventType.EqualsLiteral("pagehide")) {
    
    
    
    
    
    
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(aTargetNode));
    nsCOMPtr<nsIAccessibleDocument> accDoc = GetDocAccessibleFor(doc);
    if (accDoc) {
      nsRefPtr<nsAccessNode> docAccNode = nsAccUtils::QueryAccessNode(accDoc);
      docAccNode->Shutdown();
    }

    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> eventShell = nsCoreUtils::GetPresShellFor(aTargetNode);
  if (!eventShell) {
    return NS_OK;
  }

  if (eventType.EqualsLiteral("DOMContentLoaded")) {
    
    
    
    TryFireEarlyLoadEvent(aTargetNode);
    return NS_OK;
  }

  nsCOMPtr<nsIAccessible> accessible;
  accService->GetAccessibleInShell(aTargetNode, eventShell,
                                   getter_AddRefs(accessible));

  if (eventType.EqualsLiteral("popuphiding"))
    return HandlePopupHidingEvent(aTargetNode, accessible);

  nsRefPtr<nsAccessible> acc(nsAccUtils::QueryAccessible(accessible));
  if (!acc)
    return NS_OK;

#ifdef MOZ_XUL
  if (isTree) {
    nsRefPtr<nsXULTreeAccessible> treeAcc =
      nsAccUtils::QueryAccessibleTree(accessible);
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

    nsCOMPtr<nsIAccessibleStateChangeEvent> accEvent =
      new nsAccStateChangeEvent(accessible, nsIAccessibleStates::STATE_CHECKED,
                                PR_FALSE, isEnabled);
    acc->FireAccessibleEvent(accEvent);

    if (isEnabled)
      FireAccessibleFocusEvent(accessible, aTargetNode, aEvent);

    return NS_OK;
  }

  if (eventType.EqualsLiteral("CheckboxStateChange")) {
    PRUint32 state = nsAccUtils::State(accessible);

    PRBool isEnabled = !!(state & nsIAccessibleStates::STATE_CHECKED);

    nsCOMPtr<nsIAccessibleStateChangeEvent> accEvent =
      new nsAccStateChangeEvent(accessible,
                                nsIAccessibleStates::STATE_CHECKED,
                                PR_FALSE, isEnabled);

    return acc->FireAccessibleEvent(accEvent);
  }

  nsCOMPtr<nsIAccessible> treeItemAccessible;
#ifdef MOZ_XUL
  
  if (isTree) {
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
      do_QueryInterface(aTargetNode);
    if (multiSelect) {
      PRInt32 treeIndex = -1;
      multiSelect->GetCurrentIndex(&treeIndex);
      if (treeIndex >= 0) {
        nsRefPtr<nsXULTreeAccessible> treeAcc =
          nsAccUtils::QueryAccessibleTree(accessible);
        if (treeAcc) {
          treeAcc->GetTreeItemAccessible(treeIndex,
                                         getter_AddRefs(treeItemAccessible));
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

    nsCOMPtr<nsIAccessibleStateChangeEvent> accEvent =
      new nsAccStateChangeEvent(accessible, nsIAccessibleStates::STATE_EXPANDED,
                                PR_FALSE, isEnabled);
    return FireAccessibleEvent(accEvent);
  }

  if (treeItemAccessible && eventType.EqualsLiteral("select")) {
    
    if (gLastFocusedNode == aTargetNode) {
      nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSel =
        do_QueryInterface(aTargetNode);
      nsAutoString selType;
      multiSel->GetSelType(selType);
      if (selType.IsEmpty() || !selType.EqualsLiteral("single")) {
        
        
        
        
        return nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_SELECTION_WITHIN,
                                        accessible);
      }

      return nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_SELECTION,
                                      treeItemAccessible);
    }
  }
  else
#endif
  if (eventType.EqualsLiteral("focus")) {
    if (aTargetNode == mDOMNode && mDOMNode != gLastFocusedNode) {
      
      
      
      if (!mFireFocusTimer) {
        mFireFocusTimer = do_CreateInstance("@mozilla.org/timer;1");
      }
      if (mFireFocusTimer) {
        mFireFocusTimer->InitWithFuncCallback(FireFocusCallback, this,
                                              0, nsITimer::TYPE_ONE_SHOT);
      }
    }

    
    
    
    nsCOMPtr<nsIDOMNode> focusedItem(aTargetNode);

    if (!treeItemAccessible) {
      nsCOMPtr<nsIDOMXULSelectControlElement> selectControl =
        do_QueryInterface(aTargetNode);
      if (selectControl) {
        nsCOMPtr<nsIDOMXULMenuListElement> menuList =
          do_QueryInterface(aTargetNode);
        if (!menuList) {
          
          
          nsCOMPtr<nsIDOMXULSelectControlItemElement> selectedItem;
          selectControl->GetSelectedItem(getter_AddRefs(selectedItem));
          if (selectedItem)
            focusedItem = do_QueryInterface(selectedItem);

          if (!focusedItem)
            return NS_OK;

          accService->GetAccessibleInShell(focusedItem, eventShell,
                                           getter_AddRefs(accessible));
          if (!accessible)
            return NS_OK;
        }
      }
    }
    FireAccessibleFocusEvent(accessible, focusedItem, aEvent);
  }
  else if (eventType.EqualsLiteral("AlertActive")) { 
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_ALERT, accessible);
  }
  else if (eventType.EqualsLiteral("popupshown")) {
    HandlePopupShownEvent(accessible);
  }
  else if (eventType.EqualsLiteral("DOMMenuInactive")) {
    if (nsAccUtils::Role(accessible) == nsIAccessibleRole::ROLE_MENUPOPUP) {
      nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_END,
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
      nsRefPtr<nsAccessNode> menuAccessNode =
        nsAccUtils::QueryAccessNode(accessible);
  
      nsIFrame* menuFrame = menuAccessNode->GetFrame();
      NS_ENSURE_TRUE(menuFrame, NS_ERROR_FAILURE);

      nsIMenuFrame* imenuFrame = do_QueryFrame(menuFrame);
      if (imenuFrame)
        fireFocus = PR_TRUE;
      
      if (imenuFrame && imenuFrame->IsOnMenuBar() &&
                       !imenuFrame->IsOnActiveMenuBar()) {
        
        
        return NS_OK;
      } else {
        nsCOMPtr<nsIAccessible> containerAccessible;
        accessible->GetParent(getter_AddRefs(containerAccessible));
        NS_ENSURE_TRUE(containerAccessible, NS_ERROR_FAILURE);
        
        
        
        if (nsAccUtils::State(containerAccessible) & nsIAccessibleStates::STATE_COLLAPSED) {
          nsCOMPtr<nsIAccessible> containerParent;
          containerAccessible->GetParent(getter_AddRefs(containerParent));
          NS_ENSURE_TRUE(containerParent, NS_ERROR_FAILURE);
          if (nsAccUtils::Role(containerParent) != nsIAccessibleRole::ROLE_COMBOBOX) {
            return NS_OK;
          }
        }
      }
    }
    if (!fireFocus) {
      nsCOMPtr<nsIDOMNode> realFocusedNode = GetCurrentFocus();
      nsCOMPtr<nsIContent> realFocusedContent = do_QueryInterface(realFocusedNode);
      nsCOMPtr<nsIContent> targetContent = do_QueryInterface(aTargetNode);
      nsIContent *containerContent = targetContent;
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
      nsAccEvent::PrepareForEvent(aTargetNode, PR_TRUE);  
      FireAccessibleFocusEvent(accessible, aTargetNode, aEvent, PR_TRUE, PR_TRUE);
    }
  }
  else if (eventType.EqualsLiteral("DOMMenuBarActive")) {  
    nsAccEvent::PrepareForEvent(aTargetNode, PR_TRUE);
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_MENU_START,
                             accessible, PR_TRUE);
  }
  else if (eventType.EqualsLiteral("DOMMenuBarInactive")) {  
    nsAccEvent::PrepareForEvent(aTargetNode, PR_TRUE);
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_MENU_END,
                             accessible, PR_TRUE);
    FireCurrentFocusEvent();
  }
  else if (eventType.EqualsLiteral("ValueChange")) {
    FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, aTargetNode, nsAccEvent::eRemoveDupes);
  }
#ifdef DEBUG
  else if (eventType.EqualsLiteral("mouseover")) {
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_DRAGDROP_START,
                             accessible);
  }
#endif
  return NS_OK;
}

void nsRootAccessible::GetTargetNode(nsIDOMEvent *aEvent, nsIDOMNode **aTargetNode)
{
  *aTargetNode = nsnull;

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aEvent));

  if (!nsevent)
    return;

  nsCOMPtr<nsIDOMEventTarget> domEventTarget;
  nsevent->GetOriginalTarget(getter_AddRefs(domEventTarget));
  nsCOMPtr<nsIDOMNode> eventTarget(do_QueryInterface(domEventTarget));
  if (!eventTarget)
    return;

  nsIAccessibilityService* accService = GetAccService();
  if (accService) {
    nsresult rv = accService->GetRelevantContentNodeFor(eventTarget,
                                                        aTargetNode);
    if (NS_SUCCEEDED(rv) && *aTargetNode)
      return;
  }

  NS_ADDREF(*aTargetNode = eventTarget);
}

void nsRootAccessible::FireFocusCallback(nsITimer *aTimer, void *aClosure)
{
  nsRootAccessible *rootAccessible = static_cast<nsRootAccessible*>(aClosure);
  NS_ASSERTION(rootAccessible, "How did we get here without a root accessible?");
  rootAccessible->FireCurrentFocusEvent();
}

nsresult
nsRootAccessible::Init()
{
  nsRefPtr<nsApplicationAccessibleWrap> root = GetApplicationAccessible();
  NS_ENSURE_STATE(root);

  root->AddRootAccessible(this);

  return nsDocAccessibleWrap::Init();
}

nsresult
nsRootAccessible::Shutdown()
{
  
  if (!mWeakShell) {
    return NS_OK;  
  }

  nsRefPtr<nsApplicationAccessibleWrap> root = GetApplicationAccessible();
  NS_ENSURE_STATE(root);

  root->RemoveRootAccessible(this);

  mCurrentARIAMenubar = nsnull;

  if (mFireFocusTimer) {
    mFireFocusTimer->Cancel();
    mFireFocusTimer = nsnull;
  }

  return nsDocAccessibleWrap::Shutdown();
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
    nsCOMPtr<nsIAccessibleDocument> accDoc =
      GetDocAccessibleFor(aStart, PR_TRUE);

    
    
    if (!accDoc)
      return nsnull;

    nsCOMPtr<nsIAccessible> accessible = do_QueryInterface(accDoc);

    
    
    
    while (accessible) {
      if (nsAccUtils::State(accessible) & nsIAccessibleStates::STATE_INVISIBLE)
        return nsnull;

      nsCOMPtr<nsIAccessible> ancestor;
      accessible->GetParent(getter_AddRefs(ancestor));
      if (ancestor == this) {
        break; 
      }
      accessible.swap(ancestor);
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

  if (!mDOMNode || aRelationType != nsIAccessibleRelation::RELATION_EMBEDS) {
    return nsDocAccessibleWrap::GetRelationByType(aRelationType, aRelation);
  }

  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDOMNode);
  nsCOMPtr<nsIDocShellTreeItem> contentTreeItem = GetContentDocShell(treeItem);
  
  if (contentTreeItem) {
    nsCOMPtr<nsIAccessibleDocument> accDoc =
      GetDocAccessibleFor(contentTreeItem, PR_TRUE);

    nsCOMPtr<nsIAccessible> acc(do_QueryInterface(accDoc));
    return nsRelUtils::AddTarget(aRelationType, aRelation, acc);
  }

  return NS_OK;
}

void
nsRootAccessible::FireDocLoadEvents(PRUint32 aEventType)
{
  if (IsDefunct())
    return;

  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mDOMNode);
  NS_ASSERTION(docShellTreeItem, "No doc shell tree item for document");
  if (!docShellTreeItem)
    return;

  PRInt32 contentType;
  docShellTreeItem->GetItemType(&contentType);
  if (contentType == nsIDocShellTreeItem::typeContent)
    nsDocAccessibleWrap::FireDocLoadEvents(aEventType); 

  
  mIsContentLoaded = (aEventType == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE ||
                      aEventType == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_STOPPED);
}

nsresult
nsRootAccessible::HandlePopupShownEvent(nsIAccessible *aAccessible)
{
  PRUint32 role = nsAccUtils::Role(aAccessible);

  if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
    
    return nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START,
                                    aAccessible);
  }

  if (role == nsIAccessibleRole::ROLE_TOOLTIP) {
    
    
    
    
    return nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_ASYNCH_SHOW,
                                    aAccessible);
  }

  if (role == nsIAccessibleRole::ROLE_COMBOBOX_LIST) {
    
    nsCOMPtr<nsIAccessible> comboboxAcc;
    nsresult rv = aAccessible->GetParent(getter_AddRefs(comboboxAcc));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 comboboxRole = nsAccUtils::Role(comboboxAcc);
    if (comboboxRole == nsIAccessibleRole::ROLE_COMBOBOX ||
        comboboxRole == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
      nsCOMPtr<nsIAccessibleStateChangeEvent> event =
        new nsAccStateChangeEvent(comboboxAcc,
                                  nsIAccessibleStates::STATE_EXPANDED,
                                  PR_FALSE, PR_TRUE);
      NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

      nsRefPtr<nsAccessible> acc(nsAccUtils::QueryAccessible(comboboxAcc));
      return acc->FireAccessibleEvent(event);
    }
  }

  return NS_OK;
}

nsresult
nsRootAccessible::HandlePopupHidingEvent(nsIDOMNode *aNode,
                                         nsIAccessible *aAccessible)
{
  
  
  
  
  
  if (gLastFocusedNode &&
      nsCoreUtils::IsAncestorOf(aNode, gLastFocusedNode)) {
    
    FireCurrentFocusEvent();
  }

  
  if (!aAccessible)
    return NS_OK;

  PRUint32 role = nsAccUtils::Role(aAccessible);
  if (role != nsIAccessibleRole::ROLE_COMBOBOX_LIST)
    return NS_OK;

  nsCOMPtr<nsIAccessible> comboboxAcc;
  nsresult rv = aAccessible->GetParent(getter_AddRefs(comboboxAcc));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 comboboxRole = nsAccUtils::Role(comboboxAcc);
  if (comboboxRole == nsIAccessibleRole::ROLE_COMBOBOX ||
      comboboxRole == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
    nsCOMPtr<nsIAccessibleStateChangeEvent> event =
      new nsAccStateChangeEvent(comboboxAcc,
                                nsIAccessibleStates::STATE_EXPANDED,
                                PR_FALSE, PR_FALSE);
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

    nsRefPtr<nsAccessible> acc(nsAccUtils::QueryAccessible(comboboxAcc));
    return acc->FireAccessibleEvent(event);
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

