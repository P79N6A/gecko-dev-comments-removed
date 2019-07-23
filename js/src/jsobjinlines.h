







































#ifndef jsobjinlines_h___
#define jsobjinlines_h___

#include "jsobj.h"
#include "jsscope.h"

inline jsval
JSObject::getSlotMT(JSContext *cx, uintN slot)
{
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
JSObject::setSlotMT(JSContext *cx, uintN slot, jsval value)
{
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

inline uint32
JSObject::getArrayLength() const
{
    JS_ASSERT(isArray());
    return uint32(fslots[JSSLOT_ARRAY_LENGTH]);
}

inline uint32 
JSObject::getArrayCount() const
{
    JS_ASSERT(isArray());
    return uint32(fslots[JSSLOT_ARRAY_COUNT]);
}

inline void 
JSObject::setArrayLength(uint32 length)
{
    JS_ASSERT(isArray());
    fslots[JSSLOT_ARRAY_LENGTH] = length;
}

inline void 
JSObject::setArrayCount(uint32 count)
{
    JS_ASSERT(isArray());
    fslots[JSSLOT_ARRAY_COUNT] = count;
}

inline void 
JSObject::voidDenseArrayCount()
{
    JS_ASSERT(isDenseArray());
    fslots[JSSLOT_ARRAY_COUNT] = JSVAL_VOID;
}

inline void 
JSObject::incArrayCountBy(uint32 posDelta)
{
    JS_ASSERT(isArray());
    fslots[JSSLOT_ARRAY_COUNT] += posDelta;
}

inline void 
JSObject::decArrayCountBy(uint32 negDelta)
{
    JS_ASSERT(isArray());
    fslots[JSSLOT_ARRAY_COUNT] -= negDelta;
}

inline void
JSObject::voidArrayUnused()
{
    JS_ASSERT(isArray());
    fslots[JSSLOT_ARRAY_COUNT] = JSVAL_VOID;
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
    if (this->isNative()) {
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
