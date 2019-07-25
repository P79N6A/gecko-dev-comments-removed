





#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "WinMouseScrollHandler.h"
#include "nsWindow.h"
#include "WinUtils.h"

namespace mozilla {
namespace widget {

#ifdef PR_LOGGING
PRLogModuleInfo* gMouseScrollLog = nsnull;
#endif

MouseScrollHandler* MouseScrollHandler::sInstance = nsnull;









void
MouseScrollHandler::Initialize()
{
#ifdef PR_LOGGING
  if (!gMouseScrollLog) {
    gMouseScrollLog = PR_NewLogModule("MouseScrollHandlerWidgets");
  }
#endif
}


void
MouseScrollHandler::Shutdown()
{
  delete sInstance;
  sInstance = nsnull;
}


MouseScrollHandler*
MouseScrollHandler::GetInstance()
{
  if (!sInstance) {
    sInstance = new MouseScrollHandler();
  }
  return sInstance;
}

MouseScrollHandler::MouseScrollHandler()
{
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll: Creating an instance, this=%p, sInstance=%p",
     this, sInstance));
}

MouseScrollHandler::~MouseScrollHandler()
{
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll: Destroying an instance, this=%p, sInstance=%p",
     this, sInstance));
}


bool
MouseScrollHandler::ProcessMessage(nsWindow* aWindow, UINT msg,
                                   WPARAM wParam, LPARAM lParam,
                                   LRESULT *aRetValue, bool &aEatMessage)
{
  switch (msg) {
    case WM_SETTINGCHANGE:
      if (!sInstance) {
        return false;
      }
      if (wParam == SPI_SETWHEELSCROLLLINES ||
          wParam == SPI_SETWHEELSCROLLCHARS) {
        sInstance->mSystemSettings.MarkDirty();
      }
      return false;
    default:
      return false;
  }
}







void
MouseScrollHandler::SystemSettings::Init()
{
  if (mInitialized) {
    return;
  }

  mInitialized = true;

  if (!::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &mScrollLines, 0)) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): ::SystemParametersInfo("
         "SPI_GETWHEELSCROLLLINES) failed"));
    mScrollLines = 3;
  } else if (mScrollLines > WHEEL_DELTA) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): the result of "
         "::SystemParametersInfo(SPI_GETWHEELSCROLLLINES) is too large: %d",
       mScrollLines));
    
    
    
    
    
    
    mScrollLines = WHEEL_PAGESCROLL;
  }

  if (!::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &mScrollChars, 0)) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): ::SystemParametersInfo("
         "SPI_GETWHEELSCROLLCHARS) failed, %s",
       WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION ?
         "this is unexpected on Vista or later" :
         "but on XP or earlier, this is not a problem"));
    mScrollChars = 1;
  } else if (mScrollChars > WHEEL_DELTA) {
    PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
      ("MouseScroll::SystemSettings::Init(): the result of "
         "::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS) is too large: %d",
       mScrollChars));
    
    mScrollChars = WHEEL_PAGESCROLL;
  }

  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScroll::SystemSettings::Init(): initialized, "
       "mScrollLines=%d, mScrollChars=%d",
     mScrollLines, mScrollChars));
}

void
MouseScrollHandler::SystemSettings::MarkDirty()
{
  PR_LOG(gMouseScrollLog, PR_LOG_ALWAYS,
    ("MouseScrollHandler::SystemSettings::MarkDirty(): "
       "Marking SystemSettings dirty"));
  mInitialized = false;
}

} 
} 
