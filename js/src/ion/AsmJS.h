






#if !defined(jsion_asmjs_h__)
#define jsion_asmjs_h__



#if defined(JS_ION) && \
    !defined(ANDROID) && \
    (defined(JS_CPU_X86) || defined(JS_CPU_X64)) &&  \
    (defined(__linux__) || defined(XP_WIN) || defined(XP_MACOSX))
# define JS_ASMJS
#endif

namespace js {

class SPSProfiler;
class AsmJSModule;
namespace frontend { struct TokenStream; struct ParseNode; }



extern JSBool
IsAsmJSCompilationAvailable(JSContext *cx, unsigned argc, Value *vp);






extern bool
CompileAsmJS(JSContext *cx, frontend::TokenStream &ts, frontend::ParseNode *fn, HandleScript s);










extern bool
LinkAsmJS(JSContext *cx, StackFrame *fp, MutableHandleValue rval);



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




# ifdef JS_CPU_X64
static const size_t AsmJSBufferProtectedSize = 4 * 1024ULL * 1024ULL * 1024ULL;
# endif

} 

#endif 
