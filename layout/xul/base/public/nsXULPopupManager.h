







































#ifndef nsXULPopupManager_h__
#define nsXULPopupManager_h__

#include "prlog.h"
#include "nsGUIEvent.h"
#include "nsIContent.h"
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIDOMEventListener.h"
#include "nsPoint.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsIReflowCallback.h"
#include "nsThreadUtils.h"
#include "nsStyleConsts.h"


#ifdef KeyPress
#undef KeyPress
#endif

















class nsMenuFrame;
class nsMenuPopupFrame;
class nsMenuBarFrame;
class nsMenuParent;
class nsIDOMKeyEvent;
class nsIDocShellTreeItem;
class nsIView;



enum CloseMenuMode {
  CloseMenuMode_Auto, 
  CloseMenuMode_None, 
  CloseMenuMode_Single 
};



























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

PR_STATIC_ASSERT(NS_STYLE_DIRECTION_LTR == 0 && NS_STYLE_DIRECTION_RTL == 1);
PR_STATIC_ASSERT((NS_VK_HOME == NS_VK_END + 1) &&
                 (NS_VK_LEFT == NS_VK_END + 2) &&
                 (NS_VK_UP == NS_VK_END + 3) &&
                 (NS_VK_RIGHT == NS_VK_END + 4) &&
                 (NS_VK_DOWN == NS_VK_END + 5));






extern const nsNavigationDirection DirectionFromKeyCodeTable[2][6];

#define NS_DIRECTION_FROM_KEY_CODE(frame, keycode)                     \
  (DirectionFromKeyCodeTable[frame->GetStyleVisibility()->mDirection]  \
                            [keycode - NS_VK_END])




class nsMenuChainItem
{
private:
  nsMenuPopupFrame* mFrame; 
  nsPopupType mPopupType; 
  PRPackedBool mIsContext; 
  PRPackedBool mOnMenuBar; 
  PRPackedBool mIgnoreKeys; 

  nsMenuChainItem* mParent;
  nsMenuChainItem* mChild;

public:
  nsMenuChainItem(nsMenuPopupFrame* aFrame, PRBool aIsContext, nsPopupType aPopupType)
    : mFrame(aFrame),
      mPopupType(aPopupType),
      mIsContext(aIsContext),
      mOnMenuBar(PR_FALSE),
      mIgnoreKeys(PR_FALSE),
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
  nsPopupType PopupType() { return mPopupType; }
  PRBool IsMenu() { return mPopupType == ePopupTypeMenu; }
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
                         PRBool aIsContextMenu,
                         PRBool aSelectFirstItem)
    : mPopup(aPopup),
      mIsContextMenu(aIsContextMenu),
      mSelectFirstItem(aSelectFirstItem)
  {
    NS_ASSERTION(aPopup, "null popup supplied to nsXULPopupShowingEvent constructor");
  }

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIContent> mPopup;
  PRBool mIsContextMenu;
  PRBool mSelectFirstItem;
};


class nsXULPopupHidingEvent : public nsRunnable
{
public:
  nsXULPopupHidingEvent(nsIContent *aPopup,
                        nsIContent* aNextPopup,
                        nsIContent* aLastPopup,
                        nsPopupType aPopupType,
                        PRBool aDeselectMenu)
    : mPopup(aPopup),
      mNextPopup(aNextPopup),
      mLastPopup(aLastPopup),
      mPopupType(aPopupType),
      mDeselectMenu(aDeselectMenu)
  {
    NS_ASSERTION(aPopup, "null popup supplied to nsXULPopupHidingEvent constructor");
    
  }

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIContent> mPopup;
  nsCOMPtr<nsIContent> mNextPopup;
  nsCOMPtr<nsIContent> mLastPopup;
  nsPopupType mPopupType;
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
                        PRBool aUserInput,
                        PRBool aFlipChecked)
    : mMenu(aMenu),
      mIsTrusted(aIsTrusted),
      mShift(aShift),
      mControl(aControl),
      mAlt(aAlt),
      mMeta(aMeta),
      mUserInput(aUserInput),
      mFlipChecked(aFlipChecked),
      mCloseMenuMode(CloseMenuMode_Auto)
  {
    NS_ASSERTION(aMenu, "null menu supplied to nsXULMenuCommandEvent constructor");
  }

  NS_IMETHOD Run();

  void SetCloseMenuMode(CloseMenuMode aCloseMenuMode) { mCloseMenuMode = aCloseMenuMode; }

private:
  nsCOMPtr<nsIContent> mMenu;
  PRBool mIsTrusted;
  PRBool mShift;
  PRBool mControl;
  PRBool mAlt;
  PRBool mMeta;
  PRBool mUserInput;
  PRBool mFlipChecked;
  CloseMenuMode mCloseMenuMode;
};

class nsXULPopupManager : public nsIDOMEventListener,
                          public nsIMenuRollup,
                          public nsIRollupListener,
                          public nsITimerCallback,
                          public nsIObserver
{

public:
  friend class nsXULPopupShowingEvent;
  friend class nsXULPopupHidingEvent;
  friend class nsXULMenuCommandEvent;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIDOMEVENTLISTENER

  
  NS_IMETHOD Rollup(PRUint32 aCount, nsIContent **aContent);
  NS_IMETHOD ShouldRollupOnMouseWheelEvent(PRBool *aShould);
  NS_IMETHOD ShouldRollupOnMouseActivate(PRBool *aShould);

  virtual PRUint32 GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain);
  virtual void AdjustPopupsOnWindowChange(nsPIDOMWindow* aWindow);

  static nsXULPopupManager* sInstance;

  
  static nsresult Init();
  static void Shutdown();

  
  
  static nsXULPopupManager* GetInstance();

  
  
  
  nsIFrame* GetFrameOfTypeForContent(nsIContent* aContent,
                                     nsIAtom* aFrameType,
                                     PRBool aShouldFlush);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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

  





  void ShowMenu(nsIContent *aMenu, PRBool aSelectFirstItem, PRBool aAsynchronous);

  










  void ShowPopup(nsIContent* aPopup,
                 nsIContent* aAnchorContent,
                 const nsAString& aPosition,
                 PRInt32 aXPos, PRInt32 aYPos,
                 PRBool aIsContextMenu,
                 PRBool aAttributesOverride,
                 PRBool aSelectFirstItem,
                 nsIDOMEvent* aTriggerEvent);

  









  void ShowPopupAtScreen(nsIContent* aPopup,
                         PRInt32 aXPos, PRInt32 aYPos,
                         PRBool aIsContextMenu,
                         nsIDOMEvent* aTriggerEvent);

  





  void ShowTooltipAtScreen(nsIContent* aPopup,
                           nsIContent* aTriggerContent,
                           PRInt32 aXPos, PRInt32 aYPos);

  





  void ShowPopupWithAnchorAlign(nsIContent* aPopup,
                                nsIContent* aAnchorContent,
                                nsAString& aAnchor,
                                nsAString& aAlign,
                                PRInt32 aXPos, PRInt32 aYPos,
                                PRBool aIsContextMenu);

  














  void HidePopup(nsIContent* aPopup,
                 PRBool aHideChain,
                 PRBool aDeselectMenu,
                 PRBool aAsynchronous,
                 nsIContent* aLastPopup = nsnull);

  


  void HidePopup(nsIView* aView);

  




  void HidePopupAfterDelay(nsMenuPopupFrame* aPopup);

  



  void HidePopupsInDocShell(nsIDocShellTreeItem* aDocShellToHide);

  






  void ExecuteMenu(nsIContent* aMenu, nsXULMenuCommandEvent* aEvent);

  


  PRBool IsPopupOpen(nsIContent* aPopup);

  


  PRBool IsPopupOpenForMenuParent(nsMenuParent* aMenuParent);

  




  nsIFrame* GetTopPopup(nsPopupType aType);

  



  nsTArray<nsIFrame *> GetVisiblePopups();

  




  already_AddRefed<nsIDOMNode> GetLastTriggerPopupNode(nsIDocument* aDocument)
  {
    return GetLastTriggerNode(aDocument, PR_FALSE);
  }

  already_AddRefed<nsIDOMNode> GetLastTriggerTooltipNode(nsIDocument* aDocument)
  {
    return GetLastTriggerNode(aDocument, PR_TRUE);
  }

  




  PRBool MayShowPopup(nsMenuPopupFrame* aFrame);

  



  void PopupMoved(nsIView* aView, nsIntPoint aPoint);

  



  void PopupResized(nsIView* aView, nsIntSize ASize);

  




  void PopupDestroyed(nsMenuPopupFrame* aFrame);

  




  PRBool HasContextMenu(nsMenuPopupFrame* aPopup);

  





  void UpdateMenuItems(nsIContent* aPopup);

  




  void KillMenuTimer();

  







  void CancelMenuTimer(nsMenuParent* aMenuParent);

  





  PRBool HandleShortcutNavigation(nsIDOMKeyEvent* aKeyEvent,
                                  nsMenuPopupFrame* aFrame);

  



  PRBool HandleKeyboardNavigation(PRUint32 aKeyCode);

  




  PRBool HandleKeyboardNavigationInPopup(nsMenuPopupFrame* aFrame,
                                         nsNavigationDirection aDir)
  {
    return HandleKeyboardNavigationInPopup(nsnull, aFrame, aDir);
  }

  nsresult KeyUp(nsIDOMKeyEvent* aKeyEvent);
  nsresult KeyDown(nsIDOMKeyEvent* aKeyEvent);
  nsresult KeyPress(nsIDOMKeyEvent* aKeyEvent);

protected:
  nsXULPopupManager();
  ~nsXULPopupManager();

  
  nsMenuFrame* GetMenuFrameForContent(nsIContent* aContent);

  
  nsMenuPopupFrame* GetPopupFrameForContent(nsIContent* aContent, PRBool aShouldFlush);

  
  nsMenuChainItem* GetTopVisibleMenu();

  
  
  
  
  void HidePopupsInList(const nsTArray<nsMenuPopupFrame *> &aFrames,
                        PRBool aDeselectMenu);

  
  
  void InitTriggerEvent(nsIDOMEvent* aEvent, nsIContent* aPopup, nsIContent** aTriggerContent);

  
  void ShowPopupCallback(nsIContent* aPopup,
                         nsMenuPopupFrame* aPopupFrame,
                         PRBool aIsContextMenu,
                         PRBool aSelectFirstItem);
  void HidePopupCallback(nsIContent* aPopup,
                         nsMenuPopupFrame* aPopupFrame,
                         nsIContent* aNextPopup,
                         nsIContent* aLastPopup,
                         nsPopupType aPopupType,
                         PRBool aDeselectMenu);

  






  void FirePopupShowingEvent(nsIContent* aPopup,
                             PRBool aIsContextMenu,
                             PRBool aSelectFirstItem);

  

















  void FirePopupHidingEvent(nsIContent* aPopup,
                            nsIContent* aNextPopup,
                            nsIContent* aLastPopup,
                            nsPresContext *aPresContext,
                            nsPopupType aPopupType,
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

  already_AddRefed<nsIDOMNode> GetLastTriggerNode(nsIDocument* aDocument, PRBool aIsTooltip);

  




  void SetCaptureState(nsIContent *aOldPopup);

  










  void UpdateKeyboardListeners();

  


  PRBool IsChildOfDocShell(nsIDocument* aDoc, nsIDocShellTreeItem* aExpected);

  
  nsCOMPtr<nsIDOMEventTarget> mKeyListener;

  
  nsCOMPtr<nsIWidget> mWidget;

  
  nsCOMPtr<nsIDOMNode> mRangeParent;
  PRInt32 mRangeOffset;
  
  
  nsIntPoint mCachedMousePoint;

  
  nsMenuBarFrame* mActiveMenuBar;

  
  nsMenuChainItem* mPopups;

  
  nsMenuChainItem* mNoHidePanels;

  
  nsCOMPtr<nsITimer> mCloseTimer;

  
  nsMenuPopupFrame* mTimerMenu;

  
  
  nsCOMPtr<nsIContent> mOpeningPopup;
};

nsresult
NS_NewXULPopupManager(nsISupports** aResult);

#endif
