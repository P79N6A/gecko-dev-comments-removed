







































#ifndef nsXULPopupManager_h__
#define nsXULPopupManager_h__

#include "nsIContent.h"
#include "nsIWidget.h"
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIDOMKeyListener.h"
#include "nsPoint.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"


















class nsIPresShell;
class nsMenuFrame;
class nsMenuPopupFrame;
class nsMenuBarFrame;
class nsIMenuParent;
class nsIDOMKeyEvent;



























enum nsNavigationDirection {
  eNavigationDirection_Last,
  eNavigationDirection_First,
  eNavigationDirection_Start,
  eNavigationDirection_Before,
  eNavigationDirection_End,
  eNavigationDirection_After
};

#define NS_DIRECTION_IS_INLINE(dir) (dir == eNavigationDirection_Start ||     \
                                     dir == eNavigationDirection_End)
#define NS_DIRECTION_IS_BLOCK(dir) (dir == eNavigationDirection_Before || \
                                    dir == eNavigationDirection_After)
#define NS_DIRECTION_IS_BLOCK_TO_EDGE(dir) (dir == eNavigationDirection_First ||    \
                                            dir == eNavigationDirection_Last)






extern nsNavigationDirection DirectionFromKeyCode_lr_tb [6];






extern nsNavigationDirection DirectionFromKeyCode_rl_tb [6];

#define NS_DIRECTION_FROM_KEY_CODE(frame, direction, keycode)    \
  NS_ASSERTION(NS_VK_HOME == NS_VK_END + 1, "Broken ordering");  \
  NS_ASSERTION(NS_VK_LEFT == NS_VK_END + 2, "Broken ordering");  \
  NS_ASSERTION(NS_VK_UP == NS_VK_END + 3, "Broken ordering");    \
  NS_ASSERTION(NS_VK_RIGHT == NS_VK_END + 4, "Broken ordering"); \
  NS_ASSERTION(NS_VK_DOWN == NS_VK_END + 5, "Broken ordering");  \
  NS_ASSERTION(keycode >= NS_VK_END && keycode <= NS_VK_DOWN,    \
               "Illegal key code");                              \
  const nsStyleVisibility* vis = frame->GetStyleVisibility();    \
  if (vis->mDirection == NS_STYLE_DIRECTION_RTL)                 \
    direction = DirectionFromKeyCode_rl_tb[keycode - NS_VK_END]; \
  else                                                           \
    direction = DirectionFromKeyCode_lr_tb[keycode - NS_VK_END];




class nsMenuChainItem
{
private:
  nsMenuPopupFrame* mFrame; 
  PRPackedBool mIsMenu; 
  PRPackedBool mIsContext; 
  PRPackedBool mOnMenuBar; 
  PRPackedBool mIgnoreKeys; 

  nsMenuChainItem* mParent;
  nsMenuChainItem* mChild;

public:
  nsMenuChainItem(nsMenuPopupFrame* aFrame, PRBool aIsContext, PRBool aIsMenu)
    : mFrame(aFrame),
      mIsMenu(aIsMenu),
      mIsContext(aIsContext),
      mOnMenuBar(PR_FALSE),
      mIgnoreKeys(!aIsMenu), 
      mParent(nsnull),
      mChild(nsnull)
  {
    NS_ASSERTION(aFrame, "null frame passed to nsMenuChainItem constructor");
    MOZ_COUNT_CTOR(nsMenuChainItem);
  }

  ~nsMenuChainItem()
  {
    MOZ_COUNT_DTOR(nsMenuChainItem);
  }

  nsIContent* Content();
  nsMenuPopupFrame* Frame() { return mFrame; }
  PRBool IsMenu() { return mIsMenu; }
  PRBool IsContextMenu() { return mIsContext; }
  PRBool IgnoreKeys() { return mIgnoreKeys; }
  PRBool IsOnMenuBar() { return mOnMenuBar; }
  void SetIgnoreKeys(PRBool aIgnoreKeys) { mIgnoreKeys = aIgnoreKeys; }
  void SetOnMenuBar(PRBool aOnMenuBar) { mOnMenuBar = aOnMenuBar; }
  nsMenuChainItem* GetParent() { return mParent; }
  nsMenuChainItem* GetChild() { return mChild; }

  
  
  void SetParent(nsMenuChainItem* aParent);

  
  
  
  
  void Detach(nsMenuChainItem** aRoot);
};


class nsXULPopupShowingEvent : public nsRunnable
{
public:
  nsXULPopupShowingEvent(nsIContent *aPopup,
                         nsIContent *aMenu,
                         PRBool aIsContextMenu,
                         PRBool aSelectFirstItem)
    : mPopup(aPopup),
      mMenu(aMenu),
      mIsContextMenu(aIsContextMenu),
      mSelectFirstItem(aSelectFirstItem)
  {
    NS_ASSERTION(aPopup, "null popup supplied to nsXULPopupShowingEvent constructor");
    NS_ASSERTION(aMenu, "null menu supplied to nsXULPopupShowingEvent constructor");
  }

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIContent> mPopup;
  nsCOMPtr<nsIContent> mMenu;
  PRBool mIsContextMenu;
  PRBool mSelectFirstItem;
};


class nsXULPopupHidingEvent : public nsRunnable
{
public:
  nsXULPopupHidingEvent(nsIContent *aPopup,
                        nsIContent* aNextPopup,
                        nsIContent* aLastPopup,
                        PRBool aIsMenu,
                        PRBool aDeselectMenu)
    : mPopup(aPopup),
      mNextPopup(aNextPopup),
      mLastPopup(aLastPopup),
      mIsMenu(aIsMenu),
      mDeselectMenu(aDeselectMenu)
  {
    NS_ASSERTION(aPopup, "null popup supplied to nsXULPopupHidingEvent constructor");
    
  }

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIContent> mPopup;
  nsCOMPtr<nsIContent> mNextPopup;
  nsCOMPtr<nsIContent> mLastPopup;
  PRBool mIsMenu;
  PRBool mDeselectMenu;
};


class nsXULMenuCommandEvent : public nsRunnable
{
public:
  nsXULMenuCommandEvent(nsIContent *aMenu,
                        PRBool aIsTrusted,
                        PRBool aShift,
                        PRBool aControl,
                        PRBool aAlt,
                        PRBool aMeta,
                        PRBool aUserInput)
    : mMenu(aMenu),
      mIsTrusted(aIsTrusted),
      mShift(aShift),
      mControl(aControl),
      mAlt(aAlt),
      mMeta(aMeta),
      mUserInput(aUserInput)
  {
    NS_ASSERTION(aMenu, "null menu supplied to nsXULMenuCommandEvent constructor");
  }

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIContent> mMenu;
  PRBool mIsTrusted;
  PRBool mShift;
  PRBool mControl;
  PRBool mAlt;
  PRBool mMeta;
  PRBool mUserInput;
};

class nsXULPopupManager : public nsIDOMKeyListener,
                          public nsIMenuRollup,
                          public nsIRollupListener,
                          public nsITimerCallback
{

public:
  friend class nsXULPopupShowingEvent;
  friend class nsXULPopupHidingEvent;
  friend class nsXULMenuCommandEvent;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIROLLUPLISTENER
  NS_DECL_NSIMENUROLLUP
  NS_DECL_NSITIMERCALLBACK

  static nsXULPopupManager* sInstance;

  
  static nsresult Init();
  static void Shutdown();

  
  
  static nsXULPopupManager* GetInstance();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static nsMenuFrame* GetPreviousMenuItem(nsIFrame* aParent,
                                          nsMenuFrame* aStart,
                                          PRBool aIsPopup);
  static nsMenuFrame* GetNextMenuItem(nsIFrame* aParent,
                                      nsMenuFrame* aStart,
                                      PRBool aIsPopup);

  
  
  
  static PRBool IsValidMenuItem(nsPresContext* aPresContext,
                                nsIContent* aContent,
                                PRBool aOnPopup);

  
  
  
  
  
  
  
  void SetActiveMenuBar(nsMenuBarFrame* aMenuBar, PRBool aActivate);

  
  
  
  
  
  void GetMouseLocation(nsIDOMNode** aNode, PRInt32* aOffset);
  
  
  void SetMouseLocation(nsIDOMEvent* aEvent, nsIContent* aPopup);

  





  void ShowMenu(nsIContent *aMenu, PRBool aSelectFirstItem, PRBool aAsynchronous);

  






  void ShowPopup(nsIContent* aPopup,
                 nsIContent* aAnchorContent,
                 const nsAString& aPosition,
                 PRInt32 aXPos, PRInt32 aYPos,
                 PRBool aIsContextMenu,
                 PRBool aAttributesOverride,
                 PRBool aSelectFirstItem);

  





  void ShowPopupAtScreen(nsIContent* aPopup,
                         PRInt32 aXPos, PRInt32 aYPos,
                         PRBool aIsContextMenu);

  





  void ShowPopupWithAnchorAlign(nsIContent* aPopup,
                                nsIContent* aAnchorContent,
                                nsAString& aAnchor,
                                nsAString& aAlign,
                                PRInt32 aXPos, PRInt32 aYPos,
                                PRBool aIsContextMenu);

  












  void HidePopup(nsIContent* aPopup,
                 PRBool aHideChain,
                 PRBool aDeselectMenu,
                 PRBool aAsynchronous);

  




  void HidePopupAfterDelay(nsMenuPopupFrame* aPopup);

  



  void HidePopupsInDocument(nsIDocument* aDocument);

  






  void ExecuteMenu(nsIContent* aMenu, nsEvent* aEvent);

  


  PRBool IsPopupOpen(nsIContent* aPopup);

  


  PRBool IsPopupOpenForMenuParent(nsIMenuParent* aMenuParent);

  



  nsTArray<nsIFrame *> GetOpenPopups();

  




  PRBool MayShowPopup(nsMenuPopupFrame* aFrame);

  




  void PopupDestroyed(nsMenuPopupFrame* aFrame);

  




  PRBool HasContextMenu(nsMenuPopupFrame* aPopup);

  





  void UpdateMenuItems(nsIContent* aPopup);

  




  void KillMenuTimer();

  







  void CancelMenuTimer(nsIMenuParent* aMenuParent);

  





  PRBool HandleShortcutNavigation(nsIDOMKeyEvent* aKeyEvent,
                                  nsMenuPopupFrame* aFrame);

  



  PRBool HandleKeyboardNavigation(PRUint32 aKeyCode);

  




  PRBool HandleKeyboardNavigationInPopup(nsMenuPopupFrame* aFrame,
                                         nsNavigationDirection aDir)
  {
    return HandleKeyboardNavigationInPopup(nsnull, aFrame, aDir);
  }

  NS_IMETHODIMP HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);

protected:
  nsXULPopupManager();
  ~nsXULPopupManager();

  
  
  nsIFrame* GetFrameOfTypeForContent(nsIContent* aContent, nsIAtom* aFrameType);

  
  nsMenuFrame* GetMenuFrameForContent(nsIContent* aContent);

  
  nsMenuPopupFrame* GetPopupFrameForContent(nsIContent* aContent);

  
  nsMenuChainItem* GetTopVisibleMenu();

  
  void ShowPopupCallback(nsIContent* aPopup,
                         nsMenuPopupFrame* aPopupFrame,
                         PRBool aIsContextMenu,
                         PRBool aSelectFirstItem);
  void HidePopupCallback(nsIContent* aPopup,
                         nsMenuPopupFrame* aPopupFrame,
                         nsIContent* aNextPopup,
                         nsIContent* aLastPopup,
                         PRBool aIsMenu,
                         PRBool aDeselectMenu);

  











  void FirePopupShowingEvent(nsIContent* aPopup,
                             nsIContent* aMenu,
                             nsPresContext* aPresContext,
                             PRBool aIsContextMenu,
                             PRBool aSelectFirstItem);

  
















  void FirePopupHidingEvent(nsIContent* aPopup,
                            nsIContent* aNextPopup,
                            nsIContent* aLastPopup,
                            nsPresContext *aPresContext,
                            PRBool aIsMenu,
                            PRBool aDeselectMenu);

  


  PRBool HandleKeyboardNavigationInPopup(nsMenuChainItem* aItem,
                                         nsNavigationDirection aDir)
  {
    return HandleKeyboardNavigationInPopup(aItem, aItem->Frame(), aDir);
  }

private:
  






  PRBool HandleKeyboardNavigationInPopup(nsMenuChainItem* aItem,
                                         nsMenuPopupFrame* aFrame,
                                         nsNavigationDirection aDir);

protected:

  




  void SetCaptureState(nsIContent *aOldPopup);

  










  void UpdateKeyboardListeners();

  
  nsCOMPtr<nsIDOMEventTarget> mKeyListener;

  
  nsCOMPtr<nsIWidget> mWidget;

  
  nsCOMPtr<nsIDOMNode> mRangeParent;
  PRInt32 mRangeOffset;
  nsPoint mCachedMousePoint;

  
  nsMenuBarFrame* mActiveMenuBar;

  
  nsMenuChainItem* mCurrentMenu;

  
  nsMenuChainItem* mPanels;

  
  nsCOMPtr<nsITimer> mCloseTimer;

  
  nsMenuPopupFrame* mTimerMenu;
};

#endif
