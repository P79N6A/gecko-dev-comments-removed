



































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
#include "nsIPrivateDOMEvent.h"
#include "nsEventDispatcher.h"
#include "nsEventStateManager.h"
#include "nsCSSFrameConstructor.h"
#include "nsLayoutUtils.h"
#include "nsIViewManager.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsITimer.h"
#include "nsIFocusController.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShell.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIBaseWindow.h"


nsNavigationDirection DirectionFromKeyCode_lr_tb [6] = {
  eNavigationDirection_Last,   
  eNavigationDirection_First,  
  eNavigationDirection_Start,  
  eNavigationDirection_Before, 
  eNavigationDirection_End,    
  eNavigationDirection_After   
};

nsNavigationDirection DirectionFromKeyCode_rl_tb [6] = {
  eNavigationDirection_Last,   
  eNavigationDirection_First,  
  eNavigationDirection_End,    
  eNavigationDirection_Before, 
  eNavigationDirection_Start,  
  eNavigationDirection_After   
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

NS_IMPL_ISUPPORTS4(nsXULPopupManager, nsIDOMKeyListener,
                   nsIMenuRollup, nsIRollupListener, nsITimerCallback)

nsXULPopupManager::nsXULPopupManager() :
  mRangeOffset(0),
  mActiveMenuBar(nsnull),
  mCurrentMenu(nsnull),
  mPanels(nsnull),
  mTimerMenu(nsnull)
{
}

nsXULPopupManager::~nsXULPopupManager() 
{
  NS_ASSERTION(!mCurrentMenu && !mPanels, "XUL popups still open");
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
nsXULPopupManager::Rollup()
{
  if (mCurrentMenu)
    HidePopup(mCurrentMenu->Content(), PR_TRUE, PR_TRUE, PR_TRUE);
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

NS_IMETHODIMP
nsXULPopupManager::GetSubmenuWidgetChain(nsISupportsArray **_retval)
{
  
  
  
  nsresult rv = NS_NewISupportsArray(_retval);
  NS_ENSURE_SUCCESS(rv, rv);
  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item) {
    nsCOMPtr<nsIWidget> widget;
    item->Frame()->GetWidget(getter_AddRefs(widget));
    nsCOMPtr<nsISupports> genericWidget(do_QueryInterface(widget));
    (*_retval)->AppendElement(genericWidget);
    
    
    
    nsMenuChainItem* parent= item->GetParent();
    if (parent && item->Frame()->PopupType() != parent->Frame()->PopupType())
      break;
    item = parent;
  }

  return NS_OK;
}

nsIFrame*
nsXULPopupManager::GetFrameOfTypeForContent(nsIContent* aContent,
                                            nsIAtom* aFrameType)
{
  nsIDocument *document = aContent->GetCurrentDoc();
  if (document) {
    nsIPresShell* presShell = document->GetPrimaryShell();
    if (presShell) {
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
                    (GetFrameOfTypeForContent(aContent, nsGkAtoms::menuFrame));
}

nsMenuPopupFrame*
nsXULPopupManager::GetPopupFrameForContent(nsIContent* aContent)
{
  return static_cast<nsMenuPopupFrame *>
                    (GetFrameOfTypeForContent(aContent, nsGkAtoms::menuPopupFrame));
}

nsMenuChainItem*
nsXULPopupManager::GetTopVisibleMenu()
{
  nsMenuChainItem* item = mCurrentMenu;
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
nsXULPopupManager::SetMouseLocation(nsIDOMEvent* aEvent, nsIContent* aPopup)
{
  mCachedMousePoint = nsPoint(0, 0);

  nsCOMPtr<nsIDOMNSUIEvent> uiEvent = do_QueryInterface(aEvent);
  NS_ASSERTION(!aEvent || uiEvent, "Expected an nsIDOMNSUIEvent");
  if (uiEvent) {
    uiEvent->GetRangeParent(getter_AddRefs(mRangeParent));
    uiEvent->GetRangeOffset(&mRangeOffset);

    
    
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aEvent));
    if (privateEvent) {
      NS_ASSERTION(aPopup, "Expected a popup node");
      nsEvent* event;
      nsresult rv = privateEvent->GetInternalNSEvent(&event);
      if (NS_SUCCEEDED(rv) && event) {
        nsIDocument* doc = aPopup->GetCurrentDoc();
        if (doc) {
          nsIPresShell* presShell = doc->GetPrimaryShell();
          if (presShell) {
            nsPresContext* presContext = presShell->GetPresContext();
            nsIFrame* rootFrame = presShell->GetRootFrame();
            if (rootFrame && presContext) {
              nsPoint pnt =
                nsLayoutUtils::GetEventCoordinatesRelativeTo(event, rootFrame);
              mCachedMousePoint = nsPoint(presContext->AppUnitsToDevPixels(pnt.x),
                                          presContext->AppUnitsToDevPixels(pnt.y));
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
  nsMenuFrame* menuFrame = GetMenuFrameForContent(aMenu);
  if (!menuFrame || !menuFrame->IsMenu())
    return;

  nsMenuPopupFrame* popupFrame =  menuFrame->GetPopup();
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  
  PRBool parentIsContextMenu = PR_FALSE;
  PRBool onMenuBar = PR_FALSE;
  PRBool onmenu = menuFrame->IsOnMenu();

  nsIMenuParent* parent = menuFrame->GetMenuParent();
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
    SetMouseLocation(nsnull, nsnull);
    nsCOMPtr<nsIRunnable> event =
      new nsXULPopupShowingEvent(popupFrame->GetContent(), aMenu,
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
                             PRBool aSelectFirstItem)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  popupFrame->InitializePopup(aAnchorContent, aPosition, aXPos, aYPos,
                              aAttributesOverride);

  FirePopupShowingEvent(aPopup, nsnull, popupFrame->PresContext(),
                        popupFrame->PopupType(), aIsContextMenu, aSelectFirstItem);
}

void
nsXULPopupManager::ShowPopupAtScreen(nsIContent* aPopup,
                                     PRInt32 aXPos, PRInt32 aYPos,
                                     PRBool aIsContextMenu)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  popupFrame->InitializePopupAtScreen(aXPos, aYPos);

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

  popupFrame->InitializePopupWithAnchorAlign(aAnchorContent, aAnchor,
                                             aAlign, aXPos, aYPos);

  FirePopupShowingEvent(aPopup, nsnull, popupFrame->PresContext(),
                        popupFrame->PopupType(), aIsContextMenu, PR_FALSE);
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

  
  
  
  
  if (ismenu) {
    if (aPopup->AttrValueIs(kNameSpaceID_None, nsGkAtoms::ignorekeys,
                             nsGkAtoms::_true, eCaseMatters))
      item->SetIgnoreKeys(PR_TRUE);

    
    nsIFrame* parent = aPopupFrame->GetParent();
    if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
      nsMenuFrame* menuFrame = static_cast<nsMenuFrame *>(parent);
      item->SetOnMenuBar(menuFrame->IsOnMenuBar());
    }
  }

  
  nsWeakFrame weakFrame(aPopupFrame);
  PRBool hasChildren = aPopupFrame->ShowPopup(aIsContextMenu, aSelectFirstItem);
  ENSURE_TRUE(weakFrame.IsAlive());

  
  
  
  
  if (popupType == ePopupTypeTooltip ||
      (popupType == ePopupTypePanel &&
       aPopup->AttrValueIs(kNameSpaceID_None, nsGkAtoms::noautohide,
                            nsGkAtoms::_true, eIgnoreCase))) {
    item->SetParent(mPanels);
    mPanels = item;
  }
  else {
    nsIContent* oldmenu = nsnull;
    if (mCurrentMenu)
      oldmenu = mCurrentMenu->Content();
    item->SetParent(mCurrentMenu);
    mCurrentMenu = item;
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
}

void
nsXULPopupManager::HidePopup(nsIContent* aPopup,
                             PRBool aHideChain,
                             PRBool aDeselectMenu,
                             PRBool aAsynchronous)
{
  
  

  
  nsMenuPopupFrame* popupFrame = nsnull;
  PRBool foundPanel = PR_FALSE;
  nsMenuChainItem* item = mPanels;
  while (item) {
    if (item->Content() == aPopup) {
      foundPanel = PR_TRUE;
      popupFrame = item->Frame();
      break;
    }
    item = item->GetParent();
  }

  
  nsMenuChainItem* foundMenu = nsnull;
  item = mCurrentMenu;
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
    
    
    
    
    
    
    
    
    
    
    
    
    
    deselectMenu = aDeselectMenu;
    popupToHide = mCurrentMenu->Content();
    popupFrame = mCurrentMenu->Frame();
    type = popupFrame->PopupType();

    nsMenuChainItem* parent = mCurrentMenu->GetParent();

    
    
    if (parent && (aHideChain || mCurrentMenu != item))
      nextPopup = parent->Content();

    lastPopup = aHideChain ? nsnull : aPopup;
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
  if (mCloseTimer) {
    mCloseTimer->Cancel();
    mCloseTimer = nsnull;
    mTimerMenu = nsnull;
  }

  
  
  
  
  
  nsMenuChainItem* item = mPanels;
  while (item) {
    if (item->Content() == aPopup) {
      item->Detach(&mPanels);
      break;
    }
    item = item->GetParent();
  }

  if (!item) {
    item = mCurrentMenu;
    while (item) {
      if (item->Content() == aPopup) {
        item->Detach(&mCurrentMenu);
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
    nsMenuChainItem* item = mCurrentMenu;
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
nsXULPopupManager::HidePopupsInDocument(nsIDocument* aDocument)
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item) {
    nsMenuChainItem* parent = item->GetParent();
    if (item->Content()->GetOwnerDoc() == aDocument) {
      item->Frame()->HidePopup(PR_TRUE, ePopupInvisible);
      item->Detach(&mCurrentMenu);
      delete item;
    }
    item = parent;
  }

  item = mPanels;
  while (item) {
    nsMenuChainItem* parent = item->GetParent();
    if (item->Content()->GetOwnerDoc() == aDocument) {
      item->Frame()->HidePopup(PR_TRUE, ePopupInvisible);
      item->Detach(&mPanels);
      delete item;
    }
    item = parent;
  }

  SetCaptureState(nsnull);
}

void
nsXULPopupManager::ExecuteMenu(nsIContent* aMenu, nsEvent* aEvent)
{
  
  
  
  
  
  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item) {
    
    if (!item->IsMenu())
      break;
    nsMenuChainItem* next = item->GetParent();
    item->Frame()->HidePopup(PR_TRUE, ePopupInvisible);

    item = next;
  }

  SetCaptureState(nsnull);

  
  
  
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
    new nsXULMenuCommandEvent(aMenu, isTrusted, shift, control, alt, meta, userinput);
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

  
  
  if (aMenu)
    aMenu->SetAttr(kNameSpaceID_None, nsGkAtoms::open,
                   NS_LITERAL_STRING("true"), PR_TRUE);

  
  
  
  
  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWING, nsnull, nsMouseEvent::eReal);
  nsPoint pnt;
  event.widget = presShell->GetRootFrame()->
                            GetClosestView()->GetNearestWidget(&pnt);
  event.refPoint = mCachedMousePoint;
  nsEventDispatcher::Dispatch(aPopup, aPresContext, &event, nsnull, &status);
  mCachedMousePoint = nsPoint(0, 0);

  
  
  
  
  if (aPopupType == ePopupTypePanel &&
      !aPopup->AttrValueIs(kNameSpaceID_None, nsGkAtoms::noautofocus,
                           nsGkAtoms::_true, eCaseMatters)) {
    nsIEventStateManager* esm = presShell->GetPresContext()->EventStateManager();

    
    
    
    
    
    nsCOMPtr<nsIContent> currentFocus;
    esm->GetFocusedContent(getter_AddRefs(currentFocus));
    if (currentFocus &&
        !nsContentUtils::ContentIsDescendantOf(currentFocus, aPopup)) {
      esm->SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
      esm->SetFocusedContent(nsnull);
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
    nsIEventStateManager* esm = presShell->GetPresContext()->EventStateManager();

    
    nsCOMPtr<nsIContent> currentFocus;
    esm->GetFocusedContent(getter_AddRefs(currentFocus));
    if (currentFocus &&
        nsContentUtils::ContentIsDescendantOf(currentFocus, aPopup)) {
      esm->SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
      esm->SetFocusedContent(nsnull);
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
  
  
  
  nsMenuChainItem* item = mCurrentMenu;
  while (item) {
    if (item->Content() == aPopup) {
      NS_ASSERTION(item->Frame()->IsOpen() ||
                   item->Frame()->PopupState() == ePopupHiding ||
                   item->Frame()->PopupState() == ePopupInvisible,
                   "popup is open list not actually open");
      return PR_TRUE;
    }
    item = item->GetParent();
  }

  item = mPanels;
  while (item) {
    if (item->Content() == aPopup) {
      NS_ASSERTION(item->Frame()->IsOpen() ||
                   item->Frame()->PopupState() == ePopupHiding ||
                   item->Frame()->PopupState() == ePopupInvisible,
                   "popup is open list not actually open");
      return PR_TRUE;
    }
    item = item->GetParent();
  }

  return PR_FALSE;
}

PRBool
nsXULPopupManager::IsPopupOpenForMenuParent(nsIMenuParent* aMenuParent)
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
  if (aType == ePopupTypePanel && mPanels)
    return mPanels->Frame();

  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item) {
    if (item->PopupType() == aType)
      return item->Frame();
    item = item->GetParent();
  }

  return nsnull;
}

nsTArray<nsIFrame *>
nsXULPopupManager::GetOpenPopups()
{
  nsTArray<nsIFrame *> popups;

  nsMenuChainItem* item = mCurrentMenu;
  while (item) {
    if (item->Frame()->PopupState() != ePopupInvisible)
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

  nsCOMPtr<nsISupports> cont = aPopup->PresContext()->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(cont);
  if (!dsti)
    return PR_FALSE;

  
  
  PRInt32 type = -1;
  if (NS_FAILED(dsti->GetItemType(&type)))
    return PR_FALSE;

  if (type != nsIDocShellTreeItem::typeChrome) {
    nsCOMPtr<nsPIDOMWindow> win = do_GetInterface(dsti);
    if (!win)
      return PR_FALSE;

    
    PRBool active;
    nsIFocusController* focusController = win->GetRootFocusController();
    focusController->GetActive(&active);
    if (!active)
      return PR_FALSE;

    nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(dsti);
    if (!baseWin)
      return PR_FALSE;

    
    PRBool visible;
    baseWin->GetVisibility(&visible);
    if (!visible)
      return PR_FALSE;
  }

  
  nsIFrame* parent = aPopup->GetParent();
  if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
    nsMenuFrame* menuFrame = static_cast<nsMenuFrame *>(parent);
    nsIMenuParent* parentPopup = menuFrame->GetMenuParent();
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

  nsMenuChainItem* item = mPanels;
  while (item) {
    if (item->Frame() == aPopup) {
      item->Detach(&mPanels);
      delete item;
      break;
    }
    item = item->GetParent();
  }

  nsCOMPtr<nsIContent> oldMenu;
  if (mCurrentMenu)
    oldMenu = mCurrentMenu->Content();

  nsMenuChainItem* menuToDestroy = nsnull;
  item = mCurrentMenu;
  while (item) {
    if (item->Frame() == aPopup) {
      item->Detach(&mCurrentMenu);
      menuToDestroy = item;
      break;
    }
    item = item->GetParent();
  }

  if (menuToDestroy) {
    
    
    nsIFrame* menuToDestroyFrame = menuToDestroy->Frame();
    item = menuToDestroy->GetChild();
    while (item) {
      nsMenuChainItem* next = item->GetChild();

      
      
      
      
      
      if (nsLayoutUtils::IsProperAncestorFrame(menuToDestroyFrame, item->Frame())) {
        item->Detach(&mCurrentMenu);
        item->Frame()->HidePopup(PR_FALSE, ePopupInvisible);
      }
      else {
        HidePopup(item->Content(), PR_FALSE, PR_FALSE, PR_TRUE);
        break;
      }

      delete item;
      item = next;
    }

    delete menuToDestroy;
  }

  if (oldMenu)
    SetCaptureState(oldMenu);
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
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item) {
    if (!item->IgnoreKeys())
      newTarget = do_QueryInterface(item->Content()->GetDocument());
  }
  else if (mActiveMenuBar) {
    newTarget = do_QueryInterface(mActiveMenuBar->GetContent()->GetDocument());
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
      nsContentUtils::NotifyInstalledMenuKeyboardListener(PR_TRUE);
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
nsXULPopupManager::CancelMenuTimer(nsIMenuParent* aMenuParent)
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

      
      
      
      nsIMenuParent* expectedParent = static_cast<nsIMenuParent *>(nextitem->Frame());
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
  NS_DIRECTION_FROM_KEY_CODE(itemFrame, theDirection, aKeyCode);

  
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
    else if NS_DIRECTION_IS_BLOCK(theDirection) {
      
      nsCOMPtr<nsIContent> content = currentMenu->GetContent();
      ShowMenu(content, PR_TRUE, PR_FALSE);
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
  if (ns == kNameSpaceID_XUL &&
       tag != nsGkAtoms::menu &&
       tag != nsGkAtoms::menuitem)
    return PR_FALSE;

  if (ns == kNameSpaceID_XHTML && (!aOnPopup || tag != nsGkAtoms::option))
    return PR_FALSE;

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
  aKeyEvent->StopPropagation();
  aKeyEvent->PreventDefault();

  return NS_OK; 
}

nsresult
nsXULPopupManager::KeyDown(nsIDOMEvent* aKeyEvent)
{
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
        
        
        Rollup();
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

  if (theChar == NS_VK_LEFT ||
      theChar == NS_VK_RIGHT ||
      theChar == NS_VK_UP ||
      theChar == NS_VK_DOWN ||
      theChar == NS_VK_HOME ||
      theChar == NS_VK_END) {
    HandleKeyboardNavigation(theChar);
  }
  else if (theChar == NS_VK_ESCAPE) {
    
    
    
    
    nsMenuChainItem* item = GetTopVisibleMenu();
    if (item)
      HidePopup(item->Content(), PR_FALSE, PR_FALSE, PR_FALSE);
    else if (mActiveMenuBar)
      mActiveMenuBar->MenuClosed();
  }
  else if (theChar == NS_VK_TAB) {
    if (mCurrentMenu)
      Rollup();
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
#if !defined(XP_MAC) && !defined(XP_MACOSX)
  else if (theChar == NS_VK_F10) {
    
    
    Rollup();
  }
#endif 
  else {
    HandleShortcutNavigation(keyEvent, nsnull);
  }

  aKeyEvent->StopPropagation();
  aKeyEvent->PreventDefault();
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
    
    
    pm->FirePopupShowingEvent(mPopup, mMenu, context, ePopupTypeMenu,
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

  
  
  
  
  
  nsMenuFrame* menuFrame = pm->GetMenuFrameForContent(mMenu);
  if (menuFrame) {
    nsPresContext* presContext = menuFrame->PresContext();
    nsCOMPtr<nsIViewManager> kungFuDeathGrip = presContext->GetViewManager();
    nsCOMPtr<nsIPresShell> shell = presContext->PresShell();

    
    menuFrame->SelectMenu(PR_FALSE);

    nsAutoHandlingUserInputStatePusher userInpStatePusher(mUserInput);

    nsEventStatus status = nsEventStatus_eIgnore;
    nsXULCommandEvent commandEvent(mIsTrusted, NS_XUL_COMMAND, nsnull);
    commandEvent.isShift = mShift;
    commandEvent.isControl = mControl;
    commandEvent.isAlt = mAlt;
    commandEvent.isMeta = mMeta;
    shell->HandleDOMEventWithTarget(mMenu, &commandEvent, &status);
  }

  pm->Rollup();

  return NS_OK;
}
