










































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
#include "nsCSSFrameConstructor.h"
#include "nsIEventStateManager.h"
#include "nsIBoxLayout.h"
#include "nsIPopupBoxObject.h"
#include "nsIReflowCallback.h"
#include "nsBindingManager.h"
#ifdef XP_WIN
#include "nsISound.h"
#endif

const PRInt32 kMaxZ = 0x7fffffff; 

static nsPopupSetFrame*
GetPopupSetFrame(nsPresContext* aPresContext)
{
  nsIRootBox* rootBox = nsIRootBox::GetRootBox(aPresContext->PresShell());
  if (!rootBox)
    return nsnull;

  return rootBox->GetPopupSetFrame();
}





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
  mIsOpen(PR_FALSE),
  mIsOpenChanged(PR_FALSE),
  mIsOpenPending(PR_FALSE),
  mIsContextMenu(PR_FALSE),
  mGeneratedChildren(PR_FALSE),
  mMenuCanOverlapOSBar(PR_FALSE),
  mShouldAutoPosition(PR_TRUE),
  mConsumeRollupEvent(nsIPopupBoxObject::ROLLUP_DEFAULT),
  mInContentShell(PR_TRUE)
{
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

  PRBool viewHasTransparentContent = !mInContentShell &&
                                     nsLayoutUtils::FrameHasTransparency(this);
  nsIContent* parentContent = GetContent()->GetParent();
  nsIAtom *tag = nsnull;
  if (parentContent)
    tag = parentContent->Tag();
  widgetData.mDropShadow = !(viewHasTransparentContent || tag == nsGkAtoms::menulist);
  
#if defined(XP_MACOSX) || defined(XP_BEOS)
  static NS_DEFINE_IID(kCPopupCID,  NS_POPUP_CID);
  aView->CreateWidget(kCPopupCID, &widgetData, nsnull, PR_TRUE, PR_TRUE, 
                      eContentTypeUI);
#else
  static NS_DEFINE_IID(kCChildCID,  NS_CHILD_CID);
  aView->CreateWidget(kCChildCID, &widgetData, nsnull, PR_TRUE, PR_TRUE);
#endif
  aView->GetWidget()->SetWindowTranslucency(viewHasTransparentContent);
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

void
nsMenuPopupFrame::AdjustView()
{
  if (mIsOpen) {
    
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

  mIsOpenPending = PR_TRUE;
  mAnchorContent = aAnchorContent;
  mXPos = aXPos;
  mYPos = aYPos;

  
  
  
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
nsMenuPopupFrame::InitializePopupAtScreen(PRInt32 aXPos, PRInt32 aYPos)
{
  EnsureWidget();

  mIsOpenPending = PR_TRUE;
  mAnchorContent = nsnull;
  mScreenXPos = aXPos;
  mScreenYPos = aYPos;
  mPopupAnchor = POPUPALIGNMENT_NONE;
  mPopupAlignment = POPUPALIGNMENT_NONE;
}

void
nsMenuPopupFrame::InitializePopupWithAnchorAlign(nsIContent* aAnchorContent,
                                                 nsAString& aAnchor,
                                                 nsAString& aAlign,
                                                 PRInt32 aXPos, PRInt32 aYPos)
{
  EnsureWidget();

  mIsOpenPending = PR_TRUE;
  mXPos = aXPos;
  mYPos = aYPos;

  
  
  
  if (aXPos == -1 && aYPos == -1) {
    mAnchorContent = aAnchorContent;
    mScreenXPos = -1;
    mScreenYPos = -1;
    InitPositionFromAnchorAlign(aAnchor, aAlign);
  }
  else {
    mAnchorContent = nsnull;
    mPopupAnchor = POPUPALIGNMENT_NONE;
    mPopupAlignment = POPUPALIGNMENT_NONE;
    mScreenXPos = aXPos;
    mScreenYPos = aYPos;
  }
}

void PR_CALLBACK
LazyGeneratePopupDone(nsIContent* aPopup, nsIFrame* aFrame, void* aArg)
{
  
  if (aFrame->GetType() == nsGkAtoms::menuPopupFrame) {
    nsWeakFrame weakFrame(aFrame);
    nsMenuPopupFrame* popupFrame = NS_STATIC_CAST(nsMenuPopupFrame*, aFrame);

    popupFrame->SetGeneratedChildren();

    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm && popupFrame->IsMenu()) {
      nsCOMPtr<nsIContent> popup = aPopup;
      PRBool selectFirstItem = (PRBool)NS_PTR_TO_INT32(aArg);
      if (selectFirstItem) {
        nsMenuFrame* next = pm->GetNextMenuItem(popupFrame, nsnull, PR_TRUE);
        popupFrame->SetCurrentMenuItem(next);
      }

      pm->UpdateMenuItems(popup);
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

  if (!mIsOpen) {
    mIsOpen = PR_TRUE;
    mIsOpenChanged = PR_TRUE;

    nsIFrame* parent = GetParent();
    if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
      nsWeakFrame weakFrame(this);
      (NS_STATIC_CAST(nsMenuFrame*, parent))->PopupOpened();
      if (!weakFrame.IsAlive())
        return PR_FALSE;
      PresContext()->RootPresContext()->NotifyAddedActivePopupToTop(this);
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
nsMenuPopupFrame::HidePopup(PRBool aDeselectMenu)
{
  if (mIsOpen) {
    if (IsMenu())
      SetCurrentMenuItem(nsnull);

    mIncrementalString.Truncate();

    mIsOpen = PR_FALSE;
    mIsOpenChanged = PR_FALSE;
    mCurrentMenu = nsnull; 
 
    nsIView* view = GetView();
    nsIViewManager* viewManager = view->GetViewManager();
    viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
    viewManager->ResizeView(view, nsRect(0, 0, 0, 0));

    FireDOMEvent(NS_LITERAL_STRING("DOMMenuInactive"), mContent);
  }

  
  
  
  nsIEventStateManager *esm = PresContext()->EventStateManager();

  PRInt32 state;
  esm->GetContentState(mContent, state);

  if (state & NS_EVENT_STATE_HOVER)
    esm->SetContentState(nsnull, NS_EVENT_STATE_HOVER);

  nsIFrame* parent = GetParent();
  if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
    (NS_STATIC_CAST(nsMenuFrame*, parent))->PopupClosed(aDeselectMenu);
    PresContext()->RootPresContext()->NotifyRemovedActivePopup(this);
  }
}

void
nsMenuPopupFrame::InvalidateInternal(const nsRect& aDamageRect,
                                     nscoord aX, nscoord aY, nsIFrame* aForChild,
                                     PRBool aImmediate)
{
  InvalidateRoot(aDamageRect, aX, aY, aImmediate);
}

void
nsMenuPopupFrame::GetLayoutFlags(PRUint32& aFlags)
{
  aFlags = NS_FRAME_NO_SIZE_VIEW | NS_FRAME_NO_MOVE_VIEW | NS_FRAME_NO_VISIBILITY;
}





void
nsMenuPopupFrame::GetViewOffset(nsIView* aView, nsPoint& aPoint)
{
  
  
  
  
  
  
  
  
  
  nsIView* rootView;
  aView->GetViewManager()->GetRootView(rootView);
  aPoint = aView->GetOffsetTo(rootView);
}








nsIView*
nsMenuPopupFrame::GetRootViewForPopup(nsIFrame* aStartFrame,
                                      PRBool    aStopAtViewManagerRoot)
{
  nsIView* view = aStartFrame->GetClosestView();
  NS_ASSERTION(view, "frame must have a closest view!");
  if (view) {
    nsIView* rootView = nsnull;
    if (aStopAtViewManagerRoot) {
      view->GetViewManager()->GetRootView(rootView);
    }
    
    while (view) {
      
      
      
      nsIWidget* widget = view->GetWidget();
      if (widget) {
        nsWindowType wtype;
        widget->GetWindowType(wtype);
        if (wtype == eWindowType_popup) {
          return view;
        }
      }

      if (aStopAtViewManagerRoot && view == rootView) {
        return view;
      }

      nsIView* temp = view->GetParent();
      if (!temp) {
        
        
        return view;
      }
      view = temp;
    }
  }

  return nsnull;
}









void
nsMenuPopupFrame::AdjustPositionForAnchorAlign(PRInt32* ioXPos, PRInt32* ioYPos, const nsRect & inParentRect,
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
    *ioXPos += inParentRect.width;
  }
  else if (popupAnchor == POPUPALIGNMENT_TOPLEFT && popupAlign == POPUPALIGNMENT_TOPLEFT) {
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == POPUPALIGNMENT_TOPRIGHT && popupAlign == POPUPALIGNMENT_BOTTOMRIGHT) {
    *ioXPos -= (mRect.width - inParentRect.width);
    *ioYPos -= mRect.height;
    *outFlushWithTopBottom = PR_TRUE;
  }
  else if (popupAnchor == POPUPALIGNMENT_BOTTOMRIGHT && popupAlign == POPUPALIGNMENT_BOTTOMLEFT) {
    *ioXPos += inParentRect.width;
    *ioYPos -= (mRect.height - inParentRect.height);
  }
  else if (popupAnchor == POPUPALIGNMENT_BOTTOMRIGHT && popupAlign == POPUPALIGNMENT_TOPRIGHT) {
    *ioXPos -= (mRect.width - inParentRect.width);
    *ioYPos += inParentRect.height;
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
    *ioYPos -= (mRect.height - inParentRect.height);
  }
  else if (popupAnchor == POPUPALIGNMENT_BOTTOMLEFT && popupAlign == POPUPALIGNMENT_TOPLEFT) {
    *ioYPos += inParentRect.height;
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

  
  
  
  if (!aAnchorFrame) {
    if (mAnchorContent) {
      nsCOMPtr<nsIDocument> document = mAnchorContent->GetDocument();
      nsIPresShell *shell = document->GetPrimaryShell();
      if (!shell)
        return NS_ERROR_FAILURE;
      
      aAnchorFrame = shell->GetPrimaryFrameFor(mAnchorContent);
    }
    else {
      aAnchorFrame = presContext->PresShell()->FrameManager()->GetRootFrame();
    }

    if (!aAnchorFrame)
      return NS_OK;
  }
  else {
    
    sizedToPopup = nsMenuFrame::IsSizedToPopup(aAnchorFrame->GetContent(), PR_FALSE);
  }

  
  
  
  
  nsIView* containingView = nsnull;
  nsPoint offset;
  nsMargin margin;
  containingView = aAnchorFrame->GetClosestView(&offset);
  if (!containingView)
    return NS_OK;

  
  
  
  nsPoint parentPos;
  GetViewOffset(containingView, parentPos);

  
  
  nsRect parentRect = aAnchorFrame->GetRect();

  
  nsIPresShell *presShell = presContext->PresShell();
  nsIDocument *document = presShell->GetDocument();

  
  
  if (sizedToPopup) {
    mRect.width = parentRect.width;
  }

  
  
  nsPoint parentViewWidgetOffset;
  nsIWidget* parentViewWidget = containingView->GetNearestWidget(&parentViewWidgetOffset);
  nsRect localParentWidgetRect(0,0,0,0), screenParentWidgetRect;
  parentViewWidget->WidgetToScreen ( localParentWidgetRect, screenParentWidgetRect );

  
  
  PRBool readjustAboveBelow = PR_FALSE;
  PRInt32 xpos = 0, ypos = 0;
  PRInt32 screenViewLocX, screenViewLocY;

  if (mScreenXPos == -1 && mScreenYPos == -1) {
    
    
    
    
    

    if (mAnchorContent) {
      xpos = parentPos.x + offset.x;
      ypos = parentPos.y + offset.y;

      
      
      AdjustPositionForAnchorAlign(&xpos, &ypos, parentRect, &readjustAboveBelow);

      
      xpos += presContext->DevPixelsToAppUnits(mXPos);
      ypos += presContext->DevPixelsToAppUnits(mYPos);
    }
    else {
      GetStyleMargin()->GetMargin(margin);
      xpos = presContext->DevPixelsToAppUnits(mXPos) + margin.left;
      ypos = presContext->DevPixelsToAppUnits(mYPos) + margin.top;
    }

    
    
    
    
    
    
    

    
    
    
    
    
    nsIView* parentView = GetRootViewForPopup(aAnchorFrame, PR_FALSE);
    if (!parentView)
      return NS_OK;

    screenViewLocX = presContext->DevPixelsToAppUnits(screenParentWidgetRect.x) +
      (xpos - parentPos.x) + parentViewWidgetOffset.x;
    screenViewLocY = presContext->DevPixelsToAppUnits(screenParentWidgetRect.y) +
      (ypos - parentPos.y) + parentViewWidgetOffset.y;
  }
  else {
    
    GetStyleMargin()->GetMargin(margin);
    screenViewLocX = nsPresContext::CSSPixelsToAppUnits(mScreenXPos) + margin.left;
    screenViewLocY = nsPresContext::CSSPixelsToAppUnits(mScreenYPos) + margin.top;

    xpos = screenViewLocX - presContext->DevPixelsToAppUnits(screenParentWidgetRect.x) -
           parentViewWidgetOffset.x - parentPos.x;
    ypos = screenViewLocY - presContext->DevPixelsToAppUnits(screenParentWidgetRect.y) -
           parentViewWidgetOffset.y - parentPos.y;

    
    mShouldAutoPosition = PR_FALSE;
  }

  
  
  
  nsPIDOMWindow *window = document->GetWindow();
  if (!window)
    return NS_OK;

  nsIDeviceContext* devContext = PresContext()->DeviceContext();
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
    nsRect rootScreenRect = presShell->GetRootFrame()->GetScreenRect();
    rootScreenRect.ScaleRoundIn(presContext->AppUnitsPerDevPixel());
    rect.IntersectRect(rect, rootScreenRect);
  }

  PRInt32 screenLeftTwips   = rect.x;
  PRInt32 screenTopTwips    = rect.y;
  PRInt32 screenWidthTwips  = rect.width;
  PRInt32 screenHeightTwips = rect.height;
  PRInt32 screenRightTwips  = rect.XMost();
  PRInt32 screenBottomTwips = rect.YMost();

  if (mPopupAnchor != POPUPALIGNMENT_NONE) {
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    



    
    
    nsRect screenParentFrameRect (presContext->AppUnitsToDevPixels(offset.x), presContext->AppUnitsToDevPixels(offset.y),
                                    parentRect.width, parentRect.height );
    parentViewWidget->WidgetToScreen ( screenParentFrameRect, screenParentFrameRect );
    screenParentFrameRect.x = presContext->DevPixelsToAppUnits(screenParentFrameRect.x);
    screenParentFrameRect.y = presContext->DevPixelsToAppUnits(screenParentFrameRect.y);

    
    if (screenViewLocY < screenTopTwips) {
      PRInt32 moveDist = screenTopTwips - screenViewLocY;
      screenViewLocY = screenTopTwips;
      ypos += moveDist;
    }
    
    
    if ( (screenViewLocX + mRect.width) > screenRightTwips ||
           screenViewLocX < screenLeftTwips ||
          (screenViewLocY + mRect.height) > screenBottomTwips ) {
      
      
      
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
          
          PRInt32 moveDistY = (screenViewLocY + mRect.height) - screenHeightTwips;
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
      
      
      
      if (screenBottomTwips - screenViewLocY >
          screenViewLocY - screenTopTwips) {
        
        
        
        mRect.height = screenBottomTwips - screenViewLocY;
      } else {
        
        
        if (mRect.height > screenViewLocY - screenTopTwips) {
          
          mRect.height = screenViewLocY - screenTopTwips;
        }
        ypos -= (mRect.height + margin.top + margin.bottom);
      }
    }
  }  

  presContext->GetViewManager()->MoveViewTo(GetView(), xpos, ypos); 

  
  nsPoint frameOrigin = GetPosition();
  nsPoint offsetToView;
  GetOriginToViewOffset(offsetToView, nsnull);
  frameOrigin -= offsetToView;
  nsBoxFrame::SetPosition(frameOrigin);

  if (sizedToPopup) {
    nsBoxLayoutState state(PresContext());
    SetBounds(state, nsRect(mRect.x, mRect.y, parentRect.width, mRect.height));
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
    PRUnichar uniChar = ToLowerCase(NS_STATIC_CAST(PRUnichar, charCode));
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
            
            frameShortcut = NS_STATIC_CAST(nsMenuFrame *, currFrame);
          }
          if (!foundActive) {
            
            if (!frameBefore)
              frameBefore = NS_STATIC_CAST(nsMenuFrame *, currFrame);
          }
          else {
            
            if (!frameAfter)
              frameAfter = NS_STATIC_CAST(nsMenuFrame *, currFrame);
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
  
  
  nsIView * view = GetRootViewForPopup(this, PR_FALSE);
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
      AddLazyChildren(mContent, LazyGeneratePopupDone, nsnull);
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

  nsPresContext* rootPresContext = PresContext()->RootPresContext();
  if (rootPresContext->ContainsActivePopup(this)) {
    rootPresContext->NotifyRemovedActivePopup(this);
  }

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

  
  
  view->GetWidget()->Move(aLeft - screenPos.x, aTop - screenPos.y);
}

void 
nsMenuPopupFrame::GetAutoPosition(PRBool* aShouldAutoPosition)
{
  *aShouldAutoPosition = mShouldAutoPosition;
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
