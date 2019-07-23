






































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
        GC::Alloc(sizeof(struct Tracker::Page) + (NJ_PAGE_SIZE >> 2) * sizeof(LInsp));
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
    LIns* i = p->map[(((long)v) & 0xfff) >> 2];
    JS_ASSERT(i != 0);
    return i;
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

TraceRecorder::TraceRecorder(JSContext* cx, Fragmento* fragmento) 
{
    entryFrame = currentFrame = cx->fp;
    entryRegs = *(entryFrame->regs);
    
    InterpState state;
    state.ip = (FOpcodep)entryFrame->regs->pc;
    state.sp = entryFrame->regs->sp;
    state.rp = NULL;
    state.f = NULL;
        
    fragment = fragmento->getLoop(state);
    lirbuf = new (&gc) LirBuffer(fragmento, builtins);
#ifdef DEBUG    
    lirbuf->names = new (&gc) LirNameMap(&gc, builtins, fragmento->labels);
#endif    
    fragment->lirbuf = lirbuf;
    lir = new (&gc) LirBufWriter(lirbuf);
    lir->ins0(LIR_trace);
    fragment->param0 = lir->insImm8(LIR_param, Assembler::argRegs[0], 0);
    fragment->param1 = lir->insImm8(LIR_param, Assembler::argRegs[1], 0);
    
    init(&cx, lir->insLoadi(fragment->param0, 0));
    init(&entryRegs.sp, lir->insLoadi(fragment->param0, 4));

    JSStackFrame* fp = cx->fp;
    unsigned n;
    for (n = 0; n < fp->argc; ++n)
        readstack(&fp->argv[n]);
    for (n = 0; n < fp->nvars; ++n) 
        readstack(&fp->vars[n]);
    for (n = 0; n < (unsigned)(fp->regs->sp - fp->spbase); ++n)
        readstack(&fp->spbase[n]);
}

TraceRecorder::~TraceRecorder()
{
    delete lir;
#ifdef DEBUG    
    delete lirbuf->names;
#endif
    delete lirbuf;
    delete fragment;
}


unsigned
TraceRecorder::calldepth() const
{
    JSStackFrame* fp = currentFrame;
    unsigned depth = 0;
    while (fp != entryFrame) {
        ++depth;
        fp = fp->down;
    }
    return depth;
}


JSStackFrame*
TraceRecorder::findFrame(void* p) const
{
    JSStackFrame* fp = currentFrame;
    while (1) {
        if ((p >= &fp->argv[0] && p < &fp->argv[fp->argc]) ||
            (p >= &fp->vars[0] && p < &fp->vars[fp->nvars]) ||
            (p >= &fp->spbase[0] && p < &fp->spbase[fp->script->depth]))
            return fp;
        if (fp == entryFrame)
            return NULL;
        fp = fp->down;
    }
}


bool
TraceRecorder::onFrame(void* p) const
{
    return findFrame(p) != NULL;
}

unsigned
TraceRecorder::nativeFrameSize(JSStackFrame* fp) const
{
    unsigned size;
    while (1) {
        size += fp->argc + fp->nvars + (fp->regs->sp - fp->spbase);
        if (fp == entryFrame)
            return size;
        fp = fp->down;
    } 
}

unsigned
TraceRecorder::nativeFrameSize() const
{
    return nativeFrameSize(currentFrame);
}



unsigned
TraceRecorder::nativeFrameOffset(void* p) const
{
    JSStackFrame* fp = findFrame(p);
    JS_ASSERT(fp != NULL); 
    unsigned offset;
    if (p >= &fp->argv[0] && p < &fp->argv[fp->argc])
        offset = unsigned((jsval*)p - &fp->argv[0]);
    if (p >= &fp->vars[0] && p < &fp->vars[fp->nvars])
        offset = (fp->argc + unsigned((jsval*)p - &fp->vars[0]));
    else {
        JS_ASSERT((p >= &fp->spbase[0] && p < &fp->spbase[fp->script->depth]));
        offset = (fp->argc + fp->nvars + unsigned((jsval*)p - &fp->spbase[0]));
    }
    if (fp != entryFrame) 
        offset += nativeFrameSize(fp->down);
    return offset;
}


void 
TraceRecorder::readstack(void* p)
{
    JS_ASSERT(onFrame(p));
    tracker.set(p, lir->insLoadi(tracker.get(&entryRegs.sp), 
            nativeFrameOffset(p)));
}

void 
TraceRecorder::init(void* p, LIns* i)
{
    tracker.set(p, i);
}

void 
TraceRecorder::set(void* p, LIns* i)
{
    init(p, i);
    if (onFrame(p))
        lir->insStorei(i, get(&entryRegs.sp), 
                nativeFrameOffset(p));
}

LIns* 
TraceRecorder::get(void* p)
{
    return tracker.get(p);
}

void 
TraceRecorder::copy(void* a, void* v)
{
    set(v, get(a));
}

void
TraceRecorder::imm(jsint i, void* v)
{
    set(v, lir->insImm(i));
}

void 
TraceRecorder::imm(jsdouble d, void* v)
{
    set(v, lir->insImmq(*(uint64_t*)&d));
}

void 
TraceRecorder::unary(LOpcode op, void* a, void* v)
{
    set(v, lir->ins1(op, get(a)));
}

void 
TraceRecorder::binary(LOpcode op, void* a, void* b, void* v)
{
    set(v, lir->ins2(op, get(a), get(b)));
}

void
TraceRecorder::binary0(LOpcode op, void* a, void* v)
{
    set(v, lir->ins2i(op, get(a), 0)); 
}

void 
TraceRecorder::call(int id, void* a, void* v)
{
    LInsp args[] = { get(a) };
    set(v, lir->insCall(id, args));
}

void 
TraceRecorder::call(int id, void* a, void* b, void* v)
{
    LInsp args[] = { get(a), get(b) };
    set(v, lir->insCall(id, args));
}

void 
TraceRecorder::call(int id, void* a, void* b, void* c, void* v)
{
    LInsp args[] = { get(a), get(b), get(c) };
    set(v, lir->insCall(id, args));
}

void
TraceRecorder::iinc(void* a, int incr, void* v)
{
    LIns* ov = lir->ins2(LIR_add, get(a), lir->insImm(incr));
    
    
    
    
    
    
    
    guard_ov(false, a); 
    set(v, ov);
}

SideExit*
TraceRecorder::snapshot(SideExit& exit)
{
    memset(&exit, 0, sizeof(exit));
    exit.from = fragment;
    exit.calldepth = calldepth();
    exit.sp_adj = ((char*)currentFrame->regs->sp) - ((char*)entryRegs.sp);
    exit.ip_adj = ((char*)currentFrame->regs->pc) - ((char*)entryRegs.pc);
    return &exit;
}

void
TraceRecorder::guard_0(bool expected, void* a)
{
    SideExit exit;
    lir->insGuard(expected ? LIR_xf : LIR_xt, 
            get(a), 
            snapshot(exit));
}

void
TraceRecorder::guard_h(bool expected, void* a)
{
    SideExit exit;
    lir->insGuard(expected ? LIR_xf : LIR_xt, 
            lir->ins1(LIR_callh, get(a)), 
            snapshot(exit));
}

void
TraceRecorder::guard_ov(bool expected, void* a)
{
#if 0    
    SideExit exit;
    lir->insGuard(expected ? LIR_xf : LIR_xt, 
            lir->ins1(LIR_ov, get(a)), 
            snapshot(exit));
#endif    
}

void
TraceRecorder::guard_eq(bool expected, void* a, void* b)
{
    SideExit exit;
    lir->insGuard(expected ? LIR_xf : LIR_xt,
                  lir->ins2(LIR_eq, get(a), get(b)),
                  snapshot(exit));
}

void
TraceRecorder::guard_eqi(bool expected, void* a, int i)
{
    SideExit exit;
    lir->insGuard(expected ? LIR_xf : LIR_xt,
                  lir->ins2i(LIR_eq, get(a), i),
                  snapshot(exit));
}

void
TraceRecorder::closeLoop(Fragmento* fragmento)
{
    fragment->lastIns = lir->ins0(LIR_loop);
    compile(fragmento->assm(), fragment);
}

void
TraceRecorder::load(void* a, int32_t i, void* v)
{
    set(v, lir->insLoadi(get(a), i));
}

bool
js_StartRecording(JSContext* cx)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    
    if (!tm->fragmento) {
        Fragmento* fragmento = new Fragmento(core);
#ifdef DEBUG        
        fragmento->labels = new LabelMap(core, NULL);
#endif        
        fragmento->assm()->setCallTable(builtins);
        tm->fragmento = fragmento;
    }

    tm->recorder = new TraceRecorder(cx, tm->fragmento);

    return true;
}

void
js_EndRecording(JSContext* cx)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (tm->recorder != NULL) {
        tm->recorder->closeLoop(tm->fragmento);
        delete tm->recorder;
        tm->recorder = NULL;
    }
}
