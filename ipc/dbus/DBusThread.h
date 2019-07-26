





#ifndef mozilla_ipc_dbus_gonk_dbusthread_h__
#define mozilla_ipc_dbus_gonk_dbusthread_h__

#include "nscore.h"

class nsIRunnable;

namespace mozilla {
namespace ipc {

class RawDBusConnection;








bool StartDBus();







bool StopDBus();







nsresult
DispatchToDBusThread(nsIRunnable* event);

RawDBusConnection*
GetDBusConnection(void);

}
}

#endif
