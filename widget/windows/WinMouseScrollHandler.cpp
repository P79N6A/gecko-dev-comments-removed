





#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "WinMouseScrollHandler.h"
#include "nsWindow.h"

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
  return false;
}


} 
} 
