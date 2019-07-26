






#ifndef ParallelDo_h__
#define ParallelDo_h__

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "ion/Ion.h"

namespace js {
namespace parallel {





bool Do(JSContext *cx, CallArgs &args);




enum ExecutionStatus {
    
    ExecutionFatal,

    
    ExecutionSequential,

    
    ExecutionParallel
};

enum SpewChannel {
    SpewOps,
    SpewCompile,
    SpewBailouts,
    NumSpewChannels
};

#if defined(DEBUG) && defined(JS_THREADSAFE) && defined(JS_ION)

bool SpewEnabled(SpewChannel channel);
void Spew(SpewChannel channel, const char *fmt, ...);
void SpewBeginOp(JSContext *cx, const char *name);
void SpewBailout(uint32_t count);
ExecutionStatus SpewEndOp(ExecutionStatus status);
void SpewBeginCompile(HandleFunction fun);
ion::MethodStatus SpewEndCompile(ion::MethodStatus status);
void SpewMIR(ion::MDefinition *mir, const char *fmt, ...);
void SpewBailoutIR(uint32_t bblockId, uint32_t lirId,
                   const char *lir, const char *mir, JSScript *script, jsbytecode *pc);

#else

static inline bool SpewEnabled(SpewChannel channel) { return false; }
static inline void Spew(SpewChannel channel, const char *fmt, ...) { }
static inline void SpewBeginOp(JSContext *cx, const char *name) { }
static inline void SpewBailout(uint32_t count) {}
static inline ExecutionStatus SpewEndOp(ExecutionStatus status) { return status; }
static inline void SpewBeginCompile(HandleFunction fun) { }
#ifdef JS_ION
static inline ion::MethodStatus SpewEndCompile(ion::MethodStatus status) { return status; }
static inline void SpewMIR(ion::MDefinition *mir, const char *fmt, ...) { }
#endif
static inline void SpewBailoutIR(uint32_t bblockId, uint32_t lirId,
                                 const char *lir, const char *mir,
                                 JSScript *script, jsbytecode *pc) { }

#endif 

} 
} 

#endif
