






































#include "jscntxt.h"
#include "jscompartment.h"
#include "jsfriendapi.h"

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
