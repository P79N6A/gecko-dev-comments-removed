





#ifndef mozilla_ipc_dbus_gonk_rawdbusconnection_h__
#define mozilla_ipc_dbus_gonk_rawdbusconnection_h__

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include "nscore.h"
#include "mozilla/Scoped.h"
#include <mozilla/RefPtr.h>
#include <mozilla/Mutex.h>

struct DBusConnection;

namespace mozilla {
namespace ipc {

class RawDBusConnection : public AtomicRefCounted<RawDBusConnection>
{
  struct ScopedDBusConnectionPtrTraits : ScopedFreePtrTraits<DBusConnection>
  {
    static void release(DBusConnection* ptr);
  };

public:
  RawDBusConnection();
  virtual ~RawDBusConnection();
  nsresult EstablishDBusConnection();
  DBusConnection* GetConnection() {
    return mConnection;
  }

protected:
  Scoped<ScopedDBusConnectionPtrTraits> mConnection;

private:
  static bool sDBusIsInit;
};

}
}

#endif
