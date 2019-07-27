









#ifndef WEBRTC_BASE_LIBDBUSGLIBSYMBOLTABLE_H_
#define WEBRTC_BASE_LIBDBUSGLIBSYMBOLTABLE_H_

#ifdef HAVE_DBUS_GLIB

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "webrtc/base/latebindingsymboltable.h"

namespace rtc {

#define LIBDBUS_GLIB_CLASS_NAME LibDBusGlibSymbolTable



#define LIBDBUS_GLIB_SYMBOLS_LIST \
  X(dbus_bus_add_match) \
  X(dbus_connection_add_filter) \
  X(dbus_connection_close) \
  X(dbus_connection_remove_filter) \
  X(dbus_connection_set_exit_on_disconnect) \
  X(dbus_g_bus_get) \
  X(dbus_g_bus_get_private) \
  X(dbus_g_connection_get_connection) \
  X(dbus_g_connection_unref) \
  X(dbus_g_thread_init) \
  X(dbus_message_get_interface) \
  X(dbus_message_get_member) \
  X(dbus_message_get_path) \
  X(dbus_message_get_type) \
  X(dbus_message_iter_get_arg_type) \
  X(dbus_message_iter_get_basic) \
  X(dbus_message_iter_init) \
  X(dbus_message_ref) \
  X(dbus_message_unref)

#define LATE_BINDING_SYMBOL_TABLE_CLASS_NAME LIBDBUS_GLIB_CLASS_NAME
#define LATE_BINDING_SYMBOL_TABLE_SYMBOLS_LIST LIBDBUS_GLIB_SYMBOLS_LIST
#include "webrtc/base/latebindingsymboltable.h.def"

}  

#endif  

#endif  
