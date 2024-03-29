

















#include <dbus/dbus.h>
#include "nsAutoPtr.h"
#include "DBusUtils.h"

#undef CHROMIUM_LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define CHROMIUM_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args);
#else
#define CHROMIUM_LOG(args...)  printf(args);
#endif

namespace mozilla {
namespace ipc {





DBusMessageRefPtr::DBusMessageRefPtr(DBusMessage* aMsg)
  : mMsg(aMsg)
{
  if (mMsg) {
    dbus_message_ref(mMsg);
  }
}

DBusMessageRefPtr::~DBusMessageRefPtr()
{
  if (mMsg) {
    dbus_message_unref(mMsg);
  }
}





void DBusReplyHandler::Callback(DBusMessage* aReply, void* aData)
{
  MOZ_ASSERT(aData);

  nsRefPtr<DBusReplyHandler> handler =
    already_AddRefed<DBusReplyHandler>(static_cast<DBusReplyHandler*>(aData));

  handler->Handle(aReply);
}





void
log_and_free_dbus_error(DBusError* err, const char* function, DBusMessage* msg)
{
  if (msg) {
    CHROMIUM_LOG("%s: D-Bus error in %s: %s (%s)", function,
                 dbus_message_get_member((msg)), (err)->name, (err)->message);
  }	else {
    CHROMIUM_LOG("%s: D-Bus error: %s (%s)", __FUNCTION__,
                 (err)->name, (err)->message);
  }
  dbus_error_free((err));
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

  return ret;
}

}
}
