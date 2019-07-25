







































#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "jsutil.h"
#include "MethodJIT.h"
#include "Logging.h"

#if defined(JS_METHODJIT_SPEW)

static bool LoggingChecked = false;
static uint32 LoggingBits = 0;

static const char *ChannelNames[] =
{
#define _(name) #name,
    JSPEW_CHAN_MAP(_)
#undef  _
};

void
js::JMCheckLogging()
{
    
    if (LoggingChecked)
        return;
    LoggingChecked = true;
    const char *env = getenv("JMFLAGS");
    if (!env)
        return;
    if (strstr(env, "help")) {
        fflush(NULL);
        printf(
            "\n"
            "usage: JMFLAGS=option,option,option,... where options can be:\n"
            "\n"
            "  help          show this message\n"
            "  abort/aborts  ???\n"
            "  scripts       ???\n"
            "  profile       ???\n"
#ifdef DEBUG
            "  pcprofile     Runtime hit counts of every JS opcode executed\n"
            "  jsops         JS opcodes\n"
#endif
            "  insns         JS opcodes and generated insns\n"
            "  vmframe       VMFrame contents\n"
            "  pics          PIC patching activity\n"
            "  slowcalls     Calls to slow path functions\n"
            "  full          everything\n"
            "  notrace       disable trace hints\n"
            "\n"
        );
        exit(0);
        
    }
    if (strstr(env, "abort") || strstr(env, "aborts"))
        LoggingBits |= (1 << uint32(JSpew_Abort));
    if (strstr(env, "scripts"))
        LoggingBits |= (1 << uint32(JSpew_Scripts));
    if (strstr(env, "profile"))
        LoggingBits |= (1 << uint32(JSpew_Prof));
#ifdef DEBUG
    if (strstr(env, "pcprofile"))
        LoggingBits |= (1 << uint32(JSpew_PCProf));
    if (strstr(env, "jsops"))
        LoggingBits |= (1 << uint32(JSpew_JSOps));
#endif
    if (strstr(env, "insns"))
        LoggingBits |= (1 << uint32(JSpew_Insns) | (1 << uint32(JSpew_JSOps)));
    if (strstr(env, "vmframe"))
        LoggingBits |= (1 << uint32(JSpew_VMFrame));
    if (strstr(env, "pics"))
        LoggingBits |= (1 << uint32(JSpew_PICs));
    if (strstr(env, "slowcalls"))
        LoggingBits |= (1 << uint32(JSpew_SlowCalls));
    if (strstr(env, "full"))
        LoggingBits |= 0xFFFFFFFF;
}

bool
js::IsJaegerSpewChannelActive(JaegerSpewChannel channel)
{
    JS_ASSERT(LoggingChecked);
    return !!(LoggingBits & (1 << uint32(channel)));
}

void
js::JaegerSpew(JaegerSpewChannel channel, const char *fmt, ...)
{
    JS_ASSERT(LoggingChecked);

    if (!(LoggingBits & (1 << uint32(channel))))
        return;

    fprintf(stderr, "[jaeger] %-7s  ", ChannelNames[channel]);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    
}

#endif

