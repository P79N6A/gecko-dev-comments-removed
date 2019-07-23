






































#ifndef LIBRARY_H
#define LIBRARY_H

#include "Function.h"
#include "jsapi.h"

struct PRLibrary;
class Function;

namespace mozilla {
namespace ctypes {

class Library
{
public:
  static JSObject* Create(JSContext* cx, jsval aPath);
  static void Finalize(JSContext* cx, JSObject* obj);

  static PRLibrary* GetLibrary(JSContext* cx, JSObject* obj);
  static bool AddFunction(JSContext* cx, JSObject* aLibrary, Function* aFunction);

  
  static JSBool Open(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Close(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Declare(JSContext* cx, uintN argc, jsval* vp);

private:
  
  Library();
};

}
}

#endif
