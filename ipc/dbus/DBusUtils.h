

















#ifndef mozilla_ipc_dbus_dbusutils_h__
#define mozilla_ipc_dbus_dbusutils_h__

#include "mozilla/RefPtr.h"



#define LOG_AND_FREE_DBUS_ERROR_WITH_MSG(err, msg) log_and_free_dbus_error(err, __FUNCTION__, msg);
#define LOG_AND_FREE_DBUS_ERROR(err) log_and_free_dbus_error(err, __FUNCTION__);

struct DBusError;
struct DBusMessage;

namespace mozilla {
namespace ipc {

class DBusMessageRefPtr
{
public:
  DBusMessageRefPtr(DBusMessage* aMsg);
  ~DBusMessageRefPtr();

  operator DBusMessage* ()
  {
    return mMsg;
  }

  DBusMessage* get()
  {
    return mMsg;
  }

private:
  DBusMessage* mMsg;
};







class DBusReplyHandler : public mozilla::RefCounted<DBusReplyHandler>
{
public:
  virtual ~DBusReplyHandler() {
  }

  



  static void Callback(DBusMessage* aReply, void* aData);

  


  virtual void Handle(DBusMessage* aReply) = 0;

protected:
  DBusReplyHandler()
  {
  }

  DBusReplyHandler(const DBusReplyHandler& aHandler)
  {
  }

  DBusReplyHandler& operator = (const DBusReplyHandler& aRhs)
  {
    return *this;
  }
};

void log_and_free_dbus_error(DBusError* err,
                             const char* function,
                             DBusMessage* msg = nullptr);

int dbus_returns_int32(DBusMessage *reply);

}
}

#endif

