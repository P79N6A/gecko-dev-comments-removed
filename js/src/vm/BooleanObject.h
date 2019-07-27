





#ifndef vm_BooleanObject_h
#define vm_BooleanObject_h

#include "jsbool.h"

#include "vm/NativeObject.h"

namespace js {

class BooleanObject : public NativeObject
{
    
    static const unsigned PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const unsigned RESERVED_SLOTS = 1;

    static const Class class_;

    



    static inline BooleanObject *create(JSContext *cx, bool b);

    bool unbox() const {
        return getFixedSlot(PRIMITIVE_VALUE_SLOT).toBoolean();
    }

  private:
    inline void setPrimitiveValue(bool b) {
        setFixedSlot(PRIMITIVE_VALUE_SLOT, BooleanValue(b));
    }

    
    friend JSObject *
    js::InitBooleanClass(JSContext *cx, js::HandleObject global);
};

} 

#endif 
