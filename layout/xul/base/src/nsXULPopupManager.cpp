



































#include "nsGkAtoms.h"
#include "nsXULPopupManager.h"
#include "nsMenuFrame.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIPopupBoxObject.h"
#include "nsMenuBarListener.h"
#include "nsContentUtils.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMXULElement.h"
#include "nsIXULDocument.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIPrivateDOMEvent.h"
#include "nsEventDispatcher.h"
#include "nsEventStateManager.h"
#include "nsCSSFrameConstructor.h"
#include "nsLayoutUtils.h"
#include "nsIViewManager.h"
#include "nsIComponentManager.h"
#include "nsITimer.h"
#include "nsFocusManager.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShell.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIBaseWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMMouseEvent.h"
#include "nsCaret.h"
#include "nsIDocument.h"
#include "nsPIDOMWindow.h"
#include "nsPIWindowRoot.h"
#include "nsFrameManager.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/LookAndFeel.h"

using namespace mozilla;

#define FLAG_ALT        0x01
#define FLAG_CONTROL    0x02
#define FLAG_SHIFT      0x04
#define FLAG_META       0x08

const nsNavigationDirection DirectionFromKeyCodeTable[2][6] = {
  {
    eNavigationDirection_Last,   
    eNavigationDirection_First,  
    eNavigationDirection_Start,  
    eNavigationDirection_Before, 
    eNavigationDirection_End,    
    eNavigationDirection_After   
  },
  {
    eNavigationDirection_Last,   
    eNavigationDirection_First,  
    eNavigationDirection_End,    
    eNavigationDirection_Before, 
    eNavigationDirection_Start,  
    eNavigationDirection_After   
  }
};

nsXULPopupManager* nsXULPopupManager::sInstance = nsnull;

nsIContent* nsMenuChainItem::Content()
{
  return mFrame->GetContent();
}

void nsMenuChainItem::SetParent(nsMenuChainItem* aParent)
{
  if (mParent) {
    NS_ASSERTION(mParent->mChild == this, "Unexpected - parent's child not set to this");
    mParent->mChild = nsnull;
  }
  mParent = aParent;
  if (mParent) {
    if (mParent->mChild)
      mParent->mChild->mParent = nsnull;
    mParent->mChild = this;
  }
}

void nsMenuChainItem::Detach(nsMenuChainItem** aRoot)
{
  
  
  
  if (mChild) {
    NS_ASSERTION(this != *aRoot, "Unexpected - popup with child at end of chain");
    mChild->SetParent(mParent);
  }
  else {
    
    
    NS_ASSERTION(this == *aRoot, "Unexpected - popup with no child not at end of chain");
    *aRoot = mParent;
    SetParent(nsnull);
  }
}

NS_IMPL_ISUPPORTS4(nsXULPopupManager,
                   nsIDOMEventListener,
                   nsIMenuRollup,
                   nsITimerCallback,
                   nsIObserver)

nsXULPopupManager::nsXULPopupManager() :
  mRangeOffset(0),
  mCachedMousePoint(0, 0),
  mCachedModifiers(0),
  mActiveMenuBar(nsnull),
  mPopups(nsnull),
  mNoHidePanels(nsnull),
  mTimerMenu(nsnull)
{
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->AddObserver(this, "xpcom-shutdown", false);
  }
}

nsXULPopupManager::~nsXULPopupManager() 
{
  NS_ASSERTION(!mPopups && !mNoHidePanels, "XUL popups still open");
}

nsresult
nsXULPopupManager::Init()
{
  sInstance = new nsXULPopupManager();
  NS_ENSURE_TRUE(sInstance, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(sInstance);
  return NS_OK;
}

void
nsXULPopupManager::Shutdown()
{
  NS_IF_RELEASE(sInstance);
}

NS_IMETHODIMP
nsXULPopupManager::Observe(nsISupports *aSubject,
                           const char *aTopic,
                           const PRUnichar *aData)
{
  if (!nsCRT::strcmp(aTopic, "xpcom-shutdown")) {
    if (mKeyListener) {
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keypress"), this, true);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keydown"), this, true);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keyup"), this, true);
      mKeyListener = nsnull;
    }
    mRangeParent = nsnull;
    
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      obs->RemoveObserver(this, "xpcom-shutdown");
    }
  }

  return NS_OK;
}

nsXULPopupManager*
nsXULPopupManager::GetInstance()
{
  return sInstance;
}

NS_IMETHODIMP
nsXULPopupManager::Rollup(PRUint32 aCount, nsIContent** aLastRolledUp)
{
  if (aLastRolledUp)
    *aLastRolledUp = nsnull;

  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item) {
    if (aLastRolledUp) {
      
      
      
      
      
      
      
      
      nsMenuChainItem* first = item;
      while (first->GetParent())
        first = first->GetParent();
      NS_ADDREF(*aLastRolledUp = first->Content());
    }

    
    
    nsIContent* lastPopup = nsnull;
    if (aCount != PR_UINT32_MAX) {
      nsMenuChainItem* last = item;
      while (--aCount && last->GetParent()) {
        last = last->GetParent();
      }
      if (last) {
        lastPopup = last->Content();
      }
    }

    HidePopup(item->Content(), true, true, false, lastPopup);
  }
  return NS_OK;
}


NS_IMETHODIMP nsXULPopupManager::ShouldRollupOnMouseWheelEvent(bool *aShouldRollup) 
{
  
  

  *aShouldRollup = false;
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (!item)
    return NS_OK;

  nsIContent* content = item->Frame()->GetContent();
  if (content) {
    nsAutoString value;
    content->GetAttr(kNameSpaceID_None, nsGkAtoms::type, value);
    *aShouldRollup = StringBeginsWith(value, NS_LITERAL_STRING("autocomplete"));
  }

  return NS_OK;
}


NS_IMETHODIMP nsXULPopupManager::ShouldRollupOnMouseActivate(bool *aShouldRollup) 
{
  *aShouldRollup = false;
  return NS_OK;
}

PRUint32
nsXULPopupManager::GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain)
{
  
  
  
  PRUint32 count = 0, sameTypeCount = 0;

  NS_ASSERTION(aWidgetChain, "null parameter");
  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item) {
    nsCOMPtr<nsIWidget> widget;
    item->Frame()->GetWidget(getter_AddRefs(widget));
    NS_ASSERTION(widget, "open popup has no widget");
    aWidgetChain->AppendElement(widget.get());
    
    
    
    nsMenuChainItem* parent = item->GetParent();
    if (!sameTypeCount) {
      count++;
      if (!parent || item->Frame()->PopupType() != parent->Frame()->PopupType() ||
                     item->IsContextMenu() != parent->IsContextMenu()) {
        sameTypeCount = count;
      }
    }
    item = parent;
  }

  return sameTypeCount;
}

void
nsXULPopupManager::AdjustPopupsOnWindowChange(nsPIDOMWindow* aWindow)
{
  
  
  
  nsMenuChainItem* item = mNoHidePanels;
  while (item) {
    
    
    nsMenuPopupFrame* frame= item->Frame();
    if (frame->GetAutoPosition()) {
      nsIContent* popup = frame->GetContent();
      if (popup) {
        nsIDocument* document = popup->GetCurrentDoc();
        if (document) {
          nsPIDOMWindow* window = document->GetWindow();
          if (window) {
            window = window->GetPrivateRoot();
            if (window == aWindow) {
              frame->SetPopupPosition(nsnull, true);
            }
          }
        }
      }
    }

    item = item->GetParent();
  }
}

static
nsMenuPopupFrame* GetPopupToMoveOrResize(nsIView* aView)
{
  nsIFrame *frame = static_cast<nsIFrame *>(aView->GetClientData());
  if (!frame || frame->GetType() != nsGkAtoms::menuPopupFrame)
    return nsnull;

  
  nsMenuPopupFrame* menuPopupFrame = static_cast<nsMenuPopupFrame *>(frame);
  if (menuPopupFrame->PopupState() != ePopupOpenAndVisible)
    return nsnull;

  return menuPopupFrame;
}

void
nsXULPopupManager::PopupMoved(nsIView* aView, nsIntPoint aPnt)
{
  nsMenuPopupFrame* menuPopupFrame = GetPopupToMoveOrResize(aView);
  if (!menuPopupFrame)
    return;

  
  
  nsIntPoint currentPnt = menuPopupFrame->ScreenPosition();
  if (aPnt.x != currentPnt.x || aPnt.y != currentPnt.y) {
    
    
    
    
    if (menuPopupFrame->IsAnchored() &&
        menuPopupFrame->PopupLevel() == ePopupLevelParent) {
      menuPopupFrame->SetPopupPosition(nsnull, true);
    }
    else {
      menuPopupFrame->MoveTo(aPnt.x, aPnt.y, false);
    }
  }
}

void
nsXULPopupManager::PopupResized(nsIView* aView, nsIntSize aSize)
{
  nsMenuPopupFrame* menuPopupFrame = GetPopupToMoveOrResize(aView);
  if (!menuPopupFrame)
    return;

  nsPresContext* presContext = menuPopupFrame->PresContext();

  nsSize currentSize = menuPopupFrame->GetSize();
  if (aSize.width != presContext->AppUnitsToDevPixels(currentSize.width) ||
      aSize.height != presContext->AppUnitsToDevPixels(currentSize.height)) {
    
    nsIContent* popup = menuPopupFrame->GetContent();
    nsAutoString width, height;
    width.AppendInt(aSize.width);
    height.AppendInt(aSize.height);
    popup->SetAttr(kNameSpaceID_None, nsGkAtoms::width, width, false);
    popup->SetAttr(kNameSpaceID_None, nsGkAtoms::height, height, true);
  }
}

nsIFrame*
nsXULPopupManager::GetFrameOfTypeForContent(nsIContent* aContent,
                                            nsIAtom* aFrameType,
                                            bool aShouldFlush)
{
  if (aShouldFlush) {
    nsIDocument *document = aContent->GetCurrentDoc();
    if (document) {
      nsCOMPtr<nsIPresShell> presShell = document->GetShell();
      if (presShell)
        presShell->FlushPendingNotifications(Flush_Layout);
    }
  }

  nsIFrame* frame = aContent->GetPrimaryFrame();
  return (frame && frame->GetType() == aFrameType) ? frame : nsnull;
}

nsMenuFrame*
nsXULPopupManager::GetMenuFrameForContent(nsIContent* aContent)
{
  
  return static_cast<nsMenuFrame *>
                    (GetFrameOfTypeForContent(aContent, nsGkAtoms::menuFrame, false));
}

nsMenuPopupFrame*
nsXULPopupManager::GetPopupFrameForContent(nsIContent* aContent, bool aShouldFlush)
{
  return static_cast<nsMenuPopupFrame *>
                    (GetFrameOfTypeForContent(aContent, nsGkAtoms::menuPopupFrame, aShouldFlush));
}

nsMenuChainItem*
nsXULPopupManager::GetTopVisibleMenu()
{
  nsMenuChainItem* item = mPopups;
  while (item && item->Frame()->PopupState() == ePopupInvisible)
    item = item->GetParent();
  return item;
}

void
nsXULPopupManager::GetMouseLocation(nsIDOMNode** aNode, PRInt32* aOffset)
{
  *aNode = mRangeParent;
  NS_IF_ADDREF(*aNode);
  *aOffset = mRangeOffset;
}

void
nsXULPopupManager::InitTriggerEvent(nsIDOMEvent* aEvent, nsIContent* aPopup,
                                    nsIContent** aTriggerContent)
{
  mCachedMousePoint = nsIntPoint(0, 0);

  if (aTriggerContent) {
    *aTriggerContent = nsnull;
    if (aEvent) {
      
      nsCOMPtr<nsIDOMEventTarget> target;
      aEvent->GetTarget(getter_AddRefs(target));
      if (target) {
        CallQueryInterface(target, aTriggerContent);
      }
    }
  }

  mCachedModifiers = 0;

  nsCOMPtr<nsIDOMUIEvent> uiEvent = do_QueryInterface(aEvent);
  if (uiEvent) {
    uiEvent->GetRangeParent(getter_AddRefs(mRangeParent));
    uiEvent->GetRangeOffset(&mRangeOffset);

    
    
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aEvent));
    if (privateEvent) {
      NS_ASSERTION(aPopup, "Expected a popup node");
      nsEvent* event;
      event = privateEvent->GetInternalNSEvent();
      if (event) {
        if (event->eventStructType == NS_MOUSE_EVENT ||
            event->eventStructType == NS_KEY_EVENT) {
          nsInputEvent* inputEvent = static_cast<nsInputEvent*>(event);
          if (inputEvent->isAlt) {
            mCachedModifiers |= FLAG_ALT;
          }
          if (inputEvent->isControl) {
            mCachedModifiers |= FLAG_CONTROL;
          }
          if (inputEvent->isShift) {
            mCachedModifiers |= FLAG_SHIFT;
          }
          if (inputEvent->isMeta) {
            mCachedModifiers |= FLAG_META;
          }
        }
        nsIDocument* doc = aPopup->GetCurrentDoc();
        if (doc) {
          nsIPresShell* presShell = doc->GetShell();
          nsPresContext* presContext;
          if (presShell && (presContext = presShell->GetPresContext())) {
            nsPresContext* rootDocPresContext =
              presContext->GetRootPresContext();
            if (!rootDocPresContext)
              return;
            nsIFrame* rootDocumentRootFrame = rootDocPresContext->
                PresShell()->FrameManager()->GetRootFrame();
            if ((event->eventStructType == NS_MOUSE_EVENT || 
                 event->eventStructType == NS_MOUSE_SCROLL_EVENT) &&
                 !(static_cast<nsGUIEvent *>(event))->widget) {
              
              nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
              nsIntPoint clientPt;
              mouseEvent->GetClientX(&clientPt.x);
              mouseEvent->GetClientY(&clientPt.y);

              
              nsPoint thisDocToRootDocOffset = presShell->FrameManager()->
                GetRootFrame()->GetOffsetToCrossDoc(rootDocumentRootFrame);
              
              mCachedMousePoint.x = presContext->AppUnitsToDevPixels(
                  nsPresContext::CSSPixelsToAppUnits(clientPt.x) + thisDocToRootDocOffset.x);
              mCachedMousePoint.y = presContext->AppUnitsToDevPixels(
                  nsPresContext::CSSPixelsToAppUnits(clientPt.y) + thisDocToRootDocOffset.y);
            }
            else if (rootDocumentRootFrame) {
              nsPoint pnt =
                nsLayoutUtils::GetEventCoordinatesRelativeTo(event, rootDocumentRootFrame);
              mCachedMousePoint = nsIntPoint(rootDocPresContext->AppUnitsToDevPixels(pnt.x),
                                             rootDocPresContext->AppUnitsToDevPixels(pnt.y));
            }
          }
        }
      }
    }
  }
  else {
    mRangeParent = nsnull;
    mRangeOffset = 0;
  }
}

void
nsXULPopupManager::SetActiveMenuBar(nsMenuBarFrame* aMenuBar, bool aActivate)
{
  if (aActivate)
    mActiveMenuBar = aMenuBar;
  else if (mActiveMenuBar == aMenuBar)
    mActiveMenuBar = nsnull;

  UpdateKeyboardListeners();
}

void
nsXULPopupManager::ShowMenu(nsIContent *aMenu,
                            bool aSelectFirstItem,
                            bool aAsynchronous)
{
  
  
  if (aMenu) {
    nsIContent* element = aMenu;
    do {
      nsCOMPtr<nsIDOMXULElement> xulelem = do_QueryInterface(element);
      if (xulelem) {
        nsCOMPtr<nsIXULTemplateBuilder> builder;
        xulelem->GetBuilder(getter_AddRefs(builder));
        if (builder) {
          builder->CreateContents(aMenu, true);
          break;
        }
      }
      element = element->GetParent();
    } while (element);
  }

  nsMenuFrame* menuFrame = GetMenuFrameForContent(aMenu);
  if (!menuFrame || !menuFrame->IsMenu())
    return;

  nsMenuPopupFrame* popupFrame =  menuFrame->GetPopup();
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  
  bool parentIsContextMenu = false;
  bool onMenuBar = false;
  bool onmenu = menuFrame->IsOnMenu();

  nsMenuParent* parent = menuFrame->GetMenuParent();
  if (parent && onmenu) {
    parentIsContextMenu = parent->IsContextMenu();
    onMenuBar = parent->IsMenuBar();
  }

  nsAutoString position;
  if (onMenuBar || !onmenu)
    position.AssignLiteral("after_start");
  else
    position.AssignLiteral("end_before");

  
  InitTriggerEvent(nsnull, nsnull, nsnull);
  popupFrame->InitializePopup(aMenu, nsnull, position, 0, 0, true);

  if (aAsynchronous) {
    nsCOMPtr<nsIRunnable> event =
      new nsXULPopupShowingEvent(popupFrame->GetContent(),
                                 parentIsContextMenu, aSelectFirstItem);
    NS_DispatchToCurrentThread(event);
  }
  else {
    nsCOMPtr<nsIContent> popupContent = popupFrame->GetContent();
    FirePopupShowingEvent(popupContent, parentIsContextMenu, aSelectFirstItem);
  }
}

void
nsXULPopupManager::ShowPopup(nsIContent* aPopup,
                             nsIContent* aAnchorContent,
                             const nsAString& aPosition,
                             PRInt32 aXPos, PRInt32 aYPos,
                             bool aIsContextMenu,
                             bool aAttributesOverride,
                             bool aSelectFirstItem,
                             nsIDOMEvent* aTriggerEvent)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup, true);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  nsCOMPtr<nsIContent> triggerContent;
  InitTriggerEvent(aTriggerEvent, aPopup, getter_AddRefs(triggerContent));

  popupFrame->InitializePopup(aAnchorContent, triggerContent, aPosition,
                              aXPos, aYPos, aAttributesOverride);

  FirePopupShowingEvent(aPopup, aIsContextMenu, aSelectFirstItem);
}

void
nsXULPopupManager::ShowPopupAtScreen(nsIContent* aPopup,
                                     PRInt32 aXPos, PRInt32 aYPos,
                                     bool aIsContextMenu,
                                     nsIDOMEvent* aTriggerEvent)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup, true);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  nsCOMPtr<nsIContent> triggerContent;
  InitTriggerEvent(aTriggerEvent, aPopup, getter_AddRefs(triggerContent));

  popupFrame->InitializePopupAtScreen(triggerContent, aXPos, aYPos, aIsContextMenu);
  FirePopupShowingEvent(aPopup, aIsContextMenu, false);
}

void
nsXULPopupManager::ShowTooltipAtScreen(nsIContent* aPopup,
                                       nsIContent* aTriggerContent,
                                       PRInt32 aXPos, PRInt32 aYPos)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup, true);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  InitTriggerEvent(nsnull, nsnull, nsnull);

  mCachedMousePoint = nsIntPoint(aXPos, aYPos);
  
  nsPresContext* rootPresContext =
    popupFrame->PresContext()->GetRootPresContext();
  if (rootPresContext) {
    nsCOMPtr<nsIWidget> widget;
    rootPresContext->PresShell()->GetViewManager()->
      GetRootWidget(getter_AddRefs(widget));
    mCachedMousePoint -= widget->WidgetToScreenOffset();
  }

  popupFrame->InitializePopupAtScreen(aTriggerContent, aXPos, aYPos, false);

  FirePopupShowingEvent(aPopup, false, false);
}

void
nsXULPopupManager::ShowPopupWithAnchorAlign(nsIContent* aPopup,
                                            nsIContent* aAnchorContent,
                                            nsAString& aAnchor,
                                            nsAString& aAlign,
                                            PRInt32 aXPos, PRInt32 aYPos,
                                            bool aIsContextMenu)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup, true);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  InitTriggerEvent(nsnull, nsnull, nsnull);

  popupFrame->InitializePopupWithAnchorAlign(aAnchorContent, aAnchor,
                                             aAlign, aXPos, aYPos);
  FirePopupShowingEvent(aPopup, aIsContextMenu, false);
}

static void
CheckCaretDrawingState() {

  
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm) {
    nsCOMPtr<nsIDOMWindow> window;
    fm->GetFocusedWindow(getter_AddRefs(window));
    if (!window)
      return;

    nsCOMPtr<nsIDOMDocument> domDoc;
    nsCOMPtr<nsIDocument> focusedDoc;
    window->GetDocument(getter_AddRefs(domDoc));
    focusedDoc = do_QueryInterface(domDoc);
    if (!focusedDoc)
      return;

    nsIPresShell* presShell = focusedDoc->GetShell();
    if (!presShell)
      return;

    nsRefPtr<nsCaret> caret = presShell->GetCaret();
    if (!caret)
      return;
    caret->CheckCaretDrawingState();
  }
}

void
nsXULPopupManager::ShowPopupCallback(nsIContent* aPopup,
                                     nsMenuPopupFrame* aPopupFrame,
                                     bool aIsContextMenu,
                                     bool aSelectFirstItem)
{
  nsPopupType popupType = aPopupFrame->PopupType();
  bool ismenu = (popupType == ePopupTypeMenu);

  nsMenuChainItem* item =
    new nsMenuChainItem(aPopupFrame, aIsContextMenu, popupType);
  if (!item)
    return;

  
  
  
  
  if (aPopup->AttrValueIs(kNameSpaceID_None, nsGkAtoms::ignorekeys,
                           nsGkAtoms::_true, eCaseMatters))
    item->SetIgnoreKeys(true);

  if (ismenu) {
    
    nsMenuFrame* menuFrame = aPopupFrame->GetParentMenu();
    if (menuFrame) {
      item->SetOnMenuBar(menuFrame->IsOnMenuBar());
    }
  }

  
  nsWeakFrame weakFrame(aPopupFrame);
  aPopupFrame->ShowPopup(aIsContextMenu, aSelectFirstItem);
  ENSURE_TRUE(weakFrame.IsAlive());

  
  
  
  
  if (aPopupFrame->IsNoAutoHide() || popupType == ePopupTypeTooltip) {
    item->SetParent(mNoHidePanels);
    mNoHidePanels = item;
  }
  else {
    nsIContent* oldmenu = nsnull;
    if (mPopups)
      oldmenu = mPopups->Content();
    item->SetParent(mPopups);
    mPopups = item;
    SetCaptureState(oldmenu);
  }

  if (aSelectFirstItem) {
    nsMenuFrame* next = GetNextMenuItem(aPopupFrame, nsnull, true);
    aPopupFrame->SetCurrentMenuItem(next);
  }

  if (ismenu)
    UpdateMenuItems(aPopup);

  
  
  CheckCaretDrawingState();
}

void
nsXULPopupManager::HidePopup(nsIContent* aPopup,
                             bool aHideChain,
                             bool aDeselectMenu,
                             bool aAsynchronous,
                             nsIContent* aLastPopup)
{
  
  
  nsMenuPopupFrame* popupFrame = nsnull;
  bool foundPanel = false;
  nsMenuChainItem* item = mNoHidePanels;
  while (item) {
    if (item->Content() == aPopup) {
      foundPanel = true;
      popupFrame = item->Frame();
      break;
    }
    item = item->GetParent();
  }

  
  nsMenuChainItem* foundMenu = nsnull;
  item = mPopups;
  while (item) {
    if (item->Content() == aPopup) {
      foundMenu = item;
      break;
    }
    item = item->GetParent();
  }

  nsPopupType type = ePopupTypePanel;
  bool deselectMenu = false;
  nsCOMPtr<nsIContent> popupToHide, nextPopup, lastPopup;
  if (foundMenu) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsMenuChainItem* topMenu = foundMenu;
    
    
    
    if (foundMenu->IsMenu()) {
      item = topMenu->GetChild();
      while (item && item->IsMenu()) {
        topMenu = item;
        item = item->GetChild();
      }
    }
    
    deselectMenu = aDeselectMenu;
    popupToHide = topMenu->Content();
    popupFrame = topMenu->Frame();
    type = popupFrame->PopupType();

    nsMenuChainItem* parent = topMenu->GetParent();

    
    
    if (parent && (aHideChain || topMenu != foundMenu))
      nextPopup = parent->Content();

    lastPopup = aLastPopup ? aLastPopup : (aHideChain ? nsnull : aPopup);
  }
  else if (foundPanel) {
    popupToHide = aPopup;
  }

  if (popupFrame) {
    nsPopupState state = popupFrame->PopupState();
    
    if (state == ePopupHiding)
      return;
    
    
    
    if (state != ePopupInvisible)
      popupFrame->SetPopupState(ePopupHiding);

    
    if (aAsynchronous) {
      nsCOMPtr<nsIRunnable> event =
        new nsXULPopupHidingEvent(popupToHide, nextPopup, lastPopup,
                                  type, deselectMenu);
        NS_DispatchToCurrentThread(event);
    }
    else {
      FirePopupHidingEvent(popupToHide, nextPopup, lastPopup,
                           popupFrame->PresContext(), type, deselectMenu);
    }
  }
}

void
nsXULPopupManager::HidePopupCallback(nsIContent* aPopup,
                                     nsMenuPopupFrame* aPopupFrame,
                                     nsIContent* aNextPopup,
                                     nsIContent* aLastPopup,
                                     nsPopupType aPopupType,
                                     bool aDeselectMenu)
{
  if (mCloseTimer && mTimerMenu == aPopupFrame) {
    mCloseTimer->Cancel();
    mCloseTimer = nsnull;
    mTimerMenu = nsnull;
  }

  
  
  
  
  
  nsMenuChainItem* item = mNoHidePanels;
  while (item) {
    if (item->Content() == aPopup) {
      item->Detach(&mNoHidePanels);
      break;
    }
    item = item->GetParent();
  }

  if (!item) {
    item = mPopups;
    while (item) {
      if (item->Content() == aPopup) {
        item->Detach(&mPopups);
        SetCaptureState(aPopup);
        break;
      }
      item = item->GetParent();
    }
  }

  delete item;

  nsWeakFrame weakFrame(aPopupFrame);
  aPopupFrame->HidePopup(aDeselectMenu, ePopupClosed);
  ENSURE_TRUE(weakFrame.IsAlive());

  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(true, NS_XUL_POPUP_HIDDEN, nsnull, nsMouseEvent::eReal);
  nsEventDispatcher::Dispatch(aPopup, aPopupFrame->PresContext(),
                              &event, nsnull, &status);

  
  if (aNextPopup && aPopup != aLastPopup) {
    nsMenuChainItem* foundMenu = nsnull;
    nsMenuChainItem* item = mPopups;
    while (item) {
      if (item->Content() == aNextPopup) {
        foundMenu = item;
        break;
      }
      item = item->GetParent();
    }

    
    
    
    
    if (foundMenu &&
        (aLastPopup || aPopupType == foundMenu->PopupType())) {

      nsCOMPtr<nsIContent> popupToHide = item->Content();
      nsMenuChainItem* parent = item->GetParent();

      nsCOMPtr<nsIContent> nextPopup;
      if (parent && popupToHide != aLastPopup)
        nextPopup = parent->Content();

      nsMenuPopupFrame* popupFrame = item->Frame();
      nsPopupState state = popupFrame->PopupState();
      if (state == ePopupHiding)
        return;
      if (state != ePopupInvisible)
        popupFrame->SetPopupState(ePopupHiding);

      FirePopupHidingEvent(popupToHide, nextPopup, aLastPopup,
                           popupFrame->PresContext(),
                           foundMenu->PopupType(), aDeselectMenu);
    }
  }
}

void
nsXULPopupManager::HidePopup(nsIView* aView)
{
  nsIFrame *frame = static_cast<nsIFrame *>(aView->GetClientData());
  if (!frame || frame->GetType() != nsGkAtoms::menuPopupFrame)
    return;

  HidePopup(frame->GetContent(), false, true, false);
}

void
nsXULPopupManager::HidePopupAfterDelay(nsMenuPopupFrame* aPopup)
{
  
  
  KillMenuTimer();

  PRInt32 menuDelay =
    LookAndFeel::GetInt(LookAndFeel::eIntID_SubmenuDelay, 300); 

  
  mCloseTimer = do_CreateInstance("@mozilla.org/timer;1");
  mCloseTimer->InitWithCallback(this, menuDelay, nsITimer::TYPE_ONE_SHOT);

  
  
  mTimerMenu = aPopup;
}

void
nsXULPopupManager::HidePopupsInList(const nsTArray<nsMenuPopupFrame *> &aFrames,
                                    bool aDeselectMenu)
{
  
  
  
  nsTArray<nsWeakFrame> weakPopups(aFrames.Length());
  PRUint32 f;
  for (f = 0; f < aFrames.Length(); f++) {
    nsWeakFrame* wframe = weakPopups.AppendElement();
    if (wframe)
      *wframe = aFrames[f];
  }

  for (f = 0; f < weakPopups.Length(); f++) {
    
    if (weakPopups[f].IsAlive()) {
      nsMenuPopupFrame* frame =
        static_cast<nsMenuPopupFrame *>(weakPopups[f].GetFrame());
      frame->HidePopup(true, ePopupInvisible);
    }
  }

  SetCaptureState(nsnull);
}

bool
nsXULPopupManager::IsChildOfDocShell(nsIDocument* aDoc, nsIDocShellTreeItem* aExpected)
{
  nsCOMPtr<nsISupports> doc = aDoc->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellItem(do_QueryInterface(doc));
  while(docShellItem) {
    if (docShellItem == aExpected)
      return true;

    nsCOMPtr<nsIDocShellTreeItem> parent;
    docShellItem->GetParent(getter_AddRefs(parent));
    docShellItem = parent;
  }

  return false;
}

void
nsXULPopupManager::HidePopupsInDocShell(nsIDocShellTreeItem* aDocShellToHide)
{
  nsTArray<nsMenuPopupFrame *> popupsToHide;

  
  nsMenuChainItem* item = mPopups;
  while (item) {
    nsMenuChainItem* parent = item->GetParent();
    if (item->Frame()->PopupState() != ePopupInvisible &&
        IsChildOfDocShell(item->Content()->OwnerDoc(), aDocShellToHide)) {
      nsMenuPopupFrame* frame = item->Frame();
      item->Detach(&mPopups);
      delete item;
      popupsToHide.AppendElement(frame);
    }
    item = parent;
  }

  
  item = mNoHidePanels;
  while (item) {
    nsMenuChainItem* parent = item->GetParent();
    if (item->Frame()->PopupState() != ePopupInvisible &&
        IsChildOfDocShell(item->Content()->OwnerDoc(), aDocShellToHide)) {
      nsMenuPopupFrame* frame = item->Frame();
      item->Detach(&mNoHidePanels);
      delete item;
      popupsToHide.AppendElement(frame);
    }
    item = parent;
  }

  HidePopupsInList(popupsToHide, true);
}

void
nsXULPopupManager::ExecuteMenu(nsIContent* aMenu, nsXULMenuCommandEvent* aEvent)
{
  CloseMenuMode cmm = CloseMenuMode_Auto;

  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::none, &nsGkAtoms::single, nsnull};

  switch (aMenu->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::closemenu,
                                 strings, eCaseMatters)) {
    case 0:
      cmm = CloseMenuMode_None;
      break;
    case 1:
      cmm = CloseMenuMode_Single;
      break;
    default:
      break;
  }

  
  
  
  
  
  nsTArray<nsMenuPopupFrame *> popupsToHide;
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (cmm != CloseMenuMode_None) {
    while (item) {
      
      if (!item->IsMenu())
        break;
      nsMenuChainItem* next = item->GetParent();
      popupsToHide.AppendElement(item->Frame());
      if (cmm == CloseMenuMode_Single) 
        break;
      item = next;
    }

    
    
    HidePopupsInList(popupsToHide, cmm == CloseMenuMode_Auto);
  }

  aEvent->SetCloseMenuMode(cmm);
  nsCOMPtr<nsIRunnable> event = aEvent;
  NS_DispatchToCurrentThread(event);
}

void
nsXULPopupManager::FirePopupShowingEvent(nsIContent* aPopup,
                                         bool aIsContextMenu,
                                         bool aSelectFirstItem)
{
  nsCOMPtr<nsIContent> popup = aPopup; 

  nsIFrame* frame = aPopup->GetPrimaryFrame();
  if (!frame || frame->GetType() != nsGkAtoms::menuPopupFrame)
    return;

  nsMenuPopupFrame* popupFrame = static_cast<nsMenuPopupFrame *>(frame);
  nsPresContext *presContext = popupFrame->PresContext();
  nsCOMPtr<nsIPresShell> presShell = presContext->PresShell();
  nsPopupType popupType = popupFrame->PopupType();

  
  if (!popupFrame->HasGeneratedChildren()) {
    popupFrame->SetGeneratedChildren();
    presShell->FrameConstructor()->GenerateChildFrames(popupFrame);
  }

  
  frame = aPopup->GetPrimaryFrame();
  if (!frame)
    return;

  presShell->FrameNeedsReflow(frame, nsIPresShell::eTreeChange,
                              NS_FRAME_HAS_DIRTY_CHILDREN);

  
  
  
  mOpeningPopup = aPopup;

  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(true, NS_XUL_POPUP_SHOWING, nsnull, nsMouseEvent::eReal);

  
  nsPresContext* rootPresContext =
    presShell->GetPresContext()->GetRootPresContext();
  if (rootPresContext) {
    rootPresContext->PresShell()->GetViewManager()->
      GetRootWidget(getter_AddRefs(event.widget));
  }
  else {
    event.widget = nsnull;
  }

  event.refPoint = mCachedMousePoint;

  event.isAlt = !!(mCachedModifiers & FLAG_ALT);
  event.isControl = !!(mCachedModifiers & FLAG_CONTROL);
  event.isShift = !!(mCachedModifiers & FLAG_SHIFT);
  event.isMeta = !!(mCachedModifiers & FLAG_META);

  nsEventDispatcher::Dispatch(popup, presContext, &event, nsnull, &status);

  mCachedMousePoint = nsIntPoint(0, 0);
  mOpeningPopup = nsnull;

  mCachedModifiers = 0;

  
  
  
  
  if (popupType == ePopupTypePanel &&
      !popup->AttrValueIs(kNameSpaceID_None, nsGkAtoms::noautofocus,
                           nsGkAtoms::_true, eCaseMatters)) {
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (fm) {
      nsIDocument* doc = popup->GetCurrentDoc();

      
      
      
      
      
      nsCOMPtr<nsIDOMElement> currentFocusElement;
      fm->GetFocusedElement(getter_AddRefs(currentFocusElement));
      nsCOMPtr<nsIContent> currentFocus = do_QueryInterface(currentFocusElement);
      if (doc && currentFocus &&
          !nsContentUtils::ContentIsCrossDocDescendantOf(currentFocus, popup)) {
        fm->ClearFocus(doc->GetWindow());
      }
    }
  }

  
  mRangeParent = nsnull;
  mRangeOffset = 0;

  
  frame = aPopup->GetPrimaryFrame();
  if (frame && frame->GetType() == nsGkAtoms::menuPopupFrame) {
    nsMenuPopupFrame* popupFrame = static_cast<nsMenuPopupFrame *>(frame);

    
    
    if (status == nsEventStatus_eConsumeNoDefault) {
      popupFrame->SetPopupState(ePopupClosed);
      popupFrame->ClearTriggerContent();
    }
    else {
      ShowPopupCallback(aPopup, popupFrame, aIsContextMenu, aSelectFirstItem);
    }
  }
}

void
nsXULPopupManager::FirePopupHidingEvent(nsIContent* aPopup,
                                        nsIContent* aNextPopup,
                                        nsIContent* aLastPopup,
                                        nsPresContext *aPresContext,
                                        nsPopupType aPopupType,
                                        bool aDeselectMenu)
{
  nsCOMPtr<nsIPresShell> presShell = aPresContext->PresShell();

  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(true, NS_XUL_POPUP_HIDING, nsnull, nsMouseEvent::eReal);
  nsEventDispatcher::Dispatch(aPopup, aPresContext, &event, nsnull, &status);

  
  if (aPopupType == ePopupTypePanel &&
      !aPopup->AttrValueIs(kNameSpaceID_None, nsGkAtoms::noautofocus,
                           nsGkAtoms::_true, eCaseMatters)) {
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (fm) {
      nsIDocument* doc = aPopup->GetCurrentDoc();

      
      nsCOMPtr<nsIDOMElement> currentFocusElement;
      fm->GetFocusedElement(getter_AddRefs(currentFocusElement));
      nsCOMPtr<nsIContent> currentFocus = do_QueryInterface(currentFocusElement);
      if (doc && currentFocus &&
          nsContentUtils::ContentIsCrossDocDescendantOf(currentFocus, aPopup)) {
        fm->ClearFocus(doc->GetWindow());
      }
    }
  }

  
  nsIFrame* frame = aPopup->GetPrimaryFrame();
  if (frame && frame->GetType() == nsGkAtoms::menuPopupFrame) {
    nsMenuPopupFrame* popupFrame = static_cast<nsMenuPopupFrame *>(frame);

    
    
    
    if (status == nsEventStatus_eConsumeNoDefault &&
        !popupFrame->IsInContentShell()) {
      popupFrame->SetPopupState(ePopupOpenAndVisible);
    }
    else {
      HidePopupCallback(aPopup, popupFrame, aNextPopup, aLastPopup,
                        aPopupType, aDeselectMenu);
    }
  }
}

bool
nsXULPopupManager::IsPopupOpen(nsIContent* aPopup)
{
  
  
  
  nsMenuChainItem* item = mPopups;
  while (item) {
    if (item->Content() == aPopup) {
      NS_ASSERTION(item->Frame()->IsOpen() ||
                   item->Frame()->PopupState() == ePopupHiding ||
                   item->Frame()->PopupState() == ePopupInvisible,
                   "popup in open list not actually open");
      return true;
    }
    item = item->GetParent();
  }

  item = mNoHidePanels;
  while (item) {
    if (item->Content() == aPopup) {
      NS_ASSERTION(item->Frame()->IsOpen() ||
                   item->Frame()->PopupState() == ePopupHiding ||
                   item->Frame()->PopupState() == ePopupInvisible,
                   "popup in open list not actually open");
      return true;
    }
    item = item->GetParent();
  }

  return false;
}

bool
nsXULPopupManager::IsPopupOpenForMenuParent(nsMenuParent* aMenuParent)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item) {
    nsMenuPopupFrame* popup = item->Frame();
    if (popup && popup->IsOpen()) {
      nsMenuFrame* menuFrame = popup->GetParentMenu();
      if (menuFrame && menuFrame->GetMenuParent() == aMenuParent) {
        return true;
      }
    }
    item = item->GetParent();
  }

  return false;
}

nsIFrame*
nsXULPopupManager::GetTopPopup(nsPopupType aType)
{
  if ((aType == ePopupTypePanel || aType == ePopupTypeTooltip) && mNoHidePanels)
    return mNoHidePanels->Frame();

  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item) {
    if (item->PopupType() == aType || aType == ePopupTypeAny)
      return item->Frame();
    item = item->GetParent();
  }

  return nsnull;
}

nsTArray<nsIFrame *>
nsXULPopupManager::GetVisiblePopups()
{
  nsTArray<nsIFrame *> popups;

  nsMenuChainItem* item = mPopups;
  while (item) {
    if (item->Frame()->PopupState() == ePopupOpenAndVisible)
      popups.AppendElement(static_cast<nsIFrame*>(item->Frame()));
    item = item->GetParent();
  }

  item = mNoHidePanels;
  while (item) {
    
    
    if (item->Frame()->PopupState() == ePopupOpenAndVisible && !item->Frame()->IsDragPopup()) {
      popups.AppendElement(static_cast<nsIFrame*>(item->Frame()));
    }
    item = item->GetParent();
  }

  return popups;
}

already_AddRefed<nsIDOMNode>
nsXULPopupManager::GetLastTriggerNode(nsIDocument* aDocument, bool aIsTooltip)
{
  if (!aDocument)
    return nsnull;

  nsCOMPtr<nsIDOMNode> node;

  
  
  
  if (mOpeningPopup && mOpeningPopup->GetCurrentDoc() == aDocument &&
      aIsTooltip == (mOpeningPopup->Tag() == nsGkAtoms::tooltip)) {
    node = do_QueryInterface(nsMenuPopupFrame::GetTriggerContent(GetPopupFrameForContent(mOpeningPopup, false)));
  }
  else {
    nsMenuChainItem* item = aIsTooltip ? mNoHidePanels : mPopups;
    while (item) {
      
      if ((item->PopupType() == ePopupTypeTooltip) == aIsTooltip &&
          item->Content()->GetCurrentDoc() == aDocument) {
        node = do_QueryInterface(nsMenuPopupFrame::GetTriggerContent(item->Frame()));
        if (node)
          break;
      }
      item = item->GetParent();
    }
  }

  return node.forget();
}

bool
nsXULPopupManager::MayShowPopup(nsMenuPopupFrame* aPopup)
{
  
  
  NS_ASSERTION(!aPopup->IsOpen() || IsPopupOpen(aPopup->GetContent()),
               "popup frame state doesn't match XULPopupManager open state");

  nsPopupState state = aPopup->PopupState();

  
  
  NS_ASSERTION(IsPopupOpen(aPopup->GetContent()) || state == ePopupClosed ||
               state == ePopupShowing || state == ePopupInvisible,
               "popup not in XULPopupManager open list is open");

  
  if (state != ePopupClosed && state != ePopupInvisible)
    return false;

  
  if (IsPopupOpen(aPopup->GetContent())) {
    NS_WARNING("Refusing to show duplicate popup");
    return false;
  }

  
  nsCOMPtr<nsIWidget> widget;
  aPopup->GetWidget(getter_AddRefs(widget));
  if (widget && widget->GetLastRollup() == aPopup->GetContent())
      return false;

  nsCOMPtr<nsISupports> cont = aPopup->PresContext()->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(cont);
  nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(dsti);
  if (!baseWin)
    return false;

  PRInt32 type = -1;
  if (NS_FAILED(dsti->GetItemType(&type)))
    return false;

  
  
  if (type != nsIDocShellTreeItem::typeChrome) {
    
    nsCOMPtr<nsIDocShellTreeItem> root;
    dsti->GetRootTreeItem(getter_AddRefs(root));
    nsCOMPtr<nsIDOMWindow> rootWin = do_GetInterface(root);

    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (!fm || !rootWin)
      return false;

    nsCOMPtr<nsIDOMWindow> activeWindow;
    fm->GetActiveWindow(getter_AddRefs(activeWindow));
    if (activeWindow != rootWin)
      return false;

    
    bool visible;
    baseWin->GetVisibility(&visible);
    if (!visible)
      return false;
  }

  
  
  nsCOMPtr<nsIWidget> mainWidget;
  baseWin->GetMainWidget(getter_AddRefs(mainWidget));
  if (mainWidget) {
    PRInt32 sizeMode;
    mainWidget->GetSizeMode(&sizeMode);
    if (sizeMode == nsSizeMode_Minimized)
      return false;
  }

  
  nsMenuFrame* menuFrame = aPopup->GetParentMenu();
  if (menuFrame) {
    nsMenuParent* parentPopup = menuFrame->GetMenuParent();
    if (parentPopup && !parentPopup->IsOpen())
      return false;
  }

  return true;
}

void
nsXULPopupManager::PopupDestroyed(nsMenuPopupFrame* aPopup)
{
  
  if (mTimerMenu == aPopup) {
    if (mCloseTimer) {
      mCloseTimer->Cancel();
      mCloseTimer = nsnull;
    }
    mTimerMenu = nsnull;
  }

  nsMenuChainItem* item = mNoHidePanels;
  while (item) {
    if (item->Frame() == aPopup) {
      item->Detach(&mNoHidePanels);
      delete item;
      break;
    }
    item = item->GetParent();
  }

  nsTArray<nsMenuPopupFrame *> popupsToHide;

  item = mPopups;
  while (item) {
    nsMenuPopupFrame* frame = item->Frame();
    if (frame == aPopup) {
      if (frame->PopupState() != ePopupInvisible) {
        
        
        
        
        nsMenuChainItem* child = item->GetChild();
        while (child) {
          
          
          
          
          
          nsMenuPopupFrame* childframe = child->Frame();
          if (nsLayoutUtils::IsProperAncestorFrame(frame, childframe)) {
            popupsToHide.AppendElement(childframe);
          }
          else {
            
            
            HidePopup(child->Content(), false, false, true);
            break;
          }

          child = child->GetChild();
        }
      }

      item->Detach(&mPopups);
      delete item;
      break;
    }

    item = item->GetParent();
  }

  HidePopupsInList(popupsToHide, false);
}

bool
nsXULPopupManager::HasContextMenu(nsMenuPopupFrame* aPopup)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item && item->Frame() != aPopup) {
    if (item->IsContextMenu())
      return true;
    item = item->GetParent();
  }

  return false;
}

void
nsXULPopupManager::SetCaptureState(nsIContent* aOldPopup)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item && aOldPopup == item->Content())
    return;

  if (mWidget) {
    mWidget->CaptureRollupEvents(this, this, false, false);
    mWidget = nsnull;
  }

  if (item) {
    nsMenuPopupFrame* popup = item->Frame();
    nsCOMPtr<nsIWidget> widget;
    popup->GetWidget(getter_AddRefs(widget));
    if (widget) {
      widget->CaptureRollupEvents(this, this, true,
                                  popup->ConsumeOutsideClicks());
      mWidget = widget;
      popup->AttachedDismissalListener();
    }
  }

  UpdateKeyboardListeners();
}

void
nsXULPopupManager::UpdateKeyboardListeners()
{
  nsCOMPtr<nsIDOMEventTarget> newTarget;
  bool isForMenu = false;
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item) {
    if (!item->IgnoreKeys())
      newTarget = do_QueryInterface(item->Content()->GetDocument());
    isForMenu = item->PopupType() == ePopupTypeMenu;
  }
  else if (mActiveMenuBar) {
    newTarget = do_QueryInterface(mActiveMenuBar->GetContent()->GetDocument());
    isForMenu = true;
  }

  if (mKeyListener != newTarget) {
    if (mKeyListener) {
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keypress"), this, true);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keydown"), this, true);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keyup"), this, true);
      mKeyListener = nsnull;
      nsContentUtils::NotifyInstalledMenuKeyboardListener(false);
    }

    if (newTarget) {
      newTarget->AddEventListener(NS_LITERAL_STRING("keypress"), this, true);
      newTarget->AddEventListener(NS_LITERAL_STRING("keydown"), this, true);
      newTarget->AddEventListener(NS_LITERAL_STRING("keyup"), this, true);
      nsContentUtils::NotifyInstalledMenuKeyboardListener(isForMenu);
      mKeyListener = newTarget;
    }
  }
}

void
nsXULPopupManager::UpdateMenuItems(nsIContent* aPopup)
{
  
  
 
  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(aPopup->GetDocument()));
  PRUint32 count = aPopup->GetChildCount();
  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<nsIContent> grandChild = aPopup->GetChildAt(i);

    if (grandChild->NodeInfo()->Equals(nsGkAtoms::menuitem, kNameSpaceID_XUL)) {
      
      nsAutoString command;
      grandChild->GetAttr(kNameSpaceID_None, nsGkAtoms::command, command);
      if (!command.IsEmpty()) {
        
        nsCOMPtr<nsIDOMElement> commandElt;
        domDoc->GetElementById(command, getter_AddRefs(commandElt));
        nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));
        if (commandContent) {
          nsAutoString commandValue;
          
          if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::disabled, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, commandValue, true);
          else
            grandChild->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, true);

          
          
          
          if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::label, commandValue, true);

          if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandValue, true);

          if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandValue, true);
        }
      }
    }
  }
}











































nsresult
nsXULPopupManager::Notify(nsITimer* aTimer)
{
  if (aTimer == mCloseTimer)
    KillMenuTimer();

  return NS_OK;
}

void
nsXULPopupManager::KillMenuTimer()
{
  if (mCloseTimer && mTimerMenu) {
    mCloseTimer->Cancel();
    mCloseTimer = nsnull;

    if (mTimerMenu->IsOpen())
      HidePopup(mTimerMenu->GetContent(), false, false, true);
  }

  mTimerMenu = nsnull;
}

void
nsXULPopupManager::CancelMenuTimer(nsMenuParent* aMenuParent)
{
  if (mCloseTimer && mTimerMenu == aMenuParent) {
    mCloseTimer->Cancel();
    mCloseTimer = nsnull;
    mTimerMenu = nsnull;
  }
}

static nsGUIEvent* DOMKeyEventToGUIEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aEvent));
  nsEvent* evt = privateEvent ? privateEvent->GetInternalNSEvent() : nsnull;
  return (evt->eventStructType == NS_KEY_EVENT) ? static_cast<nsGUIEvent *>(evt) : nsnull;
}

bool
nsXULPopupManager::HandleShortcutNavigation(nsIDOMKeyEvent* aKeyEvent,
                                            nsMenuPopupFrame* aFrame)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (!aFrame && item)
    aFrame = item->Frame();

  if (aFrame) {
    bool action;
    nsMenuFrame* result = aFrame->FindMenuWithShortcut(aKeyEvent, action);
    if (result) {
      aFrame->ChangeMenuItem(result, false);
      if (action) {
        nsGUIEvent* evt = DOMKeyEventToGUIEvent(aKeyEvent);
        nsMenuFrame* menuToOpen = result->Enter(evt);
        if (menuToOpen) {
          nsCOMPtr<nsIContent> content = menuToOpen->GetContent();
          ShowMenu(content, true, false);
        }
      }
      return true;
    }

    return false;
  }

  if (mActiveMenuBar) {
    nsMenuFrame* result = mActiveMenuBar->FindMenuWithShortcut(aKeyEvent);
    if (result) {
      mActiveMenuBar->SetActive(true);
      result->OpenMenu(true);
      return true;
    }
  }

  return false;
}


bool
nsXULPopupManager::HandleKeyboardNavigation(PRUint32 aKeyCode)
{
  
  
  nsMenuChainItem* item = nsnull;
  nsMenuChainItem* nextitem = GetTopVisibleMenu();

  while (nextitem) {
    item = nextitem;
    nextitem = item->GetParent();

    if (nextitem) {
      
      if (!nextitem->IsMenu())
        break;

      
      
      
      nsMenuParent* expectedParent = static_cast<nsMenuParent *>(nextitem->Frame());
      nsMenuFrame* menuFrame = item->Frame()->GetParentMenu();
      if (!menuFrame || menuFrame->GetMenuParent() != expectedParent) {
        break;
      }
    }
  }

  nsIFrame* itemFrame;
  if (item)
    itemFrame = item->Frame();
  else if (mActiveMenuBar)
    itemFrame = mActiveMenuBar;
  else
    return false;

  nsNavigationDirection theDirection;
  NS_ASSERTION(aKeyCode >= NS_VK_END && aKeyCode <= NS_VK_DOWN, "Illegal key code");
  theDirection = NS_DIRECTION_FROM_KEY_CODE(itemFrame, aKeyCode);

  
  if (item && HandleKeyboardNavigationInPopup(item, theDirection))
    return true;

  
  if (mActiveMenuBar) {
    nsMenuFrame* currentMenu = mActiveMenuBar->GetCurrentMenuItem();
  
    if (NS_DIRECTION_IS_INLINE(theDirection)) {
      nsMenuFrame* nextItem = (theDirection == eNavigationDirection_End) ?
                              GetNextMenuItem(mActiveMenuBar, currentMenu, false) : 
                              GetPreviousMenuItem(mActiveMenuBar, currentMenu, false);
      mActiveMenuBar->ChangeMenuItem(nextItem, true);
      return true;
    }
    else if (NS_DIRECTION_IS_BLOCK(theDirection)) {
      
      if (currentMenu) {
        nsCOMPtr<nsIContent> content = currentMenu->GetContent();
        ShowMenu(content, true, false);
      }
      return true;
    }
  }

  return false;
}

bool
nsXULPopupManager::HandleKeyboardNavigationInPopup(nsMenuChainItem* item,
                                                   nsMenuPopupFrame* aFrame,
                                                   nsNavigationDirection aDir)
{
  NS_ASSERTION(aFrame, "aFrame is null");
  NS_ASSERTION(!item || item->Frame() == aFrame,
               "aFrame is expected to be equal to item->Frame()");

  nsMenuFrame* currentMenu = aFrame->GetCurrentMenuItem();

  aFrame->ClearIncrementalString();

  
  if (!currentMenu && NS_DIRECTION_IS_INLINE(aDir)) {
    
    
    if (aDir == eNavigationDirection_End) {
      nsMenuFrame* nextItem = GetNextMenuItem(aFrame, nsnull, true);
      if (nextItem) {
        aFrame->ChangeMenuItem(nextItem, false);
        return true;
      }
    }
    return false;
  }

  bool isContainer = false;
  bool isOpen = false;
  if (currentMenu) {
    isOpen = currentMenu->IsOpen();
    isContainer = currentMenu->IsMenu();
    if (isOpen) {
      
      nsMenuChainItem* child = item ? item->GetChild() : nsnull;
      if (child && HandleKeyboardNavigationInPopup(child, aDir))
        return true;
    }
    else if (aDir == eNavigationDirection_End &&
             isContainer && !currentMenu->IsDisabled()) {
      
      nsCOMPtr<nsIContent> content = currentMenu->GetContent();
      ShowMenu(content, true, false);
      return true;
    }
  }

  
  if (NS_DIRECTION_IS_BLOCK(aDir) ||
      NS_DIRECTION_IS_BLOCK_TO_EDGE(aDir)) {
    nsMenuFrame* nextItem;

    if (aDir == eNavigationDirection_Before)
      nextItem = GetPreviousMenuItem(aFrame, currentMenu, true);
    else if (aDir == eNavigationDirection_After)
      nextItem = GetNextMenuItem(aFrame, currentMenu, true);
    else if (aDir == eNavigationDirection_First)
      nextItem = GetNextMenuItem(aFrame, nsnull, true);
    else
      nextItem = GetPreviousMenuItem(aFrame, nsnull, true);

    if (nextItem) {
      aFrame->ChangeMenuItem(nextItem, false);
      return true;
    }
  }
  else if (currentMenu && isContainer && isOpen) {
    if (aDir == eNavigationDirection_Start) {
      
      nsMenuPopupFrame* popupFrame = currentMenu->GetPopup();
      if (popupFrame)
        HidePopup(popupFrame->GetContent(), false, false, false);
      return true;
    }
  }

  return false;
}

nsMenuFrame*
nsXULPopupManager::GetNextMenuItem(nsIFrame* aParent,
                                   nsMenuFrame* aStart,
                                   bool aIsPopup)
{
  nsIFrame* immediateParent = nsnull;
  nsPresContext* presContext = aParent->PresContext();
  presContext->PresShell()->
    FrameConstructor()->GetInsertionPoint(aParent, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = aParent;

  nsIFrame* currFrame = nsnull;
  if (aStart)
    currFrame = aStart->GetNextSibling();
  else 
    currFrame = immediateParent->GetFirstPrincipalChild();
  
  while (currFrame) {
    
    if (IsValidMenuItem(presContext, currFrame->GetContent(), aIsPopup)) {
      return (currFrame->GetType() == nsGkAtoms::menuFrame) ?
             static_cast<nsMenuFrame *>(currFrame) : nsnull;
    }
    currFrame = currFrame->GetNextSibling();
  }

  currFrame = immediateParent->GetFirstPrincipalChild();

  
  while (currFrame && currFrame != aStart) {
    
    if (IsValidMenuItem(presContext, currFrame->GetContent(), aIsPopup)) {
      return (currFrame->GetType() == nsGkAtoms::menuFrame) ?
             static_cast<nsMenuFrame *>(currFrame) : nsnull;
    }

    currFrame = currFrame->GetNextSibling();
  }

  
  return aStart;
}

nsMenuFrame*
nsXULPopupManager::GetPreviousMenuItem(nsIFrame* aParent,
                                       nsMenuFrame* aStart,
                                       bool aIsPopup)
{
  nsIFrame* immediateParent = nsnull;
  nsPresContext* presContext = aParent->PresContext();
  presContext->PresShell()->
    FrameConstructor()->GetInsertionPoint(aParent, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = aParent;

  const nsFrameList& frames(immediateParent->PrincipalChildList());

  nsIFrame* currFrame = nsnull;
  if (aStart)
    currFrame = aStart->GetPrevSibling();
  else
    currFrame = frames.LastChild();

  while (currFrame) {
    
    if (IsValidMenuItem(presContext, currFrame->GetContent(), aIsPopup)) {
      return (currFrame->GetType() == nsGkAtoms::menuFrame) ?
             static_cast<nsMenuFrame *>(currFrame) : nsnull;
    }
    currFrame = currFrame->GetPrevSibling();
  }

  currFrame = frames.LastChild();

  
  while (currFrame && currFrame != aStart) {
    
    if (IsValidMenuItem(presContext, currFrame->GetContent(), aIsPopup)) {
      return (currFrame->GetType() == nsGkAtoms::menuFrame) ?
             static_cast<nsMenuFrame *>(currFrame) : nsnull;
    }

    currFrame = currFrame->GetPrevSibling();
  }

  
  return aStart;
}

bool
nsXULPopupManager::IsValidMenuItem(nsPresContext* aPresContext,
                                   nsIContent* aContent,
                                   bool aOnPopup)
{
  PRInt32 ns = aContent->GetNameSpaceID();
  nsIAtom *tag = aContent->Tag();
  if (ns == kNameSpaceID_XUL) {
    if (tag != nsGkAtoms::menu && tag != nsGkAtoms::menuitem)
      return false;
  }
  else if (ns != kNameSpaceID_XHTML || !aOnPopup || tag != nsGkAtoms::option) {
    return false;
  }

  bool skipNavigatingDisabledMenuItem = true;
  if (aOnPopup) {
    skipNavigatingDisabledMenuItem =
      LookAndFeel::GetInt(LookAndFeel::eIntID_SkipNavigatingDisabledMenuItem,
                          0) != 0;
  }

  return !(skipNavigatingDisabledMenuItem &&
           aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                                 nsGkAtoms::_true, eCaseMatters));
}

nsresult
nsXULPopupManager::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aEvent);
  NS_ENSURE_TRUE(keyEvent, NS_ERROR_UNEXPECTED);

  nsAutoString eventType;
  keyEvent->GetType(eventType);
  if (eventType.EqualsLiteral("keyup")) {
    return KeyUp(keyEvent);
  }
  if (eventType.EqualsLiteral("keydown")) {
    return KeyDown(keyEvent);
  }
  if (eventType.EqualsLiteral("keypress")) {
    return KeyPress(keyEvent);
  }

  NS_ABORT();

  return NS_OK;
}

nsresult
nsXULPopupManager::KeyUp(nsIDOMKeyEvent* aKeyEvent)
{
  
  if (!mActiveMenuBar) {
    nsMenuChainItem* item = GetTopVisibleMenu();
    if (!item || item->PopupType() != ePopupTypeMenu)
      return NS_OK;
  }

  aKeyEvent->StopPropagation();
  aKeyEvent->PreventDefault();

  return NS_OK; 
}

nsresult
nsXULPopupManager::KeyDown(nsIDOMKeyEvent* aKeyEvent)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item && item->Frame()->IsMenuLocked())
    return NS_OK;

  
  if (!mActiveMenuBar && (!item || item->PopupType() != ePopupTypeMenu))
    return NS_OK;

  PRInt32 menuAccessKey = -1;

  
  

  nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
  if (menuAccessKey) {
    PRUint32 theChar;
    aKeyEvent->GetKeyCode(&theChar);

    if (theChar == (PRUint32)menuAccessKey) {
      bool ctrl = false;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_CONTROL)
        aKeyEvent->GetCtrlKey(&ctrl);
      bool alt=false;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_ALT)
        aKeyEvent->GetAltKey(&alt);
      bool shift=false;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_SHIFT)
        aKeyEvent->GetShiftKey(&shift);
      bool meta=false;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_META)
        aKeyEvent->GetMetaKey(&meta);
      if (!(ctrl || alt || shift || meta)) {
        
        
        if (mPopups)
          Rollup(nsnull, nsnull);
        else if (mActiveMenuBar)
          mActiveMenuBar->MenuClosed();
      }
    }
  }

  
  
  aKeyEvent->StopPropagation();
  aKeyEvent->PreventDefault();
  return NS_OK; 
}

nsresult
nsXULPopupManager::KeyPress(nsIDOMKeyEvent* aKeyEvent)
{
  
  
  

  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item && item->Frame()->IsMenuLocked())
    return NS_OK;

  
  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(aKeyEvent);
  bool trustedEvent = false;

  if (domNSEvent) {
    domNSEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  NS_ENSURE_TRUE(keyEvent, NS_ERROR_UNEXPECTED);
  PRUint32 theChar;
  keyEvent->GetKeyCode(&theChar);

  
  if (item && item->PopupType() != ePopupTypeMenu) {
    if (theChar == NS_VK_ESCAPE) {
      HidePopup(item->Content(), false, false, false);
      aKeyEvent->StopPropagation();
      aKeyEvent->PreventDefault();
    }
    return NS_OK;
  }

  
  bool consume = (mPopups || mActiveMenuBar);

  if (theChar == NS_VK_LEFT ||
      theChar == NS_VK_RIGHT ||
      theChar == NS_VK_UP ||
      theChar == NS_VK_DOWN ||
      theChar == NS_VK_HOME ||
      theChar == NS_VK_END) {
    HandleKeyboardNavigation(theChar);
  }
  else if (theChar == NS_VK_ESCAPE) {
    
    
    
    
    if (item)
      HidePopup(item->Content(), false, false, false);
    else if (mActiveMenuBar)
      mActiveMenuBar->MenuClosed();
  }
  else if (theChar == NS_VK_TAB
#ifndef XP_MACOSX
           || theChar == NS_VK_F10
#endif
  ) {
    
    if (item)
      Rollup(nsnull, nsnull);
    else if (mActiveMenuBar)
      mActiveMenuBar->MenuClosed();
  }
  else if (theChar == NS_VK_ENTER ||
           theChar == NS_VK_RETURN) {
    
    
    
    nsMenuFrame* menuToOpen = nsnull;
    nsMenuChainItem* item = GetTopVisibleMenu();
    nsGUIEvent* evt = DOMKeyEventToGUIEvent(aKeyEvent);
    if (item)
      menuToOpen = item->Frame()->Enter(evt);
    else if (mActiveMenuBar)
      menuToOpen = mActiveMenuBar->Enter(evt);
    if (menuToOpen) {
      nsCOMPtr<nsIContent> content = menuToOpen->GetContent();
      ShowMenu(content, true, false);
    }
  }
  else {
    HandleShortcutNavigation(keyEvent, nsnull);
  }

  if (consume) {
    aKeyEvent->StopPropagation();
    aKeyEvent->PreventDefault();
  }

  return NS_OK; 
}

NS_IMETHODIMP
nsXULPopupShowingEvent::Run()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    pm->FirePopupShowingEvent(mPopup, mIsContextMenu, mSelectFirstItem);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULPopupHidingEvent::Run()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();

  nsIDocument *document = mPopup->GetCurrentDoc();
  if (pm && document) {
    nsIPresShell* presShell = document->GetShell();
    if (presShell) {
      nsPresContext* context = presShell->GetPresContext();
      if (context) {
        pm->FirePopupHidingEvent(mPopup, mNextPopup, mLastPopup,
                                 context, mPopupType, mDeselectMenu);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULMenuCommandEvent::Run()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (!pm)
    return NS_OK;

  
  
  
  
  

  nsCOMPtr<nsIContent> popup;
  nsMenuFrame* menuFrame = pm->GetMenuFrameForContent(mMenu);
  nsWeakFrame weakFrame(menuFrame);
  if (menuFrame && mFlipChecked) {
    if (menuFrame->IsChecked()) {
      mMenu->UnsetAttr(kNameSpaceID_None, nsGkAtoms::checked, true);
    } else {
      mMenu->SetAttr(kNameSpaceID_None, nsGkAtoms::checked,
                     NS_LITERAL_STRING("true"), true);
    }
  }

  if (menuFrame && weakFrame.IsAlive()) {
    
    
    nsIFrame* popupFrame = menuFrame->GetParent();
    while (popupFrame) {
      if (popupFrame->GetType() == nsGkAtoms::menuPopupFrame) {
        popup = popupFrame->GetContent();
        break;
      }
      popupFrame = popupFrame->GetParent();
    }

    nsPresContext* presContext = menuFrame->PresContext();
    nsCOMPtr<nsIPresShell> shell = presContext->PresShell();
    nsCOMPtr<nsIViewManager> kungFuDeathGrip = shell->GetViewManager();

    
    if (mCloseMenuMode != CloseMenuMode_None)
      menuFrame->SelectMenu(false);

    nsAutoHandlingUserInputStatePusher userInpStatePusher(mUserInput, nsnull,
                                                          shell->GetDocument());
    nsContentUtils::DispatchXULCommand(mMenu, mIsTrusted, nsnull, shell,
                                       mControl, mAlt, mShift, mMeta);
  }

  if (popup && mCloseMenuMode != CloseMenuMode_None)
    pm->HidePopup(popup, mCloseMenuMode == CloseMenuMode_Auto, true, false);

  return NS_OK;
}

nsresult
NS_NewXULPopupManager(nsISupports** aResult)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  NS_IF_ADDREF(pm);
  *aResult = static_cast<nsIMenuRollup *>(pm);
  return NS_OK;
}
