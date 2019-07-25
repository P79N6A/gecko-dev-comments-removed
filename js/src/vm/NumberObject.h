







































#ifndef NumberObject_h___
#define NumberObject_h___

#include "jsnum.h"

namespace js {

class NumberObject : public ::JSObject
{
    
    static const uintN PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const uintN RESERVED_SLOTS = 1;

    



    static inline NumberObject *create(JSContext *cx, jsdouble d);

    



    static inline NumberObject *createWithProto(JSContext *cx, jsdouble d, JSObject &proto);

    Value unbox() const {
        JS_ASSERT(getSlot(PRIMITIVE_VALUE_SLOT).isNumber());
        return getSlot(PRIMITIVE_VALUE_SLOT);
    }

  private:
    inline void setPrimitiveValue(jsdouble d) {
        setSlot(PRIMITIVE_VALUE_SLOT, NumberValue(d));
    }

    
    friend JSObject *
    ::js_InitNumberClass(JSContext *cx, JSObject *global);

  private:
    NumberObject();
    NumberObject &operator=(const NumberObject &so);
};

} 

#endif 
