







































#ifndef BooleanObject_h___
#define BooleanObject_h___

#include "jsbool.h"

namespace js {

class BooleanObject : public ::JSObject
{
    
    static const uintN PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const uintN RESERVED_SLOTS = 1;

    



    static inline BooleanObject *create(JSContext *cx, bool b);

    



    static inline BooleanObject *createWithProto(JSContext *cx, bool b, JSObject &proto);

    Value unbox() const {
        JS_ASSERT(getSlot(PRIMITIVE_VALUE_SLOT).isBoolean());
        return getSlot(PRIMITIVE_VALUE_SLOT);
    }

  private:
    inline void setPrimitiveValue(bool b) {
        setSlot(PRIMITIVE_VALUE_SLOT, BooleanValue(b));
    }

    
    friend JSObject *
    ::js_InitBooleanClass(JSContext *cx, JSObject *global);

  private:
    BooleanObject();
    BooleanObject &operator=(const BooleanObject &bo);
};

} 

#endif 
