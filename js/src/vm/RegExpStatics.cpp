






#include "vm/RegExpStatics.h"

#include "jsobjinlines.h"

#include "vm/RegExpObject-inl.h"
#include "vm/RegExpStatics-inl.h"

using namespace js;








static void
resc_finalize(FreeOp *fop, RawObject obj)
{
    RegExpStatics *res = static_cast<RegExpStatics *>(obj->getPrivate());
    fop->delete_(res);
}

static void
resc_trace(JSTracer *trc, RawObject obj)
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

bool
RegExpStatics::executeLazy(JSContext *cx)
{
    if (!pendingLazyEvaluation)
        return true;

    JS_ASSERT(regexpGuard.initialized());
    JS_ASSERT(matchesInput);
    JS_ASSERT(lastIndex != size_t(-1));

    




    size_t length = matchesInput->length();
    StableCharPtr chars(matchesInput->chars(), length);

    
    RegExpRunStatus status = regexpGuard->execute(cx, chars, length, &this->lastIndex, this->matches);
    if (status == RegExpRunStatus_Error)
        return false;

    
    pendingLazyEvaluation = false;
    regexpGuard.release();
    lastIndex = size_t(-1);

    return true;
}
