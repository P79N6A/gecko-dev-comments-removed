





#include "jit/IonOptimizationLevels.h"

#include "jsscript.h"

#include "jit/Ion.h"

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
    eagerSimdUnbox_ = true;
    gvn_ = true;
    licm_ = true;
    rangeAnalysis_ = true;
    loopUnrolling_ = true;
    autoTruncate_ = true;
    sink_ = true;
    registerAllocator_ = RegisterAllocator_Backtracking;

    inlineMaxBytecodePerCallSiteMainThread_ = 500;
    inlineMaxBytecodePerCallSiteOffThread_ = 1000;
    inlineMaxCalleeInlinedBytecodeLength_ = 3000;
    inlineMaxTotalBytecodeLength_ = 80000;
    inliningMaxCallerBytecodeLength_ = 1500;
    maxInlineDepth_ = 3;
    scalarReplacement_ = true;
    smallFunctionMaxInlineDepth_ = 10;
    compilerWarmUpThreshold_ = CompilerWarmupThreshold;
    inliningWarmUpThresholdFactor_ = 0.125;
    inliningRecompileThresholdFactor_ = 4;
}

void
OptimizationInfo::initAsmjsOptimizationInfo()
{
    
    

    
    initNormalOptimizationInfo();

    ama_ = true;
    level_ = Optimization_AsmJS;
    eagerSimdUnbox_ = false;           
    edgeCaseAnalysis_ = false;
    eliminateRedundantChecks_ = false;
    autoTruncate_ = false;
    sink_ = false;
    registerAllocator_ = RegisterAllocator_Backtracking;
    scalarReplacement_ = false;        
}

uint32_t
OptimizationInfo::compilerWarmUpThreshold(JSScript* script, jsbytecode* pc) const
{
    MOZ_ASSERT(pc == nullptr || pc == script->code() || JSOp(*pc) == JSOP_LOOPENTRY);

    if (pc == script->code())
        pc = nullptr;

    uint32_t warmUpThreshold = compilerWarmUpThreshold_;
    if (js_JitOptions.forcedDefaultIonWarmUpThreshold.isSome())
        warmUpThreshold = js_JitOptions.forcedDefaultIonWarmUpThreshold.ref();

    
    
    
    

    if (script->length() > MAX_MAIN_THREAD_SCRIPT_SIZE)
        warmUpThreshold *= (script->length() / (double) MAX_MAIN_THREAD_SCRIPT_SIZE);

    uint32_t numLocalsAndArgs = NumLocalsAndArgs(script);
    if (numLocalsAndArgs > MAX_MAIN_THREAD_LOCALS_AND_ARGS)
        warmUpThreshold *= (numLocalsAndArgs / (double) MAX_MAIN_THREAD_LOCALS_AND_ARGS);

    if (!pc || js_JitOptions.eagerCompilation)
        return warmUpThreshold;

    
    
    
    uint32_t loopDepth = LoopEntryDepthHint(pc);
    MOZ_ASSERT(loopDepth > 0);
    return warmUpThreshold + loopDepth * 100;
}

OptimizationInfos::OptimizationInfos()
{
    infos_[Optimization_Normal - 1].initNormalOptimizationInfo();
    infos_[Optimization_AsmJS - 1].initAsmjsOptimizationInfo();

#ifdef DEBUG
    OptimizationLevel level = firstLevel();
    while (!isLastLevel(level)) {
        OptimizationLevel next = nextLevel(level);
        MOZ_ASSERT(level < next);
        level = next;
    }
#endif
}

OptimizationLevel
OptimizationInfos::nextLevel(OptimizationLevel level) const
{
    MOZ_ASSERT(!isLastLevel(level));
    switch (level) {
      case Optimization_DontCompile:
        return Optimization_Normal;
      default:
        MOZ_CRASH("Unknown optimization level.");
    }
}

OptimizationLevel
OptimizationInfos::firstLevel() const
{
    return nextLevel(Optimization_DontCompile);
}

bool
OptimizationInfos::isLastLevel(OptimizationLevel level) const
{
    return level == Optimization_Normal;
}

OptimizationLevel
OptimizationInfos::levelForScript(JSScript* script, jsbytecode* pc) const
{
    OptimizationLevel prev = Optimization_DontCompile;

    while (!isLastLevel(prev)) {
        OptimizationLevel level = nextLevel(prev);
        const OptimizationInfo* info = get(level);
        if (script->getWarmUpCount() < info->compilerWarmUpThreshold(script, pc))
            return prev;

        prev = level;
    }

    return prev;
}

} 
} 
