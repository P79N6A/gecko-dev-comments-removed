






































#ifndef LIBRARY_H
#define LIBRARY_H

struct PRLibrary;

namespace mozilla {
namespace ctypes {

class Function;

enum LibrarySlot {
  SLOT_LIBRARY = 0,
  SLOT_FUNCTIONLIST = 1,
  LIBRARY_SLOTS
};

class Library
{
public:
  static JSObject* Create(JSContext* cx, jsval aPath);
  static void Trace(JSTracer *trc, JSObject* obj);
  static void Finalize(JSContext* cx, JSObject* obj);

  static PRLibrary* GetLibrary(JSContext* cx, JSObject* obj);
  static JSBool AddFunction(JSContext* cx, JSObject* aLibrary, Function* aFunction);

  
  static JSBool Open(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Close(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Declare(JSContext* cx, uintN argc, jsval* vp);

private:
  
  Library();
};

}
}

#endif
