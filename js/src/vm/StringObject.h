







































#ifndef StringObject_h___
#define StringObject_h___

#include "mozilla/Attributes.h"

#include "jsobj.h"
#include "jsstr.h"

namespace js {

class StringObject : public JSObject
{
    static const uintN PRIMITIVE_VALUE_SLOT = 0;
    static const uintN LENGTH_SLOT = 1;

  public:
    static const uintN RESERVED_SLOTS = 2;

    



    static inline StringObject *create(JSContext *cx, JSString *str);

    



    static inline StringObject *createWithProto(JSContext *cx, JSString *str, JSObject &proto);

    JSString *unbox() const {
        return getFixedSlot(PRIMITIVE_VALUE_SLOT).toString();
    }

    inline size_t length() const {
        return size_t(getFixedSlot(LENGTH_SLOT).toInt32());
    }

    static size_t getPrimitiveValueOffset() {
        return getFixedSlotOffset(PRIMITIVE_VALUE_SLOT);
    }

  private:
    inline bool init(JSContext *cx, JSString *str);

    void setStringThis(JSString *str) {
        JS_ASSERT(getReservedSlot(PRIMITIVE_VALUE_SLOT).isUndefined());
        setFixedSlot(PRIMITIVE_VALUE_SLOT, StringValue(str));
        setFixedSlot(LENGTH_SLOT, Int32Value(int32_t(str->length())));
    }

    
    friend JSObject *
    ::js_InitStringClass(JSContext *cx, JSObject *global);

    




    Shape *assignInitialShape(JSContext *cx);

  private:
    StringObject() MOZ_DELETE;
    StringObject &operator=(const StringObject &so) MOZ_DELETE;
};

} 

#endif 
