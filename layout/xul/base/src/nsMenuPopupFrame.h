











































#ifndef nsMenuPopupFrame_h__
#define nsMenuPopupFrame_h__

#include "prtypes.h"
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
#define POPUPALIGNMENT_RIGHTCENTER 17
#define POPUPALIGNMENT_TOPCENTER 18
#define POPUPALIGNMENT_BOTTOMCENTER 19

#define INC_TYP_INTERVAL  1000  // 1s. If the interval between two keypresses is shorter than this, 
                                





nsIFrame* NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsIViewManager;
class nsIView;
class nsMenuPopupFrame;

class nsMenuPopupFrame : public nsBoxFrame, public nsMenuParent
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsMenuPopupFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  
  virtual nsMenuFrame* GetCurrentMenuItem();
  NS_IMETHOD SetCurrentMenuItem(nsMenuFrame* aMenuItem);
  virtual void CurrentMenuIsBeingDestroyed();
  NS_IMETHOD ChangeMenuItem(nsMenuFrame* aMenuItem, PRBool aSelectFirstItem);

  
  
  nsPopupState PopupState() { return mPopupState; }
  void SetPopupState(nsPopupState aPopupState) { mPopupState = aPopupState; }

  NS_IMETHOD SetActive(PRBool aActiveFlag) { return NS_OK; } 
  virtual PRBool IsActive() { return PR_FALSE; }
  virtual PRBool IsMenuBar() { return PR_FALSE; }

  















  PRBool ConsumeOutsideClicks();

  virtual PRBool IsContextMenu() { return mIsContextMenu; }

  virtual PRBool MenuClosed() { return PR_TRUE; }

  virtual void LockMenuUntilClosed(PRBool aLock);
  virtual PRBool IsMenuLocked() { return mIsMenuLocked; }

  NS_IMETHOD GetWidget(nsIWidget **aWidget);

  
  void AttachedDismissalListener();

  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRUint32 aFlags);

  
  
  PRBool IsNoAutoHide() const;

  nsPopupLevel PopupLevel() const
  {
    return PopupLevel(IsNoAutoHide()); 
  }

  void EnsureWidget();

  nsresult CreateWidgetForView(nsIView* aView);
  PRUint8 GetShadowStyle();

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);

  virtual PRBool IsLeaf() const;

  
  void LayoutPopup(nsBoxLayoutState& aState, nsIFrame* aParentMenu, PRBool aSizedToPopup);

  nsIView* GetRootViewForPopup(nsIFrame* aStartFrame);

  
  
  
  
  
  nsresult SetPopupPosition(nsIFrame* aAnchorFrame, PRBool aIsMove);

  PRBool HasGeneratedChildren() { return mGeneratedChildren; }
  void SetGeneratedChildren() { mGeneratedChildren = PR_TRUE; }

  
  
  
  
  
  
  nsMenuFrame* Enter(nsGUIEvent* aEvent);

  nsPopupType PopupType() const { return mPopupType; }
  PRBool IsMenu() { return mPopupType == ePopupTypeMenu; }
  PRBool IsOpen() { return mPopupState == ePopupOpen || mPopupState == ePopupOpenAndVisible; }

  
  nsMenuFrame* GetParentMenu() {
    nsIFrame* parent = GetParent();
    if (parent && parent->GetType() == nsGkAtoms::menuFrame) {
      return static_cast<nsMenuFrame *>(parent);
    }
    return nsnull;
  }

  static nsIContent* GetTriggerContent(nsMenuPopupFrame* aMenuPopupFrame);
  void ClearTriggerContent() { mTriggerContent = nsnull; }

  
  
  PRBool IsInContentShell() { return mInContentShell; }

  
  
  void InitializePopup(nsIContent* aAnchorContent,
                       nsIContent* aTriggerContent,
                       const nsAString& aPosition,
                       PRInt32 aXPos, PRInt32 aYPos,
                       PRBool aAttributesOverride);

  




  void InitializePopupAtScreen(nsIContent* aTriggerContent,
                               PRInt32 aXPos, PRInt32 aYPos,
                               PRBool aIsContextMenu);

  void InitializePopupWithAnchorAlign(nsIContent* aAnchorContent,
                                      nsAString& aAnchor,
                                      nsAString& aAlign,
                                      PRInt32 aXPos, PRInt32 aYPos);

  
  void ShowPopup(PRBool aIsContextMenu, PRBool aSelectFirstItem);
  
  
  void HidePopup(PRBool aDeselectMenu, nsPopupState aNewState);

  
  
  
  
  
  
  nsMenuFrame* FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent, PRBool& doAction);

  void ClearIncrementalString() { mIncrementalString.Truncate(); }

  virtual nsIAtom* GetType() const { return nsGkAtoms::menuPopupFrame; }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("MenuPopup"), aResult);
  }
#endif

  void EnsureMenuItemIsVisible(nsMenuFrame* aMenuFrame);

  
  
  
  
  void MoveTo(PRInt32 aLeft, PRInt32 aTop, PRBool aUpdateAttrs);

  PRBool GetAutoPosition();
  void SetAutoPosition(PRBool aShouldAutoPosition);
  void SetConsumeRollupEvent(PRUint32 aConsumeMode);

  nsIScrollableFrame* GetScrollFrame(nsIFrame* aStart);

  
  
  
  
  
  nsRect GetConstraintRect(const nsRect& aAnchorRect, const nsRect& aRootScreenRect);

  
  
  
  
  
  
  
  
  
  
  void CanAdjustEdges(PRInt8 aHorizontalSide, PRInt8 aVerticalSide, nsIntPoint& aChange);

  
  PRBool IsAnchored() const { return mScreenXPos == -1 && mScreenYPos == -1; }

  
  nsIContent* GetAnchor() const { return mAnchorContent; }

  
  nsIntPoint ScreenPosition() const { return nsIntPoint(mScreenXPos, mScreenYPos); }

protected:

  
  nsPopupLevel PopupLevel(PRBool aIsNoAutoHide) const;

  
  virtual void GetLayoutFlags(PRUint32& aFlags);

  void InitPositionFromAnchorAlign(const nsAString& aAnchor,
                                   const nsAString& aAlign);

  
  
  
  nsPoint AdjustPositionForAnchorAlign(nsRect& anchorRect,
                                       FlipStyle& aHFlip, FlipStyle& aVFlip);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord FlipOrResize(nscoord& aScreenPoint, nscoord aSize, 
                       nscoord aScreenBegin, nscoord aScreenEnd,
                       nscoord aAnchorBegin, nscoord aAnchorEnd,
                       nscoord aMarginBegin, nscoord aMarginEnd,
                       nscoord aOffsetForContextMenu, FlipStyle aFlip,
                       PRPackedBool* aFlipSide);

  
  void MoveToAttributePosition();

  






  bool IsDirectionRTL() const {
    return mAnchorContent && mAnchorContent->GetPrimaryFrame()
      ? mAnchorContent->GetPrimaryFrame()->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL
      : GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
  }

  nsString     mIncrementalString;  

  
  
  nsCOMPtr<nsIContent> mAnchorContent;

  
  
  nsCOMPtr<nsIContent> mTriggerContent;

  nsMenuFrame* mCurrentMenu; 

  
  
  
  
  nsSize mPrefSize;

  
  
  PRInt32 mXPos;
  PRInt32 mYPos;
  PRInt32 mScreenXPos;
  PRInt32 mScreenYPos;

  nsPopupType mPopupType; 
  nsPopupState mPopupState; 

  
  PRInt8 mPopupAlignment;
  PRInt8 mPopupAnchor;
  PRPackedBool mFlipBoth; 

  PRPackedBool mIsOpenChanged; 
  PRPackedBool mIsContextMenu; 
  
  PRPackedBool mAdjustOffsetForContextMenu;
  PRPackedBool mGeneratedChildren; 

  PRPackedBool mMenuCanOverlapOSBar;    
  PRPackedBool mShouldAutoPosition; 
  PRPackedBool mConsumeRollupEvent; 
  PRPackedBool mInContentShell; 
  PRPackedBool mIsMenuLocked; 

  
  PRPackedBool mHFlip;
  PRPackedBool mVFlip;

  static PRInt8 sDefaultLevelIsTop;
}; 

#endif
