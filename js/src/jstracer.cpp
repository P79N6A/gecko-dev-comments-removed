






































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
        if (p->base == base) {
            return p;
        }
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

static GC gc = GC();
static avmplus::AvmCore* core = new (&gc) avmplus::AvmCore();

#define LO ARGSIZE_LO
#define F  ARGSIZE_F
#define Q  ARGSIZE_Q

#ifdef DEBUG
#define NAME(op) ,#op
#else
#define NAME(op)
#endif

void builtin_unimplemented(void) {
    JS_ASSERT(0);
}

#define builtin_DOUBLE_IS_INT builtin_unimplemented
#define builtin_StringToDouble builtin_unimplemented
#define builtin_ObjectToDouble builtin_unimplemented
#define builtin_ValueToBoolean builtin_unimplemented

jsint builtin_DoubleToECMAInt32(jsdouble d) {
    return js_DoubleToECMAInt32(d);
}

#define BUILTIN1(op, at0, atr, tr, t0, cse, fold) \
    { (intptr_t)&builtin_##op, (at0 << 2) | atr, cse, fold NAME(op) },
#define BUILTIN2(op, at0, at1, atr, tr, t0, t1, cse, fold) \
    { 0, (at0 << 4) | (at1 << 2) | atr, cse, fold NAME(op) },
#define BUILTIN3(op, at0, at1, at2, atr, tr, t0, t1, t2, cse, fold) \
    { 0, (at0 << 6) | (at1 << 4) | (at2 << 2) | atr, cse, fold NAME(op) },

static struct CallInfo builtins[] = {
#include "builtins.tbl"
};

#undef NAME
#undef BUILTIN1
#undef BUILTIN2
#undef BUILTIN3

TraceRecorder::TraceRecorder(JSContext* cx, Fragmento* fragmento) 
{
    this->cx = cx;
    entryFrame = cx->fp;
    entryRegs = *(entryFrame->regs);
    
    InterpState state;
    memset(&state, 0, sizeof(state));
    state.ip = (FOpcodep)entryFrame->regs->pc;
    fragment = fragmento->getLoop(state);

    lirbuf = new (&gc) LirBuffer(fragmento, builtins);
    fragment->lirbuf = lirbuf;
    lir = lir_buf_writer = new (&gc) LirBufWriter(lirbuf);
#ifdef DEBUG    
    lirbuf->names = new (&gc) LirNameMap(&gc, builtins, fragmento->labels);
    lir = verbose_filter = new (&gc) VerboseWriter(&gc, lir, lirbuf->names);
#endif    
    lir = cse_filter = new (&gc) CseFilter(lir, &gc);
    lir = expr_filter = new (&gc) ExprFilter(lir);
    lir->ins0(LIR_trace);
    
    entryNativeFrameSlots = nativeFrameSlots(entryFrame, entryRegs);
    LIns* data = lir_buf_writer->skip(entryNativeFrameSlots);
    buildTypeMap(entryFrame, entryRegs, 
            entryTypeMap = (char*)(fragment->vmprivate = data->payload()));
    fragment->param0 = lir->insImm8(LIR_param, Assembler::argRegs[0], 0);
    fragment->param1 = lir->insImm8(LIR_param, Assembler::argRegs[1], 0);
    fragment->sp = lir->insLoadi(fragment->param0, offsetof(InterpState, sp));
    cx_ins = lir->insLoadi(fragment->param0, offsetof(InterpState, f));
#ifdef DEBUG
    lirbuf->names->addName(fragment->param0, "state");
    lirbuf->names->addName(fragment->sp, "sp");
    lirbuf->names->addName(cx_ins, "cx");
#endif
    
    JSStackFrame* fp = cx->fp;
    unsigned n;
    for (n = 0; n < fp->argc; ++n)
        import(&fp->argv[n], "arg", n);
    for (n = 0; n < fp->nvars; ++n) 
        import(&fp->vars[n], "var", n);
    for (n = 0; n < (unsigned)(fp->regs->sp - fp->spbase); ++n)
        import(&fp->spbase[n], "stack", n);
}

TraceRecorder::~TraceRecorder()
{
#ifdef DEBUG
    delete lirbuf->names;
    delete verbose_filter;
#endif    
    delete cse_filter;
    delete expr_filter;
    delete lir_buf_writer;
}


unsigned
TraceRecorder::calldepth() const
{
    JSStackFrame* fp = cx->fp;
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
    JSStackFrame* fp = cx->fp;
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
TraceRecorder::nativeFrameSlots(JSStackFrame* fp, JSFrameRegs& regs) const
{
    unsigned size = 0;
    while (1) {
        size += fp->argc + fp->nvars + (regs.sp - fp->spbase);
        if (fp == entryFrame)
            return size;
        fp = fp->down;
    } 
}



unsigned
TraceRecorder::nativeFrameOffset(void* p) const
{
    JSStackFrame* fp = findFrame(p);
    JS_ASSERT(fp != NULL); 
    unsigned offset = 0;
    if (p >= &fp->argv[0] && p < &fp->argv[fp->argc])
        offset = unsigned((jsval*)p - &fp->argv[0]);
    else if (p >= &fp->vars[0] && p < &fp->vars[fp->nvars])
        offset = (fp->argc + unsigned((jsval*)p - &fp->vars[0]));
    else {
        JS_ASSERT((p >= &fp->spbase[0] && p < &fp->spbase[fp->script->depth]));
        offset = (fp->argc + fp->nvars + unsigned((jsval*)p - &fp->spbase[0]));
    }
    if (fp != entryFrame) 
        offset += nativeFrameSlots(fp->down, *fp->regs);
    return offset * sizeof(double);
}



static inline int gettag(jsval v)
{
    if (JSVAL_IS_INT(v))
        return JSVAL_INT;
    return JSVAL_TAG(v);
}   



void
TraceRecorder::buildTypeMap(JSStackFrame* fp, JSFrameRegs& regs, char* m) const
{
    if (fp != entryFrame)
        buildTypeMap(fp->down, *fp->down->regs, m);
    for (unsigned n = 0; n < fp->argc; ++n)
        *m++ = gettag(fp->argv[n]);
    for (unsigned n = 0; n < fp->nvars; ++n)
        *m++ = gettag(fp->vars[n]);
    for (jsval* sp = fp->spbase; sp < regs.sp; ++sp)
        *m++ = gettag(*sp);
}


bool
TraceRecorder::verifyTypeStability(JSStackFrame* fp, JSFrameRegs& regs, char* m) const
{
    if (fp != entryFrame)
        verifyTypeStability(fp->down, *fp->down->regs, m);
    for (unsigned n = 0; n < fp->argc; ++n)
        if (*m++ != gettag(fp->argv[n]))
            return false;
    for (unsigned n = 0; n < fp->nvars; ++n)
        if (*m++ != gettag(fp->vars[n]))
            return false;
    for (jsval* sp = fp->spbase; sp < regs.sp; ++sp)
        if (*m++ != gettag(*sp))
            return false;
    return true;
}



bool
TraceRecorder::unbox_jsval(jsval v, int t, double* slot) const
{
    if (t != gettag(v))
        return false;
    switch (t) {
    case JSVAL_BOOLEAN:
        *(bool*)slot = JSVAL_TO_BOOLEAN(v);
        break;
    case JSVAL_INT:
        *(jsint*)slot = JSVAL_TO_INT(v);
        break;
    case JSVAL_DOUBLE:
        *(jsdouble*)slot = *JSVAL_TO_DOUBLE(v);
        break;
    case JSVAL_STRING:
        *(JSString**)slot = JSVAL_TO_STRING(v);
        break;
    default:
        JS_ASSERT(JSVAL_IS_GCTHING(v));
        *(void**)slot = JSVAL_TO_GCTHING(v);
    }
    return true;
}




bool 
TraceRecorder::box_jsval(jsval* vp, int t, double* slot) const
{
    switch (t) {
    case JSVAL_BOOLEAN:
        *vp = BOOLEAN_TO_JSVAL(*(bool*)slot);
        break;
    case JSVAL_INT:
        jsint i = *(jsint*)slot;
        JS_ASSERT(INT_FITS_IN_JSVAL(i)); 
        *vp = INT_TO_JSVAL(i);
        break;
    case JSVAL_DOUBLE:
        return js_NewDoubleInRootedValue(cx, *slot, vp);
    case JSVAL_STRING:
        *vp = STRING_TO_JSVAL(*(JSString**)slot);
        break;
    default:
        JS_ASSERT(t == JSVAL_OBJECT);
        *vp = OBJECT_TO_JSVAL(*(JSObject**)slot);
        break;
    }
    return true;
}



bool
TraceRecorder::unbox(JSStackFrame* fp, JSFrameRegs& regs, char* m, double* native) const
{
    jsval* vp;
    for (vp = fp->argv; vp < fp->argv + fp->argc; ++vp)
        if (!unbox_jsval(*vp, (JSType)*m++, native++))
            return false;
    for (vp = fp->vars; vp < fp->vars + fp->nvars; ++vp)
        if (!unbox_jsval(*vp, (JSType)*m++, native++))
            return false;
    for (vp = fp->spbase; vp < regs.sp; ++vp)
        if (!unbox_jsval(*vp, (JSType)*m++, native++))
            return false;
    return true;
}



bool
TraceRecorder::box(JSStackFrame* fp, JSFrameRegs& regs, char* m, double* native) const
{
    jsval* vp;
    for (vp = fp->argv; vp < fp->argv + fp->argc; ++vp)
        if (!box_jsval(vp, (JSType)*m++, native++))
            return false;
    for (vp = fp->vars; vp < fp->vars + fp->nvars; ++vp)
        if (!box_jsval(vp, (JSType)*m++, native++))
            return false;
    for (vp = fp->spbase; vp < regs.sp; ++vp)
        if (!box_jsval(vp, (JSType)*m++, native++))
            return false;
    return true;
}


void 
TraceRecorder::import(jsval* p, char *prefix, int index)
{
    JS_ASSERT(onFrame(p));
    LIns *ins = lir->insLoad(JSVAL_IS_DOUBLE(*p) ? LIR_ldq : LIR_ld,
            fragment->sp, nativeFrameOffset(p));
    tracker.set(p, ins);
#ifdef DEBUG
    if (prefix) {
        char name[16];
        JS_ASSERT(strlen(prefix) < 10);
        JS_snprintf(name, sizeof name, "$%s%d", prefix, index);
        lirbuf->names->addName(ins, name);
    }
#endif

}



void 
TraceRecorder::set(void* p, LIns* i)
{
    tracker.set(p, i);
    if (onFrame(p)) 
        lir->insStorei(i, fragment->sp, nativeFrameOffset(p));
}

LIns* 
TraceRecorder::get(void* p)
{
    if (p == cx)
        return cx_ins;
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



void 
TraceRecorder::mark()
{
    markRegs = *cx->fp->regs;
}




void 
TraceRecorder::recover()
{
    *cx->fp->regs = markRegs;
}

SideExit*
TraceRecorder::snapshot()
{
    
    LIns* data = lir_buf_writer->skip(nativeFrameSlots(cx->fp, markRegs));
    buildTypeMap(cx->fp, markRegs, (char*)data->payload());
    
    
    memset(&exit, 0, sizeof(exit));
#ifdef DEBUG    
    exit.from = fragment;
#endif    
    exit.calldepth = calldepth();
    exit.sp_adj = (markRegs.sp - entryRegs.sp + 1) * sizeof(double);
    exit.ip_adj = markRegs.pc - entryRegs.pc;
    exit.vmprivate = data->payload();
    return &exit;
}

void
TraceRecorder::guard_0(bool expected, void* a)
{
    lir->insGuard(expected ? LIR_xf : LIR_xt, 
            get(a), 
            snapshot());
}

void
TraceRecorder::guard_h(bool expected, void* a)
{
    lir->insGuard(expected ? LIR_xf : LIR_xt, 
            lir->ins1(LIR_callh, get(a)), 
            snapshot());
}

void
TraceRecorder::guard_ov(bool expected, void* a)
{
    lir->insGuard(expected ? LIR_xf : LIR_xt, 
            lir->ins1(LIR_ov, get(a)), 
            snapshot());
}

void
TraceRecorder::guard_eq(bool expected, void* a, void* b)
{
    lir->insGuard(expected ? LIR_xf : LIR_xt,
                  lir->ins2(LIR_eq, get(a), get(b)),
                  snapshot());
}

void
TraceRecorder::guard_eqi(bool expected, void* a, int i)
{
    lir->insGuard(expected ? LIR_xf : LIR_xt,
                  lir->ins2i(LIR_eq, get(a), i),
                  snapshot());
}

void
TraceRecorder::closeLoop(Fragmento* fragmento)
{
    if (!verifyTypeStability(entryFrame, entryRegs, entryTypeMap)) {
#ifdef DEBUG
        printf("Trace rejected: unstable loop variables.\n");
#endif        
        return;
    }
    fragment->lastIns = lir->ins0(LIR_loop);
    
    compile(fragmento->assm(), fragment);
    
    
    unsigned slots = nativeFrameSlots(entryFrame, entryRegs);
    double native[slots];
    unbox(cx->fp, markRegs, (char*)fragment->vmprivate, native);
    
    
    
    
    
    
    InterpState state;
    state.ip = (FOpcodep)entryRegs.pc;
    state.sp = native;
    state.rp = NULL;
    state.f = (void*)cx;
    union { NIns *code; GuardRecord* (FASTCALL *func)(InterpState*, Fragment*); } u;
    
    
    u.code = fragment->code();
    GuardRecord* lr = u.func(&state, NULL);
    
    markRegs.sp = entryRegs.sp + (((double*)state.sp - native) - 1);
    markRegs.pc = (jsbytecode*)state.ip;
    
    
    
    box(cx->fp, markRegs, (char*)lr->vmprivate, native);
    
    
    
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
        Fragmento* fragmento = new (&gc) Fragmento(core);
#ifdef DEBUG        
        fragmento->labels = new (&gc) LabelMap(core, NULL);
#endif        
        fragmento->assm()->setCallTable(builtins);
        tm->fragmento = fragmento;
    }

    tm->recorder = new (&gc) TraceRecorder(cx, tm->fragmento);

    return true;
}

static void 
js_DeleteRecorder(JSContext* cx)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    delete tm->recorder;
    tm->recorder = NULL;
}

void
js_AbortRecording(JSContext* cx)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    JS_ASSERT(tm->recorder != NULL);
    
    tm->recorder->recover();
    js_DeleteRecorder(cx);
}

void
js_EndRecording(JSContext* cx)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    JS_ASSERT(tm->recorder != NULL);
    tm->recorder->closeLoop(tm->fragmento);
    tm->recorder->recover();
    js_DeleteRecorder(cx);
}
