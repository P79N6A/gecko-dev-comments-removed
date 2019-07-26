




#ifndef mozilla_windowsdllblocklist_h
#define mozilla_windowsdllblocklist_h

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))

#include <windows.h>
#include "mozilla/GuardObjects.h"
#include "nscore.h"

#define HAS_DLL_BLOCKLIST

NS_IMPORT void DllBlocklist_Initialize();
NS_IMPORT void DllBlocklist_SetInXPCOMLoadOnMainThread(bool inXPCOMLoadOnMainThread);
NS_IMPORT void DllBlocklist_WriteNotes(HANDLE file);

class AutoSetXPCOMLoadOnMainThread
{
  public:
    AutoSetXPCOMLoadOnMainThread(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM) {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
      DllBlocklist_SetInXPCOMLoadOnMainThread(true);
    }

    ~AutoSetXPCOMLoadOnMainThread() {
      DllBlocklist_SetInXPCOMLoadOnMainThread(false);
    }

  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#endif
#endif
