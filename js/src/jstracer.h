






































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

class TraceRecorder {
    JSContext*              cx;
    Tracker                 tracker;
    struct JSStackFrame*    entryFrame;
    struct JSFrameRegs      entryRegs;
    nanojit::Fragment*      fragment;
    nanojit::LirBuffer*     lirbuf;
    nanojit::LirWriter*     lir;

    nanojit::SideExit* snapshot(nanojit::SideExit& exit);
    unsigned nativeFrameSize(JSStackFrame* fp) const;
    void buildTypeMap(JSStackFrame* fp, char* m) const;
public:
    TraceRecorder(JSContext* cx, nanojit::Fragmento*);
    ~TraceRecorder();
    
    inline jsbytecode* entryPC() const
    {
        return entryRegs.pc;
    }
    
    unsigned calldepth() const;
    JSStackFrame* findFrame(void* p) const;
    bool TraceRecorder::onFrame(void* p) const;
    unsigned nativeFrameSize() const;
    unsigned nativeFrameOffset(void* p) const;
    void buildTypeMap(char* m) const;
    
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
    
    void iinc(void* a, int32_t incr, void* v);

    void load(void* a, int32_t i, void* v);

    void guard_0(bool expected, void* a);
    void guard_h(bool expected, void* a);
    void guard_ov(bool expected, void* a);
    void guard_eq(bool expected, void* a, void* b);
    void guard_eqi(bool expected, void* a, int i);
    
    void closeLoop(nanojit::Fragmento* fragmento);
};











struct JSTraceMonitor {
    int                     freq;
    nanojit::Fragmento*     fragmento;
    TraceRecorder*        recorder;
};

#define TRACING_ENABLED(cx)       JS_HAS_OPTION(cx, JSOPTION_JIT)
#define TRACE_TRIGGER_MASK 0x3f

extern bool
js_StartRecording(JSContext* cx);

extern void
js_AbortRecording(JSContext* cx);

extern void
js_EndRecording(JSContext* cx);

#endif 
