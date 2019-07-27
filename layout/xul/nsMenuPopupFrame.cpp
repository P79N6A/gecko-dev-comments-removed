





#include "nsMenuPopupFrame.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsNameSpaceManager.h"
#include "nsViewManager.h"
#include "nsWidgetsCID.h"
#include "nsMenuFrame.h"
#include "nsMenuBarFrame.h"
#include "nsPopupSetFrame.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMScreen.h"
#include "nsIPresShell.h"
#include "nsFrameManager.h"
#include "nsIDocument.h"
#include "nsRect.h"
#include "nsIComponentManager.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsIRootBox.h"
#include "nsIDocShell.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsLayoutUtils.h"
#include "nsContentUtils.h"
#include "nsCSSFrameConstructor.h"
#include "nsPIWindowRoot.h"
#include "nsIReflowCallback.h"
#include "nsBindingManager.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#include "nsISound.h"
#include "nsIScreenManager.h"
#include "nsIServiceManager.h"
#include "nsThemeConstants.h"
#include "nsTransitionManager.h"
#include "nsDisplayList.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/PopupBoxObject.h"
#include <algorithm>

using namespace mozilla;
using mozilla::dom::PopupBoxObject;

int8_t nsMenuPopupFrame::sDefaultLevelIsTop = -1;
uint32_t nsMenuPopupFrame::sTimeoutOfIncrementalSearch = 1000;

const char* kPrefIncrementalSearchTimeout =
  "ui.menu.incremental_search.timeout";





nsIFrame*
NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMenuPopupFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMenuPopupFrame)

NS_QUERYFRAME_HEAD(nsMenuPopupFrame)
  NS_QUERYFRAME_ENTRY(nsMenuPopupFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)




nsMenuPopupFrame::nsMenuPopupFrame(nsStyleContext* aContext)
  :nsBoxFrame(aContext),
  mCurrentMenu(nullptr),
  mPrefSize(-1, -1),
  mLastClientOffset(0, 0),
  mPopupType(ePopupTypePanel),
  mPopupState(ePopupClosed),
  mPopupAlignment(POPUPALIGNMENT_NONE),
  mPopupAnchor(POPUPALIGNMENT_NONE),
  mPosition(POPUPPOSITION_UNKNOWN),
  mConsumeRollupEvent(PopupBoxObject::ROLLUP_DEFAULT),
  mFlip(FlipType_Default),
  mIsOpenChanged(false),
  mIsContextMenu(false),
  mAdjustOffsetForContextMenu(false),
  mGeneratedChildren(false),
  mMenuCanOverlapOSBar(false),
  mShouldAutoPosition(true),
  mInContentShell(true),
  mIsMenuLocked(false),
  mMouseTransparent(false),
  mHFlip(false),
  mVFlip(false)
{
  
  
  if (sDefaultLevelIsTop >= 0)
    return;
  sDefaultLevelIsTop =
    Preferences::GetBool("ui.panel.default_level_parent", false);
  Preferences::AddUintVarCache(&sTimeoutOfIncrementalSearch,
                               kPrefIncrementalSearchTimeout, 1000);
} 

void
nsMenuPopupFrame::Init(nsIContent*       aContent,
                       nsContainerFrame* aParent,
                       nsIFrame*         aPrevInFlow)
{
  nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  
  mMenuCanOverlapOSBar =
    LookAndFeel::GetInt(LookAndFeel::eIntID_MenusCanOverlapOSBar) != 0;

  CreatePopupView();

  
  
  
  nsView* ourView = GetView();
  nsViewManager* viewManager = ourView->GetViewManager();
  viewManager->SetViewFloating(ourView, true);

  mPopupType = ePopupTypePanel;
  nsIDocument* doc = aContent->OwnerDoc();
  int32_t namespaceID;
  nsCOMPtr<nsIAtom> tag = doc->BindingManager()->ResolveTag(aContent, &namespaceID);
  if (namespaceID == kNameSpaceID_XUL) {
    if (tag == nsGkAtoms::menupopup || tag == nsGkAtoms::popup)
      mPopupType = ePopupTypeMenu;
    else if (tag == nsGkAtoms::tooltip)
      mPopupType = ePopupTypeTooltip;
  }

  nsCOMPtr<nsIDocShellTreeItem> dsti = PresContext()->GetDocShell();
  if (dsti && dsti->ItemType() == nsIDocShellTreeItem::typeChrome) {
    mInContentShell = false;
  }

  
  
  
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

  AddStateBits(NS_FRAME_IN_POPUP);
}

bool
nsMenuPopupFrame::IsNoAutoHide() const
{
  
  
  
  return (!mInContentShell && mPopupType == ePopupTypePanel &&
           mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::noautohide,
                                 nsGkAtoms::_true, eIgnoreCase));
}

nsPopupLevel
nsMenuPopupFrame::PopupLevel(bool aIsNoAutoHide) const
{
  
  
  
  
  
  

  
  if (mPopupType != ePopupTypePanel)
    return ePopupLevelTop;

  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::top, &nsGkAtoms::parent, &nsGkAtoms::floating, nullptr};
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
  nsView* ourView = GetView();
  if (!ourView->HasWidget()) {
    NS_ASSERTION(!mGeneratedChildren && !GetFirstPrincipalChild(),
                 "Creating widget for MenuPopupFrame with children");
    CreateWidgetForView(ourView);
  }
}

nsresult
nsMenuPopupFrame::CreateWidgetForView(nsView* aView)
{
  
  nsWidgetInitData widgetData;
  widgetData.mWindowType = eWindowType_popup;
  widgetData.mBorderStyle = eBorderStyle_default;
  widgetData.clipSiblings = true;
  widgetData.mPopupHint = mPopupType;
  widgetData.mNoAutoHide = IsNoAutoHide();

  if (!mInContentShell) {
    
    if (mPopupType == ePopupTypePanel &&
        mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                              nsGkAtoms::drag, eIgnoreCase)) {
      widgetData.mIsDragPopup = true;
    }

    
    
    mMouseTransparent = GetStateBits() & NS_FRAME_MOUSE_THROUGH_ALWAYS;
    widgetData.mMouseTransparent = mMouseTransparent;
  }

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
  nsIContent* parentContent = GetContent()->GetParent();
  nsIAtom *tag = nullptr;
  if (parentContent && parentContent->IsXULElement())
    tag = parentContent->NodeInfo()->NameAtom();
  widgetData.mSupportTranslucency = mode == eTransparencyTransparent;
  widgetData.mDropShadow = !(mode == eTransparencyTransparent || tag == nsGkAtoms::menulist);
  widgetData.mPopupLevel = PopupLevel(widgetData.mNoAutoHide);

  
  
  
  nsCOMPtr<nsIWidget> parentWidget;
  if (widgetData.mPopupLevel != ePopupLevelTop) {
    nsCOMPtr<nsIDocShellTreeItem> dsti = PresContext()->GetDocShell();
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
                                            true, true);
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

uint8_t
nsMenuPopupFrame::GetShadowStyle()
{
  uint8_t shadow = StyleUIReset()->mWindowShadow;
  if (shadow != NS_STYLE_WINDOW_SHADOW_DEFAULT)
    return shadow;

  switch (StyleDisplay()->mAppearance) {
    case NS_THEME_TOOLTIP:
      return NS_STYLE_WINDOW_SHADOW_TOOLTIP;
    case NS_THEME_MENUPOPUP:
      return NS_STYLE_WINDOW_SHADOW_MENU;
  }
  return NS_STYLE_WINDOW_SHADOW_DEFAULT;
}

NS_IMETHODIMP nsXULPopupShownEvent::Run()
{
  nsMenuPopupFrame* popup = do_QueryFrame(mPopup->GetPrimaryFrame());
  
  if (popup && popup->IsOpen()) {
    popup->SetPopupState(ePopupShown);
  }

  WidgetMouseEvent event(true, NS_XUL_POPUP_SHOWN, nullptr,
                         WidgetMouseEvent::eReal);
  return EventDispatcher::Dispatch(mPopup, mPresContext, &event);                 
}

NS_IMETHODIMP nsXULPopupShownEvent::HandleEvent(nsIDOMEvent* aEvent)
{
  nsMenuPopupFrame* popup = do_QueryFrame(mPopup->GetPrimaryFrame());
  if (popup) {
    
    
    nsRefPtr<nsXULPopupShownEvent> event = this;
    
    
    if (popup->ClearPopupShownDispatcher()) {
      return Run();
    }
  }

  CancelListener();
  return NS_OK;
}

void nsXULPopupShownEvent::CancelListener()
{
  mPopup->RemoveSystemEventListener(NS_LITERAL_STRING("transitionend"), this, false);
}

NS_IMPL_ISUPPORTS_INHERITED(nsXULPopupShownEvent, nsRunnable, nsIDOMEventListener);

void
nsMenuPopupFrame::SetInitialChildList(ChildListID  aListID,
                                      nsFrameList& aChildList)
{
  
  if (aChildList.NotEmpty())
    mGeneratedChildren = true;
  nsBoxFrame::SetInitialChildList(aListID, aChildList);
}

bool
nsMenuPopupFrame::IsLeaf() const
{
  if (mGeneratedChildren)
    return false;

  if (mPopupType != ePopupTypeMenu) {
    
    
    return !mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::type);
  }

  
  
  
  
  
  nsIContent* parentContent = mContent->GetParent();
  return (parentContent &&
          !parentContent->HasAttr(kNameSpaceID_None, nsGkAtoms::sizetopopup));
}

void
nsMenuPopupFrame::LayoutPopup(nsBoxLayoutState& aState, nsIFrame* aParentMenu,
                              nsIFrame* aAnchor, bool aSizedToPopup)
{
  if (!mGeneratedChildren)
    return;

  SchedulePaint();

  bool shouldPosition = true;
  bool isOpen = IsOpen();
  if (!isOpen) {
    
    
    shouldPosition = (mPopupState == ePopupShowing);
    if (!shouldPosition && !aSizedToPopup) {
      RemoveStateBits(NS_FRAME_FIRST_REFLOW);
      return;
    }
  }

  
  if (mIsOpenChanged) {
    nsIScrollableFrame *scrollframe = do_QueryFrame(nsBox::GetChildBox(this));
    if (scrollframe) {
      nsWeakFrame weakFrame(this);
      scrollframe->ScrollTo(nsPoint(0,0), nsIScrollableFrame::INSTANT);
      if (!weakFrame.IsAlive()) {
        return;
      }
    }
  }

  
  
  nsSize prefSize = GetPrefSize(aState);
  nsSize minSize = GetMinSize(aState); 
  nsSize maxSize = GetMaxSize(aState);

  if (aSizedToPopup) {
    prefSize.width = aParentMenu->GetRect().width;
  }
  prefSize = BoundsCheck(minSize, prefSize, maxSize);

  
  bool sizeChanged = (mPrefSize != prefSize);
  if (sizeChanged) {
    SetBounds(aState, nsRect(0, 0, prefSize.width, prefSize.height), false);
    mPrefSize = prefSize;
  }

  bool needCallback = false;

  if (shouldPosition) {
    SetPopupPosition(aAnchor, false, aSizedToPopup);
    needCallback = true;
  }

  nsRect bounds(GetRect());
  Layout(aState);

  
  
  
  
  if (!aParentMenu) {
    nsSize newsize = GetSize();
    if (newsize.width > bounds.width || newsize.height > bounds.height) {
      
      
      mPrefSize = newsize;
      if (isOpen) {
        SetPopupPosition(aAnchor, false, aSizedToPopup);
        needCallback = true;
      }
    }
  }

  nsPresContext* pc = PresContext();
  nsView* view = GetView();

  if (sizeChanged) {
    
    nsIWidget* widget = view->GetWidget();
    if (widget) {
      SetSizeConstraints(pc, widget, minSize, maxSize);
    }
  }

  if (isOpen) {
    nsViewManager* viewManager = view->GetViewManager();
    nsRect rect = GetRect();
    rect.x = rect.y = 0;
    viewManager->ResizeView(view, rect);

    if (mPopupState == ePopupOpening) {
      mPopupState = ePopupVisible;
    }

    viewManager->SetViewVisibility(view, nsViewVisibility_kShow);
    nsContainerFrame::SyncFrameViewProperties(pc, this, nullptr, view, 0);
  }

  
  if (mIsOpenChanged) {
    mIsOpenChanged = false;

#ifndef MOZ_WIDGET_GTK
    
    
    if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::animate,
                              nsGkAtoms::open, eCaseMatters) &&
        nsLayoutUtils::HasCurrentAnimations(mContent,
                                            nsGkAtoms::transitionsProperty)) {
      mPopupShownDispatcher = new nsXULPopupShownEvent(mContent, pc);
      mContent->AddSystemEventListener(NS_LITERAL_STRING("transitionend"),
                                       mPopupShownDispatcher, false, false);
      return;
    }
#endif

    
    nsCOMPtr<nsIRunnable> event = new nsXULPopupShownEvent(GetContent(), pc);
    NS_DispatchToCurrentThread(event);
  }

  if (needCallback && !mReflowCallbackData.mPosted) {
    pc->PresShell()->PostReflowCallback(this);
    mReflowCallbackData.MarkPosted(aAnchor, aSizedToPopup);
  }
}

bool
nsMenuPopupFrame::ReflowFinished()
{
  SetPopupPosition(mReflowCallbackData.mAnchor, false, mReflowCallbackData.mSizedToPopup);

  mReflowCallbackData.Clear();

  return false;
}

void
nsMenuPopupFrame::ReflowCallbackCanceled()
{
  mReflowCallbackData.Clear();
}

nsIContent*
nsMenuPopupFrame::GetTriggerContent(nsMenuPopupFrame* aMenuPopupFrame)
{
  while (aMenuPopupFrame) {
    if (aMenuPopupFrame->mTriggerContent)
      return aMenuPopupFrame->mTriggerContent;

    
    nsMenuFrame* menuFrame = do_QueryFrame(aMenuPopupFrame->GetParent());
    if (!menuFrame)
      break;

    nsMenuParent* parentPopup = menuFrame->GetMenuParent();
    if (!parentPopup || !parentPopup->IsMenu())
      break;

    aMenuPopupFrame = static_cast<nsMenuPopupFrame *>(parentPopup);
  }

  return nullptr;
}

void
nsMenuPopupFrame::InitPositionFromAnchorAlign(const nsAString& aAnchor,
                                              const nsAString& aAlign)
{
  mTriggerContent = nullptr;

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

  mPosition = POPUPPOSITION_UNKNOWN;
}

void
nsMenuPopupFrame::InitializePopup(nsIContent* aAnchorContent,
                                  nsIContent* aTriggerContent,
                                  const nsAString& aPosition,
                                  int32_t aXPos, int32_t aYPos,
                                  bool aAttributesOverride)
{
  EnsureWidget();

  mPopupState = ePopupShowing;
  mAnchorContent = aAnchorContent;
  mTriggerContent = aTriggerContent;
  mXPos = aXPos;
  mYPos = aYPos;
  mAdjustOffsetForContextMenu = false;
  mVFlip = false;
  mHFlip = false;
  mAlignmentOffset = 0;

  
  
  
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

    if (flip.EqualsLiteral("none")) {
      mFlip = FlipType_None;
    } else if (flip.EqualsLiteral("both")) {
      mFlip = FlipType_Both;
    } else if (flip.EqualsLiteral("slide")) {
      mFlip = FlipType_Slide;
    }

    position.CompressWhitespace();
    int32_t spaceIdx = position.FindChar(' ');
    
    
    if (spaceIdx >= 0) {
      InitPositionFromAnchorAlign(Substring(position, 0, spaceIdx), Substring(position, spaceIdx + 1));
    }
    else if (position.EqualsLiteral("before_start")) {
      mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
      mPopupAlignment = POPUPALIGNMENT_BOTTOMLEFT;
      mPosition = POPUPPOSITION_BEFORESTART;
    }
    else if (position.EqualsLiteral("before_end")) {
      mPopupAnchor = POPUPALIGNMENT_TOPRIGHT;
      mPopupAlignment = POPUPALIGNMENT_BOTTOMRIGHT;
      mPosition = POPUPPOSITION_BEFOREEND;
    }
    else if (position.EqualsLiteral("after_start")) {
      mPopupAnchor = POPUPALIGNMENT_BOTTOMLEFT;
      mPopupAlignment = POPUPALIGNMENT_TOPLEFT;
      mPosition = POPUPPOSITION_AFTERSTART;
    }
    else if (position.EqualsLiteral("after_end")) {
      mPopupAnchor = POPUPALIGNMENT_BOTTOMRIGHT;
      mPopupAlignment = POPUPALIGNMENT_TOPRIGHT;
      mPosition = POPUPPOSITION_AFTEREND;
    }
    else if (position.EqualsLiteral("start_before")) {
      mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
      mPopupAlignment = POPUPALIGNMENT_TOPRIGHT;
      mPosition = POPUPPOSITION_STARTBEFORE;
    }
    else if (position.EqualsLiteral("start_after")) {
      mPopupAnchor = POPUPALIGNMENT_BOTTOMLEFT;
      mPopupAlignment = POPUPALIGNMENT_BOTTOMRIGHT;
      mPosition = POPUPPOSITION_STARTAFTER;
    }
    else if (position.EqualsLiteral("end_before")) {
      mPopupAnchor = POPUPALIGNMENT_TOPRIGHT;
      mPopupAlignment = POPUPALIGNMENT_TOPLEFT;
      mPosition = POPUPPOSITION_ENDBEFORE;
    }
    else if (position.EqualsLiteral("end_after")) {
      mPopupAnchor = POPUPALIGNMENT_BOTTOMRIGHT;
      mPopupAlignment = POPUPALIGNMENT_BOTTOMLEFT;
      mPosition = POPUPPOSITION_ENDAFTER;
    }
    else if (position.EqualsLiteral("overlap")) {
      mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
      mPopupAlignment = POPUPALIGNMENT_TOPLEFT;
      mPosition = POPUPPOSITION_OVERLAP;
    }
    else if (position.EqualsLiteral("after_pointer")) {
      mPopupAnchor = POPUPALIGNMENT_TOPLEFT;
      mPopupAlignment = POPUPALIGNMENT_TOPLEFT;
      mPosition = POPUPPOSITION_AFTERPOINTER;
      
      
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

    nsresult err;
    if (!left.IsEmpty()) {
      int32_t x = left.ToInteger(&err);
      if (NS_SUCCEEDED(err))
        mScreenXPos = x;
    }
    if (!top.IsEmpty()) {
      int32_t y = top.ToInteger(&err);
      if (NS_SUCCEEDED(err))
        mScreenYPos = y;
    }
  }
}

void
nsMenuPopupFrame::InitializePopupAtScreen(nsIContent* aTriggerContent,
                                          int32_t aXPos, int32_t aYPos,
                                          bool aIsContextMenu)
{
  EnsureWidget();

  mPopupState = ePopupShowing;
  mAnchorContent = nullptr;
  mTriggerContent = aTriggerContent;
  mScreenXPos = aXPos;
  mScreenYPos = aYPos;
  mFlip = FlipType_Default;
  mPopupAnchor = POPUPALIGNMENT_NONE;
  mPopupAlignment = POPUPALIGNMENT_NONE;
  mIsContextMenu = aIsContextMenu;
  mAdjustOffsetForContextMenu = aIsContextMenu;
}

void
nsMenuPopupFrame::InitializePopupWithAnchorAlign(nsIContent* aAnchorContent,
                                                 nsAString& aAnchor,
                                                 nsAString& aAlign,
                                                 int32_t aXPos, int32_t aYPos)
{
  EnsureWidget();

  mPopupState = ePopupShowing;
  mAdjustOffsetForContextMenu = false;
  mFlip = FlipType_Default;

  
  
  
  if (aXPos == -1 && aYPos == -1) {
    mAnchorContent = aAnchorContent;
    mScreenXPos = -1;
    mScreenYPos = -1;
    mXPos = 0;
    mYPos = 0;
    InitPositionFromAnchorAlign(aAnchor, aAlign);
  }
  else {
    mAnchorContent = nullptr;
    mPopupAnchor = POPUPALIGNMENT_NONE;
    mPopupAlignment = POPUPALIGNMENT_NONE;
    mScreenXPos = aXPos;
    mScreenYPos = aYPos;
    mXPos = aXPos;
    mYPos = aYPos;
  }
}

void
nsMenuPopupFrame::ShowPopup(bool aIsContextMenu, bool aSelectFirstItem)
{
  mIsContextMenu = aIsContextMenu;

  InvalidateFrameSubtree();

  if (mPopupState == ePopupShowing) {
    mPopupState = ePopupOpening;
    mIsOpenChanged = true;

    nsMenuFrame* menuFrame = do_QueryFrame(GetParent());
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

  mShouldAutoPosition = true;
}

void
nsMenuPopupFrame::HidePopup(bool aDeselectMenu, nsPopupState aNewState)
{
  NS_ASSERTION(aNewState == ePopupClosed || aNewState == ePopupInvisible,
               "popup being set to unexpected state");

  ClearPopupShownDispatcher();

  
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
            root->SetPopupNode(nullptr);
          }
        }
      }
    }
    mTriggerContent = nullptr;
    mAnchorContent = nullptr;
  }

  
  
  if (mPopupState == ePopupInvisible) {
    if (aNewState == ePopupClosed)
      mPopupState = ePopupClosed;
    return;
  }

  mPopupState = aNewState;

  if (IsMenu())
    SetCurrentMenuItem(nullptr);

  mIncrementalString.Truncate();

  LockMenuUntilClosed(false);

  mIsOpenChanged = false;
  mCurrentMenu = nullptr; 
  mHFlip = mVFlip = false;

  nsView* view = GetView();
  nsViewManager* viewManager = view->GetViewManager();
  viewManager->SetViewVisibility(view, nsViewVisibility_kHide);

  FireDOMEvent(NS_LITERAL_STRING("DOMMenuInactive"), mContent);

  
  
  
  NS_ASSERTION(mContent->IsElement(), "How do we have a non-element?");
  EventStates state = mContent->AsElement()->State();

  if (state.HasState(NS_EVENT_STATE_HOVER)) {
    EventStateManager* esm = PresContext()->EventStateManager();
    esm->SetContentState(nullptr, NS_EVENT_STATE_HOVER);
  }

  nsMenuFrame* menuFrame = do_QueryFrame(GetParent());
  if (menuFrame) {
    menuFrame->PopupClosed(aDeselectMenu);
  }
}

void
nsMenuPopupFrame::GetLayoutFlags(uint32_t& aFlags)
{
  aFlags = NS_FRAME_NO_SIZE_VIEW | NS_FRAME_NO_MOVE_VIEW | NS_FRAME_NO_VISIBILITY;
}






nsView*
nsMenuPopupFrame::GetRootViewForPopup(nsIFrame* aStartFrame)
{
  nsView* view = aStartFrame->GetClosestView();
  NS_ASSERTION(view, "frame must have a closest view!");
  while (view) {
    
    
    
    nsIWidget* widget = view->GetWidget();
    if (widget && widget->WindowType() == eWindowType_popup) {
      return view;
    }

    nsView* temp = view->GetParent();
    if (!temp) {
      
      
      return view;
    }
    view = temp;
  }

  return nullptr;
}

nsPoint
nsMenuPopupFrame::AdjustPositionForAnchorAlign(nsRect& anchorRect,
                                               FlipStyle& aHFlip, FlipStyle& aVFlip)
{
  
  int8_t popupAnchor(mPopupAnchor);
  int8_t popupAlign(mPopupAlignment);
  if (IsDirectionRTL()) {
    
    if (popupAnchor <= POPUPALIGNMENT_LEFTCENTER) {
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
  StyleMargin()->GetMargin(margin);
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  switch (popupAnchor) {
    case POPUPALIGNMENT_LEFTCENTER:
    case POPUPALIGNMENT_RIGHTCENTER:
      aHFlip = FlipStyle_Outside;
      aVFlip = FlipStyle_Inside;
      break;
    case POPUPALIGNMENT_TOPCENTER:
    case POPUPALIGNMENT_BOTTOMCENTER:
      aHFlip = FlipStyle_Inside;
      aVFlip = FlipStyle_Outside;
      break;
    default:
    {
      FlipStyle anchorEdge = mFlip == FlipType_Both ? FlipStyle_Inside : FlipStyle_None;
      aHFlip = (popupAnchor == -popupAlign) ? FlipStyle_Outside : anchorEdge;
      if (((popupAnchor > 0) == (popupAlign > 0)) ||
          (popupAnchor == POPUPALIGNMENT_TOPLEFT && popupAlign == POPUPALIGNMENT_TOPLEFT))
        aVFlip = FlipStyle_Outside;
      else
        aVFlip = anchorEdge;
      break;
    }
  }

  return pnt;
}

nscoord
nsMenuPopupFrame::SlideOrResize(nscoord& aScreenPoint, nscoord aSize,
                               nscoord aScreenBegin, nscoord aScreenEnd,
                               nscoord *aOffset)
{
  
  
  nscoord newPos =
    std::max(aScreenBegin, std::min(aScreenEnd - aSize, aScreenPoint));
  *aOffset = newPos - aScreenPoint;
  aScreenPoint = newPos;
  return std::min(aSize, aScreenEnd - aScreenPoint);
}

nscoord
nsMenuPopupFrame::FlipOrResize(nscoord& aScreenPoint, nscoord aSize, 
                               nscoord aScreenBegin, nscoord aScreenEnd,
                               nscoord aAnchorBegin, nscoord aAnchorEnd,
                               nscoord aMarginBegin, nscoord aMarginEnd,
                               nscoord aOffsetForContextMenu, FlipStyle aFlip,
                               bool* aFlipSide)
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
        
        
        
        
        nscoord newScreenPoint = endpos + aMarginEnd;
        if (newScreenPoint != aScreenPoint) {
          *aFlipSide = true;
          aScreenPoint = newScreenPoint;
          
          
          if (aScreenPoint + aSize > aScreenEnd) {
            popupSize = aScreenEnd - aScreenPoint;
          }
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
          aScreenPoint = endpos + aMarginBegin;
          popupSize = aScreenEnd - aScreenPoint;
        }
      }
      else {
        
        
        
        nscoord newScreenPoint = startpos - aSize - aMarginBegin - aOffsetForContextMenu;
        if (newScreenPoint != aScreenPoint) {
          *aFlipSide = true;
          aScreenPoint = newScreenPoint;

          
          
          if (aScreenPoint < aScreenBegin) {
            aScreenPoint = aScreenBegin;
            if (!mIsContextMenu) {
              popupSize = startpos - aScreenPoint - aMarginBegin;
            }
          }
        }
      }
    }
    else {
      aScreenPoint = aScreenEnd - aSize;
    }
  }

  
  
  
  if (aScreenPoint < aScreenBegin) {
    aScreenPoint = aScreenBegin;
  }
  if (aScreenPoint > aScreenEnd) {
    aScreenPoint = aScreenEnd - aSize;
  }

  
  
  if (popupSize <= 0 || aSize < popupSize) {
    popupSize = aSize;
  }
  return std::min(popupSize, aScreenEnd - aScreenPoint);
}

nsresult
nsMenuPopupFrame::SetPopupPosition(nsIFrame* aAnchorFrame, bool aIsMove, bool aSizedToPopup)
{
  if (!mShouldAutoPosition)
    return NS_OK;

  
  
  if (aIsMove && (mPrefSize.width == -1 || mPrefSize.height == -1)) {
    return NS_OK;
  }

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

  
  nsPresContext* rootPresContext = presContext->GetRootPresContext();

  
  if (!rootPresContext) {
    return NS_OK;
  }

  
  nsIFrame* referenceFrame = rootPresContext->FrameManager()->GetRootFrame();

  
  nsRect parentRect = aAnchorFrame->GetRectRelativeToSelf();
  
  parentRect = nsLayoutUtils::TransformFrameRectToAncestor(aAnchorFrame,
                                                           parentRect,
                                                           referenceFrame);
  
  parentRect.MoveBy(referenceFrame->GetScreenRectInAppUnits().TopLeft());
  
  parentRect =
    parentRect.ScaleToOtherAppUnitsRoundOut(rootPresContext->AppUnitsPerDevPixel(),
                                            presContext->AppUnitsPerDevPixel());

  
  
  
  
  NS_ASSERTION(mPrefSize.width >= 0 || mPrefSize.height >= 0,
               "preferred size of popup not set");
  mRect.width = aSizedToPopup ? parentRect.width : mPrefSize.width;
  mRect.height = mPrefSize.height;

  
  nsPoint screenPoint;

  
  
  nsRect anchorRect = parentRect;

  
  FlipStyle hFlip = FlipStyle_None, vFlip = FlipStyle_None;

  nsMargin margin(0, 0, 0, 0);
  StyleMargin()->GetMargin(margin);

  
  nsRect rootScreenRect = rootFrame->GetScreenRectInAppUnits();

  nsDeviceContext* devContext = presContext->DeviceContext();
  nscoord offsetForContextMenu = 0;

  bool isNoAutoHide = IsNoAutoHide();
  nsPopupLevel popupLevel = PopupLevel(isNoAutoHide);

  if (IsAnchored()) {
    
    
    
    
    
    
    if (mAnchorContent) {
      
      
      
      
      screenPoint = AdjustPositionForAnchorAlign(anchorRect, hFlip, vFlip);
    }
    else {
      
      anchorRect = rootScreenRect;
      screenPoint = anchorRect.TopLeft() + nsPoint(margin.left, margin.top);
    }

    
    
    
    nscoord anchorXOffset = presContext->CSSPixelsToAppUnits(mXPos);
    if (IsDirectionRTL()) {
      screenPoint.x -= anchorXOffset;
      anchorRect.x -= anchorXOffset;
    } else {
      screenPoint.x += anchorXOffset;
      anchorRect.x += anchorXOffset;
    }
    nscoord anchorYOffset = presContext->CSSPixelsToAppUnits(mYPos);
    screenPoint.y += anchorYOffset;
    anchorRect.y += anchorYOffset;

    
    
    
    
    
    if (isNoAutoHide && popupLevel != ePopupLevelParent) {
      
      
      mScreenXPos = presContext->AppUnitsToIntCSSPixels(screenPoint.x - margin.left);
      mScreenYPos = presContext->AppUnitsToIntCSSPixels(screenPoint.y - margin.top);
    }
  }
  else {
    
    
    
    
    int32_t factor = devContext->AppUnitsPerDevPixelAtUnitFullZoom();

    
    
    
    if (mAdjustOffsetForContextMenu) {
      int32_t offsetForContextMenuDev =
        nsPresContext::CSSPixelsToAppUnits(CONTEXT_MENU_OFFSET_PIXELS) / factor;
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

  
  
  if (mInContentShell || (mFlip != FlipType_None && (!aIsMove || mPopupType != ePopupTypePanel))) {
    nsRect screenRect = GetConstraintRect(anchorRect, rootScreenRect, popupLevel);

    
    anchorRect = anchorRect.Intersect(screenRect);

    
    if (mRect.width > screenRect.width)
      mRect.width = screenRect.width;
    if (mRect.height > screenRect.height)
      mRect.height = screenRect.height;

    
    

    
    
    
    bool slideHorizontal = false, slideVertical = false;
    if (mFlip == FlipType_Slide) {
      int8_t position = GetAlignmentPosition();
      slideHorizontal = position >= POPUPPOSITION_BEFORESTART &&
                        position <= POPUPPOSITION_AFTEREND;
      slideVertical = position >= POPUPPOSITION_STARTBEFORE &&
                      position <= POPUPPOSITION_ENDAFTER;
    }

    
    
    
    if (slideHorizontal) {
      mRect.width = SlideOrResize(screenPoint.x, mRect.width, screenRect.x,
                                  screenRect.XMost(), &mAlignmentOffset);
    } else {
      mRect.width = FlipOrResize(screenPoint.x, mRect.width, screenRect.x,
                                 screenRect.XMost(), anchorRect.x, anchorRect.XMost(),
                                 margin.left, margin.right, offsetForContextMenu, hFlip,
                                 &mHFlip);
    }
    if (slideVertical) {
      mRect.height = SlideOrResize(screenPoint.y, mRect.height, screenRect.y,
                                  screenRect.YMost(), &mAlignmentOffset);
    } else {
      mRect.height = FlipOrResize(screenPoint.y, mRect.height, screenRect.y,
                                  screenRect.YMost(), anchorRect.y, anchorRect.YMost(),
                                  margin.top, margin.bottom, offsetForContextMenu, vFlip,
                                  &mVFlip);
    }

    NS_ASSERTION(screenPoint.x >= screenRect.x && screenPoint.y >= screenRect.y &&
                 screenPoint.x + mRect.width <= screenRect.XMost() &&
                 screenPoint.y + mRect.height <= screenRect.YMost(),
                 "Popup is offscreen");
  }

  
  
  screenPoint.x = presContext->RoundAppUnitsToNearestDevPixels(screenPoint.x);
  screenPoint.y = presContext->RoundAppUnitsToNearestDevPixels(screenPoint.y);

  
  
  nsPoint viewPoint = screenPoint - rootScreenRect.TopLeft();

  nsView* view = GetView();
  NS_ASSERTION(view, "popup with no view");

  
  
  
  
  nsIWidget* widget = view->GetWidget();
  if (mPopupType == ePopupTypePanel && widget) {
    mLastClientOffset = widget->GetClientOffset();
    viewPoint.x += presContext->DevPixelsToAppUnits(mLastClientOffset.x);
    viewPoint.y += presContext->DevPixelsToAppUnits(mLastClientOffset.y);
  }

  presContext->GetPresShell()->GetViewManager()->
    MoveViewTo(view, viewPoint.x, viewPoint.y);

  
  nsBoxFrame::SetPosition(viewPoint - GetParent()->GetOffsetTo(rootFrame));

  if (aSizedToPopup) {
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
                                    const nsRect& aRootScreenRect,
                                    nsPopupLevel aPopupLevel)
{
  nsIntRect screenRectPixels;
  nsPresContext* presContext = PresContext();

  
  
  
  nsCOMPtr<nsIScreen> screen;
  nsCOMPtr<nsIScreenManager> sm(do_GetService("@mozilla.org/gfx/screenmanager;1"));
  if (sm) {
    
    
    
    
    nsRect rect = mInContentShell ? aRootScreenRect : aAnchorRect;
    
    int32_t width = std::max(1, nsPresContext::AppUnitsToIntCSSPixels(rect.width));
    int32_t height = std::max(1, nsPresContext::AppUnitsToIntCSSPixels(rect.height));
    sm->ScreenForRect(nsPresContext::AppUnitsToIntCSSPixels(rect.x),
                      nsPresContext::AppUnitsToIntCSSPixels(rect.y),
                      width, height, getter_AddRefs(screen));
    if (screen) {
      
      
      bool dontOverlapOSBar = aPopupLevel != ePopupLevelTop;
      
      if (!dontOverlapOSBar && mMenuCanOverlapOSBar && !mInContentShell)
        screen->GetRect(&screenRectPixels.x, &screenRectPixels.y,
                        &screenRectPixels.width, &screenRectPixels.height);
      else
        screen->GetAvailRect(&screenRectPixels.x, &screenRectPixels.y,
                             &screenRectPixels.width, &screenRectPixels.height);
    }
  }

  nsRect screenRect = ToAppUnits(screenRectPixels, presContext->AppUnitsPerDevPixel());
  if (mInContentShell) {
    
    screenRect.IntersectRect(screenRect, aRootScreenRect);
  }

  return screenRect;
}

void nsMenuPopupFrame::CanAdjustEdges(int8_t aHorizontalSide,
                                      int8_t aVerticalSide,
                                      LayoutDeviceIntPoint& aChange)
{
  int8_t popupAlign(mPopupAlignment);
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

ConsumeOutsideClicksResult nsMenuPopupFrame::ConsumeOutsideClicks()
{
  
  if (mConsumeRollupEvent != PopupBoxObject::ROLLUP_DEFAULT) {
    return (mConsumeRollupEvent == PopupBoxObject::ROLLUP_CONSUME) ?
           ConsumeOutsideClicks_True : ConsumeOutsideClicks_ParentOnly;
  }

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::consumeoutsideclicks,
                            nsGkAtoms::_true, eCaseMatters)) {
    return ConsumeOutsideClicks_True;
  }
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::consumeoutsideclicks,
                            nsGkAtoms::_false, eCaseMatters)) {
    return ConsumeOutsideClicks_ParentOnly;
  }
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::consumeoutsideclicks,
                            nsGkAtoms::never, eCaseMatters)) {
    return ConsumeOutsideClicks_Never;
  }

  nsCOMPtr<nsIContent> parentContent = mContent->GetParent();
  if (parentContent) {
    dom::NodeInfo *ni = parentContent->NodeInfo();
    if (ni->Equals(nsGkAtoms::menulist, kNameSpaceID_XUL)) {
      return ConsumeOutsideClicks_True;  
    }
#if defined(XP_WIN)
    
    if (ni->Equals(nsGkAtoms::menu, kNameSpaceID_XUL) ||
        ni->Equals(nsGkAtoms::splitmenu, kNameSpaceID_XUL) ||
        ni->Equals(nsGkAtoms::popupset, kNameSpaceID_XUL) ||
        ((ni->Equals(nsGkAtoms::button, kNameSpaceID_XUL) ||
          ni->Equals(nsGkAtoms::toolbarbutton, kNameSpaceID_XUL)) &&
         (parentContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                     nsGkAtoms::menu, eCaseMatters) ||
          parentContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                     nsGkAtoms::menuButton, eCaseMatters)))) {
      return ConsumeOutsideClicks_Never;
    }
#endif
    if (ni->Equals(nsGkAtoms::textbox, kNameSpaceID_XUL)) {
      
      if (parentContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                     nsGkAtoms::autocomplete, eCaseMatters)) {
        return ConsumeOutsideClicks_Never;
      }
    }
  }

  return ConsumeOutsideClicks_True;
}



nsIScrollableFrame* nsMenuPopupFrame::GetScrollFrame(nsIFrame* aStart)
{
  if (!aStart)
    return nullptr;  

  
  nsIFrame* currFrame = aStart;
  do {
    nsIScrollableFrame* sf = do_QueryFrame(currFrame);
    if (sf)
      return sf;
    currFrame = currFrame->GetNextSibling();
  } while (currFrame);

  
  currFrame = aStart;
  do {
    nsIFrame* childFrame = currFrame->GetFirstPrincipalChild();
    nsIScrollableFrame* sf = GetScrollFrame(childFrame);
    if (sf)
      return sf;
    currFrame = currFrame->GetNextSibling();
  } while (currFrame);

  return nullptr;
}

void nsMenuPopupFrame::EnsureMenuItemIsVisible(nsMenuFrame* aMenuItem)
{
  if (aMenuItem) {
    aMenuItem->PresContext()->PresShell()->ScrollFrameRectIntoView(
      aMenuItem,
      nsRect(nsPoint(0,0), aMenuItem->GetRect().Size()),
      nsIPresShell::ScrollAxis(),
      nsIPresShell::ScrollAxis(),
      nsIPresShell::SCROLL_OVERFLOW_HIDDEN |
      nsIPresShell::SCROLL_FIRST_ANCESTOR_ONLY);
  }
}

NS_IMETHODIMP nsMenuPopupFrame::SetCurrentMenuItem(nsMenuFrame* aMenuItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  if (mCurrentMenu) {
    mCurrentMenu->SelectMenu(false);
  }

  if (aMenuItem) {
    EnsureMenuItemIsVisible(aMenuItem);
    aMenuItem->SelectMenu(true);
  }

  mCurrentMenu = aMenuItem;

  return NS_OK;
}

void
nsMenuPopupFrame::CurrentMenuIsBeingDestroyed()
{
  mCurrentMenu = nullptr;
}

NS_IMETHODIMP
nsMenuPopupFrame::ChangeMenuItem(nsMenuFrame* aMenuItem,
                                 bool aSelectFirstItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  
  
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (!mIsContextMenu && pm && pm->HasContextMenu(this))
    return NS_OK;

  
  if (mCurrentMenu) {
    mCurrentMenu->SelectMenu(false);
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
    aMenuItem->SelectMenu(true);
  }

  mCurrentMenu = aMenuItem;

  return NS_OK;
}

nsMenuFrame*
nsMenuPopupFrame::Enter(WidgetGUIEvent* aEvent)
{
  mIncrementalString.Truncate();

  
  if (mCurrentMenu)
    return mCurrentMenu->Enter(aEvent);

  return nullptr;
}

nsMenuFrame*
nsMenuPopupFrame::FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent, bool& doAction)
{
  uint32_t charCode, keyCode;
  aKeyEvent->GetCharCode(&charCode);
  aKeyEvent->GetKeyCode(&keyCode);

  doAction = false;

  
  auto insertion = PresContext()->PresShell()->
    FrameConstructor()->GetInsertionPoint(GetContent(), nullptr);
  nsContainerFrame* immediateParent = insertion.mParentFrame;
  if (!immediateParent)
    immediateParent = this;

  uint32_t matchCount = 0, matchShortcutCount = 0;
  bool foundActive = false;
  bool isShortcut;
  nsMenuFrame* frameBefore = nullptr;
  nsMenuFrame* frameAfter = nullptr;
  nsMenuFrame* frameShortcut = nullptr;

  nsIContent* parentContent = mContent->GetParent();

  bool isMenu = parentContent &&
                  !parentContent->NodeInfo()->Equals(nsGkAtoms::menulist, kNameSpaceID_XUL);

  static DOMTimeStamp lastKeyTime = 0;
  DOMTimeStamp keyTime;
  aKeyEvent->GetTimeStamp(&keyTime);

  if (charCode == 0) {
    if (keyCode == nsIDOMKeyEvent::DOM_VK_BACK_SPACE) {
      if (!isMenu && !mIncrementalString.IsEmpty()) {
        mIncrementalString.SetLength(mIncrementalString.Length() - 1);
        return nullptr;
      }
      else {
#ifdef XP_WIN
        nsCOMPtr<nsISound> soundInterface = do_CreateInstance("@mozilla.org/sound;1");
        if (soundInterface)
          soundInterface->Beep();
#endif  
      }
    }
    return nullptr;
  }
  else {
    char16_t uniChar = ToLowerCase(static_cast<char16_t>(charCode));
    if (isMenu) {
      
      mIncrementalString = uniChar;
    } else if (sTimeoutOfIncrementalSearch &&
               keyTime - lastKeyTime > sTimeoutOfIncrementalSearch) {
      
      mIncrementalString = uniChar;
    } else {
      mIncrementalString.Append(uniChar);
    }
  }

  
  nsAutoString incrementalString(mIncrementalString);
  uint32_t charIndex = 1, stringLength = incrementalString.Length();
  while (charIndex < stringLength && incrementalString[charIndex] == incrementalString[charIndex - 1]) {
    charIndex++;
  }
  if (charIndex == stringLength) {
    incrementalString.Truncate(1);
    stringLength = 1;
  }

  lastKeyTime = keyTime;

  
  
  
  
  
  nsIFrame* firstMenuItem = nsXULPopupManager::GetNextMenuItem(immediateParent, nullptr, true);
  nsIFrame* currFrame = firstMenuItem;

  int32_t menuAccessKey = -1;
  nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);

  
  
  while (currFrame) {
    nsIContent* current = currFrame->GetContent();
    nsAutoString textKey;
    if (menuAccessKey >= 0) {
      
      current->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, textKey);
    }
    if (textKey.IsEmpty()) { 
      isShortcut = false;
      current->GetAttr(kNameSpaceID_None, nsGkAtoms::label, textKey);
      if (textKey.IsEmpty()) 
        current->GetAttr(kNameSpaceID_None, nsGkAtoms::value, textKey);
    }
    else
      isShortcut = true;

    if (StringBeginsWith(textKey, incrementalString,
                         nsCaseInsensitiveStringComparator())) {
      
      nsMenuFrame* menu = do_QueryFrame(currFrame);
      if (menu) {
        
        matchCount++;
        if (isShortcut) {
          
          matchShortcutCount++;
          
          frameShortcut = menu;
        }
        if (!foundActive) {
          
          if (!frameBefore)
            frameBefore = menu;
        }
        else {
          
          if (!frameAfter)
            frameAfter = menu;
        }
      }
      else
        return nullptr;
    }

    
    if (current->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menuactive,
                             nsGkAtoms::_true, eCaseMatters)) {
      foundActive = true;
      if (stringLength > 1) {
        
        
        if (currFrame == frameBefore)
          return frameBefore;
      }
    }

    nsMenuFrame* menu = do_QueryFrame(currFrame);
    currFrame = nsXULPopupManager::GetNextMenuItem(immediateParent, menu, true);
    if (currFrame == firstMenuItem)
      break;
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

  return nullptr;
}

void
nsMenuPopupFrame::LockMenuUntilClosed(bool aLock)
{
  mIsMenuLocked = aLock;

  
  nsMenuFrame* menu = do_QueryFrame(GetParent());
  if (menu) {
    nsMenuParent* parentParent = menu->GetMenuParent();
    if (parentParent) {
      parentParent->LockMenuUntilClosed(aLock);
    }
  }
}

nsIWidget*
nsMenuPopupFrame::GetWidget()
{
  nsView * view = GetRootViewForPopup(this);
  if (!view)
    return nullptr;

  return view->GetWidget();
}

void
nsMenuPopupFrame::AttachedDismissalListener()
{
  mConsumeRollupEvent = PopupBoxObject::ROLLUP_DEFAULT;
}



nsresult 
nsMenuPopupFrame::AttributeChanged(int32_t aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   int32_t aModType)

{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  
  if (aAttribute == nsGkAtoms::left || aAttribute == nsGkAtoms::top)
    MoveToAttributePosition();

  if (aAttribute == nsGkAtoms::label) {
    
    nsView* view = GetView();
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
  nsresult err1, err2;
  int32_t xpos = left.ToInteger(&err1);
  int32_t ypos = top.ToInteger(&err2);

  if (NS_SUCCEEDED(err1) && NS_SUCCEEDED(err2))
    MoveTo(xpos, ypos, false);
}

void
nsMenuPopupFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (mReflowCallbackData.mPosted) {
    PresContext()->PresShell()->CancelReflowCallback(this);
    mReflowCallbackData.Clear();
  }

  nsMenuFrame* menu = do_QueryFrame(GetParent());
  if (menu) {
    
    nsContentUtils::AddScriptRunner(
      new nsUnsetAttrRunnable(menu->GetContent(), nsGkAtoms::open));
  }

  ClearPopupShownDispatcher();

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm)
    pm->PopupDestroyed(this);

  nsIRootBox* rootBox =
    nsIRootBox::GetRootBox(PresContext()->GetPresShell());
  if (rootBox && rootBox->GetDefaultTooltip() == mContent) {
    rootBox->SetDefaultTooltip(nullptr);
  }

  nsBoxFrame::DestroyFrom(aDestructRoot);
}


void
nsMenuPopupFrame::MoveTo(int32_t aLeft, int32_t aTop, bool aUpdateAttrs)
{
  nsIWidget* widget = GetWidget();
  if ((mScreenXPos == aLeft && mScreenYPos == aTop) &&
      (!widget || widget->GetClientOffset() == mLastClientOffset)) {
    return;
  }

  
  
  
  
  nsMargin margin(0, 0, 0, 0);
  StyleMargin()->GetMargin(margin);

  
  if (mAdjustOffsetForContextMenu) {
    nscoord offsetForContextMenu =
      nsPresContext::CSSPixelsToAppUnits(CONTEXT_MENU_OFFSET_PIXELS);
    margin.left += offsetForContextMenu;
    margin.top += offsetForContextMenu;
  }

  nsPresContext* presContext = PresContext();
  mScreenXPos = aLeft - presContext->AppUnitsToIntCSSPixels(margin.left);
  mScreenYPos = aTop - presContext->AppUnitsToIntCSSPixels(margin.top);

  SetPopupPosition(nullptr, true, false);

  nsCOMPtr<nsIContent> popup = mContent;
  if (aUpdateAttrs && (popup->HasAttr(kNameSpaceID_None, nsGkAtoms::left) ||
                       popup->HasAttr(kNameSpaceID_None, nsGkAtoms::top)))
  {
    nsAutoString left, top;
    left.AppendInt(aLeft);
    top.AppendInt(aTop);
    popup->SetAttr(kNameSpaceID_None, nsGkAtoms::left, left, false);
    popup->SetAttr(kNameSpaceID_None, nsGkAtoms::top, top, false);
  }
}

void
nsMenuPopupFrame::MoveToAnchor(nsIContent* aAnchorContent,
                               const nsAString& aPosition,
                               int32_t aXPos, int32_t aYPos,
                               bool aAttributesOverride)
{
  NS_ASSERTION(IsVisible(), "popup must be visible to move it");

  nsPopupState oldstate = mPopupState;
  InitializePopup(aAnchorContent, mTriggerContent, aPosition,
                  aXPos, aYPos, aAttributesOverride);
  
  mPopupState = oldstate;

  
  SetPopupPosition(nullptr, false, false);
}

bool
nsMenuPopupFrame::GetAutoPosition()
{
  return mShouldAutoPosition;
}

void
nsMenuPopupFrame::SetAutoPosition(bool aShouldAutoPosition)
{
  mShouldAutoPosition = aShouldAutoPosition;
}

void
nsMenuPopupFrame::SetConsumeRollupEvent(uint32_t aConsumeMode)
{
  mConsumeRollupEvent = aConsumeMode;
}

int8_t
nsMenuPopupFrame::GetAlignmentPosition() const
{
  
  

  if (mPosition == POPUPPOSITION_OVERLAP || mPosition == POPUPPOSITION_AFTERPOINTER)
    return mPosition;

  int8_t position = mPosition;

  if (position == POPUPPOSITION_UNKNOWN) {
    switch (mPopupAnchor) {
      case POPUPALIGNMENT_BOTTOMCENTER:
        position = mPopupAlignment == POPUPALIGNMENT_TOPRIGHT ?
                     POPUPPOSITION_AFTEREND : POPUPPOSITION_AFTERSTART;
        break;
      case POPUPALIGNMENT_TOPCENTER:
        position = mPopupAlignment == POPUPALIGNMENT_BOTTOMRIGHT ?
                     POPUPPOSITION_BEFOREEND : POPUPPOSITION_BEFORESTART;
        break;
      case POPUPALIGNMENT_LEFTCENTER:
        position = mPopupAlignment == POPUPALIGNMENT_BOTTOMRIGHT ?
                     POPUPPOSITION_STARTAFTER : POPUPPOSITION_STARTBEFORE;
        break;
      case POPUPALIGNMENT_RIGHTCENTER:
        position = mPopupAlignment == POPUPALIGNMENT_BOTTOMLEFT ?
                     POPUPPOSITION_ENDAFTER : POPUPPOSITION_ENDBEFORE;
        break;
      default:
        break;
    }
  }

  if (mHFlip) {
    position = POPUPPOSITION_HFLIP(position);
  }

  if (mVFlip) {
    position = POPUPPOSITION_VFLIP(position);
  }

  return position;
}





void
nsMenuPopupFrame::CreatePopupView()
{
  if (HasView()) {
    return;
  }

  nsViewManager* viewManager = PresContext()->GetPresShell()->GetViewManager();
  NS_ASSERTION(nullptr != viewManager, "null view manager");

  
  nsView* parentView = viewManager->GetRootView();
  nsViewVisibility visibility = nsViewVisibility_kHide;
  int32_t zIndex = INT32_MAX;
  bool    autoZIndex = false;

  NS_ASSERTION(parentView, "no parent view");

  
  nsView *view = viewManager->CreateView(GetRect(), parentView, visibility);
  viewManager->SetViewZIndex(view, autoZIndex, zIndex);
  
  viewManager->InsertChild(parentView, view, nullptr, true);

  
  SetView(view);

  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
    ("nsMenuPopupFrame::CreatePopupView: frame=%p view=%p", this, view));
}
