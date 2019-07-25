







































#ifndef BooleanObject_h___
#define BooleanObject_h___

#include "mozilla/Attributes.h"

#include "jsbool.h"

namespace js {

class BooleanObject : public JSObject
{
    
    static const unsigned PRIMITIVE_VALUE_SLOT = 0;

  public:
    static const unsigned RESERVED_SLOTS = 1;

    



    static inline BooleanObject *create(JSContext *cx, bool b);

    



    static inline BooleanObject *createWithProto(JSContext *cx, bool b, JSObject &proto);

    bool unbox() const {
        return getFixedSlot(PRIMITIVE_VALUE_SLOT).toBoolean();
    }

  private:
    inline void setPrimitiveValue(bool b) {
        setFixedSlot(PRIMITIVE_VALUE_SLOT, BooleanValue(b));
    }

    
    friend JSObject *
    ::js_InitBooleanClass(JSContext *cx, JSObject *global);

  private:
    BooleanObject() MOZ_DELETE;
    BooleanObject &operator=(const BooleanObject &bo) MOZ_DELETE;
};

} 

#endif 
