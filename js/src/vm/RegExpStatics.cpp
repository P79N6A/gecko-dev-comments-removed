






#include "vm/RegExpStatics.h"

#include "jsobjinlines.h"

#include "vm/RegExpStatics-inl.h"

using namespace js;








static void
resc_finalize(FreeOp *fop, JSObject *obj)
{
    RegExpStatics *res = static_cast<RegExpStatics *>(obj->getPrivate());
    fop->delete_(res);
}

static void
resc_trace(JSTracer *trc, JSObject *obj)
{
    void *pdata = obj->getPrivate();
    JS_ASSERT(pdata);
    RegExpStatics *res = static_cast<RegExpStatics *>(pdata);
    res->mark(trc);
}

Class js::RegExpStaticsClass = {
    "RegExpStatics",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    resc_finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    resc_trace
};

JSObject *
RegExpStatics::create(JSContext *cx, GlobalObject *parent)
{
    JSObject *obj = NewObjectWithGivenProto(cx, &RegExpStaticsClass, NULL, parent);
    if (!obj)
        return NULL;
    RegExpStatics *res = cx->new_<RegExpStatics>();
    if (!res)
        return NULL;
    obj->setPrivate(static_cast<void *>(res));
    return obj;
}
