





#include "jit/BaselineInspector.h"

#include "mozilla/DebugOnly.h"

#include "jit/BaselineIC.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;

bool
SetElemICInspector::sawOOBDenseWrite() const
{
    if (!icEntry_)
        return false;

    
    for (ICStub *stub = icEntry_->firstStub(); stub; stub = stub->next()) {
        if (stub->isSetElem_DenseAdd())
            return true;
    }

    
    ICStub *stub = icEntry_->fallbackStub();
    if (stub->isSetElem_Fallback())
        return stub->toSetElem_Fallback()->hasArrayWriteHole();

    return false;
}

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

bool
SetElemICInspector::sawDenseWrite() const
{
    if (!icEntry_)
        return false;

    
    for (ICStub *stub = icEntry_->firstStub(); stub; stub = stub->next()) {
        if (stub->isSetElem_DenseAdd() || stub->isSetElem_Dense())
            return true;
    }
    return false;
}

bool
SetElemICInspector::sawTypedArrayWrite() const
{
    if (!icEntry_)
        return false;

    
    for (ICStub *stub = icEntry_->firstStub(); stub; stub = stub->next()) {
        if (stub->isSetElem_TypedArray())
            return true;
    }
    return false;
}

bool
BaselineInspector::maybeShapesForPropertyOp(jsbytecode *pc, ShapeVector &shapes)
{
    
    
    
    JS_ASSERT(shapes.empty());

    if (!hasBaselineScript())
        return true;

    JS_ASSERT(isValidPC(pc));
    const ICEntry &entry = icEntryFromPC(pc);

    ICStub *stub = entry.firstStub();
    while (stub->next()) {
        Shape *shape;
        if (stub->isGetProp_Native()) {
            shape = stub->toGetProp_Native()->shape();
        } else if (stub->isSetProp_Native()) {
            shape = stub->toSetProp_Native()->shape();
        } else {
            shapes.clear();
            return true;
        }

        
        
        bool found = false;
        for (size_t i = 0; i < shapes.length(); i++) {
            if (shapes[i] == shape) {
                found = true;
                break;
            }
        }

        if (!found && !shapes.append(shape))
            return false;

        stub = stub->next();
    }

    if (stub->isGetProp_Fallback()) {
        if (stub->toGetProp_Fallback()->hadUnoptimizableAccess())
            shapes.clear();
    } else {
        if (stub->toSetProp_Fallback()->hadUnoptimizableAccess())
            shapes.clear();
    }

    
    if (shapes.length() > 5)
        shapes.clear();

    return true;
}

ICStub *
BaselineInspector::monomorphicStub(jsbytecode *pc)
{
    if (!hasBaselineScript())
        return nullptr;

    const ICEntry &entry = icEntryFromPC(pc);

    ICStub *stub = entry.firstStub();
    ICStub *next = stub->next();

    if (!next || !next->isFallback())
        return nullptr;

    return stub;
}

bool
BaselineInspector::dimorphicStub(jsbytecode *pc, ICStub **pfirst, ICStub **psecond)
{
    if (!hasBaselineScript())
        return false;

    const ICEntry &entry = icEntryFromPC(pc);

    ICStub *stub = entry.firstStub();
    ICStub *next = stub->next();
    ICStub *after = next ? next->next() : nullptr;

    if (!after || !after->isFallback())
        return false;

    *pfirst = stub;
    *psecond = next;
    return true;
}

MIRType
BaselineInspector::expectedResultType(jsbytecode *pc)
{
    
    

    ICStub *stub = monomorphicStub(pc);
    if (!stub)
        return MIRType_None;

    switch (stub->kind()) {
      case ICStub::BinaryArith_Int32:
        if (stub->toBinaryArith_Int32()->allowDouble())
            return MIRType_Double;
        return MIRType_Int32;
      case ICStub::BinaryArith_BooleanWithInt32:
      case ICStub::UnaryArith_Int32:
      case ICStub::BinaryArith_DoubleWithInt32:
        return MIRType_Int32;
      case ICStub::BinaryArith_Double:
      case ICStub::UnaryArith_Double:
        return MIRType_Double;
      case ICStub::BinaryArith_StringConcat:
      case ICStub::BinaryArith_StringObjectConcat:
        return MIRType_String;
      default:
        return MIRType_None;
    }
}



static bool
CanUseDoubleCompare(ICStub::Kind kind)
{
    return kind == ICStub::Compare_Double || kind == ICStub::Compare_NumberWithUndefined;
}



static bool
CanUseInt32Compare(ICStub::Kind kind)
{
    return kind == ICStub::Compare_Int32 || kind == ICStub::Compare_Int32WithBoolean;
}

MCompare::CompareType
BaselineInspector::expectedCompareType(jsbytecode *pc)
{
    ICStub *first = monomorphicStub(pc), *second = nullptr;
    if (!first && !dimorphicStub(pc, &first, &second))
        return MCompare::Compare_Unknown;

    if (CanUseInt32Compare(first->kind()) && (!second || CanUseInt32Compare(second->kind())))
        return MCompare::Compare_Int32;

    if (CanUseDoubleCompare(first->kind()) && (!second || CanUseDoubleCompare(second->kind()))) {
        ICCompare_NumberWithUndefined *coerce =
            first->isCompare_NumberWithUndefined()
            ? first->toCompare_NumberWithUndefined()
            : (second && second->isCompare_NumberWithUndefined())
              ? second->toCompare_NumberWithUndefined()
              : nullptr;
        if (coerce) {
            return coerce->lhsIsUndefined()
                   ? MCompare::Compare_DoubleMaybeCoerceLHS
                   : MCompare::Compare_DoubleMaybeCoerceRHS;
        }
        return MCompare::Compare_Double;
    }

    return MCompare::Compare_Unknown;
}

static bool
TryToSpecializeBinaryArithOp(ICStub **stubs,
                             uint32_t nstubs,
                             MIRType *result)
{
    DebugOnly<bool> sawInt32 = false;
    bool sawDouble = false;
    bool sawOther = false;

    for (uint32_t i = 0; i < nstubs; i++) {
        switch (stubs[i]->kind()) {
          case ICStub::BinaryArith_Int32:
            sawInt32 = true;
            break;
          case ICStub::BinaryArith_BooleanWithInt32:
            sawInt32 = true;
            break;
          case ICStub::BinaryArith_Double:
            sawDouble = true;
            break;
          case ICStub::BinaryArith_DoubleWithInt32:
            sawDouble = true;
            break;
          default:
            sawOther = true;
            break;
        }
    }

    if (sawOther)
        return false;

    if (sawDouble) {
        *result = MIRType_Double;
        return true;
    }

    JS_ASSERT(sawInt32);
    *result = MIRType_Int32;
    return true;
}

MIRType
BaselineInspector::expectedBinaryArithSpecialization(jsbytecode *pc)
{
    MIRType result;
    ICStub *stubs[2];

    stubs[0] = monomorphicStub(pc);
    if (stubs[0]) {
        if (TryToSpecializeBinaryArithOp(stubs, 1, &result))
            return result;
    }

    if (dimorphicStub(pc, &stubs[0], &stubs[1])) {
        if (TryToSpecializeBinaryArithOp(stubs, 2, &result))
            return result;
    }

    return MIRType_None;
}

bool
BaselineInspector::hasSeenNonNativeGetElement(jsbytecode *pc)
{
    if (!hasBaselineScript())
        return false;

    const ICEntry &entry = icEntryFromPC(pc);
    ICStub *stub = entry.fallbackStub();

    if (stub->isGetElem_Fallback())
        return stub->toGetElem_Fallback()->hasNonNativeAccess();
    return false;
}

bool
BaselineInspector::hasSeenNegativeIndexGetElement(jsbytecode *pc)
{
    if (!hasBaselineScript())
        return false;

    const ICEntry &entry = icEntryFromPC(pc);
    ICStub *stub = entry.fallbackStub();

    if (stub->isGetElem_Fallback())
        return stub->toGetElem_Fallback()->hasNegativeIndex();
    return false;
}

bool
BaselineInspector::hasSeenAccessedGetter(jsbytecode *pc)
{
    if (!hasBaselineScript())
        return false;

    const ICEntry &entry = icEntryFromPC(pc);
    ICStub *stub = entry.fallbackStub();

    if (stub->isGetProp_Fallback())
        return stub->toGetProp_Fallback()->hasAccessedGetter();
    return false;
}

bool
BaselineInspector::hasSeenDoubleResult(jsbytecode *pc)
{
    if (!hasBaselineScript())
        return false;

    const ICEntry &entry = icEntryFromPC(pc);
    ICStub *stub = entry.fallbackStub();

    JS_ASSERT(stub->isUnaryArith_Fallback() || stub->isBinaryArith_Fallback());

    if (stub->isUnaryArith_Fallback())
        return stub->toUnaryArith_Fallback()->sawDoubleResult();
    else
        return stub->toBinaryArith_Fallback()->sawDoubleResult();

    return false;
}
