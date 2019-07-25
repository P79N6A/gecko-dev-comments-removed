






































#include "jscntxt.h"
#include "jscompartment.h"
#include "jsfriendapi.h"

#include "jsobjinlines.h"

using namespace js;

JS_FRIEND_API(JSString *)
JS_GetAnonymousString(JSRuntime *rt)
{
    JS_ASSERT(rt->state == JSRTS_UP);
    return rt->atomState.anonymousAtom;
}

JS_FRIEND_API(JSObject *)
JS_FindCompilationScope(JSContext *cx, JSObject *obj)
{
    



    if (obj->isWrapper())
        obj = obj->unwrap();
    
    



    if (JSObjectOp op = obj->getClass()->ext.innerObject)
        obj = op(cx, obj);
    return obj;
}

JS_FRIEND_API(JSObject *)
JS_UnwrapObject(JSObject *obj)
{
    return obj->unwrap();
}

JS_FRIEND_API(JSObject *)
JS_GetFrameScopeChainRaw(JSStackFrame *fp)
{
    return &Valueify(fp)->scopeChain();
}

JS_FRIEND_API(void)
JS_SplicePrototype(JSContext *cx, JSObject *obj, JSObject *proto)
{
    




    CHECK_REQUEST(cx);
    obj->getType()->splicePrototype(cx, proto);
}

JS_FRIEND_API(JSObject *)
JS_NewObjectWithUniqueType(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent)
{
    JSObject *obj = JS_NewObject(cx, clasp, proto, parent);
    if (!obj)
        return NULL;

    types::TypeObject *type = cx->compartment->types.newTypeObject(cx, NULL, "Unique", "",
                                                                   false, false, proto);
    if (!type)
        return NULL;
    if (obj->hasSpecialEquality())
        types::MarkTypeObjectFlags(cx, type, types::OBJECT_FLAG_SPECIAL_EQUALITY);
    if (!obj->setTypeAndUniqueShape(cx, type))
        return NULL;
    type->singleton = obj;

    return obj;
}
