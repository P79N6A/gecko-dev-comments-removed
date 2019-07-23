





































#include "nsAccessibilityService.h"
#include "nsAccessibleEventData.h"
#include "nsHTMLSelectAccessible.h"
#include "nsIBaseWindow.h"
#include "nsICaret.h"
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
#include "nsIDOMNSEvent.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULPopupElement.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIFocusController.h"
#include "nsIFrame.h"
#include "nsIMenuFrame.h"
#include "nsIHTMLDocument.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIMenuParent.h"
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

#ifdef MOZ_XUL
#include "nsXULTreeAccessible.h"
#include "nsIXULDocument.h"
#include "nsIXULWindow.h"
#endif

#ifdef MOZ_ACCESSIBILITY_ATK
#include "nsAppRootAccessible.h"
#else
#include "nsApplicationAccessibleWrap.h"
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



NS_IMETHODIMP nsRootAccessible::GetName(nsAString& aName)
{
  if (!mDocument) {
    return NS_ERROR_FAILURE;
  }

  if (mRoleMapEntry) {
    nsAccessible::GetName(aName);
    if (!aName.IsEmpty()) {
      return NS_OK;
    }
  }

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
    nsAccUtils::GetDocShellTreeItemFor(mDOMNode);
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));

  nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(treeOwner));
  if (baseWindow) {
    nsXPIDLString title;
    baseWindow->GetTitle(getter_Copies(title));
    aName.Assign(title);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsRootAccessible::GetParent(nsIAccessible * *aParent) 
{
  NS_ENSURE_ARG_POINTER(aParent);
  *aParent = nsnull;

  nsRefPtr<nsApplicationAccessibleWrap> root = GetApplicationAccessible();
  NS_IF_ADDREF(*aParent = root);

  return NS_OK;
}


NS_IMETHODIMP nsRootAccessible::GetRole(PRUint32 *aRole) 
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

  return nsDocAccessibleWrap::GetRole(aRole);
}

#ifdef MOZ_XUL
PRUint32 nsRootAccessible::GetChromeFlags()
{
  
  
  
  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsAccUtils::GetDocShellTreeItemFor(mDOMNode);
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

NS_IMETHODIMP
nsRootAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsDocAccessibleWrap::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

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
  nsCOMPtr<nsPIDOMWindow> privateDOMWindow(do_QueryInterface(domWin));
  if (privateDOMWindow) {
    nsIFocusController *focusController =
      privateDOMWindow->GetRootFocusController();
    PRBool isActive = PR_FALSE;
    focusController->GetActive(&isActive);
    if (isActive) {
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
  
  "focus",
  
  "select",
  
  "NameChange",
  
  "ValueChange",
  
  "AlertActive",
  
  "TreeViewChanged",
  
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

  
  
  mFireFocusTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mFireFocusTimer) {
    mFireFocusTimer->InitWithFuncCallback(FireFocusCallback, this,
                                          0, nsITimer::TYPE_ONE_SHOT);
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

  if (mCaretAccessible) {
    mCaretAccessible->Shutdown();
    mCaretAccessible = nsnull;
  }

  return nsDocAccessible::RemoveEventListeners();
}

nsCaretAccessible*
nsRootAccessible::GetCaretAccessible()
{
  return mCaretAccessible;
}

void nsRootAccessible::TryFireEarlyLoadEvent(nsIDOMNode *aDocNode)
{
  
  
  

  
  
  

  

  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsAccUtils::GetDocShellTreeItemFor(aDocNode);
  NS_ASSERTION(treeItem, "No docshelltreeitem for aDocNode");
  if (!treeItem) {
    return;
  }
  PRInt32 itemType;
  treeItem->GetItemType(&itemType);
  if (itemType != nsIDocShellTreeItem::typeContent) {
    return;
  }

  
  
  nsCOMPtr<nsIAccessible> docAccessible;
  GetAccService()->GetAccessibleFor(aDocNode, getter_AddRefs(docAccessible));

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
  if (!rootContentTreeItem) { 
    return;
  }
  if (rootContentTreeItem != treeItem) {
    nsCOMPtr<nsIAccessibleDocument> rootContentDocAccessible =
      GetDocAccessibleFor(rootContentTreeItem);
    nsCOMPtr<nsIAccessible> rootContentAccessible =
      do_QueryInterface(rootContentDocAccessible);
    if (!rootContentAccessible) {
      return;
    }
    PRUint32 state = State(rootContentAccessible);
    if (state & nsIAccessibleStates::STATE_BUSY) {
      
      return;
    }
  }

  
  FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_INTERNAL_LOAD, aDocNode,
                          nsnull, eRemoveDupes);
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
  nsCOMPtr<nsIContent> finalFocusContent  = do_QueryInterface(aNode);
  if (finalFocusContent) {
    nsAutoString id;
    if (nsAccUtils::GetAriaProperty(finalFocusContent, nsnull, eAria_activedescendant, id)) {
      nsCOMPtr<nsIDOMDocument> domDoc;
      aNode->GetOwnerDocument(getter_AddRefs(domDoc));
      if (!domDoc) {
        return PR_FALSE;
      }
      nsCOMPtr<nsIDOMElement> relatedEl;
      domDoc->GetElementById(id, getter_AddRefs(relatedEl));
      finalFocusNode = do_QueryInterface(relatedEl);
      if (!finalFocusNode) {
        return PR_FALSE;
      }
      GetAccService()->GetAccessibleFor(finalFocusNode, getter_AddRefs(finalFocusAccessible));      
      
      
      
      if (!finalFocusAccessible) {
        return PR_FALSE;
      }
    }
  }

  
  if (gLastFocusedNode == finalFocusNode && !aForceEvent) {
    return PR_FALSE;
  }

  gLastFocusedAccessiblesState = State(finalFocusAccessible);
  PRUint32 role = Role(finalFocusAccessible);
  if (role == nsIAccessibleRole::ROLE_MENUITEM) {
    if (!mCurrentARIAMenubar) {  
      PRUint32 naturalRole; 
      finalFocusAccessible->GetRole(&naturalRole);
      if (role != naturalRole) { 
        mCurrentARIAMenubar =
          nsAccUtils::GetAncestorWithRole(finalFocusAccessible, nsIAccessibleRole::ROLE_MENUBAR);
        if (mCurrentARIAMenubar) {
          nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_MENU_START, mCurrentARIAMenubar);
        }
      }
    }
  }
  else if (mCurrentARIAMenubar) {
    nsCOMPtr<nsIAccessibleEvent> menuEndEvent =
      new nsAccEvent(nsIAccessibleEvent::EVENT_MENU_END, mCurrentARIAMenubar, nsnull, PR_FALSE);
    if (menuEndEvent) {
      FireDelayedAccessibleEvent(menuEndEvent, eAllowDupes, PR_FALSE);
    }
    mCurrentARIAMenubar = nsnull;
  }

  NS_IF_RELEASE(gLastFocusedNode);
  gLastFocusedNode = finalFocusNode;
  NS_IF_ADDREF(gLastFocusedNode);

  nsCOMPtr<nsIAccessibleDocument> docAccessible = do_QueryInterface(finalFocusAccessible);
  if (docAccessible) {
    
    nsCOMPtr<nsIDOMNode> realFocusedNode = GetCurrentFocus();
    if (realFocusedNode != aNode || realFocusedNode == mDOMNode) {
      
      
      
      return PR_FALSE;
    }
  }

  FireDelayedToolkitEvent(nsIAccessibleEvent::EVENT_FOCUS,
                          finalFocusNode, nsnull, eRemoveDupes, aIsAsynch);

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
#ifdef DEBUG_A11Y
  
  if (eventType.EqualsLiteral("AlertActive")) {
    printf("\ndebugging %s events for %s", NS_ConvertUTF16toUTF8(eventType).get(), NS_ConvertUTF16toUTF8(localName).get());
  }
  if (localName.LowerCaseEqualsLiteral("textbox")) {
    printf("\ndebugging %s events for %s", NS_ConvertUTF16toUTF8(eventType).get(), NS_ConvertUTF16toUTF8(localName).get());
  }
#endif

  nsCOMPtr<nsIPresShell> eventShell = GetPresShellFor(aTargetNode);
  if (!eventShell) {
    return NS_OK;
  }
      
  nsIAccessibilityService *accService = GetAccService();
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);

  if (eventType.EqualsLiteral("pagehide")) {
    
    
    
    
    
    
    nsCOMPtr<nsIWeakReference> weakShell(do_GetWeakReference(eventShell));
    nsCOMPtr<nsIAccessible> accessible;
    accService->GetCachedAccessible(aTargetNode, weakShell, getter_AddRefs(accessible));
    nsCOMPtr<nsPIAccessibleDocument> privateAccDoc = do_QueryInterface(accessible);
    if (privateAccDoc) {
      privateAccDoc->Destroy();
    }
    return NS_OK;
  }

  if (eventType.EqualsLiteral("DOMContentLoaded")) {
    
    
    
    TryFireEarlyLoadEvent(aTargetNode);
    return NS_OK;
  }

  if (eventType.EqualsLiteral("TreeViewChanged")) { 
    NS_ENSURE_TRUE(localName.EqualsLiteral("tree"), NS_OK);
    nsCOMPtr<nsIContent> treeContent = do_QueryInterface(aTargetNode);
    nsAccEvent::PrepareForEvent(aTargetNode, PR_TRUE);
    return accService->InvalidateSubtreeFor(eventShell, treeContent,
                                            nsIAccessibleEvent::EVENT_ASYNCH_SIGNIFICANT_CHANGE);
  }

  nsCOMPtr<nsIAccessible> accessible;
  accService->GetAccessibleInShell(aTargetNode, eventShell,
                                   getter_AddRefs(accessible));
  nsCOMPtr<nsPIAccessible> privAcc(do_QueryInterface(accessible));
  if (!privAcc)
    return NS_OK;

  if (eventType.EqualsLiteral("RadioStateChange")) {
    PRUint32 state = State(accessible);

    
    
    
    
    PRBool isEnabled = (state & (nsIAccessibleStates::STATE_CHECKED |
                        nsIAccessibleStates::STATE_SELECTED)) != 0;

    nsCOMPtr<nsIAccessibleStateChangeEvent> accEvent =
      new nsAccStateChangeEvent(accessible, nsIAccessibleStates::STATE_CHECKED,
                                PR_FALSE, isEnabled);
    privAcc->FireAccessibleEvent(accEvent);

    if (isEnabled)
      FireAccessibleFocusEvent(accessible, aTargetNode, aEvent);

    return NS_OK;
  }

  if (eventType.EqualsLiteral("CheckboxStateChange")) {
    PRUint32 state = State(accessible);

    PRBool isEnabled = state & nsIAccessibleStates::STATE_CHECKED;

    nsCOMPtr<nsIAccessibleStateChangeEvent> accEvent =
      new nsAccStateChangeEvent(accessible,
                                nsIAccessibleStates::STATE_CHECKED,
                                PR_FALSE, isEnabled);

    return privAcc->FireAccessibleEvent(accEvent);
  }

  nsCOMPtr<nsIAccessible> treeItemAccessible;
#ifdef MOZ_XUL
  
  if (localName.EqualsLiteral("tree")) {
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
      do_QueryInterface(aTargetNode);
    if (multiSelect) {
      PRInt32 treeIndex = -1;
      multiSelect->GetCurrentIndex(&treeIndex);
      if (treeIndex >= 0) {
        nsCOMPtr<nsIAccessibleTreeCache> treeCache(do_QueryInterface(accessible));
        if (!treeCache ||
            NS_FAILED(treeCache->GetCachedTreeitemAccessible(
                      treeIndex,
                      nsnull,
                      getter_AddRefs(treeItemAccessible))) ||
                      !treeItemAccessible) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
        accessible = treeItemAccessible;
      }
    }
  }
#endif

#ifdef MOZ_XUL
  if (treeItemAccessible && eventType.EqualsLiteral("OpenStateChange")) {
    PRUint32 state = State(accessible); 
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
  else if (eventType.EqualsLiteral("NameChange")) {
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_NAME_CHANGE, accessible);
  }
  else if (eventType.EqualsLiteral("AlertActive")) { 
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_ALERT, accessible);
  }
  else if (eventType.EqualsLiteral("popupshown")) {
    
    PRUint32 role = Role(accessible);
    PRInt32 event = 0;
    if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
      event = nsIAccessibleEvent::EVENT_MENUPOPUP_START;
    }
    else if (role == nsIAccessibleRole::ROLE_TOOLTIP) {
      
      
      
      
      event = nsIAccessibleEvent::EVENT_ASYNCH_SHOW;
    }
    if (event) {
      nsAccUtils::FireAccEvent(event, accessible);
    }
  }

  else if (eventType.EqualsLiteral("popuphiding")) {
    
    
    
    
    
    
    if (!gLastFocusedNode) {
      return NS_OK;
    }
    if (gLastFocusedNode != aTargetNode) {
      
      nsCOMPtr<nsIDOMNode> parentOfFocus;
      gLastFocusedNode->GetParentNode(getter_AddRefs(parentOfFocus));
      if (parentOfFocus != aTargetNode) {
        return NS_OK;  
      }
    }
    
    FireCurrentFocusEvent();
  }
  else if (eventType.EqualsLiteral("DOMMenuInactive")) {
    if (Role(accessible) == nsIAccessibleRole::ROLE_MENUPOPUP) {
      nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_END,
                               accessible);
    }
  }
  else if (eventType.EqualsLiteral("DOMMenuItemActive")) {
    if (!treeItemAccessible) {
      nsCOMPtr<nsPIAccessNode> menuAccessNode = do_QueryInterface(accessible);
      NS_ENSURE_TRUE(menuAccessNode, NS_ERROR_FAILURE);
      nsIFrame* menuFrame = menuAccessNode->GetFrame();
      NS_ENSURE_TRUE(menuFrame, NS_ERROR_FAILURE);
      nsIMenuFrame* imenuFrame;
      CallQueryInterface(menuFrame, &imenuFrame);
      
      if (imenuFrame && imenuFrame->IsOnMenuBar() &&
                       !imenuFrame->IsOnActiveMenuBar()) {
        
        
        return NS_OK;
      } else {
        nsCOMPtr<nsIAccessible> containerAccessible;
        accessible->GetParent(getter_AddRefs(containerAccessible));
        NS_ENSURE_TRUE(containerAccessible, NS_ERROR_FAILURE);
        
        
        if (State(containerAccessible) & nsIAccessibleStates::STATE_COLLAPSED)
          return NS_OK;
      }
    }
    nsAccEvent::PrepareForEvent(aTargetNode, PR_TRUE);  
    FireAccessibleFocusEvent(accessible, aTargetNode, aEvent, PR_TRUE, PR_TRUE);
  }
  else if (eventType.EqualsLiteral("DOMMenuBarActive")) {  
    nsAccEvent::PrepareForEvent(aTargetNode, PR_TRUE);
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_MENU_START, accessible, PR_TRUE);
  }
  else if (eventType.EqualsLiteral("DOMMenuBarInactive")) {  
    nsAccEvent::PrepareForEvent(aTargetNode, PR_TRUE);
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_MENU_END, accessible, PR_TRUE);
    FireCurrentFocusEvent();
  }
  else if (eventType.EqualsLiteral("ValueChange")) {
    nsAccUtils::FireAccEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, accessible);
  }
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

NS_IMETHODIMP
nsRootAccessible::Init()
{
  nsresult rv = nsDocAccessibleWrap::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsApplicationAccessibleWrap> root = GetApplicationAccessible();
  NS_ENSURE_STATE(root);

  root->AddRootAccessible(this);
  return NS_OK;
}

NS_IMETHODIMP nsRootAccessible::Shutdown()
{
  nsRefPtr<nsApplicationAccessibleWrap> root = GetApplicationAccessible();
  NS_ENSURE_STATE(root);

  root->RemoveRootAccessible(this);

  
  if (!mWeakShell) {
    return NS_OK;  
  }
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
    nsCOMPtr<nsIAccessible> accessible = do_QueryInterface(accDoc);
    
    
    
    while (accessible) {
      if (State(accessible) & nsIAccessibleStates::STATE_INVISIBLE) {
        return nsnull;
      }
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

NS_IMETHODIMP nsRootAccessible::GetAccessibleRelated(PRUint32 aRelationType,
                                                     nsIAccessible **aRelated)
{
  *aRelated = nsnull;

  if (!mDOMNode || aRelationType != nsIAccessibleRelation::RELATION_EMBEDS) {
    return nsDocAccessibleWrap::GetAccessibleRelated(aRelationType, aRelated);
  }

  nsCOMPtr<nsIDocShellTreeItem> treeItem =
    nsAccUtils::GetDocShellTreeItemFor(mDOMNode);
  nsCOMPtr<nsIDocShellTreeItem> contentTreeItem = GetContentDocShell(treeItem);
  
  if (contentTreeItem) {
    nsCOMPtr<nsIAccessibleDocument> accDoc =
      GetDocAccessibleFor(contentTreeItem, PR_TRUE);
    NS_ASSERTION(accDoc, "No EMBEDS document");
    if (accDoc) {
      accDoc->QueryInterface(NS_GET_IID(nsIAccessible), (void**)aRelated);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsRootAccessible::FireDocLoadEvents(PRUint32 aEventType)
{
  if (!mDocument || !mWeakShell) {
    return NS_OK;  
  }

  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsAccUtils::GetDocShellTreeItemFor(mDOMNode);
  NS_ASSERTION(docShellTreeItem, "No doc shell tree item for document");
  NS_ENSURE_TRUE(docShellTreeItem, NS_ERROR_FAILURE);
  PRInt32 contentType;
  docShellTreeItem->GetItemType(&contentType);
  if (contentType == nsIDocShellTreeItem::typeContent) {
    return nsDocAccessibleWrap::FireDocLoadEvents(aEventType); 
  }

  
  mIsContentLoaded = (aEventType == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE ||
                      aEventType == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_STOPPED);

  return NS_OK;
}
