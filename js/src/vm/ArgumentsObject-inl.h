





#ifndef vm_ArgumentsObject_inl_h
#define vm_ArgumentsObject_inl_h

#include "vm/ArgumentsObject.h"

#include "vm/ScopeObject.h"

#include "jsscriptinlines.h"

#include "vm/ScopeObject-inl.h"

namespace js {

inline const Value &
ArgumentsObject::element(uint32_t i) const
{
    MOZ_ASSERT(!isElementDeleted(i));
    const Value &v = data()->args[i];
    if (IsMagicScopeSlotValue(v)) {
        CallObject &callobj = getFixedSlot(MAYBE_CALL_SLOT).toObject().as<CallObject>();
        return callobj.aliasedVarFromArguments(v);
    }
    return v;
}

inline void
ArgumentsObject::setElement(JSContext *cx, uint32_t i, const Value &v)
{
    MOZ_ASSERT(!isElementDeleted(i));
    HeapValue &lhs = data()->args[i];
    if (IsMagicScopeSlotValue(lhs)) {
        uint32_t slot = SlotFromMagicScopeSlotValue(lhs);
        CallObject &callobj = getFixedSlot(MAYBE_CALL_SLOT).toObject().as<CallObject>();
        for (Shape::Range<NoGC> r(callobj.lastProperty()); !r.empty(); r.popFront()) {
            if (r.front().slot() == slot) {
                callobj.setAliasedVarFromArguments(cx, lhs, r.front().propid(), v);
                return;
            }
        }
        MOZ_CRASH("Bad Arguments::setElement");
    }
    lhs = v;
}

inline bool
ArgumentsObject::maybeGetElements(uint32_t start, uint32_t count, Value *vp)
{
    MOZ_ASSERT(start + count >= start);

    uint32_t length = initialLength();
    if (start > length || start + count > length || isAnyElementDeleted())
        return false;

    for (uint32_t i = start, end = start + count; i < end; ++i, ++vp)
        *vp = element(i);
    return true;
}

} 

#endif 
