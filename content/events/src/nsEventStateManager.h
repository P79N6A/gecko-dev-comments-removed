





































#ifndef nsEventStateManager_h__
#define nsEventStateManager_h__

#include "nsIEventStateManager.h"
#include "nsGUIEvent.h"
#include "nsIContent.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsHashtable.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsCOMArray.h"
#include "nsIFrame.h"
#include "nsCycleCollectionParticipant.h"

class nsIScrollableView;
class nsIPresShell;
class nsIDocShell;
class nsIDocShellTreeNode;
class nsIDocShellTreeItem;
class nsIFocusController;
class imgIContainer;


#if defined(XP_MAC) || defined(XP_MACOSX)
#define CLICK_HOLD_CONTEXT_MENUS 1
#endif






class nsEventStateManager : public nsSupportsWeakReference,
                            public nsIEventStateManager,
                            public nsIObserver
{
public:
  nsEventStateManager();
  virtual ~nsEventStateManager();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIOBSERVER

  NS_IMETHOD Init();
  nsresult Shutdown();

  






  NS_IMETHOD PreHandleEvent(nsPresContext* aPresContext,
                         nsEvent *aEvent,
                         nsIFrame* aTargetFrame,
                         nsEventStatus* aStatus,
                         nsIView* aView);

  




  NS_IMETHOD PostHandleEvent(nsPresContext* aPresContext,
                             nsEvent *aEvent,
                             nsIFrame* aTargetFrame,
                             nsEventStatus* aStatus,
                             nsIView* aView);

  NS_IMETHOD NotifyDestroyPresContext(nsPresContext* aPresContext);
  NS_IMETHOD SetPresContext(nsPresContext* aPresContext);
  NS_IMETHOD ClearFrameRefs(nsIFrame* aFrame);

  NS_IMETHOD GetEventTarget(nsIFrame **aFrame);
  NS_IMETHOD GetEventTargetContent(nsEvent* aEvent, nsIContent** aContent);

  NS_IMETHOD GetContentState(nsIContent *aContent, PRInt32& aState);
  virtual PRBool SetContentState(nsIContent *aContent, PRInt32 aState);
  NS_IMETHOD GetFocusedContent(nsIContent **aContent);
  NS_IMETHOD SetFocusedContent(nsIContent* aContent);
  NS_IMETHOD GetLastFocusedContent(nsIContent **aContent);
  NS_IMETHOD GetFocusedFrame(nsIFrame **aFrame);
  NS_IMETHOD ContentRemoved(nsIContent* aContent);
  NS_IMETHOD EventStatusOK(nsGUIEvent* aEvent, PRBool *aOK);

  
  NS_IMETHOD RegisterAccessKey(nsIContent* aContent, PRUint32 aKey);
  NS_IMETHOD UnregisterAccessKey(nsIContent* aContent, PRUint32 aKey);

  NS_IMETHOD SetCursor(PRInt32 aCursor, imgIContainer* aContainer,
                       PRBool aHaveHotspot, float aHotspotX, float aHotspotY,
                       nsIWidget* aWidget, PRBool aLockCursor);

  NS_IMETHOD ShiftFocus(PRBool aForward, nsIContent* aStart=nsnull);

  virtual PRBool GetBrowseWithCaret();
  void ResetBrowseWithCaret();

  NS_IMETHOD MoveFocusToCaret(PRBool aCanFocusDoc, PRBool *aIsSelectionWithFocus);
  NS_IMETHOD MoveCaretToFocus();
  NS_IMETHOD ChangeFocusWith(nsIContent* aFocus, EFocusedWithType aFocusedWith);

  static void StartHandlingUserInput()
  {
    ++sUserInputEventDepth;
  }

  static void StopHandlingUserInput()
  {
    --sUserInputEventDepth;
  }

  static PRBool IsHandlingUserInput()
  {
    return sUserInputEventDepth > 0;
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsEventStateManager,
                                           nsIEventStateManager)

protected:
  





  void EnsureFocusSynchronization();

  void UpdateCursor(nsPresContext* aPresContext, nsEvent* aEvent, nsIFrame* aTargetFrame, nsEventStatus* aStatus);
  




  nsIFrame* DispatchMouseEvent(nsGUIEvent* aEvent, PRUint32 aMessage,
                               nsIContent* aTargetContent,
                               nsIContent* aRelatedContent);
  



  void GenerateMouseEnterExit(nsGUIEvent* aEvent);
  



  void NotifyMouseOver(nsGUIEvent* aEvent, nsIContent* aContent);
  








  void NotifyMouseOut(nsGUIEvent* aEvent, nsIContent* aMovingInto);
  void GenerateDragDropEnterExit(nsPresContext* aPresContext, nsGUIEvent* aEvent);
  







  void FireDragEnterOrExit(nsPresContext* aPresContext,
                           nsGUIEvent* aEvent,
                           PRUint32 aMsg,
                           nsIContent* aRelatedTarget,
                           nsIContent* aTargetContent,
                           nsWeakFrame& aTargetFrame);
  nsresult SetClickCount(nsPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);
  nsresult CheckForAndDispatchClick(nsPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);
  nsresult GetNextTabbableContent(nsIContent* aRootContent,
                                  nsIContent* aStartContent,
                                  nsIFrame* aStartFrame,
                                  PRBool forward, PRBool ignoreTabIndex,
                                  nsIContent** aResultNode,
                                  nsIFrame** aResultFrame);
  nsIContent *GetNextTabbableMapArea(PRBool aForward, nsIContent *imageContent);

  PRInt32 GetNextTabIndex(nsIContent* aParent, PRBool foward);
  nsresult SendFocusBlur(nsPresContext* aPresContext, nsIContent *aContent, PRBool aEnsureWindowHasFocus);
  void EnsureDocument(nsIPresShell* aPresShell);
  void EnsureDocument(nsPresContext* aPresContext);
  void FlushPendingEvents(nsPresContext* aPresContext);
  nsIFocusController* GetFocusControllerForDocument(nsIDocument* aDocument);

  


  typedef enum {
    eAccessKeyProcessingNormal = 0,
    eAccessKeyProcessingUp,
    eAccessKeyProcessingDown
  } ProcessingAccessKeyState;

  


















  void HandleAccessKey(nsPresContext* aPresContext,
                       nsKeyEvent* aEvent,
                       nsEventStatus* aStatus,
                       nsIDocShellTreeItem* aBubbledFrom,
                       ProcessingAccessKeyState aAccessKeyState,
                       PRInt32 aModifierMask);

  
  
  

  nsresult ShiftFocusInternal(PRBool aForward, nsIContent* aStart = nsnull);
  void TabIntoDocument(nsIDocShell* aDocShell, PRBool aForward);
  void ShiftFocusByDoc(PRBool forward);
  PRBool IsFrameSetDoc(nsIDocShell* aDocShell);
  PRBool IsIFrameDoc(nsIDocShell* aDocShell);
  PRBool IsShellVisible(nsIDocShell* aShell);
  void GetLastChildDocShell(nsIDocShellTreeItem* aItem,
                            nsIDocShellTreeItem** aResult);
  void GetNextDocShell(nsIDocShellTreeNode* aNode,
                       nsIDocShellTreeItem** aResult);
  void GetPrevDocShell(nsIDocShellTreeNode* aNode,
                       nsIDocShellTreeItem** aResult);

  
  nsresult GetParentScrollingView(nsInputEvent* aEvent,
                                  nsPresContext* aPresContext,
                                  nsIFrame* &targetOuterFrame,
                                  nsPresContext* &presCtxOuter);

  typedef enum {
    eScrollByPixel,
    eScrollByLine,
    eScrollByPage
  } ScrollQuantity;
  nsresult DoScrollText(nsPresContext* aPresContext,
                        nsIFrame* aTargetFrame,
                        nsInputEvent* aEvent,
                        PRInt32 aNumLines,
                        PRBool aScrollHorizontal,
                        ScrollQuantity aScrollQuantity);
  void ForceViewUpdate(nsIView* aView);
  void DoScrollHistory(PRInt32 direction);
  void DoScrollTextsize(nsIFrame *aTargetFrame, PRInt32 adjustment);
  nsresult ChangeTextSize(PRInt32 change);
  

  
  void BeginTrackingDragGesture ( nsPresContext* aPresContext, nsMouseEvent* inDownEvent,
                                  nsIFrame* inDownFrame ) ;
  void StopTrackingDragGesture ( ) ;
  void GenerateDragGesture ( nsPresContext* aPresContext, nsMouseEvent *aEvent ) ;
  PRBool IsTrackingDragGesture ( ) const { return mGestureDownContent != nsnull; }
  





  void FillInEventFromGestureDown(nsMouseEvent* aEvent);

  PRBool mSuppressFocusChange; 

  nsresult SetCaretEnabled(nsIPresShell *aPresShell, PRBool aVisibility);
  nsresult SetContentCaretVisible(nsIPresShell* aPresShell, nsIContent *aContent, PRBool aVisible);
  void FocusElementButNotDocument(nsIContent *aElement);

  
  nsresult GetDocSelectionLocation(nsIContent **start, nsIContent **end, 
                                   nsIFrame **startFrame, PRUint32 *startOffset);

  PRInt32     mLockCursor;

  nsWeakFrame mCurrentTarget;
  nsCOMPtr<nsIContent> mCurrentTargetContent;
  nsWeakFrame mLastMouseOverFrame;
  nsCOMPtr<nsIContent> mLastMouseOverElement;
  nsWeakFrame mLastDragOverFrame;

  
  nsPoint mGestureDownPoint; 
  
  nsCOMPtr<nsIContent> mGestureDownContent;
  
  
  
  nsCOMPtr<nsIContent> mGestureDownFrameOwner;
  
  PRPackedBool mGestureDownShift;
  PRPackedBool mGestureDownControl;
  PRPackedBool mGestureDownAlt;
  PRPackedBool mGestureDownMeta;

  nsCOMPtr<nsIContent> mLastLeftMouseDownContent;
  nsCOMPtr<nsIContent> mLastMiddleMouseDownContent;
  nsCOMPtr<nsIContent> mLastRightMouseDownContent;

  nsCOMPtr<nsIContent> mActiveContent;
  nsCOMPtr<nsIContent> mHoverContent;
  nsCOMPtr<nsIContent> mDragOverContent;
  nsCOMPtr<nsIContent> mURLTargetContent;
  nsCOMPtr<nsIContent> mCurrentFocus;
  nsCOMPtr<nsIContent> mLastFocus;
  nsWeakFrame mCurrentFocusFrame;
  PRInt32 mCurrentTabIndex;
  EFocusedWithType mLastFocusedWith;

  
  nsCOMPtr<nsIContent> mLastContentFocus;

  

  nsCOMPtr<nsIContent> mFirstBlurEvent;
  nsCOMPtr<nsIContent> mFirstFocusEvent;

  
  
  nsCOMPtr<nsIContent> mFirstMouseOverEventElement;

  
  
  nsCOMPtr<nsIContent> mFirstMouseOutEventElement;

  nsPresContext* mPresContext;      
  nsCOMPtr<nsIDocument> mDocument;   

  PRUint32 mLClickCount;
  PRUint32 mMClickCount;
  PRUint32 mRClickCount;

  PRPackedBool mNormalLMouseEventInProcess;

  PRPackedBool m_haveShutdown;

  
  PRPackedBool mBrowseWithCaret;

  
  PRPackedBool mTabbedThroughDocument;

  
  nsSupportsHashtable *mAccessKeys;

  nsCOMArray<nsIDocShell> mTabbingFromDocShells;

#ifdef CLICK_HOLD_CONTEXT_MENUS
  enum { kClickHoldDelay = 500 } ;        

  void CreateClickHoldTimer ( nsPresContext* aPresContext, nsIFrame* inDownFrame,
                              nsGUIEvent* inMouseDownEvent ) ;
  void KillClickHoldTimer ( ) ;
  void FireContextClick ( ) ;
  static void sClickHoldCallback ( nsITimer* aTimer, void* aESM ) ;
  
  nsCOMPtr<nsITimer> mClickHoldTimer;
#endif

  static PRInt32 sUserInputEventDepth;
};


class nsAutoHandlingUserInputStatePusher
{
public:
  nsAutoHandlingUserInputStatePusher(PRBool aIsHandlingUserInput)
    : mIsHandlingUserInput(aIsHandlingUserInput)
  {
    if (aIsHandlingUserInput) {
      nsEventStateManager::StartHandlingUserInput();
    }
  }

  ~nsAutoHandlingUserInputStatePusher()
  {
    if (mIsHandlingUserInput) {
      nsEventStateManager::StopHandlingUserInput();
    }
  }

protected:
  PRBool mIsHandlingUserInput;

private:
  
  static void* operator new(size_t ) CPP_THROW_NEW { return nsnull; }
  static void operator delete(void* ) {}
};

#endif 
