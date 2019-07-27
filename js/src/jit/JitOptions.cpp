





#include "jit/JitOptions.h"
#include "mozilla/TypeTraits.h"

#include <cstdlib>
#include "jsfun.h"
using namespace js;
using namespace js::jit;

using mozilla::Maybe;

namespace js {
namespace jit {

JitOptions js_JitOptions;

static void Warn(const char *env, const char *value)
{
    fprintf(stderr, "Warning: I didn't understand %s=\"%s\"\n", env, value);
}

template<typename T> struct IsBool : mozilla::FalseType {};
template<> struct IsBool<bool> : mozilla::TrueType {};

static Maybe<int>
ParseInt(const char *str)
{
    char *endp;
    int retval = strtol(str, &endp, 0);
    if (*endp == '\0')
        return mozilla::Some(retval);
    return mozilla::Nothing();
}

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
        Warn(param, str);
    } else {
        Maybe<int> value = ParseInt(str);
        if (value.isSome())
            return value.ref();
        Warn(param, str);
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

    
    SET_DEFAULT(disableScalarReplacement, false);

    
    SET_DEFAULT(disableEagerSimdUnbox, false);

    
    SET_DEFAULT(disableGvn, false);

    
    SET_DEFAULT(disableLicm, false);

    
    SET_DEFAULT(disableInlining, false);

    
    SET_DEFAULT(disableEdgeCaseAnalysis, false);

    
    SET_DEFAULT(disableRangeAnalysis, false);

    
    SET_DEFAULT(disableSink, true);

    
    SET_DEFAULT(disableLoopUnrolling, true);

    
    SET_DEFAULT(disableEaa, false);

    
    SET_DEFAULT(disableAma, false);

    
    SET_DEFAULT(eagerCompilation, false);

    
    
    
    const char *forcedDefaultIonWarmUpThresholdEnv = "JIT_OPTION_forcedDefaultIonWarmUpThreshold";
    if (const char *env = getenv(forcedDefaultIonWarmUpThresholdEnv)) {
        Maybe<int> value = ParseInt(env);
        if (value.isSome())
            forcedDefaultIonWarmUpThreshold.emplace(value.ref());
        else
            Warn(forcedDefaultIonWarmUpThresholdEnv, env);
    }

    
    
    const char *forcedRegisterAllocatorEnv = "JIT_OPTION_forcedRegisterAllocator";
    if (const char *env = getenv(forcedRegisterAllocatorEnv)) {
        forcedRegisterAllocator = LookupRegisterAllocator(env);
        if (!forcedRegisterAllocator.isSome())
            Warn(forcedRegisterAllocatorEnv, env);
    }

    
    SET_DEFAULT(limitScriptSize, true);

    
    SET_DEFAULT(osr, true);

    
    
    SET_DEFAULT(baselineWarmUpThreshold, 10);

    
    
    SET_DEFAULT(exceptionBailoutThreshold, 10);

    
    
    SET_DEFAULT(frequentBailoutThreshold, 10);

    
    SET_DEFAULT(maxStackArgs, 4096);

    
    
    SET_DEFAULT(osrPcMismatchesBeforeRecompile, 6000);

    
    
    
    
    
    SET_DEFAULT(smallFunctionMaxBytecodeLength_, 100);
}

bool
JitOptions::isSmallFunction(JSScript *script) const
{
    return script->length() <= smallFunctionMaxBytecodeLength_;
}

void
JitOptions::enableGvn(bool enable)
{
    disableGvn = !enable;
}

void
JitOptions::setEagerCompilation()
{
    eagerCompilation = true;
    baselineWarmUpThreshold = 0;
    forcedDefaultIonWarmUpThreshold.reset();
    forcedDefaultIonWarmUpThreshold.emplace(0);
}

void
JitOptions::setCompilerWarmUpThreshold(uint32_t warmUpThreshold)
{
    forcedDefaultIonWarmUpThreshold.reset();
    forcedDefaultIonWarmUpThreshold.emplace(warmUpThreshold);

    
    if (eagerCompilation && warmUpThreshold != 0) {
        jit::JitOptions defaultValues;
        eagerCompilation = false;
        baselineWarmUpThreshold = defaultValues.baselineWarmUpThreshold;
    }
}

void
JitOptions::resetCompilerWarmUpThreshold()
{
    forcedDefaultIonWarmUpThreshold.reset();

    
    if (eagerCompilation) {
        jit::JitOptions defaultValues;
        eagerCompilation = false;
        baselineWarmUpThreshold = defaultValues.baselineWarmUpThreshold;
    }
}

} 
} 
