





































#define jstracer_cpp___

#include "nanojit/avmplus.h"
#include "nanojit/nanojit.h"

using namespace nanojit;

#include "jsinterp.cpp"

template <typename T>
Tracker<T>::Tracker()
{
    pagelist = 0;
}

template <typename T>
Tracker<T>::~Tracker()
{
    clear();
}

template <typename T> long
Tracker<T>::getPageBase(const void* v) const
{
    return ((long)v) & (~(NJ_PAGE_SIZE-1));
}

template <typename T> struct Tracker<T>::Page*
Tracker<T>::findPage(const void* v) const
{
    long base = getPageBase(v);
    struct Tracker::Page* p = pagelist;
    while (p) {
        if (p->base == base)
            return p;
        p = p->next;
    }
    return 0;
}

template <typename T> struct Tracker<T>::Page*
Tracker<T>::addPage(const void* v) {
    long base = getPageBase(v);
    struct Tracker<T>::Page* p = (struct Tracker<T>::Page*)
        GC::Alloc(sizeof(struct Tracker<T>::Page) + (NJ_PAGE_SIZE >> 2) * sizeof(T));
    p->base = base;
    p->next = pagelist;
    pagelist = p;
    return p;
}

template <typename T> void
Tracker<T>::clear()
{
    while (pagelist) {
        Page* p = pagelist;
        pagelist = pagelist->next;
        GC::Free(p);
    }
}

template <typename T> T
Tracker<T>::get(const void* v) const
{
    struct Page* p = findPage(v);
    JS_ASSERT(p != 0); 
    T i = p->map[(((long)v) & 0xfff) >> 2];
    JS_ASSERT(i != 0);
    return i;
}

template <typename T> void
Tracker<T>::set(const void* v, T i)
{
    struct Tracker::Page* p = findPage(v);
    if (!p)
        p = addPage(v);
    p->map[(((long)v) & 0xfff) >> 2] = i;
}

using namespace avmplus;
using namespace nanojit;

static avmplus::AvmCore* core = new avmplus::AvmCore();
static GC gc = GC();

#define LO ARGSIZE_LO
#define F  ARGSIZE_F
#define Q  ARGSIZE_Q

#ifdef DEBUG
#define NAME(op) ,#op
#else
#define NAME(op)
#endif

#define BUILTIN1(op, at0, atr, tr, t0, cse, fold) \
    { 0, (at0 | (atr << 2)), cse, fold NAME(op) },
#define BUILTIN2(op, at0, at1, atr, tr, t0, t1, cse, fold) \
    { 0, (at0 | (at1 << 2) | (atr << 4)), cse, fold NAME(op) },
#define BUILTIN3(op, at0, at1, at2, atr, tr, t0, t1, t2, cse, fold) \
    { 0, (at0 | (at1 << 2) | (at2 << 4) | (atr << 6)), cse, fold NAME(op) },

static struct CallInfo builtins[] = {
#include "builtins.tbl"
};

#undef NAME
#undef BUILTIN1
#undef BUILTIN2
#undef BUILTIN3

bool
js_StartRecording(JSContext* cx, JSFrameRegs& regs)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    
    if (!tm->fragmento) {
        Fragmento* fragmento = new (&gc) Fragmento(core);
#ifdef DEBUG        
        fragmento->labels = new (&gc) LabelMap(core, NULL);
#endif        
        fragmento->assm()->setCallTable(builtins);
        tm->fragmento = fragmento;
    }

    JSTraceRecorder* recorder = new JSTraceRecorder(regs);
    tm->recorder = recorder;

    InterpState state;
    state.ip = NULL;
    state.sp = NULL;
    state.rp = NULL;
    state.f = NULL;

    Fragment* fragment = tm->fragmento->getLoop(state);
    LirBuffer* lirbuf = new (&gc) LirBuffer(tm->fragmento, builtins);
#ifdef DEBUG    
    lirbuf->names = new (&gc) LirNameMap(&gc, builtins, tm->fragmento->labels);
#endif    
    fragment->lirbuf = lirbuf;
    LirWriter* lir = new (&gc) LirBufWriter(lirbuf);
    lir->ins0(LIR_trace);
    fragment->param0 = lir->insImm8(LIR_param, Assembler::argRegs[0], 0);
    fragment->param1 = lir->insImm8(LIR_param, Assembler::argRegs[1], 0);
    JSStackFrame* fp = cx->fp;

    recorder->tracker.set(cx, lir->insLoadi(fragment->param0, 0));
    recorder->tracker.set(&recorder->entryState.sp, lir->insLoadi(fragment->param0, 4));

#define LOAD_CONTEXT(p)                                                       \
    recorder->tracker.set(p,                                                  \
        lir->insLoadi(recorder->tracker.get(&recorder->entryState.sp),        \
                      STACK_OFFSET(p)))

    unsigned n;
    for (n = 0; n < fp->argc; ++n)
        LOAD_CONTEXT(&fp->argv[n]);
    for (n = 0; n < fp->nvars; ++n) 
        LOAD_CONTEXT(&fp->vars[n]);
    for (n = 0; n < (unsigned)(regs.sp - fp->spbase); ++n)
        LOAD_CONTEXT(&fp->spbase[n]);

    recorder->fragment = fragment;
    recorder->lir = lir;

    return true;
}

void
js_EndRecording(JSContext* cx, JSFrameRegs& regs)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (tm->recorder != NULL) {
        JSTraceRecorder* recorder = tm->recorder;
        recorder->fragment->lastIns = recorder->lir->ins0(LIR_loop);
        compile(tm->fragmento->assm(), recorder->fragment);
        delete recorder;
        tm->recorder = NULL;
    }
}
