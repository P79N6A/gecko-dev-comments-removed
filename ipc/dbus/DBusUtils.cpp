

















#include <stdio.h>
#include "dbus/dbus.h"

#undef LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args);
#else
#define LOG(args...)  printf(args);
#endif

namespace mozilla {
namespace ipc {

void
log_and_free_dbus_error(DBusError* err, const char* function, DBusMessage* msg)
{
	if(msg) {
		LOG("%s: D-Bus error in %s: %s (%s)", function,
				dbus_message_get_member((msg)), (err)->name, (err)->message);
	}	else {
		LOG("%s: D-Bus error: %s (%s)", __FUNCTION__,
        (err)->name, (err)->message);
	}
	dbus_error_free((err));
}

}
}
