





































#ifndef jsperf_h___
#define jsperf_h___

#include "jsapi.h"

namespace JS {















class JS_FRIEND_API(PerfMeasurement)
{
  protected:
    
    void* impl;

  public:
    




    enum EventMask {
        CPU_CYCLES          = 0x00000001,
        INSTRUCTIONS        = 0x00000002,
        CACHE_REFERENCES    = 0x00000004,
        CACHE_MISSES        = 0x00000008,
        BRANCH_INSTRUCTIONS = 0x00000010,
        BRANCH_MISSES       = 0x00000020,
        BUS_CYCLES          = 0x00000040,
        PAGE_FAULTS         = 0x00000080,
        MAJOR_PAGE_FAULTS   = 0x00000100,
        CONTEXT_SWITCHES    = 0x00000200,
        CPU_MIGRATIONS      = 0x00000400,

        ALL                 = 0x000007ff,
        NUM_MEASURABLE_EVENTS  = 11
    };

    





    const EventMask eventsMeasured;

    





    uint64 cpu_cycles;
    uint64 instructions;
    uint64 cache_references;
    uint64 cache_misses;
    uint64 branch_instructions;
    uint64 branch_misses;
    uint64 bus_cycles;
    uint64 page_faults;
    uint64 major_page_faults;
    uint64 context_switches;
    uint64 cpu_migrations;

    





    PerfMeasurement(EventMask toMeasure);

    
    ~PerfMeasurement();

    
    void start();

    




    void stop();

    
    void reset();

    



    static bool canMeasureSomething();
};





extern JS_FRIEND_API(JSObject*)
    RegisterPerfMeasurement(JSContext *cx, JSObject *global);






extern JS_FRIEND_API(PerfMeasurement*)
    ExtractPerfMeasurement(jsval wrapper);

} 

#endif
