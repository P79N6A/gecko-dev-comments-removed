

















#include "DBusUtils.h"
#include "DBusThread.h"
#include "nsThreadUtils.h"
#include "mozilla/Monitor.h"
#include "nsAutoPtr.h"
#include <cstdio>
#include <cstring>

#undef LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args);
#else
#define LOG(args...)  printf(args);
#endif

#define BLUEZ_DBUS_BASE_PATH      "/org/bluez"
#define BLUEZ_DBUS_BASE_IFC       "org.bluez"
#define BLUEZ_ERROR_IFC           "org.bluez.Error"

namespace mozilla {
namespace ipc {

void
log_and_free_dbus_error(DBusError* err, const char* function, DBusMessage* msg)
{
  if (msg) {
    LOG("%s: D-Bus error in %s: %s (%s)", function,
        dbus_message_get_member((msg)), (err)->name, (err)->message);
  }	else {
    LOG("%s: D-Bus error: %s (%s)", __FUNCTION__,
        (err)->name, (err)->message);
  }
  dbus_error_free((err));
}

class DBusConnectionSendRunnableBase : public nsRunnable
{
protected:
  DBusConnectionSendRunnableBase(DBusConnection* aConnection,
                                 DBusMessage* aMessage)
  : mConnection(aConnection),
    mMessage(aMessage)
  {
    MOZ_ASSERT(mConnection);
    MOZ_ASSERT(mMessage);
  }

  virtual ~DBusConnectionSendRunnableBase()
  { }

  DBusConnection*   mConnection;
  DBusMessageRefPtr mMessage;
};

class DBusConnectionSendSyncRunnable : public DBusConnectionSendRunnableBase
{
public:
  bool WaitForCompletion()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    MonitorAutoLock autoLock(mCompletedMonitor);
    while (!mCompleted) {
      mCompletedMonitor.Wait();
    }
    return mSuccess;
  }

protected:
  DBusConnectionSendSyncRunnable(DBusConnection* aConnection,
                                 DBusMessage* aMessage)
  : DBusConnectionSendRunnableBase(aConnection, aMessage),
    mCompletedMonitor("DBusConnectionSendSyncRunnable.mCompleted"),
    mCompleted(false),
    mSuccess(false)
  { }

  virtual ~DBusConnectionSendSyncRunnable()
  { }

  
  
  void Completed(bool aSuccess)
  {
    MonitorAutoLock autoLock(mCompletedMonitor);
    MOZ_ASSERT(!mCompleted);
    mSuccess = aSuccess;
    mCompleted = true;
    mCompletedMonitor.Notify();
  }

private:
  Monitor mCompletedMonitor;
  bool    mCompleted;
  bool    mSuccess;
};





class DBusConnectionSendRunnable : public DBusConnectionSendSyncRunnable
{
public:
  DBusConnectionSendRunnable(DBusConnection* aConnection,
                             DBusMessage* aMessage,
                             dbus_uint32_t* aSerial)
  : DBusConnectionSendSyncRunnable(aConnection, aMessage),
    mSerial(aSerial)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    dbus_bool_t success = dbus_connection_send(mConnection, mMessage, mSerial);
    Completed(success == TRUE);

    NS_ENSURE_TRUE(success == TRUE, NS_ERROR_FAILURE);

    return NS_OK;
  }

protected:
  ~DBusConnectionSendRunnable()
  { }

private:
  dbus_uint32_t* mSerial;
};

dbus_bool_t
dbus_func_send(DBusConnection* aConnection, dbus_uint32_t* aSerial,
               DBusMessage* aMessage)
{
  nsRefPtr<DBusConnectionSendRunnable> t(
    new DBusConnectionSendRunnable(aConnection, aMessage, aSerial));
  MOZ_ASSERT(t);

  nsresult rv = DispatchToDBusThread(t);

  if (NS_FAILED(rv)) {
    if (aMessage) {
      dbus_message_unref(aMessage);
    }
    return FALSE;
  }

  if (aSerial && !t->WaitForCompletion()) {
    return FALSE;
  }

  return TRUE;
}

static dbus_bool_t
dbus_func_args_send_valist(DBusConnection* aConnection,
                           dbus_uint32_t* aSerial,
                           const char* aPath,
                           const char* aInterface,
                           const char* aFunction,
                           int aFirstArgType,
                           va_list aArgs)
{
  
  DBusMessage* message = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC,
                                                      aPath,
                                                      aInterface,
                                                      aFunction);
  if (!message) {
    LOG("Could not allocate D-Bus message object!");
    goto done;
  }

  
  if (!dbus_message_append_args_valist(message, aFirstArgType, aArgs)) {
    LOG("Could not append argument to method call!");
    goto done;
  }

  return dbus_func_send(aConnection, aSerial, message);

done:
  if (message) {
    dbus_message_unref(message);
  }
  return FALSE;
}

dbus_bool_t
dbus_func_args_send(DBusConnection* aConnection,
                    dbus_uint32_t* aSerial,
                    const char* aPath,
                    const char* aInterface,
                    const char* aFunction,
                    int aFirstArgType, ...)
{
  va_list args;
  va_start(args, aFirstArgType);

  dbus_bool_t success = dbus_func_args_send_valist(aConnection,
                                                   aSerial,
                                                   aPath,
                                                   aInterface,
                                                   aFunction,
                                                   aFirstArgType,
                                                   args);
  va_end(args);

  return success;
}





class DBusConnectionSendWithReplyRunnable : public DBusConnectionSendRunnableBase
{
private:
  class NotifyData
  {
  public:
    NotifyData(void (*aCallback)(DBusMessage*, void*), void* aData)
    : mCallback(aCallback),
      mData(aData)
    { }

    void RunNotifyCallback(DBusMessage* aMessage)
    {
      if (mCallback) {
        mCallback(aMessage, mData);
      }
    }

  private:
    void (*mCallback)(DBusMessage*, void*);
    void*  mData;
  };

  
  
  static void Notify(DBusPendingCall* aCall, void* aData)
  {
    MOZ_ASSERT(!NS_IsMainThread());

    nsAutoPtr<NotifyData> data(static_cast<NotifyData*>(aData));

    
    
    DBusMessage* reply = dbus_pending_call_steal_reply(aCall);

    if (reply) {
      data->RunNotifyCallback(reply);
      dbus_message_unref(reply);
    }

    dbus_pending_call_cancel(aCall);
    dbus_pending_call_unref(aCall);
  }

public:
  DBusConnectionSendWithReplyRunnable(DBusConnection* aConnection,
                                      DBusMessage* aMessage,
                                      int aTimeout,
                                      void (*aCallback)(DBusMessage*, void*),
                                      void* aData)
  : DBusConnectionSendRunnableBase(aConnection, aMessage),
    mCallback(aCallback),
    mData(aData),
    mTimeout(aTimeout)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    
    nsAutoPtr<NotifyData> data(new NotifyData(mCallback, mData));
    NS_ENSURE_TRUE(data, NS_ERROR_OUT_OF_MEMORY);

    DBusPendingCall* call;

    dbus_bool_t success = dbus_connection_send_with_reply(mConnection,
                                                          mMessage,
                                                          &call,
                                                          mTimeout);
    NS_ENSURE_TRUE(success == TRUE, NS_ERROR_FAILURE);

    success = dbus_pending_call_set_notify(call, Notify, data, nullptr);
    NS_ENSURE_TRUE(success == TRUE, NS_ERROR_FAILURE);

    data.forget();
    dbus_message_unref(mMessage);

    return NS_OK;
  };

protected:
  ~DBusConnectionSendWithReplyRunnable()
  { }

private:
  void (*mCallback)(DBusMessage*, void*);
  void*  mData;
  int    mTimeout;
};

dbus_bool_t dbus_func_send_async(DBusConnection* conn,
                                 DBusMessage* msg,
                                 int timeout_ms,
                                 void (*user_cb)(DBusMessage*,
                                                 void*),
                                 void* user)
{
  nsRefPtr<nsIRunnable> t(new DBusConnectionSendWithReplyRunnable(conn, msg,
                                                                  timeout_ms,
                                                                  user_cb,
                                                                  user));
  MOZ_ASSERT(t);

  nsresult rv = DispatchToDBusThread(t);

  if (NS_FAILED(rv)) {
    if (msg) {
      dbus_message_unref(msg);
    }
    return FALSE;
  }

  return TRUE;
}

static dbus_bool_t dbus_func_args_async_valist(DBusConnection *conn,
                                               int timeout_ms,
                                               void (*user_cb)(DBusMessage*,
                                                               void*),
                                               void *user,
                                               const char *path,
                                               const char *ifc,
                                               const char *func,
                                               int first_arg_type,
                                               va_list args) {
  DBusMessage *msg = NULL;  
  
  msg = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC, path, ifc, func);

  if (msg == NULL) {
    LOG("Could not allocate D-Bus message object!");
    goto done;
  }

  
  if (!dbus_message_append_args_valist(msg, first_arg_type, args)) {
    LOG("Could not append argument to method call!");
    goto done;
  }

  return dbus_func_send_async(conn, msg, timeout_ms, user_cb, user);
done:
  if (msg) dbus_message_unref(msg);
  return FALSE;
}

dbus_bool_t dbus_func_args_async(DBusConnection *conn,
                                 int timeout_ms,
                                 void (*reply)(DBusMessage *, void *),
                                 void *user,
                                 const char *path,
                                 const char *ifc,
                                 const char *func,
                                 int first_arg_type,
                                 ...) {
  dbus_bool_t ret;
  va_list lst;
  va_start(lst, first_arg_type);

  ret = dbus_func_args_async_valist(conn,
                                    timeout_ms,
                                    reply, user,
                                    path, ifc, func,
                                    first_arg_type, lst);
  va_end(lst);
  return ret;
}







DBusMessage * dbus_func_args_timeout_valist(DBusConnection *conn,
                                            int timeout_ms,
                                            DBusError *err,
                                            const char *path,
                                            const char *ifc,
                                            const char *func,
                                            int first_arg_type,
                                            va_list args) {
  
  DBusMessage *msg = NULL, *reply = NULL;
  bool return_error = (err != NULL);

  if (!return_error) {
    err = (DBusError*)malloc(sizeof(DBusError));
    dbus_error_init(err);
  }

  
  msg = dbus_message_new_method_call(BLUEZ_DBUS_BASE_IFC, path, ifc, func);

  if (msg == NULL) {
    LOG("Could not allocate D-Bus message object!");
    goto done;
  }

  
  if (!dbus_message_append_args_valist(msg, first_arg_type, args)) {
    LOG("Could not append argument to method call!");
    goto done;
  }

  
  reply = dbus_connection_send_with_reply_and_block(conn, msg, timeout_ms, err);
  if (!return_error && dbus_error_is_set(err)) {
    LOG_AND_FREE_DBUS_ERROR_WITH_MSG(err, msg);
  }

done:
  if (!return_error) {
    free(err);
  }
  if (msg) dbus_message_unref(msg);
  return reply;
}

DBusMessage * dbus_func_args_timeout(DBusConnection *conn,
                                     int timeout_ms,
                                     DBusError* err,
                                     const char *path,
                                     const char *ifc,
                                     const char *func,
                                     int first_arg_type,
                                     ...) {
  DBusMessage *ret;
  va_list lst;
  va_start(lst, first_arg_type);
  ret = dbus_func_args_timeout_valist(conn, timeout_ms, err,
                                      path, ifc, func,
                                      first_arg_type, lst);
  va_end(lst);
  return ret;
}

DBusMessage * dbus_func_args(DBusConnection *conn,
                             const char *path,
                             const char *ifc,
                             const char *func,
                             int first_arg_type,
                             ...) {
  DBusMessage *ret;
  va_list lst;
  va_start(lst, first_arg_type);
  ret = dbus_func_args_timeout_valist(conn, -1, NULL,
                                      path, ifc, func,
                                      first_arg_type, lst);
  va_end(lst);
  return ret;
}

DBusMessage * dbus_func_args_error(DBusConnection *conn,
                                   DBusError *err,
                                   const char *path,
                                   const char *ifc,
                                   const char *func,
                                   int first_arg_type,
                                   ...) {
  DBusMessage *ret;
  va_list lst;
  va_start(lst, first_arg_type);
  ret = dbus_func_args_timeout_valist(conn, -1, err,
                                      path, ifc, func,
                                      first_arg_type, lst);
  va_end(lst);
  return ret;
}

int dbus_returns_int32(DBusMessage *reply) 
{
  DBusError err;
  int32_t ret = -1;

  dbus_error_init(&err);
  if (!dbus_message_get_args(reply, &err,
                             DBUS_TYPE_INT32, &ret,
                             DBUS_TYPE_INVALID)) {
    LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
  }

  dbus_message_unref(reply);
  return ret;
}

int dbus_returns_uint32(DBusMessage *reply)
{
  DBusError err;
  uint32_t ret = -1;

  dbus_error_init(&err);
  if (!dbus_message_get_args(reply, &err,
                             DBUS_TYPE_UINT32, &ret,
                             DBUS_TYPE_INVALID)) {
    LOG_AND_FREE_DBUS_ERROR_WITH_MSG(&err, reply);
  }

  dbus_message_unref(reply);
  return ret;
}

}
}
