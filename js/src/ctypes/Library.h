




#ifndef ctypes_Library_h
#define ctypes_Library_h

#include "js/TypeDecls.h"

struct JSCTypesCallbacks;
struct PRLibrary;

namespace js {
namespace ctypes {

enum LibrarySlot {
  SLOT_LIBRARY = 0,
  LIBRARY_SLOTS
};

namespace Library
{
  bool Name(JSContext* cx, unsigned argc, JS::Value* vp);

  JSObject* Create(JSContext* cx, JS::Value path, const JSCTypesCallbacks* callbacks);

  bool IsLibrary(JSObject* obj);
  PRLibrary* GetLibrary(JSObject* obj);

  bool Open(JSContext* cx, unsigned argc, JS::Value* vp);
} 

} 
} 

#endif 
