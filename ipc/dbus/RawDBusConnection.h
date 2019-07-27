





#ifndef mozilla_ipc_dbus_gonk_rawdbusconnection_h__
#define mozilla_ipc_dbus_gonk_rawdbusconnection_h__

#include "mozilla/Scoped.h"

struct DBusConnection;
struct DBusMessage;

namespace mozilla {
namespace ipc {

typedef void (*DBusReplyCallback)(DBusMessage*, void*);

class RawDBusConnection
{
  struct ScopedDBusConnectionPtrTraits : ScopedFreePtrTraits<DBusConnection>
  {
    static void release(DBusConnection* ptr);
  };

public:
  RawDBusConnection();
  virtual ~RawDBusConnection();

  nsresult EstablishDBusConnection();

  bool Watch();

  DBusConnection* GetConnection()
  {
    return mConnection;
  }

  bool Send(DBusMessage* aMessage);

  bool SendWithReply(DBusReplyCallback aCallback, void* aData,
                     int aTimeout, DBusMessage* aMessage);

  bool SendWithReply(DBusReplyCallback aCallback, void* aData,
                     int aTimeout,
                     const char* aDestination,
                     const char* aPath, const char* aIntf,
                     const char *aFunc, int aFirstArgType, ...);

protected:
  DBusMessage* BuildDBusMessage(const char* aDestination,
                                const char* aPath, const char* aIntf,
                                const char* aFunc, int aFirstArgType,
                                va_list args);

  Scoped<ScopedDBusConnectionPtrTraits> mConnection;

private:
  static bool sDBusIsInit;
};

}
}

#endif
