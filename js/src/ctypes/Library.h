






































#ifndef LIBRARY_H
#define LIBRARY_H

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
  JSBool Name(JSContext* cx, uintN argc, jsval *vp);

  JSObject* Create(JSContext* cx, jsval path, JSCTypesCallbacks* callbacks);

  bool IsLibrary(JSContext* cx, JSObject* obj);
  PRLibrary* GetLibrary(JSContext* cx, JSObject* obj);

  JSBool Open(JSContext* cx, uintN argc, jsval* vp);
}

}
}

#endif
