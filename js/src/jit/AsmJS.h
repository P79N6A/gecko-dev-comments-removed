





#ifndef jit_AsmJS_h
#define jit_AsmJS_h

#include <stddef.h>

#include "js/TypeDecls.h"

namespace js {

class ExclusiveContext;
class AsmJSModule;
class SPSProfiler;
namespace frontend {
    template <typename ParseHandler> struct Parser;
    template <typename ParseHandler> struct ParseContext;
    class FullParseHandler;
    struct ParseNode;
}

typedef frontend::Parser<frontend::FullParseHandler> AsmJSParser;
typedef frontend::ParseContext<frontend::FullParseHandler> AsmJSParseContext;






extern bool
CompileAsmJS(ExclusiveContext *cx, AsmJSParser &parser, frontend::ParseNode *stmtList,
             bool *validated);











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
    AsmJSActivation *prev() const { return prev_; }

    
    static unsigned offsetOfContext() { return offsetof(AsmJSActivation, cx_); }
    static unsigned offsetOfResumePC() { return offsetof(AsmJSActivation, resumePC_); }

    
    static unsigned offsetOfErrorRejoinSP() { return offsetof(AsmJSActivation, errorRejoinSP_); }

    
    void setResumePC(void *pc) { resumePC_ = pc; }
};


const size_t AsmJSPageSize = 4096;


static const size_t AsmJSAllocationGranularity = 4096;


extern uint32_t
RoundUpToNextValidAsmJSHeapLength(uint32_t length);

extern bool
IsValidAsmJSHeapLength(uint32_t length);

#ifdef JS_CPU_X64



static const size_t AsmJSBufferProtectedSize = 4 * 1024ULL * 1024ULL * 1024ULL;
#endif

#ifdef JS_ION



extern bool
IsAsmJSCompilationAvailable(JSContext *cx, unsigned argc, JS::Value *vp);

#else 

inline bool
IsAsmJSCompilationAvailable(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().set(BooleanValue(false));
    return true;
}

#endif 

} 

#endif 
