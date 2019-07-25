






































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

JS_FRIEND_API(JSObject *)
JS_GetFrameScopeChainRaw(JSStackFrame *fp)
{
    return &Valueify(fp)->scopeChain();
}






extern size_t sE4XObjectsCreated;

JS_FRIEND_API(size_t)
JS_GetE4XObjectsCreated(JSContext *)
{
  return sE4XObjectsCreated;
}

extern size_t sSetProtoCalled;

JS_FRIEND_API(size_t)
JS_SetProtoCalled(JSContext *)
{
  return sSetProtoCalled;
}

extern size_t sCustomIteratorCount;

JS_FRIEND_API(size_t)
JS_GetCustomIteratorCount(JSContext *cx)
{
  return sCustomIteratorCount;
}
