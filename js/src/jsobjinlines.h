







































#ifndef jsobjinlines_h___
#define jsobjinlines_h___

#include "jsobj.h"
#include "jsscope.h"

inline jsval
JSObject::lockAndGetSlot(JSContext *cx, uintN slot) {
#ifdef JS_THREADSAFE
    








    OBJ_CHECK_SLOT(this, slot);
    return (OBJ_SCOPE(this)->title.ownercx == cx)
           ? LOCKED_OBJ_GET_SLOT(this, slot)
           : js_GetSlotThreadSafe(cx, this, slot);
#else
    return LOCKED_OBJ_GET_SLOT(this, slot);
#endif
}

inline void
JSObject::lockAndSetSlot(JSContext *cx, uintN slot, jsval value) {
#ifdef JS_THREADSAFE
    
    OBJ_CHECK_SLOT(this, slot);
    if (OBJ_SCOPE(this)->title.ownercx == cx)
        LOCKED_OBJ_SET_SLOT(this, slot, value);
    else
        js_SetSlotThreadSafe(cx, this, slot, value);
#else
    LOCKED_OBJ_SET_SLOT(this, slot, value);
#endif
}

inline void
JSObject::initSharingEmptyScope(JSClass *clasp, JSObject *proto, JSObject *parent,
                                jsval privateSlotValue)
{
    init(clasp, proto, parent, privateSlotValue);

    JSEmptyScope *emptyScope = OBJ_SCOPE(proto)->emptyScope;
    JS_ASSERT(emptyScope->clasp == clasp);
    emptyScope->hold();
    map = emptyScope;
}

inline void
JSObject::freeSlotsArray(JSContext *cx)
{
    JS_ASSERT(hasSlotsArray());
    JS_ASSERT(size_t(dslots[-1]) > JS_INITIAL_NSLOTS);
    cx->free(dslots - 1);
}

inline bool
JSObject::unbrand(JSContext *cx)
{
    if (OBJ_IS_NATIVE(this)) {
        JS_LOCK_OBJ(cx, this);
        JSScope *scope = OBJ_SCOPE(this);
        if (scope->isSharedEmpty()) {
            scope = js_GetMutableScope(cx, this);
            if (!scope) {
                JS_UNLOCK_OBJ(cx, this);
                return false;
            }
        }
        scope->setGeneric();
        JS_UNLOCK_SCOPE(cx, scope);
    }
    return true;
}

namespace js {

typedef Vector<PropertyDescriptor, 1> PropertyDescriptorArray;

class AutoDescriptorArray : private AutoGCRooter
{
  public:
    AutoDescriptorArray(JSContext *cx)
      : AutoGCRooter(cx, DESCRIPTORS), descriptors(cx)
    { }

    PropertyDescriptor *append() {
        if (!descriptors.append(PropertyDescriptor()))
            return NULL;
        return &descriptors.back();
    }

    PropertyDescriptor& operator[](size_t i) {
        JS_ASSERT(i < descriptors.length());
        return descriptors[i];
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    PropertyDescriptorArray descriptors;
};

}

#endif 
