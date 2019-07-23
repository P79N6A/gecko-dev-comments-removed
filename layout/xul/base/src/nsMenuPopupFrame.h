











































#ifndef nsMenuPopupFrame_h__
#define nsMenuPopupFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventTarget.h"
#include "nsMenuListener.h"

#include "nsBoxFrame.h"
#include "nsIMenuParent.h"
#include "nsIWidget.h"

#include "nsITimer.h"

#define INC_TYP_INTERVAL  1000  // 1s. If the interval between two keypresses is shorter than this, 
                                





nsIFrame* NS_NewMenuPopupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsIViewManager;
class nsIView;
class nsIMenuParent;
class nsIMenuFrame;
class nsIDOMXULDocument;

class nsMenuPopupFrame;








class nsMenuPopupTimerMediator : public nsITimerCallback
{
public:
  nsMenuPopupTimerMediator(nsMenuPopupFrame* aFrame);
  ~nsMenuPopupTimerMediator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  void ClearFrame();

private:

  
  nsMenuPopupFrame* mFrame;
};

class nsMenuPopupFrame : public nsBoxFrame, public nsIMenuParent
{
public:
  nsMenuPopupFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  NS_DECL_ISUPPORTS


  
  virtual nsIMenuFrame* GetCurrentMenuItem();
  NS_IMETHOD SetCurrentMenuItem(nsIMenuFrame* aMenuItem);
  virtual nsIMenuFrame* GetNextMenuItem(nsIMenuFrame* aStart);
  virtual nsIMenuFrame* GetPreviousMenuItem(nsIMenuFrame* aStart);
  NS_IMETHOD SetActive(PRBool aActiveFlag) { return NS_OK; } 
  NS_IMETHOD GetIsActive(PRBool& isActive) { isActive = PR_FALSE; return NS_OK; }
  NS_IMETHOD IsMenuBar(PRBool& isMenuBar) { isMenuBar = PR_FALSE; return NS_OK; }
  NS_IMETHOD ConsumeOutsideClicks(PRBool& aConsumeOutsideClicks);
  NS_IMETHOD ClearRecentlyRolledUp() {return NS_OK;}
  NS_IMETHOD RecentlyRolledUp(nsIMenuFrame *aMenuFrame, PRBool *aJustRolledUp) {*aJustRolledUp = PR_FALSE; return NS_OK;}
  NS_IMETHOD SetIsContextMenu(PRBool aIsContextMenu) { mIsContextMenu = aIsContextMenu; return NS_OK; }
  NS_IMETHOD GetIsContextMenu(PRBool& aIsContextMenu) { aIsContextMenu = mIsContextMenu; return NS_OK; }
  
  NS_IMETHOD GetParentPopup(nsIMenuParent** aResult);

  
  NS_IMETHOD DismissChain();

  
  NS_IMETHOD HideChain();

  NS_IMETHOD KillPendingTimers();
  NS_IMETHOD CancelPendingTimers();

  NS_IMETHOD InstallKeyboardNavigator();
  NS_IMETHOD RemoveKeyboardNavigator();

  NS_IMETHOD GetWidget(nsIWidget **aWidget);

  
  NS_IMETHOD AttachedDismissalListener();

  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus);

  virtual void Destroy();

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRBool aImmediate);

  virtual nsresult CreateWidgetForView(nsIView* aView);

  void GetViewOffset(nsIView* aView, nsPoint& aPoint);
  static void GetRootViewForPopup(nsIFrame* aStartFrame,
                                  PRBool aStopAtViewManagerRoot,
                                  nsIView** aResult);

  nsresult SyncViewWithFrame(nsPresContext* aPresContext, const nsString& aPopupAnchor,
                             const nsString& aPopupAlign,
                             nsIFrame* aFrame, PRInt32 aXPos, PRInt32 aYPos);

  NS_IMETHOD KeyboardNavigation(PRUint32 aKeyCode, PRBool& aHandledFlag);
  NS_IMETHOD ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag);
  
  NS_IMETHOD Escape(PRBool& aHandledFlag);
  NS_IMETHOD Enter();

  nsIMenuFrame* FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent, PRBool& doAction);

  PRBool IsValidItem(nsIContent* aContent);
  PRBool IsDisabled(nsIContent* aContent);

  nsIMenuParent* GetContextMenu();

  NS_IMETHOD KillCloseTimer();

  virtual nsIAtom* GetType() const { return nsGkAtoms::menuPopupFrame; }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("MenuPopup"), aResult);
  }
#endif

  void EnsureMenuItemIsVisible(nsIMenuFrame* aMenuFrame);

  
  
  void MoveTo(PRInt32 aLeft, PRInt32 aTop);

  void GetAutoPosition(PRBool* aShouldAutoPosition);
  void SetAutoPosition(PRBool aShouldAutoPosition);
  void EnableRollup(PRBool aShouldRollup);
  void SetConsumeRollupEvent(PRUint32 aConsumeMode);

  nsIScrollableView* GetScrollableView(nsIFrame* aStart);
  
protected:
  friend class nsMenuPopupTimerMediator;
  NS_HIDDEN_(nsresult) Notify(nsITimer* aTimer);

  
  void MoveToInternal(PRInt32 aLeft, PRInt32 aTop);

  
  
  virtual void GetLayoutFlags(PRUint32& aFlags);

  
  void AdjustClientXYForNestedDocuments ( nsIDOMXULDocument* inPopupDoc, nsIPresShell* inPopupShell, 
                                            PRInt32 inClientX, PRInt32 inClientY, 
                                            PRInt32* outAdjX, PRInt32* outAdjY ) ;

  void AdjustPositionForAnchorAlign ( PRInt32* ioXPos, PRInt32* ioYPos, const nsRect & inParentRect,
                                        const nsString& aPopupAnchor, const nsString& aPopupAlign,
                                        PRBool* outFlushWithTopBottom ) ;

  PRBool IsMoreRoomOnOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32 inScreenViewLocX, PRInt32 inScreenViewLocY,
                                           const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                           PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips ) ;

  void MovePopupToOtherSideOfParent ( PRBool inFlushAboveBelow, PRInt32* ioXPos, PRInt32* ioYPos, 
                                           PRInt32* ioScreenViewLocX, PRInt32* ioScreenViewLocY,
                                           const nsRect & inScreenParentFrameRect, PRInt32 inScreenTopTwips, PRInt32 inScreenLeftTwips,
                                           PRInt32 inScreenBottomTwips, PRInt32 inScreenRightTwips ) ;

  
  void MoveToAttributePosition();


  nsIMenuFrame* mCurrentMenu; 

  nsMenuListener* mKeyboardNavigator; 
  nsIDOMEventTarget* mTarget;

  nsIMenuFrame* mTimerMenu; 
  nsCOMPtr<nsITimer> mCloseTimer; 

  
  nsRefPtr<nsMenuPopupTimerMediator> mTimerMediator;

  PRPackedBool mIsContextMenu;  
  
  PRPackedBool mMenuCanOverlapOSBar;    

  PRPackedBool mShouldAutoPosition; 
  PRPackedBool mShouldRollup; 
  PRPackedBool mConsumeRollupEvent; 
  PRPackedBool mInContentShell; 

  nsString     mIncrementalString;  

}; 

#endif
