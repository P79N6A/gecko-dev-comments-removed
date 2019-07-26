




#include "RootAccessible.h"

#include "mozilla/Util.h"

#define CreateEvent CreateEventA
#include "nsIDOMDocument.h"

#include "Accessible-inl.h"
#include "DocAccessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"
#ifdef MOZ_XUL
#include "XULTreeAccessible.h"
#endif

#include "mozilla/dom/Element.h"

#include "nsIAccessibleRelation.h"
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
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMXULPopupElement.h"
#include "nsIDocument.h"
#include "nsEventListenerManager.h"
#include "nsIFrame.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIServiceManager.h"
#include "nsPIDOMWindow.h"
#include "nsIWebBrowserChrome.h"
#include "nsReadableUtils.h"
#include "nsFocusManager.h"

#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#include "nsIXULWindow.h"
#endif

using namespace mozilla;
using namespace mozilla::a11y;




NS_IMPL_ISUPPORTS_INHERITED1(RootAccessible, DocAccessible, nsIAccessibleDocument)




RootAccessible::
  RootAccessible(nsIDocument* aDocument, nsIContent* aRootContent,
                 nsIPresShell* aPresShell) :
  DocAccessibleWrap(aDocument, aRootContent, aPresShell)
{
  mFlags |= eRootAccessible;
}

RootAccessible::~RootAccessible()
{
}




ENameValueFlag
RootAccessible::Name(nsString& aName)
{
  aName.Truncate();

  if (mRoleMapEntry) {
    Accessible::Name(aName);
    if (!aName.IsEmpty())
      return eNameOK;
  }

  nsCOMPtr<nsIDOMDocument> document = do_QueryInterface(mDocument);
  NS_ENSURE_TRUE(document, eNameOK);
  document->GetTitle(aName);
  return eNameOK;
}

role
RootAccessible::NativeRole()
{
  
  dom::Element* rootElm = mDocument->GetRootElement();
  if (rootElm && (rootElm->Tag() == nsGkAtoms::dialog ||
                  rootElm->Tag() == nsGkAtoms::wizard))
    return roles::DIALOG;

  return DocAccessibleWrap::NativeRole();
}


#ifdef MOZ_XUL
PRUint32
RootAccessible::GetChromeFlags()
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
RootAccessible::NativeState()
{
  PRUint64 state = DocAccessibleWrap::NativeState();
  if (state & states::DEFUNCT)
    return state;

#ifdef MOZ_XUL
  PRUint32 chromeFlags = GetChromeFlags();
  if (chromeFlags & nsIWebBrowserChrome::CHROME_WINDOW_RESIZE)
    state |= states::SIZEABLE;
    
    
    
  if (chromeFlags & nsIWebBrowserChrome::CHROME_TITLEBAR)
    state |= states::MOVEABLE;
  if (chromeFlags & nsIWebBrowserChrome::CHROME_MODAL)
    state |= states::MODAL;
#endif

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm && fm->GetActiveWindow() == mDocument->GetWindow())
    state |= states::ACTIVE;

  return state;
}

const char* const docEvents[] = {
#ifdef DEBUG_DRAGDROPSTART
  
  
  "mouseover",
#endif
  
  "select",
  
  "ValueChange",
  
  "AlertActive",
  "TreeRowCountChanged",
  "TreeInvalidated",
  
  "OpenStateChange",
  
  "CheckboxStateChange",
  
  "RadioStateChange",
  "popupshown",
  "popuphiding",
  "DOMMenuInactive",
  "DOMMenuItemActive",
  "DOMMenuItemInactive",
  "DOMMenuBarActive",
  "DOMMenuBarInactive"
};

nsresult
RootAccessible::AddEventListeners()
{
  
  
  
  
  nsCOMPtr<nsIDOMEventTarget> nstarget(do_QueryInterface(mDocument));

  if (nstarget) {
    for (const char* const* e = docEvents,
                   * const* e_end = ArrayEnd(docEvents);
         e < e_end; ++e) {
      nsresult rv = nstarget->AddEventListener(NS_ConvertASCIItoUTF16(*e),
                                               this, true, true, 2);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (!mCaretAccessible) {
    mCaretAccessible = new nsCaretAccessible(this);
  }

  return DocAccessible::AddEventListeners();
}

nsresult
RootAccessible::RemoveEventListeners()
{
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mDocument));
  if (target) { 
    for (const char* const* e = docEvents,
                   * const* e_end = ArrayEnd(docEvents);
         e < e_end; ++e) {
      nsresult rv = target->RemoveEventListener(NS_ConvertASCIItoUTF16(*e), this, true);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  DocAccessible::RemoveEventListeners();

  if (mCaretAccessible) {
    mCaretAccessible->Shutdown();
    mCaretAccessible = nsnull;
  }

  return NS_OK;
}




nsCaretAccessible*
RootAccessible::GetCaretAccessible()
{
  return mCaretAccessible;
}

void
RootAccessible::DocumentActivated(DocAccessible* aDocument)
{
}




NS_IMETHODIMP
RootAccessible::HandleEvent(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr<nsIDOMNSEvent> DOMNSEvent(do_QueryInterface(aDOMEvent));
  nsCOMPtr<nsIDOMEventTarget> DOMEventTarget;
  DOMNSEvent->GetOriginalTarget(getter_AddRefs(DOMEventTarget));
  nsCOMPtr<nsINode> origTargetNode(do_QueryInterface(DOMEventTarget));
  if (!origTargetNode)
    return NS_OK;

  DocAccessible* document =
    GetAccService()->GetDocAccessible(origTargetNode->OwnerDoc());

  if (document) {
#ifdef DEBUG
    if (logging::IsEnabled(logging::eDOMEvents)) {
      nsAutoString eventType;
      aDOMEvent->GetType(eventType);

      logging::MsgBegin("DOMEvents", "event '%s' handled",
                        NS_ConvertUTF16toUTF8(eventType).get());
      logging::Node("target", origTargetNode);
      logging::MsgEnd();
    }
#endif

    
    
    
    document->HandleNotification<RootAccessible, nsIDOMEvent>
      (this, &RootAccessible::ProcessDOMEvent, aDOMEvent);
  }

  return NS_OK;
}


void
RootAccessible::ProcessDOMEvent(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr<nsIDOMNSEvent> DOMNSEvent(do_QueryInterface(aDOMEvent));
  nsCOMPtr<nsIDOMEventTarget> DOMEventTarget;
  DOMNSEvent->GetOriginalTarget(getter_AddRefs(DOMEventTarget));
  nsCOMPtr<nsINode> origTargetNode(do_QueryInterface(DOMEventTarget));

  nsAutoString eventType;
  aDOMEvent->GetType(eventType);

  if (eventType.EqualsLiteral("popuphiding")) {
    HandlePopupHidingEvent(origTargetNode);
    return;
  }

  DocAccessible* targetDocument = GetAccService()->
    GetDocAccessible(origTargetNode->OwnerDoc());
  NS_ASSERTION(targetDocument, "No document while accessible is in document?!");

  Accessible* accessible = 
    targetDocument->GetAccessibleOrContainer(origTargetNode);
  if (!accessible)
    return;

  nsINode* targetNode = accessible->GetNode();

#ifdef MOZ_XUL
  XULTreeAccessible* treeAcc = accessible->AsXULTree();
  if (treeAcc) {
    if (eventType.EqualsLiteral("TreeRowCountChanged")) {
      HandleTreeRowCountChangedEvent(aDOMEvent, treeAcc);
      return;
    }

    if (eventType.EqualsLiteral("TreeInvalidated")) {
      HandleTreeInvalidatedEvent(aDOMEvent, treeAcc);
      return;
    }
  }
#endif

  if (eventType.EqualsLiteral("RadioStateChange")) {
    PRUint64 state = accessible->State();

    
    
    
    
    bool isEnabled = (state & (states::CHECKED | states::SELECTED)) != 0;

    nsRefPtr<AccEvent> accEvent =
      new AccStateChangeEvent(accessible, states::CHECKED, isEnabled);
    nsEventShell::FireEvent(accEvent);

    if (isEnabled) {
      FocusMgr()->ActiveItemChanged(accessible);
      A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE("RadioStateChange", accessible)
    }

    return;
  }

  if (eventType.EqualsLiteral("CheckboxStateChange")) {
    PRUint64 state = accessible->State();

    bool isEnabled = !!(state & states::CHECKED);

    nsRefPtr<AccEvent> accEvent =
      new AccStateChangeEvent(accessible, states::CHECKED, isEnabled);

    nsEventShell::FireEvent(accEvent);
    return;
  }

  Accessible* treeItemAcc = nsnull;
#ifdef MOZ_XUL
  
  if (treeAcc) {
    treeItemAcc = accessible->CurrentItem();
    if (treeItemAcc)
      accessible = treeItemAcc;
  }

  if (treeItemAcc && eventType.EqualsLiteral("OpenStateChange")) {
    PRUint64 state = accessible->State();
    bool isEnabled = (state & states::EXPANDED) != 0;

    nsRefPtr<AccEvent> accEvent =
      new AccStateChangeEvent(accessible, states::EXPANDED, isEnabled);
    nsEventShell::FireEvent(accEvent);
    return;
  }

  if (treeItemAcc && eventType.EqualsLiteral("select")) {
    
    
    
    if (FocusMgr()->HasDOMFocus(targetNode)) {
      nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSel =
        do_QueryInterface(targetNode);
      nsAutoString selType;
      multiSel->GetSelType(selType);
      if (selType.IsEmpty() || !selType.EqualsLiteral("single")) {
        
        
        
        
        nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SELECTION_WITHIN,
                                accessible);
        return;
      }

      nsRefPtr<AccSelChangeEvent> selChangeEvent =
        new AccSelChangeEvent(treeAcc, treeItemAcc,
                              AccSelChangeEvent::eSelectionAdd);
      nsEventShell::FireEvent(selChangeEvent);
      return;
    }
  }
  else
#endif
  if (eventType.EqualsLiteral("AlertActive")) {
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_ALERT, accessible);
  }
  else if (eventType.EqualsLiteral("popupshown")) {
    HandlePopupShownEvent(accessible);
  }
  else if (eventType.EqualsLiteral("DOMMenuInactive")) {
    if (accessible->Role() == roles::MENUPOPUP) {
      nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_END,
                              accessible);
    }
  }
  else if (eventType.EqualsLiteral("DOMMenuItemActive")) {
    FocusMgr()->ActiveItemChanged(accessible);
    A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE("DOMMenuItemActive", accessible)
  }
  else if (eventType.EqualsLiteral("DOMMenuItemInactive")) {
    
    
    
    
    Accessible* widget =
      accessible->IsWidget() ? accessible : accessible->ContainerWidget();
    if (widget && widget->IsAutoCompletePopup()) {
      FocusMgr()->ActiveItemChanged(nsnull);
      A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE("DOMMenuItemInactive", accessible)
    }
  }
  else if (eventType.EqualsLiteral("DOMMenuBarActive")) {  
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENU_START,
                            accessible, eFromUserInput);

    
    
    
    
    
    
    Accessible* activeItem = accessible->CurrentItem();
    if (activeItem) {
      FocusMgr()->ActiveItemChanged(activeItem);
      A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE("DOMMenuBarActive", accessible)
    }
  }
  else if (eventType.EqualsLiteral("DOMMenuBarInactive")) {  
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENU_END,
                            accessible, eFromUserInput);

    FocusMgr()->ActiveItemChanged(nsnull);
    A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE("DOMMenuBarInactive", accessible)
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
RootAccessible::Shutdown()
{
  
  if (!PresShell())
    return;  

  DocAccessibleWrap::Shutdown();
}


Relation
RootAccessible::RelationByType(PRUint32 aType)
{
  if (!mDocument || aType != nsIAccessibleRelation::RELATION_EMBEDS)
    return DocAccessibleWrap::RelationByType(aType);

  nsIDOMWindow* rootWindow = mDocument->GetWindow();
  if (rootWindow) {
    nsCOMPtr<nsIDOMWindow> contentWindow;
    rootWindow->GetContent(getter_AddRefs(contentWindow));
    if (contentWindow) {
      nsCOMPtr<nsIDOMDocument> contentDOMDocument;
      contentWindow->GetDocument(getter_AddRefs(contentDOMDocument));
      nsCOMPtr<nsIDocument> contentDocumentNode =
        do_QueryInterface(contentDOMDocument);
      if (contentDocumentNode) {
        DocAccessible* contentDocument =
          GetAccService()->GetDocAccessible(contentDocumentNode);
        if (contentDocument)
          return Relation(contentDocument);
      }
    }
  }

  return Relation();
}




void
RootAccessible::HandlePopupShownEvent(Accessible* aAccessible)
{
  roles::Role role = aAccessible->Role();

  if (role == roles::MENUPOPUP) {
    
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_MENUPOPUP_START,
                            aAccessible);
    return;
  }

  if (role == roles::TOOLTIP) {
    
    
    
    
    nsEventShell::FireEvent(nsIAccessibleEvent::EVENT_SHOW, aAccessible);
    return;
  }

  if (role == roles::COMBOBOX_LIST) {
    
    Accessible* combobox = aAccessible->Parent();
    if (!combobox)
      return;

    roles::Role comboboxRole = combobox->Role();
    if (comboboxRole == roles::COMBOBOX || 
	comboboxRole == roles::AUTOCOMPLETE) {
      nsRefPtr<AccEvent> event =
        new AccStateChangeEvent(combobox, states::EXPANDED, true);
      if (event)
        nsEventShell::FireEvent(event);
    }
  }
}

void
RootAccessible::HandlePopupHidingEvent(nsINode* aPopupNode)
{
  
  
  
  DocAccessible* document = nsAccUtils::GetDocAccessibleFor(aPopupNode);
  if (!document)
    return;

  Accessible* popup = document->GetAccessible(aPopupNode);
  if (!popup) {
    Accessible* popupContainer = document->GetContainerAccessible(aPopupNode);
    if (!popupContainer)
      return;

    PRUint32 childCount = popupContainer->ChildCount();
    for (PRUint32 idx = 0; idx < childCount; idx++) {
      Accessible* child = popupContainer->GetChildAt(idx);
      if (child->IsAutoCompletePopup()) {
        popup = child;
        break;
      }
    }

    
    
    if (!popup)
      return;
  }

  
  
  
  
  

  static const PRUint32 kNotifyOfFocus = 1;
  static const PRUint32 kNotifyOfState = 2;
  PRUint32 notifyOf = 0;

  
  
  
  Accessible* widget = nsnull;
  if (popup->IsCombobox()) {
    widget = popup;
  } else {
    widget = popup->ContainerWidget();
    if (!widget) {
      if (!popup->IsMenuPopup())
        return;

      widget = popup;
    }
  }

  if (popup->IsAutoCompletePopup()) {
    
    
    if (widget->IsAutoComplete())
      notifyOf = kNotifyOfState;

  } else if (widget->IsCombobox()) {
    
    
    if (widget->IsActiveWidget())
      notifyOf = kNotifyOfFocus;
    notifyOf |= kNotifyOfState;

  } else if (widget->IsMenuButton()) {
    
    Accessible* compositeWidget = widget->ContainerWidget();
    if (compositeWidget && compositeWidget->IsAutoComplete()) {
      widget = compositeWidget;
      notifyOf = kNotifyOfState;
    }

    
    notifyOf |= kNotifyOfFocus;

  } else if (widget == popup) {
    
    
    
    
    
    notifyOf = kNotifyOfFocus;
  }

  
  if (notifyOf & kNotifyOfFocus) {
    FocusMgr()->ActiveItemChanged(nsnull);
    A11YDEBUG_FOCUS_ACTIVEITEMCHANGE_CAUSE("popuphiding", popup)
  }

  
  if (notifyOf & kNotifyOfState) {
    nsRefPtr<AccEvent> event =
      new AccStateChangeEvent(widget, states::EXPANDED, false);
    document->FireDelayedAccessibleEvent(event);
  }
}

#ifdef MOZ_XUL
void
RootAccessible::HandleTreeRowCountChangedEvent(nsIDOMEvent* aEvent,
                                               XULTreeAccessible* aAccessible)
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
RootAccessible::HandleTreeInvalidatedEvent(nsIDOMEvent* aEvent,
                                           XULTreeAccessible* aAccessible)
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
