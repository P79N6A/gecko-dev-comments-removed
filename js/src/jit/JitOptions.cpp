





#include "jit/JitOptions.h"

#include "jsfun.h"

using namespace js;
using namespace js::jit;

namespace js {
namespace jit {

JitOptions js_JitOptions;

JitOptions::JitOptions()
{
    
    
    
    checkGraphConsistency = true;

#ifdef CHECK_OSIPOINT_REGISTERS
    
    
    checkOsiPointRegisters = false;
#endif

    
    
    checkRangeAnalysis = false;

    
    compileTryCatch = true;

    
    disableScalarReplacement = true; 

    
    disableGvn = false;

    
    disableLicm = false;

    
    disableInlining = false;

    
    disableEdgeCaseAnalysis = false;

    
    disableRangeAnalysis = false;

    
    disableLoopUnrolling = true;

    
    disableUce = false;

    
    disableEaa = false;

    
    eagerCompilation = false;

    
    
    
    forceDefaultIonWarmUpThreshold = false;
    forcedDefaultIonWarmUpThreshold = 1000;

    
    
    forceRegisterAllocator = false;
    forcedRegisterAllocator = RegisterAllocator_LSRA;

    
    limitScriptSize = true;

    
    osr = true;

    
    
    baselineWarmUpThreshold = 10;

    
    
    exceptionBailoutThreshold = 10;

    
    
    frequentBailoutThreshold = 10;

    
    maxStackArgs = 4096;

    
    
    osrPcMismatchesBeforeRecompile = 6000;

    
    
    
    
    
    smallFunctionMaxBytecodeLength_ = 100;

    
    compilerWarmUpThresholdPar = 1;
}

bool
JitOptions::isSmallFunction(JSScript *script) const
{
    return script->length() <= smallFunctionMaxBytecodeLength_;
}

void
JitOptions::setEagerCompilation()
{
    eagerCompilation = true;
    baselineWarmUpThreshold = 0;
    forceDefaultIonWarmUpThreshold = true;
    forcedDefaultIonWarmUpThreshold = 0;
}

void
JitOptions::setCompilerWarmUpThreshold(uint32_t warmUpCounter)
{
    forceDefaultIonWarmUpThreshold = true;
    forcedDefaultIonWarmUpThreshold = warmUpCounter;

    
    if (eagerCompilation && warmUpCounter != 0) {
        jit::JitOptions defaultValues;
        eagerCompilation = false;
        baselineWarmUpThreshold = defaultValues.baselineWarmUpThreshold;
    }
}

void
JitOptions::resetCompilerWarmUpThreshold()
{
    forceDefaultIonWarmUpThreshold = false;

    
    if (eagerCompilation) {
        jit::JitOptions defaultValues;
        eagerCompilation = false;
        baselineWarmUpThreshold = defaultValues.baselineWarmUpThreshold;
    }
}

} 
} 
