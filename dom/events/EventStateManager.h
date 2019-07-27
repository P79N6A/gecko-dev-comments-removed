




#ifndef mozilla_EventStateManager_h_
#define mozilla_EventStateManager_h_

#include "mozilla/EventForwards.h"

#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/TimeStamp.h"
#include "nsIFrame.h"
#include "Units.h"

class nsFrameLoader;
class nsIContent;
class nsIDocument;
class nsIDocShell;
class nsIDocShellTreeItem;
class imgIContainer;
class EnterLeaveDispatcher;
class nsIContentViewer;
class nsIScrollableFrame;
class nsITimer;
class nsPresContext;

namespace mozilla {

class EnterLeaveDispatcher;
class EventStates;
class IMEContentObserver;
class ScrollbarsForWheel;
class WheelTransaction;

namespace dom {
class DataTransfer;
class Element;
class TabParent;
} 

class OverOutElementsWrapper final : public nsISupports
{
  ~OverOutElementsWrapper();

public:
  OverOutElementsWrapper();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(OverOutElementsWrapper)

  nsWeakFrame mLastOverFrame;

  nsCOMPtr<nsIContent> mLastOverElement;

  
  
  nsCOMPtr<nsIContent> mFirstOverEventElement;

  
  
  nsCOMPtr<nsIContent> mFirstOutEventElement;
};

class EventStateManager : public nsSupportsWeakReference,
                          public nsIObserver
{
  friend class mozilla::EnterLeaveDispatcher;
  friend class mozilla::ScrollbarsForWheel;
  friend class mozilla::WheelTransaction;

  virtual ~EventStateManager();

public:
  EventStateManager();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsresult Init();
  nsresult Shutdown();

  






  nsresult PreHandleEvent(nsPresContext* aPresContext,
                          WidgetEvent* aEvent,
                          nsIFrame* aTargetFrame,
                          nsIContent* aTargetContent,
                          nsEventStatus* aStatus);

  




  nsresult PostHandleEvent(nsPresContext* aPresContext,
                           WidgetEvent* aEvent,
                           nsIFrame* aTargetFrame,
                           nsEventStatus* aStatus);

  



  void DispatchLegacyMouseScrollEvents(nsIFrame* aTargetFrame,
                                       WidgetWheelEvent* aEvent,
                                       nsEventStatus* aStatus);

  void NotifyDestroyPresContext(nsPresContext* aPresContext);
  void SetPresContext(nsPresContext* aPresContext);
  void ClearFrameRefs(nsIFrame* aFrame);

  nsIFrame* GetEventTarget();
  already_AddRefed<nsIContent> GetEventTargetContent(WidgetEvent* aEvent);

  










  bool SetContentState(nsIContent* aContent, EventStates aState);
  void ContentRemoved(nsIDocument* aDocument, nsIContent* aContent);
  bool EventStatusOK(WidgetGUIEvent* aEvent);

  




  void OnStartToObserveContent(IMEContentObserver* aIMEContentObserver);
  void OnStopObservingContent(IMEContentObserver* aIMEContentObserver);

  






  void RegisterAccessKey(nsIContent* aContent, uint32_t aKey);

  





  void UnregisterAccessKey(nsIContent* aContent, uint32_t aKey);

  





  uint32_t GetRegisteredAccessKey(nsIContent* aContent);

  static void GetAccessKeyLabelPrefix(dom::Element* aElement, nsAString& aPrefix);

  nsresult SetCursor(int32_t aCursor, imgIContainer* aContainer,
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

  









  static bool IsHandlingUserInput();

  nsPresContext* GetPresContext() { return mPresContext; }

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(EventStateManager,
                                           nsIObserver)

  static nsIDocument* sMouseOverDocument;

  static EventStateManager* GetActiveEventStateManager() { return sActiveESM; }

  
  
  static void SetActiveManager(EventStateManager* aNewESM,
                               nsIContent* aContent);

  
  static void SetFullScreenState(dom::Element* aElement, bool aIsFullScreen);

  static bool IsRemoteTarget(nsIContent* aTarget);

  
  static bool WheelEventIsScrollAction(WidgetWheelEvent* aEvent);

  
  
  static bool WheelEventNeedsDeltaMultipliers(WidgetWheelEvent* aEvent);

  
  
  static bool CanVerticallyScrollFrameWithWheel(nsIFrame* aFrame);

  
  
  
  
  
  
  static LayoutDeviceIntPoint sLastScreenPoint;

  
  
  
  static CSSIntPoint sLastClientPoint;

  static bool sIsPointerLocked;
  static nsWeakPtr sPointerLockedElement;
  static nsWeakPtr sPointerLockedDoc;

protected:
  


  class Prefs
  {
  public:
    static bool KeyCausesActivation() { return sKeyCausesActivation; }
    static bool ClickHoldContextMenu() { return sClickHoldContextMenu; }
    static int32_t ChromeAccessModifierMask();
    static int32_t ContentAccessModifierMask();

    static void Init();
    static void OnChange(const char* aPrefName, void*);
    static void Shutdown();

  private:
    static bool sKeyCausesActivation;
    static bool sClickHoldContextMenu;
    static int32_t sGenericAccessModifierKey;
    static int32_t sChromeAccessModifierMask;
    static int32_t sContentAccessModifierMask;

    static int32_t GetAccessModifierMask(int32_t aItemType);
  };

  



  static int32_t GetAccessModifierMaskFor(nsISupports* aDocShell);

  






  void ClearCachedWidgetCursor(nsIFrame* aTargetFrame);

  void UpdateCursor(nsPresContext* aPresContext,
                    WidgetEvent* aEvent,
                    nsIFrame* aTargetFrame,
                    nsEventStatus* aStatus);
  




  nsIFrame* DispatchMouseOrPointerEvent(WidgetMouseEvent* aMouseEvent,
                                        uint32_t aMessage,
                                        nsIContent* aTargetContent,
                                        nsIContent* aRelatedContent);
  


  void GeneratePointerEnterExit(uint32_t aMessage, WidgetMouseEvent* aEvent);
  



  void GenerateMouseEnterExit(WidgetMouseEvent* aMouseEvent);
  



  void NotifyMouseOver(WidgetMouseEvent* aMouseEvent,
                       nsIContent* aContent);
  








  void NotifyMouseOut(WidgetMouseEvent* aMouseEvent,
                      nsIContent* aMovingInto);
  void GenerateDragDropEnterExit(nsPresContext* aPresContext,
                                 WidgetDragEvent* aDragEvent);

  



  OverOutElementsWrapper* GetWrapperByEventID(WidgetMouseEvent* aMouseEvent);

  







  void FireDragEnterOrExit(nsPresContext* aPresContext,
                           WidgetDragEvent* aDragEvent,
                           uint32_t aMsg,
                           nsIContent* aRelatedTarget,
                           nsIContent* aTargetContent,
                           nsWeakFrame& aTargetFrame);
  



  void UpdateDragDataTransfer(WidgetDragEvent* dragEvent);

  nsresult SetClickCount(nsPresContext* aPresContext,
                         WidgetMouseEvent* aEvent,
                         nsEventStatus* aStatus);
  nsresult CheckForAndDispatchClick(nsPresContext* aPresContext,
                                    WidgetMouseEvent* aEvent,
                                    nsEventStatus* aStatus);
  void EnsureDocument(nsPresContext* aPresContext);
  void FlushPendingEvents(nsPresContext* aPresContext);

  


  typedef enum {
    eAccessKeyProcessingNormal = 0,
    eAccessKeyProcessingUp,
    eAccessKeyProcessingDown
  } ProcessingAccessKeyState;

  


















  void HandleAccessKey(nsPresContext* aPresContext,
                       WidgetKeyboardEvent* aEvent,
                       nsEventStatus* aStatus,
                       nsIDocShellTreeItem* aBubbledFrom,
                       ProcessingAccessKeyState aAccessKeyState,
                       int32_t aModifierMask);

  bool ExecuteAccessKey(nsTArray<uint32_t>& aAccessCharCodes,
                          bool aIsTrustedEvent);

  
  
  

  nsIContent* GetFocusedContent();
  bool IsShellVisible(nsIDocShell* aShell);

  

  class WheelPrefs
  {
  public:
    static WheelPrefs* GetInstance();
    static void Shutdown();

    



    void ApplyUserPrefsToDelta(WidgetWheelEvent* aEvent);

    



    bool HasUserPrefsForDelta(WidgetWheelEvent* aEvent);

    




    void CancelApplyingUserPrefsFromOverflowDelta(WidgetWheelEvent* aEvent);

    


    enum Action : uint8_t
    {
      ACTION_NONE = 0,
      ACTION_SCROLL,
      ACTION_HISTORY,
      ACTION_ZOOM,
      ACTION_LAST = ACTION_ZOOM
    };
    Action ComputeActionFor(WidgetWheelEvent* aEvent);

    



    bool NeedToComputeLineOrPageDelta(WidgetWheelEvent* aEvent);

    



    bool IsOverOnePageScrollAllowedX(WidgetWheelEvent* aEvent);
    bool IsOverOnePageScrollAllowedY(WidgetWheelEvent* aEvent);

  private:
    WheelPrefs();
    ~WheelPrefs();

    static void OnPrefChanged(const char* aPrefName, void* aClosure);

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

    







    Index GetIndexFor(WidgetWheelEvent* aEvent);

    







    void GetBasePrefName(Index aIndex, nsACString& aBasePrefName);

    void Init(Index aIndex);

    void Reset();

    




    enum {
      MIN_MULTIPLIER_VALUE_ALLOWING_OVER_ONE_PAGE_SCROLL = 1000
    };

    bool mInit[COUNT_OF_MULTIPLIERS];
    double mMultiplierX[COUNT_OF_MULTIPLIERS];
    double mMultiplierY[COUNT_OF_MULTIPLIERS];
    double mMultiplierZ[COUNT_OF_MULTIPLIERS];
    Action mActions[COUNT_OF_MULTIPLIERS];
    




    Action mOverriddenActionsX[COUNT_OF_MULTIPLIERS];

    static WheelPrefs* sInstance;
  };

  




  enum DeltaDirection
  {
    DELTA_DIRECTION_X = 0,
    DELTA_DIRECTION_Y
  };

  struct MOZ_STACK_CLASS EventState
  {
    bool mDefaultPrevented;
    bool mDefaultPreventedByContent;

    EventState() :
      mDefaultPrevented(false), mDefaultPreventedByContent(false)
    {
    }
  };

  












  void SendLineScrollEvent(nsIFrame* aTargetFrame,
                           WidgetWheelEvent* aEvent,
                           EventState& aState,
                           int32_t aDelta,
                           DeltaDirection aDeltaDirection);

  












  void SendPixelScrollEvent(nsIFrame* aTargetFrame,
                            WidgetWheelEvent* aEvent,
                            EventState& aState,
                            int32_t aPixelDelta,
                            DeltaDirection aDeltaDirection);

  









  
  
  enum
  {
    PREFER_MOUSE_WHEEL_TRANSACTION               = 1,
    PREFER_ACTUAL_SCROLLABLE_TARGET_ALONG_X_AXIS = 2,
    PREFER_ACTUAL_SCROLLABLE_TARGET_ALONG_Y_AXIS = 4,
    START_FROM_PARENT                            = 8
  };
  enum ComputeScrollTargetOptions
  {
    
    
    COMPUTE_LEGACY_MOUSE_SCROLL_EVENT_TARGET     = 0,
    
    
    
    COMPUTE_DEFAULT_ACTION_TARGET                =
      (PREFER_MOUSE_WHEEL_TRANSACTION |
       PREFER_ACTUAL_SCROLLABLE_TARGET_ALONG_X_AXIS |
       PREFER_ACTUAL_SCROLLABLE_TARGET_ALONG_Y_AXIS),
    
    
    COMPUTE_SCROLLABLE_ANCESTOR_ALONG_X_AXIS     =
      (PREFER_ACTUAL_SCROLLABLE_TARGET_ALONG_X_AXIS | START_FROM_PARENT),
    COMPUTE_SCROLLABLE_ANCESTOR_ALONG_Y_AXIS     =
      (PREFER_ACTUAL_SCROLLABLE_TARGET_ALONG_Y_AXIS | START_FROM_PARENT)
  };
  nsIScrollableFrame* ComputeScrollTarget(nsIFrame* aTargetFrame,
                                          WidgetWheelEvent* aEvent,
                                          ComputeScrollTargetOptions aOptions);

  nsIScrollableFrame* ComputeScrollTarget(nsIFrame* aTargetFrame,
                                          double aDirectionX,
                                          double aDirectionY,
                                          WidgetWheelEvent* aEvent,
                                          ComputeScrollTargetOptions aOptions);

  











  nsSize GetScrollAmount(nsPresContext* aPresContext,
                         WidgetWheelEvent* aEvent,
                         nsIScrollableFrame* aScrollableFrame);

  


  void DoScrollText(nsIScrollableFrame* aScrollableFrame,
                    WidgetWheelEvent* aEvent);

  void DoScrollHistory(int32_t direction);
  void DoScrollZoom(nsIFrame *aTargetFrame, int32_t adjustment);
  nsresult GetContentViewer(nsIContentViewer** aCv);
  nsresult ChangeTextSize(int32_t change);
  nsresult ChangeFullZoom(int32_t change);

  





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

    bool IsInTransaction() { return mHandlingDeltaMode != UINT32_MAX; }

    




    void InitLineOrPageDelta(nsIFrame* aTargetFrame,
                             EventStateManager* aESM,
                             WidgetWheelEvent* aEvent);

    


    void Reset();

    



    nsIntPoint ComputeScrollAmountForDefaultAction(
                 WidgetWheelEvent* aEvent,
                 const nsIntSize& aScrollAmountInDevPixels);

  private:
    DeltaAccumulator() :
      mX(0.0), mY(0.0), mPendingScrollAmountX(0.0), mPendingScrollAmountY(0.0),
      mHandlingDeltaMode(UINT32_MAX), mIsNoLineOrPageDeltaDevice(false)
    {
    }

    double mX;
    double mY;

    
    
    
    double mPendingScrollAmountX;
    double mPendingScrollAmountY;

    TimeStamp mLastTime;

    uint32_t mHandlingDeltaMode;
    bool mIsNoLineOrPageDeltaDevice;

    static DeltaAccumulator* sInstance;
  };

  

  







  void DecideGestureEvent(WidgetGestureNotifyEvent* aEvent,
                          nsIFrame* targetFrame);

  
  void BeginTrackingDragGesture(nsPresContext* aPresContext,
                                WidgetMouseEvent* aDownEvent,
                                nsIFrame* aDownFrame);

  friend class mozilla::dom::TabParent;
  void BeginTrackingRemoteDragGesture(nsIContent* aContent);
  void StopTrackingDragGesture();
  void GenerateDragGesture(nsPresContext* aPresContext,
                           WidgetMouseEvent* aEvent);

  









  void DetermineDragTargetAndDefaultData(nsPIDOMWindow* aWindow,
                                         nsIContent* aSelectionTarget,
                                         dom::DataTransfer* aDataTransfer,
                                         nsISelection** aSelection,
                                         nsIContent** aTargetNode);

  









  bool DoDefaultDragStart(nsPresContext* aPresContext,
                          WidgetDragEvent* aDragEvent,
                          dom::DataTransfer* aDataTransfer,
                          nsIContent* aDragTarget,
                          nsISelection* aSelection);

  bool IsTrackingDragGesture ( ) const { return mGestureDownContent != nullptr; }
  





  void FillInEventFromGestureDown(WidgetMouseEvent* aEvent);

  nsresult DoContentCommandEvent(WidgetContentCommandEvent* aEvent);
  nsresult DoContentCommandScrollEvent(WidgetContentCommandEvent* aEvent);

  void DoQuerySelectedText(WidgetQueryContentEvent* aEvent);

  bool RemoteQueryContentEvent(WidgetEvent* aEvent);
  dom::TabParent *GetCrossProcessTarget();
  bool IsTargetCrossProcess(WidgetGUIEvent* aEvent);

  bool DispatchCrossProcessEvent(WidgetEvent* aEvent,
                                 nsFrameLoader* aRemote,
                                 nsEventStatus *aStatus);
  bool HandleCrossProcessEvent(WidgetEvent* aEvent,
                               nsEventStatus* aStatus);

  void ReleaseCurrentIMEContentObserver();

private:
  static inline void DoStateChange(dom::Element* aElement,
                                   EventStates aState, bool aAddState);
  static inline void DoStateChange(nsIContent* aContent, EventStates aState,
                                   bool aAddState);
  static void UpdateAncestorState(nsIContent* aStartNode,
                                  nsIContent* aStopBefore,
                                  EventStates aState,
                                  bool aAddState);
  static PLDHashOperator ResetLastOverForContent(const uint32_t& aIdx,
                                                 nsRefPtr<OverOutElementsWrapper>& aChunk,
                                                 void* aClosure);
  void PostHandleKeyboardEvent(WidgetKeyboardEvent* aKeyboardEvent,
                               nsEventStatus& aStatus,
                               bool dispatchedToContentProcess);

  int32_t     mLockCursor;

  
  
  
  LayoutDeviceIntPoint mPreLockPoint;

  
  
  
  
  
  static LayoutDeviceIntPoint sSynthCenteringPoint;

  nsWeakFrame mCurrentTarget;
  nsCOMPtr<nsIContent> mCurrentTargetContent;
  static nsWeakFrame sLastDragOverFrame;

  
  
  static LayoutDeviceIntPoint sLastRefPoint;

  
  LayoutDeviceIntPoint mGestureDownPoint; 
  
  nsCOMPtr<nsIContent> mGestureDownContent;
  
  
  
  nsCOMPtr<nsIContent> mGestureDownFrameOwner;
  
  Modifiers mGestureModifiers;
  uint16_t mGestureDownButtons;

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

  nsPresContext* mPresContext;      
  nsCOMPtr<nsIDocument> mDocument;   

  nsRefPtr<IMEContentObserver> mIMEContentObserver;

  uint32_t mLClickCount;
  uint32_t mMClickCount;
  uint32_t mRClickCount;

  bool m_haveShutdown;

  
  static TimeStamp sHandlingInputStart;

  nsRefPtr<OverOutElementsWrapper> mMouseEnterLeaveHelper;
  nsRefPtrHashtable<nsUint32HashKey, OverOutElementsWrapper> mPointersEnterLeaveHelper;

public:
  static nsresult UpdateUserActivityTimer(void);
  
  nsCOMArray<nsIContent> mAccessKeys;

  static int32_t sUserInputEventDepth;
  
  static bool sNormalLMouseEventInProcess;

  static EventStateManager* sActiveESM;
  
  static void ClearGlobalActiveContent(EventStateManager* aClearer);

  
  nsCOMPtr<nsITimer> mClickHoldTimer;
  void CreateClickHoldTimer(nsPresContext* aPresContext,
                            nsIFrame* aDownFrame,
                            WidgetGUIEvent* aMouseDownEvent);
  void KillClickHoldTimer();
  void FireContextClick();

  void SetPointerLock(nsIWidget* aWidget, nsIContent* aElement) ;
  static void sClickHoldCallback ( nsITimer* aTimer, void* aESM ) ;
};





class AutoHandlingUserInputStatePusher
{
public:
  AutoHandlingUserInputStatePusher(bool aIsHandlingUserInput,
                                   WidgetEvent* aEvent,
                                   nsIDocument* aDocument);
  ~AutoHandlingUserInputStatePusher();

protected:
  bool mIsHandlingUserInput;
  bool mIsMouseDown;
  bool mResetFMMouseButtonHandlingState;

  nsCOMPtr<nsIDocument> mMouseButtonEventHandlingDocument;

private:
  
  static void* operator new(size_t ) CPP_THROW_NEW { return nullptr; }
  static void operator delete(void* ) {}
};

} 



#define NS_EVENT_NEEDS_FRAME(event) \
    (!(event)->HasPluginActivationEventMessage() && \
     (event)->message != NS_MOUSE_CLICK && \
     (event)->message != NS_MOUSE_DOUBLECLICK)

#endif
