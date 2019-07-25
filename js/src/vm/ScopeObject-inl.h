







































#ifndef ScopeObject_inl_h___
#define ScopeObject_inl_h___

#include "ScopeObject.h"

namespace js {

inline JSObject &
ScopeObject::enclosingScope() const
{
    return getReservedSlot(SCOPE_CHAIN_SLOT).toObject();
}

inline bool
ScopeObject::setEnclosingScope(JSContext *cx, JSObject &obj)
{
    if (!obj.setDelegate(cx))
        return false;
    setFixedSlot(SCOPE_CHAIN_SLOT, ObjectValue(obj));
    return true;
}

inline StackFrame *
ScopeObject::maybeStackFrame() const
{
    JS_ASSERT(!isStaticBlock());
    return reinterpret_cast<StackFrame *>(JSObject::getPrivate());
}

inline void
ScopeObject::setStackFrame(StackFrame *frame)
{
    return setPrivate(frame);
}

 inline size_t
ScopeObject::offsetOfEnclosingScope()
{
    return getFixedSlotOffset(SCOPE_CHAIN_SLOT);
}

inline bool
CallObject::isForEval() const
{
    JS_ASSERT(getReservedSlot(CALLEE_SLOT).isObjectOrNull());
    JS_ASSERT_IF(getReservedSlot(CALLEE_SLOT).isObject(),
                 getReservedSlot(CALLEE_SLOT).toObject().isFunction());
    return getReservedSlot(CALLEE_SLOT).isNull();
}

inline void
CallObject::setCallee(JSObject *callee)
{
    JS_ASSERT_IF(callee, callee->isFunction());
    setFixedSlot(CALLEE_SLOT, ObjectOrNullValue(callee));
}

inline JSObject *
CallObject::getCallee() const
{
    return getReservedSlot(CALLEE_SLOT).toObjectOrNull();
}

inline JSFunction *
CallObject::getCalleeFunction() const
{
    return getReservedSlot(CALLEE_SLOT).toObject().toFunction();
}

inline const Value &
CallObject::arguments() const
{
    JS_ASSERT(!isForEval());
    return getReservedSlot(ARGUMENTS_SLOT);
}

inline void
CallObject::setArguments(const Value &v)
{
    JS_ASSERT(!isForEval());
    setFixedSlot(ARGUMENTS_SLOT, v);
}

inline const Value &
CallObject::arg(unsigned i) const
{
    JS_ASSERT(i < getCalleeFunction()->nargs);
    return getSlot(RESERVED_SLOTS + i);
}

inline void
CallObject::setArg(unsigned i, const Value &v)
{
    JS_ASSERT(i < getCalleeFunction()->nargs);
    setSlot(RESERVED_SLOTS + i, v);
}

inline void
CallObject::initArgUnchecked(unsigned i, const Value &v)
{
    JS_ASSERT(i < getCalleeFunction()->nargs);
    initSlotUnchecked(RESERVED_SLOTS + i, v);
}

inline const Value &
CallObject::var(unsigned i) const
{
    JSFunction *fun = getCalleeFunction();
    JS_ASSERT(fun->nargs == fun->script()->bindings.countArgs());
    JS_ASSERT(i < fun->script()->bindings.countVars());
    return getSlot(RESERVED_SLOTS + fun->nargs + i);
}

inline void
CallObject::setVar(unsigned i, const Value &v)
{
    JSFunction *fun = getCalleeFunction();
    JS_ASSERT(fun->nargs == fun->script()->bindings.countArgs());
    JS_ASSERT(i < fun->script()->bindings.countVars());
    setSlot(RESERVED_SLOTS + fun->nargs + i, v);
}

inline void
CallObject::initVarUnchecked(unsigned i, const Value &v)
{
    JSFunction *fun = getCalleeFunction();
    JS_ASSERT(fun->nargs == fun->script()->bindings.countArgs());
    JS_ASSERT(i < fun->script()->bindings.countVars());
    initSlotUnchecked(RESERVED_SLOTS + fun->nargs + i, v);
}

inline void
CallObject::copyValues(unsigned nargs, Value *argv, unsigned nvars, Value *slots)
{
    JS_ASSERT(slotInRange(RESERVED_SLOTS + nargs + nvars, SENTINEL_ALLOWED));
    copySlotRange(RESERVED_SLOTS, argv, nargs);
    copySlotRange(RESERVED_SLOTS + nargs, slots, nvars);
}

inline HeapSlotArray
CallObject::argArray()
{
    DebugOnly<JSFunction*> fun = getCalleeFunction();
    JS_ASSERT(hasContiguousSlots(RESERVED_SLOTS, fun->nargs));
    return HeapSlotArray(getSlotAddress(RESERVED_SLOTS));
}

inline HeapSlotArray
CallObject::varArray()
{
    JSFunction *fun = getCalleeFunction();
    JS_ASSERT(hasContiguousSlots(RESERVED_SLOTS + fun->nargs,
                                 fun->script()->bindings.countVars()));
    return HeapSlotArray(getSlotAddress(RESERVED_SLOTS + fun->nargs));
}

inline uint32_t
NestedScopeObject::stackDepth() const
{
    return getReservedSlot(DEPTH_SLOT).toPrivateUint32();
}

inline JSObject &
WithObject::withThis() const
{
    return getReservedSlot(THIS_SLOT).toObject();
}

inline JSObject &
WithObject::object() const
{
    return *JSObject::getProto();
}

inline uint32_t
BlockObject::slotCount() const
{
    return propertyCount();
}

inline HeapSlot &
BlockObject::slotValue(unsigned i)
{
    JS_ASSERT(i < slotCount());
    return getSlotRef(RESERVED_SLOTS + i);
}

inline StaticBlockObject *
StaticBlockObject::enclosingBlock() const
{
    JSObject *obj = getReservedSlot(SCOPE_CHAIN_SLOT).toObjectOrNull();
    return obj ? &obj->asStaticBlock() : NULL;
}

inline void
StaticBlockObject::setEnclosingBlock(StaticBlockObject *blockObj)
{
    setFixedSlot(SCOPE_CHAIN_SLOT, ObjectOrNullValue(blockObj));
}

inline void
StaticBlockObject::setStackDepth(uint32_t depth)
{
    JS_ASSERT(getReservedSlot(DEPTH_SLOT).isUndefined());
    initReservedSlot(DEPTH_SLOT, PrivateUint32Value(depth));
}

inline void
StaticBlockObject::setDefinitionParseNode(unsigned i, Definition *def)
{
    JS_ASSERT(slotValue(i).isUndefined());
    slotValue(i).init(this, i, PrivateValue(def));
}

inline Definition *
StaticBlockObject::maybeDefinitionParseNode(unsigned i)
{
    Value v = slotValue(i);
    return v.isUndefined() ? NULL : reinterpret_cast<Definition *>(v.toPrivate());
}

inline void
StaticBlockObject::setAliased(unsigned i, bool aliased)
{
    slotValue(i).init(this, i, BooleanValue(aliased));
}

inline bool
StaticBlockObject::isAliased(unsigned i)
{
    return slotValue(i).isTrue();
}

inline StaticBlockObject &
ClonedBlockObject::staticBlock() const
{
    return getProto()->asStaticBlock();
}

inline const Value &
ClonedBlockObject::closedSlot(unsigned i)
{
    JS_ASSERT(!maybeStackFrame());
    return slotValue(i);
}

}  

inline js::ScopeObject &
JSObject::asScope()
{
    JS_ASSERT(isScope());
    return *static_cast<js::ScopeObject *>(this);
}

inline js::CallObject &
JSObject::asCall()
{
    JS_ASSERT(isCall());
    return *static_cast<js::CallObject *>(this);
}

inline js::DeclEnvObject &
JSObject::asDeclEnv()
{
    JS_ASSERT(isDeclEnv());
    return *static_cast<js::DeclEnvObject *>(this);
}

inline js::NestedScopeObject &
JSObject::asNestedScope()
{
    JS_ASSERT(isWith() || isBlock());
    return *static_cast<js::NestedScopeObject *>(this);
}

inline js::WithObject &
JSObject::asWith()
{
    JS_ASSERT(isWith());
    return *static_cast<js::WithObject *>(this);
}

inline js::BlockObject &
JSObject::asBlock()
{
    JS_ASSERT(isBlock());
    return *static_cast<js::BlockObject *>(this);
}

inline js::StaticBlockObject &
JSObject::asStaticBlock()
{
    JS_ASSERT(isStaticBlock());
    return *static_cast<js::StaticBlockObject *>(this);
}

inline js::ClonedBlockObject &
JSObject::asClonedBlock()
{
    JS_ASSERT(isClonedBlock());
    return *static_cast<js::ClonedBlockObject *>(this);
}

#endif 
