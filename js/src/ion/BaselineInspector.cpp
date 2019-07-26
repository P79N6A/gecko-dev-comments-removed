






#include "BaselineIC.h"
#include "BaselineInspector.h"

using namespace js;
using namespace js::ion;

RawShape
BaselineInspector::maybeMonomorphicShapeForPropertyOp(jsbytecode *pc)
{
    if (!hasBaselineScript())
        return NULL;

    JS_ASSERT(isValidPC(pc));
    const ICEntry &entry = icEntryFromPC(pc);

    ICStub *stub = entry.firstStub();
    ICStub *next = stub->next();

    if (!next || !next->isFallback())
        return NULL;

    if (stub->isGetProp_Native()) {
        JS_ASSERT(next->isGetProp_Fallback());
        if (next->toGetProp_Fallback()->hadUnoptimizableAccess())
            return NULL;
        return stub->toGetProp_Native()->shape();
    }

    if (stub->isSetProp_Native()) {
        JS_ASSERT(next->isSetProp_Fallback());
        if (next->toSetProp_Fallback()->hadUnoptimizableAccess())
            return NULL;
        return stub->toSetProp_Native()->shape();
    }

    return NULL;
}
