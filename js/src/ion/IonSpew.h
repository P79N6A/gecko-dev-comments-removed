








































#ifndef jsion_ion_spew_h__
#define jsion_ion_spew_h__

#include <stdarg.h>
#include "jscntxt.h"
#include "MIR.h"

namespace js {
namespace ion {


#define IONSPEW_CHANNEL_LIST(_)             \
    /* Used to abort SSA construction */    \
    _(Abort)                                \
    /* Information during MIR building */   \
    _(MIR)                                  \
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
void IonSpew(IonSpewChannel channel, const char *fmt, ...);
void IonSpewVA(IonSpewChannel channel, const char *fmt, va_list ap);
#else
static inline void IonSpew(IonSpewChannel, const char *fmt, ...)
{ }
static inline void IonSpewVA(IonSpewChannel, const char *fmt, va_list ap)
{ } 
#endif

void CheckLogging();

class C1Spewer
{
    MIRGraph &graph;
    JSScript *script;
    FILE *spewout_;

  public:
    C1Spewer(MIRGraph &graph, JSScript *script);
    ~C1Spewer();
    void enable(const char *path);
    void spew(const char *pass);

  private:
    void spew(FILE *fp, const char *pass);
    void spew(FILE *fp, MBasicBlock *block);
};

} 
} 

#endif 

