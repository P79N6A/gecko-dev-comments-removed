






#include "BaselineIC.h"
#include "BaselineInspector.h"

using namespace js;
using namespace js::ion;

bool
SetElemICInspector::sawOOBTypedArrayWrite() const
{
    if (!icEntry_)
        return false;

    
    for (ICStub *stub = icEntry_->firstStub(); stub; stub = stub->next()) {
        if (!stub->isSetElem_TypedArray())
            continue;
        if (stub->toSetElem_TypedArray()->expectOutOfBounds())
            return true;
    }
    return false;
}

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

MIRType
BaselineInspector::expectedResultType(jsbytecode *pc)
{
    
    

    if (!hasBaselineScript())
        return MIRType_None;

    const ICEntry &entry = icEntryFromPC(pc);

    ICStub *stub = entry.firstStub();
    ICStub *next = stub->next();

    if (!next || !next->isFallback())
        return MIRType_None;

    switch (stub->kind()) {
      case ICStub::BinaryArith_Int32:
      case ICStub::BinaryArith_BooleanWithInt32:
      case ICStub::UnaryArith_Int32:
        return MIRType_Int32;
      case ICStub::BinaryArith_Double:
      case ICStub::BinaryArith_DoubleWithInt32:
      case ICStub::UnaryArith_Double:
        return MIRType_Double;
      case ICStub::BinaryArith_StringConcat:
      case ICStub::BinaryArith_StringObjectConcat:
        return MIRType_String;
      default:
        return MIRType_None;
    }
}
