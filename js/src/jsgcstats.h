





































#ifndef jsgcstats_h___
#define jsgcstats_h___

#if !defined JS_DUMP_CONSERVATIVE_GC_ROOTS && defined DEBUG
# define JS_DUMP_CONSERVATIVE_GC_ROOTS 1
#endif


#if defined JS_GCMETER
const bool JS_WANT_GC_METER_PRINT = true;
const bool JS_WANT_GC_PER_COMPARTMENT_PRINT = true;
const bool JS_WANT_CONSERVATIVE_GC_PRINT = true;
#elif defined DEBUG
# define JS_GCMETER 1
const bool JS_WANT_GC_METER_PRINT = false;
const bool JS_WANT_GC_PER_COMPARTMENT_PRINT = false;
const bool JS_WANT_CONSERVATIVE_GC_PRINT = false;
#endif

#ifdef JSGC_TESTPILOT
JS_BEGIN_EXTERN_C

struct JSGCInfo
{
    double appTime, gcTime, waitTime, markTime, sweepTime;
    double sweepObjTime, sweepStringTime, sweepShapeTime, destroyTime, endTime;
    bool isCompartmental;
};

extern JS_PUBLIC_API(void)
JS_SetGCInfoEnabled(JSRuntime *rt, bool enabled);

extern JS_PUBLIC_API(bool)
JS_GetGCInfoEnabled(JSRuntime *rt);

















extern JS_PUBLIC_API(JSGCInfo *)
JS_GCInfoFront(JSRuntime *rt);


extern JS_PUBLIC_API(bool)
JS_GCInfoPopFront(JSRuntime *rt);

JS_END_EXTERN_C
#endif

namespace js {
namespace gc {




enum ConservativeGCTest
{
    CGCT_VALID,
    CGCT_LOWBITSET, 
    CGCT_NOTARENA,  
    CGCT_NOTCHUNK,  
    CGCT_FREEARENA, 
    CGCT_WRONGTAG,  
    CGCT_NOTLIVE,   
    CGCT_END
};

struct ConservativeGCStats
{
    uint32  counter[gc::CGCT_END];  

    uint32  unaligned;              
 

    void add(const ConservativeGCStats &another) {
        for (size_t i = 0; i != JS_ARRAY_LENGTH(counter); ++i)
            counter[i] += another.counter[i];
    }

    void dump(FILE *fp);
};

#ifdef JS_GCMETER
struct JSGCArenaStats
{
    uint32  alloc;          
    uint32  localalloc;     
    uint32  nthings;        
    uint32  maxthings;      
    double  totalthings;    
    uint32  narenas;        
    uint32  newarenas;      
    uint32  livearenas;     
    uint32  maxarenas;      
    uint32  totalarenas;    

};
#endif

#ifdef JS_GCMETER

struct JSGCStats
{
    uint32  lock;       
    uint32  unlock;     
    uint32  unmarked;   

    uint32  lastditch;  
    uint32  fail;       
#ifdef DEBUG
    uint32  maxunmarked;

#endif
    uint32  afree;          
    uint32  nallarenas;     
    uint32  maxnallarenas;  
    uint32  nchunks;        
    uint32  maxnchunks;     

    ConservativeGCStats conservative;
};

extern void
UpdateCompartmentGCStats(JSCompartment *comp, unsigned thingKind);

extern void
UpdateAllCompartmentGCStats(JSCompartment *comp);
#endif 

} 

#if defined(MOZ_GCTIMER) || defined(JSGC_TESTPILOT)

const bool JS_WANT_GC_SUITE_PRINT = false;  

extern jsrefcount newChunkCount;
extern jsrefcount destroyChunkCount;

struct GCTimer
{
    JSRuntime *rt;

    uint64 enter;
    uint64 startMark;
    uint64 startSweep;
    uint64 sweepObjectEnd;
    uint64 sweepStringEnd;
    uint64 sweepShapeEnd;
    uint64 sweepDestroyEnd;
    uint64 end;

    bool isCompartmental;
    bool enabled; 

    GCTimer(JSRuntime *rt, JSCompartment *comp);

    uint64 getFirstEnter();

    void clearTimestamps() {
        memset(&enter, 0, &end - &enter + sizeof(end));
    }

    void finish(bool lastGC);

    enum JSGCReason {
        PUBLIC_API,
        MAYBEGC,
        LASTCONTEXT,
        DESTROYCONTEXT,
        COMPARTMENT,
        LASTDITCH,
        TOOMUCHMALLOC,
        ALLOCTRIGGER,
        CHUNK,
        SHAPE,
        NOREASON
    };
};


extern volatile GCTimer::JSGCReason gcReason;

#define GCREASON(x) ((gcReason == GCTimer::NOREASON) ? gcReason = GCTimer::x : gcReason = gcReason)

# define GCTIMER_PARAM              , GCTimer &gcTimer
# define GCTIMER_ARG                , gcTimer
# define GCTIMESTAMP(stamp_name_) \
    JS_BEGIN_MACRO \
        if (gcTimer.enabled) \
            gcTimer.stamp_name_ = PRMJ_Now(); \
    JS_END_MACRO
# define GCTIMER_BEGIN(rt, comp)    GCTimer gcTimer(rt, comp)
# define GCTIMER_END(last)          (gcTimer.finish(last))
#else
# define GCREASON(x)                ((void) 0)
# define GCTIMER_PARAM
# define GCTIMER_ARG
# define GCTIMESTAMP(x)             ((void) 0)
# define GCTIMER_BEGIN(rt, comp)    ((void) 0)
# define GCTIMER_END(last)          ((void) 0)
#endif

} 

extern JS_FRIEND_API(void)
js_DumpGCStats(JSRuntime *rt, FILE *fp);

#endif 
