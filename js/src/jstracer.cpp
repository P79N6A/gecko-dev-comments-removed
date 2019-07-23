








































#include "jsstdint.h"
#include "jsbit.h"              
#include "jsprf.h"
#include <math.h>               

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#ifdef _MSC_VER
#define alloca _alloca
#endif
#endif
#ifdef SOLARIS
#include <alloca.h>
#endif
#include <limits.h>

#include "nanojit/nanojit.h"
#include "jsapi.h"              
#include "jsarray.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsdate.h"
#include "jsstaticcheck.h"
#include "jstracer.h"
#include "jsxml.h"

#include "jsautooplen.h"        
#include "imacros.c.out"

#if JS_HAS_XML_SUPPORT
#define ABORT_IF_XML(v)                                                       \
    JS_BEGIN_MACRO                                                            \
    if (!JSVAL_IS_PRIMITIVE(v) && OBJECT_IS_XML(BOGUS_CX, JSVAL_TO_OBJECT(v)))\
        ABORT_TRACE("xml detected");                                          \
    JS_END_MACRO
#else
#define ABORT_IF_XML(cx, v) ((void) 0)
#endif




#undef JSVAL_IS_BOOLEAN
#define JSVAL_IS_BOOLEAN(x) JS_STATIC_ASSERT(0)



#define JSVAL_BOXED 3


#define JSVAL_TNULL 5


#define JSVAL_TFUN 7


static const char typeChar[] = "OIDXSNBF";
static const char tagChar[]  = "OIDISIBI";





#define HOTLOOP 2


#define BL_ATTEMPTS 2


#define BL_BACKOFF 32


#define HOTEXIT 1


#define MAXEXIT 3


#define MAXPEERS 9


#define MAX_CALLDEPTH 10


#define MAX_NATIVE_STACK_SLOTS 1024


#define MAX_CALL_STACK_ENTRIES 64


#define MAX_GLOBAL_SLOTS 4096


#define MAX_SKIP_BYTES (NJ_PAGE_SIZE - sizeof(LIns))


#define MAX_INTERP_STACK_BYTES                                                \
    (MAX_NATIVE_STACK_SLOTS * sizeof(jsval) +                                 \
     MAX_CALL_STACK_ENTRIES * sizeof(JSInlineFrame) +                         \
     sizeof(JSInlineFrame)) /* possibly slow native frame at top of stack */


#define MAX_BRANCHES 32

#define CHECK_STATUS(expr)                                                    \
    JS_BEGIN_MACRO                                                            \
        JSRecordingStatus _status = (expr);                                   \
        if (_status != JSRS_CONTINUE)                                        \
          return _status;                                                     \
    JS_END_MACRO

#ifdef JS_JIT_SPEW
#define debug_only_a(x) if (js_verboseAbort || js_verboseDebug ) { x; }
#define ABORT_TRACE_RV(msg, value)                                    \
    JS_BEGIN_MACRO                                                            \
        debug_only_a(fprintf(stdout, "abort: %d: %s\n", __LINE__, (msg));)    \
        return (value);                                                       \
    JS_END_MACRO
#else
#define debug_only_a(x)
#define ABORT_TRACE_RV(msg, value)   return (value)
#endif

#define ABORT_TRACE(msg)         ABORT_TRACE_RV(msg, JSRS_STOP)
#define ABORT_TRACE_ERROR(msg)   ABORT_TRACE_RV(msg, JSRS_ERROR) 

#ifdef JS_JIT_SPEW
struct __jitstats {
#define JITSTAT(x) uint64 x;
#include "jitstats.tbl"
#undef JITSTAT
} jitstats = { 0LL, };

JS_STATIC_ASSERT(sizeof(jitstats) % sizeof(uint64) == 0);

enum jitstat_ids {
#define JITSTAT(x) STAT ## x ## ID,
#include "jitstats.tbl"
#undef JITSTAT
    STAT_IDS_TOTAL
};

static JSPropertySpec jitstats_props[] = {
#define JITSTAT(x) { #x, STAT ## x ## ID, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT },
#include "jitstats.tbl"
#undef JITSTAT
    { 0 }
};

static JSBool
jitstats_getProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    int index = -1;

    if (JSVAL_IS_STRING(id)) {
        JSString* str = JSVAL_TO_STRING(id);
        if (strcmp(JS_GetStringBytes(str), "HOTLOOP") == 0) {
            *vp = INT_TO_JSVAL(HOTLOOP);
            return JS_TRUE;
        }
    }

    if (JSVAL_IS_INT(id))
        index = JSVAL_TO_INT(id);

    uint64 result = 0;
    switch (index) {
#define JITSTAT(x) case STAT ## x ## ID: result = jitstats.x; break;
#include "jitstats.tbl"
#undef JITSTAT
      default:
        *vp = JSVAL_VOID;
        return JS_TRUE;
    }

    if (result < JSVAL_INT_MAX) {
        *vp = INT_TO_JSVAL(result);
        return JS_TRUE;
    }
    char retstr[64];
    JS_snprintf(retstr, sizeof retstr, "%llu", result);
    *vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, retstr));
    return JS_TRUE;
}

JSClass jitstats_class = {
    "jitstats",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,       JS_PropertyStub,
    jitstats_getProperty,  JS_PropertyStub,
    JS_EnumerateStub,      JS_ResolveStub,
    JS_ConvertStub,        JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

void
js_InitJITStatsClass(JSContext *cx, JSObject *glob)
{
    JS_InitClass(cx, glob, NULL, &jitstats_class, NULL, 0, jitstats_props, NULL, NULL, NULL);
}

#define AUDIT(x) (jitstats.x++)
#else
#define AUDIT(x) ((void)0)
#endif 

#define INS_CONST(c)        addName(lir->insImm(c), #c)
#define INS_CONSTPTR(p)     addName(lir->insImmPtr(p), #p)
#define INS_CONSTFUNPTR(p)  addName(lir->insImmPtr(JS_FUNC_TO_DATA_PTR(void*, p)), #p)
#define INS_CONSTWORD(v)    addName(lir->insImmPtr((void *) v), #v)

using namespace avmplus;
using namespace nanojit;

static GC gc = GC();
static avmplus::AvmCore s_core = avmplus::AvmCore();
static avmplus::AvmCore* core = &s_core;

#ifdef JS_JIT_SPEW
void
js_DumpPeerStability(JSTraceMonitor* tm, const void* ip, JSObject* globalObj, uint32 globalShape, uint32 argc);
#endif


static bool did_we_check_processor_features = false;

#ifdef JS_JIT_SPEW
bool js_verboseDebug = getenv("TRACEMONKEY") && strstr(getenv("TRACEMONKEY"), "verbose");
bool js_verboseStats = js_verboseDebug ||
    (getenv("TRACEMONKEY") && strstr(getenv("TRACEMONKEY"), "stats"));
bool js_verboseAbort = getenv("TRACEMONKEY") && strstr(getenv("TRACEMONKEY"), "abort");
#endif



static Oracle oracle;

Tracker::Tracker()
{
    pagelist = 0;
}

Tracker::~Tracker()
{
    clear();
}

jsuword
Tracker::getPageBase(const void* v) const
{
    return jsuword(v) & ~jsuword(NJ_PAGE_SIZE-1);
}

struct Tracker::Page*
Tracker::findPage(const void* v) const
{
    jsuword base = getPageBase(v);
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
    jsuword base = getPageBase(v);
    struct Tracker::Page* p = (struct Tracker::Page*)
        GC::Alloc(sizeof(*p) - sizeof(p->map) + (NJ_PAGE_SIZE >> 2) * sizeof(LIns*));
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

bool
Tracker::has(const void *v) const
{
    return get(v) != NULL;
}

#if defined NANOJIT_64BIT
#define PAGEMASK 0x7ff
#else
#define PAGEMASK 0xfff
#endif

LIns*
Tracker::get(const void* v) const
{
    struct Tracker::Page* p = findPage(v);
    if (!p)
        return NULL;
    return p->map[(jsuword(v) & PAGEMASK) >> 2];
}

void
Tracker::set(const void* v, LIns* i)
{
    struct Tracker::Page* p = findPage(v);
    if (!p)
        p = addPage(v);
    p->map[(jsuword(v) & PAGEMASK) >> 2] = i;
}

static inline jsuint argSlots(JSStackFrame* fp)
{
    return JS_MAX(fp->argc, fp->fun->nargs);
}

static inline bool isNumber(jsval v)
{
    return JSVAL_IS_INT(v) || JSVAL_IS_DOUBLE(v);
}

static inline jsdouble asNumber(jsval v)
{
    JS_ASSERT(isNumber(v));
    if (JSVAL_IS_DOUBLE(v))
        return *JSVAL_TO_DOUBLE(v);
    return (jsdouble)JSVAL_TO_INT(v);
}

static inline bool isInt32(jsval v)
{
    if (!isNumber(v))
        return false;
    jsdouble d = asNumber(v);
    jsint i;
    return JSDOUBLE_IS_INT(d, i);
}

static inline jsint asInt32(jsval v)
{
    JS_ASSERT(isNumber(v));
    if (JSVAL_IS_INT(v))
        return JSVAL_TO_INT(v);
#ifdef DEBUG
    jsint i;
    JS_ASSERT(JSDOUBLE_IS_INT(*JSVAL_TO_DOUBLE(v), i));
#endif
    return jsint(*JSVAL_TO_DOUBLE(v));
}


static inline uint8 getPromotedType(jsval v)
{
    if (JSVAL_IS_INT(v))
        return JSVAL_DOUBLE;
    if (JSVAL_IS_OBJECT(v)) {
        if (JSVAL_IS_NULL(v))
            return JSVAL_TNULL;
        if (HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v)))
            return JSVAL_TFUN;
        return JSVAL_OBJECT;
    }
    return uint8(JSVAL_TAG(v));
}


static inline uint8 getCoercedType(jsval v)
{
    if (isInt32(v))
        return JSVAL_INT;
    if (JSVAL_IS_OBJECT(v)) {
        if (JSVAL_IS_NULL(v))
            return JSVAL_TNULL;
        if (HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v)))
            return JSVAL_TFUN;
        return JSVAL_OBJECT;
    }
    return uint8(JSVAL_TAG(v));
}





#define ORACLE_MASK (ORACLE_SIZE - 1)
#define FRAGMENT_TABLE_MASK (FRAGMENT_TABLE_SIZE - 1)
#define HASH_SEED 5381

static inline void
hash_accum(uintptr_t& h, uintptr_t i, uintptr_t mask)
{
    h = ((h << 5) + h + (mask & i)) & mask;
}

JS_REQUIRES_STACK static inline int
stackSlotHash(JSContext* cx, unsigned slot)
{
    uintptr_t h = HASH_SEED;
    hash_accum(h, uintptr_t(cx->fp->script), ORACLE_MASK);
    hash_accum(h, uintptr_t(cx->fp->regs->pc), ORACLE_MASK);
    hash_accum(h, uintptr_t(slot), ORACLE_MASK);
    return int(h);
}

JS_REQUIRES_STACK static inline int
globalSlotHash(JSContext* cx, unsigned slot)
{
    uintptr_t h = HASH_SEED;
    JSStackFrame* fp = cx->fp;

    while (fp->down)
        fp = fp->down;

    hash_accum(h, uintptr_t(fp->script), ORACLE_MASK);
    hash_accum(h, uintptr_t(OBJ_SHAPE(JS_GetGlobalForObject(cx, fp->scopeChain))),
               ORACLE_MASK);
    hash_accum(h, uintptr_t(slot), ORACLE_MASK);
    return int(h);
}

Oracle::Oracle()
{
    
    _stackDontDemote.set(&gc, ORACLE_SIZE-1);
    _globalDontDemote.set(&gc, ORACLE_SIZE-1);
    clear();
}


JS_REQUIRES_STACK void
Oracle::markGlobalSlotUndemotable(JSContext* cx, unsigned slot)
{
    _globalDontDemote.set(&gc, globalSlotHash(cx, slot));
}


JS_REQUIRES_STACK bool
Oracle::isGlobalSlotUndemotable(JSContext* cx, unsigned slot) const
{
    return _globalDontDemote.get(globalSlotHash(cx, slot));
}


JS_REQUIRES_STACK void
Oracle::markStackSlotUndemotable(JSContext* cx, unsigned slot)
{
    _stackDontDemote.set(&gc, stackSlotHash(cx, slot));
}


JS_REQUIRES_STACK bool
Oracle::isStackSlotUndemotable(JSContext* cx, unsigned slot) const
{
    return _stackDontDemote.get(stackSlotHash(cx, slot));
}

void
Oracle::clearDemotability()
{
    _stackDontDemote.reset();
    _globalDontDemote.reset();
}


struct PCHashEntry : public JSDHashEntryStub {
    size_t          count;
};

#define PC_HASH_COUNT 1024

static void
js_Blacklist(jsbytecode* pc)
{
    JS_ASSERT(*pc == JSOP_LOOP || *pc == JSOP_NOP);
    *pc = JSOP_NOP;
}

static void
js_Backoff(JSContext *cx, jsbytecode* pc, Fragment* tree=NULL)
{
    JSDHashTable *table = &JS_TRACE_MONITOR(cx).recordAttempts;

    if (table->ops) {
        PCHashEntry *entry = (PCHashEntry *)
            JS_DHashTableOperate(table, pc, JS_DHASH_ADD);
        
        if (entry) {
            if (!entry->key) {
                entry->key = pc;
                JS_ASSERT(entry->count == 0);
            }
            JS_ASSERT(JS_DHASH_ENTRY_IS_LIVE(&(entry->hdr)));
            if (entry->count++ > (BL_ATTEMPTS * MAXPEERS)) {
                entry->count = 0;
                js_Blacklist(pc);
                return;
            }
        }
    }

    if (tree) {
        tree->hits() -= BL_BACKOFF;

        





        if (++tree->recordAttempts > BL_ATTEMPTS)
            js_Blacklist(pc);
    }
}

static void
js_resetRecordingAttempts(JSContext *cx, jsbytecode* pc)
{
    JSDHashTable *table = &JS_TRACE_MONITOR(cx).recordAttempts;
    if (table->ops) {
        PCHashEntry *entry = (PCHashEntry *)
            JS_DHashTableOperate(table, pc, JS_DHASH_LOOKUP);

        if (JS_DHASH_ENTRY_IS_FREE(&(entry->hdr)))
            return;
        JS_ASSERT(JS_DHASH_ENTRY_IS_LIVE(&(entry->hdr)));
        entry->count = 0;
    }
}

static inline size_t
fragmentHash(const void *ip, JSObject* globalObj, uint32 globalShape, uint32 argc)
{
    uintptr_t h = HASH_SEED;
    hash_accum(h, uintptr_t(ip), FRAGMENT_TABLE_MASK);
    hash_accum(h, uintptr_t(globalObj), FRAGMENT_TABLE_MASK);
    hash_accum(h, uintptr_t(globalShape), FRAGMENT_TABLE_MASK);
    hash_accum(h, uintptr_t(argc), FRAGMENT_TABLE_MASK);
    return size_t(h);
}










struct VMFragment : public Fragment
{
    VMFragment(const void* _ip, JSObject* _globalObj, uint32 _globalShape, uint32 _argc) :
        Fragment(_ip),
        next(NULL),
        globalObj(_globalObj),
        globalShape(_globalShape),
        argc(_argc)
    {}
    VMFragment* next;
    JSObject* globalObj;
    uint32 globalShape;
    uint32 argc;
};

static VMFragment*
getVMFragment(JSTraceMonitor* tm, const void *ip, JSObject* globalObj, uint32 globalShape, 
              uint32 argc)
{
    size_t h = fragmentHash(ip, globalObj, globalShape, argc);
    VMFragment* vf = tm->vmfragments[h];
    while (vf &&
           ! (vf->globalObj == globalObj &&
              vf->globalShape == globalShape &&
              vf->ip == ip &&
              vf->argc == argc)) {
        vf = vf->next;
    }
    return vf;
}

static VMFragment*
getLoop(JSTraceMonitor* tm, const void *ip, JSObject* globalObj, uint32 globalShape, 
        uint32 argc)
{
    return getVMFragment(tm, ip, globalObj, globalShape, argc);
}

static Fragment*
getAnchor(JSTraceMonitor* tm, const void *ip, JSObject* globalObj, uint32 globalShape, 
          uint32 argc)
{
    VMFragment *f = new (&gc) VMFragment(ip, globalObj, globalShape, argc);
    JS_ASSERT(f);

    Fragment *p = getVMFragment(tm, ip, globalObj, globalShape, argc);

    if (p) {
        f->first = p;
        
        Fragment* next;
        while ((next = p->peer) != NULL)
            p = next;
        p->peer = f;
    } else {
        
        f->first = f;
        size_t h = fragmentHash(ip, globalObj, globalShape, argc);
        f->next = tm->vmfragments[h];
        tm->vmfragments[h] = f;
    }
    f->anchor = f;
    f->root = f;
    f->kind = LoopTrace;
    return f;
}

static void
js_AttemptCompilation(JSContext *cx, JSTraceMonitor* tm, JSObject* globalObj, jsbytecode* pc,
                      uint32 argc)
{
    


    JS_ASSERT(*(jsbytecode*)pc == JSOP_NOP || *(jsbytecode*)pc == JSOP_LOOP);
    *(jsbytecode*)pc = JSOP_LOOP;
    js_resetRecordingAttempts(cx, pc);

    


    Fragment* f = (VMFragment*)getLoop(tm, pc, globalObj, OBJ_SHAPE(globalObj),
                                       argc);
    if (!f) {
        





        return;
    }
    JS_ASSERT(f->root == f);
    f = f->first;
    while (f) {
        JS_ASSERT(f->root == f);
        --f->recordAttempts;
        f->hits() = HOTLOOP;
        f = f->peer;
    }
}


JS_DEFINE_CALLINFO_1(static, DOUBLE, i2f,  INT32, 1, 1)
JS_DEFINE_CALLINFO_1(static, DOUBLE, u2f, UINT32, 1, 1)

static bool isi2f(LInsp i)
{
    if (i->isop(LIR_i2f))
        return true;

    if (nanojit::AvmCore::config.soft_float &&
        i->isop(LIR_qjoin) &&
        i->oprnd1()->isop(LIR_call) &&
        i->oprnd2()->isop(LIR_callh))
    {
        if (i->oprnd1()->callInfo() == &i2f_ci)
            return true;
    }

    return false;
}

static bool isu2f(LInsp i)
{
    if (i->isop(LIR_u2f))
        return true;

    if (nanojit::AvmCore::config.soft_float &&
        i->isop(LIR_qjoin) &&
        i->oprnd1()->isop(LIR_call) &&
        i->oprnd2()->isop(LIR_callh))
    {
        if (i->oprnd1()->callInfo() == &u2f_ci)
            return true;
    }

    return false;
}

static LInsp iu2fArg(LInsp i)
{
    if (nanojit::AvmCore::config.soft_float &&
        i->isop(LIR_qjoin))
    {
        return i->oprnd1()->arg(0);
    }

    return i->oprnd1();
}


static LIns* demote(LirWriter *out, LInsp i)
{
    if (i->isCall())
        return callArgN(i, 0);
    if (isi2f(i) || isu2f(i))
        return iu2fArg(i);
    if (i->isconst())
        return i;
    AvmAssert(i->isconstq());
    double cf = i->imm64f();
    int32_t ci = cf > 0x7fffffff ? uint32_t(cf) : int32_t(cf);
    return out->insImm(ci);
}

static bool isPromoteInt(LIns* i)
{
    if (isi2f(i) || i->isconst())
        return true;
    if (!i->isconstq())
        return false;
    jsdouble d = i->imm64f();
    return d == jsdouble(jsint(d)) && !JSDOUBLE_IS_NEGZERO(d);
}

static bool isPromoteUint(LIns* i)
{
    if (isu2f(i) || i->isconst())
        return true;
    if (!i->isconstq())
        return false;
    jsdouble d = i->imm64f();
    return d == jsdouble(jsuint(d)) && !JSDOUBLE_IS_NEGZERO(d);
}

static bool isPromote(LIns* i)
{
    return isPromoteInt(i) || isPromoteUint(i);
}

static bool isconst(LIns* i, int32_t c)
{
    return i->isconst() && i->imm32() == c;
}

static bool overflowSafe(LIns* i)
{
    LIns* c;
    return (i->isop(LIR_and) && ((c = i->oprnd2())->isconst()) &&
            ((c->imm32() & 0xc0000000) == 0)) ||
           (i->isop(LIR_rsh) && ((c = i->oprnd2())->isconst()) &&
            ((c->imm32() > 0)));
}



static jsdouble FASTCALL
fneg(jsdouble x)
{
    return -x;
}
JS_DEFINE_CALLINFO_1(static, DOUBLE, fneg, DOUBLE, 1, 1)

static jsdouble FASTCALL
i2f(int32 i)
{
    return i;
}

static jsdouble FASTCALL
u2f(jsuint u)
{
    return u;
}

static int32 FASTCALL
fcmpeq(jsdouble x, jsdouble y)
{
    return x==y;
}
JS_DEFINE_CALLINFO_2(static, INT32, fcmpeq, DOUBLE, DOUBLE, 1, 1)

static int32 FASTCALL
fcmplt(jsdouble x, jsdouble y)
{
    return x < y;
}
JS_DEFINE_CALLINFO_2(static, INT32, fcmplt, DOUBLE, DOUBLE, 1, 1)

static int32 FASTCALL
fcmple(jsdouble x, jsdouble y)
{
    return x <= y;
}
JS_DEFINE_CALLINFO_2(static, INT32, fcmple, DOUBLE, DOUBLE, 1, 1)

static int32 FASTCALL
fcmpgt(jsdouble x, jsdouble y)
{
    return x > y;
}
JS_DEFINE_CALLINFO_2(static, INT32, fcmpgt, DOUBLE, DOUBLE, 1, 1)

static int32 FASTCALL
fcmpge(jsdouble x, jsdouble y)
{
    return x >= y;
}
JS_DEFINE_CALLINFO_2(static, INT32, fcmpge, DOUBLE, DOUBLE, 1, 1)

static jsdouble FASTCALL
fmul(jsdouble x, jsdouble y)
{
    return x * y;
}
JS_DEFINE_CALLINFO_2(static, DOUBLE, fmul, DOUBLE, DOUBLE, 1, 1)

static jsdouble FASTCALL
fadd(jsdouble x, jsdouble y)
{
    return x + y;
}
JS_DEFINE_CALLINFO_2(static, DOUBLE, fadd, DOUBLE, DOUBLE, 1, 1)

static jsdouble FASTCALL
fdiv(jsdouble x, jsdouble y)
{
    return x / y;
}
JS_DEFINE_CALLINFO_2(static, DOUBLE, fdiv, DOUBLE, DOUBLE, 1, 1)

static jsdouble FASTCALL
fsub(jsdouble x, jsdouble y)
{
    return x - y;
}
JS_DEFINE_CALLINFO_2(static, DOUBLE, fsub, DOUBLE, DOUBLE, 1, 1)

class SoftFloatFilter: public LirWriter
{
public:
    SoftFloatFilter(LirWriter* out):
        LirWriter(out)
    {
    }

    LInsp quadCall(const CallInfo *ci, LInsp args[]) {
        LInsp qlo, qhi;

        qlo = out->insCall(ci, args);
        qhi = out->ins1(LIR_callh, qlo);
        return out->qjoin(qlo, qhi);
    }

    LInsp ins1(LOpcode v, LInsp s0)
    {
        if (v == LIR_fneg)
            return quadCall(&fneg_ci, &s0);

        if (v == LIR_i2f)
            return quadCall(&i2f_ci, &s0);

        if (v == LIR_u2f)
            return quadCall(&u2f_ci, &s0);

        return out->ins1(v, s0);
    }

    LInsp ins2(LOpcode v, LInsp s0, LInsp s1)
    {
        LInsp args[2];
        LInsp bv;

        
        if (LIR_fadd <= v && v <= LIR_fdiv) {
            static const CallInfo *fmap[] = { &fadd_ci, &fsub_ci, &fmul_ci, &fdiv_ci };

            args[0] = s1;
            args[1] = s0;

            return quadCall(fmap[v - LIR_fadd], args);
        }

        if (LIR_feq <= v && v <= LIR_fge) {
            static const CallInfo *fmap[] = { &fcmpeq_ci, &fcmplt_ci, &fcmpgt_ci, &fcmple_ci, &fcmpge_ci };

            args[0] = s1;
            args[1] = s0;

            bv = out->insCall(fmap[v - LIR_feq], args);
            return out->ins2(LIR_eq, bv, out->insImm(1));
        }

        return out->ins2(v, s0, s1);
    }

    LInsp insCall(const CallInfo *ci, LInsp args[])
    {
        
        
        if ((ci->_argtypes & 3) == ARGSIZE_F)
            return quadCall(ci, args);

        return out->insCall(ci, args);
    }
};

class FuncFilter: public LirWriter
{
public:
    FuncFilter(LirWriter* out):
        LirWriter(out)
    {
    }

    LInsp ins2(LOpcode v, LInsp s0, LInsp s1)
    {
        if (s0 == s1 && v == LIR_feq) {
            if (isPromote(s0)) {
                
                return insImm(1);
            }
            if (s0->isop(LIR_fmul) || s0->isop(LIR_fsub) || s0->isop(LIR_fadd)) {
                LInsp lhs = s0->oprnd1();
                LInsp rhs = s0->oprnd2();
                if (isPromote(lhs) && isPromote(rhs)) {
                    
                    return insImm(1);
                }
            }
        } else if (LIR_feq <= v && v <= LIR_fge) {
            if (isPromoteInt(s0) && isPromoteInt(s1)) {
                
                v = LOpcode(v + (LIR_eq - LIR_feq));
                return out->ins2(v, demote(out, s0), demote(out, s1));
            } else if (isPromoteUint(s0) && isPromoteUint(s1)) {
                
                v = LOpcode(v + (LIR_eq - LIR_feq));
                if (v != LIR_eq)
                    v = LOpcode(v + (LIR_ult - LIR_lt)); 
                return out->ins2(v, demote(out, s0), demote(out, s1));
            }
        } else if (v == LIR_or &&
                   s0->isop(LIR_lsh) && isconst(s0->oprnd2(), 16) &&
                   s1->isop(LIR_and) && isconst(s1->oprnd2(), 0xffff)) {
            LIns* msw = s0->oprnd1();
            LIns* lsw = s1->oprnd1();
            LIns* x;
            LIns* y;
            if (lsw->isop(LIR_add) &&
                lsw->oprnd1()->isop(LIR_and) &&
                lsw->oprnd2()->isop(LIR_and) &&
                isconst(lsw->oprnd1()->oprnd2(), 0xffff) &&
                isconst(lsw->oprnd2()->oprnd2(), 0xffff) &&
                msw->isop(LIR_add) &&
                msw->oprnd1()->isop(LIR_add) &&
                msw->oprnd2()->isop(LIR_rsh) &&
                msw->oprnd1()->oprnd1()->isop(LIR_rsh) &&
                msw->oprnd1()->oprnd2()->isop(LIR_rsh) &&
                isconst(msw->oprnd2()->oprnd2(), 16) &&
                isconst(msw->oprnd1()->oprnd1()->oprnd2(), 16) &&
                isconst(msw->oprnd1()->oprnd2()->oprnd2(), 16) &&
                (x = lsw->oprnd1()->oprnd1()) == msw->oprnd1()->oprnd1()->oprnd1() &&
                (y = lsw->oprnd2()->oprnd1()) == msw->oprnd1()->oprnd2()->oprnd1() &&
                lsw == msw->oprnd2()->oprnd1()) {
                return out->ins2(LIR_add, x, y);
            }
        }

        return out->ins2(v, s0, s1);
    }

    LInsp insCall(const CallInfo *ci, LInsp args[])
    {
        if (ci == &js_DoubleToUint32_ci) {
            LInsp s0 = args[0];
            if (s0->isconstq())
                return out->insImm(js_DoubleToECMAUint32(s0->imm64f()));
            if (isi2f(s0) || isu2f(s0))
                return iu2fArg(s0);
        } else if (ci == &js_DoubleToInt32_ci) {
            LInsp s0 = args[0];
            if (s0->isconstq())
                return out->insImm(js_DoubleToECMAInt32(s0->imm64f()));
            if (s0->isop(LIR_fadd) || s0->isop(LIR_fsub)) {
                LInsp lhs = s0->oprnd1();
                LInsp rhs = s0->oprnd2();
                if (isPromote(lhs) && isPromote(rhs)) {
                    LOpcode op = LOpcode(s0->opcode() & ~LIR64);
                    return out->ins2(op, demote(out, lhs), demote(out, rhs));
                }
            }
            if (isi2f(s0) || isu2f(s0))
                return iu2fArg(s0);
            
            if (s0->isCall()) {
                const CallInfo* ci2 = s0->callInfo();
                if (ci2 == &js_UnboxDouble_ci) {
                    LIns* args2[] = { callArgN(s0, 0) };
                    return out->insCall(&js_UnboxInt32_ci, args2);
                } else if (ci2 == &js_StringToNumber_ci) {
                    
                    
                    LIns* args2[] = { callArgN(s0, 1), callArgN(s0, 0) };
                    return out->insCall(&js_StringToInt32_ci, args2);
                } else if (ci2 == &js_String_p_charCodeAt0_ci) {
                    
                    LIns* args2[] = { callArgN(s0, 0) };
                    return out->insCall(&js_String_p_charCodeAt0_int_ci, args2);
                } else if (ci2 == &js_String_p_charCodeAt_ci) {
                    LIns* idx = callArgN(s0, 1);
                    
                    idx = isPromote(idx)
                        ? demote(out, idx)
                        : out->insCall(&js_DoubleToInt32_ci, &idx);
                    LIns* args2[] = { idx, callArgN(s0, 0) };
                    return out->insCall(&js_String_p_charCodeAt_int_ci, args2);
                }
            }
        } else if (ci == &js_BoxDouble_ci) {
            LInsp s0 = args[0];
            JS_ASSERT(s0->isQuad());
            if (isi2f(s0)) {
                LIns* args2[] = { iu2fArg(s0), args[1] };
                return out->insCall(&js_BoxInt32_ci, args2);
            }
            if (s0->isCall() && s0->callInfo() == &js_UnboxDouble_ci)
                return callArgN(s0, 0);
        }
        return out->insCall(ci, args);
    }
};





#ifdef JS_JIT_SPEW
#define DEF_VPNAME          const char* vpname; unsigned vpnum
#define SET_VPNAME(name)    do { vpname = name; vpnum = 0; } while(0)
#define INC_VPNUM()         do { ++vpnum; } while(0)
#else
#define DEF_VPNAME          do {} while (0)
#define vpname ""
#define vpnum 0x40000000
#define SET_VPNAME(name)    ((void)0)
#define INC_VPNUM()         ((void)0)
#endif


#define FORALL_GLOBAL_SLOTS(cx, ngslots, gslots, code)                        \
    JS_BEGIN_MACRO                                                            \
        DEF_VPNAME;                                                           \
        JSObject* globalObj = JS_GetGlobalForObject(cx, cx->fp->scopeChain);  \
        unsigned n;                                                           \
        jsval* vp;                                                            \
        SET_VPNAME("global");                                                 \
        for (n = 0; n < ngslots; ++n) {                                       \
            vp = &STOBJ_GET_SLOT(globalObj, gslots[n]);                       \
            { code; }                                                         \
            INC_VPNUM();                                                      \
        }                                                                     \
    JS_END_MACRO



#define FORALL_FRAME_SLOTS(fp, depth, code)                                   \
    JS_BEGIN_MACRO                                                            \
        jsval* vp;                                                            \
        jsval* vpstop;                                                        \
        if (fp->callee) {                                                     \
            if (depth == 0) {                                                 \
                SET_VPNAME("callee");                                         \
                vp = &fp->argv[-2];                                           \
                { code; }                                                     \
                SET_VPNAME("this");                                           \
                vp = &fp->argv[-1];                                           \
                { code; }                                                     \
                SET_VPNAME("argv");                                           \
                vp = &fp->argv[0]; vpstop = &fp->argv[argSlots(fp)];          \
                while (vp < vpstop) { code; ++vp; INC_VPNUM(); }              \
            }                                                                 \
            SET_VPNAME("vars");                                               \
            vp = fp->slots; vpstop = &fp->slots[fp->script->nfixed];          \
            while (vp < vpstop) { code; ++vp; INC_VPNUM(); }                  \
        }                                                                     \
        SET_VPNAME("stack");                                                  \
        vp = StackBase(fp); vpstop = fp->regs->sp;                            \
        while (vp < vpstop) { code; ++vp; INC_VPNUM(); }                      \
        if (fsp < fspstop - 1) {                                              \
            JSStackFrame* fp2 = fsp[1];                                       \
            int missing = fp2->fun->nargs - fp2->argc;                        \
            if (missing > 0) {                                                \
                SET_VPNAME("missing");                                        \
                vp = fp->regs->sp;                                            \
                vpstop = vp + missing;                                        \
                while (vp < vpstop) { code; ++vp; INC_VPNUM(); }              \
            }                                                                 \
        }                                                                     \
    JS_END_MACRO


#define FORALL_SLOTS_IN_PENDING_FRAMES(cx, callDepth, code)                   \
    JS_BEGIN_MACRO                                                            \
        DEF_VPNAME;                                                           \
        unsigned n;                                                           \
        JSStackFrame* currentFrame = cx->fp;                                  \
        JSStackFrame* entryFrame;                                             \
        JSStackFrame* fp = currentFrame;                                      \
        for (n = 0; n < callDepth; ++n) { fp = fp->down; }                    \
        entryFrame = fp;                                                      \
        unsigned frames = callDepth+1;                                        \
        JSStackFrame** fstack =                                               \
            (JSStackFrame**) alloca(frames * sizeof (JSStackFrame*));         \
        JSStackFrame** fspstop = &fstack[frames];                             \
        JSStackFrame** fsp = fspstop-1;                                       \
        fp = currentFrame;                                                    \
        for (;; fp = fp->down) { *fsp-- = fp; if (fp == entryFrame) break; }  \
        unsigned depth;                                                       \
        for (depth = 0, fsp = fstack; fsp < fspstop; ++fsp, ++depth) {        \
            fp = *fsp;                                                        \
            FORALL_FRAME_SLOTS(fp, depth, code);                              \
        }                                                                     \
    JS_END_MACRO

#define FORALL_SLOTS(cx, ngslots, gslots, callDepth, code)                    \
    JS_BEGIN_MACRO                                                            \
        FORALL_SLOTS_IN_PENDING_FRAMES(cx, callDepth, code);                  \
        FORALL_GLOBAL_SLOTS(cx, ngslots, gslots, code);                       \
    JS_END_MACRO



JS_REQUIRES_STACK unsigned
js_NativeStackSlots(JSContext *cx, unsigned callDepth)
{
    JSStackFrame* fp = cx->fp;
    unsigned slots = 0;
#if defined _DEBUG
    unsigned int origCallDepth = callDepth;
#endif
    for (;;) {
        unsigned operands = fp->regs->sp - StackBase(fp);
        slots += operands;
        if (fp->callee)
            slots += fp->script->nfixed;
        if (callDepth-- == 0) {
            if (fp->callee)
                slots += 2 + argSlots(fp);
#if defined _DEBUG
            unsigned int m = 0;
            FORALL_SLOTS_IN_PENDING_FRAMES(cx, origCallDepth, m++);
            JS_ASSERT(m == slots);
#endif
            return slots;
        }
        JSStackFrame* fp2 = fp;
        fp = fp->down;
        int missing = fp2->fun->nargs - fp2->argc;
        if (missing > 0)
            slots += missing;
    }
    JS_NOT_REACHED("js_NativeStackSlots");
}





JS_REQUIRES_STACK void
TypeMap::captureTypes(JSContext* cx, SlotList& slots, unsigned callDepth)
{
    unsigned ngslots = slots.length();
    uint16* gslots = slots.data();
    setLength(js_NativeStackSlots(cx, callDepth) + ngslots);
    uint8* map = data();
    uint8* m = map;
    FORALL_SLOTS_IN_PENDING_FRAMES(cx, callDepth,
        uint8 type = getCoercedType(*vp);
        if ((type == JSVAL_INT) && oracle.isStackSlotUndemotable(cx, unsigned(m - map)))
            type = JSVAL_DOUBLE;
        JS_ASSERT(type != JSVAL_BOXED);
        debug_only_v(printf("capture stack type %s%d: %d=%c\n", vpname, vpnum, type, typeChar[type]);)
        JS_ASSERT(uintptr_t(m - map) < length());
        *m++ = type;
    );
    FORALL_GLOBAL_SLOTS(cx, ngslots, gslots,
        uint8 type = getCoercedType(*vp);
        if ((type == JSVAL_INT) && oracle.isGlobalSlotUndemotable(cx, gslots[n]))
            type = JSVAL_DOUBLE;
        JS_ASSERT(type != JSVAL_BOXED);
        debug_only_v(printf("capture global type %s%d: %d=%c\n", vpname, vpnum, type, typeChar[type]);)
        JS_ASSERT(uintptr_t(m - map) < length());
        *m++ = type;
    );
    JS_ASSERT(uintptr_t(m - map) == length());
}

JS_REQUIRES_STACK void
TypeMap::captureMissingGlobalTypes(JSContext* cx, SlotList& slots, unsigned stackSlots)
{
    unsigned oldSlots = length() - stackSlots;
    int diff = slots.length() - oldSlots;
    JS_ASSERT(diff >= 0);
    unsigned ngslots = slots.length();
    uint16* gslots = slots.data();
    setLength(length() + diff);
    uint8* map = data() + stackSlots;
    uint8* m = map;
    FORALL_GLOBAL_SLOTS(cx, ngslots, gslots,
        if (n >= oldSlots) {
            uint8 type = getCoercedType(*vp);
            if ((type == JSVAL_INT) && oracle.isGlobalSlotUndemotable(cx, gslots[n]))
                type = JSVAL_DOUBLE;
            JS_ASSERT(type != JSVAL_BOXED);
            debug_only_v(printf("capture global type %s%d: %d=%c\n", vpname, vpnum, type, typeChar[type]);)
            *m = type;
            JS_ASSERT((m > map + oldSlots) || (*m == type));
        }
        m++;
    );
}


bool
TypeMap::matches(TypeMap& other) const
{
    if (length() != other.length())
        return false;
    return !memcmp(data(), other.data(), length());
}



static void
mergeTypeMaps(uint8** partial, unsigned* plength, uint8* complete, unsigned clength, uint8* mem)
{
    unsigned l = *plength;
    JS_ASSERT(l < clength);
    memcpy(mem, *partial, l * sizeof(uint8));
    memcpy(mem + l, complete + l, (clength - l) * sizeof(uint8));
    *partial = mem;
    *plength = clength;
}


static JS_REQUIRES_STACK void
specializeTreesToMissingGlobals(JSContext* cx, TreeInfo* root)
{
    TreeInfo* ti = root;

    ti->typeMap.captureMissingGlobalTypes(cx, *ti->globalSlots, ti->nStackTypes);
    JS_ASSERT(ti->globalSlots->length() == ti->typeMap.length() - ti->nStackTypes);

    for (unsigned i = 0; i < root->dependentTrees.length(); i++) {
        ti = (TreeInfo*)root->dependentTrees.data()[i]->vmprivate;
        
        if (ti && ti->nGlobalTypes() < ti->globalSlots->length())
            specializeTreesToMissingGlobals(cx, ti);
    }
    for (unsigned i = 0; i < root->linkedTrees.length(); i++) {
        ti = (TreeInfo*)root->linkedTrees.data()[i]->vmprivate;
        if (ti && ti->nGlobalTypes() < ti->globalSlots->length())
            specializeTreesToMissingGlobals(cx, ti);
    }
}

static void
js_TrashTree(JSContext* cx, Fragment* f);

JS_REQUIRES_STACK
TraceRecorder::TraceRecorder(JSContext* cx, VMSideExit* _anchor, Fragment* _fragment,
        TreeInfo* ti, unsigned stackSlots, unsigned ngslots, uint8* typeMap,
        VMSideExit* innermostNestedGuard, jsbytecode* outer, uint32 outerArgc)
{
    JS_ASSERT(!_fragment->vmprivate && ti && cx->fp->regs->pc == (jsbytecode*)_fragment->ip);

    
    _fragment->lastIns = NULL;

    this->cx = cx;
    this->traceMonitor = &JS_TRACE_MONITOR(cx);
    this->globalObj = JS_GetGlobalForObject(cx, cx->fp->scopeChain);
    this->lexicalBlock = cx->fp->blockChain;
    this->anchor = _anchor;
    this->fragment = _fragment;
    this->lirbuf = _fragment->lirbuf;
    this->treeInfo = ti;
    this->callDepth = _anchor ? _anchor->calldepth : 0;
    this->atoms = FrameAtomBase(cx, cx->fp);
    this->deepAborted = false;
    this->trashSelf = false;
    this->global_dslots = this->globalObj->dslots;
    this->loop = true; 
    this->wasRootFragment = _fragment == _fragment->root;
    this->outer = outer;
    this->outerArgc = outerArgc;
    this->pendingTraceableNative = NULL;
    this->newobj_ins = NULL;
    this->generatedTraceableNative = new JSTraceableNative();
    JS_ASSERT(generatedTraceableNative);

    debug_only_v(printf("recording starting from %s:%u@%u\n",
                        ti->treeFileName, ti->treeLineNumber, ti->treePCOffset);)
    debug_only_v(printf("globalObj=%p, shape=%d\n", (void*)this->globalObj, OBJ_SHAPE(this->globalObj));)

    lir = lir_buf_writer = new (&gc) LirBufWriter(lirbuf);
    debug_only_v(lir = verbose_filter = new (&gc) VerboseWriter(&gc, lir, lirbuf->names);)
    if (nanojit::AvmCore::config.soft_float)
        lir = float_filter = new (&gc) SoftFloatFilter(lir);
    else
        float_filter = 0;
    lir = cse_filter = new (&gc) CseFilter(lir, &gc);
    lir = expr_filter = new (&gc) ExprFilter(lir);
    lir = func_filter = new (&gc) FuncFilter(lir);
    lir->ins0(LIR_start);

    if (!nanojit::AvmCore::config.tree_opt || fragment->root == fragment)
        lirbuf->state = addName(lir->insParam(0, 0), "state");

    lirbuf->sp = addName(lir->insLoad(LIR_ldp, lirbuf->state, (int)offsetof(InterpState, sp)), "sp");
    lirbuf->rp = addName(lir->insLoad(LIR_ldp, lirbuf->state, offsetof(InterpState, rp)), "rp");
    cx_ins = addName(lir->insLoad(LIR_ldp, lirbuf->state, offsetof(InterpState, cx)), "cx");
    eos_ins = addName(lir->insLoad(LIR_ldp, lirbuf->state, offsetof(InterpState, eos)), "eos");
    eor_ins = addName(lir->insLoad(LIR_ldp, lirbuf->state, offsetof(InterpState, eor)), "eor");

    
    if (ti->globalSlots->length() > ti->nGlobalTypes())
        specializeTreesToMissingGlobals(cx, ti);

    
    import(treeInfo, lirbuf->sp, stackSlots, ngslots, callDepth, typeMap);

    if (fragment == fragment->root) {
        



        LIns* x = lir->insLoadi(cx_ins, offsetof(JSContext, operationCallbackFlag));
        guard(true, lir->ins_eq0(x), snapshot(TIMEOUT_EXIT));
    }

    

    if (_anchor && _anchor->exitType == NESTED_EXIT) {
        LIns* nested_ins = addName(lir->insLoad(LIR_ldp, lirbuf->state,
                                                offsetof(InterpState, lastTreeExitGuard)),
                                                "lastTreeExitGuard");
        guard(true, lir->ins2(LIR_eq, nested_ins, INS_CONSTPTR(innermostNestedGuard)), NESTED_EXIT);
    }
}

TreeInfo::~TreeInfo()
{
    UnstableExit* temp;
    
    while (unstableExits) {
        temp = unstableExits->next;
        delete unstableExits;
        unstableExits = temp;
    }
}

TraceRecorder::~TraceRecorder()
{
    JS_ASSERT(nextRecorderToAbort == NULL);
    JS_ASSERT(treeInfo && (fragment || wasDeepAborted()));
#ifdef DEBUG
    TraceRecorder* tr = JS_TRACE_MONITOR(cx).abortStack;
    while (tr != NULL)
    {
        JS_ASSERT(this != tr);
        tr = tr->nextRecorderToAbort;
    }
#endif
    if (fragment) {
        if (wasRootFragment && !fragment->root->code()) {
            JS_ASSERT(!fragment->root->vmprivate);
            delete treeInfo;
        }

        if (trashSelf)
            js_TrashTree(cx, fragment->root);

        for (unsigned int i = 0; i < whichTreesToTrash.length(); i++)
            js_TrashTree(cx, whichTreesToTrash.get(i));
    } else if (wasRootFragment) {
        delete treeInfo;
    }
#ifdef DEBUG
    delete verbose_filter;
#endif
    delete cse_filter;
    delete expr_filter;
    delete func_filter;
    delete float_filter;
    delete lir_buf_writer;
    delete generatedTraceableNative;
}

void TraceRecorder::removeFragmentoReferences()
{
    fragment = NULL;
}

void TraceRecorder::deepAbort()
{
    debug_only_v(printf("deep abort");)
    deepAborted = true;
}


inline LIns*
TraceRecorder::addName(LIns* ins, const char* name)
{
#ifdef JS_JIT_SPEW
    if (js_verboseDebug)
        lirbuf->names->addName(ins, name);
#endif
    return ins;
}


unsigned
TraceRecorder::getCallDepth() const
{
    return callDepth;
}


ptrdiff_t
TraceRecorder::nativeGlobalOffset(jsval* p) const
{
    JS_ASSERT(isGlobal(p));
    if (size_t(p - globalObj->fslots) < JS_INITIAL_NSLOTS)
        return sizeof(InterpState) + size_t(p - globalObj->fslots) * sizeof(double);
    return sizeof(InterpState) + ((p - globalObj->dslots) + JS_INITIAL_NSLOTS) * sizeof(double);
}


bool
TraceRecorder::isGlobal(jsval* p) const
{
    return ((size_t(p - globalObj->fslots) < JS_INITIAL_NSLOTS) ||
            (size_t(p - globalObj->dslots) < (STOBJ_NSLOTS(globalObj) - JS_INITIAL_NSLOTS)));
}


JS_REQUIRES_STACK ptrdiff_t
TraceRecorder::nativeStackOffset(jsval* p) const
{
#ifdef DEBUG
    size_t slow_offset = 0;
    FORALL_SLOTS_IN_PENDING_FRAMES(cx, callDepth,
        if (vp == p) goto done;
        slow_offset += sizeof(double)
    );

    



    JS_ASSERT(size_t(p - cx->fp->slots) < cx->fp->script->nslots);
    slow_offset += size_t(p - cx->fp->regs->sp) * sizeof(double);

done:
#define RETURN(offset) { JS_ASSERT((offset) == slow_offset); return offset; }
#else
#define RETURN(offset) { return offset; }
#endif
    size_t offset = 0;
    JSStackFrame* currentFrame = cx->fp;
    JSStackFrame* entryFrame;
    JSStackFrame* fp = currentFrame;
    for (unsigned n = 0; n < callDepth; ++n) { fp = fp->down; }
    entryFrame = fp;
    unsigned frames = callDepth+1;
    JSStackFrame** fstack = (JSStackFrame **)alloca(frames * sizeof (JSStackFrame *));
    JSStackFrame** fspstop = &fstack[frames];
    JSStackFrame** fsp = fspstop-1;
    fp = currentFrame;
    for (;; fp = fp->down) { *fsp-- = fp; if (fp == entryFrame) break; }
    for (fsp = fstack; fsp < fspstop; ++fsp) {
        fp = *fsp;
        if (fp->callee) {
            if (fsp == fstack) {
                if (size_t(p - &fp->argv[-2]) < size_t(2 + argSlots(fp)))
                    RETURN(offset + size_t(p - &fp->argv[-2]) * sizeof(double));
                offset += (2 + argSlots(fp)) * sizeof(double);
            }
            if (size_t(p - &fp->slots[0]) < fp->script->nfixed)
                RETURN(offset + size_t(p - &fp->slots[0]) * sizeof(double));
            offset += fp->script->nfixed * sizeof(double);
        }
        jsval* spbase = StackBase(fp);
        if (size_t(p - spbase) < size_t(fp->regs->sp - spbase))
            RETURN(offset + size_t(p - spbase) * sizeof(double));
        offset += size_t(fp->regs->sp - spbase) * sizeof(double);
        if (fsp < fspstop - 1) {
            JSStackFrame* fp2 = fsp[1];
            int missing = fp2->fun->nargs - fp2->argc;
            if (missing > 0) {
                if (size_t(p - fp->regs->sp) < size_t(missing))
                    RETURN(offset + size_t(p - fp->regs->sp) * sizeof(double));
                offset += size_t(missing) * sizeof(double);
            }
        }
    }

    



    JS_ASSERT(size_t(p - currentFrame->slots) < currentFrame->script->nslots);
    offset += size_t(p - currentFrame->regs->sp) * sizeof(double);
    RETURN(offset);
#undef RETURN
}



void
TraceRecorder::trackNativeStackUse(unsigned slots)
{
    if (slots > treeInfo->maxNativeStackSlots)
        treeInfo->maxNativeStackSlots = slots;
}




static void
ValueToNative(JSContext* cx, jsval v, uint8 type, double* slot)
{
    unsigned tag = JSVAL_TAG(v);
    switch (type) {
      case JSVAL_OBJECT:
        JS_ASSERT(tag == JSVAL_OBJECT);
        JS_ASSERT(!JSVAL_IS_NULL(v) && !HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v)));
        *(JSObject**)slot = JSVAL_TO_OBJECT(v);
        debug_only_v(printf("object<%p:%s> ", (void*)JSVAL_TO_OBJECT(v),
                            JSVAL_IS_NULL(v)
                            ? "null"
                            : STOBJ_GET_CLASS(JSVAL_TO_OBJECT(v))->name);)
        return;
      case JSVAL_INT:
        jsint i;
        if (JSVAL_IS_INT(v))
            *(jsint*)slot = JSVAL_TO_INT(v);
        else if ((tag == JSVAL_DOUBLE) && JSDOUBLE_IS_INT(*JSVAL_TO_DOUBLE(v), i))
            *(jsint*)slot = i;
        else
            JS_ASSERT(JSVAL_IS_INT(v));
        debug_only_v(printf("int<%d> ", *(jsint*)slot);)
        return;
      case JSVAL_DOUBLE:
        jsdouble d;
        if (JSVAL_IS_INT(v))
            d = JSVAL_TO_INT(v);
        else
            d = *JSVAL_TO_DOUBLE(v);
        JS_ASSERT(JSVAL_IS_INT(v) || JSVAL_IS_DOUBLE(v));
        *(jsdouble*)slot = d;
        debug_only_v(printf("double<%g> ", d);)
        return;
      case JSVAL_BOXED:
        JS_NOT_REACHED("found boxed type in an entry type map");
        return;
      case JSVAL_STRING:
        JS_ASSERT(tag == JSVAL_STRING);
        *(JSString**)slot = JSVAL_TO_STRING(v);
        debug_only_v(printf("string<%p> ", (void*)(*(JSString**)slot));)
        return;
      case JSVAL_TNULL:
        JS_ASSERT(tag == JSVAL_OBJECT);
        *(JSObject**)slot = NULL;
        debug_only_v(printf("null ");)
        return;
      case JSVAL_BOOLEAN:
        
        JS_ASSERT(tag == JSVAL_BOOLEAN);
        *(JSBool*)slot = JSVAL_TO_PSEUDO_BOOLEAN(v);
        debug_only_v(printf("boolean<%d> ", *(JSBool*)slot);)
        return;
      case JSVAL_TFUN: {
        JS_ASSERT(tag == JSVAL_OBJECT);
        JSObject* obj = JSVAL_TO_OBJECT(v);
        *(JSObject**)slot = obj;
#ifdef DEBUG
        JSFunction* fun = GET_FUNCTION_PRIVATE(cx, obj);
        debug_only_v(printf("function<%p:%s> ", (void*) obj,
                            fun->atom
                            ? JS_GetStringBytes(ATOM_TO_STRING(fun->atom))
                            : "unnamed");)
#endif
        return;
      }
    }

    JS_NOT_REACHED("unexpected type");
}



static jsval
AllocateDoubleFromReservedPool(JSContext* cx)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    JS_ASSERT(tm->reservedDoublePoolPtr > tm->reservedDoublePool);
    return *--tm->reservedDoublePoolPtr;
}

static bool
js_ReplenishReservedPool(JSContext* cx, JSTraceMonitor* tm)
{
    
    JS_ASSERT((size_t) (tm->reservedDoublePoolPtr - tm->reservedDoublePool) <
              MAX_NATIVE_STACK_SLOTS);

    



    JSRuntime* rt = cx->runtime;
    uintN gcNumber = rt->gcNumber;
    uintN lastgcNumber = gcNumber;
    jsval* ptr = tm->reservedDoublePoolPtr;
    while (ptr < tm->reservedDoublePool + MAX_NATIVE_STACK_SLOTS) {
        if (!js_NewDoubleInRootedValue(cx, 0.0, ptr))
            goto oom;

        
        if (rt->gcNumber != lastgcNumber) {
            lastgcNumber = rt->gcNumber;
            JS_ASSERT(tm->reservedDoublePoolPtr == tm->reservedDoublePool);
            ptr = tm->reservedDoublePool;

            



            if (uintN(rt->gcNumber - gcNumber) > uintN(1))
                goto oom;
            continue;
        }
        ++ptr;
    }
    tm->reservedDoublePoolPtr = ptr;
    return true;

oom:
    



    tm->reservedDoublePoolPtr = tm->reservedDoublePool;
    return false;
}




static void
NativeToValue(JSContext* cx, jsval& v, uint8 type, double* slot)
{
    jsint i;
    jsdouble d;
    switch (type) {
      case JSVAL_OBJECT:
        v = OBJECT_TO_JSVAL(*(JSObject**)slot);
        JS_ASSERT(JSVAL_TAG(v) == JSVAL_OBJECT); 
        JS_ASSERT(v != JSVAL_ERROR_COOKIE); 
        debug_only_v(printf("object<%p:%s> ", (void*)JSVAL_TO_OBJECT(v),
                            JSVAL_IS_NULL(v)
                            ? "null"
                            : STOBJ_GET_CLASS(JSVAL_TO_OBJECT(v))->name);)
        break;
      case JSVAL_INT:
        i = *(jsint*)slot;
        debug_only_v(printf("int<%d> ", i);)
      store_int:
        if (INT_FITS_IN_JSVAL(i)) {
            v = INT_TO_JSVAL(i);
            break;
        }
        d = (jsdouble)i;
        goto store_double;
      case JSVAL_DOUBLE:
        d = *slot;
        debug_only_v(printf("double<%g> ", d);)
        if (JSDOUBLE_IS_INT(d, i))
            goto store_int;
      store_double: {
        

        if (cx->doubleFreeList) {
#ifdef DEBUG
            JSBool ok =
#endif
                js_NewDoubleInRootedValue(cx, d, &v);
            JS_ASSERT(ok);
            return;
        }
        v = AllocateDoubleFromReservedPool(cx);
        JS_ASSERT(JSVAL_IS_DOUBLE(v) && *JSVAL_TO_DOUBLE(v) == 0.0);
        *JSVAL_TO_DOUBLE(v) = d;
        return;
      }
      case JSVAL_BOXED:
        v = *(jsval*)slot;
        JS_ASSERT(v != JSVAL_ERROR_COOKIE); 
        debug_only_v(printf("box<%p> ", (void*)v));
        break;
      case JSVAL_STRING:
        v = STRING_TO_JSVAL(*(JSString**)slot);
        JS_ASSERT(JSVAL_TAG(v) == JSVAL_STRING); 
        debug_only_v(printf("string<%p> ", (void*)(*(JSString**)slot));)
        break;
      case JSVAL_TNULL:
        JS_ASSERT(*(JSObject**)slot == NULL);
        v = JSVAL_NULL;
        debug_only_v(printf("null<%p> ", (void*)(*(JSObject**)slot)));
        break;
      case JSVAL_BOOLEAN:
        
        v = PSEUDO_BOOLEAN_TO_JSVAL(*(JSBool*)slot);
        debug_only_v(printf("boolean<%d> ", *(JSBool*)slot);)
        break;
      case JSVAL_TFUN: {
        JS_ASSERT(HAS_FUNCTION_CLASS(*(JSObject**)slot));
        v = OBJECT_TO_JSVAL(*(JSObject**)slot);
#ifdef DEBUG
        JSFunction* fun = GET_FUNCTION_PRIVATE(cx, JSVAL_TO_OBJECT(v));
        debug_only_v(printf("function<%p:%s> ", (void*)JSVAL_TO_OBJECT(v),
                            fun->atom
                            ? JS_GetStringBytes(ATOM_TO_STRING(fun->atom))
                            : "unnamed");)
#endif
        break;
      }
    }
}


static JS_REQUIRES_STACK void
BuildNativeGlobalFrame(JSContext* cx, unsigned ngslots, uint16* gslots, uint8* mp, double* np)
{
    debug_only_v(printf("global: ");)
    FORALL_GLOBAL_SLOTS(cx, ngslots, gslots,
        ValueToNative(cx, *vp, *mp, np + gslots[n]);
        ++mp;
    );
    debug_only_v(printf("\n");)
}


static JS_REQUIRES_STACK void
BuildNativeStackFrame(JSContext* cx, unsigned callDepth, uint8* mp, double* np)
{
    debug_only_v(printf("stack: ");)
    FORALL_SLOTS_IN_PENDING_FRAMES(cx, callDepth,
        debug_only_v(printf("%s%u=", vpname, vpnum);)
        ValueToNative(cx, *vp, *mp, np);
        ++mp; ++np;
    );
    debug_only_v(printf("\n");)
}


static JS_REQUIRES_STACK int
FlushNativeGlobalFrame(JSContext* cx, unsigned ngslots, uint16* gslots, uint8* mp, double* np)
{
    uint8* mp_base = mp;
    FORALL_GLOBAL_SLOTS(cx, ngslots, gslots,
        debug_only_v(printf("%s%u=", vpname, vpnum);)
        NativeToValue(cx, *vp, *mp, np + gslots[n]);
        ++mp;
    );
    debug_only_v(printf("\n");)
    return mp - mp_base;
}






uint32 JS_FASTCALL
js_GetUpvarOnTrace(JSContext *cx, uint32 level, uint32 cookie, double* result)
{
    uintN skip = UPVAR_FRAME_SKIP(cookie);
    InterpState* state = cx->interpState;
    uintN callDepth = state->rp - state->callstackBase;

    



    if (skip > callDepth) {
        jsval v = js_GetUpvar(cx, level, cookie);
        uint8 type = getCoercedType(v);
        ValueToNative(cx, v, type, result);
        return type;
    }

    




    uintN frameIndex = callDepth - skip; 
    uintN nativeStackFramePos = 0; 
    for (uintN i = 0; i < frameIndex; ++i)
        nativeStackFramePos += state->callstackBase[i]->s.spdist;
    FrameInfo* fi = state->callstackBase[frameIndex];
    uint8* typemap = (uint8*) (fi+1);

    uintN slot = UPVAR_FRAME_SLOT(cookie);
    slot = slot == CALLEE_UPVAR_SLOT ? 0 : slot + 2;
    *result = state->stackBase[nativeStackFramePos + slot];
    return typemap[slot];
}
















static JS_REQUIRES_STACK int
FlushNativeStackFrame(JSContext* cx, unsigned callDepth, uint8* mp, double* np,
                      JSStackFrame* stopFrame)
{
    jsval* stopAt = stopFrame ? &stopFrame->argv[-2] : NULL;
    uint8* mp_base = mp;
    
    FORALL_SLOTS_IN_PENDING_FRAMES(cx, callDepth,
        if (vp == stopAt) goto skip;
        debug_only_v(printf("%s%u=", vpname, vpnum);)
        NativeToValue(cx, *vp, *mp, np);
        ++mp; ++np
    );
skip:
    
    
    
    {
        unsigned n = callDepth+1; 
        JSStackFrame* fp = cx->fp;
        if (stopFrame) {
            for (; fp != stopFrame; fp = fp->down) {
                JS_ASSERT(n != 0);
                --n;
            }
            
            JS_ASSERT(n != 0);
            --n;
            fp = fp->down;
        }
        for (; n != 0; fp = fp->down) {
            --n;
            if (fp->callee) {
                JS_ASSERT(JSVAL_IS_OBJECT(fp->argv[-1]));
                fp->thisp = JSVAL_TO_OBJECT(fp->argv[-1]);
                if (fp->flags & JSFRAME_CONSTRUCTING) 
                    fp->flags |= JSFRAME_COMPUTED_THIS;
            }
        }
    }
    debug_only_v(printf("\n");)
    return mp - mp_base;
}


JS_REQUIRES_STACK void
TraceRecorder::import(LIns* base, ptrdiff_t offset, jsval* p, uint8 t,
                      const char *prefix, uintN index, JSStackFrame *fp)
{
    LIns* ins;
    if (t == JSVAL_INT) { 
        JS_ASSERT(isInt32(*p));
        



        ins = lir->insLoadi(base, offset);
        ins = lir->ins1(LIR_i2f, ins);
    } else {
        JS_ASSERT_IF(t != JSVAL_BOXED, isNumber(*p) == (t == JSVAL_DOUBLE));
        if (t == JSVAL_DOUBLE) {
            ins = lir->insLoad(LIR_ldq, base, offset);
        } else if (t == JSVAL_BOOLEAN) {
            ins = lir->insLoad(LIR_ld, base, offset);
        } else {
            ins = lir->insLoad(LIR_ldp, base, offset);
        }
    }
    checkForGlobalObjectReallocation();
    tracker.set(p, ins);

#ifdef DEBUG
    char name[64];
    JS_ASSERT(strlen(prefix) < 10);
    void* mark = NULL;
    jsuword* localNames = NULL;
    const char* funName = NULL;
    if (*prefix == 'a' || *prefix == 'v') {
        mark = JS_ARENA_MARK(&cx->tempPool);
        if (JS_GET_LOCAL_NAME_COUNT(fp->fun) != 0)
            localNames = js_GetLocalNameArray(cx, fp->fun, &cx->tempPool);
        funName = fp->fun->atom ? js_AtomToPrintableString(cx, fp->fun->atom) : "<anonymous>";
    }
    if (!strcmp(prefix, "argv")) {
        if (index < fp->fun->nargs) {
            JSAtom *atom = JS_LOCAL_NAME_TO_ATOM(localNames[index]);
            JS_snprintf(name, sizeof name, "$%s.%s", funName, js_AtomToPrintableString(cx, atom));
        } else {
            JS_snprintf(name, sizeof name, "$%s.<arg%d>", funName, index);
        }
    } else if (!strcmp(prefix, "vars")) {
        JSAtom *atom = JS_LOCAL_NAME_TO_ATOM(localNames[fp->fun->nargs + index]);
        JS_snprintf(name, sizeof name, "$%s.%s", funName, js_AtomToPrintableString(cx, atom));
    } else {
        JS_snprintf(name, sizeof name, "$%s%d", prefix, index);
    }

    if (mark)
        JS_ARENA_RELEASE(&cx->tempPool, mark);
    addName(ins, name);

    static const char* typestr[] = {
        "object", "int", "double", "boxed", "string", "null", "boolean", "function"
    };
    debug_only_v(printf("import vp=%p name=%s type=%s flags=%d\n",
                        (void*)p, name, typestr[t & 7], t >> 3);)
#endif
}

JS_REQUIRES_STACK void
TraceRecorder::import(TreeInfo* treeInfo, LIns* sp, unsigned stackSlots, unsigned ngslots,
                      unsigned callDepth, uint8* typeMap)
{
    










    uint8* globalTypeMap = typeMap + stackSlots;
    unsigned length = treeInfo->nGlobalTypes();

    



    if (ngslots < length) {
        mergeTypeMaps(&globalTypeMap, &ngslots,
                      treeInfo->globalTypeMap(), length,
                      (uint8*)alloca(sizeof(uint8) * length));
    }
    JS_ASSERT(ngslots == treeInfo->nGlobalTypes());

    



    ptrdiff_t offset = -treeInfo->nativeStackBase;
    uint8* m = typeMap;
    FORALL_SLOTS_IN_PENDING_FRAMES(cx, callDepth,
        if (*m == JSVAL_BOXED) {
            import(sp, offset, vp, JSVAL_BOXED, "boxed", vpnum, cx->fp);
            LIns* vp_ins = get(vp);
            unbox_jsval(*vp, vp_ins, copy(anchor));
            set(vp, vp_ins);
        }
        m++; offset += sizeof(double);
    );

    


    uint16* gslots = treeInfo->globalSlots->data();
    m = globalTypeMap;
    FORALL_GLOBAL_SLOTS(cx, ngslots, gslots,
        JS_ASSERT(*m != JSVAL_BOXED);
        import(lirbuf->state, nativeGlobalOffset(vp), vp, *m, vpname, vpnum, NULL);
        m++;
    );
    offset = -treeInfo->nativeStackBase;
    m = typeMap;
    FORALL_SLOTS_IN_PENDING_FRAMES(cx, callDepth,
        if (*m != JSVAL_BOXED)
            import(sp, offset, vp, *m, vpname, vpnum, fp);
        m++; offset += sizeof(double);
    );
}

JS_REQUIRES_STACK bool
TraceRecorder::isValidSlot(JSScope* scope, JSScopeProperty* sprop)
{
    uint32 setflags = (js_CodeSpec[*cx->fp->regs->pc].format & (JOF_SET | JOF_INCDEC | JOF_FOR));

    if (setflags) {
        if (!SPROP_HAS_STUB_SETTER(sprop))
            ABORT_TRACE_RV("non-stub setter", false);
        if (sprop->attrs & JSPROP_READONLY)
            ABORT_TRACE_RV("writing to a read-only property", false);
    }
    
    if (setflags != JOF_SET && !SPROP_HAS_STUB_GETTER(sprop))
        ABORT_TRACE_RV("non-stub getter", false);

    if (!SPROP_HAS_VALID_SLOT(sprop, scope))
        ABORT_TRACE_RV("slotless obj property", false);

    return true;
}


JS_REQUIRES_STACK bool
TraceRecorder::lazilyImportGlobalSlot(unsigned slot)
{
    if (slot != uint16(slot)) 
        return false;
    



    if (STOBJ_NSLOTS(globalObj) > MAX_GLOBAL_SLOTS)
        return false;
    jsval* vp = &STOBJ_GET_SLOT(globalObj, slot);
    if (known(vp))
        return true; 
    unsigned index = treeInfo->globalSlots->length();
    
    JS_ASSERT(treeInfo->nGlobalTypes() == treeInfo->globalSlots->length());
    treeInfo->globalSlots->add(slot);
    uint8 type = getCoercedType(*vp);
    if ((type == JSVAL_INT) && oracle.isGlobalSlotUndemotable(cx, slot))
        type = JSVAL_DOUBLE;
    treeInfo->typeMap.add(type);
    import(lirbuf->state, sizeof(struct InterpState) + slot*sizeof(double),
           vp, type, "global", index, NULL);
    specializeTreesToMissingGlobals(cx, treeInfo);
    return true;
}


LIns*
TraceRecorder::writeBack(LIns* i, LIns* base, ptrdiff_t offset)
{
    


    if (isPromoteInt(i))
        i = ::demote(lir, i);
    return lir->insStorei(i, base, offset);
}


JS_REQUIRES_STACK void
TraceRecorder::set(jsval* p, LIns* i, bool initializing)
{
    JS_ASSERT(i != NULL);
    JS_ASSERT(initializing || known(p));
    checkForGlobalObjectReallocation();
    tracker.set(p, i);
    


    LIns* x = nativeFrameTracker.get(p);
    if (!x) {
        if (isGlobal(p))
            x = writeBack(i, lirbuf->state, nativeGlobalOffset(p));
        else
            x = writeBack(i, lirbuf->sp, -treeInfo->nativeStackBase + nativeStackOffset(p));
        nativeFrameTracker.set(p, x);
    } else {
#define ASSERT_VALID_CACHE_HIT(base, offset)                                  \
    JS_ASSERT(base == lirbuf->sp || base == lirbuf->state);                   \
    JS_ASSERT(offset == ((base == lirbuf->sp)                                 \
        ? -treeInfo->nativeStackBase + nativeStackOffset(p)                   \
        : nativeGlobalOffset(p)));                                            \

        JS_ASSERT(x->isop(LIR_sti) || x->isop(LIR_stqi));
        ASSERT_VALID_CACHE_HIT(x->oprnd2(), x->immdisp());
        writeBack(i, x->oprnd2(), x->immdisp());
    }
#undef ASSERT_VALID_CACHE_HIT
}

JS_REQUIRES_STACK LIns*
TraceRecorder::get(jsval* p)
{
    checkForGlobalObjectReallocation();
    return tracker.get(p);
}

JS_REQUIRES_STACK bool
TraceRecorder::known(jsval* p)
{
    checkForGlobalObjectReallocation();
    return tracker.has(p);
}






JS_REQUIRES_STACK void
TraceRecorder::checkForGlobalObjectReallocation()
{
    if (global_dslots != globalObj->dslots) {
        debug_only_v(printf("globalObj->dslots relocated, updating tracker\n");)
        jsval* src = global_dslots;
        jsval* dst = globalObj->dslots;
        jsuint length = globalObj->dslots[-1] - JS_INITIAL_NSLOTS;
        LIns** map = (LIns**)alloca(sizeof(LIns*) * length);
        for (jsuint n = 0; n < length; ++n) {
            map[n] = tracker.get(src);
            tracker.set(src++, NULL);
        }
        for (jsuint n = 0; n < length; ++n)
            tracker.set(dst++, map[n]);
        global_dslots = globalObj->dslots;
    }
}


static JS_REQUIRES_STACK bool
js_IsLoopEdge(jsbytecode* pc, jsbytecode* header)
{
    switch (*pc) {
      case JSOP_IFEQ:
      case JSOP_IFNE:
        return ((pc + GET_JUMP_OFFSET(pc)) == header);
      case JSOP_IFEQX:
      case JSOP_IFNEX:
        return ((pc + GET_JUMPX_OFFSET(pc)) == header);
      default:
        JS_ASSERT((*pc == JSOP_AND) || (*pc == JSOP_ANDX) ||
                  (*pc == JSOP_OR) || (*pc == JSOP_ORX));
    }
    return false;
}






JS_REQUIRES_STACK void
TraceRecorder::adjustCallerTypes(Fragment* f)
{
    uint16* gslots = treeInfo->globalSlots->data();
    unsigned ngslots = treeInfo->globalSlots->length();
    JS_ASSERT(ngslots == treeInfo->nGlobalTypes());
    TreeInfo* ti = (TreeInfo*)f->vmprivate;
    uint8* map = ti->globalTypeMap();
    uint8* m = map;
    FORALL_GLOBAL_SLOTS(cx, ngslots, gslots,
        LIns* i = get(vp);
        bool isPromote = isPromoteInt(i);
        if (isPromote && *m == JSVAL_DOUBLE)
            lir->insStorei(get(vp), lirbuf->state, nativeGlobalOffset(vp));
        JS_ASSERT(!(!isPromote && *m == JSVAL_INT));
        ++m;
    );
    JS_ASSERT(unsigned(m - map) == ti->nGlobalTypes());
    map = ti->stackTypeMap();
    m = map;
    FORALL_SLOTS_IN_PENDING_FRAMES(cx, 0,
        LIns* i = get(vp);
        bool isPromote = isPromoteInt(i);
        if (isPromote && *m == JSVAL_DOUBLE) {
            lir->insStorei(get(vp), lirbuf->sp,
                           -treeInfo->nativeStackBase + nativeStackOffset(vp));
            
            oracle.markStackSlotUndemotable(cx, unsigned(m - map));
        }
        JS_ASSERT(!(!isPromote && *m == JSVAL_INT));
        ++m;
    );
    JS_ASSERT(unsigned(m - map) == ti->nStackTypes);
    JS_ASSERT(f == f->root);
}

JS_REQUIRES_STACK uint8
TraceRecorder::determineSlotType(jsval* vp)
{
    uint8 m;
    LIns* i = get(vp);
    if (isNumber(*vp)) {
        m = isPromoteInt(i) ? JSVAL_INT : JSVAL_DOUBLE;
    } else if (JSVAL_IS_OBJECT(*vp)) {
        if (JSVAL_IS_NULL(*vp))
            m = JSVAL_TNULL;
        else if (HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(*vp)))
            m = JSVAL_TFUN;
        else
            m = JSVAL_OBJECT;
    } else {
        m = JSVAL_TAG(*vp);
    }
    JS_ASSERT((m != JSVAL_INT) || isInt32(*vp));
    return m;
}

JS_REQUIRES_STACK VMSideExit*
TraceRecorder::snapshot(ExitType exitType)
{
    JSStackFrame* fp = cx->fp;
    JSFrameRegs* regs = fp->regs;
    jsbytecode* pc = regs->pc;

    
    const JSCodeSpec& cs = js_CodeSpec[*pc];

    




    bool resumeAfter = (pendingTraceableNative &&
                        JSTN_ERRTYPE(pendingTraceableNative) == FAIL_STATUS);
    if (resumeAfter) {
        JS_ASSERT(*pc == JSOP_CALL || *pc == JSOP_APPLY || *pc == JSOP_NEW);
        pc += cs.length;
        regs->pc = pc;
        MUST_FLOW_THROUGH("restore_pc");
    }

    
    unsigned stackSlots = js_NativeStackSlots(cx, callDepth);

    

    trackNativeStackUse(stackSlots + 1);

    
    unsigned ngslots = treeInfo->globalSlots->length();
    unsigned typemap_size = (stackSlots + ngslots) * sizeof(uint8);
    uint8* typemap = (uint8*)alloca(typemap_size);
    uint8* m = typemap;

    


    FORALL_SLOTS(cx, ngslots, treeInfo->globalSlots->data(), callDepth,
        *m++ = determineSlotType(vp);
    );
    JS_ASSERT(unsigned(m - typemap) == ngslots + stackSlots);

    



    if (pendingTraceableNative && (pendingTraceableNative->flags & JSTN_UNBOX_AFTER))
        typemap[stackSlots - 1] = JSVAL_BOXED;

    
    if (resumeAfter) {
        MUST_FLOW_LABEL(restore_pc);
        regs->pc = pc - cs.length;
    } else {
        


        if (*pc == JSOP_GOTO)
            pc += GET_JUMP_OFFSET(pc);
        else if (*pc == JSOP_GOTOX)
            pc += GET_JUMPX_OFFSET(pc);
    }

    



    VMSideExit** exits = treeInfo->sideExits.data();
    unsigned nexits = treeInfo->sideExits.length();
    if (exitType == LOOP_EXIT) {
        for (unsigned n = 0; n < nexits; ++n) {
            VMSideExit* e = exits[n];
            if (e->pc == pc && e->imacpc == fp->imacpc &&
                !memcmp(getFullTypeMap(exits[n]), typemap, typemap_size)) {
                AUDIT(mergedLoopExits);
                return e;
            }
        }
    }

    if (sizeof(VMSideExit) + (stackSlots + ngslots) * sizeof(uint8) >= MAX_SKIP_BYTES) {
        







        stackSlots = 0;
        ngslots = 0;
        trashSelf = true;
    }

    
    LIns* data = lir->insSkip(sizeof(VMSideExit) + (stackSlots + ngslots) * sizeof(uint8));
    VMSideExit* exit = (VMSideExit*) data->payload();

    
    memset(exit, 0, sizeof(VMSideExit));
    exit->from = fragment;
    exit->calldepth = callDepth;
    exit->numGlobalSlots = ngslots;
    exit->numStackSlots = stackSlots;
    exit->numStackSlotsBelowCurrentFrame = cx->fp->callee
        ? nativeStackOffset(&cx->fp->argv[-2])/sizeof(double)
        : 0;
    exit->exitType = exitType;
    exit->block = fp->blockChain;
    exit->pc = pc;
    exit->imacpc = fp->imacpc;
    exit->sp_adj = (stackSlots * sizeof(double)) - treeInfo->nativeStackBase;
    exit->rp_adj = exit->calldepth * sizeof(FrameInfo*);
    exit->nativeCalleeWord = 0;
    memcpy(getFullTypeMap(exit), typemap, typemap_size);
    return exit;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::createGuardRecord(VMSideExit* exit)
{
    LIns* guardRec = lir->insSkip(sizeof(GuardRecord));
    GuardRecord* gr = (GuardRecord*) guardRec->payload();

    memset(gr, 0, sizeof(GuardRecord));
    gr->exit = exit;
    exit->addGuard(gr);

    return guardRec;
}





JS_REQUIRES_STACK void
TraceRecorder::guard(bool expected, LIns* cond, VMSideExit* exit)
{
    LIns* guardRec = createGuardRecord(exit);

    





    if (exit->exitType == LOOP_EXIT)
        treeInfo->sideExits.add(exit);

    if (!cond->isCond()) {
        expected = !expected;
        cond = lir->ins_eq0(cond);
    }

    LIns* guardIns =
        lir->insGuard(expected ? LIR_xf : LIR_xt, cond, guardRec);
    if (guardIns) {
        debug_only_v(printf("    SideExit=%p exitType=%d\n", (void*)exit, exit->exitType);)
    } else {
        debug_only_v(printf("    redundant guard, eliminated\n");)
    }
}

JS_REQUIRES_STACK VMSideExit*
TraceRecorder::copy(VMSideExit* copy)
{
    size_t typemap_size = copy->numGlobalSlots + copy->numStackSlots;
    LIns* data = lir->insSkip(sizeof(VMSideExit) + typemap_size * sizeof(uint8));
    VMSideExit* exit = (VMSideExit*) data->payload();

    
    memcpy(exit, copy, sizeof(VMSideExit) + typemap_size * sizeof(uint8));
    exit->guards = NULL;
    exit->from = fragment;
    exit->target = NULL;

    





    if (exit->exitType == LOOP_EXIT)
        treeInfo->sideExits.add(exit);
    return exit;
}



JS_REQUIRES_STACK void
TraceRecorder::guard(bool expected, LIns* cond, ExitType exitType)
{
    guard(expected, cond, snapshot(exitType));
}











JS_REQUIRES_STACK bool
TraceRecorder::checkType(jsval& v, uint8 t, jsval*& stage_val, LIns*& stage_ins,
                         unsigned& stage_count)
{
    if (t == JSVAL_INT) { 
        debug_only_v(printf("checkType(tag=1, t=%d, isnum=%d, i2f=%d) stage_count=%d\n",
                            t,
                            isNumber(v),
                            isPromoteInt(get(&v)),
                            stage_count);)
        if (!isNumber(v))
            return false; 
        LIns* i = get(&v);
        
        if (!isPromoteInt(i))
            return false;
        
        JS_ASSERT(isInt32(v) && isPromoteInt(i));
        
        stage_val = &v;
        stage_ins = f2i(i);
        stage_count++;
        return true;
    }
    if (t == JSVAL_DOUBLE) {
        debug_only_v(printf("checkType(tag=2, t=%d, isnum=%d, promote=%d) stage_count=%d\n",
                            t,
                            isNumber(v),
                            isPromoteInt(get(&v)),
                            stage_count);)
        if (!isNumber(v))
            return false; 
        LIns* i = get(&v);
        

        if (isPromoteInt(i)) {
            stage_val = &v;
            stage_ins = lir->ins1(LIR_i2f, i);
            stage_count++;
        }
        return true;
    }
    if (t == JSVAL_TNULL)
        return JSVAL_IS_NULL(v);
    if (t == JSVAL_TFUN)
        return !JSVAL_IS_PRIMITIVE(v) && HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v));
    if (t == JSVAL_OBJECT)
        return !JSVAL_IS_PRIMITIVE(v) && !HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v));

    
    uint8 vt = getCoercedType(v);
#ifdef DEBUG
    if (vt != t) {
        debug_only_v(printf("Type mismatch: val %c, map %c ", typeChar[vt],
                            typeChar[t]);)
    }
#endif
    debug_only_v(printf("checkType(vt=%d, t=%d) stage_count=%d\n",
                        (int) vt, t, stage_count);)
    return vt == t;
}










JS_REQUIRES_STACK bool
TraceRecorder::deduceTypeStability(Fragment* root_peer, Fragment** stable_peer, bool& demote)
{
    uint8* m;
    uint8* typemap;
    unsigned ngslots = treeInfo->globalSlots->length();
    uint16* gslots = treeInfo->globalSlots->data();
    JS_ASSERT(ngslots == treeInfo->nGlobalTypes());

    if (stable_peer)
        *stable_peer = NULL;

    



    bool success;
    unsigned stage_count;
    jsval** stage_vals = (jsval**)alloca(sizeof(jsval*) * (treeInfo->typeMap.length()));
    LIns** stage_ins = (LIns**)alloca(sizeof(LIns*) * (treeInfo->typeMap.length()));

    
    stage_count = 0;
    success = false;

    debug_only_v(printf("Checking type stability against self=%p\n", (void*)fragment);)

    m = typemap = treeInfo->globalTypeMap();
    FORALL_GLOBAL_SLOTS(cx, ngslots, gslots,
        debug_only_v(printf("%s%d ", vpname, vpnum);)
        if (!checkType(*vp, *m, stage_vals[stage_count], stage_ins[stage_count], stage_count)) {
            
            if (*m == JSVAL_INT && isNumber(*vp) && !isPromoteInt(get(vp))) {
                oracle.markGlobalSlotUndemotable(cx, gslots[n]);
                demote = true;
            } else {
                goto checktype_fail_1;
            }
        }
        ++m;
    );
    m = typemap = treeInfo->stackTypeMap();
    FORALL_SLOTS_IN_PENDING_FRAMES(cx, 0,
        debug_only_v(printf("%s%d ", vpname, vpnum);)
        if (!checkType(*vp, *m, stage_vals[stage_count], stage_ins[stage_count], stage_count)) {
            if (*m == JSVAL_INT && isNumber(*vp) && !isPromoteInt(get(vp))) {
                oracle.markStackSlotUndemotable(cx, unsigned(m - typemap));
                demote = true;
            } else {
                goto checktype_fail_1;
            }
        }
        ++m;
    );

    success = true;

checktype_fail_1:
    
    if (success && !demote) {
        for (unsigned i = 0; i < stage_count; i++)
            set(stage_vals[i], stage_ins[i]);
        return true;
    
    } else if (trashSelf) {
        return false;
    }

    demote = false;

    


    Fragment* f;
    TreeInfo* ti;
    for (f = root_peer; f != NULL; f = f->peer) {
        debug_only_v(printf("Checking type stability against peer=%p (code=%p)\n", (void*)f, f->code());)
        if (!f->code())
            continue;
        ti = (TreeInfo*)f->vmprivate;
        
        if ((ti->nStackTypes != treeInfo->nStackTypes) ||
            (ti->typeMap.length() != treeInfo->typeMap.length()) ||
            (ti->globalSlots->length() != treeInfo->globalSlots->length()))
            continue;
        stage_count = 0;
        success = false;

        m = ti->globalTypeMap();
        FORALL_GLOBAL_SLOTS(cx, treeInfo->globalSlots->length(), treeInfo->globalSlots->data(),
                if (!checkType(*vp, *m, stage_vals[stage_count], stage_ins[stage_count], stage_count))
                    goto checktype_fail_2;
                ++m;
            );

        m = ti->stackTypeMap();
        FORALL_SLOTS_IN_PENDING_FRAMES(cx, 0,
                if (!checkType(*vp, *m, stage_vals[stage_count], stage_ins[stage_count], stage_count))
                    goto checktype_fail_2;
                ++m;
            );

        success = true;

checktype_fail_2:
        if (success) {
            



            for (unsigned i = 0; i < stage_count; i++)
                set(stage_vals[i], stage_ins[i]);
            if (stable_peer)
                *stable_peer = f;
            demote = false;
            return false;
        }
    }

    




    if (demote && fragment->kind == LoopTrace) {
        typemap = m = treeInfo->globalTypeMap();
        FORALL_GLOBAL_SLOTS(cx, treeInfo->globalSlots->length(), treeInfo->globalSlots->data(),
            if (*m == JSVAL_INT) {
                JS_ASSERT(isNumber(*vp));
                if (!isPromoteInt(get(vp)))
                    oracle.markGlobalSlotUndemotable(cx, gslots[n]);
            } else if (*m == JSVAL_DOUBLE) {
                JS_ASSERT(isNumber(*vp));
                oracle.markGlobalSlotUndemotable(cx, gslots[n]);
            } else {
                JS_ASSERT(*m == JSVAL_TAG(*vp));
            }
            m++;
        );

        typemap = m = treeInfo->stackTypeMap();
        FORALL_SLOTS_IN_PENDING_FRAMES(cx, 0,
            if (*m == JSVAL_INT) {
                JS_ASSERT(isNumber(*vp));
                if (!isPromoteInt(get(vp)))
                    oracle.markStackSlotUndemotable(cx, unsigned(m - typemap));
            } else if (*m == JSVAL_DOUBLE) {
                JS_ASSERT(isNumber(*vp));
                oracle.markStackSlotUndemotable(cx, unsigned(m - typemap));
            } else {
                JS_ASSERT((*m == JSVAL_TNULL)
                          ? JSVAL_IS_NULL(*vp)
                          : *m == JSVAL_TFUN
                          ? !JSVAL_IS_PRIMITIVE(*vp) && HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(*vp))
                          : *m == JSVAL_TAG(*vp));
            }
            m++;
        );
        return true;
    } else {
        demote = false;
    }

    return false;
}

static JS_REQUIRES_STACK void
FlushJITCache(JSContext* cx)
{
    if (!TRACING_ENABLED(cx))
        return;
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    debug_only_v(printf("Flushing cache.\n");)
    if (tm->recorder)
        js_AbortRecording(cx, "flush cache");
    TraceRecorder* tr;
    while ((tr = tm->abortStack) != NULL) {
        tr->removeFragmentoReferences();
        tr->deepAbort();
        tr->popAbortStack();
    }
    Fragmento* fragmento = tm->fragmento;
    if (fragmento) {
        if (tm->prohibitFlush) {
            debug_only_v(printf("Deferring fragmento flush due to deep bail.\n");)
            tm->needFlush = JS_TRUE;
            return;
        }

        fragmento->clearFrags();
#ifdef DEBUG
        JS_ASSERT(fragmento->labels);
        fragmento->labels->clear();
#endif
        tm->lirbuf->rewind();
        for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
            VMFragment* f = tm->vmfragments[i];
            while (f) {
                VMFragment* next = f->next;
                fragmento->clearFragment(f);
                f = next;
            }
            tm->vmfragments[i] = NULL;
        }
        for (size_t i = 0; i < MONITOR_N_GLOBAL_STATES; ++i) {
            tm->globalStates[i].globalShape = -1;
            tm->globalStates[i].globalSlots->clear();
        }
    }
    tm->needFlush = JS_FALSE;
}


JS_REQUIRES_STACK void
TraceRecorder::compile(JSTraceMonitor* tm)
{
    if (tm->needFlush) {
        FlushJITCache(cx);
        return;
    }
    Fragmento* fragmento = tm->fragmento;
    if (treeInfo->maxNativeStackSlots >= MAX_NATIVE_STACK_SLOTS) {
        debug_only_v(printf("Blacklist: excessive stack use.\n"));
        js_Blacklist((jsbytecode*) fragment->root->ip);
        return;
    }
    if (anchor && anchor->exitType != CASE_EXIT)
        ++treeInfo->branchCount;
    if (lirbuf->outOMem()) {
        fragmento->assm()->setError(nanojit::OutOMem);
        return;
    }
    ::compile(fragmento->assm(), fragment);
    if (fragmento->assm()->error() == nanojit::OutOMem)
        return;
    if (fragmento->assm()->error() != nanojit::None) {
        debug_only_v(printf("Blacklisted: error during compilation\n");)
        js_Blacklist((jsbytecode*) fragment->root->ip);
        return;
    }
    js_resetRecordingAttempts(cx, (jsbytecode*) fragment->ip);
    js_resetRecordingAttempts(cx, (jsbytecode*) fragment->root->ip);
    if (anchor) {
#ifdef NANOJIT_IA32
        if (anchor->exitType == CASE_EXIT)
            fragmento->assm()->patch(anchor, anchor->switchInfo);
        else
#endif
            fragmento->assm()->patch(anchor);
    }
    JS_ASSERT(fragment->code());
    JS_ASSERT(!fragment->vmprivate);
    if (fragment == fragment->root)
        fragment->vmprivate = treeInfo;
    
#if defined DEBUG && !defined WIN32
    const char* filename = cx->fp->script->filename;
    char* label = (char*)malloc((filename ? strlen(filename) : 7) + 16);
    sprintf(label, "%s:%u", filename ? filename : "<stdin>",
            js_FramePCToLineNumber(cx, cx->fp));
    fragmento->labels->add(fragment, sizeof(Fragment), 0, label);
    free(label);
#endif
    AUDIT(traceCompleted);
}

static bool
js_JoinPeersIfCompatible(Fragmento* frago, Fragment* stableFrag, TreeInfo* stableTree,
                         VMSideExit* exit)
{
    JS_ASSERT(exit->numStackSlots == stableTree->nStackTypes);

    
    if ((exit->numGlobalSlots + exit->numStackSlots != stableTree->typeMap.length()) ||
        memcmp(getFullTypeMap(exit), stableTree->typeMap.data(), stableTree->typeMap.length())) {
       return false;
    }

    exit->target = stableFrag;
    frago->assm()->patch(exit);

    stableTree->dependentTrees.addUnique(exit->from->root);
    ((TreeInfo*)exit->from->root->vmprivate)->linkedTrees.addUnique(stableFrag);

    return true;
}


JS_REQUIRES_STACK void
TraceRecorder::closeLoop(JSTraceMonitor* tm, bool& demote)
{
    




    JS_ASSERT((*cx->fp->regs->pc == JSOP_LOOP || *cx->fp->regs->pc == JSOP_NOP) && !cx->fp->imacpc);

    bool stable;
    Fragment* peer;
    VMFragment* peer_root;
    Fragmento* fragmento = tm->fragmento;

    if (callDepth != 0) {
        debug_only_v(printf("Blacklisted: stack depth mismatch, possible recursion.\n");)
        js_Blacklist((jsbytecode*) fragment->root->ip);
        trashSelf = true;
        return;
    }

    VMSideExit* exit = snapshot(UNSTABLE_LOOP_EXIT);
    JS_ASSERT(exit->numStackSlots == treeInfo->nStackTypes);

    VMFragment* root = (VMFragment*)fragment->root;
    peer_root = getLoop(traceMonitor, root->ip, root->globalObj, root->globalShape, root->argc);
    JS_ASSERT(peer_root != NULL);

    stable = deduceTypeStability(peer_root, &peer, demote);

#if DEBUG
    if (!stable)
        AUDIT(unstableLoopVariable);
#endif

    if (trashSelf) {
        debug_only_v(printf("Trashing tree from type instability.\n");)
        return;
    }

    if (stable && demote) {
        JS_ASSERT(fragment->kind == LoopTrace);
        return;
    }

    if (!stable) {
        fragment->lastIns = lir->insGuard(LIR_x, lir->insImm(1), createGuardRecord(exit));

        



        if (!peer) {
            




            debug_only_v(printf("Trace has unstable loop variable with no stable peer, "
                                "compiling anyway.\n");)
            UnstableExit* uexit = new UnstableExit;
            uexit->fragment = fragment;
            uexit->exit = exit;
            uexit->next = treeInfo->unstableExits;
            treeInfo->unstableExits = uexit;
        } else {
            JS_ASSERT(peer->code());
            exit->target = peer;
            debug_only_v(printf("Joining type-unstable trace to target fragment %p.\n", (void*)peer);)
            stable = true;
            ((TreeInfo*)peer->vmprivate)->dependentTrees.addUnique(fragment->root);
            treeInfo->linkedTrees.addUnique(peer);
        }
    } else {
        exit->target = fragment->root;
        fragment->lastIns = lir->insGuard(LIR_loop, lir->insImm(1), createGuardRecord(exit));
    }
    compile(tm);

    if (fragmento->assm()->error() != nanojit::None)
        return;

    joinEdgesToEntry(fragmento, peer_root);

    debug_only_v(printf("updating specializations on dependent and linked trees\n"))
    if (fragment->root->vmprivate)
        specializeTreesToMissingGlobals(cx, (TreeInfo*)fragment->root->vmprivate);

    



    if (outer)
        js_AttemptCompilation(cx, tm, globalObj, outer, outerArgc);
    
    debug_only_v(printf("recording completed at %s:%u@%u via closeLoop\n",
                        cx->fp->script->filename,
                        js_FramePCToLineNumber(cx, cx->fp),
                        FramePCOffset(cx->fp));)
}

JS_REQUIRES_STACK void
TraceRecorder::joinEdgesToEntry(Fragmento* fragmento, VMFragment* peer_root)
{
    if (fragment->kind == LoopTrace) {
        TreeInfo* ti;
        Fragment* peer;
        uint8* t1, *t2;
        UnstableExit* uexit, **unext;
        uint32* stackDemotes = (uint32*)alloca(sizeof(uint32) * treeInfo->nStackTypes);
        uint32* globalDemotes = (uint32*)alloca(sizeof(uint32) * treeInfo->nGlobalTypes());

        for (peer = peer_root; peer != NULL; peer = peer->peer) {
            if (!peer->code())
                continue;
            ti = (TreeInfo*)peer->vmprivate;
            uexit = ti->unstableExits;
            unext = &ti->unstableExits;
            while (uexit != NULL) {
                bool remove = js_JoinPeersIfCompatible(fragmento, fragment, treeInfo, uexit->exit);
                JS_ASSERT(!remove || fragment != peer);
                debug_only_v(if (remove) {
                             printf("Joining type-stable trace to target exit %p->%p.\n",
                                    (void*)uexit->fragment, (void*)uexit->exit); });
                if (!remove) {
                    



                    unsigned stackCount = 0;
                    unsigned globalCount = 0;
                    t1 = treeInfo->stackTypeMap();
                    t2 = getStackTypeMap(uexit->exit);
                    for (unsigned i = 0; i < uexit->exit->numStackSlots; i++) {
                        if (t2[i] == JSVAL_INT && t1[i] == JSVAL_DOUBLE) {
                            stackDemotes[stackCount++] = i;
                        } else if (t2[i] != t1[i]) {
                            stackCount = 0;
                            break;
                        }
                    }
                    t1 = treeInfo->globalTypeMap();
                    t2 = getGlobalTypeMap(uexit->exit);
                    for (unsigned i = 0; i < uexit->exit->numGlobalSlots; i++) {
                        if (t2[i] == JSVAL_INT && t1[i] == JSVAL_DOUBLE) {
                            globalDemotes[globalCount++] = i;
                        } else if (t2[i] != t1[i]) {
                            globalCount = 0;
                            stackCount = 0;
                            break;
                        }
                    }
                    if (stackCount || globalCount) {
                        for (unsigned i = 0; i < stackCount; i++)
                            oracle.markStackSlotUndemotable(cx, stackDemotes[i]);
                        for (unsigned i = 0; i < globalCount; i++)
                            oracle.markGlobalSlotUndemotable(cx, ti->globalSlots->data()[globalDemotes[i]]);
                        JS_ASSERT(peer == uexit->fragment->root);
                        if (fragment == peer)
                            trashSelf = true;
                        else
                            whichTreesToTrash.addUnique(uexit->fragment->root);
                        break;
                    }
                }
                if (remove) {
                    *unext = uexit->next;
                    delete uexit;
                    uexit = *unext;
                } else {
                    unext = &uexit->next;
                    uexit = uexit->next;
                }
            }
        }
    }

    debug_only_v(js_DumpPeerStability(traceMonitor, peer_root->ip, peer_root->globalObj, 
                                      peer_root->globalShape, peer_root->argc);)
}


JS_REQUIRES_STACK void
TraceRecorder::endLoop(JSTraceMonitor* tm)
{
    if (callDepth != 0) {
        debug_only_v(printf("Blacklisted: stack depth mismatch, possible recursion.\n");)
        js_Blacklist((jsbytecode*) fragment->root->ip);
        trashSelf = true;
        return;
    }

    fragment->lastIns =
        lir->insGuard(LIR_x, lir->insImm(1), createGuardRecord(snapshot(LOOP_EXIT)));
    compile(tm);

    if (tm->fragmento->assm()->error() != nanojit::None)
        return;

    VMFragment* root = (VMFragment*)fragment->root;
    joinEdgesToEntry(tm->fragmento, getLoop(tm, root->ip, root->globalObj, root->globalShape, root->argc));

    

    debug_only_v(printf("updating specializations on dependent and linked trees\n"))
    if (fragment->root->vmprivate)
        specializeTreesToMissingGlobals(cx, (TreeInfo*)fragment->root->vmprivate);

    



    if (outer)
        js_AttemptCompilation(cx, tm, globalObj, outer, outerArgc);
    
    debug_only_v(printf("recording completed at %s:%u@%u via endLoop\n",
                        cx->fp->script->filename,
                        js_FramePCToLineNumber(cx, cx->fp),
                        FramePCOffset(cx->fp));)
}


JS_REQUIRES_STACK void
TraceRecorder::prepareTreeCall(Fragment* inner)
{
    TreeInfo* ti = (TreeInfo*)inner->vmprivate;
    inner_sp_ins = lirbuf->sp;
    



    if (callDepth > 0) {
        

        ptrdiff_t sp_adj = nativeStackOffset(&cx->fp->argv[-2]);
        
        ptrdiff_t rp_adj = callDepth * sizeof(FrameInfo*);
        

        debug_only_v(printf("sp_adj=%d outer=%d inner=%d\n",
                          sp_adj, treeInfo->nativeStackBase, ti->nativeStackBase));
        LIns* sp_top = lir->ins2i(LIR_piadd, lirbuf->sp,
                - treeInfo->nativeStackBase 
                + sp_adj 
                + ti->maxNativeStackSlots * sizeof(double)); 
        guard(true, lir->ins2(LIR_lt, sp_top, eos_ins), OOM_EXIT);
        
        LIns* rp_top = lir->ins2i(LIR_piadd, lirbuf->rp, rp_adj +
                ti->maxCallDepth * sizeof(FrameInfo*));
        guard(true, lir->ins2(LIR_lt, rp_top, eor_ins), OOM_EXIT);
        
        lir->insStorei(inner_sp_ins = lir->ins2i(LIR_piadd, lirbuf->sp,
                - treeInfo->nativeStackBase 
                + sp_adj 
                + ti->nativeStackBase), 
                lirbuf->state, offsetof(InterpState, sp));
        lir->insStorei(lir->ins2i(LIR_piadd, lirbuf->rp, rp_adj),
                lirbuf->state, offsetof(InterpState, rp));
    }
}


JS_REQUIRES_STACK void
TraceRecorder::emitTreeCall(Fragment* inner, VMSideExit* exit)
{
    TreeInfo* ti = (TreeInfo*)inner->vmprivate;

    
    LIns* args[] = { INS_CONSTPTR(inner), lirbuf->state }; 
    LIns* ret = lir->insCall(&js_CallTree_ci, args);

    
    JS_ASSERT(!memchr(getGlobalTypeMap(exit), JSVAL_BOXED, exit->numGlobalSlots) &&
              !memchr(getStackTypeMap(exit), JSVAL_BOXED, exit->numStackSlots));
    import(ti, inner_sp_ins, exit->numStackSlots, exit->numGlobalSlots,
           exit->calldepth, getFullTypeMap(exit));

    
    if (callDepth > 0) {
        lir->insStorei(lirbuf->sp, lirbuf->state, offsetof(InterpState, sp));
        lir->insStorei(lirbuf->rp, lirbuf->state, offsetof(InterpState, rp));
    }

    



    guard(true, lir->ins2(LIR_eq, ret, INS_CONSTPTR(exit)), NESTED_EXIT);
    
    ((TreeInfo*)inner->vmprivate)->dependentTrees.addUnique(fragment->root);
    treeInfo->linkedTrees.addUnique(inner);
}


JS_REQUIRES_STACK void
TraceRecorder::trackCfgMerges(jsbytecode* pc)
{
    
    JS_ASSERT((*pc == JSOP_IFEQ) || (*pc == JSOP_IFEQX));
    jssrcnote* sn = js_GetSrcNote(cx->fp->script, pc);
    if (sn != NULL) {
        if (SN_TYPE(sn) == SRC_IF) {
            cfgMerges.add((*pc == JSOP_IFEQ)
                          ? pc + GET_JUMP_OFFSET(pc)
                          : pc + GET_JUMPX_OFFSET(pc));
        } else if (SN_TYPE(sn) == SRC_IF_ELSE)
            cfgMerges.add(pc + js_GetSrcNoteOffset(sn, 0));
    }
}



JS_REQUIRES_STACK void
TraceRecorder::emitIf(jsbytecode* pc, bool cond, LIns* x)
{
    ExitType exitType;
    if (js_IsLoopEdge(pc, (jsbytecode*)fragment->root->ip)) {
        exitType = LOOP_EXIT;

        



        if ((*pc == JSOP_IFEQ || *pc == JSOP_IFEQX) == cond) {
            JS_ASSERT(*pc == JSOP_IFNE || *pc == JSOP_IFNEX || *pc == JSOP_IFEQ || *pc == JSOP_IFEQX);
            debug_only_v(printf("Walking out of the loop, terminating it anyway.\n");)
            cond = !cond;
        }

        




        if (x->isconst()) {
            loop = (x->imm32() == cond);
            return;
        }
    } else {
        exitType = BRANCH_EXIT;
    }
    if (!x->isconst())
        guard(cond, x, exitType);
}


JS_REQUIRES_STACK void
TraceRecorder::fuseIf(jsbytecode* pc, bool cond, LIns* x)
{
    if (*pc == JSOP_IFEQ || *pc == JSOP_IFNE) {
        emitIf(pc, cond, x);
        if (*pc == JSOP_IFEQ)
            trackCfgMerges(pc);
    }
}


JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::checkTraceEnd(jsbytecode *pc)
{
    if (js_IsLoopEdge(pc, (jsbytecode*)fragment->root->ip)) {
        





        if (loop) {
            JS_ASSERT(!cx->fp->imacpc && (pc == cx->fp->regs->pc || pc == cx->fp->regs->pc + 1));
            bool fused = pc != cx->fp->regs->pc;
            JSFrameRegs orig = *cx->fp->regs;

            cx->fp->regs->pc = (jsbytecode*)fragment->root->ip;
            cx->fp->regs->sp -= fused ? 2 : 1;

            bool demote = false;
            closeLoop(traceMonitor, demote);

            *cx->fp->regs = orig;

            






            if (demote)
                js_AttemptCompilation(cx, traceMonitor, globalObj, outer, outerArgc);
        } else {
            endLoop(traceMonitor);
        }
        return JSRS_STOP;
    }
    return JSRS_CONTINUE;
}

bool
TraceRecorder::hasMethod(JSObject* obj, jsid id)
{
    if (!obj)
        return false;

    JSObject* pobj;
    JSProperty* prop;
    int protoIndex = OBJ_LOOKUP_PROPERTY(cx, obj, id, &pobj, &prop);
    if (protoIndex < 0 || !prop)
        return false;

    bool found = false;
    if (OBJ_IS_NATIVE(pobj)) {
        JSScope* scope = OBJ_SCOPE(pobj);
        JSScopeProperty* sprop = (JSScopeProperty*) prop;

        if (SPROP_HAS_STUB_GETTER(sprop) &&
            SPROP_HAS_VALID_SLOT(sprop, scope)) {
            jsval v = LOCKED_OBJ_GET_SLOT(pobj, sprop->slot);
            if (VALUE_IS_FUNCTION(cx, v)) {
                found = true;
                if (!SCOPE_IS_BRANDED(scope)) {
                    js_MakeScopeShapeUnique(cx, scope);
                    SCOPE_SET_BRANDED(scope);
                }
            }
        }
    }

    OBJ_DROP_PROPERTY(cx, pobj, prop);
    return found;
}

JS_REQUIRES_STACK bool
TraceRecorder::hasIteratorMethod(JSObject* obj)
{
    JS_ASSERT(cx->fp->regs->sp + 2 <= cx->fp->slots + cx->fp->script->nslots);

    return hasMethod(obj, ATOM_TO_JSID(cx->runtime->atomState.iteratorAtom));
}

int
nanojit::StackFilter::getTop(LInsp guard)
{
    VMSideExit* e = (VMSideExit*)guard->record()->exit;
    if (sp == lirbuf->sp)
        return e->sp_adj;
    JS_ASSERT(sp == lirbuf->rp);
    return e->rp_adj;
}

#if defined NJ_VERBOSE
void
nanojit::LirNameMap::formatGuard(LIns *i, char *out)
{
    VMSideExit *x;

    x = (VMSideExit *)i->record()->exit;
    sprintf(out,
            "%s: %s %s -> pc=%p imacpc=%p sp%+ld rp%+ld",
            formatRef(i),
            lirNames[i->opcode()],
            i->oprnd1()->isCond() ? formatRef(i->oprnd1()) : "",
            (void *)x->pc,
            (void *)x->imacpc,
            (long int)x->sp_adj,
            (long int)x->rp_adj);
}
#endif

void
nanojit::Fragment::onDestroy()
{
    delete (TreeInfo *)vmprivate;
}

static JS_REQUIRES_STACK bool
js_DeleteRecorder(JSContext* cx)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    
    delete tm->recorder;
    tm->recorder = NULL;

    


    if (JS_TRACE_MONITOR(cx).fragmento->assm()->error() == OutOMem ||
        js_OverfullFragmento(tm, tm->fragmento)) {
        FlushJITCache(cx);
        return false;
    }

    return true;
}




static JS_REQUIRES_STACK bool
CheckGlobalObjectShape(JSContext* cx, JSTraceMonitor* tm, JSObject* globalObj,
                       uint32 *shape=NULL, SlotList** slots=NULL)
{
    if (tm->needFlush) {
        FlushJITCache(cx);
        return false;
    }

    if (STOBJ_NSLOTS(globalObj) > MAX_GLOBAL_SLOTS)
        return false;

    uint32 globalShape = OBJ_SHAPE(globalObj);

    if (tm->recorder) {
        VMFragment* root = (VMFragment*)tm->recorder->getFragment()->root;
        TreeInfo* ti = tm->recorder->getTreeInfo();
        
        if (globalObj != root->globalObj || globalShape != root->globalShape) {
            AUDIT(globalShapeMismatchAtEntry);
            debug_only_v(printf("Global object/shape mismatch (%p/%u vs. %p/%u), flushing cache.\n",
                                (void*)globalObj, globalShape, (void*)root->globalObj,
                                root->globalShape);)
            js_Backoff(cx, (jsbytecode*) root->ip);
            FlushJITCache(cx);
            return false;
        }
        if (shape)
            *shape = globalShape;
        if (slots)
            *slots = ti->globalSlots;
        return true;
    }

    
    for (size_t i = 0; i < MONITOR_N_GLOBAL_STATES; ++i) {
        GlobalState &state = tm->globalStates[i];

        if (state.globalShape == uint32(-1)) {
            state.globalObj = globalObj;
            state.globalShape = globalShape;
            JS_ASSERT(state.globalSlots);
            JS_ASSERT(state.globalSlots->length() == 0);
        }

        if (state.globalObj == globalObj && state.globalShape == globalShape) {
            if (shape)
                *shape = globalShape;
            if (slots)
                *slots = state.globalSlots;
            return true;
        }
    }

    
    AUDIT(globalShapeMismatchAtEntry);
    debug_only_v(printf("No global slotlist for global shape %u, flushing cache.\n",
                        globalShape));
    FlushJITCache(cx);
    return false;
}

static JS_REQUIRES_STACK bool
js_StartRecorder(JSContext* cx, VMSideExit* anchor, Fragment* f, TreeInfo* ti,
                 unsigned stackSlots, unsigned ngslots, uint8* typeMap,
                 VMSideExit* expectedInnerExit, jsbytecode* outer, uint32 outerArgc)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (JS_TRACE_MONITOR(cx).needFlush) {
        FlushJITCache(cx);
        return false;
    }

    JS_ASSERT(f->root != f || !cx->fp->imacpc);

    
    tm->recorder = new (&gc) TraceRecorder(cx, anchor, f, ti,
                                           stackSlots, ngslots, typeMap,
                                           expectedInnerExit, outer, outerArgc);

    if (cx->throwing) {
        js_AbortRecording(cx, "setting up recorder failed");
        return false;
    }
    
    tm->fragmento->assm()->setError(None);
    return true;
}

static void
js_TrashTree(JSContext* cx, Fragment* f)
{
    JS_ASSERT((!f->code()) == (!f->vmprivate));
    JS_ASSERT(f == f->root);
    if (!f->code())
        return;
    AUDIT(treesTrashed);
    debug_only_v(printf("Trashing tree info.\n");)
    Fragmento* fragmento = JS_TRACE_MONITOR(cx).fragmento;
    TreeInfo* ti = (TreeInfo*)f->vmprivate;
    f->vmprivate = NULL;
    f->releaseCode(fragmento);
    Fragment** data = ti->dependentTrees.data();
    unsigned length = ti->dependentTrees.length();
    for (unsigned n = 0; n < length; ++n)
        js_TrashTree(cx, data[n]);
    delete ti;
    JS_ASSERT(!f->code() && !f->vmprivate);
}

static int
js_SynthesizeFrame(JSContext* cx, const FrameInfo& fi)
{
    VOUCH_DOES_NOT_REQUIRE_STACK();

    JS_ASSERT(HAS_FUNCTION_CLASS(fi.callee));

    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, fi.callee);
    JS_ASSERT(FUN_INTERPRETED(fun));

    
    JSStackFrame* fp = cx->fp;
    JS_ASSERT_IF(!fi.imacpc,
                 js_ReconstructStackDepth(cx, fp->script, fi.pc)
                 == uintN(fi.s.spdist - fp->script->nfixed));

    uintN nframeslots = JS_HOWMANY(sizeof(JSInlineFrame), sizeof(jsval));
    JSScript* script = fun->u.i.script;
    size_t nbytes = (nframeslots + script->nslots) * sizeof(jsval);

    
    JSArena* a = cx->stackPool.current;
    void* newmark = (void*) a->avail;
    uintN argc = fi.s.argc & 0x7fff;
    jsval* vp = fp->slots + fi.s.spdist - (2 + argc);
    uintN missing = 0;
    jsval* newsp;

    if (fun->nargs > argc) {
        const JSFrameRegs& regs = *fp->regs;

        newsp = vp + 2 + fun->nargs;
        JS_ASSERT(newsp > regs.sp);
        if ((jsuword) newsp <= a->limit) {
            if ((jsuword) newsp > a->avail)
                a->avail = (jsuword) newsp;
            jsval* argsp = newsp;
            do {
                *--argsp = JSVAL_VOID;
            } while (argsp != regs.sp);
            missing = 0;
        } else {
            missing = fun->nargs - argc;
            nbytes += (2 + fun->nargs) * sizeof(jsval);
        }
    }

    
    if (a->avail + nbytes <= a->limit) {
        newsp = (jsval *) a->avail;
        a->avail += nbytes;
        JS_ASSERT(missing == 0);
    } else {
        
        JS_ARENA_ALLOCATE_CAST(newsp, jsval *, &cx->stackPool, nbytes);
        JS_ASSERT(newsp);

        



        if (missing) {
            memcpy(newsp, vp, (2 + argc) * sizeof(jsval));
            vp = newsp;
            newsp = vp + 2 + argc;
            do {
                *newsp++ = JSVAL_VOID;
            } while (--missing != 0);
        }
    }

    
    JSInlineFrame* newifp = (JSInlineFrame *) newsp;
    newsp += nframeslots;

    newifp->frame.callobj = NULL;
    newifp->frame.argsobj = NULL;
    newifp->frame.varobj = NULL;
    newifp->frame.script = script;
    newifp->frame.callee = fi.callee;
    newifp->frame.fun = fun;

    bool constructing = (fi.s.argc & 0x8000) != 0;
    newifp->frame.argc = argc;
    newifp->callerRegs.pc = fi.pc;
    newifp->callerRegs.sp = fp->slots + fi.s.spdist;
    fp->imacpc = fi.imacpc;

#ifdef DEBUG
    if (fi.block != fp->blockChain) {
        for (JSObject* obj = fi.block; obj != fp->blockChain; obj = STOBJ_GET_PARENT(obj))
            JS_ASSERT(obj);
    }
#endif
    fp->blockChain = fi.block;

    newifp->frame.argv = newifp->callerRegs.sp - argc;
    JS_ASSERT(newifp->frame.argv);
#ifdef DEBUG
    
    
    newifp->frame.argv[-1] = JSVAL_HOLE;
#endif
    JS_ASSERT(newifp->frame.argv >= StackBase(fp) + 2);

    newifp->frame.rval = JSVAL_VOID;
    newifp->frame.down = fp;
    newifp->frame.annotation = NULL;
    newifp->frame.scopeChain = OBJ_GET_PARENT(cx, fi.callee);
    newifp->frame.sharpDepth = 0;
    newifp->frame.sharpArray = NULL;
    newifp->frame.flags = constructing ? JSFRAME_CONSTRUCTING : 0;
    newifp->frame.dormantNext = NULL;
    newifp->frame.xmlNamespace = NULL;
    newifp->frame.blockChain = NULL;
    newifp->mark = newmark;
    newifp->frame.thisp = NULL; 

    newifp->frame.regs = fp->regs;
    newifp->frame.regs->pc = script->code;
    newifp->frame.regs->sp = newsp + script->nfixed;
    newifp->frame.imacpc = NULL;
    newifp->frame.slots = newsp;
    if (script->staticLevel < JS_DISPLAY_SIZE) {
        JSStackFrame **disp = &cx->display[script->staticLevel];
        newifp->frame.displaySave = *disp;
        *disp = &newifp->frame;
    }

    



    newifp->callerVersion = (JSVersion) fp->script->version;

    
    fp->regs = &newifp->callerRegs;
    fp = cx->fp = &newifp->frame;

    if (fun->flags & JSFUN_HEAVYWEIGHT) {
        






        newifp->hookData = NULL;
        JS_ASSERT(!JS_TRACE_MONITOR(cx).useReservedObjects);
        JS_TRACE_MONITOR(cx).useReservedObjects = JS_TRUE;
#ifdef DEBUG
        JSObject *obj =
#endif
            js_GetCallObject(cx, &newifp->frame);
        JS_ASSERT(obj);
        JS_TRACE_MONITOR(cx).useReservedObjects = JS_FALSE;
    }

    



    JSInterpreterHook hook = cx->debugHooks->callHook;
    if (hook) {
        newifp->hookData = hook(cx, &newifp->frame, JS_TRUE, 0,
                                cx->debugHooks->callHookData);
    } else {
        newifp->hookData = NULL;
    }

    
    
    
    return (fi.s.spdist - fp->down->script->nfixed) +
           ((fun->nargs > fp->argc) ? fun->nargs - fp->argc : 0) +
           script->nfixed;
}

static void
SynthesizeSlowNativeFrame(JSContext *cx, VMSideExit *exit)
{
    VOUCH_DOES_NOT_REQUIRE_STACK();

    void *mark;
    JSInlineFrame *ifp;

    
    mark = JS_ARENA_MARK(&cx->stackPool);
    JS_ARENA_ALLOCATE_CAST(ifp, JSInlineFrame *, &cx->stackPool, sizeof(JSInlineFrame));
    JS_ASSERT(ifp);

    JSStackFrame *fp = &ifp->frame;
    fp->regs = NULL;
    fp->imacpc = NULL;
    fp->slots = NULL;
    fp->callobj = NULL;
    fp->argsobj = NULL;
    fp->varobj = cx->fp->varobj;
    fp->callee = exit->nativeCallee();
    fp->script = NULL;
    fp->fun = GET_FUNCTION_PRIVATE(cx, fp->callee);
    
    fp->thisp = (JSObject *) cx->nativeVp[1];
    fp->argc = cx->nativeVpLen - 2;
    fp->argv = cx->nativeVp + 2;
    fp->rval = JSVAL_VOID;
    fp->down = cx->fp;
    fp->annotation = NULL;
    JS_ASSERT(cx->fp->scopeChain);
    fp->scopeChain = cx->fp->scopeChain;
    fp->blockChain = NULL;
    fp->sharpDepth = 0;
    fp->sharpArray = NULL;
    fp->flags = exit->constructing() ? JSFRAME_CONSTRUCTING : 0;
    fp->dormantNext = NULL;
    fp->xmlNamespace = NULL;
    fp->displaySave = NULL;

    ifp->mark = mark;
    cx->fp = fp;
}

JS_REQUIRES_STACK bool
js_RecordTree(JSContext* cx, JSTraceMonitor* tm, Fragment* f, jsbytecode* outer, 
              uint32 outerArgc, JSObject* globalObj, uint32 globalShape, 
              SlotList* globalSlots, uint32 argc)
{
    JS_ASSERT(f->root == f);

    
    if (!CheckGlobalObjectShape(cx, tm, globalObj)) {
        js_Backoff(cx, (jsbytecode*) f->root->ip);
        return false;
    }

    AUDIT(recorderStarted);

    
    while (f->code() && f->peer)
        f = f->peer;
    if (f->code())
        f = getAnchor(&JS_TRACE_MONITOR(cx), f->root->ip, globalObj, globalShape, argc);

    if (!f) {
        FlushJITCache(cx);
        return false;
    }

    f->root = f;
    f->lirbuf = tm->lirbuf;

    if (f->lirbuf->outOMem() || js_OverfullFragmento(tm, tm->fragmento)) {
        js_Backoff(cx, (jsbytecode*) f->root->ip);
        FlushJITCache(cx);
        debug_only_v(printf("Out of memory recording new tree, flushing cache.\n");)
        return false;
    }

    JS_ASSERT(!f->code() && !f->vmprivate);

    
    TreeInfo* ti = new (&gc) TreeInfo(f, globalSlots);

    
    ti->typeMap.captureTypes(cx, *globalSlots, 0);
    ti->nStackTypes = ti->typeMap.length() - globalSlots->length();

#ifdef DEBUG
    




    TreeInfo* ti_other;
    for (Fragment* peer = getLoop(tm, f->root->ip, globalObj, globalShape, argc); peer != NULL;
         peer = peer->peer) {
        if (!peer->code() || peer == f)
            continue;
        ti_other = (TreeInfo*)peer->vmprivate;
        JS_ASSERT(ti_other);
        JS_ASSERT(!ti->typeMap.matches(ti_other->typeMap));
    }
    ti->treeFileName = cx->fp->script->filename;
    ti->treeLineNumber = js_FramePCToLineNumber(cx, cx->fp);
    ti->treePCOffset = FramePCOffset(cx->fp);
#endif

    
    unsigned entryNativeStackSlots = ti->nStackTypes;
    JS_ASSERT(entryNativeStackSlots == js_NativeStackSlots(cx, 0));
    ti->nativeStackBase = (entryNativeStackSlots -
            (cx->fp->regs->sp - StackBase(cx->fp))) * sizeof(double);
    ti->maxNativeStackSlots = entryNativeStackSlots;
    ti->maxCallDepth = 0;
    ti->script = cx->fp->script;

    
    if (!js_StartRecorder(cx, NULL, f, ti,
                          ti->nStackTypes,
                          ti->globalSlots->length(),
                          ti->typeMap.data(), NULL, outer, outerArgc)) {
        return false;
    }

    return true;
}

JS_REQUIRES_STACK static inline bool
isSlotUndemotable(JSContext* cx, TreeInfo* ti, unsigned slot)
{
    if (slot < ti->nStackTypes)
        return oracle.isStackSlotUndemotable(cx, slot);

    uint16* gslots = ti->globalSlots->data();
    return oracle.isGlobalSlotUndemotable(cx, gslots[slot - ti->nStackTypes]);
}

JS_REQUIRES_STACK static bool
js_AttemptToStabilizeTree(JSContext* cx, VMSideExit* exit, jsbytecode* outer, uint32 outerArgc)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (tm->needFlush) {
        FlushJITCache(cx);
        return false;
    }

    VMFragment* from = (VMFragment*)exit->from->root;
    TreeInfo* from_ti = (TreeInfo*)from->vmprivate;

    JS_ASSERT(exit->from->root->code());

    
    uint8* m = getStackTypeMap(exit);
    for (unsigned i = 0; i < exit->numStackSlots; i++) {
        if (m[i] == JSVAL_DOUBLE)
            oracle.markStackSlotUndemotable(cx, i);
    }
    m = getGlobalTypeMap(exit);
    for (unsigned i = 0; i < exit->numGlobalSlots; i++) {
        if (m[i] == JSVAL_DOUBLE)
            oracle.markGlobalSlotUndemotable(cx, from_ti->globalSlots->data()[i]);
    }

    


    m = getFullTypeMap(exit);
    if (exit->numGlobalSlots < from_ti->nGlobalTypes()) {
        uint32 partial = exit->numStackSlots + exit->numGlobalSlots;
        m = (uint8*)alloca(from_ti->typeMap.length());
        memcpy(m, getFullTypeMap(exit), partial);
        memcpy(m + partial, from_ti->globalTypeMap() + exit->numGlobalSlots,
               from_ti->nGlobalTypes() - exit->numGlobalSlots);
    }

    bool bound = false;
    for (Fragment* f = from->first; f != NULL; f = f->peer) {
        if (!f->code())
            continue;
        TreeInfo* ti = (TreeInfo*)f->vmprivate;
        JS_ASSERT(exit->numStackSlots == ti->nStackTypes);
        
        unsigned checkSlots = JS_MIN(from_ti->typeMap.length(), ti->typeMap.length());
        uint8* m2 = ti->typeMap.data();
        






        bool matched = true;
        bool undemote = false;
        for (uint32 i = 0; i < checkSlots; i++) {
            
            if (m[i] == m2[i])
                continue;
            matched = false;
            


            if (m[i] == JSVAL_INT && m2[i] == JSVAL_DOUBLE && isSlotUndemotable(cx, ti, i)) {
                undemote = true;
            } else {
                undemote = false;
                break;
            }
        }
        if (matched) {
            JS_ASSERT(from_ti->globalSlots == ti->globalSlots);
            JS_ASSERT(from_ti->nStackTypes == ti->nStackTypes);
            
            if (from != f) {
                ti->dependentTrees.addUnique(from);
                from_ti->linkedTrees.addUnique(f);
            }
            if (ti->nGlobalTypes() < ti->globalSlots->length())
                specializeTreesToMissingGlobals(cx, ti);
            exit->target = f;
            tm->fragmento->assm()->patch(exit);
            
            UnstableExit** tail = &from_ti->unstableExits;
            for (UnstableExit* uexit = from_ti->unstableExits; uexit != NULL; uexit = uexit->next) {
                if (uexit->exit == exit) {
                    *tail = uexit->next;
                    delete uexit;
                    bound = true;
                    break;
                }
                tail = &uexit->next;
            }
            JS_ASSERT(bound);
            debug_only_v(js_DumpPeerStability(tm, f->ip, from->globalObj, from->globalShape, from->argc);)
            break;
        } else if (undemote) {
            
            js_TrashTree(cx, f);
            
            return false;
        }
    }
    if (bound)
        return false;

    VMFragment* root = (VMFragment*)from->root;
    return js_RecordTree(cx, tm, from->first, outer, outerArgc, root->globalObj,
                         root->globalShape, from_ti->globalSlots, cx->fp->argc);
}

static JS_REQUIRES_STACK bool
js_AttemptToExtendTree(JSContext* cx, VMSideExit* anchor, VMSideExit* exitedFrom, jsbytecode* outer)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (tm->needFlush) {
        FlushJITCache(cx);
        return false;
    }

    Fragment* f = anchor->from->root;
    JS_ASSERT(f->vmprivate);
    TreeInfo* ti = (TreeInfo*)f->vmprivate;

    
    if (ti->branchCount >= MAX_BRANCHES)
        return false;

    Fragment* c;
    if (!(c = anchor->target)) {
        c = JS_TRACE_MONITOR(cx).fragmento->createBranch(anchor, cx->fp->regs->pc);
        c->spawnedFrom = anchor;
        c->parent = f;
        anchor->target = c;
        c->root = f;
    }

    




    c->ip = cx->fp->regs->pc;

    debug_only_v(printf("trying to attach another branch to the tree (hits = %d)\n", c->hits());)

    int32_t& hits = c->hits();
    if (outer || (hits++ >= HOTEXIT && hits <= HOTEXIT+MAXEXIT)) {
        
        c->lirbuf = f->lirbuf;
        unsigned stackSlots;
        unsigned ngslots;
        uint8* typeMap;
        TypeMap fullMap;
        if (exitedFrom == NULL) {
            

            ngslots = anchor->numGlobalSlots;
            stackSlots = anchor->numStackSlots;
            typeMap = getFullTypeMap(anchor);
        } else {
            



            VMSideExit* e1 = anchor;
            VMSideExit* e2 = exitedFrom;
            fullMap.add(getStackTypeMap(e1), e1->numStackSlotsBelowCurrentFrame);
            fullMap.add(getStackTypeMap(e2), e2->numStackSlots);
            stackSlots = fullMap.length();
            fullMap.add(getGlobalTypeMap(e2), e2->numGlobalSlots);
            ngslots = e2->numGlobalSlots;
            typeMap = fullMap.data();
        }
        return js_StartRecorder(cx, anchor, c, (TreeInfo*)f->vmprivate, stackSlots,
                                ngslots, typeMap, exitedFrom, outer, cx->fp->argc);
    }
    return false;
}

static JS_REQUIRES_STACK VMSideExit*
js_ExecuteTree(JSContext* cx, Fragment* f, uintN& inlineCallCount,
               VMSideExit** innermostNestedGuardp);

JS_REQUIRES_STACK bool
js_RecordLoopEdge(JSContext* cx, TraceRecorder* r, uintN& inlineCallCount)
{
#ifdef JS_THREADSAFE
    if (OBJ_SCOPE(JS_GetGlobalForObject(cx, cx->fp->scopeChain))->title.ownercx != cx) {
        js_AbortRecording(cx, "Global object not owned by this context");
        return false; 
    }
#endif

    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    
    if (tm->needFlush) {
        FlushJITCache(cx);
        return false;
    }
    if (r->wasDeepAborted()) {
        js_AbortRecording(cx, "deep abort requested");
        return false;
    }

    JS_ASSERT(r->getFragment() && !r->getFragment()->lastIns);
    VMFragment* root = (VMFragment*)r->getFragment()->root;

    
    Fragment* first = getLoop(&JS_TRACE_MONITOR(cx), cx->fp->regs->pc,
                              root->globalObj, root->globalShape, cx->fp->argc);
    if (!first) {
        
        AUDIT(returnToDifferentLoopHeader);
        JS_ASSERT(!cx->fp->imacpc);
        debug_only_v(printf("loop edge to %d, header %d\n",
                            cx->fp->regs->pc - cx->fp->script->code,
                            (jsbytecode*)r->getFragment()->root->ip - cx->fp->script->code));
        js_AbortRecording(cx, "Loop edge does not return to header");
        return false;
    }

    
    if (tm->reservedDoublePoolPtr < (tm->reservedDoublePool + MAX_NATIVE_STACK_SLOTS) &&
        !js_ReplenishReservedPool(cx, tm)) {
        js_AbortRecording(cx, "Couldn't call inner tree (out of memory)");
        return false;
    }

    

    JSObject* globalObj = JS_GetGlobalForObject(cx, cx->fp->scopeChain);
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;
    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots))
        return false;

    debug_only_v(printf("Looking for type-compatible peer (%s:%d@%d)\n",
                        cx->fp->script->filename,
                        js_FramePCToLineNumber(cx, cx->fp),
                        FramePCOffset(cx->fp));)

    
    Fragment* f = r->findNestedCompatiblePeer(first);
    if (!f || !f->code()) {
        AUDIT(noCompatInnerTrees);

        VMFragment* outerFragment = (VMFragment*) tm->recorder->getFragment()->root;
        jsbytecode* outer = (jsbytecode*) outerFragment->ip;
        uint32 outerArgc = outerFragment->argc;
        uint32 argc = cx->fp->argc;
        js_AbortRecording(cx, "No compatible inner tree");

        
        for (f = first; f != NULL; f = f->peer) {
            if (!f->code())
                break;
        }
        if (!f || f->code()) {
            f = getAnchor(tm, cx->fp->regs->pc, globalObj, globalShape, argc);
            if (!f) {
                FlushJITCache(cx);
                return false;
            }
        }
        return js_RecordTree(cx, tm, f, outer, outerArgc, globalObj, globalShape, globalSlots, argc);
    }

    r->adjustCallerTypes(f);
    r->prepareTreeCall(f);
    VMSideExit* innermostNestedGuard = NULL;
    VMSideExit* lr = js_ExecuteTree(cx, f, inlineCallCount, &innermostNestedGuard);
    if (!lr || r->wasDeepAborted()) {
        if (!lr)
            js_AbortRecording(cx, "Couldn't call inner tree");
        return false;
    }

    VMFragment* outerFragment = (VMFragment*) tm->recorder->getFragment()->root;
    jsbytecode* outer = (jsbytecode*) outerFragment->ip;
    switch (lr->exitType) {
      case LOOP_EXIT:
        
        if (innermostNestedGuard) {
            js_AbortRecording(cx, "Inner tree took different side exit, abort current "
                              "recording and grow nesting tree");
            return js_AttemptToExtendTree(cx, innermostNestedGuard, lr, outer);
        }
        
        r->emitTreeCall(f, lr);
        return true;
      case UNSTABLE_LOOP_EXIT:
        
        js_AbortRecording(cx, "Inner tree is trying to stabilize, abort outer recording");
        return js_AttemptToStabilizeTree(cx, lr, outer, outerFragment->argc);
      case BRANCH_EXIT:
        
        js_AbortRecording(cx, "Inner tree is trying to grow, abort outer recording");
        return js_AttemptToExtendTree(cx, lr, NULL, outer);
      default:
        debug_only_v(printf("exit_type=%d\n", lr->exitType);)
            js_AbortRecording(cx, "Inner tree not suitable for calling");
        return false;
    }
}

static bool
js_IsEntryTypeCompatible(jsval* vp, uint8* m)
{
    unsigned tag = JSVAL_TAG(*vp);

    debug_only_v(printf("%c/%c ", tagChar[tag], typeChar[*m]);)

    switch (*m) {
      case JSVAL_OBJECT:
        if (tag == JSVAL_OBJECT && !JSVAL_IS_NULL(*vp) &&
            !HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(*vp))) {
            return true;
        }
        debug_only_v(printf("object != tag%u ", tag);)
        return false;
      case JSVAL_INT:
        jsint i;
        if (JSVAL_IS_INT(*vp))
            return true;
        if ((tag == JSVAL_DOUBLE) && JSDOUBLE_IS_INT(*JSVAL_TO_DOUBLE(*vp), i))
            return true;
        debug_only_v(printf("int != tag%u(value=%lu) ", tag, (unsigned long)*vp);)
        return false;
      case JSVAL_DOUBLE:
        if (JSVAL_IS_INT(*vp) || tag == JSVAL_DOUBLE)
            return true;
        debug_only_v(printf("double != tag%u ", tag);)
        return false;
      case JSVAL_BOXED:
        JS_NOT_REACHED("shouldn't see boxed type in entry");
        return false;
      case JSVAL_STRING:
        if (tag == JSVAL_STRING)
            return true;
        debug_only_v(printf("string != tag%u ", tag);)
        return false;
      case JSVAL_TNULL:
        if (JSVAL_IS_NULL(*vp))
            return true;
        debug_only_v(printf("null != tag%u ", tag);)
        return false;
      case JSVAL_BOOLEAN:
        if (tag == JSVAL_BOOLEAN)
            return true;
        debug_only_v(printf("bool != tag%u ", tag);)
        return false;
      default:
        JS_ASSERT(*m == JSVAL_TFUN);
        if (tag == JSVAL_OBJECT && !JSVAL_IS_NULL(*vp) &&
            HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(*vp))) {
            return true;
        }
        debug_only_v(printf("fun != tag%u ", tag);)
        return false;
    }
}

JS_REQUIRES_STACK Fragment*
TraceRecorder::findNestedCompatiblePeer(Fragment* f)
{
    JSTraceMonitor* tm;

    tm = &JS_TRACE_MONITOR(cx);
    unsigned int ngslots = treeInfo->globalSlots->length();
    uint16* gslots = treeInfo->globalSlots->data();

    TreeInfo* ti;
    for (; f != NULL; f = f->peer) {
        if (!f->code())
            continue;

        ti = (TreeInfo*)f->vmprivate;

        debug_only_v(printf("checking nested types %p: ", (void*)f);)

        if (ngslots > ti->nGlobalTypes())
            specializeTreesToMissingGlobals(cx, ti);

        uint8* typemap = ti->typeMap.data();

        







        bool ok = true;
        uint8* m = typemap;
        FORALL_SLOTS_IN_PENDING_FRAMES(cx, 0,
            debug_only_v(printf("%s%d=", vpname, vpnum);)
            if (!js_IsEntryTypeCompatible(vp, m)) {
                ok = false;
            } else if (!isPromoteInt(get(vp)) && *m == JSVAL_INT) {
                oracle.markStackSlotUndemotable(cx, unsigned(m - typemap));
                ok = false;
            } else if (JSVAL_IS_INT(*vp) && *m == JSVAL_DOUBLE) {
                oracle.markStackSlotUndemotable(cx, unsigned(m - typemap));
            }
            m++;
        );
        FORALL_GLOBAL_SLOTS(cx, ngslots, gslots,
            debug_only_v(printf("%s%d=", vpname, vpnum);)
            if (!js_IsEntryTypeCompatible(vp, m)) {
                ok = false;
            } else if (!isPromoteInt(get(vp)) && *m == JSVAL_INT) {
                oracle.markGlobalSlotUndemotable(cx, gslots[n]);
                ok = false;
            } else if (JSVAL_IS_INT(*vp) && *m == JSVAL_DOUBLE) {
                oracle.markGlobalSlotUndemotable(cx, gslots[n]);
            }
            m++;
        );
        JS_ASSERT(unsigned(m - ti->typeMap.data()) == ti->typeMap.length());

        debug_only_v(printf(" %s\n", ok ? "match" : "");)

        if (ok)
            return f;
    }

    return NULL;
}








static JS_REQUIRES_STACK bool
js_CheckEntryTypes(JSContext* cx, TreeInfo* ti)
{
    unsigned int ngslots = ti->globalSlots->length();
    uint16* gslots = ti->globalSlots->data();

    JS_ASSERT(ti->nStackTypes == js_NativeStackSlots(cx, 0));

    if (ngslots > ti->nGlobalTypes())
        specializeTreesToMissingGlobals(cx, ti);

    uint8* m = ti->typeMap.data();

    JS_ASSERT(ti->typeMap.length() == js_NativeStackSlots(cx, 0) + ngslots);
    JS_ASSERT(ti->typeMap.length() == ti->nStackTypes + ngslots);
    JS_ASSERT(ti->nGlobalTypes() == ngslots);
    FORALL_SLOTS(cx, ngslots, gslots, 0,
        debug_only_v(printf("%s%d=", vpname, vpnum);)
        JS_ASSERT(*m != 0xCD);
        if (!js_IsEntryTypeCompatible(vp, m))
            goto check_fail;
        m++;
    );
    JS_ASSERT(unsigned(m - ti->typeMap.data()) == ti->typeMap.length());

    debug_only_v(printf("\n");)
    return true;

check_fail:
    debug_only_v(printf("\n");)
    return false;
}









static JS_REQUIRES_STACK Fragment*
js_FindVMCompatiblePeer(JSContext* cx, Fragment* f, uintN& count)
{
    count = 0;
    for (; f != NULL; f = f->peer) {
        if (f->vmprivate == NULL)
            continue;
        debug_only_v(printf("checking vm types %p (ip: %p): ", (void*)f, f->ip);)
        if (js_CheckEntryTypes(cx, (TreeInfo*)f->vmprivate))
            return f;
        ++count;
    }
    return NULL;
}

static void
LeaveTree(InterpState&, VMSideExit* lr);




static JS_REQUIRES_STACK VMSideExit*
js_ExecuteTree(JSContext* cx, Fragment* f, uintN& inlineCallCount,
               VMSideExit** innermostNestedGuardp)
{
    JS_ASSERT(f->root == f && f->code() && f->vmprivate);

    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    JSObject* globalObj = JS_GetGlobalForObject(cx, cx->fp->scopeChain);
    TreeInfo* ti = (TreeInfo*)f->vmprivate;
    unsigned ngslots = ti->globalSlots->length();
    uint16* gslots = ti->globalSlots->data();
    unsigned globalFrameSize = STOBJ_NSLOTS(globalObj);

    
    JS_ASSERT(!ngslots || (OBJ_SHAPE(JS_GetGlobalForObject(cx, cx->fp->scopeChain)) ==
              ((VMFragment*)f)->globalShape));
    
    JS_ASSERT(tm->reservedDoublePoolPtr >= tm->reservedDoublePool + MAX_NATIVE_STACK_SLOTS);

    
    if (!js_ReserveObjects(cx, MAX_CALL_STACK_ENTRIES))
        return NULL;

#ifdef DEBUG
    uintN savedProhibitFlush = JS_TRACE_MONITOR(cx).prohibitFlush;
#endif

    
    InterpState* state = (InterpState*)alloca(sizeof(InterpState) + (globalFrameSize+1)*sizeof(double));
    state->cx = cx;
    state->inlineCallCountp = &inlineCallCount;
    state->innermostNestedGuardp = innermostNestedGuardp;
    state->outermostTree = ti;
    state->lastTreeExitGuard = NULL;
    state->lastTreeCallGuard = NULL;
    state->rpAtLastTreeCall = NULL;
    state->builtinStatus = 0;

    
    double* global = (double*)(state+1);

    
    double stack_buffer[MAX_NATIVE_STACK_SLOTS];
    state->stackBase = stack_buffer;
    state->sp = stack_buffer + (ti->nativeStackBase/sizeof(double));
    state->eos = stack_buffer + MAX_NATIVE_STACK_SLOTS;

    
    FrameInfo* callstack_buffer[MAX_CALL_STACK_ENTRIES];
    state->callstackBase = callstack_buffer;
    state->rp = callstack_buffer;
    state->eor = callstack_buffer + MAX_CALL_STACK_ENTRIES;

    void *reserve;
    state->stackMark = JS_ARENA_MARK(&cx->stackPool);
    JS_ARENA_ALLOCATE(reserve, &cx->stackPool, MAX_INTERP_STACK_BYTES);
    if (!reserve)
        return NULL;

#ifdef DEBUG
    memset(stack_buffer, 0xCD, sizeof(stack_buffer));
    memset(global, 0xCD, (globalFrameSize+1)*sizeof(double));
    JS_ASSERT(globalFrameSize <= MAX_GLOBAL_SLOTS);
#endif

    debug_only(*(uint64*)&global[globalFrameSize] = 0xdeadbeefdeadbeefLL;)
    debug_only_v(printf("entering trace at %s:%u@%u, native stack slots: %u code: %p\n",
                        cx->fp->script->filename,
                        js_FramePCToLineNumber(cx, cx->fp),
                        FramePCOffset(cx->fp),
                        ti->maxNativeStackSlots,
                        f->code());)

    JS_ASSERT(ti->nGlobalTypes() == ngslots);

    if (ngslots)
        BuildNativeGlobalFrame(cx, ngslots, gslots, ti->globalTypeMap(), global);
    BuildNativeStackFrame(cx, 0, ti->typeMap.data(), stack_buffer);

    union { NIns *code; GuardRecord* (FASTCALL *func)(InterpState*, Fragment*); } u;
    u.code = f->code();

#ifdef EXECUTE_TREE_TIMER
    state->startTime = rdtsc();
#endif

    JS_ASSERT(!tm->tracecx);
    tm->tracecx = cx;
    state->prev = cx->interpState;
    cx->interpState = state;

    debug_only(fflush(NULL);)
    GuardRecord* rec;
#if defined(JS_NO_FASTCALL) && defined(NANOJIT_IA32)
    SIMULATE_FASTCALL(rec, state, NULL, u.func);
#else
    rec = u.func(state, NULL);
#endif
    VMSideExit* lr = (VMSideExit*)rec->exit;

    AUDIT(traceTriggered);

    cx->interpState = state->prev;

    JS_ASSERT(lr->exitType != LOOP_EXIT || !lr->calldepth);
    tm->tracecx = NULL;
    LeaveTree(*state, lr);
    JS_ASSERT(JS_TRACE_MONITOR(cx).prohibitFlush == savedProhibitFlush);
    return state->innermost;
}

static JS_FORCES_STACK void
LeaveTree(InterpState& state, VMSideExit* lr)
{
    VOUCH_DOES_NOT_REQUIRE_STACK();

    JSContext* cx = state.cx;
    FrameInfo** callstack = state.callstackBase;
    double* stack = state.stackBase;

    

    VMSideExit* innermost = lr;

    







    FrameInfo** rp = (FrameInfo**)state.rp;
    if (lr->exitType == NESTED_EXIT) {
        VMSideExit* nested = state.lastTreeCallGuard;
        if (!nested) {
            





            nested = lr;
            rp += lr->calldepth;
        } else {
            


            rp = (FrameInfo**)state.rpAtLastTreeCall;
        }
        innermost = state.lastTreeExitGuard;
        if (state.innermostNestedGuardp)
            *state.innermostNestedGuardp = nested;
        JS_ASSERT(nested);
        JS_ASSERT(nested->exitType == NESTED_EXIT);
        JS_ASSERT(state.lastTreeExitGuard);
        JS_ASSERT(state.lastTreeExitGuard->exitType != NESTED_EXIT);
    }

    int32_t bs = state.builtinStatus;
    bool bailed = innermost->exitType == STATUS_EXIT && (bs & JSBUILTIN_BAILED);
    if (bailed) {
        








        if (!cx->fp->script) {
            JSStackFrame *fp = cx->fp;
            JS_ASSERT(FUN_SLOW_NATIVE(GET_FUNCTION_PRIVATE(cx, fp->callee)));
            JS_ASSERT(fp->regs == NULL);
            JS_ASSERT(fp->down->regs != &((JSInlineFrame *) fp)->callerRegs);
            cx->fp = fp->down;
            JS_ARENA_RELEASE(&cx->stackPool, ((JSInlineFrame *) fp)->mark);
        }
        JS_ASSERT(cx->fp->script);

        if (!(bs & JSBUILTIN_ERROR)) {
            








            JS_ASSERT(*cx->fp->regs->pc == JSOP_CALL ||
                      *cx->fp->regs->pc == JSOP_APPLY ||
                      *cx->fp->regs->pc == JSOP_NEW);
            uintN argc = GET_ARGC(cx->fp->regs->pc);
            cx->fp->regs->pc += JSOP_CALL_LENGTH;
            cx->fp->regs->sp -= argc + 1;
            JS_ASSERT_IF(!cx->fp->imacpc,
                         cx->fp->slots + cx->fp->script->nfixed +
                         js_ReconstructStackDepth(cx, cx->fp->script, cx->fp->regs->pc) ==
                         cx->fp->regs->sp);

            



            uint8* typeMap = getStackTypeMap(innermost);
            NativeToValue(cx,
                          cx->fp->regs->sp[-1],
                          typeMap[innermost->numStackSlots - 1],
                          (jsdouble *) state.sp + innermost->sp_adj / sizeof(jsdouble) - 1);
        }
        JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
        if (tm->prohibitFlush && --tm->prohibitFlush == 0 && tm->needFlush)
            FlushJITCache(cx);
        return;
    }

    JS_ARENA_RELEASE(&cx->stackPool, state.stackMark);
    while (callstack < rp) {
        

        js_SynthesizeFrame(cx, **callstack);
        int slots = FlushNativeStackFrame(cx, 1, (uint8*)(*callstack+1), stack, cx->fp);
#ifdef DEBUG
        JSStackFrame* fp = cx->fp;
        debug_only_v(printf("synthesized deep frame for %s:%u@%u, slots=%d\n",
                            fp->script->filename,
                            js_FramePCToLineNumber(cx, fp),
                            FramePCOffset(fp),
                            slots);)
#endif
        

        ++*state.inlineCallCountp;
        ++callstack;
        stack += slots;
    }

    

    JS_ASSERT(rp == callstack);
    unsigned calldepth = innermost->calldepth;
    unsigned calldepth_slots = 0;
    for (unsigned n = 0; n < calldepth; ++n) {
        calldepth_slots += js_SynthesizeFrame(cx, *callstack[n]);
        ++*state.inlineCallCountp;
#ifdef DEBUG
        JSStackFrame* fp = cx->fp;
        debug_only_v(printf("synthesized shallow frame for %s:%u@%u\n",
                            fp->script->filename, js_FramePCToLineNumber(cx, fp),
                            FramePCOffset(fp));)
#endif
    }

    



    JSStackFrame* fp = cx->fp;

    fp->blockChain = innermost->block;

    

    fp->regs->pc = innermost->pc;
    fp->imacpc = innermost->imacpc;
    fp->regs->sp = StackBase(fp) + (innermost->sp_adj / sizeof(double)) - calldepth_slots;
    JS_ASSERT_IF(!fp->imacpc,
                 fp->slots + fp->script->nfixed +
                 js_ReconstructStackDepth(cx, fp->script, fp->regs->pc) == fp->regs->sp);

#ifdef EXECUTE_TREE_TIMER
    uint64 cycles = rdtsc() - state.startTime;
#elif defined(JS_JIT_SPEW)
    uint64 cycles = 0;
#endif

    debug_only_v(printf("leaving trace at %s:%u@%u, op=%s, lr=%p, exitType=%d, sp=%d, "
                        "calldepth=%d, cycles=%llu\n",
                        fp->script->filename,
                        js_FramePCToLineNumber(cx, fp),
                        FramePCOffset(fp),
                        js_CodeName[fp->imacpc ? *fp->imacpc : *fp->regs->pc],
                        (void*)lr,
                        lr->exitType,
                        fp->regs->sp - StackBase(fp),
                        calldepth,
                        cycles));

    



    TreeInfo* outermostTree = state.outermostTree;
    uint16* gslots = outermostTree->globalSlots->data();
    unsigned ngslots = outermostTree->globalSlots->length();
    JS_ASSERT(ngslots == outermostTree->nGlobalTypes());
    uint8* globalTypeMap;

    
    if (innermost->numGlobalSlots == ngslots) {
        globalTypeMap = getGlobalTypeMap(innermost);
    



    } else {
        TreeInfo* ti = (TreeInfo*)innermost->from->root->vmprivate;
        JS_ASSERT(ti->nGlobalTypes() == ngslots);
        JS_ASSERT(ti->nGlobalTypes() > innermost->numGlobalSlots);
        globalTypeMap = (uint8*)alloca(ngslots * sizeof(uint8));
        memcpy(globalTypeMap, getGlobalTypeMap(innermost), innermost->numGlobalSlots);
        memcpy(globalTypeMap + innermost->numGlobalSlots,
               ti->globalTypeMap() + innermost->numGlobalSlots,
               ti->nGlobalTypes() - innermost->numGlobalSlots);
    }

    
    double* global = (double*)(&state + 1);
    FlushNativeGlobalFrame(cx, ngslots, gslots, globalTypeMap, global);
    JS_ASSERT(*(uint64*)&global[STOBJ_NSLOTS(JS_GetGlobalForObject(cx, cx->fp->scopeChain))] ==
              0xdeadbeefdeadbeefLL);

    
#ifdef DEBUG
    int slots =
#endif
        FlushNativeStackFrame(cx, innermost->calldepth,
                              getStackTypeMap(innermost),
                              stack, NULL);
    JS_ASSERT(unsigned(slots) == innermost->numStackSlots);

    if (innermost->nativeCalleeWord)
        SynthesizeSlowNativeFrame(cx, innermost);

    cx->nativeVp = NULL;

#ifdef DEBUG
    
    for (JSStackFrame* fp = cx->fp; fp; fp = fp->down) {
        JS_ASSERT_IF(fp->callee, JSVAL_IS_OBJECT(fp->argv[-1]));
    }
#endif
#ifdef JS_JIT_SPEW
    if (innermost->exitType != TIMEOUT_EXIT)
        AUDIT(sideExitIntoInterpreter);
    else
        AUDIT(timeoutIntoInterpreter);
#endif

    state.innermost = innermost;
}

JS_REQUIRES_STACK bool
js_MonitorLoopEdge(JSContext* cx, uintN& inlineCallCount)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    
    if (tm->recorder) {
        jsbytecode* innerLoopHeaderPC = cx->fp->regs->pc;

        if (js_RecordLoopEdge(cx, tm->recorder, inlineCallCount))
            return true;

        









        if (innerLoopHeaderPC != cx->fp->regs->pc)
            return false;
    }
    JS_ASSERT(!tm->recorder);

    
    if (tm->reservedDoublePoolPtr < (tm->reservedDoublePool + MAX_NATIVE_STACK_SLOTS) &&
        !js_ReplenishReservedPool(cx, tm)) {
        return false; 
    }

    
    JSObject* globalObj = JS_GetGlobalForObject(cx, cx->fp->scopeChain);
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;

    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots)) {
        js_Backoff(cx, cx->fp->regs->pc);
        return false;
    }

    
    if (cx->operationCallbackFlag)
        return false;
    
    jsbytecode* pc = cx->fp->regs->pc;
    uint32 argc = cx->fp->argc;

    Fragment* f = getLoop(tm, pc, globalObj, globalShape, argc);
    if (!f)
        f = getAnchor(tm, pc, globalObj, globalShape, argc);

    if (!f) {
        FlushJITCache(cx);
        return false;
    }

    

    if (!f->code() && !f->peer) {
    record:
        if (++f->hits() < HOTLOOP)
            return false;
        

        return js_RecordTree(cx, tm, f->first, NULL, 0, globalObj, globalShape, 
                             globalSlots, argc);
    }

    debug_only_v(printf("Looking for compat peer %d@%d, from %p (ip: %p)\n",
                        js_FramePCToLineNumber(cx, cx->fp),
                        FramePCOffset(cx->fp), (void*)f, f->ip);)

    uintN count;
    Fragment* match = js_FindVMCompatiblePeer(cx, f, count);
    if (!match) {
        if (count < MAXPEERS)
            goto record;
        

        debug_only_v(printf("Blacklisted: too many peer trees.\n");)
        js_Blacklist((jsbytecode*) f->root->ip);
        return false;
    }

    VMSideExit* lr = NULL;
    VMSideExit* innermostNestedGuard = NULL;

    lr = js_ExecuteTree(cx, match, inlineCallCount, &innermostNestedGuard);
    if (!lr)
        return false;

    


    switch (lr->exitType) {
      case UNSTABLE_LOOP_EXIT:
        return js_AttemptToStabilizeTree(cx, lr, NULL, NULL);
      case BRANCH_EXIT:
      case CASE_EXIT:
        return js_AttemptToExtendTree(cx, lr, NULL, NULL);
      case LOOP_EXIT:
        if (innermostNestedGuard)
            return js_AttemptToExtendTree(cx, innermostNestedGuard, lr, NULL);
        return false;
      default:
        
        return false;
    }
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::monitorRecording(JSContext* cx, TraceRecorder* tr, JSOp op)
{
    
    if (JS_TRACE_MONITOR(cx).needFlush) {
        FlushJITCache(cx);
        return JSRS_STOP;
    }
    if (tr->wasDeepAborted()) {
        js_AbortRecording(cx, "deep abort requested");
        return JSRS_STOP;
    }
    JS_ASSERT(!tr->fragment->lastIns);

    



    tr->pendingTraceableNative = NULL;
    tr->newobj_ins = NULL;

    debug_only_v(js_Disassemble1(cx, cx->fp->script, cx->fp->regs->pc,
                                 cx->fp->imacpc ? 0 : cx->fp->regs->pc - cx->fp->script->code,
                                 !cx->fp->imacpc, stdout);)

    



    JSRecordingStatus status;
#ifdef DEBUG
    bool wasInImacro = (cx->fp->imacpc != NULL);
#endif
    switch (op) {
      default:
          status = JSRS_ERROR;
          goto stop_recording;
# define OPDEF(x,val,name,token,length,nuses,ndefs,prec,format)               \
      case x:                                                                 \
        status = tr->record_##x();                                            \
        if (JSOP_IS_IMACOP(x))                                                \
            goto imacro;                                                      \
        break;
# include "jsopcode.tbl"
# undef OPDEF
    }

    JS_ASSERT(status != JSRS_IMACRO);
    JS_ASSERT_IF(!wasInImacro, cx->fp->imacpc == NULL);

    
    if (tr->wasDeepAborted()) {
        js_AbortRecording(cx, "deep abort requested");
        return JSRS_STOP;
    }

    if (JS_TRACE_MONITOR(cx).fragmento->assm()->error()) {
        js_AbortRecording(cx, "error during recording");
        return JSRS_STOP;
    }

    if (tr->lirbuf->outOMem() ||
        js_OverfullFragmento(&JS_TRACE_MONITOR(cx), JS_TRACE_MONITOR(cx).fragmento)) {
        js_AbortRecording(cx, "no more LIR memory");
        FlushJITCache(cx);
        return JSRS_STOP;
    }

  imacro:
    if (!STATUS_ABORTS_RECORDING(status))
        return status;

  stop_recording:
    
    if (tr->fragment->lastIns) {
        js_DeleteRecorder(cx);
        return status;
    }

    
    js_AbortRecording(cx, js_CodeName[op]);
    return status;
}

JS_REQUIRES_STACK void
js_AbortRecording(JSContext* cx, const char* reason)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    JS_ASSERT(tm->recorder != NULL);
    AUDIT(recorderAborted);

    
    Fragment* f = tm->recorder->getFragment();

    





    if (!f || f->lastIns) {
        js_DeleteRecorder(cx);
        return;
    }

    JS_ASSERT(!f->vmprivate);
#ifdef DEBUG
    TreeInfo* ti = tm->recorder->getTreeInfo();
    debug_only_a(printf("Abort recording of tree %s:%d@%d at %s:%d@%d: %s.\n",
                        ti->treeFileName,
                        ti->treeLineNumber,
                        ti->treePCOffset,
                        cx->fp->script->filename,
                        js_FramePCToLineNumber(cx, cx->fp),
                        FramePCOffset(cx->fp),
                        reason);)
#endif

    js_Backoff(cx, (jsbytecode*) f->root->ip, f->root);

    


    if (!js_DeleteRecorder(cx))
        return;

    



    if (!f->code() && (f->root == f))
        js_TrashTree(cx, f);
}

#if defined NANOJIT_IA32
static bool
js_CheckForSSE2()
{
    int features = 0;
#if defined _MSC_VER
    __asm
    {
        pushad
        mov eax, 1
        cpuid
        mov features, edx
        popad
    }
#elif defined __GNUC__
    asm("xchg %%esi, %%ebx\n" 
        "mov $0x01, %%eax\n"
        "cpuid\n"
        "mov %%edx, %0\n"
        "xchg %%esi, %%ebx\n"
        : "=m" (features)
        : 
        : "%eax", "%esi", "%ecx", "%edx"
       );
#elif defined __SUNPRO_C || defined __SUNPRO_CC
    asm("push %%ebx\n"
        "mov $0x01, %%eax\n"
        "cpuid\n"
        "pop %%ebx\n"
        : "=d" (features)
        : 
        : "%eax", "%ecx"
       );
#endif
    return (features & (1<<26)) != 0;
}
#endif

#if defined(NANOJIT_ARM)

#if defined(_MSC_VER) && defined(WINCE)


extern "C" int js_arm_try_thumb_op();
extern "C" int js_arm_try_armv6t2_op();
extern "C" int js_arm_try_armv5_op();
extern "C" int js_arm_try_armv6_op();
extern "C" int js_arm_try_armv7_op();
extern "C" int js_arm_try_vfp_op();

static bool
js_arm_check_thumb() {
    bool ret = false;
    __try {
        js_arm_try_thumb_op();
        ret = true;
    } __except(GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
        ret = false;
    }
    return ret;
}

static bool
js_arm_check_thumb2() {
    bool ret = false;
    __try {
        js_arm_try_armv6t2_op();
        ret = true;
    } __except(GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
        ret = false;
    }
    return ret;
}

static unsigned int
js_arm_check_arch() {
    unsigned int arch = 4;
    __try {
        js_arm_try_armv5_op();
        arch = 5;
        js_arm_try_armv6_op();
        arch = 6;
        js_arm_try_armv7_op();
        arch = 7;
    } __except(GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
    }
    return arch;
}

static bool
js_arm_check_vfp() {
    bool ret = false;
    __try {
        js_arm_try_vfp_op();
        ret = true;
    } __except(GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
        ret = false;
    }
    return ret;
}

#elif defined(__GNUC__) && defined(AVMPLUS_LINUX)

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <elf.h>


static unsigned int arm_arch = 4;
static bool arm_has_thumb = false;
static bool arm_has_vfp = false;
static bool arm_has_neon = false;
static bool arm_has_iwmmxt = false;
static bool arm_tests_initialized = false;

static void
arm_read_auxv() {
    int fd;
    Elf32_auxv_t aux;

    fd = open("/proc/self/auxv", O_RDONLY);
    if (fd > 0) {
        while (read(fd, &aux, sizeof(Elf32_auxv_t))) {
            if (aux.a_type == AT_HWCAP) {
                uint32_t hwcap = aux.a_un.a_val;
                if (getenv("ARM_FORCE_HWCAP"))
                    hwcap = strtoul(getenv("ARM_FORCE_HWCAP"), NULL, 0);
                
                
                arm_has_thumb = (hwcap & 4) != 0;
                arm_has_vfp = (hwcap & 64) != 0;
                arm_has_iwmmxt = (hwcap & 512) != 0;
                
                arm_has_neon = (hwcap & 4096) != 0;
            } else if (aux.a_type == AT_PLATFORM) {
                const char *plat = (const char*) aux.a_un.a_val;
                if (getenv("ARM_FORCE_PLATFORM"))
                    plat = getenv("ARM_FORCE_PLATFORM");
                
                
                
                
                
                if ((plat[0] == 'v') &&
                    (plat[1] >= '4') && (plat[1] <= '9') &&
                    ((plat[2] == 'l') || (plat[2] == 'b')))
                {
                    arm_arch = plat[1] - '0';
                }
                else
                {
                    
                    
                    JS_ASSERT(false);
                }
            }
        }
        close (fd);

        
        
        if (!getenv("ARM_TRUST_HWCAP") && (arm_arch >= 7))
            arm_has_neon = true;
    }

    arm_tests_initialized = true;
}

static bool
js_arm_check_thumb() {
    if (!arm_tests_initialized)
        arm_read_auxv();

    return arm_has_thumb;
}

static bool
js_arm_check_thumb2() {
    if (!arm_tests_initialized)
        arm_read_auxv();

    
    
    
    return (arm_arch >= 7);
}

static unsigned int
js_arm_check_arch() {
    if (!arm_tests_initialized)
        arm_read_auxv();

    return arm_arch;
}

static bool
js_arm_check_vfp() {
    if (!arm_tests_initialized)
        arm_read_auxv();

    return arm_has_vfp;
}

#else
#warning Not sure how to check for architecture variant on your platform. Assuming ARMv4.
static bool
js_arm_check_thumb() { return false; }
static bool
js_arm_check_thumb2() { return false; }
static unsigned int
js_arm_check_arch() { return 4; }
static bool
js_arm_check_vfp() { return false; }
#endif

#endif 

#define K *1024
#define M K K
#define G K M

void
js_SetMaxCodeCacheBytes(JSContext* cx, uint32 bytes)
{
    JSTraceMonitor* tm = &JS_THREAD_DATA(cx)->traceMonitor;
    JS_ASSERT(tm->fragmento && tm->reFragmento);
    if (bytes > 1 G)
        bytes = 1 G;
    if (bytes < 128 K)
        bytes = 128 K;
    tm->maxCodeCacheBytes = bytes;
}

void
js_InitJIT(JSTraceMonitor *tm)
{
    if (!did_we_check_processor_features) {
#if defined NANOJIT_IA32
        avmplus::AvmCore::config.use_cmov =
        avmplus::AvmCore::config.sse2 = js_CheckForSSE2();
#endif
#if defined NANOJIT_ARM
        bool            arm_vfp     = js_arm_check_vfp();
        bool            arm_thumb   = js_arm_check_thumb();
        bool            arm_thumb2  = js_arm_check_thumb2();
        unsigned int    arm_arch    = js_arm_check_arch();

        avmplus::AvmCore::config.vfp        = arm_vfp;
        avmplus::AvmCore::config.soft_float = !arm_vfp;
        avmplus::AvmCore::config.thumb      = arm_thumb;
        avmplus::AvmCore::config.thumb2     = arm_thumb2;
        avmplus::AvmCore::config.arch       = arm_arch;

        
        
        JS_ASSERT(arm_arch >= 4);
        
        JS_ASSERT((arm_thumb) || (arm_arch == 4));
        
        JS_ASSERT((arm_thumb2) || (arm_arch <= 6));
        
        JS_ASSERT((arm_thumb2 && arm_thumb) || (!arm_thumb2));
#endif
        did_we_check_processor_features = true;
    }

    


    tm->maxCodeCacheBytes = 16 M;

    if (!tm->recordAttempts.ops) {
        JS_DHashTableInit(&tm->recordAttempts, JS_DHashGetStubOps(),
                          NULL, sizeof(PCHashEntry),
                          JS_DHASH_DEFAULT_CAPACITY(PC_HASH_COUNT));
    }

    if (!tm->fragmento) {
        JS_ASSERT(!tm->reservedDoublePool);
        Fragmento* fragmento = new (&gc) Fragmento(core, 32);
        verbose_only(fragmento->labels = new (&gc) LabelMap(core, NULL);)
        tm->fragmento = fragmento;
        tm->lirbuf = new (&gc) LirBuffer(fragmento, NULL);
#ifdef DEBUG
        tm->lirbuf->names = new (&gc) LirNameMap(&gc, tm->fragmento->labels);
#endif
        for (size_t i = 0; i < MONITOR_N_GLOBAL_STATES; ++i) {
            tm->globalStates[i].globalShape = -1;
            JS_ASSERT(!tm->globalStates[i].globalSlots);
            tm->globalStates[i].globalSlots = new (&gc) SlotList();
        }
        tm->reservedDoublePoolPtr = tm->reservedDoublePool = new jsval[MAX_NATIVE_STACK_SLOTS];
        memset(tm->vmfragments, 0, sizeof(tm->vmfragments));
    }
    if (!tm->reFragmento) {
        Fragmento* fragmento = new (&gc) Fragmento(core, 32);
        verbose_only(fragmento->labels = new (&gc) LabelMap(core, NULL);)
        tm->reFragmento = fragmento;
        tm->reLirBuf = new (&gc) LirBuffer(fragmento, NULL);
    }
#if !defined XP_WIN
    debug_only(memset(&jitstats, 0, sizeof(jitstats)));
#endif
}

void
js_FinishJIT(JSTraceMonitor *tm)
{
#ifdef JS_JIT_SPEW
    if (js_verboseStats && jitstats.recorderStarted) {
        printf("recorder: started(%llu), aborted(%llu), completed(%llu), different header(%llu), "
               "trees trashed(%llu), slot promoted(%llu), unstable loop variable(%llu), "
               "breaks(%llu), returns(%llu), unstableInnerCalls(%llu)\n",
               jitstats.recorderStarted, jitstats.recorderAborted, jitstats.traceCompleted,
               jitstats.returnToDifferentLoopHeader, jitstats.treesTrashed, jitstats.slotPromoted,
               jitstats.unstableLoopVariable, jitstats.breakLoopExits, jitstats.returnLoopExits,
               jitstats.noCompatInnerTrees);
        printf("monitor: triggered(%llu), exits(%llu), type mismatch(%llu), "
               "global mismatch(%llu)\n", jitstats.traceTriggered, jitstats.sideExitIntoInterpreter,
               jitstats.typeMapMismatchAtEntry, jitstats.globalShapeMismatchAtEntry);
    }
#endif
    if (tm->fragmento != NULL) {
        JS_ASSERT(tm->reservedDoublePool);
        verbose_only(delete tm->fragmento->labels;)
#ifdef DEBUG
        delete tm->lirbuf->names;
        tm->lirbuf->names = NULL;
#endif
        delete tm->lirbuf;
        tm->lirbuf = NULL;

        if (tm->recordAttempts.ops)
            JS_DHashTableFinish(&tm->recordAttempts);

        for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
            VMFragment* f = tm->vmfragments[i];
            while(f) {
                VMFragment* next = f->next;
                tm->fragmento->clearFragment(f);
                f = next;
            }
            tm->vmfragments[i] = NULL;
        }
        delete tm->fragmento;
        tm->fragmento = NULL;
        for (size_t i = 0; i < MONITOR_N_GLOBAL_STATES; ++i) {
            JS_ASSERT(tm->globalStates[i].globalSlots);
            delete tm->globalStates[i].globalSlots;
        }
        delete[] tm->reservedDoublePool;
        tm->reservedDoublePool = tm->reservedDoublePoolPtr = NULL;
    }
    if (tm->reFragmento != NULL) {
        delete tm->reLirBuf;
        verbose_only(delete tm->reFragmento->labels;)
        delete tm->reFragmento;
    }
}

void
TraceRecorder::pushAbortStack()
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    JS_ASSERT(tm->abortStack != this);

    nextRecorderToAbort = tm->abortStack;
    tm->abortStack = this;
}

void
TraceRecorder::popAbortStack()
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    JS_ASSERT(tm->abortStack == this);

    tm->abortStack = nextRecorderToAbort;
    nextRecorderToAbort = NULL;
}

void
js_PurgeJITOracle()
{
    oracle.clear();
}

static JSDHashOperator
js_PurgeScriptRecordingAttempts(JSDHashTable *table,
                                JSDHashEntryHdr *hdr,
                                uint32 number, void *arg)
{
    PCHashEntry *e = (PCHashEntry *)hdr;
    JSScript *script = (JSScript *)arg;
    jsbytecode *pc = (jsbytecode *)e->key;

    if (JS_UPTRDIFF(pc, script->code) < script->length)
        return JS_DHASH_REMOVE;
    return JS_DHASH_NEXT;
}

JS_REQUIRES_STACK void
js_PurgeScriptFragments(JSContext* cx, JSScript* script)
{
    if (!TRACING_ENABLED(cx))
        return;
    debug_only_v(printf("Purging fragments for JSScript %p.\n", (void*)script);)
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
        for (VMFragment **f = &(tm->vmfragments[i]); *f; ) {
            VMFragment* frag = *f;
            
            if (JS_UPTRDIFF(frag->ip, script->code) < script->length) {
                JS_ASSERT(frag->root == frag);
                debug_only_v(printf("Disconnecting VMFragment %p "
                                    "with ip %p, in range [%p,%p).\n",
                                    (void*)frag, frag->ip, script->code,
                                    script->code + script->length));
                VMFragment* next = frag->next;
                js_TrashTree(cx, frag);
                *f = next;
            } else {
                f = &((*f)->next);
            }
        }
    }
    JS_DHashTableEnumerate(&(tm->recordAttempts),
                           js_PurgeScriptRecordingAttempts, script);

}

bool
js_OverfullFragmento(JSTraceMonitor* tm, Fragmento *fragmento)
{
    

































    jsuint maxsz = tm->maxCodeCacheBytes;
    if (fragmento == tm->fragmento) {
        if (tm->prohibitFlush)
            return false;
    } else {
        





        maxsz /= 16;
    }
    return (fragmento->cacheUsed() > maxsz);
}

JS_FORCES_STACK JS_FRIEND_API(void)
js_DeepBail(JSContext *cx)
{
    JS_ASSERT(JS_ON_TRACE(cx));

    



    JSTraceMonitor *tm = &JS_TRACE_MONITOR(cx);
    JSContext *tracecx = tm->tracecx;

    
    JS_ASSERT(tracecx->bailExit);

    tm->tracecx = NULL;
    tm->prohibitFlush++;
    debug_only_v(printf("Deep bail.\n");)
    LeaveTree(*tracecx->interpState, tracecx->bailExit);
    tracecx->bailExit = NULL;
    tracecx->interpState->builtinStatus |= JSBUILTIN_BAILED;
}

JS_REQUIRES_STACK jsval&
TraceRecorder::argval(unsigned n) const
{
    JS_ASSERT(n < cx->fp->fun->nargs);
    return cx->fp->argv[n];
}

JS_REQUIRES_STACK jsval&
TraceRecorder::varval(unsigned n) const
{
    JS_ASSERT(n < cx->fp->script->nslots);
    return cx->fp->slots[n];
}

JS_REQUIRES_STACK jsval&
TraceRecorder::stackval(int n) const
{
    jsval* sp = cx->fp->regs->sp;
    return sp[n];
}

JS_REQUIRES_STACK LIns*
TraceRecorder::scopeChain() const
{
    return lir->insLoad(LIR_ldp,
                        lir->insLoad(LIR_ldp, cx_ins, offsetof(JSContext, fp)),
                        offsetof(JSStackFrame, scopeChain));
}

static inline bool
FrameInRange(JSStackFrame* fp, JSStackFrame *target, unsigned callDepth)
{
    while (fp != target) {
        if (callDepth-- == 0)
            return false;
        if (!(fp = fp->down))
            return false;
    }
    return true;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::activeCallOrGlobalSlot(JSObject* obj, jsval*& vp)
{
    
    
    
    
    
    
    
    

    JS_ASSERT(obj != globalObj);

    JSAtom* atom = atoms[GET_INDEX(cx->fp->regs->pc)];
    JSObject* obj2;
    JSProperty* prop;
    if (!js_FindProperty(cx, ATOM_TO_JSID(atom), &obj, &obj2, &prop))
        ABORT_TRACE_ERROR("error in js_FindProperty");
    if (!prop)
        ABORT_TRACE("failed to find name in non-global scope chain");

    if (obj == globalObj) {
        JSScopeProperty* sprop = (JSScopeProperty*) prop;

        if (obj2 != obj) {
            OBJ_DROP_PROPERTY(cx, obj2, prop);
            ABORT_TRACE("prototype property");
        }
        if (!isValidSlot(OBJ_SCOPE(obj), sprop)) {
            OBJ_DROP_PROPERTY(cx, obj2, prop);
            return JSRS_STOP;
        }
        if (!lazilyImportGlobalSlot(sprop->slot)) {
            OBJ_DROP_PROPERTY(cx, obj2, prop);
            ABORT_TRACE("lazy import of global slot failed");
        }
        vp = &STOBJ_GET_SLOT(obj, sprop->slot);
        OBJ_DROP_PROPERTY(cx, obj2, prop);
        return JSRS_CONTINUE;
    }

    if (wasDeepAborted())
        ABORT_TRACE("deep abort from property lookup");

    if (obj == obj2 && OBJ_GET_CLASS(cx, obj) == &js_CallClass) {
        JSStackFrame* cfp = (JSStackFrame*) JS_GetPrivate(cx, obj);
        if (cfp && FrameInRange(cx->fp, cfp, callDepth)) {
            JSScopeProperty* sprop = (JSScopeProperty*) prop;
            uintN slot = sprop->shortid;

            vp = NULL;
            if (sprop->getter == js_GetCallArg) {
                JS_ASSERT(slot < cfp->fun->nargs);
                vp = &cfp->argv[slot];
            } else if (sprop->getter == js_GetCallVar) {
                JS_ASSERT(slot < cfp->script->nslots);
                vp = &cfp->slots[slot];
            }
            OBJ_DROP_PROPERTY(cx, obj2, prop);
            if (!vp)
                ABORT_TRACE("dynamic property of Call object");
            return JSRS_CONTINUE;
        }
    }

    OBJ_DROP_PROPERTY(cx, obj2, prop);
    ABORT_TRACE("fp->scopeChain is not global or active call object");
}

JS_REQUIRES_STACK LIns*
TraceRecorder::arg(unsigned n)
{
    return get(&argval(n));
}

JS_REQUIRES_STACK void
TraceRecorder::arg(unsigned n, LIns* i)
{
    set(&argval(n), i);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::var(unsigned n)
{
    return get(&varval(n));
}

JS_REQUIRES_STACK void
TraceRecorder::var(unsigned n, LIns* i)
{
    set(&varval(n), i);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::stack(int n)
{
    return get(&stackval(n));
}

JS_REQUIRES_STACK void
TraceRecorder::stack(int n, LIns* i)
{
    set(&stackval(n), i, n >= 0);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::alu(LOpcode v, jsdouble v0, jsdouble v1, LIns* s0, LIns* s1)
{
    if (v == LIR_fadd || v == LIR_fsub) {
        jsdouble r;
        if (v == LIR_fadd)
            r = v0 + v1;
        else
            r = v0 - v1;
        




        if (!JSDOUBLE_IS_NEGZERO(r) && (jsint(r) == r) && isPromoteInt(s0) && isPromoteInt(s1)) {
            LIns* d0 = ::demote(lir, s0);
            LIns* d1 = ::demote(lir, s1);
            



            if (d0->isconst() && d1->isconst())
                return lir->ins1(LIR_i2f, lir->insImm(jsint(r)));
            




            v = (LOpcode)((int)v & ~LIR64);
            LIns* result = lir->ins2(v, d0, d1);
            if (!result->isconst() && (!overflowSafe(d0) || !overflowSafe(d1))) {
                VMSideExit* exit = snapshot(OVERFLOW_EXIT);
                lir->insGuard(LIR_xt, lir->ins1(LIR_ov, result), createGuardRecord(exit));
            }
            return lir->ins1(LIR_i2f, result);
        }
        



        if (s0->isconst() && s1->isconst())
            return lir->insImmf(r);
        return lir->ins2(v, s0, s1);
    }
    return lir->ins2(v, s0, s1);
}

LIns*
TraceRecorder::f2i(LIns* f)
{
    return lir->insCall(&js_DoubleToInt32_ci, &f);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::makeNumberInt32(LIns* f)
{
    JS_ASSERT(f->isQuad());
    LIns* x;
    if (!isPromote(f)) {
        x = f2i(f);
        guard(true, lir->ins2(LIR_feq, f, lir->ins1(LIR_i2f, x)), MISMATCH_EXIT);
    } else {
        x = ::demote(lir, f);
    }
    return x;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::stringify(jsval& v)
{
    LIns* v_ins = get(&v);
    if (JSVAL_IS_STRING(v))
        return v_ins;

    LIns* args[] = { v_ins, cx_ins };
    const CallInfo* ci;
    if (JSVAL_IS_NUMBER(v)) {
        ci = &js_NumberToString_ci;
    } else if (JSVAL_TAG(v) == JSVAL_BOOLEAN) {
        ci = &js_BooleanOrUndefinedToString_ci;
    } else {
        




        JS_ASSERT(JSVAL_IS_NULL(v));
        return INS_CONSTPTR(ATOM_TO_STRING(cx->runtime->atomState.nullAtom));
    }

    v_ins = lir->insCall(ci, args);
    guard(false, lir->ins_eq0(v_ins), OOM_EXIT);
    return v_ins;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::call_imacro(jsbytecode* imacro)
{
    JSStackFrame* fp = cx->fp;
    JSFrameRegs* regs = fp->regs;

    
    if (fp->imacpc)
        return JSRS_STOP;

    fp->imacpc = regs->pc;
    regs->pc = imacro;
    atoms = COMMON_ATOMS_START(&cx->runtime->atomState);
    return JSRS_IMACRO;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::ifop()
{
    jsval& v = stackval(-1);
    LIns* v_ins = get(&v);
    bool cond;
    LIns* x;

    if (JSVAL_IS_NULL(v)) {
        cond = false;
        x = lir->insImm(0);
    } else if (!JSVAL_IS_PRIMITIVE(v)) {
        cond = true;
        x = lir->insImm(1);
    } else if (JSVAL_TAG(v) == JSVAL_BOOLEAN) {
        
        cond = JSVAL_TO_PSEUDO_BOOLEAN(v) == JS_TRUE;
        x = lir->ins2i(LIR_eq, v_ins, 1);
    } else if (isNumber(v)) {
        jsdouble d = asNumber(v);
        cond = !JSDOUBLE_IS_NaN(d) && d;
        x = lir->ins2(LIR_and,
                      lir->ins2(LIR_feq, v_ins, v_ins),
                      lir->ins_eq0(lir->ins2(LIR_feq, v_ins, lir->insImmq(0))));
    } else if (JSVAL_IS_STRING(v)) {
        cond = JSSTRING_LENGTH(JSVAL_TO_STRING(v)) != 0;
        x = lir->ins2(LIR_piand,
                      lir->insLoad(LIR_ldp,
                                   v_ins,
                                   (int)offsetof(JSString, length)),
                      INS_CONSTPTR(reinterpret_cast<void *>(JSSTRING_LENGTH_MASK)));
    } else {
        JS_NOT_REACHED("ifop");
        return JSRS_STOP;
    }

    jsbytecode* pc = cx->fp->regs->pc;
    emitIf(pc, cond, x);
    return checkTraceEnd(pc);
}

#ifdef NANOJIT_IA32



JS_REQUIRES_STACK LIns*
TraceRecorder::tableswitch()
{
    jsval& v = stackval(-1);
    if (!isNumber(v))
        return NULL;

    
    LIns* v_ins = f2i(get(&v));
    if (v_ins->isconst() || v_ins->isconstq())
        return NULL;

    jsbytecode* pc = cx->fp->regs->pc;
    
    if (anchor &&
        (anchor->exitType == CASE_EXIT || anchor->exitType == DEFAULT_EXIT) &&
        fragment->ip == pc) {
        return NULL;
    }

    
    jsint low, high;
    if (*pc == JSOP_TABLESWITCH) {
        pc += JUMP_OFFSET_LEN;
        low = GET_JUMP_OFFSET(pc);
        pc += JUMP_OFFSET_LEN;
        high = GET_JUMP_OFFSET(pc);
    } else {
        pc += JUMPX_OFFSET_LEN;
        low = GET_JUMPX_OFFSET(pc);
        pc += JUMPX_OFFSET_LEN;
        high = GET_JUMPX_OFFSET(pc);
    }

    

    if ((high + 1 - low) * sizeof(intptr_t*) + 128 > (unsigned) LARGEST_UNDERRUN_PROT) {
        
        
        (void) switchop();
        return NULL;
    }

    
    LIns* si_ins = lir_buf_writer->insSkip(sizeof(SwitchInfo));
    SwitchInfo* si = (SwitchInfo*) si_ins->payload();
    si->count = high + 1 - low;
    si->table = 0;
    si->index = (uint32) -1;
    LIns* diff = lir->ins2(LIR_sub, v_ins, lir->insImm(low));
    LIns* cmp = lir->ins2(LIR_ult, diff, lir->insImm(si->count));
    lir->insGuard(LIR_xf, cmp, createGuardRecord(snapshot(DEFAULT_EXIT)));
    lir->insStorei(diff, lir->insImmPtr(&si->index), 0);
    VMSideExit* exit = snapshot(CASE_EXIT);
    exit->switchInfo = si;
    return lir->insGuard(LIR_xtbl, diff, createGuardRecord(exit));
}
#endif

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::switchop()
{
    jsval& v = stackval(-1);
    LIns* v_ins = get(&v);
    
    if (v_ins->isconst() || v_ins->isconstq())
        return JSRS_CONTINUE;
    if (isNumber(v)) {
        jsdouble d = asNumber(v);
        guard(true,
              addName(lir->ins2(LIR_feq, v_ins, lir->insImmf(d)),
                      "guard(switch on numeric)"),
              BRANCH_EXIT);
    } else if (JSVAL_IS_STRING(v)) {
        LIns* args[] = { v_ins, INS_CONSTPTR(JSVAL_TO_STRING(v)) };
        guard(true,
              addName(lir->ins_eq0(lir->ins_eq0(lir->insCall(&js_EqualStrings_ci, args))),
                      "guard(switch on string)"),
              BRANCH_EXIT);
    } else if (JSVAL_TAG(v) == JSVAL_BOOLEAN) {
        guard(true,
              addName(lir->ins2(LIR_eq, v_ins, lir->insImm(JSVAL_TO_PUBLIC_PSEUDO_BOOLEAN(v))),
                      "guard(switch on boolean)"),
              BRANCH_EXIT);
    } else {
        ABORT_TRACE("switch on object or null");
    }
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::inc(jsval& v, jsint incr, bool pre)
{
    LIns* v_ins = get(&v);
    CHECK_STATUS(inc(v, v_ins, incr, pre));
    set(&v, v_ins);
    return JSRS_CONTINUE;
}





JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::inc(jsval& v, LIns*& v_ins, jsint incr, bool pre)
{
    if (!isNumber(v))
        ABORT_TRACE("can only inc numbers");

    LIns* v_after = alu(LIR_fadd, asNumber(v), incr, v_ins, lir->insImmf(incr));

    const JSCodeSpec& cs = js_CodeSpec[*cx->fp->regs->pc];
    JS_ASSERT(cs.ndefs == 1);
    stack(-cs.nuses, pre ? v_after : v_ins);
    v_ins = v_after;
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::incProp(jsint incr, bool pre)
{
    jsval& l = stackval(-1);
    if (JSVAL_IS_PRIMITIVE(l))
        ABORT_TRACE("incProp on primitive");

    JSObject* obj = JSVAL_TO_OBJECT(l);
    LIns* obj_ins = get(&l);

    uint32 slot;
    LIns* v_ins;
    CHECK_STATUS(prop(obj, obj_ins, slot, v_ins));

    if (slot == SPROP_INVALID_SLOT)
        ABORT_TRACE("incProp on invalid slot");

    jsval& v = STOBJ_GET_SLOT(obj, slot);
    CHECK_STATUS(inc(v, v_ins, incr, pre));

    box_jsval(v, v_ins);

    LIns* dslots_ins = NULL;
    stobj_set_slot(obj_ins, slot, dslots_ins, v_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::incElem(jsint incr, bool pre)
{
    jsval& r = stackval(-1);
    jsval& l = stackval(-2);
    jsval* vp;
    LIns* v_ins;
    LIns* addr_ins;

    if (!JSVAL_IS_OBJECT(l) || !JSVAL_IS_INT(r) ||
        !guardDenseArray(JSVAL_TO_OBJECT(l), get(&l))) {
        return JSRS_STOP;
    }

    CHECK_STATUS(denseArrayElement(l, r, vp, v_ins, addr_ins));
    if (!addr_ins) 
        return JSRS_STOP;
    CHECK_STATUS(inc(*vp, v_ins, incr, pre));
    box_jsval(*vp, v_ins);
    lir->insStorei(v_ins, addr_ins, 0);
    return JSRS_CONTINUE;
}

static bool
evalCmp(LOpcode op, double result)
{
    bool cond;
    switch (op) {
      case LIR_feq:
        cond = (result == 0);
        break;
      case LIR_flt:
        cond = result < 0;
        break;
      case LIR_fgt:
        cond = result > 0;
        break;
      case LIR_fle:
        cond = result <= 0;
        break;
      case LIR_fge:
        cond = result >= 0;
        break;
      default:
        JS_NOT_REACHED("unexpected comparison op");
        return false;
    }
    return cond;
}

static bool
evalCmp(LOpcode op, double l, double r)
{
    return evalCmp(op, l - r);
}

static bool
evalCmp(LOpcode op, JSString* l, JSString* r)
{
    if (op == LIR_feq)
        return js_EqualStrings(l, r);
    return evalCmp(op, js_CompareStrings(l, r));
}

JS_REQUIRES_STACK void
TraceRecorder::strictEquality(bool equal, bool cmpCase)
{
    jsval& r = stackval(-1);
    jsval& l = stackval(-2);
    LIns* l_ins = get(&l);
    LIns* r_ins = get(&r);
    LIns* x;
    bool cond;

    uint8 ltag = getPromotedType(l);
    if (ltag != getPromotedType(r)) {
        cond = !equal;
        x = lir->insImm(cond);
    } else if (ltag == JSVAL_STRING) {
        LIns* args[] = { r_ins, l_ins };
        x = lir->ins2i(LIR_eq, lir->insCall(&js_EqualStrings_ci, args), equal);
        cond = js_EqualStrings(JSVAL_TO_STRING(l), JSVAL_TO_STRING(r));
    } else {
        LOpcode op = (ltag != JSVAL_DOUBLE) ? LIR_eq : LIR_feq;
        x = lir->ins2(op, l_ins, r_ins);
        if (!equal)
            x = lir->ins_eq0(x);
        cond = (ltag == JSVAL_DOUBLE)
               ? asNumber(l) == asNumber(r)
               : l == r;
    }
    cond = (cond == equal);

    if (cmpCase) {
        
        if (!x->isconst())
            guard(cond, x, BRANCH_EXIT);
        return;
    }

    set(&l, x);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::equality(bool negate, bool tryBranchAfterCond)
{
    jsval& rval = stackval(-1);
    jsval& lval = stackval(-2);
    LIns* l_ins = get(&lval);
    LIns* r_ins = get(&rval);

    return equalityHelper(lval, rval, l_ins, r_ins, negate, tryBranchAfterCond, lval);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::equalityHelper(jsval l, jsval r, LIns* l_ins, LIns* r_ins,
                              bool negate, bool tryBranchAfterCond,
                              jsval& rval)
{
    bool fp = false;
    bool cond;
    LIns* args[] = { NULL, NULL };

    










    if (getPromotedType(l) == getPromotedType(r)) {
        if (JSVAL_TAG(l) == JSVAL_OBJECT || JSVAL_TAG(l) == JSVAL_BOOLEAN) {
            cond = (l == r);
        } else if (JSVAL_IS_STRING(l)) {
            args[0] = r_ins, args[1] = l_ins;
            l_ins = lir->insCall(&js_EqualStrings_ci, args);
            r_ins = lir->insImm(1);
            cond = js_EqualStrings(JSVAL_TO_STRING(l), JSVAL_TO_STRING(r));
        } else {
            JS_ASSERT(isNumber(l) && isNumber(r));
            cond = (asNumber(l) == asNumber(r));
            fp = true;
        }
    } else if (JSVAL_IS_NULL(l) && JSVAL_TAG(r) == JSVAL_BOOLEAN) {
        l_ins = lir->insImm(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID));
        cond = (r == JSVAL_VOID);
    } else if (JSVAL_TAG(l) == JSVAL_BOOLEAN && JSVAL_IS_NULL(r)) {
        r_ins = lir->insImm(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID));
        cond = (l == JSVAL_VOID);
    } else if (isNumber(l) && JSVAL_IS_STRING(r)) {
        args[0] = r_ins, args[1] = cx_ins;
        r_ins = lir->insCall(&js_StringToNumber_ci, args);
        cond = (asNumber(l) == js_StringToNumber(cx, JSVAL_TO_STRING(r)));
        fp = true;
    } else if (JSVAL_IS_STRING(l) && isNumber(r)) {
        args[0] = l_ins, args[1] = cx_ins;
        l_ins = lir->insCall(&js_StringToNumber_ci, args);
        cond = (js_StringToNumber(cx, JSVAL_TO_STRING(l)) == asNumber(r));
        fp = true;
    } else {
        if (JSVAL_TAG(l) == JSVAL_BOOLEAN) {
            bool isVoid = JSVAL_IS_VOID(l);
            guard(isVoid,
                  lir->ins2(LIR_eq, l_ins, INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID))),
                  BRANCH_EXIT);
            if (!isVoid) {
                args[0] = l_ins, args[1] = cx_ins;
                l_ins = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
                l = (l == JSVAL_VOID)
                    ? DOUBLE_TO_JSVAL(cx->runtime->jsNaN)
                    : INT_TO_JSVAL(l == JSVAL_TRUE);
                return equalityHelper(l, r, l_ins, r_ins, negate,
                                      tryBranchAfterCond, rval);
            }
        } else if (JSVAL_TAG(r) == JSVAL_BOOLEAN) {
            bool isVoid = JSVAL_IS_VOID(r);
            guard(isVoid,
                  lir->ins2(LIR_eq, r_ins, INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID))),
                  BRANCH_EXIT);
            if (!isVoid) {
                args[0] = r_ins, args[1] = cx_ins;
                r_ins = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
                r = (r == JSVAL_VOID)
                    ? DOUBLE_TO_JSVAL(cx->runtime->jsNaN)
                    : INT_TO_JSVAL(r == JSVAL_TRUE);
                return equalityHelper(l, r, l_ins, r_ins, negate,
                                      tryBranchAfterCond, rval);
            }
        } else {
            if ((JSVAL_IS_STRING(l) || isNumber(l)) && !JSVAL_IS_PRIMITIVE(r)) {
                ABORT_IF_XML(r);
                return call_imacro(equality_imacros.any_obj);
            }
            if (!JSVAL_IS_PRIMITIVE(l) && (JSVAL_IS_STRING(r) || isNumber(r))) {
                ABORT_IF_XML(l);
                return call_imacro(equality_imacros.obj_any);
            }
        }

        l_ins = lir->insImm(0);
        r_ins = lir->insImm(1);
        cond = false;
    }

    
    LOpcode op = fp ? LIR_feq : LIR_eq;
    LIns* x = lir->ins2(op, l_ins, r_ins);
    if (negate) {
        x = lir->ins_eq0(x);
        cond = !cond;
    }

    jsbytecode* pc = cx->fp->regs->pc;

    




    if (tryBranchAfterCond)
        fuseIf(pc + 1, cond, x);

    



    if (pc[1] == JSOP_IFNE || pc[1] == JSOP_IFEQ)
        CHECK_STATUS(checkTraceEnd(pc + 1));

    





    set(&rval, x);

    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::relational(LOpcode op, bool tryBranchAfterCond)
{
    jsval& r = stackval(-1);
    jsval& l = stackval(-2);
    LIns* x = NULL;
    bool cond;
    LIns* l_ins = get(&l);
    LIns* r_ins = get(&r);
    bool fp = false;
    jsdouble lnum, rnum;

    




    if (!JSVAL_IS_PRIMITIVE(l)) {
        ABORT_IF_XML(l);
        if (!JSVAL_IS_PRIMITIVE(r)) {
            ABORT_IF_XML(r);
            return call_imacro(binary_imacros.obj_obj);
        }
        return call_imacro(binary_imacros.obj_any);
    }
    if (!JSVAL_IS_PRIMITIVE(r)) {
        ABORT_IF_XML(r);
        return call_imacro(binary_imacros.any_obj);
    }

    
    if (JSVAL_IS_STRING(l) && JSVAL_IS_STRING(r)) {
        LIns* args[] = { r_ins, l_ins };
        l_ins = lir->insCall(&js_CompareStrings_ci, args);
        r_ins = lir->insImm(0);
        cond = evalCmp(op, JSVAL_TO_STRING(l), JSVAL_TO_STRING(r));
        goto do_comparison;
    }

    
    if (!JSVAL_IS_NUMBER(l)) {
        LIns* args[] = { l_ins, cx_ins };
        switch (JSVAL_TAG(l)) {
          case JSVAL_BOOLEAN:
            l_ins = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
            break;
          case JSVAL_STRING:
            l_ins = lir->insCall(&js_StringToNumber_ci, args);
            break;
          case JSVAL_OBJECT:
            if (JSVAL_IS_NULL(l)) {
                l_ins = lir->insImmq(0);
                break;
            }
            
          case JSVAL_INT:
          case JSVAL_DOUBLE:
          default:
            JS_NOT_REACHED("JSVAL_IS_NUMBER if int/double, objects should "
                           "have been handled at start of method");
            ABORT_TRACE("safety belt");
        }
    }
    if (!JSVAL_IS_NUMBER(r)) {
        LIns* args[] = { r_ins, cx_ins };
        switch (JSVAL_TAG(r)) {
          case JSVAL_BOOLEAN:
            r_ins = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
            break;
          case JSVAL_STRING:
            r_ins = lir->insCall(&js_StringToNumber_ci, args);
            break;
          case JSVAL_OBJECT:
            if (JSVAL_IS_NULL(r)) {
                r_ins = lir->insImmq(0);
                break;
            }
            
          case JSVAL_INT:
          case JSVAL_DOUBLE:
          default:
            JS_NOT_REACHED("JSVAL_IS_NUMBER if int/double, objects should "
                           "have been handled at start of method");
            ABORT_TRACE("safety belt");
        }
    }
    {
        jsval tmp = JSVAL_NULL;
        JSAutoTempValueRooter tvr(cx, 1, &tmp);

        tmp = l;
        lnum = js_ValueToNumber(cx, &tmp);
        tmp = r;
        rnum = js_ValueToNumber(cx, &tmp);
    }
    cond = evalCmp(op, lnum, rnum);
    fp = true;

    
  do_comparison:
    
    if (!fp) {
        JS_ASSERT(op >= LIR_feq && op <= LIR_fge);
        op = LOpcode(op + (LIR_eq - LIR_feq));
    }
    x = lir->ins2(op, l_ins, r_ins);

    jsbytecode* pc = cx->fp->regs->pc;

    




    if (tryBranchAfterCond)
        fuseIf(pc + 1, cond, x);

    



    if (pc[1] == JSOP_IFNE || pc[1] == JSOP_IFEQ)
        CHECK_STATUS(checkTraceEnd(pc + 1));

    





    set(&l, x);

    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::unary(LOpcode op)
{
    jsval& v = stackval(-1);
    bool intop = !(op & LIR64);
    if (isNumber(v)) {
        LIns* a = get(&v);
        if (intop)
            a = f2i(a);
        a = lir->ins1(op, a);
        if (intop)
            a = lir->ins1(LIR_i2f, a);
        set(&v, a);
        return JSRS_CONTINUE;
    }
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::binary(LOpcode op)
{
    jsval& r = stackval(-1);
    jsval& l = stackval(-2);

    if (!JSVAL_IS_PRIMITIVE(l)) {
        ABORT_IF_XML(l);
        if (!JSVAL_IS_PRIMITIVE(r)) {
            ABORT_IF_XML(r);
            return call_imacro(binary_imacros.obj_obj);
        }
        return call_imacro(binary_imacros.obj_any);
    }
    if (!JSVAL_IS_PRIMITIVE(r)) {
        ABORT_IF_XML(r);
        return call_imacro(binary_imacros.any_obj);
    }

    bool intop = !(op & LIR64);
    LIns* a = get(&l);
    LIns* b = get(&r);

    bool leftIsNumber = isNumber(l);
    jsdouble lnum = leftIsNumber ? asNumber(l) : 0;

    bool rightIsNumber = isNumber(r);
    jsdouble rnum = rightIsNumber ? asNumber(r) : 0;

    if ((op >= LIR_sub && op <= LIR_ush) ||  
        (op >= LIR_fsub && op <= LIR_fdiv)) { 
        LIns* args[2];
        if (JSVAL_IS_STRING(l)) {
            args[0] = a;
            args[1] = cx_ins;
            a = lir->insCall(&js_StringToNumber_ci, args);
            lnum = js_StringToNumber(cx, JSVAL_TO_STRING(l));
            leftIsNumber = true;
        }
        if (JSVAL_IS_STRING(r)) {
            args[0] = b;
            args[1] = cx_ins;
            b = lir->insCall(&js_StringToNumber_ci, args);
            rnum = js_StringToNumber(cx, JSVAL_TO_STRING(r));
            rightIsNumber = true;
        }
    }
    if (JSVAL_TAG(l) == JSVAL_BOOLEAN) {
        LIns* args[] = { a, cx_ins };
        a = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
        lnum = js_BooleanOrUndefinedToNumber(cx, JSVAL_TO_PSEUDO_BOOLEAN(l));
        leftIsNumber = true;
    }
    if (JSVAL_TAG(r) == JSVAL_BOOLEAN) {
        LIns* args[] = { b, cx_ins };
        b = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
        rnum = js_BooleanOrUndefinedToNumber(cx, JSVAL_TO_PSEUDO_BOOLEAN(r));
        rightIsNumber = true;
    }
    if (leftIsNumber && rightIsNumber) {
        if (intop) {
            LIns *args[] = { a };
            a = lir->insCall(op == LIR_ush ? &js_DoubleToUint32_ci : &js_DoubleToInt32_ci, args);
            b = f2i(b);
        }
        a = alu(op, lnum, rnum, a, b);
        if (intop)
            a = lir->ins1(op == LIR_ush ? LIR_u2f : LIR_i2f, a);
        set(&l, a);
        return JSRS_CONTINUE;
    }
    return JSRS_STOP;
}

JS_STATIC_ASSERT(offsetof(JSObjectOps, objectMap) == 0);

bool
TraceRecorder::map_is_native(JSObjectMap* map, LIns* map_ins, LIns*& ops_ins, size_t op_offset)
{
    JS_ASSERT(op_offset < sizeof(JSObjectOps));
    JS_ASSERT(op_offset % sizeof(void *) == 0);

#define OP(ops) (*(void **) ((uint8 *) (ops) + op_offset))
    void* ptr = OP(map->ops);
    if (ptr != OP(&js_ObjectOps))
        return false;
#undef OP

    ops_ins = addName(lir->insLoad(LIR_ldp, map_ins, int(offsetof(JSObjectMap, ops))), "ops");
    LIns* n = lir->insLoad(LIR_ldp, ops_ins, op_offset);
    guard(true,
          addName(lir->ins2(LIR_eq, n, INS_CONSTPTR(ptr)), "guard(native-map)"),
          BRANCH_EXIT);

    return true;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::test_property_cache(JSObject* obj, LIns* obj_ins, JSObject*& obj2, jsuword& pcval)
{
    jsbytecode* pc = cx->fp->regs->pc;
    JS_ASSERT(*pc != JSOP_INITPROP && *pc != JSOP_SETNAME && *pc != JSOP_SETPROP);

    
    
    
    JSObject* aobj = obj;
    if (OBJ_IS_DENSE_ARRAY(cx, obj)) {
        guardDenseArray(obj, obj_ins, BRANCH_EXIT);
        aobj = OBJ_GET_PROTO(cx, obj);
        obj_ins = stobj_get_fslot(obj_ins, JSSLOT_PROTO);
    }

    LIns* map_ins = lir->insLoad(LIR_ldp, obj_ins, (int)offsetof(JSObject, map));
    LIns* ops_ins;

    
    
    
    
    
    
    
    
    uint32 format = js_CodeSpec[*pc].format;
    uint32 mode = JOF_MODE(format);

    
    JS_ASSERT(OBJ_IS_NATIVE(globalObj));
    if (aobj != globalObj) {
        size_t op_offset = offsetof(JSObjectOps, objectMap);
        if (mode == JOF_PROP || mode == JOF_VARPROP) {
            JS_ASSERT(!(format & JOF_SET));
            op_offset = offsetof(JSObjectOps, getProperty);
        } else {
            JS_ASSERT(mode == JOF_NAME);
        }

        if (!map_is_native(aobj->map, map_ins, ops_ins, op_offset))
            ABORT_TRACE("non-native map");
    }

    JSAtom* atom;
    JSPropCacheEntry* entry;
    PROPERTY_CACHE_TEST(cx, pc, aobj, obj2, entry, atom);
    if (!atom) {
        
        JS_UNLOCK_OBJ(cx, obj2);
    } else {
        
        jsid id = ATOM_TO_JSID(atom);
        JSProperty* prop;
        if (JOF_OPMODE(*pc) == JOF_NAME) {
            JS_ASSERT(aobj == obj);
            entry = js_FindPropertyHelper(cx, id, true, &obj, &obj2, &prop);

            if (!entry)
                ABORT_TRACE_ERROR("error in js_FindPropertyHelper");
            if (entry == JS_NO_PROP_CACHE_FILL)
                ABORT_TRACE("cannot cache name");
        } else {
            int protoIndex = js_LookupPropertyWithFlags(cx, aobj, id,
                                                        cx->resolveFlags,
                                                        &obj2, &prop);

            if (protoIndex < 0)
                ABORT_TRACE_ERROR("error in js_LookupPropertyWithFlags");

            if (prop) {
                if (!OBJ_IS_NATIVE(obj2)) {
                    OBJ_DROP_PROPERTY(cx, obj2, prop);
                    ABORT_TRACE("property found on non-native object");
                }
                entry = js_FillPropertyCache(cx, aobj, 0, protoIndex, obj2,
                                             (JSScopeProperty*) prop, false);
                JS_ASSERT(entry);
                if (entry == JS_NO_PROP_CACHE_FILL)
                    entry = NULL;
            }
        }

        if (!prop) {
            
            
            
            obj2 = obj;

            
            pcval = PCVAL_NULL;
            return JSRS_CONTINUE;
        }

        OBJ_DROP_PROPERTY(cx, obj2, prop);
        if (!entry)
            ABORT_TRACE("failed to fill property cache");
    }

    if (wasDeepAborted())
        ABORT_TRACE("deep abort from property lookup");

#ifdef JS_THREADSAFE
    
    
    
    
    JS_ASSERT(cx->requestDepth);
#endif

    
    
    
    if (PCVCAP_TAG(entry->vcap) <= 1) {
        if (aobj != globalObj) {
            LIns* shape_ins = addName(lir->insLoad(LIR_ld, map_ins, offsetof(JSScope, shape)),
                                      "shape");
            guard(true, addName(lir->ins2i(LIR_eq, shape_ins, entry->kshape), "guard(kshape)"),
                  BRANCH_EXIT);
        }
    } else {
#ifdef DEBUG
        JSOp op = js_GetOpcode(cx, cx->fp->script, pc);
        JSAtom *pcatom;
        if (op == JSOP_LENGTH) {
            pcatom = cx->runtime->atomState.lengthAtom;
        } else {
            ptrdiff_t pcoff = (JOF_TYPE(js_CodeSpec[op].format) == JOF_SLOTATOM) ? SLOTNO_LEN : 0;
            GET_ATOM_FROM_BYTECODE(cx->fp->script, pc, pcoff, pcatom);
        }
        JS_ASSERT(entry->kpc == (jsbytecode *) pcatom);
        JS_ASSERT(entry->kshape == jsuword(aobj));
#endif
        if (aobj != globalObj && !obj_ins->isconstp()) {
            guard(true, addName(lir->ins2i(LIR_eq, obj_ins, entry->kshape), "guard(kobj)"),
                  BRANCH_EXIT);
        }
    }

    
    
    if (PCVCAP_TAG(entry->vcap) >= 1) {
        jsuword vcap = entry->vcap;
        uint32 vshape = PCVCAP_SHAPE(vcap);
        JS_ASSERT(OBJ_SHAPE(obj2) == vshape);

        LIns* obj2_ins;
        if (PCVCAP_TAG(entry->vcap) == 1) {
            
            obj2_ins = stobj_get_fslot(obj_ins, JSSLOT_PROTO);
            guard(false, lir->ins_eq0(obj2_ins), BRANCH_EXIT);
        } else {
            obj2_ins = INS_CONSTPTR(obj2);
        }
        map_ins = lir->insLoad(LIR_ldp, obj2_ins, (int)offsetof(JSObject, map));
        if (!map_is_native(obj2->map, map_ins, ops_ins))
            ABORT_TRACE("non-native map");

        LIns* shape_ins = addName(lir->insLoad(LIR_ld, map_ins, offsetof(JSScope, shape)),
                                  "shape");
        guard(true,
              addName(lir->ins2i(LIR_eq, shape_ins, vshape), "guard(vshape)"),
              BRANCH_EXIT);
    }

    pcval = entry->vword;
    return JSRS_CONTINUE;
}

void
TraceRecorder::stobj_set_fslot(LIns *obj_ins, unsigned slot, LIns* v_ins, const char *name)
{
    addName(lir->insStorei(v_ins, obj_ins, offsetof(JSObject, fslots) + slot * sizeof(jsval)),
            name);
}

void
TraceRecorder::stobj_set_dslot(LIns *obj_ins, unsigned slot, LIns*& dslots_ins, LIns* v_ins,
                               const char *name)
{
    if (!dslots_ins)
        dslots_ins = lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, dslots));
    addName(lir->insStorei(v_ins, dslots_ins, slot * sizeof(jsval)), name);
}

void
TraceRecorder::stobj_set_slot(LIns* obj_ins, unsigned slot, LIns*& dslots_ins, LIns* v_ins)
{
    if (slot < JS_INITIAL_NSLOTS) {
        stobj_set_fslot(obj_ins, slot, v_ins, "set_slot(fslots)");
    } else {
        stobj_set_dslot(obj_ins, slot - JS_INITIAL_NSLOTS, dslots_ins, v_ins,
                        "set_slot(dslots)");
    }
}

LIns*
TraceRecorder::stobj_get_fslot(LIns* obj_ins, unsigned slot)
{
    JS_ASSERT(slot < JS_INITIAL_NSLOTS);
    return lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, fslots) + slot * sizeof(jsval));
}

LIns*
TraceRecorder::stobj_get_dslot(LIns* obj_ins, unsigned index, LIns*& dslots_ins)
{
    if (!dslots_ins)
        dslots_ins = lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, dslots));
    return lir->insLoad(LIR_ldp, dslots_ins, index * sizeof(jsval));
}

LIns*
TraceRecorder::stobj_get_slot(LIns* obj_ins, unsigned slot, LIns*& dslots_ins)
{
    if (slot < JS_INITIAL_NSLOTS)
        return stobj_get_fslot(obj_ins, slot);
    return stobj_get_dslot(obj_ins, slot - JS_INITIAL_NSLOTS, dslots_ins);
}

JSRecordingStatus
TraceRecorder::native_set(LIns* obj_ins, JSScopeProperty* sprop, LIns*& dslots_ins, LIns* v_ins)
{
    if (SPROP_HAS_STUB_SETTER(sprop) && sprop->slot != SPROP_INVALID_SLOT) {
        stobj_set_slot(obj_ins, sprop->slot, dslots_ins, v_ins);
        return JSRS_CONTINUE;
    }
    ABORT_TRACE("unallocated or non-stub sprop");
}

JSRecordingStatus
TraceRecorder::native_get(LIns* obj_ins, LIns* pobj_ins, JSScopeProperty* sprop,
                          LIns*& dslots_ins, LIns*& v_ins)
{
    if (!SPROP_HAS_STUB_GETTER(sprop))
        return JSRS_STOP;

    if (sprop->slot != SPROP_INVALID_SLOT)
        v_ins = stobj_get_slot(pobj_ins, sprop->slot, dslots_ins);
    else
        v_ins = INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK void
TraceRecorder::box_jsval(jsval v, LIns*& v_ins)
{
    if (isNumber(v)) {
        LIns* args[] = { v_ins, cx_ins };
        v_ins = lir->insCall(&js_BoxDouble_ci, args);
        guard(false, lir->ins2(LIR_eq, v_ins, INS_CONST(JSVAL_ERROR_COOKIE)),
              OOM_EXIT);
        return;
    }
    switch (JSVAL_TAG(v)) {
      case JSVAL_BOOLEAN:
        v_ins = lir->ins2i(LIR_pior, lir->ins2i(LIR_pilsh, v_ins, JSVAL_TAGBITS), JSVAL_BOOLEAN);
        return;
      case JSVAL_OBJECT:
        return;
      default:
        JS_ASSERT(JSVAL_TAG(v) == JSVAL_STRING);
        v_ins = lir->ins2(LIR_pior, v_ins, INS_CONST(JSVAL_STRING));
        return;
    }
}

JS_REQUIRES_STACK void
TraceRecorder::unbox_jsval(jsval v, LIns*& v_ins, VMSideExit* exit)
{
    if (isNumber(v)) {
        
        guard(false,
              lir->ins_eq0(lir->ins2(LIR_pior,
                                     lir->ins2(LIR_piand, v_ins, INS_CONST(JSVAL_INT)),
                                     lir->ins2i(LIR_eq,
                                                lir->ins2(LIR_piand, v_ins,
                                                          INS_CONST(JSVAL_TAGMASK)),
                                                JSVAL_DOUBLE))),
              exit);
        LIns* args[] = { v_ins };
        v_ins = lir->insCall(&js_UnboxDouble_ci, args);
        return;
    }
    switch (JSVAL_TAG(v)) {
      case JSVAL_BOOLEAN:
        guard(true,
              lir->ins2i(LIR_eq,
                         lir->ins2(LIR_piand, v_ins, INS_CONST(JSVAL_TAGMASK)),
                         JSVAL_BOOLEAN),
              exit);
        v_ins = lir->ins2i(LIR_ush, v_ins, JSVAL_TAGBITS);
        return;
      case JSVAL_OBJECT:
        if (JSVAL_IS_NULL(v)) {
            
            guard(true, lir->ins_eq0(v_ins), exit);
        } else {
            guard(false, lir->ins_eq0(v_ins), exit);
            guard(true,
                  lir->ins2i(LIR_eq,
                             lir->ins2(LIR_piand, v_ins, INS_CONSTWORD(JSVAL_TAGMASK)),
                             JSVAL_OBJECT),
                  exit);
            guard(HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v)),
                  lir->ins2(LIR_eq,
                            lir->ins2(LIR_piand,
                                      lir->insLoad(LIR_ldp, v_ins, offsetof(JSObject, classword)),
                                      INS_CONSTWORD(~JSSLOT_CLASS_MASK_BITS)),
                            INS_CONSTPTR(&js_FunctionClass)),
                  exit);
        }
        return;
      default:
        JS_ASSERT(JSVAL_TAG(v) == JSVAL_STRING);
        guard(true,
              lir->ins2i(LIR_eq,
                        lir->ins2(LIR_piand, v_ins, INS_CONST(JSVAL_TAGMASK)),
                        JSVAL_STRING),
              exit);
        v_ins = lir->ins2(LIR_piand, v_ins, INS_CONST(~JSVAL_TAGMASK));
        return;
    }
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::getThis(LIns*& this_ins)
{
    JSObject* thisObj = js_ComputeThisForFrame(cx, cx->fp);
    if (!thisObj)
        ABORT_TRACE_ERROR("js_ComputeThisForName failed");

    


    if (!cx->fp->callee) {
        JS_ASSERT(callDepth == 0);
        this_ins = INS_CONSTPTR(thisObj);

        


        return JSRS_CONTINUE;
    }

    jsval& thisv = cx->fp->argv[-1];

    






    if (JSVAL_IS_NULL(thisv)) {
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(thisv));
        if (thisObj != globalObj)
            ABORT_TRACE("global object was wrapped while recording");
        this_ins = INS_CONSTPTR(thisObj);
        set(&thisv, this_ins);
        return JSRS_CONTINUE;
    }
    this_ins = get(&thisv);

    







    if (guardClass(JSVAL_TO_OBJECT(thisv), this_ins, &js_WithClass, snapshot(MISMATCH_EXIT)))
        ABORT_TRACE("can't trace getThis on With object");

    return JSRS_CONTINUE;
}


LIns*
TraceRecorder::getStringLength(LIns* str_ins)
{
    LIns* len_ins = lir->insLoad(LIR_ldp, str_ins, (int)offsetof(JSString, length));

    LIns* masked_len_ins = lir->ins2(LIR_piand,
                                     len_ins,
                                     INS_CONSTWORD(JSSTRING_LENGTH_MASK));

    return
        lir->ins_choose(lir->ins_eq0(lir->ins2(LIR_piand,
                                               len_ins,
                                               INS_CONSTWORD(JSSTRFLAG_DEPENDENT))),
                        masked_len_ins,
                        lir->ins_choose(lir->ins_eq0(lir->ins2(LIR_piand,
                                                               len_ins,
                                                               INS_CONSTWORD(JSSTRFLAG_PREFIX))),
                                        lir->ins2(LIR_piand,
                                                  len_ins,
                                                  INS_CONSTWORD(JSSTRDEP_LENGTH_MASK)),
                                        masked_len_ins));
}

JS_REQUIRES_STACK bool
TraceRecorder::guardClass(JSObject* obj, LIns* obj_ins, JSClass* clasp, VMSideExit* exit)
{
    bool cond = STOBJ_GET_CLASS(obj) == clasp;

    LIns* class_ins = lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, classword));
    class_ins = lir->ins2(LIR_piand, class_ins, lir->insImm(~JSSLOT_CLASS_MASK_BITS));

    char namebuf[32];
    JS_snprintf(namebuf, sizeof namebuf, "guard(class is %s)", clasp->name);
    guard(cond, addName(lir->ins2(LIR_eq, class_ins, INS_CONSTPTR(clasp)), namebuf), exit);
    return cond;
}

JS_REQUIRES_STACK bool
TraceRecorder::guardDenseArray(JSObject* obj, LIns* obj_ins, ExitType exitType)
{
    return guardClass(obj, obj_ins, &js_ArrayClass, snapshot(exitType));
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::guardPrototypeHasNoIndexedProperties(JSObject* obj, LIns* obj_ins, ExitType exitType)
{
    



    VMSideExit* exit = snapshot(exitType);

    if (js_PrototypeHasIndexedProperties(cx, obj))
        return JSRS_STOP;

    while ((obj = JSVAL_TO_OBJECT(obj->fslots[JSSLOT_PROTO])) != NULL) {
        obj_ins = stobj_get_fslot(obj_ins, JSSLOT_PROTO);
        LIns* map_ins = lir->insLoad(LIR_ldp, obj_ins, (int)offsetof(JSObject, map));
        LIns* ops_ins;
        if (!map_is_native(obj->map, map_ins, ops_ins))
            ABORT_TRACE("non-native object involved along prototype chain");

        LIns* shape_ins = addName(lir->insLoad(LIR_ld, map_ins, offsetof(JSScope, shape)),
                                  "shape");
        guard(true,
              addName(lir->ins2i(LIR_eq, shape_ins, OBJ_SHAPE(obj)), "guard(shape)"),
              exit);
    }
    return JSRS_CONTINUE;
}

JSRecordingStatus
TraceRecorder::guardNotGlobalObject(JSObject* obj, LIns* obj_ins)
{
    if (obj == globalObj)
        ABORT_TRACE("reference aliases global object");
    guard(false, lir->ins2(LIR_eq, obj_ins, INS_CONSTPTR(globalObj)), MISMATCH_EXIT);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK void
TraceRecorder::clearFrameSlotsFromCache()
{
    



    JSStackFrame* fp = cx->fp;
    jsval* vp;
    jsval* vpstop;
    if (fp->callee) {
        vp = &fp->argv[-2];
        vpstop = &fp->argv[argSlots(fp)];
        while (vp < vpstop)
            nativeFrameTracker.set(vp++, (LIns*)0);
    }
    vp = &fp->slots[0];
    vpstop = &fp->slots[fp->script->nslots];
    while (vp < vpstop)
        nativeFrameTracker.set(vp++, (LIns*)0);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_EnterFrame()
{
    JSStackFrame* fp = cx->fp;

    if (++callDepth >= MAX_CALLDEPTH)
        ABORT_TRACE("exceeded maximum call depth");
    
    
    if (fp->script == fp->down->script && fp->down->down && fp->down->down->script == fp->script)
        ABORT_TRACE("recursive call");

    debug_only_v(printf("EnterFrame %s, callDepth=%d\n",
                        js_AtomToPrintableString(cx, cx->fp->fun->atom),
                        callDepth);)
    debug_only_v(
        js_Disassemble(cx, cx->fp->script, JS_TRUE, stdout);
        printf("----\n");)
    LIns* void_ins = INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID));

    jsval* vp = &fp->argv[fp->argc];
    jsval* vpstop = vp + ptrdiff_t(fp->fun->nargs) - ptrdiff_t(fp->argc);
    while (vp < vpstop) {
        if (vp >= fp->down->regs->sp)
            nativeFrameTracker.set(vp, (LIns*)0);
        set(vp++, void_ins, true);
    }

    vp = &fp->slots[0];
    vpstop = vp + fp->script->nfixed;
    while (vp < vpstop)
        set(vp++, void_ins, true);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_LeaveFrame()
{
    debug_only_v(
        if (cx->fp->fun)
            printf("LeaveFrame (back to %s), callDepth=%d\n",
                   js_AtomToPrintableString(cx, cx->fp->fun->atom),
                   callDepth);
        );
    if (callDepth-- <= 0)
        ABORT_TRACE("returned out of a loop we started tracing");

    
    
    atoms = FrameAtomBase(cx, cx->fp);
    set(&stackval(-1), rval_ins, true);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_PUSH()
{
    stack(0, INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_POPV()
{
    jsval& rval = stackval(-1);
    LIns *rval_ins = get(&rval);
    box_jsval(rval, rval_ins);

    
    
    
    LIns *fp_ins = lir->insLoad(LIR_ldp, cx_ins, offsetof(JSContext, fp));
    lir->insStorei(rval_ins, fp_ins, offsetof(JSStackFrame, rval));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ENTERWITH()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LEAVEWITH()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_RETURN()
{
    
    if (callDepth == 0) {
        AUDIT(returnLoopExits);
        endLoop(traceMonitor);
        return JSRS_STOP;
    }

    
    jsval& rval = stackval(-1);
    JSStackFrame *fp = cx->fp;
    if ((cx->fp->flags & JSFRAME_CONSTRUCTING) && JSVAL_IS_PRIMITIVE(rval)) {
        JS_ASSERT(OBJECT_TO_JSVAL(fp->thisp) == fp->argv[-1]);
        rval_ins = get(&fp->argv[-1]);
    } else {
        rval_ins = get(&rval);
    }
    debug_only_v(printf("returning from %s\n", js_AtomToPrintableString(cx, cx->fp->fun->atom));)
    clearFrameSlotsFromCache();

    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GOTO()
{
    



    jssrcnote* sn = js_GetSrcNote(cx->fp->script, cx->fp->regs->pc);

    if (sn && SN_TYPE(sn) == SRC_BREAK) {
        AUDIT(breakLoopExits);
        endLoop(traceMonitor);
        return JSRS_STOP;
    }
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_IFEQ()
{
    trackCfgMerges(cx->fp->regs->pc);
    return ifop();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_IFNE()
{
    return ifop();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ARGUMENTS()
{
#if 1
    ABORT_TRACE("can't trace arguments yet");
#else
    LIns* args[] = { cx_ins };
    LIns* a_ins = lir->insCall(&js_Arguments_ci, args);
    guard(false, lir->ins_eq0(a_ins), OOM_EXIT);
    stack(0, a_ins);
    return JSRS_CONTINUE;
#endif
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DUP()
{
    stack(0, get(&stackval(-1)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DUP2()
{
    stack(0, get(&stackval(-2)));
    stack(1, get(&stackval(-1)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SWAP()
{
    jsval& l = stackval(-2);
    jsval& r = stackval(-1);
    LIns* l_ins = get(&l);
    LIns* r_ins = get(&r);
    set(&r, l_ins);
    set(&l, r_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_PICK()
{
    jsval* sp = cx->fp->regs->sp;
    jsint n = cx->fp->regs->pc[1];
    JS_ASSERT(sp - (n+1) >= StackBase(cx->fp));
    LIns* top = get(sp - (n+1));
    for (jsint i = 0; i < n; ++i)
        set(sp - (n+1) + i, get(sp - n + i));
    set(&sp[-1], top);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETCONST()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_BITOR()
{
    return binary(LIR_or);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_BITXOR()
{
    return binary(LIR_xor);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_BITAND()
{
    return binary(LIR_and);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_EQ()
{
    return equality(false, true);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NE()
{
    return equality(true, true);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LT()
{
    return relational(LIR_flt, true);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LE()
{
    return relational(LIR_fle, true);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GT()
{
    return relational(LIR_fgt, true);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GE()
{
    return relational(LIR_fge, true);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LSH()
{
    return binary(LIR_lsh);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_RSH()
{
    return binary(LIR_rsh);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_URSH()
{
    return binary(LIR_ush);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ADD()
{
    jsval& r = stackval(-1);
    jsval& l = stackval(-2);

    if (!JSVAL_IS_PRIMITIVE(l)) {
        ABORT_IF_XML(l);
        if (!JSVAL_IS_PRIMITIVE(r)) {
            ABORT_IF_XML(r);
            return call_imacro(add_imacros.obj_obj);
        }
        return call_imacro(add_imacros.obj_any);
    }
    if (!JSVAL_IS_PRIMITIVE(r)) {
        ABORT_IF_XML(r);
        return call_imacro(add_imacros.any_obj);
    }

    if (JSVAL_IS_STRING(l) || JSVAL_IS_STRING(r)) {
        LIns* args[] = { stringify(r), stringify(l), cx_ins };
        LIns* concat = lir->insCall(&js_ConcatStrings_ci, args);
        guard(false, lir->ins_eq0(concat), OOM_EXIT);
        set(&l, concat);
        return JSRS_CONTINUE;
    }

    return binary(LIR_fadd);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SUB()
{
    return binary(LIR_fsub);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_MUL()
{
    return binary(LIR_fmul);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DIV()
{
    return binary(LIR_fdiv);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_MOD()
{
    jsval& r = stackval(-1);
    jsval& l = stackval(-2);

    if (!JSVAL_IS_PRIMITIVE(l)) {
        ABORT_IF_XML(l);
        if (!JSVAL_IS_PRIMITIVE(r)) {
            ABORT_IF_XML(r);
            return call_imacro(binary_imacros.obj_obj);
        }
        return call_imacro(binary_imacros.obj_any);
    }
    if (!JSVAL_IS_PRIMITIVE(r)) {
        ABORT_IF_XML(r);
        return call_imacro(binary_imacros.any_obj);
    }

    if (isNumber(l) && isNumber(r)) {
        LIns* l_ins = get(&l);
        LIns* r_ins = get(&r);
        LIns* x;
        
        if (isPromote(l_ins) && isPromote(r_ins) && asNumber(l) >= 0 && asNumber(r) > 0) {
            LIns* args[] = { ::demote(lir, r_ins), ::demote(lir, l_ins) };
            x = lir->insCall(&js_imod_ci, args);
            guard(false, lir->ins2(LIR_eq, x, lir->insImm(-1)), BRANCH_EXIT);
            x = lir->ins1(LIR_i2f, x);
        } else {
            LIns* args[] = { r_ins, l_ins };
            x = lir->insCall(&js_dmod_ci, args);
        }
        set(&l, x);
        return JSRS_CONTINUE;
    }
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NOT()
{
    jsval& v = stackval(-1);
    if (JSVAL_TAG(v) == JSVAL_BOOLEAN) {
        set(&v, lir->ins_eq0(lir->ins2i(LIR_eq, get(&v), 1)));
        return JSRS_CONTINUE;
    }
    if (isNumber(v)) {
        LIns* v_ins = get(&v);
        set(&v, lir->ins2(LIR_or, lir->ins2(LIR_feq, v_ins, lir->insImmq(0)),
                                  lir->ins_eq0(lir->ins2(LIR_feq, v_ins, v_ins))));
        return JSRS_CONTINUE;
    }
    if (JSVAL_TAG(v) == JSVAL_OBJECT) {
        set(&v, lir->ins_eq0(get(&v)));
        return JSRS_CONTINUE;
    }
    JS_ASSERT(JSVAL_IS_STRING(v));
    set(&v, lir->ins_eq0(lir->ins2(LIR_piand,
                                   lir->insLoad(LIR_ldp, get(&v), (int)offsetof(JSString, length)),
                                   INS_CONSTPTR(reinterpret_cast<void *>(JSSTRING_LENGTH_MASK)))));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_BITNOT()
{
    return unary(LIR_not);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NEG()
{
    jsval& v = stackval(-1);

    if (!JSVAL_IS_PRIMITIVE(v)) {
        ABORT_IF_XML(v);
        return call_imacro(unary_imacros.sign);
    }

    if (isNumber(v)) {
        LIns* a = get(&v);

        



        if (isPromoteInt(a) &&
            (!JSVAL_IS_INT(v) || JSVAL_TO_INT(v) != 0) &&
            (!JSVAL_IS_DOUBLE(v) || !JSDOUBLE_IS_NEGZERO(*JSVAL_TO_DOUBLE(v))) &&
            -asNumber(v) == (int)-asNumber(v)) {
            a = lir->ins1(LIR_neg, ::demote(lir, a));
            if (!a->isconst()) {
                VMSideExit* exit = snapshot(OVERFLOW_EXIT);
                lir->insGuard(LIR_xt, lir->ins1(LIR_ov, a),
                              createGuardRecord(exit));
                lir->insGuard(LIR_xt, lir->ins2(LIR_eq, a, lir->insImm(0)),
                              createGuardRecord(exit));
            }
            a = lir->ins1(LIR_i2f, a);
        } else {
            a = lir->ins1(LIR_fneg, a);
        }

        set(&v, a);
        return JSRS_CONTINUE;
    }

    if (JSVAL_IS_NULL(v)) {
        set(&v, lir->insImmf(-0.0));
        return JSRS_CONTINUE;
    }

    JS_ASSERT(JSVAL_TAG(v) == JSVAL_STRING || JSVAL_TAG(v) == JSVAL_BOOLEAN);

    LIns* args[] = { get(&v), cx_ins };
    set(&v, lir->ins1(LIR_fneg,
                      lir->insCall(JSVAL_IS_STRING(v)
                                   ? &js_StringToNumber_ci
                                   : &js_BooleanOrUndefinedToNumber_ci,
                                   args)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_POS()
{
    jsval& v = stackval(-1);

    if (!JSVAL_IS_PRIMITIVE(v)) {
        ABORT_IF_XML(v);
        return call_imacro(unary_imacros.sign);
    }

    if (isNumber(v))
        return JSRS_CONTINUE;

    if (JSVAL_IS_NULL(v)) {
        set(&v, lir->insImmq(0));
        return JSRS_CONTINUE;
    }

    JS_ASSERT(JSVAL_TAG(v) == JSVAL_STRING || JSVAL_TAG(v) == JSVAL_BOOLEAN);

    LIns* args[] = { get(&v), cx_ins };
    set(&v, lir->insCall(JSVAL_IS_STRING(v)
                         ? &js_StringToNumber_ci
                         : &js_BooleanOrUndefinedToNumber_ci,
                         args));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_PRIMTOP()
{
    
    
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_OBJTOP()
{
    jsval& v = stackval(-1);
    ABORT_IF_XML(v);
    return JSRS_CONTINUE;
}

JSBool
js_Array(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);

JSBool
js_Object(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool
js_Date(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSRecordingStatus
TraceRecorder::getClassPrototype(JSObject* ctor, LIns*& proto_ins)
{
    jsval pval;

    if (!OBJ_GET_PROPERTY(cx, ctor,
                          ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom),
                          &pval)) {
        ABORT_TRACE_ERROR("error getting prototype from constructor");
    }
    if (JSVAL_TAG(pval) != JSVAL_OBJECT)
        ABORT_TRACE("got primitive prototype from constructor");
#ifdef DEBUG
    JSBool ok, found;
    uintN attrs;
    ok = JS_GetPropertyAttributes(cx, ctor, js_class_prototype_str, &attrs, &found);
    JS_ASSERT(ok);
    JS_ASSERT(found);
    JS_ASSERT((~attrs & (JSPROP_READONLY | JSPROP_PERMANENT)) == 0);
#endif
    proto_ins = INS_CONSTPTR(JSVAL_TO_OBJECT(pval));
    return JSRS_CONTINUE;
}

JSRecordingStatus
TraceRecorder::getClassPrototype(JSProtoKey key, LIns*& proto_ins)
{
    JSObject* proto;
    if (!js_GetClassPrototype(cx, globalObj, INT_TO_JSID(key), &proto))
        ABORT_TRACE_ERROR("error in js_GetClassPrototype");
    proto_ins = INS_CONSTPTR(proto);
    return JSRS_CONTINUE;
}

#define IGNORE_NATIVE_CALL_COMPLETE_CALLBACK ((JSTraceableNative*)1)

JSRecordingStatus
TraceRecorder::newString(JSObject* ctor, uint32 argc, jsval* argv, jsval* rval)
{
    JS_ASSERT(argc == 1);

    if (!JSVAL_IS_PRIMITIVE(argv[0])) {
        ABORT_IF_XML(argv[0]);
        return call_imacro(new_imacros.String);
    }

    LIns* proto_ins;
    CHECK_STATUS(getClassPrototype(ctor, proto_ins));

    LIns* args[] = { stringify(argv[0]), proto_ins, cx_ins };
    LIns* obj_ins = lir->insCall(&js_String_tn_ci, args);
    guard(false, lir->ins_eq0(obj_ins), OOM_EXIT);

    set(rval, obj_ins);
    pendingTraceableNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
    return JSRS_CONTINUE;
}

JSRecordingStatus
TraceRecorder::newArray(JSObject* ctor, uint32 argc, jsval* argv, jsval* rval)
{
    LIns *proto_ins;
    CHECK_STATUS(getClassPrototype(ctor, proto_ins));

    LIns *arr_ins;
    if (argc == 0 || (argc == 1 && JSVAL_IS_NUMBER(argv[0]))) {
        
        LIns *args[] = { proto_ins, cx_ins };
        arr_ins = lir->insCall(&js_NewEmptyArray_ci, args);
        guard(false, lir->ins_eq0(arr_ins), OOM_EXIT);
        if (argc == 1) {
            
            lir->insStorei(f2i(get(argv)), 
                           arr_ins,
                           offsetof(JSObject, fslots) + JSSLOT_ARRAY_LENGTH * sizeof(jsval));
        }
    } else {
        
        LIns *args[] = { INS_CONST(argc), proto_ins, cx_ins };
        arr_ins = lir->insCall(&js_NewUninitializedArray_ci, args);
        guard(false, lir->ins_eq0(arr_ins), OOM_EXIT);

        
        LIns *dslots_ins = NULL;
        for (uint32 i = 0; i < argc && !lirbuf->outOMem(); i++) {
            LIns *elt_ins = get(argv + i);
            box_jsval(argv[i], elt_ins);
            stobj_set_dslot(arr_ins, i, dslots_ins, elt_ins, "set_array_elt");
        }

        if (argc > 0)
            stobj_set_fslot(arr_ins, JSSLOT_ARRAY_COUNT, INS_CONST(argc), "set_array_count");
    }

    set(rval, arr_ins);
    pendingTraceableNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::emitNativeCall(JSTraceableNative* known, uintN argc, LIns* args[])
{
    bool constructing = known->flags & JSTN_CONSTRUCTOR;

    if (JSTN_ERRTYPE(known) == FAIL_STATUS) {
        
        
        JS_ASSERT(!pendingTraceableNative);

        
        
        
        VMSideExit* exit = snapshot(DEEP_BAIL_EXIT);
        JSObject* funobj = JSVAL_TO_OBJECT(stackval(0 - (2 + argc)));
        if (FUN_SLOW_NATIVE(GET_FUNCTION_PRIVATE(cx, funobj)))
            exit->setNativeCallee(funobj, constructing);
        lir->insStorei(INS_CONSTPTR(exit), cx_ins, offsetof(JSContext, bailExit));

        
        LIns* guardRec = createGuardRecord(exit);
        lir->insGuard(LIR_xbarrier, guardRec, guardRec);
    }

    LIns* res_ins = lir->insCall(known->builtin, args);
    rval_ins = res_ins;
    switch (JSTN_ERRTYPE(known)) {
      case FAIL_NULL:
        guard(false, lir->ins_eq0(res_ins), OOM_EXIT);
        break;
      case FAIL_NEG:
        res_ins = lir->ins1(LIR_i2f, res_ins);
        guard(false, lir->ins2(LIR_flt, res_ins, lir->insImmq(0)), OOM_EXIT);
        break;
      case FAIL_VOID:
        guard(false, lir->ins2i(LIR_eq, res_ins, JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID)), OOM_EXIT);
        break;
      case FAIL_COOKIE:
        guard(false, lir->ins2(LIR_eq, res_ins, INS_CONST(JSVAL_ERROR_COOKIE)), OOM_EXIT);
        break;
      default:;
    }

    set(&stackval(0 - (2 + argc)), res_ins);

    




    pendingTraceableNative = known;

    return JSRS_CONTINUE;
}




JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::callTraceableNative(JSFunction* fun, uintN argc, bool constructing)
{
    JSTraceableNative* known = FUN_TRCINFO(fun);
    JS_ASSERT(known && (JSFastNative)fun->u.n.native == known->native);

    JSStackFrame* fp = cx->fp;
    jsbytecode *pc = fp->regs->pc;

    jsval& fval = stackval(0 - (2 + argc));
    jsval& tval = stackval(0 - (1 + argc));

    LIns* this_ins = get(&tval);

    LIns* args[nanojit::MAXARGS];
    do {
        if (((known->flags & JSTN_CONSTRUCTOR) != 0) != constructing)
            continue;

        uintN knownargc = strlen(known->argtypes);
        if (argc != knownargc)
            continue;

        intN prefixc = strlen(known->prefix);
        JS_ASSERT(prefixc <= 3);
        LIns** argp = &args[argc + prefixc - 1];
        char argtype;

#if defined _DEBUG
        memset(args, 0xCD, sizeof(args));
#endif

        uintN i;
        for (i = prefixc; i--; ) {
            argtype = known->prefix[i];
            if (argtype == 'C') {
                *argp = cx_ins;
            } else if (argtype == 'T') {   
                if (JSVAL_IS_PRIMITIVE(tval))
                    goto next_specialization;
                *argp = this_ins;
            } else if (argtype == 'S') {   
                if (!JSVAL_IS_STRING(tval))
                    goto next_specialization;
                *argp = this_ins;
            } else if (argtype == 'f') {
                *argp = INS_CONSTPTR(JSVAL_TO_OBJECT(fval));
            } else if (argtype == 'p') {
                CHECK_STATUS(getClassPrototype(JSVAL_TO_OBJECT(fval), *argp));
            } else if (argtype == 'R') {
                *argp = INS_CONSTPTR(cx->runtime);
            } else if (argtype == 'P') {
                
                
                if (*pc == JSOP_CALL && fp->imacpc && *fp->imacpc == JSOP_GETELEM)
                    *argp = INS_CONSTPTR(fp->imacpc);
                else
                    *argp = INS_CONSTPTR(pc);
            } else if (argtype == 'D') {  
                if (!isNumber(tval))
                    goto next_specialization;
                *argp = this_ins;
            } else {
                JS_NOT_REACHED("unknown prefix arg type");
            }
            argp--;
        }

        for (i = knownargc; i--; ) {
            jsval& arg = stackval(0 - (i + 1));
            *argp = get(&arg);

            argtype = known->argtypes[i];
            if (argtype == 'd' || argtype == 'i') {
                if (!isNumber(arg))
                    goto next_specialization;
                if (argtype == 'i')
                    *argp = f2i(*argp);
            } else if (argtype == 'o') {
                if (JSVAL_IS_PRIMITIVE(arg))
                    goto next_specialization;
            } else if (argtype == 's') {
                if (!JSVAL_IS_STRING(arg))
                    goto next_specialization;
            } else if (argtype == 'r') {
                if (!VALUE_IS_REGEXP(cx, arg))
                    goto next_specialization;
            } else if (argtype == 'f') {
                if (!VALUE_IS_FUNCTION(cx, arg))
                    goto next_specialization;
            } else if (argtype == 'v') {
                box_jsval(arg, *argp);
            } else {
                goto next_specialization;
            }
            argp--;
        }
#if defined _DEBUG
        JS_ASSERT(args[0] != (LIns *)0xcdcdcdcd);
#endif
        return emitNativeCall(known, argc, args);

next_specialization:;
    } while ((known++)->flags & JSTN_MORE);

    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::callNative(uintN argc, JSOp mode)
{
    LIns* args[5];

    JS_ASSERT(mode == JSOP_CALL || mode == JSOP_NEW || mode == JSOP_APPLY);

    jsval* vp = &stackval(0 - (2 + argc));
    JSObject* funobj = JSVAL_TO_OBJECT(vp[0]);
    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, funobj);

    if (fun->flags & JSFUN_TRACEABLE) {
        JSRecordingStatus status;
        if ((status = callTraceableNative(fun, argc, mode == JSOP_NEW)) != JSRS_STOP)
            return status;
    }

    JSFastNative native = (JSFastNative)fun->u.n.native;
    if (native == js_fun_apply || native == js_fun_call)
        ABORT_TRACE("trying to call native apply or call");

    
    uintN vplen = 2 + JS_MAX(argc, FUN_MINARGS(fun)) + fun->u.n.extra;
    if (!(fun->flags & JSFUN_FAST_NATIVE))
        vplen++;  
    lir->insStorei(INS_CONST(vplen), cx_ins, offsetof(JSContext, nativeVpLen));
    LIns* invokevp_ins = lir->insAlloc(vplen * sizeof(jsval));
    lir->insStorei(invokevp_ins, cx_ins, offsetof(JSContext, nativeVp));

    
    lir->insStorei(INS_CONSTWORD(OBJECT_TO_JSVAL(funobj)), invokevp_ins, 0);

    
    LIns* this_ins;
    if (mode == JSOP_NEW) {
        JSClass* clasp = fun->u.n.clasp;
        JS_ASSERT(clasp != &js_SlowArrayClass);
        if (!clasp)
            clasp = &js_ObjectClass;
        JS_ASSERT(((jsuword) clasp & 3) == 0);

        
        
        
        if (clasp == &js_FunctionClass)
            ABORT_TRACE("new Function");

        if (clasp->getObjectOps)
            ABORT_TRACE("new with non-native ops");

        args[0] = INS_CONSTPTR(funobj);
        args[1] = INS_CONSTPTR(clasp);
        args[2] = cx_ins;
        newobj_ins = lir->insCall(&js_NewInstance_ci, args);
        guard(false, lir->ins_eq0(newobj_ins), OOM_EXIT);
        this_ins = newobj_ins;  
    } else if (JSFUN_BOUND_METHOD_TEST(fun->flags)) {
        this_ins = INS_CONSTWORD(OBJECT_TO_JSVAL(OBJ_GET_PARENT(cx, funobj)));
    } else {
        this_ins = get(&vp[1]);
        





        if (!(fun->flags & JSFUN_FAST_NATIVE)) {
            if (JSVAL_IS_NULL(vp[1])) {
                JSObject* thisObj = js_ComputeThis(cx, JS_FALSE, vp + 2);
                if (!thisObj)
                    ABORT_TRACE_ERROR("error in js_ComputeGlobalThis");
                this_ins = INS_CONSTPTR(thisObj);
            } else if (!JSVAL_IS_OBJECT(vp[1])) {
                ABORT_TRACE("slow native(primitive, args)");
            } else {
                if (guardClass(JSVAL_TO_OBJECT(vp[1]), this_ins, &js_WithClass, snapshot(MISMATCH_EXIT)))
                    ABORT_TRACE("can't trace slow native invocation on With object");

                this_ins = lir->ins_choose(lir->ins_eq0(stobj_get_fslot(this_ins, JSSLOT_PARENT)),
                                           INS_CONSTPTR(globalObj),
                                           this_ins);
            }
        }
        box_jsval(vp[1], this_ins);
    }
    lir->insStorei(this_ins, invokevp_ins, 1 * sizeof(jsval));

    
    for (uintN n = 2; n < 2 + argc; n++) {
        LIns* i = get(&vp[n]);
        box_jsval(vp[n], i);
        lir->insStorei(i, invokevp_ins, n * sizeof(jsval));

        
        
        if (lirbuf->outOMem())
            ABORT_TRACE("out of memory in argument list");
    }

    
    if (2 + argc < vplen) {
        LIns* undef_ins = INS_CONSTWORD(JSVAL_VOID);
        for (uintN n = 2 + argc; n < vplen; n++) {
            lir->insStorei(undef_ins, invokevp_ins, n * sizeof(jsval));

            if (lirbuf->outOMem())
                ABORT_TRACE("out of memory in extra slots");
        }
    }

    
    uint32 types;
    if (fun->flags & JSFUN_FAST_NATIVE) {
        if (mode == JSOP_NEW)
            ABORT_TRACE("untraceable fast native constructor");
        native_rval_ins = invokevp_ins;
        args[0] = invokevp_ins;
        args[1] = lir->insImm(argc);
        args[2] = cx_ins;
        types = ARGSIZE_LO | ARGSIZE_LO << 2 | ARGSIZE_LO << 4 | ARGSIZE_LO << 6;
    } else {
        native_rval_ins = lir->ins2i(LIR_piadd, invokevp_ins, int32_t((vplen - 1) * sizeof(jsval)));
        args[0] = native_rval_ins;
        args[1] = lir->ins2i(LIR_piadd, invokevp_ins, int32_t(2 * sizeof(jsval)));
        args[2] = lir->insImm(argc);
        args[3] = this_ins;
        args[4] = cx_ins;
        types = ARGSIZE_LO | ARGSIZE_LO << 2 | ARGSIZE_LO << 4 | ARGSIZE_LO << 6 |
                ARGSIZE_LO << 8 | ARGSIZE_LO << 10;
    }

    
    
    

    CallInfo* ci = (CallInfo*) lir->insSkip(sizeof(struct CallInfo))->payload();
    ci->_address = uintptr_t(fun->u.n.native);
    ci->_cse = ci->_fold = 0;
    ci->_abi = ABI_CDECL;
    ci->_argtypes = types;
#ifdef DEBUG
    ci->_name = JS_GetFunctionName(fun);
 #endif

    
    generatedTraceableNative->builtin = ci;
    generatedTraceableNative->native = (JSFastNative)fun->u.n.native;
    generatedTraceableNative->flags = FAIL_STATUS | ((mode == JSOP_NEW)
                                                     ? JSTN_CONSTRUCTOR
                                                     : JSTN_UNBOX_AFTER);

    generatedTraceableNative->prefix = generatedTraceableNative->argtypes = NULL;

    
    
    JSRecordingStatus status;
    if ((status = emitNativeCall(generatedTraceableNative, argc, args)) != JSRS_CONTINUE)
        return status;

    
    lir->insStorei(INS_CONSTPTR(NULL), cx_ins, offsetof(JSContext, nativeVp));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::functionCall(uintN argc, JSOp mode)
{
    jsval& fval = stackval(0 - (2 + argc));
    JS_ASSERT(&fval >= StackBase(cx->fp));

    if (!VALUE_IS_FUNCTION(cx, fval))
        ABORT_TRACE("callee is not a function");

    jsval& tval = stackval(0 - (1 + argc));

    



    if (!get(&fval)->isconst())
        CHECK_STATUS(guardCallee(fval));

    










    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, JSVAL_TO_OBJECT(fval));

    if (FUN_INTERPRETED(fun)) {
        if (mode == JSOP_NEW) {
            LIns* args[] = { get(&fval), INS_CONSTPTR(&js_ObjectClass), cx_ins };
            LIns* tv_ins = lir->insCall(&js_NewInstance_ci, args);
            guard(false, lir->ins_eq0(tv_ins), OOM_EXIT);
            set(&tval, tv_ins);
        }
        return interpretedFunctionCall(fval, fun, argc, mode == JSOP_NEW);
    }

    if (FUN_SLOW_NATIVE(fun)) {
        JSNative native = fun->u.n.native;
        jsval* argv = &tval + 1;
        if (native == js_Array)
            return newArray(JSVAL_TO_OBJECT(fval), argc, argv, &fval);
        if (native == js_String && argc == 1) {
            if (mode == JSOP_NEW)
                return newString(JSVAL_TO_OBJECT(fval), 1, argv, &fval);
            if (!JSVAL_IS_PRIMITIVE(argv[0])) {
                ABORT_IF_XML(argv[0]);
                return call_imacro(call_imacros.String);
            }
            set(&fval, stringify(argv[0]));
            pendingTraceableNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
            return JSRS_CONTINUE;
        }
    }

    return callNative(argc, mode);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NEW()
{
    uintN argc = GET_ARGC(cx->fp->regs->pc);
    cx->fp->assertValidStackDepth(argc + 2);
    return functionCall(argc, JSOP_NEW);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DELNAME()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DELPROP()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DELELEM()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TYPEOF()
{
    jsval& r = stackval(-1);
    LIns* type;
    if (JSVAL_IS_STRING(r)) {
        type = INS_CONSTPTR(ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[JSTYPE_STRING]));
    } else if (isNumber(r)) {
        type = INS_CONSTPTR(ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[JSTYPE_NUMBER]));
    } else if (VALUE_IS_FUNCTION(cx, r)) {
        type = INS_CONSTPTR(ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[JSTYPE_FUNCTION]));
    } else {
        LIns* args[] = { get(&r), cx_ins };
        if (JSVAL_TAG(r) == JSVAL_BOOLEAN) {
            
            
            JS_ASSERT(r == JSVAL_TRUE || r == JSVAL_FALSE || r == JSVAL_VOID);
            type = lir->insCall(&js_TypeOfBoolean_ci, args);
        } else {
            JS_ASSERT(JSVAL_TAG(r) == JSVAL_OBJECT);
            type = lir->insCall(&js_TypeOfObject_ci, args);
        }
    }
    set(&r, type);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_VOID()
{
    stack(-1, INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INCNAME()
{
    return incName(1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INCPROP()
{
    return incProp(1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INCELEM()
{
    return incElem(1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DECNAME()
{
    return incName(-1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DECPROP()
{
    return incProp(-1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DECELEM()
{
    return incElem(-1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::incName(jsint incr, bool pre)
{
    jsval* vp;
    CHECK_STATUS(name(vp));
    LIns* v_ins = get(vp);
    CHECK_STATUS(inc(*vp, v_ins, incr, pre));
    set(vp, v_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NAMEINC()
{
    return incName(1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_PROPINC()
{
    return incProp(1, false);
}


JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ELEMINC()
{
    return incElem(1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NAMEDEC()
{
    return incName(-1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_PROPDEC()
{
    return incProp(-1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ELEMDEC()
{
    return incElem(-1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETPROP()
{
    return getProp(stackval(-1));
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETPROP()
{
    jsval& l = stackval(-2);
    if (JSVAL_IS_PRIMITIVE(l))
        ABORT_TRACE("primitive this for SETPROP");

    JSObject* obj = JSVAL_TO_OBJECT(l);
    if (obj->map->ops->setProperty != js_SetProperty)
        ABORT_TRACE("non-native JSObjectOps::setProperty");
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_SetPropHit(JSPropCacheEntry* entry, JSScopeProperty* sprop)
{
    if (entry == JS_NO_PROP_CACHE_FILL)
        ABORT_TRACE("can't trace uncacheable property set");
    if (PCVCAP_TAG(entry->vcap) >= 1)
        ABORT_TRACE("can't trace inherited property set");

    jsbytecode* pc = cx->fp->regs->pc;
    JS_ASSERT(entry->kpc == pc);

    jsval& r = stackval(-1);
    jsval& l = stackval(-2);

    JS_ASSERT(!JSVAL_IS_PRIMITIVE(l));
    JSObject* obj = JSVAL_TO_OBJECT(l);
    LIns* obj_ins = get(&l);
    JSScope* scope = OBJ_SCOPE(obj);

    JS_ASSERT(scope->object == obj);
    JS_ASSERT(SCOPE_HAS_PROPERTY(scope, sprop));

    if (!isValidSlot(scope, sprop))
        return JSRS_STOP;

    if (obj == globalObj) {
        JS_ASSERT(SPROP_HAS_VALID_SLOT(sprop, scope));
        uint32 slot = sprop->slot;
        if (!lazilyImportGlobalSlot(slot))
            ABORT_TRACE("lazy import of global slot failed");

        LIns* r_ins = get(&r);

        





        if (VALUE_IS_FUNCTION(cx, r))
            ABORT_TRACE("potential rebranding of the global object");
        set(&STOBJ_GET_SLOT(obj, slot), r_ins);

        JS_ASSERT(*pc != JSOP_INITPROP);
        if (pc[JSOP_SETPROP_LENGTH] != JSOP_POP)
            set(&l, r_ins);
        return JSRS_CONTINUE;
    }

    
    LIns* map_ins = lir->insLoad(LIR_ldp, obj_ins, (int)offsetof(JSObject, map));
    LIns* ops_ins;
    if (!map_is_native(obj->map, map_ins, ops_ins, offsetof(JSObjectOps, setProperty)))
        ABORT_TRACE("non-native map");

    LIns* shape_ins = addName(lir->insLoad(LIR_ld, map_ins, offsetof(JSScope, shape)), "shape");
    guard(true, addName(lir->ins2i(LIR_eq, shape_ins, entry->kshape), "guard(kshape)"),
          BRANCH_EXIT);

    uint32 vshape = PCVCAP_SHAPE(entry->vcap);
    if (entry->kshape != vshape) {
        LIns *vshape_ins = lir->insLoad(LIR_ld,
                                        lir->insLoad(LIR_ldp, cx_ins, offsetof(JSContext, runtime)),
                                        offsetof(JSRuntime, protoHazardShape));
        guard(true, addName(lir->ins2i(LIR_eq, vshape_ins, vshape), "guard(vshape)"),
              MISMATCH_EXIT);

        LIns* args[] = { INS_CONSTPTR(sprop), obj_ins, cx_ins };
        LIns* ok_ins = lir->insCall(&js_AddProperty_ci, args);
        guard(false, lir->ins_eq0(ok_ins), OOM_EXIT);
    }

    LIns* dslots_ins = NULL;
    LIns* v_ins = get(&r);
    LIns* boxed_ins = v_ins;
    box_jsval(r, boxed_ins);
    CHECK_STATUS(native_set(obj_ins, sprop, dslots_ins, boxed_ins));

    if (*pc != JSOP_INITPROP && pc[JSOP_SETPROP_LENGTH] != JSOP_POP)
        set(&l, v_ins);
    return JSRS_CONTINUE;
}



static JSBool
GetProperty(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv;
    jsid id;

    JS_ASSERT_NOT_ON_TRACE(cx);
    JS_ASSERT(cx->fp->imacpc && argc == 1);
    argv = JS_ARGV(cx, vp);
    JS_ASSERT(JSVAL_IS_STRING(argv[0]));
    if (!js_ValueToStringId(cx, argv[0], &id))
        return JS_FALSE;
    argv[0] = ID_TO_VALUE(id);
    return OBJ_GET_PROPERTY(cx, JS_THIS_OBJECT(cx, vp), id, &JS_RVAL(cx, vp));
}

static jsval FASTCALL
GetProperty_tn(JSContext *cx, jsbytecode *pc, JSObject *obj, JSString *name)
{
    JSAutoTempIdRooter idr(cx);
    JSAutoTempValueRooter tvr(cx);

    if (!js_ValueToStringId(cx, STRING_TO_JSVAL(name), idr.addr()) ||
        !OBJ_GET_PROPERTY(cx, obj, idr.id(), tvr.addr())) {
        js_SetBuiltinError(cx);
        *tvr.addr() = JSVAL_ERROR_COOKIE;
    }
    return tvr.value();
}

static JSBool
GetElement(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv;
    jsid id;

    JS_ASSERT_NOT_ON_TRACE(cx);
    JS_ASSERT(cx->fp->imacpc && argc == 1);
    argv = JS_ARGV(cx, vp);
    JS_ASSERT(JSVAL_IS_NUMBER(argv[0]));
    if (!JS_ValueToId(cx, argv[0], &id))
        return JS_FALSE;
    argv[0] = ID_TO_VALUE(id);
    return OBJ_GET_PROPERTY(cx, JS_THIS_OBJECT(cx, vp), id, &JS_RVAL(cx, vp));
}

static jsval FASTCALL
GetElement_tn(JSContext* cx, jsbytecode *pc, JSObject* obj, int32 index)
{
    JSAutoTempValueRooter tvr(cx);
    JSAutoTempIdRooter idr(cx);

    if (!js_Int32ToId(cx, index, idr.addr())) {
        js_SetBuiltinError(cx);
        return JSVAL_ERROR_COOKIE;
    }
    if (!OBJ_GET_PROPERTY(cx, obj, idr.id(), tvr.addr())) {
        js_SetBuiltinError(cx);
        *tvr.addr() = JSVAL_ERROR_COOKIE;
    }
    return tvr.value();
}

JS_DEFINE_TRCINFO_1(GetProperty,
    (4, (static, JSVAL_FAIL,    GetProperty_tn, CONTEXT, PC, THIS, STRING,      0, 0)))
JS_DEFINE_TRCINFO_1(GetElement,
    (4, (extern, JSVAL_FAIL,    GetElement_tn,  CONTEXT, PC, THIS, INT32,       0, 0)))

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETELEM()
{
    bool call = *cx->fp->regs->pc == JSOP_CALLELEM;

    jsval& idx = stackval(-1);
    jsval& lval = stackval(-2);

    LIns* obj_ins = get(&lval);
    LIns* idx_ins = get(&idx);

    
    if (JSVAL_IS_STRING(lval) && isInt32(idx)) {
        if (call)
            ABORT_TRACE("JSOP_CALLELEM on a string");
        int i = asInt32(idx);
        if (size_t(i) >= JSSTRING_LENGTH(JSVAL_TO_STRING(lval)))
            ABORT_TRACE("Invalid string index in JSOP_GETELEM");
        idx_ins = makeNumberInt32(idx_ins);
        LIns* args[] = { idx_ins, obj_ins, cx_ins };
        LIns* unitstr_ins = lir->insCall(&js_String_getelem_ci, args);
        guard(false, lir->ins_eq0(unitstr_ins), MISMATCH_EXIT);
        set(&lval, unitstr_ins);
        return JSRS_CONTINUE;
    }

    if (JSVAL_IS_PRIMITIVE(lval))
        ABORT_TRACE("JSOP_GETLEM on a primitive");
    ABORT_IF_XML(lval);

    JSObject* obj = JSVAL_TO_OBJECT(lval);
    jsval id;
    LIns* v_ins;

    
    if (!JSVAL_IS_INT(idx)) {
        if (!JSVAL_IS_PRIMITIVE(idx))
            ABORT_TRACE("non-primitive index");
        
        if (!js_InternNonIntElementId(cx, obj, idx, &id))
            ABORT_TRACE_ERROR("failed to intern non-int element id");
        set(&idx, stringify(idx));

        
        idx = ID_TO_VALUE(id);

        
        
        CHECK_STATUS(guardNotGlobalObject(obj, obj_ins));

        return call_imacro(call ? callelem_imacros.callprop : getelem_imacros.getprop);
    }

    if (!guardDenseArray(obj, obj_ins, BRANCH_EXIT)) {
        CHECK_STATUS(guardNotGlobalObject(obj, obj_ins));

        return call_imacro(call ? callelem_imacros.callelem : getelem_imacros.getelem);
    }

    
    jsval* vp;
    LIns* addr_ins;
    CHECK_STATUS(denseArrayElement(lval, idx, vp, v_ins, addr_ins));
    set(&lval, v_ins);
    if (call)
        set(&idx, obj_ins);
    return JSRS_CONTINUE;
}



static JSBool
SetProperty(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv;
    jsid id;

    JS_ASSERT(argc == 2);
    argv = JS_ARGV(cx, vp);
    JS_ASSERT(JSVAL_IS_STRING(argv[0]));
    if (!js_ValueToStringId(cx, argv[0], &id))
        return JS_FALSE;
    argv[0] = ID_TO_VALUE(id);
    if (!OBJ_SET_PROPERTY(cx, JS_THIS_OBJECT(cx, vp), id, &argv[1]))
        return JS_FALSE;
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool FASTCALL
SetProperty_tn(JSContext* cx, JSObject* obj, JSString* idstr, jsval v)
{
    JSAutoTempValueRooter tvr(cx, v);
    JSAutoTempIdRooter idr(cx);

    if (!js_ValueToStringId(cx, STRING_TO_JSVAL(idstr), idr.addr()) ||
        !OBJ_SET_PROPERTY(cx, obj, idr.id(), tvr.addr())) {
        js_SetBuiltinError(cx);
    }
    return JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID);
}

static JSBool
SetElement(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv;
    jsid id;

    JS_ASSERT(argc == 2);
    argv = JS_ARGV(cx, vp);
    JS_ASSERT(JSVAL_IS_NUMBER(argv[0]));
    if (!JS_ValueToId(cx, argv[0], &id))
        return JS_FALSE;
    argv[0] = ID_TO_VALUE(id);
    if (!OBJ_SET_PROPERTY(cx, JS_THIS_OBJECT(cx, vp), id, &argv[1]))
        return JS_FALSE;
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool FASTCALL
SetElement_tn(JSContext* cx, JSObject* obj, int32 index, jsval v)
{
    JSAutoTempIdRooter idr(cx);
    JSAutoTempValueRooter tvr(cx, v);

    if (!js_Int32ToId(cx, index, idr.addr()) ||
        !OBJ_SET_PROPERTY(cx, obj, idr.id(), tvr.addr())) {
        js_SetBuiltinError(cx);
    }
    return JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID);
}

JS_DEFINE_TRCINFO_1(SetProperty,
    (4, (extern, BOOL_FAIL,     SetProperty_tn, CONTEXT, THIS, STRING, JSVAL,   0, 0)))
JS_DEFINE_TRCINFO_1(SetElement,
    (4, (extern, BOOL_FAIL,     SetElement_tn,  CONTEXT, THIS, INT32, JSVAL,    0, 0)))

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETELEM()
{
    jsval& v = stackval(-1);
    jsval& idx = stackval(-2);
    jsval& lval = stackval(-3);

    
    if (JSVAL_IS_PRIMITIVE(lval))
        ABORT_TRACE("left JSOP_SETELEM operand is not an object");
    ABORT_IF_XML(lval);

    JSObject* obj = JSVAL_TO_OBJECT(lval);
    LIns* obj_ins = get(&lval);
    LIns* idx_ins = get(&idx);
    LIns* v_ins = get(&v);
    jsid id;

    if (!JSVAL_IS_INT(idx)) {
        if (!JSVAL_IS_PRIMITIVE(idx))
            ABORT_TRACE("non-primitive index");
        
        if (!js_InternNonIntElementId(cx, obj, idx, &id))
            ABORT_TRACE_ERROR("failed to intern non-int element id");
        set(&idx, stringify(idx));

        
        idx = ID_TO_VALUE(id);

        
        
        CHECK_STATUS(guardNotGlobalObject(obj, obj_ins));

        return call_imacro((*cx->fp->regs->pc == JSOP_INITELEM)
                           ? initelem_imacros.initprop
                           : setelem_imacros.setprop);
    }

    if (JSVAL_TO_INT(idx) < 0 || !OBJ_IS_DENSE_ARRAY(cx, obj)) {
        CHECK_STATUS(guardNotGlobalObject(obj, obj_ins));

        return call_imacro((*cx->fp->regs->pc == JSOP_INITELEM)
                           ? initelem_imacros.initelem
                           : setelem_imacros.setelem);
    }

    
    if (!guardDenseArray(obj, obj_ins, BRANCH_EXIT))
        return JSRS_STOP;

    
    
    idx_ins = makeNumberInt32(idx_ins);

    
    
    LIns* boxed_v_ins = v_ins;
    box_jsval(v, boxed_v_ins);

    LIns* args[] = { boxed_v_ins, idx_ins, obj_ins, cx_ins };
    LIns* res_ins = lir->insCall(&js_Array_dense_setelem_ci, args);
    guard(false, lir->ins_eq0(res_ins), MISMATCH_EXIT);

    jsbytecode* pc = cx->fp->regs->pc;
    if (*pc == JSOP_SETELEM && pc[JSOP_SETELEM_LENGTH] != JSOP_POP)
        set(&lval, v_ins);

    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLNAME()
{
    JSObject* obj = cx->fp->scopeChain;
    if (obj != globalObj) {
        jsval* vp;
        CHECK_STATUS(activeCallOrGlobalSlot(obj, vp));
        stack(0, get(vp));
        stack(1, INS_CONSTPTR(globalObj));
        return JSRS_CONTINUE;
    }

    LIns* obj_ins = scopeChain();
    JSObject* obj2;
    jsuword pcval;

    CHECK_STATUS(test_property_cache(obj, obj_ins, obj2, pcval));

    if (PCVAL_IS_NULL(pcval) || !PCVAL_IS_OBJECT(pcval))
        ABORT_TRACE("callee is not an object");

    JS_ASSERT(HAS_FUNCTION_CLASS(PCVAL_TO_OBJECT(pcval)));

    stack(0, INS_CONSTPTR(PCVAL_TO_OBJECT(pcval)));
    stack(1, obj_ins);
    return JSRS_CONTINUE;
}

JS_DEFINE_CALLINFO_4(extern, UINT32, js_GetUpvarOnTrace, CONTEXT, UINT32, UINT32, DOUBLEPTR, 0, 0)

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETUPVAR()
{
    uintN index = GET_UINT16(cx->fp->regs->pc);
    JSScript *script = cx->fp->script;

    JSUpvarArray* uva = JS_SCRIPT_UPVARS(script);
    JS_ASSERT(index < uva->length);

    


    jsval& v = js_GetUpvar(cx, script->staticLevel, uva->vector[index]);
    LIns* upvar_ins = get(&v);
    if (upvar_ins) {
        stack(0, upvar_ins);
        return JSRS_CONTINUE;
    }

    



    LIns* outp = lir->insAlloc(sizeof(double));
    LIns* args[] = { 
        outp,
        lir->insImm(uva->vector[index]), 
        lir->insImm(script->staticLevel), 
        cx_ins 
    };
    const CallInfo* ci = &js_GetUpvarOnTrace_ci;
    LIns* call_ins = lir->insCall(ci, args);
    uint8 type = getCoercedType(v);
    guard(true,
          addName(lir->ins2(LIR_eq, call_ins, lir->insImm(type)),
                  "guard(type-stable upvar)"),
          BRANCH_EXIT);

    LOpcode loadOp;
    switch (type) {
      case JSVAL_DOUBLE:
        loadOp = LIR_ldq;
        break;
      case JSVAL_OBJECT:
      case JSVAL_STRING:
      case JSVAL_TFUN:
      case JSVAL_TNULL:
        loadOp = LIR_ldp;
        break;
      case JSVAL_INT:
      case JSVAL_BOOLEAN:
        loadOp = LIR_ld;
        break;
      case JSVAL_BOXED:
      default:
        JS_NOT_REACHED("found boxed type in an upvar type map entry");
        return JSRS_STOP;
    }

    LIns* result = lir->insLoad(loadOp, outp, lir->insImm(0));
    if (type == JSVAL_INT)
        result = lir->ins1(LIR_i2f, result);
    stack(0, result);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLUPVAR()
{
    CHECK_STATUS(record_JSOP_GETUPVAR());
    stack(1, INS_CONSTPTR(NULL));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETDSLOT()
{
    JSObject* callee = cx->fp->callee;
    LIns* callee_ins = get(&cx->fp->argv[-2]);

    unsigned index = GET_UINT16(cx->fp->regs->pc);
    LIns* dslots_ins = NULL;
    LIns* v_ins = stobj_get_dslot(callee_ins, index, dslots_ins);

    unbox_jsval(callee->dslots[index], v_ins, snapshot(BRANCH_EXIT));
    stack(0, v_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLDSLOT()
{
    CHECK_STATUS(record_JSOP_GETDSLOT());
    stack(1, INS_CONSTPTR(NULL));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::guardCallee(jsval& callee)
{
    JS_ASSERT(VALUE_IS_FUNCTION(cx, callee));

    VMSideExit* branchExit = snapshot(BRANCH_EXIT);
    JSObject* callee_obj = JSVAL_TO_OBJECT(callee);
    LIns* callee_ins = get(&callee);

    guard(true,
          lir->ins2(LIR_eq,
                    lir->ins2(LIR_piand,
                              stobj_get_fslot(callee_ins, JSSLOT_PRIVATE),
                              INS_CONSTWORD(~JSVAL_INT)),
                    INS_CONSTPTR(OBJ_GET_PRIVATE(cx, callee_obj))),
          branchExit);
    guard(true,
          lir->ins2(LIR_eq,
                    stobj_get_fslot(callee_ins, JSSLOT_PARENT),
                    INS_CONSTPTR(OBJ_GET_PARENT(cx, callee_obj))),
          branchExit);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::interpretedFunctionCall(jsval& fval, JSFunction* fun, uintN argc, bool constructing)
{
    if (JS_GetGlobalForObject(cx, JSVAL_TO_OBJECT(fval)) != globalObj)
        ABORT_TRACE("JSOP_CALL or JSOP_NEW crosses global scopes");

    JSStackFrame* fp = cx->fp;

    
    if (argc < fun->nargs &&
        jsuword(fp->regs->sp + (fun->nargs - argc)) > cx->stackPool.current->limit) {
        ABORT_TRACE("can't trace calls with too few args requiring argv move");
    }

    
    unsigned stackSlots = js_NativeStackSlots(cx, 0);
    if (sizeof(FrameInfo) + stackSlots * sizeof(uint8) > MAX_SKIP_BYTES)
        ABORT_TRACE("interpreted function call requires saving too much stack");
    LIns* data = lir->insSkip(sizeof(FrameInfo) + stackSlots * sizeof(uint8));
    FrameInfo* fi = (FrameInfo*)data->payload();
    uint8* typemap = (uint8 *)(fi + 1);
    uint8* m = typemap;
    


    FORALL_SLOTS_IN_PENDING_FRAMES(cx, 0,
        *m++ = determineSlotType(vp);
    );

    if (argc >= 0x8000)
        ABORT_TRACE("too many arguments");

    fi->callee = JSVAL_TO_OBJECT(fval);
    fi->block = fp->blockChain;
    fi->pc = fp->regs->pc;
    fi->imacpc = fp->imacpc;
    fi->s.spdist = fp->regs->sp - fp->slots;
    fi->s.argc = argc | (constructing ? 0x8000 : 0);

    unsigned callDepth = getCallDepth();
    if (callDepth >= treeInfo->maxCallDepth)
        treeInfo->maxCallDepth = callDepth + 1;

    lir->insStorei(INS_CONSTPTR(fi), lirbuf->rp, callDepth * sizeof(FrameInfo*));

    atoms = fun->u.i.script->atomMap.vector;
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALL()
{
    uintN argc = GET_ARGC(cx->fp->regs->pc);
    cx->fp->assertValidStackDepth(argc + 2);
    return functionCall(argc,
                        (cx->fp->imacpc && *cx->fp->imacpc == JSOP_APPLY)
                        ? JSOP_APPLY
                        : JSOP_CALL);
}

static jsbytecode* apply_imacro_table[] = {
    apply_imacros.apply0,
    apply_imacros.apply1,
    apply_imacros.apply2,
    apply_imacros.apply3,
    apply_imacros.apply4,
    apply_imacros.apply5,
    apply_imacros.apply6,
    apply_imacros.apply7,
    apply_imacros.apply8
};

static jsbytecode* call_imacro_table[] = {
    apply_imacros.call0,
    apply_imacros.call1,
    apply_imacros.call2,
    apply_imacros.call3,
    apply_imacros.call4,
    apply_imacros.call5,
    apply_imacros.call6,
    apply_imacros.call7,
    apply_imacros.call8
};

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_APPLY()
{
    JSStackFrame* fp = cx->fp;
    jsbytecode *pc = fp->regs->pc;
    uintN argc = GET_ARGC(pc);
    cx->fp->assertValidStackDepth(argc + 2);

    jsval* vp = fp->regs->sp - (argc + 2);
    jsuint length = 0;
    JSObject* aobj = NULL;
    LIns* aobj_ins = NULL;

    JS_ASSERT(!fp->imacpc);

    if (!VALUE_IS_FUNCTION(cx, vp[0]))
        return record_JSOP_CALL();
    ABORT_IF_XML(vp[0]);

    JSObject* obj = JSVAL_TO_OBJECT(vp[0]);
    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, obj);
    if (FUN_INTERPRETED(fun))
        return record_JSOP_CALL();

    bool apply = (JSFastNative)fun->u.n.native == js_fun_apply;
    if (!apply && (JSFastNative)fun->u.n.native != js_fun_call)
        return record_JSOP_CALL();

    



    if (argc > 0 && JSVAL_IS_PRIMITIVE(vp[2]))
        return record_JSOP_CALL();

    


    if (!VALUE_IS_FUNCTION(cx, vp[1]))
        ABORT_TRACE("callee is not a function");
    CHECK_STATUS(guardCallee(vp[1]));

    if (apply && argc >= 2) {
        if (argc != 2)
            ABORT_TRACE("apply with excess arguments");
        if (JSVAL_IS_PRIMITIVE(vp[3]))
            ABORT_TRACE("arguments parameter of apply is primitive");
        aobj = JSVAL_TO_OBJECT(vp[3]);
        aobj_ins = get(&vp[3]);

        




        if (!guardDenseArray(aobj, aobj_ins))
            ABORT_TRACE("arguments parameter of apply is not a dense array");

        


        length = jsuint(aobj->fslots[JSSLOT_ARRAY_LENGTH]);
        if (length >= JS_ARRAY_LENGTH(apply_imacro_table))
            ABORT_TRACE("too many arguments to apply");

        


        guard(true,
              lir->ins2i(LIR_eq,
                         stobj_get_fslot(aobj_ins, JSSLOT_ARRAY_LENGTH),
                         length),
              BRANCH_EXIT);

        return call_imacro(apply_imacro_table[length]);
    }

    if (argc >= JS_ARRAY_LENGTH(call_imacro_table))
        ABORT_TRACE("too many arguments to call");

    return call_imacro(call_imacro_table[argc]);
}

static JSBool FASTCALL
CatchStopIteration_tn(JSContext* cx, JSBool ok, jsval* vp)
{
    if (!ok && cx->throwing && js_ValueIsStopIteration(cx->exception)) {
        cx->throwing = JS_FALSE;
        cx->exception = JSVAL_VOID;
        *vp = JSVAL_HOLE;
        return JS_TRUE;
    }
    return ok;
}

JS_DEFINE_TRCINFO_1(CatchStopIteration_tn,
    (3, (static, BOOL, CatchStopIteration_tn, CONTEXT, BOOL, JSVALPTR, 0, 0)))

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_NativeCallComplete()
{
    if (pendingTraceableNative == IGNORE_NATIVE_CALL_COMPLETE_CALLBACK)
        return JSRS_CONTINUE;

    jsbytecode* pc = cx->fp->regs->pc;

    JS_ASSERT(pendingTraceableNative);
    JS_ASSERT(*pc == JSOP_CALL || *pc == JSOP_APPLY || *pc == JSOP_NEW);

    jsval& v = stackval(-1);
    LIns* v_ins = get(&v);

    











    if (JSTN_ERRTYPE(pendingTraceableNative) == FAIL_STATUS) {
        
        lir->insStorei(INS_CONSTPTR(NULL), cx_ins, (int) offsetof(JSContext, bailExit));

        LIns* status = lir->insLoad(LIR_ld, lirbuf->state, (int) offsetof(InterpState, builtinStatus));
        if (pendingTraceableNative == generatedTraceableNative) {
            LIns* ok_ins = v_ins;

            



            if (uintptr_t(pc - nextiter_imacros.custom_iter_next) <
                sizeof(nextiter_imacros.custom_iter_next)) {
                LIns* args[] = { native_rval_ins, ok_ins, cx_ins }; 
                ok_ins = lir->insCall(&CatchStopIteration_tn_ci, args);
            }

            




            v_ins = lir->insLoad(LIR_ld, native_rval_ins, 0);
            if (*pc == JSOP_NEW) {
                LIns* x = lir->ins_eq0(lir->ins2i(LIR_piand, v_ins, JSVAL_TAGMASK));
                x = lir->ins_choose(x, v_ins, INS_CONST(0));
                v_ins = lir->ins_choose(lir->ins_eq0(x), newobj_ins, x);
            }
            set(&v, v_ins);

            






            JS_STATIC_ASSERT((1 - JS_TRUE) << 1 == 0);
            JS_STATIC_ASSERT((1 - JS_FALSE) << 1 == JSBUILTIN_ERROR);
            status = lir->ins2(LIR_or,
                               status,
                               lir->ins2i(LIR_lsh,
                                          lir->ins2i(LIR_xor,
                                                     lir->ins2i(LIR_and, ok_ins, 1),
                                                     1),
                                          1));
            lir->insStorei(status, lirbuf->state, (int) offsetof(InterpState, builtinStatus));
        }
        guard(true,
              lir->ins_eq0(status),
              STATUS_EXIT);
    }

    JSRecordingStatus ok = JSRS_CONTINUE;
    if (pendingTraceableNative->flags & JSTN_UNBOX_AFTER) {
        




        JS_ASSERT(&v == &cx->fp->regs->sp[-1] && get(&v) == v_ins);
        unbox_jsval(v, v_ins, snapshot(BRANCH_EXIT));
        set(&v, v_ins);
    } else if (JSTN_ERRTYPE(pendingTraceableNative) == FAIL_NEG) {
        
        JS_ASSERT(JSVAL_IS_NUMBER(v));
    } else {
        
        if (JSVAL_IS_NUMBER(v) &&
            (pendingTraceableNative->builtin->_argtypes & 3) == nanojit::ARGSIZE_LO) {
            set(&v, lir->ins1(LIR_i2f, v_ins));
        }
    }

    
    
    return ok;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::name(jsval*& vp)
{
    JSObject* obj = cx->fp->scopeChain;
    if (obj != globalObj)
        return activeCallOrGlobalSlot(obj, vp);

    
    LIns* obj_ins = scopeChain();
    uint32 slot;

    JSObject* obj2;
    jsuword pcval;

    



    CHECK_STATUS(test_property_cache(obj, obj_ins, obj2, pcval));

    


    if (PCVAL_IS_NULL(pcval))
        ABORT_TRACE("named property not found");

    


    if (obj2 != obj)
        ABORT_TRACE("name() hit prototype chain");

    
    if (PCVAL_IS_SPROP(pcval)) {
        JSScopeProperty* sprop = PCVAL_TO_SPROP(pcval);
        if (!isValidSlot(OBJ_SCOPE(obj), sprop))
            ABORT_TRACE("name() not accessing a valid slot");
        slot = sprop->slot;
    } else {
        if (!PCVAL_IS_SLOT(pcval))
            ABORT_TRACE("PCE is not a slot");
        slot = PCVAL_TO_SLOT(pcval);
    }

    if (!lazilyImportGlobalSlot(slot))
        ABORT_TRACE("lazy import of global slot failed");

    vp = &STOBJ_GET_SLOT(obj, slot);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::prop(JSObject* obj, LIns* obj_ins, uint32& slot, LIns*& v_ins)
{
    



    CHECK_STATUS(guardNotGlobalObject(obj, obj_ins));

    



    JSObject* obj2;
    jsuword pcval;
    CHECK_STATUS(test_property_cache(obj, obj_ins, obj2, pcval));

    
    const JSCodeSpec& cs = js_CodeSpec[*cx->fp->regs->pc];
    if (PCVAL_IS_NULL(pcval)) {
        



        VMSideExit* exit = snapshot(BRANCH_EXIT);
        for (;;) {
            LIns* map_ins = lir->insLoad(LIR_ldp, obj_ins, (int)offsetof(JSObject, map));
            LIns* ops_ins;
            if (map_is_native(obj->map, map_ins, ops_ins)) {
                LIns* shape_ins = addName(lir->insLoad(LIR_ld, map_ins, offsetof(JSScope, shape)),
                                          "shape");
                guard(true,
                      addName(lir->ins2i(LIR_eq, shape_ins, OBJ_SHAPE(obj)), "guard(shape)"),
                      exit);
            } else if (!guardDenseArray(obj, obj_ins, BRANCH_EXIT))
                ABORT_TRACE("non-native object involved in undefined property access");

            obj = JSVAL_TO_OBJECT(obj->fslots[JSSLOT_PROTO]);
            if (!obj)
                break;
            obj_ins = stobj_get_fslot(obj_ins, JSSLOT_PROTO);
        }

        v_ins = INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID));
        slot = SPROP_INVALID_SLOT;
        return JSRS_CONTINUE;
    }

    
    uint32 setflags = (cs.format & (JOF_SET | JOF_INCDEC | JOF_FOR));
    LIns* dslots_ins = NULL;

    
    if (PCVAL_IS_SPROP(pcval)) {
        JSScopeProperty* sprop = PCVAL_TO_SPROP(pcval);

        if (setflags && !SPROP_HAS_STUB_SETTER(sprop))
            ABORT_TRACE("non-stub setter");
        if (setflags && (sprop->attrs & JSPROP_READONLY))
            ABORT_TRACE("writing to a readonly property");
        if (setflags != JOF_SET && !SPROP_HAS_STUB_GETTER(sprop)) {
            
            if (setflags == 0 &&
                sprop->getter == js_RegExpClass.getProperty &&
                sprop->shortid < 0) {
                if (sprop->shortid == REGEXP_LAST_INDEX)
                    ABORT_TRACE("can't trace RegExp.lastIndex yet");
                LIns* args[] = { INS_CONSTPTR(sprop), obj_ins, cx_ins };
                v_ins = lir->insCall(&js_CallGetter_ci, args);
                guard(false, lir->ins2(LIR_eq, v_ins, INS_CONST(JSVAL_ERROR_COOKIE)), OOM_EXIT);
                



                unbox_jsval((sprop->shortid == REGEXP_SOURCE) ? JSVAL_STRING : JSVAL_BOOLEAN,
                            v_ins,
                            snapshot(MISMATCH_EXIT));
                return JSRS_CONTINUE;
            }
            if (setflags == 0 &&
                sprop->getter == js_StringClass.getProperty &&
                sprop->id == ATOM_KEY(cx->runtime->atomState.lengthAtom)) {
                if (!guardClass(obj, obj_ins, &js_StringClass, snapshot(MISMATCH_EXIT)))
                    ABORT_TRACE("can't trace String.length on non-String objects");
                LIns* str_ins = stobj_get_fslot(obj_ins, JSSLOT_PRIVATE);
                str_ins = lir->ins2(LIR_piand, str_ins, INS_CONSTWORD(~JSVAL_TAGMASK));
                v_ins = lir->ins1(LIR_i2f, getStringLength(str_ins));
                return JSRS_CONTINUE;
            }
            ABORT_TRACE("non-stub getter");
        }
        if (!SPROP_HAS_VALID_SLOT(sprop, OBJ_SCOPE(obj)))
            ABORT_TRACE("no valid slot");
        slot = sprop->slot;
    } else {
        if (!PCVAL_IS_SLOT(pcval))
            ABORT_TRACE("PCE is not a slot");
        slot = PCVAL_TO_SLOT(pcval);
    }

    if (obj2 != obj) {
        if (setflags)
            ABORT_TRACE("JOF_SET opcode hit prototype chain");

        




        while (obj != obj2) {
            obj_ins = stobj_get_slot(obj_ins, JSSLOT_PROTO, dslots_ins);
            obj = STOBJ_GET_PROTO(obj);
        }
    }

    v_ins = stobj_get_slot(obj_ins, slot, dslots_ins);
    unbox_jsval(STOBJ_GET_SLOT(obj, slot), v_ins, snapshot(BRANCH_EXIT));

    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::denseArrayElement(jsval& oval, jsval& ival, jsval*& vp, LIns*& v_ins,
                                 LIns*& addr_ins)
{
    JS_ASSERT(JSVAL_IS_OBJECT(oval) && JSVAL_IS_INT(ival));

    JSObject* obj = JSVAL_TO_OBJECT(oval);
    LIns* obj_ins = get(&oval);
    jsint idx = JSVAL_TO_INT(ival);
    LIns* idx_ins = makeNumberInt32(get(&ival));

    VMSideExit* exit = snapshot(BRANCH_EXIT);

    
    LIns* dslots_ins = lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, dslots));
    jsuint capacity = js_DenseArrayCapacity(obj);
    bool within = (jsuint(idx) < jsuint(obj->fslots[JSSLOT_ARRAY_LENGTH]) && jsuint(idx) < capacity);
    if (!within) {
        
        LIns* br1 = NULL;
        if (MAX_DSLOTS_LENGTH > JS_BITMASK(30) && !idx_ins->isconst()) {
            JS_ASSERT(sizeof(jsval) == 8); 
            br1 = lir->insBranch(LIR_jt,
                                 lir->ins2i(LIR_lt, idx_ins, 0),
                                 NULL);
        }

        
        LIns* br2 = lir->insBranch(LIR_jf,
                                   lir->ins2(LIR_ult,
                                             idx_ins,
                                             stobj_get_fslot(obj_ins, JSSLOT_ARRAY_LENGTH)),
                                   NULL);

        
        LIns* br3 = lir->insBranch(LIR_jt, lir->ins_eq0(dslots_ins), NULL);

        
        LIns* br4 = lir->insBranch(LIR_jf,
                                   lir->ins2(LIR_ult,
                                             idx_ins,
                                             lir->insLoad(LIR_ldp,
                                                          dslots_ins,
                                                          -(int)sizeof(jsval))),
                                   NULL);
        lir->insGuard(LIR_x, lir->insImm(1), createGuardRecord(exit));
        LIns* label = lir->ins0(LIR_label);
        if (br1)
            br1->setTarget(label);
        br2->setTarget(label);
        br3->setTarget(label);
        br4->setTarget(label);

        CHECK_STATUS(guardPrototypeHasNoIndexedProperties(obj, obj_ins, MISMATCH_EXIT));

        
        v_ins = lir->insImm(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID));
        addr_ins = NULL;
        return JSRS_CONTINUE;
    }

    
    if (MAX_DSLOTS_LENGTH > JS_BITMASK(30) && !idx_ins->isconst()) {
        JS_ASSERT(sizeof(jsval) == 8); 
        guard(false,
              lir->ins2i(LIR_lt, idx_ins, 0),
              exit);
    }

    
    guard(true,
          lir->ins2(LIR_ult, idx_ins, stobj_get_fslot(obj_ins, JSSLOT_ARRAY_LENGTH)),
          exit);

    
    guard(false,
          lir->ins_eq0(dslots_ins),
          exit);

    
    guard(true,
          lir->ins2(LIR_ult,
                    idx_ins,
                    lir->insLoad(LIR_ldp, dslots_ins, 0 - (int)sizeof(jsval))),
          exit);

    
    vp = &obj->dslots[jsuint(idx)];
    addr_ins = lir->ins2(LIR_piadd, dslots_ins,
                         lir->ins2i(LIR_pilsh, idx_ins, (sizeof(jsval) == 4) ? 2 : 3));
    v_ins = lir->insLoad(LIR_ldp, addr_ins, 0);
    unbox_jsval(*vp, v_ins, exit);

    if (JSVAL_TAG(*vp) == JSVAL_BOOLEAN) {
        



        LIns* br = lir->insBranch(LIR_jf,
                                  lir->ins2i(LIR_eq, v_ins, JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_HOLE)),
                                  NULL);
        CHECK_STATUS(guardPrototypeHasNoIndexedProperties(obj, obj_ins, MISMATCH_EXIT));
        br->setTarget(lir->ins0(LIR_label));

        


        v_ins = lir->ins2i(LIR_and, v_ins, ~(JSVAL_HOLE_FLAG >> JSVAL_TAGBITS));
    }
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::getProp(JSObject* obj, LIns* obj_ins)
{
    uint32 slot;
    LIns* v_ins;
    CHECK_STATUS(prop(obj, obj_ins, slot, v_ins));

    const JSCodeSpec& cs = js_CodeSpec[*cx->fp->regs->pc];
    JS_ASSERT(cs.ndefs == 1);
    stack(-cs.nuses, v_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::getProp(jsval& v)
{
    if (JSVAL_IS_PRIMITIVE(v))
        ABORT_TRACE("primitive lhs");

    return getProp(JSVAL_TO_OBJECT(v), get(&v));
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NAME()
{
    jsval* vp;
    CHECK_STATUS(name(vp));
    stack(0, get(vp));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DOUBLE()
{
    jsval v = jsval(atoms[GET_INDEX(cx->fp->regs->pc)]);
    stack(0, lir->insImmf(*JSVAL_TO_DOUBLE(v)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_STRING()
{
    JSAtom* atom = atoms[GET_INDEX(cx->fp->regs->pc)];
    JS_ASSERT(ATOM_IS_STRING(atom));
    stack(0, INS_CONSTPTR(ATOM_TO_STRING(atom)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ZERO()
{
    stack(0, lir->insImmq(0));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ONE()
{
    stack(0, lir->insImmf(1));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NULL()
{
    stack(0, INS_CONSTPTR(NULL));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_THIS()
{
    LIns* this_ins;
    CHECK_STATUS(getThis(this_ins));
    stack(0, this_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_FALSE()
{
    stack(0, lir->insImm(0));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TRUE()
{
    stack(0, lir->insImm(1));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_OR()
{
    return ifop();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_AND()
{
    return ifop();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TABLESWITCH()
{
#ifdef NANOJIT_IA32
    
    LIns* guardIns = tableswitch();
    if (guardIns) {
        fragment->lastIns = guardIns;
        compile(&JS_TRACE_MONITOR(cx));
    }
    return JSRS_STOP;
#else
    return switchop();
#endif
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LOOKUPSWITCH()
{
    return switchop();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_STRICTEQ()
{
    strictEquality(true, false);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_STRICTNE()
{
    strictEquality(false, false);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_OBJECT()
{
    JSStackFrame* fp = cx->fp;
    JSScript* script = fp->script;
    unsigned index = atoms - script->atomMap.vector + GET_INDEX(fp->regs->pc);

    JSObject* obj;
    JS_GET_SCRIPT_OBJECT(script, index, obj);
    stack(0, INS_CONSTPTR(obj));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_POP()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TRAP()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETARG()
{
    stack(0, arg(GET_ARGNO(cx->fp->regs->pc)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETARG()
{
    arg(GET_ARGNO(cx->fp->regs->pc), stack(-1));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETLOCAL()
{
    stack(0, var(GET_SLOTNO(cx->fp->regs->pc)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETLOCAL()
{
    var(GET_SLOTNO(cx->fp->regs->pc), stack(-1));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_UINT16()
{
    stack(0, lir->insImmf(GET_UINT16(cx->fp->regs->pc)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NEWINIT()
{
    JSProtoKey key = JSProtoKey(GET_INT8(cx->fp->regs->pc));
    LIns *proto_ins;
    CHECK_STATUS(getClassPrototype(key, proto_ins));

    LIns* args[] = { proto_ins, cx_ins };
    const CallInfo *ci = (key == JSProto_Array) ? &js_NewEmptyArray_ci : &js_Object_tn_ci;
    LIns* v_ins = lir->insCall(ci, args);
    guard(false, lir->ins_eq0(v_ins), OOM_EXIT);
    stack(0, v_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ENDINIT()
{
#ifdef DEBUG
    jsval& v = stackval(-1);
    JS_ASSERT(!JSVAL_IS_PRIMITIVE(v));
#endif
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INITPROP()
{
    
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INITELEM()
{
    return record_JSOP_SETELEM();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFSHARP()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_USESHARP()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INCARG()
{
    return inc(argval(GET_ARGNO(cx->fp->regs->pc)), 1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INCLOCAL()
{
    return inc(varval(GET_SLOTNO(cx->fp->regs->pc)), 1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DECARG()
{
    return inc(argval(GET_ARGNO(cx->fp->regs->pc)), -1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DECLOCAL()
{
    return inc(varval(GET_SLOTNO(cx->fp->regs->pc)), -1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ARGINC()
{
    return inc(argval(GET_ARGNO(cx->fp->regs->pc)), 1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LOCALINC()
{
    return inc(varval(GET_SLOTNO(cx->fp->regs->pc)), 1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ARGDEC()
{
    return inc(argval(GET_ARGNO(cx->fp->regs->pc)), -1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LOCALDEC()
{
    return inc(varval(GET_SLOTNO(cx->fp->regs->pc)), -1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_IMACOP()
{
    JS_ASSERT(cx->fp->imacpc);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ITER()
{
    jsval& v = stackval(-1);
    if (JSVAL_IS_PRIMITIVE(v))
        ABORT_TRACE("for-in on a primitive value");
    ABORT_IF_XML(v);

    jsuint flags = cx->fp->regs->pc[1];

    if (hasIteratorMethod(JSVAL_TO_OBJECT(v))) {
        if (flags == JSITER_ENUMERATE)
            return call_imacro(iter_imacros.for_in);
        if (flags == (JSITER_ENUMERATE | JSITER_FOREACH))
            return call_imacro(iter_imacros.for_each);
    } else {
        if (flags == JSITER_ENUMERATE)
            return call_imacro(iter_imacros.for_in_native);
        if (flags == (JSITER_ENUMERATE | JSITER_FOREACH))
            return call_imacro(iter_imacros.for_each_native);
    }
    ABORT_TRACE("unimplemented JSITER_* flags");
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NEXTITER()
{
    jsval& iterobj_val = stackval(-2);
    if (JSVAL_IS_PRIMITIVE(iterobj_val))
        ABORT_TRACE("for-in on a primitive value");
    ABORT_IF_XML(iterobj_val);
    JSObject* iterobj = JSVAL_TO_OBJECT(iterobj_val);
    JSClass* clasp = STOBJ_GET_CLASS(iterobj);
    LIns* iterobj_ins = get(&iterobj_val);
    if (clasp == &js_IteratorClass || clasp == &js_GeneratorClass) {
        guardClass(iterobj, iterobj_ins, clasp, snapshot(BRANCH_EXIT));
        return call_imacro(nextiter_imacros.native_iter_next);
    }
    return call_imacro(nextiter_imacros.custom_iter_next);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ENDITER()
{
    LIns* args[] = { stack(-2), cx_ins };
    LIns* ok_ins = lir->insCall(&js_CloseIterator_ci, args);
    guard(false, lir->ins_eq0(ok_ins), MISMATCH_EXIT);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_FORNAME()
{
    jsval* vp;
    CHECK_STATUS(name(vp));
    set(vp, stack(-1));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_FORPROP()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_FORELEM()
{
    return record_JSOP_DUP();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_FORARG()
{
    return record_JSOP_SETARG();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_FORLOCAL()
{
    return record_JSOP_SETLOCAL();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_POPN()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_BINDNAME()
{
    JSStackFrame *fp = cx->fp;
    JSObject *obj;

    if (fp->fun) {
        
        
        
        if (JSFUN_HEAVYWEIGHT_TEST(fp->fun->flags))
            ABORT_TRACE("Can't trace JSOP_BINDNAME in heavyweight functions.");

        
        
        obj = OBJ_GET_PARENT(cx, fp->callee);
    } else {
        obj = fp->scopeChain;

        
        
        
        while (OBJ_GET_CLASS(cx, obj) == &js_BlockClass) {
            
            JS_ASSERT(OBJ_GET_PRIVATE(cx, obj) == fp);

            obj = OBJ_GET_PARENT(cx, obj);

            
            JS_ASSERT(obj);
        }
    }

    if (obj != globalObj)
        ABORT_TRACE("JSOP_BINDNAME must return global object on trace");

    
    
    
    
    
    
    stack(0, INS_CONSTPTR(globalObj));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETNAME()
{
    jsval& l = stackval(-2);
    JS_ASSERT(!JSVAL_IS_PRIMITIVE(l));

    



    JSObject* obj = JSVAL_TO_OBJECT(l);
    if (obj != cx->fp->scopeChain || obj != globalObj)
        ABORT_TRACE("JSOP_SETNAME left operand is not the global object");

    
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_THROW()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_IN()
{
    jsval& rval = stackval(-1);
    jsval& lval = stackval(-2);

    if (JSVAL_IS_PRIMITIVE(rval))
        ABORT_TRACE("JSOP_IN on non-object right operand");
    JSObject* obj = JSVAL_TO_OBJECT(rval);
    LIns* obj_ins = get(&rval);

    jsid id;
    LIns* x;
    if (JSVAL_IS_INT(lval)) {
        id = INT_JSVAL_TO_JSID(lval);
        LIns* args[] = { makeNumberInt32(get(&lval)), obj_ins, cx_ins };
        x = lir->insCall(&js_HasNamedPropertyInt32_ci, args);
    } else if (JSVAL_IS_STRING(lval)) {
        if (!js_ValueToStringId(cx, lval, &id))
            ABORT_TRACE_ERROR("left operand of JSOP_IN didn't convert to a string-id");
        LIns* args[] = { get(&lval), obj_ins, cx_ins };
        x = lir->insCall(&js_HasNamedProperty_ci, args);
    } else {
        ABORT_TRACE("string or integer expected");
    }

    guard(false, lir->ins2i(LIR_eq, x, JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID)), OOM_EXIT);
    x = lir->ins2i(LIR_eq, x, 1);

    JSObject* obj2;
    JSProperty* prop;
    if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
        ABORT_TRACE_ERROR("OBJ_LOOKUP_PROPERTY failed in JSOP_IN");
    bool cond = prop != NULL;
    if (prop)
        OBJ_DROP_PROPERTY(cx, obj2, prop);

    

    fuseIf(cx->fp->regs->pc + 1, cond, x);

    




    set(&lval, x);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INSTANCEOF()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEBUGGER()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GOSUB()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_RETSUB()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_EXCEPTION()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LINENO()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CONDSWITCH()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CASE()
{
    strictEquality(true, true);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFAULT()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_EVAL()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ENUMELEM()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETTER()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETTER()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFFUN()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFFUN_FC()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFCONST()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFVAR()
{
    return JSRS_STOP;
}

jsatomid
TraceRecorder::getFullIndex(ptrdiff_t pcoff)
{
    jsatomid index = GET_INDEX(cx->fp->regs->pc + pcoff);
    index += atoms - cx->fp->script->atomMap.vector;
    return index;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LAMBDA()
{
    JSFunction* fun;
    JS_GET_SCRIPT_FUNCTION(cx->fp->script, getFullIndex(), fun);

    if (FUN_NULL_CLOSURE(fun) && OBJ_GET_PARENT(cx, FUN_OBJECT(fun)) == globalObj) {
        LIns *proto_ins;
        CHECK_STATUS(getClassPrototype(JSProto_Function, proto_ins));

        LIns* args[] = { INS_CONSTPTR(globalObj), proto_ins, INS_CONSTPTR(fun), cx_ins };
        LIns* x = lir->insCall(&js_NewNullClosure_ci, args);
        stack(0, x);
        return JSRS_CONTINUE;
    }
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LAMBDA_FC()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLEE()
{
    stack(0, get(&cx->fp->argv[-2]));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETLOCALPOP()
{
    var(GET_SLOTNO(cx->fp->regs->pc), stack(-1));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_IFPRIMTOP()
{
    
    
    
    
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETCALL()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TRY()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_FINALLY()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NOP()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ARGSUB()
{
    JSStackFrame* fp = cx->fp;
    if (!(fp->fun->flags & JSFUN_HEAVYWEIGHT)) {
        uintN slot = GET_ARGNO(fp->regs->pc);
        if (slot < fp->fun->nargs && slot < fp->argc && !fp->argsobj) {
            stack(0, get(&cx->fp->argv[slot]));
            return JSRS_CONTINUE;
        }
    }
    ABORT_TRACE("can't trace JSOP_ARGSUB hard case");
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ARGCNT()
{
    if (!(cx->fp->fun->flags & JSFUN_HEAVYWEIGHT)) {
        stack(0, lir->insImmf(cx->fp->argc));
        return JSRS_CONTINUE;
    }
    ABORT_TRACE("can't trace heavyweight JSOP_ARGCNT");
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_DefLocalFunSetSlot(uint32 slot, JSObject* obj)
{
    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, obj);

    if (FUN_NULL_CLOSURE(fun) && OBJ_GET_PARENT(cx, FUN_OBJECT(fun)) == globalObj) {
        LIns *proto_ins;
        CHECK_STATUS(getClassPrototype(JSProto_Function, proto_ins));

        LIns* args[] = { INS_CONSTPTR(globalObj), proto_ins, INS_CONSTPTR(fun), cx_ins };
        LIns* x = lir->insCall(&js_NewNullClosure_ci, args);
        var(slot, x);
        return JSRS_CONTINUE;
    }

    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFLOCALFUN()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFLOCALFUN_FC()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GOTOX()
{
    return record_JSOP_GOTO();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_IFEQX()
{
    return record_JSOP_IFEQ();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_IFNEX()
{
    return record_JSOP_IFNE();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ORX()
{
    return record_JSOP_OR();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ANDX()
{
    return record_JSOP_AND();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GOSUBX()
{
    return record_JSOP_GOSUB();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CASEX()
{
    strictEquality(true, true);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFAULTX()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TABLESWITCHX()
{
    return record_JSOP_TABLESWITCH();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LOOKUPSWITCHX()
{
    return switchop();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_BACKPATCH()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_BACKPATCH_POP()
{
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_THROWING()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETRVAL()
{
    
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_RETRVAL()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETGVAR()
{
    jsval slotval = cx->fp->slots[GET_SLOTNO(cx->fp->regs->pc)];
    if (JSVAL_IS_NULL(slotval))
        return JSRS_CONTINUE; 

    uint32 slot = JSVAL_TO_INT(slotval);

    if (!lazilyImportGlobalSlot(slot))
         ABORT_TRACE("lazy import of global slot failed");

    stack(0, get(&STOBJ_GET_SLOT(globalObj, slot)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETGVAR()
{
    jsval slotval = cx->fp->slots[GET_SLOTNO(cx->fp->regs->pc)];
    if (JSVAL_IS_NULL(slotval))
        return JSRS_CONTINUE; 

    uint32 slot = JSVAL_TO_INT(slotval);

    if (!lazilyImportGlobalSlot(slot))
         ABORT_TRACE("lazy import of global slot failed");

    set(&STOBJ_GET_SLOT(globalObj, slot), stack(-1));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INCGVAR()
{
    jsval slotval = cx->fp->slots[GET_SLOTNO(cx->fp->regs->pc)];
    if (JSVAL_IS_NULL(slotval))
        
        return JSRS_CONTINUE;

    uint32 slot = JSVAL_TO_INT(slotval);

    if (!lazilyImportGlobalSlot(slot))
         ABORT_TRACE("lazy import of global slot failed");

    return inc(STOBJ_GET_SLOT(globalObj, slot), 1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DECGVAR()
{
    jsval slotval = cx->fp->slots[GET_SLOTNO(cx->fp->regs->pc)];
    if (JSVAL_IS_NULL(slotval))
        
        return JSRS_CONTINUE;

    uint32 slot = JSVAL_TO_INT(slotval);

    if (!lazilyImportGlobalSlot(slot))
         ABORT_TRACE("lazy import of global slot failed");

    return inc(STOBJ_GET_SLOT(globalObj, slot), -1);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GVARINC()
{
    jsval slotval = cx->fp->slots[GET_SLOTNO(cx->fp->regs->pc)];
    if (JSVAL_IS_NULL(slotval))
        
        return JSRS_CONTINUE;

    uint32 slot = JSVAL_TO_INT(slotval);

    if (!lazilyImportGlobalSlot(slot))
         ABORT_TRACE("lazy import of global slot failed");

    return inc(STOBJ_GET_SLOT(globalObj, slot), 1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GVARDEC()
{
    jsval slotval = cx->fp->slots[GET_SLOTNO(cx->fp->regs->pc)];
    if (JSVAL_IS_NULL(slotval))
        
        return JSRS_CONTINUE;

    uint32 slot = JSVAL_TO_INT(slotval);

    if (!lazilyImportGlobalSlot(slot))
         ABORT_TRACE("lazy import of global slot failed");

    return inc(STOBJ_GET_SLOT(globalObj, slot), -1, false);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_REGEXP()
{
    return JSRS_STOP;
}



JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DEFXMLNS()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ANYNAME()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_QNAMEPART()
{
    return record_JSOP_STRING();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_QNAMECONST()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_QNAME()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TOATTRNAME()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TOATTRVAL()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ADDATTRNAME()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ADDATTRVAL()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_BINDXMLNAME()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETXMLNAME()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_XMLNAME()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DESCENDANTS()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_FILTER()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ENDFILTER()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TOXML()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TOXMLLIST()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_XMLTAGEXPR()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_XMLELTEXPR()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_XMLOBJECT()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_XMLCDATA()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_XMLCOMMENT()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_XMLPI()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETFUNNS()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_STARTXML()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_STARTXMLEXPR()
{
    return JSRS_STOP;
}



JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLPROP()
{
    jsval& l = stackval(-1);
    JSObject* obj;
    LIns* obj_ins;
    LIns* this_ins;
    if (!JSVAL_IS_PRIMITIVE(l)) {
        obj = JSVAL_TO_OBJECT(l);
        obj_ins = get(&l);
        this_ins = obj_ins; 
    } else {
        jsint i;
        debug_only(const char* protoname = NULL;)
        if (JSVAL_IS_STRING(l)) {
            i = JSProto_String;
            debug_only(protoname = "String.prototype";)
        } else if (JSVAL_IS_NUMBER(l)) {
            i = JSProto_Number;
            debug_only(protoname = "Number.prototype";)
        } else if (JSVAL_TAG(l) == JSVAL_BOOLEAN) {
            if (l == JSVAL_VOID)
                ABORT_TRACE("callprop on void");
            guard(false, lir->ins2i(LIR_eq, get(&l), JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID)), MISMATCH_EXIT);
            i = JSProto_Boolean;
            debug_only(protoname = "Boolean.prototype";)
        } else {
            JS_ASSERT(JSVAL_IS_NULL(l) || JSVAL_IS_VOID(l));
            ABORT_TRACE("callprop on null or void");
        }

        if (!js_GetClassPrototype(cx, NULL, INT_TO_JSID(i), &obj))
            ABORT_TRACE_ERROR("GetClassPrototype failed!");

        obj_ins = INS_CONSTPTR(obj);
        debug_only(obj_ins = addName(obj_ins, protoname);)
        this_ins = get(&l); 
    }

    JSObject* obj2;
    jsuword pcval;
    CHECK_STATUS(test_property_cache(obj, obj_ins, obj2, pcval));

    if (PCVAL_IS_NULL(pcval) || !PCVAL_IS_OBJECT(pcval))
        ABORT_TRACE("callee is not an object");
    JS_ASSERT(HAS_FUNCTION_CLASS(PCVAL_TO_OBJECT(pcval)));

    if (JSVAL_IS_PRIMITIVE(l)) {
        JSFunction* fun = GET_FUNCTION_PRIVATE(cx, PCVAL_TO_OBJECT(pcval));
        if (!PRIMITIVE_THIS_TEST(fun, l))
            ABORT_TRACE("callee does not accept primitive |this|");
    }

    stack(0, this_ins);
    stack(-1, INS_CONSTPTR(PCVAL_TO_OBJECT(pcval)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_DELDESC()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_UINT24()
{
    stack(0, lir->insImmf(GET_UINT24(cx->fp->regs->pc)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INDEXBASE()
{
    atoms += GET_INDEXBASE(cx->fp->regs->pc);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_RESETBASE()
{
    atoms = cx->fp->script->atomMap.vector;
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_RESETBASE0()
{
    atoms = cx->fp->script->atomMap.vector;
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLELEM()
{
    return record_JSOP_GETELEM();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_STOP()
{
    JSStackFrame *fp = cx->fp;

    if (fp->imacpc) {
        
        
        
        atoms = fp->script->atomMap.vector;
        return JSRS_CONTINUE;
    }

    







    if (fp->flags & JSFRAME_CONSTRUCTING) {
        JS_ASSERT(OBJECT_TO_JSVAL(fp->thisp) == fp->argv[-1]);
        rval_ins = get(&fp->argv[-1]);
    } else {
        rval_ins = INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID));
    }
    clearFrameSlotsFromCache();
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETXPROP()
{
    jsval& l = stackval(-1);
    if (JSVAL_IS_PRIMITIVE(l))
        ABORT_TRACE("primitive-this for GETXPROP?");

    JSObject* obj = JSVAL_TO_OBJECT(l);
    if (obj != cx->fp->scopeChain || obj != globalObj)
        return JSRS_STOP;

    jsval* vp;
    CHECK_STATUS(name(vp));
    stack(-1, get(vp));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLXMLNAME()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_TYPEOFEXPR()
{
    return record_JSOP_TYPEOF();
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ENTERBLOCK()
{
    JSObject* obj;
    JS_GET_SCRIPT_OBJECT(cx->fp->script, getFullIndex(0), obj);

    LIns* void_ins = INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_VOID));
    for (int i = 0, n = OBJ_BLOCK_COUNT(cx, obj); i < n; i++)
        stack(i, void_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LEAVEBLOCK()
{
    
    if (cx->fp->blockChain != lexicalBlock)
        return JSRS_CONTINUE;
    else
        return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GENERATOR()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_YIELD()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ARRAYPUSH()
{
    uint32_t slot = GET_UINT16(cx->fp->regs->pc);
    JS_ASSERT(cx->fp->script->nfixed <= slot);
    JS_ASSERT(cx->fp->slots + slot < cx->fp->regs->sp - 1);
    jsval &arrayval = cx->fp->slots[slot];
    JS_ASSERT(JSVAL_IS_OBJECT(arrayval));
    JS_ASSERT(OBJ_IS_DENSE_ARRAY(cx, JSVAL_TO_OBJECT(arrayval)));
    LIns *array_ins = get(&arrayval);
    jsval &elt = stackval(-1);
    LIns *elt_ins = get(&elt);
    box_jsval(elt, elt_ins);

    LIns *args[] = { elt_ins, array_ins, cx_ins };
    LIns *ok_ins = lir->insCall(&js_ArrayCompPush_ci, args);
    guard(false, lir->ins_eq0(ok_ins), OOM_EXIT);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_ENUMCONSTELEM()
{
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LEAVEBLOCKEXPR()
{
    LIns* v_ins = stack(-1);
    int n = -1 - GET_UINT16(cx->fp->regs->pc);
    stack(n, v_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETTHISPROP()
{
    LIns* this_ins;

    CHECK_STATUS(getThis(this_ins));
    



    CHECK_STATUS(getProp(cx->fp->thisp, this_ins));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETARGPROP()
{
    return getProp(argval(GET_ARGNO(cx->fp->regs->pc)));
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETLOCALPROP()
{
    return getProp(varval(GET_SLOTNO(cx->fp->regs->pc)));
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INDEXBASE1()
{
    atoms += 1 << 16;
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INDEXBASE2()
{
    atoms += 2 << 16;
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INDEXBASE3()
{
    atoms += 3 << 16;
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLGVAR()
{
    jsval slotval = cx->fp->slots[GET_SLOTNO(cx->fp->regs->pc)];
    if (JSVAL_IS_NULL(slotval))
        
        return JSRS_CONTINUE;

    uint32 slot = JSVAL_TO_INT(slotval);

    if (!lazilyImportGlobalSlot(slot))
         ABORT_TRACE("lazy import of global slot failed");

    jsval& v = STOBJ_GET_SLOT(globalObj, slot);
    stack(0, get(&v));
    stack(1, INS_CONSTPTR(NULL));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLLOCAL()
{
    uintN slot = GET_SLOTNO(cx->fp->regs->pc);
    stack(0, var(slot));
    stack(1, INS_CONSTPTR(NULL));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLARG()
{
    uintN slot = GET_ARGNO(cx->fp->regs->pc);
    stack(0, arg(slot));
    stack(1, INS_CONSTPTR(NULL));
    return JSRS_CONTINUE;
}



static JSBool
ObjectToIterator(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv = JS_ARGV(cx, vp);
    JS_ASSERT(JSVAL_IS_INT(argv[0]));
    JS_SET_RVAL(cx, vp, JS_THIS(cx, vp));
    return js_ValueToIterator(cx, JSVAL_TO_INT(argv[0]), &JS_RVAL(cx, vp));
}

static JSObject* FASTCALL
ObjectToIterator_tn(JSContext* cx, jsbytecode* pc, JSObject *obj, int32 flags)
{
    jsval v = OBJECT_TO_JSVAL(obj);
    JSBool ok = js_ValueToIterator(cx, flags, &v);

    if (!ok) {
        js_SetBuiltinError(cx);
        return NULL;
    }
    return JSVAL_TO_OBJECT(v);
}

static JSBool
CallIteratorNext(JSContext *cx, uintN argc, jsval *vp)
{
    return js_CallIteratorNext(cx, JS_THIS_OBJECT(cx, vp), &JS_RVAL(cx, vp));
}

static jsval FASTCALL
CallIteratorNext_tn(JSContext* cx, jsbytecode* pc, JSObject* iterobj)
{
    JSAutoTempValueRooter tvr(cx);
    JSBool ok = js_CallIteratorNext(cx, iterobj, tvr.addr());

    if (!ok) {
        js_SetBuiltinError(cx);
        return JSVAL_ERROR_COOKIE;
    }
    return tvr.value();
}

JS_DEFINE_TRCINFO_1(ObjectToIterator,
    (4, (static, OBJECT_FAIL, ObjectToIterator_tn, CONTEXT, PC, THIS, INT32, 0, 0)))
JS_DEFINE_TRCINFO_1(CallIteratorNext,
    (3, (static, JSVAL_FAIL,  CallIteratorNext_tn, CONTEXT, PC, THIS,        0, 0)))

static const struct BuiltinFunctionInfo {
    JSTraceableNative *tn;
    int nargs;
} builtinFunctionInfo[JSBUILTIN_LIMIT] = {
    {ObjectToIterator_trcinfo,   1},
    {CallIteratorNext_trcinfo,   0},
    {GetProperty_trcinfo,        1},
    {GetElement_trcinfo,         1},
    {SetProperty_trcinfo,        2},
    {SetElement_trcinfo,         2}
};

JSObject *
js_GetBuiltinFunction(JSContext *cx, uintN index)
{
    JSRuntime *rt = cx->runtime;
    JSObject *funobj = rt->builtinFunctions[index];

    if (!funobj) {
        
        JS_ASSERT(index < JS_ARRAY_LENGTH(builtinFunctionInfo));
        const BuiltinFunctionInfo *bfi = &builtinFunctionInfo[index];
        JSFunction *fun = js_NewFunction(cx,
                                         NULL,
                                         JS_DATA_TO_FUNC_PTR(JSNative, bfi->tn),
                                         bfi->nargs,
                                         JSFUN_FAST_NATIVE | JSFUN_TRACEABLE,
                                         NULL,
                                         NULL);
        if (fun) {
            funobj = FUN_OBJECT(fun);
            STOBJ_CLEAR_PROTO(funobj);
            STOBJ_CLEAR_PARENT(funobj);

            JS_LOCK_GC(rt);
            if (!rt->builtinFunctions[index])  
                rt->builtinFunctions[index] = funobj;
            else
                funobj = rt->builtinFunctions[index];
            JS_UNLOCK_GC(rt);
        }
    }
    return funobj;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLBUILTIN()
{
    JSObject *obj = js_GetBuiltinFunction(cx, GET_INDEX(cx->fp->regs->pc));
    if (!obj)
        ABORT_TRACE_ERROR("error in js_GetBuiltinFunction");

    stack(0, get(&stackval(-1)));
    stack(-1, INS_CONSTPTR(obj));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INT8()
{
    stack(0, lir->insImmf(GET_INT8(cx->fp->regs->pc)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INT32()
{
    stack(0, lir->insImmf(GET_INT32(cx->fp->regs->pc)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LENGTH()
{
    jsval& l = stackval(-1);
    if (JSVAL_IS_PRIMITIVE(l)) {
        if (!JSVAL_IS_STRING(l))
            ABORT_TRACE("non-string primitive JSOP_LENGTH unsupported");
        set(&l, lir->ins1(LIR_i2f, getStringLength(get(&l))));
        return JSRS_CONTINUE;
    }

    JSObject* obj = JSVAL_TO_OBJECT(l);
    LIns* obj_ins = get(&l);
    LIns* v_ins;
    if (OBJ_IS_ARRAY(cx, obj)) {
        if (OBJ_IS_DENSE_ARRAY(cx, obj)) {
            if (!guardDenseArray(obj, obj_ins, BRANCH_EXIT)) {
                JS_NOT_REACHED("OBJ_IS_DENSE_ARRAY but not?!?");
                return JSRS_STOP;
            }
        } else {
            if (!guardClass(obj, obj_ins, &js_SlowArrayClass, snapshot(BRANCH_EXIT)))
                ABORT_TRACE("can't trace length property access on non-array");
        }
        v_ins = lir->ins1(LIR_i2f, stobj_get_fslot(obj_ins, JSSLOT_ARRAY_LENGTH));
    } else {
        if (!OBJ_IS_NATIVE(obj))
            ABORT_TRACE("can't trace length property access on non-array, non-native object");
        return getProp(obj, obj_ins);
    }
    set(&l, v_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NEWARRAY()
{
    LIns *proto_ins;
    CHECK_STATUS(getClassPrototype(JSProto_Array, proto_ins));

    uint32 len = GET_UINT16(cx->fp->regs->pc);
    cx->fp->assertValidStackDepth(len);

    LIns* args[] = { lir->insImm(len), proto_ins, cx_ins };
    LIns* v_ins = lir->insCall(&js_NewUninitializedArray_ci, args);
    guard(false, lir->ins_eq0(v_ins), OOM_EXIT);

    LIns* dslots_ins = NULL;
    uint32 count = 0;
    for (uint32 i = 0; i < len; i++) {
        jsval& v = stackval(int(i) - int(len));
        if (v != JSVAL_HOLE)
            count++;
        LIns* elt_ins = get(&v);
        box_jsval(v, elt_ins);
        stobj_set_dslot(v_ins, i, dslots_ins, elt_ins, "set_array_elt");
    }

    if (count > 0)
        stobj_set_fslot(v_ins, JSSLOT_ARRAY_COUNT, INS_CONST(count), "set_array_count");

    stack(-int(len), v_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_HOLE()
{
    stack(0, INS_CONST(JSVAL_TO_PSEUDO_BOOLEAN(JSVAL_HOLE)));
    return JSRS_CONTINUE;
}

JSRecordingStatus
TraceRecorder::record_JSOP_LOOP()
{
    return JSRS_CONTINUE;
}

#ifdef JS_JIT_SPEW

void
js_DumpPeerStability(JSTraceMonitor* tm, const void* ip, JSObject* globalObj, uint32 globalShape,
                     uint32 argc)
{
    Fragment* f;
    TreeInfo* ti;
    bool looped = false;
    unsigned length = 0;

    for (f = getLoop(tm, ip, globalObj, globalShape, argc); f != NULL; f = f->peer) {
        if (!f->vmprivate)
            continue;
        printf("fragment %p:\nENTRY: ", (void*)f);
        ti = (TreeInfo*)f->vmprivate;
        if (looped)
            JS_ASSERT(ti->nStackTypes == length);
        for (unsigned i = 0; i < ti->nStackTypes; i++)
            printf("S%d ", ti->stackTypeMap()[i]);
        for (unsigned i = 0; i < ti->nGlobalTypes(); i++)
            printf("G%d ", ti->globalTypeMap()[i]);
        printf("\n");
        UnstableExit* uexit = ti->unstableExits;
        while (uexit != NULL) {
            printf("EXIT:  ");
            uint8* m = getFullTypeMap(uexit->exit);
            for (unsigned i = 0; i < uexit->exit->numStackSlots; i++)
                printf("S%d ", m[i]);
            for (unsigned i = 0; i < uexit->exit->numGlobalSlots; i++)
                printf("G%d ", m[uexit->exit->numStackSlots + i]);
            printf("\n");
            uexit = uexit->next;
        }
        length = ti->nStackTypes;
        looped = true;
    }
}
#endif

#define UNUSED(n)                                                             \
    JS_REQUIRES_STACK bool                                                    \
    TraceRecorder::record_JSOP_UNUSED##n() {                                  \
        JS_NOT_REACHED("JSOP_UNUSED" # n);                                    \
        return false;                                                         \
    }
