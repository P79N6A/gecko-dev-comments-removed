





#ifndef mozilla_widget_WinMouseScrollHandler_h__
#define mozilla_widget_WinMouseScrollHandler_h__

#include "nscore.h"
#include "nsDebug.h"
#include "mozilla/Assertions.h"
#include <windows.h>

class nsWindow;

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

private:
  MouseScrollHandler();
  ~MouseScrollHandler();

  static MouseScrollHandler* sInstance;

public:
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

  SystemSettings& GetSystemSettings()
  {
    return mSystemSettings;
  }

private:
  SystemSettings mSystemSettings;
};

} 
} 

#endif 