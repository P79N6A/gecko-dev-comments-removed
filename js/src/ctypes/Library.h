




#ifndef ctypes_Library_h
#define ctypes_Library_h

#include "jsapi.h"

struct PRLibrary;

namespace js {
namespace ctypes {

enum LibrarySlot {
  SLOT_LIBRARY = 0,
  LIBRARY_SLOTS
};

namespace Library
{
  bool Name(JSContext* cx, unsigned argc, jsval *vp);

  JSObject* Create(JSContext* cx, jsval path, JSCTypesCallbacks* callbacks);

  bool IsLibrary(JSObject* obj);
  PRLibrary* GetLibrary(JSObject* obj);

  bool Open(JSContext* cx, unsigned argc, jsval* vp);
}

}
}

#endif 
