





#ifndef jit_MIRGenerator_h
#define jit_MIRGenerator_h




#include <stdarg.h>

#include "jscntxt.h"
#include "jscompartment.h"

#include "jit/CompileInfo.h"
#include "jit/IonAllocPolicy.h"
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
        return GetIonContext()->runtime->jitRuntime();
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
        return GetIonContext()->runtime->spsProfiler().enabled();
    }

    
    bool shouldCancel(const char *why) {
        return cancelBuild_;
    }
    void cancel() {
        cancelBuild_ = 1;
    }

    bool compilingAsmJS() const {
        return info_->script() == nullptr;
    }

    uint32_t maxAsmJSStackArgBytes() const {
        JS_ASSERT(compilingAsmJS());
        return maxAsmJSStackArgBytes_;
    }
    uint32_t resetAsmJSMaxStackArgBytes() {
        JS_ASSERT(compilingAsmJS());
        uint32_t old = maxAsmJSStackArgBytes_;
        maxAsmJSStackArgBytes_ = 0;
        return old;
    }
    void setAsmJSMaxStackArgBytes(uint32_t n) {
        JS_ASSERT(compilingAsmJS());
        maxAsmJSStackArgBytes_ = n;
    }
    void setPerformsAsmJSCall() {
        JS_ASSERT(compilingAsmJS());
        performsAsmJSCall_ = true;
    }
    bool performsAsmJSCall() const {
        JS_ASSERT(compilingAsmJS());
        return performsAsmJSCall_;
    }
    bool noteHeapAccess(AsmJSHeapAccess heapAccess) {
        return asmJSHeapAccesses_.append(heapAccess);
    }
    const Vector<AsmJSHeapAccess, 0, IonAllocPolicy> &heapAccesses() const {
        return asmJSHeapAccesses_;
    }
    void noteMinAsmJSHeapLength(uint32_t len) {
        minAsmJSHeapLength_ = len;
    }
    uint32_t minAsmJSHeapLength() const {
        return minAsmJSHeapLength_;
    }
    bool noteGlobalAccess(unsigned offset, unsigned globalDataOffset) {
        return asmJSGlobalAccesses_.append(AsmJSGlobalAccess(offset, globalDataOffset));
    }
    const Vector<AsmJSGlobalAccess, 0, IonAllocPolicy> &globalAccesses() const {
        return asmJSGlobalAccesses_;
    }

    bool modifiesFrameArguments() const {
        return modifiesFrameArguments_;
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
    bool error_;
    size_t cancelBuild_;

    uint32_t maxAsmJSStackArgBytes_;
    bool performsAsmJSCall_;
    AsmJSHeapAccessVector asmJSHeapAccesses_;
    AsmJSGlobalAccessVector asmJSGlobalAccesses_;
    uint32_t minAsmJSHeapLength_;

    
    
    
    bool modifiesFrameArguments_;

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
