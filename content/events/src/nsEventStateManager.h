




#ifndef nsEventStateManager_h__
#define nsEventStateManager_h__

#include "nsEvent.h"
#include "nsGUIEvent.h"
#include "nsIContent.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsHashtable.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsCOMArray.h"
#include "nsIFrameLoader.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIScrollableFrame.h"
#include "nsFocusManager.h"
#include "nsIDocument.h"
#include "nsEventStates.h"
#include "mozilla/TimeStamp.h"
#include "nsContentUtils.h"
#include "nsIFrame.h"

class nsIPresShell;
class nsIDocShell;
class nsIDocShellTreeNode;
class nsIDocShellTreeItem;
class imgIContainer;
class nsDOMDataTransfer;
class MouseEnterLeaveDispatcher;
class nsIFrame;

namespace mozilla {
namespace dom {
class TabParent;
}
}





class nsEventStateManager : public nsSupportsWeakReference,
                            public nsIObserver
{
  friend class nsMouseWheelTransaction;
public:

  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  nsEventStateManager();
  virtual ~nsEventStateManager();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsresult Init();
  nsresult Shutdown();

  






  nsresult PreHandleEvent(nsPresContext* aPresContext,
                          nsEvent *aEvent,
                          nsIFrame* aTargetFrame,
                          nsEventStatus* aStatus);

  




  nsresult PostHandleEvent(nsPresContext* aPresContext,
                           nsEvent *aEvent,
                           nsIFrame* aTargetFrame,
                           nsEventStatus* aStatus);

  



  void DispatchLegacyMouseScrollEvents(nsIFrame* aTargetFrame,
                                       mozilla::widget::WheelEvent* aEvent,
                                       nsEventStatus* aStatus);

  void NotifyDestroyPresContext(nsPresContext* aPresContext);
  void SetPresContext(nsPresContext* aPresContext);
  void ClearFrameRefs(nsIFrame* aFrame);

  nsIFrame* GetEventTarget();
  already_AddRefed<nsIContent> GetEventTargetContent(nsEvent* aEvent);

  










  bool SetContentState(nsIContent *aContent, nsEventStates aState);
  void ContentRemoved(nsIDocument* aDocument, nsIContent* aContent);
  bool EventStatusOK(nsGUIEvent* aEvent);

  






  void RegisterAccessKey(nsIContent* aContent, PRUint32 aKey);

  





  void UnregisterAccessKey(nsIContent* aContent, PRUint32 aKey);

  





  PRUint32 GetRegisteredAccessKey(nsIContent* aContent);

  bool GetAccessKeyLabelPrefix(nsAString& aPrefix);

  nsresult SetCursor(PRInt32 aCursor, imgIContainer* aContainer,
                     bool aHaveHotspot, float aHotspotX, float aHotspotY,
                     nsIWidget* aWidget, bool aLockCursor); 

  static void StartHandlingUserInput()
  {
    ++sUserInputEventDepth;
    if (sUserInputEventDepth == 1) {
      sHandlingInputStart = TimeStamp::Now();
    }
  }

  static void StopHandlingUserInput()
  {
    --sUserInputEventDepth;
    if (sUserInputEventDepth == 0) {
      sHandlingInputStart = TimeStamp();
    }
  }

  static bool IsHandlingUserInput()
  {
    if (sUserInputEventDepth <= 0) {
      return false;
    }
    TimeDuration timeout = nsContentUtils::HandlingUserInputTimeout();
    return timeout <= TimeDuration(0) ||
           (TimeStamp::Now() - sHandlingInputStart) <= timeout;
  }

  









  NS_IMETHOD_(bool) IsHandlingUserInputExternal() { return IsHandlingUserInput(); }
  
  nsPresContext* GetPresContext() { return mPresContext; }

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsEventStateManager,
                                           nsIObserver)

  static nsIDocument* sMouseOverDocument;

  static nsEventStateManager* GetActiveEventStateManager() { return sActiveESM; }

  
  
  static void SetActiveManager(nsEventStateManager* aNewESM,
                               nsIContent* aContent);

  
  static void SetFullScreenState(mozilla::dom::Element* aElement, bool aIsFullScreen);

  static bool IsRemoteTarget(nsIContent* aTarget);

  
  
  
  
  
  
  static nsIntPoint sLastScreenPoint;

  
  
  
  static nsIntPoint sLastClientPoint;

  static bool sIsPointerLocked;
  static nsWeakPtr sPointerLockedElement;
  static nsWeakPtr sPointerLockedDoc;

protected:
  friend class MouseEnterLeaveDispatcher;

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
  



  void UpdateDragDataTransfer(nsDragEvent* dragEvent);

  nsresult SetClickCount(nsPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);
  nsresult CheckForAndDispatchClick(nsPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);
  void EnsureDocument(nsPresContext* aPresContext);
  void FlushPendingEvents(nsPresContext* aPresContext);

  


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

  bool ExecuteAccessKey(nsTArray<PRUint32>& aAccessCharCodes,
                          bool aIsTrustedEvent);

  
  
  

  nsIContent* GetFocusedContent();
  bool IsShellVisible(nsIDocShell* aShell);

  

  class WheelPrefs
  {
  public:
    static WheelPrefs* GetInstance();
    static void Shutdown();

    



    void ApplyUserPrefsToDelta(mozilla::widget::WheelEvent* aEvent);

    




    void CancelApplyingUserPrefsFromOverflowDelta(
                                    mozilla::widget::WheelEvent* aEvent);

    


    enum Action
    {
      ACTION_NONE = 0,
      ACTION_SCROLL,
      ACTION_HISTORY,
      ACTION_ZOOM,
      ACTION_LAST = ACTION_ZOOM
    };
    Action ComputeActionFor(mozilla::widget::WheelEvent* aEvent);

    



    bool NeedToComputeLineOrPageDelta(mozilla::widget::WheelEvent* aEvent);

  private:
    WheelPrefs();
    ~WheelPrefs();

    static int OnPrefChanged(const char* aPrefName, void* aClosure);

    enum Index
    {
      INDEX_DEFAULT = 0,
      INDEX_ALT,
      INDEX_CONTROL,
      INDEX_META,
      INDEX_SHIFT,
      INDEX_OS,
      COUNT_OF_MULTIPLIERS
    };

    







    Index GetIndexFor(mozilla::widget::WheelEvent* aEvent);

    







    void GetBasePrefName(Index aIndex, nsACString& aBasePrefName);

    void Init(Index aIndex);

    void Reset();

    bool mInit[COUNT_OF_MULTIPLIERS];
    double mMultiplierX[COUNT_OF_MULTIPLIERS];
    double mMultiplierY[COUNT_OF_MULTIPLIERS];
    double mMultiplierZ[COUNT_OF_MULTIPLIERS];
    Action mActions[COUNT_OF_MULTIPLIERS];

    static WheelPrefs* sInstance;
  };

  




  enum DeltaDirection
  {
    DELTA_DIRECTION_X = 0,
    DELTA_DIRECTION_Y
  };

  











  void SendLineScrollEvent(nsIFrame* aTargetFrame,
                           mozilla::widget::WheelEvent* aEvent,
                           nsEventStatus* aStatus,
                           PRInt32 aDelta,
                           DeltaDirection aDeltaDirection);

  











  void SendPixelScrollEvent(nsIFrame* aTargetFrame,
                            mozilla::widget::WheelEvent* aEvent,
                            nsEventStatus* aStatus,
                            PRInt32 aPixelDelta,
                            DeltaDirection aDeltaDirection);

  











  nsIScrollableFrame* ComputeScrollTarget(nsIFrame* aTargetFrame,
                                          mozilla::widget::WheelEvent* aEvent,
                                          bool aForDefaultAction);

  











  nsSize GetScrollAmount(nsPresContext* aPresContext,
                         mozilla::widget::WheelEvent* aEvent,
                         nsIScrollableFrame* aScrollableFrame);

  


  void DoScrollText(nsIScrollableFrame* aScrollableFrame,
                    mozilla::widget::WheelEvent* aEvent);

  void DoScrollHistory(PRInt32 direction);
  void DoScrollZoom(nsIFrame *aTargetFrame, PRInt32 adjustment);
  nsresult GetMarkupDocumentViewer(nsIMarkupDocumentViewer** aMv);
  nsresult ChangeTextSize(PRInt32 change);
  nsresult ChangeFullZoom(PRInt32 change);

  





  class DeltaAccumulator
  {
  public:
    static DeltaAccumulator* GetInstance()
    {
      if (!sInstance) {
        sInstance = new DeltaAccumulator;
      }
      return sInstance;
    }

    static void Shutdown()
    {
      delete sInstance;
      sInstance = nullptr;
    }

    bool IsInTransaction() { return mHandlingDeltaMode != PR_UINT32_MAX; }

    




    void InitLineOrPageDelta(nsIFrame* aTargetFrame,
                             nsEventStateManager* aESM,
                             mozilla::widget::WheelEvent* aEvent);

    


    void Reset();

    



    nsIntPoint ComputeScrollAmountForDefaultAction(
                 mozilla::widget::WheelEvent* aEvent,
                 const nsIntSize& aScrollAmountInDevPixels);

  private:
    DeltaAccumulator() :
      mX(0.0), mY(0.0), mPendingScrollAmountX(0.0), mPendingScrollAmountY(0.0),
      mHandlingDeltaMode(PR_UINT32_MAX), mHandlingPixelOnlyDevice(false)
    {
    }

    double mX;
    double mY;

    
    
    
    double mPendingScrollAmountX;
    double mPendingScrollAmountY;

    TimeStamp mLastTime;

    PRUint32 mHandlingDeltaMode;
    bool mHandlingPixelOnlyDevice;

    static DeltaAccumulator* sInstance;
  };

  

  







  void DecideGestureEvent(nsGestureNotifyEvent* aEvent, nsIFrame* targetFrame);

  
  void BeginTrackingDragGesture ( nsPresContext* aPresContext, nsMouseEvent* inDownEvent,
                                  nsIFrame* inDownFrame ) ;
  void StopTrackingDragGesture ( ) ;
  void GenerateDragGesture ( nsPresContext* aPresContext, nsMouseEvent *aEvent ) ;

  









  void DetermineDragTarget(nsPresContext* aPresContext,
                           nsIContent* aSelectionTarget,
                           nsDOMDataTransfer* aDataTransfer,
                           nsISelection** aSelection,
                           nsIContent** aTargetNode);

  









  bool DoDefaultDragStart(nsPresContext* aPresContext,
                            nsDragEvent* aDragEvent,
                            nsDOMDataTransfer* aDataTransfer,
                            nsIContent* aDragTarget,
                            nsISelection* aSelection);

  bool IsTrackingDragGesture ( ) const { return mGestureDownContent != nullptr; }
  





  void FillInEventFromGestureDown(nsMouseEvent* aEvent);

  nsresult DoContentCommandEvent(nsContentCommandEvent* aEvent);
  nsresult DoContentCommandScrollEvent(nsContentCommandEvent* aEvent);

  void DoQuerySelectedText(nsQueryContentEvent* aEvent);

  bool RemoteQueryContentEvent(nsEvent *aEvent);
  mozilla::dom::TabParent *GetCrossProcessTarget();
  bool IsTargetCrossProcess(nsGUIEvent *aEvent);

  bool DispatchCrossProcessEvent(nsEvent* aEvent, nsFrameLoader* remote,
                                 nsEventStatus *aStatus);
  bool HandleCrossProcessEvent(nsEvent *aEvent,
                                 nsIFrame* aTargetFrame,
                                 nsEventStatus *aStatus);

private:
  static inline void DoStateChange(mozilla::dom::Element* aElement,
                                   nsEventStates aState, bool aAddState);
  static inline void DoStateChange(nsIContent* aContent, nsEventStates aState,
                                   bool aAddState);
  static void UpdateAncestorState(nsIContent* aStartNode,
                                  nsIContent* aStopBefore,
                                  nsEventStates aState,
                                  bool aAddState);

  PRInt32     mLockCursor;

  
  
  
  nsIntPoint  mPreLockPoint;

  nsWeakFrame mCurrentTarget;
  nsCOMPtr<nsIContent> mCurrentTargetContent;
  nsWeakFrame mLastMouseOverFrame;
  nsCOMPtr<nsIContent> mLastMouseOverElement;
  static nsWeakFrame sLastDragOverFrame;

  
  
  static nsIntPoint sLastRefPoint;

  
  nsIntPoint mGestureDownPoint; 
  
  nsCOMPtr<nsIContent> mGestureDownContent;
  
  
  
  nsCOMPtr<nsIContent> mGestureDownFrameOwner;
  
  mozilla::widget::Modifiers mGestureModifiers;
  PRUint16 mGestureDownButtons;

  nsCOMPtr<nsIContent> mLastLeftMouseDownContent;
  nsCOMPtr<nsIContent> mLastLeftMouseDownContentParent;
  nsCOMPtr<nsIContent> mLastMiddleMouseDownContent;
  nsCOMPtr<nsIContent> mLastMiddleMouseDownContentParent;
  nsCOMPtr<nsIContent> mLastRightMouseDownContent;
  nsCOMPtr<nsIContent> mLastRightMouseDownContentParent;

  nsCOMPtr<nsIContent> mActiveContent;
  nsCOMPtr<nsIContent> mHoverContent;
  static nsCOMPtr<nsIContent> sDragOverContent;
  nsCOMPtr<nsIContent> mURLTargetContent;

  
  
  nsCOMPtr<nsIContent> mFirstMouseOverEventElement;

  
  
  nsCOMPtr<nsIContent> mFirstMouseOutEventElement;

  nsPresContext* mPresContext;      
  nsCOMPtr<nsIDocument> mDocument;   

  PRUint32 mLClickCount;
  PRUint32 mMClickCount;
  PRUint32 mRClickCount;

  bool m_haveShutdown;

  
  static TimeStamp sHandlingInputStart;

public:
  static nsresult UpdateUserActivityTimer(void);
  
  nsCOMArray<nsIContent> mAccessKeys;

  static PRInt32 sUserInputEventDepth;
  
  static bool sNormalLMouseEventInProcess;

  static nsEventStateManager* sActiveESM;
  
  static void ClearGlobalActiveContent(nsEventStateManager* aClearer);

  
  bool mClickHoldContextMenu;
  nsCOMPtr<nsITimer> mClickHoldTimer;
  void CreateClickHoldTimer ( nsPresContext* aPresContext, nsIFrame* inDownFrame,
                              nsGUIEvent* inMouseDownEvent ) ;
  void KillClickHoldTimer ( ) ;
  void FireContextClick ( ) ;

  void SetPointerLock(nsIWidget* aWidget, nsIContent* aElement) ;
  static void sClickHoldCallback ( nsITimer* aTimer, void* aESM ) ;
};





class nsAutoHandlingUserInputStatePusher
{
public:
  nsAutoHandlingUserInputStatePusher(bool aIsHandlingUserInput,
                                     nsEvent* aEvent,
                                     nsIDocument* aDocument)
    : mIsHandlingUserInput(aIsHandlingUserInput),
      mIsMouseDown(aEvent && aEvent->message == NS_MOUSE_BUTTON_DOWN),
      mResetFMMouseDownState(false)
  {
    if (aIsHandlingUserInput) {
      nsEventStateManager::StartHandlingUserInput();
      if (mIsMouseDown) {
        nsIPresShell::SetCapturingContent(nullptr, 0);
        nsIPresShell::AllowMouseCapture(true);
        if (aDocument && NS_IS_TRUSTED_EVENT(aEvent)) {
          nsFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            fm->SetMouseButtonDownHandlingDocument(aDocument);
            mResetFMMouseDownState = true;
          }
        }
      }
    }
  }

  ~nsAutoHandlingUserInputStatePusher()
  {
    if (mIsHandlingUserInput) {
      nsEventStateManager::StopHandlingUserInput();
      if (mIsMouseDown) {
        nsIPresShell::AllowMouseCapture(false);
        if (mResetFMMouseDownState) {
          nsFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            fm->SetMouseButtonDownHandlingDocument(nullptr);
          }
        }
      }
    }
  }

protected:
  bool mIsHandlingUserInput;
  bool mIsMouseDown;
  bool mResetFMMouseDownState;

private:
  
  static void* operator new(size_t ) CPP_THROW_NEW { return nullptr; }
  static void operator delete(void* ) {}
};



#define NS_EVENT_NEEDS_FRAME(event) \
    (!NS_IS_ACTIVATION_EVENT(event) && (event)->message != NS_MOUSE_CLICK && \
     (event)->message != NS_MOUSE_DOUBLECLICK)

#endif 
