





#ifndef ion_AsmJS_h
#define ion_AsmJS_h

#ifdef XP_MACOSX
# include <pthread.h>
# include <mach/mach.h>
#endif

#include "jstypes.h"

#include "ds/LifoAlloc.h"
#include "js/CallArgs.h"

struct JSContext;
struct JSRuntime;

namespace js {

class ScriptSource;
class SPSProfiler;
class AsmJSModule;
namespace frontend {
    template <typename ParseHandler> struct Parser;
    template <typename ParseHandler> struct ParseContext;
    class FullParseHandler;
    struct ParseNode;
}
namespace ion {
    class MIRGenerator;
    class LIRGraph;
}

typedef frontend::Parser<frontend::FullParseHandler> AsmJSParser;
typedef frontend::ParseContext<frontend::FullParseHandler> AsmJSParseContext;






extern bool
CompileAsmJS(JSContext *cx, AsmJSParser &parser, frontend::ParseNode *stmtList, bool *validated);







extern JSBool
LinkAsmJS(JSContext *cx, unsigned argc, JS::Value *vp);



extern JSBool
CallAsmJS(JSContext *cx, unsigned argc, JS::Value *vp);



void
TriggerOperationCallbackForAsmJSCode(JSRuntime *rt);











class AsmJSActivation
{
    JSContext *cx_;
    AsmJSModule &module_;
    AsmJSActivation *prev_;
    void *errorRejoinSP_;
    SPSProfiler *profiler_;
    void *resumePC_;

  public:
    AsmJSActivation(JSContext *cx, AsmJSModule &module);
    ~AsmJSActivation();

    JSContext *cx() { return cx_; }
    AsmJSModule &module() const { return module_; }

    
    static unsigned offsetOfContext() { return offsetof(AsmJSActivation, cx_); }
    static unsigned offsetOfResumePC() { return offsetof(AsmJSActivation, resumePC_); }

    
    static unsigned offsetOfErrorRejoinSP() { return offsetof(AsmJSActivation, errorRejoinSP_); }

    
    void setResumePC(void *pc) { resumePC_ = pc; }
};


const size_t AsmJSPageSize = 4096;


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

struct DependentAsmJSModuleExit
{
    const AsmJSModule *module;
    size_t exitIndex;

    DependentAsmJSModuleExit(const AsmJSModule *module, size_t exitIndex)
      : module(module),
        exitIndex(exitIndex)
    { }
};



struct AsmJSParallelTask
{
    LifoAlloc lifo;         

    void *func;             
    ion::MIRGenerator *mir; 
    ion::LIRGraph *lir;     
    unsigned compileTime;

    AsmJSParallelTask(size_t defaultChunkSize)
      : lifo(defaultChunkSize), func(NULL), mir(NULL), lir(NULL), compileTime(0)
    { }

    void init(void *func, ion::MIRGenerator *mir) {
        this->func = func;
        this->mir = mir;
        this->lir = NULL;
    }
};



#ifdef JS_ION
extern bool
IsAsmJSModuleNative(JSNative native);
#else
inline bool
IsAsmJSModuleNative(JSNative native)
{
    return false;
}
#endif





extern JSBool
IsAsmJSCompilationAvailable(JSContext *cx, unsigned argc, JS::Value *vp);



extern JSBool
IsAsmJSModule(JSContext *cx, unsigned argc, JS::Value *vp);



extern JSBool
IsAsmJSFunction(JSContext *cx, unsigned argc, JS::Value *vp);

} 

#endif 
