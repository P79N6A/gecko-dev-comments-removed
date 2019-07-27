





#ifndef jit_JitOptions_h
#define jit_JitOptions_h

#include "jit/IonTypes.h"
#include "js/TypeDecls.h"

namespace js {
namespace jit {



static const uint32_t MAX_MAIN_THREAD_SCRIPT_SIZE = 2 * 1000;
static const uint32_t MAX_MAIN_THREAD_LOCALS_AND_ARGS = 256;


enum IonRegisterAllocator {
    RegisterAllocator_LSRA,
    RegisterAllocator_Backtracking,
    RegisterAllocator_Stupid
};

struct JitOptions
{
    bool checkGraphConsistency;
#ifdef CHECK_OSIPOINT_REGISTERS
    bool checkOsiPointRegisters;
#endif
    bool checkRangeAnalysis;
    bool compileTryCatch;
    bool disableScalarReplacement;
    bool disableGvn;
    bool disableLicm;
    bool disableInlining;
    bool disableEdgeCaseAnalysis;
    bool disableRangeAnalysis;
    bool disableLoopUnrolling;
    bool disableEaa;
    bool eagerCompilation;
    bool forceDefaultIonWarmUpThreshold;
    uint32_t forcedDefaultIonWarmUpThreshold;
    bool forceRegisterAllocator;
    IonRegisterAllocator forcedRegisterAllocator;
    bool limitScriptSize;
    bool osr;
    uint32_t baselineWarmUpThreshold;
    uint32_t exceptionBailoutThreshold;
    uint32_t frequentBailoutThreshold;
    uint32_t maxStackArgs;
    uint32_t osrPcMismatchesBeforeRecompile;
    uint32_t smallFunctionMaxBytecodeLength_;
    uint32_t compilerWarmUpThresholdPar;

    JitOptions();
    bool isSmallFunction(JSScript *script) const;
    void setEagerCompilation();
    void setCompilerWarmUpThreshold(uint32_t warmUpThreshold);
    void resetCompilerWarmUpThreshold();
};

extern JitOptions js_JitOptions;

} 
} 

#endif 
