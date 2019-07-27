





#include "vm/RegExpStatics.h"

#include "vm/RegExpStaticsObject.h"

#include "vm/NativeObject-inl.h"

using namespace js;








static void
resc_finalize(FreeOp* fop, JSObject* obj)
{
    RegExpStatics* res = static_cast<RegExpStatics*>(obj->as<RegExpStaticsObject>().getPrivate());
    fop->delete_(res);
}

static void
resc_trace(JSTracer* trc, JSObject* obj)
{
    void* pdata = obj->as<RegExpStaticsObject>().getPrivate();
    MOZ_ASSERT(pdata);
    RegExpStatics* res = static_cast<RegExpStatics*>(pdata);
    res->mark(trc);
}

const Class RegExpStaticsObject::class_ = {
    "RegExpStatics",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    resc_finalize,
    nullptr, 
    nullptr, 
    nullptr, 
    resc_trace
};

RegExpStaticsObject*
RegExpStatics::create(ExclusiveContext* cx, Handle<GlobalObject*> parent)
{
    RegExpStaticsObject* obj = NewObjectWithGivenProto<RegExpStaticsObject>(cx, NullPtr());
    if (!obj)
        return nullptr;
    RegExpStatics* res = cx->new_<RegExpStatics>();
    if (!res)
        return nullptr;
    obj->setPrivate(static_cast<void*>(res));
    return obj;
}

void
RegExpStatics::markFlagsSet(JSContext* cx)
{
    
    
    
    
    
    
    MOZ_ASSERT_IF(cx->global()->hasRegExpStatics(), this == cx->global()->getRegExpStatics(cx));

    MarkObjectGroupFlags(cx, cx->global(), OBJECT_FLAG_REGEXP_FLAGS_SET);
}

bool
RegExpStatics::executeLazy(JSContext* cx)
{
    if (!pendingLazyEvaluation)
        return true;

    MOZ_ASSERT(lazySource);
    MOZ_ASSERT(matchesInput);
    MOZ_ASSERT(lazyIndex != size_t(-1));

    
    RegExpGuard g(cx);
    if (!cx->compartment()->regExps.get(cx, lazySource, lazyFlags, &g))
        return false;

    




    
    RootedLinearString input(cx, matchesInput);
    RegExpRunStatus status = g->execute(cx, input, lazyIndex, &this->matches);
    if (status == RegExpRunStatus_Error)
        return false;

    



    MOZ_ASSERT(status == RegExpRunStatus_Success);

    
    pendingLazyEvaluation = false;
    lazySource = nullptr;
    lazyIndex = size_t(-1);

    return true;
}
