






































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
        block->remove(passArg);
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
IonBuilder::optimizeNativeCall(uint32 argc)
{
    
    types::TypeSet *calleeTypes = oracle->getCallTarget(script, argc, pc);
    if (!calleeTypes)
        return false;

    JSObject *funObject = calleeTypes->getSingleton(cx);
    if (!funObject)
        return false;

    if (!funObject->isFunction())
        return false;
    JSFunction *fun = funObject->toFunction();

    JSNative native = fun->maybeNative();
    if (!native)
        return false;

    

    types::TypeSet *returnTypes = oracle->getCallReturn(script, pc);
    MIRType returnType = MIRTypeFromValueType(returnTypes->getKnownTypeTag(cx));

    types::TypeSet *thisTypes = oracle->getCallArg(script, argc, 0, pc);
    MIRType thisType = MIRTypeFromValueType(thisTypes->getKnownTypeTag(cx));
    MDefinitionVector argv;

    if (argc == 0)
        return false;

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
        if (native == js_math_floor) {
            
            if (arg1Type == MIRType_Double && returnType == MIRType_Int32) {
                if (!discardCall(argc, argv, current))
                    return false;
                MRound *ins = new MRound(argv[1], MRound::RoundingMode_Floor);
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
                MRound *ins = new MRound(argv[1], MRound::RoundingMode_Round);
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
        return false;
    }

    return false;
}

} 
} 
