




































#define CreateEvent CreateEventA
#include "nsIDOMDocument.h"

#include "States.h"
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
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMDataContainerEvent.h"
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
#include "nsIPrivateDOMEvent.h"
#include "nsFocusManager.h"

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
  mFlags |= eRootAccessible;
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

  nsCOMPtr<nsIDOMDocument> document = do_QueryInterface(mDocument);
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

PRUint64
nsRootAccessible::NativeState()
{
  PRUint64 states = nsDocAccessibleWrap::NativeState();

#ifdef MOZ_XUL
  PRUint32 chromeFlags = GetChromeFlags();
  if (chromeFlags & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE)
    states |= states::SIZEABLE;
    
    
    
  if (chromeFlags & nsIWebBrowserChrome::CHROME_TITLEBAR)
    states |= states::MOVEABLE;
  if (chromeFlags & nsIWebBrowserChrome::CHROME_MODAL)
    states |= states::MODAL;
#endif

  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm) {
    nsCOMPtr<nsIDOMWindow> rootWindow;
    GetWindow(getter_AddRefs(rootWindow));

    nsCOMPtr<nsIDOMWindow> activeWindow;
    fm->GetActiveWindow(getter_AddRefs(activeWindow));
    if (activeWindow == rootWindow)
      states |= states::ACTIVE;
  }

  return states;
}

const char* const docEvents[] = {
#ifdef DEBUG_DRAGDROPSTART
  
  
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
  
  
  
  
  nsCOMPtr<nsIDOMEventTarget> nstarget(do_QueryInterface(mDocument));

  if (nstarget) {
    for (const char* const* e = docEvents,
                   * const* e_end = docEvents + NS_ARRAY_LENGTH(docEvents);
         e < e_end; ++e) {
      nsresult rv = nstarget->AddEventListener(NS_ConvertASCIItoUTF16(*e),
                                               this, PR_TRUE, PR_TRUE, 2);
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

void
nsRootAccessible::FireAccessibleFocusEvent(nsAccessible* aFocusAccessible,
                                           nsIContent* aRealFocusContent,
                                           PRBool aForceEvent,
                                           EIsFromUserInput aIsFromUserInput)
{
  

  
  if (mCaretAccessible && aRealFocusContent)
    mCaretAccessible->SetControlSelectionListener(aRealFocusContent);

  nsAccessible* focusAccessible = aFocusAccessible;

  
  
  
  
  nsIContent* content = focusAccessible->GetContent();
  if (content) {
    nsAutoString id;
    if (content->GetAttr(kNameSpaceID_None,
                         nsAccessibilityAtoms::aria_activedescendant, id)) {
      nsIDocument* DOMDoc = content->GetOwnerDoc();
      nsIContent* activeDescendantContent = DOMDoc->GetElementById(id);

      
      
      if (activeDescendantContent) {
        nsAccessible* activeDescendant = 
          GetAccService()->GetAccessible(activeDescendantContent);
        if (activeDescendant) {
          focusAccessible = activeDescendant;
        }
      }
    }
  }

  
  
  nsINode* focusNode = focusAccessible->GetNode();
  if (gLastFocusedNode == focusNode && !aForceEvent)
    return;

  nsDocAccessible* focusDocument = focusAccessible->GetDocAccessible();
  NS_ASSERTION(focusDocument, "No document while accessible is in document?!");

  gLastFocusedAccessiblesState = focusAccessible->State();

  
  if (focusAccessible->ARIARole() == nsIAccessibleRole::ROLE_MENUITEM) {
    
    if (!mCurrentARIAMenubar) {
      
      nsAccessible* menuBarAccessible =
        nsAccUtils::GetAncestorWithRole(focusAccessible,
                                        nsIAccessibleRole::ROLE_MENUBAR);
      if (menuBarAccessible) {
        mCurrentARIAMenubar = menuBarAccessible->GetNode();
        if (mCurrentARIAMenubar) {
          nsRefPtr<AccEvent> menuStartEvent =
            new AccEvent(nsIAccessibleEvent::EVENT_MENU_START,
                         menuBarAccessible, aIsFromUserInput,
                         AccEvent::eAllowDupes);
          if (menuStartEvent)
            focusDocument->FireDelayedAccessibleEvent(menuStartEvent);
        }
      }
    }
  }
  else if (mCurrentARIAMenubar) {
    
    nsRefPtr<AccEvent> menuEndEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_MENU_END, mCurrentARIAMenubar,
                   aIsFromUserInput, AccEvent::eAllowDupes);
    if (menuEndEvent) {
      focusDocument->FireDelayedAccessibleEvent(menuEndEvent);
    }
    mCurrentARIAMenubar = nsnull;
  }

  NS_IF_RELEASE(gLastFocusedNode);
  gLastFocusedNode = focusNode;
  NS_IF_ADDREF(gLastFocusedNode);

  
  
  focusDocument->FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_FOCUS,
                                            focusNode,
                                            AccEvent::eCoalesceFromSameDocument,
                                            aIsFromUserInput);
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

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(mDocument);
  if (domDoc) {
    nsCOMPtr<nsIDOMEvent> event;
    if (NS_SUCCEEDED(domDoc->CreateEvent(NS_LITERAL_STRING("Events"),
                                         getter_AddRefs(event))) &&
        NS_SUCCEEDED(event->InitEvent(NS_LITERAL_STRING("focus"), PR_TRUE, PR_TRUE))) {

      nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
      nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(focusedNode));
      privateEvent->SetTarget(target);
      HandleEvent(event);
    }
  }
}

void
nsRootAccessible::DocumentActivated(nsDocAccessible* aDocument)
{
}




NS_IMETHODIMP
nsRootAccessible::HandleEvent(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr<nsIDOMNSEvent> DOMNSEvent(do_QueryInterface(aDOMEvent));
  nsCOMPtr<nsIDOMEventTarget> DOMEventTarget;
  DOMNSEvent->GetOriginalTarget(getter_AddRefs(DOMEventTarget));
  nsCOMPtr<nsINode> origTargetNode(do_QueryInterface(DOMEventTarget));
  if (!origTargetNode)
    return NS_OK;

  nsDocAccessible* document =
    GetAccService()->GetDocAccessible(origTargetNode->GetOwnerDoc());

  if (document) {
#ifdef DEBUG_NOTIFICATIONS
    if (origTargetNode->IsElement()) {
      nsIContent* elm = origTargetNode->AsElement();

      nsAutoString tag;
      elm->Tag()->ToString(tag);

      nsIAtom* atomid = elm->GetID();
      nsCAutoString id;
      if (atomid)
        atomid->ToUTF8String(id);

      nsAutoString eventType;
      aDOMEvent->GetType(eventType);

      printf("\nPend DOM event processing for %s@id='%s', type: %s\n\n",
             NS_ConvertUTF16toUTF8(tag).get(), id.get(),
             NS_ConvertUTF16toUTF8(eventType).get());
    }
#endif

    
    
    
    document->HandleNotification<nsRootAccessible, nsIDOMEvent>
      (this, &nsRootAccessible::ProcessDOMEvent, aDOMEvent);
  }

  return NS_OK;
}


void
nsRootAccessible::ProcessDOMEvent(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr<nsIDOMNSEvent> DOMNSEvent(do_QueryInterface(aDOMEvent));
  nsCOMPtr<nsIDOMEventTarget> DOMEventTarget;
  DOMNSEvent->GetOriginalTarget(getter_AddRefs(DOMEventTarget));
  nsCOMPtr<nsINode> origTargetNode(do_QueryInterface(DOMEventTarget));

  nsAutoString eventType;
  aDOMEvent->GetType(eventType);

  nsCOMPtr<nsIWeakReference> weakShell =
    nsCoreUtils::GetWeakShellFor(origTargetNode);
  if (!weakShell)
    return;

  nsAccessible* accessible =
    GetAccService()->GetAccessibleOrContainer(origTargetNode, weakShell);

  if (eventType.EqualsLiteral("popuphiding")) {
    HandlePopupHidingEvent(origTargetNode, accessible);
    return;
  }

  if (!accessible)
    return;

  nsDocAccessible* targetDocument = accessible->GetDocAccessible();
  NS_ASSERTION(targetDocument, "No document while accessible is in document?!");

  nsINode* targetNode = accessible->GetNode();
  nsIContent* targetContent = targetNode->IsElement() ?
    targetNode->AsElement() : nsnull;
  nsIContent* origTargetContent = origTargetNode->IsElement() ?
    origTargetNode->AsElement() : nsnull;

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
        return;
      }

      if (eventType.EqualsLiteral("TreeRowCountChanged")) {
        HandleTreeRowCountChangedEvent(aDOMEvent, treeAcc);
        return;
      }
      
      if (eventType.EqualsLiteral("TreeInvalidated")) {
        HandleTreeInvalidatedEvent(aDOMEvent, treeAcc);
        return;
      }
    }
  }
#endif

  if (eventType.EqualsLiteral("RadioStateChange")) {
    PRUint64 state = accessible->State();

    
    
    
    
    PRBool isEnabled = (state & (states::CHECKED | states::SELECTED)) != 0;

    nsRefPtr<AccEvent> accEvent =
      new AccStateChangeEvent(accessible, states::CHECKED, isEnabled);
    nsEventShell::FireEvent(accEvent);

    if (isEnabled)
      FireAccessibleFocusEvent(accessible, origTargetContent);

    return;
  }

  if (eventType.EqualsLiteral("CheckboxStateChange")) {
    PRUint64 state = accessible->State();

    PRBool isEnabled = !!(state & states::CHECKED);

    nsRefPtr<AccEvent> accEvent =
      new AccStateChangeEvent(accessible, states::CHECKED, isEnabled);

    nsEventShell::FireEvent(accEvent);
    return;
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
    PRUint64 state = accessible->State();
    PRBool isEnabled = (state & states::EXPANDED) != 0;

    nsRefPtr<AccEvent> accEvent =
      new AccStateChangeEvent(accessible, states::EXPANDED, isEnabled);
    nsEventShell::FireEvent(accEvent);
    return;
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
        return;
      }

      nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SELECTION,
                              treeItemAccessible);
      return;
    }
  }
  else
#endif
  if (eventType.EqualsLiteral("focus")) {
    
    
    
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
            return;

          accessible = GetAccService()->GetAccessibleInWeakShell(focusedItem,
                                                                 weakShell);
          if (!accessible)
            return;
        }
      }
    }
    FireAccessibleFocusEvent(accessible, origTargetContent);
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
        return; 
      }
#endif
      nsIFrame* menuFrame = accessible->GetFrame();
      if (!menuFrame)
        return;

      nsIMenuFrame* imenuFrame = do_QueryFrame(menuFrame);
      if (imenuFrame)
        fireFocus = PR_TRUE;
      
      if (imenuFrame && imenuFrame->IsOnMenuBar() &&
                       !imenuFrame->IsOnActiveMenuBar()) {
        
        
        return;
      } else {
        nsAccessible *containerAccessible = accessible->GetParent();
        if (!containerAccessible)
          return;
        
        
        
        if (containerAccessible->State() & states::COLLAPSED) {
          nsAccessible *containerParent = containerAccessible->GetParent();
          if (!containerParent)
            return;
          if (containerParent->Role() != nsIAccessibleRole::ROLE_COMBOBOX) {
            return;
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
      
      FireAccessibleFocusEvent(accessible, origTargetContent, PR_TRUE,
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
    targetDocument->
      FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE,
                                 targetNode, AccEvent::eRemoveDupes);
  }
#ifdef DEBUG_DRAGDROPSTART
  else if (eventType.EqualsLiteral("mouseover")) {
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_DRAGDROP_START,
                            accessible);
  }
#endif
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
      if (parent->State() & states::INVISIBLE)
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




void
nsRootAccessible::HandlePopupShownEvent(nsAccessible* aAccessible)
{
  PRUint32 role = aAccessible->Role();

  if (role == nsIAccessibleRole::ROLE_MENUPOPUP) {
    
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START,
                            aAccessible);
    return;
  }

  if (role == nsIAccessibleRole::ROLE_TOOLTIP) {
    
    
    
    
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SHOW, aAccessible);
    return;
  }

  if (role == nsIAccessibleRole::ROLE_COMBOBOX_LIST) {
    
    nsAccessible* combobox = aAccessible->GetParent();
    if (!combobox)
      return;

    PRUint32 comboboxRole = combobox->Role();
    if (comboboxRole == nsIAccessibleRole::ROLE_COMBOBOX ||
        comboboxRole == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
      nsRefPtr<AccEvent> event =
        new AccStateChangeEvent(combobox, states::EXPANDED, PR_TRUE);
      if (event)
        nsEventShell::FireEvent(event);
    }
  }
}

void
nsRootAccessible::HandlePopupHidingEvent(nsINode* aNode,
                                         nsAccessible* aAccessible)
{
  
  
  
  
  

  if (gLastFocusedNode &&
      nsCoreUtils::IsAncestorOf(aNode, gLastFocusedNode)) {
    
    FireCurrentFocusEvent();
  }

  
  if (!aAccessible ||
      aAccessible->Role() != nsIAccessibleRole::ROLE_COMBOBOX_LIST)
    return;

  nsAccessible* combobox = aAccessible->GetParent();
  if (!combobox)
    return;

  PRUint32 comboboxRole = combobox->Role();
  if (comboboxRole == nsIAccessibleRole::ROLE_COMBOBOX ||
      comboboxRole == nsIAccessibleRole::ROLE_AUTOCOMPLETE) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(combobox, states::EXPANDED, PR_FALSE);
    if (event)
      nsEventShell::FireEvent(event);
  }
}

#ifdef MOZ_XUL
void
nsRootAccessible::HandleTreeRowCountChangedEvent(nsIDOMEvent* aEvent,
                                                 nsXULTreeAccessible* aAccessible)
{
  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent(do_QueryInterface(aEvent));
  if (!dataEvent)
    return;

  nsCOMPtr<nsIVariant> indexVariant;
  dataEvent->GetData(NS_LITERAL_STRING("index"),
                     getter_AddRefs(indexVariant));
  if (!indexVariant)
    return;

  nsCOMPtr<nsIVariant> countVariant;
  dataEvent->GetData(NS_LITERAL_STRING("count"),
                     getter_AddRefs(countVariant));
  if (!countVariant)
    return;

  PRInt32 index, count;
  indexVariant->GetAsInt32(&index);
  countVariant->GetAsInt32(&count);

  aAccessible->InvalidateCache(index, count);
}

void
nsRootAccessible::HandleTreeInvalidatedEvent(nsIDOMEvent* aEvent,
                                             nsXULTreeAccessible* aAccessible)
{
  nsCOMPtr<nsIDOMDataContainerEvent> dataEvent(do_QueryInterface(aEvent));
  if (!dataEvent)
    return;

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
}
#endif
