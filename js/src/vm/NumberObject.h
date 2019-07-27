





#ifndef vm_NumberObject_h
#define vm_NumberObject_h

#include "jsnum.h"

namespace js {

class NumberObject : public NativeObject
{
    
    static const unsigned PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const unsigned RESERVED_SLOTS = 1;

    static const Class class_;

    



    static inline NumberObject *create(JSContext *cx, double d);

    double unbox() const {
        return getFixedSlot(PRIMITIVE_VALUE_SLOT).toNumber();
    }

  private:
    inline void setPrimitiveValue(double d) {
        setFixedSlot(PRIMITIVE_VALUE_SLOT, NumberValue(d));
    }

    
    friend JSObject *
    js::InitNumberClass(JSContext *cx, HandleObject global);
};

} 

#endif 
