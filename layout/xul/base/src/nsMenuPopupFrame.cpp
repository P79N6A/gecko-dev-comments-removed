









































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
#include "nsRect.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsGUIEvent.h"
#include "nsIRootBox.h"
#include "nsIDocShellTreeItem.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsLayoutUtils.h"
#include "nsContentUtils.h"
#include "nsCSSFrameConstructor.h"
#include "nsEventStateManager.h"
#include "nsIBoxLayout.h"
#include "nsIPopupBoxObject.h"
#include "nsPIWindowRoot.h"
#include "nsIReflowCallback.h"
#include "nsBindingManager.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#include "nsISound.h"
#include "nsIRootBox.h"
#include "nsIScreenManager.h"
#include "nsIServiceManager.h"
#include "nsThemeConstants.h"

PRInt8 nsMenuPopupFrame::sDefaultLevelIsTop = -1;





nsIFrame*
NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMenuPopupFrame (aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMenuPopupFrame)




nsMenuPopupFrame::nsMenuPopupFrame(nsIPresShell* aShell, nsStyleContext* aContext)
  :nsBoxFrame(aShell, aContext),
  mCurrentMenu(nsnull),
  mPrefSize(-1, -1),
  mPopupType(ePopupTypePanel),
  mPopupState(ePopupClosed),
  mPopupAlignment(POPUPALIGNMENT_NONE),
  mPopupAnchor(POPUPALIGNMENT_NONE),
  mFlipBoth(PR_FALSE),
  mIsOpenChanged(PR_FALSE),
  mIsContextMenu(PR_FALSE),
  mAdjustOffsetForContextMenu(PR_FALSE),
  mGeneratedChildren(PR_FALSE),
  mMenuCanOverlapOSBar(PR_FALSE),
  mShouldAutoPosition(PR_TRUE),
  mConsumeRollupEvent(nsIPopupBoxObject::ROLLUP_DEFAULT),
  mInContentShell(PR_TRUE),
  mIsMenuLocked(PR_FALSE),
  mHFlip(PR_FALSE),
  mVFlip(PR_FALSE)
{
  
  
  if (sDefaultLevelIsTop >= 0)
    return;
  sDefaultLevelIsTop =
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

  rv = CreatePopupViewForFrame();
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

  if (aContent->NodeInfo()->Equals(nsGkAtoms::tooltip, kNameSpaceID_XUL) &&
      aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::_default,
                            nsGkAtoms::_true, eIgnoreCase)) {
    nsIRootBox* rootBox =
      nsIRootBox::GetRootBox(PresContext()->GetPresShell());
    if (rootBox) {
      rootBox->SetDefaultTooltip(aContent);
    }
  }

  return rv;
}

PRBool
nsMenuPopupFrame::IsNoAutoHide() const
{
  
  
  
  return (!mInContentShell && mPopupType == ePopupTypePanel &&
           mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::noautohide,
                                 nsGkAtoms::_true, eIgnoreCase));
}

nsPopupLevel
nsMenuPopupFrame::PopupLevel(PRBool aIsNoAutoHide) const
{
  
  
  
  
  
  

  
  if (mPopupType != ePopupTypePanel)
    return ePopupLevelTop;

  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::top, &nsGkAtoms::parent, &nsGkAtoms::floating, nsnull};
  switch (mContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::level,
                                    strings, eCaseMatters)) {
    case 0:
      return ePopupLevelTop;
    case 1:
      return ePopupLevelParent;
    case 2:
      return ePopupLevelFloating;
  }

  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::titlebar))
    return ePopupLevelFloating;

  
  if (aIsNoAutoHide)
    return ePopupLevelParent;

  
  return sDefaultLevelIsTop ? ePopupLevelTop : ePopupLevelParent;
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
  widgetData.mNoAutoHide = IsNoAutoHide();

  nsAutoString title;
  if (mContent && widgetData.mNoAutoHide) {
    if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::titlebar,
                              nsGkAtoms::normal, eCaseMatters)) {
      widgetData.mBorderStyle = eBorderStyle_title;

      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, title);

      if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::close,
                                nsGkAtoms::_true, eCaseMatters)) {
        widgetData.mBorderStyle =
          static_cast<enum nsBorderStyle>(widgetData.mBorderStyle | eBorderStyle_close);
      }
    }
  }

  nsTransparencyMode mode = nsLayoutUtils::GetFrameTransparency(this, this);
  PRBool viewHasTransparentContent = !mInContentShell &&
                                     (eTransparencyTransparent ==
                                      mode);
  nsIContent* parentContent = GetContent()->GetParent();
  nsIAtom *tag = nsnull;
  if (parentContent)
    tag = parentContent->Tag();
  widgetData.mDropShadow = !(viewHasTransparentContent || tag == nsGkAtoms::menulist);
  widgetData.mPopupLevel = PopupLevel(widgetData.mNoAutoHide);

  
  
  
  nsCOMPtr<nsIWidget> parentWidget;
  if (widgetData.mPopupLevel != ePopupLevelTop) {
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

  nsresult rv = aView->CreateWidgetForPopup(&widgetData, parentWidget,
                                            PR_TRUE, PR_TRUE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsIWidget* widget = aView->GetWidget();
  widget->SetTransparencyMode(mode);
  widget->SetWindowShadowStyle(GetShadowStyle());

  
  if (!title.IsEmpty()) {
    widget->SetTitle(title);
  }

  return NS_OK;
}

PRUint8
nsMenuPopupFrame::GetShadowStyle()
{
  PRUint8 shadow = GetStyleUIReset()->mWindowShadow;
  if (shadow != NS_STYLE_WINDOW_SHADOW_DEFAULT)
    return shadow;

  switch (GetStyleDisplay()->mAppearance) {
    case NS_THEME_TOOLTIP:
      return NS_STYLE_WINDOW_SHADOW_TOOLTIP;
    case NS_THEME_MENUPOPUP:
      return NS_STYLE_WINDOW_SHADOW_MENU;
  }
  return NS_STYLE_WINDOW_SHADOW_DEFAULT;
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
                                      nsFrameList& aChildList)
{
  
  if (aChildList.NotEmpty())
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
nsMenuPopupFrame::LayoutPopup(nsBoxLayoutState& aState, nsIFrame* aParentMenu, PRBool aSizedToPopup)
{
  if (!mGeneratedChildren)
    return;

  PRBool shouldPosition = PR_TRUE;
  PRBool isOpen = IsOpen();
  if (!isOpen) {
    
    
    shouldPosition = (mPopupState == ePopupShowing);
    if (!shouldPosition && !aSizedToPopup)
      return;
  }

  
  if (mIsOpenChanged) {
    nsIScrollableFrame *scrollframe = do_QueryFrame(GetChildBox());
    if (scrollframe) {
      scrollframe->ScrollTo(nsPoint(0,0), nsIScrollableFrame::INSTANT);
    }
  }

  
  
  nsSize prefSize = GetPrefSize(aState);
  nsSize minSize = GetMinSize(aState); 
  nsSize maxSize = GetMaxSize(aState);

  if (aSizedToPopup) {
    prefSize.width = aParentMenu->GetRect().width;
  }
  prefSize = BoundsCheck(minSize, prefSize, maxSize);

  
  PRBool sizeChanged = (mPrefSize != prefSize);
  if (sizeChanged) {
    SetBounds(aState, nsRect(0, 0, prefSize.width, prefSize.height), PR_FALSE);
    mPrefSize = prefSize;
  }

  if (shouldPosition) {
    SetPopupPosition(aParentMenu, PR_FALSE);
  }

  nsRect bounds(GetRect());
  Layout(aState);

  
  
  
  
  if (!aParentMenu) {
    nsSize newsize = GetSize();
    if (newsize.width > bounds.width || newsize.height > bounds.height) {
      
      
      mPrefSize = newsize;
      if (isOpen) {
        SetPopupPosition(nsnull, PR_FALSE);
      }
    }
  }

  nsPresContext* pc = PresContext();
  if (isOpen) {
    nsIView* view = GetView();
    nsIViewManager* viewManager = view->GetViewManager();
    nsRect rect = GetRect();
    rect.x = rect.y = 0;

    
    
    
    
    
    if (mPopupType == ePopupTypePanel && view) {
      nsIWidget* widget = view->GetWidget();
      if (widget) {
        nsIntSize popupSize = nsIntSize(pc->AppUnitsToDevPixels(rect.width),
                                        pc->AppUnitsToDevPixels(rect.height));
        popupSize = widget->ClientToWindowSize(popupSize);
        rect.width = pc->DevPixelsToAppUnits(popupSize.width);
        rect.height = pc->DevPixelsToAppUnits(popupSize.height);
      }
    }
    viewManager->ResizeView(view, rect);

    viewManager->SetViewVisibility(view, nsViewVisibility_kShow);
    mPopupState = ePopupOpenAndVisible;
    nsContainerFrame::SyncFrameViewProperties(pc, this, nsnull, view, 0);
  }

  
  if (mIsOpenChanged) {
    mIsOpenChanged = PR_FALSE;
    nsCOMPtr<nsIRunnable> event = new nsXULPopupShownEvent(GetContent(), pc);
    NS_DispatchToCurrentThread(event);
  }
}

nsIContent*
nsMenuPopupFrame::GetTriggerContent(nsMenuPopupFrame* aMenuPopupFrame)
{
  while (aMenuPopupFrame) {
    if (aMenuPopupFrame->mTriggerContent)
      return aMenuPopupFrame->mTriggerContent;

    
    nsMenuFrame* menuFrame = aMenuPopupFrame->GetParentMenu();
    if (!menuFrame)
      break;

    nsMenuParent* parentPopup = menuFrame->GetMenuParent();
    if (!parentPopup || !parentPopup->IsMenu())
      break;

    aMenuPopupFrame = static_cast<nsMenuPopupFrame *>(parentPopup);
  }

  return nsnull;
}

void
nsMenuPopupFrame::InitPositionFromAnchorAlign(const nsAString& aAnchor,
                                              const nsAString& aAlign)
{
  mTriggerContent = nsnull;

  if (aAnchor.EqualsLiteral("topleft"))
    mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
  else if (aAnchor.EqualsLiteral("topright"))
    mPopupAnchor = POPUPALIGNMENT_TOPRIGHT;
  else if (aAnchor.EqualsLiteral("bottomleft"))
    mPopupAnchor = POPUPALIGNMENT_BOTTOMLEFT;
  else if (aAnchor.EqualsLiteral("bottomright"))
    mPopupAnchor = POPUPALIGNMENT_BOTTOMRIGHT;
  else if (aAnchor.EqualsLiteral("leftcenter"))
    mPopupAnchor = POPUPALIGNMENT_LEFTCENTER;
  else if (aAnchor.EqualsLiteral("rightcenter"))
    mPopupAnchor = POPUPALIGNMENT_RIGHTCENTER;
  else if (aAnchor.EqualsLiteral("topcenter"))
    mPopupAnchor = POPUPALIGNMENT_TOPCENTER;
  else if (aAnchor.EqualsLiteral("bottomcenter"))
    mPopupAnchor = POPUPALIGNMENT_BOTTOMCENTER;
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
                                  nsIContent* aTriggerContent,
                                  const nsAString& aPosition,
                                  PRInt32 aXPos, PRInt32 aYPos,
                                  PRBool aAttributesOverride)
{
  EnsureWidget();

  mPopupState = ePopupShowing;
  mAnchorContent = aAnchorContent;
  mTriggerContent = aTriggerContent;
  mXPos = aXPos;
  mYPos = aYPos;
  mAdjustOffsetForContextMenu = PR_FALSE;

  
  
  
  if (aAnchorContent) {
    nsAutoString anchor, align, position, flip;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::popupanchor, anchor);
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::popupalign, align);
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::position, position);
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::flip, flip);

    if (aAttributesOverride) {
      
      
      if (anchor.IsEmpty() && align.IsEmpty() && position.IsEmpty())
        position.Assign(aPosition);
      else
        mXPos = mYPos = 0;
    }
    else if (!aPosition.IsEmpty()) {
      position.Assign(aPosition);
    }

    mFlipBoth = flip.EqualsLiteral("both");

    position.CompressWhitespace();
    PRInt32 spaceIdx = position.FindChar(' ');
    
    
    if (spaceIdx >= 0) {
      InitPositionFromAnchorAlign(Substring(position, 0, spaceIdx), Substring(position, spaceIdx + 1));
    }
    else if (position.EqualsLiteral("before_start")) {
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
      mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
      mPopupAlignment = POPUPALIGNMENT_TOPLEFT;
      
      
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
nsMenuPopupFrame::InitializePopupAtScreen(nsIContent* aTriggerContent,
                                          PRInt32 aXPos, PRInt32 aYPos,
                                          PRBool aIsContextMenu)
{
  EnsureWidget();

  mPopupState = ePopupShowing;
  mAnchorContent = nsnull;
  mTriggerContent = aTriggerContent;
  mScreenXPos = aXPos;
  mScreenYPos = aYPos;
  mFlipBoth = PR_FALSE;
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
  mFlipBoth = PR_FALSE;

  
  
  
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
nsMenuPopupFrame::ShowPopup(PRBool aIsContextMenu, PRBool aSelectFirstItem)
{
  mIsContextMenu = aIsContextMenu;

  if (mPopupState == ePopupShowing) {
    mPopupState = ePopupOpen;
    mIsOpenChanged = PR_TRUE;

    nsMenuFrame* menuFrame = GetParentMenu();
    if (menuFrame) {
      nsWeakFrame weakFrame(this);
      menuFrame->PopupOpened();
      if (!weakFrame.IsAlive())
        return;
    }

    
    
    PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                                 NS_FRAME_HAS_DIRTY_CHILDREN);

    if (mPopupType == ePopupTypeMenu) {
      nsCOMPtr<nsISound> sound(do_CreateInstance("@mozilla.org/sound;1"));
      if (sound)
        sound->PlayEventSound(nsISound::EVENT_MENU_POPUP);
    }
  }

  mShouldAutoPosition = PR_TRUE;
}

void
nsMenuPopupFrame::HidePopup(PRBool aDeselectMenu, nsPopupState aNewState)
{
  NS_ASSERTION(aNewState == ePopupClosed || aNewState == ePopupInvisible,
               "popup being set to unexpected state");

  
  if (mPopupState == ePopupClosed || mPopupState == ePopupShowing)
    return;

  
  
  
  if (aNewState == ePopupClosed) {
    
    
    if (mTriggerContent) {
      nsIDocument* doc = mContent->GetCurrentDoc();
      if (doc) {
        nsPIDOMWindow* win = doc->GetWindow();
        if (win) {
          nsCOMPtr<nsPIWindowRoot> root = win->GetTopWindowRoot();
          if (root) {
            root->SetPopupNode(nsnull);
          }
        }
      }
    }
    mTriggerContent = nsnull;
    mAnchorContent = nsnull;
  }

  
  
  if (mPopupState == ePopupInvisible) {
    if (aNewState == ePopupClosed)
      mPopupState = ePopupClosed;
    return;
  }

  mPopupState = aNewState;

  if (IsMenu())
    SetCurrentMenuItem(nsnull);

  mIncrementalString.Truncate();

  LockMenuUntilClosed(PR_FALSE);

  mIsOpenChanged = PR_FALSE;
  mCurrentMenu = nsnull; 
  mHFlip = mVFlip = PR_FALSE;

  nsIView* view = GetView();
  nsIViewManager* viewManager = view->GetViewManager();
  viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
  viewManager->ResizeView(view, nsRect(0, 0, 0, 0));

  FireDOMEvent(NS_LITERAL_STRING("DOMMenuInactive"), mContent);

  
  
  
  nsEventStateManager *esm = PresContext()->EventStateManager();

  nsEventStates state = esm->GetContentState(mContent);

  if (state.HasState(NS_EVENT_STATE_HOVER))
    esm->SetContentState(nsnull, NS_EVENT_STATE_HOVER);

  nsMenuFrame* menuFrame = GetParentMenu();
  if (menuFrame) {
    menuFrame->PopupClosed(aDeselectMenu);
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

nsPoint
nsMenuPopupFrame::AdjustPositionForAnchorAlign(nsRect& anchorRect,
                                               FlipStyle& aHFlip, FlipStyle& aVFlip)
{
  
  PRInt8 popupAnchor(mPopupAnchor);
  PRInt8 popupAlign(mPopupAlignment);
  if (IsDirectionRTL()) {
    
    if (popupAnchor < POPUPALIGNMENT_LEFTCENTER) {
      popupAnchor = -popupAnchor;
    }
    popupAlign = -popupAlign;
  }

  
  nsPoint pnt;
  switch (popupAnchor) {
    case POPUPALIGNMENT_LEFTCENTER:
      pnt = nsPoint(anchorRect.x, anchorRect.y + anchorRect.height / 2);
      anchorRect.y = pnt.y;
      anchorRect.height = 0;
      break;
    case POPUPALIGNMENT_RIGHTCENTER:
      pnt = nsPoint(anchorRect.XMost(), anchorRect.y + anchorRect.height / 2);
      anchorRect.y = pnt.y;
      anchorRect.height = 0;
      break;
    case POPUPALIGNMENT_TOPCENTER:
      pnt = nsPoint(anchorRect.x + anchorRect.width / 2, anchorRect.y);
      anchorRect.x = pnt.x;
      anchorRect.width = 0;
      break;
    case POPUPALIGNMENT_BOTTOMCENTER:
      pnt = nsPoint(anchorRect.x + anchorRect.width / 2, anchorRect.YMost());
      anchorRect.x = pnt.x;
      anchorRect.width = 0;
      break;
    case POPUPALIGNMENT_TOPRIGHT:
      pnt = anchorRect.TopRight();
      break;
    case POPUPALIGNMENT_BOTTOMLEFT:
      pnt = anchorRect.BottomLeft();
      break;
    case POPUPALIGNMENT_BOTTOMRIGHT:
      pnt = anchorRect.BottomRight();
      break;
    case POPUPALIGNMENT_TOPLEFT:
    default:
      pnt = anchorRect.TopLeft();
      break;
  }

  
  
  
  
  nsMargin margin(0, 0, 0, 0);
  GetStyleMargin()->GetMargin(margin);
  switch (popupAlign) {
    case POPUPALIGNMENT_TOPRIGHT:
      pnt.MoveBy(-mRect.width - margin.right, margin.top);
      break;
    case POPUPALIGNMENT_BOTTOMLEFT:
      pnt.MoveBy(margin.left, -mRect.height - margin.bottom);
      break;
    case POPUPALIGNMENT_BOTTOMRIGHT:
      pnt.MoveBy(-mRect.width - margin.right, -mRect.height - margin.bottom);
      break;
    case POPUPALIGNMENT_TOPLEFT:
    default:
      pnt.MoveBy(margin.left, margin.top);
      break;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (popupAnchor >= POPUPALIGNMENT_LEFTCENTER) {
    if (popupAnchor == POPUPALIGNMENT_LEFTCENTER ||
        popupAnchor == POPUPALIGNMENT_RIGHTCENTER) {
      aHFlip = FlipStyle_Outside;
      aVFlip = FlipStyle_Inside;
    }
    else {
      aHFlip = FlipStyle_Inside;
      aVFlip = FlipStyle_Outside;
    }
  }
  else {
    FlipStyle anchorEdge = mFlipBoth ? FlipStyle_Inside : FlipStyle_None;
    aHFlip = (popupAnchor == -popupAlign) ? FlipStyle_Outside : anchorEdge;
    if (((popupAnchor > 0) == (popupAlign > 0)) ||
        (popupAnchor == POPUPALIGNMENT_TOPLEFT && popupAlign == POPUPALIGNMENT_TOPLEFT))
      aVFlip = FlipStyle_Outside;
    else
      aVFlip = anchorEdge;
  }

  return pnt;
}

nscoord
nsMenuPopupFrame::FlipOrResize(nscoord& aScreenPoint, nscoord aSize, 
                               nscoord aScreenBegin, nscoord aScreenEnd,
                               nscoord aAnchorBegin, nscoord aAnchorEnd,
                               nscoord aMarginBegin, nscoord aMarginEnd,
                               nscoord aOffsetForContextMenu, FlipStyle aFlip,
                               PRPackedBool* aFlipSide)
{
  
  nscoord popupSize = aSize;
  if (aScreenPoint < aScreenBegin) {
    
    
    if (aFlip) {
      
      nscoord startpos = aFlip == FlipStyle_Outside ? aAnchorBegin : aAnchorEnd;
      nscoord endpos = aFlip == FlipStyle_Outside ? aAnchorEnd : aAnchorBegin;

      
      
      if (startpos - aScreenBegin >= aScreenEnd - endpos) {
        aScreenPoint = aScreenBegin;
        popupSize = startpos - aScreenPoint - aMarginEnd;
      }
      else {
        
        
        *aFlipSide = PR_TRUE;
        aScreenPoint = endpos + aMarginEnd;
        
        
        if (aScreenPoint + aSize > aScreenEnd) {
          popupSize = aScreenEnd - aScreenPoint;
        }
      }
    }
    else {
      aScreenPoint = aScreenBegin;
    }
  }
  else if (aScreenPoint + aSize > aScreenEnd) {
    
    
    if (aFlip) {
      
      nscoord startpos = aFlip == FlipStyle_Outside ? aAnchorBegin : aAnchorEnd;
      nscoord endpos = aFlip == FlipStyle_Outside ? aAnchorEnd : aAnchorBegin;

      
      
      if (aScreenEnd - endpos >= startpos - aScreenBegin) {
        if (mIsContextMenu) {
          aScreenPoint = aScreenEnd - aSize;
        }
        else {
          popupSize = aScreenEnd - aScreenPoint;
        }
      }
      else {
        
        
        *aFlipSide = PR_TRUE;
        aScreenPoint = startpos - aSize - aMarginBegin - aOffsetForContextMenu;

        
        
        if (aScreenPoint < aScreenBegin) {
          aScreenPoint = aScreenBegin;
          if (!mIsContextMenu) {
            popupSize = startpos - aScreenPoint - aMarginBegin;
          }
        }
      }
    }
    else {
      aScreenPoint = aScreenEnd - aSize;
    }
  }

  return popupSize;
}

nsresult
nsMenuPopupFrame::SetPopupPosition(nsIFrame* aAnchorFrame, PRBool aIsMove)
{
  if (!mShouldAutoPosition)
    return NS_OK;

  nsPresContext* presContext = PresContext();
  nsIFrame* rootFrame = presContext->PresShell()->FrameManager()->GetRootFrame();
  NS_ASSERTION(rootFrame->GetView() && GetView() &&
               rootFrame->GetView() == GetView()->GetParent(),
               "rootFrame's view is not our view's parent???");

  
  
  
  if (!aAnchorFrame) {
    if (mAnchorContent) {
      aAnchorFrame = mAnchorContent->GetPrimaryFrame();
    }

    if (!aAnchorFrame) {
      aAnchorFrame = rootFrame;
      if (!aAnchorFrame)
        return NS_OK;
    }
  }

  PRBool sizedToPopup = PR_FALSE;
  if (aAnchorFrame->GetContent()) {
    
    sizedToPopup = nsMenuFrame::IsSizedToPopup(aAnchorFrame->GetContent(), PR_FALSE);
  }

  
  nsRect parentRect = aAnchorFrame->GetScreenRectInAppUnits();

  
  
  
  parentRect = parentRect.ConvertAppUnitsRoundOut(
    aAnchorFrame->PresContext()->AppUnitsPerDevPixel(),
    presContext->AppUnitsPerDevPixel());

  
  
  
  
  NS_ASSERTION(mPrefSize.width >= 0 || mPrefSize.height >= 0,
               "preferred size of popup not set");
  mRect.width = sizedToPopup ? parentRect.width : mPrefSize.width;
  mRect.height = mPrefSize.height;

  
  nsPoint screenPoint;

  
  
  nsRect anchorRect = parentRect;

  
  FlipStyle hFlip = FlipStyle_None, vFlip = FlipStyle_None;

  nsMargin margin(0, 0, 0, 0);
  GetStyleMargin()->GetMargin(margin);

  
  nsRect rootScreenRect = rootFrame->GetScreenRectInAppUnits();

  nsDeviceContext* devContext = presContext->DeviceContext();
  nscoord offsetForContextMenu = 0;
  
  
  
  if (mScreenXPos == -1 && mScreenYPos == -1) {
    
    
    
    
    
    
    if (mAnchorContent) {
      
      
      
      
      screenPoint = AdjustPositionForAnchorAlign(anchorRect, hFlip, vFlip);
    }
    else {
      
      anchorRect = rootScreenRect;
      screenPoint = anchorRect.TopLeft() + nsPoint(margin.left, margin.top);
    }

    
    
    if (IsDirectionRTL())
      screenPoint.x -= presContext->CSSPixelsToAppUnits(mXPos);
    else
      screenPoint.x += presContext->CSSPixelsToAppUnits(mXPos);
    screenPoint.y += presContext->CSSPixelsToAppUnits(mYPos);

    
    
    
    
    
    if (IsNoAutoHide() && PopupLevel(PR_TRUE) != ePopupLevelParent) {
      
      
      mScreenXPos = presContext->AppUnitsToIntCSSPixels(screenPoint.x - margin.left);
      mScreenYPos = presContext->AppUnitsToIntCSSPixels(screenPoint.y - margin.top);
    }
  }
  else {
    
    
    
    
    PRInt32 factor = devContext->UnscaledAppUnitsPerDevPixel();

    
    
    
    if (mAdjustOffsetForContextMenu) {
      PRInt32 offsetForContextMenuDev =
        nsPresContext::CSSPixelsToAppUnits(2) / factor;
      offsetForContextMenu = presContext->DevPixelsToAppUnits(offsetForContextMenuDev);
    }

    
    screenPoint.x = presContext->DevPixelsToAppUnits(
                      nsPresContext::CSSPixelsToAppUnits(mScreenXPos) / factor);
    screenPoint.y = presContext->DevPixelsToAppUnits(
                      nsPresContext::CSSPixelsToAppUnits(mScreenYPos) / factor);
    anchorRect = nsRect(screenPoint, nsSize(0, 0));

    
    screenPoint.MoveBy(margin.left + offsetForContextMenu,
                       margin.top + offsetForContextMenu);

    
    vFlip = FlipStyle_Outside;
  }

  
  
  if (mInContentShell || !aIsMove || mPopupType != ePopupTypePanel) {
    nsRect screenRect = GetConstraintRect(anchorRect, rootScreenRect);

    
    if (!anchorRect.IntersectRect(anchorRect, screenRect)) {
      anchorRect.width = anchorRect.height = 0;
      
      if (anchorRect.x < screenRect.x)
        anchorRect.x = screenRect.x;
      if (anchorRect.XMost() > screenRect.XMost())
        anchorRect.x = screenRect.XMost();
      if (anchorRect.y < screenRect.y)
        anchorRect.y = screenRect.y;
      if (anchorRect.YMost() > screenRect.YMost())
        anchorRect.y = screenRect.YMost();
    }

    
    if (mRect.width > screenRect.width)
      mRect.width = screenRect.width;
    if (mRect.height > screenRect.height)
      mRect.height = screenRect.height;

    
    
    
    
    
    mRect.width = FlipOrResize(screenPoint.x, mRect.width, screenRect.x,
                               screenRect.XMost(), anchorRect.x, anchorRect.XMost(),
                               margin.left, margin.right, offsetForContextMenu, hFlip, &mHFlip);

    mRect.height = FlipOrResize(screenPoint.y, mRect.height, screenRect.y,
                                screenRect.YMost(), anchorRect.y, anchorRect.YMost(),
                                margin.top, margin.bottom, offsetForContextMenu, vFlip, &mVFlip);

    NS_ASSERTION(screenPoint.x >= screenRect.x && screenPoint.y >= screenRect.y &&
                 screenPoint.x + mRect.width <= screenRect.XMost() &&
                 screenPoint.y + mRect.height <= screenRect.YMost(),
                 "Popup is offscreen");
  }

  
  
  nsPoint viewPoint = screenPoint - rootScreenRect.TopLeft();

  
  viewPoint.x = presContext->RoundAppUnitsToNearestDevPixels(viewPoint.x);
  viewPoint.y = presContext->RoundAppUnitsToNearestDevPixels(viewPoint.y);

  nsIView* view = GetView();
  NS_ASSERTION(view, "popup with no view");
  presContext->GetPresShell()->GetViewManager()->
    MoveViewTo(view, viewPoint.x, viewPoint.y);

  
  
  
  
  nsIWidget* widget = view->GetWidget();
  if (mPopupType == ePopupTypePanel && widget) {
    nsIntPoint offset = widget->GetClientOffset();
    viewPoint.x += presContext->DevPixelsToAppUnits(offset.x);
    viewPoint.y += presContext->DevPixelsToAppUnits(offset.y);
  }

  
  nsBoxFrame::SetPosition(viewPoint - GetParent()->GetOffsetTo(rootFrame));

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

nsRect
nsMenuPopupFrame::GetConstraintRect(const nsRect& aAnchorRect,
                                    const nsRect& aRootScreenRect)
{
  nsIntRect screenRectPixels;
  nsPresContext* presContext = PresContext();

  
  
  
  nsCOMPtr<nsIScreen> screen;
  nsCOMPtr<nsIScreenManager> sm(do_GetService("@mozilla.org/gfx/screenmanager;1"));
  if (sm) {
    
    
    
    
    nsRect rect = mInContentShell ? aRootScreenRect : aAnchorRect;
    PRInt32 width = rect.width > 0 ? presContext->AppUnitsToDevPixels(rect.width) : 1;
    PRInt32 height = rect.height > 0 ? presContext->AppUnitsToDevPixels(rect.height) : 1;
    sm->ScreenForRect(presContext->AppUnitsToDevPixels(rect.x),
                      presContext->AppUnitsToDevPixels(rect.y),
                      width, height, getter_AddRefs(screen));
    if (screen) {
      
      if (mMenuCanOverlapOSBar && !mInContentShell)
        screen->GetRect(&screenRectPixels.x, &screenRectPixels.y,
                        &screenRectPixels.width, &screenRectPixels.height);
      else
        screen->GetAvailRect(&screenRectPixels.x, &screenRectPixels.y,
                             &screenRectPixels.width, &screenRectPixels.height);
    }
  }

  
  screenRectPixels.SizeTo(screenRectPixels.width - 3, screenRectPixels.height - 3);

  nsRect screenRect = screenRectPixels.ToAppUnits(presContext->AppUnitsPerDevPixel());
  if (mInContentShell) {
    
    screenRect.IntersectRect(screenRect, aRootScreenRect);
  }

  return screenRect;
}

void nsMenuPopupFrame::CanAdjustEdges(PRInt8 aHorizontalSide, PRInt8 aVerticalSide, nsIntPoint& aChange)
{
  PRInt8 popupAlign(mPopupAlignment);
  if (IsDirectionRTL()) {
    popupAlign = -popupAlign;
  }

  if (aHorizontalSide == (mHFlip ? NS_SIDE_RIGHT : NS_SIDE_LEFT)) {
    if (popupAlign == POPUPALIGNMENT_TOPLEFT || popupAlign == POPUPALIGNMENT_BOTTOMLEFT) {
      aChange.x = 0;
    }
  }
  else if (aHorizontalSide == (mHFlip ? NS_SIDE_LEFT : NS_SIDE_RIGHT)) {
    if (popupAlign == POPUPALIGNMENT_TOPRIGHT || popupAlign == POPUPALIGNMENT_BOTTOMRIGHT) {
      aChange.x = 0;
    }
  }

  if (aVerticalSide == (mVFlip ? NS_SIDE_BOTTOM : NS_SIDE_TOP)) {
    if (popupAlign == POPUPALIGNMENT_TOPLEFT || popupAlign == POPUPALIGNMENT_TOPRIGHT) {
      aChange.y = 0;
    }
  }
  else if (aVerticalSide == (mVFlip ? NS_SIDE_TOP : NS_SIDE_BOTTOM)) {
    if (popupAlign == POPUPALIGNMENT_BOTTOMLEFT || popupAlign == POPUPALIGNMENT_BOTTOMRIGHT) {
      aChange.y = 0;
    }
  }
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
        ni->Equals(nsGkAtoms::splitmenu, kNameSpaceID_XUL) ||
        ni->Equals(nsGkAtoms::popupset, kNameSpaceID_XUL) ||
        ((ni->Equals(nsGkAtoms::button, kNameSpaceID_XUL) ||
          ni->Equals(nsGkAtoms::toolbarbutton, kNameSpaceID_XUL)) &&
         (parentContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                     nsGkAtoms::menu, eCaseMatters) ||
          parentContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                     nsGkAtoms::menuButton, eCaseMatters)))) {
      return PR_FALSE;
    }
#endif
    if (ni->Equals(nsGkAtoms::textbox, kNameSpaceID_XUL)) {
      
      if (parentContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                     nsGkAtoms::autocomplete, eCaseMatters))
        return PR_FALSE;
    }
  }

  return PR_TRUE;
}



nsIScrollableFrame* nsMenuPopupFrame::GetScrollFrame(nsIFrame* aStart)
{
  if (!aStart)
    return nsnull;  

  
  nsIFrame* currFrame = aStart;
  do {
    nsIScrollableFrame* sf = do_QueryFrame(currFrame);
    if (sf)
      return sf;
    currFrame = currFrame->GetNextSibling();
  } while (currFrame);

  
  currFrame = aStart;
  do {
    nsIFrame* childFrame = currFrame->GetFirstChild(nsnull);
    nsIScrollableFrame* sf = GetScrollFrame(childFrame);
    if (sf)
      return sf;
    currFrame = currFrame->GetNextSibling();
  } while (currFrame);

  return nsnull;
}

void nsMenuPopupFrame::EnsureMenuItemIsVisible(nsMenuFrame* aMenuItem)
{
  if (aMenuItem) {
    aMenuItem->PresContext()->PresShell()->
      ScrollFrameRectIntoView(aMenuItem,
                              nsRect(nsPoint(0,0), aMenuItem->GetRect().Size()),
                              NS_PRESSHELL_SCROLL_ANYWHERE,
                              NS_PRESSHELL_SCROLL_ANYWHERE,
                              nsIPresShell::SCROLL_OVERFLOW_HIDDEN |
                              nsIPresShell::SCROLL_FIRST_ANCESTOR_ONLY);
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
nsMenuPopupFrame::Enter(nsGUIEvent* aEvent)
{
  mIncrementalString.Truncate();

  
  if (mCurrentMenu)
    return mCurrentMenu->Enter(aEvent);

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

void
nsMenuPopupFrame::LockMenuUntilClosed(PRBool aLock)
{
  mIsMenuLocked = aLock;

  
  nsIFrame* parent = GetParent();
  if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
    nsMenuParent* parentParent = static_cast<nsMenuFrame*>(parent)->GetMenuParent();
    if (parentParent) {
      parentParent->LockMenuUntilClosed(aLock);
    }
  }
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

  if (aAttribute == nsGkAtoms::label) {
    
    nsIView* view = GetView();
    if (view) {
      nsIWidget* widget = view->GetWidget();
      if (widget) {
        nsAutoString title;
        mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, title);
        if (!title.IsEmpty()) {
          widget->SetTitle(title);
        }
      }
    }
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
  PRInt32 xpos = left.ToInteger(&err1);
  PRInt32 ypos = top.ToInteger(&err2);

  if (NS_SUCCEEDED(err1) && NS_SUCCEEDED(err2))
    MoveTo(xpos, ypos, PR_FALSE);
}

void
nsMenuPopupFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsIFrame* parent = GetParent();
  if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
    
    nsContentUtils::AddScriptRunner(
      new nsUnsetAttrRunnable(parent->GetContent(), nsGkAtoms::open));
  }

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm)
    pm->PopupDestroyed(this);

  nsIRootBox* rootBox =
    nsIRootBox::GetRootBox(PresContext()->GetPresShell());
  if (rootBox && rootBox->GetDefaultTooltip() == mContent) {
    rootBox->SetDefaultTooltip(nsnull);
  }

  nsBoxFrame::DestroyFrom(aDestructRoot);
}


void
nsMenuPopupFrame::MoveTo(PRInt32 aLeft, PRInt32 aTop, PRBool aUpdateAttrs)
{
  if (mScreenXPos == aLeft && mScreenYPos == aTop)
    return;

  
  
  
  
  nsMargin margin(0, 0, 0, 0);
  GetStyleMargin()->GetMargin(margin);
  nsPresContext* presContext = PresContext();
  mScreenXPos = aLeft - presContext->AppUnitsToIntCSSPixels(margin.left);
  mScreenYPos = aTop - presContext->AppUnitsToIntCSSPixels(margin.top);

  SetPopupPosition(nsnull, PR_TRUE);

  nsCOMPtr<nsIContent> popup = mContent;
  if (aUpdateAttrs && (popup->HasAttr(kNameSpaceID_None, nsGkAtoms::left) ||
                       popup->HasAttr(kNameSpaceID_None, nsGkAtoms::top)))
  {
    nsAutoString left, top;
    left.AppendInt(aLeft);
    top.AppendInt(aTop);
    popup->SetAttr(kNameSpaceID_None, nsGkAtoms::left, left, PR_FALSE);
    popup->SetAttr(kNameSpaceID_None, nsGkAtoms::top, top, PR_FALSE);
  }
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





nsresult
nsMenuPopupFrame::CreatePopupViewForFrame()
{
  if (HasView()) {
    return NS_OK;
  }

  nsViewVisibility visibility = nsViewVisibility_kShow;
  PRInt32 zIndex = 0;
  PRBool  autoZIndex = PR_FALSE;

  nsIView* parentView;
  nsIViewManager* viewManager = PresContext()->GetPresShell()->GetViewManager();
  NS_ASSERTION(nsnull != viewManager, "null view manager");

  
  parentView = viewManager->GetRootView();
  visibility = nsViewVisibility_kHide;
  zIndex = PR_INT32_MAX;

  NS_ASSERTION(parentView, "no parent view");

  
  nsIView *view = viewManager->CreateView(GetRect(), parentView, visibility);
  if (view) {
    viewManager->SetViewZIndex(view, autoZIndex, zIndex);
    
    viewManager->InsertChild(parentView, view, nsnull, PR_TRUE);
  }

  
  SetView(view);

  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
    ("nsMenuPopupFrame::CreatePopupViewForFrame: frame=%p view=%p", this, view));

  if (!view)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}
