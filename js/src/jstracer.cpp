





































#define jstracer_cpp___

#include "nanojit/avmplus.h"
#include "nanojit/nanojit.h"

using namespace nanojit;

#include "jsinterp.cpp"

Tracker::Tracker()
{
    pagelist = 0;
}

Tracker::~Tracker()
{
    clear();
}

long
Tracker::getPageBase(const void* v) const
{
    return ((long)v) & (~(NJ_PAGE_SIZE-1));
}

struct Tracker::Page* 
Tracker::findPage(const void* v) const 
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

struct Tracker::Page*
Tracker::addPage(const void* v) {
    long base = getPageBase(v);
    struct Tracker::Page* p = (struct Tracker::Page*)
        GC::Alloc(sizeof(struct Tracker::Page) + (NJ_PAGE_SIZE >> 2));
    p->base = base;
    p->next = pagelist;
    pagelist = p;
    return p;
}

void
Tracker::clear()
{
    while (pagelist) {
        Page* p = pagelist;
        pagelist = pagelist->next;
        GC::Free(p);
    }
}

LIns* 
Tracker::get(const void* v) const 
{
    struct Tracker::Page* p = findPage(v);
    JS_ASSERT(p != 0); 
    return p->map[(((long)v) & 0xfff) >> 2];
}

void 
Tracker::set(const void* v, LIns* ins) 
{
    struct Tracker::Page* p = findPage(v);
    if (!p) 
        p = addPage(v);
    p->map[(((long)v) & 0xfff) >> 2] = ins;
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

#define BUILTIN1(op, at0, atr, tr, t0) \
    { 0, (at0 | (atr << 2)), 1/*cse*/, 1/*fold*/ NAME(op) },
#define BUILTIN2(op, at0, at1, atr, tr, t0, t1) \
    { 0, (at0 | (at1 << 2) | (atr << 4)), 1/*cse*/, 1/*fold*/ NAME(op) },
#define BUILTIN3(op, at0, at1, at2, atr, tr, t0, t1, t2) \
    { 0, (at0 | (at1 << 2) | (at2 << 4) | (atr << 6)), 1/*cse*/, 1/*fold*/ NAME(op) },

static struct CallInfo builtins[] = {
#include "builtins.tbl"
};        

#undef NAME    
#undef BUILTIN1
#undef BUILTIN2
#undef BUILTIN3    

void
js_StartRecorder(JSContext* cx, JSFrameRegs& regs)
{
    struct JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    if (!tm->fragmento) {
        Fragmento* fragmento = new (&gc) Fragmento(core);
        fragmento->labels = new (&gc) LabelMap(core, NULL);
        fragmento->assm()->setCallTable(builtins);
        tm->fragmento = fragmento;
    }   

    InterpState state;
    state.ip = NULL;
    state.sp = NULL;
    state.rp = NULL;
    state.f = NULL;

    Fragment* fragment = tm->fragmento->getLoop(state);
    LirBuffer* lirbuf = new (&gc) LirBuffer(tm->fragmento, builtins);
    lirbuf->names = new (&gc) LirNameMap(&gc, builtins, tm->fragmento->labels);
    fragment->lirbuf = lirbuf;
    LirWriter* lir = new (&gc) LirBufWriter(lirbuf);
    lir->ins0(LIR_trace);
    fragment->param0 = lir->insImm8(LIR_param, Assembler::argRegs[0], 0);
    fragment->param1 = lir->insImm8(LIR_param, Assembler::argRegs[1], 0);
    JSStackFrame* fp = cx->fp;

    tm->tracker.set(cx, fragment->param0);

#define STACK_OFFSET(p) (((char*)(p)) - ((char*)regs.sp))
#define LOAD_CONTEXT(p) tm->tracker.set(p, lir->insLoadi(fragment->param1, STACK_OFFSET(p)))    

    unsigned n;
    for (n = 0; n < fp->argc; ++n)
        LOAD_CONTEXT(&fp->argv[n]);
    for (n = 0; n < fp->nvars; ++n)
        LOAD_CONTEXT(&fp->vars[n]);
    for (n = 0; n < (unsigned)(regs.sp - fp->spbase); ++n)
        LOAD_CONTEXT(&fp->spbase[n]);
    
    tm->fragment = fragment;
    tm->lir = lir;
}

void
js_StopRecorder(JSContext* cx, JSFrameRegs& regs)
{
    struct JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    SideExit exit;
    memset(&exit, 0, sizeof(exit));
    exit.from = tm->fragment;
    tm->fragment->lastIns = tm->lir->insGuard(LIR_x, NULL, &exit);
    compile(tm->fragmento->assm(), tm->fragment);
}

bool
js_GetRecorderError(JSContext* cx)
{
    return false;
}


