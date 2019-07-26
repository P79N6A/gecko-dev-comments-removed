





#ifndef vm_ArgumentsObject_inl_h
#define vm_ArgumentsObject_inl_h

#include "vm/ArgumentsObject.h"

#include "mozilla/MemoryReporting.h"

#include "vm/ScopeObject.h"
#include "vm/ScopeObject-inl.h"

namespace js {

inline const Value &
ArgumentsObject::element(uint32_t i) const
{
    JS_ASSERT(!isElementDeleted(i));
    const Value &v = data()->args[i];
    if (v.isMagic(JS_FORWARD_TO_CALL_OBJECT)) {
        CallObject &callobj = getFixedSlot(MAYBE_CALL_SLOT).toObject().as<CallObject>();
        for (AliasedFormalIter fi(callobj.callee().nonLazyScript()); ; fi++) {
            if (fi.frameIndex() == i)
                return callobj.aliasedVar(fi);
        }
    }
    return v;
}

inline void
ArgumentsObject::setElement(JSContext *cx, uint32_t i, const Value &v)
{
    JS_ASSERT(!isElementDeleted(i));
    HeapValue &lhs = data()->args[i];
    if (lhs.isMagic(JS_FORWARD_TO_CALL_OBJECT)) {
        CallObject &callobj = getFixedSlot(MAYBE_CALL_SLOT).toObject().as<CallObject>();
        for (AliasedFormalIter fi(callobj.callee().nonLazyScript()); ; fi++) {
            if (fi.frameIndex() == i) {
                callobj.setAliasedVar(cx, fi, fi->name(), v);
                return;
            }
        }
    }
    lhs = v;
}

inline bool
ArgumentsObject::maybeGetElements(uint32_t start, uint32_t count, Value *vp)
{
    JS_ASSERT(start + count >= start);

    uint32_t length = initialLength();
    if (start > length || start + count > length || isAnyElementDeleted())
        return false;

    for (uint32_t i = start, end = start + count; i < end; ++i, ++vp)
        *vp = element(i);
    return true;
}

} 

#endif 
