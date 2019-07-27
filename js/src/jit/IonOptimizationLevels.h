





#ifndef jit_IonOptimizationLevels_h
#define jit_IonOptimizationLevels_h

#include "jsbytecode.h"
#include "jstypes.h"

#include "jit/JitOptions.h"
#include "js/TypeDecls.h"

namespace js {
namespace jit {

enum OptimizationLevel
{
    Optimization_DontCompile,
    Optimization_Normal,
    Optimization_AsmJS,
    Optimization_Count
};

#ifdef DEBUG
inline const char *
OptimizationLevelString(OptimizationLevel level)
{
    switch (level) {
      case Optimization_DontCompile:
        return "Optimization_DontCompile";
      case Optimization_Normal:
        return "Optimization_Normal";
      case Optimization_AsmJS:
        return "Optimization_AsmJS";
      default:
        MOZ_CRASH("Invalid OptimizationLevel");
    }
}
#endif

class OptimizationInfo
{
  public:
    OptimizationLevel level_;

    
    bool eaa_;

    
    bool edgeCaseAnalysis_;

    
    bool eliminateRedundantChecks_;

    
    bool inlineInterpreted_;

    
    bool inlineNative_;

    
    bool gvn_;

    
    bool licm_;

    
    bool rangeAnalysis_;

    
    bool loopUnrolling_;

    
    bool autoTruncate_;

    
    IonRegisterAllocator registerAllocator_;

    
    uint32_t inlineMaxTotalBytecodeLength_;

    
    
    uint32_t inliningMaxCallerBytecodeLength_;

    
    uint32_t maxInlineDepth_;

    
    bool scalarReplacement_;

    
    
    
    
    
    
    uint32_t smallFunctionMaxInlineDepth_;

    
    
    uint32_t compilerWarmUpThreshold_;

    
    
    double inliningWarmUpThresholdFactor_;

    OptimizationInfo()
    { }

    void initNormalOptimizationInfo();
    void initAsmjsOptimizationInfo();

    OptimizationLevel level() const {
        return level_;
    }

    bool inlineInterpreted() const {
        return inlineInterpreted_ && !js_JitOptions.disableInlining;
    }

    bool inlineNative() const {
        return inlineNative_ && !js_JitOptions.disableInlining;
    }

    uint32_t compilerWarmUpThreshold(JSScript *script, jsbytecode *pc = nullptr) const;

    bool gvnEnabled() const {
        return gvn_ && !js_JitOptions.disableGvn;
    }

    bool licmEnabled() const {
        return licm_ && !js_JitOptions.disableLicm;
    }

    bool rangeAnalysisEnabled() const {
        return rangeAnalysis_ && !js_JitOptions.disableRangeAnalysis;
    }

    bool loopUnrollingEnabled() const {
        return loopUnrolling_ && !js_JitOptions.disableLoopUnrolling;
    }

    bool autoTruncateEnabled() const {
        return autoTruncate_ && rangeAnalysisEnabled();
    }

    bool eaaEnabled() const {
        return eaa_ && !js_JitOptions.disableEaa;
    }

    bool edgeCaseAnalysisEnabled() const {
        return edgeCaseAnalysis_ && !js_JitOptions.disableEdgeCaseAnalysis;
    }

    bool eliminateRedundantChecksEnabled() const {
        return eliminateRedundantChecks_;
    }

    IonRegisterAllocator registerAllocator() const {
        if (!js_JitOptions.forceRegisterAllocator)
            return registerAllocator_;
        return js_JitOptions.forcedRegisterAllocator;
    }

    bool scalarReplacementEnabled() const {
        return scalarReplacement_ && !js_JitOptions.disableScalarReplacement;
    }

    uint32_t smallFunctionMaxInlineDepth() const {
        return smallFunctionMaxInlineDepth_;
    }

    bool isSmallFunction(JSScript *script) const;

    uint32_t maxInlineDepth() const {
        return maxInlineDepth_;
    }

    uint32_t inlineMaxTotalBytecodeLength() const {
        return inlineMaxTotalBytecodeLength_;
    }

    uint32_t inliningMaxCallerBytecodeLength() const {
        return inlineMaxTotalBytecodeLength_;
    }

    uint32_t inliningWarmUpThreshold() const {
        uint32_t compilerWarmUpThreshold = compilerWarmUpThreshold_;
        if (js_JitOptions.forceDefaultIonWarmUpThreshold)
            compilerWarmUpThreshold = js_JitOptions.forcedDefaultIonWarmUpThreshold;
        return compilerWarmUpThreshold * inliningWarmUpThresholdFactor_;
    }
};

class OptimizationInfos
{
  private:
    OptimizationInfo infos_[Optimization_Count - 1];

  public:
    OptimizationInfos();

    const OptimizationInfo *get(OptimizationLevel level) const {
        JS_ASSERT(level < Optimization_Count);
        JS_ASSERT(level != Optimization_DontCompile);

        return &infos_[level - 1];
    }

    OptimizationLevel nextLevel(OptimizationLevel level) const;
    OptimizationLevel firstLevel() const;
    bool isLastLevel(OptimizationLevel level) const;
    OptimizationLevel levelForScript(JSScript *script, jsbytecode *pc = nullptr) const;
};

extern OptimizationInfos js_IonOptimizations;

} 
} 

#endif 
