








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






#define CONTEXT_MENU_OFFSET_PIXELS 2

nsIFrame* NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

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

class nsMenuPopupFrame final : public nsBoxFrame, public nsMenuParent,
                               public nsIReflowCallback
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsMenuPopupFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsMenuPopupFrame(nsStyleContext* aContext);

  
  virtual nsMenuFrame* GetCurrentMenuItem() override;
  NS_IMETHOD SetCurrentMenuItem(nsMenuFrame* aMenuItem) override;
  virtual void CurrentMenuIsBeingDestroyed() override;
  NS_IMETHOD ChangeMenuItem(nsMenuFrame* aMenuItem, bool aSelectFirstItem) override;

  
  
  nsPopupState PopupState() { return mPopupState; }
  void SetPopupState(nsPopupState aPopupState) { mPopupState = aPopupState; }

  NS_IMETHOD SetActive(bool aActiveFlag) override { return NS_OK; } 
  virtual bool IsActive() override { return false; }
  virtual bool IsMenuBar() override { return false; }

  















  ConsumeOutsideClicksResult ConsumeOutsideClicks();

  virtual bool IsContextMenu() override { return mIsContextMenu; }

  virtual bool MenuClosed() override { return true; }

  virtual void LockMenuUntilClosed(bool aLock) override;
  virtual bool IsMenuLocked() override { return mIsMenuLocked; }

  nsIWidget* GetWidget();

  
  void AttachedDismissalListener();

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  
  
  bool IsNoAutoHide() const;

  nsPopupLevel PopupLevel() const
  {
    return PopupLevel(IsNoAutoHide()); 
  }

  void EnsureWidget();

  nsresult CreateWidgetForView(nsView* aView);
  uint8_t GetShadowStyle();

  virtual void SetInitialChildList(ChildListID  aListID,
                                   nsFrameList& aChildList) override;

  virtual bool IsLeaf() const override;

  
  void LayoutPopup(nsBoxLayoutState& aState, nsIFrame* aParentMenu,
                   nsIFrame* aAnchor, bool aSizedToPopup);

  nsView* GetRootViewForPopup(nsIFrame* aStartFrame);

  
  
  
  
  
  nsresult SetPopupPosition(nsIFrame* aAnchorFrame, bool aIsMove, bool aSizedToPopup);

  bool HasGeneratedChildren() { return mGeneratedChildren; }
  void SetGeneratedChildren() { mGeneratedChildren = true; }

  
  
  
  
  
  
  nsMenuFrame* Enter(mozilla::WidgetGUIEvent* aEvent);

  nsPopupType PopupType() const { return mPopupType; }
  bool IsMenu() override { return mPopupType == ePopupTypeMenu; }
  bool IsOpen() override { return mPopupState == ePopupOpening ||
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

  virtual nsIAtom* GetType() const override { return nsGkAtoms::menuPopupFrame; }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
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

  
  
  
  
  
  
  
  
  
  
  void CanAdjustEdges(int8_t aHorizontalSide,
                      int8_t aVerticalSide,
                      mozilla::LayoutDeviceIntPoint& aChange);

  
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

  
  virtual bool ReflowFinished() override;
  virtual void ReflowCallbackCanceled() override;

protected:

  
  nsPopupLevel PopupLevel(bool aIsNoAutoHide) const;

  
  virtual void GetLayoutFlags(uint32_t& aFlags) override;

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

  struct ReflowCallbackData {
    ReflowCallbackData() :
      mPosted(false),
      mAnchor(nullptr),
      mSizedToPopup(false)
    {}
    void MarkPosted(nsIFrame* aAnchor, bool aSizedToPopup) {
      mPosted = true;
      mAnchor = aAnchor;
      mSizedToPopup = aSizedToPopup;
    }
    void Clear() {
      mPosted = false;
      mAnchor = nullptr;
      mSizedToPopup = false;
    }
    bool mPosted;
    nsIFrame* mAnchor;
    bool mSizedToPopup;
  };
  ReflowCallbackData mReflowCallbackData;

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

  
  static uint32_t sTimeoutOfIncrementalSearch;
}; 

#endif
