





#ifndef jit_IonOptimizationLevels_h
#define jit_IonOptimizationLevels_h

#include "jsbytecode.h"
#include "jstypes.h"

#include "jit/IonOptions.h"
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
        return inlineInterpreted_ && !js_IonOptions.disableInlining;
    }

    bool inlineNative() const {
        return inlineNative_ && !js_IonOptions.disableInlining;
    }

    uint32_t usesBeforeCompile() const {
        if (js_IonOptions.forceDefaultIonUsesBeforeCompile)
            return js_IonOptions.forcedDefaultIonUsesBeforeCompile;
        return usesBeforeCompile_;
    }

    bool gvnEnabled() const {
        return gvn_ && !js_IonOptions.disableGvn;
    }

    bool licmEnabled() const {
        return licm_ && !js_IonOptions.disableLicm;
    }

    bool uceEnabled() const {
        return uce_ && !js_IonOptions.disableUce;
    }

    bool rangeAnalysisEnabled() const {
        return rangeAnalysis_ && !js_IonOptions.disableRangeAnalysis;
    }

    bool eaaEnabled() const {
        return eaa_ && !js_IonOptions.disableEaa;
    }

    bool edgeCaseAnalysisEnabled() const {
        return edgeCaseAnalysis_ && !js_IonOptions.disableEdgeCaseAnalysis;
    }

    bool eliminateRedundantChecksEnabled() const {
        return eliminateRedundantChecks_;
    }

    IonGvnKind gvnKind() const {
        if (!js_IonOptions.forceGvnKind)
            return gvnKind_;
        return js_IonOptions.forcedGvnKind;
    }

    IonRegisterAllocator registerAllocator() const {
        if (!js_IonOptions.forceRegisterAllocator)
            return registerAllocator_;
        return js_IonOptions.forcedRegisterAllocator;
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
