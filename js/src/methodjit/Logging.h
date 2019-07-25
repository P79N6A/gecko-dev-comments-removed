







































#if !defined jsjaeger_logging_h__
#define jsjaeger_logging_h__

#include "assembler/wtf/Platform.h"
#include "prmjtime.h"

#if defined(JS_METHODJIT) || ENABLE_YARR_JIT

namespace js {

#define JSPEW_CHAN_MAP(_)   \
    _(Abort)                \
    _(Scripts)              \
    _(Prof)                 \
    _(JSOps)                \
    _(Insns)                \
    _(VMFrame)              \
    _(PICs)                 \
    _(SlowCalls)            \
    _(Analysis)             \
    _(Regalloc)             \
    _(Inlining)             \
    _(Recompile)

enum JaegerSpewChannel {
#define _(name) JSpew_##name,
    JSPEW_CHAN_MAP(_)
#undef  _
    JSpew_Terminator
};

#if defined(DEBUG) && !defined(JS_METHODJIT_SPEW)
# define JS_METHODJIT_SPEW
#endif

#if defined(JS_METHODJIT_SPEW)

void JMCheckLogging();

struct ConditionalLog {
    uint32 oldBits;
    bool logging;
    ConditionalLog(bool logging);
    ~ConditionalLog();
};

bool IsJaegerSpewChannelActive(JaegerSpewChannel channel);
void JaegerSpew(JaegerSpewChannel channel, const char *fmt, ...);

struct Profiler {
    JSInt64 t_start;
    JSInt64 t_stop;

    static inline JSInt64 now() {
        return PRMJ_Now();
    }

    inline void start() {
        t_start = now();
    }

    inline void stop() {
        t_stop = now();
    }

    inline uint32 time_ms() {
        return uint32((t_stop - t_start) / PRMJ_USEC_PER_MSEC);
    }

    inline uint32 time_us() {
        return uint32(t_stop - t_start);
    }
};

#else

static inline void JaegerSpew(JaegerSpewChannel channel, const char *fmt, ...)
{
}

#endif

}

#endif

#endif

