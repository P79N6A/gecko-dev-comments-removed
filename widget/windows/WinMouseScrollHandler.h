





#ifndef mozilla_widget_WinMouseScrollHandler_h__
#define mozilla_widget_WinMouseScrollHandler_h__

#include "nscore.h"
#include "nsDebug.h"
#include "mozilla/Assertions.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TimeStamp.h"
#include "Units.h"
#include <windows.h>
#include "nsPoint.h"

class nsWindowBase;

namespace mozilla {
namespace widget {

class ModifierKeyState;

struct MSGResult;

class MouseScrollHandler {
public:
  static MouseScrollHandler* GetInstance();

  static void Initialize();
  static void Shutdown();

  static bool NeedsMessage(UINT aMsg);
  static bool ProcessMessage(nsWindowBase* aWidget,
                             UINT msg,
                             WPARAM wParam,
                             LPARAM lParam,
                             MSGResult& aResult);

  



  static nsresult SynthesizeNativeMouseScrollEvent(nsWindowBase* aWidget,
                                                   const LayoutDeviceIntPoint& aPoint,
                                                   uint32_t aNativeMessage,
                                                   int32_t aDelta,
                                                   uint32_t aModifierFlags,
                                                   uint32_t aAdditionalFlags);

  




  static bool IsWaitingInternalMessage()
  {
    return sInstance && sInstance->mIsWaitingInternalMessage;
  }

private:
  MouseScrollHandler();
  ~MouseScrollHandler();

  bool mIsWaitingInternalMessage;

  static MouseScrollHandler* sInstance;

  



  static void InitEvent(nsWindowBase* aWidget,
                        WidgetGUIEvent& aEvent,
                        nsIntPoint* aPoint = nullptr);

  






  static ModifierKeyState GetModifierKeyState(UINT aMessage);

  





  static POINTS GetCurrentMessagePos();

  













  void ProcessNativeMouseWheelMessage(nsWindowBase* aWidget,
                                      UINT aMessage,
                                      WPARAM aWParam,
                                      LPARAM aLParam);

  











  bool ProcessNativeScrollMessage(nsWindowBase* aWidget,
                                  UINT aMessage,
                                  WPARAM aWParam,
                                  LPARAM aLParam);

  









  void HandleMouseWheelMessage(nsWindowBase* aWidget,
                               UINT aMessage,
                               WPARAM aWParam,
                               LPARAM aLParam);

  










  void HandleScrollMessageAsMouseWheelMessage(nsWindowBase* aWidget,
                                              UINT aMessage,
                                              WPARAM aWParam,
                                              LPARAM aLParam);

  










  POINT ComputeMessagePos(UINT aMessage,
                          WPARAM aWParam,
                          LPARAM aLParam);

  class EventInfo {
  public:
    



    EventInfo(nsWindowBase* aWidget, UINT aMessage, WPARAM aWParam, LPARAM aLParam);

    bool CanDispatchWheelEvent() const;

    int32_t GetNativeDelta() const { return mDelta; }
    HWND GetWindowHandle() const { return mWnd; }
    const TimeStamp& GetTimeStamp() const { return mTimeStamp; }
    bool IsVertical() const { return mIsVertical; }
    bool IsPositive() const { return (mDelta > 0); }
    bool IsPage() const { return mIsPage; }

    


    int32_t GetScrollAmount() const;

  protected:
    EventInfo() :
      mIsVertical(false), mIsPage(false), mDelta(0), mWnd(nullptr)
    {
    }

    
    bool mIsVertical;
    
    bool mIsPage;
    
    int32_t mDelta;
    
    HWND mWnd;
    
    TimeStamp mTimeStamp;
  };

  class LastEventInfo : public EventInfo {
  public:
    LastEventInfo() :
      EventInfo(), mAccumulatedDelta(0)
    {
    }

    




    bool CanContinueTransaction(const EventInfo& aNewEvent);

    



    void ResetTransaction();

    


    void RecordEvent(const EventInfo& aEvent);

    












    bool InitWheelEvent(nsWindowBase* aWidget,
                        WidgetWheelEvent& aWheelEvent,
                        const ModifierKeyState& aModKeyState);

  private:
    static int32_t RoundDelta(double aDelta);

    int32_t mAccumulatedDelta;
  };

  LastEventInfo mLastEventInfo;

  class SystemSettings {
  public:
    SystemSettings() : mInitialized(false) {}

    void Init();
    void MarkDirty();
    void NotifyUserPrefsMayOverrideSystemSettings();

    int32_t GetScrollAmount(bool aForVertical) const
    {
      MOZ_ASSERT(mInitialized, "SystemSettings must be initialized");
      return aForVertical ? mScrollLines : mScrollChars;
    }

    bool IsPageScroll(bool aForVertical) const
    {
      MOZ_ASSERT(mInitialized, "SystemSettings must be initialized");
      return aForVertical ? (uint32_t(mScrollLines) == WHEEL_PAGESCROLL) :
                            (uint32_t(mScrollChars) == WHEEL_PAGESCROLL);
    }

  private:
    bool mInitialized;
    int32_t mScrollLines;
    int32_t mScrollChars;
  };

  SystemSettings mSystemSettings;

  class UserPrefs {
  public:
    UserPrefs();
    ~UserPrefs();

    void MarkDirty();

    bool IsScrollMessageHandledAsWheelMessage()
    {
      Init();
      return mScrollMessageHandledAsWheelMessage;
    }

    int32_t GetOverriddenVerticalScrollAmout()
    {
      Init();
      return mOverriddenVerticalScrollAmount;
    }

    int32_t GetOverriddenHorizontalScrollAmout()
    {
      Init();
      return mOverriddenHorizontalScrollAmount;
    }

    int32_t GetMouseScrollTransactionTimeout()
    {
      Init();
      return mMouseScrollTransactionTimeout;
    }

  private:
    void Init();

    static void OnChange(const char* aPrefName, void* aClosure)
    {
      static_cast<UserPrefs*>(aClosure)->MarkDirty();
    }

    bool mInitialized;
    bool mScrollMessageHandledAsWheelMessage;
    int32_t mOverriddenVerticalScrollAmount;
    int32_t mOverriddenHorizontalScrollAmount;
    int32_t mMouseScrollTransactionTimeout;
  };

  UserPrefs mUserPrefs;

  class SynthesizingEvent {
  public:
    SynthesizingEvent() :
      mWnd(nullptr), mMessage(0), mWParam(0), mLParam(0),
      mStatus(NOT_SYNTHESIZING)
    {
    }

    ~SynthesizingEvent() {}

    static bool IsSynthesizing();

    nsresult Synthesize(const POINTS& aCursorPoint, HWND aWnd,
                        UINT aMessage, WPARAM aWParam, LPARAM aLParam,
                        const BYTE (&aKeyStates)[256]);

    void NativeMessageReceived(nsWindowBase* aWidget, UINT aMessage,
                               WPARAM aWParam, LPARAM aLParam);

    void NotifyNativeMessageHandlingFinished();
    void NotifyInternalMessageHandlingFinished();

    const POINTS& GetCursorPoint() const { return mCursorPoint; }

  private:
    POINTS mCursorPoint;
    HWND mWnd;
    UINT mMessage;
    WPARAM mWParam;
    LPARAM mLParam;
    BYTE mKeyState[256];
    BYTE mOriginalKeyState[256];

    enum Status {
      NOT_SYNTHESIZING,
      SENDING_MESSAGE,
      NATIVE_MESSAGE_RECEIVED,
      INTERNAL_MESSAGE_POSTED,
    };
    Status mStatus;

#ifdef PR_LOGGING
    const char* GetStatusName()
    {
      switch (mStatus) {
        case NOT_SYNTHESIZING:
          return "NOT_SYNTHESIZING";
        case SENDING_MESSAGE:
          return "SENDING_MESSAGE";
        case NATIVE_MESSAGE_RECEIVED:
          return "NATIVE_MESSAGE_RECEIVED";
        case INTERNAL_MESSAGE_POSTED:
          return "INTERNAL_MESSAGE_POSTED";
        default:
          return "Unknown";
      }
    }
#endif

    void Finish();
  }; 

  SynthesizingEvent* mSynthesizingEvent;

public:

  class Device {
  public:
    class Elantech {
    public:
      



      static int32_t GetDriverMajorVersion();

      



      static bool IsHelperWindow(HWND aWnd);

      



      static bool HandleKeyMessage(nsWindowBase* aWidget,
                                   UINT aMsg,
                                   WPARAM aWParam);

      static void UpdateZoomUntil();
      static bool IsZooming();

      static void Init();

      static bool IsPinchHackNeeded() { return sUsePinchHack; }


    private:
      
      static bool sUseSwipeHack;
      
      static bool sUsePinchHack;
      static DWORD sZoomUntil;
    }; 

    class TrackPoint {
    public:
      



      static bool IsDriverInstalled();
    }; 

    class UltraNav {
    public:
      




      static bool IsObsoleteDriverInstalled();
    }; 

    class SetPoint {
    public:
      




      static bool IsGetMessagePosResponseValid(UINT aMessage,
                                               WPARAM aWParam,
                                               LPARAM aLParam);
    private:
      static bool sMightBeUsing;
    };

    static void Init();

    static bool IsFakeScrollableWindowNeeded()
    {
      return sFakeScrollableWindowNeeded;
    }

  private:
    









    static bool GetWorkaroundPref(const char* aPrefName,
                                  bool aValueIfAutomatic);

    static bool sFakeScrollableWindowNeeded;
  }; 
};

} 
} 

#endif 
