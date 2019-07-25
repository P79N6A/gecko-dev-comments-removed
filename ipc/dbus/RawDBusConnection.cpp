





#include "RawDBusConnection.h"
#include <dbus/dbus.h>

using namespace mozilla::ipc;

RawDBusConnection::RawDBusConnection() {
}

RawDBusConnection::~RawDBusConnection() {
}

bool RawDBusConnection::Create() {
  DBusError err;
  dbus_error_init(&err);
  dbus_threads_init_default();
  mConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    dbus_error_free(&err);
    return false;
  }
  dbus_connection_set_exit_on_disconnect(mConnection, FALSE);
  return true;
}

