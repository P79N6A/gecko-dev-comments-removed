





































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
    
    long            getPageBase(const void* v) const;
    struct Page*    findPage(const void* v) const;
    struct Page*    addPage(const void* v);
public:    
    Tracker();
    ~Tracker();
    
    nanojit::LIns*  get(const void* v) const;
    void            set(const void* v, nanojit::LIns* ins);
    void            clear();
};





class FrameStack 
{
    JSStackFrame* stack[16];
    uint32 depth;
public:
    FrameStack(JSStackFrame& entryFrame);
    ~FrameStack();
    
    bool enter(JSStackFrame& frame);
    void leave();
    
    JSStackFrame* findFrame(void* p) const;
    bool contains(void* p) const;
    int  nativeOffset(void* p) const;
};

class TraceRecorder {
public:
    FrameStack              frameStack;
    struct JSFrameRegs      entryState;
    Tracker                 tracker;
    nanojit::Fragment*      fragment;
    nanojit::LirWriter*     lir;

    TraceRecorder(JSStackFrame& _stackFrame, JSFrameRegs& _entryState);
};











struct JSTraceMonitor {
    int                     freq;
    nanojit::Fragmento*     fragmento;
    TraceRecorder*        recorder;
};

#define ENABLE_TRACER      true
#define TRACE_TRIGGER_MASK 0x3f

extern bool
js_StartRecording(JSContext* cx, JSFrameRegs& regs);

extern void
js_EndRecording(JSContext* cx, JSFrameRegs& regs);

#endif 
