






#if !defined(jsion_baseline_jit_h__) && defined(JS_ION)
#define jsion_baseline_jit_h__

#include "jscntxt.h"
#include "jscompartment.h"
#include "IonCode.h"
#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

struct CacheData;

struct BaselineScript
{
  private:
    
    HeapPtr<IonCode> method_;

  private:
    void trace(JSTracer *trc);

    uint32 cacheList_;
    uint32 cacheEntries_;

  public:
    
    BaselineScript();

    static BaselineScript *New(JSContext *cx, size_t cacheEntries);
    static void Trace(JSTracer *trc, BaselineScript *script);
    static void Destroy(FreeOp *fop, BaselineScript *script);

    static inline size_t offsetOfMethod() {
        return offsetof(BaselineScript, method_);
    }

    CacheData *cacheList() {
        return (CacheData *)(reinterpret_cast<uint8 *>(this) + cacheList_);
    }

    IonCode *method() const {
        return method_;
    }
    void setMethod(IonCode *code) {
        
        method_ = code;
    }

    CacheData &getCache(size_t index);
    CacheData &cacheDataFromReturnAddr(uint8_t *addr);

    size_t numCaches() const {
        return cacheEntries_;
    }

    void copyCacheEntries(const CacheData *caches, MacroAssembler &masm);
};

MethodStatus
CanEnterBaselineJIT(JSContext *cx, HandleScript script, StackFrame *fp);

IonExecStatus
EnterBaselineMethod(JSContext *cx, StackFrame *fp);

} 
} 

#endif

