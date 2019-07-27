





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
    gvn_ = true;
    licm_ = true;
    uce_ = true;
    rangeAnalysis_ = true;
    autoTruncate_ = true;
    registerAllocator_ = RegisterAllocator_LSRA;

    inlineMaxTotalBytecodeLength_ = 1000;
    inliningMaxCallerBytecodeLength_ = 10000;
    maxInlineDepth_ = 3;
    scalarReplacement_ = true;
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
    autoTruncate_ = false;
    registerAllocator_ = RegisterAllocator_Backtracking;
    scalarReplacement_ = false;        
}

uint32_t
OptimizationInfo::usesBeforeCompile(JSScript *script, jsbytecode *pc) const
{
    JS_ASSERT(pc == nullptr || pc == script->code() || JSOp(*pc) == JSOP_LOOPENTRY);

    if (pc == script->code())
        pc = nullptr;

    uint32_t minUses = usesBeforeCompile_;
    if (js_JitOptions.forceDefaultIonUsesBeforeCompile)
        minUses = js_JitOptions.forcedDefaultIonUsesBeforeCompile;

    
    
    
    

    if (script->length() > MAX_MAIN_THREAD_SCRIPT_SIZE)
        minUses = minUses * (script->length() / (double) MAX_MAIN_THREAD_SCRIPT_SIZE);

    uint32_t numLocalsAndArgs = NumLocalsAndArgs(script);
    if (numLocalsAndArgs > MAX_MAIN_THREAD_LOCALS_AND_ARGS)
        minUses = minUses * (numLocalsAndArgs / (double) MAX_MAIN_THREAD_LOCALS_AND_ARGS);

    if (!pc || js_JitOptions.eagerCompilation)
        return minUses;

    
    
    
    uint32_t loopDepth = LoopEntryDepthHint(pc);
    JS_ASSERT(loopDepth > 0);
    return minUses + loopDepth * 100;
}

OptimizationInfos::OptimizationInfos()
{
    infos_[Optimization_Normal - 1].initNormalOptimizationInfo();
    infos_[Optimization_AsmJS - 1].initAsmjsOptimizationInfo();

#ifdef DEBUG
    OptimizationLevel level = firstLevel();
    while (!isLastLevel(level)) {
        OptimizationLevel next = nextLevel(level);
        JS_ASSERT(level < next);
        level = next;
    }
#endif
}

OptimizationLevel
OptimizationInfos::nextLevel(OptimizationLevel level) const
{
    JS_ASSERT(!isLastLevel(level));
    switch (level) {
      case Optimization_DontCompile:
        return Optimization_Normal;
      default:
        MOZ_ASSUME_UNREACHABLE("Unknown optimization level.");
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
OptimizationInfos::levelForScript(JSScript *script, jsbytecode *pc) const
{
    OptimizationLevel prev = Optimization_DontCompile;

    while (!isLastLevel(prev)) {
        OptimizationLevel level = nextLevel(prev);
        const OptimizationInfo *info = get(level);
        if (script->getUseCount() < info->usesBeforeCompile(script, pc))
            return prev;

        prev = level;
    }

    return prev;
}

} 
} 
