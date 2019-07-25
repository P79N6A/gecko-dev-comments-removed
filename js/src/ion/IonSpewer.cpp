








































#include "IonSpewer.h"

using namespace js;
using namespace js::ion;

static bool LoggingChecked = false;

static uint32 LoggingBits = 0;

static const char *ChannelNames[] =
{
#define IONSPEW_CHANNEL(name) #name,
    IONSPEW_CHANNEL_LIST(IONSPEW_CHANNEL)
#undef IONSPEW_CHANNEL
};

bool
IonSpewer::init()
{
    c1Spewer.enable("/tmp/ion.cfg");
    if (!jsonSpewer.init("/tmp/ion.json"))
        return false;
    jsonSpewer.beginFunction(function);
    return true;
}

void
IonSpewer::spewPass(const char *pass)
{
    c1Spewer.spew(pass);
    jsonSpewer.beginPass(pass);
    jsonSpewer.spewMIR(graph);
    jsonSpewer.spewLIR(graph);
    jsonSpewer.endPass();
}

void
IonSpewer::finish()
{
    jsonSpewer.endFunction();
    jsonSpewer.finish();
}

#ifdef DEBUG
FILE *ion::IonSpewFile = NULL;

void
ion::CheckLogging()
{
    if (LoggingChecked)
        return;
    LoggingChecked = true;
    const char *env = getenv("IONFLAGS");
    if (!env)
        return;
    if (strstr(env, "help")) {
        fflush(NULL);
        printf(
            "\n"
            "usage: IONFLAGS=option,option,option,... where options can be:\n"
            "\n"
            "  aborts   Compilation abort messages\n"
            "  mir      MIR information\n"
            "  licm     Loop invariant code motion\n"

            "  all      Everything\n"
            "\n"
        );
        exit(0);
        
    }
    if (strstr(env, "aborts"))
        LoggingBits |= (1 << uint32(IonSpew_Abort));
    if (strstr(env, "mir"))
        LoggingBits |= (1 << uint32(IonSpew_MIR));
    if (strstr(env, "licm"))
        LoggingBits |= (1 << uint32(IonSpew_LICM));
    if (strstr(env, "all"))
        LoggingBits = uint32(-1);

    IonSpewFile = stderr;
}

void
ion::IonSpewVA(IonSpewChannel channel, const char *fmt, va_list ap)
{
    if (!IonSpewEnabled(channel))
        return;

    IonSpewHeader(channel);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

void
ion::IonSpew(IonSpewChannel channel, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    IonSpewVA(channel, fmt, ap);
    va_end(ap);
}

void
ion::IonSpewHeader(IonSpewChannel channel)
{
    if (!IonSpewEnabled(channel))
        return;

    fprintf(stderr, "[%s] ", ChannelNames[channel]);
}

bool
ion::IonSpewEnabled(IonSpewChannel channel)
{
    JS_ASSERT(LoggingChecked);

    return LoggingBits & (1 << uint32(channel));
}

#endif
