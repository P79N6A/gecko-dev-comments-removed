






#ifndef NumberObject_h___
#define NumberObject_h___

#include "jsnum.h"

namespace js {

class NumberObject : public JSObject
{
    
    static const unsigned PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const unsigned RESERVED_SLOTS = 1;

    



    static inline NumberObject *create(JSContext *cx, double d);

    double unbox() const {
        return getFixedSlot(PRIMITIVE_VALUE_SLOT).toNumber();
    }

  private:
    inline void setPrimitiveValue(double d) {
        setFixedSlot(PRIMITIVE_VALUE_SLOT, NumberValue(d));
    }

    
    friend JSObject *
    ::js_InitNumberClass(JSContext *cx, js::HandleObject global);
};

} 

#endif 
