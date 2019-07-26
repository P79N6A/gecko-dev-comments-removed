






#ifndef StringObject_h___
#define StringObject_h___

#include "jsobj.h"
#include "jsstr.h"

namespace js {

class StringObject : public JSObject
{
    static const unsigned PRIMITIVE_VALUE_SLOT = 0;
    static const unsigned LENGTH_SLOT = 1;

  public:
    static const unsigned RESERVED_SLOTS = 2;

    



    static inline StringObject *create(JSContext *cx, HandleString str);

    JSString *unbox() const {
        return getFixedSlot(PRIMITIVE_VALUE_SLOT).toString();
    }

    inline size_t length() const {
        return size_t(getFixedSlot(LENGTH_SLOT).toInt32());
    }

    static size_t offsetOfPrimitiveValue() {
        return getFixedSlotOffset(PRIMITIVE_VALUE_SLOT);
    }
    static size_t offsetOfLength() {
        return getFixedSlotOffset(LENGTH_SLOT);
    }

  private:
    inline bool init(JSContext *cx, HandleString str);

    void setStringThis(JSString *str) {
        JS_ASSERT(getReservedSlot(PRIMITIVE_VALUE_SLOT).isUndefined());
        setFixedSlot(PRIMITIVE_VALUE_SLOT, StringValue(str));
        setFixedSlot(LENGTH_SLOT, Int32Value(int32_t(str->length())));
    }

    
    friend JSObject *
    ::js_InitStringClass(JSContext *cx, js::HandleObject global);

    




    RawShape assignInitialShape(JSContext *cx);
};

} 

#endif 
