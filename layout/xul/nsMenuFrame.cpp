




#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsMenuFrame.h"
#include "nsBoxFrame.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsNameSpaceManager.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIDocument.h"
#include "nsIDOMElement.h"
#include "nsIComponentManager.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsBindingManager.h"
#include "nsIServiceManager.h"
#include "nsCSSFrameConstructor.h"
#include "nsIDOMKeyEvent.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIStringBundle.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsIReflowCallback.h"
#include "nsISound.h"
#include "nsIDOMXULMenuListElement.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/Likely.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/TextEvents.h"
#include "mozilla/dom/Element.h"
#include <algorithm>

using namespace mozilla;

#define NS_MENU_POPUP_LIST_INDEX 0

#if defined(XP_WIN)
#define NSCONTEXTMENUISMOUSEUP 1
#endif

static void
AssertNotCalled(void* aPropertyValue)
{
  NS_ERROR("popup list should never be destroyed by the FramePropertyTable");
}
NS_DECLARE_FRAME_PROPERTY(PopupListProperty, AssertNotCalled)

static int32_t gEatMouseMove = false;

const int32_t kBlinkDelay = 67; 


class nsMenuActivateEvent : public nsRunnable
{
public:
  nsMenuActivateEvent(nsIContent *aMenu,
                      nsPresContext* aPresContext,
                      bool aIsActivate)
    : mMenu(aMenu), mPresContext(aPresContext), mIsActivate(aIsActivate)
  {
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    nsAutoString domEventToFire;

    if (mIsActivate) {
      
      mMenu->SetAttr(kNameSpaceID_None, nsGkAtoms::menuactive,
                     NS_LITERAL_STRING("true"), true);
      
      
      domEventToFire.AssignLiteral("DOMMenuItemActive");
    }
    else {
      
      mMenu->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menuactive, true);
      domEventToFire.AssignLiteral("DOMMenuItemInactive");
    }

    nsCOMPtr<nsIDOMEvent> event;
    if (NS_SUCCEEDED(EventDispatcher::CreateEvent(mMenu, mPresContext, nullptr,
                                                  NS_LITERAL_STRING("Events"),
                                                  getter_AddRefs(event)))) {
      event->InitEvent(domEventToFire, true, true);

      event->SetTrusted(true);

      EventDispatcher::DispatchDOMEvent(mMenu, nullptr, event,
                                        mPresContext, nullptr);
    }

    return NS_OK;
  }

private:
  nsCOMPtr<nsIContent> mMenu;
  nsRefPtr<nsPresContext> mPresContext;
  bool mIsActivate;
};

class nsMenuAttributeChangedEvent : public nsRunnable
{
public:
  nsMenuAttributeChangedEvent(nsIFrame* aFrame, nsIAtom* aAttr)
  : mFrame(aFrame), mAttr(aAttr)
  {
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    nsMenuFrame* frame = static_cast<nsMenuFrame*>(mFrame.GetFrame());
    NS_ENSURE_STATE(frame);
    if (mAttr == nsGkAtoms::checked) {
      frame->UpdateMenuSpecialState(frame->PresContext());
    } else if (mAttr == nsGkAtoms::acceltext) {
      
      
      frame->RemoveStateBits(NS_STATE_ACCELTEXT_IS_DERIVED);
      frame->BuildAcceleratorText(true);
    }
    else if (mAttr == nsGkAtoms::key) {
      frame->BuildAcceleratorText(true);
    } else if (mAttr == nsGkAtoms::type || mAttr == nsGkAtoms::name) {
      frame->UpdateMenuType(frame->PresContext());
    }
    return NS_OK;
  }
protected:
  nsWeakFrame       mFrame;
  nsCOMPtr<nsIAtom> mAttr;
};






nsIFrame*
NS_NewMenuFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsMenuFrame* it = new (aPresShell) nsMenuFrame (aPresShell, aContext);
  it->SetIsMenu(true);
  return it;
}

nsIFrame*
NS_NewMenuItemFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsMenuFrame* it = new (aPresShell) nsMenuFrame (aPresShell, aContext);
  it->SetIsMenu(false);
  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsMenuFrame)

NS_QUERYFRAME_HEAD(nsMenuFrame)
  NS_QUERYFRAME_ENTRY(nsMenuFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)

nsMenuFrame::nsMenuFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsBoxFrame(aShell, aContext),
    mIsMenu(false),
    mChecked(false),
    mIgnoreAccelTextChange(false),
    mType(eMenuType_Normal),
    mBlinkState(0)
{
}

nsMenuParent*
nsMenuFrame::GetMenuParent() const
{
  nsContainerFrame* parent = GetParent();
  for (; parent; parent = parent->GetParent()) {
    nsMenuPopupFrame* popup = do_QueryFrame(parent);
    if (popup) {
      return popup;
    }
    nsMenuBarFrame* menubar = do_QueryFrame(parent);
    if (menubar) {
      return menubar;
    }
  }
  return nullptr;
}

class nsASyncMenuInitialization MOZ_FINAL : public nsIReflowCallback
{
public:
  explicit nsASyncMenuInitialization(nsIFrame* aFrame)
    : mWeakFrame(aFrame)
  {
  }

  virtual bool ReflowFinished() MOZ_OVERRIDE
  {
    bool shouldFlush = false;
    nsMenuFrame* menu = do_QueryFrame(mWeakFrame.GetFrame());
    if (menu) {
      menu->UpdateMenuType(menu->PresContext());
      shouldFlush = true;
    }
    delete this;
    return shouldFlush;
  }

  virtual void ReflowCallbackCanceled() MOZ_OVERRIDE
  {
    delete this;
  }

  nsWeakFrame mWeakFrame;
};

void
nsMenuFrame::Init(nsIContent*       aContent,
                  nsContainerFrame* aParent,
                  nsIFrame*         aPrevInFlow)
{
  nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  mTimerMediator = new nsMenuTimerMediator(this);

  BuildAcceleratorText(false);
  nsIReflowCallback* cb = new nsASyncMenuInitialization(this);
  PresContext()->PresShell()->PostReflowCallback(cb);
}

const nsFrameList&
nsMenuFrame::GetChildList(ChildListID aListID) const
{
  if (kPopupList == aListID) {
    nsFrameList* list = GetPopupList();
    return list ? *list : nsFrameList::EmptyList();
  }
  return nsBoxFrame::GetChildList(aListID);
}

void
nsMenuFrame::GetChildLists(nsTArray<ChildList>* aLists) const
{
  nsBoxFrame::GetChildLists(aLists);
  nsFrameList* list = GetPopupList();
  if (list) {
    list->AppendIfNonempty(aLists, kPopupList);
  }
}

nsMenuPopupFrame*
nsMenuFrame::GetPopup()
{
  nsFrameList* popupList = GetPopupList();
  return popupList ? static_cast<nsMenuPopupFrame*>(popupList->FirstChild()) :
                     nullptr;
}

nsFrameList*
nsMenuFrame::GetPopupList() const
{
  if (!HasPopup()) {
    return nullptr;
  }
  nsFrameList* prop =
    static_cast<nsFrameList*>(Properties().Get(PopupListProperty()));
  NS_ASSERTION(prop && prop->GetLength() == 1 &&
               prop->FirstChild()->GetType() == nsGkAtoms::menuPopupFrame,
               "popup list should have exactly one nsMenuPopupFrame");
  return prop;
}

void
nsMenuFrame::DestroyPopupList()
{
  NS_ASSERTION(HasPopup(), "huh?");
  nsFrameList* prop =
    static_cast<nsFrameList*>(Properties().Remove(PopupListProperty()));
  NS_ASSERTION(prop && prop->IsEmpty(),
               "popup list must exist and be empty when destroying");
  RemoveStateBits(NS_STATE_MENU_HAS_POPUP_LIST);
  prop->Delete(PresContext()->PresShell());
}

void
nsMenuFrame::SetPopupFrame(nsFrameList& aFrameList)
{
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    nsMenuPopupFrame* popupFrame = do_QueryFrame(e.get());
    if (popupFrame) {
      
      aFrameList.RemoveFrame(popupFrame);
      nsFrameList* popupList = new (PresContext()->PresShell()) nsFrameList(popupFrame, popupFrame);
      Properties().Set(PopupListProperty(), popupList);
      AddStateBits(NS_STATE_MENU_HAS_POPUP_LIST);
      break;
    }
  }
}

void
nsMenuFrame::SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList)
{
  NS_ASSERTION(!HasPopup(), "SetInitialChildList called twice?");
  if (aListID == kPrincipalList || aListID == kPopupList) {
    SetPopupFrame(aChildList);
  }
  nsBoxFrame::SetInitialChildList(aListID, aChildList);
}

void
nsMenuFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  
  
  if (mOpenTimer) {
    mOpenTimer->Cancel();
  }

  StopBlinking();

  
  
  mTimerMediator->ClearFrame();

  
  
  mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menuactive, false);

  
  nsMenuParent* menuParent = GetMenuParent();
  if (menuParent && menuParent->GetCurrentMenuItem() == this) {
    
    menuParent->CurrentMenuIsBeingDestroyed();
  }

  nsFrameList* popupList = GetPopupList();
  if (popupList) {
    popupList->DestroyFramesFrom(aDestructRoot);
    DestroyPopupList();
  }

  nsBoxFrame::DestroyFrom(aDestructRoot);
}

void
nsMenuFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  if (!aBuilder->IsForEventDelivery()) {
    nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
    return;
  }
    
  nsDisplayListCollection set;
  nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, set);
  
  WrapListsInRedirector(aBuilder, set, aLists);
}

nsresult
nsMenuFrame::HandleEvent(nsPresContext* aPresContext,
                         WidgetGUIEvent* aEvent,
                         nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }
  nsMenuParent* menuParent = GetMenuParent();
  if (menuParent && menuParent->IsMenuLocked()) {
    return NS_OK;
  }

  nsWeakFrame weakFrame(this);
  if (*aEventStatus == nsEventStatus_eIgnore)
    *aEventStatus = nsEventStatus_eConsumeDoDefault;

  bool onmenu = IsOnMenu();

  if (aEvent->message == NS_KEY_PRESS && !IsDisabled()) {
    WidgetKeyboardEvent* keyEvent = aEvent->AsKeyboardEvent();
    uint32_t keyCode = keyEvent->keyCode;
#ifdef XP_MACOSX
    
    if (!IsOpen() && ((keyEvent->charCode == NS_VK_SPACE && !keyEvent->IsMeta()) ||
        (keyCode == NS_VK_UP || keyCode == NS_VK_DOWN))) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
      OpenMenu(false);
    }
#else
    
    if ((keyCode == NS_VK_F4 && !keyEvent->IsAlt()) ||
        ((keyCode == NS_VK_UP || keyCode == NS_VK_DOWN) && keyEvent->IsAlt())) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
      ToggleMenuState();
    }
#endif
  }
  else if (aEvent->message == NS_MOUSE_BUTTON_DOWN &&
           aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton &&
           !IsDisabled() && IsMenu()) {
    
    
    
    
    if (!menuParent || menuParent->IsMenuBar()) {
      ToggleMenuState();
    }
    else {
      if (!IsOpen()) {
        OpenMenu(false);
      }
    }
  }
  else if (
#ifndef NSCONTEXTMENUISMOUSEUP
           (aEvent->message == NS_MOUSE_BUTTON_UP &&
            aEvent->AsMouseEvent()->button == WidgetMouseEvent::eRightButton) &&
#else
           aEvent->message == NS_CONTEXTMENU &&
#endif
           onmenu && !IsMenu() && !IsDisabled()) {
    
    
    
    
    
    
    
    
    
    
    if (menuParent->IsContextMenu()) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
      Execute(aEvent);
    }
  }
  else if (aEvent->message == NS_MOUSE_BUTTON_UP &&
           aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton &&
           !IsMenu() && !IsDisabled()) {
    
    *aEventStatus = nsEventStatus_eConsumeNoDefault;
    Execute(aEvent);
  }
  else if (aEvent->message == NS_MOUSE_EXIT_SYNTH) {
    
    if (mOpenTimer) {
      mOpenTimer->Cancel();
      mOpenTimer = nullptr;
    }

    
    if (menuParent) {
      bool onmenubar = menuParent->IsMenuBar();
      if (!(onmenubar && menuParent->IsActive())) {
        if (IsMenu() && !onmenubar && IsOpen()) {
          
        }
        else if (this == menuParent->GetCurrentMenuItem()) {
          menuParent->ChangeMenuItem(nullptr, false);
        }
      }
    }
  }
  else if (aEvent->message == NS_MOUSE_MOVE &&
           (onmenu || (menuParent && menuParent->IsMenuBar()))) {
    if (gEatMouseMove) {
      gEatMouseMove = false;
      return NS_OK;
    }

    
    menuParent->ChangeMenuItem(this, false);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    NS_ENSURE_TRUE(menuParent, NS_OK);

    
    
    nsMenuFrame *realCurrentItem = menuParent->GetCurrentMenuItem();
    if (realCurrentItem != this) {
      
      return NS_OK;
    }

    
    
    
    
    if (!IsDisabled() && IsMenu() && !IsOpen() && !mOpenTimer && !menuParent->IsMenuBar()) {
      int32_t menuDelay =
        LookAndFeel::GetInt(LookAndFeel::eIntID_SubmenuDelay, 300); 

      
      mOpenTimer = do_CreateInstance("@mozilla.org/timer;1");
      mOpenTimer->InitWithCallback(mTimerMediator, menuDelay, nsITimer::TYPE_ONE_SHOT);
    }
  }
  
  return NS_OK;
}

void
nsMenuFrame::ToggleMenuState()
{
  if (IsOpen())
    CloseMenu(false);
  else
    OpenMenu(false);
}

void
nsMenuFrame::PopupOpened()
{
  nsWeakFrame weakFrame(this);
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::open,
                    NS_LITERAL_STRING("true"), true);
  if (!weakFrame.IsAlive())
    return;

  nsMenuParent* menuParent = GetMenuParent();
  if (menuParent) {
    menuParent->SetActive(true);
    
    
    menuParent->SetCurrentMenuItem(this);
  }
}

void
nsMenuFrame::PopupClosed(bool aDeselectMenu)
{
  nsWeakFrame weakFrame(this);
  nsContentUtils::AddScriptRunner(
    new nsUnsetAttrRunnable(mContent, nsGkAtoms::open));
  if (!weakFrame.IsAlive())
    return;

  
  nsMenuParent* menuParent = GetMenuParent();
  if (menuParent && menuParent->MenuClosed()) {
    if (aDeselectMenu) {
      SelectMenu(false);
    } else {
      
      
      
      nsMenuFrame *current = menuParent->GetCurrentMenuItem();
      if (current) {
        
        
        
        
        
        
        
        nsIFrame* parent = current;
        while (parent) {
          nsMenuBarFrame* menubar = do_QueryFrame(parent);
          if (menubar && menubar->GetStayActive())
            return;

          parent = parent->GetParent();
        }

        nsCOMPtr<nsIRunnable> event =
          new nsMenuActivateEvent(current->GetContent(),
                                  PresContext(), true);
        NS_DispatchToCurrentThread(event);
      }
    }
  }
}

NS_IMETHODIMP
nsMenuFrame::SelectMenu(bool aActivateFlag)
{
  if (mContent) {
    
    
    
    
    
    if (aActivateFlag) {
      nsIFrame* parent = GetParent();
      while (parent) {
        nsMenuPopupFrame* menupopup = do_QueryFrame(parent);
        if (menupopup) {
          
          nsMenuFrame* menu = do_QueryFrame(menupopup->GetParent());
          if (menu) {
            
            nsIFrame* popupParent = menu->GetParent();
            while (popupParent) {
              menupopup = do_QueryFrame(popupParent);
              if (menupopup) {
                menupopup->SetCurrentMenuItem(menu);
                break;
              }
              popupParent = popupParent->GetParent();
            }
          }
          break;
        }
        parent = parent->GetParent();
      }
    }

    
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm) {
      nsMenuParent* menuParent = GetMenuParent();
      pm->CancelMenuTimer(menuParent);
    }

    nsCOMPtr<nsIRunnable> event =
      new nsMenuActivateEvent(mContent, PresContext(), aActivateFlag);
    NS_DispatchToCurrentThread(event);
  }

  return NS_OK;
}

nsresult
nsMenuFrame::AttributeChanged(int32_t aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t aModType)
{
  if (aAttribute == nsGkAtoms::acceltext && mIgnoreAccelTextChange) {
    
    mIgnoreAccelTextChange = false;
    return NS_OK;
  }

  if (aAttribute == nsGkAtoms::checked ||
      aAttribute == nsGkAtoms::acceltext ||
      aAttribute == nsGkAtoms::key ||
      aAttribute == nsGkAtoms::type ||
      aAttribute == nsGkAtoms::name) {
    nsCOMPtr<nsIRunnable> event =
      new nsMenuAttributeChangedEvent(this, aAttribute);
    nsContentUtils::AddScriptRunner(event);
  }
  return NS_OK;
}

nsIContent*
nsMenuFrame::GetAnchor()
{
  mozilla::dom::Element* anchor = nullptr;

  nsAutoString id;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::anchor, id);
  if (!id.IsEmpty()) {
    nsIDocument* doc = mContent->OwnerDoc();

    anchor =
      doc->GetAnonymousElementByAttribute(mContent, nsGkAtoms::anonid, id);
    if (!anchor) {
      anchor = doc->GetElementById(id);
    }
  }

  
  return anchor && anchor->GetPrimaryFrame() ? anchor : mContent;
}

void
nsMenuFrame::OpenMenu(bool aSelectFirstItem)
{
  if (!mContent)
    return;

  gEatMouseMove = true;

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    pm->KillMenuTimer();
    
    pm->ShowMenu(mContent, aSelectFirstItem, true);
  }
}

void
nsMenuFrame::CloseMenu(bool aDeselectMenu)
{
  gEatMouseMove = true;

  
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && HasPopup())
    pm->HidePopup(GetPopup()->GetContent(), false, aDeselectMenu, true, false);
}

bool
nsMenuFrame::IsSizedToPopup(nsIContent* aContent, bool aRequireAlways)
{
  nsAutoString sizedToPopup;
  aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::sizetopopup, sizedToPopup);
  return sizedToPopup.EqualsLiteral("always") ||
         (!aRequireAlways && sizedToPopup.EqualsLiteral("pref"));
}

nsSize
nsMenuFrame::GetMinSize(nsBoxLayoutState& aBoxLayoutState)
{
  nsSize size = nsBoxFrame::GetMinSize(aBoxLayoutState);
  DISPLAY_MIN_SIZE(this, size);

  if (IsSizedToPopup(mContent, true))
    SizeToPopup(aBoxLayoutState, size);

  return size;
}

NS_IMETHODIMP
nsMenuFrame::DoLayout(nsBoxLayoutState& aState)
{
  
  nsresult rv = nsBoxFrame::DoLayout(aState);

  nsMenuPopupFrame* popupFrame = GetPopup();
  if (popupFrame) {
    bool sizeToPopup = IsSizedToPopup(mContent, false);
    popupFrame->LayoutPopup(aState, this, GetAnchor()->GetPrimaryFrame(), sizeToPopup);
  }

  return rv;
}

#ifdef DEBUG_LAYOUT
nsresult
nsMenuFrame::SetDebug(nsBoxLayoutState& aState, bool aDebug)
{
  
  bool debugSet = mState & NS_STATE_CURRENTLY_IN_DEBUG;
  bool debugChanged = (!aDebug && debugSet) || (aDebug && !debugSet);

  
  if (debugChanged)
  {
      nsBoxFrame::SetDebug(aState, aDebug);
      nsMenuPopupFrame* popupFrame = GetPopup();
      if (popupFrame)
        SetDebug(aState, popupFrame, aDebug);
  }

  return NS_OK;
}

nsresult
nsMenuFrame::SetDebug(nsBoxLayoutState& aState, nsIFrame* aList, bool aDebug)
{
      if (!aList)
          return NS_OK;

      while (aList) {
        if (aList->IsBoxFrame())
          aList->SetDebug(aState, aDebug);

        aList = aList->GetNextSibling();
      }

      return NS_OK;
}
#endif








nsMenuFrame*
nsMenuFrame::Enter(WidgetGUIEvent* aEvent)
{
  if (IsDisabled()) {
#ifdef XP_WIN
    
    nsMenuParent* menuParent = GetMenuParent();
    if (menuParent) {
      nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
      if (pm) {
        nsIFrame* popup = pm->GetTopPopup(ePopupTypeAny);
        if (popup)
          pm->HidePopup(popup->GetContent(), true, true, true, false);
      }
    }
#endif   
    
    return nullptr;
  }

  if (!IsOpen()) {
    
    nsMenuParent* menuParent = GetMenuParent();
    if (!IsMenu() && menuParent)
      Execute(aEvent);          
    else
      return this;
  }

  return nullptr;
}

bool
nsMenuFrame::IsOpen()
{
  nsMenuPopupFrame* popupFrame = GetPopup();
  return popupFrame && popupFrame->IsOpen();
}

bool
nsMenuFrame::IsMenu()
{
  return mIsMenu;
}

nsMenuListType
nsMenuFrame::GetParentMenuListType()
{
  nsMenuParent* menuParent = GetMenuParent();
  if (menuParent && menuParent->IsMenu()) {
    nsMenuPopupFrame* popupFrame = static_cast<nsMenuPopupFrame*>(menuParent);
    nsIFrame* parentMenu = popupFrame->GetParent();
    if (parentMenu) {
      nsCOMPtr<nsIDOMXULMenuListElement> menulist = do_QueryInterface(parentMenu->GetContent());
      if (menulist) {
        bool isEditable = false;
        menulist->GetEditable(&isEditable);
        return isEditable ? eEditableMenuList : eReadonlyMenuList;
      }
    }
  }
  return eNotMenuList;
}

nsresult
nsMenuFrame::Notify(nsITimer* aTimer)
{
  
  if (aTimer == mOpenTimer.get()) {
    mOpenTimer = nullptr;

    nsMenuParent* menuParent = GetMenuParent();
    if (!IsOpen() && menuParent) {
      
      
      nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
      if (pm) {
        if ((!pm->HasContextMenu(nullptr) || menuParent->IsContextMenu()) &&
            mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menuactive,
                                  nsGkAtoms::_true, eCaseMatters)) {
          OpenMenu(false);
        }
      }
    }
  } else if (aTimer == mBlinkTimer) {
    switch (mBlinkState++) {
      case 0:
        NS_ASSERTION(false, "Blink timer fired while not blinking");
        StopBlinking();
        break;
      case 1:
        {
          
          nsWeakFrame weakFrame(this);
          mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::menuactive,
                            NS_LITERAL_STRING("true"), true);
          if (weakFrame.IsAlive()) {
            aTimer->InitWithCallback(mTimerMediator, kBlinkDelay, nsITimer::TYPE_ONE_SHOT);
          }
        }
        break;
      default: {
        nsMenuParent* menuParent = GetMenuParent();
        if (menuParent) {
          menuParent->LockMenuUntilClosed(false);
        }
        PassMenuCommandEventToPopupManager();
        StopBlinking();
        break;
      }
    }
  }

  return NS_OK;
}

bool 
nsMenuFrame::IsDisabled()
{
  return mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                               nsGkAtoms::_true, eCaseMatters);
}

void
nsMenuFrame::UpdateMenuType(nsPresContext* aPresContext)
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::checkbox, &nsGkAtoms::radio, nullptr};
  switch (mContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::type,
                                    strings, eCaseMatters)) {
    case 0: mType = eMenuType_Checkbox; break;
    case 1:
      mType = eMenuType_Radio;
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::name, mGroupName);
      break;

    default:
      if (mType != eMenuType_Normal) {
        nsWeakFrame weakFrame(this);
        mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::checked,
                            true);
        ENSURE_TRUE(weakFrame.IsAlive());
      }
      mType = eMenuType_Normal;
      break;
  }
  UpdateMenuSpecialState(aPresContext);
}


void
nsMenuFrame::UpdateMenuSpecialState(nsPresContext* aPresContext)
{
  bool newChecked =
    mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::checked,
                          nsGkAtoms::_true, eCaseMatters); 
  if (newChecked == mChecked) {
    

    if (mType != eMenuType_Radio)
      return; 

    if (!mChecked || mGroupName.IsEmpty())
      return;                   
  } else { 
    mChecked = newChecked;
    if (mType != eMenuType_Radio || !mChecked)
      



      return;
  }

  





  










  
  
  
  
  nsIFrame* firstMenuItem = nsXULPopupManager::GetNextMenuItem(GetParent(), nullptr, true);
  nsIFrame* sib = firstMenuItem;
  while (sib) {
    nsMenuFrame* menu = do_QueryFrame(sib);
    if (sib != this) {
      if (menu && menu->GetMenuType() == eMenuType_Radio &&
          menu->IsChecked() && menu->GetRadioGroupName() == mGroupName) {
        
        sib->GetContent()->UnsetAttr(kNameSpaceID_None, nsGkAtoms::checked,
                                     true);
        
        return;
      }
    }
    sib = nsXULPopupManager::GetNextMenuItem(GetParent(), menu, true);
    if (sib == firstMenuItem) {
      break;
    }
  } 
}

void 
nsMenuFrame::BuildAcceleratorText(bool aNotify)
{
  nsAutoString accelText;

  if ((GetStateBits() & NS_STATE_ACCELTEXT_IS_DERIVED) == 0) {
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::acceltext, accelText);
    if (!accelText.IsEmpty())
      return;
  }
  

  
  AddStateBits(NS_STATE_ACCELTEXT_IS_DERIVED);

  
  nsWeakFrame weakFrame(this);
  mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::acceltext, aNotify);
  ENSURE_TRUE(weakFrame.IsAlive());

  
  nsAutoString keyValue;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::key, keyValue);
  if (keyValue.IsEmpty())
    return;

  
  nsIDocument *document = mContent->GetUncomposedDoc();
  if (!document)
    return;

  
  
  nsIContent *keyElement = document->GetElementById(keyValue);
  if (!keyElement) {
#ifdef DEBUG
    nsAutoString label;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, label);
    nsAutoString msg = NS_LITERAL_STRING("Key '") +
                       keyValue +
                       NS_LITERAL_STRING("' of menu item '") +
                       label +
                       NS_LITERAL_STRING("' could not be found");
    NS_WARNING(NS_ConvertUTF16toUTF8(msg).get());
#endif
    return;
  }

  
  
  
  nsAutoString accelString;
  keyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::keytext, accelString);

  if (accelString.IsEmpty()) {
    keyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::key, accelString);

    if (!accelString.IsEmpty()) {
      ToUpperCase(accelString);
    } else {
      nsAutoString keyCode;
      keyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::keycode, keyCode);
      ToUpperCase(keyCode);

      nsresult rv;
      nsCOMPtr<nsIStringBundleService> bundleService =
        mozilla::services::GetStringBundleService();
      if (bundleService) {
        nsCOMPtr<nsIStringBundle> bundle;
        rv = bundleService->CreateBundle("chrome://global/locale/keys.properties",
                                         getter_AddRefs(bundle));

        if (NS_SUCCEEDED(rv) && bundle) {
          nsXPIDLString keyName;
          rv = bundle->GetStringFromName(keyCode.get(), getter_Copies(keyName));
          if (keyName)
            accelString = keyName;
        }
      }

      
      if (accelString.IsEmpty())
        return;
    }
  }

  nsAutoString modifiers;
  keyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::modifiers, modifiers);
  
  char* str = ToNewCString(modifiers);
  char* newStr;
  char* token = nsCRT::strtok(str, ", \t", &newStr);

  nsAutoString shiftText;
  nsAutoString altText;
  nsAutoString metaText;
  nsAutoString controlText;
  nsAutoString osText;
  nsAutoString modifierSeparator;

  nsContentUtils::GetShiftText(shiftText);
  nsContentUtils::GetAltText(altText);
  nsContentUtils::GetMetaText(metaText);
  nsContentUtils::GetControlText(controlText);
  nsContentUtils::GetOSText(osText);
  nsContentUtils::GetModifierSeparatorText(modifierSeparator);

  while (token) {
      
    if (PL_strcmp(token, "shift") == 0)
      accelText += shiftText;
    else if (PL_strcmp(token, "alt") == 0) 
      accelText += altText; 
    else if (PL_strcmp(token, "meta") == 0) 
      accelText += metaText; 
    else if (PL_strcmp(token, "os") == 0)
      accelText += osText; 
    else if (PL_strcmp(token, "control") == 0) 
      accelText += controlText; 
    else if (PL_strcmp(token, "accel") == 0) {
      switch (WidgetInputEvent::AccelModifier()) {
        case MODIFIER_META:
          accelText += metaText;
          break;
        case MODIFIER_OS:
          accelText += osText;
          break;
        case MODIFIER_ALT:
          accelText += altText;
          break;
        case MODIFIER_CONTROL:
          accelText += controlText;
          break;
        default:
          MOZ_CRASH(
            "Handle the new result of WidgetInputEvent::AccelModifier()");
          break;
      }
    }
    
    accelText += modifierSeparator;

    token = nsCRT::strtok(newStr, ", \t", &newStr);
  }

  nsMemory::Free(str);

  accelText += accelString;

  mIgnoreAccelTextChange = true;
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::acceltext, accelText, aNotify);
  ENSURE_TRUE(weakFrame.IsAlive());

  mIgnoreAccelTextChange = false;
}

void
nsMenuFrame::Execute(WidgetGUIEvent* aEvent)
{
  
  bool needToFlipChecked = false;
  if (mType == eMenuType_Checkbox || (mType == eMenuType_Radio && !mChecked)) {
    needToFlipChecked = !mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::autocheck,
                                               nsGkAtoms::_false, eCaseMatters);
  }

  nsCOMPtr<nsISound> sound(do_CreateInstance("@mozilla.org/sound;1"));
  if (sound)
    sound->PlayEventSound(nsISound::EVENT_MENU_EXECUTE);

  StartBlinking(aEvent, needToFlipChecked);
}

bool
nsMenuFrame::ShouldBlink()
{
  int32_t shouldBlink =
    LookAndFeel::GetInt(LookAndFeel::eIntID_ChosenMenuItemsShouldBlink, 0);
  if (!shouldBlink)
    return false;

  
  if (GetParentMenuListType() == eEditableMenuList)
    return false;

  return true;
}

void
nsMenuFrame::StartBlinking(WidgetGUIEvent* aEvent, bool aFlipChecked)
{
  StopBlinking();
  CreateMenuCommandEvent(aEvent, aFlipChecked);

  if (!ShouldBlink()) {
    PassMenuCommandEventToPopupManager();
    return;
  }

  
  nsWeakFrame weakFrame(this);
  mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menuactive, true);
  if (!weakFrame.IsAlive())
    return;

  nsMenuParent* menuParent = GetMenuParent();
  if (menuParent) {
    
    menuParent->LockMenuUntilClosed(true);
  }

  
  mBlinkTimer = do_CreateInstance("@mozilla.org/timer;1");
  mBlinkTimer->InitWithCallback(mTimerMediator, kBlinkDelay, nsITimer::TYPE_ONE_SHOT);
  mBlinkState = 1;
}

void
nsMenuFrame::StopBlinking()
{
  mBlinkState = 0;
  if (mBlinkTimer) {
    mBlinkTimer->Cancel();
    mBlinkTimer = nullptr;
  }
  mDelayedMenuCommandEvent = nullptr;
}

void
nsMenuFrame::CreateMenuCommandEvent(WidgetGUIEvent* aEvent, bool aFlipChecked)
{
  
  
  
  bool isTrusted = aEvent ? aEvent->mFlags.mIsTrusted :
                              nsContentUtils::IsCallerChrome();

  bool shift = false, control = false, alt = false, meta = false;
  WidgetInputEvent* inputEvent = aEvent ? aEvent->AsInputEvent() : nullptr;
  if (inputEvent) {
    shift = inputEvent->IsShift();
    control = inputEvent->IsControl();
    alt = inputEvent->IsAlt();
    meta = inputEvent->IsMeta();
  }

  
  
  
  bool userinput = EventStateManager::IsHandlingUserInput();

  mDelayedMenuCommandEvent =
    new nsXULMenuCommandEvent(mContent, isTrusted, shift, control, alt, meta,
                              userinput, aFlipChecked);
}

void
nsMenuFrame::PassMenuCommandEventToPopupManager()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  nsMenuParent* menuParent = GetMenuParent();
  if (pm && menuParent && mDelayedMenuCommandEvent) {
    pm->ExecuteMenu(mContent, mDelayedMenuCommandEvent);
  }
  mDelayedMenuCommandEvent = nullptr;
}

void
nsMenuFrame::RemoveFrame(ChildListID     aListID,
                         nsIFrame*       aOldFrame)
{
  nsFrameList* popupList = GetPopupList();
  if (popupList && popupList->FirstChild() == aOldFrame) {
    popupList->RemoveFirstChild();
    aOldFrame->Destroy();
    DestroyPopupList();
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    return;
  }
  nsBoxFrame::RemoveFrame(aListID, aOldFrame);
}

void
nsMenuFrame::InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList)
{
  if (!HasPopup() && (aListID == kPrincipalList || aListID == kPopupList)) {
    SetPopupFrame(aFrameList);
    if (HasPopup()) {
#ifdef DEBUG_LAYOUT
      nsBoxLayoutState state(PresContext());
      SetDebug(state, aFrameList, mState & NS_STATE_CURRENTLY_IN_DEBUG);
#endif

      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }

  if (aFrameList.IsEmpty())
    return;

  if (MOZ_UNLIKELY(aPrevFrame && aPrevFrame == GetPopup())) {
    aPrevFrame = nullptr;
  }

  nsBoxFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
}

void
nsMenuFrame::AppendFrames(ChildListID     aListID,
                          nsFrameList&    aFrameList)
{
  if (!HasPopup() && (aListID == kPrincipalList || aListID == kPopupList)) {
    SetPopupFrame(aFrameList);
    if (HasPopup()) {

#ifdef DEBUG_LAYOUT
      nsBoxLayoutState state(PresContext());
      SetDebug(state, aFrameList, mState & NS_STATE_CURRENTLY_IN_DEBUG);
#endif
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }

  if (aFrameList.IsEmpty())
    return;

  nsBoxFrame::AppendFrames(aListID, aFrameList); 
}

bool
nsMenuFrame::SizeToPopup(nsBoxLayoutState& aState, nsSize& aSize)
{
  if (!IsCollapsed()) {
    bool widthSet, heightSet;
    nsSize tmpSize(-1, 0);
    nsIFrame::AddCSSPrefSize(this, tmpSize, widthSet, heightSet);
    if (!widthSet && GetFlex(aState) == 0) {
      nsMenuPopupFrame* popupFrame = GetPopup();
      if (!popupFrame)
        return false;
      tmpSize = popupFrame->GetPrefSize(aState);

      
      
      
      
      
      
      nsMargin borderPadding;
      GetBorderAndPadding(borderPadding);

      
      nsIScrollableFrame* scrollFrame = do_QueryFrame(popupFrame->GetFirstPrincipalChild());
      nscoord scrollbarWidth = 0;
      if (scrollFrame) {
        scrollbarWidth =
          scrollFrame->GetDesiredScrollbarSizes(&aState).LeftRight();
      }

      aSize.width =
        tmpSize.width + std::max(borderPadding.LeftRight(), scrollbarWidth);

      return true;
    }
  }

  return false;
}

nsSize
nsMenuFrame::GetPrefSize(nsBoxLayoutState& aState)
{
  nsSize size = nsBoxFrame::GetPrefSize(aState);
  DISPLAY_PREF_SIZE(this, size);

  
  
  if (!IsSizedToPopup(mContent, true) &&
      IsSizedToPopup(mContent, false) &&
      SizeToPopup(aState, size)) {
    
    nsSize minSize = nsBoxFrame::GetMinSize(aState);
    nsSize maxSize = GetMaxSize(aState);
    size = BoundsCheck(minSize, size, maxSize);
  }

  return size;
}

NS_IMETHODIMP
nsMenuFrame::GetActiveChild(nsIDOMElement** aResult)
{
  nsMenuPopupFrame* popupFrame = GetPopup();
  if (!popupFrame)
    return NS_ERROR_FAILURE;

  nsMenuFrame* menuFrame = popupFrame->GetCurrentMenuItem();
  if (!menuFrame) {
    *aResult = nullptr;
  }
  else {
    nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(menuFrame->GetContent()));
    *aResult = elt;
    NS_IF_ADDREF(*aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SetActiveChild(nsIDOMElement* aChild)
{
  nsMenuPopupFrame* popupFrame = GetPopup();
  if (!popupFrame)
    return NS_ERROR_FAILURE;

  if (!aChild) {
    
    popupFrame->ChangeMenuItem(nullptr, false);
    return NS_OK;
  }

  nsCOMPtr<nsIContent> child(do_QueryInterface(aChild));

  nsMenuFrame* menu = do_QueryFrame(child->GetPrimaryFrame());
  if (menu)
    popupFrame->ChangeMenuItem(menu, false);
  return NS_OK;
}

nsIScrollableFrame* nsMenuFrame::GetScrollTargetFrame()
{
  nsMenuPopupFrame* popupFrame = GetPopup();
  if (!popupFrame)
    return nullptr;
  nsIFrame* childFrame = popupFrame->GetFirstPrincipalChild();
  if (childFrame)
    return popupFrame->GetScrollFrame(childFrame);
  return nullptr;
}


NS_IMPL_ISUPPORTS(nsMenuTimerMediator, nsITimerCallback)





nsMenuTimerMediator::nsMenuTimerMediator(nsMenuFrame *aFrame) :
  mFrame(aFrame)
{
  NS_ASSERTION(mFrame, "Must have frame");
}

nsMenuTimerMediator::~nsMenuTimerMediator()
{
}






NS_IMETHODIMP nsMenuTimerMediator::Notify(nsITimer* aTimer)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->Notify(aTimer);
}





void nsMenuTimerMediator::ClearFrame()
{
  mFrame = nullptr;
}
