





#include "jsmath.h"

#include "builtin/AtomicsObject.h"
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

using mozilla::ArrayLength;

namespace js {
namespace jit {

IonBuilder::InliningStatus
IonBuilder::inlineNativeCall(CallInfo &callInfo, JSFunction *target)
{
    MOZ_ASSERT(target->isNative());
    JSNative native = target->native();

    if (!optimizationInfo().inlineNative())
        return InliningStatus_NotInlined;

    
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

    
    if (native == js_Array)
        return inlineArray(callInfo);
    if (native == js::array_pop)
        return inlineArrayPopShift(callInfo, MArrayPopShift::Pop);
    if (native == js::array_shift)
        return inlineArrayPopShift(callInfo, MArrayPopShift::Shift);
    if (native == js::array_push)
        return inlineArrayPush(callInfo);
    if (native == js::array_concat)
        return inlineArrayConcat(callInfo);
    if (native == js::array_join)
        return inlineArrayJoin(callInfo);
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

    
    if (native == js_String)
        return inlineStringObject(callInfo);
    if (native == js::str_split)
        return inlineStringSplit(callInfo);
    if (native == js_str_charCodeAt)
        return inlineStrCharCodeAt(callInfo);
    if (native == js::str_fromCharCode)
        return inlineStrFromCharCode(callInfo);
    if (native == js_str_charAt)
        return inlineStrCharAt(callInfo);
    if (native == str_replace)
        return inlineStrReplace(callInfo);

    
    if (native == regexp_exec && CallResultEscapes(pc))
        return inlineRegExpExec(callInfo);
    if (native == regexp_exec && !CallResultEscapes(pc))
        return inlineRegExpTest(callInfo);
    if (native == regexp_test)
        return inlineRegExpTest(callInfo);

    
    if (native == intrinsic_UnsafePutElements)
        return inlineUnsafePutElements(callInfo);
    if (native == intrinsic_NewDenseArray)
        return inlineNewDenseArray(callInfo);

    
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

    
    if (native == intrinsic_ShouldForceSequential ||
        native == intrinsic_InParallelSection)
        return inlineForceSequentialOrInParallelSection(callInfo);
    if (native == intrinsic_ForkJoinGetSlice)
        return inlineForkJoinGetSlice(callInfo);

    
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
    if (native == intrinsic_TypedArrayLength)
        return inlineTypedArrayLength(callInfo);

    
    if (native == intrinsic_ObjectIsTypedObject)
        return inlineHasClass(callInfo,
                              &OutlineTransparentTypedObject::class_,
                              &OutlineOpaqueTypedObject::class_,
                              &InlineTransparentTypedObject::class_,
                              &InlineOpaqueTypedObject::class_);
    if (native == intrinsic_ObjectIsTransparentTypedObject)
        return inlineHasClass(callInfo,
                              &OutlineTransparentTypedObject::class_,
                              &InlineTransparentTypedObject::class_);
    if (native == intrinsic_ObjectIsOpaqueTypedObject)
        return inlineHasClass(callInfo,
                              &OutlineOpaqueTypedObject::class_,
                              &InlineOpaqueTypedObject::class_);
    if (native == intrinsic_ObjectIsTypeDescr)
        return inlineObjectIsTypeDescr(callInfo);
    if (native == intrinsic_TypeDescrIsSimpleType)
        return inlineHasClass(callInfo,
                              &ScalarTypeDescr::class_, &ReferenceTypeDescr::class_);
    if (native == intrinsic_TypeDescrIsArrayType)
        return inlineHasClass(callInfo, &ArrayTypeDescr::class_);
    if (native == intrinsic_SetTypedObjectOffset)
        return inlineSetTypedObjectOffset(callInfo);

    
    if (native == testingFunc_inParallelSection)
        return inlineForceSequentialOrInParallelSection(callInfo);
    if (native == testingFunc_bailout)
        return inlineBailout(callInfo);
    if (native == testingFunc_assertFloat32)
        return inlineAssertFloat32(callInfo);

    
    if (native == js::CallOrConstructBoundFunction)
        return inlineBoundFunction(callInfo, target);

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineNativeGetter(CallInfo &callInfo, JSFunction *target)
{
    MOZ_ASSERT(target->isNative());
    JSNative native = target->native();

    if (!optimizationInfo().inlineNative())
        return InliningStatus_NotInlined;

    types::TemporaryTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    MOZ_ASSERT(callInfo.argc() == 0);

    
    
    
    
    if (thisTypes) {
        Scalar::Type type;

        type = thisTypes->getTypedArrayType();
        if (type != Scalar::MaxTypedArrayViewType &&
            TypedArrayObject::isOriginalLengthGetter(native))
        {
            MInstruction *length = addTypedArrayLength(callInfo.thisArg());
            current->push(length);
            return InliningStatus_Inlined;
        }

        type = thisTypes->getSharedTypedArrayType();
        if (type != Scalar::MaxTypedArrayViewType &&
            SharedTypedArrayObject::isOriginalLengthGetter(type, native))
        {
            MInstruction *length = addTypedArrayLength(callInfo.thisArg());
            current->push(length);
            return InliningStatus_Inlined;
        }
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineNonFunctionCall(CallInfo &callInfo, JSObject *target)
{
    
    

    if (callInfo.constructing() && target->constructHook() == TypedObject::construct)
        return inlineConstructTypedObject(callInfo, &target->as<TypeDescr>());

    return InliningStatus_NotInlined;
}

types::TemporaryTypeSet *
IonBuilder::getInlineReturnTypeSet()
{
    return bytecodeTypes(pc);
}

MIRType
IonBuilder::getInlineReturnType()
{
    types::TemporaryTypeSet *returnTypes = getInlineReturnTypeSet();
    return returnTypes->getKnownMIRType();
}

IonBuilder::InliningStatus
IonBuilder::inlineMathFunction(CallInfo &callInfo, MMathFunction::Function function)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;
    if (!IsNumberType(callInfo.getArg(0)->type()))
        return InliningStatus_NotInlined;

    const MathCache *cache = compartment->runtime()->maybeGetMathCache();

    callInfo.fun()->setImplicitlyUsedUnchecked();
    callInfo.thisArg()->setImplicitlyUsedUnchecked();

    MMathFunction *ins = MMathFunction::New(alloc(), callInfo.getArg(0), function, cache);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArray(CallInfo &callInfo)
{
    uint32_t initLength = 0;
    AllocatingBehaviour allocating = NewArray_Unallocating;

    NativeObject *templateObject = inspector->getTemplateObjectForNative(pc, js_Array);
    if (!templateObject)
        return InliningStatus_NotInlined;
    MOZ_ASSERT(templateObject->is<ArrayObject>());

    
    if (callInfo.argc() >= 2) {
        initLength = callInfo.argc();
        allocating = NewArray_FullyAllocating;

        types::TypeObjectKey *type = types::TypeObjectKey::get(templateObject);
        if (!type->unknownProperties()) {
            types::HeapTypeSetKey elemTypes = type->property(JSID_VOID);

            for (uint32_t i = 0; i < initLength; i++) {
                MDefinition *value = callInfo.getArg(i);
                if (!TypeSetIncludes(elemTypes.maybeTypes(), value->type(), value->resultTypeSet())) {
                    elemTypes.freeze(constraints());
                    return InliningStatus_NotInlined;
                }
            }
        }
    }

    types::TemporaryTypeSet::DoubleConversion conversion =
        getInlineReturnTypeSet()->convertDoubleElements(constraints());
    if (conversion == types::TemporaryTypeSet::AlwaysConvertToDoubles)
        templateObject->setShouldConvertDoubleElements();
    else
        templateObject->clearShouldConvertDoubleElements();

    
    if (callInfo.argc() == 1) {
        if (callInfo.getArg(0)->type() != MIRType_Int32)
            return InliningStatus_NotInlined;

        MDefinition *arg = callInfo.getArg(0);
        if (!arg->isConstantValue()) {
            callInfo.setImplicitlyUsedUnchecked();
            ArrayObject *templateArray = &templateObject->as<ArrayObject>();
            MNewArrayDynamicLength *ins =
                MNewArrayDynamicLength::New(alloc(), constraints(), templateArray,
                                            templateArray->type()->initialHeap(constraints()),
                                            arg);
            current->add(ins);
            current->push(ins);
            return InliningStatus_Inlined;
        }

        
        initLength = arg->constantValue().toInt32();
        if (initLength >= NativeObject::NELEMENTS_LIMIT)
            return InliningStatus_NotInlined;

        
        
        
        if (initLength != templateObject->as<ArrayObject>().length())
            return InliningStatus_NotInlined;

        
        if (initLength > ArrayObject::EagerAllocationMaxLength)
            return InliningStatus_NotInlined;

        allocating = NewArray_FullyAllocating;
    }

    callInfo.setImplicitlyUsedUnchecked();

    MConstant *templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    current->add(templateConst);

    MNewArray *ins = MNewArray::New(alloc(), constraints(), initLength, templateConst,
                                    templateObject->type()->initialHeap(constraints()),
                                    allocating);
    current->add(ins);
    current->push(ins);

    if (callInfo.argc() >= 2) {
        
        MElements *elements = MElements::New(alloc(), ins);
        current->add(elements);

        
        
        
        MConstant *id = nullptr;
        for (uint32_t i = 0; i < initLength; i++) {
            id = MConstant::New(alloc(), Int32Value(i));
            current->add(id);

            MDefinition *value = callInfo.getArg(i);
            if (conversion == types::TemporaryTypeSet::AlwaysConvertToDoubles) {
                MInstruction *valueDouble = MToDouble::New(alloc(), value);
                current->add(valueDouble);
                value = valueDouble;
            }

            
            
            
            if (ins->initialHeap() == gc::TenuredHeap)
                current->add(MPostWriteBarrier::New(alloc(), ins, value));

            MStoreElement *store = MStoreElement::New(alloc(), elements, id, value,
                                                       false);
            current->add(store);
        }

        
        MSetInitializedLength *length = MSetInitializedLength::New(alloc(), elements, id);
        current->add(length);

        if (!resumeAfter(length))
            return InliningStatus_Error;
    }

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArrayPopShift(CallInfo &callInfo, MArrayPopShift::Mode mode)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    MIRType returnType = getInlineReturnType();
    if (returnType == MIRType_Undefined || returnType == MIRType_Null)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    
    
    
    types::TypeObjectFlags unhandledFlags =
        types::OBJECT_FLAG_SPARSE_INDEXES |
        types::OBJECT_FLAG_LENGTH_OVERFLOW |
        types::OBJECT_FLAG_ITERATED;

    MDefinition *obj = callInfo.thisArg();
    types::TemporaryTypeSet *thisTypes = obj->resultTypeSet();
    if (!thisTypes || thisTypes->getKnownClass() != &ArrayObject::class_)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(constraints(), unhandledFlags))
        return InliningStatus_NotInlined;

    if (types::ArrayPrototypeHasIndexedProperty(constraints(), script()))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    obj = addMaybeCopyElementsForWrite(obj);

    types::TemporaryTypeSet *returnTypes = getInlineReturnTypeSet();
    bool needsHoleCheck = thisTypes->hasObjectFlags(constraints(), types::OBJECT_FLAG_NON_PACKED);
    bool maybeUndefined = returnTypes->hasType(types::Type::UndefinedType());

    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(),
                                                       obj, nullptr, returnTypes);
    if (barrier != BarrierKind::NoBarrier)
        returnType = MIRType_Value;

    MArrayPopShift *ins = MArrayPopShift::New(alloc(), obj, mode, needsHoleCheck, maybeUndefined);
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
IonBuilder::inlineArraySplice(CallInfo &callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing())
        return InliningStatus_NotInlined;

    
    if (getInlineReturnType() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    
    
    if (!BytecodeIsPopped(pc))
        return InliningStatus_NotInlined;

    MArraySplice *ins = MArraySplice::New(alloc(),
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
IonBuilder::inlineArrayJoin(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MArrayJoin *ins = MArrayJoin::New(alloc(), callInfo.thisArg(), callInfo.getArg(0));

    current->add(ins);
    current->push(ins);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArrayPush(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    MDefinition *obj = callInfo.thisArg();
    MDefinition *value = callInfo.getArg(0);
    if (PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current,
                                      &obj, nullptr, &value,  false))
    {
        return InliningStatus_NotInlined;
    }
    MOZ_ASSERT(obj == callInfo.thisArg() && value == callInfo.getArg(0));

    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    types::TemporaryTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    if (!thisTypes || thisTypes->getKnownClass() != &ArrayObject::class_)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(constraints(), types::OBJECT_FLAG_SPARSE_INDEXES |
                                  types::OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        return InliningStatus_NotInlined;
    }

    if (types::ArrayPrototypeHasIndexedProperty(constraints(), script()))
        return InliningStatus_NotInlined;

    types::TemporaryTypeSet::DoubleConversion conversion =
        thisTypes->convertDoubleElements(constraints());
    if (conversion == types::TemporaryTypeSet::AmbiguousDoubleConversion)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    value = callInfo.getArg(0);

    if (conversion == types::TemporaryTypeSet::AlwaysConvertToDoubles ||
        conversion == types::TemporaryTypeSet::MaybeConvertToDoubles)
    {
        MInstruction *valueDouble = MToDouble::New(alloc(), value);
        current->add(valueDouble);
        value = valueDouble;
    }

    obj = addMaybeCopyElementsForWrite(obj);

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    MArrayPush *ins = MArrayPush::New(alloc(), obj, value);
    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArrayConcat(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    
    if (getInlineReturnType() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    
    types::TemporaryTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    types::TemporaryTypeSet *argTypes = callInfo.getArg(0)->resultTypeSet();
    if (!thisTypes || !argTypes)
        return InliningStatus_NotInlined;

    if (thisTypes->getKnownClass() != &ArrayObject::class_)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(constraints(), types::OBJECT_FLAG_SPARSE_INDEXES |
                                  types::OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        return InliningStatus_NotInlined;
    }

    if (argTypes->getKnownClass() != &ArrayObject::class_)
        return InliningStatus_NotInlined;
    if (argTypes->hasObjectFlags(constraints(), types::OBJECT_FLAG_SPARSE_INDEXES |
                                 types::OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        return InliningStatus_NotInlined;
    }

    
    if (types::ArrayPrototypeHasIndexedProperty(constraints(), script()))
        return InliningStatus_NotInlined;

    
    
    if (thisTypes->getObjectCount() != 1)
        return InliningStatus_NotInlined;

    types::TypeObject *baseThisType = thisTypes->getTypeObject(0);
    if (!baseThisType)
        return InliningStatus_NotInlined;
    types::TypeObjectKey *thisType = types::TypeObjectKey::get(baseThisType);
    if (thisType->unknownProperties())
        return InliningStatus_NotInlined;

    
    
    if (!thisTypes->hasObjectFlags(constraints(), types::OBJECT_FLAG_NON_PACKED) &&
        argTypes->hasObjectFlags(constraints(), types::OBJECT_FLAG_NON_PACKED))
    {
        return InliningStatus_NotInlined;
    }

    
    
    
    types::HeapTypeSetKey thisElemTypes = thisType->property(JSID_VOID);

    types::TemporaryTypeSet *resTypes = getInlineReturnTypeSet();
    if (!resTypes->hasType(types::Type::ObjectType(thisType)))
        return InliningStatus_NotInlined;

    for (unsigned i = 0; i < argTypes->getObjectCount(); i++) {
        types::TypeObjectKey *argType = argTypes->getObject(i);
        if (!argType)
            continue;

        if (argType->unknownProperties())
            return InliningStatus_NotInlined;

        types::HeapTypeSetKey elemTypes = argType->property(JSID_VOID);
        if (!elemTypes.knownSubset(constraints(), thisElemTypes))
            return InliningStatus_NotInlined;
    }

    
    NativeObject *templateObj = inspector->getTemplateObjectForNative(pc, js::array_concat);
    if (!templateObj || templateObj->type() != baseThisType)
        return InliningStatus_NotInlined;
    MOZ_ASSERT(templateObj->is<ArrayObject>());

    callInfo.setImplicitlyUsedUnchecked();

    MArrayConcat *ins = MArrayConcat::New(alloc(), constraints(), callInfo.thisArg(), callInfo.getArg(0),
                                          &templateObj->as<ArrayObject>(),
                                          templateObj->type()->initialHeap(constraints()));
    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathAbs(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 1)
        return InliningStatus_NotInlined;

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
    MInstruction *ins = MAbs::New(alloc(), callInfo.getArg(0), absType);
    current->add(ins);

    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathFloor(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    MIRType argType = callInfo.getArg(0)->type();
    MIRType returnType = getInlineReturnType();

    
    if (argType == MIRType_Int32 && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        
        
        
        
        MLimitedTruncate *ins = MLimitedTruncate::New(alloc(), callInfo.getArg(0),
                                                      MDefinition::IndirectTruncate);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        MFloor *ins = MFloor::New(alloc(), callInfo.getArg(0));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Double) {
        callInfo.setImplicitlyUsedUnchecked();
        MMathFunction *ins = MMathFunction::New(alloc(), callInfo.getArg(0), MMathFunction::Floor, nullptr);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathCeil(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    MIRType argType = callInfo.getArg(0)->type();
    MIRType returnType = getInlineReturnType();

    
    if (argType == MIRType_Int32 && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        
        
        
        
        MLimitedTruncate *ins = MLimitedTruncate::New(alloc(), callInfo.getArg(0),
                                                      MDefinition::IndirectTruncate);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        MCeil *ins = MCeil::New(alloc(), callInfo.getArg(0));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Double) {
        callInfo.setImplicitlyUsedUnchecked();
        MMathFunction *ins = MMathFunction::New(alloc(), callInfo.getArg(0), MMathFunction::Ceil, nullptr);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathClz32(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    MIRType returnType = getInlineReturnType();
    if (returnType != MIRType_Int32)
        return InliningStatus_NotInlined;

    if (!IsNumberType(callInfo.getArg(0)->type()))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MClz *ins = MClz::New(alloc(), callInfo.getArg(0));
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;

}

IonBuilder::InliningStatus
IonBuilder::inlineMathRound(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    MIRType returnType = getInlineReturnType();
    MIRType argType = callInfo.getArg(0)->type();

    
    if (argType == MIRType_Int32 && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        
        
        
        
        MLimitedTruncate *ins = MLimitedTruncate::New(alloc(), callInfo.getArg(0),
                                                      MDefinition::IndirectTruncate);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Int32) {
        callInfo.setImplicitlyUsedUnchecked();
        MRound *ins = MRound::New(alloc(), callInfo.getArg(0));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (IsFloatingPointType(argType) && returnType == MIRType_Double) {
        callInfo.setImplicitlyUsedUnchecked();
        MMathFunction *ins = MMathFunction::New(alloc(), callInfo.getArg(0), MMathFunction::Round, nullptr);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathSqrt(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    MIRType argType = callInfo.getArg(0)->type();
    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;
    if (!IsNumberType(argType))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MSqrt *sqrt = MSqrt::New(alloc(), callInfo.getArg(0));
    current->add(sqrt);
    current->push(sqrt);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathAtan2(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 2)
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;

    MIRType argType0 = callInfo.getArg(0)->type();
    MIRType argType1 = callInfo.getArg(1)->type();

    if (!IsNumberType(argType0) || !IsNumberType(argType1))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MAtan2 *atan2 = MAtan2::New(alloc(), callInfo.getArg(0), callInfo.getArg(1));
    current->add(atan2);
    current->push(atan2);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathHypot(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 2)
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;

    MIRType argType0 = callInfo.getArg(0)->type();
    MIRType argType1 = callInfo.getArg(1)->type();

    if (!IsNumberType(argType0) || !IsNumberType(argType1))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MHypot *hypot = MHypot::New(alloc(), callInfo.getArg(0), callInfo.getArg(1));
    current->add(hypot);
    current->push(hypot);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathPow(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 2)
        return InliningStatus_NotInlined;

    
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

    MDefinition *base = callInfo.getArg(0);
    MDefinition *power = callInfo.getArg(1);
    MDefinition *output = nullptr;

    
    if (callInfo.getArg(1)->isConstantValue() &&
        callInfo.getArg(1)->constantValue().isNumber())
    {
        double pow = callInfo.getArg(1)->constantValue().toNumber();

        
        if (pow == 0.5) {
            MPowHalf *half = MPowHalf::New(alloc(), base);
            current->add(half);
            output = half;
        }

        
        if (pow == -0.5) {
            MPowHalf *half = MPowHalf::New(alloc(), base);
            current->add(half);
            MConstant *one = MConstant::New(alloc(), DoubleValue(1.0));
            current->add(one);
            MDiv *div = MDiv::New(alloc(), one, half, MIRType_Double);
            current->add(div);
            output = div;
        }

        
        if (pow == 1.0)
            output = base;

        
        if (pow == 2.0) {
            MMul *mul = MMul::New(alloc(), base, base, outputType);
            current->add(mul);
            output = mul;
        }

        
        if (pow == 3.0) {
            MMul *mul1 = MMul::New(alloc(), base, base, outputType);
            current->add(mul1);
            MMul *mul2 = MMul::New(alloc(), base, mul1, outputType);
            current->add(mul2);
            output = mul2;
        }

        
        if (pow == 4.0) {
            MMul *y = MMul::New(alloc(), base, base, outputType);
            current->add(y);
            MMul *mul = MMul::New(alloc(), y, y, outputType);
            current->add(mul);
            output = mul;
        }
    }

    
    if (!output) {
        if (powerType == MIRType_Float32)
            powerType = MIRType_Double;
        MPow *pow = MPow::New(alloc(), base, power, powerType);
        current->add(pow);
        output = pow;
    }

    
    if (outputType == MIRType_Int32 && output->type() != MIRType_Int32) {
        MToInt32 *toInt = MToInt32::New(alloc(), output);
        current->add(toInt);
        output = toInt;
    }
    if (outputType == MIRType_Double && output->type() != MIRType_Double) {
        MToDouble *toDouble = MToDouble::New(alloc(), output);
        current->add(toDouble);
        output = toDouble;
    }

    current->push(output);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathRandom(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MRandom *rand = MRandom::New(alloc());
    current->add(rand);
    current->push(rand);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathImul(CallInfo &callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing())
        return InliningStatus_NotInlined;

    MIRType returnType = getInlineReturnType();
    if (returnType != MIRType_Int32)
        return InliningStatus_NotInlined;

    if (!IsNumberType(callInfo.getArg(0)->type()))
        return InliningStatus_NotInlined;
    if (!IsNumberType(callInfo.getArg(1)->type()))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction *first = MTruncateToInt32::New(alloc(), callInfo.getArg(0));
    current->add(first);

    MInstruction *second = MTruncateToInt32::New(alloc(), callInfo.getArg(1));
    current->add(second);

    MMul *ins = MMul::New(alloc(), first, second, MIRType_Int32, MMul::Integer);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathFRound(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    
    
    types::TemporaryTypeSet *returned = getInlineReturnTypeSet();
    if (returned->empty()) {
        
        
        returned->addType(types::Type::DoubleType(), alloc_->lifoAlloc());
    } else {
        MIRType returnType = getInlineReturnType();
        if (!IsNumberType(returnType))
            return InliningStatus_NotInlined;
    }

    MIRType arg = callInfo.getArg(0)->type();
    if (!IsNumberType(arg))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MToFloat32 *ins = MToFloat32::New(alloc(), callInfo.getArg(0));
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathMinMax(CallInfo &callInfo, bool max)
{
    if (callInfo.argc() < 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    MIRType returnType = getInlineReturnType();
    if (!IsNumberType(returnType))
        return InliningStatus_NotInlined;

    MDefinitionVector int32_cases(alloc());
    for (unsigned i = 0; i < callInfo.argc(); i++) {
        MDefinition *arg = callInfo.getArg(i);

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

    MDefinitionVector &cases = (returnType == MIRType_Int32) ? int32_cases : callInfo.argv();

    if (cases.length() == 1) {
        MLimitedTruncate *limit = MLimitedTruncate::New(alloc(), cases[0], MDefinition::NoTruncate);
        current->add(limit);
        current->push(limit);
        return InliningStatus_Inlined;
    }

    
    MMinMax *last = MMinMax::New(alloc(), cases[0], cases[1], returnType, max);
    current->add(last);

    for (unsigned i = 2; i < cases.length(); i++) {
        MMinMax *ins = MMinMax::New(alloc(), last, cases[i], returnType, max);
        current->add(ins);
        last = ins;
    }

    current->push(last);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStringObject(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || !callInfo.constructing())
        return InliningStatus_NotInlined;

    
    if (callInfo.getArg(0)->mightBeType(MIRType_Object))
        return InliningStatus_NotInlined;

    JSObject *templateObj = inspector->getTemplateObjectForNative(pc, js_String);
    if (!templateObj)
        return InliningStatus_NotInlined;
    MOZ_ASSERT(templateObj->is<StringObject>());

    callInfo.setImplicitlyUsedUnchecked();

    MNewStringObject *ins = MNewStringObject::New(alloc(), callInfo.getArg(0), templateObj);
    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStringSplit(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_String)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    JSObject *templateObject = inspector->getTemplateObjectForNative(pc, js::str_split);
    if (!templateObject)
        return InliningStatus_NotInlined;
    MOZ_ASSERT(templateObject->is<ArrayObject>());

    types::TypeObjectKey *retType = types::TypeObjectKey::get(templateObject);
    if (retType->unknownProperties())
        return InliningStatus_NotInlined;

    types::HeapTypeSetKey key = retType->property(JSID_VOID);
    if (!key.maybeTypes())
        return InliningStatus_NotInlined;

    if (!key.maybeTypes()->hasType(types::Type::StringType())) {
        key.freeze(constraints());
        return InliningStatus_NotInlined;
    }

    callInfo.setImplicitlyUsedUnchecked();
    MConstant *templateObjectDef = MConstant::New(alloc(), ObjectValue(*templateObject), constraints());
    current->add(templateObjectDef);

    MStringSplit *ins = MStringSplit::New(alloc(), constraints(), callInfo.thisArg(),
                                          callInfo.getArg(0), templateObjectDef);
    current->add(ins);
    current->push(ins);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrCharCodeAt(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

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

    MInstruction *index = MToInt32::New(alloc(), callInfo.getArg(0));
    current->add(index);

    MStringLength *length = MStringLength::New(alloc(), callInfo.thisArg());
    current->add(length);

    index = addBoundsCheck(index, length);

    MCharCodeAt *charCode = MCharCodeAt::New(alloc(), callInfo.thisArg(), index);
    current->add(charCode);
    current->push(charCode);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineConstantCharCodeAt(CallInfo &callInfo)
{
    if (!callInfo.thisArg()->isConstantValue())
        return InliningStatus_NotInlined;

    if (!callInfo.getArg(0)->isConstantValue())
        return InliningStatus_NotInlined;

    const js::Value *strval = callInfo.thisArg()->constantVp();
    const js::Value *idxval  = callInfo.getArg(0)->constantVp();

    if (!strval->isString() || !idxval->isInt32())
        return InliningStatus_NotInlined;

    JSString *str = strval->toString();
    if (!str->isLinear())
        return InliningStatus_NotInlined;

    int32_t idx = idxval->toInt32();
    if (idx < 0 || (uint32_t(idx) >= str->length()))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    JSLinearString &linstr = str->asLinear();
    char16_t ch = linstr.latin1OrTwoByteChar(idx);
    MConstant *result = MConstant::New(alloc(), Int32Value(ch));
    current->add(result);
    current->push(result);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrFromCharCode(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MToInt32 *charCode = MToInt32::New(alloc(), callInfo.getArg(0));
    current->add(charCode);

    MFromCharCode *string = MFromCharCode::New(alloc(), charCode);
    current->add(string);
    current->push(string);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrCharAt(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_String)
        return InliningStatus_NotInlined;
    MIRType argType = callInfo.getArg(0)->type();
    if (argType != MIRType_Int32 && argType != MIRType_Double)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction *index = MToInt32::New(alloc(), callInfo.getArg(0));
    current->add(index);

    MStringLength *length = MStringLength::New(alloc(), callInfo.thisArg());
    current->add(length);

    index = addBoundsCheck(index, length);

    
    MCharCodeAt *charCode = MCharCodeAt::New(alloc(), callInfo.thisArg(), index);
    current->add(charCode);

    MFromCharCode *string = MFromCharCode::New(alloc(), charCode);
    current->add(string);
    current->push(string);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineRegExpExec(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    types::TemporaryTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    const Class *clasp = thisTypes ? thisTypes->getKnownClass() : nullptr;
    if (clasp != &RegExpObject::class_)
        return InliningStatus_NotInlined;

    if (callInfo.getArg(0)->mightBeType(MIRType_Object))
        return InliningStatus_NotInlined;

    JSContext *cx = GetJitContext()->cx;
    if (!cx->compartment()->jitCompartment()->ensureRegExpExecStubExists(cx))
        return InliningStatus_Error;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction *exec = MRegExpExec::New(alloc(), callInfo.thisArg(), callInfo.getArg(0));
    current->add(exec);
    current->push(exec);

    if (!resumeAfter(exec))
        return InliningStatus_Error;

    if (!pushTypeBarrier(exec, getInlineReturnTypeSet(), BarrierKind::TypeSet))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineRegExpTest(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    
    if (CallResultEscapes(pc) && getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    types::TemporaryTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    const Class *clasp = thisTypes ? thisTypes->getKnownClass() : nullptr;
    if (clasp != &RegExpObject::class_)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->mightBeType(MIRType_Object))
        return InliningStatus_NotInlined;

    JSContext *cx = GetJitContext()->cx;
    if (!cx->compartment()->jitCompartment()->ensureRegExpTestStubExists(cx))
        return InliningStatus_Error;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction *match = MRegExpTest::New(alloc(), callInfo.thisArg(), callInfo.getArg(0));
    current->add(match);
    current->push(match);
    if (!resumeAfter(match))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrReplace(CallInfo &callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing())
        return InliningStatus_NotInlined;

    
    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;

    
    if (callInfo.thisArg()->type() != MIRType_String)
        return InliningStatus_NotInlined;

    
    types::TemporaryTypeSet *arg0Type = callInfo.getArg(0)->resultTypeSet();
    const Class *clasp = arg0Type ? arg0Type->getKnownClass() : nullptr;
    if (clasp != &RegExpObject::class_ && callInfo.getArg(0)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    
    if (callInfo.getArg(1)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction *cte;
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
IonBuilder::inlineSubstringKernel(CallInfo &callInfo)
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

    MSubstr *substr = MSubstr::New(alloc(), callInfo.getArg(0), callInfo.getArg(1),
                                            callInfo.getArg(2));
    current->add(substr);
    current->push(substr);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineUnsafePutElements(CallInfo &callInfo)
{
    uint32_t argc = callInfo.argc();
    if (argc < 3 || (argc % 3) != 0 || callInfo.constructing())
        return InliningStatus_NotInlined;

    







    for (uint32_t base = 0; base < argc; base += 3) {
        uint32_t arri = base + 0;
        uint32_t idxi = base + 1;
        uint32_t elemi = base + 2;

        MDefinition *obj = callInfo.getArg(arri);
        MDefinition *id = callInfo.getArg(idxi);
        MDefinition *elem = callInfo.getArg(elemi);

        bool isDenseNative = ElementAccessIsDenseNative(obj, id);

        bool writeNeedsBarrier = false;
        if (isDenseNative) {
            writeNeedsBarrier = PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current,
                                                              &obj, nullptr, &elem,
                                                               false);
        }

        
        
        Scalar::Type arrayType;
        if ((!isDenseNative || writeNeedsBarrier) &&
            !ElementAccessIsAnyTypedArray(obj, id, &arrayType) &&
            !elementAccessIsTypedObjectArrayOfScalarType(obj, id, &arrayType))
        {
            return InliningStatus_NotInlined;
        }
    }

    callInfo.setImplicitlyUsedUnchecked();

    
    
    MConstant *udef = MConstant::New(alloc(), UndefinedValue());
    current->add(udef);
    current->push(udef);

    for (uint32_t base = 0; base < argc; base += 3) {
        uint32_t arri = base + 0;
        uint32_t idxi = base + 1;

        MDefinition *obj = callInfo.getArg(arri);
        MDefinition *id = callInfo.getArg(idxi);

        if (ElementAccessIsDenseNative(obj, id)) {
            if (!inlineUnsafeSetDenseArrayElement(callInfo, base))
                return InliningStatus_Error;
            continue;
        }

        Scalar::Type arrayType;
        if (ElementAccessIsAnyTypedArray(obj, id, &arrayType)) {
            if (!inlineUnsafeSetTypedArrayElement(callInfo, base, arrayType))
                return InliningStatus_Error;
            continue;
        }

        if (elementAccessIsTypedObjectArrayOfScalarType(obj, id, &arrayType)) {
            if (!inlineUnsafeSetTypedObjectArrayElement(callInfo, base, arrayType))
                return InliningStatus_Error;
            continue;
        }

        MOZ_CRASH("Element access not dense array nor typed array");
    }

    return InliningStatus_Inlined;
}

bool
IonBuilder::elementAccessIsTypedObjectArrayOfScalarType(MDefinition* obj, MDefinition* id,
                                                        Scalar::Type *arrayType)
{
    if (obj->type() != MIRType_Object) 
        return false;

    if (id->type() != MIRType_Int32 && id->type() != MIRType_Double)
        return false;

    TypedObjectPrediction prediction = typedObjectPrediction(obj);
    if (prediction.isUseless() || !prediction.ofArrayKind())
        return false;

    TypedObjectPrediction elemPrediction = prediction.arrayElementType();
    if (elemPrediction.isUseless() || elemPrediction.kind() != type::Scalar)
        return false;

    *arrayType = elemPrediction.scalarType();
    return true;
}

bool
IonBuilder::inlineUnsafeSetDenseArrayElement(CallInfo &callInfo, uint32_t base)
{
    
    
    
    
    
    

    MDefinition *obj = callInfo.getArg(base + 0);
    MDefinition *id = callInfo.getArg(base + 1);
    MDefinition *elem = callInfo.getArg(base + 2);

    types::TemporaryTypeSet::DoubleConversion conversion =
        obj->resultTypeSet()->convertDoubleElements(constraints());
    if (!jsop_setelem_dense(conversion, SetElem_Unsafe, obj, id, elem))
        return false;
    return true;
}

bool
IonBuilder::inlineUnsafeSetTypedArrayElement(CallInfo &callInfo,
                                             uint32_t base,
                                             Scalar::Type arrayType)
{
    
    
    
    

    MDefinition *obj = callInfo.getArg(base + 0);
    MDefinition *id = callInfo.getArg(base + 1);
    MDefinition *elem = callInfo.getArg(base + 2);

    if (!jsop_setelem_typed(arrayType, SetElem_Unsafe, obj, id, elem))
        return false;

    return true;
}

bool
IonBuilder::inlineUnsafeSetTypedObjectArrayElement(CallInfo &callInfo,
                                                   uint32_t base,
                                                   Scalar::Type arrayType)
{
    
    
    
    

    MDefinition *obj = callInfo.getArg(base + 0);
    MDefinition *id = callInfo.getArg(base + 1);
    MDefinition *elem = callInfo.getArg(base + 2);

    if (!jsop_setelem_typed_object(arrayType, SetElem_Unsafe, true, obj, id, elem))
        return false;

    return true;
}

IonBuilder::InliningStatus
IonBuilder::inlineForceSequentialOrInParallelSection(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    ExecutionMode executionMode = info().executionMode();
    switch (executionMode) {
      case ParallelExecution: {
        
        
        
        callInfo.setImplicitlyUsedUnchecked();
        MConstant *ins = MConstant::New(alloc(), BooleanValue(true));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
      }

      default:
        
        
        return InliningStatus_NotInlined;
    }

    MOZ_CRASH("Invalid execution mode");
}

IonBuilder::InliningStatus
IonBuilder::inlineForkJoinGetSlice(CallInfo &callInfo)
{
    if (info().executionMode() != ParallelExecution)
        return InliningStatus_NotInlined;

    
    
    MOZ_ASSERT(callInfo.argc() == 1 && !callInfo.constructing());
    MOZ_ASSERT(callInfo.getArg(0)->type() == MIRType_Int32);

    
    
    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    switch (info().executionMode()) {
      case ParallelExecution:
        if (LIRGenerator::allowInlineForkJoinGetSlice()) {
            MForkJoinGetSlice *getSlice = MForkJoinGetSlice::New(alloc(),
                                                                 graph().forkJoinContext());
            current->add(getSlice);
            current->push(getSlice);
            return InliningStatus_Inlined;
        }
        return InliningStatus_NotInlined;

      default:
        
        current->push(callInfo.getArg(0));
        return InliningStatus_Inlined;
    }

    MOZ_CRASH("Invalid execution mode");
}

IonBuilder::InliningStatus
IonBuilder::inlineNewDenseArray(CallInfo &callInfo)
{
    if (callInfo.constructing() || callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    
    
    ExecutionMode executionMode = info().executionMode();
    switch (executionMode) {
      case ParallelExecution:
        return inlineNewDenseArrayForParallelExecution(callInfo);
      default:
        return inlineNewDenseArrayForSequentialExecution(callInfo);
    }

    MOZ_CRASH("unknown ExecutionMode");
}

IonBuilder::InliningStatus
IonBuilder::inlineNewDenseArrayForSequentialExecution(CallInfo &callInfo)
{
    
    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineNewDenseArrayForParallelExecution(CallInfo &callInfo)
{
    
    
    
    types::TemporaryTypeSet *returnTypes = getInlineReturnTypeSet();
    if (returnTypes->getKnownMIRType() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (returnTypes->unknownObject() || returnTypes->getObjectCount() != 1)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;
    types::TypeObject *typeObject = returnTypes->getTypeObject(0);

    NativeObject *templateObject = inspector->getTemplateObjectForNative(pc, intrinsic_NewDenseArray);
    if (!templateObject || templateObject->type() != typeObject)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MNewDenseArrayPar *newObject = MNewDenseArrayPar::New(alloc(),
                                                          graph().forkJoinContext(),
                                                          callInfo.getArg(0),
                                                          &templateObject->as<ArrayObject>());
    current->add(newObject);
    current->push(newObject);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineHasClass(CallInfo &callInfo,
                           const Class *clasp1, const Class *clasp2,
                           const Class *clasp3, const Class *clasp4)
{
    if (callInfo.constructing() || callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    types::TemporaryTypeSet *types = callInfo.getArg(0)->resultTypeSet();
    const Class *knownClass = types ? types->getKnownClass() : nullptr;
    if (knownClass) {
        pushConstant(BooleanValue(knownClass == clasp1 ||
                                  knownClass == clasp2 ||
                                  knownClass == clasp3 ||
                                  knownClass == clasp4));
    } else {
        MHasClass *hasClass1 = MHasClass::New(alloc(), callInfo.getArg(0), clasp1);
        current->add(hasClass1);

        if (!clasp2 && !clasp3 && !clasp4) {
            current->push(hasClass1);
        } else {
            const Class *remaining[] = { clasp2, clasp3, clasp4 };
            MDefinition *last = hasClass1;
            for (size_t i = 0; i < ArrayLength(remaining); i++) {
                MHasClass *hasClass = MHasClass::New(alloc(), callInfo.getArg(0), remaining[i]);
                current->add(hasClass);
                MBitOr *either = MBitOr::New(alloc(), last, hasClass);
                either->infer(inspector, pc);
                current->add(either);
                last = either;
            }

            
            MNot *resultInverted = MNot::New(alloc(), last);
            resultInverted->cacheOperandMightEmulateUndefined();
            current->add(resultInverted);
            MNot *result = MNot::New(alloc(), resultInverted);
            result->cacheOperandMightEmulateUndefined();
            current->add(result);
            current->push(result);
        }
    }

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineIsTypedArray(CallInfo &callInfo)
{
    MOZ_ASSERT(!callInfo.constructing());
    MOZ_ASSERT(callInfo.argc() == 1);
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    
    

    types::TemporaryTypeSet *types = callInfo.getArg(0)->resultTypeSet();
    if (!types)
        return InliningStatus_NotInlined;

    bool result = false;
    switch (types->forAllClasses(IsTypedArrayClass)) {
      case types::TemporaryTypeSet::ForAllResult::ALL_FALSE:
      case types::TemporaryTypeSet::ForAllResult::EMPTY:
        result = false;
        break;
      case types::TemporaryTypeSet::ForAllResult::ALL_TRUE:
        result = true;
        break;
      case types::TemporaryTypeSet::ForAllResult::MIXED:
        return InliningStatus_NotInlined;
    }

    pushConstant(BooleanValue(result));

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineTypedArrayLength(CallInfo &callInfo)
{
    MOZ_ASSERT(!callInfo.constructing());
    MOZ_ASSERT(callInfo.argc() == 1);
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    

    MInstruction *length = addTypedArrayLength(callInfo.getArg(0));
    current->push(length);

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}


IonBuilder::InliningStatus
IonBuilder::inlineObjectIsTypeDescr(CallInfo &callInfo)
{
    if (callInfo.constructing() || callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    
    

    types::TemporaryTypeSet *types = callInfo.getArg(0)->resultTypeSet();
    if (!types)
        return InliningStatus_NotInlined;

    bool result = false;
    switch (types->forAllClasses(IsTypeDescrClass)) {
    case types::TemporaryTypeSet::ForAllResult::ALL_FALSE:
    case types::TemporaryTypeSet::ForAllResult::EMPTY:
        result = false;
        break;
    case types::TemporaryTypeSet::ForAllResult::ALL_TRUE:
        result = true;
        break;
    case types::TemporaryTypeSet::ForAllResult::MIXED:
        return InliningStatus_NotInlined;
    }

    pushConstant(BooleanValue(result));

    callInfo.setImplicitlyUsedUnchecked();
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineSetTypedObjectOffset(CallInfo &callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing())
        return InliningStatus_NotInlined;

    MDefinition *typedObj = callInfo.getArg(0);
    MDefinition *offset = callInfo.getArg(1);

    
    if (getInlineReturnType() != MIRType_Undefined)
        return InliningStatus_NotInlined;

    
    
    
    
    
    
    types::TemporaryTypeSet *types = typedObj->resultTypeSet();
    if (typedObj->type() != MIRType_Object || !types)
        return InliningStatus_NotInlined;
    switch (types->forAllClasses(IsTypedObjectClass)) {
      case types::TemporaryTypeSet::ForAllResult::ALL_FALSE:
      case types::TemporaryTypeSet::ForAllResult::EMPTY:
      case types::TemporaryTypeSet::ForAllResult::MIXED:
        return InliningStatus_NotInlined;
      case types::TemporaryTypeSet::ForAllResult::ALL_TRUE:
        break;
    }

    
    if (offset->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    MInstruction *ins = MSetTypedObjectOffset::New(alloc(), typedObj, offset);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineUnsafeSetReservedSlot(CallInfo &callInfo)
{
    if (callInfo.argc() != 3 || callInfo.constructing())
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Undefined)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    MDefinition *arg = callInfo.getArg(1);
    if (!arg->isConstantValue())
        return InliningStatus_NotInlined;
    uint32_t slot = arg->constantValue().toPrivateUint32();

    callInfo.setImplicitlyUsedUnchecked();

    MStoreFixedSlot *store = MStoreFixedSlot::New(alloc(), callInfo.getArg(0), slot, callInfo.getArg(2));
    current->add(store);
    current->push(store);

    if (NeedsPostBarrier(info(), callInfo.getArg(2)))
        current->add(MPostWriteBarrier::New(alloc(), callInfo.getArg(0), callInfo.getArg(2)));

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineUnsafeGetReservedSlot(CallInfo &callInfo, MIRType knownValueType)
{
    if (callInfo.argc() != 2 || callInfo.constructing())
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    MDefinition *arg = callInfo.getArg(1);
    if (!arg->isConstantValue())
        return InliningStatus_NotInlined;
    uint32_t slot = arg->constantValue().toPrivateUint32();

    callInfo.setImplicitlyUsedUnchecked();

    MLoadFixedSlot *load = MLoadFixedSlot::New(alloc(), callInfo.getArg(0), slot);
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
IonBuilder::inlineIsCallable(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

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
        types::TemporaryTypeSet *types = callInfo.getArg(0)->resultTypeSet();
        const Class *clasp = types ? types->getKnownClass() : nullptr;
        if (clasp && !clasp->isProxy()) {
            isCallableKnown = true;
            isCallableConstant = clasp->nonProxyCallable();
        }
    }

    callInfo.setImplicitlyUsedUnchecked();

    if (isCallableKnown) {
        MConstant *constant = MConstant::New(alloc(), BooleanValue(isCallableConstant));
        current->add(constant);
        current->push(constant);
        return InliningStatus_Inlined;
    }

    MIsCallable *isCallable = MIsCallable::New(alloc(), callInfo.getArg(0));
    current->add(isCallable);
    current->push(isCallable);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineIsObject(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;
    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    if (callInfo.getArg(0)->type() == MIRType_Object) {
        pushConstant(BooleanValue(true));
    } else {
        MIsObject *isObject = MIsObject::New(alloc(), callInfo.getArg(0));
        current->add(isObject);
        current->push(isObject);
    }
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineToObject(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    
    if (getInlineReturnType() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    MDefinition *object = callInfo.getArg(0);

    current->push(object);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineToInteger(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    MDefinition *input = callInfo.getArg(0);

    
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

    MToInt32 *toInt32 = MToInt32::New(alloc(), callInfo.getArg(0));
    current->add(toInt32);
    current->push(toInt32);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineToString(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();
    MToString *toString = MToString::New(alloc(), callInfo.getArg(0));
    current->add(toString);
    current->push(toString);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineBailout(CallInfo &callInfo)
{
    callInfo.setImplicitlyUsedUnchecked();

    current->add(MBail::New(alloc()));

    MConstant *undefined = MConstant::New(alloc(), UndefinedValue());
    current->add(undefined);
    current->push(undefined);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAssertFloat32(CallInfo &callInfo)
{
    callInfo.setImplicitlyUsedUnchecked();

    MDefinition *secondArg = callInfo.getArg(1);

    MOZ_ASSERT(secondArg->type() == MIRType_Boolean);
    MOZ_ASSERT(secondArg->isConstantValue());

    bool mustBeFloat32 = secondArg->constantValue().toBoolean();
    current->add(MAssertFloat32::New(alloc(), callInfo.getArg(0), mustBeFloat32));

    MConstant *undefined = MConstant::New(alloc(), UndefinedValue());
    current->add(undefined);
    current->push(undefined);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineBoundFunction(CallInfo &nativeCallInfo, JSFunction *target)
{
     if (!target->getBoundFunctionTarget()->is<JSFunction>())
         return InliningStatus_NotInlined;

    JSFunction *scriptedTarget = &(target->getBoundFunctionTarget()->as<JSFunction>());

    
    
    
    if (nativeCallInfo.constructing() && !scriptedTarget->isInterpretedConstructor() &&
        !scriptedTarget->isNativeConstructor())
    {
        return InliningStatus_NotInlined;
    }

    if (gc::IsInsideNursery(scriptedTarget))
        return InliningStatus_NotInlined;

    for (size_t i = 0; i < target->getBoundFunctionArgumentCount(); i++) {
        const Value val = target->getBoundFunctionArgument(i);
        if (val.isObject() && gc::IsInsideNursery(&val.toObject()))
            return InliningStatus_NotInlined;
    }

    const Value thisVal = target->getBoundFunctionThis();
    if (thisVal.isObject() && gc::IsInsideNursery(&thisVal.toObject()))
        return InliningStatus_NotInlined;

    size_t argc = target->getBoundFunctionArgumentCount() + nativeCallInfo.argc();
    if (argc > ARGS_LENGTH_MAX)
        return InliningStatus_NotInlined;

    nativeCallInfo.thisArg()->setImplicitlyUsedUnchecked();

    CallInfo callInfo(alloc(), nativeCallInfo.constructing());
    callInfo.setFun(constant(ObjectValue(*scriptedTarget)));
    callInfo.setThis(constant(target->getBoundFunctionThis()));

    if (!callInfo.argv().reserve(argc))
        return InliningStatus_Error;

    for (size_t i = 0; i < target->getBoundFunctionArgumentCount(); i++)
        callInfo.argv().infallibleAppend(constant(target->getBoundFunctionArgument(i)));
    for (size_t i = 0; i < nativeCallInfo.argc(); i++)
        callInfo.argv().infallibleAppend(nativeCallInfo.getArg(i));

    if (!makeCall(scriptedTarget, callInfo, false))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsCompareExchange(CallInfo &callInfo)
{
    if (callInfo.argc() != 4 || callInfo.constructing())
        return InliningStatus_NotInlined;

    Scalar::Type arrayType;
    if (!atomicsMeetsPreconditions(callInfo, &arrayType))
        return InliningStatus_NotInlined;

    MDefinition *oldval = callInfo.getArg(2);
    if (!(oldval->type() == MIRType_Int32 || oldval->type() == MIRType_Double))
        return InliningStatus_NotInlined;

    MDefinition *newval = callInfo.getArg(3);
    if (!(newval->type() == MIRType_Int32 || newval->type() == MIRType_Double))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction *elements;
    MDefinition *index;
    atomicsCheckBounds(callInfo, &elements, &index);

    MDefinition *oldvalToWrite = oldval;
    if (oldval->type() == MIRType_Double) {
        oldvalToWrite = MTruncateToInt32::New(alloc(), oldval);
        current->add(oldvalToWrite->toInstruction());
    }

    MDefinition *newvalToWrite = newval;
    if (newval->type() == MIRType_Double) {
        newvalToWrite = MTruncateToInt32::New(alloc(), newval);
        current->add(newvalToWrite->toInstruction());
    }

    MCompareExchangeTypedArrayElement *cas =
        MCompareExchangeTypedArrayElement::New(alloc(), elements, index, arrayType,
                                               oldvalToWrite, newvalToWrite);
    cas->setResultType(getInlineReturnType());
    current->add(cas);
    current->push(cas);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsLoad(CallInfo &callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing())
        return InliningStatus_NotInlined;

    Scalar::Type arrayType;
    if (!atomicsMeetsPreconditions(callInfo, &arrayType))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction *elements;
    MDefinition *index;
    atomicsCheckBounds(callInfo, &elements, &index);

    MLoadTypedArrayElement *load =
        MLoadTypedArrayElement::New(alloc(), elements, index, arrayType,
                                    DoesRequireMemoryBarrier);
    load->setResultType(getInlineReturnType());
    current->add(load);
    current->push(load);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsStore(CallInfo &callInfo)
{
    if (callInfo.argc() != 3 || callInfo.constructing())
        return InliningStatus_NotInlined;

    Scalar::Type arrayType;
    if (!atomicsMeetsPreconditions(callInfo, &arrayType))
        return InliningStatus_NotInlined;

    MDefinition *value = callInfo.getArg(2);
    if (!(value->type() == MIRType_Int32 || value->type() == MIRType_Double))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction *elements;
    MDefinition *index;
    atomicsCheckBounds(callInfo, &elements, &index);

    MDefinition *toWrite = value;
    if (value->type() == MIRType_Double) {
        toWrite = MTruncateToInt32::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }
    MStoreTypedArrayElement *store =
        MStoreTypedArrayElement::New(alloc(), elements, index, toWrite, arrayType,
                                     DoesRequireMemoryBarrier);
    current->add(store);
    current->push(value);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsFence(CallInfo &callInfo)
{
    if (callInfo.argc() != 0 || callInfo.constructing())
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MMemoryBarrier *fence = MMemoryBarrier::New(alloc());
    current->add(fence);
    pushConstant(UndefinedValue());

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAtomicsBinop(CallInfo &callInfo, JSFunction *target)
{
    if (callInfo.argc() != 3 || callInfo.constructing())
        return InliningStatus_NotInlined;

    Scalar::Type arrayType;
    if (!atomicsMeetsPreconditions(callInfo, &arrayType))
        return InliningStatus_NotInlined;

    MDefinition *value = callInfo.getArg(2);
    if (!(value->type() == MIRType_Int32 || value->type() == MIRType_Double))
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MInstruction *elements;
    MDefinition *index;
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

    MDefinition *toWrite = value;
    if (value->type() == MIRType_Double) {
        toWrite = MTruncateToInt32::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }
    MAtomicTypedArrayElementBinop *binop =
        MAtomicTypedArrayElementBinop::New(alloc(), k, elements, index, arrayType, toWrite);
    binop->setResultType(getInlineReturnType());
    current->add(binop);
    current->push(binop);

    return InliningStatus_Inlined;
}

bool
IonBuilder::atomicsMeetsPreconditions(CallInfo &callInfo, Scalar::Type *arrayType)
{
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return false;

    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return false;

    
    
    
    
    

    types::TemporaryTypeSet *arg0Types = callInfo.getArg(0)->resultTypeSet();
    if (!arg0Types)
        return false;

    *arrayType = arg0Types->getSharedTypedArrayType();
    switch (*arrayType) {
      case Scalar::Int8:
      case Scalar::Uint8:
      case Scalar::Int16:
      case Scalar::Uint16:
      case Scalar::Int32:
        return getInlineReturnType() == MIRType_Int32;
      case Scalar::Uint32:
        
        
        
        return getInlineReturnType() == MIRType_Double;
      default:
        
        return false;
    }
}

void
IonBuilder::atomicsCheckBounds(CallInfo &callInfo, MInstruction **elements, MDefinition **index)
{
    
    MDefinition *obj = callInfo.getArg(0);
    MInstruction *length = nullptr;
    *index = callInfo.getArg(1);
    *elements = nullptr;
    addTypedArrayLengthAndData(obj, DoBoundsCheck, index, &length, elements);
}

IonBuilder::InliningStatus
IonBuilder::inlineIsConstructing(CallInfo &callInfo)
{
    MOZ_ASSERT(!callInfo.constructing());
    MOZ_ASSERT(callInfo.argc() == 0);
    MOZ_ASSERT(script()->functionNonDelazifying(),
               "isConstructing() should only be called in function scripts");

    if (getInlineReturnType() != MIRType_Boolean)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    if (inliningDepth_ == 0) {
        MInstruction *ins = MIsConstructing::New(alloc());
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    bool constructing = inlineCallInfo_->constructing();
    pushConstant(BooleanValue(constructing));
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineConstructTypedObject(CallInfo &callInfo, TypeDescr *descr)
{
    
    if (callInfo.argc() != 0)
        return InliningStatus_NotInlined;

    if (size_t(descr->size()) > InlineTypedObject::MaximumSize)
        return InliningStatus_NotInlined;

    JSObject *obj = inspector->getTemplateObjectForClassHook(pc, descr->getClass());
    if (!obj || !obj->is<InlineTypedObject>())
        return InliningStatus_NotInlined;

    InlineTypedObject *templateObject = &obj->as<InlineTypedObject>();
    if (&templateObject->typeDescr() != descr)
        return InliningStatus_NotInlined;

    callInfo.setImplicitlyUsedUnchecked();

    MNewTypedObject *ins = MNewTypedObject::New(alloc(), constraints(), templateObject,
                                                templateObject->type()->initialHeap(constraints()));
    current->add(ins);
    current->push(ins);

    return InliningStatus_Inlined;
}

} 
} 
