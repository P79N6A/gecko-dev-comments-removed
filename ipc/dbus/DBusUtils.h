

















#ifndef mozilla_ipc_dbus_dbusutils_h__
#define mozilla_ipc_dbus_dbusutils_h__

#include <dbus/dbus.h>
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

typedef void (*DBusCallback)(DBusMessage *, void *);


void log_and_free_dbus_error(DBusError* err,
                             const char* function,
                             DBusMessage* msg = NULL);

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

int dbus_returns_uint32(DBusMessage *reply);

}
}

#endif

