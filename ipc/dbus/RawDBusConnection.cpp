





#include "RawDBusConnection.h"
#include <dbus/dbus.h>

using namespace mozilla::ipc;

RawDBusConnection::RawDBusConnection() {
}

RawDBusConnection::~RawDBusConnection() {
}

nsresult RawDBusConnection::EstablishDBusConnection() {
  DBusError err;
  dbus_error_init(&err);
  mConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    dbus_error_free(&err);
    return NS_ERROR_FAILURE;
  }
  dbus_connection_set_exit_on_disconnect(mConnection, FALSE);
  return NS_OK;
}

void RawDBusConnection::ScopedDBusConnectionPtrTraits::release(DBusConnection* ptr)
{
  if (ptr) dbus_connection_unref(ptr);
}
