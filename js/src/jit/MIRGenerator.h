





#ifndef jit_MIRGenerator_h
#define jit_MIRGenerator_h




#include "mozilla/Atomics.h"

#include <stdarg.h>

#include "jscntxt.h"
#include "jscompartment.h"

#include "jit/CompileInfo.h"
#include "jit/JitAllocPolicy.h"
#include "jit/JitCompartment.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/RegisterSets.h"

namespace js {
namespace jit {

class MBasicBlock;
class MIRGraph;
class MStart;
class OptimizationInfo;

class MIRGenerator
{
  public:
    MIRGenerator(CompileCompartment *compartment, const JitCompileOptions &options,
                 TempAllocator *alloc, MIRGraph *graph,
                 CompileInfo *info, const OptimizationInfo *optimizationInfo);

    TempAllocator &alloc() {
        return *alloc_;
    }
    MIRGraph &graph() {
        return *graph_;
    }
    bool ensureBallast() {
        return alloc().ensureBallast();
    }
    const JitRuntime *jitRuntime() const {
        return GetJitContext()->runtime->jitRuntime();
    }
    CompileInfo &info() {
        return *info_;
    }
    const OptimizationInfo &optimizationInfo() const {
        return *optimizationInfo_;
    }

    template <typename T>
    T * allocate(size_t count = 1) {
        if (count & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
            return nullptr;
        return reinterpret_cast<T *>(alloc().allocate(sizeof(T) * count));
    }

    
    
    bool abort(const char *message, ...);
    bool abortFmt(const char *message, va_list ap);

    bool errored() const {
        return error_;
    }

    bool instrumentedProfiling() {
        if (!instrumentedProfilingIsCached_) {
            instrumentedProfiling_ = GetJitContext()->runtime->spsProfiler().enabled();
            instrumentedProfilingIsCached_ = true;
        }
        return instrumentedProfiling_;
    }

    bool isProfilerInstrumentationEnabled() {
        return !compilingAsmJS() && instrumentedProfiling();
    }

    
    bool shouldCancel(const char *why) {
        maybePause();
        return cancelBuild_;
    }
    void cancel() {
        cancelBuild_ = true;
    }

    void maybePause() {
        if (pauseBuild_ && *pauseBuild_)
            PauseCurrentHelperThread();
    }
    void setPauseFlag(mozilla::Atomic<bool, mozilla::Relaxed> *pauseBuild) {
        pauseBuild_ = pauseBuild;
    }

    void disable() {
        abortReason_ = AbortReason_Disable;
    }
    AbortReason abortReason() {
        return abortReason_;
    }

    bool compilingAsmJS() const {
        return info_->compilingAsmJS();
    }

    uint32_t maxAsmJSStackArgBytes() const {
        MOZ_ASSERT(compilingAsmJS());
        return maxAsmJSStackArgBytes_;
    }
    uint32_t resetAsmJSMaxStackArgBytes() {
        MOZ_ASSERT(compilingAsmJS());
        uint32_t old = maxAsmJSStackArgBytes_;
        maxAsmJSStackArgBytes_ = 0;
        return old;
    }
    void setAsmJSMaxStackArgBytes(uint32_t n) {
        MOZ_ASSERT(compilingAsmJS());
        maxAsmJSStackArgBytes_ = n;
    }
    void setPerformsCall() {
        performsCall_ = true;
    }
    bool performsCall() const {
        return performsCall_;
    }
    
    
    bool usesSimd();
    void initMinAsmJSHeapLength(uint32_t len) {
        MOZ_ASSERT(minAsmJSHeapLength_ == 0);
        minAsmJSHeapLength_ = len;
    }
    uint32_t minAsmJSHeapLength() const {
        return minAsmJSHeapLength_;
    }

    bool modifiesFrameArguments() const {
        return modifiesFrameArguments_;
    }

    typedef Vector<types::TypeObject *, 0, JitAllocPolicy> TypeObjectVector;

    
    
    const TypeObjectVector &abortedNewScriptPropertiesTypes() const {
        return abortedNewScriptPropertiesTypes_;
    }

  public:
    CompileCompartment *compartment;

  protected:
    CompileInfo *info_;
    const OptimizationInfo *optimizationInfo_;
    TempAllocator *alloc_;
    JSFunction *fun_;
    uint32_t nslots_;
    MIRGraph *graph_;
    AbortReason abortReason_;
    bool shouldForceAbort_; 
    TypeObjectVector abortedNewScriptPropertiesTypes_;
    bool error_;
    mozilla::Atomic<bool, mozilla::Relaxed> *pauseBuild_;
    mozilla::Atomic<bool, mozilla::Relaxed> cancelBuild_;

    uint32_t maxAsmJSStackArgBytes_;
    bool performsCall_;
    bool usesSimd_;
    bool usesSimdCached_;
    uint32_t minAsmJSHeapLength_;

    
    
    
    bool modifiesFrameArguments_;

    bool instrumentedProfiling_;
    bool instrumentedProfilingIsCached_;

    void addAbortedNewScriptPropertiesType(types::TypeObject *type);
    void setForceAbort() {
        shouldForceAbort_ = true;
    }
    bool shouldForceAbort() {
        return shouldForceAbort_;
    }

#if defined(JS_ION_PERF)
    AsmJSPerfSpewer asmJSPerfSpewer_;

  public:
    AsmJSPerfSpewer &perfSpewer() { return asmJSPerfSpewer_; }
#endif

  public:
    const JitCompileOptions options;
};

} 
} 

#endif 
