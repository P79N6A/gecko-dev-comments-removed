





































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"

namespace nanojit {
    class LIns;
    class Fragmento;
    class Fragment;
    class LirWriter;
}





class Tracker 
{
    struct Page {
        struct Page* next;
        long base;
        nanojit::LIns* map[0];
    };
    struct Page* pagelist;
    
    long         getPageBase(const void* v) const;
    struct Page* findPage(const void* v) const;
    struct Page* addPage(const void* v);
public:    
    Tracker();
    ~Tracker();
    
    nanojit::LIns*  get(const void* v) const;
    void            set(const void* v, nanojit::LIns* ins);
    void            clear();
};

enum TraceRecorderStatus {
    IDLE, RECORDING, ABORTED
};











struct JSTraceMonitor {
    int                 freq;
    TraceRecorderStatus status;
    JSFrameRegs         entryState;
    Tracker             tracker;
    nanojit::Fragment*  fragment;
    nanojit::Fragmento* fragmento;
    nanojit::LirWriter* lir;
};

#define ENABLE_TRACER      true
#define TRACE_TRIGGER_MASK 0x3f

extern void
js_StartRecording(JSContext* cx, JSFrameRegs& regs);

extern void
js_EndRecording(JSContext* cx, JSFrameRegs& regs);

#endif 
