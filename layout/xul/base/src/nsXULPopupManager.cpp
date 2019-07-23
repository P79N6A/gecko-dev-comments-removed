



































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
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMXULElement.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIPrivateDOMEvent.h"
#include "nsEventDispatcher.h"
#include "nsEventStateManager.h"
#include "nsCSSFrameConstructor.h"
#include "nsLayoutUtils.h"
#include "nsIViewManager.h"
#include "nsILookAndFeel.h"
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
#include "nsFrameManager.h"

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

NS_IMPL_ISUPPORTS5(nsXULPopupManager,
                   nsIDOMKeyListener,
                   nsIDOMEventListener,
                   nsIMenuRollup,
                   nsIRollupListener,
                   nsITimerCallback)

nsXULPopupManager::nsXULPopupManager() :
  mRangeOffset(0),
  mCachedMousePoint(0, 0),
  mActiveMenuBar(nsnull),
  mPopups(nsnull),
  mNoHidePanels(nsnull),
  mTimerMenu(nsnull)
{
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

    HidePopup(item->Content(), PR_TRUE, PR_TRUE, PR_FALSE, lastPopup);
  }
  return NS_OK;
}


NS_IMETHODIMP nsXULPopupManager::ShouldRollupOnMouseWheelEvent(PRBool *aShouldRollup) 
{
  
  
  nsMenuChainItem* item = GetTopVisibleMenu();
  *aShouldRollup = (item && !item->Frame()->IsMenu()); 
  return NS_OK;
}


NS_IMETHODIMP nsXULPopupManager::ShouldRollupOnMouseActivate(PRBool *aShouldRollup) 
{
  *aShouldRollup = PR_FALSE;
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
nsXULPopupManager::AdjustPopupsOnWindowChange()
{
  
  
  
  nsMenuChainItem* item = mNoHidePanels;
  while (item) {
    
    if (item->Frame()->GetAutoPosition())
      item->Frame()->SetPopupPosition(nsnull, PR_TRUE);
    item = item->GetParent();
  }
}

nsIFrame*
nsXULPopupManager::GetFrameOfTypeForContent(nsIContent* aContent,
                                            nsIAtom* aFrameType,
                                            PRBool aShouldFlush)
{
  nsIDocument *document = aContent->GetCurrentDoc();
  if (document) {
    nsCOMPtr<nsIPresShell> presShell = document->GetPrimaryShell();
    if (presShell) {
      if (aShouldFlush)
        presShell->FlushPendingNotifications(Flush_Frames);

      nsIFrame* frame = presShell->GetPrimaryFrameFor(aContent);
      if (frame && frame->GetType() == aFrameType)
        return frame;
    }
  }

  return nsnull;
}

nsMenuFrame*
nsXULPopupManager::GetMenuFrameForContent(nsIContent* aContent)
{
  
  return static_cast<nsMenuFrame *>
                    (GetFrameOfTypeForContent(aContent, nsGkAtoms::menuFrame, PR_FALSE));
}

nsMenuPopupFrame*
nsXULPopupManager::GetPopupFrameForContent(nsIContent* aContent)
{
  return static_cast<nsMenuPopupFrame *>
                    (GetFrameOfTypeForContent(aContent, nsGkAtoms::menuPopupFrame, PR_TRUE));
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
nsXULPopupManager::SetTriggerEvent(nsIDOMEvent* aEvent, nsIContent* aPopup)
{
  mCachedMousePoint = nsIntPoint(0, 0);

  nsCOMPtr<nsIDOMNSUIEvent> uiEvent = do_QueryInterface(aEvent);
  if (uiEvent) {
    uiEvent->GetRangeParent(getter_AddRefs(mRangeParent));
    uiEvent->GetRangeOffset(&mRangeOffset);

    
    
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aEvent));
    if (privateEvent) {
      NS_ASSERTION(aPopup, "Expected a popup node");
      nsEvent* event;
      event = privateEvent->GetInternalNSEvent();
      if (event) {
        nsIDocument* doc = aPopup->GetCurrentDoc();
        if (doc) {
          nsIPresShell* presShell = doc->GetPrimaryShell();
          if (presShell && presShell->GetPresContext()) {
            nsPresContext* rootDocPresContext =
                presShell->GetPresContext()->RootPresContext();
            nsIFrame* rootDocumentRootFrame = rootDocPresContext->
                PresShell()->FrameManager()->GetRootFrame();
            if ((event->eventStructType == NS_MOUSE_EVENT || 
                 event->eventStructType == NS_MOUSE_SCROLL_EVENT) &&
                 !(static_cast<nsGUIEvent *>(event))->widget) {
              
              nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
              nsIntPoint clientPt;
              mouseEvent->GetClientX(&clientPt.x);
              mouseEvent->GetClientY(&clientPt.y);

              
              nsPoint thisDocToRootDocOffset =
                presShell->FrameManager()->GetRootFrame()->GetOffsetTo(rootDocumentRootFrame);
              
              mCachedMousePoint.x = rootDocPresContext->AppUnitsToDevPixels(
                  nsPresContext::CSSPixelsToAppUnits(clientPt.x) + thisDocToRootDocOffset.x);
              mCachedMousePoint.y = rootDocPresContext->AppUnitsToDevPixels(
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
nsXULPopupManager::SetActiveMenuBar(nsMenuBarFrame* aMenuBar, PRBool aActivate)
{
  if (aActivate)
    mActiveMenuBar = aMenuBar;
  else if (mActiveMenuBar == aMenuBar)
    mActiveMenuBar = nsnull;

  UpdateKeyboardListeners();
}

void
nsXULPopupManager::ShowMenu(nsIContent *aMenu,
                            PRBool aSelectFirstItem,
                            PRBool aAsynchronous)
{
  
  
  if (aMenu) {
    nsIContent* element = aMenu;
    do {
      nsCOMPtr<nsIDOMXULElement> xulelem = do_QueryInterface(element);
      if (xulelem) {
        nsCOMPtr<nsIXULTemplateBuilder> builder;
        xulelem->GetBuilder(getter_AddRefs(builder));
        if (builder) {
          builder->CreateContents(aMenu, PR_TRUE);
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

  
  PRBool parentIsContextMenu = PR_FALSE;
  PRBool onMenuBar = PR_FALSE;
  PRBool onmenu = menuFrame->IsOnMenu();

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
  popupFrame->InitializePopup(aMenu, position, 0, 0, PR_TRUE);

  if (aAsynchronous) {
    SetTriggerEvent(nsnull, nsnull);
    nsCOMPtr<nsIRunnable> event =
      new nsXULPopupShowingEvent(popupFrame->GetContent(), aMenu, popupFrame->PopupType(),
                                 parentIsContextMenu, aSelectFirstItem);
    NS_DispatchToCurrentThread(event);
  }
  else {
    nsCOMPtr<nsIContent> popupContent = popupFrame->GetContent();
    FirePopupShowingEvent(popupContent, aMenu,
                          popupFrame->PresContext(), popupFrame->PopupType(),
                          parentIsContextMenu, aSelectFirstItem);
  }
}

void
nsXULPopupManager::ShowPopup(nsIContent* aPopup,
                             nsIContent* aAnchorContent,
                             const nsAString& aPosition,
                             PRInt32 aXPos, PRInt32 aYPos,
                             PRBool aIsContextMenu,
                             PRBool aAttributesOverride,
                             PRBool aSelectFirstItem,
                             nsIDOMEvent* aTriggerEvent)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  SetTriggerEvent(aTriggerEvent, aPopup);

  popupFrame->InitializePopup(aAnchorContent, aPosition, aXPos, aYPos,
                              aAttributesOverride);

  FirePopupShowingEvent(aPopup, nsnull, popupFrame->PresContext(),
                        popupFrame->PopupType(), aIsContextMenu, aSelectFirstItem);
}

void
nsXULPopupManager::ShowPopupAtScreen(nsIContent* aPopup,
                                     PRInt32 aXPos, PRInt32 aYPos,
                                     PRBool aIsContextMenu,
                                     nsIDOMEvent* aTriggerEvent)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  SetTriggerEvent(aTriggerEvent, aPopup);

  popupFrame->InitializePopupAtScreen(aXPos, aYPos, aIsContextMenu);

  FirePopupShowingEvent(aPopup, nsnull, popupFrame->PresContext(),
                        popupFrame->PopupType(), aIsContextMenu, PR_FALSE);
}

void
nsXULPopupManager::ShowPopupWithAnchorAlign(nsIContent* aPopup,
                                            nsIContent* aAnchorContent,
                                            nsAString& aAnchor,
                                            nsAString& aAlign,
                                            PRInt32 aXPos, PRInt32 aYPos,
                                            PRBool aIsContextMenu)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  SetTriggerEvent(nsnull, nsnull);

  popupFrame->InitializePopupWithAnchorAlign(aAnchorContent, aAnchor,
                                             aAlign, aXPos, aYPos);

  FirePopupShowingEvent(aPopup, nsnull, popupFrame->PresContext(),
                        popupFrame->PopupType(), aIsContextMenu, PR_FALSE);
}

static void
CheckCaretDrawingState() {

  
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm) {
    nsCOMPtr<nsIDOMWindow> window;
    fm->GetFocusedWindow(getter_AddRefs(window));
    if (!window)
      return;

    nsCOMPtr<nsIDOMWindowInternal> windowInternal = do_QueryInterface(window);

    nsCOMPtr<nsIDOMDocument> domDoc;
    nsCOMPtr<nsIDocument> focusedDoc;
    windowInternal->GetDocument(getter_AddRefs(domDoc));
    focusedDoc = do_QueryInterface(domDoc);
    if (!focusedDoc)
      return;

    nsIPresShell* presShell = focusedDoc->GetPrimaryShell();
    if (!presShell)
      return;

    nsRefPtr<nsCaret> caret;
    presShell->GetCaret(getter_AddRefs(caret));
    if (!caret)
      return;
    caret->CheckCaretDrawingState();
  }
}

void
nsXULPopupManager::ShowPopupCallback(nsIContent* aPopup,
                                     nsMenuPopupFrame* aPopupFrame,
                                     PRBool aIsContextMenu,
                                     PRBool aSelectFirstItem)
{
  
  mRangeParent = nsnull;
  mRangeOffset = 0;

  nsPopupType popupType = aPopupFrame->PopupType();
  PRBool ismenu = (popupType == ePopupTypeMenu);

  nsMenuChainItem* item =
    new nsMenuChainItem(aPopupFrame, aIsContextMenu, popupType);
  if (!item)
    return;

  
  
  
  
  if (aPopup->AttrValueIs(kNameSpaceID_None, nsGkAtoms::ignorekeys,
                           nsGkAtoms::_true, eCaseMatters))
    item->SetIgnoreKeys(PR_TRUE);

  if (ismenu) {
    
    nsIFrame* parent = aPopupFrame->GetParent();
    if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
      nsMenuFrame* menuFrame = static_cast<nsMenuFrame *>(parent);
      item->SetOnMenuBar(menuFrame->IsOnMenuBar());
    }
  }

  
  nsWeakFrame weakFrame(aPopupFrame);
  PRBool hasChildren = aPopupFrame->ShowPopup(aIsContextMenu, aSelectFirstItem);
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

  if (hasChildren) {
    if (aSelectFirstItem) {
      nsMenuFrame* next = GetNextMenuItem(aPopupFrame, nsnull, PR_TRUE);
      aPopupFrame->SetCurrentMenuItem(next);
    }

    if (ismenu)
      UpdateMenuItems(aPopup);
  }

  
  
  CheckCaretDrawingState();
}

void
nsXULPopupManager::HidePopup(nsIContent* aPopup,
                             PRBool aHideChain,
                             PRBool aDeselectMenu,
                             PRBool aAsynchronous,
                             nsIContent* aLastPopup)
{
  
  
  nsMenuPopupFrame* popupFrame = nsnull;
  PRBool foundPanel = PR_FALSE;
  nsMenuChainItem* item = mNoHidePanels;
  while (item) {
    if (item->Content() == aPopup) {
      foundPanel = PR_TRUE;
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
  PRBool deselectMenu = PR_FALSE;
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
                                     PRBool aDeselectMenu)
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
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDDEN, nsnull, nsMouseEvent::eReal);
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
nsXULPopupManager::HidePopupAfterDelay(nsMenuPopupFrame* aPopup)
{
  
  
  KillMenuTimer();

  PRInt32 menuDelay = 300;   
  aPopup->PresContext()->LookAndFeel()->
    GetMetric(nsILookAndFeel::eMetric_SubmenuDelay, menuDelay);

  
  mCloseTimer = do_CreateInstance("@mozilla.org/timer;1");
  mCloseTimer->InitWithCallback(this, menuDelay, nsITimer::TYPE_ONE_SHOT);

  
  
  mTimerMenu = aPopup;
}

void
nsXULPopupManager::HidePopupsInList(const nsTArray<nsMenuPopupFrame *> &aFrames,
                                    PRBool aDeselectMenu)
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
      frame->HidePopup(PR_TRUE, ePopupInvisible);
    }
  }

  SetCaptureState(nsnull);
}

PRBool
nsXULPopupManager::IsChildOfDocShell(nsIDocument* aDoc, nsIDocShellTreeItem* aExpected)
{
  nsCOMPtr<nsISupports> doc = aDoc->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellItem(do_QueryInterface(doc));
  while(docShellItem) {
    if (docShellItem == aExpected)
      return PR_TRUE;

    nsCOMPtr<nsIDocShellTreeItem> parent;
    docShellItem->GetParent(getter_AddRefs(parent));
    docShellItem = parent;
  }

  return PR_FALSE;
}

void
nsXULPopupManager::HidePopupsInDocShell(nsIDocShellTreeItem* aDocShellToHide)
{
  nsTArray<nsMenuPopupFrame *> popupsToHide;

  
  nsMenuChainItem* item = mPopups;
  while (item) {
    nsMenuChainItem* parent = item->GetParent();
    if (item->Frame()->PopupState() != ePopupInvisible &&
        IsChildOfDocShell(item->Content()->GetOwnerDoc(), aDocShellToHide)) {
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
        IsChildOfDocShell(item->Content()->GetOwnerDoc(), aDocShellToHide)) {
      nsMenuPopupFrame* frame = item->Frame();
      item->Detach(&mNoHidePanels);
      delete item;
      popupsToHide.AppendElement(frame);
    }
    item = parent;
  }

  HidePopupsInList(popupsToHide, PR_TRUE);
}

void
nsXULPopupManager::ExecuteMenu(nsIContent* aMenu, nsEvent* aEvent)
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

  
  
  
  PRBool isTrusted = aEvent ? NS_IS_TRUSTED_EVENT(aEvent) :
                              nsContentUtils::IsCallerChrome();

  PRBool shift = PR_FALSE, control = PR_FALSE, alt = PR_FALSE, meta = PR_FALSE;
  if (aEvent && (aEvent->eventStructType == NS_MOUSE_EVENT ||
                 aEvent->eventStructType == NS_KEY_EVENT ||
                 aEvent->eventStructType == NS_ACCESSIBLE_EVENT)) {
    shift = static_cast<nsInputEvent *>(aEvent)->isShift;
    control = static_cast<nsInputEvent *>(aEvent)->isControl;
    alt = static_cast<nsInputEvent *>(aEvent)->isAlt;
    meta = static_cast<nsInputEvent *>(aEvent)->isMeta;
  }

  
  
  
  PRBool userinput = nsEventStateManager::IsHandlingUserInput();

  nsCOMPtr<nsIRunnable> event =
    new nsXULMenuCommandEvent(aMenu, isTrusted, shift, control,
                              alt, meta, userinput, cmm);
  NS_DispatchToCurrentThread(event);
}

void
nsXULPopupManager::FirePopupShowingEvent(nsIContent* aPopup,
                                         nsIContent* aMenu,
                                         nsPresContext* aPresContext,
                                         nsPopupType aPopupType,
                                         PRBool aIsContextMenu,
                                         PRBool aSelectFirstItem)
{
  nsCOMPtr<nsIPresShell> presShell = aPresContext->PresShell();

  
  
  
  
  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWING, nsnull, nsMouseEvent::eReal);
  presShell->GetViewManager()->GetRootWidget(getter_AddRefs(event.widget));
  event.refPoint = mCachedMousePoint;
  nsEventDispatcher::Dispatch(aPopup, aPresContext, &event, nsnull, &status);
  mCachedMousePoint = nsIntPoint(0, 0);

  
  
  
  
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
          !nsContentUtils::ContentIsCrossDocDescendantOf(currentFocus, aPopup)) {
        fm->ClearFocus(doc->GetWindow());
      }
    }
  }

  
  
  
  
  
  nsIDocument *document = aPopup->GetCurrentDoc();
  if (document)
    document->FlushPendingNotifications(Flush_Layout);

  
  nsIFrame* frame = presShell->GetPrimaryFrameFor(aPopup);
  if (frame && frame->GetType() == nsGkAtoms::menuPopupFrame) {
    nsMenuPopupFrame* popupFrame = static_cast<nsMenuPopupFrame *>(frame);

    
    
    if (status == nsEventStatus_eConsumeNoDefault) {
      popupFrame->SetPopupState(ePopupClosed);
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
                                        PRBool aDeselectMenu)
{
  nsCOMPtr<nsIPresShell> presShell = aPresContext->PresShell();

  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDING, nsnull, nsMouseEvent::eReal);
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

  
  nsIFrame* frame = presShell->GetPrimaryFrameFor(aPopup);
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

PRBool
nsXULPopupManager::IsPopupOpen(nsIContent* aPopup)
{
  
  
  
  nsMenuChainItem* item = mPopups;
  while (item) {
    if (item->Content() == aPopup) {
      NS_ASSERTION(item->Frame()->IsOpen() ||
                   item->Frame()->PopupState() == ePopupHiding ||
                   item->Frame()->PopupState() == ePopupInvisible,
                   "popup in open list not actually open");
      return PR_TRUE;
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
      return PR_TRUE;
    }
    item = item->GetParent();
  }

  return PR_FALSE;
}

PRBool
nsXULPopupManager::IsPopupOpenForMenuParent(nsMenuParent* aMenuParent)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item) {
    nsMenuPopupFrame* popup = item->Frame();
    if (popup && popup->IsOpen()) {
      nsIFrame* parent = popup->GetParent();
      if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
        nsMenuFrame* menuFrame = static_cast<nsMenuFrame *>(parent);
        if (menuFrame->GetMenuParent() == aMenuParent)
          return PR_TRUE;
      }
    }
    item = item->GetParent();
  }

  return PR_FALSE;
}

nsIFrame*
nsXULPopupManager::GetTopPopup(nsPopupType aType)
{
  if (aType == ePopupTypePanel && mNoHidePanels)
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

  return popups;
}

PRBool
nsXULPopupManager::MayShowPopup(nsMenuPopupFrame* aPopup)
{
  
  
  NS_ASSERTION(!aPopup->IsOpen() || IsPopupOpen(aPopup->GetContent()),
               "popup frame state doesn't match XULPopupManager open state");

  nsPopupState state = aPopup->PopupState();

  
  
  NS_ASSERTION(IsPopupOpen(aPopup->GetContent()) || state == ePopupClosed ||
               state == ePopupShowing || state == ePopupInvisible,
               "popup not in XULPopupManager open list is open");

  
  if (state != ePopupClosed && state != ePopupInvisible)
    return PR_FALSE;

  
  nsCOMPtr<nsIWidget> widget;
  aPopup->GetWidget(getter_AddRefs(widget));
  if (widget && widget->GetLastRollup() == aPopup->GetContent())
      return PR_FALSE;

  nsCOMPtr<nsISupports> cont = aPopup->PresContext()->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(cont);
  nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(dsti);
  if (!baseWin)
    return PR_FALSE;

  PRInt32 type = -1;
  if (NS_FAILED(dsti->GetItemType(&type)))
    return PR_FALSE;

  
  
  if (type != nsIDocShellTreeItem::typeChrome) {
    
    nsCOMPtr<nsIDocShellTreeItem> root;
    dsti->GetRootTreeItem(getter_AddRefs(root));
    nsCOMPtr<nsIDOMWindow> rootWin = do_GetInterface(root);

    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (!fm || !rootWin)
      return PR_FALSE;

    nsCOMPtr<nsIDOMWindow> activeWindow;
    fm->GetActiveWindow(getter_AddRefs(activeWindow));
    if (activeWindow != rootWin)
      return PR_FALSE;

    
    PRBool visible;
    baseWin->GetVisibility(&visible);
    if (!visible)
      return PR_FALSE;
  }

  
  
  nsCOMPtr<nsIWidget> mainWidget;
  baseWin->GetMainWidget(getter_AddRefs(mainWidget));
  if (mainWidget) {
    PRInt32 sizeMode;
    mainWidget->GetSizeMode(&sizeMode);
    if (sizeMode == nsSizeMode_Minimized)
      return PR_FALSE;
  }

  
  nsIFrame* parent = aPopup->GetParent();
  if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
    nsMenuFrame* menuFrame = static_cast<nsMenuFrame *>(parent);
    nsMenuParent* parentPopup = menuFrame->GetMenuParent();
    if (parentPopup && !parentPopup->IsOpen())
      return PR_FALSE;
  }

  return PR_TRUE;
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
            
            
            HidePopup(child->Content(), PR_FALSE, PR_FALSE, PR_TRUE);
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

  HidePopupsInList(popupsToHide, PR_FALSE);
}

PRBool
nsXULPopupManager::HasContextMenu(nsMenuPopupFrame* aPopup)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item && item->Frame() != aPopup) {
    if (item->IsContextMenu())
      return PR_TRUE;
    item = item->GetParent();
  }

  return PR_FALSE;
}

void
nsXULPopupManager::SetCaptureState(nsIContent* aOldPopup)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item && aOldPopup == item->Content())
    return;

  if (mWidget) {
    mWidget->CaptureRollupEvents(this, PR_FALSE, PR_FALSE);
    mWidget = nsnull;
  }

  if (item) {
    nsMenuPopupFrame* popup = item->Frame();
    nsCOMPtr<nsIWidget> widget;
    popup->GetWidget(getter_AddRefs(widget));
    if (widget) {
      widget->CaptureRollupEvents(this, PR_TRUE, popup->ConsumeOutsideClicks());
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
  PRBool isForMenu = PR_FALSE;
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item) {
    if (!item->IgnoreKeys())
      newTarget = do_QueryInterface(item->Content()->GetDocument());
    isForMenu = item->PopupType() == ePopupTypeMenu;
  }
  else if (mActiveMenuBar) {
    newTarget = do_QueryInterface(mActiveMenuBar->GetContent()->GetDocument());
    isForMenu = PR_TRUE;
  }

  if (mKeyListener != newTarget) {
    if (mKeyListener) {
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keypress"), this, PR_TRUE);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keydown"), this, PR_TRUE);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keyup"), this, PR_TRUE);
      mKeyListener = nsnull;
      nsContentUtils::NotifyInstalledMenuKeyboardListener(PR_FALSE);
    }

    if (newTarget) {
      newTarget->AddEventListener(NS_LITERAL_STRING("keypress"), this, PR_TRUE);
      newTarget->AddEventListener(NS_LITERAL_STRING("keydown"), this, PR_TRUE);
      newTarget->AddEventListener(NS_LITERAL_STRING("keyup"), this, PR_TRUE);
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
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, commandValue, PR_TRUE);
          else
            grandChild->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, PR_TRUE);

          
          
          
          if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::label, commandValue, PR_TRUE);

          if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandValue, PR_TRUE);

          if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandValue, PR_TRUE);
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
      HidePopup(mTimerMenu->GetContent(), PR_FALSE, PR_FALSE, PR_TRUE);
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

PRBool
nsXULPopupManager::HandleShortcutNavigation(nsIDOMKeyEvent* aKeyEvent,
                                            nsMenuPopupFrame* aFrame)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (!aFrame && item)
    aFrame = item->Frame();

  if (aFrame) {
    PRBool action;
    nsMenuFrame* result = aFrame->FindMenuWithShortcut(aKeyEvent, action);
    if (result) {
      aFrame->ChangeMenuItem(result, PR_FALSE);
      if (action) {
        nsMenuFrame* menuToOpen = result->Enter();
        if (menuToOpen) {
          nsCOMPtr<nsIContent> content = menuToOpen->GetContent();
          ShowMenu(content, PR_TRUE, PR_FALSE);
        }
      }
      return PR_TRUE;
    }

    return PR_FALSE;
  }

  if (mActiveMenuBar) {
    nsMenuFrame* result = mActiveMenuBar->FindMenuWithShortcut(aKeyEvent);
    if (result) {
      mActiveMenuBar->SetActive(PR_TRUE);
      result->OpenMenu(PR_TRUE);
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}


PRBool
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
      nsIFrame* parent = item->Frame()->GetParent();
      if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
        nsMenuFrame* menuFrame = static_cast<nsMenuFrame *>(parent);
        if (menuFrame->GetMenuParent() != expectedParent)
          break;
      }
      else {
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
    return PR_FALSE;

  nsNavigationDirection theDirection;
  NS_ASSERTION(aKeyCode >= NS_VK_END && aKeyCode <= NS_VK_DOWN, "Illegal key code");
  theDirection = NS_DIRECTION_FROM_KEY_CODE(itemFrame, aKeyCode);

  
  if (item && HandleKeyboardNavigationInPopup(item, theDirection))
    return PR_TRUE;

  
  if (mActiveMenuBar) {
    nsMenuFrame* currentMenu = mActiveMenuBar->GetCurrentMenuItem();
  
    if (NS_DIRECTION_IS_INLINE(theDirection)) {
      nsMenuFrame* nextItem = (theDirection == eNavigationDirection_End) ?
                              GetNextMenuItem(mActiveMenuBar, currentMenu, PR_FALSE) : 
                              GetPreviousMenuItem(mActiveMenuBar, currentMenu, PR_FALSE);
      mActiveMenuBar->ChangeMenuItem(nextItem, PR_TRUE);
      return PR_TRUE;
    }
    else if (NS_DIRECTION_IS_BLOCK(theDirection)) {
      
      if (currentMenu) {
        nsCOMPtr<nsIContent> content = currentMenu->GetContent();
        ShowMenu(content, PR_TRUE, PR_FALSE);
      }
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

PRBool
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
      nsMenuFrame* nextItem = GetNextMenuItem(aFrame, nsnull, PR_TRUE);
      if (nextItem) {
        aFrame->ChangeMenuItem(nextItem, PR_FALSE);
        return PR_TRUE;
      }
    }
    return PR_FALSE;
  }

  PRBool isContainer = PR_FALSE;
  PRBool isOpen = PR_FALSE;
  if (currentMenu) {
    isOpen = currentMenu->IsOpen();
    isContainer = currentMenu->IsMenu();
    if (isOpen) {
      
      nsMenuChainItem* child = item ? item->GetChild() : nsnull;
      if (child && HandleKeyboardNavigationInPopup(child, aDir))
        return PR_TRUE;
    }
    else if (aDir == eNavigationDirection_End &&
             isContainer && !currentMenu->IsDisabled()) {
      
      nsCOMPtr<nsIContent> content = currentMenu->GetContent();
      ShowMenu(content, PR_TRUE, PR_FALSE);
      return PR_TRUE;
    }
  }

  
  if (NS_DIRECTION_IS_BLOCK(aDir) ||
      NS_DIRECTION_IS_BLOCK_TO_EDGE(aDir)) {
    nsMenuFrame* nextItem;

    if (aDir == eNavigationDirection_Before)
      nextItem = GetPreviousMenuItem(aFrame, currentMenu, PR_TRUE);
    else if (aDir == eNavigationDirection_After)
      nextItem = GetNextMenuItem(aFrame, currentMenu, PR_TRUE);
    else if (aDir == eNavigationDirection_First)
      nextItem = GetNextMenuItem(aFrame, nsnull, PR_TRUE);
    else
      nextItem = GetPreviousMenuItem(aFrame, nsnull, PR_TRUE);

    if (nextItem) {
      aFrame->ChangeMenuItem(nextItem, PR_FALSE);
      return PR_TRUE;
    }
  }
  else if (currentMenu && isContainer && isOpen) {
    if (aDir == eNavigationDirection_Start) {
      
      nsMenuPopupFrame* popupFrame = currentMenu->GetPopup();
      if (popupFrame)
        HidePopup(popupFrame->GetContent(), PR_FALSE, PR_FALSE, PR_FALSE);
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsMenuFrame*
nsXULPopupManager::GetNextMenuItem(nsIFrame* aParent,
                                   nsMenuFrame* aStart,
                                   PRBool aIsPopup)
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
    currFrame = immediateParent->GetFirstChild(nsnull);
  
  while (currFrame) {
    
    if (IsValidMenuItem(presContext, currFrame->GetContent(), aIsPopup)) {
      return (currFrame->GetType() == nsGkAtoms::menuFrame) ?
             static_cast<nsMenuFrame *>(currFrame) : nsnull;
    }
    currFrame = currFrame->GetNextSibling();
  }

  currFrame = immediateParent->GetFirstChild(nsnull);

  
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
                                       PRBool aIsPopup)
{
  nsIFrame* immediateParent = nsnull;
  nsPresContext* presContext = aParent->PresContext();
  presContext->PresShell()->
    FrameConstructor()->GetInsertionPoint(aParent, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = aParent;

  nsFrameList frames(immediateParent->GetFirstChild(nsnull));

  nsIFrame* currFrame = nsnull;
  if (aStart)
    currFrame = frames.GetPrevSiblingFor(aStart);
  else
    currFrame = frames.LastChild();

  while (currFrame) {
    
    if (IsValidMenuItem(presContext, currFrame->GetContent(), aIsPopup)) {
      return (currFrame->GetType() == nsGkAtoms::menuFrame) ?
             static_cast<nsMenuFrame *>(currFrame) : nsnull;
    }
    currFrame = frames.GetPrevSiblingFor(currFrame);
  }

  currFrame = frames.LastChild();

  
  while (currFrame && currFrame != aStart) {
    
    if (IsValidMenuItem(presContext, currFrame->GetContent(), aIsPopup)) {
      return (currFrame->GetType() == nsGkAtoms::menuFrame) ?
             static_cast<nsMenuFrame *>(currFrame) : nsnull;
    }

    currFrame = frames.GetPrevSiblingFor(currFrame);
  }

  
  return aStart;
}

PRBool
nsXULPopupManager::IsValidMenuItem(nsPresContext* aPresContext,
                                   nsIContent* aContent,
                                   PRBool aOnPopup)
{
  PRInt32 ns = aContent->GetNameSpaceID();
  nsIAtom *tag = aContent->Tag();
  if (ns == kNameSpaceID_XUL) {
    if (tag != nsGkAtoms::menu && tag != nsGkAtoms::menuitem)
      return PR_FALSE;
  }
  else if (ns != kNameSpaceID_XHTML || !aOnPopup || tag != nsGkAtoms::option) {
    return PR_FALSE;
  }

  PRBool skipNavigatingDisabledMenuItem = PR_TRUE;
  if (aOnPopup) {
    aPresContext->LookAndFeel()->
      GetMetric(nsILookAndFeel::eMetric_SkipNavigatingDisabledMenuItem,
                skipNavigatingDisabledMenuItem);
  }

  return !(skipNavigatingDisabledMenuItem &&
           aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                                 nsGkAtoms::_true, eCaseMatters));
}

nsresult
nsXULPopupManager::KeyUp(nsIDOMEvent* aKeyEvent)
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
nsXULPopupManager::KeyDown(nsIDOMEvent* aKeyEvent)
{
  
  if (!mActiveMenuBar) {
    nsMenuChainItem* item = GetTopVisibleMenu();
    if (!item || item->PopupType() != ePopupTypeMenu)
      return NS_OK;
  }

  PRInt32 menuAccessKey = -1;

  
  

  nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
  if (menuAccessKey) {
    PRUint32 theChar;
    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
    keyEvent->GetKeyCode(&theChar);

    if (theChar == (PRUint32)menuAccessKey) {
      PRBool ctrl = PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_CONTROL)
        keyEvent->GetCtrlKey(&ctrl);
      PRBool alt=PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_ALT)
        keyEvent->GetAltKey(&alt);
      PRBool shift=PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_SHIFT)
        keyEvent->GetShiftKey(&shift);
      PRBool meta=PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_META)
        keyEvent->GetMetaKey(&meta);
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
nsXULPopupManager::KeyPress(nsIDOMEvent* aKeyEvent)
{
  
  
  

  
  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(aKeyEvent);
  PRBool trustedEvent = PR_FALSE;

  if (domNSEvent) {
    domNSEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  PRUint32 theChar;
  keyEvent->GetKeyCode(&theChar);

  
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item && item->PopupType() != ePopupTypeMenu) {
    if (theChar == NS_VK_ESCAPE) {
      HidePopup(item->Content(), PR_FALSE, PR_FALSE, PR_FALSE);
      aKeyEvent->StopPropagation();
      aKeyEvent->PreventDefault();
    }
    return NS_OK;
  }

  
  PRBool consume = (mPopups || mActiveMenuBar);

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
      HidePopup(item->Content(), PR_FALSE, PR_FALSE, PR_FALSE);
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
    if (item)
      menuToOpen = item->Frame()->Enter();
    else if (mActiveMenuBar)
      menuToOpen = mActiveMenuBar->Enter();
    if (menuToOpen) {
      nsCOMPtr<nsIContent> content = menuToOpen->GetContent();
      ShowMenu(content, PR_TRUE, PR_FALSE);
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

static nsPresContext*
GetPresContextFor(nsIContent* aContent)
{
  nsIDocument *document = aContent->GetCurrentDoc();
  if (document) {
    nsIPresShell* presShell = document->GetPrimaryShell();
    if (presShell)
      return presShell->GetPresContext();
  }

  return nsnull;
}

NS_IMETHODIMP
nsXULPopupShowingEvent::Run()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  nsPresContext* context = GetPresContextFor(mPopup);
  if (pm && context) {
    pm->FirePopupShowingEvent(mPopup, mMenu, context, mPopupType,
                              mIsContextMenu, mSelectFirstItem);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULPopupHidingEvent::Run()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  nsPresContext* context = GetPresContextFor(mPopup);
  if (pm && context) {
    pm->FirePopupHidingEvent(mPopup, mNextPopup, mLastPopup,
                             context, mPopupType, mDeselectMenu);
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
  if (menuFrame) {
    
    
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
      menuFrame->SelectMenu(PR_FALSE);

    nsAutoHandlingUserInputStatePusher userInpStatePusher(mUserInput);
    nsContentUtils::DispatchXULCommand(mMenu, mIsTrusted, nsnull, shell,
                                       mControl, mAlt, mShift, mMeta);
  }

  if (popup && mCloseMenuMode != CloseMenuMode_None)
    pm->HidePopup(popup, mCloseMenuMode == CloseMenuMode_Auto, PR_TRUE, PR_FALSE);

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
