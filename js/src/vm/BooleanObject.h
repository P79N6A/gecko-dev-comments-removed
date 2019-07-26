






#ifndef BooleanObject_h___
#define BooleanObject_h___

#include "jsbool.h"

namespace js {

class BooleanObject : public JSObject
{
    
    static const unsigned PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const unsigned RESERVED_SLOTS = 1;

    



    static inline BooleanObject *create(JSContext *cx, bool b);

    bool unbox() const {
        return getFixedSlot(PRIMITIVE_VALUE_SLOT).toBoolean();
    }

  private:
    inline void setPrimitiveValue(bool b) {
        setFixedSlot(PRIMITIVE_VALUE_SLOT, BooleanValue(b));
    }

    
    friend JSObject *
    ::js_InitBooleanClass(JSContext *cx, js::HandleObject global);
};

} 

#endif 
