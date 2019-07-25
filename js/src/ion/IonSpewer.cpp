








































#ifdef DEBUG

#include "IonSpewer.h"

using namespace js;
using namespace js::ion;


static IonSpewer ionspewer;

static bool LoggingChecked = false;
static uint32 LoggingBits = 0;

static const char *ChannelNames[] =
{
#define IONSPEW_CHANNEL(name) #name,
    IONSPEW_CHANNEL_LIST(IONSPEW_CHANNEL)
#undef IONSPEW_CHANNEL
};


void
ion::IonSpewNewFunction(MIRGraph *graph, JSScript *function)
{
    if (!ionspewer.init())
        return;
    ionspewer.beginFunction(graph, function);
}

void
ion::IonSpewPass(const char *pass)
{
    ionspewer.spewPass(pass);
}

void
ion::IonSpewPass(const char *pass, LinearScanAllocator *ra)
{
    ionspewer.spewPass(pass, ra);
}

void
ion::IonSpewEndFunction()
{
    ionspewer.endFunction();
}


IonSpewer::~IonSpewer()
{
    if (!inited_)
        return;

    c1Spewer.finish();
    jsonSpewer.finish();
}

bool
IonSpewer::init()
{
    if (inited_)
        return true;

    if (!c1Spewer.init("/tmp/ion.cfg"))
        return false;
    if (!jsonSpewer.init("/tmp/ion.json"))
        return false;

    inited_ = true;
    return true;
}

void
IonSpewer::beginFunction(MIRGraph *graph, JSScript *function)
{
    if (!inited_)
        return;

    this->graph = graph;
    this->function = function;

    c1Spewer.beginFunction(graph, function);
    jsonSpewer.beginFunction(function);
}

void
IonSpewer::spewPass(const char *pass)
{
    if (!inited_)
        return;

    c1Spewer.spewPass(pass);
    jsonSpewer.beginPass(pass);
    jsonSpewer.spewMIR(graph);
    jsonSpewer.spewLIR(graph);
    jsonSpewer.endPass();
}

void
IonSpewer::spewPass(const char *pass, LinearScanAllocator *ra)
{
    if (!inited_)
        return;

    c1Spewer.spewPass(pass);
    c1Spewer.spewIntervals(pass, ra);
    jsonSpewer.beginPass(pass);
    jsonSpewer.spewMIR(graph);
    jsonSpewer.spewLIR(graph);
    jsonSpewer.spewIntervals(ra);
    jsonSpewer.endPass();
}

void
IonSpewer::endFunction()
{
    if (!inited_)
        return;

    c1Spewer.endFunction();
    jsonSpewer.endFunction();
}


FILE *ion::IonSpewFile = NULL;

static bool
ContainsFlag(const char *str, const char *flag)
{
    size_t flaglen = strlen(flag);
    const char *index = strstr(str, flag);
    while (index) {
        if ((index == str || index[-1] == ' ') && (index[flaglen] == 0 || index[flaglen] == ' '))
            return true;
        index = strstr(index + flaglen, flag);
    }
    return false;
}

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
            "  gvn      Global Value Numbering\n"
            "  licm     Loop invariant code motion\n"
            "  lsra     Linear scan register allocation\n"

            "  all      Everything\n"
            "\n"
        );
        exit(0);
        
    }
    if (ContainsFlag(env, "aborts"))
        LoggingBits |= (1 << uint32(IonSpew_Abort));
    if (ContainsFlag(env, "mir"))
        LoggingBits |= (1 << uint32(IonSpew_MIR));
    if (ContainsFlag(env, "gvn"))
        LoggingBits |= (1 << uint32(IonSpew_GVN));
    if (ContainsFlag(env, "licm"))
        LoggingBits |= (1 << uint32(IonSpew_LICM));
    if (ContainsFlag(env, "regalloc"))
        LoggingBits |= (1 << uint32(IonSpew_RegAlloc));
    if (ContainsFlag(env, "snapshots"))
        LoggingBits |= (1 << uint32(IonSpew_Snapshots));
    if (ContainsFlag(env, "all"))
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

