








































#include <stdarg.h>
#include "C1Spewer.h"
#include "JSONSpewer.h"
#include "MIRGraph.h"

namespace js {
namespace ion {


#define IONSPEW_CHANNEL_LIST(_)             \
    /* Used to abort SSA construction */    \
    _(Abort)                                \
    /* Information during MIR building */   \
    _(MIR)                                  \
    /* Information during GVN */            \
    _(GVN)                                  \
    /* Information during LICM */           \
    _(LICM)

enum IonSpewChannel {
#define IONSPEW_CHANNEL(name) IonSpew_##name,
    IONSPEW_CHANNEL_LIST(IONSPEW_CHANNEL)
#undef IONSPEW_CHANNEL
    IonSpew_Terminator
};

#if defined(DEBUG) && !defined(JS_ION_SPEW)
# define JS_ION_SPEW
#endif

#if defined(JS_ION_SPEW)
void CheckLogging();
extern FILE *IonSpewFile;
void IonSpew(IonSpewChannel channel, const char *fmt, ...);
void IonSpewHeader(IonSpewChannel channel);
bool IonSpewEnabled(IonSpewChannel channel);
void IonSpewVA(IonSpewChannel channel, const char *fmt, va_list ap);
#else
static inline void CheckLogging()
{ }
static FILE *const IonSpewFile = NULL;
static inline void IonSpew(IonSpewChannel, const char *fmt, ...)
{ }
static inline void IonSpewHeader(IonSpewChannel channel)
{ }
static inline bool IonSpewEnabled(IonSpewChannel channel)
{ return false; }
static inline void IonSpewVA(IonSpewChannel, const char *fmt, va_list ap)
{ } 
#endif


class IonSpewer
{
  private:
    MIRGraph *graph;
    JSScript *function;
    C1Spewer c1Spewer;
    JSONSpewer jsonSpewer;

  public:
    IonSpewer(MIRGraph *graph, JSScript *function)
      : graph(graph),
        function(function),
        c1Spewer(*graph, function)
    { }

    bool init();
    void spewPass(const char *pass);
    void finish();

};



}
}
