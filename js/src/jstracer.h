





































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





template <typename T>
class Tracker 
{
    struct Page {
        struct Page* next;
        long base;
        T map[0];
    };
    struct Page* pagelist;
    
    long         getPageBase(const void* v) const;
    struct Page* findPage(const void* v) const;
    struct Page* addPage(const void* v);
public:    
    Tracker();
    ~Tracker();
    
    T            get(const void* v) const;
    void         set(const void* v, T i);
    void         clear();
};

class JSTraceRecorder {
public:
    struct JSFrameRegs      entryState;
    Tracker<nanojit::LIns*> tracker;
    nanojit::Fragment*      fragment;
    nanojit::LirWriter*     lir;

    JSTraceRecorder(JSFrameRegs& _entryState) {
        entryState = _entryState;
    }
};











struct JSTraceMonitor {
    int                     freq;
    nanojit::Fragmento*     fragmento;
    JSTraceRecorder*        recorder;
};

#define ENABLE_TRACER      true
#define TRACE_TRIGGER_MASK 0x3f

extern bool
js_StartRecording(JSContext* cx, JSFrameRegs& regs);

extern void
js_EndRecording(JSContext* cx, JSFrameRegs& regs);

#endif 
