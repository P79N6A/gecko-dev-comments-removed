









#ifdef HAVE_DBUS_GLIB

#include "webrtc/base/libdbusglibsymboltable.h"

namespace rtc {

#define LATE_BINDING_SYMBOL_TABLE_CLASS_NAME LIBDBUS_GLIB_CLASS_NAME
#define LATE_BINDING_SYMBOL_TABLE_SYMBOLS_LIST LIBDBUS_GLIB_SYMBOLS_LIST
#define LATE_BINDING_SYMBOL_TABLE_DLL_NAME "libdbus-glib-1.so.2"
#include "webrtc/base/latebindingsymboltable.cc.def"

}  

#endif  
