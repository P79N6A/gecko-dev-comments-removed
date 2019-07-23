










































#include "nsMenuPopupFrame.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsIViewManager.h"
#include "nsWidgetsCID.h"
#include "nsMenuFrame.h"
#include "nsMenuBarFrame.h"
#include "nsPopupSetFrame.h"
#include "nsEventDispatcher.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMScreen.h"
#include "nsIPresShell.h"
#include "nsFrameManager.h"
#include "nsIDocument.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableView.h"
#include "nsIScrollableFrame.h"
#include "nsGUIEvent.h"
#include "nsIRootBox.h"
#include "nsIDocShellTreeItem.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsLayoutUtils.h"
#include "nsContentUtils.h"
#include "nsCSSFrameConstructor.h"
#include "nsIEventStateManager.h"
#include "nsIBoxLayout.h"
#include "nsIPopupBoxObject.h"
#include "nsIReflowCallback.h"
#include "nsBindingManager.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#ifdef XP_WIN
#include "nsISound.h"
#endif

const PRInt32 kMaxZ = 0x7fffffff; 

PRInt8 nsMenuPopupFrame::sDefaultLevelParent = -1;





nsIFrame*
NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMenuPopupFrame (aPresShell, aContext);
}




nsMenuPopupFrame::nsMenuPopupFrame(nsIPresShell* aShell, nsStyleContext* aContext)
  :nsBoxFrame(aShell, aContext),
  mCurrentMenu(nsnull),
  mPopupAlignment(POPUPALIGNMENT_NONE),
  mPopupAnchor(POPUPALIGNMENT_NONE),
  mPopupType(ePopupTypePanel),
  mPopupState(ePopupClosed),
  mIsOpenChanged(PR_FALSE),
  mIsContextMenu(PR_FALSE),
  mAdjustOffsetForContextMenu(PR_FALSE),
  mGeneratedChildren(PR_FALSE),
  mMenuCanOverlapOSBar(PR_FALSE),
  mShouldAutoPosition(PR_TRUE),
  mConsumeRollupEvent(nsIPopupBoxObject::ROLLUP_DEFAULT),
  mInContentShell(PR_TRUE),
  mPrefSize(-1, -1)
{
  if (sDefaultLevelParent >= 0)
    return;
  sDefaultLevelParent =
    nsContentUtils::GetBoolPref("ui.panel.default_level_parent", PR_FALSE);
} 


NS_IMETHODIMP
nsMenuPopupFrame::Init(nsIContent*      aContent,
                       nsIFrame*        aParent,
                       nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  nsPresContext* presContext = PresContext();

  
  
  PRBool tempBool;
  presContext->LookAndFeel()->
    GetMetric(nsILookAndFeel::eMetric_MenusCanOverlapOSBar, tempBool);
  mMenuCanOverlapOSBar = tempBool;

  rv = CreateViewForFrame(presContext, this, GetStyleContext(), PR_TRUE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsIView* ourView = GetView();
  nsIViewManager* viewManager = ourView->GetViewManager();
  viewManager->SetViewFloating(ourView, PR_TRUE);

  mPopupType = ePopupTypePanel;
  nsIDocument* doc = aContent->GetOwnerDoc();
  if (doc) {
    PRInt32 namespaceID;
    nsCOMPtr<nsIAtom> tag = doc->BindingManager()->ResolveTag(aContent, &namespaceID);
    if (namespaceID == kNameSpaceID_XUL) {
      if (tag == nsGkAtoms::menupopup || tag == nsGkAtoms::popup)
        mPopupType = ePopupTypeMenu;
      else if (tag == nsGkAtoms::tooltip)
        mPopupType = ePopupTypeTooltip;
    }
  }

  nsCOMPtr<nsISupports> cont = PresContext()->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(cont);
  PRInt32 type = -1;
  if (dsti && NS_SUCCEEDED(dsti->GetItemType(&type)) &&
      type == nsIDocShellTreeItem::typeChrome)
    mInContentShell = PR_FALSE;

  
  
  
  if (!IsLeaf() && !ourView->HasWidget()) {
    CreateWidgetForView(ourView);
  }

  return rv;
}

PRBool
nsMenuPopupFrame::IsNoAutoHide()
{
  
  
  
  return (!mInContentShell && mPopupType == ePopupTypePanel &&
           mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::noautohide,
                                 nsGkAtoms::_true, eIgnoreCase));
}

PRBool
nsMenuPopupFrame::IsTopMost()
{
  
  if (mPopupType != ePopupTypePanel)
    return PR_TRUE;

  
  
  if (IsNoAutoHide())
    return PR_FALSE;

  
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::level,
                            nsGkAtoms::top, eIgnoreCase))
    return PR_TRUE;

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::level,
                            nsGkAtoms::parent, eIgnoreCase))
    return PR_FALSE;

  
  return sDefaultLevelParent ? PR_TRUE : PR_FALSE;
}

void
nsMenuPopupFrame::EnsureWidget()
{
  nsIView* ourView = GetView();
  if (!ourView->HasWidget()) {
    NS_ASSERTION(!mGeneratedChildren && !GetFirstChild(nsnull),
                 "Creating widget for MenuPopupFrame with children");
    CreateWidgetForView(ourView);
  }
}

nsresult
nsMenuPopupFrame::CreateWidgetForView(nsIView* aView)
{
  
  nsWidgetInitData widgetData;
  widgetData.mWindowType = eWindowType_popup;
  widgetData.mBorderStyle = eBorderStyle_default;
  widgetData.clipSiblings = PR_TRUE;
  widgetData.mPopupHint = mPopupType;

  nsTransparencyMode mode = nsLayoutUtils::GetFrameTransparency(this);
  PRBool viewHasTransparentContent = !mInContentShell &&
                                     (eTransparencyTransparent ==
                                      mode);
  nsIContent* parentContent = GetContent()->GetParent();
  nsIAtom *tag = nsnull;
  if (parentContent)
    tag = parentContent->Tag();
  widgetData.mDropShadow = !(viewHasTransparentContent || tag == nsGkAtoms::menulist);

  
  
  
  nsCOMPtr<nsIWidget> parentWidget;
  if (!IsTopMost()) {
    nsCOMPtr<nsISupports> cont = PresContext()->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(cont);
    if (!dsti)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
    dsti->GetTreeOwner(getter_AddRefs(treeOwner));
    if (!treeOwner) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(treeOwner));
    if (baseWindow)
      baseWindow->GetMainWidget(getter_AddRefs(parentWidget));
  }

#if defined(XP_MACOSX) || defined(XP_BEOS)
  static NS_DEFINE_IID(kCPopupCID,  NS_POPUP_CID);
  aView->CreateWidget(kCPopupCID, &widgetData, nsnull, PR_TRUE, PR_TRUE, 
                      eContentTypeUI, parentWidget);
#else
  static NS_DEFINE_IID(kCChildCID,  NS_CHILD_CID);
  aView->CreateWidget(kCChildCID, &widgetData, nsnull, PR_TRUE, PR_TRUE,
                      eContentTypeInherit, parentWidget);
#endif
  nsIWidget* widget = aView->GetWidget();
  widget->SetTransparencyMode(mode);
  widget->SetWindowShadowStyle(GetStyleUIReset()->mWindowShadow);
  return NS_OK;
}


class nsXULPopupShownEvent : public nsRunnable
{
public:
  nsXULPopupShownEvent(nsIContent *aPopup, nsPresContext* aPresContext)
    : mPopup(aPopup), mPresContext(aPresContext)
  {
  }

  NS_IMETHOD Run()
  {
    nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWN, nsnull, nsMouseEvent::eReal);
    return nsEventDispatcher::Dispatch(mPopup, mPresContext, &event);                 
  }

private:
  nsCOMPtr<nsIContent> mPopup;
  nsRefPtr<nsPresContext> mPresContext;
};

NS_IMETHODIMP
nsMenuPopupFrame::SetInitialChildList(nsIAtom* aListName,
                                      nsIFrame* aChildList)
{
  
  if (aChildList)
    mGeneratedChildren = PR_TRUE;
  return nsBoxFrame::SetInitialChildList(aListName, aChildList);
}

PRBool
nsMenuPopupFrame::IsLeaf() const
{
  if (mGeneratedChildren)
    return PR_FALSE;

  if (mPopupType != ePopupTypeMenu) {
    
    
    return !mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::type);
  }

  
  
  
  
  
  nsIContent* parentContent = mContent->GetParent();
  return (parentContent &&
          !parentContent->HasAttr(kNameSpaceID_None, nsGkAtoms::sizetopopup));
}

void
nsMenuPopupFrame::SetPreferredBounds(nsBoxLayoutState& aState,
                                     const nsRect& aRect)
{
  nsBox::SetBounds(aState, aRect, PR_FALSE);
  mPrefSize = aRect.Size();
}

void
nsMenuPopupFrame::AdjustView()
{
  if ((mPopupState == ePopupOpen || mPopupState == ePopupOpenAndVisible) &&
      mGeneratedChildren) {
    
    if (mIsOpenChanged) {
      nsIBox* child = GetChildBox();
      nsCOMPtr<nsIScrollableFrame> scrollframe(do_QueryInterface(child));
      if (scrollframe)
        scrollframe->ScrollTo(nsPoint(0,0));
    }

    nsIView* view = GetView();
    nsIViewManager* viewManager = view->GetViewManager();
    nsRect rect = GetRect();
    rect.x = rect.y = 0;
    viewManager->ResizeView(view, rect);
    viewManager->SetViewVisibility(view, nsViewVisibility_kShow);
    mPopupState = ePopupOpenAndVisible;

    nsPresContext* pc = PresContext();
    nsContainerFrame::SyncFrameViewProperties(pc, this, nsnull, view, 0);

    
    if (mIsOpenChanged) {
      mIsOpenChanged = PR_FALSE;
      nsCOMPtr<nsIRunnable> event = new nsXULPopupShownEvent(GetContent(), pc);
      NS_DispatchToCurrentThread(event);
    }
  }
}

void
nsMenuPopupFrame::InitPositionFromAnchorAlign(const nsAString& aAnchor,
                                              const nsAString& aAlign)
{
  if (aAnchor.EqualsLiteral("topleft"))
    mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
  else if (aAnchor.EqualsLiteral("topright"))
    mPopupAnchor = POPUPALIGNMENT_TOPRIGHT;
  else if (aAnchor.EqualsLiteral("bottomleft"))
    mPopupAnchor = POPUPALIGNMENT_BOTTOMLEFT;
  else if (aAnchor.EqualsLiteral("bottomright"))
    mPopupAnchor = POPUPALIGNMENT_BOTTOMRIGHT;
  else
    mPopupAnchor = POPUPALIGNMENT_NONE;

  if (aAlign.EqualsLiteral("topleft"))
    mPopupAlignment = POPUPALIGNMENT_TOPLEFT;
  else if (aAlign.EqualsLiteral("topright"))
    mPopupAlignment = POPUPALIGNMENT_TOPRIGHT;
  else if (aAlign.EqualsLiteral("bottomleft"))
    mPopupAlignment = POPUPALIGNMENT_BOTTOMLEFT;
  else if (aAlign.EqualsLiteral("bottomright"))
    mPopupAlignment = POPUPALIGNMENT_BOTTOMRIGHT;
  else
    mPopupAlignment = POPUPALIGNMENT_NONE;
}

void
nsMenuPopupFrame::InitializePopup(nsIContent* aAnchorContent,
                                  const nsAString& aPosition,
                                  PRInt32 aXPos, PRInt32 aYPos,
                                  PRBool aAttributesOverride)
{
  EnsureWidget();

  mPopupState = ePopupShowing;
  mAnchorContent = aAnchorContent;
  mXPos = aXPos;
  mYPos = aYPos;
  mAdjustOffsetForContextMenu = PR_FALSE;

  
  
  
  if (aAnchorContent) {
    nsAutoString anchor, align, position;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::popupanchor, anchor);
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::popupalign, align);
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::position, position);

    if (aAttributesOverride) {
      
      
      if (anchor.IsEmpty() && align.IsEmpty() && position.IsEmpty())
        position.Assign(aPosition);
      else
        mXPos = mYPos = 0;
    }
    else if (!aPosition.IsEmpty()) {
      position.Assign(aPosition);
    }

    if (position.EqualsLiteral("before_start")) {
      mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
      mPopupAlignment = POPUPALIGNMENT_BOTTOMLEFT;
    }
    else if (position.EqualsLiteral("before_end")) {
      mPopupAnchor = POPUPALIGNMENT_TOPRIGHT;
      mPopupAlignment = POPUPALIGNMENT_BOTTOMRIGHT;
    }
    else if (position.EqualsLiteral("after_start")) {
      mPopupAnchor = POPUPALIGNMENT_BOTTOMLEFT;
      mPopupAlignment = POPUPALIGNMENT_TOPLEFT;
    }
    else if (position.EqualsLiteral("after_end")) {
      mPopupAnchor = POPUPALIGNMENT_BOTTOMRIGHT;
      mPopupAlignment = POPUPALIGNMENT_TOPRIGHT;
    }
    else if (position.EqualsLiteral("start_before")) {
      mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
      mPopupAlignment = POPUPALIGNMENT_TOPRIGHT;
    }
    else if (position.EqualsLiteral("start_after")) {
      mPopupAnchor = POPUPALIGNMENT_BOTTOMLEFT;
      mPopupAlignment = POPUPALIGNMENT_BOTTOMRIGHT;
    }
    else if (position.EqualsLiteral("end_before")) {
      mPopupAnchor = POPUPALIGNMENT_TOPRIGHT;
      mPopupAlignment = POPUPALIGNMENT_TOPLEFT;
    }
    else if (position.EqualsLiteral("end_after")) {
      mPopupAnchor = POPUPALIGNMENT_BOTTOMRIGHT;
      mPopupAlignment = POPUPALIGNMENT_BOTTOMLEFT;
    }
    else if (position.EqualsLiteral("overlap")) {
      mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
      mPopupAlignment = POPUPALIGNMENT_TOPLEFT;
    }
    else if (position.EqualsLiteral("after_pointer")) {
      mPopupAnchor = POPUPALIGNMENT_NONE;
      mPopupAlignment = POPUPALIGNMENT_NONE;
      
      
      mYPos += 21;
    }
    else {
      InitPositionFromAnchorAlign(anchor, align);
    }
  }

  mScreenXPos = -1;
  mScreenYPos = -1;

  if (aAttributesOverride) {
    
    
    nsAutoString left, top;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::left, left);
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::top, top);

    PRInt32 err;
    if (!left.IsEmpty()) {
      PRInt32 x = left.ToInteger(&err);
      if (NS_SUCCEEDED(err))
        mScreenXPos = x;
    }
    if (!top.IsEmpty()) {
      PRInt32 y = top.ToInteger(&err);
      if (NS_SUCCEEDED(err))
        mScreenYPos = y;
    }
  }
}

void
nsMenuPopupFrame::InitializePopupAtScreen(PRInt32 aXPos, PRInt32 aYPos,
                                          PRBool aIsContextMenu)
{
  EnsureWidget();

  mPopupState = ePopupShowing;
  mAnchorContent = nsnull;
  mScreenXPos = aXPos;
  mScreenYPos = aYPos;
  mPopupAnchor = POPUPALIGNMENT_NONE;
  mPopupAlignment = POPUPALIGNMENT_NONE;
  mIsContextMenu = aIsContextMenu;
  mAdjustOffsetForContextMenu = aIsContextMenu;
}

void
nsMenuPopupFrame::InitializePopupWithAnchorAlign(nsIContent* aAnchorContent,
                                                 nsAString& aAnchor,
                                                 nsAString& aAlign,
                                                 PRInt32 aXPos, PRInt32 aYPos)
{
  EnsureWidget();

  mPopupState = ePopupShowing;
  mAdjustOffsetForContextMenu = PR_FALSE;

  
  
  
  if (aXPos == -1 && aYPos == -1) {
    mAnchorContent = aAnchorContent;
    mScreenXPos = -1;
    mScreenYPos = -1;
    mXPos = 0;
    mYPos = 0;
    InitPositionFromAnchorAlign(aAnchor, aAlign);
  }
  else {
    mAnchorContent = nsnull;
    mPopupAnchor = POPUPALIGNMENT_NONE;
    mPopupAlignment = POPUPALIGNMENT_NONE;
    mScreenXPos = aXPos;
    mScreenYPos = aYPos;
    mXPos = aXPos;
    mYPos = aYPos;
  }
}

void
LazyGeneratePopupDone(nsIContent* aPopup, nsIFrame* aFrame, void* aArg)
{
  
  if (aFrame->GetType() == nsGkAtoms::menuPopupFrame) {
    nsWeakFrame weakFrame(aFrame);
    nsMenuPopupFrame* popupFrame = static_cast<nsMenuPopupFrame*>(aFrame);

    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm && popupFrame->IsMenu()) {
      nsCOMPtr<nsIContent> popup = aPopup;
      pm->UpdateMenuItems(popup);

      if (!weakFrame.IsAlive())
        return;

      PRBool selectFirstItem = (PRBool)NS_PTR_TO_INT32(aArg);
      if (selectFirstItem) {
        nsMenuFrame* next = pm->GetNextMenuItem(popupFrame, nsnull, PR_TRUE);
        popupFrame->SetCurrentMenuItem(next);
      }
    }

    if (weakFrame.IsAlive()) {
      popupFrame->PresContext()->PresShell()->
        FrameNeedsReflow(popupFrame, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
}


PRBool
nsMenuPopupFrame::ShowPopup(PRBool aIsContextMenu, PRBool aSelectFirstItem)
{
  mIsContextMenu = aIsContextMenu;

  PRBool hasChildren = PR_FALSE;

  if (mPopupState == ePopupShowing) {
    mPopupState = ePopupOpen;
    mIsOpenChanged = PR_TRUE;

    nsIFrame* parent = GetParent();
    if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
      nsWeakFrame weakFrame(this);
      (static_cast<nsMenuFrame*>(parent))->PopupOpened();
      if (!weakFrame.IsAlive())
        return PR_FALSE;
    }

    
    
    if (mFrames.IsEmpty() && !mGeneratedChildren) {
      PresContext()->PresShell()->FrameConstructor()->
        AddLazyChildren(mContent, LazyGeneratePopupDone, NS_INT32_TO_PTR(aSelectFirstItem));
    }
    else {
      hasChildren = PR_TRUE;
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }

  mShouldAutoPosition = PR_TRUE;
  return hasChildren;
}

void
nsMenuPopupFrame::HidePopup(PRBool aDeselectMenu, nsPopupState aNewState)
{
  NS_ASSERTION(aNewState == ePopupClosed || aNewState == ePopupInvisible,
               "popup being set to unexpected state");

  
  if (mPopupState == ePopupClosed || mPopupState == ePopupShowing)
    return;

  
  
  if (mPopupState == ePopupInvisible) {
    if (aNewState == ePopupClosed)
      mPopupState = ePopupClosed;
    return;
  }

  mPopupState = aNewState;

  if (IsMenu())
    SetCurrentMenuItem(nsnull);

  mIncrementalString.Truncate();

  mIsOpenChanged = PR_FALSE;
  mCurrentMenu = nsnull; 
 
  nsIView* view = GetView();
  nsIViewManager* viewManager = view->GetViewManager();
  viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
  viewManager->ResizeView(view, nsRect(0, 0, 0, 0));

  FireDOMEvent(NS_LITERAL_STRING("DOMMenuInactive"), mContent);

  
  
  
  nsIEventStateManager *esm = PresContext()->EventStateManager();

  PRInt32 state;
  esm->GetContentState(mContent, state);

  if (state & NS_EVENT_STATE_HOVER)
    esm->SetContentState(nsnull, NS_EVENT_STATE_HOVER);

  nsIFrame* parent = GetParent();
  if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
    (static_cast<nsMenuFrame*>(parent))->PopupClosed(aDeselectMenu);
  }
}

void
nsMenuPopupFrame::InvalidateInternal(const nsRect& aDamageRect,
                                     nscoord aX, nscoord aY, nsIFrame* aForChild,
                                     PRUint32 aFlags)
{
  InvalidateRoot(aDamageRect + nsPoint(aX, aY), aFlags);
}

void
nsMenuPopupFrame::GetLayoutFlags(PRUint32& aFlags)
{
  aFlags = NS_FRAME_NO_SIZE_VIEW | NS_FRAME_NO_MOVE_VIEW | NS_FRAME_NO_VISIBILITY;
}






nsIView*
nsMenuPopupFrame::GetRootViewForPopup(nsIFrame* aStartFrame)
{
  nsIView* view = aStartFrame->GetClosestView();
  NS_ASSERTION(view, "frame must have a closest view!");
  while (view) {
    
    
    
    nsIWidget* widget = view->GetWidget();
    if (widget) {
      nsWindowType wtype;
      widget->GetWindowType(wtype);
      if (wtype == eWindowType_popup) {
        return view;
      }
    }

    nsIView* temp = view->GetParent();
    if (!temp) {
      
      
      return view;
    }
    view = temp;
  }

  return nsnull;
}









void
nsMenuPopupFrame::AdjustPositionForAnchorAlign(PRInt32* ioXPos, PRInt32* ioYPos, const nsSize & inParentSize,
                                               PRBool* outFlushWithTopBottom)
{
  PRInt8 popupAnchor(mPopupAnchor);
  PRInt8 popupAlign(mPopupAlignment);

  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    popupAnchor = -popupAnchor;
    popupAlign = -popupAlign;
  }

  
  nsMargin margin;
  GetStyleMargin()->GetMargin(margin);
  if (popupAlign == POPUPALIGNMENT_TOPLEFT) {
    *ioXPos += margin.left;
    *ioYPos += margin.top;
  } else if (popupAlign == POPUPALIGNMENT_TOPRIGHT) {
    *ioXPos += margin.right;
    *ioYPos += margin.top;
  } else if (popupAlign == POPUPALIGNMENT_BOTTOMLEFT) {
    *ioXPos += margin.left;
    *ioYPos += margin.bottom;
  } else if (popupAlign == POPUPALIGNMENT_BOTTOMRIGHT) {
    *ioXPos += margin.right;
    *ioYPos += margin.bottom;
  }
  
  if (popupAnchor == POPUPALIGNMENT_TOPRIGHT && popupAlign == POPUPALIGNMENT_TOPLEFT) {
    *ioXPos += inParentSize.width;
  }
  else if (popupAnchor == POPUPALIGNMENT_TOPLEFT && popupAlign == POPUPALIGNMENT_TOPLEFT) {
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == POPUPALIGNMENT_TOPRIGHT && popupAlign == POPUPALIGNMENT_BOTTOMRIGHT) {
    *ioXPos -= (mRect.width - inParentSize.width);
    *ioYPos -= mRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == POPUPALIGNMENT_BOTTOMRIGHT && popupAlign == POPUPALIGNMENT_BOTTOMLEFT) {
    *ioXPos += inParentSize.width;
    *ioYPos -= (mRect.height - inParentSize.height);
  }
  else if (popupAnchor == POPUPALIGNMENT_BOTTOMRIGHT && popupAlign == POPUPALIGNMENT_TOPRIGHT) {
    *ioXPos -= (mRect.width - inParentSize.width);
    *ioYPos += inParentSize.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == POPUPALIGNMENT_TOPLEFT && popupAlign == POPUPALIGNMENT_TOPRIGHT) {
    *ioXPos -= mRect.width;
  }
  else if (popupAnchor == POPUPALIGNMENT_TOPLEFT && popupAlign == POPUPALIGNMENT_BOTTOMLEFT) {
    *ioYPos -= mRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == POPUPALIGNMENT_BOTTOMLEFT && popupAlign == POPUPALIGNMENT_BOTTOMRIGHT) {
    *ioXPos -= mRect.width;
    *ioYPos -= (mRect.height - inParentSize.height);
  }
  else if (popupAnchor == POPUPALIGNMENT_BOTTOMLEFT && popupAlign == POPUPALIGNMENT_TOPLEFT) {
    *ioYPos += inParentSize.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else
    NS_WARNING ( "Hmmm, looks like you've hit a anchor/align case we weren't setup for." );

} 









PRBool
nsMenuPopupFrame::IsMoreRoomOnOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32 inScreenViewLocX, PRInt32 inScreenViewLocY,
                                                     const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                                     PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips )
{
  PRBool switchSides = PR_FALSE;
  if ( inFlushAboveBelow ) {
    PRInt32 availAbove = inScreenParentFrameRect.y - inScreenTopTwips;
    PRInt32 availBelow = inScreenBottomTwips - (inScreenParentFrameRect.y + inScreenParentFrameRect.height) ;
    if ( inScreenViewLocY > inScreenParentFrameRect.y )       
      switchSides = availAbove > availBelow;
    else
      switchSides = availBelow > availAbove;
  }
  else {
    PRInt32 availLeft = inScreenParentFrameRect.x - inScreenLeftTwips;
    PRInt32 availRight = inScreenRightTwips - (inScreenParentFrameRect.x + inScreenParentFrameRect.width) ;
    if ( inScreenViewLocX > inScreenParentFrameRect.x )       
      switchSides = availLeft > availRight;
    else
      switchSides = availRight > availLeft;           
  }

  return switchSides;
  
} 












void
nsMenuPopupFrame::MovePopupToOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32* ioXPos, PRInt32* ioYPos, 
                                                 PRInt32* ioScreenViewLocX, PRInt32* ioScreenViewLocY,
                                                 const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                                 PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips )
{
  if ( inFlushAboveBelow ) {
    if ( *ioScreenViewLocY > inScreenParentFrameRect.y ) {     
      
      PRInt32 shiftDistY = inScreenParentFrameRect.height + mRect.height;
      *ioYPos -= shiftDistY;
      *ioScreenViewLocY -= shiftDistY;
      
      if ( *ioScreenViewLocY < inScreenTopTwips ) {
        PRInt32 trimY = inScreenTopTwips - *ioScreenViewLocY;
        *ioYPos += trimY;
        *ioScreenViewLocY += trimY;
        mRect.height -= trimY;
      }
    }
    else {                                               
      
      PRInt32 shiftDistY = inScreenParentFrameRect.height + mRect.height;
      *ioYPos += shiftDistY;
      *ioScreenViewLocY += shiftDistY;
    }
  }
  else {
    if ( *ioScreenViewLocX > inScreenParentFrameRect.x ) {     
      
      PRInt32 shiftDistX = inScreenParentFrameRect.width + mRect.width;
      *ioXPos -= shiftDistX;
      *ioScreenViewLocX -= shiftDistX;
      
      if ( *ioScreenViewLocX < inScreenLeftTwips ) {
        PRInt32 trimX = inScreenLeftTwips - *ioScreenViewLocX;
        *ioXPos += trimX;
        *ioScreenViewLocX += trimX;
        mRect.width -= trimX;
      }
    }
    else {                                               
      
      PRInt32 shiftDistX = inScreenParentFrameRect.width + mRect.width;
      *ioXPos += shiftDistX;
      *ioScreenViewLocX += shiftDistX;
    }               
  }

} 




nsresult
nsMenuPopupFrame::SetPopupPosition(nsIFrame* aAnchorFrame)
{
  if (!mShouldAutoPosition && !mInContentShell) 
    return NS_OK;

  PRBool sizedToPopup = PR_FALSE;

  nsPresContext* presContext = PresContext();
  nsIFrame* rootFrame = presContext->PresShell()->FrameManager()->GetRootFrame();
  NS_ASSERTION(rootFrame->GetView() && GetView() &&
               rootFrame->GetView() == GetView()->GetParent(),
               "rootFrame's view is not our view's parent???");

  
  
  
  if (!aAnchorFrame) {
    if (mAnchorContent) {
      nsCOMPtr<nsIDocument> document = mAnchorContent->GetDocument();
      if (document) {
        nsIPresShell *shell = document->GetPrimaryShell();
        if (!shell)
          return NS_ERROR_FAILURE;

        aAnchorFrame = shell->GetPrimaryFrameFor(mAnchorContent);
      }
    }

    if (!aAnchorFrame) {
      aAnchorFrame = rootFrame;
      if (!aAnchorFrame)
        return NS_OK;
    }
  }

  if (aAnchorFrame->GetContent()) {
    
    sizedToPopup = nsMenuFrame::IsSizedToPopup(aAnchorFrame->GetContent(), PR_FALSE);
  }

  
  
  nsSize parentSize = aAnchorFrame->GetSize();

  
  
  
  
  
  float adj = float(presContext->AppUnitsPerDevPixel()) /
              aAnchorFrame->PresContext()->AppUnitsPerDevPixel();
  parentSize.width = NSToCoordCeil(parentSize.width * adj);
  parentSize.height = NSToCoordCeil(parentSize.height * adj);

  
  
  
  
  NS_ASSERTION(mPrefSize.width >= 0 || mPrefSize.height >= 0,
               "preferred size of popup not set");
  if (sizedToPopup) {
    mRect.width = parentSize.width;
  }
  else {
    mRect.width = mPrefSize.width;
  }
  mRect.height = mPrefSize.height;

  
  
  PRBool readjustAboveBelow = PR_FALSE;
  PRInt32 xpos = 0, ypos = 0;
  nsMargin margin;

  
  PRInt32 screenViewLocX, screenViewLocY;

  
  nsRect anchorScreenRect;
  nsRect rootScreenRect = rootFrame->GetScreenRectInAppUnits();

  nsIDeviceContext* devContext = PresContext()->DeviceContext();
  nscoord offsetForContextMenu = 0;
  if (mScreenXPos == -1 && mScreenYPos == -1) {
    
    
    
    
    
    if (mAnchorContent) {
      anchorScreenRect = aAnchorFrame->GetScreenRectInAppUnits();
      
      anchorScreenRect.ScaleRoundOut(adj);
      xpos = anchorScreenRect.x - rootScreenRect.x;
      ypos = anchorScreenRect.y - rootScreenRect.y;

      
      
      AdjustPositionForAnchorAlign(&xpos, &ypos, parentSize, &readjustAboveBelow);
    }
    else {
      
      anchorScreenRect = rootScreenRect;
      GetStyleMargin()->GetMargin(margin);
      xpos = margin.left;
      ypos = margin.top;
    }

    
    xpos += presContext->CSSPixelsToAppUnits(mXPos);
    ypos += presContext->CSSPixelsToAppUnits(mYPos);

    screenViewLocX = rootScreenRect.x + xpos;
    screenViewLocY = rootScreenRect.y + ypos;
  }
  else {
    
    
    
    
    PRInt32 factor = devContext->UnscaledAppUnitsPerDevPixel();
    screenViewLocX = nsPresContext::CSSPixelsToAppUnits(mScreenXPos) / factor;
    screenViewLocY = nsPresContext::CSSPixelsToAppUnits(mScreenYPos) / factor;

    if (mAdjustOffsetForContextMenu) {
      PRInt32 offsetForContextMenuDev =
        nsPresContext::CSSPixelsToAppUnits(2) / factor;
      offsetForContextMenu = presContext->DevPixelsToAppUnits(offsetForContextMenuDev);
    }

    
    
    GetStyleMargin()->GetMargin(margin);
    screenViewLocX = presContext->DevPixelsToAppUnits(screenViewLocX) +
        margin.left + offsetForContextMenu;
    screenViewLocY = presContext->DevPixelsToAppUnits(screenViewLocY) +
        margin.top + offsetForContextMenu;

    
    
    xpos = screenViewLocX - rootScreenRect.x;
    ypos = screenViewLocY - rootScreenRect.y;
  }

  
  
  
  nsRect rect;
  if ( mMenuCanOverlapOSBar ) {
    devContext->GetRect(rect);
  }
  else {
    devContext->GetClientRect(rect);
  }

  
  rect.width  -= nsPresContext::CSSPixelsToAppUnits(3);
  rect.height -= nsPresContext::CSSPixelsToAppUnits(3);

  
  if (mInContentShell) {
    rect.IntersectRect(rect, rootScreenRect);
  }

  PRInt32 screenLeftTwips   = rect.x;
  PRInt32 screenTopTwips    = rect.y;
  PRInt32 screenWidthTwips  = rect.width;
  PRInt32 screenHeightTwips = rect.height;
  PRInt32 screenRightTwips  = rect.XMost();
  PRInt32 screenBottomTwips = rect.YMost();

  if (mPopupAnchor != POPUPALIGNMENT_NONE && mScreenXPos == -1 && mScreenYPos == -1) {
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    if (screenViewLocY < screenTopTwips) {
      PRInt32 moveDist = screenTopTwips - screenViewLocY;
      screenViewLocY = screenTopTwips;
      ypos += moveDist;
    }
    
    
    if ( (screenViewLocX + mRect.width) > screenRightTwips ||
           screenViewLocX < screenLeftTwips ||
          (screenViewLocY + mRect.height) > screenBottomTwips ) {
      nsRect screenParentFrameRect(anchorScreenRect);

      
      
      PRBool switchSides = IsMoreRoomOnOtherSideOfParent ( readjustAboveBelow, screenViewLocX, screenViewLocY,
                                                            screenParentFrameRect, screenTopTwips, screenLeftTwips,
                                                            screenBottomTwips, screenRightTwips );
      
      
      
      if ( switchSides )
        MovePopupToOtherSideOfParent ( readjustAboveBelow, &xpos, &ypos, &screenViewLocX, &screenViewLocY, 
                                        screenParentFrameRect, screenTopTwips, screenLeftTwips,
                                        screenBottomTwips, screenRightTwips );
                                        
      
      
      if ( readjustAboveBelow ) {
        
        if ( (screenViewLocX + mRect.width) > screenRightTwips ) {
          PRInt32 moveDistX = (screenViewLocX + mRect.width) - screenRightTwips;
          if ( screenViewLocX - moveDistX < screenLeftTwips )
            moveDistX = screenViewLocX - screenLeftTwips;          
          screenViewLocX -= moveDistX;
          xpos -= moveDistX;
        } else if (screenViewLocX < screenLeftTwips) {
          
          PRInt32 moveDistX = screenLeftTwips - screenViewLocX;
          if ( (screenViewLocX + mRect.width + moveDistX) > screenRightTwips )
            moveDistX = screenRightTwips - screenViewLocX - mRect.width;
          screenViewLocX += moveDistX;
          xpos += moveDistX;
        }
      }
      else {
        
        















        if ( (screenViewLocY + mRect.height) > screenBottomTwips ) {
          PRInt32 moveDistY = (screenViewLocY + mRect.height) - screenBottomTwips;
          if ( screenViewLocY - moveDistY < screenTopTwips )
            moveDistY = screenViewLocY - screenTopTwips;          
          screenViewLocY -= moveDistY;
          ypos -= moveDistY; 
        } 
      }
      
      
      
      
      
      

      PRInt32 xSpillage = (screenViewLocX + mRect.width) - screenRightTwips;
      if ( xSpillage > 0 )
        mRect.width -= xSpillage;
      PRInt32 ySpillage = (screenViewLocY + mRect.height) - screenBottomTwips;
      if ( ySpillage > 0 )
        mRect.height -= ySpillage;

      
      if(mRect.width > screenWidthTwips) 
          mRect.width = screenWidthTwips;    
      if(mRect.height > screenHeightTwips)
          mRect.height = screenHeightTwips;   

    } 
  } 
  else {
  
    
    
    
    

    
    
    

    
    
    
    
    
    

    
    if(mRect.width > screenWidthTwips)
       mRect.width = screenWidthTwips;
    if(mRect.height > screenHeightTwips)
       mRect.height = screenHeightTwips;

    
    
    if ( screenViewLocX < screenLeftTwips ) {
      PRInt32 moveDistX = screenLeftTwips - screenViewLocX;
      xpos += moveDistX;
      screenViewLocX += moveDistX;
    }
    if ( (screenViewLocX + mRect.width) > screenRightTwips )
      xpos -= (screenViewLocX + mRect.width) - screenRightTwips;

    
    
    
    if ( screenViewLocY < screenTopTwips ) {
      PRInt32 moveDistY = screenTopTwips - screenViewLocY;
      ypos += moveDistY;
      screenViewLocY += moveDistY;
    }

    
    
    
    if ( (screenViewLocY + mRect.height) > screenBottomTwips ) {
      
      
      
      if (screenBottomTwips - screenViewLocY > screenViewLocY - screenTopTwips) {
        
        
        
        mRect.height = screenBottomTwips - screenViewLocY;
      } else {
        
        
        
        nscoord newBottomY =
          screenViewLocY - 2*offsetForContextMenu - margin.TopBottom();
        
        newBottomY = PR_MIN(newBottomY, screenBottomTwips);
        newBottomY = PR_MAX(newBottomY, screenTopTwips);
        if (mRect.height > newBottomY - screenTopTwips) {
          
          mRect.height = newBottomY - screenTopTwips;
        }
        
        ypos += newBottomY - screenViewLocY - mRect.height;
      }
    }
  }

  presContext->GetViewManager()->MoveViewTo(GetView(), xpos, ypos); 

  
  
  nsBoxFrame::SetPosition(nsPoint(xpos, ypos) -
                          GetParent()->GetOffsetTo(rootFrame));

  if (sizedToPopup) {
    nsBoxLayoutState state(PresContext());
    SetBounds(state, nsRect(mRect.x, mRect.y, parentSize.width, mRect.height));
  }

  return NS_OK;
}

 nsMenuFrame*
nsMenuPopupFrame::GetCurrentMenuItem()
{
  return mCurrentMenu;
}

PRBool nsMenuPopupFrame::ConsumeOutsideClicks()
{
  
  if (mConsumeRollupEvent != nsIPopupBoxObject::ROLLUP_DEFAULT)
    return (mConsumeRollupEvent == nsIPopupBoxObject::ROLLUP_CONSUME);

  nsCOMPtr<nsIContent> parentContent = mContent->GetParent();
  if (parentContent) {
    nsINodeInfo *ni = parentContent->NodeInfo();
    if (ni->Equals(nsGkAtoms::menulist, kNameSpaceID_XUL))
      return PR_TRUE;  
#if defined(XP_WIN) || defined(XP_OS2)
    
    if (ni->Equals(nsGkAtoms::menu, kNameSpaceID_XUL) ||
       (ni->Equals(nsGkAtoms::popupset, kNameSpaceID_XUL)))
      return PR_FALSE;
#endif
    if (ni->Equals(nsGkAtoms::textbox, kNameSpaceID_XUL)) {
      
      if (parentContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                     nsGkAtoms::autocomplete, eCaseMatters))
        return PR_FALSE;
    }
  }

  return PR_TRUE;
}

static nsIScrollableView* GetScrollableViewForFrame(nsIFrame* aFrame)
{
  nsIScrollableFrame* sf;
  nsresult rv = CallQueryInterface(aFrame, &sf);
  if (NS_FAILED(rv))
    return nsnull;
  return sf->GetScrollableView();
}



nsIScrollableView* nsMenuPopupFrame::GetScrollableView(nsIFrame* aStart)
{
  if ( ! aStart )
    return nsnull;  

  nsIFrame* currFrame;
  nsIScrollableView* scrollableView=nsnull;

  
  currFrame=aStart;
  do {
    scrollableView = GetScrollableViewForFrame(currFrame);
    if ( scrollableView )
      return scrollableView;
    currFrame = currFrame->GetNextSibling();
  } while ( currFrame );

  
  nsIFrame* childFrame;
  currFrame=aStart;
  do {
    childFrame = currFrame->GetFirstChild(nsnull);
    scrollableView=GetScrollableView(childFrame);
    if ( scrollableView )
      return scrollableView;
    currFrame = currFrame->GetNextSibling();
  } while ( currFrame );

  return nsnull;
}

void nsMenuPopupFrame::EnsureMenuItemIsVisible(nsMenuFrame* aMenuItem)
{
  if (aMenuItem) {
    nsIFrame* childFrame = GetFirstChild(nsnull);
    nsIScrollableView *scrollableView;
    scrollableView = GetScrollableView(childFrame);
    if (scrollableView) {
      nscoord scrollX, scrollY;

      nsRect viewRect = scrollableView->View()->GetBounds();
      nsRect itemRect = aMenuItem->GetRect();
      scrollableView->GetScrollPosition(scrollX, scrollY);
  
      
      if ( itemRect.y + itemRect.height > scrollY + viewRect.height )
        scrollableView->ScrollTo(scrollX, itemRect.y + itemRect.height - viewRect.height, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
      
      
      else if ( itemRect.y < scrollY )
        scrollableView->ScrollTo(scrollX, itemRect.y, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
    }
  }
}

NS_IMETHODIMP nsMenuPopupFrame::SetCurrentMenuItem(nsMenuFrame* aMenuItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  if (mCurrentMenu) {
    mCurrentMenu->SelectMenu(PR_FALSE);
  }

  if (aMenuItem) {
    EnsureMenuItemIsVisible(aMenuItem);
    aMenuItem->SelectMenu(PR_TRUE);
  }

  mCurrentMenu = aMenuItem;

  return NS_OK;
}

void
nsMenuPopupFrame::CurrentMenuIsBeingDestroyed()
{
  mCurrentMenu = nsnull;
}

NS_IMETHODIMP
nsMenuPopupFrame::ChangeMenuItem(nsMenuFrame* aMenuItem,
                                 PRBool aSelectFirstItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  
  
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (!mIsContextMenu && pm && pm->HasContextMenu(this))
    return NS_OK;

  
  if (mCurrentMenu) {
    mCurrentMenu->SelectMenu(PR_FALSE);
    nsMenuPopupFrame* popup = mCurrentMenu->GetPopup();
    if (popup) {
      if (mCurrentMenu->IsOpen()) {
        if (pm)
          pm->HidePopupAfterDelay(popup);
      }
    }
  }

  
  if (aMenuItem) {
    EnsureMenuItemIsVisible(aMenuItem);
    aMenuItem->SelectMenu(PR_TRUE);
  }

  mCurrentMenu = aMenuItem;

  return NS_OK;
}

nsMenuFrame*
nsMenuPopupFrame::Enter()
{
  mIncrementalString.Truncate();

  
  if (mCurrentMenu)
    return mCurrentMenu->Enter();

  return nsnull;
}

nsMenuFrame*
nsMenuPopupFrame::FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent, PRBool& doAction)
{
  PRUint32 charCode, keyCode;
  aKeyEvent->GetCharCode(&charCode);
  aKeyEvent->GetKeyCode(&keyCode);

  doAction = PR_FALSE;

  
  nsIFrame* immediateParent = nsnull;
  PresContext()->PresShell()->
    FrameConstructor()->GetInsertionPoint(this, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  PRUint32 matchCount = 0, matchShortcutCount = 0;
  PRBool foundActive = PR_FALSE;
  PRBool isShortcut;
  nsMenuFrame* frameBefore = nsnull;
  nsMenuFrame* frameAfter = nsnull;
  nsMenuFrame* frameShortcut = nsnull;

  nsIContent* parentContent = mContent->GetParent();

  PRBool isMenu = parentContent &&
                  !parentContent->NodeInfo()->Equals(nsGkAtoms::menulist, kNameSpaceID_XUL);

  static DOMTimeStamp lastKeyTime = 0;
  DOMTimeStamp keyTime;
  aKeyEvent->GetTimeStamp(&keyTime);

  if (charCode == 0) {
    if (keyCode == NS_VK_BACK) {
      if (!isMenu && !mIncrementalString.IsEmpty()) {
        mIncrementalString.SetLength(mIncrementalString.Length() - 1);
        return nsnull;
      }
      else {
#ifdef XP_WIN
        nsCOMPtr<nsISound> soundInterface = do_CreateInstance("@mozilla.org/sound;1");
        if (soundInterface)
          soundInterface->Beep();
#endif  
      }
    }
    return nsnull;
  }
  else {
    PRUnichar uniChar = ToLowerCase(static_cast<PRUnichar>(charCode));
    if (isMenu || 
        keyTime - lastKeyTime > INC_TYP_INTERVAL) 
      mIncrementalString = uniChar;
    else {
      mIncrementalString.Append(uniChar);
    }
  }

  
  nsAutoString incrementalString(mIncrementalString);
  PRUint32 charIndex = 1, stringLength = incrementalString.Length();
  while (charIndex < stringLength && incrementalString[charIndex] == incrementalString[charIndex - 1]) {
    charIndex++;
  }
  if (charIndex == stringLength) {
    incrementalString.Truncate(1);
    stringLength = 1;
  }

  lastKeyTime = keyTime;

  nsIFrame* currFrame;
  
  
  
  
  
  currFrame = immediateParent->GetFirstChild(nsnull);

  PRInt32 menuAccessKey = -1;
  nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);

  
  
  while (currFrame) {
    nsIContent* current = currFrame->GetContent();
    
    
    if (nsXULPopupManager::IsValidMenuItem(PresContext(), current, PR_TRUE)) {
      nsAutoString textKey;
      if (menuAccessKey >= 0) {
        
        current->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, textKey);
      }
      if (textKey.IsEmpty()) { 
        isShortcut = PR_FALSE;
        current->GetAttr(kNameSpaceID_None, nsGkAtoms::label, textKey);
        if (textKey.IsEmpty()) 
          current->GetAttr(kNameSpaceID_None, nsGkAtoms::value, textKey);
      }
      else
        isShortcut = PR_TRUE;

      if (StringBeginsWith(textKey, incrementalString,
                           nsCaseInsensitiveStringComparator())) {
        
        if (currFrame->GetType() == nsGkAtoms::menuFrame) {
          
          matchCount++;
          if (isShortcut) {
            
            matchShortcutCount++;
            
            frameShortcut = static_cast<nsMenuFrame *>(currFrame);
          }
          if (!foundActive) {
            
            if (!frameBefore)
              frameBefore = static_cast<nsMenuFrame *>(currFrame);
          }
          else {
            
            if (!frameAfter)
              frameAfter = static_cast<nsMenuFrame *>(currFrame);
          }
        }
        else
          return nsnull;
      }

      
      if (current->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menuactive,
                               nsGkAtoms::_true, eCaseMatters)) {
        foundActive = PR_TRUE;
        if (stringLength > 1) {
          
          
          if (currFrame == frameBefore)
            return frameBefore;
        }
      }
    }
    currFrame = currFrame->GetNextSibling();
  }

  doAction = (isMenu && (matchCount == 1 || matchShortcutCount == 1));

  if (matchShortcutCount == 1) 
    return frameShortcut;
  if (frameAfter) 
    return frameAfter;
  else if (frameBefore) 
    return frameBefore;

  
  mIncrementalString.SetLength(mIncrementalString.Length() - 1);

  
#ifdef XP_WIN
  
  
  if (isMenu) {
    nsCOMPtr<nsISound> soundInterface = do_CreateInstance("@mozilla.org/sound;1");
    if (soundInterface)
      soundInterface->Beep();
  }
#endif  

  return nsnull;
}

NS_IMETHODIMP
nsMenuPopupFrame::GetWidget(nsIWidget **aWidget)
{
  nsIView * view = GetRootViewForPopup(this);
  if (!view)
    return NS_OK;

  *aWidget = view->GetWidget();
  NS_IF_ADDREF(*aWidget);
  return NS_OK;
}

void
nsMenuPopupFrame::AttachedDismissalListener()
{
  mConsumeRollupEvent = nsIPopupBoxObject::ROLLUP_DEFAULT;
}



NS_IMETHODIMP 
nsMenuPopupFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32 aModType)

{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  
  if (aAttribute == nsGkAtoms::left || aAttribute == nsGkAtoms::top)
    MoveToAttributePosition();

  
  
  if (aAttribute == nsGkAtoms::menugenerated &&
      mFrames.IsEmpty() && !mGeneratedChildren) {
    PresContext()->PresShell()->FrameConstructor()->
      AddLazyChildren(mContent, LazyGeneratePopupDone, nsnull, PR_TRUE);
  }
  
  return rv;
}

void
nsMenuPopupFrame::MoveToAttributePosition()
{
  
  
  
  
  nsAutoString left, top;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::left, left);
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::top, top);
  PRInt32 err1, err2;
  mScreenXPos = left.ToInteger(&err1);
  mScreenYPos = top.ToInteger(&err2);

  if (NS_SUCCEEDED(err1) && NS_SUCCEEDED(err2))
    MoveToInternal(mScreenXPos, mScreenYPos);
}

void
nsMenuPopupFrame::Destroy()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm)
    pm->PopupDestroyed(this);

  nsBoxFrame::Destroy();
}

void
nsMenuPopupFrame::MoveTo(PRInt32 aLeft, PRInt32 aTop)
{
  
  nsAutoString left, top;
  left.AppendInt(aLeft);
  top.AppendInt(aTop);

  nsWeakFrame weakFrame(this);
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::left, left, PR_FALSE);
  if (!weakFrame.IsAlive()) {
    return;
  }
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::top, top, PR_FALSE);
  if (!weakFrame.IsAlive()) {
    return;
  }

  MoveToInternal(aLeft, aTop);
}

void
nsMenuPopupFrame::MoveToInternal(PRInt32 aLeft, PRInt32 aTop)
{
  
  if (mInContentShell)
    return;

  nsIView* view = GetView();
  NS_ASSERTION(view->GetParent(), "Must have parent!");

  
  nsIntPoint screenPos = view->GetParent()->GetScreenPosition();

  nsPresContext* context = PresContext();
  aLeft = context->AppUnitsToDevPixels(nsPresContext::CSSPixelsToAppUnits(aLeft));
  aTop = context->AppUnitsToDevPixels(nsPresContext::CSSPixelsToAppUnits(aTop));

  
  
  
  nsIWidget* widget = view->GetWidget();
  if (widget) 
    widget->Move(aLeft - screenPos.x, aTop - screenPos.y);
}

PRBool
nsMenuPopupFrame::GetAutoPosition()
{
  return mShouldAutoPosition;
}

void
nsMenuPopupFrame::SetAutoPosition(PRBool aShouldAutoPosition)
{
  mShouldAutoPosition = aShouldAutoPosition;
}

void
nsMenuPopupFrame::SetConsumeRollupEvent(PRUint32 aConsumeMode)
{
  mConsumeRollupEvent = aConsumeMode;
}
