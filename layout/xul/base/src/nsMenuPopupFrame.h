








#ifndef nsMenuPopupFrame_h__
#define nsMenuPopupFrame_h__

#include "mozilla/Attributes.h"
#include "nsIAtom.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsMenuFrame.h"
#include "nsIDOMEventTarget.h"

#include "nsBoxFrame.h"
#include "nsMenuParent.h"

#include "nsITimer.h"

class nsIWidget;













enum nsPopupState {
  
  ePopupClosed,
  
  
  ePopupShowing,
  
  ePopupOpen,
  
  ePopupOpenAndVisible,
  
  ePopupHiding,
  
  
  
  
  
  
  ePopupInvisible
};




enum FlipStyle {
  FlipStyle_None = 0,
  FlipStyle_Outside = 1,
  FlipStyle_Inside = 2
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

#define INC_TYP_INTERVAL  1000  // 1s. If the interval between two keypresses is shorter than this, 
                                





#define CONTEXT_MENU_OFFSET_PIXELS 2

nsIFrame* NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsViewManager;
class nsView;
class nsMenuPopupFrame;

class nsMenuPopupFrame : public nsBoxFrame, public nsMenuParent
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsMenuPopupFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  nsMenuPopupFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  
  virtual nsMenuFrame* GetCurrentMenuItem();
  NS_IMETHOD SetCurrentMenuItem(nsMenuFrame* aMenuItem) MOZ_OVERRIDE;
  virtual void CurrentMenuIsBeingDestroyed() MOZ_OVERRIDE;
  NS_IMETHOD ChangeMenuItem(nsMenuFrame* aMenuItem, bool aSelectFirstItem) MOZ_OVERRIDE;

  
  
  nsPopupState PopupState() { return mPopupState; }
  void SetPopupState(nsPopupState aPopupState) { mPopupState = aPopupState; }

  NS_IMETHOD SetActive(bool aActiveFlag) { return NS_OK; } 
  virtual bool IsActive() MOZ_OVERRIDE { return false; }
  virtual bool IsMenuBar() MOZ_OVERRIDE { return false; }

  















  bool ConsumeOutsideClicks();

  virtual bool IsContextMenu() MOZ_OVERRIDE { return mIsContextMenu; }

  virtual bool MenuClosed() MOZ_OVERRIDE { return true; }

  virtual void LockMenuUntilClosed(bool aLock) MOZ_OVERRIDE;
  virtual bool IsMenuLocked() MOZ_OVERRIDE { return mIsMenuLocked; }

  nsIWidget* GetWidget();

  
  void AttachedDismissalListener();

  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;

  NS_IMETHOD AttributeChanged(int32_t aNameSpaceID,
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

  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList) MOZ_OVERRIDE;

  virtual bool IsLeaf() const;

  
  void LayoutPopup(nsBoxLayoutState& aState, nsIFrame* aParentMenu, bool aSizedToPopup);

  nsView* GetRootViewForPopup(nsIFrame* aStartFrame);

  
  
  
  
  
  nsresult SetPopupPosition(nsIFrame* aAnchorFrame, bool aIsMove);

  bool HasGeneratedChildren() { return mGeneratedChildren; }
  void SetGeneratedChildren() { mGeneratedChildren = true; }

  
  
  
  
  
  
  nsMenuFrame* Enter(nsGUIEvent* aEvent);

  nsPopupType PopupType() const { return mPopupType; }
  bool IsMenu() { return mPopupType == ePopupTypeMenu; }
  bool IsOpen() MOZ_OVERRIDE { return mPopupState == ePopupOpen || mPopupState == ePopupOpenAndVisible; }

  bool IsDragPopup() { return mIsDragPopup; }

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

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
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

  
  
  
  
  
  nsRect GetConstraintRect(const nsRect& aAnchorRect, const nsRect& aRootScreenRect);

  
  
  
  
  
  
  
  
  
  
  void CanAdjustEdges(int8_t aHorizontalSide, int8_t aVerticalSide, nsIntPoint& aChange);

  
  bool IsAnchored() const { return mScreenXPos == -1 && mScreenYPos == -1; }

  
  nsIContent* GetAnchor() const { return mAnchorContent; }

  
  
  nsIntPoint ScreenPosition() const { return nsIntPoint(mScreenXPos, mScreenYPos); }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  nsIntPoint GetLastClientOffset() const { return mLastClientOffset; }

protected:

  
  nsPopupLevel PopupLevel(bool aIsNoAutoHide) const;

  
  virtual void GetLayoutFlags(uint32_t& aFlags);

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

  
  void MoveToAttributePosition();

  






  bool IsDirectionRTL() const {
    return mAnchorContent && mAnchorContent->GetPrimaryFrame()
      ? mAnchorContent->GetPrimaryFrame()->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL
      : GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  }

  
  
  nsresult CreatePopupView();

  nsString     mIncrementalString;  

  
  
  nsCOMPtr<nsIContent> mAnchorContent;

  
  
  nsCOMPtr<nsIContent> mTriggerContent;

  nsMenuFrame* mCurrentMenu; 

  
  
  
  
  nsSize mPrefSize;

  
  
  
  int32_t mXPos;
  int32_t mYPos;
  int32_t mScreenXPos;
  int32_t mScreenYPos;
  
  
  
  nsIntPoint mLastClientOffset;

  nsPopupType mPopupType; 
  nsPopupState mPopupState; 

  
  int8_t mPopupAlignment;
  int8_t mPopupAnchor;
  
  int8_t mConsumeRollupEvent;
  bool mFlipBoth; 

  bool mIsOpenChanged; 
  bool mIsContextMenu; 
  
  bool mAdjustOffsetForContextMenu;
  bool mGeneratedChildren; 

  bool mMenuCanOverlapOSBar;    
  bool mShouldAutoPosition; 
  bool mInContentShell; 
  bool mIsMenuLocked; 
  bool mIsDragPopup; 

  
  bool mHFlip;
  bool mVFlip;

  static int8_t sDefaultLevelIsTop;
}; 

#endif
