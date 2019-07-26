





#include "jit/IonOptions.h"

using namespace js;
using namespace js::jit;

namespace js {
namespace jit {

IonOptions js_IonOptions;

IonOptions::IonOptions()
{
    
    
    
    checkGraphConsistency = true;

#ifdef CHECK_OSIPOINT_REGISTERS
    
    
    checkOsiPointRegisters = false;
#endif

    
    
    checkRangeAnalysis = false;

    
    
    checkThreadSafety = false;

    
    compileTryCatch = true;

    
    disableGvn = false;

    
    disableLicm = false;

    
    disableInlining = false;

    
    disableEdgeCaseAnalysis = false;

    
    disableRangeAnalysis = false;

    
    disableUce = false;

    
    disableEaa = false;

    
    eagerCompilation = false;

    
    
    
    forceDefaultIonUsesBeforeCompile = false;
    forcedDefaultIonUsesBeforeCompile = 1000;

    
    
    forceGvnKind = false;
    forcedGvnKind = GVN_Optimistic;

    
    
    forceRegisterAllocator = false;
    forcedRegisterAllocator = RegisterAllocator_LSRA;

    
    limitScriptSize = true;

    
    osr = true;

    
    
    baselineUsesBeforeCompile = 10;

    
    
    exceptionBailoutThreshold = 10;

    
    
    frequentBailoutThreshold = 10;

    
    maxStackArgs = 4096;

    
    
    osrPcMismatchesBeforeRecompile = 6000;

    
    
    
    
    
    smallFunctionMaxBytecodeLength_ = 100;

    
    usesBeforeCompilePar = 1;
}

bool
IonOptions::isSmallFunction(JSScript *script) const
{
    return script->length() <= smallFunctionMaxBytecodeLength_;
}

void
IonOptions::setEagerCompilation()
{
    eagerCompilation = true;
    baselineUsesBeforeCompile = 0;
    forceDefaultIonUsesBeforeCompile = true;
    forcedDefaultIonUsesBeforeCompile = 0;
}

void
IonOptions::setUsesBeforeCompile(uint32_t useCount)
{
    forceDefaultIonUsesBeforeCompile = true;
    forcedDefaultIonUsesBeforeCompile = useCount;

    
    if (eagerCompilation && useCount != 0) {
        jit::IonOptions defaultValues;
        eagerCompilation = false;
        baselineUsesBeforeCompile = defaultValues.baselineUsesBeforeCompile;
    }
}

void
IonOptions::resetUsesBeforeCompile()
{
    forceDefaultIonUsesBeforeCompile = false;

    
    if (eagerCompilation) {
        jit::IonOptions defaultValues;
        eagerCompilation = false;
        baselineUsesBeforeCompile = defaultValues.baselineUsesBeforeCompile;
    }
}

} 
} 
