








#ifndef nsXULPopupManager_h__
#define nsXULPopupManager_h__

#include "prlog.h"
#include "nsIContent.h"
#include "nsIRollupListener.h"
#include "nsIDOMEventListener.h"
#include "nsPoint.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsIReflowCallback.h"
#include "nsThreadUtils.h"
#include "nsStyleConsts.h"
#include "nsWidgetInitData.h"
#include "mozilla/Attributes.h"


#ifdef KeyPress
#undef KeyPress
#endif

















class nsContainerFrame;
class nsMenuFrame;
class nsMenuPopupFrame;
class nsMenuBarFrame;
class nsMenuParent;
class nsIDOMKeyEvent;
class nsIDocShellTreeItem;
class nsPIDOMWindow;



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






extern const nsNavigationDirection DirectionFromKeyCodeTable[2][6];

#define NS_DIRECTION_FROM_KEY_CODE(frame, keycode)                     \
  (DirectionFromKeyCodeTable[frame->StyleVisibility()->mDirection]  \
                            [keycode - nsIDOMKeyEvent::DOM_VK_END])




class nsMenuChainItem
{
private:
  nsMenuPopupFrame* mFrame; 
  nsPopupType mPopupType; 
  bool mIsContext; 
  bool mOnMenuBar; 
  bool mIgnoreKeys; 

  nsMenuChainItem* mParent;
  nsMenuChainItem* mChild;

public:
  nsMenuChainItem(nsMenuPopupFrame* aFrame, bool aIsContext, nsPopupType aPopupType)
    : mFrame(aFrame),
      mPopupType(aPopupType),
      mIsContext(aIsContext),
      mOnMenuBar(false),
      mIgnoreKeys(false),
      mParent(nullptr),
      mChild(nullptr)
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
  bool IsMenu() { return mPopupType == ePopupTypeMenu; }
  bool IsContextMenu() { return mIsContext; }
  bool IgnoreKeys() { return mIgnoreKeys; }
  bool IsOnMenuBar() { return mOnMenuBar; }
  void SetIgnoreKeys(bool aIgnoreKeys) { mIgnoreKeys = aIgnoreKeys; }
  void SetOnMenuBar(bool aOnMenuBar) { mOnMenuBar = aOnMenuBar; }
  nsMenuChainItem* GetParent() { return mParent; }
  nsMenuChainItem* GetChild() { return mChild; }

  
  
  void SetParent(nsMenuChainItem* aParent);

  
  
  
  
  void Detach(nsMenuChainItem** aRoot);
};


class nsXULPopupShowingEvent : public nsRunnable
{
public:
  nsXULPopupShowingEvent(nsIContent *aPopup,
                         bool aIsContextMenu,
                         bool aSelectFirstItem)
    : mPopup(aPopup),
      mIsContextMenu(aIsContextMenu),
      mSelectFirstItem(aSelectFirstItem)
  {
    NS_ASSERTION(aPopup, "null popup supplied to nsXULPopupShowingEvent constructor");
  }

  NS_IMETHOD Run() MOZ_OVERRIDE;

private:
  nsCOMPtr<nsIContent> mPopup;
  bool mIsContextMenu;
  bool mSelectFirstItem;
};


class nsXULPopupHidingEvent : public nsRunnable
{
public:
  nsXULPopupHidingEvent(nsIContent *aPopup,
                        nsIContent* aNextPopup,
                        nsIContent* aLastPopup,
                        nsPopupType aPopupType,
                        bool aDeselectMenu,
                        bool aIsCancel)
    : mPopup(aPopup),
      mNextPopup(aNextPopup),
      mLastPopup(aLastPopup),
      mPopupType(aPopupType),
      mDeselectMenu(aDeselectMenu),
      mIsRollup(aIsCancel)
  {
    NS_ASSERTION(aPopup, "null popup supplied to nsXULPopupHidingEvent constructor");
    
  }

  NS_IMETHOD Run() MOZ_OVERRIDE;

private:
  nsCOMPtr<nsIContent> mPopup;
  nsCOMPtr<nsIContent> mNextPopup;
  nsCOMPtr<nsIContent> mLastPopup;
  nsPopupType mPopupType;
  bool mDeselectMenu;
  bool mIsRollup;
};


class nsXULMenuCommandEvent : public nsRunnable
{
public:
  nsXULMenuCommandEvent(nsIContent *aMenu,
                        bool aIsTrusted,
                        bool aShift,
                        bool aControl,
                        bool aAlt,
                        bool aMeta,
                        bool aUserInput,
                        bool aFlipChecked)
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

  NS_IMETHOD Run() MOZ_OVERRIDE;

  void SetCloseMenuMode(CloseMenuMode aCloseMenuMode) { mCloseMenuMode = aCloseMenuMode; }

private:
  nsCOMPtr<nsIContent> mMenu;
  bool mIsTrusted;
  bool mShift;
  bool mControl;
  bool mAlt;
  bool mMeta;
  bool mUserInput;
  bool mFlipChecked;
  CloseMenuMode mCloseMenuMode;
};

class nsXULPopupManager MOZ_FINAL : public nsIDOMEventListener,
                                    public nsIRollupListener,
                                    public nsITimerCallback,
                                    public nsIObserver
{

public:
  friend class nsXULPopupShowingEvent;
  friend class nsXULPopupHidingEvent;
  friend class nsXULMenuCommandEvent;
  friend class TransitionEnder;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIDOMEVENTLISTENER

  
  virtual bool Rollup(uint32_t aCount, const nsIntPoint* pos, nsIContent** aLastRolledUp) MOZ_OVERRIDE;
  virtual bool ShouldRollupOnMouseWheelEvent() MOZ_OVERRIDE;
  virtual bool ShouldConsumeOnMouseWheelEvent() MOZ_OVERRIDE;
  virtual bool ShouldRollupOnMouseActivate() MOZ_OVERRIDE;
  virtual uint32_t GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain) MOZ_OVERRIDE;
  virtual void NotifyGeometryChange() MOZ_OVERRIDE {}
  virtual nsIWidget* GetRollupWidget() MOZ_OVERRIDE;

  static nsXULPopupManager* sInstance;

  
  static nsresult Init();
  static void Shutdown();

  
  
  static nsXULPopupManager* GetInstance();

  
  
  void AdjustPopupsOnWindowChange(nsPIDOMWindow* aWindow);
  void AdjustPopupsOnWindowChange(nsIPresShell* aPresShell);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static nsMenuFrame* GetPreviousMenuItem(nsContainerFrame* aParent,
                                          nsMenuFrame* aStart,
                                          bool aIsPopup);
  static nsMenuFrame* GetNextMenuItem(nsContainerFrame* aParent,
                                      nsMenuFrame* aStart,
                                      bool aIsPopup);

  
  
  
  static bool IsValidMenuItem(nsPresContext* aPresContext,
                                nsIContent* aContent,
                                bool aOnPopup);

  
  
  
  
  
  
  
  void SetActiveMenuBar(nsMenuBarFrame* aMenuBar, bool aActivate);

  
  
  
  
  
  void GetMouseLocation(nsIDOMNode** aNode, int32_t* aOffset);

  





  void ShowMenu(nsIContent *aMenu, bool aSelectFirstItem, bool aAsynchronous);

  










  void ShowPopup(nsIContent* aPopup,
                 nsIContent* aAnchorContent,
                 const nsAString& aPosition,
                 int32_t aXPos, int32_t aYPos,
                 bool aIsContextMenu,
                 bool aAttributesOverride,
                 bool aSelectFirstItem,
                 nsIDOMEvent* aTriggerEvent);

  









  void ShowPopupAtScreen(nsIContent* aPopup,
                         int32_t aXPos, int32_t aYPos,
                         bool aIsContextMenu,
                         nsIDOMEvent* aTriggerEvent);

  





  void ShowTooltipAtScreen(nsIContent* aPopup,
                           nsIContent* aTriggerContent,
                           int32_t aXPos, int32_t aYPos);

  





  void ShowPopupWithAnchorAlign(nsIContent* aPopup,
                                nsIContent* aAnchorContent,
                                nsAString& aAnchor,
                                nsAString& aAlign,
                                int32_t aXPos, int32_t aYPos,
                                bool aIsContextMenu);

  















  void HidePopup(nsIContent* aPopup,
                 bool aHideChain,
                 bool aDeselectMenu,
                 bool aAsynchronous,
                 bool aIsCancel,
                 nsIContent* aLastPopup = nullptr);

  



  void HidePopup(nsIFrame* aFrame);

  




  void HidePopupAfterDelay(nsMenuPopupFrame* aPopup);

  



  void HidePopupsInDocShell(nsIDocShellTreeItem* aDocShellToHide);

  






  void ExecuteMenu(nsIContent* aMenu, nsXULMenuCommandEvent* aEvent);

  


  bool IsPopupOpen(nsIContent* aPopup);

  


  bool IsPopupOpenForMenuParent(nsMenuParent* aMenuParent);

  




  nsIFrame* GetTopPopup(nsPopupType aType);

  



  void GetVisiblePopups(nsTArray<nsIFrame *>& aPopups);

  




  already_AddRefed<nsIDOMNode> GetLastTriggerPopupNode(nsIDocument* aDocument)
  {
    return GetLastTriggerNode(aDocument, false);
  }

  already_AddRefed<nsIDOMNode> GetLastTriggerTooltipNode(nsIDocument* aDocument)
  {
    return GetLastTriggerNode(aDocument, true);
  }

  




  bool MayShowPopup(nsMenuPopupFrame* aFrame);

  



  void PopupMoved(nsIFrame* aFrame, nsIntPoint aPoint);

  



  void PopupResized(nsIFrame* aFrame, nsIntSize ASize);

  




  void PopupDestroyed(nsMenuPopupFrame* aFrame);

  




  bool HasContextMenu(nsMenuPopupFrame* aPopup);

  





  void UpdateMenuItems(nsIContent* aPopup);

  




  void KillMenuTimer();

  







  void CancelMenuTimer(nsMenuParent* aMenuParent);

  





  bool HandleShortcutNavigation(nsIDOMKeyEvent* aKeyEvent,
                                  nsMenuPopupFrame* aFrame);

  



  bool HandleKeyboardNavigation(uint32_t aKeyCode);

  




  bool HandleKeyboardNavigationInPopup(nsMenuPopupFrame* aFrame,
                                         nsNavigationDirection aDir)
  {
    return HandleKeyboardNavigationInPopup(nullptr, aFrame, aDir);
  }

  



  bool HandleKeyboardEventWithKeyCode(nsIDOMKeyEvent* aKeyEvent,
                                      nsMenuChainItem* aTopVisibleMenuItem);

  nsresult KeyUp(nsIDOMKeyEvent* aKeyEvent);
  nsresult KeyDown(nsIDOMKeyEvent* aKeyEvent);
  nsresult KeyPress(nsIDOMKeyEvent* aKeyEvent);

protected:
  nsXULPopupManager();
  ~nsXULPopupManager();

  
  nsMenuPopupFrame* GetPopupFrameForContent(nsIContent* aContent, bool aShouldFlush);

  
  nsMenuChainItem* GetTopVisibleMenu();

  
  
  
  
  void HidePopupsInList(const nsTArray<nsMenuPopupFrame *> &aFrames,
                        bool aDeselectMenu);

  
  
  void InitTriggerEvent(nsIDOMEvent* aEvent, nsIContent* aPopup, nsIContent** aTriggerContent);

  
  void ShowPopupCallback(nsIContent* aPopup,
                         nsMenuPopupFrame* aPopupFrame,
                         bool aIsContextMenu,
                         bool aSelectFirstItem);
  void HidePopupCallback(nsIContent* aPopup,
                         nsMenuPopupFrame* aPopupFrame,
                         nsIContent* aNextPopup,
                         nsIContent* aLastPopup,
                         nsPopupType aPopupType,
                         bool aDeselectMenu);

  






  void FirePopupShowingEvent(nsIContent* aPopup,
                             bool aIsContextMenu,
                             bool aSelectFirstItem);

  


















  void FirePopupHidingEvent(nsIContent* aPopup,
                            nsIContent* aNextPopup,
                            nsIContent* aLastPopup,
                            nsPresContext *aPresContext,
                            nsPopupType aPopupType,
                            bool aDeselectMenu,
                            bool aIsCancel);

  


  bool HandleKeyboardNavigationInPopup(nsMenuChainItem* aItem,
                                         nsNavigationDirection aDir)
  {
    return HandleKeyboardNavigationInPopup(aItem, aItem->Frame(), aDir);
  }

private:
  






  bool HandleKeyboardNavigationInPopup(nsMenuChainItem* aItem,
                                         nsMenuPopupFrame* aFrame,
                                         nsNavigationDirection aDir);

protected:

  already_AddRefed<nsIDOMNode> GetLastTriggerNode(nsIDocument* aDocument, bool aIsTooltip);

  




  void SetCaptureState(nsIContent *aOldPopup);

  










  void UpdateKeyboardListeners();

  


  bool IsChildOfDocShell(nsIDocument* aDoc, nsIDocShellTreeItem* aExpected);

  
  nsCOMPtr<mozilla::dom::EventTarget> mKeyListener;

  
  nsCOMPtr<nsIWidget> mWidget;

  
  nsCOMPtr<nsIDOMNode> mRangeParent;
  int32_t mRangeOffset;
  
  
  nsIntPoint mCachedMousePoint;

  
  mozilla::Modifiers mCachedModifiers;

  
  nsMenuBarFrame* mActiveMenuBar;

  
  nsMenuChainItem* mPopups;

  
  nsMenuChainItem* mNoHidePanels;

  
  nsCOMPtr<nsITimer> mCloseTimer;

  
  nsMenuPopupFrame* mTimerMenu;

  
  
  nsCOMPtr<nsIContent> mOpeningPopup;
};

#endif
