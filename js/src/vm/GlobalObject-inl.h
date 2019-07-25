







































#ifndef GlobalObject_inl_h___
#define GlobalObject_inl_h___

namespace js {

inline void
GlobalObject::setFlags(int32 flags)
{
    setSlot(FLAGS, Int32Value(flags));
}

inline void
GlobalObject::initFlags(int32 flags)
{
    initSlot(FLAGS, Int32Value(flags));
}

inline void
GlobalObject::setDetailsForKey(JSProtoKey key, JSObject *ctor, JSObject *proto)
{
    HeapValue &ctorVal = getSlotRef(key);
    HeapValue &protoVal = getSlotRef(JSProto_LIMIT + key);
    HeapValue &visibleVal = getSlotRef(2 * JSProto_LIMIT + key);
    JS_ASSERT(ctorVal.isUndefined());
    JS_ASSERT(protoVal.isUndefined());
    JS_ASSERT(visibleVal.isUndefined());
    ctorVal = ObjectValue(*ctor);
    protoVal = ObjectValue(*proto);
    visibleVal = ctorVal;
}

inline void
GlobalObject::setObjectClassDetails(JSFunction *ctor, JSObject *proto)
{
    setDetailsForKey(JSProto_Object, ctor, proto);
}

inline void
GlobalObject::setFunctionClassDetails(JSFunction *ctor, JSObject *proto)
{
    setDetailsForKey(JSProto_Function, ctor, proto);
}

void
GlobalObject::setThrowTypeError(JSFunction *fun)
{
    HeapValue &v = getSlotRef(THROWTYPEERROR);
    JS_ASSERT(v.isUndefined());
    v = ObjectValue(*fun);
}

void
GlobalObject::setOriginalEval(JSObject *evalobj)
{
    HeapValue &v = getSlotRef(EVAL);
    JS_ASSERT(v.isUndefined());
    v = ObjectValue(*evalobj);
}

} 

#endif
