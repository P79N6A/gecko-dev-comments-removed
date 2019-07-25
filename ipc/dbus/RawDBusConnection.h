





#ifndef mozilla_ipc_dbus_gonk_rawdbusconnection_h__
#define mozilla_ipc_dbus_gonk_rawdbusconnection_h__

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include "nscore.h"
#include "mozilla/Scoped.h"

struct DBusConnection;

namespace mozilla {
namespace ipc {

class RawDBusConnection
{
  struct ScopedDBusConnectionPtrTraits : ScopedFreePtrTraits<DBusConnection>
  {
    static void release(DBusConnection* ptr);
  };

public:
  RawDBusConnection();
  ~RawDBusConnection();
  nsresult EstablishDBusConnection();
  Scoped<ScopedDBusConnectionPtrTraits> mConnection;
};

}
}

#endif
