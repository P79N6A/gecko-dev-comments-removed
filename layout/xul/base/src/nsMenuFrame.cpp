








































#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsMenuFrame.h"
#include "nsBoxFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIView.h"
#include "nsIDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsWidgetsCID.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsIViewManager.h"
#include "nsBindingManager.h"
#include "nsIServiceManager.h"
#include "nsCSSFrameConstructor.h"
#include "nsIDOMKeyEvent.h"
#include "nsEventDispatcher.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIScrollableView.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIStringBundle.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsIReflowCallback.h"
#include "nsISound.h"

#define NS_MENU_POPUP_LIST_INDEX 0

#if defined(XP_WIN) || defined(XP_OS2)
#define NSCONTEXTMENUISMOUSEUP 1
#endif

static PRInt32 gEatMouseMove = PR_FALSE;

static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

nsrefcnt nsMenuFrame::gRefCnt = 0;
nsString *nsMenuFrame::gShiftText = nsnull;
nsString *nsMenuFrame::gControlText = nsnull;
nsString *nsMenuFrame::gMetaText = nsnull;
nsString *nsMenuFrame::gAltText = nsnull;
nsString *nsMenuFrame::gModifierSeparator = nsnull;


class nsMenuActivateEvent : public nsRunnable
{
public:
  nsMenuActivateEvent(nsIContent *aMenu,
                      nsPresContext* aPresContext,
                      PRBool aIsActivate)
    : mMenu(aMenu), mPresContext(aPresContext), mIsActivate(aIsActivate)
  {
  }

  NS_IMETHOD Run()
  {
    nsAutoString domEventToFire;

    if (mIsActivate) {
      
      mMenu->SetAttr(kNameSpaceID_None, nsGkAtoms::menuactive,
                     NS_LITERAL_STRING("true"), PR_TRUE);
      
      
      domEventToFire.AssignLiteral("DOMMenuItemActive");
    }
    else {
      
      mMenu->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menuactive, PR_TRUE);
      domEventToFire.AssignLiteral("DOMMenuItemInactive");
    }

    nsCOMPtr<nsIDOMEvent> event;
    if (NS_SUCCEEDED(nsEventDispatcher::CreateEvent(mPresContext, nsnull,
                                                    NS_LITERAL_STRING("Events"),
                                                    getter_AddRefs(event)))) {
      event->InitEvent(domEventToFire, PR_TRUE, PR_TRUE);

      nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
      privateEvent->SetTrusted(PR_TRUE);

      nsEventDispatcher::DispatchDOMEvent(mMenu, nsnull, event,
                                          mPresContext, nsnull);
    }

    return NS_OK;
  }

private:
  nsCOMPtr<nsIContent> mMenu;
  nsCOMPtr<nsPresContext> mPresContext;
  PRBool mIsActivate;
};






nsIFrame*
NS_NewMenuFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsMenuFrame* it = new (aPresShell) nsMenuFrame (aPresShell, aContext);
  
  if (it)
    it->SetIsMenu(PR_TRUE);

  return it;
}

nsIFrame*
NS_NewMenuItemFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsMenuFrame* it = new (aPresShell) nsMenuFrame (aPresShell, aContext);

  if (it)
    it->SetIsMenu(PR_FALSE);

  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsMenuFrame)

NS_QUERYFRAME_HEAD(nsMenuFrame)
  NS_QUERYFRAME_ENTRY(nsIMenuFrame)
  NS_QUERYFRAME_ENTRY(nsIScrollableViewProvider)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)




nsMenuFrame::nsMenuFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsBoxFrame(aShell, aContext),
    mIsMenu(PR_FALSE),
    mChecked(PR_FALSE),
    mType(eMenuType_Normal),
    mMenuParent(nsnull),
    mPopupFrame(nsnull)
{

} 

NS_IMETHODIMP
nsMenuFrame::SetParent(const nsIFrame* aParent)
{
  nsBoxFrame::SetParent(aParent);
  InitMenuParent(const_cast<nsIFrame *>(aParent));
  return NS_OK;
}

void
nsMenuFrame::InitMenuParent(nsIFrame* aParent)
{
  while (aParent) {
    nsIAtom* type = aParent->GetType();
    if (type == nsGkAtoms::menuPopupFrame) {
      mMenuParent = static_cast<nsMenuPopupFrame *>(aParent);
      break;
    }
    else if (type == nsGkAtoms::menuBarFrame) {
      mMenuParent = static_cast<nsMenuBarFrame *>(aParent);
      break;
    }
    aParent = aParent->GetParent();
  }
}

class nsASyncMenuInitialization : public nsIReflowCallback
{
public:
  nsASyncMenuInitialization(nsIFrame* aFrame)
    : mWeakFrame(aFrame)
  {
  }

  virtual PRBool ReflowFinished()
  {
    PRBool shouldFlush = PR_FALSE;
    if (mWeakFrame.IsAlive()) {
      if (mWeakFrame.GetFrame()->GetType() == nsGkAtoms::menuFrame) {
        nsMenuFrame* menu = static_cast<nsMenuFrame*>(mWeakFrame.GetFrame());
        menu->UpdateMenuType(menu->PresContext());
        shouldFlush = PR_TRUE;
      }
    }
    delete this;
    return shouldFlush;
  }

  virtual void ReflowCallbackCanceled()
  {
    delete this;
  }

  nsWeakFrame mWeakFrame;
};

NS_IMETHODIMP
nsMenuFrame::Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  mTimerMediator = new nsMenuTimerMediator(this);
  if (NS_UNLIKELY(!mTimerMediator))
    return NS_ERROR_OUT_OF_MEMORY;

  InitMenuParent(aParent);

  
  if (gRefCnt++ == 0) {
    
    nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
    nsCOMPtr<nsIStringBundle> bundle;
    if (NS_SUCCEEDED(rv) && bundleService) {
      rv = bundleService->CreateBundle( "chrome://global-platform/locale/platformKeys.properties",
                                        getter_AddRefs(bundle));
    }    
    
    NS_ASSERTION(NS_SUCCEEDED(rv) && bundle, "chrome://global/locale/platformKeys.properties could not be loaded");
    nsXPIDLString shiftModifier;
    nsXPIDLString metaModifier;
    nsXPIDLString altModifier;
    nsXPIDLString controlModifier;
    nsXPIDLString modifierSeparator;
    if (NS_SUCCEEDED(rv) && bundle) {
      
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("VK_SHIFT").get(), getter_Copies(shiftModifier));
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("VK_META").get(), getter_Copies(metaModifier));
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("VK_ALT").get(), getter_Copies(altModifier));
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("VK_CONTROL").get(), getter_Copies(controlModifier));
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("MODIFIER_SEPARATOR").get(), getter_Copies(modifierSeparator));
    } else {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    
    gShiftText = new nsString(shiftModifier);
    gMetaText = new nsString(metaModifier);
    gAltText = new nsString(altModifier);
    gControlText = new nsString(controlModifier);
    gModifierSeparator = new nsString(modifierSeparator);    
  }

  BuildAcceleratorText();
  nsIReflowCallback* cb = new nsASyncMenuInitialization(this);
  NS_ENSURE_TRUE(cb, NS_ERROR_OUT_OF_MEMORY);
  PresContext()->PresShell()->PostReflowCallback(cb);
  return rv;
}

nsMenuFrame::~nsMenuFrame()
{
  
  if (--gRefCnt == 0) {
    delete gShiftText;
    gShiftText = nsnull;
    delete gControlText;  
    gControlText = nsnull;
    delete gMetaText;  
    gMetaText = nsnull;
    delete gAltText;  
    gAltText = nsnull;
    delete gModifierSeparator;
    gModifierSeparator = nsnull;
  }
}



nsFrameList
nsMenuFrame::GetChildList(nsIAtom* aListName) const
{
  if (nsGkAtoms::popupList == aListName) {
    return nsFrameList(mPopupFrame, mPopupFrame);
  }
  return nsBoxFrame::GetChildList(aListName);
}

void
nsMenuFrame::SetPopupFrame(nsFrameList& aFrameList)
{
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    if (e.get()->GetType() == nsGkAtoms::menuPopupFrame) {
      
      mPopupFrame = (nsMenuPopupFrame *)e.get();
      aFrameList.RemoveFrame(e.get());
      break;
    }
  }
}

NS_IMETHODIMP
nsMenuFrame::SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList)
{
  NS_ASSERTION(!mPopupFrame, "already have a popup frame set");
  if (!aListName || aListName == nsGkAtoms::popupList)
    SetPopupFrame(aChildList);
  return nsBoxFrame::SetInitialChildList(aListName, aChildList);
}

nsIAtom*
nsMenuFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (NS_MENU_POPUP_LIST_INDEX == aIndex) {
    return nsGkAtoms::popupList;
  }
  return nsnull;
}

void
nsMenuFrame::Destroy()
{
  
  
  
  if (mOpenTimer) {
    mOpenTimer->Cancel();
  }

  
  
  mTimerMediator->ClearFrame();

  
  
  mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menuactive, PR_FALSE);

  
  if (mMenuParent && mMenuParent->GetCurrentMenuItem() == this) {
    
    mMenuParent->CurrentMenuIsBeingDestroyed();
  }

  if (mPopupFrame)
    mPopupFrame->Destroy();

  nsBoxFrame::Destroy();
}

NS_IMETHODIMP
nsMenuFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  if (!aBuilder->IsForEventDelivery())
    return nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
    
  nsDisplayListCollection set;
  nsresult rv = nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, set);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return WrapListsInRedirector(aBuilder, set, aLists);
}

NS_IMETHODIMP
nsMenuFrame::HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  nsWeakFrame weakFrame(this);
  if (*aEventStatus == nsEventStatus_eIgnore)
    *aEventStatus = nsEventStatus_eConsumeDoDefault;

  PRBool onmenu = IsOnMenu();

  if (aEvent->message == NS_KEY_PRESS && !IsDisabled()) {
    nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
    PRUint32 keyCode = keyEvent->keyCode;
#ifdef XP_MACOSX
    
    if (!IsOpen() && ((keyEvent->charCode == NS_VK_SPACE && !keyEvent->isMeta) ||
        (keyCode == NS_VK_UP || keyCode == NS_VK_DOWN))) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
      OpenMenu(PR_FALSE);
    }
#else
    
    if ((keyCode == NS_VK_F4 && !keyEvent->isAlt) ||
        ((keyCode == NS_VK_UP || keyCode == NS_VK_DOWN) && keyEvent->isAlt)) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
      ToggleMenuState();
    }
#endif
  }
  else if (aEvent->eventStructType == NS_MOUSE_EVENT &&
           aEvent->message == NS_MOUSE_BUTTON_DOWN &&
           static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton &&
           !IsDisabled() && IsMenu()) {
    
    
    
    
    if (!mMenuParent || mMenuParent->IsMenuBar()) {
      ToggleMenuState();
    }
    else {
      if (!IsOpen()) {
        OpenMenu(PR_FALSE);
      }
    }
  }
  else if (
#ifndef NSCONTEXTMENUISMOUSEUP
           (aEvent->eventStructType == NS_MOUSE_EVENT &&
            aEvent->message == NS_MOUSE_BUTTON_UP &&
            static_cast<nsMouseEvent*>(aEvent)->button ==
              nsMouseEvent::eRightButton) &&
#else
            aEvent->message == NS_CONTEXTMENU &&
#endif
            onmenu && !IsMenu() && !IsDisabled()) {
    
    
    
    
    
    
    
    
    
    
    if (mMenuParent->IsContextMenu()) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
      Execute(aEvent);
    }
  }
  else if (aEvent->eventStructType == NS_MOUSE_EVENT &&
           aEvent->message == NS_MOUSE_BUTTON_UP &&
           static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton &&
           !IsMenu() && !IsDisabled()) {
    
    *aEventStatus = nsEventStatus_eConsumeNoDefault;
    Execute(aEvent);
  }
  else if (aEvent->message == NS_MOUSE_EXIT_SYNTH) {
    
    if (mOpenTimer) {
      mOpenTimer->Cancel();
      mOpenTimer = nsnull;
    }

    
    if (mMenuParent) {
      PRBool onmenubar = mMenuParent->IsMenuBar();
      if (!(onmenubar && mMenuParent->IsActive())) {
        if (IsMenu() && !onmenubar && IsOpen()) {
          
        }
        else if (this == mMenuParent->GetCurrentMenuItem()) {
          mMenuParent->ChangeMenuItem(nsnull, PR_FALSE);
        }
      }
    }
  }
  else if (aEvent->message == NS_MOUSE_MOVE &&
           (onmenu || (mMenuParent && mMenuParent->IsMenuBar()))) {
    if (gEatMouseMove) {
      gEatMouseMove = PR_FALSE;
      return NS_OK;
    }

    
    mMenuParent->ChangeMenuItem(this, PR_FALSE);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    NS_ENSURE_TRUE(mMenuParent, NS_OK);

    
    
    nsMenuFrame *realCurrentItem = mMenuParent->GetCurrentMenuItem();
    if (realCurrentItem != this) {
      
      return NS_OK;
    }

    
    
    
    
    if (!IsDisabled() && IsMenu() && !IsOpen() && !mOpenTimer && !mMenuParent->IsMenuBar()) {
      PRInt32 menuDelay = 300;   

      nsCOMPtr<nsILookAndFeel> lookAndFeel(do_GetService(kLookAndFeelCID));
      if (lookAndFeel)
        lookAndFeel->GetMetric(nsILookAndFeel::eMetric_SubmenuDelay, menuDelay);

      
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
    CloseMenu(PR_FALSE);
  else
    OpenMenu(PR_FALSE);
}

void
nsMenuFrame::PopupOpened()
{
  nsWeakFrame weakFrame(this);
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::open,
                    NS_LITERAL_STRING("true"), PR_TRUE);
  if (!weakFrame.IsAlive())
    return;

  if (mMenuParent) {
    mMenuParent->SetActive(PR_TRUE);
    
    
    mMenuParent->SetCurrentMenuItem(this);
  }
}

void
nsMenuFrame::PopupClosed(PRBool aDeselectMenu)
{
  nsWeakFrame weakFrame(this);
  nsContentUtils::AddScriptRunner(
    new nsUnsetAttrRunnable(mContent, nsGkAtoms::open));
  if (!weakFrame.IsAlive())
    return;

  
  if (mMenuParent && mMenuParent->MenuClosed()) {
    if (aDeselectMenu) {
      SelectMenu(PR_FALSE);
    } else {
      
      
      
      nsMenuFrame *current = mMenuParent->GetCurrentMenuItem();
      if (current) {
        nsCOMPtr<nsIRunnable> event =
          new nsMenuActivateEvent(current->GetContent(),
                                  PresContext(), PR_TRUE);
        NS_DispatchToCurrentThread(event);
      }
    }
  }
}

NS_IMETHODIMP
nsMenuFrame::SelectMenu(PRBool aActivateFlag)
{
  if (mContent) {
    
    
    
    
    
    if (aActivateFlag) {
      nsIFrame* parent = GetParent();
      while (parent) {
        if (parent->GetType() == nsGkAtoms::menuPopupFrame) {
          
          parent = parent->GetParent();
          if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
            
            nsIFrame* popupParent = parent->GetParent();
            while (popupParent) {
              if (popupParent->GetType() == nsGkAtoms::menuPopupFrame) {
                nsMenuPopupFrame* popup = static_cast<nsMenuPopupFrame *>(popupParent);
                popup->SetCurrentMenuItem(static_cast<nsMenuFrame *>(parent));
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
    if (pm)
      pm->CancelMenuTimer(mMenuParent);

    nsCOMPtr<nsIRunnable> event =
      new nsMenuActivateEvent(mContent, PresContext(), aActivateFlag);
    NS_DispatchToCurrentThread(event);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType)
{
  nsAutoString value;

  if (aAttribute == nsGkAtoms::checked) {
    if (mType != eMenuType_Normal)
        UpdateMenuSpecialState(PresContext());
  } else if (aAttribute == nsGkAtoms::acceltext) {
    
    AddStateBits(NS_STATE_ACCELTEXT_IS_DERIVED);
    BuildAcceleratorText();
  } else if (aAttribute == nsGkAtoms::key) {
    BuildAcceleratorText();
  } else if (aAttribute == nsGkAtoms::type || aAttribute == nsGkAtoms::name)
    UpdateMenuType(PresContext());

  return NS_OK;
}

void
nsMenuFrame::OpenMenu(PRBool aSelectFirstItem)
{
  if (!mContent)
    return;

  gEatMouseMove = PR_TRUE;

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    pm->KillMenuTimer();
    
    pm->ShowMenu(mContent, aSelectFirstItem, PR_TRUE);
  }
}

void
nsMenuFrame::CloseMenu(PRBool aDeselectMenu)
{
  gEatMouseMove = PR_TRUE;

  
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mPopupFrame)
    pm->HidePopup(mPopupFrame->GetContent(), PR_FALSE, aDeselectMenu, PR_TRUE);
}

PRBool
nsMenuFrame::IsSizedToPopup(nsIContent* aContent, PRBool aRequireAlways)
{
  PRBool sizeToPopup;
  if (aContent->Tag() == nsGkAtoms::select)
    sizeToPopup = PR_TRUE;
  else {
    nsAutoString sizedToPopup;
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::sizetopopup, sizedToPopup);
    sizeToPopup = sizedToPopup.EqualsLiteral("always") ||
                  (!aRequireAlways && sizedToPopup.EqualsLiteral("pref"));
  }
  
  return sizeToPopup;
}

nsSize
nsMenuFrame::GetMinSize(nsBoxLayoutState& aBoxLayoutState)
{
  nsSize size = nsBoxFrame::GetMinSize(aBoxLayoutState);
  DISPLAY_MIN_SIZE(this, size);

  if (IsSizedToPopup(mContent, PR_TRUE))
    SizeToPopup(aBoxLayoutState, size);

  return size;
}

NS_IMETHODIMP
nsMenuFrame::DoLayout(nsBoxLayoutState& aState)
{
  
  nsresult rv = nsBoxFrame::DoLayout(aState);

  
  if (mPopupFrame) {
    PRBool sizeToPopup = IsSizedToPopup(mContent, PR_FALSE);
    
    nsSize prefSize = mPopupFrame->GetPrefSize(aState);
    nsSize minSize = mPopupFrame->GetMinSize(aState); 
    nsSize maxSize = mPopupFrame->GetMaxSize(aState);

    prefSize = BoundsCheck(minSize, prefSize, maxSize);

    if (sizeToPopup)
        prefSize.width = mRect.width;

    
    PRBool sizeChanged = (mPopupFrame->PreferredSize() != prefSize);
    if (sizeChanged) {
      mPopupFrame->SetPreferredBounds(aState, nsRect(0,0,prefSize.width, prefSize.height));
    }

    
    
    
    if (IsOpen() && (sizeChanged || mPopupFrame->HasOpenChanged()))
      mPopupFrame->SetPopupPosition(this);

    
    nsIBox* child = mPopupFrame->GetChildBox();

    nsRect bounds(mPopupFrame->GetRect());

    nsIScrollableFrame *scrollframe = do_QueryFrame(child);
    if (scrollframe &&
        scrollframe->GetScrollbarStyles().mVertical == NS_STYLE_OVERFLOW_AUTO) {
      if (bounds.height < prefSize.height) {
        
        mPopupFrame->Layout(aState);

        nsMargin scrollbars = scrollframe->GetActualScrollbarSizes();
        if (bounds.width < prefSize.width + scrollbars.left + scrollbars.right)
        {
          bounds.width += scrollbars.left + scrollbars.right;
          mPopupFrame->SetBounds(aState, bounds);
        }
      }
    }

    
    mPopupFrame->Layout(aState);
    mPopupFrame->AdjustView();
  }

  return rv;
}

#ifdef DEBUG_LAYOUT
NS_IMETHODIMP
nsMenuFrame::SetDebug(nsBoxLayoutState& aState, PRBool aDebug)
{
  
  PRBool debugSet = mState & NS_STATE_CURRENTLY_IN_DEBUG;
  PRBool debugChanged = (!aDebug && debugSet) || (aDebug && !debugSet);

  
  if (debugChanged)
  {
      nsBoxFrame::SetDebug(aState, aDebug);
      if (mPopupFrame)
        SetDebug(aState, mPopupFrame, aDebug);
  }

  return NS_OK;
}

nsresult
nsMenuFrame::SetDebug(nsBoxLayoutState& aState, nsIFrame* aList, PRBool aDebug)
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
nsMenuFrame::Enter()
{
  if (IsDisabled()) {
#ifdef XP_WIN
    
    if (mMenuParent) {
      nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
      if (pm) {
        nsIFrame* popup = pm->GetTopPopup(ePopupTypeAny);
        if (popup)
          pm->HidePopup(popup->GetContent(), PR_TRUE, PR_TRUE, PR_TRUE);
      }
    }
#endif   
    
    return nsnull;
  }

  if (!IsOpen()) {
    
    if (!IsMenu() && mMenuParent)
      Execute(0);          
    else
      return this;
  }

  return nsnull;
}

PRBool
nsMenuFrame::IsOpen()
{
  return mPopupFrame && mPopupFrame->IsOpen();
}

PRBool
nsMenuFrame::IsMenu()
{
  return mIsMenu;
}

nsresult
nsMenuFrame::Notify(nsITimer* aTimer)
{
  
  if (aTimer == mOpenTimer.get()) {
    mOpenTimer = nsnull;

    if (!IsOpen() && mMenuParent) {
      
      
      nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
      if (pm) {
        if ((!pm->HasContextMenu(nsnull) || mMenuParent->IsContextMenu()) &&
            mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menuactive,
                                  nsGkAtoms::_true, eCaseMatters)) {
          OpenMenu(PR_FALSE);
        }
      }
    }
  }

  return NS_OK;
}

PRBool 
nsMenuFrame::IsDisabled()
{
  return mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                               nsGkAtoms::_true, eCaseMatters);
}

void
nsMenuFrame::UpdateMenuType(nsPresContext* aPresContext)
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::checkbox, &nsGkAtoms::radio, nsnull};
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
                            PR_TRUE);
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
  PRBool newChecked =
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

  





  










  
  
  
  
  nsIFrame* sib = GetParent()->GetFirstChild(nsnull);

  while (sib) {
    if (sib != this && sib->GetType() == nsGkAtoms::menuFrame) {
      nsMenuFrame* menu = static_cast<nsMenuFrame*>(sib);
      if (menu->GetMenuType() == eMenuType_Radio &&
          menu->IsChecked() &&
          (menu->GetRadioGroupName() == mGroupName)) {      
        
        sib->GetContent()->UnsetAttr(kNameSpaceID_None, nsGkAtoms::checked,
                                     PR_TRUE);
        
        return;
      }
    }

    sib = sib->GetNextSibling();
  } 
}

void 
nsMenuFrame::BuildAcceleratorText()
{
  nsAutoString accelText;

  if ((GetStateBits() & NS_STATE_ACCELTEXT_IS_DERIVED) == 0) {
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::acceltext, accelText);
    if (!accelText.IsEmpty())
      return;
  }
  

  
  AddStateBits(NS_STATE_ACCELTEXT_IS_DERIVED);

  
  nsWeakFrame weakFrame(this);
  mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::acceltext, PR_FALSE);
  ENSURE_TRUE(weakFrame.IsAlive());

  
  nsAutoString keyValue;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::key, keyValue);
  if (keyValue.IsEmpty())
    return;

  
  nsCOMPtr<nsIDOMDocument> domDocument(do_QueryInterface(mContent->GetDocument()));
  if (!domDocument)
    return;

  nsCOMPtr<nsIDOMElement> keyDOMElement;
  domDocument->GetElementById(keyValue, getter_AddRefs(keyDOMElement));
  if (!keyDOMElement) {
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

  nsCOMPtr<nsIContent> keyElement(do_QueryInterface(keyDOMElement));
  if (!keyElement)
    return;

  
  
  
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
      nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
      if (NS_SUCCEEDED(rv) && bundleService) {
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

  static PRInt32 accelKey = 0;

  if (!accelKey)
  {
    
    
#ifdef XP_MACOSX
    accelKey = nsIDOMKeyEvent::DOM_VK_META;
#else
    accelKey = nsIDOMKeyEvent::DOM_VK_CONTROL;
#endif

    
    accelKey = nsContentUtils::GetIntPref("ui.key.accelKey", accelKey);
  }

  nsAutoString modifiers;
  keyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::modifiers, modifiers);
  
  char* str = ToNewCString(modifiers);
  char* newStr;
  char* token = nsCRT::strtok(str, ", \t", &newStr);
  while (token) {
      
    if (PL_strcmp(token, "shift") == 0)
      accelText += *gShiftText;
    else if (PL_strcmp(token, "alt") == 0) 
      accelText += *gAltText; 
    else if (PL_strcmp(token, "meta") == 0) 
      accelText += *gMetaText; 
    else if (PL_strcmp(token, "control") == 0) 
      accelText += *gControlText; 
    else if (PL_strcmp(token, "accel") == 0) {
      switch (accelKey)
      {
        case nsIDOMKeyEvent::DOM_VK_META:
          accelText += *gMetaText;
          break;

        case nsIDOMKeyEvent::DOM_VK_ALT:
          accelText += *gAltText;
          break;

        case nsIDOMKeyEvent::DOM_VK_CONTROL:
        default:
          accelText += *gControlText;
          break;
      }
    }
    
    accelText += *gModifierSeparator;

    token = nsCRT::strtok(newStr, ", \t", &newStr);
  }

  nsMemory::Free(str);

  accelText += accelString;
  
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::acceltext, accelText, PR_FALSE);
}

void
nsMenuFrame::Execute(nsGUIEvent *aEvent)
{
  nsWeakFrame weakFrame(this);
  
  if (mType == eMenuType_Checkbox || (mType == eMenuType_Radio && !mChecked)) {
    if (!mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::autocheck,
                               nsGkAtoms::_false, eCaseMatters)) {
      if (mChecked) {
        mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::checked,
                            PR_TRUE);
        ENSURE_TRUE(weakFrame.IsAlive());
      }
      else {
        mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::checked, NS_LITERAL_STRING("true"),
                          PR_TRUE);
        ENSURE_TRUE(weakFrame.IsAlive());
      }
    }
  }

  nsCOMPtr<nsISound> sound(do_CreateInstance("@mozilla.org/sound;1"));
  if (sound)
    sound->PlayEventSound(nsISound::EVENT_MENU_EXECUTE);

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && mMenuParent)
    pm->ExecuteMenu(mContent, aEvent);
}

NS_IMETHODIMP
nsMenuFrame::RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame)
{
  nsresult rv =  NS_OK;

  if (mPopupFrame == aOldFrame) {
    
    mPopupFrame->Destroy();
    mPopupFrame = nsnull;
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    rv = NS_OK;
  } else {
    rv = nsBoxFrame::RemoveFrame(aListName, aOldFrame);
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList)
{
  if (!mPopupFrame && (!aListName || aListName == nsGkAtoms::popupList)) {
    SetPopupFrame(aFrameList);
    if (mPopupFrame) {
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
    return NS_OK;

  if (NS_UNLIKELY(aPrevFrame == mPopupFrame)) {
    aPrevFrame = nsnull;
  }

  return nsBoxFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
}

NS_IMETHODIMP
nsMenuFrame::AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList)
{
  if (!mPopupFrame && (!aListName || aListName == nsGkAtoms::popupList)) {
    SetPopupFrame(aFrameList);
    if (mPopupFrame) {

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
    return NS_OK;

  return nsBoxFrame::AppendFrames(aListName, aFrameList); 
}

PRBool
nsMenuFrame::SizeToPopup(nsBoxLayoutState& aState, nsSize& aSize)
{
  if (!IsCollapsed(aState)) {
    nsSize tmpSize(-1, 0);
    nsIBox::AddCSSPrefSize(aState, this, tmpSize);
    if (tmpSize.width == -1 && GetFlex(aState) == 0) {
      if (!mPopupFrame)
        return PR_FALSE;
      tmpSize = mPopupFrame->GetPrefSize(aState);
      aSize.width = tmpSize.width;
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsSize
nsMenuFrame::GetPrefSize(nsBoxLayoutState& aState)
{
  nsSize size = nsBoxFrame::GetPrefSize(aState);
  DISPLAY_PREF_SIZE(this, size);

  
  
  if (!IsSizedToPopup(mContent, PR_TRUE) &&
      IsSizedToPopup(mContent, PR_FALSE) &&
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
  if (!mPopupFrame)
    return NS_ERROR_FAILURE;

  nsMenuFrame* menuFrame = mPopupFrame->GetCurrentMenuItem();
  if (!menuFrame) {
    *aResult = nsnull;
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
  if (!mPopupFrame)
    return NS_ERROR_FAILURE;

  if (!aChild) {
    
    mPopupFrame->ChangeMenuItem(nsnull, PR_FALSE);
    return NS_OK;
  }

  nsCOMPtr<nsIContent> child(do_QueryInterface(aChild));

  nsIFrame* kid = PresContext()->PresShell()->GetPrimaryFrameFor(child);
  if (kid && kid->GetType() == nsGkAtoms::menuFrame)
    mPopupFrame->ChangeMenuItem(static_cast<nsMenuFrame *>(kid), PR_FALSE);
  return NS_OK;
}

nsIScrollableView* nsMenuFrame::GetScrollableView()
{
  if (!mPopupFrame)
    return nsnull;

  nsIFrame* childFrame = mPopupFrame->GetFirstChild(nsnull);
  if (childFrame)
    return mPopupFrame->GetScrollableView(childFrame);
  return nsnull;
}


NS_IMPL_ISUPPORTS1(nsMenuTimerMediator, nsITimerCallback)





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
  mFrame = nsnull;
}
