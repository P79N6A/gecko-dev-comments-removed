








































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
#include "nsIWidget.h"
#include "nsIDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsISupportsArray.h"
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
#include "nsIScrollableView.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIStringBundle.h"
#include "nsGUIEvent.h"
#include "nsIEventStateManager.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsIReflowCallback.h"

#define NS_MENU_POPUP_LIST_INDEX   0

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






nsIFrame*
NS_NewMenuFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags)
{
  nsMenuFrame* it = new (aPresShell) nsMenuFrame (aPresShell, aContext);
  
  if ((it != nsnull) && aFlags)
    it->SetIsMenu(PR_TRUE);

  return it;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsMenuFrame::Release(void)
{
    return NS_OK;
}




NS_INTERFACE_MAP_BEGIN(nsMenuFrame)
  NS_INTERFACE_MAP_ENTRY(nsIMenuFrame)
  NS_INTERFACE_MAP_ENTRY(nsIScrollableViewProvider)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)




nsMenuFrame::nsMenuFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsBoxFrame(aShell, aContext),
    mIsMenu(PR_FALSE),
    mMenuOpen(PR_FALSE),
    mCreateHandlerSucceeded(PR_FALSE),
    mChecked(PR_FALSE),
    mType(eMenuType_Normal),
    mMenuParent(nsnull),
    mLastPref(-1,-1)
{

} 

NS_IMETHODIMP
nsMenuFrame::SetParent(const nsIFrame* aParent)
{
  nsBoxFrame::SetParent(aParent);
  const nsIFrame* currFrame = aParent;
  while (!mMenuParent && currFrame) {
    
    CallQueryInterface(NS_CONST_CAST(nsIFrame*, currFrame), &mMenuParent);

    currFrame = currFrame->GetParent();
  }

  return NS_OK;
}

class nsASyncMenuInitialization : public nsIReflowCallback
{
public:
  nsASyncMenuInitialization(nsIFrame* aFrame)
    : mWeakFrame(aFrame)
  {
  }

  virtual PRBool ReflowFinished() {
    PRBool shouldFlush = PR_FALSE;
    if (mWeakFrame.IsAlive()) {
      nsIMenuFrame* imenu = nsnull;
      CallQueryInterface(mWeakFrame.GetFrame(), &imenu);
      if (imenu) {
        nsMenuFrame* menu = NS_STATIC_CAST(nsMenuFrame*, imenu);
        menu->UpdateMenuType(menu->GetPresContext());
        shouldFlush = PR_TRUE;
      }
    }
    delete this;
    return shouldFlush;
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

  nsIFrame* currFrame = aParent;
  while (!mMenuParent && currFrame) {
    
    CallQueryInterface(currFrame, &mMenuParent);

    currFrame = currFrame->GetParent();
  }

  
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
  GetPresContext()->PresShell()->PostReflowCallback(cb);
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



nsIFrame*
nsMenuFrame::GetFirstChild(nsIAtom* aListName) const
{
  if (nsGkAtoms::popupList == aListName) {
    return mPopupFrames.FirstChild();
  }
  return nsBoxFrame::GetFirstChild(aListName);
}

NS_IMETHODIMP
nsMenuFrame::SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;
  if (nsGkAtoms::popupList == aListName) {
    mPopupFrames.SetFrames(aChildList);
  } else {

    nsFrameList frames(aChildList);

    
    
    nsIFrame* frame = frames.FirstChild();
    while (frame) {
      nsIMenuParent *menuPar;
      CallQueryInterface(frame, &menuPar);
      if (menuPar) {
        PRBool isMenuBar;
        menuPar->IsMenuBar(isMenuBar);
        if (!isMenuBar) {
          
          frames.RemoveFrame(frame);
          mPopupFrames.AppendFrame(this, frame);
          nsIFrame* first = frames.FirstChild();
          rv = nsBoxFrame::SetInitialChildList(aListName, first);
          return rv;
        }
      }
      frame = frame->GetNextSibling();
    }

    
    rv = nsBoxFrame::SetInitialChildList(aListName, aChildList);
  }
  return rv;
}

nsIAtom*
nsMenuFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (NS_MENU_POPUP_LIST_INDEX == aIndex) {
    return nsGkAtoms::popupList;
  }

  return nsnull;
}

nsresult
nsMenuFrame::DestroyPopupFrames(nsPresContext* aPresContext)
{
  
  nsCSSFrameConstructor* frameConstructor =
    aPresContext->PresShell()->FrameConstructor();
  nsIFrame* curFrame = mPopupFrames.FirstChild();
  while (curFrame) {
    frameConstructor->RemoveMappingsForFrameSubtree(curFrame);
    curFrame = curFrame->GetNextSibling();
  }

   
  mPopupFrames.DestroyFrames();
  return NS_OK;
}

void
nsMenuFrame::Destroy()
{
  
  
  
  if (mOpenTimer) {
    mOpenTimer->Cancel();
  }

  
  
  mTimerMediator->ClearFrame();

  nsWeakFrame weakFrame(this);
  
  if (mMenuParent) {
    nsIMenuFrame *curItem = mMenuParent->GetCurrentMenuItem();
    if (curItem == this) {
      
      mMenuParent->SetCurrentMenuItem(nsnull);
      ENSURE_TRUE(weakFrame.IsAlive());
    }
  }

  UngenerateMenu();
  ENSURE_TRUE(weakFrame.IsAlive());
  DestroyPopupFrames(GetPresContext());
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
  nsWeakFrame weakFrame(this);
  if (*aEventStatus == nsEventStatus_eIgnore)
    *aEventStatus = nsEventStatus_eConsumeDoDefault;
  
  if (aEvent->message == NS_KEY_PRESS && !IsDisabled()) {
    nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
    PRUint32 keyCode = keyEvent->keyCode;
#ifdef XP_MACOSX
    
    if (!IsOpen() && ((keyEvent->charCode == NS_VK_SPACE && !keyEvent->isMeta) ||
        (keyCode == NS_VK_UP || keyCode == NS_VK_DOWN)))
      OpenMenu(PR_TRUE);
#else
    
    if ((keyCode == NS_VK_F4 && !keyEvent->isAlt) ||
        ((keyCode == NS_VK_UP || keyCode == NS_VK_DOWN) && keyEvent->isAlt))
      OpenMenu(!IsOpen());
#endif
  }
  else if (aEvent->eventStructType == NS_MOUSE_EVENT &&
           aEvent->message == NS_MOUSE_BUTTON_DOWN &&
           NS_STATIC_CAST(nsMouseEvent*, aEvent)->button == nsMouseEvent::eLeftButton &&
           !IsDisabled() && IsMenu()) {
    PRBool isMenuBar = PR_FALSE;
    if (mMenuParent)
      mMenuParent->IsMenuBar(isMenuBar);

    
    
    if ( isMenuBar || !mMenuParent ) {
      ToggleMenuState();
      NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);

      if (!IsOpen() && mMenuParent) {
        
        
        mMenuParent->SetActive(PR_FALSE);
      }
    }
    else
      if ( !IsOpen() ) {
        
        
        
        
        if ( mMenuParent )
          mMenuParent->KillPendingTimers();

        
        OpenMenu(PR_TRUE);
      }
  }
  else if (
#ifndef NSCONTEXTMENUISMOUSEUP
           (aEvent->eventStructType == NS_MOUSE_EVENT &&
            aEvent->message == NS_MOUSE_BUTTON_UP &&
            NS_STATIC_CAST(nsMouseEvent*, aEvent)->button ==
              nsMouseEvent::eRightButton) &&
#else
            aEvent->message == NS_CONTEXTMENU &&
#endif
            mMenuParent && !IsMenu() && !IsDisabled()) {
    
    
    
    
    
    
    
    
    
    
    PRBool isContextMenu = PR_FALSE;
    mMenuParent->GetIsContextMenu(isContextMenu);
    if ( isContextMenu ) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
      Execute(aEvent);
    }
  }
  else if (aEvent->eventStructType == NS_MOUSE_EVENT &&
           aEvent->message == NS_MOUSE_BUTTON_UP &&
           NS_STATIC_CAST(nsMouseEvent*, aEvent)->button == nsMouseEvent::eLeftButton &&
           !IsMenu() && mMenuParent && !IsDisabled()) {
    
    Execute(aEvent);
  }
  else if (aEvent->message == NS_MOUSE_EXIT_SYNTH) {
    
    if (mOpenTimer) {
      mOpenTimer->Cancel();
      mOpenTimer = nsnull;
    }

    
    PRBool isActive = PR_FALSE;
    PRBool isMenuBar = PR_FALSE;
    if (mMenuParent) {
      mMenuParent->IsMenuBar(isMenuBar);
      PRBool cancel = PR_TRUE;
      if (isMenuBar) {
        mMenuParent->GetIsActive(isActive);
        if (isActive) cancel = PR_FALSE;
      }
      
      if (cancel) {
        if (IsMenu() && !isMenuBar && mMenuOpen) {
          
        }
        else mMenuParent->SetCurrentMenuItem(nsnull);
      }
    }
  }
  else if (aEvent->message == NS_MOUSE_MOVE && mMenuParent) {
    if (gEatMouseMove) {
      gEatMouseMove = PR_FALSE;
      return NS_OK;
    }

    

    PRBool isMenuBar = PR_FALSE;
    mMenuParent->IsMenuBar(isMenuBar);

    
    mMenuParent->SetCurrentMenuItem(this);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    NS_ENSURE_TRUE(mMenuParent, NS_OK);
    
    
    
    nsIMenuFrame *realCurrentItem = mMenuParent->GetCurrentMenuItem();
    if (realCurrentItem != this) {
      
      return NS_OK;
    }

    
    
    if (!IsDisabled() && !isMenuBar && IsMenu() && !mMenuOpen && !mOpenTimer) {

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

NS_IMETHODIMP
nsMenuFrame::ToggleMenuState()
{
  nsWeakFrame weakFrame(this);
  if (mMenuOpen) {
    OpenMenu(PR_FALSE);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
  }
  else {
    PRBool justRolledUp = PR_FALSE;
    if (mMenuParent) {
      mMenuParent->RecentlyRolledUp(this, &justRolledUp);
    }
    if (justRolledUp) {
      
      
      
      OpenMenu(PR_FALSE);
      NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
      SelectMenu(PR_TRUE);
      NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
      NS_ENSURE_TRUE(mMenuParent, NS_OK);
      mMenuParent->SetActive(PR_FALSE);
    }
    else {
      if (mMenuParent) {
        mMenuParent->SetActive(PR_TRUE);
        NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
      }
      OpenMenu(PR_TRUE);
    }
  }
  NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);

  if (mMenuParent) {
    
    
    mMenuParent->SetCurrentMenuItem(this);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    NS_ENSURE_TRUE(mMenuParent, NS_OK);
    
    
    
    
    mMenuParent->ClearRecentlyRolledUp();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SelectMenu(PRBool aActivateFlag)
{
  if (!mContent) {
    return NS_OK;
  }

  nsAutoString domEventToFire;

  nsWeakFrame weakFrame(this);
  if (aActivateFlag) {
    if (mMenuParent) {
      nsIMenuParent* ancestor = nsnull;
      nsresult rv = mMenuParent->GetParentPopup(&ancestor);
      while (NS_SUCCEEDED(rv) && ancestor) {
        ancestor->CancelPendingTimers();
        rv = ancestor->GetParentPopup(&ancestor);
      }
    }
    
    mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::menuactive, NS_LITERAL_STRING("true"), PR_TRUE);
    
    domEventToFire.AssignLiteral("DOMMenuItemActive");
  }
  else {
    
    mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menuactive, PR_TRUE);
    domEventToFire.AssignLiteral("DOMMenuItemInactive");
  }

  if (weakFrame.IsAlive()) {
    FireDOMEventSynch(domEventToFire);
  }
  return NS_OK;
}

PRBool nsMenuFrame::IsGenerated()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  
  
  
  if (child &&
      !nsContentUtils::HasNonEmptyAttr(child, kNameSpaceID_None,
                                       nsGkAtoms::menugenerated)) {
    return PR_FALSE;
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsMenuFrame::MarkAsGenerated()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  
  
  
  if (child &&
      !nsContentUtils::HasNonEmptyAttr(child, kNameSpaceID_None,
                                       nsGkAtoms::menugenerated)) {
    child->SetAttr(kNameSpaceID_None, nsGkAtoms::menugenerated,
                   NS_LITERAL_STRING("true"), PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::UngenerateMenu()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  if (child &&
      nsContentUtils::HasNonEmptyAttr(child, kNameSpaceID_None,
                                      nsGkAtoms::menugenerated)) {
    child->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menugenerated, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::ActivateMenu(PRBool aActivateFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  
  if (!menuPopup) 
    return NS_OK;

  if (aActivateFlag) {
      nsRect rect = menuPopup->GetRect();
      nsIView* view = menuPopup->GetView();
      nsIViewManager* viewManager = view->GetViewManager();
      rect.x = rect.y = 0;
      viewManager->ResizeView(view, rect);

      
      if (mLastPref.height <= rect.height) {
        nsIBox* child = menuPopup->GetChildBox();

        nsCOMPtr<nsIScrollableFrame> scrollframe(do_QueryInterface(child));
        if (scrollframe) {
          scrollframe->ScrollTo(nsPoint(0,0));
        }
      }

      viewManager->UpdateView(view, rect, NS_VMREFRESH_IMMEDIATE);
      viewManager->SetViewVisibility(view, nsViewVisibility_kShow);
      GetPresContext()->RootPresContext()->NotifyAddedActivePopupToTop(menuPopup);
  } else {
    if (mMenuOpen) {
      nsWeakFrame weakFrame(this);
      nsWeakFrame weakPopup(menuPopup);
      FireDOMEventSynch(NS_LITERAL_STRING("DOMMenuInactive"), menuPopup->GetContent());
      NS_ENSURE_TRUE(weakFrame.IsAlive() && weakPopup.IsAlive(), NS_OK);
    }
    nsIView* view = menuPopup->GetView();
    NS_ASSERTION(view, "View is gone, looks like someone forgot to rollup the popup!");
    if (view) {
      nsIViewManager* viewManager = view->GetViewManager();
      if (viewManager) { 
        viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
        viewManager->ResizeView(view, nsRect(0, 0, 0, 0));
      }
    }
    
    mMenuOpen = PR_FALSE;
    GetPresContext()->RootPresContext()->NotifyRemovedActivePopup(menuPopup);
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
        UpdateMenuSpecialState(GetPresContext());
  } else if (aAttribute == nsGkAtoms::acceltext) {
    
    AddStateBits(NS_STATE_ACCELTEXT_IS_DERIVED);
    BuildAcceleratorText();
  } else if (aAttribute == nsGkAtoms::key) {
    BuildAcceleratorText();
  } else if ( aAttribute == nsGkAtoms::type || aAttribute == nsGkAtoms::name )
    UpdateMenuType(GetPresContext());

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::OpenMenu(PRBool aActivateFlag)
{
  if (!mContent)
    return NS_OK;

  nsWeakFrame weakFrame(this);
  if (aActivateFlag) {
    
    
    MarkAsGenerated();
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);

    mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::open, NS_LITERAL_STRING("true"), PR_TRUE);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    FireDOMEventSynch(NS_LITERAL_STRING("DOMMenuItemActive"));
  }
  else {
    mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::open, PR_TRUE);
  }

  NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
  OpenMenuInternal(aActivateFlag);

  return NS_OK;
}

void 
nsMenuFrame::OpenMenuInternal(PRBool aActivateFlag) 
{
  gEatMouseMove = PR_TRUE;

  if (!mIsMenu)
    return;

  nsPresContext* presContext = GetPresContext();
  nsWeakFrame weakFrame(this);

  if (aActivateFlag) {
    
    if (!OnCreate() || !weakFrame.IsAlive())
      return;

    mCreateHandlerSucceeded = PR_TRUE;
  
    
    if (nsMenuDismissalListener::sInstance)
      nsMenuDismissalListener::sInstance->EnableListener(PR_FALSE);
    
    
    MarkAsGenerated();
    ENSURE_TRUE(weakFrame.IsAlive());

    nsIFrame* frame = mPopupFrames.FirstChild();
    nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
    
    PRBool wasOpen = mMenuOpen;
    mMenuOpen = PR_TRUE;

    if (menuPopup) {
      nsWeakFrame weakMenuPopup(frame);
      
      if ( mMenuParent ) {
        PRBool parentIsContextMenu = PR_FALSE;
        mMenuParent->GetIsContextMenu(parentIsContextMenu);
        menuPopup->SetIsContextMenu(parentIsContextMenu);
        ENSURE_TRUE(weakFrame.IsAlive());
      }

      
      PRBool onMenuBar = PR_TRUE;
      if (mMenuParent)
        mMenuParent->IsMenuBar(onMenuBar);

      if (mMenuParent && onMenuBar)
        mMenuParent->InstallKeyboardNavigator();
      else if (!mMenuParent) {
        ENSURE_TRUE(weakMenuPopup.IsAlive());
        menuPopup->InstallKeyboardNavigator();
      }
      
      
      if (mMenuParent) {
        mMenuParent->SetActive(PR_TRUE);
        ENSURE_TRUE(weakFrame.IsAlive());
      }

      nsIContent* menuPopupContent = menuPopup->GetContent();

      
      nsAutoString popupAnchor, popupAlign;
      
      menuPopupContent->GetAttr(kNameSpaceID_None, nsGkAtoms::popupanchor, popupAnchor);
      menuPopupContent->GetAttr(kNameSpaceID_None, nsGkAtoms::popupalign, popupAlign);

      ConvertPosition(menuPopupContent, popupAnchor, popupAlign);

      if (onMenuBar) {
        if (popupAnchor.IsEmpty())
          popupAnchor.AssignLiteral("bottomleft");
        if (popupAlign.IsEmpty())
          popupAlign.AssignLiteral("topleft");
      }
      else {
        if (popupAnchor.IsEmpty())
          popupAnchor.AssignLiteral("topright");
        if (popupAlign.IsEmpty())
          popupAlign.AssignLiteral("topleft");
      }

      
      
      
      
      if (!wasOpen)
      {
         menuPopup->AddStateBits(NS_FRAME_IS_DIRTY);
         presContext->PresShell()->
           FrameNeedsReflow(menuPopup, nsIPresShell::eStyleChange);
         presContext->PresShell()->FlushPendingNotifications(Flush_OnlyReflow);
      }

      nsRect curRect(menuPopup->GetRect());
      nsBoxLayoutState state(presContext);
      menuPopup->SetBounds(state, nsRect(0,0,mLastPref.width, mLastPref.height));

      nsIView* view = menuPopup->GetView();
      nsIViewManager* vm = view->GetViewManager();
      if (vm) {
        vm->SetViewVisibility(view, nsViewVisibility_kHide);
      }
      menuPopup->SyncViewWithFrame(presContext, popupAnchor, popupAlign, this, -1, -1);
      nscoord newHeight = menuPopup->GetRect().height;

      
      if (curRect.height != newHeight || mLastPref.height != newHeight)
      {
         menuPopup->AddStateBits(NS_FRAME_IS_DIRTY);
         presContext->PresShell()->
           FrameNeedsReflow(menuPopup, nsIPresShell::eStyleChange);
         presContext->PresShell()->FlushPendingNotifications(Flush_OnlyReflow);
      }

      ActivateMenu(PR_TRUE);
      ENSURE_TRUE(weakFrame.IsAlive());

      nsIMenuParent *childPopup = nsnull;
      CallQueryInterface(frame, &childPopup);

      nsMenuDismissalListener* listener = nsMenuDismissalListener::GetInstance();
      if (listener)
        listener->SetCurrentMenuParent(childPopup);

      OnCreated();
      ENSURE_TRUE(weakFrame.IsAlive());
    }

    
    if (nsMenuDismissalListener::sInstance)
      nsMenuDismissalListener::sInstance->EnableListener(PR_TRUE);

  }
  else {

    
    
    if ( !mCreateHandlerSucceeded || !OnDestroy() || !weakFrame.IsAlive())
      return;

    
    if (nsMenuDismissalListener::sInstance) {
      nsMenuDismissalListener::sInstance->EnableListener(PR_FALSE);
      nsMenuDismissalListener::sInstance->SetCurrentMenuParent(mMenuParent);
    }

    nsIFrame* frame = mPopupFrames.FirstChild();
    nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  
    
    if (menuPopup) {
      menuPopup->SetCurrentMenuItem(nsnull);
      ENSURE_TRUE(weakFrame.IsAlive());
      menuPopup->KillCloseTimer();

      PRBool onMenuBar = PR_TRUE;
      if (mMenuParent)
        mMenuParent->IsMenuBar(onMenuBar);

      if (mMenuParent && onMenuBar)
        mMenuParent->RemoveKeyboardNavigator();
      else if (!mMenuParent)
        menuPopup->RemoveKeyboardNavigator();

      
      
      

      nsIEventStateManager *esm = presContext->EventStateManager();

      PRInt32 state;
      esm->GetContentState(menuPopup->GetContent(), state);

      if (state & NS_EVENT_STATE_HOVER)
        esm->SetContentState(nsnull, NS_EVENT_STATE_HOVER);
    }

    ActivateMenu(PR_FALSE);
    ENSURE_TRUE(weakFrame.IsAlive());
    
    
    
    
    
    
    
    mMenuOpen = PR_FALSE;

    OnDestroyed();
    ENSURE_TRUE(weakFrame.IsAlive());

    if (nsMenuDismissalListener::sInstance)
      nsMenuDismissalListener::sInstance->EnableListener(PR_TRUE);

    mCreateHandlerSucceeded = PR_FALSE;
  }

}

void
nsMenuFrame::GetMenuChildrenElement(nsIContent** aResult)
{
  *aResult = nsContentUtils::FindFirstChildWithResolvedTag(mContent,
                                                           kNameSpaceID_XUL,
                                                           nsGkAtoms::menupopup);
  NS_IF_ADDREF(*aResult);
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
                  !aRequireAlways && sizedToPopup.EqualsLiteral("pref");
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

  
  nsIFrame* popupChild = mPopupFrames.FirstChild();

  if (popupChild) {
    PRBool sizeToPopup = IsSizedToPopup(mContent, PR_FALSE);
    
    NS_ASSERTION(popupChild->IsBoxFrame(), "popupChild is not box!!");

    
    nsSize prefSize = popupChild->GetPrefSize(aState);
    nsSize minSize = popupChild->GetMinSize(aState); 
    nsSize maxSize = popupChild->GetMaxSize(aState);

    BoundsCheck(minSize, prefSize, maxSize);

    if (sizeToPopup)
        prefSize.width = mRect.width;

    
    
    if (mLastPref != prefSize) {
      popupChild->SetBounds(aState, nsRect(0,0,prefSize.width, prefSize.height));
      RePositionPopup(aState);
      mLastPref = prefSize;
    }

    
    nsIBox* child = popupChild->GetChildBox();

    nsRect bounds(popupChild->GetRect());

    nsCOMPtr<nsIScrollableFrame> scrollframe(do_QueryInterface(child));
    if (scrollframe &&
        scrollframe->GetScrollbarStyles().mVertical == NS_STYLE_OVERFLOW_AUTO) {
      if (bounds.height < prefSize.height) {
        
        popupChild->Layout(aState);

        nsMargin scrollbars = scrollframe->GetActualScrollbarSizes();
        if (bounds.width < prefSize.width + scrollbars.left + scrollbars.right)
        {
          bounds.width += scrollbars.left + scrollbars.right;
          
          popupChild->SetBounds(aState, bounds);
        }
      }
    }
    
    
    popupChild->Layout(aState);

    
    if (mMenuOpen) {
      nsIView* view = popupChild->GetView();
      nsRect r(0, 0, bounds.width, bounds.height);
      view->GetViewManager()->ResizeView(view, r);
    }

  }

  SyncLayout(aState);

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
      SetDebug(aState, mPopupFrames.FirstChild(), aDebug);
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

void
nsMenuFrame::ConvertPosition(nsIContent* aPopupElt, nsString& aAnchor, nsString& aAlign)
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::before_start, &nsGkAtoms::before_end,
     &nsGkAtoms::after_start, &nsGkAtoms::after_end, &nsGkAtoms::start_before,
     &nsGkAtoms::start_after, &nsGkAtoms::end_before, &nsGkAtoms::end_after,
     &nsGkAtoms::overlap, nsnull};

  switch (aPopupElt->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::position,
                                     strings, eCaseMatters)) {
    case nsIContent::ATTR_MISSING:
    case 0:
      return;
    case 1:
      aAnchor.AssignLiteral("topleft");
      aAlign.AssignLiteral("bottomleft");
      break;
    case 2:
      aAnchor.AssignLiteral("topright");
      aAlign.AssignLiteral("bottomright");
      break;
    case 3:
      aAnchor.AssignLiteral("bottomleft");
      aAlign.AssignLiteral("topleft");
      break;
    case 4:
      aAnchor.AssignLiteral("bottomright");
      aAlign.AssignLiteral("topright");
      break;
    case 5:
      aAnchor.AssignLiteral("topleft");
      aAlign.AssignLiteral("topright");
      break;
    case 6:
      aAnchor.AssignLiteral("bottomleft");
      aAlign.AssignLiteral("bottomright");
      break;
    case 7:
      aAnchor.AssignLiteral("topright");
      aAlign.AssignLiteral("topleft");
      break;
    case 8:
      aAnchor.AssignLiteral("bottomright");
      aAlign.AssignLiteral("bottomleft");
      break;
    case 9:
      aAnchor.AssignLiteral("topleft");
      aAlign.AssignLiteral("topleft");
      break;
  }
}

void
nsMenuFrame::RePositionPopup(nsBoxLayoutState& aState)
{  
  nsPresContext* presContext = aState.PresContext();

  
  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  if (mMenuOpen && menuPopup) {
    nsIContent* menuPopupContent = menuPopup->GetContent();
    nsAutoString popupAnchor, popupAlign;
      
    menuPopupContent->GetAttr(kNameSpaceID_None, nsGkAtoms::popupanchor, popupAnchor);
    menuPopupContent->GetAttr(kNameSpaceID_None, nsGkAtoms::popupalign, popupAlign);

    ConvertPosition(menuPopupContent, popupAnchor, popupAlign);

    PRBool onMenuBar = PR_TRUE;
    if (mMenuParent)
      mMenuParent->IsMenuBar(onMenuBar);

    if (onMenuBar) {
      if (popupAnchor.IsEmpty())
          popupAnchor.AssignLiteral("bottomleft");
      if (popupAlign.IsEmpty())
          popupAlign.AssignLiteral("topleft");
    }
    else {
      if (popupAnchor.IsEmpty())
        popupAnchor.AssignLiteral("topright");
      if (popupAlign.IsEmpty())
        popupAlign.AssignLiteral("topleft");
    }

    menuPopup->SyncViewWithFrame(presContext, popupAnchor, popupAlign, this, -1, -1);
  }
}

NS_IMETHODIMP
nsMenuFrame::ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->ShortcutNavigation(aKeyEvent, aHandledFlag);
  } 

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::KeyboardNavigation(PRUint32 aKeyCode, PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->KeyboardNavigation(aKeyCode, aHandledFlag);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::Escape(PRBool& aHandledFlag)
{
  if (mMenuParent) {
    mMenuParent->ClearRecentlyRolledUp();
  }
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->Escape(aHandledFlag);
  }

  return NS_OK;
}










NS_IMETHODIMP
nsMenuFrame::Enter()
{
  if (IsDisabled()) {
#ifdef XP_WIN
    
    if (mMenuParent)
      mMenuParent->DismissChain();
#endif   
    
    return NS_OK;
  }
    
  if (!mMenuOpen) {
    
    if (!IsMenu() && mMenuParent)
      Execute(0);          
    else {
      OpenMenu(PR_TRUE);
      SelectFirstItem();
    }

    return NS_OK;
  }

  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->Enter();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SelectFirstItem()
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->SetCurrentMenuItem(popup->GetNextMenuItem(nsnull));
  }

  return NS_OK;
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
    if (!mMenuOpen && mMenuParent) {
      
      
      
      
      nsIMenuParent *ctxMenu = nsMenuFrame::GetContextMenu();
      PRBool parentIsContextMenu = PR_FALSE;

      if (ctxMenu)
        mMenuParent->GetIsContextMenu(parentIsContextMenu);

      if (ctxMenu == nsnull || parentIsContextMenu) {
        if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menuactive,
                                  nsGkAtoms::_true, eCaseMatters)) {
          
          
          mMenuParent->KillPendingTimers();
          OpenMenu(PR_TRUE);
        }
      }
    }
    mOpenTimer->Cancel();
    mOpenTimer = nsnull;
  }
  
  mOpenTimer = nsnull;
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
nsMenuFrame::UpdateMenuSpecialState(nsPresContext* aPresContext) {
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

  





  










  
  nsIMenuFrame *sibMenu;
  nsMenuType sibType;
  nsAutoString sibGroup;
  PRBool sibChecked;
  
  
  
  
  nsIFrame* sib = GetParent()->GetFirstChild(nsnull);
  if ( !sib )
    return;

  
  
  
  

  do {
    if (NS_FAILED(sib->QueryInterface(NS_GET_IID(nsIMenuFrame),
                                      (void **)&sibMenu)))
        continue;
        
    if (sibMenu != (nsIMenuFrame *)this &&        
        (sibMenu->GetMenuType(sibType), sibType == eMenuType_Radio) &&
        (sibMenu->MenuIsChecked(sibChecked), sibChecked) &&
        (sibMenu->GetRadioGroupName(sibGroup), sibGroup == mGroupName)) {
      
      
      sib->GetContent()->UnsetAttr(kNameSpaceID_None, nsGkAtoms::checked,
                                   PR_TRUE);

      
      return;
    }

  } while ((sib = sib->GetNextSibling()) != nsnull);

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
    
    
#if defined(XP_MAC) || defined(XP_MACOSX)
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

  
  
  
  if ( nsMenuDismissalListener::sInstance ) {
    nsMenuDismissalListener::sInstance->EnableListener(PR_FALSE);
  }

  
  nsCOMPtr<nsIContent> content = mContent;

  
  SelectMenu(PR_FALSE);
  ENSURE_TRUE(weakFrame.IsAlive());

  
  if (mMenuParent) {
    mMenuParent->HideChain();

    
    
    
    
    mMenuParent->ClearRecentlyRolledUp();
  }


  nsEventStatus status = nsEventStatus_eIgnore;
  
  
  
  nsXULCommandEvent event(aEvent ? NS_IS_TRUSTED_EVENT(aEvent) :
                          nsContentUtils::IsCallerChrome(),
                          NS_XUL_COMMAND, nsnull);
  if (aEvent && (aEvent->eventStructType == NS_MOUSE_EVENT ||
                 aEvent->eventStructType == NS_KEY_EVENT ||
                 aEvent->eventStructType == NS_ACCESSIBLE_EVENT)) {

    event.isShift = NS_STATIC_CAST(nsInputEvent *, aEvent)->isShift;
    event.isControl = NS_STATIC_CAST(nsInputEvent *, aEvent)->isControl;
    event.isAlt = NS_STATIC_CAST(nsInputEvent *, aEvent)->isAlt;
    event.isMeta = NS_STATIC_CAST(nsInputEvent *, aEvent)->isMeta;
  }

  
  
  
  
  nsPresContext* presContext = GetPresContext();
  nsCOMPtr<nsIViewManager> kungFuDeathGrip = presContext->GetViewManager();
  nsCOMPtr<nsIPresShell> shell = presContext->GetPresShell();
  if (shell) {
    shell->HandleDOMEventWithTarget(mContent, &event, &status);
    ENSURE_TRUE(weakFrame.IsAlive());
  }

  if (mMenuParent) {
    mMenuParent->DismissChain();
  }

  
  if ( nsMenuDismissalListener::sInstance ) {
    nsMenuDismissalListener::sInstance->EnableListener(PR_TRUE);
  }
}

PRBool
nsMenuFrame::OnCreate()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWING, nsnull,
                     nsMouseEvent::eReal);

  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv = NS_OK;

  nsCOMPtr<nsIPresShell> shell = GetPresContext()->GetPresShell();
  if (shell) {
    if (child) {
      rv = shell->HandleDOMEventWithTarget(child, &event, &status);
    }
    else {
      rv = shell->HandleDOMEventWithTarget(mContent, &event, &status);
    }
  }

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;

  
  
  
  
  if (child) {
    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(child->GetDocument()));

    PRUint32 count = child->GetChildCount();
    for (PRUint32 i = 0; i < count; i++) {
      nsCOMPtr<nsIContent> grandChild = child->GetChildAt(i);

      if (grandChild->Tag() == nsGkAtoms::menuitem) {
        
        nsAutoString command;
        grandChild->GetAttr(kNameSpaceID_None, nsGkAtoms::command, command);
        if (!command.IsEmpty()) {
          
          nsCOMPtr<nsIDOMElement> commandElt;
          domDoc->GetElementById(command, getter_AddRefs(commandElt));
          nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));

          if ( commandContent ) {
            nsAutoString commandAttr;
            
            if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::disabled, commandAttr))
              grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, commandAttr, PR_TRUE);
            else
              grandChild->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, PR_TRUE);

            
            
            
            if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandAttr))
              grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandAttr, PR_TRUE);

            if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandAttr))
              grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandAttr, PR_TRUE);

            if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, commandAttr))
              grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::label, commandAttr, PR_TRUE);
          }
        }
      }
    }
  }

  return PR_TRUE;
}

PRBool
nsMenuFrame::OnCreated()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWN, nsnull,
                     nsMouseEvent::eReal);

  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIPresShell> shell = GetPresContext()->GetPresShell();
  if (shell) {
    if (child) {
      rv = shell->HandleDOMEventWithTarget(child, &event, &status);
    }
    else {
      rv = shell->HandleDOMEventWithTarget(mContent, &event, &status);
    }
  }

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

PRBool
nsMenuFrame::OnDestroy()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDING, nsnull,
                     nsMouseEvent::eReal);

  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIPresShell> shell = GetPresContext()->GetPresShell();
  if (shell) {
    if (child) {
      rv = shell->HandleDOMEventWithTarget(child, &event, &status);
    }
    else {
      rv = shell->HandleDOMEventWithTarget(mContent, &event, &status);
    }
  }

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

PRBool
nsMenuFrame::OnDestroyed()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDDEN, nsnull,
                     nsMouseEvent::eReal);

  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIPresShell> shell = GetPresContext()->GetPresShell();
  if (shell) {
    if (child) {
      rv = shell->HandleDOMEventWithTarget(child, &event, &status);
    }
    else {
      rv = shell->HandleDOMEventWithTarget(mContent, &event, &status);
    }
  }

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

NS_IMETHODIMP
nsMenuFrame::RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame)
{
  nsresult  rv;

  if (mPopupFrames.ContainsFrame(aOldFrame)) {
    
    mPopupFrames.DestroyFrame(aOldFrame);
    AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
    GetPresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange);
    rv = NS_OK;
  } else {
    rv = nsBoxFrame::RemoveFrame(aListName, aOldFrame);
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList)
{
  nsresult          rv;

  nsIMenuParent *menuPar;
  if (aFrameList && NS_SUCCEEDED(CallQueryInterface(aFrameList, &menuPar))) {
    NS_ASSERTION(aFrameList->IsBoxFrame(),"Popup is not a box!!!");
    mPopupFrames.InsertFrames(nsnull, nsnull, aFrameList);

#ifdef DEBUG_LAYOUT
    nsBoxLayoutState state(GetPresContext());
    SetDebug(state, aFrameList, mState & NS_STATE_CURRENTLY_IN_DEBUG);
#endif
    AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
    GetPresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange);
    rv = NS_OK;
  } else {
    rv = nsBoxFrame::InsertFrames(aListName, aPrevFrame, aFrameList);  
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList)
{
  if (!aFrameList)
    return NS_OK;

  nsresult          rv;

  nsIMenuParent *menuPar;
  if (aFrameList && NS_SUCCEEDED(CallQueryInterface(aFrameList, &menuPar))) {
    NS_ASSERTION(aFrameList->IsBoxFrame(),"Popup is not a box!!!");

    mPopupFrames.AppendFrames(nsnull, aFrameList);
#ifdef DEBUG_LAYOUT
    nsBoxLayoutState state(GetPresContext());
    SetDebug(state, aFrameList, mState & NS_STATE_CURRENTLY_IN_DEBUG);
#endif
    AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
    GetPresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange);
    rv = NS_OK;
  } else {
    rv = nsBoxFrame::AppendFrames(aListName, aFrameList); 
  }

  return rv;
}

class nsASyncMenuGeneration : public nsIReflowCallback
{
public:
  nsASyncMenuGeneration(nsIFrame* aFrame)
    : mWeakFrame(aFrame)
  {
    nsIContent* content = aFrame ? aFrame->GetContent() : nsnull;
    mDocument = content ? content->GetCurrentDoc() : nsnull;
    if (mDocument) {
      mDocument->BlockOnload();
    }
  }

  virtual PRBool ReflowFinished() {
    PRBool shouldFlush = PR_FALSE;
    nsIFrame* frame = mWeakFrame.GetFrame();
    if (frame) {
      nsBoxLayoutState state(frame->GetPresContext());
      if (!frame->IsCollapsed(state)) {
        nsIMenuFrame* imenu = nsnull;
        CallQueryInterface(frame, &imenu);
        if (imenu) {
          imenu->MarkAsGenerated();
          shouldFlush = PR_TRUE;
        }
      }
    }
    if (mDocument) {
      mDocument->UnblockOnload(PR_FALSE);
    }
    delete this;
    return shouldFlush;
  }

  nsWeakFrame           mWeakFrame;
  nsCOMPtr<nsIDocument> mDocument;
};

PRBool
nsMenuFrame::SizeToPopup(nsBoxLayoutState& aState, nsSize& aSize)
{
  if (!IsCollapsed(aState)) {
    nsSize tmpSize(-1, 0);
    nsIBox::AddCSSPrefSize(aState, this, tmpSize);
    if (tmpSize.width == -1 && GetFlex(aState) == 0) {
      nsIFrame* frame = mPopupFrames.FirstChild();
      if (!frame) {
        nsCOMPtr<nsIContent> child;
        GetMenuChildrenElement(getter_AddRefs(child));
        if (child &&
            !nsContentUtils::HasNonEmptyAttr(child, kNameSpaceID_None,
                                             nsGkAtoms::menugenerated)) {
          nsIReflowCallback* cb = new nsASyncMenuGeneration(this);
          if (cb) {
            GetPresContext()->PresShell()->PostReflowCallback(cb);
          }
        }
        return PR_FALSE;
      }

      NS_ASSERTION(frame->IsBoxFrame(), "popupChild is not box!!");

      tmpSize = frame->GetPrefSize(aState);
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
    BoundsCheck(minSize, size, maxSize);
  }

  return size;
}

NS_IMETHODIMP
nsMenuFrame::GetActiveChild(nsIDOMElement** aResult)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  if (!frame)
    return NS_ERROR_FAILURE;

  nsIMenuFrame* menuFrame = menuPopup->GetCurrentMenuItem();
  
  if (!menuFrame) {
    *aResult = nsnull;
  }
  else {
    nsIFrame* f;
    menuFrame->QueryInterface(NS_GET_IID(nsIFrame), (void**)&f);
    nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(f->GetContent()));
    *aResult = elt;
    NS_IF_ADDREF(*aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SetActiveChild(nsIDOMElement* aChild)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  if (!frame)
    return NS_ERROR_FAILURE;

  if (!aChild) {
    
    menuPopup->SetCurrentMenuItem(nsnull);
    return NS_OK;
  }

  nsCOMPtr<nsIContent> child(do_QueryInterface(aChild));
  
  nsIFrame* kid = GetPresContext()->PresShell()->GetPrimaryFrameFor(child);
  if (!kid)
    return NS_ERROR_FAILURE;
  nsIMenuFrame *menuFrame;
  nsresult rv = CallQueryInterface(kid, &menuFrame);
  if (NS_FAILED(rv))
    return rv;
  menuPopup->SetCurrentMenuItem(menuFrame);
  return NS_OK;
}

nsIScrollableView* nsMenuFrame::GetScrollableView()
{
  if (!mPopupFrames.FirstChild())
    return nsnull;

  nsMenuPopupFrame* popup = (nsMenuPopupFrame*) mPopupFrames.FirstChild();
  nsIFrame* childFrame = popup->GetFirstChild(nsnull);
  if (childFrame) {
    return popup->GetScrollableView(childFrame);
  }
  return nsnull;
}





































nsIMenuParent*
nsMenuFrame::GetContextMenu()
{
  if (!nsMenuDismissalListener::sInstance)
    return nsnull;

  nsIMenuParent *menuParent =
    nsMenuDismissalListener::sInstance->GetCurrentMenuParent();
  if (!menuParent)
    return nsnull;

  PRBool isContextMenu;
  menuParent->GetIsContextMenu(isContextMenu);
  if (isContextMenu)
    return menuParent;

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
