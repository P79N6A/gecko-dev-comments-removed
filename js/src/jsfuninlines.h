






































#ifndef jsfuninlines_h___
#define jsfuninlines_h___

#include "jsfun.h"
#include "jsscript.h"

#include "vm/GlobalObject.h"

inline bool
js::IsConstructing(CallReceiver call)
{
    return IsConstructing(call.base());
}

inline bool
JSFunction::inStrictMode() const
{
    return script()->strictModeCode;
}

inline void
JSFunction::setJoinable()
{
    JS_ASSERT(isInterpreted());
    setSlot(METHOD_ATOM_SLOT, js::NullValue());
    flags |= JSFUN_JOINABLE;
}

inline void
JSFunction::setMethodAtom(JSAtom *atom)
{
    JS_ASSERT(joinable());
    setSlot(METHOD_ATOM_SLOT, js::StringValue(atom));
}

inline JSObject *
CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent,
                    bool ignoreSingletonClone )
{
    JS_ASSERT(parent);
    JSObject *proto = parent->getGlobal()->getOrCreateFunctionPrototype(cx);
    if (!proto)
        return NULL;

    






    if (ignoreSingletonClone && fun->hasSingletonType()) {
        JS_ASSERT(fun->getProto() == proto);
        fun->setParent(parent);
        return fun;
    }

    return js_CloneFunctionObject(cx, fun, parent, proto);
}

#endif 
