

















#ifndef mozilla_ipc_dbus_dbusutils_h__
#define mozilla_ipc_dbus_dbusutils_h__

#include <dbus/dbus.h>
#include "mozilla/RefPtr.h"
#include "mozilla/Scoped.h"



#define LOG_AND_FREE_DBUS_ERROR_WITH_MSG(err, msg) log_and_free_dbus_error(err, __FUNCTION__, msg);
#define LOG_AND_FREE_DBUS_ERROR(err) log_and_free_dbus_error(err, __FUNCTION__);

struct DBusMessage;
struct DBusError;

namespace mozilla {
namespace ipc {

class DBusMessageRefPtr
{
public:
  DBusMessageRefPtr(DBusMessage* aMsg) : mMsg(aMsg)
  {
    if (mMsg) dbus_message_ref(mMsg);
  }
  ~DBusMessageRefPtr()
  {
    if (mMsg) dbus_message_unref(mMsg);
  }
  operator DBusMessage*() { return mMsg; }
  DBusMessage* get() { return mMsg; }
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

typedef void (*DBusCallback)(DBusMessage *, void *);

void log_and_free_dbus_error(DBusError* err,
                             const char* function,
                             DBusMessage* msg = NULL);

dbus_bool_t dbus_func_send(DBusConnection *aConnection,
                           dbus_uint32_t *aSerial,
                           DBusMessage *aMessage);

dbus_bool_t dbus_func_args_send(DBusConnection *aConnection,
                                dbus_uint32_t *aSerial,
                                const char *aPath,
                                const char *aInterface,
                                const char *aFunction,
                                int aFirstArgType, ...);

dbus_bool_t dbus_func_send_async(DBusConnection* conn,
                                 DBusMessage* msg,
                                 int timeout_ms,
                                 DBusCallback user_cb,
                                 void* user);

dbus_bool_t dbus_func_args_async(DBusConnection* conn,
                                 int timeout_ms,
                                 DBusCallback reply,
                                 void* user,
                                 const char* path,
                                 const char* ifc,
                                 const char* func,
                                 int first_arg_type,
                                 ...);

dbus_bool_t dbus_func_send_and_block(DBusConnection* aConnection,
                                     int aTimeout,
                                     DBusMessage** aReply,
                                     DBusError* aError,
                                     DBusMessage* aMessage);

DBusMessage*  dbus_func_args(DBusConnection* conn,
                             const char* path,
                             const char* ifc,
                             const char* func,
                             int first_arg_type,
                             ...);

DBusMessage*  dbus_func_args_error(DBusConnection* conn,
                                   DBusError* err,
                                   const char* path,
                                   const char* ifc,
                                   const char* func,
                                   int first_arg_type,
                                   ...);

DBusMessage*  dbus_func_args_timeout(DBusConnection* conn,
                                     int timeout_ms,
                                     DBusError* err,
                                     const char* path,
                                     const char* ifc,
                                     const char* func,
                                     int first_arg_type,
                                     ...);

DBusMessage*  dbus_func_args_timeout_valist(DBusConnection* conn,
                                            int timeout_ms,
                                            DBusError* err,
                                            const char* path,
                                            const char* ifc,
                                            const char* func,
                                            int first_arg_type,
                                            va_list args);

int dbus_returns_int32(DBusMessage *reply);

}
}

#endif

