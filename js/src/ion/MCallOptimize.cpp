






































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
        
        return false;
    }

    return false;
}

} 
} 
