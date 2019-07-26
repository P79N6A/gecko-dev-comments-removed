






#include "jslibmath.h"
#include "jsmath.h"

#include "MIR.h"
#include "MIRGraph.h"
#include "IonBuilder.h"

namespace js {
namespace ion {

IonBuilder::InliningStatus
IonBuilder::inlineNativeCall(JSNative native, uint32 argc, bool constructing)
{
    
    if (native == js_Array)
        return inlineArray(argc, constructing);
    if (native == js::array_pop)
        return inlineArrayPopShift(MArrayPopShift::Pop, argc, constructing);
    if (native == js::array_shift)
        return inlineArrayPopShift(MArrayPopShift::Shift, argc, constructing);
    if (native == js::array_push)
        return inlineArrayPush(argc, constructing);

    
    if (native == js_math_abs)
        return inlineMathAbs(argc, constructing);
    if (native == js_math_floor)
        return inlineMathFloor(argc, constructing);
    if (native == js_math_round)
        return inlineMathRound(argc, constructing);
    if (native == js_math_sqrt)
        return inlineMathSqrt(argc, constructing);
    if (native == js_math_pow)
        return inlineMathPow(argc, constructing);
    if (native == js::math_sin)
        return inlineMathFunction(MMathFunction::Sin, argc, constructing);
    if (native == js::math_cos)
        return inlineMathFunction(MMathFunction::Cos, argc, constructing);
    if (native == js::math_tan)
        return inlineMathFunction(MMathFunction::Tan, argc, constructing);
    if (native == js::math_log)
        return inlineMathFunction(MMathFunction::Log, argc, constructing);

    
    if (native == js_str_charCodeAt)
        return inlineStrCharCodeAt(argc, constructing);
    if (native == js::str_fromCharCode)
        return inlineStrFromCharCode(argc, constructing);
    if (native == js_str_charAt)
        return inlineStrCharAt(argc, constructing);

    return InliningStatus_NotInlined;
}

bool
IonBuilder::discardCallArgs(uint32 argc, MDefinitionVector &argv, MBasicBlock *bb)
{
    if (!argv.resizeUninitialized(argc + 1))
        return false;

    for (int32 i = argc; i >= 0; i--) {
        
        MPassArg *passArg = bb->pop()->toPassArg();
        MBasicBlock *block = passArg->block();
        MDefinition *wrapped = passArg->getArgument();
        passArg->replaceAllUsesWith(wrapped);
        block->discard(passArg);

        
        argv[i] = wrapped;
    }

    return true;
}

bool
IonBuilder::discardCall(uint32 argc, MDefinitionVector &argv, MBasicBlock *bb)
{
    if (!discardCallArgs(argc, argv, bb))
        return false;

    
    bb->pop();
    return true;
}

types::TypeSet*
IonBuilder::getInlineReturnTypeSet()
{
    types::TypeSet *barrier;
    types::TypeSet *returnTypes = oracle->returnTypeSet(script, pc, &barrier);

    JS_ASSERT(returnTypes);
    return returnTypes;
}

MIRType
IonBuilder::getInlineReturnType()
{
    types::TypeSet *returnTypes = getInlineReturnTypeSet();
    return MIRTypeFromValueType(returnTypes->getKnownTypeTag(cx));
}

types::TypeSet*
IonBuilder::getInlineArgTypeSet(uint32 argc, uint32 arg)
{
    types::TypeSet *argTypes = oracle->getCallArg(script, argc, arg, pc);
    JS_ASSERT(argTypes);
    return argTypes;
}

MIRType
IonBuilder::getInlineArgType(uint32 argc, uint32 arg)
{
    types::TypeSet *argTypes = getInlineArgTypeSet(argc, arg);
    return MIRTypeFromValueType(argTypes->getKnownTypeTag(cx));
}

IonBuilder::InliningStatus
IonBuilder::inlineNanResult(int argc)
{
    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;
    MConstant *nan = MConstant::New(GetIonContext()->cx->runtime->NaNValue);
    current->add(nan);
    current->push(nan);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathFunction(MMathFunction::Function function, uint32 argc, bool constructing)
{
    if (constructing)
        return InliningStatus_NotInlined;

    
    if (argc == 0)
        return inlineNanResult(argc);

    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;
    if (!IsNumberType(getInlineArgType(argc, 1)))
        return InliningStatus_NotInlined;

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    MathCache *cache = cx->runtime->getMathCache(cx);
    if (!cache)
        return InliningStatus_Error;

    MMathFunction *ins = MMathFunction::New(argv[1], function, cache);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArray(uint32 argc, bool constructing)
{
    uint32_t initLength = 0;

    
    if (argc >= 2)
        initLength = argc;

    
    if (argc == 1) {
        if (getInlineArgType(argc, 1) != MIRType_Int32)
            return InliningStatus_NotInlined;
        MDefinition *arg = current->peek(-1)->toPassArg()->getArgument();
        if (!arg->isConstant())
            return InliningStatus_NotInlined;

        
        initLength = arg->toConstant()->value().toInt32();
        if (initLength >= JSObject::NELEMENTS_LIMIT)
            return InliningStatus_NotInlined;
    }

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    JSObject *templateObject = getNewArrayTemplateObject(initLength);
    if (!templateObject)
        return InliningStatus_Error;

    MNewArray *ins = new MNewArray(initLength, templateObject, MNewArray::NewArray_Unallocating);
    current->add(ins);
    current->push(ins);

    if (argc >= 2) {
        
        MElements *elements = MElements::New(ins);
        current->add(elements);

        
        
        
        MConstant *id = NULL;
        for (uint32_t i = 0; i < initLength; i++) {
            id = MConstant::New(Int32Value(i));
            current->add(id);

            MStoreElement *store = MStoreElement::New(elements, id, argv[i + 1]);
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
IonBuilder::inlineArrayPopShift(MArrayPopShift::Mode mode, uint32 argc, bool constructing)
{
    if (constructing)
        return InliningStatus_NotInlined;

    MIRType returnType = getInlineReturnType();
    if (returnType == MIRType_Undefined || returnType == MIRType_Null)
        return InliningStatus_NotInlined;
    if (getInlineArgType(argc, 0) != MIRType_Object)
        return InliningStatus_NotInlined;

    
    
    
    
    
    
    types::TypeObjectFlags unhandledFlags =
        types::OBJECT_FLAG_NON_DENSE_ARRAY | types::OBJECT_FLAG_ITERATED;

    types::TypeSet *thisTypes = getInlineArgTypeSet(argc, 0);
    if (thisTypes->hasObjectFlags(cx, unhandledFlags))
        return InliningStatus_NotInlined;
    if (types::ArrayPrototypeHasIndexedProperty(cx, script))
        return InliningStatus_NotInlined;

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    types::TypeSet *returnTypes = getInlineReturnTypeSet();
    bool needsHoleCheck = thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED_ARRAY);
    bool maybeUndefined = returnTypes->hasType(types::Type::UndefinedType());

    MArrayPopShift *ins = MArrayPopShift::New(argv[0], mode, needsHoleCheck, maybeUndefined);
    current->add(ins);
    current->push(ins);
    ins->setResultType(returnType);

    if (!resumeAfter(ins))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineArrayPush(uint32 argc, bool constructing)
{
    if (argc != 1 || constructing)
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;
    if (getInlineArgType(argc, 0) != MIRType_Object)
        return InliningStatus_NotInlined;

    
    
    types::TypeSet *thisTypes = getInlineArgTypeSet(argc, 0);
    if (thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY))
        return InliningStatus_NotInlined;
    if (types::ArrayPrototypeHasIndexedProperty(cx, script))
        return InliningStatus_NotInlined;

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    MArrayPush *ins = MArrayPush::New(argv[0], argv[1]);
    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathAbs(uint32 argc, bool constructing)
{
    if (constructing)
        return InliningStatus_NotInlined;

    
    if (argc == 0)
        return inlineNanResult(argc);

    MIRType returnType = getInlineReturnType();
    MIRType argType = getInlineArgType(argc, 1);
    if (argType != MIRType_Int32 && argType != MIRType_Double)
        return InliningStatus_NotInlined;
    if (argType != returnType)
        return InliningStatus_NotInlined;

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    MAbs *ins = MAbs::New(argv[1], returnType);
    current->add(ins);
    current->push(ins);

    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathFloor(uint32 argc, bool constructing)
{

    if (constructing)
        return InliningStatus_NotInlined;

    
    if (argc == 0)
        return inlineNanResult(argc);

    MIRType argType = getInlineArgType(argc, 1);
    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;

    
    if (argType == MIRType_Int32) {
        MDefinitionVector argv;
        if (!discardCall(argc, argv, current))
            return InliningStatus_Error;
        current->push(argv[1]);
        return InliningStatus_Inlined;
    }

    if (argType == MIRType_Double) {
        MDefinitionVector argv;
        if (!discardCall(argc, argv, current))
            return InliningStatus_Error;
        MFloor *ins = new MFloor(argv[1]);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathRound(uint32 argc, bool constructing)
{
    if (constructing)
        return InliningStatus_NotInlined;

    
    if (argc == 0)
        return inlineNanResult(argc);

    MIRType returnType = getInlineReturnType();
    MIRType argType = getInlineArgType(argc, 1);

    
    if (argType == MIRType_Int32 && returnType == MIRType_Int32) {
        MDefinitionVector argv;
        if (!discardCall(argc, argv, current))
            return InliningStatus_Error;
        current->push(argv[1]);
        return InliningStatus_Inlined;
    }

    if (argType == MIRType_Double && returnType == MIRType_Int32) {
        MDefinitionVector argv;
        if (!discardCall(argc, argv, current))
            return InliningStatus_Error;
        MRound *ins = new MRound(argv[1]);
        current->add(ins);
        current->push(ins);
        return InliningStatus_Inlined;
    }

    return InliningStatus_NotInlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathSqrt(uint32 argc, bool constructing)
{
    if (constructing)
        return InliningStatus_NotInlined;

    
    if (argc == 0)
        return inlineNanResult(argc);

    MIRType argType = getInlineArgType(argc, 1);
    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;
    if (argType != MIRType_Double && argType != MIRType_Int32)
        return InliningStatus_NotInlined;

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    MSqrt *sqrt = MSqrt::New(argv[1]);
    current->add(sqrt);
    current->push(sqrt);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineMathPow(uint32 argc, bool constructing)
{
    if (constructing)
        return InliningStatus_NotInlined;

    
    if (argc < 2)
        return inlineNanResult(argc);

    
    if (getInlineReturnType() != MIRType_Double)
        return InliningStatus_NotInlined;

    MIRType arg1Type = getInlineArgType(argc, 1);
    MIRType arg2Type = getInlineArgType(argc, 2);

    if (arg1Type != MIRType_Int32 && arg1Type != MIRType_Double)
        return InliningStatus_NotInlined;
    if (arg2Type != MIRType_Int32 && arg2Type != MIRType_Double)
        return InliningStatus_NotInlined;

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    
    
    if (arg1Type == MIRType_Int32) {
        MToDouble *conv = MToDouble::New(argv[1]);
        current->add(conv);
        argv[1] = conv;
    }

    
    if (argv[2]->isConstant()) {
        double pow;
        if (!ToNumber(GetIonContext()->cx, argv[2]->toConstant()->value(), &pow))
            return InliningStatus_Error;

        
        if (pow == 0.0) {
            MConstant *nan = MConstant::New(GetIonContext()->cx->runtime->NaNValue);
            current->add(nan);
            current->push(nan);
            return InliningStatus_Inlined;
        }

        
        if (pow == 0.5) {
            MPowHalf *half = MPowHalf::New(argv[1]);
            current->add(half);
            current->push(half);
            return InliningStatus_Inlined;
        }

        
        if (pow == -0.5) {
            MPowHalf *half = MPowHalf::New(argv[1]);
            current->add(half);
            MConstant *one = MConstant::New(DoubleValue(1.0));
            current->add(one);
            MDiv *div = MDiv::New(one, half, MIRType_Double);
            current->add(div);
            current->push(div);
            return InliningStatus_Inlined;
        }

        
        if (pow == 1.0) {
            current->push(argv[1]);
            return InliningStatus_Inlined;
        }

        
        if (pow == 2.0) {
            MMul *mul = MMul::New(argv[1], argv[1], MIRType_Double);
            current->add(mul);
            current->push(mul);
            return InliningStatus_Inlined;
        }

        
        if (pow == 3.0) {
            MMul *mul1 = MMul::New(argv[1], argv[1], MIRType_Double);
            current->add(mul1);
            MMul *mul2 = MMul::New(argv[1], mul1, MIRType_Double);
            current->add(mul2);
            current->push(mul2);
            return InliningStatus_Inlined;
        }

        
        if (pow == 4.0) {
            MMul *y = MMul::New(argv[1], argv[1], MIRType_Double);
            current->add(y);
            MMul *mul = MMul::New(y, y, MIRType_Double);
            current->add(mul);
            current->push(mul);
            return InliningStatus_Inlined;
        }
    }

    MPow *ins = MPow::New(argv[1], argv[2], arg2Type);
    current->add(ins);
    current->push(ins);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrCharCodeAt(uint32 argc, bool constructing)
{
    if (argc != 1 || constructing)
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_Int32)
        return InliningStatus_NotInlined;
    if (getInlineArgType(argc, 0) != MIRType_String)
        return InliningStatus_NotInlined;
    if (getInlineArgType(argc, 1) != MIRType_Int32)
        return InliningStatus_NotInlined;

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    MInstruction *index = MToInt32::New(argv[1]);
    current->add(index);

    MStringLength *length = MStringLength::New(argv[0]);
    current->add(length);

    index = addBoundsCheck(index, length);

    MCharCodeAt *charCode = MCharCodeAt::New(argv[0], index);
    current->add(charCode);
    current->push(charCode);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrFromCharCode(uint32 argc, bool constructing)
{
    if (argc != 1 || constructing)
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;
    if (getInlineArgType(argc, 1) != MIRType_Int32)
        return InliningStatus_NotInlined;

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    MToInt32 *charCode = MToInt32::New(argv[1]);
    current->add(charCode);

    MFromCharCode *string = MFromCharCode::New(charCode);
    current->add(string);
    current->push(string);
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineStrCharAt(uint32 argc, bool constructing)
{
    if (argc != 1 || constructing)
        return InliningStatus_NotInlined;

    if (getInlineReturnType() != MIRType_String)
        return InliningStatus_NotInlined;
    if (getInlineArgType(argc, 0) != MIRType_String)
        return InliningStatus_NotInlined;
    if (getInlineArgType(argc, 1) != MIRType_Int32)
        return InliningStatus_NotInlined;

    MDefinitionVector argv;
    if (!discardCall(argc, argv, current))
        return InliningStatus_Error;

    MInstruction *index = MToInt32::New(argv[1]);
    current->add(index);

    MStringLength *length = MStringLength::New(argv[0]);
    current->add(length);

    index = addBoundsCheck(index, length);

    
    MCharCodeAt *charCode = MCharCodeAt::New(argv[0], index);
    current->add(charCode);

    MFromCharCode *string = MFromCharCode::New(charCode);
    current->add(string);
    current->push(string);
    return InliningStatus_Inlined;
}

} 
} 
