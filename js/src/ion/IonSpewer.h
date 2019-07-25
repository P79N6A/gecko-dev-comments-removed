








































#ifndef jsion_ion_spewer_h__
#define jsion_ion_spewer_h__

#include <stdarg.h>
#include "C1Spewer.h"
#include "JSONSpewer.h"
#include "LinearScan.h"
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
    _(LICM)                                 \
    /* Information during LSRA */           \
    _(LSRA)                                 \
    /* Debug info about snapshots */        \
    _(Snapshots)

enum IonSpewChannel {
#define IONSPEW_CHANNEL(name) IonSpew_##name,
    IONSPEW_CHANNEL_LIST(IONSPEW_CHANNEL)
#undef IONSPEW_CHANNEL
    IonSpew_Terminator
};





#ifdef DEBUG

class IonSpewer
{
  private:
    MIRGraph *graph;
    JSScript *function;
    C1Spewer c1Spewer;
    JSONSpewer jsonSpewer;
    bool inited_;

  public:
    IonSpewer()
      : graph(NULL), function(NULL), inited_(false)
    { }

    
    ~IonSpewer();

    bool init();
    void beginFunction(MIRGraph *graph, JSScript *);
    void spewPass(const char *pass);
    void spewPass(const char *pass, LinearScanAllocator *ra);
    void endFunction();
};

void IonSpewNewFunction(MIRGraph *graph, JSScript *function);
void IonSpewPass(const char *pass);
void IonSpewPass(const char *pass, LinearScanAllocator *ra);
void IonSpewEndFunction();

void CheckLogging();
extern FILE *IonSpewFile;
void IonSpew(IonSpewChannel channel, const char *fmt, ...);
void IonSpewHeader(IonSpewChannel channel);
bool IonSpewEnabled(IonSpewChannel channel);
void IonSpewVA(IonSpewChannel channel, const char *fmt, va_list ap);

#else

static inline void IonSpewNewFunction(MIRGraph *graph, JSScript *function)
{ }
static inline void IonSpewPass(const char *pass)
{ }
static inline void IonSpewPass(const char *pass, LinearScanAllocator *ra)
{ }
static inline void IonSpewEndFunction()
{ }

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

} 
} 

#endif 

