





































#ifndef jsgcstats_h___
#define jsgcstats_h___

#if !defined JS_DUMP_CONSERVATIVE_GC_ROOTS && defined DEBUG
# define JS_DUMP_CONSERVATIVE_GC_ROOTS 1
#endif


#if defined JS_GCMETER
const bool JS_WANT_GC_METER_PRINT = true;
#elif defined DEBUG
# define JS_GCMETER 1
const bool JS_WANT_GC_METER_PRINT = false;
#endif


#if defined MOZ_GCTIMER
const bool JS_WANT_GC_TIMER_PRINT = true;
#elif defined DEBUG
# define MOZ_GCTIMER 1
const bool JS_WANT_GC_TIMER_PRINT = false;
#endif

#define METER_UPDATE_MAX(maxLval, rval)                                       \
    METER_IF((maxLval) < (rval), (maxLval) = (rval))

namespace js {





enum ConservativeGCTest {
    CGCT_VALID,
    CGCT_LOWBITSET, 
    CGCT_NOTARENA,  
    CGCT_NOTCHUNK,  
    CGCT_FREEARENA, 
    CGCT_WRONGTAG,  
    CGCT_NOTLIVE,   
    CGCT_END
};

struct ConservativeGCStats {
    uint32  counter[CGCT_END];  


    void add(const ConservativeGCStats &another) {
        for (size_t i = 0; i != JS_ARRAY_LENGTH(counter); ++i)
            counter[i] += another.counter[i];
    }

    void dump(FILE *fp);
};

} 

#ifdef JS_GCMETER

struct JSGCArenaStats {
    uint32  alloc;          
    uint32  localalloc;     
    uint32  retry;          
    uint32  fail;           
    uint32  nthings;        
    uint32  maxthings;      
    double  totalthings;    
    uint32  narenas;        
    uint32  newarenas;      
    uint32  livearenas;     
    uint32  maxarenas;      
    uint32  totalarenas;    

};

struct JSGCStats {
    uint32  lock;       
    uint32  unlock;     
    uint32  unmarked;   

#ifdef DEBUG
    uint32  maxunmarked;

#endif
    uint32  poke;           
    uint32  afree;          
    uint32  nallarenas;     
    uint32  maxnallarenas;  
    uint32  nchunks;        
    uint32  maxnchunks;     

    js::ConservativeGCStats conservative;
};

extern JS_FRIEND_API(void)
js_DumpGCStats(JSRuntime *rt, FILE *fp);

extern void
UpdateArenaStats(JSGCArenaStats *st, uint32 nlivearenas, uint32 nkilledArenas,
                 uint32 nthings);

#endif 

namespace js {

#ifdef MOZ_GCTIMER

extern jsrefcount newChunkCount;
extern jsrefcount destroyChunkCount;

const bool JS_WANT_GC_SUITE_PRINT = false;  

struct GCTimer {
    uint64 enter;
    uint64 startMark;
    uint64 startSweep;
    uint64 sweepObjectEnd;
    uint64 sweepStringEnd;
    uint64 sweepDestroyEnd;
    uint64 end;

    GCTimer();
    static uint64 getFirstEnter();
    void finish(bool lastGC);
};

# define GCTIMER_PARAM      , GCTimer &gcTimer
# define GCTIMER_ARG        , gcTimer
# define TIMESTAMP(x)       (gcTimer.x = rdtsc())
# define GCTIMER_BEGIN()    GCTimer gcTimer
# define GCTIMER_END(last)  (gcTimer.finish(last))
#else
# define GCTIMER_PARAM
# define GCTIMER_ARG
# define TIMESTAMP(x)       ((void) 0)
# define GCTIMER_BEGIN()    ((void) 0)
# define GCTIMER_END(last)  ((void) 0)
#endif

#ifdef JS_SCOPE_DEPTH_METER
extern void
DumpScopeDepthMeter(JSRuntime *rt);
#endif

#ifdef JS_DUMP_LOOP_STATS
extern void
DumpLoopStats(JSRuntime *rt);
#endif

} 

#endif 
