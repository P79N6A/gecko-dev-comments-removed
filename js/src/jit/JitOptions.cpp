





#include "jit/JitOptions.h"
#include "mozilla/TypeTraits.h"

#include <cstdlib>
#include "jsfun.h"
using namespace js;
using namespace js::jit;

namespace js {
namespace jit {

JitOptions js_JitOptions;

template<typename T> struct IsBool : mozilla::FalseType {};
template<> struct IsBool<bool> : mozilla::TrueType {};

template<typename T>
T overrideDefault(const char *param, T dflt) {
    char *str = getenv(param);
    if (!str)
        return dflt;
    if (IsBool<T>::value) {
        if (strcmp(str, "true") == 0 ||
            strcmp(str, "yes")) {
            return true;
        }
        if (strcmp(str, "false") == 0 ||
            strcmp(str, "no")) {
            return false;
        }
        fprintf(stderr, "Warning: I didn't understand %s=\"%s\"", param, str);
    } else {
        char *endp;
        int retval = strtol(str, &endp, 0);
        if (*endp == '\0')
            return retval;

        fprintf(stderr, "Warning: I didn't understand %s=\"%s\"", param, str);
    }
    return dflt;
}
#define SET_DEFAULT(var, dflt) var = overrideDefault("JIT_OPTION_" #var, dflt)
JitOptions::JitOptions()
{
    
    
    
    SET_DEFAULT(checkGraphConsistency, true);

#ifdef CHECK_OSIPOINT_REGISTERS
    
    
    SET_DEFAULT(checkOsiPointRegisters, false);
#endif

    
    
    SET_DEFAULT(checkRangeAnalysis, false);

    
    SET_DEFAULT(compileTryCatch, true);

    
    SET_DEFAULT(disableScalarReplacement, true); 

    
    SET_DEFAULT(disableGvn, false);

    
    SET_DEFAULT(disableLicm, false);

    
    SET_DEFAULT(disableInlining, false);

    
    SET_DEFAULT(disableEdgeCaseAnalysis, false);

    
    SET_DEFAULT(disableRangeAnalysis, false);

    
    SET_DEFAULT(disableLoopUnrolling, true);

    
    SET_DEFAULT(disableEaa, false);

    
    SET_DEFAULT(eagerCompilation, false);

    
    
    
    SET_DEFAULT(forceDefaultIonWarmUpThreshold, false);
    SET_DEFAULT(forcedDefaultIonWarmUpThreshold, 1000);

    
    
    forceRegisterAllocator = false;
    forcedRegisterAllocator = RegisterAllocator_LSRA;

    
    SET_DEFAULT(limitScriptSize, true);

    
    SET_DEFAULT(osr, true);

    
    
    SET_DEFAULT(baselineWarmUpThreshold, 10);

    
    
    SET_DEFAULT(exceptionBailoutThreshold, 10);

    
    
    SET_DEFAULT(frequentBailoutThreshold, 10);

    
    SET_DEFAULT(maxStackArgs, 4096);

    
    
    SET_DEFAULT(osrPcMismatchesBeforeRecompile, 6000);

    
    
    
    
    
    SET_DEFAULT(smallFunctionMaxBytecodeLength_, 100);

    
    SET_DEFAULT(compilerWarmUpThresholdPar, 1);
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
JitOptions::setCompilerWarmUpThreshold(uint32_t warmUpThreshold)
{
    forceDefaultIonWarmUpThreshold = true;
    forcedDefaultIonWarmUpThreshold = warmUpThreshold;

    
    if (eagerCompilation && warmUpThreshold != 0) {
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
