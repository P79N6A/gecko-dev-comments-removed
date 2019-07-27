








#ifndef nsMenuPopupFrame_h__
#define nsMenuPopupFrame_h__

#include "mozilla/Attributes.h"
#include "nsIAtom.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsMenuFrame.h"

#include "nsBoxFrame.h"
#include "nsMenuParent.h"

#include "nsITimer.h"

class nsIWidget;


















enum nsPopupState {
  
  ePopupClosed,
  
  
  ePopupShowing,
  
  ePopupOpening,
  
  ePopupVisible,
  
  ePopupShown,
  
  ePopupHiding,
  
  
  
  
  
  
  ePopupInvisible
};

enum ConsumeOutsideClicksResult {
  ConsumeOutsideClicks_ParentOnly = 0, 
  ConsumeOutsideClicks_True = 1, 
  ConsumeOutsideClicks_Never = 2 
};




enum FlipStyle {
  FlipStyle_None = 0,
  FlipStyle_Outside = 1,
  FlipStyle_Inside = 2
};


enum FlipType {
  FlipType_Default = 0,
  FlipType_None = 1,    
  FlipType_Both = 2,    
  FlipType_Slide = 3    
};



#define POPUPALIGNMENT_NONE 0
#define POPUPALIGNMENT_TOPLEFT 1
#define POPUPALIGNMENT_TOPRIGHT -1
#define POPUPALIGNMENT_BOTTOMLEFT 2
#define POPUPALIGNMENT_BOTTOMRIGHT -2

#define POPUPALIGNMENT_LEFTCENTER 16
#define POPUPALIGNMENT_RIGHTCENTER -16
#define POPUPALIGNMENT_TOPCENTER 17
#define POPUPALIGNMENT_BOTTOMCENTER 18



#define POPUPPOSITION_UNKNOWN -1
#define POPUPPOSITION_BEFORESTART 0
#define POPUPPOSITION_BEFOREEND 1
#define POPUPPOSITION_AFTERSTART 2
#define POPUPPOSITION_AFTEREND 3
#define POPUPPOSITION_STARTBEFORE 4
#define POPUPPOSITION_ENDBEFORE 5
#define POPUPPOSITION_STARTAFTER 6
#define POPUPPOSITION_ENDAFTER 7
#define POPUPPOSITION_OVERLAP 8
#define POPUPPOSITION_AFTERPOINTER 9

#define POPUPPOSITION_HFLIP(v) (v ^ 1)
#define POPUPPOSITION_VFLIP(v) (v ^ 2)

#define INC_TYP_INTERVAL  1000  // 1s. If the interval between two keypresses is shorter than this, 
                                





#define CONTEXT_MENU_OFFSET_PIXELS 2

nsIFrame* NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsViewManager;
class nsView;
class nsMenuPopupFrame;


class nsXULPopupShownEvent : public nsRunnable, public nsIDOMEventListener
{
public:
  nsXULPopupShownEvent(nsIContent *aPopup, nsPresContext* aPresContext)
    : mPopup(aPopup), mPresContext(aPresContext)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIDOMEVENTLISTENER

  void CancelListener();

protected:
  virtual ~nsXULPopupShownEvent() { }

private:
  nsCOMPtr<nsIContent> mPopup;
  nsRefPtr<nsPresContext> mPresContext;
};

class nsMenuPopupFrame MOZ_FINAL : public nsBoxFrame, public nsMenuParent
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsMenuPopupFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsMenuPopupFrame(nsStyleContext* aContext);

  
  virtual nsMenuFrame* GetCurrentMenuItem() MOZ_OVERRIDE;
  NS_IMETHOD SetCurrentMenuItem(nsMenuFrame* aMenuItem) MOZ_OVERRIDE;
  virtual void CurrentMenuIsBeingDestroyed() MOZ_OVERRIDE;
  NS_IMETHOD ChangeMenuItem(nsMenuFrame* aMenuItem, bool aSelectFirstItem) MOZ_OVERRIDE;

  
  
  nsPopupState PopupState() { return mPopupState; }
  void SetPopupState(nsPopupState aPopupState) { mPopupState = aPopupState; }

  NS_IMETHOD SetActive(bool aActiveFlag) MOZ_OVERRIDE { return NS_OK; } 
  virtual bool IsActive() MOZ_OVERRIDE { return false; }
  virtual bool IsMenuBar() MOZ_OVERRIDE { return false; }

  















  ConsumeOutsideClicksResult ConsumeOutsideClicks();

  virtual bool IsContextMenu() MOZ_OVERRIDE { return mIsContextMenu; }

  virtual bool MenuClosed() MOZ_OVERRIDE { return true; }

  virtual void LockMenuUntilClosed(bool aLock) MOZ_OVERRIDE;
  virtual bool IsMenuLocked() MOZ_OVERRIDE { return mIsMenuLocked; }

  nsIWidget* GetWidget();

  
  void AttachedDismissalListener();

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  
  
  bool IsNoAutoHide() const;

  nsPopupLevel PopupLevel() const
  {
    return PopupLevel(IsNoAutoHide()); 
  }

  void EnsureWidget();

  nsresult CreateWidgetForView(nsView* aView);
  uint8_t GetShadowStyle();

  virtual void SetInitialChildList(ChildListID  aListID,
                                   nsFrameList& aChildList) MOZ_OVERRIDE;

  virtual bool IsLeaf() const MOZ_OVERRIDE;

  
  void LayoutPopup(nsBoxLayoutState& aState, nsIFrame* aParentMenu,
                   nsIFrame* aAnchor, bool aSizedToPopup);

  nsView* GetRootViewForPopup(nsIFrame* aStartFrame);

  
  
  
  
  
  nsresult SetPopupPosition(nsIFrame* aAnchorFrame, bool aIsMove, bool aSizedToPopup);

  bool HasGeneratedChildren() { return mGeneratedChildren; }
  void SetGeneratedChildren() { mGeneratedChildren = true; }

  
  
  
  
  
  
  nsMenuFrame* Enter(mozilla::WidgetGUIEvent* aEvent);

  nsPopupType PopupType() const { return mPopupType; }
  bool IsMenu() MOZ_OVERRIDE { return mPopupType == ePopupTypeMenu; }
  bool IsOpen() MOZ_OVERRIDE { return mPopupState == ePopupOpening ||
                                      mPopupState == ePopupVisible ||
                                      mPopupState == ePopupShown; }
  bool IsVisible() { return mPopupState == ePopupVisible ||
                            mPopupState == ePopupShown; }

  bool IsMouseTransparent() { return mMouseTransparent; }

  static nsIContent* GetTriggerContent(nsMenuPopupFrame* aMenuPopupFrame);
  void ClearTriggerContent() { mTriggerContent = nullptr; }

  
  
  bool IsInContentShell() { return mInContentShell; }

  
  
  void InitializePopup(nsIContent* aAnchorContent,
                       nsIContent* aTriggerContent,
                       const nsAString& aPosition,
                       int32_t aXPos, int32_t aYPos,
                       bool aAttributesOverride);

  




  void InitializePopupAtScreen(nsIContent* aTriggerContent,
                               int32_t aXPos, int32_t aYPos,
                               bool aIsContextMenu);

  void InitializePopupWithAnchorAlign(nsIContent* aAnchorContent,
                                      nsAString& aAnchor,
                                      nsAString& aAlign,
                                      int32_t aXPos, int32_t aYPos);

  
  void ShowPopup(bool aIsContextMenu, bool aSelectFirstItem);
  
  
  void HidePopup(bool aDeselectMenu, nsPopupState aNewState);

  
  
  
  
  
  
  nsMenuFrame* FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent, bool& doAction);

  void ClearIncrementalString() { mIncrementalString.Truncate(); }

  virtual nsIAtom* GetType() const MOZ_OVERRIDE { return nsGkAtoms::menuPopupFrame; }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
      return MakeFrameName(NS_LITERAL_STRING("MenuPopup"), aResult);
  }
#endif

  void EnsureMenuItemIsVisible(nsMenuFrame* aMenuFrame);

  
  
  
  
  void MoveTo(int32_t aLeft, int32_t aTop, bool aUpdateAttrs);

  void MoveToAnchor(nsIContent* aAnchorContent,
                    const nsAString& aPosition,
                    int32_t aXPos, int32_t aYPos,
                    bool aAttributesOverride);

  bool GetAutoPosition();
  void SetAutoPosition(bool aShouldAutoPosition);
  void SetConsumeRollupEvent(uint32_t aConsumeMode);

  nsIScrollableFrame* GetScrollFrame(nsIFrame* aStart);

  
  
  
  
  
  
  
  
  nsRect GetConstraintRect(const nsRect& aAnchorRect, const nsRect& aRootScreenRect,
                           nsPopupLevel aPopupLevel);

  
  
  
  
  
  
  
  
  
  
  void CanAdjustEdges(int8_t aHorizontalSide, int8_t aVerticalSide, nsIntPoint& aChange);

  
  bool IsAnchored() const { return mScreenXPos == -1 && mScreenYPos == -1; }

  
  nsIContent* GetAnchor() const { return mAnchorContent; }

  
  
  nsIntPoint ScreenPosition() const { return nsIntPoint(mScreenXPos, mScreenYPos); }

  nsIntPoint GetLastClientOffset() const { return mLastClientOffset; }

  
  int8_t GetAlignmentPosition() const;

  
  nscoord GetAlignmentOffset() const { return mAlignmentOffset; }

  
  
  bool ClearPopupShownDispatcher()
  {
    if (mPopupShownDispatcher) {
      mPopupShownDispatcher->CancelListener();
      mPopupShownDispatcher = nullptr;
      return true;
    }

    return false;
  }

protected:

  
  nsPopupLevel PopupLevel(bool aIsNoAutoHide) const;

  
  virtual void GetLayoutFlags(uint32_t& aFlags) MOZ_OVERRIDE;

  void InitPositionFromAnchorAlign(const nsAString& aAnchor,
                                   const nsAString& aAlign);

  
  
  
  nsPoint AdjustPositionForAnchorAlign(nsRect& anchorRect,
                                       FlipStyle& aHFlip, FlipStyle& aVFlip);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord FlipOrResize(nscoord& aScreenPoint, nscoord aSize, 
                       nscoord aScreenBegin, nscoord aScreenEnd,
                       nscoord aAnchorBegin, nscoord aAnchorEnd,
                       nscoord aMarginBegin, nscoord aMarginEnd,
                       nscoord aOffsetForContextMenu, FlipStyle aFlip,
                       bool* aFlipSide);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord SlideOrResize(nscoord& aScreenPoint, nscoord aSize,
                        nscoord aScreenBegin, nscoord aScreenEnd,
                        nscoord *aOffset);

  
  void MoveToAttributePosition();

  






  bool IsDirectionRTL() const {
    return mAnchorContent && mAnchorContent->GetPrimaryFrame()
      ? mAnchorContent->GetPrimaryFrame()->StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL
      : StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  }

  
  
  void CreatePopupView();

  nsString     mIncrementalString;  

  
  
  nsCOMPtr<nsIContent> mAnchorContent;

  
  
  nsCOMPtr<nsIContent> mTriggerContent;

  nsMenuFrame* mCurrentMenu; 

  nsRefPtr<nsXULPopupShownEvent> mPopupShownDispatcher;

  
  
  
  
  nsSize mPrefSize;

  
  
  
  int32_t mXPos;
  int32_t mYPos;
  int32_t mScreenXPos;
  int32_t mScreenYPos;

  
  
  
  nscoord mAlignmentOffset;

  
  
  
  nsIntPoint mLastClientOffset;

  nsPopupType mPopupType; 
  nsPopupState mPopupState; 

  
  int8_t mPopupAlignment;
  int8_t mPopupAnchor;
  int8_t mPosition;

  
  uint8_t mConsumeRollupEvent;
  FlipType mFlip; 

  bool mIsOpenChanged; 
  bool mIsContextMenu; 
  
  bool mAdjustOffsetForContextMenu;
  bool mGeneratedChildren; 

  bool mMenuCanOverlapOSBar;    
  bool mShouldAutoPosition; 
  bool mInContentShell; 
  bool mIsMenuLocked; 
  bool mMouseTransparent; 

  
  bool mHFlip;
  bool mVFlip;

  static int8_t sDefaultLevelIsTop;
}; 

#endif
