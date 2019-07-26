






































#include "jslibmath.h"
#include "jsmath.h"

#include "MIR.h"
#include "MIRGraph.h"
#include "IonBuilder.h"

namespace js {
namespace ion {

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

bool
IonBuilder::inlineNativeCall(JSFunction *target, uint32 argc, bool constructing)
{
    JSNative native = target->native();

    
    if (constructing && native != js_Array)
        return false;

    

    types::TypeSet *barrier;
    types::TypeSet *returnTypes = oracle->returnTypeSet(script, pc, &barrier);
    MIRType returnType = MIRTypeFromValueType(returnTypes->getKnownTypeTag(cx));

    types::TypeSet *thisTypes = oracle->getCallArg(script, argc, 0, pc);
    MIRType thisType = MIRTypeFromValueType(thisTypes->getKnownTypeTag(cx));
    MDefinitionVector argv;

    if (argc == 0) {
        if (native == js_Array) {
            if (!discardCall(argc, argv, current))
                return false;
            types::TypeObject *type = types::TypeScript::InitObject(cx, script, pc, JSProto_Array);
            MNewArray *ins = new MNewArray(0, type, MNewArray::NewArray_Unallocating);
            current->add(ins);
            current->push(ins);
            if (!resumeAfter(ins))
                return false;
            return true;
        }

        if ((native == js::array_pop || native == js::array_shift) && thisType == MIRType_Object) {
            
            
            
            
            
            
            if (!thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY |
                                           types::OBJECT_FLAG_ITERATED) &&
                !cx->compartment->needsBarrier() &&
                !types::ArrayPrototypeHasIndexedProperty(cx, script))
            {
                bool needsHoleCheck = thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED_ARRAY);
                bool maybeUndefined = returnTypes->hasType(types::Type::UndefinedType());
                MArrayPopShift::Mode mode = (native == js::array_pop) ? MArrayPopShift::Pop : MArrayPopShift::Shift;
                JSValueType knownType = returnTypes->getKnownTypeTag(cx);
                if (knownType == JSVAL_TYPE_UNDEFINED || knownType == JSVAL_TYPE_NULL)
                    return false;
                MIRType resultType = MIRTypeFromValueType(knownType);
                if (!discardCall(argc, argv, current))
                    return false;
                MArrayPopShift *ins = MArrayPopShift::New(argv[0], mode, needsHoleCheck, maybeUndefined);
                current->add(ins);
                current->push(ins);
                ins->setResultType(resultType);
                return resumeAfter(ins);
            }
        }

        return false;
    }

    types::TypeSet *arg1Types = oracle->getCallArg(script, argc, 1, pc);
    MIRType arg1Type = MIRTypeFromValueType(arg1Types->getKnownTypeTag(cx));

    if (argc == 1) {
        if (native == js_math_abs) {
            
            if ((arg1Type == MIRType_Double || arg1Type == MIRType_Int32) &&
                arg1Type == returnType) {
                if (!discardCall(argc, argv, current))
                    return false;
                MAbs *ins = MAbs::New(argv[1], returnType);
                current->add(ins);
                current->push(ins);
                return true;
            }
        }
        if (native == js_math_sqrt) {
            
            if ((arg1Type == MIRType_Double || arg1Type == MIRType_Int32) &&
                returnType == MIRType_Double) {
                if (!discardCall(argc, argv, current))
                    return false;
                MSqrt *ins = MSqrt::New(argv[1]);
                current->add(ins);
                current->push(ins);
                return true;
            }
        }
        if (native == js_math_floor) {
            
            if (arg1Type == MIRType_Double && returnType == MIRType_Int32) {
                if (!discardCall(argc, argv, current))
                    return false;
                MFloor *ins = new MFloor(argv[1]);
                current->add(ins);
                current->push(ins);
                return true;
            }
            if (arg1Type == MIRType_Int32 && returnType == MIRType_Int32) {
                
                if (!discardCall(argc, argv, current))
                    return false;
                current->push(argv[1]);
                return true;
            }
        }
        if (native == js_math_round) {
            
            if (arg1Type == MIRType_Double && returnType == MIRType_Int32) {
                if (!discardCall(argc, argv, current))
                    return false;
                MRound *ins = new MRound(argv[1]);
                current->add(ins);
                current->push(ins);
                return true;
            }
            if (arg1Type == MIRType_Int32 && returnType == MIRType_Int32) {
                
                if (!discardCall(argc, argv, current))
                    return false;
                current->push(argv[1]);
                return true;
            }
        }
        

        
        if (native == js_str_charCodeAt || native == js_str_charAt ||
            native == js::str_fromCharCode) {

            if (native == js_str_charCodeAt) {
                if (returnType != MIRType_Int32 || thisType != MIRType_String ||
                    arg1Type != MIRType_Int32) {
                    return false;
                }
            }

            if (native == js_str_charAt) {
                if (returnType != MIRType_String || thisType != MIRType_String ||
                    arg1Type != MIRType_Int32) {
                    return false;
                }
            }

            if (native == js::str_fromCharCode) {
                
                if (returnType != MIRType_String || arg1Type != MIRType_Int32)
                    return false;
            }

            if (!discardCall(argc, argv, current))
                return false;
            MDefinition *str = argv[0];
            MDefinition *indexOrCode = argv[1];
            MInstruction *ins = MToInt32::New(indexOrCode);
            current->add(ins);
            indexOrCode = ins;

            if (native == js_str_charCodeAt || native == js_str_charAt) {
                MStringLength *length = MStringLength::New(str);
                MBoundsCheck *boundsCheck = MBoundsCheck::New(indexOrCode, length);
                current->add(length);
                current->add(boundsCheck);
                ins = new MCharCodeAt(str, indexOrCode);
                current->add(ins);
                indexOrCode = ins;
            }
            if (native == js::str_fromCharCode || native == js_str_charAt) {
                ins = new MFromCharCode(indexOrCode);
                current->add(ins);
            }
            current->push(ins);
            return true;
        }
        if (native == js_Array) {
            if (arg1Type == MIRType_Int32) {
                MDefinition *argv1 = current->peek(-1)->toPassArg()->getArgument();
                if (argv1->isConstant()) {
                    int32 arg = argv1->toConstant()->value().toInt32();
                    if (!discardCall(argc, argv, current))
                        return false;
                    types::TypeObject *type = types::TypeScript::InitObject(cx, script, pc, JSProto_Array);
                    MNewArray *ins = new MNewArray(arg, type, MNewArray::NewArray_Unallocating);
                    current->add(ins);
                    current->push(ins);
                    if (!resumeAfter(ins))
                        return false;
                    return true;
                }
            }
        }
        if (native == js::array_push) {
            
            
            if (thisType == MIRType_Object && returnType == MIRType_Int32 &&
                !thisTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY) &&
                !types::ArrayPrototypeHasIndexedProperty(cx, script))
            {
                if (!discardCall(argc, argv, current))
                    return false;
                MArrayPush *ins = MArrayPush::New(argv[0], argv[1]);
                current->add(ins);
                current->push(ins);
                return resumeAfter(ins);
            }
        }

        return false;
    }

    return false;
}

} 
} 
