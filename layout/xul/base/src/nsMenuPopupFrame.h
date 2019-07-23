











































#ifndef nsMenuPopupFrame_h__
#define nsMenuPopupFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsMenuFrame.h"
#include "nsIDOMEventTarget.h"

#include "nsBoxFrame.h"
#include "nsIMenuParent.h"
#include "nsIWidget.h"

#include "nsITimer.h"

enum nsPopupType {
  ePopupTypePanel,
  ePopupTypeMenu,
  ePopupTypeTooltip
};














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
class nsIMenuParent;
class nsMenuPopupFrame;

class nsMenuPopupFrame : public nsBoxFrame, public nsIMenuParent
{
public:
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
                                  PRBool aImmediate);

  void EnsureWidget();

  virtual nsresult CreateWidgetForView(nsIView* aView);

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  virtual PRBool IsLeaf() const
  {
    if (!mGeneratedChildren && mPopupType == ePopupTypeMenu) {
      
      
      
      
      
      nsIContent* parentContent = mContent->GetParent();
      if (parentContent &&
          !parentContent->HasAttr(kNameSpaceID_None, nsGkAtoms::sizetopopup))
        return PR_TRUE;
    }

    return PR_FALSE;
  }

  
  
  void AdjustView();

  void GetViewOffset(nsIView* aView, nsPoint& aPoint);
  nsIView* GetRootViewForPopup(nsIFrame* aStartFrame,
                               PRBool aStopAtViewManagerRoot);

  
  
  
  
  nsresult SetPopupPosition(nsIFrame* aAnchorFrame);

  PRBool HasGeneratedChildren() { return mGeneratedChildren; }
  void SetGeneratedChildren() { mGeneratedChildren = PR_TRUE; }

  
  
  
  
  nsMenuFrame* Enter();

  PRInt32 PopupType() const { return mPopupType; }
  PRBool IsMenu() { return mPopupType == ePopupTypeMenu; }
  PRBool IsOpen() { return mPopupState == ePopupOpen || mPopupState == ePopupOpenAndVisible; }
  PRBool HasOpenChanged() { return mIsOpenChanged; }

  
  
  PRBool IsInContentShell() { return mInContentShell; }

  
  
  void InitializePopup(nsIContent* aAnchorContent,
                       const nsAString& aPosition,
                       PRInt32 aXPos, PRInt32 aYPos,
                       PRBool aAttributesOverride);

  void InitializePopupAtScreen(PRInt32 aXPos, PRInt32 aYPos);

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

  
  
  void MoveTo(PRInt32 aLeft, PRInt32 aTop);

  void GetAutoPosition(PRBool* aShouldAutoPosition);
  void SetAutoPosition(PRBool aShouldAutoPosition);
  void SetConsumeRollupEvent(PRUint32 aConsumeMode);

  nsIScrollableView* GetScrollableView(nsIFrame* aStart);
  
protected:
  
  void MoveToInternal(PRInt32 aLeft, PRInt32 aTop);                             

  
  virtual void GetLayoutFlags(PRUint32& aFlags);

  void InitPositionFromAnchorAlign(const nsAString& aAnchor,
                                   const nsAString& aAlign);

  void AdjustPositionForAnchorAlign ( PRInt32* ioXPos, PRInt32* ioYPos, const nsRect & inParentRect,
                                      PRBool* outFlushWithTopBottom ) ;

  PRBool IsMoreRoomOnOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32 inScreenViewLocX, PRInt32 inScreenViewLocY,
                                           const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                           PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips ) ;

  void MovePopupToOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32* ioXPos, PRInt32* ioYPos, 
                                           PRInt32* ioScreenViewLocX, PRInt32* ioScreenViewLocY,
                                           const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                           PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips ) ;

  
  void MoveToAttributePosition();

  
  
  nsCOMPtr<nsIContent> mAnchorContent;

  nsMenuFrame* mCurrentMenu; 

  
  PRInt8 mPopupAlignment;
  PRInt8 mPopupAnchor;

  
  
  PRInt32 mXPos;
  PRInt32 mYPos;
  PRInt32 mScreenXPos;
  PRInt32 mScreenYPos;

  nsPopupType mPopupType; 
  nsPopupState mPopupState; 

  PRPackedBool mIsOpenChanged; 
  PRPackedBool mIsContextMenu; 
  PRPackedBool mGeneratedChildren; 

  PRPackedBool mMenuCanOverlapOSBar;    
  PRPackedBool mShouldAutoPosition; 
  PRPackedBool mConsumeRollupEvent; 
  PRPackedBool mInContentShell; 

  nsString     mIncrementalString;  

}; 

#endif
