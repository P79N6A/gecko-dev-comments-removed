






































#ifndef LIBRARY_H
#define LIBRARY_H

struct PRLibrary;

namespace mozilla {
namespace ctypes {

enum LibrarySlot {
  SLOT_LIBRARY = 0,
  LIBRARY_SLOTS
};

namespace Library
{
  JSObject* Create(JSContext* cx, jsval aPath);

  bool IsLibrary(JSContext* cx, JSObject* obj);
  PRLibrary* GetLibrary(JSContext* cx, JSObject* obj);

  JSBool Open(JSContext* cx, uintN argc, jsval* vp);
}

}
}

#endif
