






#if !defined(jsion_asmjs_h__)
#define jsion_asmjs_h__

#ifdef XP_MACOSX
# include <pthread.h>
# include <mach/mach.h>
#endif



#if defined(JS_ION) && \
    !defined(ANDROID) && \
    (defined(JS_CPU_X86) || defined(JS_CPU_X64))
# define JS_ASMJS
#endif

namespace js {

class ScriptSource;
class SPSProfiler;
class AsmJSModule;
namespace frontend { struct TokenStream; struct ParseNode; }
namespace ion { class MIRGenerator; class LIRGraph; }



extern JSBool
IsAsmJSCompilationAvailable(JSContext *cx, unsigned argc, Value *vp);







extern bool
CompileAsmJS(JSContext *cx, frontend::TokenStream &ts, frontend::ParseNode *fn,
             const CompileOptions &options,
             ScriptSource *scriptSource, uint32_t bufStart, uint32_t bufEnd,
             MutableHandleFunction moduleFun);








extern JSBool
LinkAsmJS(JSContext *cx, unsigned argc, JS::Value *vp);



void
TriggerOperationCallbackForAsmJSCode(JSRuntime *rt);











class AsmJSActivation
{
    JSContext *cx_;
    const AsmJSModule &module_;
    unsigned entryIndex_;
    AsmJSActivation *prev_;
    void *errorRejoinSP_;
    SPSProfiler *profiler_;
    void *resumePC_;

  public:
    AsmJSActivation(JSContext *cx, const AsmJSModule &module, unsigned entryIndex);
    ~AsmJSActivation();

    const AsmJSModule &module() const { return module_; }

    
    static unsigned offsetOfContext() { return offsetof(AsmJSActivation, cx_); }
    static unsigned offsetOfResumePC() { return offsetof(AsmJSActivation, resumePC_); }

    
    static unsigned offsetOfErrorRejoinSP() { return offsetof(AsmJSActivation, errorRejoinSP_); }

    
    void setResumePC(void *pc) { resumePC_ = pc; }
};


static const size_t AsmJSAllocationGranularity = 4096;

#ifdef JS_CPU_X64



static const size_t AsmJSBufferProtectedSize = 4 * 1024ULL * 1024ULL * 1024ULL;
#endif

#ifdef XP_MACOSX
class AsmJSMachExceptionHandler
{
    bool installed_;
    pthread_t thread_;
    mach_port_t port_;

    void release();

  public:
    AsmJSMachExceptionHandler();
    ~AsmJSMachExceptionHandler() { release(); }
    mach_port_t port() const { return port_; }
    bool installed() const { return installed_; }
    bool install(JSRuntime *rt);
    void clearCurrentThread();
    void setCurrentThread();
};
#endif



struct AsmJSParallelTask
{
    LifoAlloc lifo;         

    uint32_t funcNum;       
    ion::MIRGenerator *mir; 
    ion::LIRGraph *lir;     

    AsmJSParallelTask(size_t defaultChunkSize)
      : lifo(defaultChunkSize),
        funcNum(0), mir(NULL), lir(NULL)
    { }

    void init(uint32_t newFuncNum, ion::MIRGenerator *newMir) {
        funcNum = newFuncNum;
        mir = newMir;
        lir = NULL;
    }
};



#ifdef JS_ASMJS
bool
IsAsmJSModuleNative(js::Native native);
#else
static inline bool
IsAsmJSModuleNative(js::Native native)
{
    return false;
}
#endif

} 

#endif 
