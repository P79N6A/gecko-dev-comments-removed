





#include "jit/IonOptimizationLevels.h"

#include "jsanalyze.h"
#include "jsscript.h"

using namespace js;
using namespace js::jit;

namespace js {
namespace jit {

OptimizationInfos js_IonOptimizations;

void
OptimizationInfo::initNormalOptimizationInfo()
{
    level_ = Optimization_Normal;

    eaa_ = true;
    edgeCaseAnalysis_ = true;
    eliminateRedundantChecks_ = true;
    inlineInterpreted_ = true;
    inlineNative_ = true;
    gvn_ = true;
    gvnKind_ = GVN_Optimistic;
    licm_ = true;
    uce_ = true;
    rangeAnalysis_ = true;
    registerAllocator_ = RegisterAllocator_LSRA;

    inlineMaxTotalBytecodeLength_ = 1000;
    inliningMaxCallerBytecodeLength_ = 10000;
    maxInlineDepth_ = 3;
    smallFunctionMaxInlineDepth_ = 10;
    usesBeforeCompile_ = 1000;
    usesBeforeInliningFactor_ = 0.125;
}

void
OptimizationInfo::initAsmjsOptimizationInfo()
{
    
    

    
    initNormalOptimizationInfo();

    level_ = Optimization_AsmJS;
    edgeCaseAnalysis_ = false;
    eliminateRedundantChecks_ = false;
}

OptimizationInfos::OptimizationInfos()
{
    infos_[Optimization_Normal - 1].initNormalOptimizationInfo();
    infos_[Optimization_AsmJS - 1].initAsmjsOptimizationInfo();

#ifdef DEBUG
    OptimizationLevel prev = nextLevel(Optimization_DontCompile);
    while (!isLastLevel(prev)) {
        OptimizationLevel next = nextLevel(prev);
        JS_ASSERT(get(prev)->usesBeforeCompile() < get(next)->usesBeforeCompile());
        prev = next;
    }
#endif
}

OptimizationLevel
OptimizationInfos::nextLevel(OptimizationLevel level)
{
    JS_ASSERT(!isLastLevel(level));
    switch (level) {
      case Optimization_DontCompile:
        return Optimization_Normal;
      default:
        MOZ_ASSUME_UNREACHABLE("Unknown optimization level.");
    }
}

bool
OptimizationInfos::isLastLevel(OptimizationLevel level)
{
    return level == Optimization_Normal;
}

OptimizationLevel
OptimizationInfos::levelForUseCount(uint32_t useCount)
{
    OptimizationLevel prev = Optimization_DontCompile;

    while (!isLastLevel(prev)) {
        OptimizationLevel level = nextLevel(prev);
        const OptimizationInfo *info = get(level);
        if (useCount < info->usesBeforeCompile())
            return prev;

        prev = level;
    }

    return prev;
}

} 
} 
