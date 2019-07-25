





#ifndef mozilla_widget_WinMouseScrollHandler_h__
#define mozilla_widget_WinMouseScrollHandler_h__

#include "nscore.h"
#include "nsDebug.h"
#include "mozilla/Assertions.h"
#include "mozilla/TimeStamp.h"
#include <windows.h>

class nsWindow;
class nsGUIEvent;
class nsMouseScrollEvent;
struct nsModifierKeyState;

namespace mozilla {
namespace widget {

class MouseScrollHandler {
public:
  static MouseScrollHandler* GetInstance();

  static void Initialize();
  static void Shutdown();

  static bool ProcessMessage(nsWindow* aWindow,
                             UINT msg,
                             WPARAM wParam,
                             LPARAM lParam,
                             LRESULT *aRetValue,
                             bool &aEatMessage);

  










  POINT ComputeMessagePos(UINT aMessage,
                          WPARAM aWParam,
                          LPARAM aLParam);

private:
  MouseScrollHandler();
  ~MouseScrollHandler();

  static MouseScrollHandler* sInstance;

  




  static bool DispatchEvent(nsWindow* aWindow, nsGUIEvent& aEvent);

  






  static nsModifierKeyState GetModifierKeyState(UINT aMessage);

  









  void HandleMouseWheelMessage(nsWindow* aWindow,
                               UINT aMessage,
                               WPARAM aWParam,
                               LPARAM aLParam);

  










  void HandleScrollMessageAsMouseWheelMessage(nsWindow* aWindow,
                                              UINT aMessage,
                                              WPARAM aWParam,
                                              LPARAM aLParam);

  class EventInfo;
  







  struct ScrollTargetInfo {
    
    bool dispatchPixelScrollEvent;
    
    
    bool reversePixelScrollDirection;
    
    PRInt32 actualScrollAmount;
    
    
    PRInt32 actualScrollAction;
    
    PRInt32 pixelsPerUnit;
  };
  ScrollTargetInfo GetScrollTargetInfo(
                     nsWindow* aWindow,
                     const EventInfo& aEvent,
                     const nsModifierKeyState& aModiferKeyState);

  class EventInfo {
  public:
    



    EventInfo(nsWindow* aWindow, UINT aMessage, WPARAM aWParam, LPARAM aLParam);

    bool CanDispatchMouseScrollEvent() const;

    PRInt32 GetNativeDelta() const { return mDelta; }
    HWND GetWindowHandle() const { return mWnd; }
    const TimeStamp& GetTimeStamp() const { return mTimeStamp; }
    bool IsVertical() const { return mIsVertical; }
    bool IsPositive() const { return (mDelta > 0); }
    bool IsPage() const { return mIsPage; }

    


    PRInt32 GetScrollAmount() const;

    



    PRInt32 GetScrollFlags() const;

  protected:
    EventInfo() :
      mIsVertical(false), mIsPage(false), mDelta(0), mWnd(nsnull)
    {
    }

    
    bool mIsVertical;
    
    bool mIsPage;
    
    PRInt32 mDelta;
    
    HWND mWnd;
    
    TimeStamp mTimeStamp;
  };

  class LastEventInfo : public EventInfo {
  public:
    LastEventInfo() :
      EventInfo(), mRemainingDeltaForScroll(0), mRemainingDeltaForPixel(0)
    {
    }

    




    bool CanContinueTransaction(const EventInfo& aNewEvent);

    



    void ResetTransaction();

    


    void RecordEvent(const EventInfo& aEvent);

    













    bool InitMouseScrollEvent(nsWindow* aWindow,
                              nsMouseScrollEvent& aMouseScrollEvent,
                              const ScrollTargetInfo& aScrollTargetInfo,
                              const nsModifierKeyState& aModKeyState);

    













    bool InitMousePixelScrollEvent(nsWindow* aWindow,
                                   nsMouseScrollEvent& aPixelScrollEvent,
                                   const ScrollTargetInfo& aScrollTargetInfo,
                                   const nsModifierKeyState& aModKeyState);

  private:
    static PRInt32 RoundDelta(double aDelta);

    
    
    PRInt32 mRemainingDeltaForScroll;
    PRInt32 mRemainingDeltaForPixel;
  };

  LastEventInfo mLastEventInfo;

  class SystemSettings {
  public:
    SystemSettings() : mInitialized(false) {}

    void Init();
    void MarkDirty();

    PRInt32 GetScrollAmount(bool aForVertical) const
    {
      MOZ_ASSERT(mInitialized, "SystemSettings must be initialized");
      return aForVertical ? mScrollLines : mScrollChars;
    }

    bool IsPageScroll(bool aForVertical) const
    {
      MOZ_ASSERT(mInitialized, "SystemSettings must be initialized");
      return aForVertical ? (mScrollLines == WHEEL_PAGESCROLL) :
                            (mScrollChars == WHEEL_PAGESCROLL);
    }

  private:
    bool mInitialized;
    PRInt32 mScrollLines;
    PRInt32 mScrollChars;
  };

  SystemSettings mSystemSettings;

public:
  class UserPrefs {
  public:
    UserPrefs();
    ~UserPrefs();

    void MarkDirty();

    bool IsPixelScrollingEnabled()
    {
      Init();
      return mPixelScrollingEnabled;
    }

    bool IsScrollMessageHandledAsWheelMessage()
    {
      Init();
      return mScrollMessageHandledAsWheelMessage;
    }

  private:
    void Init();

    static int OnChange(const char* aPrefName, void* aClosure)
    {
      static_cast<UserPrefs*>(aClosure)->MarkDirty();
      return 0;
    }

    bool mInitialized;
    bool mPixelScrollingEnabled;
    bool mScrollMessageHandledAsWheelMessage;
  };

  UserPrefs& GetUserPrefs()
  {
    return mUserPrefs;
  }

private:
  UserPrefs mUserPrefs;

public:

  class Device {
  public:
    class Elantech {
    public:
      



      static PRInt32 GetDriverMajorVersion();

      



      static bool IsHelperWindow(HWND aWnd);

      



      static bool HandleKeyMessage(nsWindow* aWindow,
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