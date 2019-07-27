





#ifndef builtin_SymbolObject_h
#define builtin_SymbolObject_h

#include "vm/NativeObject.h"
#include "vm/Symbol.h"

namespace js {

class SymbolObject : public NativeObject
{
    
    static const unsigned PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const unsigned RESERVED_SLOTS = 1;

    static const Class class_;

    static JSObject *initClass(JSContext *cx, js::HandleObject obj);

    



    static SymbolObject *create(JSContext *cx, JS::HandleSymbol symbol);

    JS::Symbol *unbox() const {
        return getFixedSlot(PRIMITIVE_VALUE_SLOT).toSymbol();
    }

  private:
    inline void setPrimitiveValue(JS::Symbol *symbol) {
        setFixedSlot(PRIMITIVE_VALUE_SLOT, SymbolValue(symbol));
    }

    static bool construct(JSContext *cx, unsigned argc, Value *vp);

    static bool convert(JSContext *cx, HandleObject obj, JSType type, MutableHandleValue vp);

    
    static bool for_(JSContext *cx, unsigned argc, Value *vp);
    static bool keyFor(JSContext *cx, unsigned argc, Value *vp);

    
    static bool toString_impl(JSContext *cx, CallArgs args);
    static bool toString(JSContext *cx, unsigned argc, Value *vp);
    static bool valueOf_impl(JSContext *cx, CallArgs args);
    static bool valueOf(JSContext *cx, unsigned argc, Value *vp);

    static const JSPropertySpec properties[];
    static const JSFunctionSpec methods[];
    static const JSFunctionSpec staticMethods[];
};

} 

extern JSObject *
js_InitSymbolClass(JSContext *cx, js::HandleObject obj);

#endif 
