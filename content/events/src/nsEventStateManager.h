




#ifndef nsEventStateManager_h__
#define nsEventStateManager_h__

#include "mozilla/BasicEvents.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TypedEnum.h"

#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsCycleCollectionParticipant.h"
#include "nsFocusManager.h"
#include "mozilla/TimeStamp.h"
#include "nsIFrame.h"
#include "Units.h"

class nsIContent;
class nsIDocument;
class nsIDocShell;
class nsIDocShellTreeNode;
class nsIDocShellTreeItem;
class imgIContainer;
class nsDOMDataTransfer;
class MouseEnterLeaveDispatcher;
class nsIMarkupDocumentViewer;
class nsIScrollableFrame;
class nsITimer;

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
  typedef mozilla::LayoutDeviceIntPoint LayoutDeviceIntPoint;

  nsEventStateManager();
  virtual ~nsEventStateManager();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsresult Init();
  nsresult Shutdown();

  






  nsresult PreHandleEvent(nsPresContext* aPresContext,
                          mozilla::WidgetEvent* aEvent,
                          nsIFrame* aTargetFrame,
                          nsEventStatus* aStatus);

  




  nsresult PostHandleEvent(nsPresContext* aPresContext,
                           mozilla::WidgetEvent* aEvent,
                           nsIFrame* aTargetFrame,
                           nsEventStatus* aStatus);

  



  void DispatchLegacyMouseScrollEvents(nsIFrame* aTargetFrame,
                                       mozilla::WheelEvent* aEvent,
                                       nsEventStatus* aStatus);

  void NotifyDestroyPresContext(nsPresContext* aPresContext);
  void SetPresContext(nsPresContext* aPresContext);
  void ClearFrameRefs(nsIFrame* aFrame);

  nsIFrame* GetEventTarget();
  already_AddRefed<nsIContent> GetEventTargetContent(
                                 mozilla::WidgetEvent* aEvent);

  










  bool SetContentState(nsIContent *aContent, nsEventStates aState);
  void ContentRemoved(nsIDocument* aDocument, nsIContent* aContent);
  bool EventStatusOK(mozilla::WidgetGUIEvent* aEvent);

  






  void RegisterAccessKey(nsIContent* aContent, uint32_t aKey);

  





  void UnregisterAccessKey(nsIContent* aContent, uint32_t aKey);

  





  uint32_t GetRegisteredAccessKey(nsIContent* aContent);

  bool GetAccessKeyLabelPrefix(nsAString& aPrefix);

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

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsEventStateManager,
                                           nsIObserver)

  static nsIDocument* sMouseOverDocument;

  static nsEventStateManager* GetActiveEventStateManager() { return sActiveESM; }

  
  
  static void SetActiveManager(nsEventStateManager* aNewESM,
                               nsIContent* aContent);

  
  static void SetFullScreenState(mozilla::dom::Element* aElement, bool aIsFullScreen);

  static bool IsRemoteTarget(nsIContent* aTarget);
  static LayoutDeviceIntPoint GetChildProcessOffset(
                                nsFrameLoader* aFrameLoader,
                                const mozilla::WidgetEvent& aEvent);

  
  
  
  
  
  
  static nsIntPoint sLastScreenPoint;

  
  
  
  static mozilla::CSSIntPoint sLastClientPoint;

  static bool sIsPointerLocked;
  static nsWeakPtr sPointerLockedElement;
  static nsWeakPtr sPointerLockedDoc;

protected:
  friend class MouseEnterLeaveDispatcher;

  


  class Prefs
  {
  public:
    static bool KeyCausesActivation() { return sKeyCausesActivation; }
    static bool ClickHoldContextMenu() { return sClickHoldContextMenu; }
    static int32_t ChromeAccessModifierMask();
    static int32_t ContentAccessModifierMask();

    static void Init();
    static int OnChange(const char* aPrefName, void*);
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

  void UpdateCursor(nsPresContext* aPresContext,
                    mozilla::WidgetEvent* aEvent,
                    nsIFrame* aTargetFrame,
                    nsEventStatus* aStatus);
  




  nsIFrame* DispatchMouseEvent(mozilla::WidgetGUIEvent* aEvent,
                               uint32_t aMessage,
                               nsIContent* aTargetContent,
                               nsIContent* aRelatedContent);
  



  void GenerateMouseEnterExit(mozilla::WidgetGUIEvent* aEvent);
  



  void NotifyMouseOver(mozilla::WidgetGUIEvent* aEvent, nsIContent* aContent);
  








  void NotifyMouseOut(mozilla::WidgetGUIEvent* aEvent, nsIContent* aMovingInto);
  void GenerateDragDropEnterExit(nsPresContext* aPresContext,
                                 mozilla::WidgetGUIEvent* aEvent);
  







  void FireDragEnterOrExit(nsPresContext* aPresContext,
                           mozilla::WidgetGUIEvent* aEvent,
                           uint32_t aMsg,
                           nsIContent* aRelatedTarget,
                           nsIContent* aTargetContent,
                           nsWeakFrame& aTargetFrame);
  



  void UpdateDragDataTransfer(mozilla::WidgetDragEvent* dragEvent);

  nsresult SetClickCount(nsPresContext* aPresContext,
                         mozilla::WidgetMouseEvent* aEvent,
                         nsEventStatus* aStatus);
  nsresult CheckForAndDispatchClick(nsPresContext* aPresContext,
                                    mozilla::WidgetMouseEvent* aEvent,
                                    nsEventStatus* aStatus);
  void EnsureDocument(nsPresContext* aPresContext);
  void FlushPendingEvents(nsPresContext* aPresContext);

  


  typedef enum {
    eAccessKeyProcessingNormal = 0,
    eAccessKeyProcessingUp,
    eAccessKeyProcessingDown
  } ProcessingAccessKeyState;

  


















  void HandleAccessKey(nsPresContext* aPresContext,
                       mozilla::WidgetKeyboardEvent* aEvent,
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

    



    void ApplyUserPrefsToDelta(mozilla::WheelEvent* aEvent);

    




    void CancelApplyingUserPrefsFromOverflowDelta(
                                    mozilla::WheelEvent* aEvent);

    


    enum Action MOZ_ENUM_TYPE(uint8_t)
    {
      ACTION_NONE = 0,
      ACTION_SCROLL,
      ACTION_HISTORY,
      ACTION_ZOOM,
      ACTION_LAST = ACTION_ZOOM
    };
    Action ComputeActionFor(mozilla::WheelEvent* aEvent);

    



    bool NeedToComputeLineOrPageDelta(mozilla::WheelEvent* aEvent);

    



    bool IsOverOnePageScrollAllowedX(mozilla::WheelEvent* aEvent);
    bool IsOverOnePageScrollAllowedY(mozilla::WheelEvent* aEvent);

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

    







    Index GetIndexFor(mozilla::WheelEvent* aEvent);

    







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

  











  void SendLineScrollEvent(nsIFrame* aTargetFrame,
                           mozilla::WheelEvent* aEvent,
                           nsEventStatus* aStatus,
                           int32_t aDelta,
                           DeltaDirection aDeltaDirection);

  











  void SendPixelScrollEvent(nsIFrame* aTargetFrame,
                            mozilla::WheelEvent* aEvent,
                            nsEventStatus* aStatus,
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
                                          mozilla::WheelEvent* aEvent,
                                          ComputeScrollTargetOptions aOptions);

  











  nsSize GetScrollAmount(nsPresContext* aPresContext,
                         mozilla::WheelEvent* aEvent,
                         nsIScrollableFrame* aScrollableFrame);

  


  void DoScrollText(nsIScrollableFrame* aScrollableFrame,
                    mozilla::WheelEvent* aEvent);

  void DoScrollHistory(int32_t direction);
  void DoScrollZoom(nsIFrame *aTargetFrame, int32_t adjustment);
  nsresult GetMarkupDocumentViewer(nsIMarkupDocumentViewer** aMv);
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
                             nsEventStateManager* aESM,
                             mozilla::WheelEvent* aEvent);

    


    void Reset();

    



    nsIntPoint ComputeScrollAmountForDefaultAction(
                 mozilla::WheelEvent* aEvent,
                 const nsIntSize& aScrollAmountInDevPixels);

  private:
    DeltaAccumulator() :
      mX(0.0), mY(0.0), mPendingScrollAmountX(0.0), mPendingScrollAmountY(0.0),
      mHandlingDeltaMode(UINT32_MAX), mHandlingPixelOnlyDevice(false)
    {
    }

    double mX;
    double mY;

    
    
    
    double mPendingScrollAmountX;
    double mPendingScrollAmountY;

    TimeStamp mLastTime;

    uint32_t mHandlingDeltaMode;
    bool mHandlingPixelOnlyDevice;

    static DeltaAccumulator* sInstance;
  };

  

  







  void DecideGestureEvent(mozilla::WidgetGestureNotifyEvent* aEvent,
                          nsIFrame* targetFrame);

  
  void BeginTrackingDragGesture(nsPresContext* aPresContext,
                                mozilla::WidgetMouseEvent* aDownEvent,
                                nsIFrame* aDownFrame);
  void StopTrackingDragGesture();
  void GenerateDragGesture(nsPresContext* aPresContext,
                           mozilla::WidgetMouseEvent* aEvent);

  









  void DetermineDragTarget(nsPresContext* aPresContext,
                           nsIContent* aSelectionTarget,
                           nsDOMDataTransfer* aDataTransfer,
                           nsISelection** aSelection,
                           nsIContent** aTargetNode);

  









  bool DoDefaultDragStart(nsPresContext* aPresContext,
                          mozilla::WidgetDragEvent* aDragEvent,
                          nsDOMDataTransfer* aDataTransfer,
                          nsIContent* aDragTarget,
                          nsISelection* aSelection);

  bool IsTrackingDragGesture ( ) const { return mGestureDownContent != nullptr; }
  





  void FillInEventFromGestureDown(mozilla::WidgetMouseEvent* aEvent);

  nsresult DoContentCommandEvent(mozilla::WidgetContentCommandEvent* aEvent);
  nsresult DoContentCommandScrollEvent(
             mozilla::WidgetContentCommandEvent* aEvent);

  void DoQuerySelectedText(mozilla::WidgetQueryContentEvent* aEvent);

  bool RemoteQueryContentEvent(mozilla::WidgetEvent* aEvent);
  mozilla::dom::TabParent *GetCrossProcessTarget();
  bool IsTargetCrossProcess(mozilla::WidgetGUIEvent* aEvent);

  bool DispatchCrossProcessEvent(mozilla::WidgetEvent* aEvent,
                                 nsFrameLoader* aRemote,
                                 nsEventStatus *aStatus);
  bool HandleCrossProcessEvent(mozilla::WidgetEvent* aEvent,
                               nsIFrame* aTargetFrame,
                               nsEventStatus* aStatus);

private:
  static inline void DoStateChange(mozilla::dom::Element* aElement,
                                   nsEventStates aState, bool aAddState);
  static inline void DoStateChange(nsIContent* aContent, nsEventStates aState,
                                   bool aAddState);
  static void UpdateAncestorState(nsIContent* aStartNode,
                                  nsIContent* aStopBefore,
                                  nsEventStates aState,
                                  bool aAddState);

  int32_t     mLockCursor;

  
  
  
  mozilla::LayoutDeviceIntPoint mPreLockPoint;

  
  
  
  
  
  static mozilla::LayoutDeviceIntPoint sSynthCenteringPoint;

  nsWeakFrame mCurrentTarget;
  nsCOMPtr<nsIContent> mCurrentTargetContent;
  nsWeakFrame mLastMouseOverFrame;
  nsCOMPtr<nsIContent> mLastMouseOverElement;
  static nsWeakFrame sLastDragOverFrame;

  
  
  static mozilla::LayoutDeviceIntPoint sLastRefPoint;

  
  mozilla::LayoutDeviceIntPoint mGestureDownPoint; 
  
  nsCOMPtr<nsIContent> mGestureDownContent;
  
  
  
  nsCOMPtr<nsIContent> mGestureDownFrameOwner;
  
  mozilla::Modifiers mGestureModifiers;
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

  
  
  nsCOMPtr<nsIContent> mFirstMouseOverEventElement;

  
  
  nsCOMPtr<nsIContent> mFirstMouseOutEventElement;

  nsPresContext* mPresContext;      
  nsCOMPtr<nsIDocument> mDocument;   

  uint32_t mLClickCount;
  uint32_t mMClickCount;
  uint32_t mRClickCount;

  bool m_haveShutdown;

  
  static TimeStamp sHandlingInputStart;

public:
  static nsresult UpdateUserActivityTimer(void);
  
  nsCOMArray<nsIContent> mAccessKeys;

  static int32_t sUserInputEventDepth;
  
  static bool sNormalLMouseEventInProcess;

  static nsEventStateManager* sActiveESM;
  
  static void ClearGlobalActiveContent(nsEventStateManager* aClearer);

  
  nsCOMPtr<nsITimer> mClickHoldTimer;
  void CreateClickHoldTimer(nsPresContext* aPresContext,
                            nsIFrame* aDownFrame,
                            mozilla::WidgetGUIEvent* aMouseDownEvent);
  void KillClickHoldTimer();
  void FireContextClick();

  void SetPointerLock(nsIWidget* aWidget, nsIContent* aElement) ;
  static void sClickHoldCallback ( nsITimer* aTimer, void* aESM ) ;
};





class nsAutoHandlingUserInputStatePusher
{
public:
  nsAutoHandlingUserInputStatePusher(bool aIsHandlingUserInput,
                                     mozilla::WidgetEvent* aEvent,
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
        if (aDocument && aEvent->mFlags.mIsTrusted) {
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
    (!(event)->HasPluginActivationEventMessage() && \
     (event)->message != NS_MOUSE_CLICK && \
     (event)->message != NS_MOUSE_DOUBLECLICK)

#endif
