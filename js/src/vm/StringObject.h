







































#ifndef StringObject_h___
#define StringObject_h___

#include "jsobj.h"
#include "jsstr.h"

namespace js {

class StringObject : public ::JSObject
{
    static const uintN PRIMITIVE_THIS_SLOT = 0;
    static const uintN LENGTH_SLOT = 1;

  public:
    static const uintN RESERVED_SLOTS = 2;

    



    static inline StringObject *create(JSContext *cx, JSString *str);

    



    static inline StringObject *createWithProto(JSContext *cx, JSString *str, JSObject &proto);

    JSString *unbox() const {
        return getSlot(PRIMITIVE_THIS_SLOT).toString();
    }

    inline size_t length() const {
        return size_t(getSlot(LENGTH_SLOT).toInt32());
    }

  private:
    inline bool init(JSContext *cx, JSString *str);

    void setStringThis(JSString *str) {
        JS_ASSERT(getSlot(PRIMITIVE_THIS_SLOT).isUndefined());
        setSlot(PRIMITIVE_THIS_SLOT, StringValue(str));
        setSlot(LENGTH_SLOT, Int32Value(int32(str->length())));
    }

    
    friend JSObject *
    ::js_InitStringClass(JSContext *cx, JSObject *global);

    




    const js::Shape *assignInitialShape(JSContext *cx);

  private:
    StringObject();
    StringObject &operator=(const StringObject &so);
};

} 

#endif 
