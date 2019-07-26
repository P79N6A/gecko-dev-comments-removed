






#include "jslibmath.h"
#include "jsmath.h"
#include "builtin/ParallelArray.h"
#include "builtin/TestingFunctions.h"

#include "MIR.h"
#include "MIRGraph.h"
#include "IonBuilder.h"

#include "vm/StringObject-inl.h"

namespace js {
namespace ion {

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
    if (native == js_math_floor)
        return inlineMathFloor(callInfo);
    if (native == js_math_round)
        return inlineMathRound(callInfo);
    if (native == js_math_sqrt)
        return inlineMathSqrt(callInfo);
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

    
    if (native == intrinsic_UnsafeSetElement)
        return inlineUnsafeSetElement(callInfo);
    if (native == testingFunc_inParallelSection)
        return inlineShouldForceSequentialOrInParallelSection(callInfo);
    if (native == intrinsic_NewDenseArray)
        return inlineNewDenseArray(callInfo);

    
    if (native == intrinsic_ThrowError)
        return inlineThrowError(callInfo);
#ifdef DEBUG
    if (native == intrinsic_Dump)
        return inlineDump(callInfo);
#endif

    
    if (native == intrinsic_UnsafeSetElement)
        return inlineUnsafeSetElement(callInfo);
    if (native == intrinsic_ShouldForceSequential)
        return inlineShouldForceSequentialOrInParallelSection(callInfo);
    if (native == testingFunc_inParallelSection)
        return inlineShouldForceSequentialOrInParallelSection(callInfo);
    if (native == intrinsic_NewParallelArray)
        return inlineNewParallelArray(callInfo);
    if (native == ParallelArrayObject::construct)
        return inlineParallelArray(callInfo);
    if (native == intrinsic_NewDenseArray)
        return inlineNewDenseArray(callInfo);

    
    if (native == intrinsic_ThrowError)
        return inlineThrowError(callInfo);
#ifdef DEBUG
    if (native == intrinsic_Dump)
        return inlineDump(callInfo);
#endif

    return InliningStatus_NotInlined;
}

types::StackTypeSet *
IonBuilder::getInlineReturnTypeSet()
{
    return script()->analysis()->bytecodeTypes(pc);
}

MIRType
IonBuilder::getInlineReturnType()
{
    types::StackTypeSet *returnTypes = getInlineReturnTypeSet();
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

    MathCache *cache = cx->runtime->getMathCache(cx);
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

    
    if (callInfo.argc() >= 2) {
        initLength = callInfo.argc();
        allocating = MNewArray::NewArray_Allocating;

        types::TypeObject *type = types::TypeScript::InitObject(cx, script(), pc, JSProto_Array);
        if (!type)
            return InliningStatus_Error;
        if (!type->unknownProperties()) {
            types::HeapTypeSet *elemTypes = type->getProperty(cx, JSID_VOID, false);
            if (!elemTypes)
                return InliningStatus_Error;

            for (uint32_t i = 0; i < initLength; i++) {
                MDefinition *value = callInfo.getArg(i);
                if (!TypeSetIncludes(elemTypes, value->type(), value->resultTypeSet())) {
                    elemTypes->addFreeze(cx);
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

    JSObject *templateObject = getNewArrayTemplateObject(initLength);
    if (!templateObject)
        return InliningStatus_Error;

    types::StackTypeSet::DoubleConversion conversion =
        getInlineReturnTypeSet()->convertDoubleElements(cx);
    if (conversion == types::StackTypeSet::AlwaysConvertToDoubles)
        templateObject->setShouldConvertDoubleElements();

    MNewArray *ins = new MNewArray(initLength, templateObject, allocating);
    current->add(ins);
    current->push(ins);

    if (callInfo.argc() >= 2) {
        
        MElements *elements = MElements::New(ins);
        current->add(elements);

        
        
        
        MConstant *id = NULL;
        for (uint32_t i = 0; i < initLength; i++) {
            id = MConstant::New(Int32Value(i));
            current->add(id);

            MDefinition *value = callInfo.getArg(i);
            if (conversion == types::StackTypeSet::AlwaysConvertToDoubles) {
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

    types::StackTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    if (!thisTypes || thisTypes->getKnownClass() != &ArrayClass)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(cx, unhandledFlags))
        return InliningStatus_NotInlined;
    RootedScript script(cx, script_);
    if (types::ArrayPrototypeHasIndexedProperty(cx, script))
        return InliningStatus_NotInlined;

    callInfo.unwrapArgs();

    types::StackTypeSet *returnTypes = getInlineReturnTypeSet();
    bool needsHoleCheck = thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED);
    bool maybeUndefined = returnTypes->hasType(types::Type::UndefinedType());

    bool barrier = propertyReadNeedsTypeBarrier(callInfo.thisArg(), NULL, returnTypes);
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

    MDefinition *value = callInfo.getArg(0);
    if (propertyWriteNeedsTypeBarrier(callInfo.thisArg(), NULL, &value) || value != callInfo.getArg(0))
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;
    if (callInfo.thisArg()->type() != MIRType_Object)
        return InliningStatus_NotInlined;

    types::StackTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    if (!thisTypes || thisTypes->getKnownClass() != &ArrayClass)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_SPARSE_INDEXES |
                                  types::OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        return InliningStatus_NotInlined;
    }
    RootedScript script(cx, script_);
    if (types::ArrayPrototypeHasIndexedProperty(cx, script))
        return InliningStatus_NotInlined;

    types::StackTypeSet::DoubleConversion conversion = thisTypes->convertDoubleElements(cx);
    if (conversion == types::StackTypeSet::AmbiguousDoubleConversion)
        return InliningStatus_NotInlined;

    callInfo.unwrapArgs();
    value = callInfo.getArg(0);

    if (conversion == types::StackTypeSet::AlwaysConvertToDoubles ||
        conversion == types::StackTypeSet::MaybeConvertToDoubles)
    {
        MInstruction *valueDouble = MToDouble::New(value);
        current->add(valueDouble);
        value = valueDouble;
    }

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

    
    types::StackTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    types::StackTypeSet *argTypes = callInfo.getArg(0)->resultTypeSet();
    if (!thisTypes || !argTypes)
        return InliningStatus_NotInlined;

    if (thisTypes->getKnownClass() != &ArrayClass)
        return InliningStatus_NotInlined;
    if (thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_SPARSE_INDEXES |
                                  types::OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        return InliningStatus_NotInlined;
    }

    if (argTypes->getKnownClass() != &ArrayClass)
        return InliningStatus_NotInlined;
    if (argTypes->hasObjectFlags(cx, types::OBJECT_FLAG_SPARSE_INDEXES |
                                 types::OBJECT_FLAG_LENGTH_OVERFLOW))
    {
        return InliningStatus_NotInlined;
    }

    
    RootedScript script(cx, script_);
    if (types::ArrayPrototypeHasIndexedProperty(cx, script))
        return InliningStatus_NotInlined;

    
    
    if (thisTypes->getObjectCount() != 1)
        return InliningStatus_NotInlined;

    types::TypeObject *thisType = thisTypes->getTypeObject(0);
    if (!thisType ||
        thisType->unknownProperties() ||
        &thisType->proto->global() != &script->global())
    {
        return InliningStatus_NotInlined;
    }

    
    
    
    types::HeapTypeSet *thisElemTypes = thisType->getProperty(cx, JSID_VOID, false);
    if (!thisElemTypes)
        return InliningStatus_Error;

    types::StackTypeSet *resTypes = getInlineReturnTypeSet();
    if (!resTypes->hasType(types::Type::ObjectType(thisType)))
        return InliningStatus_NotInlined;

    for (unsigned i = 0; i < argTypes->getObjectCount(); i++) {
        if (argTypes->getSingleObject(i))
            return InliningStatus_NotInlined;

        types::TypeObject *argType = argTypes->getTypeObject(i);
        if (!argType)
            continue;

        if (argType->unknownProperties())
            return InliningStatus_NotInlined;

        types::HeapTypeSet *elemTypes = argType->getProperty(cx, JSID_VOID, false);
        if (!elemTypes)
            return InliningStatus_Error;

        if (!elemTypes->knownSubset(cx, thisElemTypes))
            return InliningStatus_NotInlined;
    }

    
    RootedObject templateObj(cx, NewDenseEmptyArray(cx, thisType->proto));
    if (!templateObj)
        return InliningStatus_Error;
    templateObj->setType(thisType);

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
    if (argType != MIRType_Int32 && argType != MIRType_Double)
        return InliningStatus_NotInlined;

    if (argType != returnType && returnType != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.unwrapArgs();

    MInstruction *ins = MAbs::New(callInfo.getArg(0), argType);
    current->add(ins);

    if (argType != returnType) {
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
    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    if (argType == MIRType_Int32) {
        callInfo.unwrapArgs();
        current->push(callInfo.getArg(0));
        return InliningStatus_Inlined;
    }

    if (argType == MIRType_Double) {
        callInfo.unwrapArgs();
        MFloor *ins = new MFloor(callInfo.getArg(0));
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
    if (argType != MIRType_Double && argType != MIRType_Int32)
        return InliningStatus_NotInlined;

    callInfo.unwrapArgs();

    MSqrt *sqrt = MSqrt::New(callInfo.getArg(0));
    current->add(sqrt);
    current->push(sqrt);
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
    MDefinition *output = NULL;

    
    if (callInfo.getArg(1)->isConstant()) {
        double pow;
        if (!ToNumber(GetIonContext()->cx, callInfo.getArg(1)->toConstant()->value(), &pow))
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

    callInfo.unwrapArgs();

    RootedString emptyString(cx, cx->runtime->emptyString);
    RootedObject templateObj(cx, StringObject::create(cx, emptyString));
    if (!templateObj)
        return InliningStatus_Error;

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
    types::StackTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
    Class *clasp = thisTypes ? thisTypes->getKnownClass() : NULL;
    if (clasp != &RegExpClass)
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
IonBuilder::inlineUnsafeSetElement(CallInfo &callInfo)
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

        if (propertyWriteNeedsTypeBarrier(obj, NULL, &elem))
            return InliningStatus_NotInlined;

        int arrayType;
        if (!elementAccessIsDenseNative(obj, id) &&
            !elementAccessIsTypedArray(obj, id, &arrayType))
        {
            return InliningStatus_NotInlined;
        }

        if (obj->resultTypeSet()->convertDoubleElements(cx) !=
            types::StackTypeSet::DontConvertToDoubles)
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

        if (elementAccessIsDenseNative(obj, id)) {
            if (!inlineUnsafeSetDenseArrayElement(callInfo, base))
                return InliningStatus_Error;
            continue;
        }

        int arrayType;
        if (elementAccessIsTypedArray(obj, id, &arrayType)) {
            if (!inlineUnsafeSetTypedArrayElement(callInfo, base, arrayType))
                return InliningStatus_Error;
            continue;
        }

        JS_NOT_REACHED("Element access not dense array nor typed array");
    }

    return InliningStatus_Inlined;
}

bool
IonBuilder::inlineUnsafeSetDenseArrayElement(CallInfo &callInfo, uint32_t base)
{
    
    
    
    
    
    

    uint32_t arri = base + 0;
    uint32_t idxi = base + 1;
    uint32_t elemi = base + 2;

    MElements *elements = MElements::New(callInfo.getArg(arri));
    current->add(elements);

    MToInt32 *id = MToInt32::New(callInfo.getArg(idxi));
    current->add(id);

    
    
    

    MStoreElement *store = MStoreElement::New(elements, id,
                                              callInfo.getArg(elemi),
                                               false);
    store->setRacy();

    if (callInfo.getArg(arri)->resultTypeSet()->propertyNeedsBarrier(cx, JSID_VOID))
        store->setNeedsBarrier();

    current->add(store);

    if (!resumeAfter(store))
        return false;

    return true;
}

bool
IonBuilder::inlineUnsafeSetTypedArrayElement(CallInfo &callInfo,
                                             uint32_t base,
                                             int arrayType)
{
    
    
    
    

    uint32_t arri = base + 1;
    uint32_t idxi = base + 2;
    uint32_t elemi = base + 3;

    MInstruction *elements = getTypedArrayElements(callInfo.getArg(arri));
    current->add(elements);

    MToInt32 *id = MToInt32::New(callInfo.getArg(idxi));
    current->add(id);

    MDefinition *value = callInfo.getArg(elemi);
    if (arrayType == TypedArray::TYPE_UINT8_CLAMPED) {
        value = MClampToUint8::New(value);
        current->add(value->toInstruction());
    }

    MStoreTypedArrayElement *store = MStoreTypedArrayElement::New(elements, id, value, arrayType);
    store->setRacy();

    current->add(store);

    if (!resumeAfter(store))
        return false;

    return true;
}

IonBuilder::InliningStatus
IonBuilder::inlineShouldForceSequentialOrInParallelSection(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    ExecutionMode executionMode = info().executionMode();
    switch (executionMode) {
      case SequentialExecution:
        
        
        return InliningStatus_NotInlined;

      case ParallelExecution:
        
        
        
        callInfo.unwrapArgs();
        MConstant *ins = MConstant::New(BooleanValue(true));
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    JS_NOT_REACHED("Invalid execution mode");
}

IonBuilder::InliningStatus
IonBuilder::inlineNewParallelArray(CallInfo &callInfo)
{
    
    
    
    
    
    
    
    

    uint32_t argc = callInfo.argc();
    if (argc < 1 || callInfo.constructing())
        return InliningStatus_NotInlined;

    types::StackTypeSet *ctorTypes = callInfo.getArg(0)->resultTypeSet();
    RawObject targetObj = ctorTypes ? ctorTypes->getSingleton() : NULL;
    RootedFunction target(cx);
    if (targetObj && targetObj->isFunction())
        target = targetObj->toFunction();
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
                                   target ? NULL : ctorTypes, 1);
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

    return inlineParallelArrayTail(callInfo, target, ctor, NULL, 0);
}

IonBuilder::InliningStatus
IonBuilder::inlineParallelArrayTail(CallInfo &callInfo,
                                    HandleFunction target,
                                    MDefinition *ctor,
                                    types::StackTypeSet *ctorTypes,
                                    uint32_t discards)
{
    
    
    

    uint32_t argc = callInfo.argc() - discards;

    
    
    
    types::StackTypeSet *returnTypes = getInlineReturnTypeSet();
    if (returnTypes->getKnownTypeTag() != JSVAL_TYPE_OBJECT)
        return InliningStatus_NotInlined;
    if (returnTypes->unknownObject() || returnTypes->getObjectCount() != 1)
        return InliningStatus_NotInlined;
    types::TypeObject *typeObject = returnTypes->getTypeObject(0);

    
    uint32_t targetArgs = argc;
    if (target && !target->isNative())
        targetArgs = Max<uint32_t>(target->nargs, argc);

    MCall *call = MCall::New(target, targetArgs + 1, argc, false);
    if (!call)
        return InliningStatus_Error;

    
    if (target && target->isInterpreted()) {
        if (!target->getOrCreateScript(cx))
            return InliningStatus_Error;
        call->rootTargetScript(target);
    }

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

    
    
    RootedObject templateObject(cx, ParallelArrayObject::newInstance(cx));
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
        return inlineNewDenseArrayForSequentialExecution(callInfo);
      case ParallelExecution:
        return inlineNewDenseArrayForParallelExecution(callInfo);
    }

    JS_NOT_REACHED("unknown ExecutionMode");
}

IonBuilder::InliningStatus
IonBuilder::inlineNewDenseArrayForSequentialExecution(CallInfo &callInfo)
{
    
    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineNewDenseArrayForParallelExecution(CallInfo &callInfo)
{
    
    
    
    types::StackTypeSet *returnTypes = getInlineReturnTypeSet();
    if (returnTypes->getKnownTypeTag() != JSVAL_TYPE_OBJECT)
        return InliningStatus_NotInlined;
    if (returnTypes->unknownObject() || returnTypes->getObjectCount() != 1)
        return InliningStatus_NotInlined;
    types::TypeObject *typeObject = returnTypes->getTypeObject(0);

    RootedObject templateObject(cx, NewDenseAllocatedArray(cx, 0));
    if (!templateObject)
        return InliningStatus_Error;
    templateObject->setType(typeObject);

    callInfo.unwrapArgs();

    MParNewDenseArray *newObject = new MParNewDenseArray(graph().parSlice(),
                                                         callInfo.getArg(0),
                                                         templateObject);
    current->add(newObject);
    current->push(newObject);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineThrowError(CallInfo &callInfo)
{
    

    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    ExecutionMode executionMode = info().executionMode();
    switch (executionMode) {
      case SequentialExecution:
        return InliningStatus_NotInlined;
      case ParallelExecution:
        break;
    }

    callInfo.unwrapArgs();

    MParBailout *bailout = new MParBailout();
    if (!bailout)
        return InliningStatus_Error;
    current->end(bailout);

    setCurrent(newBlock(pc));
    if (!current)
        return InliningStatus_Error;

    MConstant *udef = MConstant::New(UndefinedValue());
    current->add(udef);
    current->push(udef);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineDump(CallInfo &callInfo)
{
    
    

    if (callInfo.constructing())
        return InliningStatus_NotInlined;

    ExecutionMode executionMode = info().executionMode();
    switch (executionMode) {
      case SequentialExecution:
        return InliningStatus_NotInlined;
      case ParallelExecution:
        break;
    }

    callInfo.unwrapArgs();

    MParDump *dump = new MParDump(callInfo.getArg(1));
    current->add(dump);

    MConstant *udef = MConstant::New(UndefinedValue());
    current->add(udef);
    current->push(udef);

    return InliningStatus_Inlined;
}

} 
} 
