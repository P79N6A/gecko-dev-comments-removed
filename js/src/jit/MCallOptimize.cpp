





#include "jsmath.h"

#include "builtin/ParallelArray.h"
#include "builtin/TestingFunctions.h"
#include "jit/BaselineInspector.h"
#include "jit/IonBuilder.h"
#include "jit/Lowering.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"

#include "jsscriptinlines.h"

#include "vm/StringObject-inl.h"

namespace js {
namespace jit {

IonBuilder::InliningStatus
IonBuilder::inlineNativeCall(CallInfo &callInfo, JSNative native)
{
    
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

    
    if (native == js_math_abs)
        return inlineMathAbs(callInfo);
    if (native == js::math_floor)
        return inlineMathFloor(callInfo);
    if (native == js::math_round)
        return inlineMathRound(callInfo);
    if (native == js_math_sqrt)
        return inlineMathSqrt(callInfo);
    if (native == math_atan2)
        return inlineMathAtan2(callInfo);
    if (native == js_math_max)
        return inlineMathMinMax(callInfo, true );
    if (native == js_math_min)
        return inlineMathMinMax(callInfo, false );
    if (native == js_math_pow)
        return inlineMathPow(callInfo);
    if (native == js_math_random)
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
    if (native == js::math_sin)
        return inlineMathFunction(callInfo, MMathFunction::SinH);
    if (native == js::math_tan)
        return inlineMathFunction(callInfo, MMathFunction::TanH);
    if (native == js::math_acosh)
        return inlineMathFunction(callInfo, MMathFunction::ACosH);
    if (native == js::math_asin)
        return inlineMathFunction(callInfo, MMathFunction::ASinH);
    if (native == js::math_atan)
        return inlineMathFunction(callInfo, MMathFunction::ATanH);
    if (native == js::math_sign)
        return inlineMathFunction(callInfo, MMathFunction::Sign);
    if (native == js::math_trunc)
        return inlineMathFunction(callInfo, MMathFunction::Trunc);
    if (native == js::math_cbrt)
        return inlineMathFunction(callInfo, MMathFunction::Cbrt);

    
    if (native == js_String)
        return inlineStringObject(callInfo);
    if (native == js_str_charCodeAt)
        return inlineStrCharCodeAt(callInfo);
    if (native == js::str_fromCharCode)
        return inlineStrFromCharCode(callInfo);
    if (native == js_str_charAt)
        return inlineStrCharAt(callInfo);

    
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
        return inlineUnsafeGetReservedSlot(callInfo);

    
    if (native == intrinsic_ShouldForceSequential)
        return inlineForceSequentialOrInParallelSection(callInfo);
    if (native == intrinsic_NewParallelArray)
        return inlineNewParallelArray(callInfo);
    if (native == ParallelArrayObject::construct)
        return inlineParallelArray(callInfo);

    
    if (native == intrinsic_IsCallable)
        return inlineIsCallable(callInfo);
    if (native == intrinsic_HaveSameClass)
        return inlineHaveSameClass(callInfo);
    if (native == intrinsic_ToObject)
        return inlineToObject(callInfo);

    
    if (native == testingFunc_inParallelSection)
        return inlineForceSequentialOrInParallelSection(callInfo);
    if (native == testingFunc_bailout)
        return inlineBailout(callInfo);
    if (native == testingFunc_assertFloat32)
        return inlineAssertFloat32(callInfo);

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
    return MIRTypeFromValueType(returnTypes->getKnownTypeTag());
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

    callInfo.unwrapArgs();

    MathCache *cache = cx->runtime()->getMathCache(cx);
    if (!cache)
        return InliningStatus_Error;

    MMathFunction *ins = MMathFunction::New(callInfo.getArg(0), function, cache);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArray(CallInfo &callInfo)
{
    uint32_t initLength = 0;
    MNewArray::AllocatingBehaviour allocating = MNewArray::NewArray_Unallocating;

    JSObject *templateObject = inspector->getTemplateObjectForNative(pc, js_Array);
    if (!templateObject)
        return InliningStatus_NotInlined;
    JS_ASSERT(templateObject->is<ArrayObject>());

    
    if (callInfo.argc() >= 2) {
        initLength = callInfo.argc();
        allocating = MNewArray::NewArray_Allocating;

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

    
    if (callInfo.argc() == 1) {
        if (callInfo.getArg(0)->type() != MIRType_Int32)
            return InliningStatus_NotInlined;
        MDefinition *arg = callInfo.getArg(0)->toPassArg()->getArgument();
        if (!arg->isConstant())
            return InliningStatus_NotInlined;

        
        initLength = arg->toConstant()->value().toInt32();
        if (initLength >= JSObject::NELEMENTS_LIMIT)
            return InliningStatus_NotInlined;
    }

    callInfo.unwrapArgs();

    types::TemporaryTypeSet::DoubleConversion conversion =
        getInlineReturnTypeSet()->convertDoubleElements(constraints());
    if (conversion == types::TemporaryTypeSet::AlwaysConvertToDoubles)
        templateObject->setShouldConvertDoubleElements();

    MNewArray *ins = new MNewArray(initLength, templateObject, allocating);
    current->add(ins);
    current->push(ins);

    if (callInfo.argc() >= 2) {
        
        MElements *elements = MElements::New(ins);
        current->add(elements);

        
        
        
        
        
        MConstant *id = nullptr;
        for (uint32_t i = 0; i < initLength; i++) {
            id = MConstant::New(Int32Value(i));
            current->add(id);

            MDefinition *value = callInfo.getArg(i);
            if (conversion == types::TemporaryTypeSet::AlwaysConvertToDoubles) {
                MInstruction *valueDouble = MToDouble::New(value);
                current->add(valueDouble);
                value = valueDouble;
            }

            MStoreElement *store = MStoreElement::New(elements, id, value,
                                                       false);
            current->add(store);
        }

        
        MSetInitializedLength *length = MSetInitializedLength::New(elements, id);
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

    types::TemporaryTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    if (!thisTypes || thisTypes->getKnownClass() != &ArrayObject::class_)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(constraints(), unhandledFlags))
        return InliningStatus_NotInlined;

    RootedScript scriptRoot(cx, script());
    if (types::ArrayPrototypeHasIndexedProperty(constraints(), scriptRoot))
        return InliningStatus_NotInlined;

    callInfo.unwrapArgs();

    types::TemporaryTypeSet *returnTypes = getInlineReturnTypeSet();
    bool needsHoleCheck = thisTypes->hasObjectFlags(constraints(), types::OBJECT_FLAG_NON_PACKED);
    bool maybeUndefined = returnTypes->hasType(types::Type::UndefinedType());

    bool barrier = PropertyReadNeedsTypeBarrier(cx, context(), constraints(),
                                                callInfo.thisArg(), nullptr, returnTypes);
    if (barrier)
        returnType = MIRType_Value;

    MArrayPopShift *ins = MArrayPopShift::New(callInfo.thisArg(), mode,
                                              needsHoleCheck, maybeUndefined);
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
IonBuilder::inlineArrayPush(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    MDefinition *obj = callInfo.thisArg();
    MDefinition *value = callInfo.getArg(0);
    if (PropertyWriteNeedsTypeBarrier(constraints(), current,
                                      &obj, nullptr, &value,  false))
    {
        return InliningStatus_NotInlined;
    }
    JS_ASSERT(obj == callInfo.thisArg() && value == callInfo.getArg(0));

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

    RootedScript scriptRoot(cx, script());
    if (types::ArrayPrototypeHasIndexedProperty(constraints(), scriptRoot))
        return InliningStatus_NotInlined;

    types::TemporaryTypeSet::DoubleConversion conversion =
        thisTypes->convertDoubleElements(constraints());
    if (conversion == types::TemporaryTypeSet::AmbiguousDoubleConversion)
        return InliningStatus_NotInlined;

    callInfo.unwrapArgs();
    value = callInfo.getArg(0);

    if (conversion == types::TemporaryTypeSet::AlwaysConvertToDoubles ||
        conversion == types::TemporaryTypeSet::MaybeConvertToDoubles)
    {
        MInstruction *valueDouble = MToDouble::New(value);
        current->add(valueDouble);
        value = valueDouble;
    }

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(callInfo.thisArg(), value));

    MArrayPush *ins = MArrayPush::New(callInfo.thisArg(), value);
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

    
    RootedScript scriptRoot(cx, script());
    if (types::ArrayPrototypeHasIndexedProperty(constraints(), scriptRoot))
        return InliningStatus_NotInlined;

    
    
    if (thisTypes->getObjectCount() != 1)
        return InliningStatus_NotInlined;

    types::TypeObject *baseThisType = thisTypes->getTypeObject(0);
    if (!baseThisType)
        return InliningStatus_NotInlined;
    types::TypeObjectKey *thisType = types::TypeObjectKey::get(baseThisType);
    if (thisType->unknownProperties() ||
        &thisType->proto().toObject()->global() != &script()->global())
    {
        return InliningStatus_NotInlined;
    }

    
    
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

    
    JSObject *templateObj = inspector->getTemplateObjectForNative(pc, js::array_concat);
    if (!templateObj || templateObj->type() != baseThisType)
        return InliningStatus_NotInlined;
    JS_ASSERT(templateObj->is<ArrayObject>());

    callInfo.unwrapArgs();

    MArrayConcat *ins = MArrayConcat::New(callInfo.thisArg(), callInfo.getArg(0), templateObj);
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

    callInfo.unwrapArgs();

    
    
    MIRType absType = (argType == MIRType_Float32) ? MIRType_Double : argType;
    MInstruction *ins = MAbs::New(callInfo.getArg(0), absType);
    current->add(ins);

    if (IsFloatingPointType(argType) && returnType == MIRType_Int32) {
        MToInt32 *toInt = MToInt32::New(ins);
        toInt->setCanBeNegativeZero(false);
        current->add(toInt);
        ins = toInt;
    }

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
        callInfo.unwrapArgs();
        current->push(callInfo.getArg(0));
        return InliningStatus_Inlined;
    }

    if (argType == MIRType_Double && returnType == MIRType_Int32) {
        callInfo.unwrapArgs();
        MFloor *ins = new MFloor(callInfo.getArg(0));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (argType == MIRType_Double && returnType == MIRType_Double) {
        callInfo.unwrapArgs();
        MMathFunction *ins = MMathFunction::New(callInfo.getArg(0), MMathFunction::Floor, nullptr);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    return InliningStatus_NotInlined;
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
        callInfo.unwrapArgs();
        current->push(callInfo.getArg(0));
        return InliningStatus_Inlined;
    }

    if (argType == MIRType_Double && returnType == MIRType_Int32) {
        callInfo.unwrapArgs();
        MRound *ins = new MRound(callInfo.getArg(0));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    if (argType == MIRType_Double && returnType == MIRType_Double) {
        callInfo.unwrapArgs();
        MMathFunction *ins = MMathFunction::New(callInfo.getArg(0), MMathFunction::Round, nullptr);
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

    callInfo.unwrapArgs();

    MSqrt *sqrt = MSqrt::New(callInfo.getArg(0));
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

    callInfo.unwrapArgs();

    MAtan2 *atan2 = MAtan2::New(callInfo.getArg(0), callInfo.getArg(1));
    current->add(atan2);
    current->push(atan2);
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
    if (baseType != MIRType_Int32 && baseType != MIRType_Double)
        return InliningStatus_NotInlined;
    if (powerType != MIRType_Int32 && powerType != MIRType_Double)
        return InliningStatus_NotInlined;

    callInfo.unwrapArgs();

    MDefinition *base = callInfo.getArg(0);
    MDefinition *power = callInfo.getArg(1);
    MDefinition *output = nullptr;

    
    if (callInfo.getArg(1)->isConstant()) {
        double pow;
        Value v = callInfo.getArg(1)->toConstant()->value();
        if (!ToNumber(GetIonContext()->cx, v, &pow))
            return InliningStatus_Error;

        
        if (pow == 0.5) {
            MPowHalf *half = MPowHalf::New(base);
            current->add(half);
            output = half;
        }

        
        if (pow == -0.5) {
            MPowHalf *half = MPowHalf::New(base);
            current->add(half);
            MConstant *one = MConstant::New(DoubleValue(1.0));
            current->add(one);
            MDiv *div = MDiv::New(one, half, MIRType_Double);
            current->add(div);
            output = div;
        }

        
        if (pow == 1.0)
            output = base;

        
        if (pow == 2.0) {
            MMul *mul = MMul::New(base, base, outputType);
            current->add(mul);
            output = mul;
        }

        
        if (pow == 3.0) {
            MMul *mul1 = MMul::New(base, base, outputType);
            current->add(mul1);
            MMul *mul2 = MMul::New(base, mul1, outputType);
            current->add(mul2);
            output = mul2;
        }

        
        if (pow == 4.0) {
            MMul *y = MMul::New(base, base, outputType);
            current->add(y);
            MMul *mul = MMul::New(y, y, outputType);
            current->add(mul);
            output = mul;
        }
    }

    
    if (!output) {
        MPow *pow = MPow::New(base, power, powerType);
        current->add(pow);
        output = pow;
    }

    
    if (outputType == MIRType_Int32 && output->type() != MIRType_Int32) {
        MToInt32 *toInt = MToInt32::New(output);
        current->add(toInt);
        output = toInt;
    }
    if (outputType == MIRType_Double && output->type() != MIRType_Double) {
        MToDouble *toDouble = MToDouble::New(output);
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

    callInfo.unwrapArgs();

    MRandom *rand = MRandom::New();
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

    callInfo.unwrapArgs();

    MInstruction *first = MTruncateToInt32::New(callInfo.getArg(0));
    current->add(first);

    MInstruction *second = MTruncateToInt32::New(callInfo.getArg(1));
    current->add(second);

    MMul *ins = MMul::New(first, second, MIRType_Int32, MMul::Integer);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathFRound(CallInfo &callInfo)
{
    if (!LIRGenerator::allowFloat32Optimizations())
        return InliningStatus_NotInlined;

    if (callInfo.argc() != 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    
    
    MIRType returnType = getInlineReturnType();
    if (!IsNumberType(returnType))
        return InliningStatus_NotInlined;

    MIRType arg = callInfo.getArg(0)->type();
    if (!IsNumberType(arg))
        return InliningStatus_NotInlined;

    callInfo.unwrapArgs();

    MToFloat32 *ins = MToFloat32::New(callInfo.getArg(0));
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathMinMax(CallInfo &callInfo, bool max)
{
    if (callInfo.argc() != 2 || callInfo.constructing())
        return InliningStatus_NotInlined;

    MIRType returnType = getInlineReturnType();
    if (!IsNumberType(returnType))
        return InliningStatus_NotInlined;

    MIRType arg0Type = callInfo.getArg(0)->type();
    if (!IsNumberType(arg0Type))
        return InliningStatus_NotInlined;
    MIRType arg1Type = callInfo.getArg(1)->type();
    if (!IsNumberType(arg1Type))
        return InliningStatus_NotInlined;

    if (returnType == MIRType_Int32 &&
        (arg0Type == MIRType_Double || arg1Type == MIRType_Double))
    {
        
        return InliningStatus_NotInlined;
    }

    callInfo.unwrapArgs();

    MMinMax *ins = MMinMax::New(callInfo.getArg(0), callInfo.getArg(1), returnType, max);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStringObject(CallInfo &callInfo)
{
    if (callInfo.argc() != 1 || !callInfo.constructing())
        return InliningStatus_NotInlined;

    
    MIRType type = callInfo.getArg(0)->type();
    if (type != MIRType_Int32 && type != MIRType_String)
        return InliningStatus_NotInlined;

    JSObject *templateObj = inspector->getTemplateObjectForNative(pc, js_String);
    if (!templateObj)
        return InliningStatus_NotInlined;
    JS_ASSERT(templateObj->is<StringObject>());

    callInfo.unwrapArgs();

    MNewStringObject *ins = MNewStringObject::New(callInfo.getArg(0), templateObj);
    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return InliningStatus_Error;

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

    callInfo.unwrapArgs();

    MInstruction *index = MToInt32::New(callInfo.getArg(0));
    current->add(index);

    MStringLength *length = MStringLength::New(callInfo.thisArg());
    current->add(length);

    index = addBoundsCheck(index, length);

    MCharCodeAt *charCode = MCharCodeAt::New(callInfo.thisArg(), index);
    current->add(charCode);
    current->push(charCode);
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

    callInfo.unwrapArgs();

    MToInt32 *charCode = MToInt32::New(callInfo.getArg(0));
    current->add(charCode);

    MFromCharCode *string = MFromCharCode::New(charCode);
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

    callInfo.unwrapArgs();

    MInstruction *index = MToInt32::New(callInfo.getArg(0));
    current->add(index);

    MStringLength *length = MStringLength::New(callInfo.thisArg());
    current->add(length);

    index = addBoundsCheck(index, length);

    
    MCharCodeAt *charCode = MCharCodeAt::New(callInfo.thisArg(), index);
    current->add(charCode);

    MFromCharCode *string = MFromCharCode::New(charCode);
    current->add(string);
    current->push(string);
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
    if (callInfo.getArg(0)->type() != MIRType_String)
        return InliningStatus_NotInlined;

    callInfo.unwrapArgs();

    MInstruction *match = MRegExpTest::New(callInfo.thisArg(), callInfo.getArg(0));
    current->add(match);
    current->push(match);
    if (!resumeAfter(match))
        return InliningStatus_Error;

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
            writeNeedsBarrier = PropertyWriteNeedsTypeBarrier(constraints(), current,
                                                              &obj, nullptr, &elem,
                                                               false);
        }

        
        
        ScalarTypeRepresentation::Type arrayType;
        if ((!isDenseNative || writeNeedsBarrier) &&
            !ElementAccessIsTypedArray(obj, id, &arrayType))
        {
            return InliningStatus_NotInlined;
        }
    }

    callInfo.unwrapArgs();

    
    
    MConstant *udef = MConstant::New(UndefinedValue());
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

        ScalarTypeRepresentation::Type arrayType;
        if (ElementAccessIsTypedArray(obj, id, &arrayType)) {
            if (!inlineUnsafeSetTypedArrayElement(callInfo, base, arrayType))
                return InliningStatus_Error;
            continue;
        }

        MOZ_ASSUME_UNREACHABLE("Element access not dense array nor typed array");
    }

    return InliningStatus_Inlined;
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
                                             ScalarTypeRepresentation::Type arrayType)
{
    
    
    
    

    MDefinition *obj = callInfo.getArg(base + 0);
    MDefinition *id = callInfo.getArg(base + 1);
    MDefinition *elem = callInfo.getArg(base + 2);

    if (!jsop_setelem_typed(arrayType, SetElem_Unsafe, obj, id, elem))
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
      case SequentialExecution:
      case DefinitePropertiesAnalysis:
        
        
        return InliningStatus_NotInlined;

      case ParallelExecution: {
        
        
        
        callInfo.unwrapArgs();
        MConstant *ins = MConstant::New(BooleanValue(true));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
      }
    }

    MOZ_ASSUME_UNREACHABLE("Invalid execution mode");
}

IonBuilder::InliningStatus
IonBuilder::inlineNewParallelArray(CallInfo &callInfo)
{
    
    
    
    
    
    
    
    

    uint32_t argc = callInfo.argc();
    if (argc < 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    types::TemporaryTypeSet *ctorTypes = callInfo.getArg(0)->resultTypeSet();
    JSObject *targetObj = ctorTypes ? ctorTypes->getSingleton() : nullptr;
    RootedFunction target(cx);
    if (targetObj && targetObj->is<JSFunction>())
        target = &targetObj->as<JSFunction>();
    if (target && target->isInterpreted() && target->nonLazyScript()->shouldCloneAtCallsite) {
        RootedScript scriptRoot(cx, script());
        target = CloneFunctionAtCallsite(cx, target, scriptRoot, pc);
        if (!target)
            return InliningStatus_Error;
    }
    MDefinition *ctor = makeCallsiteClone(
        target,
        callInfo.getArg(0)->toPassArg()->getArgument());

    
    return inlineParallelArrayTail(callInfo, target, ctor,
                                   target ? nullptr : ctorTypes, 1);
}

IonBuilder::InliningStatus
IonBuilder::inlineParallelArray(CallInfo &callInfo)
{
    if (!callInfo.constructing())
        return InliningStatus_NotInlined;

    uint32_t argc = callInfo.argc();
    RootedFunction target(cx, ParallelArrayObject::getConstructor(cx, argc));
    if (!target)
        return InliningStatus_Error;

    JS_ASSERT(target->nonLazyScript()->shouldCloneAtCallsite);
    RootedScript script(cx, script_);
    target = CloneFunctionAtCallsite(cx, target, script, pc);
    if (!target)
        return InliningStatus_Error;

    MConstant *ctor = MConstant::New(ObjectValue(*target));
    current->add(ctor);

    return inlineParallelArrayTail(callInfo, target, ctor, nullptr, 0);
}

IonBuilder::InliningStatus
IonBuilder::inlineParallelArrayTail(CallInfo &callInfo,
                                    HandleFunction target,
                                    MDefinition *ctor,
                                    types::TemporaryTypeSet *ctorTypes,
                                    uint32_t discards)
{
    
    
    

    uint32_t argc = callInfo.argc() - discards;

    
    
    
    types::TemporaryTypeSet *returnTypes = getInlineReturnTypeSet();
    if (returnTypes->getKnownTypeTag() != JSVAL_TYPE_OBJECT)
        return InliningStatus_NotInlined;
    if (returnTypes->unknownObject() || returnTypes->getObjectCount() != 1)
        return InliningStatus_NotInlined;
    types::TypeObject *typeObject = returnTypes->getTypeObject(0);
    if (!typeObject || typeObject->clasp != &ParallelArrayObject::class_)
        return InliningStatus_NotInlined;

    
    uint32_t targetArgs = argc;
    if (target && !target->isNative())
        targetArgs = Max<uint32_t>(target->nargs, argc);

    MCall *call = MCall::New(target, targetArgs + 1, argc, false);
    if (!call)
        return InliningStatus_Error;

    callInfo.unwrapArgs();

    
    
    for (uint32_t i = targetArgs; i > argc; i--) {
        JS_ASSERT_IF(target, !target->isNative());
        MConstant *undef = MConstant::New(UndefinedValue());
        current->add(undef);
        MPassArg *pass = MPassArg::New(undef);
        current->add(pass);
        call->addArg(i, pass);
    }

    MPassArg *oldThis = MPassArg::New(callInfo.thisArg());
    current->add(oldThis);

    
    
    for (uint32_t i = 0; i < argc; i++) {
        MDefinition *arg = callInfo.getArg(i + discards);
        MPassArg *passArg = MPassArg::New(arg);
        current->add(passArg);
        call->addArg(i + 1, passArg);
    }

    
    
    MPrepareCall *start = new MPrepareCall;
    oldThis->block()->insertBefore(oldThis, start);
    call->initPrepareCall(start);

    
    
    JSObject *templateObject = ParallelArrayObject::newInstance(cx, TenuredObject);
    if (!templateObject)
        return InliningStatus_Error;
    templateObject->setType(typeObject);
    MNewParallelArray *newObject = MNewParallelArray::New(templateObject);
    current->add(newObject);
    MPassArg *newThis = MPassArg::New(newObject);
    current->add(newThis);
    call->addArg(0, newThis);

    
    call->initFunction(ctor);

    current->add(call);
    current->push(newObject);

    if (!resumeAfter(call))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineNewDenseArray(CallInfo &callInfo)
{
    if (callInfo.constructing() || callInfo.argc() != 1)
        return InliningStatus_NotInlined;

    
    
    ExecutionMode executionMode = info().executionMode();
    switch (executionMode) {
      case SequentialExecution:
      case DefinitePropertiesAnalysis:
        return inlineNewDenseArrayForSequentialExecution(callInfo);
      case ParallelExecution:
        return inlineNewDenseArrayForParallelExecution(callInfo);
    }

    MOZ_ASSUME_UNREACHABLE("unknown ExecutionMode");
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
    if (returnTypes->getKnownTypeTag() != JSVAL_TYPE_OBJECT)
        return InliningStatus_NotInlined;
    if (returnTypes->unknownObject() || returnTypes->getObjectCount() != 1)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;
    types::TypeObject *typeObject = returnTypes->getTypeObject(0);

    JSObject *templateObject = NewDenseAllocatedArray(cx, 0, nullptr, TenuredObject);
    if (!templateObject)
        return InliningStatus_Error;
    templateObject->setType(typeObject);

    callInfo.unwrapArgs();

    MNewDenseArrayPar *newObject = new MNewDenseArrayPar(graph().forkJoinSlice(),
                                                         callInfo.getArg(0),
                                                         templateObject);
    current->add(newObject);
    current->push(newObject);

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

    
    MDefinition *arg = callInfo.getArg(1)->toPassArg()->getArgument();
    if (!arg->isConstant())
        return InliningStatus_NotInlined;
    uint32_t slot = arg->toConstant()->value().toPrivateUint32();

    callInfo.unwrapArgs();

    MStoreFixedSlot *store = MStoreFixedSlot::New(callInfo.getArg(0), slot, callInfo.getArg(2));
    current->add(store);
    current->push(store);

    if (NeedsPostBarrier(info(), callInfo.getArg(2)))
        current->add(MPostWriteBarrier::New(callInfo.thisArg(), callInfo.getArg(2)));

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineUnsafeGetReservedSlot(CallInfo &callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing())
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(1)->type() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    MDefinition *arg = callInfo.getArg(1)->toPassArg()->getArgument();
    if (!arg->isConstant())
        return InliningStatus_NotInlined;
    uint32_t slot = arg->toConstant()->value().toPrivateUint32();

    callInfo.unwrapArgs();

    MLoadFixedSlot *load = MLoadFixedSlot::New(callInfo.getArg(0), slot);
    current->add(load);
    current->push(load);

    
    pushTypeBarrier(load, getInlineReturnTypeSet(), true);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineHaveSameClass(CallInfo &callInfo)
{
    if (callInfo.argc() != 2 || callInfo.constructing())
        return InliningStatus_NotInlined;
    if (callInfo.getArg(0)->type() != MIRType_Object)
        return InliningStatus_NotInlined;
    if (callInfo.getArg(1)->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    types::TemporaryTypeSet *arg1Types = callInfo.getArg(0)->resultTypeSet();
    types::TemporaryTypeSet *arg2Types = callInfo.getArg(1)->resultTypeSet();
    const Class *arg1Clasp = arg1Types ? arg1Types->getKnownClass() : nullptr;
    const Class *arg2Clasp = arg2Types ? arg2Types->getKnownClass() : nullptr;
    if (arg1Clasp && arg2Clasp) {
        MConstant *constant = MConstant::New(BooleanValue(arg1Clasp == arg2Clasp));
        current->add(constant);
        current->push(constant);
        return InliningStatus_Inlined;
    }

    callInfo.unwrapArgs();

    MHaveSameClass *sameClass = MHaveSameClass::New(callInfo.getArg(0), callInfo.getArg(1));
    current->add(sameClass);
    current->push(sameClass);

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
        if (clasp) {
            isCallableKnown = true;
            isCallableConstant = clasp->isCallable();
        }
    }

    if (isCallableKnown) {
        MConstant *constant = MConstant::New(BooleanValue(isCallableConstant));
        current->add(constant);
        current->push(constant);
        return InliningStatus_Inlined;
    }

    callInfo.unwrapArgs();

    MIsCallable *isCallable = MIsCallable::New(callInfo.getArg(0));
    current->add(isCallable);
    current->push(isCallable);

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

    callInfo.unwrapArgs();
    MDefinition *object = callInfo.getArg(0);

    current->push(object);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineBailout(CallInfo &callInfo)
{
    callInfo.unwrapArgs();

    current->add(MBail::New());

    MConstant *undefined = MConstant::New(UndefinedValue());
    current->add(undefined);
    current->push(undefined);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineAssertFloat32(CallInfo &callInfo)
{
    callInfo.unwrapArgs();

    MDefinition *secondArg = callInfo.getArg(1);

    JS_ASSERT(secondArg->type() == MIRType_Boolean);
    JS_ASSERT(secondArg->isConstant());

    bool mustBeFloat32 = JSVAL_TO_BOOLEAN(secondArg->toConstant()->value());
    current->add(MAssertFloat32::New(callInfo.getArg(0), mustBeFloat32));

    MConstant *undefined = MConstant::New(UndefinedValue());
    current->add(undefined);
    current->push(undefined);
    return InliningStatus_Inlined;
}

} 
} 
