





#ifndef vm_BooleanObject_h
#define vm_BooleanObject_h

#include "jsbool.h"
#include "jsobj.h"

namespace js {

class BooleanObject : public JSObject
{
    
    static const unsigned PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const unsigned RESERVED_SLOTS = 1;

    static Class class_;

    



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
