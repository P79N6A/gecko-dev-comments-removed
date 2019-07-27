





#ifndef builtin_WeakSetObject_h
#define builtin_WeakSetObject_h

#include "jsobj.h"

namespace js {

class WeakSetObject : public JSObject
{
  public:
    static const unsigned RESERVED_SLOTS = 1;

    static JSObject *initClass(JSContext *cx, JSObject *obj);
    static const Class class_;

  private:
    static const JSPropertySpec properties[];
    static const JSFunctionSpec methods[];

    static WeakSetObject* create(JSContext *cx);
    static bool construct(JSContext *cx, unsigned argc, Value *vp);
};

} 

extern JSObject *
js_InitWeakSetClass(JSContext *cx, js::HandleObject obj);

#endif 
