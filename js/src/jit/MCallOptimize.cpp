





#include "jsmath.h"

#include "builtin/AtomicsObject.h"
#include "builtin/SIMD.h"
#include "builtin/TestingFunctions.h"
#include "builtin/TypedObject.h"
#include "jit/BaselineInspector.h"
#include "jit/IonBuilder.h"
#include "jit/Lowering.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"
#include "vm/ArgumentsObject.h"

#include "jsscriptinlines.h"

#include "vm/NativeObject-inl.h"
#include "vm/StringObject-inl.h"
#include "vm/UnboxedObject-inl.h"

using mozilla::ArrayLength;

using JS::DoubleNaNValue;
using JS::TrackedOutcome;
using JS::TrackedStrategy;
using JS::TrackedTypeSite;

namespace js {
namespace jit {

IonBuilder::InliningStatus
IonBuilder::inlineNativeCall(CallInfo& callInfo, JSFunction* target)
{
    MOZ_ASSERT(target->isNative());
    JSNative native = target->native();

    if (!optimizationInfo().inlineNative()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineDisabledIon);
        return InliningStatus_NotInlined;
    }

    
    trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadType);

    if (shouldAbortOnPreliminaryGroups(callInfo.thisArg()))
        return InliningStatus_NotInlined;
    for (size_t i = 0; i < callInfo.argc(); i++) {
        if (shouldAbortOnPreliminaryGroups(callInfo.getArg(i)))
            return InliningStatus_NotInlined;
    }

    
    if (native == atomics_compareExchange)
        return inlineAtomicsCompareExchange(callInfo);
    if (native == atomics_load)
        return inlineAtomicsLoad(callInfo);
    if (native == atomics_store)
        return inlineAtomicsStore(callInfo);
    if (native == atomics_fence)
        return inlineAtomicsFence(callInfo);
    if (native == atomics_add ||
        native == atomics_sub ||
        native == atomics_and ||
        native == atomics_or ||
        native == atomics_xor)
    {
        return inlineAtomicsBinop(callInfo, target);
    }

    
    if (native == ArrayConstructor)
        return inlineArray(callInfo);
    if (native == js::array_pop)
        return inlineArrayPopShift(callInfo, MArrayPopShift::Pop);
    if (native == js::array_shift)
        return inlineArrayPopShift(callInfo, MArrayPopShift::Shift);
    if (native == js::array_push)
        return inlineArrayPush(callInfo);
    if (native == js::array_concat)
        return inlineArrayConcat(callInfo);
    if (native == js::array_slice)
        return inlineArraySlice(callInfo);
    if (native == js::array_splice)
        return inlineArraySplice(callInfo);

    
    if (native == js::math_abs)
        return inlineMathAbs(callInfo);
    if (native == js::math_floor)
        return inlineMathFloor(callInfo);
    if (native == js::math_ceil)
        return inlineMathCeil(callInfo);
    if (native == js::math_clz32)
        return inlineMathClz32(callInfo);
    if (native == js::math_round)
        return inlineMathRound(callInfo);
    if (native == js::math_sqrt)
        return inlineMathSqrt(callInfo);
    if (native == js::math_atan2)
        return inlineMathAtan2(callInfo);
    if (native == js::math_hypot)
        return inlineMathHypot(callInfo);
    if (native == js::math_max)
        return inlineMathMinMax(callInfo, true );
    if (native == js::math_min)
        return inlineMathMinMax(callInfo, false );
    if (native == js::math_pow)
        return inlineMathPow(callInfo);
    if (native == js::math_random)
        return inlineMathRandom(callInfo);
    if (native == js::math_imul)
        return inlineMathImul(callInfo);
    if (native == js::math_fround)
        return inlineMathFRound(callInfo);
    if (native == js::math_sin)
        return inlineMathFunction(callInfo, MMathFunction::Sin);
    if (native == js::math_cos)
        return inlineMathFunction(callInfo, MMathFunction::Cos);
    if (native == js::math_exp)
        return inlineMathFunction(callInfo, MMathFunction::Exp);
    if (native == js::math_tan)
        return inlineMathFunction(callInfo, MMathFunction::Tan);
    if (native == js::math_log)
        return inlineMathFunction(callInfo, MMathFunction::Log);
    if (native == js::math_atan)
        return inlineMathFunction(callInfo, MMathFunction::ATan);
    if (native == js::math_asin)
        return inlineMathFunction(callInfo, MMathFunction::ASin);
    if (native == js::math_acos)
        return inlineMathFunction(callInfo, MMathFunction::ACos);
    if (native == js::math_log10)
        return inlineMathFunction(callInfo, MMathFunction::Log10);
    if (native == js::math_log2)
        return inlineMathFunction(callInfo, MMathFunction::Log2);
    if (native == js::math_log1p)
        return inlineMathFunction(callInfo, MMathFunction::Log1P);
    if (native == js::math_expm1)
        return inlineMathFunction(callInfo, MMathFunction::ExpM1);
    if (native == js::math_cosh)
        return inlineMathFunction(callInfo, MMathFunction::CosH);
    if (native == js::math_sinh)
        return inlineMathFunction(callInfo, MMathFunction::SinH);
    if (native == js::math_tanh)
        return inlineMathFunction(callInfo, MMathFunction::TanH);
    if (native == js::math_acosh)
        return inlineMathFunction(callInfo, MMathFunction::ACosH);
    if (native == js::math_asinh)
        return inlineMathFunction(callInfo, MMathFunction::ASinH);
    if (native == js::math_atanh)
        return inlineMathFunction(callInfo, MMathFunction::ATanH);
    if (native == js::math_sign)
        return inlineMathFunction(callInfo, MMathFunction::Sign);
    if (native == js::math_trunc)
        return inlineMathFunction(callInfo, MMathFunction::Trunc);
    if (native == js::math_cbrt)
        return inlineMathFunction(callInfo, MMathFunction::Cbrt);

    
    if (native == StringConstructor)
        return inlineStringObject(callInfo);
    if (native == str_split)
        return inlineStringSplit(callInfo);
    if (native == str_charCodeAt)
        return inlineStrCharCodeAt(callInfo);
    if (native == str_fromCharCode)
        return inlineStrFromCharCode(callInfo);
    if (native == str_charAt)
        return inlineStrCharAt(callInfo);
    if (native == str_replace)
        return inlineStrReplace(callInfo);

    
    if (native == regexp_exec && CallResultEscapes(pc))
        return inlineRegExpExec(callInfo);
    if (native == regexp_exec && !CallResultEscapes(pc))
        return inlineRegExpTest(callInfo);
    if (native == regexp_test)
        return inlineRegExpTest(callInfo);

    
    if (native == obj_create)
        return inlineObjectCreate(callInfo);
    if (native == intrinsic_DefineDataProperty)
        return inlineDefineDataProperty(callInfo);

    
    if (native == intrinsic_UnsafeSetReservedSlot)
        return inlineUnsafeSetReservedSlot(callInfo);
    if (native == intrinsic_UnsafeGetReservedSlot)
        return inlineUnsafeGetReservedSlot(callInfo, MIRType_Value);
    if (native == intrinsic_UnsafeGetObjectFromReservedSlot)
        return inlineUnsafeGetReservedSlot(callInfo, MIRType_Object);
    if (native == intrinsic_UnsafeGetInt32FromReservedSlot)
        return inlineUnsafeGetReservedSlot(callInfo, MIRType_Int32);
    if (native == intrinsic_UnsafeGetStringFromReservedSlot)
        return inlineUnsafeGetReservedSlot(callInfo, MIRType_String);
    if (native == intrinsic_UnsafeGetBooleanFromReservedSlot)
        return inlineUnsafeGetReservedSlot(callInfo, MIRType_Boolean);

    
    if (native == intrinsic_IsCallable)
        return inlineIsCallable(callInfo);
    if (native == intrinsic_ToObject)
        return inlineToObject(callInfo);
    if (native == intrinsic_IsObject)
        return inlineIsObject(callInfo);
    if (native == intrinsic_ToInteger)
        return inlineToInteger(callInfo);
    if (native == intrinsic_ToString)
        return inlineToString(callInfo);
    if (native == intrinsic_IsConstructing)
        return inlineIsConstructing(callInfo);
    if (native == intrinsic_SubstringKernel)
        return inlineSubstringKernel(callInfo);
    if (native == intrinsic_IsArrayIterator)
        return inlineHasClass(callInfo, &ArrayIteratorObject::class_);
    if (native == intrinsic_IsStringIterator)
        return inlineHasClass(callInfo, &StringIteratorObject::class_);

    
    if (native == intrinsic_IsTypedArray)
        return inlineIsTypedArray(callInfo);
    if (native == intrinsic_IsPossiblyWrappedTypedArray)
        return inlineIsPossiblyWrappedTypedArray(callInfo);
    if (native == intrinsic_TypedArrayLength)
        return inlineTypedArrayLength(callInfo);
    if (native == intrinsic_SetDisjointTypedElements)
        return inlineSetDisjointTypedElements(callInfo);

    
    if (native == js::ObjectIsTypedObject)
        return inlineHasClass(callInfo,
                              &OutlineTransparentTypedObject::class_,
                              &OutlineOpaqueTypedObject::class_,
                              &InlineTransparentTypedObject::class_,
                              &InlineOpaqueTypedObject::class_);
    if (native == js::ObjectIsTransparentTypedObject)
        return inlineHasClass(callInfo,
                              &OutlineTransparentTypedObject::class_,
                              &InlineTransparentTypedObject::class_);
    if (native == js::ObjectIsOpaqueTypedObject)
        return inlineHasClass(callInfo,
                              &OutlineOpaqueTypedObject::class_,
                              &InlineOpaqueTypedObject::class_);
    if (native == js::ObjectIsTypeDescr)
        return inlineObjectIsTypeDescr(callInfo);
    if (native == js::TypeDescrIsSimpleType)
        return inlineHasClass(callInfo,
                              &ScalarTypeDescr::class_, &ReferenceTypeDescr::class_);
    if (native == js::TypeDescrIsArrayType)
        return inlineHasClass(callInfo, &ArrayTypeDescr::class_);
    if (native == js::SetTypedObjectOffset)
        return inlineSetTypedObjectOffset(callInfo);

    
    if (native == testingFunc_bailout)
        return inlineBailout(callInfo);
    if (native == testingFunc_assertFloat32)
        return inlineAssertFloat32(callInfo);
    if (native == testingFunc_assertRecoveredOnBailout)
        return inlineAssertRecoveredOnBailout(callInfo);

    
    if (native == js::CallOrConstructBoundFunction)
        return inlineBoundFunction(callInfo, target);

    
#define INLINE_FLOAT32X4_SIMD_ARITH_(OP)                                                         \
    if (native == js::simd_float32x4_##OP)                                                       \
        return inlineBinarySimd<MSimdBinaryArith>(callInfo, native, MSimdBinaryArith::Op_##OP,   \
                                                  SimdTypeDescr::Float32x4);

#define INLINE_INT32X4_SIMD_ARITH_(OP)                                                           \
    if (native == js::simd_int32x4_##OP)                                                         \
        return inlineBinarySimd<MSimdBinaryArith>(callInfo, native, MSimdBinaryArith::Op_##OP,   \
                                                  SimdTypeDescr::Int32x4);

    ARITH_COMMONX4_SIMD_OP(INLINE_INT32X4_SIMD_ARITH_)
    ARITH_COMMONX4_SIMD_OP(INLINE_FLOAT32X4_SIMD_ARITH_)
    BINARY_ARITH_FLOAT32X4_SIMD_OP(INLINE_FLOAT32X4_SIMD_ARITH_)
#undef INLINE_SIMD_ARITH_
#undef INLINE_FLOAT32X4_SIMD_ARITH_

#define INLINE_SIMD_BITWISE_(OP)                                                                 \
    if (native == js::simd_int32x4_##OP)                                                         \
        return inlineBinarySimd<MSimdBinaryBitwise>(callInfo, native, MSimdBinaryBitwise::OP##_, \
                                                    SimdTypeDescr::Int32x4);                     \
    if (native == js::simd_float32x4_##OP)                                                       \
        return inlineBinarySimd<MSimdBinaryBitwise>(callInfo, native, MSimdBinaryBitwise::OP##_, \
                                                    SimdTypeDescr::Float32x4);

    BITWISE_COMMONX4_SIMD_OP(INLINE_SIMD_BITWISE_)
#undef INLINE_SIMD_BITWISE_

    if (native == js::simd_int32x4_shiftLeftByScalar)
        return inlineBinarySimd<MSimdShift>(callInfo, native, MSimdShift::lsh, SimdTypeDescr::Int32x4);
    if (native == js::simd_int32x4_shiftRightArithmeticByScalar)
        return inlineBinarySimd<MSimdShift>(callInfo, native, MSimdShift::rsh, SimdTypeDescr::Int32x4);
    if (native == js::simd_int32x4_shiftRightLogicalByScalar)
        return inlineBinarySimd<MSimdShift>(callInfo, native, MSimdShift::ursh, SimdTypeDescr::Int32x4);

#define INLINE_SIMD_COMPARISON_(OP)                                                                \
    if (native == js::simd_int32x4_##OP)                                                           \
        return inlineCompSimd(callInfo, native, MSimdBinaryComp::OP, SimdTypeDescr::Int32x4);      \
    if (native == js::simd_float32x4_##OP)                                                         \
        return inlineCompSimd(callInfo, native, MSimdBinaryComp::OP, SimdTypeDescr::Float32x4);

    COMP_COMMONX4_TO_INT32X4_SIMD_OP(INLINE_SIMD_COMPARISON_)
#undef INLINE_SIMD_COMPARISON_

#define INLINE_SIMD_SETTER_(LANE)                                                                   \
    if (native == js::simd_int32x4_with##LANE)                                                      \
        return inlineSimdWith(callInfo, native, SimdLane::Lane##LANE, SimdTypeDescr::Int32x4);      \
    if (native == js::simd_float32x4_with##LANE)                                                    \
        return inlineSimdWith(callInfo, native, SimdLane::Lane##LANE, SimdTypeDescr::Float32x4);

    INLINE_SIMD_SETTER_(X)
    INLINE_SIMD_SETTER_(Y)
    INLINE_SIMD_SETTER_(Z)
    INLINE_SIMD_SETTER_(W)
#undef INLINE_SIMD_SETTER_

    if (native == js::simd_int32x4_not)
        return inlineUnarySimd(callInfo, native, MSimdUnaryArith::not_, SimdTypeDescr::Int32x4);
    if (native == js::simd_int32x4_neg)
        return inlineUnarySimd(callInfo, native, MSimdUnaryArith::neg, SimdTypeDescr::Int32x4);

#define INLINE_SIMD_FLOAT32X4_UNARY_(OP)                                                           \
    if (native == js::simd_float32x4_##OP)                                                         \
        return inlineUnarySimd(callInfo, native, MSimdUnaryArith::OP, SimdTypeDescr::Float32x4);

    UNARY_ARITH_FLOAT32X4_SIMD_OP(INLINE_SIMD_FLOAT32X4_UNARY_)
    INLINE_SIMD_FLOAT32X4_UNARY_(neg)
#undef INLINE_SIMD_FLOAT32X4_UNARY_

    if (native == js::simd_float32x4_not)
        return inlineUnarySimd(callInfo, native, MSimdUnaryArith::not_, SimdTypeDescr::Float32x4);

    typedef bool IsCast;
    if (native == js::simd_float32x4_fromInt32x4)
        return inlineSimdConvert(callInfo, native, IsCast(false), SimdTypeDescr::Int32x4, SimdTypeDescr::Float32x4);
    if (native == js::simd_int32x4_fromFloat32x4)
        return inlineSimdConvert(callInfo, native, IsCast(false), SimdTypeDescr::Float32x4, SimdTypeDescr::Int32x4);
    if (native == js::simd_float32x4_fromInt32x4Bits)
        return inlineSimdConvert(callInfo, native, IsCast(true), SimdTypeDescr::Int32x4, SimdTypeDescr::Float32x4);
    if (native == js::simd_int32x4_fromFloat32x4Bits)
        return inlineSimdConvert(callInfo, native, IsCast(true), SimdTypeDescr::Float32x4, SimdTypeDescr::Int32x4);

    if (native == js::simd_int32x4_splat)
        return inlineSimdSplat(callInfo, native, SimdTypeDescr::Int32x4);
    if (native == js::simd_float32x4_splat)
        return inlineSimdSplat(callInfo, native, SimdTypeDescr::Float32x4);

    if (native == js::simd_int32x4_check)
        return inlineSimdCheck(callInfo, native, SimdTypeDescr::Int32x4);
    if (native == js::simd_float32x4_check)
        return inlineSimdCheck(callInfo, native, SimdTypeDescr::Float32x4);

    typedef bool IsElementWise;
    if (native == js::simd_int32x4_select)
        return inlineSimdSelect(callInfo, native, IsElementWise(true), SimdTypeDescr::Int32x4);
    if (native == js::simd_int32x4_bitselect)
        return inlineSimdSelect(callInfo, native, IsElementWise(false), SimdTypeDescr::Int32x4);
    if (native == js::simd_float32x4_select)
        return inlineSimdSelect(callInfo, native, IsElementWise(true), SimdTypeDescr::Float32x4);
    if (native == js::simd_float32x4_bitselect)
        return inlineSimdSelect(callInfo, native, IsElementWise(false), SimdTypeDescr::Float32x4);

    if (native == js::simd_int32x4_swizzle)
        return inlineSimdShuffle(callInfo, native, SimdTypeDescr::Int32x4, 1, 4);
    if (native == js::simd_float32x4_swizzle)
        return inlineSimdShuffle(callInfo, native, SimdTypeDescr::Float32x4, 1, 4);
    if (native == js::simd_int32x4_shuffle)
        return inlineSimdShuffle(callInfo, native, SimdTypeDescr::Int32x4, 2, 4);
    if (native == js::simd_float32x4_shuffle)
        return inlineSimdShuffle(callInfo, native, SimdTypeDescr::Float32x4, 2, 4);

    if (native == js::simd_int32x4_load)
        return inlineSimdLoad(callInfo, native, SimdTypeDescr::Int32x4, 4);
    if (native == js::simd_int32x4_load1)
        return inlineSimdLoad(callInfo, native, SimdTypeDescr::Int32x4, 1);
    if (native == js::simd_int32x4_load2)
        return inlineSimdLoad(callInfo, native, SimdTypeDescr::Int32x4, 2);
    if (native == js::simd_int32x4_load3)
        return inlineSimdLoad(callInfo, native, SimdTypeDescr::Int32x4, 3);

    if (native == js::simd_float32x4_load)
        return inlineSimdLoad(callInfo, native, SimdTypeDescr::Float32x4, 4);
    if (native == js::simd_float32x4_load1)
        return inlineSimdLoad(callInfo, native, SimdTypeDescr::Float32x4, 1);
    if (native == js::simd_float32x4_load2)
        return inlineSimdLoad(callInfo, native, SimdTypeDescr::Float32x4, 2);
    if (native == js::simd_float32x4_load3)
        return inlineSimdLoad(callInfo, native, SimdTypeDescr::Float32x4, 3);

    if (native == js::simd_int32x4_store)
        return inlineSimdStore(callInfo, native, SimdTypeDescr::Int32x4, 4);
    if (native == js::simd_int32x4_store1)
        return inlineSimdStore(callInfo, native, SimdTypeDescr::Int32x4, 1);
    if (native == js::simd_int32x4_store2)
        return inlineSimdStore(callInfo, native, SimdTypeDescr::Int32x4, 2);
    if (native == js::simd_int32x4_store3)
        return inlineSimdStore(callInfo, native, SimdTypeDescr::Int32x4, 3);
    if (native == js::simd_float32x4_store)
        return inlineSimdStore(callInfo, native, SimdTypeDescr::Float32x4, 4);
    if (native == js::simd_float32x4_store1)
        return inlineSimdStore(callInfo, native, SimdTypeDescr::Float32x4, 1);
    if (native == js::simd_float32x4_store2)
        return inlineSimdStore(callInfo, native, SimdTypeDescr::Float32x4, 2);
    if (native == js::simd_float32x4_store3)
        return inlineSimdStore(callInfo, native, SimdTypeDescr::Float32x4, 3);

    if (native == js::simd_int32x4_bool)
        return inlineSimdBool(callInfo, native, SimdTypeDescr::Int32x4);

    
    
    trackOptimizationOutcome(TrackedOutcome::CantInlineNativeNoSpecialization);

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineNativeGetter(CallInfo& callInfo, JSFunction* target)
{
    MOZ_ASSERT(target->isNative());
    JSNative native = target->native();

    if (!optimizationInfo().inlineNative())
        return InliningStatus_NotInlined;

    TemporaryTypeSet* thisTypes = callInfo.thisArg()->resultTypeSet();
    MOZ_ASSERT(callInfo.argc() == 0);

    
    
    
    
    if (thisTypes) {
        Scalar::Type type;

        type = thisTypes->getTypedArrayType(constraints());
        if (type != Scalar::MaxTypedArrayViewType &&
            TypedArrayObject::isOriginalLengthGetter(native))
        {
            MInstruction* length = addTypedArrayLength(callInfo.thisArg());
            current->push(length);
            return InliningStatus_Inlined;
        }

        type = thisTypes->getSharedTypedArrayType(constraints());
        if (type != Scalar::MaxTypedArrayViewType &&
            SharedTypedArrayObject::isOriginalLengthGetter(type, native))
        {
            MInstruction* length = addTypedArrayLength(callInfo.thisArg());
            current->push(length);
            return InliningStatus_Inlined;
        }
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineNonFunctionCall(CallInfo& callInfo, JSObject* target)
{
    
    

    if (callInfo.constructing() && target->constructHook() == TypedObject::construct)
        return inlineConstructTypedObject(callInfo, &target->as<TypeDescr>());

    if (!callInfo.constructing() && target->callHook() == SimdTypeDescr::call)
        return inlineConstructSimdObject(callInfo, &target->as<SimdTypeDescr>());

    return InliningStatus_NotInlined;
}

TemporaryTypeSet*
IonBuilder::getInlineReturnTypeSet()
{
    return bytecodeTypes(pc);
}

MIRType
IonBuilder::getInlineReturnType()
{
    TemporaryTypeSet* returnTypes = getInlineReturnTypeSet();
    return returnTypes->getKnownMIRType();
}

IonBuilder::InliningStatus
IonBuilder::inlineMathFunction(CallInfo& callInfo, MMathFunction::Function function)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;
    if (!IsNumberType(callInfo.getArg(0)->type()))
        return InliningStatus_NotInlined;

    const MathCache* cache = compartment->runtime()->maybeGetMathCache();

    callInfo.fun()->setImplicitlyUsedUnchecked();
    callInfo.thisArg()->setImplicitlyUsedUnchecked();

    MMathFunction* ins = MMathFunction::New(alloc(), callInfo.getArg(0), function, cache);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArray(CallInfo& callInfo)
{
    uint32_t initLength = 0;

    JSObject* templateObject = inspector->getTemplateObjectForNative(pc, ArrayConstructor);
    if (!templateObject) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeNoTemplateObj);
        return InliningStatus_NotInlined;
    }

    if (templateObject->is<UnboxedArrayObject>()) {
        if (templateObject->group()->unboxedLayout().nativeGroup())
            return InliningStatus_NotInlined;
    }

    
    if (callInfo.argc() >= 2) {
        initLength = callInfo.argc();

        TypeSet::ObjectKey* key = TypeSet::ObjectKey::get(templateObject);
        if (!key->unknownProperties()) {
            HeapTypeSetKey elemTypes = key->property(JSID_VOID);

            for (uint32_t i = 0; i < initLength; i++) {
                MDefinition* value = callInfo.getArg(i);
                if (!TypeSetIncludes(elemTypes.maybeTypes(), value->type(), value->resultTypeSet())) {
                    elemTypes.freeze(constraints());
                    return InliningStatus_NotInlined;
                }
            }
        }
    }

    
    if (callInfo.argc() == 1) {
        if (callInfo.getArg(0)->type() != MIRType_Int32)
            return InliningStatus_NotInlined;

        MDefinition* arg = callInfo.getArg(0);
        if (!arg->isConstantValue()) {
            callInfo.setImplicitlyUsedUnchecked();
            MNewArrayDynamicLength* ins =
                MNewArrayDynamicLength::New(alloc(), constraints(), templateObject,
                                            templateObject->group()->initialHeap(constraints()),
                                            arg);
            current->add(ins);
            current->push(ins);
            return InliningStatus_Inlined;
        }

        
        trackOptimizationOutcome(TrackedOutcome::ArrayRange);

        
        initLength = arg->constantValue().toInt32();
        if (initLength >= NativeObject::NELEMENTS_LIMIT)
            return InliningStatus_NotInlined;

        
        
        
        if (initLength != GetAnyBoxedOrUnboxedArrayLength(templateObject))
            return InliningStatus_NotInlined;

        
        if (initLength > ArrayObject::EagerAllocationMaxLength)
            return InliningStatus_NotInlined;
    }

    callInfo.setImplicitlyUsedUnchecked();

    MConstant* templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    current->add(templateConst);

    MNewArray* ins = MNewArray::New(alloc(), constraints(), initLength, templateConst,
                                    templateObject->group()->initialHeap(constraints()), pc);
    current->add(ins);
    current->push(ins);

    if (callInfo.argc() >= 2) {
        JSValueType unboxedType = GetBoxedOrUnboxedType(templateObject);
        for (uint32_t i = 0; i < initLength; i++) {
            MDefinition* value = callInfo.getArg(i);
            if (!initializeArrayElement(ins, i, value, unboxedType,  false))
                return InliningStatus_Error;
        }

        MInstruction* setLength = setInitializedLength(ins, unboxedType, initLength);
        if (!resumeAfter(setLength))
            return InliningStatus_Error;
    }

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArrayPopShift(CallInfo& callInfo, MArrayPopShift::Mode mode)
{
    if (callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MIRType returnType = getInlineReturnType();
    if (returnType == MIRType_Undefined || returnType == MIRType_Null)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    
    
    
    ObjectGroupFlags unhandledFlags =
        OBJECT_FLAG_SPARSE_INDEXES |
        OBJECT_FLAG_LENGTH_OVERFLOW |
        OBJECT_FLAG_ITERATED;

    MDefinition* obj = callInfo.thisArg();
    TemporaryTypeSet* thisTypes = obj->resultTypeSet();
    if (!thisTypes)
        return InliningStatus_NotInlined;
    const Class* clasp = thisTypes->getKnownClass(constraints());
    if (clasp != &ArrayObject::class_ && clasp != &UnboxedArrayObject::class_)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(constraints(), unhandledFlags)) {
        trackOptimizationOutcome(TrackedOutcome::ArrayBadFlags);
        return InliningStatus_NotInlined;
    }

    if (ArrayPrototypeHasIndexedProperty(constraints(), script())) {
        trackOptimizationOutcome(TrackedOutcome::ProtoIndexedProps);
        return InliningStatus_NotInlined;
    }

    JSValueType unboxedType = JSVAL_TYPE_MAGIC;
    if (clasp == &UnboxedArrayObject::class_) {
        unboxedType = UnboxedArrayElementType(constraints(), obj, nullptr);
        if (unboxedType == JSVAL_TYPE_MAGIC)
            return InliningStatus_NotInlined;
    }

    callInfo.setImplicitlyUsedUnchecked();

    if (clasp == &ArrayObject::class_)
        obj = addMaybeCopyElementsForWrite(obj,  false);

    TemporaryTypeSet* returnTypes = getInlineReturnTypeSet();
    bool needsHoleCheck = thisTypes->hasObjectFlags(constraints(), OBJECT_FLAG_NON_PACKED);
    bool maybeUndefined = returnTypes->hasType(TypeSet::UndefinedType());

    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(),
                                                       obj, nullptr, returnTypes);
    if (barrier != BarrierKind::NoBarrier)
        returnType = MIRType_Value;

    MArrayPopShift* ins = MArrayPopShift::New(alloc(), obj, mode,
                                              unboxedType, needsHoleCheck, maybeUndefined);
    current->add(ins);
    current->push(ins);
    ins->setResultType(returnType);

    if (!resumeAfter(ins))
        return InliningStatus_Error;

    if (!pushTypeBarrier(ins, returnTypes, barrier))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArraySplice(CallInfo& callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    
    if (getInlineReturnType() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    
    
    if (!BytecodeIsPopped(pc)) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineGeneric);
        return InliningStatus_NotInlined;
    }

    MArraySplice* ins = MArraySplice::New(alloc(),
                                          callInfo.thisArg(),
                                          callInfo.getArg(0),
                                          callInfo.getArg(1));

    current->add(ins);
    pushConstant(UndefinedValue());

    if (!resumeAfter(ins))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArrayJoin(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MArrayJoin* ins = MArrayJoin::New(alloc(), callInfo.thisArg(), callInfo.getArg(0));

    current->add(ins);
    current->push(ins);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArrayPush(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MDefinition* obj = callInfo.thisArg();
    MDefinition* value = callInfo.getArg(0);
    if (PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current,
                                      &obj, nullptr, &value,  false))
    {
        trackOptimizationOutcome(TrackedOutcome::NeedsTypeBarrier);
        return InliningStatus_NotInlined;
    }
    MOZ_ASSERT(obj == callInfo.thisArg() && value == callInfo.getArg(0));

    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    TemporaryTypeSet* thisTypes = callInfo.thisArg()->resultTypeSet();
    if (!thisTypes)
        return InliningStatus_NotInlined;
    const Class* clasp = thisTypes->getKnownClass(constraints());
    if (clasp != &ArrayObject::class_ && clasp != &UnboxedArrayObject::class_)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(constraints(), OBJECT_FLAG_SPARSE_INDEXES |
                                  OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        trackOptimizationOutcome(TrackedOutcome::ArrayBadFlags);
        return InliningStatus_NotInlined;
    }

    if (ArrayPrototypeHasIndexedProperty(constraints(), script())) {
        trackOptimizationOutcome(TrackedOutcome::ProtoIndexedProps);
        return InliningStatus_NotInlined;
    }

    TemporaryTypeSet::DoubleConversion conversion =
        thisTypes->convertDoubleElements(constraints());
    if (conversion == TemporaryTypeSet::AmbiguousDoubleConversion) {
        trackOptimizationOutcome(TrackedOutcome::ArrayDoubleConversion);
        return InliningStatus_NotInlined;
    }

    JSValueType unboxedType = JSVAL_TYPE_MAGIC;
    if (clasp == &UnboxedArrayObject::class_) {
        unboxedType = UnboxedArrayElementType(constraints(), callInfo.thisArg(), nullptr);
        if (unboxedType == JSVAL_TYPE_MAGIC)
            return InliningStatus_NotInlined;
    }

    callInfo.setImplicitlyUsedUnchecked();
    value = callInfo.getArg(0);

    if (conversion == TemporaryTypeSet::AlwaysConvertToDoubles ||
        conversion == TemporaryTypeSet::MaybeConvertToDoubles)
    {
        MInstruction* valueDouble = MToDouble::New(alloc(), value);
        current->add(valueDouble);
        value = valueDouble;
    }

    if (unboxedType == JSVAL_TYPE_MAGIC)
        obj = addMaybeCopyElementsForWrite(obj,  false);

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    MArrayPush* ins = MArrayPush::New(alloc(), obj, value, unboxedType);
    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArrayConcat(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    
    if (getInlineReturnType() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    
    TemporaryTypeSet* thisTypes = callInfo.thisArg()->resultTypeSet();
    TemporaryTypeSet* argTypes = callInfo.getArg(0)->resultTypeSet();
    if (!thisTypes || !argTypes)
        return InliningStatus_NotInlined;

    const Class* clasp = thisTypes->getKnownClass(constraints());
    if (clasp != &ArrayObject::class_ && clasp != &UnboxedArrayObject::class_)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(constraints(), OBJECT_FLAG_SPARSE_INDEXES |
                                  OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        trackOptimizationOutcome(TrackedOutcome::ArrayBadFlags);
        return InliningStatus_NotInlined;
    }

    if (argTypes->getKnownClass(constraints()) != clasp)
        return InliningStatus_NotInlined;
    if (argTypes->hasObjectFlags(constraints(), OBJECT_FLAG_SPARSE_INDEXES |
                                 OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        trackOptimizationOutcome(TrackedOutcome::ArrayBadFlags);
        return InliningStatus_NotInlined;
    }

    JSValueType unboxedType = JSVAL_TYPE_MAGIC;
    if (clasp == &UnboxedArrayObject::class_) {
        unboxedType = UnboxedArrayElementType(constraints(), callInfo.thisArg(), nullptr);
        if (unboxedType == JSVAL_TYPE_MAGIC)
            return InliningStatus_NotInlined;
        if (unboxedType != UnboxedArrayElementType(constraints(), callInfo.getArg(0), nullptr))
            return InliningStatus_NotInlined;
    }

    
    if (ArrayPrototypeHasIndexedProperty(constraints(), script())) {
        trackOptimizationOutcome(TrackedOutcome::ProtoIndexedProps);
        return InliningStatus_NotInlined;
    }

    
    
    if (thisTypes->getObjectCount() != 1)
        return InliningStatus_NotInlined;

    ObjectGroup* thisGroup = thisTypes->getGroup(0);
    if (!thisGroup)
        return InliningStatus_NotInlined;
    TypeSet::ObjectKey* thisKey = TypeSet::ObjectKey::get(thisGroup);
    if (thisKey->unknownProperties())
        return InliningStatus_NotInlined;

    
    
    if (!thisTypes->hasObjectFlags(constraints(), OBJECT_FLAG_NON_PACKED) &&
        argTypes->hasObjectFlags(constraints(), OBJECT_FLAG_NON_PACKED))
    {
        trackOptimizationOutcome(TrackedOutcome::ArrayBadFlags);
        return InliningStatus_NotInlined;
    }

    
    
    
    HeapTypeSetKey thisElemTypes = thisKey->property(JSID_VOID);

    TemporaryTypeSet* resTypes = getInlineReturnTypeSet();
    if (!resTypes->hasType(TypeSet::ObjectType(thisKey)))
        return InliningStatus_NotInlined;

    for (unsigned i = 0; i < argTypes->getObjectCount(); i++) {
        TypeSet::ObjectKey* key = argTypes->getObject(i);
        if (!key)
            continue;

        if (key->unknownProperties())
            return InliningStatus_NotInlined;

        HeapTypeSetKey elemTypes = key->property(JSID_VOID);
        if (!elemTypes.knownSubset(constraints(), thisElemTypes))
            return InliningStatus_NotInlined;
    }

    
    JSObject* templateObj = inspector->getTemplateObjectForNative(pc, js::array_concat);
    if (!templateObj || templateObj->group() != thisGroup)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MArrayConcat* ins = MArrayConcat::New(alloc(), constraints(),
                                          callInfo.thisArg(), callInfo.getArg(0),
                                          templateObj,
                                          templateObj->group()->initialHeap(constraints()),
                                          unboxedType);
    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArraySlice(CallInfo& callInfo)
{
    if (callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    
    if (getInlineReturnType() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    
    if (callInfo.argc() > 0) {
        if (callInfo.getArg(0)->type() != MIRType_Int32)
            return InliningStatus_NotInlined;
        if (callInfo.argc() > 1) {
            if (callInfo.getArg(1)->type() != MIRType_Int32)
                return InliningStatus_NotInlined;
        }
    }

    
    TemporaryTypeSet* thisTypes = callInfo.thisArg()->resultTypeSet();
    if (!thisTypes)
        return InliningStatus_NotInlined;

    const Class* clasp = thisTypes->getKnownClass(constraints());
    if (clasp != &ArrayObject::class_ && clasp != &UnboxedArrayObject::class_)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(constraints(), OBJECT_FLAG_SPARSE_INDEXES |
                                  OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        trackOptimizationOutcome(TrackedOutcome::ArrayBadFlags);
        return InliningStatus_NotInlined;
    }

    JSValueType unboxedType = JSVAL_TYPE_MAGIC;
    if (clasp == &UnboxedArrayObject::class_) {
        unboxedType = UnboxedArrayElementType(constraints(), callInfo.thisArg(), nullptr);
        if (unboxedType == JSVAL_TYPE_MAGIC)
            return InliningStatus_NotInlined;
    }

    
    if (ArrayPrototypeHasIndexedProperty(constraints(), script())) {
        trackOptimizationOutcome(TrackedOutcome::ProtoIndexedProps);
        return InliningStatus_NotInlined;
    }

    
    
    
    for (unsigned i = 0; i < thisTypes->getObjectCount(); i++) {
        TypeSet::ObjectKey* key = thisTypes->getObject(i);
        if (key && key->isSingleton())
            return InliningStatus_NotInlined;
    }

    
    JSObject* templateObj = inspector->getTemplateObjectForNative(pc, js::array_slice);
    if (!templateObj)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MDefinition* begin;
    if (callInfo.argc() > 0)
        begin = callInfo.getArg(0);
    else
        begin = constant(Int32Value(0));

    MDefinition* end;
    if (callInfo.argc() > 1) {
        end = callInfo.getArg(1);
    } else if (clasp == &ArrayObject::class_) {
        MElements* elements = MElements::New(alloc(), callInfo.thisArg());
        current->add(elements);

        end = MArrayLength::New(alloc(), elements);
        current->add(end->toInstruction());
    } else {
        end = MUnboxedArrayLength::New(alloc(), callInfo.thisArg());
        current->add(end->toInstruction());
    }

    MArraySlice* ins = MArraySlice::New(alloc(), constraints(),
                                        callInfo.thisArg(), begin, end,
                                        templateObj,
                                        templateObj->group()->initialHeap(constraints()),
                                        unboxedType);
    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathAbs(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MIRType returnType = getInlineReturnType();
    MIRType argType = callInfo.getArg(0)->type();
    if (!IsNumberType(argType))
        return InliningStatus_NotInlined;

    
    
    
    if (argType != returnType && !(IsFloatingPointType(argType) && returnType == MIRType_Int32)
        && !(argType == MIRType_Float32 && returnType == MIRType_Double))
    {
        return InliningStatus_NotInlined;
    }

    callInfo.setImplicitlyUsedUnchecked();

    
    
    MIRType absType = (argType == MIRType_Float32) ? MIRType_Double : argType;
    MInstruction* ins = MAbs::New(alloc(), callInfo.getArg(0), absType);
    current->add(ins);

    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathFloor(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MIRType argType = callInfo.getArg(0)->type();
    MIRType returnType = getInlineReturnType();

    
    if (argType == MIRType_Int32 && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        
        
        
        
        MLimitedTruncate* ins = MLimitedTruncate::New(alloc(), callInfo.getArg(0),
                                                      MDefinition::IndirectTruncate);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        MFloor* ins = MFloor::New(alloc(), callInfo.getArg(0));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Double) {
        callInfo.setImplicitlyUsedUnchecked();
        MMathFunction* ins = MMathFunction::New(alloc(), callInfo.getArg(0), MMathFunction::Floor, nullptr);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathCeil(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MIRType argType = callInfo.getArg(0)->type();
    MIRType returnType = getInlineReturnType();

    
    if (argType == MIRType_Int32 && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        
        
        
        
        MLimitedTruncate* ins = MLimitedTruncate::New(alloc(), callInfo.getArg(0),
                                                      MDefinition::IndirectTruncate);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        MCeil* ins = MCeil::New(alloc(), callInfo.getArg(0));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Double) {
        callInfo.setImplicitlyUsedUnchecked();
        MMathFunction* ins = MMathFunction::New(alloc(), callInfo.getArg(0), MMathFunction::Ceil, nullptr);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathClz32(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MIRType returnType = getInlineReturnType();
    if (returnType != MIRType_Int32)
        return InliningStatus_NotInlined;

    if (!IsNumberType(callInfo.getArg(0)->type()))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MClz* ins = MClz::New(alloc(), callInfo.getArg(0));
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;

}

IonBuilder::InliningStatus
IonBuilder::inlineMathRound(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MIRType returnType = getInlineReturnType();
    MIRType argType = callInfo.getArg(0)->type();

    
    if (argType == MIRType_Int32 && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        
        
        
        
        MLimitedTruncate* ins = MLimitedTruncate::New(alloc(), callInfo.getArg(0),
                                                      MDefinition::IndirectTruncate);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        MRound* ins = MRound::New(alloc(), callInfo.getArg(0));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Double) {
        callInfo.setImplicitlyUsedUnchecked();
        MMathFunction* ins = MMathFunction::New(alloc(), callInfo.getArg(0), MMathFunction::Round, nullptr);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathSqrt(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MIRType argType = callInfo.getArg(0)->type();
    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;
    if (!IsNumberType(argType))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MSqrt* sqrt = MSqrt::New(alloc(), callInfo.getArg(0));
    current->add(sqrt);
    current->push(sqrt);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathAtan2(CallInfo& callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;

    MIRType argType0 = callInfo.getArg(0)->type();
    MIRType argType1 = callInfo.getArg(1)->type();

    if (!IsNumberType(argType0) || !IsNumberType(argType1))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MAtan2* atan2 = MAtan2::New(alloc(), callInfo.getArg(0), callInfo.getArg(1));
    current->add(atan2);
    current->push(atan2);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathHypot(CallInfo& callInfo)
{
    if (callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    uint32_t argc = callInfo.argc();
    if (argc < 2 || argc > 4) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;

    MDefinitionVector vector(alloc());
    if (!vector.reserve(argc))
        return InliningStatus_NotInlined;

    for (uint32_t i = 0; i < argc; ++i) {
        MDefinition * arg = callInfo.getArg(i);
        if (!IsNumberType(arg->type()))
            return InliningStatus_NotInlined;
        vector.infallibleAppend(arg);
    }

    callInfo.setImplicitlyUsedUnchecked();
    MHypot* hypot = MHypot::New(alloc(), vector);

    if (!hypot)
        return InliningStatus_NotInlined;

    current->add(hypot);
    current->push(hypot);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathPow(CallInfo& callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    
    MIRType baseType = callInfo.getArg(0)->type();
    MIRType powerType = callInfo.getArg(1)->type();
    MIRType outputType = getInlineReturnType();

    if (outputType != MIRType_Int32 && outputType != MIRType_Double)
        return InliningStatus_NotInlined;
    if (!IsNumberType(baseType))
        return InliningStatus_NotInlined;
    if (!IsNumberType(powerType))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MDefinition* base = callInfo.getArg(0);
    MDefinition* power = callInfo.getArg(1);
    MDefinition* output = nullptr;

    
    if (callInfo.getArg(1)->isConstantValue() &&
        callInfo.getArg(1)->constantValue().isNumber())
    {
        double pow = callInfo.getArg(1)->constantValue().toNumber();

        
        if (pow == 0.5) {
            MPowHalf* half = MPowHalf::New(alloc(), base);
            current->add(half);
            output = half;
        }

        
        if (pow == -0.5) {
            MPowHalf* half = MPowHalf::New(alloc(), base);
            current->add(half);
            MConstant* one = MConstant::New(alloc(), DoubleValue(1.0));
            current->add(one);
            MDiv* div = MDiv::New(alloc(), one, half, MIRType_Double);
            current->add(div);
            output = div;
        }

        
        if (pow == 1.0)
            output = base;

        
        if (pow == 2.0) {
            MMul* mul = MMul::New(alloc(), base, base, outputType);
            current->add(mul);
            output = mul;
        }

        
        if (pow == 3.0) {
            MMul* mul1 = MMul::New(alloc(), base, base, outputType);
            current->add(mul1);
            MMul* mul2 = MMul::New(alloc(), base, mul1, outputType);
            current->add(mul2);
            output = mul2;
        }

        
        if (pow == 4.0) {
            MMul* y = MMul::New(alloc(), base, base, outputType);
            current->add(y);
            MMul* mul = MMul::New(alloc(), y, y, outputType);
            current->add(mul);
            output = mul;
        }
    }

    
    if (!output) {
        if (powerType == MIRType_Float32)
            powerType = MIRType_Double;
        MPow* pow = MPow::New(alloc(), base, power, powerType);
        current->add(pow);
        output = pow;
    }

    
    if (outputType == MIRType_Int32 && output->type() != MIRType_Int32) {
        MToInt32* toInt = MToInt32::New(alloc(), output);
        current->add(toInt);
        output = toInt;
    }
    if (outputType == MIRType_Double && output->type() != MIRType_Double) {
        MToDouble* toDouble = MToDouble::New(alloc(), output);
        current->add(toDouble);
        output = toDouble;
    }

    current->push(output);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathRandom(CallInfo& callInfo)
{
    if (callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MRandom* rand = MRandom::New(alloc());
    current->add(rand);
    current->push(rand);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathImul(CallInfo& callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MIRType returnType = getInlineReturnType();
    if (returnType != MIRType_Int32)
        return InliningStatus_NotInlined;

    if (!IsNumberType(callInfo.getArg(0)->type()))
        return InliningStatus_NotInlined;
    if (!IsNumberType(callInfo.getArg(1)->type()))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* first = MTruncateToInt32::New(alloc(), callInfo.getArg(0));
    current->add(first);

    MInstruction* second = MTruncateToInt32::New(alloc(), callInfo.getArg(1));
    current->add(second);

    MMul* ins = MMul::New(alloc(), first, second, MIRType_Int32, MMul::Integer);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathFRound(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    
    
    TemporaryTypeSet* returned = getInlineReturnTypeSet();
    if (returned->empty()) {
        
        
        returned->addType(TypeSet::DoubleType(), alloc_->lifoAlloc());
    } else {
        MIRType returnType = getInlineReturnType();
        if (!IsNumberType(returnType))
            return InliningStatus_NotInlined;
    }

    MIRType arg = callInfo.getArg(0)->type();
    if (!IsNumberType(arg))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MToFloat32* ins = MToFloat32::New(alloc(), callInfo.getArg(0));
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathMinMax(CallInfo& callInfo, bool max)
{
    if (callInfo.argc() < 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MIRType returnType = getInlineReturnType();
    if (!IsNumberType(returnType))
        return InliningStatus_NotInlined;

    MDefinitionVector int32_cases(alloc());
    for (unsigned i = 0; i < callInfo.argc(); i++) {
        MDefinition* arg = callInfo.getArg(i);

        switch (arg->type()) {
          case MIRType_Int32:
            if (!int32_cases.append(arg))
                return InliningStatus_Error;
            break;
          case MIRType_Double:
          case MIRType_Float32:
            
            
            if (arg->isConstantValue()) {
                double cte = arg->constantValue().toDouble();
                
                if (cte >= INT32_MAX && !max)
                    break;
                
                if (cte <= INT32_MIN && max)
                    break;
            }

            
            returnType = MIRType_Double;
            break;
          default:
            return InliningStatus_NotInlined;
        }
    }

    if (int32_cases.length() == 0)
        returnType = MIRType_Double;

    callInfo.setImplicitlyUsedUnchecked();

    MDefinitionVector& cases = (returnType == MIRType_Int32) ? int32_cases : callInfo.argv();

    if (cases.length() == 1) {
        MLimitedTruncate* limit = MLimitedTruncate::New(alloc(), cases[0], MDefinition::NoTruncate);
        current->add(limit);
        current->push(limit);
        return InliningStatus_Inlined;
    }

    
    MMinMax* last = MMinMax::New(alloc(), cases[0], cases[1], returnType, max);
    current->add(last);

    for (unsigned i = 2; i < cases.length(); i++) {
        MMinMax* ins = MMinMax::New(alloc(), last, cases[i], returnType, max);
        current->add(ins);
        last = ins;
    }

    current->push(last);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStringObject(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || !callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    
    if (callInfo.getArg(0)->mightBeType(MIRType_Object))
        return InliningStatus_NotInlined;

    JSObject* templateObj = inspector->getTemplateObjectForNative(pc, StringConstructor);
    if (!templateObj)
        return InliningStatus_NotInlined;
    MOZ_ASSERT(templateObj->is<StringObject>());

    callInfo.setImplicitlyUsedUnchecked();

    MNewStringObject* ins = MNewStringObject::New(alloc(), callInfo.getArg(0), templateObj);
    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineConstantStringSplit(CallInfo& callInfo)
{
    if (!callInfo.thisArg()->isConstant())
        return InliningStatus_NotInlined;

    if (!callInfo.getArg(0)->isConstant())
        return InliningStatus_NotInlined;

    const js::Value* argval = callInfo.getArg(0)->toConstant()->vp();
    if (!argval->isString())
        return InliningStatus_NotInlined;

    const js::Value* strval = callInfo.thisArg()->toConstant()->vp();
    if (!strval->isString())
        return InliningStatus_NotInlined;

    MOZ_ASSERT(callInfo.getArg(0)->type() == MIRType_String);
    MOZ_ASSERT(callInfo.thisArg()->type() == MIRType_String);

    
    JSString* stringThis = nullptr;
    JSString* stringArg = nullptr;
    JSObject* templateObject = nullptr;
    if (!inspector->isOptimizableCallStringSplit(pc, &stringThis, &stringArg, &templateObject))
        return InliningStatus_NotInlined;

    MOZ_ASSERT(stringThis);
    MOZ_ASSERT(stringArg);
    MOZ_ASSERT(templateObject);

    if (strval->toString() != stringThis)
        return InliningStatus_NotInlined;

    if (argval->toString() != stringArg)
        return InliningStatus_NotInlined;

    
    TypeSet::ObjectKey* retType = TypeSet::ObjectKey::get(templateObject);
    if (retType->unknownProperties())
        return InliningStatus_NotInlined;

    HeapTypeSetKey key = retType->property(JSID_VOID);
    if (!key.maybeTypes())
        return InliningStatus_NotInlined;

    if (!key.maybeTypes()->hasType(TypeSet::StringType()))
        return InliningStatus_NotInlined;

    uint32_t initLength = GetAnyBoxedOrUnboxedArrayLength(templateObject);
    if (GetAnyBoxedOrUnboxedInitializedLength(templateObject) != initLength)
        return InliningStatus_NotInlined;

    Vector<MConstant*, 0, SystemAllocPolicy> arrayValues;
    for (uint32_t i = 0; i < initLength; i++) {
        Value str = GetAnyBoxedOrUnboxedDenseElement(templateObject, i);
        MOZ_ASSERT(str.toString()->isAtom());
        MConstant* value = MConstant::New(alloc(), str, constraints());
        if (!TypeSetIncludes(key.maybeTypes(), value->type(), value->resultTypeSet()))
            return InliningStatus_NotInlined;

        if (!arrayValues.append(value))
            return InliningStatus_Error;
    }
    callInfo.setImplicitlyUsedUnchecked();

    TemporaryTypeSet::DoubleConversion conversion =
            getInlineReturnTypeSet()->convertDoubleElements(constraints());
    if (conversion == TemporaryTypeSet::AlwaysConvertToDoubles)
        return InliningStatus_NotInlined;

    MConstant* templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    current->add(templateConst);

    MNewArray* ins = MNewArray::New(alloc(), constraints(), initLength, templateConst,
                                    templateObject->group()->initialHeap(constraints()), pc);

    current->add(ins);
    current->push(ins);

    if (!initLength) {
        if (!resumeAfter(ins))
            return InliningStatus_Error;
        return InliningStatus_Inlined;
    }

    JSValueType unboxedType = GetBoxedOrUnboxedType(templateObject);

    
    
    
    for (uint32_t i = 0; i < initLength; i++) {
       MConstant* value = arrayValues[i];
       current->add(value);

       if (!initializeArrayElement(ins, i, value, unboxedType,  false))
           return InliningStatus_Error;
    }

    MInstruction* setLength = setInitializedLength(ins, unboxedType, initLength);
    if (!resumeAfter(setLength))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStringSplit(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (callInfo.thisArg()->type() != MIRType_String)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    IonBuilder::InliningStatus resultConstStringSplit = inlineConstantStringSplit(callInfo);
    if (resultConstStringSplit != InliningStatus_NotInlined)
        return resultConstStringSplit;

    JSObject* templateObject = inspector->getTemplateObjectForNative(pc, js::str_split);
    if (!templateObject)
        return InliningStatus_NotInlined;

    TypeSet::ObjectKey* retKey = TypeSet::ObjectKey::get(templateObject);
    if (retKey->unknownProperties())
        return InliningStatus_NotInlined;

    HeapTypeSetKey key = retKey->property(JSID_VOID);
    if (!key.maybeTypes())
        return InliningStatus_NotInlined;

    if (!key.maybeTypes()->hasType(TypeSet::StringType())) {
        key.freeze(constraints());
        return InliningStatus_NotInlined;
    }

    callInfo.setImplicitlyUsedUnchecked();
    MConstant* templateObjectDef = MConstant::New(alloc(), ObjectValue(*templateObject), constraints());
    current->add(templateObjectDef);

    MStringSplit* ins = MStringSplit::New(alloc(), constraints(), callInfo.thisArg(),
                                          callInfo.getArg(0), templateObjectDef);
    current->add(ins);
    current->push(ins);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrCharCodeAt(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_String && callInfo.thisArg()->type() != MIRType_Value)
        return InliningStatus_NotInlined;
    MIRType argType = callInfo.getArg(0)->type();
    if (argType != MIRType_Int32 && argType != MIRType_Double)
        return InliningStatus_NotInlined;

    
    
    InliningStatus constInlineStatus = inlineConstantCharCodeAt(callInfo);
    if (constInlineStatus != InliningStatus_NotInlined)
        return constInlineStatus;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* index = MToInt32::New(alloc(), callInfo.getArg(0));
    current->add(index);

    MStringLength* length = MStringLength::New(alloc(), callInfo.thisArg());
    current->add(length);

    index = addBoundsCheck(index, length);

    MCharCodeAt* charCode = MCharCodeAt::New(alloc(), callInfo.thisArg(), index);
    current->add(charCode);
    current->push(charCode);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineConstantCharCodeAt(CallInfo& callInfo)
{
    if (!callInfo.thisArg()->isConstantValue() || !callInfo.getArg(0)->isConstantValue()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineGeneric);
        return InliningStatus_NotInlined;
    }

    const js::Value* strval = callInfo.thisArg()->constantVp();
    const js::Value* idxval  = callInfo.getArg(0)->constantVp();

    if (!strval->isString() || !idxval->isInt32())
        return InliningStatus_NotInlined;

    JSString* str = strval->toString();
    if (!str->isLinear()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineGeneric);
        return InliningStatus_NotInlined;
    }

    int32_t idx = idxval->toInt32();
    if (idx < 0 || (uint32_t(idx) >= str->length())) {
        trackOptimizationOutcome(TrackedOutcome::OutOfBounds);
        return InliningStatus_NotInlined;
    }

    callInfo.setImplicitlyUsedUnchecked();

    JSLinearString& linstr = str->asLinear();
    char16_t ch = linstr.latin1OrTwoByteChar(idx);
    MConstant* result = MConstant::New(alloc(), Int32Value(ch));
    current->add(result);
    current->push(result);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrFromCharCode(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MToInt32* charCode = MToInt32::New(alloc(), callInfo.getArg(0));
    current->add(charCode);

    MFromCharCode* string = MFromCharCode::New(alloc(), charCode);
    current->add(string);
    current->push(string);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrCharAt(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_String)
        return InliningStatus_NotInlined;
    MIRType argType = callInfo.getArg(0)->type();
    if (argType != MIRType_Int32 && argType != MIRType_Double)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* index = MToInt32::New(alloc(), callInfo.getArg(0));
    current->add(index);

    MStringLength* length = MStringLength::New(alloc(), callInfo.thisArg());
    current->add(length);

    index = addBoundsCheck(index, length);

    
    MCharCodeAt* charCode = MCharCodeAt::New(alloc(), callInfo.thisArg(), index);
    current->add(charCode);

    MFromCharCode* string = MFromCharCode::New(alloc(), charCode);
    current->add(string);
    current->push(string);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineRegExpExec(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    TemporaryTypeSet* thisTypes = callInfo.thisArg()->resultTypeSet();
    const Class* clasp = thisTypes ? thisTypes->getKnownClass(constraints()) : nullptr;
    if (clasp != &RegExpObject::class_)
        return InliningStatus_NotInlined;

    if (callInfo.getArg(0)->mightBeType(MIRType_Object))
        return InliningStatus_NotInlined;

    JSContext* cx = GetJitContext()->cx;
    if (!cx->compartment()->jitCompartment()->ensureRegExpExecStubExists(cx))
        return InliningStatus_Error;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* exec = MRegExpExec::New(alloc(), callInfo.thisArg(), callInfo.getArg(0));
    current->add(exec);
    current->push(exec);

    if (!resumeAfter(exec))
        return InliningStatus_Error;

    if (!pushTypeBarrier(exec, getInlineReturnTypeSet(), BarrierKind::TypeSet))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineRegExpTest(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    
    if (CallResultEscapes(pc) && getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    TemporaryTypeSet* thisTypes = callInfo.thisArg()->resultTypeSet();
    const Class* clasp = thisTypes ? thisTypes->getKnownClass(constraints()) : nullptr;
    if (clasp != &RegExpObject::class_)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->mightBeType(MIRType_Object))
        return InliningStatus_NotInlined;

    JSContext* cx = GetJitContext()->cx;
    if (!cx->compartment()->jitCompartment()->ensureRegExpTestStubExists(cx))
        return InliningStatus_Error;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* match = MRegExpTest::New(alloc(), callInfo.thisArg(), callInfo.getArg(0));
    current->add(match);
    current->push(match);
    if (!resumeAfter(match))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrReplace(CallInfo& callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    
    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;

    
    if (callInfo.thisArg()->type() != MIRType_String)
        return InliningStatus_NotInlined;

    
    TemporaryTypeSet* arg0Type = callInfo.getArg(0)->resultTypeSet();
    const Class* clasp = arg0Type ? arg0Type->getKnownClass(constraints()) : nullptr;
    if (clasp != &RegExpObject::class_ && callInfo.getArg(0)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    
    if (callInfo.getArg(1)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* cte;
    if (callInfo.getArg(0)->type() == MIRType_String) {
        cte = MStringReplace::New(alloc(), callInfo.thisArg(), callInfo.getArg(0),
                                  callInfo.getArg(1));
    } else {
        cte = MRegExpReplace::New(alloc(), callInfo.thisArg(), callInfo.getArg(0),
                                  callInfo.getArg(1));
    }
    current->add(cte);
    current->push(cte);
    if (cte->isEffectful() && !resumeAfter(cte))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineSubstringKernel(CallInfo& callInfo)
{
    MOZ_ASSERT(callInfo.argc() == 3);
    MOZ_ASSERT(!callInfo.constructing());

    
    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;

    
    if (callInfo.getArg(0)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    
    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    if (callInfo.getArg(2)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MSubstr* substr = MSubstr::New(alloc(), callInfo.getArg(0), callInfo.getArg(1),
                                            callInfo.getArg(2));
    current->add(substr);
    current->push(substr);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineObjectCreate(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    JSObject* templateObject = inspector->getTemplateObjectForNative(pc, obj_create);
    if (!templateObject)
        return InliningStatus_NotInlined;

    MOZ_ASSERT(templateObject->is<PlainObject>());
    MOZ_ASSERT(!templateObject->isSingleton());

    
    MDefinition* arg = callInfo.getArg(0);
    if (JSObject* proto = templateObject->getProto()) {
        if (IsInsideNursery(proto))
            return InliningStatus_NotInlined;

        TemporaryTypeSet* types = arg->resultTypeSet();
        if (!types || types->maybeSingleton() != proto)
            return InliningStatus_NotInlined;

        MOZ_ASSERT(types->getKnownMIRType() == MIRType_Object);
    } else {
        if (arg->type() != MIRType_Null)
            return InliningStatus_NotInlined;
    }

    callInfo.setImplicitlyUsedUnchecked();

    MConstant* templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    current->add(templateConst);
    MNewObject* ins = MNewObject::New(alloc(), constraints(), templateConst,
                                      templateObject->group()->initialHeap(constraints()),
                                      MNewObject::ObjectCreate);
    current->add(ins);
    current->push(ins);
    if (!resumeAfter(ins))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineDefineDataProperty(CallInfo& callInfo)
{
    MOZ_ASSERT(!callInfo.constructing());

    
    if (callInfo.argc() != 3)
        return InliningStatus_NotInlined;

    MDefinition* obj = callInfo.getArg(0);
    MDefinition* id = callInfo.getArg(1);
    MDefinition* value = callInfo.getArg(2);

    if (ElementAccessHasExtraIndexedProperty(constraints(), obj))
        return InliningStatus_NotInlined;

    
    
    
    MOZ_ASSERT(*GetNextPc(pc) == JSOP_POP);

    bool emitted = false;
    if (!setElemTryDense(&emitted, obj, id, value,  true))
        return InliningStatus_Error;
    if (!emitted)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineHasClass(CallInfo& callInfo,
                           const Class* clasp1, const Class* clasp2,
                           const Class* clasp3, const Class* clasp4)
{
    if (callInfo.constructing() || callInfo.argc() != 1) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    TemporaryTypeSet* types = callInfo.getArg(0)->resultTypeSet();
    const Class* knownClass = types ? types->getKnownClass(constraints()) : nullptr;
    if (knownClass) {
        pushConstant(BooleanValue(knownClass == clasp1 ||
                                  knownClass == clasp2 ||
                                  knownClass == clasp3 ||
                                  knownClass == clasp4));
    } else {
        MHasClass* hasClass1 = MHasClass::New(alloc(), callInfo.getArg(0), clasp1);
        current->add(hasClass1);

        if (!clasp2 && !clasp3 && !clasp4) {
            current->push(hasClass1);
        } else {
            const Class* remaining[] = { clasp2, clasp3, clasp4 };
            MDefinition* last = hasClass1;
            for (size_t i = 0; i < ArrayLength(remaining); i++) {
                MHasClass* hasClass = MHasClass::New(alloc(), callInfo.getArg(0), remaining[i]);
                current->add(hasClass);
                MBitOr* either = MBitOr::New(alloc(), last, hasClass);
                either->infer(inspector, pc);
                current->add(either);
                last = either;
            }

            
            MNot* resultInverted = MNot::New(alloc(), last, constraints());
            current->add(resultInverted);
            MNot* result = MNot::New(alloc(), resultInverted, constraints());
            current->add(result);
            current->push(result);
        }
    }

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineIsTypedArrayHelper(CallInfo& callInfo, WrappingBehavior wrappingBehavior)
{
    MOZ_ASSERT(!callInfo.constructing());
    MOZ_ASSERT(callInfo.argc() == 1);

    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    
    

    TemporaryTypeSet* types = callInfo.getArg(0)->resultTypeSet();
    if (!types)
        return InliningStatus_NotInlined;

    bool result = false;
    switch (types->forAllClasses(constraints(), IsTypedArrayClass)) {
      case TemporaryTypeSet::ForAllResult::ALL_FALSE:
      case TemporaryTypeSet::ForAllResult::EMPTY: {
        
        
        
        
        if (wrappingBehavior == AllowWrappedTypedArrays)
            return InliningStatus_NotInlined;

        result = false;
        break;
      }

      case TemporaryTypeSet::ForAllResult::ALL_TRUE:
        result = true;
        break;

      case TemporaryTypeSet::ForAllResult::MIXED:
        return InliningStatus_NotInlined;
    }

    pushConstant(BooleanValue(result));

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineIsTypedArray(CallInfo& callInfo)
{
    return inlineIsTypedArrayHelper(callInfo, RejectWrappedTypedArrays);
}

IonBuilder::InliningStatus
IonBuilder::inlineIsPossiblyWrappedTypedArray(CallInfo& callInfo)
{
    return inlineIsTypedArrayHelper(callInfo, AllowWrappedTypedArrays);
}

IonBuilder::InliningStatus
IonBuilder::inlineTypedArrayLength(CallInfo& callInfo)
{
    MOZ_ASSERT(!callInfo.constructing());
    MOZ_ASSERT(callInfo.argc() == 1);
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    

    MInstruction* length = addTypedArrayLength(callInfo.getArg(0));
    current->push(length);

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineSetDisjointTypedElements(CallInfo& callInfo)
{
    MOZ_ASSERT(!callInfo.constructing());
    MOZ_ASSERT(callInfo.argc() == 3);

    

    MDefinition* target = callInfo.getArg(0);
    if (target->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_Undefined)
        return InliningStatus_NotInlined;

    MDefinition* targetOffset = callInfo.getArg(1);
    MOZ_ASSERT(targetOffset->type() == MIRType_Int32);

    MDefinition* sourceTypedArray = callInfo.getArg(2);
    if (sourceTypedArray->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    
    
    

    MDefinition* arrays[] = { target, sourceTypedArray };

    for (MDefinition* def : arrays) {
        TemporaryTypeSet* types = def->resultTypeSet();
        if (!types)
            return InliningStatus_NotInlined;

        if (types->forAllClasses(constraints(), IsTypedArrayClass) !=
            TemporaryTypeSet::ForAllResult::ALL_TRUE)
        {
            return InliningStatus_NotInlined;
        }
    }

    auto sets = MSetDisjointTypedElements::New(alloc(), target, targetOffset, sourceTypedArray);
    current->add(sets);

    pushConstant(UndefinedValue());

    if (!resumeAfter(sets))
        return InliningStatus_Error;

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineObjectIsTypeDescr(CallInfo& callInfo)
{
    if (callInfo.constructing() || callInfo.argc() != 1) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    
    

    TemporaryTypeSet* types = callInfo.getArg(0)->resultTypeSet();
    if (!types)
        return InliningStatus_NotInlined;

    bool result = false;
    switch (types->forAllClasses(constraints(), IsTypeDescrClass)) {
    case TemporaryTypeSet::ForAllResult::ALL_FALSE:
    case TemporaryTypeSet::ForAllResult::EMPTY:
        result = false;
        break;
    case TemporaryTypeSet::ForAllResult::ALL_TRUE:
        result = true;
        break;
    case TemporaryTypeSet::ForAllResult::MIXED:
        return InliningStatus_NotInlined;
    }

    pushConstant(BooleanValue(result));

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineSetTypedObjectOffset(CallInfo& callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MDefinition* typedObj = callInfo.getArg(0);
    MDefinition* offset = callInfo.getArg(1);

    
    if (getInlineReturnType() != MIRType_Undefined)
        return InliningStatus_NotInlined;

    
    
    
    
    
    
    TemporaryTypeSet* types = typedObj->resultTypeSet();
    if (typedObj->type() != MIRType_Object || !types)
        return InliningStatus_NotInlined;
    switch (types->forAllClasses(constraints(), IsTypedObjectClass)) {
      case TemporaryTypeSet::ForAllResult::ALL_FALSE:
      case TemporaryTypeSet::ForAllResult::EMPTY:
      case TemporaryTypeSet::ForAllResult::MIXED:
        return InliningStatus_NotInlined;
      case TemporaryTypeSet::ForAllResult::ALL_TRUE:
        break;
    }

    
    if (offset->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    MInstruction* ins = MSetTypedObjectOffset::New(alloc(), typedObj, offset);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineUnsafeSetReservedSlot(CallInfo& callInfo)
{
    if (callInfo.argc() != 3 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }
    if (getInlineReturnType() != MIRType_Undefined)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    MDefinition* arg = callInfo.getArg(1);
    if (!arg->isConstantValue())
        return InliningStatus_NotInlined;
    uint32_t slot = arg->constantValue().toPrivateUint32();

    callInfo.setImplicitlyUsedUnchecked();

    MStoreFixedSlot* store = MStoreFixedSlot::New(alloc(), callInfo.getArg(0), slot, callInfo.getArg(2));
    current->add(store);
    current->push(store);

    if (NeedsPostBarrier(info(), callInfo.getArg(2)))
        current->add(MPostWriteBarrier::New(alloc(), callInfo.getArg(0), callInfo.getArg(2)));

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineUnsafeGetReservedSlot(CallInfo& callInfo, MIRType knownValueType)
{
    if (callInfo.argc() != 2 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    MDefinition* arg = callInfo.getArg(1);
    if (!arg->isConstantValue())
        return InliningStatus_NotInlined;
    uint32_t slot = arg->constantValue().toPrivateUint32();

    callInfo.setImplicitlyUsedUnchecked();

    MLoadFixedSlot* load = MLoadFixedSlot::New(alloc(), callInfo.getArg(0), slot);
    current->add(load);
    current->push(load);
    if (knownValueType != MIRType_Value) {
        
        
        
        
        
        
        MOZ_ASSERT_IF(!getInlineReturnTypeSet()->empty(),
                      getInlineReturnType() == knownValueType);
        load->setResultType(knownValueType);
    }

    
    if (!pushTypeBarrier(load, getInlineReturnTypeSet(), BarrierKind::TypeSet))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineIsCallable(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    
    
    bool isCallableKnown = false;
    bool isCallableConstant;
    if (callInfo.getArg(0)->type() != MIRType_Object) {
        isCallableKnown = true;
        isCallableConstant = false;
    } else {
        TemporaryTypeSet* types = callInfo.getArg(0)->resultTypeSet();
        const Class* clasp = types ? types->getKnownClass(constraints()) : nullptr;
        if (clasp && !clasp->isProxy()) {
            isCallableKnown = true;
            isCallableConstant = clasp->nonProxyCallable();
        }
    }

    callInfo.setImplicitlyUsedUnchecked();

    if (isCallableKnown) {
        MConstant* constant = MConstant::New(alloc(), BooleanValue(isCallableConstant));
        current->add(constant);
        current->push(constant);
        return InliningStatus_Inlined;
    }

    MIsCallable* isCallable = MIsCallable::New(alloc(), callInfo.getArg(0));
    current->add(isCallable);
    current->push(isCallable);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineIsObject(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }
    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    if (callInfo.getArg(0)->type() == MIRType_Object) {
        pushConstant(BooleanValue(true));
    } else {
        MIsObject* isObject = MIsObject::New(alloc(), callInfo.getArg(0));
        current->add(isObject);
        current->push(isObject);
    }
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineToObject(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    
    if (getInlineReturnType() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    MDefinition* object = callInfo.getArg(0);

    current->push(object);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineToInteger(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    MDefinition* input = callInfo.getArg(0);

    
    if (input->mightBeType(MIRType_Object) ||
        input->mightBeType(MIRType_String) ||
        input->mightBeType(MIRType_Symbol) ||
        input->mightBeType(MIRType_Undefined) ||
        input->mightBeMagicType())
    {
        return InliningStatus_NotInlined;
    }

    MOZ_ASSERT(input->type() == MIRType_Value || input->type() == MIRType_Null ||
               input->type() == MIRType_Boolean || IsNumberType(input->type()));

    
    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MToInt32* toInt32 = MToInt32::New(alloc(), callInfo.getArg(0));
    current->add(toInt32);
    current->push(toInt32);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineToString(CallInfo& callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    MToString* toString = MToString::New(alloc(), callInfo.getArg(0));
    current->add(toString);
    current->push(toString);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineBailout(CallInfo& callInfo)
{
    callInfo.setImplicitlyUsedUnchecked();

    current->add(MBail::New(alloc()));

    MConstant* undefined = MConstant::New(alloc(), UndefinedValue());
    current->add(undefined);
    current->push(undefined);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAssertFloat32(CallInfo& callInfo)
{
    if (callInfo.argc() != 2)
        return InliningStatus_NotInlined;

    MDefinition* secondArg = callInfo.getArg(1);

    MOZ_ASSERT(secondArg->type() == MIRType_Boolean);
    MOZ_ASSERT(secondArg->isConstantValue());

    bool mustBeFloat32 = secondArg->constantValue().toBoolean();
    current->add(MAssertFloat32::New(alloc(), callInfo.getArg(0), mustBeFloat32));

    MConstant* undefined = MConstant::New(alloc(), UndefinedValue());
    current->add(undefined);
    current->push(undefined);
    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAssertRecoveredOnBailout(CallInfo& callInfo)
{
    if (callInfo.argc() != 2)
        return InliningStatus_NotInlined;

    if (js_JitOptions.checkRangeAnalysis) {
        
        
        
        current->push(constant(UndefinedValue()));
        callInfo.setImplicitlyUsedUnchecked();
        return InliningStatus_Inlined;
    }

    MDefinition* secondArg = callInfo.getArg(1);

    MOZ_ASSERT(secondArg->type() == MIRType_Boolean);
    MOZ_ASSERT(secondArg->isConstantValue());

    bool mustBeRecovered = secondArg->constantValue().toBoolean();
    MAssertRecoveredOnBailout* assert =
        MAssertRecoveredOnBailout::New(alloc(), callInfo.getArg(0), mustBeRecovered);
    current->add(assert);
    current->push(assert);

    
    
    
    MNop* nop = MNop::New(alloc());
    current->add(nop);
    if (!resumeAfter(nop))
        return InliningStatus_Error;
    current->add(MEncodeSnapshot::New(alloc()));

    current->pop();
    current->push(constant(UndefinedValue()));
    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineBoundFunction(CallInfo& nativeCallInfo, JSFunction* target)
{
    trackOptimizationOutcome(TrackedOutcome::CantInlineBound);

    if (!target->getBoundFunctionTarget()->is<JSFunction>())
        return InliningStatus_NotInlined;

    JSFunction* scriptedTarget = &(target->getBoundFunctionTarget()->as<JSFunction>());

    
    
    
    if (nativeCallInfo.constructing() && !scriptedTarget->isConstructor())
        return InliningStatus_NotInlined;

    if (nativeCallInfo.constructing() && nativeCallInfo.getNewTarget() != nativeCallInfo.fun())
        return InliningStatus_NotInlined;

    if (gc::IsInsideNursery(scriptedTarget))
        return InliningStatus_NotInlined;

    for (size_t i = 0; i < target->getBoundFunctionArgumentCount(); i++) {
        const Value val = target->getBoundFunctionArgument(i);
        if (val.isObject() && gc::IsInsideNursery(&val.toObject()))
            return InliningStatus_NotInlined;
        if (val.isString() && !val.toString()->isAtom())
            return InliningStatus_NotInlined;
    }

    const Value thisVal = target->getBoundFunctionThis();
    if (thisVal.isObject() && gc::IsInsideNursery(&thisVal.toObject()))
        return InliningStatus_NotInlined;
    if (thisVal.isString() && !thisVal.toString()->isAtom())
        return InliningStatus_NotInlined;

    size_t argc = target->getBoundFunctionArgumentCount() + nativeCallInfo.argc();
    if (argc > ARGS_LENGTH_MAX)
        return InliningStatus_NotInlined;

    nativeCallInfo.thisArg()->setImplicitlyUsedUnchecked();

    CallInfo callInfo(alloc(), nativeCallInfo.constructing());
    callInfo.setFun(constant(ObjectValue(*scriptedTarget)));
    callInfo.setThis(constant(thisVal));

    if (!callInfo.argv().reserve(argc))
        return InliningStatus_Error;

    for (size_t i = 0; i < target->getBoundFunctionArgumentCount(); i++) {
        MConstant* argConst = constant(target->getBoundFunctionArgument(i));
        callInfo.argv().infallibleAppend(argConst);
    }
    for (size_t i = 0; i < nativeCallInfo.argc(); i++)
        callInfo.argv().infallibleAppend(nativeCallInfo.getArg(i));

    
    
    if (nativeCallInfo.constructing())
        callInfo.setNewTarget(callInfo.fun());

    if (!makeCall(scriptedTarget, callInfo))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsCompareExchange(CallInfo& callInfo)
{
    if (callInfo.argc() != 4 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    Scalar::Type arrayType;
    if (!atomicsMeetsPreconditions(callInfo, &arrayType))
        return InliningStatus_NotInlined;

    MDefinition* oldval = callInfo.getArg(2);
    if (!(oldval->type() == MIRType_Int32 || oldval->type() == MIRType_Double))
        return InliningStatus_NotInlined;

    MDefinition* newval = callInfo.getArg(3);
    if (!(newval->type() == MIRType_Int32 || newval->type() == MIRType_Double))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* elements;
    MDefinition* index;
    atomicsCheckBounds(callInfo, &elements, &index);

    MDefinition* oldvalToWrite = oldval;
    if (oldval->type() == MIRType_Double) {
        oldvalToWrite = MTruncateToInt32::New(alloc(), oldval);
        current->add(oldvalToWrite->toInstruction());
    }

    MDefinition* newvalToWrite = newval;
    if (newval->type() == MIRType_Double) {
        newvalToWrite = MTruncateToInt32::New(alloc(), newval);
        current->add(newvalToWrite->toInstruction());
    }

    MCompareExchangeTypedArrayElement* cas =
        MCompareExchangeTypedArrayElement::New(alloc(), elements, index, arrayType,
                                               oldvalToWrite, newvalToWrite);
    cas->setResultType(getInlineReturnType());
    current->add(cas);
    current->push(cas);

    if (!resumeAfter(cas))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsLoad(CallInfo& callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    Scalar::Type arrayType;
    if (!atomicsMeetsPreconditions(callInfo, &arrayType))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* elements;
    MDefinition* index;
    atomicsCheckBounds(callInfo, &elements, &index);

    MLoadUnboxedScalar* load =
        MLoadUnboxedScalar::New(alloc(), elements, index, arrayType,
                                DoesRequireMemoryBarrier);
    load->setResultType(getInlineReturnType());
    current->add(load);
    current->push(load);

    
    if (!resumeAfter(load))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsStore(CallInfo& callInfo)
{
    if (callInfo.argc() != 3 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    Scalar::Type arrayType;
    if (!atomicsMeetsPreconditions(callInfo, &arrayType, DontCheckAtomicResult))
        return InliningStatus_NotInlined;

    MDefinition* value = callInfo.getArg(2);
    if (!(value->type() == MIRType_Int32 || value->type() == MIRType_Double))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* elements;
    MDefinition* index;
    atomicsCheckBounds(callInfo, &elements, &index);

    MDefinition* toWrite = value;
    if (value->type() == MIRType_Double) {
        toWrite = MTruncateToInt32::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }
    MStoreUnboxedScalar* store =
        MStoreUnboxedScalar::New(alloc(), elements, index, toWrite, arrayType,
                                 DoesRequireMemoryBarrier);
    current->add(store);
    current->push(value);

    if (!resumeAfter(store))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsFence(CallInfo& callInfo)
{
    if (callInfo.argc() != 0 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (!JitSupportsAtomics())
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MMemoryBarrier* fence = MMemoryBarrier::New(alloc());
    current->add(fence);
    pushConstant(UndefinedValue());

    
    if (!resumeAfter(fence))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsBinop(CallInfo& callInfo, JSFunction* target)
{
    if (callInfo.argc() != 3 || callInfo.constructing()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    Scalar::Type arrayType;
    if (!atomicsMeetsPreconditions(callInfo, &arrayType))
        return InliningStatus_NotInlined;

    MDefinition* value = callInfo.getArg(2);
    if (!(value->type() == MIRType_Int32 || value->type() == MIRType_Double))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction* elements;
    MDefinition* index;
    atomicsCheckBounds(callInfo, &elements, &index);

    JSNative native = target->native();
    AtomicOp k = AtomicFetchAddOp;
    if (native == atomics_add)
        k = AtomicFetchAddOp;
    else if (native == atomics_sub)
        k = AtomicFetchSubOp;
    else if (native == atomics_and)
        k = AtomicFetchAndOp;
    else if (native == atomics_or)
        k = AtomicFetchOrOp;
    else if (native == atomics_xor)
        k = AtomicFetchXorOp;
    else
        MOZ_CRASH("Bad atomic operation");

    MDefinition* toWrite = value;
    if (value->type() == MIRType_Double) {
        toWrite = MTruncateToInt32::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }
    MAtomicTypedArrayElementBinop* binop =
        MAtomicTypedArrayElementBinop::New(alloc(), k, elements, index, arrayType, toWrite);
    binop->setResultType(getInlineReturnType());
    current->add(binop);
    current->push(binop);

    if (!resumeAfter(binop))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

bool
IonBuilder::atomicsMeetsPreconditions(CallInfo& callInfo, Scalar::Type* arrayType,
                                      AtomicCheckResult checkResult)
{
    if (!JitSupportsAtomics())
        return false;

    if (callInfo.getArg(0)->type() != MIRType_Object)
        return false;

    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return false;

    
    
    
    
    

    TemporaryTypeSet* arg0Types = callInfo.getArg(0)->resultTypeSet();
    if (!arg0Types)
        return false;

    *arrayType = arg0Types->getSharedTypedArrayType(constraints());
    switch (*arrayType) {
      case Scalar::Int8:
      case Scalar::Uint8:
      case Scalar::Int16:
      case Scalar::Uint16:
      case Scalar::Int32:
        return checkResult == DontCheckAtomicResult || getInlineReturnType() == MIRType_Int32;
      case Scalar::Uint32:
        
        
        
        return checkResult == DontCheckAtomicResult || getInlineReturnType() == MIRType_Double;
      default:
        
        return false;
    }
}

void
IonBuilder::atomicsCheckBounds(CallInfo& callInfo, MInstruction** elements, MDefinition** index)
{
    
    MDefinition* obj = callInfo.getArg(0);
    MInstruction* length = nullptr;
    *index = callInfo.getArg(1);
    *elements = nullptr;
    addTypedArrayLengthAndData(obj, DoBoundsCheck, index, &length, elements);
}

IonBuilder::InliningStatus
IonBuilder::inlineIsConstructing(CallInfo& callInfo)
{
    MOZ_ASSERT(!callInfo.constructing());
    MOZ_ASSERT(callInfo.argc() == 0);
    MOZ_ASSERT(script()->functionNonDelazifying(),
               "isConstructing() should only be called in function scripts");

    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    if (inliningDepth_ == 0) {
        MInstruction* ins = MIsConstructing::New(alloc());
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    bool constructing = inlineCallInfo_->constructing();
    pushConstant(BooleanValue(constructing));
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineConstructTypedObject(CallInfo& callInfo, TypeDescr* descr)
{
    
    if (callInfo.argc() != 0) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNativeBadForm);
        return InliningStatus_NotInlined;
    }

    if (size_t(descr->size()) > InlineTypedObject::MaximumSize)
        return InliningStatus_NotInlined;

    JSObject* obj = inspector->getTemplateObjectForClassHook(pc, descr->getClass());
    if (!obj || !obj->is<InlineTypedObject>())
        return InliningStatus_NotInlined;

    InlineTypedObject* templateObject = &obj->as<InlineTypedObject>();
    if (&templateObject->typeDescr() != descr)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MNewTypedObject* ins = MNewTypedObject::New(alloc(), constraints(), templateObject,
                                                templateObject->group()->initialHeap(constraints()));
    current->add(ins);
    current->push(ins);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineConstructSimdObject(CallInfo& callInfo, SimdTypeDescr* descr)
{
    
    MIRType simdType = SimdTypeDescrToMIRType(descr->type());

    
    if (simdType == MIRType_Undefined)
        return InliningStatus_NotInlined;

    
    
    MOZ_ASSERT(size_t(descr->size(descr->type())) < InlineTypedObject::MaximumSize);
    JSObject* templateObject = inspector->getTemplateObjectForClassHook(pc, descr->getClass());
    if (!templateObject)
        return InliningStatus_NotInlined;

    
    
    InlineTypedObject* inlineTypedObject = &templateObject->as<InlineTypedObject>();
    MOZ_ASSERT(&inlineTypedObject->typeDescr() == descr);

    
    
    MConstant* defVal = nullptr;
    if (callInfo.argc() < SimdTypeToLength(simdType)) {
        MIRType scalarType = SimdTypeToScalarType(simdType);
        if (scalarType == MIRType_Int32) {
            defVal = constant(Int32Value(0));
        } else {
            MOZ_ASSERT(IsFloatingPointType(scalarType));
            defVal = constant(DoubleNaNValue());
            defVal->setResultType(scalarType);
        }
    }

    MSimdValueX4* values =
        MSimdValueX4::New(alloc(), simdType,
                          callInfo.getArgWithDefault(0, defVal), callInfo.getArgWithDefault(1, defVal),
                          callInfo.getArgWithDefault(2, defVal), callInfo.getArgWithDefault(3, defVal));
    current->add(values);

    MSimdBox* obj = MSimdBox::New(alloc(), constraints(), values, inlineTypedObject,
                                  inlineTypedObject->group()->initialHeap(constraints()));
    current->add(obj);
    current->push(obj);

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

bool
IonBuilder::checkInlineSimd(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type,
                            unsigned numArgs, InlineTypedObject** templateObj)
{
    if (callInfo.argc() != numArgs)
        return false;

    JSObject* templateObject = inspector->getTemplateObjectForNative(pc, native);
    if (!templateObject)
        return false;;

    InlineTypedObject* inlineTypedObject = &templateObject->as<InlineTypedObject>();
    MOZ_ASSERT(inlineTypedObject->typeDescr().as<SimdTypeDescr>().type() == type);
    *templateObj = inlineTypedObject;
    return true;
}

IonBuilder::InliningStatus
IonBuilder::boxSimd(CallInfo& callInfo, MInstruction* ins, InlineTypedObject* templateObj)
{
    MSimdBox* obj = MSimdBox::New(alloc(), constraints(), ins, templateObj,
                                  templateObj->group()->initialHeap(constraints()));
    current->add(ins);
    current->add(obj);
    current->push(obj);

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

template<typename T>
IonBuilder::InliningStatus
IonBuilder::inlineBinarySimd(CallInfo& callInfo, JSNative native, typename T::Operation op,
                             SimdTypeDescr::Type type)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, 2, &templateObj))
        return InliningStatus_NotInlined;

    
    
    
    
    
    MIRType mirType = SimdTypeDescrToMIRType(type);
    T* ins = T::New(alloc(), callInfo.getArg(0), callInfo.getArg(1), op, mirType);
    return boxSimd(callInfo, ins, templateObj);
}

IonBuilder::InliningStatus
IonBuilder::inlineCompSimd(CallInfo& callInfo, JSNative native, MSimdBinaryComp::Operation op,
                           SimdTypeDescr::Type compType)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, SimdTypeDescr::Int32x4, 2, &templateObj))
        return InliningStatus_NotInlined;

    
    
    
    
    
    MIRType mirType = SimdTypeDescrToMIRType(compType);
    MSimdBinaryComp* ins = MSimdBinaryComp::New(alloc(), callInfo.getArg(0), callInfo.getArg(1), op, mirType);
    return boxSimd(callInfo, ins, templateObj);
}

IonBuilder::InliningStatus
IonBuilder::inlineUnarySimd(CallInfo& callInfo, JSNative native, MSimdUnaryArith::Operation op,
                            SimdTypeDescr::Type type)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, 1, &templateObj))
        return InliningStatus_NotInlined;

    
    MIRType mirType = SimdTypeDescrToMIRType(type);
    MSimdUnaryArith* ins = MSimdUnaryArith::New(alloc(), callInfo.getArg(0), op, mirType);
    return boxSimd(callInfo, ins, templateObj);
}

IonBuilder::InliningStatus
IonBuilder::inlineSimdSplat(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, 1, &templateObj))
        return InliningStatus_NotInlined;

    
    MIRType mirType = SimdTypeDescrToMIRType(type);
    MSimdSplatX4* ins = MSimdSplatX4::New(alloc(), callInfo.getArg(0), mirType);
    return boxSimd(callInfo, ins, templateObj);
}

IonBuilder::InliningStatus
IonBuilder::inlineSimdWith(CallInfo& callInfo, JSNative native, SimdLane lane,
                           SimdTypeDescr::Type type)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, 2, &templateObj))
        return InliningStatus_NotInlined;

    
    MIRType mirType = SimdTypeDescrToMIRType(type);
    MSimdInsertElement* ins = MSimdInsertElement::New(alloc(), callInfo.getArg(0),
                                                      callInfo.getArg(1), mirType, lane);
    return boxSimd(callInfo, ins, templateObj);
}

IonBuilder::InliningStatus
IonBuilder::inlineSimdConvert(CallInfo& callInfo, JSNative native, bool isCast,
                              SimdTypeDescr::Type from, SimdTypeDescr::Type to)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, to, 1, &templateObj))
        return InliningStatus_NotInlined;

    
    MInstruction* ins;
    MIRType fromType = SimdTypeDescrToMIRType(from);
    MIRType toType = SimdTypeDescrToMIRType(to);
    if (isCast)
        ins = MSimdReinterpretCast::New(alloc(), callInfo.getArg(0), fromType, toType);
    else
        ins = MSimdConvert::New(alloc(), callInfo.getArg(0), fromType, toType);

    return boxSimd(callInfo, ins, templateObj);
}

IonBuilder::InliningStatus
IonBuilder::inlineSimdSelect(CallInfo& callInfo, JSNative native, bool isElementWise,
                             SimdTypeDescr::Type type)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, 3, &templateObj))
        return InliningStatus_NotInlined;

    
    MIRType mirType = SimdTypeDescrToMIRType(type);
    MSimdSelect* ins = MSimdSelect::New(alloc(), callInfo.getArg(0), callInfo.getArg(1),
                                        callInfo.getArg(2), mirType, isElementWise);
    return boxSimd(callInfo, ins, templateObj);
}

IonBuilder::InliningStatus
IonBuilder::inlineSimdCheck(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, 1, &templateObj))
        return InliningStatus_NotInlined;

    MIRType mirType = SimdTypeDescrToMIRType(type);
    MSimdUnbox* unbox = MSimdUnbox::New(alloc(), callInfo.getArg(0), mirType);
    current->add(unbox);
    current->push(callInfo.getArg(0));

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineSimdShuffle(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type,
                              unsigned numVectors, unsigned numLanes)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, numVectors + numLanes, &templateObj))
        return InliningStatus_NotInlined;

    MIRType mirType = SimdTypeDescrToMIRType(type);
    MSimdGeneralShuffle* ins = MSimdGeneralShuffle::New(alloc(), numVectors, numLanes, mirType);

    if (!ins->init(alloc()))
        return InliningStatus_Error;

    for (unsigned i = 0; i < numVectors; i++)
        ins->setVector(i, callInfo.getArg(i));
    for (size_t i = 0; i < numLanes; i++)
        ins->setLane(i, callInfo.getArg(numVectors + i));

    return boxSimd(callInfo, ins, templateObj);
}

static Scalar::Type
SimdTypeToScalarType(SimdTypeDescr::Type type)
{
    switch (type) {
      case SimdTypeDescr::Float32x4: return Scalar::Float32x4;
      case SimdTypeDescr::Int32x4:   return Scalar::Int32x4;
      case SimdTypeDescr::Float64x2: break;
    }
    MOZ_CRASH("unexpected simd type");
}

bool
IonBuilder::prepareForSimdLoadStore(CallInfo& callInfo, Scalar::Type simdType, MInstruction** elements,
                                    MDefinition** index, Scalar::Type* arrayType)
{
    MDefinition* array = callInfo.getArg(0);
    *index = callInfo.getArg(1);

    if (!ElementAccessIsAnyTypedArray(constraints(), array, *index, arrayType))
        return false;

    MInstruction* indexAsInt32 = MToInt32::New(alloc(), *index);
    current->add(indexAsInt32);
    *index = indexAsInt32;

    MDefinition* indexForBoundsCheck = *index;

    
    
    
    MOZ_ASSERT(Scalar::byteSize(simdType) % Scalar::byteSize(*arrayType) == 0);
    int32_t suppSlotsNeeded = Scalar::byteSize(simdType) / Scalar::byteSize(*arrayType) - 1;
    if (suppSlotsNeeded) {
        MConstant* suppSlots = constant(Int32Value(suppSlotsNeeded));
        MAdd* addedIndex = MAdd::New(alloc(), *index, suppSlots);
        
        
        addedIndex->setInt32();
        current->add(addedIndex);
        indexForBoundsCheck = addedIndex;
    }

    MInstruction* length;
    addTypedArrayLengthAndData(array, SkipBoundsCheck, index, &length, elements);

    
    
    MInstruction* positiveCheck = MBoundsCheck::New(alloc(), *index, length);
    current->add(positiveCheck);

    MInstruction* fullCheck = MBoundsCheck::New(alloc(), indexForBoundsCheck, length);
    current->add(fullCheck);
    return true;
}

IonBuilder::InliningStatus
IonBuilder::inlineSimdLoad(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type,
                           unsigned numElems)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, 2, &templateObj))
        return InliningStatus_NotInlined;

    Scalar::Type simdType = SimdTypeToScalarType(type);

    MDefinition* index = nullptr;
    MInstruction* elements = nullptr;
    Scalar::Type arrayType;
    if (!prepareForSimdLoadStore(callInfo, simdType, &elements, &index, &arrayType))
        return InliningStatus_NotInlined;

    MLoadUnboxedScalar* load = MLoadUnboxedScalar::New(alloc(), elements, index, arrayType);
    load->setResultType(SimdTypeDescrToMIRType(type));
    load->setSimdRead(simdType, numElems);

    return boxSimd(callInfo, load, templateObj);
}

IonBuilder::InliningStatus
IonBuilder::inlineSimdStore(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type,
                            unsigned numElems)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, 3, &templateObj))
        return InliningStatus_NotInlined;

    Scalar::Type simdType = SimdTypeToScalarType(type);

    MDefinition* index = nullptr;
    MInstruction* elements = nullptr;
    Scalar::Type arrayType;
    if (!prepareForSimdLoadStore(callInfo, simdType, &elements, &index, &arrayType))
        return InliningStatus_NotInlined;

    MDefinition* valueToWrite = callInfo.getArg(2);
    MStoreUnboxedScalar* store = MStoreUnboxedScalar::New(alloc(), elements, index,
                                                          valueToWrite, arrayType);
    store->setSimdWrite(simdType, numElems);

    current->add(store);
    current->push(valueToWrite);

    callInfo.setImplicitlyUsedUnchecked();

    if (!resumeAfter(store))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineSimdBool(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type)
{
    InlineTypedObject* templateObj = nullptr;
    if (!checkInlineSimd(callInfo, native, type, 4, &templateObj))
        return InliningStatus_NotInlined;

    MOZ_ASSERT(type == SimdTypeDescr::Int32x4, "at the moment, only int32x4.bool is inlined");

    MInstruction* operands[4];
    for (unsigned i = 0; i < 4; i++) {
        operands[i] = MNot::New(alloc(), callInfo.getArg(i), constraints());
        current->add(operands[i]);
    }

    
    MSimdValueX4* vector = MSimdValueX4::New(alloc(), MIRType_Int32x4, operands[0], operands[1],
                                             operands[2], operands[3]);
    current->add(vector);

    MSimdConstant* one = MSimdConstant::New(alloc(), SimdConstant::SplatX4(1), MIRType_Int32x4);
    current->add(one);

    MSimdBinaryArith* result = MSimdBinaryArith::New(alloc(), vector, one, MSimdBinaryArith::Op_sub,
                                                     MIRType_Int32x4);
    return boxSimd(callInfo, result, templateObj);
}

} 
} 
