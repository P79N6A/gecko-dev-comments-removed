





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

    
    IonGvnKind gvnKind_;

    
    bool licm_;

    
    bool uce_;

    
    bool rangeAnalysis_;

    
    IonRegisterAllocator registerAllocator_;

    
    uint32_t inlineMaxTotalBytecodeLength_;

    
    
    uint32_t inliningMaxCallerBytecodeLength_;

    
    uint32_t maxInlineDepth_;

    
    
    
    
    
    
    uint32_t smallFunctionMaxInlineDepth_;

    
    
    uint32_t usesBeforeCompile_;

    
    
    double usesBeforeInliningFactor_;

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

    uint32_t usesBeforeCompile() const {
        if (js_JitOptions.forceDefaultIonUsesBeforeCompile)
            return js_JitOptions.forcedDefaultIonUsesBeforeCompile;
        return usesBeforeCompile_;
    }

    bool gvnEnabled() const {
        return gvn_ && !js_JitOptions.disableGvn;
    }

    bool licmEnabled() const {
        return licm_ && !js_JitOptions.disableLicm;
    }

    bool uceEnabled() const {
        return uce_ && !js_JitOptions.disableUce;
    }

    bool rangeAnalysisEnabled() const {
        return rangeAnalysis_ && !js_JitOptions.disableRangeAnalysis;
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

    IonGvnKind gvnKind() const {
        if (!js_JitOptions.forceGvnKind)
            return gvnKind_;
        return js_JitOptions.forcedGvnKind;
    }

    IonRegisterAllocator registerAllocator() const {
        if (!js_JitOptions.forceRegisterAllocator)
            return registerAllocator_;
        return js_JitOptions.forcedRegisterAllocator;
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

    uint32_t usesBeforeInlining() const {
        return usesBeforeCompile() * usesBeforeInliningFactor_;
    }
};

class OptimizationInfos
{
  private:
    OptimizationInfo infos_[Optimization_Count - 1];

  public:
    OptimizationInfos();

    const OptimizationInfo *get(OptimizationLevel level) {
        JS_ASSERT(level < Optimization_Count);
        JS_ASSERT(level != Optimization_DontCompile);

        return &infos_[level - 1];
    }

    OptimizationLevel nextLevel(OptimizationLevel level);
    bool isLastLevel(OptimizationLevel level);
    OptimizationLevel levelForUseCount(uint32_t useCount);
};

extern OptimizationInfos js_IonOptimizations;

} 
} 

#endif 
