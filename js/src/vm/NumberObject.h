







































#ifndef NumberObject_h___
#define NumberObject_h___

#include "mozilla/Attributes.h"

#include "jsnum.h"

namespace js {

class NumberObject : public JSObject
{
    
    static const uintN PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const uintN RESERVED_SLOTS = 1;

    



    static inline NumberObject *create(JSContext *cx, jsdouble d);

    



    static inline NumberObject *createWithProto(JSContext *cx, jsdouble d, JSObject &proto);

    double unbox() const {
        return getFixedSlot(PRIMITIVE_VALUE_SLOT).toNumber();
    }

  private:
    inline void setPrimitiveValue(jsdouble d) {
        setFixedSlot(PRIMITIVE_VALUE_SLOT, NumberValue(d));
    }

    
    friend JSObject *
    ::js_InitNumberClass(JSContext *cx, JSObject *global);

  private:
    NumberObject() MOZ_DELETE;
    NumberObject &operator=(const NumberObject &so) MOZ_DELETE;
};

} 

#endif 
