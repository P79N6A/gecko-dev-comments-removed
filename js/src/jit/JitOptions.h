





#ifndef jit_IonOptions_h
#define jit_IonOptions_h

#include "jit/IonTypes.h"

#ifdef JS_ION

namespace js {
namespace jit {


enum IonRegisterAllocator {
    RegisterAllocator_LSRA,
    RegisterAllocator_Backtracking,
    RegisterAllocator_Stupid
};

enum IonGvnKind {
    GVN_Optimistic,
    GVN_Pessimistic
};

struct IonOptions
{
    bool checkGraphConsistency;
#ifdef CHECK_OSIPOINT_REGISTERS
    bool checkOsiPointRegisters;
#endif
    bool checkRangeAnalysis;
    bool checkThreadSafety;
    bool compileTryCatch;
    bool disableGvn;
    bool disableLicm;
    bool disableInlining;
    bool disableEdgeCaseAnalysis;
    bool disableRangeAnalysis;
    bool disableUce;
    bool disableEaa;
    bool eagerCompilation;
    bool forceDefaultIonUsesBeforeCompile;
    uint32_t forcedDefaultIonUsesBeforeCompile;
    bool forceGvnKind;
    IonGvnKind forcedGvnKind;
    bool forceRegisterAllocator;
    IonRegisterAllocator forcedRegisterAllocator;
    bool limitScriptSize;
    bool osr;
    uint32_t baselineUsesBeforeCompile;
    uint32_t exceptionBailoutThreshold;
    uint32_t frequentBailoutThreshold;
    uint32_t maxStackArgs;
    uint32_t osrPcMismatchesBeforeRecompile;
    uint32_t smallFunctionMaxBytecodeLength_;
    uint32_t usesBeforeCompilePar;

    IonOptions();
    bool isSmallFunction(JSScript *script) const;
    void setEagerCompilation();
    void setUsesBeforeCompile(uint32_t useCount);
    void resetUsesBeforeCompile();
};

extern IonOptions js_IonOptions;

} 
} 

#endif 

#endif 
