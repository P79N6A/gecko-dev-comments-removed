





#ifndef jit_JitOptions_h
#define jit_JitOptions_h

#include "mozilla/Maybe.h"

#include "jit/IonTypes.h"
#include "js/TypeDecls.h"

namespace js {
namespace jit {



static const uint32_t MAX_MAIN_THREAD_SCRIPT_SIZE = 2 * 1000;
static const uint32_t MAX_MAIN_THREAD_LOCALS_AND_ARGS = 256;


enum IonRegisterAllocator {
    RegisterAllocator_Backtracking,
    RegisterAllocator_Stupid
};

static inline mozilla::Maybe<IonRegisterAllocator>
LookupRegisterAllocator(const char* name)
{
    if (!strcmp(name, "backtracking"))
        return mozilla::Some(RegisterAllocator_Backtracking);
    if (!strcmp(name, "stupid"))
        return mozilla::Some(RegisterAllocator_Stupid);
    return mozilla::Nothing();
}

struct JitOptions
{
    bool checkGraphConsistency;
#ifdef CHECK_OSIPOINT_REGISTERS
    bool checkOsiPointRegisters;
#endif
    bool checkRangeAnalysis;
    bool runExtraChecks;
    bool disableScalarReplacement;
    bool disableEagerSimdUnbox;
    bool disableGvn;
    bool disableLicm;
    bool disableInlining;
    bool disableEdgeCaseAnalysis;
    bool disableRangeAnalysis;
    bool disableSink;
    bool disableLoopUnrolling;
    bool disableEaa;
    bool disableAma;
    bool eagerCompilation;
    mozilla::Maybe<uint32_t> forcedDefaultIonWarmUpThreshold;
    mozilla::Maybe<IonRegisterAllocator> forcedRegisterAllocator;
    bool limitScriptSize;
    bool osr;
    uint32_t baselineWarmUpThreshold;
    uint32_t exceptionBailoutThreshold;
    uint32_t frequentBailoutThreshold;
    uint32_t maxStackArgs;
    uint32_t osrPcMismatchesBeforeRecompile;
    uint32_t smallFunctionMaxBytecodeLength_;

    JitOptions();
    bool isSmallFunction(JSScript* script) const;
    void setEagerCompilation();
    void setCompilerWarmUpThreshold(uint32_t warmUpThreshold);
    void resetCompilerWarmUpThreshold();
    void enableGvn(bool val);
};

extern JitOptions js_JitOptions;

} 
} 

#endif 
