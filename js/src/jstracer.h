






































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"

#include "nanojit/nanojit.h"





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
    uint32_t nativeFrameOffset(void* p) const;
    uint32_t nativeFrameSize() const;
};

class TraceRecorder {
    Tracker                 tracker;
    FrameStack              frameStack;
    struct JSFrameRegs      entryState;
    nanojit::Fragment*      fragment;
    nanojit::LirBuffer*     lirbuf;
    nanojit::LirWriter*     lir;

    nanojit::SideExit* snapshot(nanojit::SideExit& exit, JSFrameRegs& regs);
public:
    TraceRecorder(JSContext* cx, JSFrameRegs& regs, nanojit::Fragmento*);
    ~TraceRecorder();
    
    inline jsbytecode* entryPC() const
    {
        return entryState.pc;
    }
    
    void init(void* p, nanojit::LIns* l);
    void set(void* p, nanojit::LIns* l);
    nanojit::LIns* get(void* p);
    
    void readstack(void*);
    
    void copy(void* a, void* v);
    void imm(jsint i, void* v);
    void imm(jsdouble d, void* v);
    void unary(nanojit::LOpcode op, void* a, void* v);
    void binary(nanojit::LOpcode op, void* a, void* b, void* v);
    void binary0(nanojit::LOpcode op, void* a, void* v);
    void call(int id, void* a, void* v);
    void call(int id, void* a, void* b, void* v);
    void call(int id, void* a, void* b, void* c, void* v);
    
    void iinc(void* a, int32_t incr, void* v, JSFrameRegs& regs);

    void guard_0(bool expected, void* a, JSFrameRegs& regs);
    void guard_h(bool expected, void* a, JSFrameRegs& regs);
    void guard_ov(bool expected, void* a, JSFrameRegs& regs);
    void guard_eq(bool expected, void* a, void* b, JSFrameRegs& regs);
    void guard_eqi(bool expected, void* a, int i, JSFrameRegs& regs);
    
    void closeLoop(nanojit::Fragmento* fragmento);

    void load(void* a, int32_t i, void* v);
};











struct JSTraceMonitor {
    int                     freq;
    nanojit::Fragmento*     fragmento;
    TraceRecorder*        recorder;
};

#define TRACING_ENABLED(cx)       JS_HAS_OPTION(cx, JSOPTION_JIT)
#define TRACE_TRIGGER_MASK 0x3f

extern bool
js_StartRecording(JSContext* cx, JSFrameRegs& regs);

extern void
js_EndRecording(JSContext* cx, JSFrameRegs& regs);

#endif 
