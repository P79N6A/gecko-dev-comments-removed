




#include "nsGkAtoms.h"
#include "nsXULPopupManager.h"
#include "nsMenuFrame.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIPopupBoxObject.h"
#include "nsMenuBarListener.h"
#include "nsContentUtils.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMXULElement.h"
#include "nsIXULDocument.h"
#include "nsIXULTemplateBuilder.h"
#include "nsCSSFrameConstructor.h"
#include "nsLayoutUtils.h"
#include "nsViewManager.h"
#include "nsIComponentManager.h"
#include "nsITimer.h"
#include "nsFocusManager.h"
#include "nsIDocShell.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIBaseWindow.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsCaret.h"
#include "nsIDocument.h"
#include "nsPIWindowRoot.h"
#include "nsFrameManager.h"
#include "nsIObserverService.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Event.h" 
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Services.h"

using namespace mozilla;
using namespace mozilla::dom;

static_assert(nsIDOMKeyEvent::DOM_VK_HOME  == nsIDOMKeyEvent::DOM_VK_END + 1 &&
              nsIDOMKeyEvent::DOM_VK_LEFT  == nsIDOMKeyEvent::DOM_VK_END + 2 &&
              nsIDOMKeyEvent::DOM_VK_UP    == nsIDOMKeyEvent::DOM_VK_END + 3 &&
              nsIDOMKeyEvent::DOM_VK_RIGHT == nsIDOMKeyEvent::DOM_VK_END + 4 &&
              nsIDOMKeyEvent::DOM_VK_DOWN  == nsIDOMKeyEvent::DOM_VK_END + 5,
              "nsXULPopupManager assumes some keyCode values are consecutive");

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

nsXULPopupManager* nsXULPopupManager::sInstance = nullptr;

nsIContent* nsMenuChainItem::Content()
{
  return mFrame->GetContent();
}

void nsMenuChainItem::SetParent(nsMenuChainItem* aParent)
{
  if (mParent) {
    NS_ASSERTION(mParent->mChild == this, "Unexpected - parent's child not set to this");
    mParent->mChild = nullptr;
  }
  mParent = aParent;
  if (mParent) {
    if (mParent->mChild)
      mParent->mChild->mParent = nullptr;
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
    SetParent(nullptr);
  }
}

NS_IMPL_ISUPPORTS(nsXULPopupManager,
                  nsIDOMEventListener,
                  nsITimerCallback,
                  nsIObserver)

nsXULPopupManager::nsXULPopupManager() :
  mRangeOffset(0),
  mCachedMousePoint(0, 0),
  mCachedModifiers(0),
  mActiveMenuBar(nullptr),
  mPopups(nullptr),
  mNoHidePanels(nullptr),
  mTimerMenu(nullptr)
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
                           const char16_t *aData)
{
  if (!nsCRT::strcmp(aTopic, "xpcom-shutdown")) {
    if (mKeyListener) {
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keypress"), this, true);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keydown"), this, true);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keyup"), this, true);
      mKeyListener = nullptr;
    }
    mRangeParent = nullptr;
    
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
  MOZ_ASSERT(sInstance);
  return sInstance;
}

bool
nsXULPopupManager::Rollup(uint32_t aCount, const nsIntPoint* pos, nsIContent** aLastRolledUp)
{
  bool consume = false;

  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item) {
    if (aLastRolledUp) {
      
      
      
      
      
      
      
      
      nsMenuChainItem* first = item;
      while (first->GetParent())
        first = first->GetParent();
      *aLastRolledUp = first->Content();
    }

    consume = item->Frame()->ConsumeOutsideClicks();
    
    
    if (!consume && pos) {
      nsCOMPtr<nsIContent> anchor = item->Frame()->GetAnchor();

      
      
      
      
      if (anchor) {
        nsAutoString consumeAnchor;
        anchor->GetAttr(kNameSpaceID_None, nsGkAtoms::consumeanchor,
                        consumeAnchor);
        if (!consumeAnchor.IsEmpty()) {
          nsIDocument* doc = anchor->GetOwnerDocument();
          nsIContent* newAnchor = doc->GetElementById(consumeAnchor);
          if (newAnchor) {
            anchor = newAnchor;
          }
        }
      }

      if (anchor && anchor->GetPrimaryFrame()) {
        
        
        
        
        if (anchor->GetPrimaryFrame()->GetScreenRect().Contains(*pos)) {
          consume = true;
        }
      }
    }

    
    
    nsIContent* lastPopup = nullptr;
    if (aCount != UINT32_MAX) {
      nsMenuChainItem* last = item;
      while (--aCount && last->GetParent()) {
        last = last->GetParent();
      }
      if (last) {
        lastPopup = last->Content();
      }
    }

    HidePopup(item->Content(), true, true, false, true, lastPopup);
  }

  return consume;
}


bool nsXULPopupManager::ShouldRollupOnMouseWheelEvent()
{
  
  

  nsMenuChainItem* item = GetTopVisibleMenu();
  if (!item)
    return false;

  nsIContent* content = item->Frame()->GetContent();
  if (!content)
    return false;

  if (content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::rolluponmousewheel,
                           nsGkAtoms::_true, eCaseMatters))
    return true;

  if (content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::rolluponmousewheel,
                           nsGkAtoms::_false, eCaseMatters))
    return false;

  nsAutoString value;
  content->GetAttr(kNameSpaceID_None, nsGkAtoms::type, value);
  return StringBeginsWith(value, NS_LITERAL_STRING("autocomplete"));
}

bool nsXULPopupManager::ShouldConsumeOnMouseWheelEvent()
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (!item)
    return false;

  nsMenuPopupFrame* frame = item->Frame();
  if (frame->PopupType() != ePopupTypePanel)
    return true;

  nsIContent* content = frame->GetContent();
  return !(content && content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                           nsGkAtoms::arrow, eCaseMatters));
}


bool nsXULPopupManager::ShouldRollupOnMouseActivate()
{
  return false;
}

uint32_t
nsXULPopupManager::GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain)
{
  
  
  
  uint32_t count = 0, sameTypeCount = 0;

  NS_ASSERTION(aWidgetChain, "null parameter");
  nsMenuChainItem* item = GetTopVisibleMenu();
  while (item) {
    nsCOMPtr<nsIWidget> widget = item->Frame()->GetWidget();
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

nsIWidget*
nsXULPopupManager::GetRollupWidget()
{
  nsMenuChainItem* item = GetTopVisibleMenu();
  return item ? item->Frame()->GetWidget() : nullptr;
}

void
nsXULPopupManager::AdjustPopupsOnWindowChange(nsPIDOMWindow* aWindow)
{
  
  
  

  
  nsTArray<nsMenuPopupFrame *> list;

  nsMenuChainItem* item = mNoHidePanels;
  while (item) {
    
    
    nsMenuPopupFrame* frame = item->Frame();
    if (frame->GetAutoPosition()) {
      nsIContent* popup = frame->GetContent();
      if (popup) {
        nsIDocument* document = popup->GetCurrentDoc();
        if (document) {
          nsPIDOMWindow* window = document->GetWindow();
          if (window) {
            window = window->GetPrivateRoot();
            if (window == aWindow) {
              list.AppendElement(frame);
            }
          }
        }
      }
    }

    item = item->GetParent();
  }

  for (int32_t l = list.Length() - 1; l >= 0; l--) {
    list[l]->SetPopupPosition(nullptr, true, false);
  }
}

void nsXULPopupManager::AdjustPopupsOnWindowChange(nsIPresShell* aPresShell)
{
  if (aPresShell->GetDocument()) {
    AdjustPopupsOnWindowChange(aPresShell->GetDocument()->GetWindow());
  }
}

static
nsMenuPopupFrame* GetPopupToMoveOrResize(nsIFrame* aFrame)
{
  nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(aFrame);
  if (!menuPopupFrame)
    return nullptr;

  
  if (!menuPopupFrame->IsVisible())
    return nullptr;

  nsIWidget* widget = menuPopupFrame->GetWidget();
  if (widget && !widget->IsVisible())
    return nullptr;

  return menuPopupFrame;
}

void
nsXULPopupManager::PopupMoved(nsIFrame* aFrame, nsIntPoint aPnt)
{
  nsMenuPopupFrame* menuPopupFrame = GetPopupToMoveOrResize(aFrame);
  if (!menuPopupFrame)
    return;

  nsView* view = menuPopupFrame->GetView();
  if (!view)
    return;

  
  
  nsIntRect curDevSize = view->CalcWidgetBounds(eWindowType_popup);
  nsIWidget* widget = menuPopupFrame->GetWidget();
  if (curDevSize.x == aPnt.x && curDevSize.y == aPnt.y &&
      (!widget || widget->GetClientOffset() == menuPopupFrame->GetLastClientOffset())) {
    return;
  }

  
  
  
  
  if (menuPopupFrame->IsAnchored() &&
      menuPopupFrame->PopupLevel() == ePopupLevelParent) {
    menuPopupFrame->SetPopupPosition(nullptr, true, false);
  }
  else {
    nsPresContext* presContext = menuPopupFrame->PresContext();
    aPnt.x = presContext->DevPixelsToIntCSSPixels(aPnt.x);
    aPnt.y = presContext->DevPixelsToIntCSSPixels(aPnt.y);
    menuPopupFrame->MoveTo(aPnt.x, aPnt.y, false);
  }
}

void
nsXULPopupManager::PopupResized(nsIFrame* aFrame, nsIntSize aSize)
{
  nsMenuPopupFrame* menuPopupFrame = GetPopupToMoveOrResize(aFrame);
  if (!menuPopupFrame)
    return;

  nsView* view = menuPopupFrame->GetView();
  if (!view)
    return;

  nsIntRect curDevSize = view->CalcWidgetBounds(eWindowType_popup);
  
  if (curDevSize.width == aSize.width && curDevSize.height == aSize.height)
    return;

  
  
  nsPresContext* presContext = menuPopupFrame->PresContext();

  nsIntSize newCSS(presContext->DevPixelsToIntCSSPixels(aSize.width),
                   presContext->DevPixelsToIntCSSPixels(aSize.height));

  nsIContent* popup = menuPopupFrame->GetContent();
  nsAutoString width, height;
  width.AppendInt(newCSS.width);
  height.AppendInt(newCSS.height);
  popup->SetAttr(kNameSpaceID_None, nsGkAtoms::width, width, false);
  popup->SetAttr(kNameSpaceID_None, nsGkAtoms::height, height, true);
}

nsMenuPopupFrame*
nsXULPopupManager::GetPopupFrameForContent(nsIContent* aContent, bool aShouldFlush)
{
  if (aShouldFlush) {
    nsIDocument *document = aContent->GetCurrentDoc();
    if (document) {
      nsCOMPtr<nsIPresShell> presShell = document->GetShell();
      if (presShell)
        presShell->FlushPendingNotifications(Flush_Layout);
    }
  }

  return do_QueryFrame(aContent->GetPrimaryFrame());
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
nsXULPopupManager::GetMouseLocation(nsIDOMNode** aNode, int32_t* aOffset)
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
    *aTriggerContent = nullptr;
    if (aEvent) {
      
      nsCOMPtr<nsIContent> target = do_QueryInterface(
        aEvent->InternalDOMEvent()->GetTarget());
      target.forget(aTriggerContent);
    }
  }

  mCachedModifiers = 0;

  nsCOMPtr<nsIDOMUIEvent> uiEvent = do_QueryInterface(aEvent);
  if (uiEvent) {
    uiEvent->GetRangeParent(getter_AddRefs(mRangeParent));
    uiEvent->GetRangeOffset(&mRangeOffset);

    
    
    NS_ASSERTION(aPopup, "Expected a popup node");
    WidgetEvent* event = aEvent->GetInternalNSEvent();
    if (event) {
      WidgetInputEvent* inputEvent = event->AsInputEvent();
      if (inputEvent) {
        mCachedModifiers = inputEvent->modifiers;
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
          if ((event->mClass == eMouseEventClass || 
               event->mClass == eMouseScrollEventClass ||
               event->mClass == eWheelEventClass) &&
               !event->AsGUIEvent()->widget) {
            
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
  else {
    mRangeParent = nullptr;
    mRangeOffset = 0;
  }
}

void
nsXULPopupManager::SetActiveMenuBar(nsMenuBarFrame* aMenuBar, bool aActivate)
{
  if (aActivate)
    mActiveMenuBar = aMenuBar;
  else if (mActiveMenuBar == aMenuBar)
    mActiveMenuBar = nullptr;

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

  nsMenuFrame* menuFrame = do_QueryFrame(aMenu->GetPrimaryFrame());
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

  
  InitTriggerEvent(nullptr, nullptr, nullptr);
  popupFrame->InitializePopup(menuFrame->GetAnchor(), nullptr, position, 0, 0, true);

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
                             int32_t aXPos, int32_t aYPos,
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
                                     int32_t aXPos, int32_t aYPos,
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
                                       int32_t aXPos, int32_t aYPos)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup, true);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  InitTriggerEvent(nullptr, nullptr, nullptr);

  nsPresContext* pc = popupFrame->PresContext();
  mCachedMousePoint = nsIntPoint(pc->CSSPixelsToDevPixels(aXPos),
                                 pc->CSSPixelsToDevPixels(aYPos));

  
  nsPresContext* rootPresContext = pc->GetRootPresContext();
  if (rootPresContext) {
    nsIWidget *rootWidget = rootPresContext->GetRootWidget();
    if (rootWidget) {
      mCachedMousePoint -= rootWidget->WidgetToScreenOffset();
    }
  }

  popupFrame->InitializePopupAtScreen(aTriggerContent, aXPos, aYPos, false);

  FirePopupShowingEvent(aPopup, false, false);
}

void
nsXULPopupManager::ShowPopupWithAnchorAlign(nsIContent* aPopup,
                                            nsIContent* aAnchorContent,
                                            nsAString& aAnchor,
                                            nsAString& aAlign,
                                            int32_t aXPos, int32_t aYPos,
                                            bool aIsContextMenu)
{
  nsMenuPopupFrame* popupFrame = GetPopupFrameForContent(aPopup, true);
  if (!popupFrame || !MayShowPopup(popupFrame))
    return;

  InitTriggerEvent(nullptr, nullptr, nullptr);

  popupFrame->InitializePopupWithAnchorAlign(aAnchorContent, aAnchor,
                                             aAlign, aXPos, aYPos);
  FirePopupShowingEvent(aPopup, aIsContextMenu, false);
}

static void
CheckCaretDrawingState()
{
  
  
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
    caret->SchedulePaint();
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
    
    nsMenuFrame* menuFrame = do_QueryFrame(aPopupFrame->GetParent());
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
    nsIContent* oldmenu = nullptr;
    if (mPopups)
      oldmenu = mPopups->Content();
    item->SetParent(mPopups);
    mPopups = item;
    SetCaptureState(oldmenu);
  }

  if (aSelectFirstItem) {
    nsMenuFrame* next = GetNextMenuItem(aPopupFrame, nullptr, true);
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
                             bool aIsCancel,
                             nsIContent* aLastPopup)
{
  
  
  nsMenuPopupFrame* popupFrame = nullptr;
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

  
  nsMenuChainItem* foundMenu = nullptr;
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

    lastPopup = aLastPopup ? aLastPopup : (aHideChain ? nullptr : aPopup);
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
                                  type, deselectMenu, aIsCancel);
        NS_DispatchToCurrentThread(event);
    }
    else {
      FirePopupHidingEvent(popupToHide, nextPopup, lastPopup,
                           popupFrame->PresContext(), type, deselectMenu, aIsCancel);
    }
  }
}


class TransitionEnder : public nsIDOMEventListener
{
protected:
  virtual ~TransitionEnder() { }

public:

  nsCOMPtr<nsIContent> mContent;
  bool mDeselectMenu;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(TransitionEnder)

  TransitionEnder(nsIContent* aContent, bool aDeselectMenu)
    : mContent(aContent), mDeselectMenu(aDeselectMenu)
  {
  }

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) MOZ_OVERRIDE
  {
    mContent->RemoveSystemEventListener(NS_LITERAL_STRING("transitionend"), this, false);

    nsMenuPopupFrame* popupFrame = do_QueryFrame(mContent->GetPrimaryFrame());

    
    
    
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm && popupFrame) {
      pm->HidePopupCallback(mContent, popupFrame, nullptr, nullptr,
                            popupFrame->PopupType(), mDeselectMenu);
    }

    return NS_OK;
  }
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(TransitionEnder)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TransitionEnder)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TransitionEnder)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION(TransitionEnder, mContent);

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
    mCloseTimer = nullptr;
    mTimerMenu = nullptr;
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
  WidgetMouseEvent event(true, NS_XUL_POPUP_HIDDEN, nullptr,
                         WidgetMouseEvent::eReal);
  EventDispatcher::Dispatch(aPopup, aPopupFrame->PresContext(),
                            &event, nullptr, &status);
  ENSURE_TRUE(weakFrame.IsAlive());

  
  if (aNextPopup && aPopup != aLastPopup) {
    nsMenuChainItem* foundMenu = nullptr;
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
                           foundMenu->PopupType(), aDeselectMenu, false);
    }
  }
}

void
nsXULPopupManager::HidePopup(nsIFrame* aFrame)
{
  nsMenuPopupFrame* popup = do_QueryFrame(aFrame);
  if (popup)
    HidePopup(aFrame->GetContent(), false, true, false, false);
}

void
nsXULPopupManager::HidePopupAfterDelay(nsMenuPopupFrame* aPopup)
{
  
  
  KillMenuTimer();

  int32_t menuDelay =
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
  uint32_t f;
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

  SetCaptureState(nullptr);
}

bool
nsXULPopupManager::IsChildOfDocShell(nsIDocument* aDoc, nsIDocShellTreeItem* aExpected)
{
  nsCOMPtr<nsIDocShellTreeItem> docShellItem(aDoc->GetDocShell());
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
    {&nsGkAtoms::none, &nsGkAtoms::single, nullptr};

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

  nsMenuPopupFrame* popupFrame = do_QueryFrame(aPopup->GetPrimaryFrame());
  if (!popupFrame)
    return;

  nsPresContext *presContext = popupFrame->PresContext();
  nsCOMPtr<nsIPresShell> presShell = presContext->PresShell();
  nsPopupType popupType = popupFrame->PopupType();

  
  if (!popupFrame->HasGeneratedChildren()) {
    popupFrame->SetGeneratedChildren();
    presShell->FrameConstructor()->GenerateChildFrames(popupFrame);
  }

  
  nsIFrame* frame = aPopup->GetPrimaryFrame();
  if (!frame)
    return;

  presShell->FrameNeedsReflow(frame, nsIPresShell::eTreeChange,
                              NS_FRAME_HAS_DIRTY_CHILDREN);

  
  
  
  mOpeningPopup = aPopup;

  nsEventStatus status = nsEventStatus_eIgnore;
  WidgetMouseEvent event(true, NS_XUL_POPUP_SHOWING, nullptr,
                         WidgetMouseEvent::eReal);

  
  nsPresContext* rootPresContext =
    presShell->GetPresContext()->GetRootPresContext();
  if (rootPresContext) {
    rootPresContext->PresShell()->GetViewManager()->
      GetRootWidget(getter_AddRefs(event.widget));
  }
  else {
    event.widget = nullptr;
  }

  event.refPoint = LayoutDeviceIntPoint::FromUntyped(mCachedMousePoint);
  event.modifiers = mCachedModifiers;
  EventDispatcher::Dispatch(popup, presContext, &event, nullptr, &status);

  mCachedMousePoint = nsIntPoint(0, 0);
  mOpeningPopup = nullptr;

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

  
  mRangeParent = nullptr;
  mRangeOffset = 0;

  
  popupFrame = do_QueryFrame(aPopup->GetPrimaryFrame());
  if (popupFrame) {
    
    
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
                                        bool aDeselectMenu,
                                        bool aIsCancel)
{
  nsCOMPtr<nsIPresShell> presShell = aPresContext->PresShell();

  nsEventStatus status = nsEventStatus_eIgnore;
  WidgetMouseEvent event(true, NS_XUL_POPUP_HIDING, nullptr,
                         WidgetMouseEvent::eReal);
  EventDispatcher::Dispatch(aPopup, aPresContext, &event, nullptr, &status);

  
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

  
  nsMenuPopupFrame* popupFrame = do_QueryFrame(aPopup->GetPrimaryFrame());
  if (popupFrame) {
    
    
    
    if (status == nsEventStatus_eConsumeNoDefault &&
        !popupFrame->IsInContentShell()) {
      
      
      
      
      popupFrame->SetPopupState(ePopupShown);
    }
    else {
      
      
      
      
      
      
      
#ifndef MOZ_WIDGET_GTK
      if (!aNextPopup && aPopup->HasAttr(kNameSpaceID_None, nsGkAtoms::animate)) {
        
        
        
        nsAutoString animate;
        aPopup->GetAttr(kNameSpaceID_None, nsGkAtoms::animate, animate);

        if (!animate.EqualsLiteral("false") &&
            (!animate.EqualsLiteral("cancel") || aIsCancel)) {
          presShell->FlushPendingNotifications(Flush_Layout);

          
          popupFrame = do_QueryFrame(aPopup->GetPrimaryFrame());
          if (!popupFrame)
            return;

          if (nsLayoutUtils::HasCurrentAnimations(aPopup, nsGkAtoms::transitionsProperty, aPresContext)) {
            nsRefPtr<TransitionEnder> ender = new TransitionEnder(aPopup, aDeselectMenu);
            aPopup->AddSystemEventListener(NS_LITERAL_STRING("transitionend"),
                                           ender, false, false);
            return;
          }
        }
      }
#endif

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
      nsMenuFrame* menuFrame = do_QueryFrame(popup->GetParent());
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

  return nullptr;
}

void
nsXULPopupManager::GetVisiblePopups(nsTArray<nsIFrame *>& aPopups)
{
  aPopups.Clear();

  
  nsMenuChainItem* item = mPopups;
  for (int32_t list = 0; list < 2; list++) {
    while (item) {
      
      
      if (item->Frame()->IsVisible() && !item->Frame()->IsMouseTransparent()) {
        aPopups.AppendElement(item->Frame());
      }

      item = item->GetParent();
    }

    item = mNoHidePanels;
  }
}

already_AddRefed<nsIDOMNode>
nsXULPopupManager::GetLastTriggerNode(nsIDocument* aDocument, bool aIsTooltip)
{
  if (!aDocument)
    return nullptr;

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

  
  nsCOMPtr<nsIWidget> widget = aPopup->GetWidget();
  if (widget && widget->GetLastRollup() == aPopup->GetContent())
      return false;

  nsCOMPtr<nsIDocShellTreeItem> dsti = aPopup->PresContext()->GetDocShell();
  nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(dsti);
  if (!baseWin)
    return false;

  
  
  if (dsti->ItemType() != nsIDocShellTreeItem::typeChrome) {
    
    nsCOMPtr<nsIDocShellTreeItem> root;
    dsti->GetRootTreeItem(getter_AddRefs(root));
    if (!root) {
      return false;
    }

    nsCOMPtr<nsIDOMWindow> rootWin = root->GetWindow();

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
  if (mainWidget && mainWidget->SizeMode() == nsSizeMode_Minimized) {
    return false;
  }

  
  nsMenuFrame* menuFrame = do_QueryFrame(aPopup->GetParent());
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
      mCloseTimer = nullptr;
    }
    mTimerMenu = nullptr;
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
            
            
            HidePopup(child->Content(), false, false, true, false);
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
    mWidget->CaptureRollupEvents(nullptr, false);
    mWidget = nullptr;
  }

  if (item) {
    nsMenuPopupFrame* popup = item->Frame();
    mWidget = popup->GetWidget();
    if (mWidget) {
      mWidget->CaptureRollupEvents(nullptr, true);
      popup->AttachedDismissalListener();
    }
  }

  UpdateKeyboardListeners();
}

void
nsXULPopupManager::UpdateKeyboardListeners()
{
  nsCOMPtr<EventTarget> newTarget;
  bool isForMenu = false;
  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item) {
    if (!item->IgnoreKeys())
      newTarget = item->Content()->GetComposedDoc();
    isForMenu = item->PopupType() == ePopupTypeMenu;
  }
  else if (mActiveMenuBar) {
    newTarget = mActiveMenuBar->GetContent()->GetComposedDoc();
    isForMenu = true;
  }

  if (mKeyListener != newTarget) {
    if (mKeyListener) {
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keypress"), this, true);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keydown"), this, true);
      mKeyListener->RemoveEventListener(NS_LITERAL_STRING("keyup"), this, true);
      mKeyListener = nullptr;
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
  
  

  nsCOMPtr<nsIDocument> document = aPopup->GetCurrentDoc();
  if (!document) {
    return;
  }

  for (nsCOMPtr<nsIContent> grandChild = aPopup->GetFirstChild();
       grandChild;
       grandChild = grandChild->GetNextSibling()) {
    if (grandChild->IsXUL(nsGkAtoms::menugroup)) {
      if (grandChild->GetChildCount() == 0) {
        continue;
      }
      grandChild = grandChild->GetFirstChild();
    }
    if (grandChild->IsXUL(nsGkAtoms::menuitem)) {
      
      nsAutoString command;
      grandChild->GetAttr(kNameSpaceID_None, nsGkAtoms::command, command);
      if (!command.IsEmpty()) {
        
        nsRefPtr<dom::Element> commandElement =
          document->GetElementById(command);
        if (commandElement) {
          nsAutoString commandValue;
          
          if (commandElement->GetAttr(kNameSpaceID_None, nsGkAtoms::disabled, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, commandValue, true);
          else
            grandChild->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, true);

          
          
          
          if (commandElement->GetAttr(kNameSpaceID_None, nsGkAtoms::label, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::label, commandValue, true);

          if (commandElement->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandValue, true);

          if (commandElement->GetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandValue, true);

          if (commandElement->GetAttr(kNameSpaceID_None, nsGkAtoms::hidden, commandValue))
            grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::hidden, commandValue, true);
        }
      }
    }
    if (!grandChild->GetNextSibling() &&
        grandChild->GetParent()->IsXUL(nsGkAtoms::menugroup)) {
      grandChild = grandChild->GetParent();
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
    mCloseTimer = nullptr;

    if (mTimerMenu->IsOpen())
      HidePopup(mTimerMenu->GetContent(), false, false, true, false);
  }

  mTimerMenu = nullptr;
}

void
nsXULPopupManager::CancelMenuTimer(nsMenuParent* aMenuParent)
{
  if (mCloseTimer && mTimerMenu == aMenuParent) {
    mCloseTimer->Cancel();
    mCloseTimer = nullptr;
    mTimerMenu = nullptr;
  }
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
        WidgetGUIEvent* evt = aKeyEvent->GetInternalNSEvent()->AsGUIEvent();
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
nsXULPopupManager::HandleKeyboardNavigation(uint32_t aKeyCode)
{
  
  
  nsMenuChainItem* item = nullptr;
  nsMenuChainItem* nextitem = GetTopVisibleMenu();

  while (nextitem) {
    item = nextitem;
    nextitem = item->GetParent();

    if (nextitem) {
      
      if (!nextitem->IsMenu())
        break;

      
      
      
      nsMenuParent* expectedParent = static_cast<nsMenuParent *>(nextitem->Frame());
      nsMenuFrame* menuFrame = do_QueryFrame(item->Frame()->GetParent());
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
  NS_ASSERTION(aKeyCode >= nsIDOMKeyEvent::DOM_VK_END &&
                 aKeyCode <= nsIDOMKeyEvent::DOM_VK_DOWN, "Illegal key code");
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
      nsMenuFrame* nextItem = GetNextMenuItem(aFrame, nullptr, true);
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
      
      nsMenuChainItem* child = item ? item->GetChild() : nullptr;
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
      nextItem = GetNextMenuItem(aFrame, nullptr, true);
    else
      nextItem = GetPreviousMenuItem(aFrame, nullptr, true);

    if (nextItem) {
      aFrame->ChangeMenuItem(nextItem, false);
      return true;
    }
  }
  else if (currentMenu && isContainer && isOpen) {
    if (aDir == eNavigationDirection_Start) {
      
      nsMenuPopupFrame* popupFrame = currentMenu->GetPopup();
      if (popupFrame)
        HidePopup(popupFrame->GetContent(), false, false, false, false);
      return true;
    }
  }

  return false;
}

bool
nsXULPopupManager::HandleKeyboardEventWithKeyCode(
                        nsIDOMKeyEvent* aKeyEvent,
                        nsMenuChainItem* aTopVisibleMenuItem)
{
  uint32_t keyCode;
  aKeyEvent->GetKeyCode(&keyCode);

  
  if (aTopVisibleMenuItem &&
      aTopVisibleMenuItem->PopupType() != ePopupTypeMenu) {
    if (keyCode == nsIDOMKeyEvent::DOM_VK_ESCAPE) {
      HidePopup(aTopVisibleMenuItem->Content(), false, false, false, true);
      aKeyEvent->StopPropagation();
      aKeyEvent->PreventDefault();
    }
    return true;
  }

  bool consume = (mPopups || mActiveMenuBar);
  switch (keyCode) {
    case nsIDOMKeyEvent::DOM_VK_LEFT:
    case nsIDOMKeyEvent::DOM_VK_RIGHT:
    case nsIDOMKeyEvent::DOM_VK_UP:
    case nsIDOMKeyEvent::DOM_VK_DOWN:
    case nsIDOMKeyEvent::DOM_VK_HOME:
    case nsIDOMKeyEvent::DOM_VK_END:
      HandleKeyboardNavigation(keyCode);
      break;

    case nsIDOMKeyEvent::DOM_VK_ESCAPE:
      
      
      
      
      if (aTopVisibleMenuItem) {
        HidePopup(aTopVisibleMenuItem->Content(), false, false, false, true);
      } else if (mActiveMenuBar) {
        mActiveMenuBar->MenuClosed();
      }
      break;

    case nsIDOMKeyEvent::DOM_VK_TAB:
#ifndef XP_MACOSX
    case nsIDOMKeyEvent::DOM_VK_F10:
#endif
      
      if (aTopVisibleMenuItem) {
        Rollup(0, nullptr, nullptr);
      } else if (mActiveMenuBar) {
        mActiveMenuBar->MenuClosed();
      }
      break;

    case nsIDOMKeyEvent::DOM_VK_RETURN: {
      
      
      
      nsMenuFrame* menuToOpen = nullptr;
      WidgetGUIEvent* GUIEvent = aKeyEvent->GetInternalNSEvent()->AsGUIEvent();
      if (aTopVisibleMenuItem) {
        menuToOpen = aTopVisibleMenuItem->Frame()->Enter(GUIEvent);
      } else if (mActiveMenuBar) {
        menuToOpen = mActiveMenuBar->Enter(GUIEvent);
      }
      if (menuToOpen) {
        nsCOMPtr<nsIContent> content = menuToOpen->GetContent();
        ShowMenu(content, true, false);
      }
      break;
    }

    default:
      return false;
  }

  if (consume) {
    aKeyEvent->StopPropagation();
    aKeyEvent->PreventDefault();
  }
  return true;
}

nsMenuFrame*
nsXULPopupManager::GetNextMenuItem(nsContainerFrame* aParent,
                                   nsMenuFrame* aStart,
                                   bool aIsPopup)
{
  nsPresContext* presContext = aParent->PresContext();
  nsContainerFrame* immediateParent = presContext->PresShell()->
    FrameConstructor()->GetInsertionPoint(aParent->GetContent(), nullptr);
  if (!immediateParent)
    immediateParent = aParent;

  nsIFrame* currFrame = nullptr;
  if (aStart) {
    if (aStart->GetNextSibling())
      currFrame = aStart->GetNextSibling();
    else if (aStart->GetParent()->GetContent()->IsXUL(nsGkAtoms::menugroup))
      currFrame = aStart->GetParent()->GetNextSibling();
  }
  else
    currFrame = immediateParent->GetFirstPrincipalChild();

  while (currFrame) {
    
    nsIContent* currFrameContent = currFrame->GetContent();
    if (IsValidMenuItem(presContext, currFrameContent, aIsPopup)) {
      return do_QueryFrame(currFrame);
    }
    if (currFrameContent->IsXUL(nsGkAtoms::menugroup) &&
        currFrameContent->GetChildCount() > 0)
      currFrame = currFrame->GetFirstPrincipalChild();
    else if (!currFrame->GetNextSibling() &&
             currFrame->GetParent()->GetContent()->IsXUL(nsGkAtoms::menugroup))
      currFrame = currFrame->GetParent()->GetNextSibling();
    else
      currFrame = currFrame->GetNextSibling();
  }

  currFrame = immediateParent->GetFirstPrincipalChild();

  
  while (currFrame && currFrame != aStart) {
    
    nsIContent* currFrameContent = currFrame->GetContent();
    if (IsValidMenuItem(presContext, currFrameContent, aIsPopup)) {
      return do_QueryFrame(currFrame);
    }
    if (currFrameContent->IsXUL(nsGkAtoms::menugroup) &&
        currFrameContent->GetChildCount() > 0)
      currFrame = currFrame->GetFirstPrincipalChild();
    else if (!currFrame->GetNextSibling() &&
             currFrame->GetParent()->GetContent()->IsXUL(nsGkAtoms::menugroup))
      currFrame = currFrame->GetParent()->GetNextSibling();
    else
      currFrame = currFrame->GetNextSibling();
  }

  
  return aStart;
}

nsMenuFrame*
nsXULPopupManager::GetPreviousMenuItem(nsContainerFrame* aParent,
                                       nsMenuFrame* aStart,
                                       bool aIsPopup)
{
  nsPresContext* presContext = aParent->PresContext();
  nsContainerFrame* immediateParent = presContext->PresShell()->
    FrameConstructor()->GetInsertionPoint(aParent->GetContent(), nullptr);
  if (!immediateParent)
    immediateParent = aParent;

  const nsFrameList& frames(immediateParent->PrincipalChildList());

  nsIFrame* currFrame = nullptr;
  if (aStart) {
    if (aStart->GetPrevSibling())
      currFrame = aStart->GetPrevSibling();
    else if (aStart->GetParent()->GetContent()->IsXUL(nsGkAtoms::menugroup))
      currFrame = aStart->GetParent()->GetPrevSibling();
  }
  else
    currFrame = frames.LastChild();

  while (currFrame) {
    
    nsIContent* currFrameContent = currFrame->GetContent();
    if (IsValidMenuItem(presContext, currFrameContent, aIsPopup)) {
      return do_QueryFrame(currFrame);
    }
    if (currFrameContent->IsXUL(nsGkAtoms::menugroup) &&
        currFrameContent->GetChildCount() > 0) {
      const nsFrameList& menugroupFrames(currFrame->PrincipalChildList());
      currFrame = menugroupFrames.LastChild();
    }
    else if (!currFrame->GetPrevSibling() &&
             currFrame->GetParent()->GetContent()->IsXUL(nsGkAtoms::menugroup))
      currFrame = currFrame->GetParent()->GetPrevSibling();
    else
      currFrame = currFrame->GetPrevSibling();
  }

  currFrame = frames.LastChild();

  
  while (currFrame && currFrame != aStart) {
    
    nsIContent* currFrameContent = currFrame->GetContent();
    if (IsValidMenuItem(presContext, currFrameContent, aIsPopup)) {
      return do_QueryFrame(currFrame);
    }
    if (currFrameContent->IsXUL(nsGkAtoms::menugroup) &&
        currFrameContent->GetChildCount() > 0) {
      const nsFrameList& menugroupFrames(currFrame->PrincipalChildList());
      currFrame = menugroupFrames.LastChild();
    }
    else if (!currFrame->GetPrevSibling() &&
             currFrame->GetParent()->GetContent()->IsXUL(nsGkAtoms::menugroup))
      currFrame = currFrame->GetParent()->GetPrevSibling();
    else
      currFrame = currFrame->GetPrevSibling();
  }

  
  return aStart;
}

bool
nsXULPopupManager::IsValidMenuItem(nsPresContext* aPresContext,
                                   nsIContent* aContent,
                                   bool aOnPopup)
{
  int32_t ns = aContent->GetNameSpaceID();
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

  
  bool trustedEvent = false;
  aEvent->GetIsTrusted(&trustedEvent);
  if (!trustedEvent) {
    return NS_OK;
  }

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

  if (HandleKeyboardEventWithKeyCode(aKeyEvent, item)) {
    return NS_OK;
  }

  
  if (!mActiveMenuBar && (!item || item->PopupType() != ePopupTypeMenu))
    return NS_OK;

  int32_t menuAccessKey = -1;

  
  

  nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
  if (menuAccessKey) {
    uint32_t theChar;
    aKeyEvent->GetKeyCode(&theChar);

    if (theChar == (uint32_t)menuAccessKey) {
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
          Rollup(0, nullptr, nullptr);
        else if (mActiveMenuBar)
          mActiveMenuBar->MenuClosed();
      }
      aKeyEvent->PreventDefault();
    }
  }

  
  
  aKeyEvent->StopPropagation();
  return NS_OK;
}

nsresult
nsXULPopupManager::KeyPress(nsIDOMKeyEvent* aKeyEvent)
{
  

  nsMenuChainItem* item = GetTopVisibleMenu();
  if (item &&
      (item->Frame()->IsMenuLocked() || item->PopupType() != ePopupTypeMenu)) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  NS_ENSURE_TRUE(keyEvent, NS_ERROR_UNEXPECTED);
  
  bool consume = (mPopups || mActiveMenuBar);
  HandleShortcutNavigation(keyEvent, nullptr);
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
                                 context, mPopupType, mDeselectMenu, mIsRollup);
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
  nsMenuFrame* menuFrame = do_QueryFrame(mMenu->GetPrimaryFrame());
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
    
    
    nsIFrame* frame = menuFrame->GetParent();
    while (frame) {
      nsMenuPopupFrame* popupFrame = do_QueryFrame(frame);
      if (popupFrame) {
        popup = popupFrame->GetContent();
        break;
      }
      frame = frame->GetParent();
    }

    nsPresContext* presContext = menuFrame->PresContext();
    nsCOMPtr<nsIPresShell> shell = presContext->PresShell();
    nsRefPtr<nsViewManager> kungFuDeathGrip = shell->GetViewManager();

    
    if (mCloseMenuMode != CloseMenuMode_None)
      menuFrame->SelectMenu(false);

    AutoHandlingUserInputStatePusher userInpStatePusher(mUserInput, nullptr,
                                                        shell->GetDocument());
    nsContentUtils::DispatchXULCommand(mMenu, mIsTrusted, nullptr, shell,
                                       mControl, mAlt, mShift, mMeta);
  }

  if (popup && mCloseMenuMode != CloseMenuMode_None)
    pm->HidePopup(popup, mCloseMenuMode == CloseMenuMode_Auto, true, false, false);

  return NS_OK;
}
