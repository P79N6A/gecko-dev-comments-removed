











































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



#define POPUPALIGNMENT_NONE 0
#define POPUPALIGNMENT_TOPLEFT 1
#define POPUPALIGNMENT_TOPRIGHT -1
#define POPUPALIGNMENT_BOTTOMLEFT 2
#define POPUPALIGNMENT_BOTTOMRIGHT -2

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

  NS_IMETHOD GetWidget(nsIWidget **aWidget);

  
  void AttachedDismissalListener();

  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  virtual void Destroy();

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRUint32 aFlags);

  
  
  PRBool IsNoAutoHide();

  
  
  PRBool IsTopMost();

  void EnsureWidget();

  virtual nsresult CreateWidgetForView(nsIView* aView);

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);

  virtual PRBool IsLeaf() const;

  
  
  void AdjustView();

  nsIView* GetRootViewForPopup(nsIFrame* aStartFrame);

  
  
  
  
  
  nsresult SetPopupPosition(nsIFrame* aAnchorFrame, PRBool aIsMove = PR_FALSE);

  PRBool HasGeneratedChildren() { return mGeneratedChildren; }
  void SetGeneratedChildren() { mGeneratedChildren = PR_TRUE; }

  
  
  
  
  
  
  nsMenuFrame* Enter();

  nsPopupType PopupType() const { return mPopupType; }
  PRBool IsMenu() { return mPopupType == ePopupTypeMenu; }
  PRBool IsOpen() { return mPopupState == ePopupOpen || mPopupState == ePopupOpenAndVisible; }
  PRBool HasOpenChanged() { return mIsOpenChanged; }

  
  
  PRBool IsInContentShell() { return mInContentShell; }

  
  
  void InitializePopup(nsIContent* aAnchorContent,
                       const nsAString& aPosition,
                       PRInt32 aXPos, PRInt32 aYPos,
                       PRBool aAttributesOverride);

  




  void InitializePopupAtScreen(PRInt32 aXPos, PRInt32 aYPos,
                               PRBool aIsContextMenu);

  void InitializePopupWithAnchorAlign(nsIContent* aAnchorContent,
                                      nsAString& aAnchor,
                                      nsAString& aAlign,
                                      PRInt32 aXPos, PRInt32 aYPos);

  
  PRBool ShowPopup(PRBool aIsContextMenu, PRBool aSelectFirstItem);
  
  
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

  nsIScrollableView* GetScrollableView(nsIFrame* aStart);

  
  void SetPreferredBounds(nsBoxLayoutState& aState, const nsRect& aRect);

  
  nsSize PreferredSize() { return mPrefSize; }
  
  void SetPreferredSize(nsSize aSize) { mPrefSize = aSize; }

protected:

  
  virtual void GetLayoutFlags(PRUint32& aFlags);

  void InitPositionFromAnchorAlign(const nsAString& aAnchor,
                                   const nsAString& aAlign);

  
  
  
  nsPoint AdjustPositionForAnchorAlign(const nsRect& anchorRect, PRBool& aHFlip, PRBool& aVFlip);


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord FlipOrResize(nscoord& aScreenPoint, nscoord aSize, 
                       nscoord aScreenBegin, nscoord aScreenEnd,
                       nscoord aAnchorBegin, nscoord aAnchorEnd,
                       nscoord aMarginBegin, nscoord aMarginEnd,
                       nscoord aOffsetForContextMenu, PRBool aFlip);

  
  void MoveToAttributePosition();

  nsString     mIncrementalString;  

  
  
  nsCOMPtr<nsIContent> mAnchorContent;

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

  PRPackedBool mIsOpenChanged; 
  PRPackedBool mIsContextMenu; 
  
  PRPackedBool mAdjustOffsetForContextMenu;
  PRPackedBool mGeneratedChildren; 

  PRPackedBool mMenuCanOverlapOSBar;    
  PRPackedBool mShouldAutoPosition; 
  PRPackedBool mConsumeRollupEvent; 
  PRPackedBool mInContentShell; 

  static PRInt8 sDefaultLevelParent;
}; 

#endif
