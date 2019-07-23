








































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
#include "jsmath.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsdate.h"
#include "jsstaticcheck.h"
#include "jstracer.h"
#include "jsxml.h"

#include "jsatominlines.h"

#include "jsautooplen.h"        
#include "imacros.c.out"

using namespace avmplus;
using namespace nanojit;

#if JS_HAS_XML_SUPPORT
#define ABORT_IF_XML(v)                                                       \
    JS_BEGIN_MACRO                                                            \
    if (!JSVAL_IS_PRIMITIVE(v) && OBJECT_IS_XML(BOGUS_CX, JSVAL_TO_OBJECT(v)))\
        ABORT_TRACE("xml detected");                                          \
    JS_END_MACRO
#else
#define ABORT_IF_XML(v) ((void) 0)
#endif






#undef JSVAL_IS_BOOLEAN
#define JSVAL_IS_BOOLEAN(x) JS_STATIC_ASSERT(0)

JS_STATIC_ASSERT(sizeof(JSTraceType) == 1);


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


#define MAX_INTERP_STACK_BYTES                                                \
    (MAX_NATIVE_STACK_SLOTS * sizeof(jsval) +                                 \
     MAX_CALL_STACK_ENTRIES * sizeof(JSInlineFrame) +                         \
     sizeof(JSInlineFrame)) /* possibly slow native frame at top of stack */


#define MAX_BRANCHES 32

#define CHECK_STATUS(expr)                                                    \
    JS_BEGIN_MACRO                                                            \
        JSRecordingStatus _status = (expr);                                   \
        if (_status != JSRS_CONTINUE)                                         \
          return _status;                                                     \
    JS_END_MACRO

#ifdef JS_JIT_SPEW
#define ABORT_TRACE_RV(msg, value)                                            \
    JS_BEGIN_MACRO                                                            \
        debug_only_printf(LC_TMAbort, "abort: %d: %s\n", __LINE__, (msg));    \
        return (value);                                                       \
    JS_END_MACRO
#else
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
    0,
    JS_PropertyStub,       JS_PropertyStub,
    jitstats_getProperty,  JS_PropertyStub,
    JS_EnumerateStub,      JS_ResolveStub,
    JS_ConvertStub,        NULL,
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







#define INS_CONST(c)          addName(lir->insImm(c), #c)
#define INS_CONSTPTR(p)       addName(lir->insImmPtr(p), #p)
#define INS_CONSTWORD(v)      addName(lir->insImmPtr((void *) v), #v)
#define INS_CONSTOBJ(obj)     addName(insImmObj(obj), #obj)
#define INS_CONSTFUN(fun)     addName(insImmFun(fun), #fun)
#define INS_CONSTSTR(str)     addName(insImmStr(str), #str)
#define INS_CONSTSPROP(sprop) addName(insImmSprop(sprop), #sprop)
#define INS_ATOM(atom)        INS_CONSTSTR(ATOM_TO_STRING(atom))
#define INS_NULL()            INS_CONSTPTR(NULL)
#define INS_VOID()            INS_CONST(JSVAL_TO_SPECIAL(JSVAL_VOID))

static GC gc = GC();
static avmplus::AvmCore s_core = avmplus::AvmCore();
static avmplus::AvmCore* core = &s_core;



void*
nanojit::Allocator::allocChunk(size_t nbytes)
{
    VMAllocator *vma = (VMAllocator*)this;
    JS_ASSERT(!vma->outOfMemory());
    void *p = malloc(nbytes);
    if (!p) {
        JS_ASSERT(nbytes < sizeof(vma->mReserve));
        vma->mOutOfMemory = true;
        p = (void*) &vma->mReserve[0];
    }
    vma->mSize += nbytes;
    return p;
}

void
nanojit::Allocator::freeChunk(void *p) {
    VMAllocator *vma = (VMAllocator*)this;
    if (p != &vma->mReserve[0])
        free(p);
}

void
nanojit::Allocator::postReset() {
    VMAllocator *vma = (VMAllocator*)this;
    vma->mOutOfMemory = false;
    vma->mSize = 0;
}


#ifdef JS_JIT_SPEW
static void
DumpPeerStability(JSTraceMonitor* tm, const void* ip, JSObject* globalObj, uint32 globalShape, uint32 argc);
#endif








static bool did_we_check_processor_features = false;







LogControl js_LogController;

#ifdef JS_JIT_SPEW





static bool did_we_set_up_debug_logging = false;

static void
InitJITLogController()
{
    char *tm, *tmf;
    uint32_t bits;

    js_LogController.lcbits = 0;

    tm = getenv("TRACEMONKEY");
    if (tm) goto help;

    tmf = getenv("TMFLAGS");
    if (!tmf) return;

    
    if (strstr(tmf, "help")) goto help;

    bits = 0;

    
    if (strstr(tmf, "minimal"))     bits |= LC_TMMinimal;
    if (strstr(tmf, "tracer"))      bits |= LC_TMTracer;
    if (strstr(tmf, "recorder"))    bits |= LC_TMRecorder;
    if (strstr(tmf, "patcher"))     bits |= LC_TMPatcher;
    if (strstr(tmf, "abort"))       bits |= LC_TMAbort;
    if (strstr(tmf, "stats"))       bits |= LC_TMStats;
    if (strstr(tmf, "regexp"))      bits |= LC_TMRegexp;
    if (strstr(tmf, "treevis"))     bits |= LC_TMTreeVis;

    
    if (strstr(tmf, "liveness"))    bits |= LC_Liveness;
    if (strstr(tmf, "readlir"))     bits |= LC_ReadLIR;
    if (strstr(tmf, "aftersf_sp"))  bits |= LC_AfterSF_SP;
    if (strstr(tmf, "aftersf_rp"))  bits |= LC_AfterSF_RP;
    if (strstr(tmf, "regalloc"))    bits |= LC_RegAlloc;
    if (strstr(tmf, "assembly"))    bits |= LC_Assembly;
    if (strstr(tmf, "nocodeaddrs")) bits |= LC_NoCodeAddrs;

    if (strstr(tmf, "full")) {
        bits |= LC_TMMinimal | LC_TMTracer | LC_TMRecorder | LC_TMPatcher | LC_TMAbort |
                LC_TMAbort   | LC_TMStats  | LC_TMRegexp   | LC_Liveness  | LC_ReadLIR |
                LC_AfterSF_SP | LC_AfterSF_RP | LC_RegAlloc | LC_Assembly;
    }

    js_LogController.lcbits = bits;
    return;

   help:
    fflush(NULL);
    printf("\n");
    printf("Debug output control help summary for TraceMonkey:\n");
    printf("\n");
    printf("TRACEMONKEY= is no longer used; use TMFLAGS= "
           "instead.\n");
    printf("\n");
    printf("usage: TMFLAGS=option,option,option,... where options can be:\n");
    printf("   help         show this message\n");
    printf("   ------ options for jstracer & jsregexp ------\n");
    printf("   minimal      ultra-minimalist output; try this first\n");
    printf("   full         everything (old verbosity)\n");
    printf("   tracer       tracer lifetime (FIXME:better description)\n");
    printf("   recorder     trace recording stuff (FIXME:better description)\n");
    printf("   patcher      patching stuff (FIXME:better description)\n");
    printf("   abort        show trace recording aborts\n");
    printf("   stats        show trace recording stats\n");
    printf("   regexp       show compilation & entry for regexps\n");
    printf("   treevis      spew that tracevis/tree.py can parse\n");
    printf("   ------ options for Nanojit ------\n");
    printf("   liveness     show LIR liveness at start of rdr pipeline\n");
    printf("   readlir      show LIR as it enters the reader pipeline\n");
    printf("   aftersf_sp   show LIR after StackFilter(sp)\n");
    printf("   aftersf_rp   show LIR after StackFilter(rp)\n");
    printf("   regalloc     show regalloc details\n");
    printf("   assembly     show final aggregated assembly code\n");
    printf("   nocodeaddrs  don't show code addresses in assembly listings\n");
    printf("\n");
    printf("Exiting now.  Bye.\n");
    printf("\n");
    exit(0);
    
}
#endif

#if defined DEBUG
static const char*
getExitName(ExitType type)
{
    static const char* exitNames[] =
    {
    #define MAKE_EXIT_STRING(x) #x,
    JS_TM_EXITCODES(MAKE_EXIT_STRING)
    #undef MAKE_EXIT_STRING
    NULL
    };

    JS_ASSERT(type < TOTAL_EXIT_TYPES);

    return exitNames[type];
}
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

static inline jsuint
argSlots(JSStackFrame* fp)
{
    return JS_MAX(fp->argc, fp->fun->nargs);
}

static inline bool
isNumber(jsval v)
{
    return JSVAL_IS_INT(v) || JSVAL_IS_DOUBLE(v);
}

static inline jsdouble
asNumber(jsval v)
{
    JS_ASSERT(isNumber(v));
    if (JSVAL_IS_DOUBLE(v))
        return *JSVAL_TO_DOUBLE(v);
    return (jsdouble)JSVAL_TO_INT(v);
}

static inline bool
isInt32(jsval v)
{
    if (!isNumber(v))
        return false;
    jsdouble d = asNumber(v);
    jsint i;
    return JSDOUBLE_IS_INT(d, i);
}

static inline jsint
asInt32(jsval v)
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


static inline JSTraceType
GetPromotedType(jsval v)
{
    if (JSVAL_IS_INT(v))
        return TT_DOUBLE;
    if (JSVAL_IS_OBJECT(v)) {
        if (JSVAL_IS_NULL(v))
            return TT_NULL;
        if (HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v)))
            return TT_FUNCTION;
        return TT_OBJECT;
    }
    uint8_t tag = JSVAL_TAG(v);
    JS_ASSERT(tag == JSVAL_DOUBLE || tag == JSVAL_STRING || tag == JSVAL_SPECIAL);
    JS_STATIC_ASSERT(static_cast<jsvaltag>(TT_DOUBLE) == JSVAL_DOUBLE);
    JS_STATIC_ASSERT(static_cast<jsvaltag>(TT_STRING) == JSVAL_STRING);
    JS_STATIC_ASSERT(static_cast<jsvaltag>(TT_PSEUDOBOOLEAN) == JSVAL_SPECIAL);
    return JSTraceType(tag);
}


static inline JSTraceType
getCoercedType(jsval v)
{
    if (isInt32(v))
        return TT_INT32;
    if (JSVAL_IS_OBJECT(v)) {
        if (JSVAL_IS_NULL(v))
            return TT_NULL;
        if (HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v)))
            return TT_FUNCTION;
        return TT_OBJECT;
    }
    uint8_t tag = JSVAL_TAG(v);
    JS_ASSERT(tag == JSVAL_DOUBLE || tag == JSVAL_STRING || tag == JSVAL_SPECIAL);
    JS_STATIC_ASSERT(static_cast<jsvaltag>(TT_DOUBLE) == JSVAL_DOUBLE);
    JS_STATIC_ASSERT(static_cast<jsvaltag>(TT_STRING) == JSVAL_STRING);
    JS_STATIC_ASSERT(static_cast<jsvaltag>(TT_PSEUDOBOOLEAN) == JSVAL_SPECIAL);
    return JSTraceType(tag);
}



const uintptr_t ORACLE_MASK = ORACLE_SIZE - 1;
JS_STATIC_ASSERT((ORACLE_MASK & ORACLE_SIZE) == 0);

const uintptr_t FRAGMENT_TABLE_MASK = FRAGMENT_TABLE_SIZE - 1;
JS_STATIC_ASSERT((FRAGMENT_TABLE_MASK & FRAGMENT_TABLE_SIZE) == 0);

const uintptr_t HASH_SEED = 5381;

static inline void
HashAccum(uintptr_t& h, uintptr_t i, uintptr_t mask)
{
    h = ((h << 5) + h + (mask & i)) & mask;
}

static JS_REQUIRES_STACK inline int
StackSlotHash(JSContext* cx, unsigned slot)
{
    uintptr_t h = HASH_SEED;
    HashAccum(h, uintptr_t(cx->fp->script), ORACLE_MASK);
    HashAccum(h, uintptr_t(cx->fp->regs->pc), ORACLE_MASK);
    HashAccum(h, uintptr_t(slot), ORACLE_MASK);
    return int(h);
}

static JS_REQUIRES_STACK inline int
GlobalSlotHash(JSContext* cx, unsigned slot)
{
    uintptr_t h = HASH_SEED;
    JSStackFrame* fp = cx->fp;

    while (fp->down)
        fp = fp->down;

    HashAccum(h, uintptr_t(fp->script), ORACLE_MASK);
    HashAccum(h, uintptr_t(OBJ_SHAPE(JS_GetGlobalForObject(cx, fp->scopeChain))), ORACLE_MASK);
    HashAccum(h, uintptr_t(slot), ORACLE_MASK);
    return int(h);
}

static inline int
PCHash(jsbytecode* pc)
{
    return int(uintptr_t(pc) & ORACLE_MASK);
}

Oracle::Oracle()
{
    
    _stackDontDemote.set(ORACLE_SIZE-1);
    _globalDontDemote.set(ORACLE_SIZE-1);
    clear();
}


JS_REQUIRES_STACK void
Oracle::markGlobalSlotUndemotable(JSContext* cx, unsigned slot)
{
    _globalDontDemote.set(GlobalSlotHash(cx, slot));
}


JS_REQUIRES_STACK bool
Oracle::isGlobalSlotUndemotable(JSContext* cx, unsigned slot) const
{
    return _globalDontDemote.get(GlobalSlotHash(cx, slot));
}


JS_REQUIRES_STACK void
Oracle::markStackSlotUndemotable(JSContext* cx, unsigned slot)
{
    _stackDontDemote.set(StackSlotHash(cx, slot));
}


JS_REQUIRES_STACK bool
Oracle::isStackSlotUndemotable(JSContext* cx, unsigned slot) const
{
    return _stackDontDemote.get(StackSlotHash(cx, slot));
}


void
Oracle::markInstructionUndemotable(jsbytecode* pc)
{
    _pcDontDemote.set(PCHash(pc));
}


bool
Oracle::isInstructionUndemotable(jsbytecode* pc) const
{
    return _pcDontDemote.get(PCHash(pc));
}

void
Oracle::clearDemotability()
{
    _stackDontDemote.reset();
    _globalDontDemote.reset();
    _pcDontDemote.reset();
}

JS_REQUIRES_STACK static JS_INLINE void
MarkSlotUndemotable(JSContext* cx, TreeInfo* ti, unsigned slot)
{
    if (slot < ti->nStackTypes) {
        oracle.markStackSlotUndemotable(cx, slot);
        return;
    }

    uint16* gslots = ti->globalSlots->data();
    oracle.markGlobalSlotUndemotable(cx, gslots[slot - ti->nStackTypes]);
}

static JS_REQUIRES_STACK inline bool
IsSlotUndemotable(JSContext* cx, TreeInfo* ti, unsigned slot)
{
    if (slot < ti->nStackTypes)
        return oracle.isStackSlotUndemotable(cx, slot);

    uint16* gslots = ti->globalSlots->data();
    return oracle.isGlobalSlotUndemotable(cx, gslots[slot - ti->nStackTypes]);
}

struct PCHashEntry : public JSDHashEntryStub {
    size_t          count;
};

#define PC_HASH_COUNT 1024

static void
Blacklist(jsbytecode* pc)
{
    AUDIT(blacklisted);
    JS_ASSERT(*pc == JSOP_LOOP || *pc == JSOP_NOP);
    *pc = JSOP_NOP;
}

static void
Backoff(JSContext *cx, jsbytecode* pc, Fragment* tree = NULL)
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
                Blacklist(pc);
                return;
            }
        }
    }

    if (tree) {
        tree->hits() -= BL_BACKOFF;

        





        if (++tree->recordAttempts > BL_ATTEMPTS)
            Blacklist(pc);
    }
}

static void
ResetRecordingAttempts(JSContext *cx, jsbytecode* pc)
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
FragmentHash(const void *ip, JSObject* globalObj, uint32 globalShape, uint32 argc)
{
    uintptr_t h = HASH_SEED;
    HashAccum(h, uintptr_t(ip), FRAGMENT_TABLE_MASK);
    HashAccum(h, uintptr_t(globalObj), FRAGMENT_TABLE_MASK);
    HashAccum(h, uintptr_t(globalShape), FRAGMENT_TABLE_MASK);
    HashAccum(h, uintptr_t(argc), FRAGMENT_TABLE_MASK);
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
    inline TreeInfo* getTreeInfo() {
        return (TreeInfo*)vmprivate;
    }
    VMFragment* next;
    JSObject* globalObj;
    uint32 globalShape;
    uint32 argc;
};

static VMFragment*
getVMFragment(JSTraceMonitor* tm, const void *ip, JSObject* globalObj, uint32 globalShape,
              uint32 argc)
{
    size_t h = FragmentHash(ip, globalObj, globalShape, argc);
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
getLoop(JSTraceMonitor* tm, const void *ip, JSObject* globalObj, uint32 globalShape, uint32 argc)
{
    return getVMFragment(tm, ip, globalObj, globalShape, argc);
}

static Fragment*
getAnchor(JSTraceMonitor* tm, const void *ip, JSObject* globalObj, uint32 globalShape, uint32 argc)
{
    VMFragment *f = new (*tm->allocator) VMFragment(ip, globalObj, globalShape, argc);
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
        size_t h = FragmentHash(ip, globalObj, globalShape, argc);
        f->next = tm->vmfragments[h];
        tm->vmfragments[h] = f;
    }
    f->anchor = f;
    f->root = f;
    f->kind = LoopTrace;
    return f;
}

#ifdef DEBUG
static void
AssertTreeIsUnique(JSTraceMonitor* tm, VMFragment* f, TreeInfo* ti)
{
    JS_ASSERT(f->root == f);

    




    TreeInfo* ti_other;
    for (Fragment* peer = getLoop(tm, f->ip, f->globalObj, f->globalShape, f->argc);
         peer != NULL;
         peer = peer->peer) {
        if (!peer->code() || peer == f)
            continue;
        ti_other = (TreeInfo*)peer->vmprivate;
        JS_ASSERT(ti_other);
        JS_ASSERT(!ti->typeMap.matches(ti_other->typeMap));
    }
}
#endif

static void
AttemptCompilation(JSContext *cx, JSTraceMonitor* tm, JSObject* globalObj, jsbytecode* pc,
                   uint32 argc)
{
    
    JS_ASSERT(*(jsbytecode*)pc == JSOP_NOP || *(jsbytecode*)pc == JSOP_LOOP);
    *(jsbytecode*)pc = JSOP_LOOP;
    ResetRecordingAttempts(cx, pc);

    
    Fragment* f = (VMFragment*)getLoop(tm, pc, globalObj, OBJ_SHAPE(globalObj), argc);
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

static bool
isi2f(LIns* i)
{
    if (i->isop(LIR_i2f))
        return true;

    if (nanojit::AvmCore::config.soft_float &&
        i->isop(LIR_qjoin) &&
        i->oprnd1()->isop(LIR_call) &&
        i->oprnd2()->isop(LIR_callh)) {
        if (i->oprnd1()->callInfo() == &i2f_ci)
            return true;
    }

    return false;
}

static bool
isu2f(LIns* i)
{
    if (i->isop(LIR_u2f))
        return true;

    if (nanojit::AvmCore::config.soft_float &&
        i->isop(LIR_qjoin) &&
        i->oprnd1()->isop(LIR_call) &&
        i->oprnd2()->isop(LIR_callh)) {
        if (i->oprnd1()->callInfo() == &u2f_ci)
            return true;
    }

    return false;
}

static LIns*
iu2fArg(LIns* i)
{
    if (nanojit::AvmCore::config.soft_float &&
        i->isop(LIR_qjoin)) {
        return i->oprnd1()->arg(0);
    }

    return i->oprnd1();
}

static LIns*
demote(LirWriter *out, LIns* i)
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

static bool
isPromoteInt(LIns* i)
{
    if (isi2f(i) || i->isconst())
        return true;
    if (!i->isconstq())
        return false;
    jsdouble d = i->imm64f();
    return d == jsdouble(jsint(d)) && !JSDOUBLE_IS_NEGZERO(d);
}

static bool
isPromoteUint(LIns* i)
{
    if (isu2f(i) || i->isconst())
        return true;
    if (!i->isconstq())
        return false;
    jsdouble d = i->imm64f();
    return d == jsdouble(jsuint(d)) && !JSDOUBLE_IS_NEGZERO(d);
}

static bool
isPromote(LIns* i)
{
    return isPromoteInt(i) || isPromoteUint(i);
}

static bool
IsConst(LIns* i, int32_t c)
{
    return i->isconst() && i->imm32() == c;
}





static bool
IsOverflowSafe(LOpcode op, LIns* i)
{
    LIns* c;
    switch (op) {
      case LIR_add:
      case LIR_sub:
          return (i->isop(LIR_and) && ((c = i->oprnd2())->isconst()) &&
                  ((c->imm32() & 0xc0000000) == 0)) ||
                 (i->isop(LIR_rsh) && ((c = i->oprnd2())->isconst()) &&
                  ((c->imm32() > 0)));
    default:
        JS_ASSERT(op == LIR_mul);
    }
    return (i->isop(LIR_and) && ((c = i->oprnd2())->isconst()) &&
            ((c->imm32() & 0xffff0000) == 0)) ||
           (i->isop(LIR_ush) && ((c = i->oprnd2())->isconst()) &&
            ((c->imm32() >= 16)));
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

    LIns* quadCall(const CallInfo *ci, LIns* args[]) {
        LInsp qlo, qhi;

        qlo = out->insCall(ci, args);
        qhi = out->ins1(LIR_callh, qlo);
        return out->qjoin(qlo, qhi);
    }

    LIns* ins1(LOpcode v, LIns* s0)
    {
        if (v == LIR_fneg)
            return quadCall(&fneg_ci, &s0);

        if (v == LIR_i2f)
            return quadCall(&i2f_ci, &s0);

        if (v == LIR_u2f)
            return quadCall(&u2f_ci, &s0);

        return out->ins1(v, s0);
    }

    LIns* ins2(LOpcode v, LIns* s0, LIns* s1)
    {
        LIns* args[2];
        LIns* bv;

        
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

    LIns* insCall(const CallInfo *ci, LIns* args[])
    {
        
        
        if ((ci->_argtypes & ARGSIZE_MASK_ANY) == ARGSIZE_F)
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

    LIns* ins2(LOpcode v, LIns* s0, LIns* s1)
    {
        if (s0 == s1 && v == LIR_feq) {
            if (isPromote(s0)) {
                
                return insImm(1);
            }
            if (s0->isop(LIR_fmul) || s0->isop(LIR_fsub) || s0->isop(LIR_fadd)) {
                LIns* lhs = s0->oprnd1();
                LIns* rhs = s0->oprnd2();
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
                   s0->isop(LIR_lsh) && IsConst(s0->oprnd2(), 16) &&
                   s1->isop(LIR_and) && IsConst(s1->oprnd2(), 0xffff)) {
            LIns* msw = s0->oprnd1();
            LIns* lsw = s1->oprnd1();
            LIns* x;
            LIns* y;
            if (lsw->isop(LIR_add) &&
                lsw->oprnd1()->isop(LIR_and) &&
                lsw->oprnd2()->isop(LIR_and) &&
                IsConst(lsw->oprnd1()->oprnd2(), 0xffff) &&
                IsConst(lsw->oprnd2()->oprnd2(), 0xffff) &&
                msw->isop(LIR_add) &&
                msw->oprnd1()->isop(LIR_add) &&
                msw->oprnd2()->isop(LIR_rsh) &&
                msw->oprnd1()->oprnd1()->isop(LIR_rsh) &&
                msw->oprnd1()->oprnd2()->isop(LIR_rsh) &&
                IsConst(msw->oprnd2()->oprnd2(), 16) &&
                IsConst(msw->oprnd1()->oprnd1()->oprnd2(), 16) &&
                IsConst(msw->oprnd1()->oprnd2()->oprnd2(), 16) &&
                (x = lsw->oprnd1()->oprnd1()) == msw->oprnd1()->oprnd1()->oprnd1() &&
                (y = lsw->oprnd2()->oprnd1()) == msw->oprnd1()->oprnd2()->oprnd1() &&
                lsw == msw->oprnd2()->oprnd1()) {
                return out->ins2(LIR_add, x, y);
            }
        }

        return out->ins2(v, s0, s1);
    }

    LIns* insCall(const CallInfo *ci, LIns* args[])
    {
        if (ci == &js_DoubleToUint32_ci) {
            LIns* s0 = args[0];
            if (s0->isconstq())
                return out->insImm(js_DoubleToECMAUint32(s0->imm64f()));
            if (isi2f(s0) || isu2f(s0))
                return iu2fArg(s0);
        } else if (ci == &js_DoubleToInt32_ci) {
            LIns* s0 = args[0];
            if (s0->isconstq())
                return out->insImm(js_DoubleToECMAInt32(s0->imm64f()));
            if (s0->isop(LIR_fadd) || s0->isop(LIR_fsub)) {
                LIns* lhs = s0->oprnd1();
                LIns* rhs = s0->oprnd2();
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
            LIns* s0 = args[0];
            JS_ASSERT(s0->isQuad());
            if (isPromoteInt(s0)) {
                LIns* args2[] = { demote(out, s0), args[1] };
                return out->insCall(&js_BoxInt32_ci, args2);
            }
            if (s0->isCall() && s0->callInfo() == &js_UnboxDouble_ci)
                return callArgN(s0, 0);
        }
        return out->insCall(ci, args);
    }
};









template <typename Visitor>
static JS_REQUIRES_STACK bool
VisitFrameSlots(Visitor &visitor, unsigned depth, JSStackFrame *fp,
                JSStackFrame *up)
{
    if (depth > 0 && !VisitFrameSlots(visitor, depth-1, fp->down, fp))
        return false;

    if (fp->argv) {
        if (depth == 0) {
            visitor.setStackSlotKind("args");
            if (!visitor.visitStackSlots(&fp->argv[-2], argSlots(fp) + 2, fp))
                return false;
        }
        visitor.setStackSlotKind("arguments");
        if (!visitor.visitStackSlots(&fp->argsobj, 1, fp))
            return false;
        visitor.setStackSlotKind("var");
        if (!visitor.visitStackSlots(fp->slots, fp->script->nfixed, fp))
            return false;
    }
    visitor.setStackSlotKind("stack");
    JS_ASSERT(fp->regs->sp >= StackBase(fp));
    if (!visitor.visitStackSlots(StackBase(fp),
                                 size_t(fp->regs->sp - StackBase(fp)),
                                 fp)) {
        return false;
    }
    if (up) {
        int missing = up->fun->nargs - up->argc;
        if (missing > 0) {
            visitor.setStackSlotKind("missing");
            if (!visitor.visitStackSlots(fp->regs->sp, size_t(missing), fp))
                return false;
        }
    }
    return true;
}

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
VisitStackSlots(Visitor &visitor, JSContext *cx, unsigned callDepth)
{
    return VisitFrameSlots(visitor, callDepth, cx->fp, NULL);
}

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE void
VisitGlobalSlots(Visitor &visitor, JSContext *cx, JSObject *globalObj,
                 unsigned ngslots, uint16 *gslots)
{
    for (unsigned n = 0; n < ngslots; ++n) {
        unsigned slot = gslots[n];
        visitor.visitGlobalSlot(&STOBJ_GET_SLOT(globalObj, slot), n, slot);
    }
}

class AdjustCallerTypeVisitor;

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE void
VisitGlobalSlots(Visitor &visitor, JSContext *cx, SlotList &gslots)
{
    VisitGlobalSlots(visitor, cx, JS_GetGlobalForObject(cx, cx->fp->scopeChain),
                     gslots.length(), gslots.data());
}


template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE void
VisitSlots(Visitor& visitor, JSContext* cx, JSObject* globalObj,
           unsigned callDepth, unsigned ngslots, uint16* gslots)
{
    if (VisitStackSlots(visitor, cx, callDepth))
        VisitGlobalSlots(visitor, cx, globalObj, ngslots, gslots);
}

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE void
VisitSlots(Visitor& visitor, JSContext* cx, unsigned callDepth,
           unsigned ngslots, uint16* gslots)
{
    VisitSlots(visitor, cx, JS_GetGlobalForObject(cx, cx->fp->scopeChain),
               callDepth, ngslots, gslots);
}

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE void
VisitSlots(Visitor &visitor, JSContext *cx, JSObject *globalObj,
           unsigned callDepth, const SlotList& slots)
{
    VisitSlots(visitor, cx, globalObj, callDepth, slots.length(),
               slots.data());
}

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE void
VisitSlots(Visitor &visitor, JSContext *cx, unsigned callDepth,
           const SlotList& slots)
{
    VisitSlots(visitor, cx, JS_GetGlobalForObject(cx, cx->fp->scopeChain),
               callDepth, slots.length(), slots.data());
}


class SlotVisitorBase {
#ifdef JS_JIT_SPEW
protected:
    char const *mStackSlotKind;
public:
    SlotVisitorBase() : mStackSlotKind(NULL) {}
    JS_ALWAYS_INLINE const char *stackSlotKind() { return mStackSlotKind; }
    JS_ALWAYS_INLINE void setStackSlotKind(char const *k) {
        mStackSlotKind = k;
    }
#else
public:
    JS_ALWAYS_INLINE const char *stackSlotKind() { return NULL; }
    JS_ALWAYS_INLINE void setStackSlotKind(char const *k) {}
#endif
};

struct CountSlotsVisitor : public SlotVisitorBase
{
    unsigned mCount;
    bool mDone;
    jsval* mStop;
public:
    JS_ALWAYS_INLINE CountSlotsVisitor(jsval* stop = NULL) :
        mCount(0),
        mDone(false),
        mStop(stop)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, size_t count, JSStackFrame* fp) {
        if (mDone)
            return false;
        if (mStop && size_t(mStop - vp) < count) {
            mCount += size_t(mStop - vp);
            mDone = true;
            return false;
        }
        mCount += count;
        return true;
    }

    JS_ALWAYS_INLINE unsigned count() {
        return mCount;
    }

    JS_ALWAYS_INLINE bool stopped() {
        return mDone;
    }
};





JS_REQUIRES_STACK unsigned
NativeStackSlots(JSContext *cx, unsigned callDepth)
{
    JSStackFrame* fp = cx->fp;
    unsigned slots = 0;
    unsigned depth = callDepth;
    for (;;) {
        



        unsigned operands = fp->regs->sp - StackBase(fp);
        slots += operands;
        if (fp->argv)
            slots += fp->script->nfixed + 1 ;
        if (depth-- == 0) {
            if (fp->argv)
                slots += 2 + argSlots(fp);
#ifdef DEBUG
            CountSlotsVisitor visitor;
            VisitStackSlots(visitor, cx, callDepth);
            JS_ASSERT(visitor.count() == slots && !visitor.stopped());
#endif
            return slots;
        }
        JSStackFrame* fp2 = fp;
        fp = fp->down;
        int missing = fp2->fun->nargs - fp2->argc;
        if (missing > 0)
            slots += missing;
    }
    JS_NOT_REACHED("NativeStackSlots");
}

class CaptureTypesVisitor : public SlotVisitorBase
{
    JSContext* mCx;
    JSTraceType* mTypeMap;
    JSTraceType* mPtr;

public:
    JS_ALWAYS_INLINE CaptureTypesVisitor(JSContext* cx, JSTraceType* typeMap) :
        mCx(cx),
        mTypeMap(typeMap),
        mPtr(typeMap)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(jsval *vp, unsigned n, unsigned slot) {
            JSTraceType type = getCoercedType(*vp);
            if (type == TT_INT32 &&
                oracle.isGlobalSlotUndemotable(mCx, slot))
                type = TT_DOUBLE;
            JS_ASSERT(type != TT_JSVAL);
            debug_only_printf(LC_TMTracer,
                              "capture type global%d: %d=%c\n",
                              n, type, typeChar[type]);
            *mPtr++ = type;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, int count, JSStackFrame* fp) {
        for (int i = 0; i < count; ++i) {
            JSTraceType type = getCoercedType(vp[i]);
            if (type == TT_INT32 &&
                oracle.isStackSlotUndemotable(mCx, length()))
                type = TT_DOUBLE;
            JS_ASSERT(type != TT_JSVAL);
            debug_only_printf(LC_TMTracer,
                              "capture type %s%d: %d=%c\n",
                              stackSlotKind(), i, type, typeChar[type]);
            *mPtr++ = type;
        }
        return true;
    }

    JS_ALWAYS_INLINE uintptr_t length() {
        return mPtr - mTypeMap;
    }
};





JS_REQUIRES_STACK void
TypeMap::captureTypes(JSContext* cx, JSObject* globalObj, SlotList& slots, unsigned callDepth)
{
    setLength(NativeStackSlots(cx, callDepth) + slots.length());
    CaptureTypesVisitor visitor(cx, data());
    VisitSlots(visitor, cx, globalObj, callDepth, slots);
    JS_ASSERT(visitor.length() == length());
}

JS_REQUIRES_STACK void
TypeMap::captureMissingGlobalTypes(JSContext* cx, JSObject* globalObj, SlotList& slots, unsigned stackSlots)
{
    unsigned oldSlots = length() - stackSlots;
    int diff = slots.length() - oldSlots;
    JS_ASSERT(diff >= 0);
    setLength(length() + diff);
    CaptureTypesVisitor visitor(cx, data() + stackSlots + oldSlots);
    VisitGlobalSlots(visitor, cx, globalObj, diff, slots.data() + oldSlots);
}


bool
TypeMap::matches(TypeMap& other) const
{
    if (length() != other.length())
        return false;
    return !memcmp(data(), other.data(), length());
}

void
TypeMap::fromRaw(JSTraceType* other, unsigned numSlots)
{
    unsigned oldLength = length();
    setLength(length() + numSlots);
    for (unsigned i = 0; i < numSlots; i++)
        get(oldLength + i) = other[i];
}






static void
MergeTypeMaps(JSTraceType** partial, unsigned* plength, JSTraceType* complete, unsigned clength, JSTraceType* mem)
{
    unsigned l = *plength;
    JS_ASSERT(l < clength);
    memcpy(mem, *partial, l * sizeof(JSTraceType));
    memcpy(mem + l, complete + l, (clength - l) * sizeof(JSTraceType));
    *partial = mem;
    *plength = clength;
}


static JS_REQUIRES_STACK void
SpecializeTreesToMissingGlobals(JSContext* cx, JSObject* globalObj, TreeInfo* root)
{
    TreeInfo* ti = root;

    ti->typeMap.captureMissingGlobalTypes(cx, globalObj, *ti->globalSlots, ti->nStackTypes);
    JS_ASSERT(ti->globalSlots->length() == ti->typeMap.length() - ti->nStackTypes);

    for (unsigned i = 0; i < root->dependentTrees.length(); i++) {
        ti = (TreeInfo*)root->dependentTrees[i]->vmprivate;

        
        if (ti && ti->nGlobalTypes() < ti->globalSlots->length())
            SpecializeTreesToMissingGlobals(cx, globalObj, ti);
    }
    for (unsigned i = 0; i < root->linkedTrees.length(); i++) {
        ti = (TreeInfo*)root->linkedTrees[i]->vmprivate;
        if (ti && ti->nGlobalTypes() < ti->globalSlots->length())
            SpecializeTreesToMissingGlobals(cx, globalObj, ti);
    }
}

static void
TrashTree(JSContext* cx, Fragment* f);

JS_REQUIRES_STACK
TraceRecorder::TraceRecorder(JSContext* cx, VMSideExit* _anchor, Fragment* _fragment,
        TreeInfo* ti, unsigned stackSlots, unsigned ngslots, JSTraceType* typeMap,
        VMSideExit* innermostNestedGuard, jsbytecode* outer, uint32 outerArgc)
    : whichTreesToTrash(JS_TRACE_MONITOR(cx).allocator),
      cfgMerges(JS_TRACE_MONITOR(cx).allocator)
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

#ifdef JS_JIT_SPEW
    debug_only_print0(LC_TMMinimal, "\n");
    debug_only_printf(LC_TMMinimal, "Recording starting from %s:%u@%u\n",
                      ti->treeFileName, ti->treeLineNumber, ti->treePCOffset);

    debug_only_printf(LC_TMTracer, "globalObj=%p, shape=%d\n",
                      (void*)this->globalObj, OBJ_SHAPE(this->globalObj));
    debug_only_printf(LC_TMTreeVis, "TREEVIS RECORD FRAG=%p ANCHOR=%p\n", (void*)fragment,
                      (void*)anchor);

    

    jitstats.archIsIA32 = 0;
    jitstats.archIsAMD64 = 0;
    jitstats.archIs64BIT = 0;
    jitstats.archIsARM = 0;
    jitstats.archIsSPARC = 0;
    jitstats.archIsPPC = 0;
#if defined NANOJIT_IA32
    jitstats.archIsIA32 = 1;
#endif
#if defined NANOJIT_ARM64
    jitstats.archIsAMD64 = 1;
#endif
#if defined NANOJIT_64BIT
    jitstats.archIs64BIT = 1;
#endif
#if defined NANOJIT_ARM
    jitstats.archIsARM = 1;
#endif
#if defined NANOJIT_SPARC
    jitstats.archIsSPARC = 1;
#endif
#if defined NANOJIT_PPC
    jitstats.archIsPPC = 1;
#endif

#endif

    lir = lir_buf_writer = new (&gc) LirBufWriter(lirbuf);
    debug_only_stmt(
        if (js_LogController.lcbits & LC_TMRecorder) {
           lir = verbose_filter
               = new (&gc) VerboseWriter(*traceMonitor->allocator, lir,
                                         lirbuf->names, &js_LogController);
        }
    )
    if (nanojit::AvmCore::config.soft_float)
        lir = float_filter = new (&gc) SoftFloatFilter(lir);
    else
        float_filter = 0;
    lir = cse_filter = new (&gc) CseFilter(lir, *traceMonitor->allocator);
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
        SpecializeTreesToMissingGlobals(cx, globalObj, ti);

    
    import(treeInfo, lirbuf->sp, stackSlots, ngslots, callDepth, typeMap);

    if (fragment == fragment->root) {
        



        LIns* x = lir->insLoad(LIR_ld, cx_ins, offsetof(JSContext, operationCallbackFlag));
        guard(true, lir->ins_eq0(x), snapshot(TIMEOUT_EXIT));
    }

    



    if (_anchor && _anchor->exitType == NESTED_EXIT) {
        LIns* nested_ins = addName(lir->insLoad(LIR_ldp, lirbuf->state,
                                                offsetof(InterpState, lastTreeExitGuard)),
                                                "lastTreeExitGuard");
        guard(true, lir->ins2(LIR_eq, nested_ins, INS_CONSTPTR(innermostNestedGuard)), NESTED_EXIT);
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

        if (trashSelf)
            TrashTree(cx, fragment->root);

        for (unsigned int i = 0; i < whichTreesToTrash.length(); i++)
            TrashTree(cx, whichTreesToTrash[i]);
    }
#ifdef DEBUG
    debug_only_stmt( delete verbose_filter; )
#endif
    delete cse_filter;
    delete expr_filter;
    delete func_filter;
    delete float_filter;
    delete lir_buf_writer;
    delete generatedTraceableNative;
}

void
TraceRecorder::removeFragmentReferences()
{
    fragment = NULL;
}

void
TraceRecorder::deepAbort()
{
    debug_only_print0(LC_TMTracer|LC_TMAbort, "deep abort");
    deepAborted = true;
}


inline LIns*
TraceRecorder::addName(LIns* ins, const char* name)
{
#ifdef JS_JIT_SPEW
    



    if (js_LogController.lcbits > 0)
        lirbuf->names->addName(ins, name);
#endif
    return ins;
}

inline LIns*
TraceRecorder::insImmObj(JSObject* obj)
{
    treeInfo->gcthings.addUnique(OBJECT_TO_JSVAL(obj));
    return lir->insImmPtr((void*)obj);
}

inline LIns*
TraceRecorder::insImmFun(JSFunction* fun)
{
    treeInfo->gcthings.addUnique(OBJECT_TO_JSVAL(FUN_OBJECT(fun)));
    return lir->insImmPtr((void*)fun);
}

inline LIns*
TraceRecorder::insImmStr(JSString* str)
{
    treeInfo->gcthings.addUnique(STRING_TO_JSVAL(str));
    return lir->insImmPtr((void*)str);
}

inline LIns*
TraceRecorder::insImmSprop(JSScopeProperty* sprop)
{
    treeInfo->sprops.addUnique(sprop);
    return lir->insImmPtr((void*)sprop);
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
    CountSlotsVisitor visitor(p);
    VisitStackSlots(visitor, cx, callDepth);
    size_t offset = visitor.count() * sizeof(double);

    



    if (!visitor.stopped()) {
        JS_ASSERT(size_t(p - cx->fp->slots) < cx->fp->script->nslots);
        offset += size_t(p - cx->fp->regs->sp) * sizeof(double);
    }
    return offset;
}


void
TraceRecorder::trackNativeStackUse(unsigned slots)
{
    if (slots > treeInfo->maxNativeStackSlots)
        treeInfo->maxNativeStackSlots = slots;
}






static void
ValueToNative(JSContext* cx, jsval v, JSTraceType type, double* slot)
{
    uint8_t tag = JSVAL_TAG(v);
    switch (type) {
      case TT_OBJECT:
        JS_ASSERT(tag == JSVAL_OBJECT);
        JS_ASSERT(!JSVAL_IS_NULL(v) && !HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(v)));
        *(JSObject**)slot = JSVAL_TO_OBJECT(v);
        debug_only_printf(LC_TMTracer,
                          "object<%p:%s> ", (void*)JSVAL_TO_OBJECT(v),
                          JSVAL_IS_NULL(v)
                          ? "null"
                          : STOBJ_GET_CLASS(JSVAL_TO_OBJECT(v))->name);
        return;

      case TT_INT32:
        jsint i;
        if (JSVAL_IS_INT(v))
            *(jsint*)slot = JSVAL_TO_INT(v);
        else if (tag == JSVAL_DOUBLE && JSDOUBLE_IS_INT(*JSVAL_TO_DOUBLE(v), i))
            *(jsint*)slot = i;
        else
            JS_ASSERT(JSVAL_IS_INT(v));
        debug_only_printf(LC_TMTracer, "int<%d> ", *(jsint*)slot);
        return;

      case TT_DOUBLE:
        jsdouble d;
        if (JSVAL_IS_INT(v))
            d = JSVAL_TO_INT(v);
        else
            d = *JSVAL_TO_DOUBLE(v);
        JS_ASSERT(JSVAL_IS_INT(v) || JSVAL_IS_DOUBLE(v));
        *(jsdouble*)slot = d;
        debug_only_printf(LC_TMTracer, "double<%g> ", d);
        return;

      case TT_JSVAL:
        JS_NOT_REACHED("found jsval type in an entry type map");
        return;

      case TT_STRING:
        JS_ASSERT(tag == JSVAL_STRING);
        *(JSString**)slot = JSVAL_TO_STRING(v);
        debug_only_printf(LC_TMTracer, "string<%p> ", (void*)(*(JSString**)slot));
        return;

      case TT_NULL:
        JS_ASSERT(tag == JSVAL_OBJECT);
        *(JSObject**)slot = NULL;
        debug_only_print0(LC_TMTracer, "null ");
        return;

      case TT_PSEUDOBOOLEAN:
        
        JS_ASSERT(tag == JSVAL_SPECIAL);
        *(JSBool*)slot = JSVAL_TO_SPECIAL(v);
        debug_only_printf(LC_TMTracer, "pseudoboolean<%d> ", *(JSBool*)slot);
        return;

      case TT_FUNCTION: {
        JS_ASSERT(tag == JSVAL_OBJECT);
        JSObject* obj = JSVAL_TO_OBJECT(v);
        *(JSObject**)slot = obj;
#ifdef DEBUG
        JSFunction* fun = GET_FUNCTION_PRIVATE(cx, obj);
        debug_only_printf(LC_TMTracer,
                          "function<%p:%s> ", (void*) obj,
                          fun->atom
                          ? JS_GetStringBytes(ATOM_TO_STRING(fun->atom))
                          : "unnamed");
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
ReplenishReservedPool(JSContext* cx, JSTraceMonitor* tm)
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

void
JSTraceMonitor::flush()
{
    memset(&vmfragments[0], 0,
           FRAGMENT_TABLE_SIZE * sizeof(VMFragment*));

    allocator->reset();
    delete codeAlloc;
    codeAlloc = new CodeAlloc();

    Allocator& alloc = *allocator;

    for (size_t i = 0; i < MONITOR_N_GLOBAL_STATES; ++i) {
        globalStates[i].globalShape = -1;
        globalStates[i].globalSlots = new (alloc) SlotList(allocator);
    }

    assembler = new (alloc) Assembler(*codeAlloc, alloc, core,
                                           &js_LogController);
    lirbuf = new (alloc) LirBuffer(alloc);

#ifdef DEBUG
    JS_ASSERT(labels);
    labels = new (alloc) LabelMap(alloc, &js_LogController);
    lirbuf->names = new (alloc) LirNameMap(alloc, labels);
#endif

    needFlush = JS_FALSE;
}

void
JSTraceMonitor::mark(JSTracer* trc)
{
    if (!trc->context->runtime->gcFlushCodeCaches) {
        for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
            VMFragment* f = vmfragments[i];
            while (f) {
                TreeInfo* ti = (TreeInfo*)f->vmprivate;
                if (ti) {
                    jsval* vp = ti->gcthings.data();
                    unsigned len = ti->gcthings.length();
                    while (len--) {
                        jsval v = *vp++;
                        JS_SET_TRACING_NAME(trc, "jitgcthing");
                        JS_CallTracer(trc, JSVAL_TO_TRACEABLE(v), JSVAL_TRACE_KIND(v));
                    }
                    JSScopeProperty** spropp = ti->sprops.data();
                    len = ti->sprops.length();
                    while (len--) {
                        JSScopeProperty* sprop = *spropp++;
                        sprop->trace(trc);
                    }
                }
                f = f->next;
            }
        }
    } else {
        flush();
    }
}






static void
NativeToValue(JSContext* cx, jsval& v, JSTraceType type, double* slot)
{
    jsint i;
    jsdouble d;
    switch (type) {
      case TT_OBJECT:
        v = OBJECT_TO_JSVAL(*(JSObject**)slot);
        JS_ASSERT(v != JSVAL_ERROR_COOKIE); 
        debug_only_printf(LC_TMTracer,
                          "object<%p:%s> ", (void*)JSVAL_TO_OBJECT(v),
                          JSVAL_IS_NULL(v)
                          ? "null"
                          : STOBJ_GET_CLASS(JSVAL_TO_OBJECT(v))->name);
        break;

      case TT_INT32:
        i = *(jsint*)slot;
        debug_only_printf(LC_TMTracer, "int<%d> ", i);
      store_int:
        if (INT_FITS_IN_JSVAL(i)) {
            v = INT_TO_JSVAL(i);
            break;
        }
        d = (jsdouble)i;
        goto store_double;
      case TT_DOUBLE:
        d = *slot;
        debug_only_printf(LC_TMTracer, "double<%g> ", d);
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

      case TT_JSVAL:
        v = *(jsval*)slot;
        JS_ASSERT(v != JSVAL_ERROR_COOKIE); 
        debug_only_printf(LC_TMTracer, "box<%p> ", (void*)v);
        break;

      case TT_STRING:
        v = STRING_TO_JSVAL(*(JSString**)slot);
        debug_only_printf(LC_TMTracer, "string<%p> ", (void*)(*(JSString**)slot));
        break;

      case TT_NULL:
        JS_ASSERT(*(JSObject**)slot == NULL);
        v = JSVAL_NULL;
        debug_only_printf(LC_TMTracer, "null<%p> ", (void*)(*(JSObject**)slot));
        break;

      case TT_PSEUDOBOOLEAN:
        
        v = SPECIAL_TO_JSVAL(*(JSBool*)slot);
        debug_only_printf(LC_TMTracer, "boolean<%d> ", *(JSBool*)slot);
        break;

      case TT_FUNCTION: {
        JS_ASSERT(HAS_FUNCTION_CLASS(*(JSObject**)slot));
        v = OBJECT_TO_JSVAL(*(JSObject**)slot);
#ifdef DEBUG
        JSFunction* fun = GET_FUNCTION_PRIVATE(cx, JSVAL_TO_OBJECT(v));
        debug_only_printf(LC_TMTracer,
                          "function<%p:%s> ", (void*)JSVAL_TO_OBJECT(v),
                          fun->atom
                          ? JS_GetStringBytes(ATOM_TO_STRING(fun->atom))
                          : "unnamed");
#endif
        break;
      }
    }
}

class BuildNativeFrameVisitor : public SlotVisitorBase
{
    JSContext *mCx;
    JSTraceType *mTypeMap;
    double *mGlobal;
    double *mStack;
public:
    BuildNativeFrameVisitor(JSContext *cx,
                            JSTraceType *typemap,
                            double *global,
                            double *stack) :
        mCx(cx),
        mTypeMap(typemap),
        mGlobal(global),
        mStack(stack)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(jsval *vp, unsigned n, unsigned slot) {
        debug_only_printf(LC_TMTracer, "global%d: ", n);
        ValueToNative(mCx, *vp, *mTypeMap++, &mGlobal[slot]);
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, int count, JSStackFrame* fp) {
        for (int i = 0; i < count; ++i) {
            debug_only_printf(LC_TMTracer, "%s%d: ", stackSlotKind(), i);
            ValueToNative(mCx, *vp++, *mTypeMap++, mStack++);
        }
        return true;
    }
};

static JS_REQUIRES_STACK void
BuildNativeFrame(JSContext *cx, JSObject *globalObj, unsigned callDepth,
                 unsigned ngslots, uint16 *gslots,
                 JSTraceType *typeMap, double *global, double *stack)
{
    BuildNativeFrameVisitor visitor(cx, typeMap, global, stack);
    VisitSlots(visitor, cx, globalObj, callDepth, ngslots, gslots);
    debug_only_print0(LC_TMTracer, "\n");
}

class FlushNativeGlobalFrameVisitor : public SlotVisitorBase
{
    JSContext *mCx;
    JSTraceType *mTypeMap;
    double *mGlobal;
public:
    FlushNativeGlobalFrameVisitor(JSContext *cx,
                                  JSTraceType *typeMap,
                                  double *global) :
        mCx(cx),
        mTypeMap(typeMap),
        mGlobal(global)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(jsval *vp, unsigned n, unsigned slot) {
        debug_only_printf(LC_TMTracer, "global%d=", n);
        NativeToValue(mCx, *vp, *mTypeMap++, &mGlobal[slot]);
    }
};

class FlushNativeStackFrameVisitor : public SlotVisitorBase
{
    JSContext *mCx;
    JSTraceType *mTypeMap;
    double *mStack;
    jsval *mStop;
public:
    FlushNativeStackFrameVisitor(JSContext *cx,
                                 JSTraceType *typeMap,
                                 double *stack,
                                 jsval *stop) :
        mCx(cx),
        mTypeMap(typeMap),
        mStack(stack),
        mStop(stop)
    {}

    JSTraceType* getTypeMap()
    {
        return mTypeMap;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            if (vp == mStop)
                return false;
            debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), unsigned(i));
            NativeToValue(mCx, *vp++, *mTypeMap++, mStack++);
        }
        return true;
    }
};


static JS_REQUIRES_STACK void
FlushNativeGlobalFrame(JSContext *cx, double *global, unsigned ngslots,
                       uint16 *gslots, JSTraceType *typemap)
{
    FlushNativeGlobalFrameVisitor visitor(cx, typemap, global);
    JSObject *globalObj = JS_GetGlobalForObject(cx, cx->fp->scopeChain);
    VisitGlobalSlots(visitor, cx, globalObj, ngslots, gslots);
    debug_only_print0(LC_TMTracer, "\n");
}






static int32
StackDepthFromCallStack(InterpState* state, uint32 callDepth)
{
    int32 nativeStackFramePos = 0;

    
    for (FrameInfo** fip = state->callstackBase; fip < state->rp + callDepth; fip++)
        nativeStackFramePos += (*fip)->callerHeight;
    return nativeStackFramePos;
}












template<typename T>
inline JSTraceType
GetUpvarOnTrace(JSContext* cx, uint32 upvarLevel, int32 slot, uint32 callDepth, double* result)
{
    InterpState* state = cx->interpState;
    FrameInfo** fip = state->rp + callDepth;

    








    int32 stackOffset = StackDepthFromCallStack(state, callDepth);
    while (--fip > state->callstackBase) {
        FrameInfo* fi = *fip;

        



        stackOffset -= fi->callerHeight;
        JSObject* callee = *(JSObject**)(&state->stackBase[stackOffset]);
        JSFunction* fun = GET_FUNCTION_PRIVATE(cx, callee);
        uintN calleeLevel = fun->u.i.script->staticLevel;
        if (calleeLevel == upvarLevel) {
            




            uint32 native_slot = T::native_slot(fi->callerArgc, slot);
            *result = state->stackBase[stackOffset + native_slot];
            return fi->get_typemap()[native_slot];
        }
    }

    
    if (state->outermostTree->script->staticLevel == upvarLevel) {
        uint32 argc = ((VMFragment*) state->outermostTree->fragment)->argc;
        uint32 native_slot = T::native_slot(argc, slot);
        *result = state->stackBase[native_slot];
        return state->callstackBase[0]->get_typemap()[native_slot];
    }

    



    JS_ASSERT(upvarLevel < JS_DISPLAY_SIZE);
    JSStackFrame* fp = cx->display[upvarLevel];
    jsval v = T::interp_get(fp, slot);
    JSTraceType type = getCoercedType(v);
    ValueToNative(cx, v, type, result);
    return type;
}


struct UpvarArgTraits {
    static jsval interp_get(JSStackFrame* fp, int32 slot) {
        return fp->argv[slot];
    }

    static uint32 native_slot(uint32 argc, int32 slot) {
        return 2  + slot;
    }
};

uint32 JS_FASTCALL
GetUpvarArgOnTrace(JSContext* cx, uint32 upvarLevel, int32 slot, uint32 callDepth, double* result)
{
    return GetUpvarOnTrace<UpvarArgTraits>(cx, upvarLevel, slot, callDepth, result);
}


struct UpvarVarTraits {
    static jsval interp_get(JSStackFrame* fp, int32 slot) {
        return fp->slots[slot];
    }

    static uint32 native_slot(uint32 argc, int32 slot) {
        return 3  + argc + slot;
    }
};

uint32 JS_FASTCALL
GetUpvarVarOnTrace(JSContext* cx, uint32 upvarLevel, int32 slot, uint32 callDepth, double* result)
{
    return GetUpvarOnTrace<UpvarVarTraits>(cx, upvarLevel, slot, callDepth, result);
}






struct UpvarStackTraits {
    static jsval interp_get(JSStackFrame* fp, int32 slot) {
        return fp->slots[slot + fp->script->nfixed];
    }

    static uint32 native_slot(uint32 argc, int32 slot) {
        



        JS_ASSERT(argc == 0);
        return slot;
    }
};

uint32 JS_FASTCALL
GetUpvarStackOnTrace(JSContext* cx, uint32 upvarLevel, int32 slot, uint32 callDepth,
                     double* result)
{
    return GetUpvarOnTrace<UpvarStackTraits>(cx, upvarLevel, slot, callDepth, result);
}









template<typename T>
inline uint32
GetFromClosure(JSContext* cx, JSObject* callee, uint32 scopeIndex, uint32 slot, uint32 callDepth,
               double* result)
{
    JS_ASSERT(scopeIndex >= 1);
    JS_ASSERT(OBJ_GET_CLASS(cx, callee) == &js_FunctionClass);
    JSObject* call = callee;

    for (uint32 i = 0; i < scopeIndex; ++i)
        call = OBJ_GET_PARENT(cx, call);

    JS_ASSERT(OBJ_GET_CLASS(cx, call) == &js_CallClass);

    InterpState* state = cx->interpState;

#ifdef DEBUG
    int32 stackOffset = StackDepthFromCallStack(state, callDepth);
    FrameInfo** fip = state->rp + callDepth;
    while (--fip > state->callstackBase) {
        FrameInfo* fi = *fip;
        JSObject* callee = *(JSObject**)(&state->stackBase[stackOffset]);
        if (callee == call) {
            
            
            
            
            
            
            
            JS_NOT_REACHED("JSOP_NAME variable found in outer trace");
        }
        stackOffset -= fi->callerHeight;
    }
#endif

    


    VOUCH_DOES_NOT_REQUIRE_STACK();
    if (cx->fp->callobj == call) {
        slot = T::adj_slot(cx->fp, slot);
        *result = state->stackBase[slot];
        return state->callstackBase[0]->get_typemap()[slot];
    }

    JSStackFrame* fp = (JSStackFrame*) call->getPrivate();
    if (!fp)
        return TT_INVALID;
    jsval v = T::slots(fp)[slot];
    JSTraceType type = getCoercedType(v);
    ValueToNative(cx, v, type, result);
    return type;
}

struct ArgClosureTraits
{
    static inline uint32 adj_slot(JSStackFrame* fp, uint32 slot) { return fp->argc + slot; }
    static inline jsval* slots(JSStackFrame* fp) { return fp->argv; }
private:
    ArgClosureTraits();
};

uint32 JS_FASTCALL
GetClosureArg(JSContext* cx, JSObject* callee, uint32 scopeIndex, uint32 slot, uint32 callDepth,
              double* result)
{
    return GetFromClosure<ArgClosureTraits>(cx, callee, scopeIndex, slot, callDepth, result);
}

struct VarClosureTraits
{
    static inline uint32 adj_slot(JSStackFrame* fp, uint32 slot) { return slot; }
    static inline jsval* slots(JSStackFrame* fp) { return fp->slots; }
private:
    VarClosureTraits();
};

uint32 JS_FASTCALL
GetClosureVar(JSContext* cx, JSObject* callee, uint32 scopeIndex, uint32 slot, uint32 callDepth,
              double* result)
{
    return GetFromClosure<VarClosureTraits>(cx, callee, scopeIndex, slot, callDepth, result);
}
















static JS_REQUIRES_STACK int
FlushNativeStackFrame(JSContext* cx, unsigned callDepth, JSTraceType* mp, double* np,
                      JSStackFrame* stopFrame)
{
    jsval* stopAt = stopFrame ? &stopFrame->argv[-2] : NULL;

    
    FlushNativeStackFrameVisitor visitor(cx, mp, np, stopAt);
    VisitStackSlots(visitor, cx, callDepth);

    
    
    
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
            if (fp->argv) {
                
                if (fp->argsobj && !JSVAL_TO_OBJECT(fp->argsobj)->getPrivate()) {
                    JSVAL_TO_OBJECT(fp->argsobj)->setPrivate(fp);
                }

                



                JS_ASSERT(JSVAL_IS_OBJECT(fp->argv[-1]));
                JS_ASSERT(HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(fp->argv[-2])));
                JS_ASSERT(GET_FUNCTION_PRIVATE(cx, JSVAL_TO_OBJECT(fp->argv[-2])) ==
                          GET_FUNCTION_PRIVATE(cx, fp->callee()));
                JS_ASSERT(GET_FUNCTION_PRIVATE(cx, fp->callee()) == fp->fun);

                




                if (!fp->scopeChain) {
                    fp->scopeChain = OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(fp->argv[-2]));
                    if (fp->fun->flags & JSFUN_HEAVYWEIGHT) {
                        






                        void* hookData = ((JSInlineFrame*)fp)->hookData;
                        ((JSInlineFrame*)fp)->hookData = NULL;
                        JS_ASSERT(!JS_TRACE_MONITOR(cx).useReservedObjects);
                        JS_TRACE_MONITOR(cx).useReservedObjects = JS_TRUE;
#ifdef DEBUG
                        JSObject *obj =
#endif
                            js_GetCallObject(cx, fp);
                        JS_ASSERT(obj);
                        JS_TRACE_MONITOR(cx).useReservedObjects = JS_FALSE;
                        ((JSInlineFrame*)fp)->hookData = hookData;
                    }
                }
                fp->thisp = JSVAL_TO_OBJECT(fp->argv[-1]);
                if (fp->flags & JSFRAME_CONSTRUCTING) 
                    fp->flags |= JSFRAME_COMPUTED_THIS;
            }
        }
    }
    debug_only_print0(LC_TMTracer, "\n");
    return visitor.getTypeMap() - mp;
}


JS_REQUIRES_STACK void
TraceRecorder::import(LIns* base, ptrdiff_t offset, jsval* p, JSTraceType t,
                      const char *prefix, uintN index, JSStackFrame *fp)
{
    LIns* ins;
    if (t == TT_INT32) { 
        JS_ASSERT(isInt32(*p));

        





        ins = lir->insLoad(LIR_ld, base, offset);
        ins = lir->ins1(LIR_i2f, ins);
    } else {
        JS_ASSERT_IF(t != TT_JSVAL, isNumber(*p) == (t == TT_DOUBLE));
        if (t == TT_DOUBLE) {
            ins = lir->insLoad(LIR_ldq, base, offset);
        } else if (t == TT_PSEUDOBOOLEAN) {
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
        if (fp->fun->hasLocalNames())
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
        "object", "int", "double", "jsval", "string", "null", "boolean", "function"
    };
    debug_only_printf(LC_TMTracer, "import vp=%p name=%s type=%s flags=%d\n",
                      (void*)p, name, typestr[t & 7], t >> 3);
#endif
}

class ImportGlobalSlotVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    LIns *mBase;
    JSTraceType *mTypemap;
public:
    ImportGlobalSlotVisitor(TraceRecorder &recorder,
                            LIns *base,
                            JSTraceType *typemap) :
        mRecorder(recorder),
        mBase(base),
        mTypemap(typemap)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(jsval *vp, unsigned n, unsigned slot) {
        JS_ASSERT(*mTypemap != TT_JSVAL);
        mRecorder.import(mBase, mRecorder.nativeGlobalOffset(vp),
                         vp, *mTypemap++, "global", n, NULL);
    }
};

class ImportBoxedStackSlotVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    LIns *mBase;
    ptrdiff_t mStackOffset;
    JSTraceType *mTypemap;
    JSStackFrame *mFp;
public:
    ImportBoxedStackSlotVisitor(TraceRecorder &recorder,
                                LIns *base,
                                ptrdiff_t stackOffset,
                                JSTraceType *typemap) :
        mRecorder(recorder),
        mBase(base),
        mStackOffset(stackOffset),
        mTypemap(typemap)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            if (*mTypemap == TT_JSVAL) {
                mRecorder.import(mBase, mStackOffset, vp, TT_JSVAL,
                                 "jsval", i, fp);
                LIns *vp_ins = mRecorder.unbox_jsval(*vp, mRecorder.get(vp),
                                                     mRecorder.copy(mRecorder.anchor));
                mRecorder.set(vp, vp_ins);
            }
            vp++;
            mTypemap++;
            mStackOffset += sizeof(double);
        }
        return true;
    }
};

class ImportUnboxedStackSlotVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    LIns *mBase;
    ptrdiff_t mStackOffset;
    JSTraceType *mTypemap;
    JSStackFrame *mFp;
public:
    ImportUnboxedStackSlotVisitor(TraceRecorder &recorder,
                                  LIns *base,
                                  ptrdiff_t stackOffset,
                                  JSTraceType *typemap) :
        mRecorder(recorder),
        mBase(base),
        mStackOffset(stackOffset),
        mTypemap(typemap)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            if (*mTypemap != TT_JSVAL) {
                mRecorder.import(mBase, mStackOffset, vp++, *mTypemap,
                                 stackSlotKind(), i, fp);
            }
            mTypemap++;
            mStackOffset += sizeof(double);
        }
        return true;
    }
};

JS_REQUIRES_STACK void
TraceRecorder::import(TreeInfo* treeInfo, LIns* sp, unsigned stackSlots, unsigned ngslots,
                      unsigned callDepth, JSTraceType* typeMap)
{
    














    JSTraceType* globalTypeMap = typeMap + stackSlots;
    unsigned length = treeInfo->nGlobalTypes();

    



    if (ngslots < length) {
        MergeTypeMaps(&globalTypeMap , &ngslots ,
                      treeInfo->globalTypeMap(), length,
                      (JSTraceType*)alloca(sizeof(JSTraceType) * length));
    }
    JS_ASSERT(ngslots == treeInfo->nGlobalTypes());
    ptrdiff_t offset = -treeInfo->nativeStackBase;

    



    ImportBoxedStackSlotVisitor boxedStackVisitor(*this, sp, offset, typeMap);
    VisitStackSlots(boxedStackVisitor, cx, callDepth);

    ImportGlobalSlotVisitor globalVisitor(*this, lirbuf->state, globalTypeMap);
    VisitGlobalSlots(globalVisitor, cx, globalObj, ngslots,
                     treeInfo->globalSlots->data());

    ImportUnboxedStackSlotVisitor unboxedStackVisitor(*this, sp, offset,
                                                      typeMap);
    VisitStackSlots(unboxedStackVisitor, cx, callDepth);
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
    JSTraceType type = getCoercedType(*vp);
    if (type == TT_INT32 && oracle.isGlobalSlotUndemotable(cx, slot))
        type = TT_DOUBLE;
    treeInfo->typeMap.add(type);
    import(lirbuf->state, sizeof(struct InterpState) + slot*sizeof(double),
           vp, type, "global", index, NULL);
    SpecializeTreesToMissingGlobals(cx, globalObj, treeInfo);
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
        ASSERT_VALID_CACHE_HIT(x->oprnd2(), x->disp());
        writeBack(i, x->oprnd2(), x->disp());
    }
#undef ASSERT_VALID_CACHE_HIT
}

JS_REQUIRES_STACK LIns*
TraceRecorder::get(jsval* p)
{
    checkForGlobalObjectReallocation();
    return tracker.get(p);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::addr(jsval* p)
{
    return isGlobal(p)
           ? lir->ins2i(LIR_piadd, lirbuf->state, nativeGlobalOffset(p))
           : lir->ins2i(LIR_piadd, lirbuf->sp, -treeInfo->nativeStackBase + nativeStackOffset(p));
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
        debug_only_print0(LC_TMTracer,
                          "globalObj->dslots relocated, updating tracker\n");
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
IsLoopEdge(jsbytecode* pc, jsbytecode* header)
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

class AdjustCallerGlobalTypesVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    JSContext *mCx;
    nanojit::LirBuffer *mLirbuf;
    nanojit::LirWriter *mLir;
    JSTraceType *mTypeMap;
public:
    AdjustCallerGlobalTypesVisitor(TraceRecorder &recorder,
                                   JSTraceType *typeMap) :
        mRecorder(recorder),
        mCx(mRecorder.cx),
        mLirbuf(mRecorder.lirbuf),
        mLir(mRecorder.lir),
        mTypeMap(typeMap)
    {}

    JSTraceType* getTypeMap()
    {
        return mTypeMap;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(jsval *vp, unsigned n, unsigned slot) {
        LIns *ins = mRecorder.get(vp);
        bool isPromote = isPromoteInt(ins);
        if (isPromote && *mTypeMap == TT_DOUBLE) {
            mLir->insStorei(mRecorder.get(vp), mLirbuf->state,
                            mRecorder.nativeGlobalOffset(vp));

            



            oracle.markGlobalSlotUndemotable(mCx, slot);
        }
        JS_ASSERT(!(!isPromote && *mTypeMap == TT_INT32));
        ++mTypeMap;
    }
};

class AdjustCallerStackTypesVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    JSContext *mCx;
    nanojit::LirBuffer *mLirbuf;
    nanojit::LirWriter *mLir;
    unsigned mSlotnum;
    JSTraceType *mTypeMap;
public:
    AdjustCallerStackTypesVisitor(TraceRecorder &recorder,
                                  JSTraceType *typeMap) :
        mRecorder(recorder),
        mCx(mRecorder.cx),
        mLirbuf(mRecorder.lirbuf),
        mLir(mRecorder.lir),
        mSlotnum(0),
        mTypeMap(typeMap)
    {}

    JSTraceType* getTypeMap()
    {
        return mTypeMap;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            LIns *ins = mRecorder.get(vp);
            bool isPromote = isPromoteInt(ins);
            if (isPromote && *mTypeMap == TT_DOUBLE) {
                mLir->insStorei(mRecorder.get(vp), mLirbuf->sp,
                                -mRecorder.treeInfo->nativeStackBase +
                                mRecorder.nativeStackOffset(vp));

                



                oracle.markStackSlotUndemotable(mCx, mSlotnum);
            }
            JS_ASSERT(!(!isPromote && *mTypeMap == TT_INT32));
            ++vp;
            ++mTypeMap;
            ++mSlotnum;
        }
        return true;
    }
};






JS_REQUIRES_STACK void
TraceRecorder::adjustCallerTypes(Fragment* f)
{
    TreeInfo* ti = (TreeInfo*)f->vmprivate;

    AdjustCallerGlobalTypesVisitor globalVisitor(*this, ti->globalTypeMap());
    VisitGlobalSlots(globalVisitor, cx, *treeInfo->globalSlots);

    AdjustCallerStackTypesVisitor stackVisitor(*this, ti->stackTypeMap());
    VisitStackSlots(stackVisitor, cx, 0);

    JS_ASSERT(f == f->root);
}

JS_REQUIRES_STACK JSTraceType
TraceRecorder::determineSlotType(jsval* vp)
{
    JSTraceType m;
    LIns* i = get(vp);
    if (isNumber(*vp)) {
        m = isPromoteInt(i) ? TT_INT32 : TT_DOUBLE;
    } else if (JSVAL_IS_OBJECT(*vp)) {
        if (JSVAL_IS_NULL(*vp))
            m = TT_NULL;
        else if (HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(*vp)))
            m = TT_FUNCTION;
        else
            m = TT_OBJECT;
    } else {
        JS_ASSERT(JSVAL_TAG(*vp) == JSVAL_STRING || JSVAL_IS_SPECIAL(*vp));
        JS_STATIC_ASSERT(static_cast<jsvaltag>(TT_STRING) == JSVAL_STRING);
        JS_STATIC_ASSERT(static_cast<jsvaltag>(TT_PSEUDOBOOLEAN) == JSVAL_SPECIAL);
        m = JSTraceType(JSVAL_TAG(*vp));
    }
    JS_ASSERT(m != TT_INT32 || isInt32(*vp));
    return m;
}

class DetermineTypesVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    JSTraceType *mTypeMap;
public:
    DetermineTypesVisitor(TraceRecorder &recorder,
                          JSTraceType *typeMap) :
        mRecorder(recorder),
        mTypeMap(typeMap)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(jsval *vp, unsigned n, unsigned slot) {
        *mTypeMap++ = mRecorder.determineSlotType(vp);
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i)
            *mTypeMap++ = mRecorder.determineSlotType(vp++);
        return true;
    }

    JSTraceType* getTypeMap()
    {
        return mTypeMap;
    }
};

#if defined JS_JIT_SPEW
JS_REQUIRES_STACK static void
TreevisLogExit(JSContext* cx, VMSideExit* exit)
{
    debug_only_printf(LC_TMTreeVis, "TREEVIS ADDEXIT EXIT=%p TYPE=%s FRAG=%p PC=%p FILE=\"%s\""
                      " LINE=%d OFFS=%d", (void*)exit, getExitName(exit->exitType),
                      (void*)exit->from, (void*)cx->fp->regs->pc, cx->fp->script->filename,
                      js_FramePCToLineNumber(cx, cx->fp), FramePCOffset(cx->fp));
    debug_only_print0(LC_TMTreeVis, " STACK=\"");
    for (unsigned i = 0; i < exit->numStackSlots; i++)
        debug_only_printf(LC_TMTreeVis, "%c", typeChar[exit->stackTypeMap()[i]]);
    debug_only_print0(LC_TMTreeVis, "\" GLOBALS=\"");
    for (unsigned i = 0; i < exit->numGlobalSlots; i++)
        debug_only_printf(LC_TMTreeVis, "%c", typeChar[exit->globalTypeMap()[i]]);
    debug_only_print0(LC_TMTreeVis, "\"\n");
}
#endif

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
        JS_ASSERT(*pc == JSOP_CALL || *pc == JSOP_APPLY || *pc == JSOP_NEW ||
                  *pc == JSOP_SETPROP || *pc == JSOP_SETNAME);
        pc += cs.length;
        regs->pc = pc;
        MUST_FLOW_THROUGH("restore_pc");
    }

    



    unsigned stackSlots = NativeStackSlots(cx, callDepth);

    



    trackNativeStackUse(stackSlots + 1);

    
    unsigned ngslots = treeInfo->globalSlots->length();
    unsigned typemap_size = (stackSlots + ngslots) * sizeof(JSTraceType);
    void *mark = JS_ARENA_MARK(&cx->tempPool);
    JSTraceType* typemap;
    JS_ARENA_ALLOCATE_CAST(typemap, JSTraceType*, &cx->tempPool, typemap_size);

    





    DetermineTypesVisitor detVisitor(*this, typemap);
    VisitSlots(detVisitor, cx, callDepth, ngslots,
               treeInfo->globalSlots->data());
    JS_ASSERT(unsigned(detVisitor.getTypeMap() - typemap) ==
              ngslots + stackSlots);

    






    if (pendingUnboxSlot ||
        (pendingTraceableNative && (pendingTraceableNative->flags & JSTN_UNBOX_AFTER))) {
        unsigned pos = stackSlots - 1;
        if (pendingUnboxSlot == cx->fp->regs->sp - 2)
            pos = stackSlots - 2;
        typemap[pos] = TT_JSVAL;
    }

    
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
                ngslots == e->numGlobalSlots &&
                !memcmp(exits[n]->fullTypeMap(), typemap, typemap_size)) {
                AUDIT(mergedLoopExits);
#if defined JS_JIT_SPEW
                TreevisLogExit(cx, e);
#endif
                JS_ARENA_RELEASE(&cx->tempPool, mark);
                return e;
            }
        }
    }

    if (sizeof(VMSideExit) + (stackSlots + ngslots) * sizeof(JSTraceType) >
        LirBuffer::MAX_SKIP_PAYLOAD_SZB) {
        







        stackSlots = 0;
        ngslots = 0;
        typemap_size = 0;
        trashSelf = true;
    }

    
    LIns* data = lir->insSkip(sizeof(VMSideExit) + (stackSlots + ngslots) * sizeof(JSTraceType));
    VMSideExit* exit = (VMSideExit*) data->payload();

    
    memset(exit, 0, sizeof(VMSideExit));
    exit->from = fragment;
    exit->calldepth = callDepth;
    exit->numGlobalSlots = ngslots;
    exit->numStackSlots = stackSlots;
    exit->numStackSlotsBelowCurrentFrame = cx->fp->argv ?
                                           nativeStackOffset(&cx->fp->argv[-2]) / sizeof(double) :
                                           0;
    exit->exitType = exitType;
    exit->block = fp->blockChain;
    if (fp->blockChain)
        treeInfo->gcthings.addUnique(OBJECT_TO_JSVAL(fp->blockChain));
    exit->pc = pc;
    exit->imacpc = fp->imacpc;
    exit->sp_adj = (stackSlots * sizeof(double)) - treeInfo->nativeStackBase;
    exit->rp_adj = exit->calldepth * sizeof(FrameInfo*);
    exit->nativeCalleeWord = 0;
    exit->lookupFlags = js_InferFlags(cx, 0);
    memcpy(exit->fullTypeMap(), typemap, typemap_size);

#if defined JS_JIT_SPEW
    TreevisLogExit(cx, exit);
#endif

    JS_ARENA_RELEASE(&cx->tempPool, mark);
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
    debug_only_printf(LC_TMRecorder,
                      "    About to try emitting guard code for "
                      "SideExit=%p exitType=%s\n",
                      (void*)exit, getExitName(exit->exitType));

    LIns* guardRec = createGuardRecord(exit);

    





    if (exit->exitType == LOOP_EXIT)
        treeInfo->sideExits.add(exit);

    if (!cond->isCond()) {
        expected = !expected;
        cond = lir->ins_eq0(cond);
    }

    LIns* guardIns =
        lir->insGuard(expected ? LIR_xf : LIR_xt, cond, guardRec);
    if (!guardIns) {
        debug_only_print0(LC_TMRecorder,
                          "    redundant guard, eliminated, no codegen\n");
    }
}

JS_REQUIRES_STACK VMSideExit*
TraceRecorder::copy(VMSideExit* copy)
{
    size_t typemap_size = copy->numGlobalSlots + copy->numStackSlots;
    LIns* data = lir->insSkip(sizeof(VMSideExit) + typemap_size * sizeof(JSTraceType));
    VMSideExit* exit = (VMSideExit*) data->payload();

    
    memcpy(exit, copy, sizeof(VMSideExit) + typemap_size * sizeof(JSTraceType));
    exit->guards = NULL;
    exit->from = fragment;
    exit->target = NULL;

    





    if (exit->exitType == LOOP_EXIT)
        treeInfo->sideExits.add(exit);
#if defined JS_JIT_SPEW
    TreevisLogExit(cx, exit);
#endif
    return exit;
}






JS_REQUIRES_STACK void
TraceRecorder::guard(bool expected, LIns* cond, ExitType exitType)
{
    guard(expected, cond, snapshot(exitType));
}





static inline bool
ProhibitFlush(JSContext* cx)
{
    if (cx->interpState) 
        return true;

    JSCList *cl;

#ifdef JS_THREADSAFE
    JSThread* thread = cx->thread;
    for (cl = thread->contextList.next; cl != &thread->contextList; cl = cl->next)
        if (CX_FROM_THREAD_LINKS(cl)->interpState)
            return true;
#else
    JSRuntime* rt = cx->runtime;
    for (cl = rt->contextList.next; cl != &rt->contextList; cl = cl->next)
        if (js_ContextFromLinkField(cl)->interpState)
            return true;
#endif
    return false;
}

static JS_REQUIRES_STACK void
ResetJIT(JSContext* cx)
{
    if (!TRACING_ENABLED(cx))
        return;
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    debug_only_print0(LC_TMTracer, "Flushing cache.\n");
    if (tm->recorder)
        js_AbortRecording(cx, "flush cache");
    TraceRecorder* tr;
    while ((tr = tm->abortStack) != NULL) {
        tr->removeFragmentReferences();
        tr->deepAbort();
        tr->popAbortStack();
    }
    if (ProhibitFlush(cx)) {
        debug_only_print0(LC_TMTracer, "Deferring JIT flush due to deep bail.\n");
        tm->needFlush = JS_TRUE;
        return;
    }
    tm->flush();
}


JS_REQUIRES_STACK void
TraceRecorder::compile(JSTraceMonitor* tm)
{
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_COMPILE);
#endif

    if (tm->needFlush) {
        ResetJIT(cx);
        return;
    }
    if (treeInfo->maxNativeStackSlots >= MAX_NATIVE_STACK_SLOTS) {
        debug_only_print0(LC_TMTracer, "Blacklist: excessive stack use.\n");
        Blacklist((jsbytecode*) fragment->root->ip);
        return;
    }
    if (anchor && anchor->exitType != CASE_EXIT)
        ++treeInfo->branchCount;
    if (tm->allocator->outOfMemory())
        return;

    Assembler *assm = tm->assembler;
    ::compile(assm, fragment, *tm->allocator verbose_only(, tm->labels));
    if (assm->error() == nanojit::OutOMem)
        return;

    if (assm->error() != nanojit::None) {
        debug_only_print0(LC_TMTracer, "Blacklisted: error during compilation\n");
        Blacklist((jsbytecode*) fragment->root->ip);
        return;
    }
    ResetRecordingAttempts(cx, (jsbytecode*) fragment->ip);
    ResetRecordingAttempts(cx, (jsbytecode*) fragment->root->ip);
    if (anchor) {
#ifdef NANOJIT_IA32
        if (anchor->exitType == CASE_EXIT)
            assm->patch(anchor, anchor->switchInfo);
        else
#endif
            assm->patch(anchor);
    }
    JS_ASSERT(fragment->code());
    JS_ASSERT(!fragment->vmprivate);
    if (fragment == fragment->root)
        fragment->vmprivate = treeInfo;

    
#if defined DEBUG && !defined WIN32
    const char* filename = cx->fp->script->filename;
    char* label = (char*)js_malloc((filename ? strlen(filename) : 7) + 16);
    sprintf(label, "%s:%u", filename ? filename : "<stdin>",
            js_FramePCToLineNumber(cx, cx->fp));
    tm->labels->add(fragment, sizeof(Fragment), 0, label);
    js_free(label);
#endif
    AUDIT(traceCompleted);
}

static void
JoinPeers(Assembler* assm, VMSideExit* exit, VMFragment* target)
{
    exit->target = target;
    assm->patch(exit);

    debug_only_printf(LC_TMTreeVis, "TREEVIS JOIN ANCHOR=%p FRAG=%p\n", (void*)exit, (void*)target);

    if (exit->root() == target)
        return;

    target->getTreeInfo()->dependentTrees.addUnique(exit->root());
    exit->root()->getTreeInfo()->linkedTrees.addUnique(target);
}


enum TypeCheckResult
{
    TypeCheck_Okay,         
    TypeCheck_Promote,      
    TypeCheck_Demote,       
    TypeCheck_Undemote,     
    TypeCheck_Bad           
};

class SlotMap : public SlotVisitorBase
{
  public:
    struct SlotInfo
    {
        SlotInfo()
            : v(0), promoteInt(false), lastCheck(TypeCheck_Bad)
        {}
        SlotInfo(jsval* v, bool promoteInt)
          : v(v), promoteInt(promoteInt), lastCheck(TypeCheck_Bad)
        {}
        jsval           *v;
        bool            promoteInt;
        TypeCheckResult lastCheck;
    };

    SlotMap(TraceRecorder& rec, unsigned slotOffset)
        : mRecorder(rec),
          mCx(rec.cx),
          slots(NULL),
          slotOffset(slotOffset)
    {
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(jsval *vp, unsigned n, unsigned slot)
    {
        addSlot(vp);
    }

    JS_ALWAYS_INLINE SlotMap::SlotInfo&
    operator [](unsigned i)
    {
        return slots[i];
    }

    JS_ALWAYS_INLINE SlotMap::SlotInfo&
    get(unsigned i)
    {
        return slots[i];
    }

    JS_ALWAYS_INLINE unsigned
    length()
    {
        return slots.length();
    }

    









    JS_REQUIRES_STACK TypeConsensus
    checkTypes(TreeInfo* ti)
    {
        if (ti->typeMap.length() < slotOffset || length() != ti->typeMap.length() - slotOffset)
            return TypeConsensus_Bad;

        bool has_undemotes = false;
        for (unsigned i = 0; i < length(); i++) {
            TypeCheckResult result = checkType(i, ti->typeMap[i + slotOffset]);
            if (result == TypeCheck_Bad)
                return TypeConsensus_Bad;
            if (result == TypeCheck_Undemote)
                has_undemotes = true;
            slots[i].lastCheck = result;
        }
        if (has_undemotes)
            return TypeConsensus_Undemotes;
        return TypeConsensus_Okay;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    addSlot(jsval* vp)
    {
        slots.add(SlotInfo(vp, isNumber(*vp) && isPromoteInt(mRecorder.get(vp))));
    }

    JS_REQUIRES_STACK void
    markUndemotes()
    {
        for (unsigned i = 0; i < length(); i++) {
            if (get(i).lastCheck == TypeCheck_Undemote)
                MarkSlotUndemotable(mRecorder.cx, mRecorder.treeInfo, slotOffset + i);
        }
    }

    JS_REQUIRES_STACK virtual void
    adjustTypes()
    {
        for (unsigned i = 0; i < length(); i++) {
            SlotInfo& info = get(i);
            JS_ASSERT(info.lastCheck != TypeCheck_Undemote && info.lastCheck != TypeCheck_Bad);
            if (info.lastCheck == TypeCheck_Promote) {
                JS_ASSERT(isNumber(*info.v));
                mRecorder.set(info.v, mRecorder.f2i(mRecorder.get(info.v)));
            } else if (info.lastCheck == TypeCheck_Demote) {
                JS_ASSERT(isNumber(*info.v));
                mRecorder.set(info.v, mRecorder.lir->ins1(LIR_i2f, mRecorder.get(info.v)));
            }
        }
    }
  private:
    TypeCheckResult
    checkType(unsigned i, JSTraceType t)
    {
        debug_only_printf(LC_TMTracer,
                          "checkType slot %d: interp=%c typemap=%c isNum=%d promoteInt=%d\n",
                          i,
                          typeChar[getCoercedType(*slots[i].v)],
                          typeChar[t],
                          isNumber(*slots[i].v),
                          slots[i].promoteInt);
        switch (t) {
          case TT_INT32:
            if (!isNumber(*slots[i].v))
                return TypeCheck_Bad; 
            
            if (!slots[i].promoteInt)
                return TypeCheck_Undemote;
            
            JS_ASSERT(isInt32(*slots[i].v) && slots[i].promoteInt);
            return TypeCheck_Promote;
          case TT_DOUBLE:
            if (!isNumber(*slots[i].v))
                return TypeCheck_Bad; 
            if (slots[i].promoteInt)
                return TypeCheck_Demote;
            return TypeCheck_Okay;
          case TT_NULL:
            return JSVAL_IS_NULL(*slots[i].v) ? TypeCheck_Okay : TypeCheck_Bad;
          case TT_FUNCTION:
            return !JSVAL_IS_PRIMITIVE(*slots[i].v) &&
                   HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(*slots[i].v)) ?
                   TypeCheck_Okay : TypeCheck_Bad;
          case TT_OBJECT:
            return !JSVAL_IS_PRIMITIVE(*slots[i].v) &&
                   !HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(*slots[i].v)) ?
                   TypeCheck_Okay : TypeCheck_Bad;
          default:
            return getCoercedType(*slots[i].v) == t ? TypeCheck_Okay : TypeCheck_Bad;
        }
        JS_NOT_REACHED("shouldn't fall through type check switch");
    }
  protected:
    TraceRecorder& mRecorder;
    JSContext* mCx;
    Queue<SlotInfo> slots;
    unsigned   slotOffset;
};

class DefaultSlotMap : public SlotMap
{
  public:
    DefaultSlotMap(TraceRecorder& tr) : SlotMap(tr, 0)
    {
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, size_t count, JSStackFrame* fp)
    {
        for (size_t i = 0; i < count; i++)
            addSlot(&vp[i]);
        return true;
    }
};

JS_REQUIRES_STACK TypeConsensus
TraceRecorder::selfTypeStability(SlotMap& slotMap)
{
    debug_only_printf(LC_TMTracer, "Checking type stability against self=%p\n", (void*)fragment);
    TypeConsensus consensus = slotMap.checkTypes(treeInfo);

    
    if (consensus == TypeConsensus_Okay)
        return TypeConsensus_Okay;

    


    if (consensus == TypeConsensus_Undemotes)
        slotMap.markUndemotes();

    return consensus;
}

JS_REQUIRES_STACK TypeConsensus
TraceRecorder::peerTypeStability(SlotMap& slotMap, VMFragment** pPeer)
{
    
    VMFragment* root = (VMFragment*)fragment->root;
    VMFragment* peer = getLoop(traceMonitor, root->ip, root->globalObj, root->globalShape,
                               root->argc);
    JS_ASSERT(peer != NULL);
    bool onlyUndemotes = false;
    for (; peer != NULL; peer = (VMFragment*)peer->peer) {
        if (!peer->vmprivate || peer == fragment)
            continue;
        debug_only_printf(LC_TMTracer, "Checking type stability against peer=%p\n", (void*)peer);
        TypeConsensus consensus = slotMap.checkTypes((TreeInfo*)peer->vmprivate);
        if (consensus == TypeConsensus_Okay) {
            *pPeer = peer;
            


            return TypeConsensus_Okay;
        }
        if (consensus == TypeConsensus_Undemotes)
            onlyUndemotes = true;
    }

    return onlyUndemotes ? TypeConsensus_Undemotes : TypeConsensus_Bad;
}

JS_REQUIRES_STACK bool
TraceRecorder::closeLoop(TypeConsensus &consensus)
{
    DefaultSlotMap slotMap(*this);
    VisitSlots(slotMap, cx, 0, *treeInfo->globalSlots);
    return closeLoop(slotMap, snapshot(UNSTABLE_LOOP_EXIT), consensus);
}




JS_REQUIRES_STACK bool
TraceRecorder::closeLoop(SlotMap& slotMap, VMSideExit* exit, TypeConsensus& consensus)
{
    




    JS_ASSERT((*cx->fp->regs->pc == JSOP_LOOP || *cx->fp->regs->pc == JSOP_NOP) && !cx->fp->imacpc);

    if (callDepth != 0) {
        debug_only_print0(LC_TMTracer,
                          "Blacklisted: stack depth mismatch, possible recursion.\n");
        Blacklist((jsbytecode*) fragment->root->ip);
        trashSelf = true;
        consensus = TypeConsensus_Bad;
        return false;
    }

    JS_ASSERT(exit->exitType == UNSTABLE_LOOP_EXIT);
    JS_ASSERT(exit->numStackSlots == treeInfo->nStackTypes);

    VMFragment* peer = NULL;
    VMFragment* root = (VMFragment*)fragment->root;

    consensus = selfTypeStability(slotMap);
    if (consensus != TypeConsensus_Okay) {
        TypeConsensus peerConsensus = peerTypeStability(slotMap, &peer);
        
        if (peerConsensus != TypeConsensus_Bad)
            consensus = peerConsensus;
    }

#if DEBUG
    if (consensus != TypeConsensus_Okay || peer)
        AUDIT(unstableLoopVariable);
#endif

    JS_ASSERT(!trashSelf);

    


    if (consensus == TypeConsensus_Okay)
        slotMap.adjustTypes();

    if (consensus != TypeConsensus_Okay || peer) {
        fragment->lastIns = lir->insGuard(LIR_x, NULL, createGuardRecord(exit));

        
        JS_ASSERT_IF(peer, consensus == TypeConsensus_Okay);

        
        if (!peer) {
            




            debug_only_print0(LC_TMTracer,
                              "Trace has unstable loop variable with no stable peer, "
                              "compiling anyway.\n");
            UnstableExit* uexit = new (*traceMonitor->allocator) UnstableExit;
            uexit->fragment = fragment;
            uexit->exit = exit;
            uexit->next = treeInfo->unstableExits;
            treeInfo->unstableExits = uexit;
        } else {
            JS_ASSERT(peer->code());
            exit->target = peer;
            debug_only_printf(LC_TMTracer,
                              "Joining type-unstable trace to target fragment %p.\n",
                              (void*)peer);
            ((TreeInfo*)peer->vmprivate)->dependentTrees.addUnique(fragment->root);
            treeInfo->linkedTrees.addUnique(peer);
        }
    } else {
        exit->exitType = LOOP_EXIT;
        debug_only_printf(LC_TMTreeVis, "TREEVIS CHANGEEXIT EXIT=%p TYPE=%s\n", (void*)exit,
                          getExitName(LOOP_EXIT));
        exit->target = fragment->root;
        fragment->lastIns = lir->insGuard(LIR_loop, lir->insImm(1), createGuardRecord(exit));
    }
    compile(traceMonitor);

    Assembler *assm = JS_TRACE_MONITOR(cx).assembler;
    if (assm->error() != nanojit::None)
        return false;

    debug_only_printf(LC_TMTreeVis, "TREEVIS CLOSELOOP EXIT=%p PEER=%p\n", (void*)exit, (void*)peer);

    peer = getLoop(traceMonitor, root->ip, root->globalObj, root->globalShape, root->argc);
    JS_ASSERT(peer);
    joinEdgesToEntry(peer);

    debug_only_stmt(DumpPeerStability(traceMonitor, peer->ip, peer->globalObj,
                                      peer->globalShape, peer->argc);)

    debug_only_print0(LC_TMTracer,
                      "updating specializations on dependent and linked trees\n");
    if (fragment->root->vmprivate)
        SpecializeTreesToMissingGlobals(cx, globalObj, (TreeInfo*)fragment->root->vmprivate);

    



    if (outer)
        AttemptCompilation(cx, traceMonitor, globalObj, outer, outerArgc);
#ifdef JS_JIT_SPEW
    debug_only_printf(LC_TMMinimal,
                      "recording completed at  %s:%u@%u via closeLoop\n",
                      cx->fp->script->filename,
                      js_FramePCToLineNumber(cx, cx->fp),
                      FramePCOffset(cx->fp));
    debug_only_print0(LC_TMMinimal, "\n");
#endif

    return true;
}

static void
FullMapFromExit(TypeMap& typeMap, VMSideExit* exit)
{
    typeMap.setLength(0);
    typeMap.fromRaw(exit->stackTypeMap(), exit->numStackSlots);
    typeMap.fromRaw(exit->globalTypeMap(), exit->numGlobalSlots);
    
    if (exit->numGlobalSlots < exit->root()->getTreeInfo()->nGlobalTypes()) {
        typeMap.fromRaw(exit->root()->getTreeInfo()->globalTypeMap() + exit->numGlobalSlots,
                        exit->root()->getTreeInfo()->nGlobalTypes() - exit->numGlobalSlots);
    }
}

static JS_REQUIRES_STACK TypeConsensus
TypeMapLinkability(JSContext* cx, const TypeMap& typeMap, VMFragment* peer)
{
    const TypeMap& peerMap = peer->getTreeInfo()->typeMap;
    unsigned minSlots = JS_MIN(typeMap.length(), peerMap.length());
    TypeConsensus consensus = TypeConsensus_Okay;
    for (unsigned i = 0; i < minSlots; i++) {
        if (typeMap[i] == peerMap[i])
            continue;
        if (typeMap[i] == TT_INT32 && peerMap[i] == TT_DOUBLE &&
            IsSlotUndemotable(cx, peer->getTreeInfo(), i)) {
            consensus = TypeConsensus_Undemotes;
        } else {
            return TypeConsensus_Bad;
        }
    }
    return consensus;
}

static JS_REQUIRES_STACK unsigned
FindUndemotesInTypemaps(JSContext* cx, const TypeMap& typeMap, TreeInfo* treeInfo,
                        Queue<unsigned>& undemotes)
{
    undemotes.setLength(0);
    unsigned minSlots = JS_MIN(typeMap.length(), treeInfo->typeMap.length());
    for (unsigned i = 0; i < minSlots; i++) {
        if (typeMap[i] == TT_INT32 && treeInfo->typeMap[i] == TT_DOUBLE) {
            undemotes.add(i);
        } else if (typeMap[i] != treeInfo->typeMap[i]) {
            return 0;
        }
    }
    for (unsigned i = 0; i < undemotes.length(); i++)
        MarkSlotUndemotable(cx, treeInfo, undemotes[i]);
    return undemotes.length();
}

JS_REQUIRES_STACK void
TraceRecorder::joinEdgesToEntry(VMFragment* peer_root)
{
    if (fragment->kind != LoopTrace)
        return;

    TypeMap typeMap(NULL);
    Queue<unsigned> undemotes(NULL);

    for (VMFragment* peer = peer_root; peer; peer = (VMFragment*)peer->peer) {
        TreeInfo* ti = peer->getTreeInfo();
        if (!ti)
            continue;
        UnstableExit* uexit = ti->unstableExits;
        while (uexit != NULL) {
            
            FullMapFromExit(typeMap, uexit->exit);
            
            TypeConsensus consensus = TypeMapLinkability(cx, typeMap, (VMFragment*)fragment->root);
            JS_ASSERT_IF(consensus == TypeConsensus_Okay, peer != fragment);
            if (consensus == TypeConsensus_Okay) {
                debug_only_printf(LC_TMTracer,
                                  "Joining type-stable trace to target exit %p->%p.\n",
                                  (void*)uexit->fragment, (void*)uexit->exit);
                
                JoinPeers(traceMonitor->assembler, uexit->exit, (VMFragment*)fragment);
                uexit = ti->removeUnstableExit(uexit->exit);
            } else {
                
                if (FindUndemotesInTypemaps(cx, typeMap, treeInfo, undemotes)) {
                    JS_ASSERT(peer == uexit->fragment->root);
                    if (fragment == peer)
                        trashSelf = true;
                    else
                        whichTreesToTrash.addUnique(uexit->fragment->root);
                    return;
                }
                uexit = uexit->next;
            }
        }
    }
}

JS_REQUIRES_STACK void
TraceRecorder::endLoop()
{
    endLoop(snapshot(LOOP_EXIT));
}


JS_REQUIRES_STACK void
TraceRecorder::endLoop(VMSideExit* exit)
{
    if (callDepth != 0) {
        debug_only_print0(LC_TMTracer, "Blacklisted: stack depth mismatch, possible recursion.\n");
        Blacklist((jsbytecode*) fragment->root->ip);
        trashSelf = true;
        return;
    }

    fragment->lastIns =
        lir->insGuard(LIR_x, NULL, createGuardRecord(exit));
    compile(traceMonitor);

    Assembler *assm = traceMonitor->assembler;
    if (assm->error() != nanojit::None)
        return;

    debug_only_printf(LC_TMTreeVis, "TREEVIS ENDLOOP EXIT=%p\n", (void*)exit);

    VMFragment* root = (VMFragment*)fragment->root;
    joinEdgesToEntry(getLoop(traceMonitor,
                             root->ip,
                             root->globalObj,
                             root->globalShape,
                             root->argc));
    debug_only_stmt(DumpPeerStability(traceMonitor, root->ip, root->globalObj,
                                      root->globalShape, root->argc);)

    



    debug_only_print0(LC_TMTracer,
                      "updating specializations on dependent and linked trees\n");
    if (fragment->root->vmprivate)
        SpecializeTreesToMissingGlobals(cx, globalObj, (TreeInfo*)fragment->root->vmprivate);

    



    if (outer)
        AttemptCompilation(cx, traceMonitor, globalObj, outer, outerArgc);
#ifdef JS_JIT_SPEW
    debug_only_printf(LC_TMMinimal,
                      "Recording completed at  %s:%u@%u via endLoop\n",
                      cx->fp->script->filename,
                      js_FramePCToLineNumber(cx, cx->fp),
                      FramePCOffset(cx->fp));
    debug_only_print0(LC_TMTracer, "\n");
#endif
}


JS_REQUIRES_STACK void
TraceRecorder::prepareTreeCall(Fragment* inner)
{
    TreeInfo* ti = (TreeInfo*)inner->vmprivate;
    inner_sp_ins = lirbuf->sp;
    VMSideExit* exit = snapshot(OOM_EXIT);

    






    if (callDepth > 0) {
        




        ptrdiff_t sp_adj = nativeStackOffset(&cx->fp->argv[-2]);

        
        ptrdiff_t rp_adj = callDepth * sizeof(FrameInfo*);

        



        debug_only_printf(LC_TMTracer,
                          "sp_adj=%d outer=%d inner=%d\n",
                          sp_adj, treeInfo->nativeStackBase, ti->nativeStackBase);
        LIns* sp_top = lir->ins2i(LIR_piadd, lirbuf->sp,
                - treeInfo->nativeStackBase 
                + sp_adj 
                + ti->maxNativeStackSlots * sizeof(double)); 
        guard(true, lir->ins2(LIR_lt, sp_top, eos_ins), exit);

        
        LIns* rp_top = lir->ins2i(LIR_piadd, lirbuf->rp, rp_adj +
                ti->maxCallDepth * sizeof(FrameInfo*));
        guard(true, lir->ins2(LIR_lt, rp_top, eor_ins), exit);

        
        lir->insStorei(inner_sp_ins = lir->ins2i(LIR_piadd, lirbuf->sp,
                - treeInfo->nativeStackBase 
                + sp_adj 
                + ti->nativeStackBase), 
                lirbuf->state, offsetof(InterpState, sp));
        lir->insStorei(lir->ins2i(LIR_piadd, lirbuf->rp, rp_adj),
                lirbuf->state, offsetof(InterpState, rp));
    }

    





    LIns* guardRec = createGuardRecord(exit);
    lir->insGuard(LIR_xbarrier, NULL, guardRec);
}

static unsigned
BuildGlobalTypeMapFromInnerTree(Queue<JSTraceType>& typeMap, VMSideExit* inner)
{
#if defined DEBUG
    unsigned initialSlots = typeMap.length();
#endif
    
    typeMap.add(inner->globalTypeMap(), inner->numGlobalSlots);

    
    TreeInfo* innerTree = inner->root()->getTreeInfo();
    unsigned slots = inner->numGlobalSlots;
    if (slots < innerTree->nGlobalTypes()) {
        typeMap.add(innerTree->globalTypeMap() + slots, innerTree->nGlobalTypes() - slots);
        slots = innerTree->nGlobalTypes();
    }
    JS_ASSERT(typeMap.length() - initialSlots == slots);
    return slots;
}


JS_REQUIRES_STACK void
TraceRecorder::emitTreeCall(Fragment* inner, VMSideExit* exit)
{
    TreeInfo* ti = (TreeInfo*)inner->vmprivate;

    
    LIns* args[] = { INS_CONSTPTR(inner), lirbuf->state }; 
    LIns* ret = lir->insCall(&js_CallTree_ci, args);

    
#ifdef DEBUG
    JSTraceType* map;
    size_t i;
    map = exit->globalTypeMap();
    for (i = 0; i < exit->numGlobalSlots; i++)
        JS_ASSERT(map[i] != TT_JSVAL);
    map = exit->stackTypeMap();
    for (i = 0; i < exit->numStackSlots; i++)
        JS_ASSERT(map[i] != TT_JSVAL);
#endif
    



    TypeMap fullMap(NULL);
    fullMap.add(exit->stackTypeMap(), exit->numStackSlots);
    BuildGlobalTypeMapFromInnerTree(fullMap, exit);
    import(ti, inner_sp_ins, exit->numStackSlots, fullMap.length() - exit->numStackSlots,
           exit->calldepth, fullMap.data());

    
    if (callDepth > 0) {
        lir->insStorei(lirbuf->sp, lirbuf->state, offsetof(InterpState, sp));
        lir->insStorei(lirbuf->rp, lirbuf->state, offsetof(InterpState, rp));
    }

    



    VMSideExit* nested = snapshot(NESTED_EXIT);
    guard(true, lir->ins2(LIR_eq, ret, INS_CONSTPTR(exit)), nested);
    debug_only_printf(LC_TMTreeVis, "TREEVIS TREECALL INNER=%p EXIT=%p GUARD=%p\n", (void*)inner,
                      (void*)nested, (void*)exit);

    
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
    if (IsLoopEdge(pc, (jsbytecode*)fragment->root->ip)) {
        exitType = LOOP_EXIT;

        




        if ((*pc == JSOP_IFEQ || *pc == JSOP_IFEQX) == cond) {
            JS_ASSERT(*pc == JSOP_IFNE || *pc == JSOP_IFNEX || *pc == JSOP_IFEQ || *pc == JSOP_IFEQX);
            debug_only_print0(LC_TMTracer,
                              "Walking out of the loop, terminating it anyway.\n");
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
    if (IsLoopEdge(pc, (jsbytecode*)fragment->root->ip)) {
        





        if (loop) {
            JS_ASSERT(!cx->fp->imacpc && (pc == cx->fp->regs->pc || pc == cx->fp->regs->pc + 1));
            bool fused = pc != cx->fp->regs->pc;
            JSFrameRegs orig = *cx->fp->regs;

            cx->fp->regs->pc = (jsbytecode*)fragment->root->ip;
            cx->fp->regs->sp -= fused ? 2 : 1;

            TypeConsensus consensus;
            closeLoop(consensus);

            *cx->fp->regs = orig;
        } else {
            endLoop();
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
    int protoIndex = obj->lookupProperty(cx, id, &pobj, &prop);
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
                if (!scope->branded()) {
                    scope->brandingShapeChange(cx, sprop->slot, v);
                    scope->setBranded();
                }
            }
        }
    }

    pobj->dropProperty(cx, prop);
    return found;
}

JS_REQUIRES_STACK bool
TraceRecorder::hasIteratorMethod(JSObject* obj)
{
    JS_ASSERT(cx->fp->regs->sp + 2 <= cx->fp->slots + cx->fp->script->nslots);

    return hasMethod(obj, ATOM_TO_JSID(cx->runtime->atomState.iteratorAtom));
}

int
nanojit::StackFilter::getTop(LIns* guard)
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
            i->oprnd1() ? formatRef(i->oprnd1()) : "",
            (void *)x->pc,
            (void *)x->imacpc,
            (long int)x->sp_adj,
            (long int)x->rp_adj);
}
#endif

static JS_REQUIRES_STACK bool
DeleteRecorder(JSContext* cx)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    
    delete tm->recorder;
    tm->recorder = NULL;

    
    Assembler *assm = JS_TRACE_MONITOR(cx).assembler;
    if (assm->error() == OutOMem ||
        js_OverfullJITCache(tm, false)) {
        ResetJIT(cx);
        return false;
    }

    return true;
}


static JS_REQUIRES_STACK bool
CheckGlobalObjectShape(JSContext* cx, JSTraceMonitor* tm, JSObject* globalObj,
                       uint32 *shape = NULL, SlotList** slots = NULL)
{
    if (tm->needFlush) {
        ResetJIT(cx);
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
            debug_only_printf(LC_TMTracer,
                              "Global object/shape mismatch (%p/%u vs. %p/%u), flushing cache.\n",
                              (void*)globalObj, globalShape, (void*)root->globalObj,
                              root->globalShape);
            Backoff(cx, (jsbytecode*) root->ip);
            ResetJIT(cx);
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
    debug_only_printf(LC_TMTracer,
                      "No global slotlist for global shape %u, flushing cache.\n",
                      globalShape);
    ResetJIT(cx);
    return false;
}

static JS_REQUIRES_STACK bool
StartRecorder(JSContext* cx, VMSideExit* anchor, Fragment* f, TreeInfo* ti,
              unsigned stackSlots, unsigned ngslots, JSTraceType* typeMap,
              VMSideExit* expectedInnerExit, jsbytecode* outer, uint32 outerArgc)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (JS_TRACE_MONITOR(cx).needFlush) {
        ResetJIT(cx);
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

    
    Assembler *assm = JS_TRACE_MONITOR(cx).assembler;
    assm->setError(None);
    return true;
}

static void
TrashTree(JSContext* cx, Fragment* f)
{
    JS_ASSERT((!f->code()) == (!f->vmprivate));
    JS_ASSERT(f == f->root);
    debug_only_printf(LC_TMTreeVis, "TREEVIS TRASH FRAG=%p\n", (void*)f);
    if (!f->code())
        return;
    AUDIT(treesTrashed);
    debug_only_print0(LC_TMTracer, "Trashing tree info.\n");
    TreeInfo* ti = (TreeInfo*)f->vmprivate;
    f->vmprivate = NULL;
    f->setCode(NULL);
    Fragment** data = ti->dependentTrees.data();
    unsigned length = ti->dependentTrees.length();
    for (unsigned n = 0; n < length; ++n)
        TrashTree(cx, data[n]);
    data = ti->linkedTrees.data();
    length = ti->linkedTrees.length();
    for (unsigned n = 0; n < length; ++n)
        TrashTree(cx, data[n]);
}

static int
SynthesizeFrame(JSContext* cx, const FrameInfo& fi, JSObject* callee)
{
    VOUCH_DOES_NOT_REQUIRE_STACK();

    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, callee);
    JS_ASSERT(FUN_INTERPRETED(fun));

    
    JSStackFrame* fp = cx->fp;
    JS_ASSERT_IF(!fi.imacpc,
                 js_ReconstructStackDepth(cx, fp->script, fi.pc) ==
                 uintN(fi.spdist - fp->script->nfixed));

    uintN nframeslots = JS_HOWMANY(sizeof(JSInlineFrame), sizeof(jsval));
    JSScript* script = fun->u.i.script;
    size_t nbytes = (nframeslots + script->nslots) * sizeof(jsval);

    
    JSArena* a = cx->stackPool.current;
    void* newmark = (void*) a->avail;
    uintN argc = fi.get_argc();
    jsval* vp = fp->slots + fi.spdist - (2 + argc);
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
    newifp->frame.fun = fun;

    bool constructing = fi.is_constructing();
    newifp->frame.argc = argc;
    newifp->callerRegs.pc = fi.pc;
    newifp->callerRegs.sp = fp->slots + fi.spdist;
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
    newifp->frame.scopeChain = NULL; 
    newifp->frame.sharpDepth = 0;
    newifp->frame.sharpArray = NULL;
    newifp->frame.flags = constructing ? JSFRAME_CONSTRUCTING : 0;
    newifp->frame.dormantNext = NULL;
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

    



    JSInterpreterHook hook = cx->debugHooks->callHook;
    if (hook) {
        newifp->hookData = hook(cx, fp, JS_TRUE, 0, cx->debugHooks->callHookData);
    } else {
        newifp->hookData = NULL;
    }

    







    return (fi.spdist - fp->down->script->nfixed) +
           ((fun->nargs > fp->argc) ? fun->nargs - fp->argc : 0) +
           script->nfixed + 1;
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
    fp->script = NULL;
    
    fp->thisp = (JSObject *) cx->nativeVp[1];
    fp->argc = cx->nativeVpLen - 2;
    fp->argv = cx->nativeVp + 2;
    fp->fun = GET_FUNCTION_PRIVATE(cx, JSVAL_TO_OBJECT(fp->argv[-2]));
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
    fp->displaySave = NULL;

    ifp->mark = mark;
    cx->fp = fp;
}

static JS_REQUIRES_STACK bool
RecordTree(JSContext* cx, JSTraceMonitor* tm, Fragment* f, jsbytecode* outer,
           uint32 outerArgc, JSObject* globalObj, uint32 globalShape,
           SlotList* globalSlots, uint32 argc)
{
    JS_ASSERT(f->root == f);

    
    if (!CheckGlobalObjectShape(cx, tm, globalObj)) {
        Backoff(cx, (jsbytecode*) f->root->ip);
        return false;
    }

    AUDIT(recorderStarted);

    
    while (f->code() && f->peer)
        f = f->peer;
    if (f->code())
        f = getAnchor(&JS_TRACE_MONITOR(cx), f->root->ip, globalObj, globalShape, argc);

    if (!f) {
        ResetJIT(cx);
        return false;
    }

    f->root = f;
    f->lirbuf = tm->lirbuf;

    if (tm->allocator->outOfMemory() || js_OverfullJITCache(tm, false)) {
        Backoff(cx, (jsbytecode*) f->root->ip);
        ResetJIT(cx);
        debug_only_print0(LC_TMTracer,
                          "Out of memory recording new tree, flushing cache.\n");
        return false;
    }

    JS_ASSERT(!f->code() && !f->vmprivate);

    
    TreeInfo* ti = new (*tm->allocator) TreeInfo(tm->allocator, f, globalSlots);

    
    ti->typeMap.captureTypes(cx, globalObj, *globalSlots, 0 );
    ti->nStackTypes = ti->typeMap.length() - globalSlots->length();

#ifdef DEBUG
    AssertTreeIsUnique(tm, (VMFragment*)f, ti);
    ti->treeFileName = cx->fp->script->filename;
    ti->treeLineNumber = js_FramePCToLineNumber(cx, cx->fp);
    ti->treePCOffset = FramePCOffset(cx->fp);
#endif
#ifdef JS_JIT_SPEW
    debug_only_printf(LC_TMTreeVis, "TREEVIS CREATETREE ROOT=%p PC=%p FILE=\"%s\" LINE=%d OFFS=%d",
                      (void*)f, f->ip, ti->treeFileName, ti->treeLineNumber,
                      FramePCOffset(cx->fp));
    debug_only_print0(LC_TMTreeVis, " STACK=\"");
    for (unsigned i = 0; i < ti->nStackTypes; i++)
        debug_only_printf(LC_TMTreeVis, "%c", typeChar[ti->typeMap[i]]);
    debug_only_print0(LC_TMTreeVis, "\" GLOBALS=\"");
    for (unsigned i = 0; i < ti->nGlobalTypes(); i++)
        debug_only_printf(LC_TMTreeVis, "%c", typeChar[ti->typeMap[ti->nStackTypes + i]]);
    debug_only_print0(LC_TMTreeVis, "\"\n");
#endif

    
    unsigned entryNativeStackSlots = ti->nStackTypes;
    JS_ASSERT(entryNativeStackSlots == NativeStackSlots(cx, 0 ));
    ti->nativeStackBase = (entryNativeStackSlots -
            (cx->fp->regs->sp - StackBase(cx->fp))) * sizeof(double);
    ti->maxNativeStackSlots = entryNativeStackSlots;
    ti->maxCallDepth = 0;
    ti->script = cx->fp->script;

    
    if (!StartRecorder(cx, NULL, f, ti,
                       ti->nStackTypes,
                       ti->globalSlots->length(),
                       ti->typeMap.data(), NULL, outer, outerArgc)) {
        return false;
    }

    return true;
}

static JS_REQUIRES_STACK TypeConsensus
FindLoopEdgeTarget(JSContext* cx, VMSideExit* exit, VMFragment** peerp)
{
    VMFragment* from = exit->root();
    TreeInfo* from_ti = from->getTreeInfo();

    JS_ASSERT(from->code());

    TypeMap typeMap(NULL);
    FullMapFromExit(typeMap, exit);
    JS_ASSERT(typeMap.length() - exit->numStackSlots == from_ti->nGlobalTypes());

    
    for (unsigned i = 0; i < typeMap.length(); i++) {
        if (typeMap[i] == TT_DOUBLE)
            MarkSlotUndemotable(cx, from_ti, i);
    }

    VMFragment* firstPeer = (VMFragment*)from->first;
    for (VMFragment* peer = firstPeer; peer; peer = (VMFragment*)peer->peer) {
        TreeInfo* peer_ti = peer->getTreeInfo();
        if (!peer_ti)
            continue;
        JS_ASSERT(peer->argc == from->argc);
        JS_ASSERT(exit->numStackSlots == peer_ti->nStackTypes);
        TypeConsensus consensus = TypeMapLinkability(cx, typeMap, peer);
        if (consensus == TypeConsensus_Okay || consensus == TypeConsensus_Undemotes) {
            *peerp = peer;
            return consensus;
        }
    }

    return TypeConsensus_Bad;
}

UnstableExit*
TreeInfo::removeUnstableExit(VMSideExit* exit)
{
    
    UnstableExit** tail = &this->unstableExits;
    for (UnstableExit* uexit = this->unstableExits; uexit != NULL; uexit = uexit->next) {
        if (uexit->exit == exit) {
            *tail = uexit->next;
            return *tail;
        }
        tail = &uexit->next;
    }
    JS_NOT_REACHED("exit not in unstable exit list");
    return NULL;
}

static JS_REQUIRES_STACK bool
AttemptToStabilizeTree(JSContext* cx, JSObject* globalObj, VMSideExit* exit, jsbytecode* outer,
                       uint32 outerArgc)
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (tm->needFlush) {
        ResetJIT(cx);
        return false;
    }

    VMFragment* from = exit->root();
    TreeInfo* from_ti = from->getTreeInfo();

    VMFragment* peer = NULL;
    TypeConsensus consensus = FindLoopEdgeTarget(cx, exit, &peer);
    if (consensus == TypeConsensus_Okay) {
        TreeInfo* peer_ti = peer->getTreeInfo();
        JS_ASSERT(from_ti->globalSlots == peer_ti->globalSlots);
        JS_ASSERT(from_ti->nStackTypes == peer_ti->nStackTypes);
        
        JoinPeers(tm->assembler, exit, peer);
        



        if (peer_ti->nGlobalTypes() < peer_ti->globalSlots->length())
            SpecializeTreesToMissingGlobals(cx, globalObj, peer_ti);
        JS_ASSERT(from_ti->nGlobalTypes() == from_ti->globalSlots->length());
        
        from_ti->removeUnstableExit(exit);
        debug_only_stmt(DumpPeerStability(tm, peer->ip, from->globalObj, from->globalShape, from->argc);)
        return false;
    } else if (consensus == TypeConsensus_Undemotes) {
        
        TrashTree(cx, peer);
        return false;
    }

    return RecordTree(cx, tm, from->first, outer, outerArgc, from->globalObj,
                      from->globalShape, from_ti->globalSlots, cx->fp->argc);
}

static JS_REQUIRES_STACK bool
AttemptToExtendTree(JSContext* cx, VMSideExit* anchor, VMSideExit* exitedFrom, jsbytecode* outer
#ifdef MOZ_TRACEVIS
    , TraceVisStateObj* tvso = NULL
#endif
    )
{
    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (tm->needFlush) {
        ResetJIT(cx);
#ifdef MOZ_TRACEVIS
        if (tvso) tvso->r = R_FAIL_EXTEND_FLUSH;
#endif
        return false;
    }

    Fragment* f = anchor->root();
    JS_ASSERT(f->vmprivate);
    TreeInfo* ti = (TreeInfo*)f->vmprivate;

    



    if (ti->branchCount >= MAX_BRANCHES) {
#ifdef MOZ_TRACEVIS
        if (tvso) tvso->r = R_FAIL_EXTEND_MAX_BRANCHES;
#endif
        return false;
    }

    Fragment* c;
    if (!(c = anchor->target)) {
        Allocator& alloc = *JS_TRACE_MONITOR(cx).allocator;
        c = new (alloc) Fragment(cx->fp->regs->pc);
        c->kind = BranchTrace;
        c->anchor = anchor->from->anchor;
        c->root = anchor->from->root;
        debug_only_printf(LC_TMTreeVis, "TREEVIS CREATEBRANCH ROOT=%p FRAG=%p PC=%p FILE=\"%s\""
                          " LINE=%d ANCHOR=%p OFFS=%d\n",
                          (void*)f, (void*)c, (void*)cx->fp->regs->pc, cx->fp->script->filename,
                          js_FramePCToLineNumber(cx, cx->fp), (void*)anchor,
                          FramePCOffset(cx->fp));
        c->parent = f;
        anchor->target = c;
        c->root = f;
    }

    





    c->ip = cx->fp->regs->pc;

    debug_only_printf(LC_TMTracer,
                      "trying to attach another branch to the tree (hits = %d)\n", c->hits());

    int32_t& hits = c->hits();
    if (outer || (hits++ >= HOTEXIT && hits <= HOTEXIT+MAXEXIT)) {
        
        c->lirbuf = f->lirbuf;
        unsigned stackSlots;
        unsigned ngslots;
        JSTraceType* typeMap;
        TypeMap fullMap(NULL);
        if (exitedFrom == NULL) {
            



            ngslots = anchor->numGlobalSlots;
            stackSlots = anchor->numStackSlots;
            typeMap = anchor->fullTypeMap();
        } else {
            






            VMSideExit* e1 = anchor;
            VMSideExit* e2 = exitedFrom;
            fullMap.add(e1->stackTypeMap(), e1->numStackSlotsBelowCurrentFrame);
            fullMap.add(e2->stackTypeMap(), e2->numStackSlots);
            stackSlots = fullMap.length();
            ngslots = BuildGlobalTypeMapFromInnerTree(fullMap, e2);
            JS_ASSERT(ngslots >= e1->numGlobalSlots); 
            JS_ASSERT(ngslots == fullMap.length() - stackSlots);
            typeMap = fullMap.data();
        }
        JS_ASSERT(ngslots >= anchor->numGlobalSlots);
        bool rv = StartRecorder(cx, anchor, c, (TreeInfo*)f->vmprivate, stackSlots,
                                ngslots, typeMap, exitedFrom, outer, cx->fp->argc);
#ifdef MOZ_TRACEVIS
        if (!rv && tvso)
            tvso->r = R_FAIL_EXTEND_START;
#endif
        return rv;
    }
#ifdef MOZ_TRACEVIS
    if (tvso) tvso->r = R_FAIL_EXTEND_COLD;
#endif
    return false;
}

static JS_REQUIRES_STACK VMSideExit*
ExecuteTree(JSContext* cx, Fragment* f, uintN& inlineCallCount,
            VMSideExit** innermostNestedGuardp);

static JS_REQUIRES_STACK bool
RecordLoopEdge(JSContext* cx, TraceRecorder* r, uintN& inlineCallCount)
{
#ifdef JS_THREADSAFE
    if (OBJ_SCOPE(JS_GetGlobalForObject(cx, cx->fp->scopeChain))->title.ownercx != cx) {
        js_AbortRecording(cx, "Global object not owned by this context");
        return false; 
    }
#endif

    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    
    if (tm->needFlush) {
        ResetJIT(cx);
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
        debug_only_printf(LC_TMTracer,
                          "loop edge to %d, header %d\n",
                          cx->fp->regs->pc - cx->fp->script->code,
                          (jsbytecode*)r->getFragment()->root->ip - cx->fp->script->code);
        js_AbortRecording(cx, "Loop edge does not return to header");
        return false;
    }

    
    if (tm->reservedDoublePoolPtr < (tm->reservedDoublePool + MAX_NATIVE_STACK_SLOTS) &&
        !ReplenishReservedPool(cx, tm)) {
        js_AbortRecording(cx, "Couldn't call inner tree (out of memory)");
        return false;
    }

    



    JSObject* globalObj = JS_GetGlobalForObject(cx, cx->fp->scopeChain);
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;
    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots))
        return false;

    debug_only_printf(LC_TMTracer,
                      "Looking for type-compatible peer (%s:%d@%d)\n",
                      cx->fp->script->filename,
                      js_FramePCToLineNumber(cx, cx->fp),
                      FramePCOffset(cx->fp));

    
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
                ResetJIT(cx);
                return false;
            }
        }
        return RecordTree(cx, tm, f, outer, outerArgc, globalObj, globalShape, globalSlots, argc);
    }

    r->adjustCallerTypes(f);
    r->prepareTreeCall(f);
    VMSideExit* innermostNestedGuard = NULL;
    VMSideExit* lr = ExecuteTree(cx, f, inlineCallCount, &innermostNestedGuard);
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
            return AttemptToExtendTree(cx, innermostNestedGuard, lr, outer);
        }

        
        r->emitTreeCall(f, lr);
        return true;

      case UNSTABLE_LOOP_EXIT:
        
        js_AbortRecording(cx, "Inner tree is trying to stabilize, abort outer recording");
        return AttemptToStabilizeTree(cx, globalObj, lr, outer, outerFragment->argc);

      case OVERFLOW_EXIT:
        oracle.markInstructionUndemotable(cx->fp->regs->pc);
        
      case BRANCH_EXIT:
      case CASE_EXIT:
        
        js_AbortRecording(cx, "Inner tree is trying to grow, abort outer recording");
        return AttemptToExtendTree(cx, lr, NULL, outer);

      default:
        debug_only_printf(LC_TMTracer, "exit_type=%s\n", getExitName(lr->exitType));
        js_AbortRecording(cx, "Inner tree not suitable for calling");
        return false;
    }
}

static bool
IsEntryTypeCompatible(jsval* vp, JSTraceType* m)
{
    unsigned tag = JSVAL_TAG(*vp);

    debug_only_printf(LC_TMTracer, "%c/%c ", tagChar[tag], typeChar[*m]);

    switch (*m) {
      case TT_OBJECT:
        if (tag == JSVAL_OBJECT && !JSVAL_IS_NULL(*vp) &&
            !HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(*vp))) {
            return true;
        }
        debug_only_printf(LC_TMTracer, "object != tag%u ", tag);
        return false;
      case TT_INT32:
        jsint i;
        if (JSVAL_IS_INT(*vp))
            return true;
        if (tag == JSVAL_DOUBLE && JSDOUBLE_IS_INT(*JSVAL_TO_DOUBLE(*vp), i))
            return true;
        debug_only_printf(LC_TMTracer, "int != tag%u(value=%lu) ", tag, (unsigned long)*vp);
        return false;
      case TT_DOUBLE:
        if (JSVAL_IS_INT(*vp) || tag == JSVAL_DOUBLE)
            return true;
        debug_only_printf(LC_TMTracer, "double != tag%u ", tag);
        return false;
      case TT_JSVAL:
        JS_NOT_REACHED("shouldn't see jsval type in entry");
        return false;
      case TT_STRING:
        if (tag == JSVAL_STRING)
            return true;
        debug_only_printf(LC_TMTracer, "string != tag%u ", tag);
        return false;
      case TT_NULL:
        if (JSVAL_IS_NULL(*vp))
            return true;
        debug_only_printf(LC_TMTracer, "null != tag%u ", tag);
        return false;
      case TT_PSEUDOBOOLEAN:
        if (tag == JSVAL_SPECIAL)
            return true;
        debug_only_printf(LC_TMTracer, "bool != tag%u ", tag);
        return false;
      default:
        JS_ASSERT(*m == TT_FUNCTION);
        if (tag == JSVAL_OBJECT && !JSVAL_IS_NULL(*vp) &&
            HAS_FUNCTION_CLASS(JSVAL_TO_OBJECT(*vp))) {
            return true;
        }
        debug_only_printf(LC_TMTracer, "fun != tag%u ", tag);
        return false;
    }
}

class TypeCompatibilityVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    JSContext *mCx;
    JSTraceType *mTypeMap;
    unsigned mStackSlotNum;
    bool mOk;
public:
    TypeCompatibilityVisitor (TraceRecorder &recorder,
                              JSTraceType *typeMap) :
        mRecorder(recorder),
        mCx(mRecorder.cx),
        mTypeMap(typeMap),
        mStackSlotNum(0),
        mOk(true)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(jsval *vp, unsigned n, unsigned slot) {
        debug_only_printf(LC_TMTracer, "global%d=", n);
        if (!IsEntryTypeCompatible(vp, mTypeMap)) {
            mOk = false;
        } else if (!isPromoteInt(mRecorder.get(vp)) && *mTypeMap == TT_INT32) {
            oracle.markGlobalSlotUndemotable(mCx, slot);
            mOk = false;
        } else if (JSVAL_IS_INT(*vp) && *mTypeMap == TT_DOUBLE) {
            oracle.markGlobalSlotUndemotable(mCx, slot);
        }
        mTypeMap++;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), unsigned(i));
            if (!IsEntryTypeCompatible(vp, mTypeMap)) {
                mOk = false;
            } else if (!isPromoteInt(mRecorder.get(vp)) && *mTypeMap == TT_INT32) {
                oracle.markStackSlotUndemotable(mCx, mStackSlotNum);
                mOk = false;
            } else if (JSVAL_IS_INT(*vp) && *mTypeMap == TT_DOUBLE) {
                oracle.markStackSlotUndemotable(mCx, mStackSlotNum);
            }
            vp++;
            mTypeMap++;
            mStackSlotNum++;
        }
        return true;
    }

    bool isOk() {
        return mOk;
    }
};

JS_REQUIRES_STACK Fragment*
TraceRecorder::findNestedCompatiblePeer(Fragment* f)
{
    JSTraceMonitor* tm;

    tm = &JS_TRACE_MONITOR(cx);
    unsigned int ngslots = treeInfo->globalSlots->length();

    TreeInfo* ti;
    for (; f != NULL; f = f->peer) {
        if (!f->code())
            continue;

        ti = (TreeInfo*)f->vmprivate;

        debug_only_printf(LC_TMTracer, "checking nested types %p: ", (void*)f);

        if (ngslots > ti->nGlobalTypes())
            SpecializeTreesToMissingGlobals(cx, globalObj, ti);

        








        TypeCompatibilityVisitor visitor(*this, ti->typeMap.data());
        VisitSlots(visitor, cx, 0, *treeInfo->globalSlots);

        debug_only_printf(LC_TMTracer, " %s\n", visitor.isOk() ? "match" : "");
        if (visitor.isOk())
            return f;
    }

    return NULL;
}

class CheckEntryTypeVisitor : public SlotVisitorBase
{
    bool mOk;
    JSTraceType *mTypeMap;
public:
    CheckEntryTypeVisitor(JSTraceType *typeMap) :
        mOk(true),
        mTypeMap(typeMap)
    {}

    JS_ALWAYS_INLINE void checkSlot(jsval *vp, char const *name, int i) {
        debug_only_printf(LC_TMTracer, "%s%d=", name, i);
        JS_ASSERT(*(uint8_t*)mTypeMap != 0xCD);
        mOk = IsEntryTypeCompatible(vp, mTypeMap++);
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(jsval *vp, unsigned n, unsigned slot) {
        if (mOk)
            checkSlot(vp, "global", n);
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(jsval *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            if (!mOk)
                break;
            checkSlot(vp++, stackSlotKind(), i);
        }
        return mOk;
    }

    bool isOk() {
        return mOk;
    }
};








static JS_REQUIRES_STACK bool
CheckEntryTypes(JSContext* cx, JSObject* globalObj, TreeInfo* ti)
{
    unsigned int ngslots = ti->globalSlots->length();

    JS_ASSERT(ti->nStackTypes == NativeStackSlots(cx, 0));

    if (ngslots > ti->nGlobalTypes())
        SpecializeTreesToMissingGlobals(cx, globalObj, ti);

    JS_ASSERT(ti->typeMap.length() == NativeStackSlots(cx, 0) + ngslots);
    JS_ASSERT(ti->typeMap.length() == ti->nStackTypes + ngslots);
    JS_ASSERT(ti->nGlobalTypes() == ngslots);

    CheckEntryTypeVisitor visitor(ti->typeMap.data());
    VisitSlots(visitor, cx, 0, *ti->globalSlots);

    debug_only_print0(LC_TMTracer, "\n");
    return visitor.isOk();
}










static JS_REQUIRES_STACK Fragment*
FindVMCompatiblePeer(JSContext* cx, JSObject* globalObj, Fragment* f, uintN& count)
{
    count = 0;
    for (; f != NULL; f = f->peer) {
        if (f->vmprivate == NULL)
            continue;
        debug_only_printf(LC_TMTracer,
                          "checking vm types %p (ip: %p): ", (void*)f, f->ip);
        if (CheckEntryTypes(cx, globalObj, (TreeInfo*)f->vmprivate))
            return f;
        ++count;
    }
    return NULL;
}

static void
LeaveTree(InterpState&, VMSideExit* lr);

static JS_REQUIRES_STACK VMSideExit*
ExecuteTree(JSContext* cx, Fragment* f, uintN& inlineCallCount,
            VMSideExit** innermostNestedGuardp)
{
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_EXECUTE);
#endif

    JS_ASSERT(f->root == f && f->code() && f->vmprivate);

    















    JSObject* parent;
    JSObject* child = cx->fp->scopeChain;
    while ((parent = OBJ_GET_PARENT(cx, child)) != NULL) {
        if (!js_IsCacheableNonGlobalScope(child)) {
            debug_only_print0(LC_TMTracer,"Blacklist: non-cacheable object on scope chain.\n");
            Blacklist((jsbytecode*) f->root->ip);
            return NULL;
        }
        child = parent;
    }
    JSObject* globalObj = child;
    if (!(OBJ_GET_CLASS(cx, globalObj)->flags & JSCLASS_IS_GLOBAL)) {
        debug_only_print0(LC_TMTracer, "Blacklist: non-global at root of scope chain.\n");
        Blacklist((jsbytecode*) f->root->ip);
        return NULL;
    }

    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    TreeInfo* ti = (TreeInfo*)f->vmprivate;
    unsigned ngslots = ti->globalSlots->length();
    uint16* gslots = ti->globalSlots->data();
    unsigned globalFrameSize = STOBJ_NSLOTS(globalObj);

    
    JS_ASSERT_IF(ngslots != 0,
                 OBJ_SHAPE(JS_GetGlobalForObject(cx, cx->fp->scopeChain)) ==
                 ((VMFragment*)f)->globalShape);

    
    JS_ASSERT(tm->reservedDoublePoolPtr >= tm->reservedDoublePool + MAX_NATIVE_STACK_SLOTS);

    
    if (!js_ReserveObjects(cx, MAX_CALL_STACK_ENTRIES))
        return NULL;

    
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

    debug_only_stmt(*(uint64*)&global[globalFrameSize] = 0xdeadbeefdeadbeefLL;)
    debug_only_printf(LC_TMTracer,
                      "entering trace at %s:%u@%u, native stack slots: %u code: %p\n",
                      cx->fp->script->filename,
                      js_FramePCToLineNumber(cx, cx->fp),
                      FramePCOffset(cx->fp),
                      ti->maxNativeStackSlots,
                      f->code());

    JS_ASSERT(ti->nGlobalTypes() == ngslots);
    BuildNativeFrame(cx, globalObj, 0 , ngslots, gslots,
                     ti->typeMap.data(), global, stack_buffer);

    union { NIns *code; GuardRecord* (FASTCALL *func)(InterpState*, Fragment*); } u;
    u.code = f->code();

#ifdef EXECUTE_TREE_TIMER
    state->startTime = rdtsc();
#endif

    JS_ASSERT(!tm->tracecx);
    tm->tracecx = cx;
    state->prev = cx->interpState;
    cx->interpState = state;

    debug_only_stmt(fflush(NULL));
    GuardRecord* rec;

    
    
    {
#ifdef MOZ_TRACEVIS
        TraceVisStateObj tvso_n(cx, S_NATIVE);
#endif
#if defined(JS_NO_FASTCALL) && defined(NANOJIT_IA32)
        SIMULATE_FASTCALL(rec, state, NULL, u.func);
#else
        rec = u.func(state, NULL);
#endif
    }
    VMSideExit* lr = (VMSideExit*)rec->exit;

    AUDIT(traceTriggered);

    cx->interpState = state->prev;

    JS_ASSERT(!cx->bailExit);
    JS_ASSERT(lr->exitType != LOOP_EXIT || !lr->calldepth);
    tm->tracecx = NULL;
    LeaveTree(*state, lr);
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
            JS_ASSERT(FUN_SLOW_NATIVE(GET_FUNCTION_PRIVATE(cx, fp->callee())));
            JS_ASSERT(fp->regs == NULL);
            JS_ASSERT(fp->down->regs != &((JSInlineFrame *) fp)->callerRegs);
            cx->fp = fp->down;
            JS_ARENA_RELEASE(&cx->stackPool, ((JSInlineFrame *) fp)->mark);
        }
        JS_ASSERT(cx->fp->script);

        if (!(bs & JSBUILTIN_ERROR)) {
            











            JSFrameRegs* regs = cx->fp->regs;
            JSOp op = (JSOp) *regs->pc;
            JS_ASSERT(op == JSOP_CALL || op == JSOP_APPLY || op == JSOP_NEW ||
                      op == JSOP_GETELEM || op == JSOP_CALLELEM ||
                      op == JSOP_SETPROP || op == JSOP_SETNAME ||
                      op == JSOP_SETELEM || op == JSOP_INITELEM ||
                      op == JSOP_INSTANCEOF);
            const JSCodeSpec& cs = js_CodeSpec[op];
            regs->sp -= (cs.format & JOF_INVOKE) ? GET_ARGC(regs->pc) + 2 : cs.nuses;
            regs->sp += cs.ndefs;
            regs->pc += cs.length;
            JS_ASSERT_IF(!cx->fp->imacpc,
                         cx->fp->slots + cx->fp->script->nfixed +
                         js_ReconstructStackDepth(cx, cx->fp->script, regs->pc) ==
                         regs->sp);

            





            JS_ASSERT(state.deepBailSp >= state.stackBase && state.sp <= state.deepBailSp);

            





            JSTraceType* typeMap = innermost->stackTypeMap();
            for (int i = 1; i <= cs.ndefs; i++) {
                NativeToValue(cx,
                              regs->sp[-i],
                              typeMap[innermost->numStackSlots - i],
                              (jsdouble *) state.deepBailSp
                                  + innermost->sp_adj / sizeof(jsdouble) - i);
            }
        }
        return;
    }

    JS_ARENA_RELEASE(&cx->stackPool, state.stackMark);
    while (callstack < rp) {
        FrameInfo* fi = *callstack;
        
        JSObject* callee = *(JSObject**)&stack[fi->callerHeight];

        



        SynthesizeFrame(cx, *fi, callee);
        int slots = FlushNativeStackFrame(cx, 1 , (JSTraceType*)(fi + 1),
                                          stack, cx->fp);
#ifdef DEBUG
        JSStackFrame* fp = cx->fp;
        debug_only_printf(LC_TMTracer,
                          "synthesized deep frame for %s:%u@%u, slots=%d\n",
                          fp->script->filename,
                          js_FramePCToLineNumber(cx, fp),
                          FramePCOffset(fp),
                          slots);
#endif
        



        ++*state.inlineCallCountp;
        ++callstack;
        stack += slots;
    }

    




    JS_ASSERT(rp == callstack);
    unsigned calldepth = innermost->calldepth;
    unsigned calldepth_slots = 0;
    unsigned calleeOffset = 0;
    for (unsigned n = 0; n < calldepth; ++n) {
        
        calleeOffset += callstack[n]->callerHeight;
        JSObject* callee = *(JSObject**)&stack[calleeOffset];

        
        calldepth_slots += SynthesizeFrame(cx, *callstack[n], callee);
        ++*state.inlineCallCountp;
#ifdef DEBUG
        JSStackFrame* fp = cx->fp;
        debug_only_printf(LC_TMTracer,
                          "synthesized shallow frame for %s:%u@%u\n",
                          fp->script->filename, js_FramePCToLineNumber(cx, fp),
                          FramePCOffset(fp));
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

    debug_only_printf(LC_TMTracer,
                      "leaving trace at %s:%u@%u, op=%s, lr=%p, exitType=%s, sp=%d, "
                      "calldepth=%d, cycles=%llu\n",
                      fp->script->filename,
                      js_FramePCToLineNumber(cx, fp),
                      FramePCOffset(fp),
                      js_CodeName[fp->imacpc ? *fp->imacpc : *fp->regs->pc],
                      (void*)lr,
                      getExitName(lr->exitType),
                      fp->regs->sp - StackBase(fp),
                      calldepth,
                      cycles);

    






    TreeInfo* outermostTree = state.outermostTree;
    uint16* gslots = outermostTree->globalSlots->data();
    unsigned ngslots = outermostTree->globalSlots->length();
    JS_ASSERT(ngslots == outermostTree->nGlobalTypes());
    JSTraceType* globalTypeMap;

    
    Queue<JSTraceType> typeMap(0);
    if (innermost->numGlobalSlots == ngslots) {
        
        globalTypeMap = innermost->globalTypeMap();
    } else {
        






        JS_ASSERT(innermost->root()->getTreeInfo()->nGlobalTypes() == ngslots);
        JS_ASSERT(innermost->root()->getTreeInfo()->nGlobalTypes() > innermost->numGlobalSlots);
        typeMap.ensure(ngslots);
#ifdef DEBUG
        unsigned check_ngslots =
#endif
        BuildGlobalTypeMapFromInnerTree(typeMap, innermost);
        JS_ASSERT(check_ngslots == ngslots);
        globalTypeMap = typeMap.data();
    }

    
#ifdef DEBUG
    int slots =
#endif
        FlushNativeStackFrame(cx, innermost->calldepth,
                              innermost->stackTypeMap(),
                              stack, NULL);
    JS_ASSERT(unsigned(slots) == innermost->numStackSlots);

    if (innermost->nativeCalleeWord)
        SynthesizeSlowNativeFrame(cx, innermost);

    
    double* global = (double*)(&state + 1);
    FlushNativeGlobalFrame(cx, global,
                           ngslots, gslots, globalTypeMap);
    JS_ASSERT(*(uint64*)&global[STOBJ_NSLOTS(JS_GetGlobalForObject(cx, cx->fp->scopeChain))] ==
              0xdeadbeefdeadbeefLL);

    cx->nativeVp = NULL;

#ifdef DEBUG
    
    for (JSStackFrame* fp = cx->fp; fp; fp = fp->down) {
        JS_ASSERT_IF(fp->argv, JSVAL_IS_OBJECT(fp->argv[-1]));
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
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_MONITOR);
#endif

    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    
    if (tm->recorder) {
        jsbytecode* innerLoopHeaderPC = cx->fp->regs->pc;

        if (RecordLoopEdge(cx, tm->recorder, inlineCallCount))
            return true;

        










         if (innerLoopHeaderPC != cx->fp->regs->pc) {
#ifdef MOZ_TRACEVIS
             tvso.r = R_INNER_SIDE_EXIT;
#endif
             return false;
         }
    }
    JS_ASSERT(!tm->recorder);

    
    if (tm->reservedDoublePoolPtr < (tm->reservedDoublePool + MAX_NATIVE_STACK_SLOTS) &&
        !ReplenishReservedPool(cx, tm)) {
#ifdef MOZ_TRACEVIS
        tvso.r = R_DOUBLES;
#endif
        return false; 
    }

    



    JSObject* globalObj = JS_GetGlobalForObject(cx, cx->fp->scopeChain);
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;

    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots)) {
        Backoff(cx, cx->fp->regs->pc);
        return false;
    }

    
    if (cx->operationCallbackFlag) {
#ifdef MOZ_TRACEVIS
        tvso.r = R_CALLBACK_PENDING;
#endif
        return false;
    }

    jsbytecode* pc = cx->fp->regs->pc;
    uint32 argc = cx->fp->argc;

    Fragment* f = getLoop(tm, pc, globalObj, globalShape, argc);
    if (!f)
        f = getAnchor(tm, pc, globalObj, globalShape, argc);

    if (!f) {
        ResetJIT(cx);
#ifdef MOZ_TRACEVIS
        tvso.r = R_OOM_GETANCHOR;
#endif
        return false;
    }

    



    if (!f->code() && !f->peer) {
    record:
        if (++f->hits() < HOTLOOP) {
#ifdef MOZ_TRACEVIS
            tvso.r = f->hits() < 1 ? R_BACKED_OFF : R_COLD;
#endif
            return false;
        }

        




        bool rv = RecordTree(cx, tm, f->first, NULL, 0, globalObj, globalShape,
                             globalSlots, argc);
#ifdef MOZ_TRACEVIS
        if (!rv)
            tvso.r = R_FAIL_RECORD_TREE;
#endif
        return rv;
    }

    debug_only_printf(LC_TMTracer,
                      "Looking for compat peer %d@%d, from %p (ip: %p)\n",
                      js_FramePCToLineNumber(cx, cx->fp),
                      FramePCOffset(cx->fp), (void*)f, f->ip);

    uintN count;
    Fragment* match = FindVMCompatiblePeer(cx, globalObj, f, count);
    if (!match) {
        if (count < MAXPEERS)
            goto record;

        



        debug_only_print0(LC_TMTracer, "Blacklisted: too many peer trees.\n");
        Blacklist((jsbytecode*) f->root->ip);
#ifdef MOZ_TRACEVIS
        tvso.r = R_MAX_PEERS;
#endif
        return false;
    }

    VMSideExit* lr = NULL;
    VMSideExit* innermostNestedGuard = NULL;

    lr = ExecuteTree(cx, match, inlineCallCount, &innermostNestedGuard);
    if (!lr) {
#ifdef MOZ_TRACEVIS
        tvso.r = R_FAIL_EXECUTE_TREE;
#endif
        return false;
    }

    




    bool rv;
    switch (lr->exitType) {
      case UNSTABLE_LOOP_EXIT:
          rv = AttemptToStabilizeTree(cx, globalObj, lr, NULL, NULL);
#ifdef MOZ_TRACEVIS
          if (!rv)
              tvso.r = R_FAIL_STABILIZE;
#endif
          return rv;

      case OVERFLOW_EXIT:
        oracle.markInstructionUndemotable(cx->fp->regs->pc);
        
      case BRANCH_EXIT:
      case CASE_EXIT:
          return AttemptToExtendTree(cx, lr, NULL, NULL
#ifdef MOZ_TRACEVIS
                                          , &tvso
#endif
                 );

      case LOOP_EXIT:
        if (innermostNestedGuard)
            return AttemptToExtendTree(cx, innermostNestedGuard, lr, NULL
#ifdef MOZ_TRACEVIS
                                            , &tvso
#endif
                   );
#ifdef MOZ_TRACEVIS
        tvso.r = R_NO_EXTEND_OUTER;
#endif
        return false;

#ifdef MOZ_TRACEVIS
      case MISMATCH_EXIT:  tvso.r = R_MISMATCH_EXIT;  return false;
      case OOM_EXIT:       tvso.r = R_OOM_EXIT;       return false;
      case TIMEOUT_EXIT:   tvso.r = R_TIMEOUT_EXIT;   return false;
      case DEEP_BAIL_EXIT: tvso.r = R_DEEP_BAIL_EXIT; return false;
      case STATUS_EXIT:    tvso.r = R_STATUS_EXIT;    return false;
#endif

      default:
        



#ifdef MOZ_TRACEVIS
        tvso.r = R_OTHER_EXIT;
#endif
        return false;
    }
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::monitorRecording(JSContext* cx, TraceRecorder* tr, JSOp op)
{
    Assembler *assm = JS_TRACE_MONITOR(cx).assembler;

    
    if (JS_TRACE_MONITOR(cx).needFlush) {
        ResetJIT(cx);
        return JSRS_STOP;
    }
    if (tr->wasDeepAborted()) {
        js_AbortRecording(cx, "deep abort requested");
        return JSRS_STOP;
    }
    JS_ASSERT(!tr->fragment->lastIns);

    



    tr->pendingTraceableNative = NULL;
    tr->newobj_ins = NULL;

    
    if (tr->pendingGuardCondition) {
        tr->guard(true, tr->pendingGuardCondition, STATUS_EXIT);
        tr->pendingGuardCondition = NULL;
    }

    
    if (tr->pendingUnboxSlot) {
        LIns* val_ins = tr->get(tr->pendingUnboxSlot);
        val_ins = tr->unbox_jsval(*tr->pendingUnboxSlot, val_ins, tr->snapshot(BRANCH_EXIT));
        tr->set(tr->pendingUnboxSlot, val_ins);
        tr->pendingUnboxSlot = 0;
    }

    debug_only_stmt(
        if (js_LogController.lcbits & LC_TMRecorder) {
            js_Disassemble1(cx, cx->fp->script, cx->fp->regs->pc,
                            cx->fp->imacpc
                                ? 0 : cx->fp->regs->pc - cx->fp->script->code,
                            !cx->fp->imacpc, stdout);
        }
    )

    






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

    if (assm->error()) {
        js_AbortRecording(cx, "error during recording");
        return JSRS_STOP;
    }

    if (tr->traceMonitor->allocator->outOfMemory() ||
        js_OverfullJITCache(&JS_TRACE_MONITOR(cx), false)) {
        js_AbortRecording(cx, "no more memory");
        ResetJIT(cx);
        return JSRS_STOP;
    }

  imacro:
    if (!STATUS_ABORTS_RECORDING(status))
        return status;

  stop_recording:
    
    if (tr->fragment->lastIns) {
        DeleteRecorder(cx);
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
        DeleteRecorder(cx);
        return;
    }

    JS_ASSERT(!f->vmprivate);
#ifdef DEBUG
    TreeInfo* ti = tm->recorder->getTreeInfo();
    debug_only_printf(LC_TMAbort,
                      "Abort recording of tree %s:%d@%d at %s:%d@%d: %s.\n",
                      ti->treeFileName,
                      ti->treeLineNumber,
                      ti->treePCOffset,
                      cx->fp->script->filename,
                      js_FramePCToLineNumber(cx, cx->fp),
                      FramePCOffset(cx->fp),
                      reason);
#endif

    Backoff(cx, (jsbytecode*) f->root->ip, f->root);

    
    if (!DeleteRecorder(cx))
        return;

    



    if (!f->code() && (f->root == f))
        TrashTree(cx, f);
}

#if defined NANOJIT_IA32
static bool
CheckForSSE2()
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
    JS_ASSERT(tm->codeAlloc && tm->reCodeAlloc &&
              tm->allocator && tm->reAllocator);
    if (bytes > 1 G)
        bytes = 1 G;
    if (bytes < 128 K)
        bytes = 128 K;
    tm->maxCodeCacheBytes = bytes;
}

void
js_InitJIT(JSTraceMonitor *tm)
{
#if defined JS_JIT_SPEW
    
    if (!did_we_set_up_debug_logging) {
        InitJITLogController();
        did_we_set_up_debug_logging = true;
    }
#else
    memset(&js_LogController, 0, sizeof(js_LogController));
#endif

    if (!did_we_check_processor_features) {
#if defined NANOJIT_IA32
        avmplus::AvmCore::config.use_cmov =
            avmplus::AvmCore::config.sse2 = CheckForSSE2();
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

    if (!tm->allocator)
        tm->allocator = new VMAllocator();

    Allocator& alloc = *tm->allocator;

    if (!tm->codeAlloc)
        tm->codeAlloc = new CodeAlloc();

    if (!tm->assembler) {
        tm->assembler = new (alloc) Assembler(*tm->codeAlloc, alloc, core,
                                              &js_LogController);


        JS_ASSERT(!tm->reservedDoublePool);
        tm->lirbuf = new (alloc) LirBuffer(alloc);
#ifdef DEBUG
        tm->labels = new (alloc) LabelMap(alloc, &js_LogController);
        tm->lirbuf->names = new (alloc) LirNameMap(alloc, tm->labels);
#endif
        for (size_t i = 0; i < MONITOR_N_GLOBAL_STATES; ++i) {
            tm->globalStates[i].globalShape = -1;
            JS_ASSERT(!tm->globalStates[i].globalSlots);
            tm->globalStates[i].globalSlots = new (alloc) SlotList(tm->allocator);
        }
        tm->reservedDoublePoolPtr = tm->reservedDoublePool = new jsval[MAX_NATIVE_STACK_SLOTS];
        memset(tm->vmfragments, 0, sizeof(tm->vmfragments));
    }

    if (!tm->reAllocator)
        tm->reAllocator = new VMAllocator();

    Allocator& reAlloc = *tm->reAllocator;

    if (!tm->reCodeAlloc)
        tm->reCodeAlloc = new CodeAlloc();

    if (!tm->reAssembler) {
        tm->reAssembler = new (reAlloc) Assembler(*tm->reCodeAlloc, reAlloc, core,
                                                  &js_LogController);

        tm->reFragments = new (reAlloc) REHashMap(reAlloc);
        tm->reLirBuf = new (reAlloc) LirBuffer(reAlloc);
#ifdef DEBUG
        tm->reLabels = new (reAlloc) LabelMap(reAlloc, &js_LogController);
        tm->reLirBuf->names = new (reAlloc) LirNameMap(reAlloc, tm->reLabels);
#endif
    }
#if !defined XP_WIN
    debug_only(memset(&jitstats, 0, sizeof(jitstats)));
#endif
}

void
js_FinishJIT(JSTraceMonitor *tm)
{
#ifdef JS_JIT_SPEW
    if (jitstats.recorderStarted) {
        debug_only_printf(LC_TMStats,
                          "recorder: started(%llu), aborted(%llu), completed(%llu), different header(%llu), "
                          "trees trashed(%llu), slot promoted(%llu), unstable loop variable(%llu), "
                          "breaks(%llu), returns(%llu), unstableInnerCalls(%llu), blacklisted(%llu)\n",
                          jitstats.recorderStarted, jitstats.recorderAborted, jitstats.traceCompleted,
                          jitstats.returnToDifferentLoopHeader, jitstats.treesTrashed, jitstats.slotPromoted,
                          jitstats.unstableLoopVariable, jitstats.breakLoopExits, jitstats.returnLoopExits,
                          jitstats.noCompatInnerTrees, jitstats.blacklisted);
        debug_only_printf(LC_TMStats,
                          "monitor: triggered(%llu), exits(%llu), type mismatch(%llu), "
                          "global mismatch(%llu)\n", jitstats.traceTriggered, jitstats.sideExitIntoInterpreter,
                          jitstats.typeMapMismatchAtEntry, jitstats.globalShapeMismatchAtEntry);
    }
#endif
    if (tm->assembler != NULL) {
        JS_ASSERT(tm->reservedDoublePool);

        tm->lirbuf = NULL;

        if (tm->recordAttempts.ops)
            JS_DHashTableFinish(&tm->recordAttempts);

        memset(&tm->vmfragments[0], 0,
               FRAGMENT_TABLE_SIZE * sizeof(VMFragment*));

        delete[] tm->reservedDoublePool;
        tm->reservedDoublePool = tm->reservedDoublePoolPtr = NULL;
    }
    if (tm->reAssembler != NULL) {
        delete tm->reAllocator;
        delete tm->reCodeAlloc;
    }
    if (tm->codeAlloc)
        delete tm->codeAlloc;
    if (tm->allocator)
        delete tm->allocator;
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
PurgeScriptRecordingAttempts(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32 number, void *arg)
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
    debug_only_printf(LC_TMTracer,
                      "Purging fragments for JSScript %p.\n", (void*)script);

    JSTraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
        for (VMFragment **f = &(tm->vmfragments[i]); *f; ) {
            VMFragment* frag = *f;
            if (JS_UPTRDIFF(frag->ip, script->code) < script->length) {
                
                debug_only_printf(LC_TMTracer,
                                  "Disconnecting VMFragment %p "
                                  "with ip %p, in range [%p,%p).\n",
                                  (void*)frag, frag->ip, script->code,
                                  script->code + script->length);

                JS_ASSERT(frag->root == frag);
                VMFragment* next = frag->next;
                for (Fragment *p = frag; p; p = p->peer)
                    TrashTree(cx, p);
                *f = next;
            } else {
                f = &((*f)->next);
            }
        }
    }

    JS_DHashTableEnumerate(&(tm->recordAttempts), PurgeScriptRecordingAttempts, script);
}

bool
js_OverfullJITCache(JSTraceMonitor* tm, bool reCache)
{
    

































    jsuint maxsz = tm->maxCodeCacheBytes;
    VMAllocator *allocator = tm->allocator;
    CodeAlloc *codeAlloc = tm->codeAlloc;
    if (reCache) {
        





        maxsz /= 16;
        allocator = tm->reAllocator;
        codeAlloc = tm->reCodeAlloc;
    }
    return (codeAlloc->size() + allocator->size() > maxsz);
}

JS_FORCES_STACK JS_FRIEND_API(void)
js_DeepBail(JSContext *cx)
{
    JS_ASSERT(JS_ON_TRACE(cx));

    



    JSTraceMonitor *tm = &JS_TRACE_MONITOR(cx);
    JSContext *tracecx = tm->tracecx;

    
    JS_ASSERT(tracecx->bailExit);

    tm->tracecx = NULL;
    debug_only_print0(LC_TMTracer, "Deep bail.\n");
    LeaveTree(*tracecx->interpState, tracecx->bailExit);
    tracecx->bailExit = NULL;

    InterpState* state = tracecx->interpState;
    state->builtinStatus |= JSBUILTIN_BAILED;
    state->deepBailSp = state->sp;
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






JS_REQUIRES_STACK JSStackFrame*
TraceRecorder::frameIfInRange(JSObject* obj, unsigned* depthp) const
{
    JSStackFrame* ofp = (JSStackFrame*) obj->getPrivate();
    JSStackFrame* fp = cx->fp;
    for (unsigned depth = 0; depth <= callDepth; ++depth) {
        if (fp == ofp) {
            if (depthp)
                *depthp = depth;
            return ofp;
        }
        if (!(fp = fp->down))
            break;
    }
    return NULL;
}

JS_DEFINE_CALLINFO_6(extern, UINT32, GetClosureVar, CONTEXT, OBJECT, UINT32,
                     UINT32, UINT32, DOUBLEPTR, 0, 0)
JS_DEFINE_CALLINFO_6(extern, UINT32, GetClosureArg, CONTEXT, OBJECT, UINT32,
                     UINT32, UINT32, DOUBLEPTR, 0, 0)










JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::scopeChainProp(JSObject* obj, jsval*& vp, LIns*& ins, NameResult& nr)
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
            obj2->dropProperty(cx, prop);
            ABORT_TRACE("prototype property");
        }
        if (!isValidSlot(OBJ_SCOPE(obj), sprop)) {
            obj2->dropProperty(cx, prop);
            return JSRS_STOP;
        }
        if (!lazilyImportGlobalSlot(sprop->slot)) {
            obj2->dropProperty(cx, prop);
            ABORT_TRACE("lazy import of global slot failed");
        }
        vp = &STOBJ_GET_SLOT(obj, sprop->slot);
        ins = get(vp);
        obj2->dropProperty(cx, prop);
        nr.tracked = true;
        return JSRS_CONTINUE;
    }

    if (wasDeepAborted())
        ABORT_TRACE("deep abort from property lookup");

    if (obj == obj2 && OBJ_GET_CLASS(cx, obj) == &js_CallClass) {
        JSStackFrame* cfp = (JSStackFrame*) obj->getPrivate();
        if (cfp) {
            JSScopeProperty* sprop = (JSScopeProperty*) prop;

            uint32 setflags = (js_CodeSpec[*cx->fp->regs->pc].format & (JOF_SET | JOF_INCDEC | JOF_FOR));
            if (setflags && (sprop->attrs & JSPROP_READONLY))
                ABORT_TRACE("writing to a read-only property");

            uintN slot = sprop->shortid;

            vp = NULL;
            uintN upvar_slot = SPROP_INVALID_SLOT;
            if (sprop->getter == js_GetCallArg) {
                JS_ASSERT(slot < cfp->fun->nargs);
                vp = &cfp->argv[slot];
                upvar_slot = slot;
            } else if (sprop->getter == js_GetCallVar) {
                JS_ASSERT(slot < cfp->script->nslots);
                vp = &cfp->slots[slot];
                upvar_slot = cx->fp->fun->nargs + slot;
            }
            obj2->dropProperty(cx, prop);
            if (!vp)
                ABORT_TRACE("dynamic property of Call object");

            if (frameIfInRange(obj)) {
                
                
                ins = get(vp);
                nr.tracked = true;
                return JSRS_CONTINUE;
            }

            
            jsint scopeIndex = 0;
            JSObject* tmp = JSVAL_TO_OBJECT(cx->fp->argv[-2]);
            while (tmp != obj) {
                tmp = OBJ_GET_PARENT(cx, tmp);
                scopeIndex++;
            }
            JS_ASSERT(scopeIndex >= 1);

            LIns* callee_ins = get(&cx->fp->argv[-2]);
            LIns* outp = lir->insAlloc(sizeof(double));
            LIns* args[] = {
                outp,
                INS_CONST(callDepth),
                INS_CONST(slot),
                INS_CONST(scopeIndex),
                callee_ins,
                cx_ins
            };
            const CallInfo* ci;
            if (sprop->getter == js_GetCallArg)
                ci = &GetClosureArg_ci;
            else
                ci = &GetClosureVar_ci;

            LIns* call_ins = lir->insCall(ci, args);
            JSTraceType type = getCoercedType(*vp);
            guard(true,
                  addName(lir->ins2(LIR_eq, call_ins, lir->insImm(type)),
                          "guard(type-stable name access)"),
                  BRANCH_EXIT);
            ins = stackLoad(outp, type);
            nr.tracked = false;
            nr.obj = obj;
            nr.scopeIndex = scopeIndex;
            nr.sprop = sprop;
            return JSRS_CONTINUE;
        }
    }

    obj2->dropProperty(cx, prop);
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
    




    if (oracle.isInstructionUndemotable(cx->fp->regs->pc) || !isPromoteInt(s0) || !isPromoteInt(s1)) {
    out:
        if (v == LIR_fmod) {
            LIns* args[] = { s1, s0 };
            return lir->insCall(&js_dmod_ci, args);
        }
        LIns* result = lir->ins2(v, s0, s1);
        JS_ASSERT_IF(s0->isconstq() && s1->isconstq(), result->isconstq());
        return result;
    }

    jsdouble r;
    switch (v) {
    case LIR_fadd:
        r = v0 + v1;
        break;
    case LIR_fsub:
        r = v0 - v1;
        break;
    case LIR_fmul:
        r = v0 * v1;
        if (r == 0.0)
            goto out;
        break;
#ifdef NANOJIT_IA32
    case LIR_fdiv:
        if (v1 == 0)
            goto out;
        r = v0 / v1;
        break;
    case LIR_fmod:
        if (v0 < 0 || v1 == 0 || (s1->isconstq() && v1 < 0))
            goto out;
        r = js_dmod(v0, v1);
        break;
#endif
    default:
        goto out;
    }

    



    if (jsint(r) != r || JSDOUBLE_IS_NEGZERO(r))
        goto out;

    LIns* d0 = ::demote(lir, s0);
    LIns* d1 = ::demote(lir, s1);

    



    VMSideExit* exit;
    LIns* result;
    switch (v) {
#ifdef NANOJIT_IA32
      case LIR_fdiv:
        if (d0->isconst() && d1->isconst())
            return lir->ins1(LIR_i2f, lir->insImm(jsint(r)));

        exit = snapshot(OVERFLOW_EXIT);

        
        if (!d1->isconst())
            guard(false, lir->ins_eq0(d1), exit);
        result = lir->ins2(v = LIR_div, d0, d1);

        
        guard(true, lir->ins_eq0(lir->ins1(LIR_mod, result)), exit);

        
        guard(false, lir->ins_eq0(result), exit);
        break;

      case LIR_fmod: {
        if (d0->isconst() && d1->isconst())
            return lir->ins1(LIR_i2f, lir->insImm(jsint(r)));

        exit = snapshot(OVERFLOW_EXIT);

        
        if (!d1->isconst())
            guard(false, lir->ins_eq0(d1), exit);
        result = lir->ins1(v = LIR_mod, lir->ins2(LIR_div, d0, d1));

        
        LIns* branch = lir->insBranch(LIR_jf, lir->ins_eq0(result), NULL);

        



        guard(false, lir->ins2i(LIR_lt, d1, 0), exit);
        branch->setTarget(lir->ins0(LIR_label));
        break;
      }
#endif

      default:
        v = (LOpcode)((int)v & ~LIR64);
        result = lir->ins2(v, d0, d1);

        






        if (!result->isconst() && (!IsOverflowSafe(v, d0) || !IsOverflowSafe(v, d1))) {
            exit = snapshot(OVERFLOW_EXIT);
            guard(false, lir->ins1(LIR_ov, result), exit);
            if (v == LIR_mul) 
                guard(false, lir->ins_eq0(result), exit);
        }
        break;
    }
    JS_ASSERT_IF(d0->isconst() && d1->isconst(),
                 result->isconst() && result->imm32() == jsint(r));
    return lir->ins1(LIR_i2f, result);
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
    } else if (JSVAL_IS_SPECIAL(v)) {
        ci = &js_BooleanOrUndefinedToString_ci;
    } else {
        




        JS_ASSERT(JSVAL_IS_NULL(v));
        return INS_ATOM(cx->runtime->atomState.nullAtom);
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
    } else if (JSVAL_IS_SPECIAL(v)) {
        
        cond = JSVAL_TO_SPECIAL(v) == JS_TRUE;
        x = lir->ins2i(LIR_eq, v_ins, 1);
    } else if (isNumber(v)) {
        jsdouble d = asNumber(v);
        cond = !JSDOUBLE_IS_NaN(d) && d;
        x = lir->ins2(LIR_and,
                      lir->ins2(LIR_feq, v_ins, v_ins),
                      lir->ins_eq0(lir->ins2(LIR_feq, v_ins, lir->insImmq(0))));
    } else if (JSVAL_IS_STRING(v)) {
        cond = JSVAL_TO_STRING(v)->length() != 0;
        x = lir->ins2(LIR_piand,
                      lir->insLoad(LIR_ldp,
                                   v_ins,
                                   (int)offsetof(JSString, mLength)),
                      INS_CONSTWORD(JSString::LENGTH_MASK));
    } else {
        JS_NOT_REACHED("ifop");
        return JSRS_STOP;
    }

    jsbytecode* pc = cx->fp->regs->pc;
    emitIf(pc, cond, x);
    return checkTraceEnd(pc);
}

#ifdef NANOJIT_IA32





JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::tableswitch()
{
    jsval& v = stackval(-1);

    
    if (!isNumber(v))
        return JSRS_CONTINUE;

    
    LIns* v_ins = f2i(get(&v));
    if (v_ins->isconst() || v_ins->isconstq())
        return JSRS_CONTINUE;

    jsbytecode* pc = cx->fp->regs->pc;
    
    if (anchor &&
        (anchor->exitType == CASE_EXIT || anchor->exitType == DEFAULT_EXIT) &&
        fragment->ip == pc) {
        return JSRS_CONTINUE;
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

    



    if ((high + 1 - low) * sizeof(intptr_t*) + 128 > (unsigned) LARGEST_UNDERRUN_PROT)
        return switchop();

    
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
    LIns* guardIns = lir->insGuard(LIR_xtbl, diff, createGuardRecord(exit));
    fragment->lastIns = guardIns;
    compile(&JS_TRACE_MONITOR(cx));
    return JSRS_STOP;
}
#endif

static JS_ALWAYS_INLINE int32_t
UnboxBooleanOrUndefined(jsval v)
{
    
    JS_ASSERT(v == JSVAL_TRUE || v == JSVAL_FALSE || v == JSVAL_VOID);
    return JSVAL_TO_SPECIAL(v);
}

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
        LIns* args[] = { v_ins, INS_CONSTSTR(JSVAL_TO_STRING(v)) };
        guard(true,
              addName(lir->ins_eq0(lir->ins_eq0(lir->insCall(&js_EqualStrings_ci, args))),
                      "guard(switch on string)"),
              BRANCH_EXIT);
    } else if (JSVAL_IS_SPECIAL(v)) {
        guard(true,
              addName(lir->ins2(LIR_eq, v_ins, lir->insImm(UnboxBooleanOrUndefined(v))),
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
TraceRecorder::inc(jsval v, LIns*& v_ins, jsint incr, bool pre)
{
    LIns* v_after;
    CHECK_STATUS(incHelper(v, v_ins, v_after, incr));

    const JSCodeSpec& cs = js_CodeSpec[*cx->fp->regs->pc];
    JS_ASSERT(cs.ndefs == 1);
    stack(-cs.nuses, pre ? v_after : v_ins);
    v_ins = v_after;
    return JSRS_CONTINUE;
}




JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::incHelper(jsval v, LIns* v_ins, LIns*& v_after, jsint incr)
{
    if (!isNumber(v))
        ABORT_TRACE("can only inc numbers");
    v_after = alu(LIR_fadd, asNumber(v), incr, v_ins, lir->insImmf(incr));
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

    LIns* dslots_ins = NULL;
    stobj_set_slot(obj_ins, slot, dslots_ins, box_jsval(v, v_ins));
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
    lir->insStorei(box_jsval(*vp, v_ins), addr_ins, 0);
    return JSRS_CONTINUE;
}

static bool
EvalCmp(LOpcode op, double l, double r)
{
    bool cond;
    switch (op) {
      case LIR_feq:
        cond = (l == r);
        break;
      case LIR_flt:
        cond = l < r;
        break;
      case LIR_fgt:
        cond = l > r;
        break;
      case LIR_fle:
        cond = l <= r;
        break;
      case LIR_fge:
        cond = l >= r;
        break;
      default:
        JS_NOT_REACHED("unexpected comparison op");
        return false;
    }
    return cond;
}

static bool
EvalCmp(LOpcode op, JSString* l, JSString* r)
{
    if (op == LIR_feq)
        return js_EqualStrings(l, r);
    return EvalCmp(op, js_CompareStrings(l, r), 0);
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

    JSTraceType ltag = GetPromotedType(l);
    if (ltag != GetPromotedType(r)) {
        cond = !equal;
        x = lir->insImm(cond);
    } else if (ltag == TT_STRING) {
        LIns* args[] = { r_ins, l_ins };
        x = lir->ins2i(LIR_eq, lir->insCall(&js_EqualStrings_ci, args), equal);
        cond = js_EqualStrings(JSVAL_TO_STRING(l), JSVAL_TO_STRING(r));
    } else {
        LOpcode op = (ltag != TT_DOUBLE) ? LIR_eq : LIR_feq;
        x = lir->ins2(op, l_ins, r_ins);
        if (!equal)
            x = lir->ins_eq0(x);
        cond = (ltag == TT_DOUBLE)
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

    










    if (GetPromotedType(l) == GetPromotedType(r)) {
        if (JSVAL_TAG(l) == JSVAL_OBJECT || JSVAL_IS_SPECIAL(l)) {
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
    } else if (JSVAL_IS_NULL(l) && JSVAL_IS_SPECIAL(r)) {
        l_ins = lir->insImm(JSVAL_TO_SPECIAL(JSVAL_VOID));
        cond = (r == JSVAL_VOID);
    } else if (JSVAL_IS_SPECIAL(l) && JSVAL_IS_NULL(r)) {
        r_ins = lir->insImm(JSVAL_TO_SPECIAL(JSVAL_VOID));
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
        if (JSVAL_IS_SPECIAL(l)) {
            bool isVoid = JSVAL_IS_VOID(l);
            guard(isVoid,
                  lir->ins2(LIR_eq, l_ins, INS_CONST(JSVAL_TO_SPECIAL(JSVAL_VOID))),
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
        } else if (JSVAL_IS_SPECIAL(r)) {
            bool isVoid = JSVAL_IS_VOID(r);
            guard(isVoid,
                  lir->ins2(LIR_eq, r_ins, INS_CONST(JSVAL_TO_SPECIAL(JSVAL_VOID))),
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
        cond = EvalCmp(op, JSVAL_TO_STRING(l), JSVAL_TO_STRING(r));
        goto do_comparison;
    }

    
    if (!JSVAL_IS_NUMBER(l)) {
        LIns* args[] = { l_ins, cx_ins };
        switch (JSVAL_TAG(l)) {
          case JSVAL_SPECIAL:
            l_ins = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
            break;
          case JSVAL_STRING:
            l_ins = lir->insCall(&js_StringToNumber_ci, args);
            break;
          case JSVAL_OBJECT:
            if (JSVAL_IS_NULL(l)) {
                l_ins = lir->insImmf(0.0);
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
          case JSVAL_SPECIAL:
            r_ins = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
            break;
          case JSVAL_STRING:
            r_ins = lir->insCall(&js_StringToNumber_ci, args);
            break;
          case JSVAL_OBJECT:
            if (JSVAL_IS_NULL(r)) {
                r_ins = lir->insImmf(0.0);
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
    cond = EvalCmp(op, lnum, rnum);
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
        (op >= LIR_fsub && op <= LIR_fmod)) { 
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
    if (JSVAL_IS_SPECIAL(l)) {
        LIns* args[] = { a, cx_ins };
        a = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
        lnum = js_BooleanOrUndefinedToNumber(cx, JSVAL_TO_SPECIAL(l));
        leftIsNumber = true;
    }
    if (JSVAL_IS_SPECIAL(r)) {
        LIns* args[] = { b, cx_ins };
        b = lir->insCall(&js_BooleanOrUndefinedToNumber_ci, args);
        rnum = js_BooleanOrUndefinedToNumber(cx, JSVAL_TO_SPECIAL(r));
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

inline LIns*
TraceRecorder::map(LIns *obj_ins)
{
    return addName(lir->insLoad(LIR_ldp, obj_ins, (int) offsetof(JSObject, map)), "map");
}

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
TraceRecorder::guardNativePropertyOp(JSObject* aobj, LIns* map_ins)
{
    









    uint32 format = js_CodeSpec[*cx->fp->regs->pc].format;
    uint32 mode = JOF_MODE(format);

    
    JS_ASSERT(OBJ_IS_NATIVE(globalObj));
    if (aobj != globalObj) {
        size_t op_offset = offsetof(JSObjectOps, objectMap);
        if (mode == JOF_PROP || mode == JOF_VARPROP) {
            op_offset = (format & JOF_SET)
                        ? offsetof(JSObjectOps, setProperty)
                        : offsetof(JSObjectOps, getProperty);
        } else {
            JS_ASSERT(mode == JOF_NAME);
        }

        LIns* ops_ins;
        if (!map_is_native(aobj->map, map_ins, ops_ins, op_offset))
            ABORT_TRACE("non-native map");
    }
    return JSRS_CONTINUE;
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

    LIns* map_ins = map(obj_ins);

    CHECK_STATUS(guardNativePropertyOp(aobj, map_ins));

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
                    obj2->dropProperty(cx, prop);
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

        obj2->dropProperty(cx, prop);
        if (!entry)
            ABORT_TRACE("failed to fill property cache");
    }

    if (wasDeepAborted())
        ABORT_TRACE("deep abort from property lookup");

#ifdef JS_THREADSAFE
    
    
    
    
    JS_ASSERT(cx->requestDepth);
#endif

    return guardPropertyCacheHit(obj_ins, map_ins, aobj, obj2, entry, pcval);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::guardPropertyCacheHit(LIns* obj_ins,
                                     LIns* map_ins,
                                     JSObject* aobj,
                                     JSObject* obj2,
                                     JSPropCacheEntry* entry,
                                     jsuword& pcval)
{
    uint32 vshape = PCVCAP_SHAPE(entry->vcap);

    
    
    if (PCVCAP_TAG(entry->vcap) <= 1) {
        if (aobj != globalObj) {
            LIns* shape_ins = addName(lir->insLoad(LIR_ld, map_ins, offsetof(JSScope, shape)),
                                      "shape");
            guard(true,
                  addName(lir->ins2i(LIR_eq, shape_ins, entry->kshape), "guard_kshape"),
                  BRANCH_EXIT);
        }

        if (entry->adding()) {
            if (aobj == globalObj)
                ABORT_TRACE("adding a property to the global object");

            LIns *vshape_ins = addName(
                lir->insLoad(LIR_ld,
                             addName(lir->insLoad(LIR_ldp, cx_ins, offsetof(JSContext, runtime)),
                                     "runtime"),
                             offsetof(JSRuntime, protoHazardShape)),
                "protoHazardShape");
            guard(true,
                  addName(lir->ins2i(LIR_eq, vshape_ins, vshape), "guard_protoHazardShape"),
                  MISMATCH_EXIT);
        }
    } else {
#ifdef DEBUG
        JSOp op = js_GetOpcode(cx, cx->fp->script, cx->fp->regs->pc);
        JSAtom *pcatom;
        if (op == JSOP_LENGTH) {
            pcatom = cx->runtime->atomState.lengthAtom;
        } else {
            ptrdiff_t pcoff = (JOF_TYPE(js_CodeSpec[op].format) == JOF_SLOTATOM) ? SLOTNO_LEN : 0;
            GET_ATOM_FROM_BYTECODE(cx->fp->script, cx->fp->regs->pc, pcoff, pcatom);
        }
        JS_ASSERT(entry->kpc == (jsbytecode *) pcatom);
        JS_ASSERT(entry->kshape == jsuword(aobj));
#endif
        if (aobj != globalObj && !obj_ins->isconstp()) {
            guard(true,
                  addName(lir->ins2i(LIR_eq, obj_ins, entry->kshape), "guard_kobj"),
                  BRANCH_EXIT);
        }
    }

    
    
    if (PCVCAP_TAG(entry->vcap) >= 1) {
        JS_ASSERT(OBJ_SHAPE(obj2) == vshape);

        LIns* obj2_ins;
        if (PCVCAP_TAG(entry->vcap) == 1) {
            
            obj2_ins = addName(stobj_get_fslot(obj_ins, JSSLOT_PROTO), "proto");
            guard(false, lir->ins_eq0(obj2_ins), BRANCH_EXIT);
        } else {
            obj2_ins = INS_CONSTOBJ(obj2);
        }
        map_ins = map(obj2_ins);
        LIns* ops_ins;
        if (!map_is_native(obj2->map, map_ins, ops_ins))
            ABORT_TRACE("non-native map");

        LIns* shape_ins = addName(lir->insLoad(LIR_ld, map_ins, offsetof(JSScope, shape)),
                                  "obj2_shape");
        guard(true,
              addName(lir->ins2i(LIR_eq, shape_ins, vshape), "guard_vshape"),
              BRANCH_EXIT);
    }

    pcval = entry->vword;
    return JSRS_CONTINUE;
}

void
TraceRecorder::stobj_set_fslot(LIns *obj_ins, unsigned slot, LIns* v_ins)
{
    lir->insStorei(v_ins, obj_ins, offsetof(JSObject, fslots) + slot * sizeof(jsval));
}

void
TraceRecorder::stobj_set_dslot(LIns *obj_ins, unsigned slot, LIns*& dslots_ins, LIns* v_ins)
{
    if (!dslots_ins)
        dslots_ins = lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, dslots));
    lir->insStorei(v_ins, dslots_ins, slot * sizeof(jsval));
}

void
TraceRecorder::stobj_set_slot(LIns* obj_ins, unsigned slot, LIns*& dslots_ins, LIns* v_ins)
{
    if (slot < JS_INITIAL_NSLOTS) {
        stobj_set_fslot(obj_ins, slot, v_ins);
    } else {
        stobj_set_dslot(obj_ins, slot - JS_INITIAL_NSLOTS, dslots_ins, v_ins);
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
TraceRecorder::native_get(LIns* obj_ins, LIns* pobj_ins, JSScopeProperty* sprop,
                          LIns*& dslots_ins, LIns*& v_ins)
{
    if (!SPROP_HAS_STUB_GETTER(sprop))
        return JSRS_STOP;

    if (sprop->slot != SPROP_INVALID_SLOT)
        v_ins = stobj_get_slot(pobj_ins, sprop->slot, dslots_ins);
    else
        v_ins = INS_CONST(JSVAL_TO_SPECIAL(JSVAL_VOID));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::box_jsval(jsval v, LIns* v_ins)
{
    if (isNumber(v)) {
        LIns* args[] = { v_ins, cx_ins };
        v_ins = lir->insCall(&js_BoxDouble_ci, args);
        guard(false, lir->ins2(LIR_eq, v_ins, INS_CONST(JSVAL_ERROR_COOKIE)),
              OOM_EXIT);
        return v_ins;
    }
    switch (JSVAL_TAG(v)) {
      case JSVAL_SPECIAL:
        return lir->ins2i(LIR_pior, lir->ins2i(LIR_pilsh, v_ins, JSVAL_TAGBITS), JSVAL_SPECIAL);
      case JSVAL_OBJECT:
        return v_ins;
      default:
        JS_ASSERT(JSVAL_TAG(v) == JSVAL_STRING);
        return lir->ins2(LIR_pior, v_ins, INS_CONST(JSVAL_STRING));
    }
}

JS_REQUIRES_STACK LIns*
TraceRecorder::unbox_jsval(jsval v, LIns* v_ins, VMSideExit* exit)
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
        return lir->insCall(&js_UnboxDouble_ci, args);
    }
    switch (JSVAL_TAG(v)) {
      case JSVAL_SPECIAL:
        guard(true,
              lir->ins2i(LIR_eq,
                         lir->ins2(LIR_piand, v_ins, INS_CONST(JSVAL_TAGMASK)),
                         JSVAL_SPECIAL),
              exit);
        return lir->ins2i(LIR_ush, v_ins, JSVAL_TAGBITS);

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
        return v_ins;

      default:
        JS_ASSERT(JSVAL_TAG(v) == JSVAL_STRING);
        guard(true,
              lir->ins2i(LIR_eq,
                        lir->ins2(LIR_piand, v_ins, INS_CONST(JSVAL_TAGMASK)),
                        JSVAL_STRING),
              exit);
        return lir->ins2(LIR_piand, v_ins, INS_CONST(~JSVAL_TAGMASK));
    }
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::getThis(LIns*& this_ins)
{
    


    jsval original = JSVAL_NULL;
    if (cx->fp->argv) {
        original = cx->fp->argv[-1];
        if (!JSVAL_IS_PRIMITIVE(original) &&
            guardClass(JSVAL_TO_OBJECT(original), get(&cx->fp->argv[-1]), &js_WithClass, snapshot(MISMATCH_EXIT))) {
            ABORT_TRACE("can't trace getThis on With object");
        }
    }

    JSObject* thisObj = js_ComputeThisForFrame(cx, cx->fp);
    if (!thisObj)
        ABORT_TRACE_ERROR("js_ComputeThisForName failed");

    
    if (!cx->fp->callee()) {
        JS_ASSERT(callDepth == 0);
        this_ins = INS_CONSTOBJ(thisObj);

        



        return JSRS_CONTINUE;
    }

    jsval& thisv = cx->fp->argv[-1];
    JS_ASSERT(JSVAL_IS_OBJECT(thisv));

    







    JSClass* clasp = NULL;;
    if (JSVAL_IS_NULL(original) ||
        (((clasp = STOBJ_GET_CLASS(JSVAL_TO_OBJECT(original))) == &js_CallClass) ||
         (clasp == &js_BlockClass))) {
        if (clasp)
            guardClass(JSVAL_TO_OBJECT(original), get(&thisv), clasp, snapshot(BRANCH_EXIT));
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(thisv));
        if (thisObj != globalObj)
            ABORT_TRACE("global object was wrapped while recording");
        this_ins = INS_CONSTOBJ(thisObj);
        set(&thisv, this_ins);
        return JSRS_CONTINUE;
    }
    this_ins = get(&thisv);

    



    JSObject* obj = js_GetWrappedObject(cx, JSVAL_TO_OBJECT(thisv));
    JSObject* inner = obj;
    OBJ_TO_INNER_OBJECT(cx, inner);
    if (!obj)
        return JSRS_ERROR;

    JS_ASSERT(original == thisv ||
              original == OBJECT_TO_JSVAL(inner) ||
              original == OBJECT_TO_JSVAL(obj));

    



    LIns* is_inner = lir->ins2(LIR_eq, this_ins, INS_CONSTOBJ(inner));
    LIns* is_outer = lir->ins2(LIR_eq, this_ins, INS_CONSTOBJ(obj));
    LIns* wrapper = INS_CONSTOBJ(JSVAL_TO_OBJECT(thisv));

    this_ins = lir->ins_choose(is_inner,
                               wrapper,
                               lir->ins_choose(is_outer,
                                               wrapper,
                                               this_ins));

    return JSRS_CONTINUE;
}


LIns*
TraceRecorder::getStringLength(LIns* str_ins)
{
    LIns* len_ins = lir->insLoad(LIR_ldp, str_ins, (int)offsetof(JSString, mLength));

    LIns* masked_len_ins = lir->ins2(LIR_piand,
                                     len_ins,
                                     INS_CONSTWORD(JSString::LENGTH_MASK));

    return
        lir->ins_choose(lir->ins_eq0(lir->ins2(LIR_piand,
                                               len_ins,
                                               INS_CONSTWORD(JSString::DEPENDENT))),
                        masked_len_ins,
                        lir->ins_choose(lir->ins_eq0(lir->ins2(LIR_piand,
                                                               len_ins,
                                                               INS_CONSTWORD(JSString::PREFIX))),
                                        lir->ins2(LIR_piand,
                                                  len_ins,
                                                  INS_CONSTWORD(JSString::DEPENDENT_LENGTH_MASK)),
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

JS_REQUIRES_STACK bool
TraceRecorder::guardHasPrototype(JSObject* obj, LIns* obj_ins,
                                 JSObject** pobj, LIns** pobj_ins,
                                 VMSideExit* exit)
{
    *pobj = JSVAL_TO_OBJECT(obj->fslots[JSSLOT_PROTO]);
    *pobj_ins = stobj_get_fslot(obj_ins, JSSLOT_PROTO);

    bool cond = *pobj == NULL;
    guard(cond, addName(lir->ins_eq0(*pobj_ins), "guard(proto-not-null)"), exit);
    return !cond;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::guardPrototypeHasNoIndexedProperties(JSObject* obj, LIns* obj_ins, ExitType exitType)
{
    



    VMSideExit* exit = snapshot(exitType);

    if (js_PrototypeHasIndexedProperties(cx, obj))
        return JSRS_STOP;

    while (guardHasPrototype(obj, obj_ins, &obj, &obj_ins, exit)) {
        LIns* map_ins = map(obj_ins);
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
    guard(false, lir->ins2(LIR_eq, obj_ins, INS_CONSTOBJ(globalObj)), MISMATCH_EXIT);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK void
TraceRecorder::clearFrameSlotsFromCache()
{
    






    JSStackFrame* fp = cx->fp;
    jsval* vp;
    jsval* vpstop;

    




    if (fp->argv) {
        vp = &fp->argv[-2];
        vpstop = &fp->argv[argSlots(fp)];
        while (vp < vpstop)
            nativeFrameTracker.set(vp++, (LIns*)0);
        nativeFrameTracker.set(&fp->argsobj, (LIns*)0);
    }
    vp = &fp->slots[0];
    vpstop = &fp->slots[fp->script->nslots];
    while (vp < vpstop)
        nativeFrameTracker.set(vp++, (LIns*)0);
}






JS_REQUIRES_STACK void
TraceRecorder::putArguments()
{
    if (cx->fp->argsobj && cx->fp->argc) {
        LIns* argsobj_ins = get(&cx->fp->argsobj);
        LIns* args_ins = lir->insAlloc(sizeof(jsval) * cx->fp->argc);
        for (uintN i = 0; i < cx->fp->argc; ++i) {
            LIns* arg_ins = box_jsval(cx->fp->argv[i], get(&cx->fp->argv[i]));
            lir->insStorei(arg_ins, args_ins, i * sizeof(jsval));
        }
        LIns* args[] = { args_ins, argsobj_ins, cx_ins };
        lir->insCall(&js_PutArguments_ci, args);
    }
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_EnterFrame()
{
    JSStackFrame* fp = cx->fp;

    if (++callDepth >= MAX_CALLDEPTH)
        ABORT_TRACE("exceeded maximum call depth");

    
    
    if (fp->script == fp->down->script && fp->down->down && fp->down->down->script == fp->script)
        ABORT_TRACE("recursive call");

    debug_only_printf(LC_TMTracer, "EnterFrame %s, callDepth=%d\n",
                      js_AtomToPrintableString(cx, cx->fp->fun->atom),
                      callDepth);
    debug_only_stmt(
        if (js_LogController.lcbits & LC_TMRecorder) {
            js_Disassemble(cx, cx->fp->script, JS_TRUE, stdout);
            debug_only_print0(LC_TMTracer, "----\n");
        }
    )
    LIns* void_ins = INS_VOID();

    
    
    
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
    set(&fp->argsobj, INS_NULL(), true);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_LeaveFrame()
{
    debug_only_stmt(
        if (cx->fp->fun)
            debug_only_printf(LC_TMTracer,
                              "LeaveFrame (back to %s), callDepth=%d\n",
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
    stack(0, INS_CONST(JSVAL_TO_SPECIAL(JSVAL_VOID)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_POPV()
{
    jsval& rval = stackval(-1);
    LIns *rval_ins = box_jsval(rval, get(&rval));

    
    
    
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
        endLoop();
        return JSRS_STOP;
    }

    putArguments();

    
    jsval& rval = stackval(-1);
    JSStackFrame *fp = cx->fp;
    if ((cx->fp->flags & JSFRAME_CONSTRUCTING) && JSVAL_IS_PRIMITIVE(rval)) {
        JS_ASSERT(OBJECT_TO_JSVAL(fp->thisp) == fp->argv[-1]);
        rval_ins = get(&fp->argv[-1]);
    } else {
        rval_ins = get(&rval);
    }
    debug_only_printf(LC_TMTracer,
                      "returning from %s\n",
                      js_AtomToPrintableString(cx, cx->fp->fun->atom));
    clearFrameSlotsFromCache();

    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GOTO()
{
    




    jssrcnote* sn = js_GetSrcNote(cx->fp->script, cx->fp->regs->pc);

    if (sn && (SN_TYPE(sn) == SRC_BREAK || SN_TYPE(sn) == SRC_CONT2LABEL)) {
        AUDIT(breakLoopExits);
        endLoop();
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
    if (cx->fp->flags & JSFRAME_OVERRIDE_ARGS)
        ABORT_TRACE("Can't trace |arguments| if |arguments| is assigned to");

    LIns* global_ins = INS_CONSTOBJ(globalObj);
    LIns* argc_ins = INS_CONST(cx->fp->argc);
    LIns* callee_ins = get(&cx->fp->argv[-2]);
    LIns* a_ins = get(&cx->fp->argsobj);

    
    LIns* args[] = { a_ins, callee_ins, argc_ins, global_ins, cx_ins };
    a_ins = lir->insCall(&js_Arguments_ci, args);
    guard(false, lir->ins_eq0(a_ins), OOM_EXIT);
    stack(0, a_ins);
    set(&cx->fp->argsobj, a_ins);
    return JSRS_CONTINUE;
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
    return binary(LIR_fmod);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_NOT()
{
    jsval& v = stackval(-1);
    if (JSVAL_IS_SPECIAL(v)) {
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
                                   lir->insLoad(LIR_ldp, get(&v), (int)offsetof(JSString, mLength)),
                                   INS_CONSTWORD(JSString::LENGTH_MASK))));
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

        




        if (!oracle.isInstructionUndemotable(cx->fp->regs->pc) &&
            isPromoteInt(a) &&
            (!JSVAL_IS_INT(v) || JSVAL_TO_INT(v) != 0) &&
            (!JSVAL_IS_DOUBLE(v) || !JSDOUBLE_IS_NEGZERO(*JSVAL_TO_DOUBLE(v))) &&
            -asNumber(v) == (int)-asNumber(v)) {
            a = lir->ins1(LIR_neg, ::demote(lir, a));
            if (!a->isconst()) {
                VMSideExit* exit = snapshot(OVERFLOW_EXIT);
                guard(false, lir->ins1(LIR_ov, a), exit);
                guard(false, lir->ins2i(LIR_eq, a, 0), exit);
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

    JS_ASSERT(JSVAL_TAG(v) == JSVAL_STRING || JSVAL_IS_SPECIAL(v));

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

    JS_ASSERT(JSVAL_TAG(v) == JSVAL_STRING || JSVAL_IS_SPECIAL(v));

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

JSRecordingStatus
TraceRecorder::getClassPrototype(JSObject* ctor, LIns*& proto_ins)
{
    jsval pval;

    if (!ctor->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &pval))
        ABORT_TRACE_ERROR("error getting prototype from constructor");
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
    proto_ins = INS_CONSTOBJ(JSVAL_TO_OBJECT(pval));
    return JSRS_CONTINUE;
}

JSRecordingStatus
TraceRecorder::getClassPrototype(JSProtoKey key, LIns*& proto_ins)
{
    JSObject* proto;
    if (!js_GetClassPrototype(cx, globalObj, INT_TO_JSID(key), &proto))
        ABORT_TRACE_ERROR("error in js_GetClassPrototype");
    proto_ins = INS_CONSTOBJ(proto);
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
        VMAllocator *alloc = traceMonitor->allocator;
        for (uint32 i = 0; i < argc && !alloc->outOfMemory(); i++) {
            LIns *elt_ins = box_jsval(argv[i], get(&argv[i]));
            stobj_set_dslot(arr_ins, i, dslots_ins, elt_ins);
        }

        if (argc > 0)
            stobj_set_fslot(arr_ins, JSSLOT_ARRAY_COUNT, INS_CONST(argc));
    }

    set(rval, arr_ins);
    pendingTraceableNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK void
TraceRecorder::propagateFailureToBuiltinStatus(LIns* ok_ins, LIns*& status_ins)
{
    









    JS_STATIC_ASSERT(((JS_TRUE & 1) ^ 1) << 1 == 0);
    JS_STATIC_ASSERT(((JS_FALSE & 1) ^ 1) << 1 == JSBUILTIN_ERROR);
    status_ins = lir->ins2(LIR_or,
                           status_ins,
                           lir->ins2i(LIR_lsh,
                                      lir->ins2i(LIR_xor,
                                                 lir->ins2i(LIR_and, ok_ins, 1),
                                                 1),
                                      1));
    lir->insStorei(status_ins, lirbuf->state, (int) offsetof(InterpState, builtinStatus));
}

JS_REQUIRES_STACK void
TraceRecorder::emitNativePropertyOp(JSScope* scope, JSScopeProperty* sprop, LIns* obj_ins,
                                    bool setflag, LIns* boxed_ins)
{
    JS_ASSERT(!(sprop->attrs & (setflag ? JSPROP_SETTER : JSPROP_GETTER)));
    JS_ASSERT(setflag ? !SPROP_HAS_STUB_SETTER(sprop) : !SPROP_HAS_STUB_GETTER(sprop));

    enterDeepBailCall();

    
    
    
    LIns* vp_ins = lir->insAlloc(sizeof(jsval));
    lir->insStorei(vp_ins, cx_ins, offsetof(JSContext, nativeVp));
    lir->insStorei(INS_CONST(1), cx_ins, offsetof(JSContext, nativeVpLen));
    if (setflag)
        lir->insStorei(boxed_ins, vp_ins, 0);

    CallInfo* ci = (CallInfo*) lir->insSkip(sizeof(struct CallInfo))->payload();
    ci->_address = uintptr_t(setflag ? sprop->setter : sprop->getter);
    ci->_argtypes = ARGSIZE_LO << (0*ARGSIZE_SHIFT) |
                    ARGSIZE_LO << (1*ARGSIZE_SHIFT) |
                    ARGSIZE_LO << (2*ARGSIZE_SHIFT) |
                    ARGSIZE_LO << (3*ARGSIZE_SHIFT) |
                    ARGSIZE_LO << (4*ARGSIZE_SHIFT);
    ci->_cse = ci->_fold = 0;
    ci->_abi = ABI_CDECL;
#ifdef DEBUG
    ci->_name = "JSPropertyOp";
#endif
    LIns* args[] = { vp_ins, INS_CONSTWORD(SPROP_USERID(sprop)), obj_ins, cx_ins };
    LIns* ok_ins = lir->insCall(ci, args);

    
    lir->insStorei(INS_NULL(), cx_ins, offsetof(JSContext, nativeVp));
    leaveDeepBailCall();

    
    
    
    
    LIns* status_ins = lir->insLoad(LIR_ld,
                                    lirbuf->state,
                                    (int) offsetof(InterpState, builtinStatus));
    propagateFailureToBuiltinStatus(ok_ins, status_ins);
    guard(true, lir->ins_eq0(status_ins), STATUS_EXIT);

    
    
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::emitNativeCall(JSTraceableNative* known, uintN argc, LIns* args[])
{
    bool constructing = known->flags & JSTN_CONSTRUCTOR;

    if (JSTN_ERRTYPE(known) == FAIL_STATUS) {
        
        
        JS_ASSERT(!pendingTraceableNative);

        
        
        
        VMSideExit* exit = snapshot(DEEP_BAIL_EXIT);
        JSObject* funobj = JSVAL_TO_OBJECT(stackval(0 - (2 + argc)));
        if (FUN_SLOW_NATIVE(GET_FUNCTION_PRIVATE(cx, funobj))) {
            exit->setNativeCallee(funobj, constructing);
            treeInfo->gcthings.addUnique(OBJECT_TO_JSVAL(funobj));
        }
        lir->insStorei(INS_CONSTPTR(exit), cx_ins, offsetof(JSContext, bailExit));

        
        LIns* guardRec = createGuardRecord(exit);
        lir->insGuard(LIR_xbarrier, NULL, guardRec);
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
        guard(false, lir->ins2i(LIR_eq, res_ins, JSVAL_TO_SPECIAL(JSVAL_VOID)), OOM_EXIT);
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

#if defined DEBUG
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
                *argp = INS_CONSTOBJ(JSVAL_TO_OBJECT(fval));
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
                *argp = box_jsval(arg, *argp);
            } else {
                goto next_specialization;
            }
            argp--;
        }
#if defined DEBUG
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
    JSFastNative native = (JSFastNative)fun->u.n.native;

    switch (argc) {
      case 1:
        if (native == js_math_ceil || native == js_math_floor || native == js_math_round) {
            LIns* a = get(&vp[2]);
            if (isPromote(a)) {
                set(&vp[0], a);
                pendingTraceableNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                return JSRS_CONTINUE;
            }
        }
        break;
      case 2:
        if (native == js_math_min || native == js_math_max) {
            LIns* a = get(&vp[2]);
            LIns* b = get(&vp[3]);
            if (isPromote(a) && isPromote(b)) {
                a = ::demote(lir, a);
                b = ::demote(lir, b);
                set(&vp[0],
                    lir->ins1(LIR_i2f,
                              lir->ins_choose(lir->ins2((native == js_math_min)
                                                        ? LIR_lt
                                                        : LIR_gt, a, b),
                                              a, b)));
                pendingTraceableNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                return JSRS_CONTINUE;
            }
        }
        break;
    }

    if (fun->flags & JSFUN_TRACEABLE) {
        JSRecordingStatus status;
        if ((status = callTraceableNative(fun, argc, mode == JSOP_NEW)) != JSRS_STOP)
            return status;
    }

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

        args[0] = INS_CONSTOBJ(funobj);
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
                this_ins = INS_CONSTOBJ(thisObj);
            } else if (!JSVAL_IS_OBJECT(vp[1])) {
                ABORT_TRACE("slow native(primitive, args)");
            } else {
                if (guardClass(JSVAL_TO_OBJECT(vp[1]), this_ins, &js_WithClass, snapshot(MISMATCH_EXIT)))
                    ABORT_TRACE("can't trace slow native invocation on With object");

                this_ins = lir->ins_choose(lir->ins_eq0(stobj_get_fslot(this_ins, JSSLOT_PARENT)),
                                           INS_CONSTOBJ(globalObj),
                                           this_ins);
            }
        }
        this_ins = box_jsval(vp[1], this_ins);
    }
    lir->insStorei(this_ins, invokevp_ins, 1 * sizeof(jsval));

    VMAllocator *alloc = traceMonitor->allocator;
    
    for (uintN n = 2; n < 2 + argc; n++) {
        LIns* i = box_jsval(vp[n], get(&vp[n]));
        lir->insStorei(i, invokevp_ins, n * sizeof(jsval));

        
        
        if (alloc->outOfMemory())
            ABORT_TRACE("out of memory in argument list");
    }

    
    if (2 + argc < vplen) {
        LIns* undef_ins = INS_CONSTWORD(JSVAL_VOID);
        for (uintN n = 2 + argc; n < vplen; n++) {
            lir->insStorei(undef_ins, invokevp_ins, n * sizeof(jsval));

            if (alloc->outOfMemory())
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
        types = ARGSIZE_LO << (0*ARGSIZE_SHIFT) |
                ARGSIZE_LO << (1*ARGSIZE_SHIFT) |
                ARGSIZE_LO << (2*ARGSIZE_SHIFT) |
                ARGSIZE_LO << (3*ARGSIZE_SHIFT);
    } else {
        native_rval_ins = lir->ins2i(LIR_piadd, invokevp_ins, int32_t((vplen - 1) * sizeof(jsval)));
        args[0] = native_rval_ins;
        args[1] = lir->ins2i(LIR_piadd, invokevp_ins, int32_t(2 * sizeof(jsval)));
        args[2] = lir->insImm(argc);
        args[3] = this_ins;
        args[4] = cx_ins;
        types = ARGSIZE_LO << (0*ARGSIZE_SHIFT) |
                ARGSIZE_LO << (1*ARGSIZE_SHIFT) |
                ARGSIZE_LO << (2*ARGSIZE_SHIFT) |
                ARGSIZE_LO << (3*ARGSIZE_SHIFT) |
                ARGSIZE_LO << (4*ARGSIZE_SHIFT) |
                ARGSIZE_LO << (5*ARGSIZE_SHIFT);
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

    
    lir->insStorei(INS_NULL(), cx_ins, offsetof(JSContext, nativeVp));
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
        type = INS_ATOM(cx->runtime->atomState.typeAtoms[JSTYPE_STRING]);
    } else if (isNumber(r)) {
        type = INS_ATOM(cx->runtime->atomState.typeAtoms[JSTYPE_NUMBER]);
    } else if (VALUE_IS_FUNCTION(cx, r)) {
        type = INS_ATOM(cx->runtime->atomState.typeAtoms[JSTYPE_FUNCTION]);
    } else {
        LIns* args[] = { get(&r), cx_ins };
        if (JSVAL_IS_SPECIAL(r)) {
            
            
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
    stack(-1, INS_CONST(JSVAL_TO_SPECIAL(JSVAL_VOID)));
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
    LIns* v_ins;
    LIns* v_after;
    NameResult nr;

    CHECK_STATUS(name(vp, v_ins, nr));
    CHECK_STATUS(incHelper(*vp, v_ins, v_after, incr));
    LIns* v_result = pre ? v_after : v_ins;
    if (nr.tracked) {
        set(vp, v_after);
        stack(0, v_result);
        return JSRS_CONTINUE;
    }

    if (OBJ_GET_CLASS(cx, nr.obj) != &js_CallClass)
        ABORT_TRACE("incName on unsupported object class");
    LIns* callobj_ins = get(&cx->fp->argv[-2]);
    for (jsint i = 0; i < nr.scopeIndex; ++i)
        callobj_ins = stobj_get_parent(callobj_ins);
    CHECK_STATUS(setCallProp(nr.obj, callobj_ins, nr.sprop, v_after, *vp));
    stack(0, v_result);
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
TraceRecorder::nativeSet(JSObject* obj, LIns* obj_ins, JSScopeProperty* sprop,
                         jsval v, LIns* v_ins)
{
    JSScope* scope = OBJ_SCOPE(obj);
    uint32 slot = sprop->slot;

    

















    JS_ASSERT(SPROP_HAS_STUB_SETTER(sprop) || slot == SPROP_INVALID_SLOT);

    
    LIns* boxed_ins = NULL;
    if (!SPROP_HAS_STUB_SETTER(sprop) || (slot != SPROP_INVALID_SLOT && obj != globalObj))
        boxed_ins = box_jsval(v, v_ins);

    
    if (!SPROP_HAS_STUB_SETTER(sprop))
        emitNativePropertyOp(scope, sprop, obj_ins, true, boxed_ins);

    
    if (slot != SPROP_INVALID_SLOT) {
        JS_ASSERT(SPROP_HAS_VALID_SLOT(sprop, scope));
        JS_ASSERT(!(sprop->attrs & JSPROP_SHARED));
        if (obj == globalObj) {
            if (!lazilyImportGlobalSlot(slot))
                ABORT_TRACE("lazy import of global slot failed");
            set(&STOBJ_GET_SLOT(obj, slot), v_ins);
        } else {
            LIns* dslots_ins = NULL;
            stobj_set_slot(obj_ins, slot, dslots_ins, boxed_ins);
        }
    }

    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::setProp(jsval &l, JSPropCacheEntry* entry, JSScopeProperty* sprop,
                       jsval &v, LIns*& v_ins)
{
    if (entry == JS_NO_PROP_CACHE_FILL)
        ABORT_TRACE("can't trace uncacheable property set");
    JS_ASSERT_IF(PCVCAP_TAG(entry->vcap) >= 1, sprop->attrs & JSPROP_SHARED);
    if (!SPROP_HAS_STUB_SETTER(sprop) && sprop->slot != SPROP_INVALID_SLOT)
        ABORT_TRACE("can't trace set of property with setter and slot");
    if (sprop->attrs & JSPROP_SETTER)
        ABORT_TRACE("can't trace JavaScript function setter");

    
    if (sprop->attrs & JSPROP_GETTER)
        ABORT_TRACE("can't assign to property with script getter but no setter");
    if (sprop->attrs & JSPROP_READONLY)
        ABORT_TRACE("can't assign to readonly property");

    JS_ASSERT(!JSVAL_IS_PRIMITIVE(l));
    JSObject* obj = JSVAL_TO_OBJECT(l);
    LIns* obj_ins = get(&l);
    JSScope* scope = OBJ_SCOPE(obj);

    JS_ASSERT_IF(entry->vcap == PCVCAP_MAKE(entry->kshape, 0, 0), scope->has(sprop));

    
    if (OBJ_GET_CLASS(cx, obj) == &js_CallClass) {
        v_ins = get(&v);
        return setCallProp(obj, obj_ins, sprop, v_ins, v);
    }

    





    if (scope->branded() && VALUE_IS_FUNCTION(cx, v))
        ABORT_TRACE("can't trace function-valued property set in branded scope");

    
    JSObject* obj2 = obj;
    for (jsuword i = PCVCAP_TAG(entry->vcap) >> PCVCAP_PROTOBITS; i; i--)
        obj2 = OBJ_GET_PARENT(cx, obj2);
    for (jsuword j = PCVCAP_TAG(entry->vcap) & PCVCAP_PROTOMASK; j; j--)
        obj2 = OBJ_GET_PROTO(cx, obj2);
    scope = OBJ_SCOPE(obj2);
    JS_ASSERT_IF(entry->adding(), obj2 == obj);

    
    LIns* map_ins = map(obj_ins);
    CHECK_STATUS(guardNativePropertyOp(obj, map_ins));
    jsuword pcval;
    CHECK_STATUS(guardPropertyCacheHit(obj_ins, map_ins, obj, obj2, entry, pcval));
    JS_ASSERT(scope->object == obj2);
    JS_ASSERT(scope->has(sprop));
    JS_ASSERT_IF(obj2 != obj, sprop->attrs & JSPROP_SHARED);

    
    if (entry->adding()) {
        JS_ASSERT(!(sprop->attrs & JSPROP_SHARED));
        if (obj == globalObj)
            ABORT_TRACE("adding a property to the global object");

        LIns* args[] = { INS_CONSTSPROP(sprop), obj_ins, cx_ins };
        LIns* ok_ins = lir->insCall(&js_AddProperty_ci, args);
        guard(false, lir->ins_eq0(ok_ins), OOM_EXIT);
    }

    v_ins = get(&v);
    return nativeSet(obj, obj_ins, sprop, v, v_ins);
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::setCallProp(JSObject *callobj, LIns *callobj_ins, JSScopeProperty *sprop,
                           LIns *v_ins, jsval v)
{
    
    JSStackFrame *fp = frameIfInRange(callobj);
    if (fp) {
        jsint slot = JSVAL_TO_INT(SPROP_USERID(sprop));
        if (sprop->setter == SetCallArg) {
            jsval *vp2 = &fp->argv[slot];
            set(vp2, v_ins);
            return JSRS_CONTINUE;
        }
        if (sprop->setter == SetCallVar) {
            jsval *vp2 = &fp->slots[slot];
            set(vp2, v_ins);
            return JSRS_CONTINUE;
        }
        ABORT_TRACE("can't trace special CallClass setter");
    }

    
    const CallInfo* ci = NULL;
    if (sprop->setter == SetCallArg)
        ci = &js_SetCallArg_ci;
    else if (sprop->setter == SetCallVar)
        ci = &js_SetCallVar_ci;
    else
        ABORT_TRACE("can't trace special CallClass setter");

    LIns* args[] = {
        box_jsval(v, v_ins),
        INS_CONST(SPROP_USERID(sprop)),
        callobj_ins,
        cx_ins
    };
    LIns* call_ins = lir->insCall(ci, args);
    guard(false, addName(lir->ins_eq0(call_ins), "guard(set upvar)"), STATUS_EXIT);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_SetPropHit(JSPropCacheEntry* entry, JSScopeProperty* sprop)
{
    jsval& r = stackval(-1);
    jsval& l = stackval(-2);
    LIns* v_ins;
    CHECK_STATUS(setProp(l, entry, sprop, r, v_ins));

    jsbytecode* pc = cx->fp->regs->pc;
    if (*pc != JSOP_INITPROP && pc[JSOP_SETPROP_LENGTH] != JSOP_POP)
        set(&l, v_ins);

    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK void
TraceRecorder::enterDeepBailCall()
{
    
    VMSideExit* exit = snapshot(DEEP_BAIL_EXIT);
    lir->insStorei(INS_CONSTPTR(exit), cx_ins, offsetof(JSContext, bailExit));

    
    LIns* guardRec = createGuardRecord(exit);
    lir->insGuard(LIR_xbarrier, guardRec, guardRec);
}

JS_REQUIRES_STACK void
TraceRecorder::leaveDeepBailCall()
{
    
    lir->insStorei(INS_NULL(), cx_ins, offsetof(JSContext, bailExit));
}

JS_REQUIRES_STACK void
TraceRecorder::finishGetProp(LIns* obj_ins, LIns* vp_ins, LIns* ok_ins, jsval* outp)
{
    
    
    
    LIns* result_ins = lir->insLoad(LIR_ldp, vp_ins, 0);
    set(outp, result_ins, true);
    if (js_CodeSpec[*cx->fp->regs->pc].format & JOF_CALLOP)
        set(outp + 1, obj_ins, true);

    
    
    pendingGuardCondition = ok_ins;

    
    
    
    pendingUnboxSlot = outp;
}

static inline bool
RootedStringToId(JSContext* cx, JSString** namep, jsid* idp)
{
    JSString* name = *namep;
    if (name->isAtomized()) {
        *idp = ATOM_TO_JSID((JSAtom*) STRING_TO_JSVAL(name));
        return true;
    }

    JSAtom* atom = js_AtomizeString(cx, name, 0);
    if (!atom)
        return false;
    *namep = ATOM_TO_STRING(atom); 
    *idp = ATOM_TO_JSID(atom);
    return true;
}

static JSBool FASTCALL
GetPropertyByName(JSContext* cx, JSObject* obj, JSString** namep, jsval* vp)
{
    js_LeaveTraceIfGlobalObject(cx, obj);

    jsid id;
    if (!RootedStringToId(cx, namep, &id) || !obj->getProperty(cx, id, vp)) {
        js_SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->interpState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, GetPropertyByName, CONTEXT, OBJECT, STRINGPTR, JSVALPTR,
                     0, 0)



JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::primitiveToStringInPlace(jsval* vp)
{
    jsval v = *vp;
    JS_ASSERT(JSVAL_IS_PRIMITIVE(v));

    if (!JSVAL_IS_STRING(v)) {
        
        
        JSString *str = js_ValueToString(cx, v);
        if (!str)
            ABORT_TRACE_ERROR("failed to stringify element id");
        v = STRING_TO_JSVAL(str);
        set(vp, stringify(*vp));

        
        
        *vp = v;
    }
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::getPropertyByName(LIns* obj_ins, jsval* idvalp, jsval* outp)
{
    CHECK_STATUS(primitiveToStringInPlace(idvalp));
    enterDeepBailCall();

    
    
    
    LIns* vp_ins = addName(lir->insAlloc(sizeof(jsval)), "vp");
    LIns* idvalp_ins = addName(addr(idvalp), "idvalp");
    LIns* args[] = {vp_ins, idvalp_ins, obj_ins, cx_ins};
    LIns* ok_ins = lir->insCall(&GetPropertyByName_ci, args);

    
    
    
    tracker.set(idvalp, lir->insLoad(LIR_ldp, idvalp_ins, 0));

    finishGetProp(obj_ins, vp_ins, ok_ins, outp);
    leaveDeepBailCall();
    return JSRS_CONTINUE;
}

static JSBool FASTCALL
GetPropertyByIndex(JSContext* cx, JSObject* obj, int32 index, jsval* vp)
{
    js_LeaveTraceIfGlobalObject(cx, obj);

    JSAutoTempIdRooter idr(cx);
    if (!js_Int32ToId(cx, index, idr.addr()) || !obj->getProperty(cx, idr.id(), vp)) {
        js_SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->interpState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, GetPropertyByIndex, CONTEXT, OBJECT, INT32, JSVALPTR, 0, 0)

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::getPropertyByIndex(LIns* obj_ins, LIns* index_ins, jsval* outp)
{
    index_ins = makeNumberInt32(index_ins);

    
    enterDeepBailCall();
    LIns* vp_ins = addName(lir->insAlloc(sizeof(jsval)), "vp");
    LIns* args[] = {vp_ins, index_ins, obj_ins, cx_ins};
    LIns* ok_ins = lir->insCall(&GetPropertyByIndex_ci, args);
    finishGetProp(obj_ins, vp_ins, ok_ins, outp);
    leaveDeepBailCall();
    return JSRS_CONTINUE;
}

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
        if (size_t(i) >= JSVAL_TO_STRING(lval)->length())
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
    if (obj == globalObj)
        ABORT_TRACE("JSOP_GETELEM on global");
    LIns* v_ins;

    
    if (!JSVAL_IS_INT(idx)) {
        if (!JSVAL_IS_PRIMITIVE(idx))
            ABORT_TRACE("object used as index");

        return getPropertyByName(obj_ins, &idx, &lval);
    }

    if (STOBJ_GET_CLASS(obj) == &js_ArgumentsClass) {
        unsigned depth;
        JSStackFrame *afp = guardArguments(obj, obj_ins, &depth);
        if (afp) {
            uintN int_idx = JSVAL_TO_INT(idx);
            jsval* vp = &afp->argv[int_idx];
            if (idx_ins->isconstq()) {
                if (int_idx >= 0 && int_idx < afp->argc)
                    v_ins = get(vp);
                else
                    v_ins = INS_VOID();
            } else {
                
                
                
                idx_ins = makeNumberInt32(idx_ins);
                if (int_idx >= 0 && int_idx < afp->argc) {
                    JSTraceType type = getCoercedType(*vp);

                    
                    LIns* typemap_ins;
                    if (callDepth == depth) {
                        
                        
                        
                        unsigned stackSlots = NativeStackSlots(cx, 0 );
                        if (stackSlots * sizeof(JSTraceType) > LirBuffer::MAX_SKIP_PAYLOAD_SZB)
                            ABORT_TRACE("|arguments| requires saving too much stack");
                        JSTraceType* typemap = (JSTraceType*) lir->insSkip(stackSlots * sizeof(JSTraceType))->payload();
                        DetermineTypesVisitor detVisitor(*this, typemap);
                        VisitStackSlots(detVisitor, cx, 0);
                        typemap_ins = INS_CONSTPTR(typemap + 2 );
                    } else {
                        
                        
                        
                        
                        
                        LIns* fip_ins = lir->insLoad(LIR_ldp, lirbuf->rp, (callDepth-depth)*sizeof(FrameInfo*));
                        typemap_ins = lir->ins2(LIR_add, fip_ins, INS_CONST(sizeof(FrameInfo) + 2 * sizeof(JSTraceType)));
                    }

                    LIns* typep_ins = lir->ins2(LIR_add, typemap_ins,
                                                lir->ins2(LIR_mul, idx_ins, INS_CONST(sizeof(JSTraceType))));
                    LIns* type_ins = lir->insLoad(LIR_ldcb, typep_ins, 0);
                    guard(true,
                          addName(lir->ins2(LIR_eq, type_ins, lir->insImm(type)),
                                  "guard(type-stable upvar)"),
                          BRANCH_EXIT);

                    
                    guard(true, lir->ins2(LIR_ult, idx_ins, INS_CONST(afp->argc)),
                          snapshot(BRANCH_EXIT));
                    size_t stackOffset = -treeInfo->nativeStackBase + nativeStackOffset(&afp->argv[0]);
                    LIns* args_addr_ins = lir->ins2(LIR_add, lirbuf->sp, INS_CONST(stackOffset));
                    LIns* argi_addr_ins = lir->ins2(LIR_add, args_addr_ins,
                                                    lir->ins2(LIR_mul, idx_ins, INS_CONST(sizeof(double))));
                    v_ins = stackLoad(argi_addr_ins, type);
                } else {
                    guard(false, lir->ins2(LIR_ult, idx_ins, INS_CONST(afp->argc)),
                          snapshot(BRANCH_EXIT));
                    v_ins = INS_VOID();
                }
            }
            JS_ASSERT(v_ins);
            set(&lval, v_ins);
            return JSRS_CONTINUE;
        }
        ABORT_TRACE("can't reach arguments object's frame");
    }
    if (js_IsDenseArray(obj)) {
        
        jsval* vp;
        LIns* addr_ins;

        guardDenseArray(obj, obj_ins, BRANCH_EXIT);
        CHECK_STATUS(denseArrayElement(lval, idx, vp, v_ins, addr_ins));
        set(&lval, v_ins);
        if (call)
            set(&idx, obj_ins);
        return JSRS_CONTINUE;
    }

    return getPropertyByIndex(obj_ins, idx_ins, &lval);
}



static JSBool FASTCALL
SetPropertyByName(JSContext* cx, JSObject* obj, JSString** namep, jsval* vp)
{
    js_LeaveTraceIfGlobalObject(cx, obj);

    jsid id;
    if (!RootedStringToId(cx, namep, &id) || !obj->setProperty(cx, id, vp)) {
        js_SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->interpState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, SetPropertyByName, CONTEXT, OBJECT, STRINGPTR, JSVALPTR,
                     0, 0)

static JSBool FASTCALL
InitPropertyByName(JSContext* cx, JSObject* obj, JSString** namep, jsval val)
{
    js_LeaveTraceIfGlobalObject(cx, obj);

    jsid id;
    if (!RootedStringToId(cx, namep, &id) ||
        !obj->defineProperty(cx, id, val, NULL, NULL, JSPROP_ENUMERATE, NULL)) {
        js_SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->interpState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, InitPropertyByName, CONTEXT, OBJECT, STRINGPTR, JSVAL,
                     0, 0)

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::initOrSetPropertyByName(LIns* obj_ins, jsval* idvalp, jsval* rvalp, bool init)
{
    CHECK_STATUS(primitiveToStringInPlace(idvalp));

    LIns* rval_ins = box_jsval(*rvalp, get(rvalp));

    enterDeepBailCall();

    LIns* ok_ins;
    LIns* idvalp_ins = addName(addr(idvalp), "idvalp");
    if (init) {
        LIns* args[] = {rval_ins, idvalp_ins, obj_ins, cx_ins};
        ok_ins = lir->insCall(&InitPropertyByName_ci, args);
    } else {
        
        LIns* vp_ins = addName(lir->insAlloc(sizeof(jsval)), "vp");
        lir->insStorei(rval_ins, vp_ins, 0);
        LIns* args[] = {vp_ins, idvalp_ins, obj_ins, cx_ins};
        ok_ins = lir->insCall(&SetPropertyByName_ci, args);
    }
    guard(true, ok_ins, STATUS_EXIT);

    leaveDeepBailCall();
    return JSRS_CONTINUE;
}

static JSBool FASTCALL
SetPropertyByIndex(JSContext* cx, JSObject* obj, int32 index, jsval* vp)
{
    js_LeaveTraceIfGlobalObject(cx, obj);

    JSAutoTempIdRooter idr(cx);
    if (!js_Int32ToId(cx, index, idr.addr()) || !obj->setProperty(cx, idr.id(), vp)) {
        js_SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->interpState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, SetPropertyByIndex, CONTEXT, OBJECT, INT32, JSVALPTR, 0, 0)

static JSBool FASTCALL
InitPropertyByIndex(JSContext* cx, JSObject* obj, int32 index, jsval val)
{
    js_LeaveTraceIfGlobalObject(cx, obj);

    JSAutoTempIdRooter idr(cx);
    if (!js_Int32ToId(cx, index, idr.addr()) ||
        !obj->defineProperty(cx, idr.id(), val, NULL, NULL, JSPROP_ENUMERATE, NULL)) {
        js_SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->interpState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, InitPropertyByIndex, CONTEXT, OBJECT, INT32, JSVAL, 0, 0)

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::initOrSetPropertyByIndex(LIns* obj_ins, LIns* index_ins, jsval* rvalp, bool init)
{
    index_ins = makeNumberInt32(index_ins);

    LIns* rval_ins = box_jsval(*rvalp, get(rvalp));

    enterDeepBailCall();

    LIns* ok_ins;
    if (init) {
        LIns* args[] = {rval_ins, index_ins, obj_ins, cx_ins};
        ok_ins = lir->insCall(&InitPropertyByIndex_ci, args);
    } else {
        
        LIns* vp_ins = addName(lir->insAlloc(sizeof(jsval)), "vp");
        lir->insStorei(rval_ins, vp_ins, 0);
        LIns* args[] = {vp_ins, index_ins, obj_ins, cx_ins};
        ok_ins = lir->insCall(&SetPropertyByIndex_ci, args);
    }
    guard(true, ok_ins, STATUS_EXIT);

    leaveDeepBailCall();
    return JSRS_CONTINUE;
}

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

    if (!JSVAL_IS_INT(idx)) {
        if (!JSVAL_IS_PRIMITIVE(idx))
            ABORT_TRACE("non-primitive index");
        CHECK_STATUS(initOrSetPropertyByName(obj_ins, &idx, &v,
                                             *cx->fp->regs->pc == JSOP_INITELEM));
    } else if (JSVAL_TO_INT(idx) < 0 || !OBJ_IS_DENSE_ARRAY(cx, obj)) {
        CHECK_STATUS(initOrSetPropertyByIndex(obj_ins, idx_ins, &v,
                                              *cx->fp->regs->pc == JSOP_INITELEM));
    } else {
        

        
        if (!guardDenseArray(obj, obj_ins, BRANCH_EXIT))
            return JSRS_STOP;

        
        
        idx_ins = makeNumberInt32(idx_ins);

        
        
        
        LIns* res_ins;
        LIns* args[] = { NULL, idx_ins, obj_ins, cx_ins };
        if (isNumber(v)) {
            if (isPromoteInt(v_ins)) {
                args[0] = ::demote(lir, v_ins);
                res_ins = lir->insCall(&js_Array_dense_setelem_int_ci, args);
            } else {
                args[0] = v_ins;
                res_ins = lir->insCall(&js_Array_dense_setelem_double_ci, args);
            }
        } else {
            LIns* args[] = { box_jsval(v, v_ins), idx_ins, obj_ins, cx_ins };
            res_ins = lir->insCall(&js_Array_dense_setelem_ci, args);
        }
        guard(false, lir->ins_eq0(res_ins), MISMATCH_EXIT);
    }

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
        LIns* ins;
        NameResult nr;
        CHECK_STATUS(scopeChainProp(obj, vp, ins, nr));
        stack(0, ins);
        stack(1, INS_CONSTOBJ(globalObj));
        return JSRS_CONTINUE;
    }

    LIns* obj_ins = scopeChain();
    JSObject* obj2;
    jsuword pcval;

    CHECK_STATUS(test_property_cache(obj, obj_ins, obj2, pcval));

    if (PCVAL_IS_NULL(pcval) || !PCVAL_IS_OBJECT(pcval))
        ABORT_TRACE("callee is not an object");

    JS_ASSERT(HAS_FUNCTION_CLASS(PCVAL_TO_OBJECT(pcval)));

    stack(0, INS_CONSTOBJ(PCVAL_TO_OBJECT(pcval)));
    stack(1, obj_ins);
    return JSRS_CONTINUE;
}

JS_DEFINE_CALLINFO_5(extern, UINT32, GetUpvarArgOnTrace, CONTEXT, UINT32, INT32, UINT32,
                     DOUBLEPTR, 0, 0)
JS_DEFINE_CALLINFO_5(extern, UINT32, GetUpvarVarOnTrace, CONTEXT, UINT32, INT32, UINT32,
                     DOUBLEPTR, 0, 0)
JS_DEFINE_CALLINFO_5(extern, UINT32, GetUpvarStackOnTrace, CONTEXT, UINT32, INT32, UINT32,
                     DOUBLEPTR, 0, 0)






JS_REQUIRES_STACK LIns*
TraceRecorder::upvar(JSScript* script, JSUpvarArray* uva, uintN index, jsval& v)
{
    






    uint32 cookie = uva->vector[index];
    jsval& vr = js_GetUpvar(cx, script->staticLevel, cookie);
    v = vr;
    LIns* upvar_ins = get(&vr);
    if (upvar_ins) {
        return upvar_ins;
    }

    



    uint32 level = script->staticLevel - UPVAR_FRAME_SKIP(cookie);
    uint32 cookieSlot = UPVAR_FRAME_SLOT(cookie);
    JSStackFrame* fp = cx->display[level];
    const CallInfo* ci;
    int32 slot;
    if (!fp->fun) {
        ci = &GetUpvarStackOnTrace_ci;
        slot = cookieSlot;
    } else if (cookieSlot < fp->fun->nargs) {
        ci = &GetUpvarArgOnTrace_ci;
        slot = cookieSlot;
    } else if (cookieSlot == CALLEE_UPVAR_SLOT) {
        ci = &GetUpvarArgOnTrace_ci;
        slot = -2;
    } else {
        ci = &GetUpvarVarOnTrace_ci;
        slot = cookieSlot - fp->fun->nargs;
    }

    LIns* outp = lir->insAlloc(sizeof(double));
    LIns* args[] = {
        outp,
        INS_CONST(callDepth),
        INS_CONST(slot),
        INS_CONST(level),
        cx_ins
    };
    LIns* call_ins = lir->insCall(ci, args);
    JSTraceType type = getCoercedType(v);
    guard(true,
          addName(lir->ins2(LIR_eq, call_ins, lir->insImm(type)),
                  "guard(type-stable upvar)"),
          BRANCH_EXIT);
    return stackLoad(outp, type);
}





LIns* TraceRecorder::stackLoad(LIns* base, uint8 type)
{
    LOpcode loadOp;
    switch (type) {
      case TT_DOUBLE:
        loadOp = LIR_ldq;
        break;
      case TT_OBJECT:
      case TT_STRING:
      case TT_FUNCTION:
      case TT_NULL:
        loadOp = LIR_ldp;
        break;
      case TT_INT32:
      case TT_PSEUDOBOOLEAN:
        loadOp = LIR_ld;
        break;
      case TT_JSVAL:
      default:
        JS_NOT_REACHED("found jsval type in an upvar type map entry");
        return NULL;
    }

    LIns* result = lir->insLoad(loadOp, base, 0);
    if (type == TT_INT32)
        result = lir->ins1(LIR_i2f, result);
    return result;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETUPVAR()
{
    uintN index = GET_UINT16(cx->fp->regs->pc);
    JSScript *script = cx->fp->script;
    JSUpvarArray* uva = JS_SCRIPT_UPVARS(script);
    JS_ASSERT(index < uva->length);

    jsval v;
    LIns* upvar_ins = upvar(script, uva, index, v);
    if (!upvar_ins)
        return JSRS_STOP;
    stack(0, upvar_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLUPVAR()
{
    CHECK_STATUS(record_JSOP_GETUPVAR());
    stack(1, INS_NULL());
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_GETDSLOT()
{
    JSObject* callee = JSVAL_TO_OBJECT(cx->fp->argv[-2]);
    LIns* callee_ins = get(&cx->fp->argv[-2]);

    unsigned index = GET_UINT16(cx->fp->regs->pc);
    LIns* dslots_ins = NULL;
    LIns* v_ins = stobj_get_dslot(callee_ins, index, dslots_ins);

    stack(0, unbox_jsval(callee->dslots[index], v_ins, snapshot(BRANCH_EXIT)));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLDSLOT()
{
    CHECK_STATUS(record_JSOP_GETDSLOT());
    stack(1, INS_NULL());
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::guardCallee(jsval& callee)
{
    JS_ASSERT(VALUE_IS_FUNCTION(cx, callee));

    VMSideExit* branchExit = snapshot(BRANCH_EXIT);
    JSObject* callee_obj = JSVAL_TO_OBJECT(callee);
    LIns* callee_ins = get(&callee);

    treeInfo->gcthings.addUnique(callee);
    guard(true,
          lir->ins2(LIR_eq,
                    stobj_get_private(callee_ins),
                    INS_CONSTPTR(callee_obj->getAssignedPrivate())),
          branchExit);
    guard(true,
          lir->ins2(LIR_eq,
                    stobj_get_fslot(callee_ins, JSSLOT_PARENT),
                    INS_CONSTOBJ(OBJ_GET_PARENT(cx, callee_obj))),
          branchExit);
    return JSRS_CONTINUE;
}







JS_REQUIRES_STACK JSStackFrame *
TraceRecorder::guardArguments(JSObject *obj, LIns* obj_ins, unsigned *depthp)
{
    JS_ASSERT(STOBJ_GET_CLASS(obj) == &js_ArgumentsClass);

    JSStackFrame *afp = frameIfInRange(obj, depthp);
    if (!afp)
        return NULL;

    VMSideExit *exit = snapshot(MISMATCH_EXIT);
    guardClass(obj, obj_ins, &js_ArgumentsClass, exit);

    LIns* args_ins = get(&afp->argsobj);
    LIns* cmp = lir->ins2(LIR_eq, args_ins, obj_ins);
    lir->insGuard(LIR_xf, cmp, createGuardRecord(exit));
    return afp;
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

    
    unsigned stackSlots = NativeStackSlots(cx, 0 );
    if (sizeof(FrameInfo) + stackSlots * sizeof(JSTraceType) > LirBuffer::MAX_SKIP_PAYLOAD_SZB)
        ABORT_TRACE("interpreted function call requires saving too much stack");
    LIns* data = lir->insSkip(sizeof(FrameInfo) + stackSlots * sizeof(JSTraceType));
    FrameInfo* fi = (FrameInfo*)data->payload();
    JSTraceType* typemap = reinterpret_cast<JSTraceType *>(fi + 1);

    DetermineTypesVisitor detVisitor(*this, typemap);
    VisitStackSlots(detVisitor, cx, 0);

    JS_ASSERT(argc < FrameInfo::CONSTRUCTING_FLAG);

    treeInfo->gcthings.addUnique(fval);
    fi->block = fp->blockChain;
    if (fp->blockChain)
        treeInfo->gcthings.addUnique(OBJECT_TO_JSVAL(fp->blockChain));
    fi->pc = fp->regs->pc;
    fi->imacpc = fp->imacpc;
    fi->spdist = fp->regs->sp - fp->slots;
    fi->set_argc(argc, constructing);
    fi->callerHeight = NativeStackSlots(cx, 0) - (2 + argc);
    fi->callerArgc = fp->argc;

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

        



        if (OBJ_IS_DENSE_ARRAY(cx, aobj)) {
            guardDenseArray(aobj, aobj_ins);
            length = jsuint(aobj->fslots[JSSLOT_ARRAY_LENGTH]);
            guard(true,
                  lir->ins2i(LIR_eq,
                             stobj_get_fslot(aobj_ins, JSSLOT_ARRAY_LENGTH),
                             length),
                  BRANCH_EXIT);
        } else if (OBJ_GET_CLASS(cx, aobj) == &js_ArgumentsClass) {
            unsigned depth;
            JSStackFrame *afp = guardArguments(aobj, aobj_ins, &depth);
            if (!afp)
                ABORT_TRACE("can't reach arguments object's frame");
            length = afp->argc;
        } else {
            ABORT_TRACE("arguments parameter of apply is not a dense array or argments object");
        }

        if (length >= JS_ARRAY_LENGTH(apply_imacro_table))
            ABORT_TRACE("too many arguments to apply");

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
    JS_ASSERT(*pc == JSOP_CALL || *pc == JSOP_APPLY || *pc == JSOP_NEW || *pc == JSOP_SETPROP);

    jsval& v = stackval(-1);
    LIns* v_ins = get(&v);

    













    if (JSTN_ERRTYPE(pendingTraceableNative) == FAIL_STATUS) {
        
        lir->insStorei(INS_NULL(), cx_ins, (int) offsetof(JSContext, bailExit));

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

            propagateFailureToBuiltinStatus(ok_ins, status);
        }
        guard(true, lir->ins_eq0(status), STATUS_EXIT);
    }

    JSRecordingStatus ok = JSRS_CONTINUE;
    if (pendingTraceableNative->flags & JSTN_UNBOX_AFTER) {
        




        JS_ASSERT(&v == &cx->fp->regs->sp[-1] && get(&v) == v_ins);
        set(&v, unbox_jsval(v, v_ins, snapshot(BRANCH_EXIT)));
    } else if (JSTN_ERRTYPE(pendingTraceableNative) == FAIL_NEG) {
        
        JS_ASSERT(JSVAL_IS_NUMBER(v));
    } else {
        
        if (JSVAL_IS_NUMBER(v) &&
            (pendingTraceableNative->builtin->_argtypes & ARGSIZE_MASK_ANY) == ARGSIZE_LO) {
            set(&v, lir->ins1(LIR_i2f, v_ins));
        }
    }

    
    
    return ok;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::name(jsval*& vp, LIns*& ins, NameResult& nr)
{
    JSObject* obj = cx->fp->scopeChain;
    if (obj != globalObj)
        return scopeChainProp(obj, vp, ins, nr);

    
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
    ins = get(vp);
    nr.tracked = true;
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
        



        if (OBJ_GET_CLASS(cx, obj)->getProperty != JS_PropertyStub) {
            ABORT_TRACE("can't trace through access to undefined property if "
                        "JSClass.getProperty hook isn't stubbed");
        }
        guardClass(obj, obj_ins, OBJ_GET_CLASS(cx, obj), snapshot(MISMATCH_EXIT));

        






        VMSideExit* exit = snapshot(BRANCH_EXIT);
        do {
            LIns* map_ins = map(obj_ins);
            LIns* ops_ins;
            if (map_is_native(obj->map, map_ins, ops_ins)) {
                LIns* shape_ins = addName(lir->insLoad(LIR_ld, map_ins, offsetof(JSScope, shape)),
                                          "shape");
                guard(true,
                      addName(lir->ins2i(LIR_eq, shape_ins, OBJ_SHAPE(obj)), "guard(shape)"),
                      exit);
            } else if (!guardDenseArray(obj, obj_ins, BRANCH_EXIT))
                ABORT_TRACE("non-native object involved in undefined property access");
        } while (guardHasPrototype(obj, obj_ins, &obj, &obj_ins, exit));

        v_ins = INS_CONST(JSVAL_TO_SPECIAL(JSVAL_VOID));
        slot = SPROP_INVALID_SLOT;
        return JSRS_CONTINUE;
    }

    uint32 setflags = (cs.format & (JOF_INCDEC | JOF_FOR));
    JS_ASSERT(!(cs.format & JOF_SET));

    
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
                LIns* args[] = { INS_CONSTSPROP(sprop), obj_ins, cx_ins };
                v_ins = lir->insCall(&js_CallGetter_ci, args);
                guard(false, lir->ins2(LIR_eq, v_ins, INS_CONST(JSVAL_ERROR_COOKIE)), OOM_EXIT);

                



                v_ins =
                    unbox_jsval((sprop->shortid == REGEXP_SOURCE) ? JSVAL_STRING : JSVAL_SPECIAL,
                                v_ins,
                                snapshot(MISMATCH_EXIT));
                return JSRS_CONTINUE;
            }
            if (setflags == 0 &&
                sprop->getter == js_StringClass.getProperty &&
                sprop->id == ATOM_KEY(cx->runtime->atomState.lengthAtom)) {
                if (!guardClass(obj, obj_ins, &js_StringClass, snapshot(MISMATCH_EXIT)))
                    ABORT_TRACE("can't trace String.length on non-String objects");
                LIns* str_ins = stobj_get_private(obj_ins, JSVAL_TAGMASK);
                v_ins = lir->ins1(LIR_i2f, getStringLength(str_ins));
                return JSRS_CONTINUE;
            }
            ABORT_TRACE("non-stub getter");
        }
        if (!SPROP_HAS_VALID_SLOT(sprop, OBJ_SCOPE(obj2)))
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
            obj_ins = stobj_get_fslot(obj_ins, JSSLOT_PROTO);
            obj = STOBJ_GET_PROTO(obj);
        }
    }

    LIns* dslots_ins = NULL;
    v_ins = unbox_jsval(STOBJ_GET_SLOT(obj, slot),
                        stobj_get_slot(obj_ins, slot, dslots_ins),
                        snapshot(BRANCH_EXIT));

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
        lir->insGuard(LIR_x, NULL, createGuardRecord(exit));
        LIns* label = lir->ins0(LIR_label);
        if (br1)
            br1->setTarget(label);
        br2->setTarget(label);
        br3->setTarget(label);
        br4->setTarget(label);

        CHECK_STATUS(guardPrototypeHasNoIndexedProperties(obj, obj_ins, MISMATCH_EXIT));

        
        v_ins = lir->insImm(JSVAL_TO_SPECIAL(JSVAL_VOID));
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
    v_ins = unbox_jsval(*vp, lir->insLoad(LIR_ldp, addr_ins, 0), exit);

    if (JSVAL_IS_SPECIAL(*vp)) {
        



        LIns* br = lir->insBranch(LIR_jf,
                                  lir->ins2i(LIR_eq, v_ins, JSVAL_TO_SPECIAL(JSVAL_HOLE)),
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
    LIns* v_ins;
    NameResult nr;
    CHECK_STATUS(name(vp, v_ins, nr));
    stack(0, v_ins);
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
    stack(0, INS_ATOM(atom));
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
    stack(0, INS_NULL());
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
    
    return tableswitch();
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
    stack(0, INS_CONSTOBJ(obj));
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
    LIns* x_ins;
    NameResult nr;
    CHECK_STATUS(name(vp, x_ins, nr));
    if (!nr.tracked)
        ABORT_TRACE("forname on non-tracked value not supported");
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

        
        
        obj = OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(fp->argv[-2]));
    } else {
        obj = fp->scopeChain;

        
        
        
        while (OBJ_GET_CLASS(cx, obj) == &js_BlockClass) {
            
            JS_ASSERT(obj->getAssignedPrivate() == fp);

            obj = OBJ_GET_PARENT(cx, obj);

            
            JS_ASSERT(obj);
        }
    }

    if (obj != globalObj) {
        if (OBJ_GET_CLASS(cx, obj) != &js_CallClass)
            ABORT_TRACE("Can only trace JSOP_BINDNAME with global or call object");

        




        JS_ASSERT(obj == cx->fp->scopeChain || obj == OBJ_GET_PARENT(cx, cx->fp->scopeChain));
        stack(0, stobj_get_parent(get(&cx->fp->argv[-2])));
        return JSRS_CONTINUE;
    }

    






    stack(0, INS_CONSTOBJ(obj));
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_SETNAME()
{
    jsval& l = stackval(-2);
    JS_ASSERT(!JSVAL_IS_PRIMITIVE(l));

    



    JSObject* obj = JSVAL_TO_OBJECT(l);
    if (OBJ_GET_CLASS(cx, obj) == &js_CallClass)
        return JSRS_CONTINUE;
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

    guard(false, lir->ins2i(LIR_eq, x, JSVAL_TO_SPECIAL(JSVAL_VOID)), OOM_EXIT);
    x = lir->ins2i(LIR_eq, x, 1);

    JSObject* obj2;
    JSProperty* prop;
    if (!obj->lookupProperty(cx, id, &obj2, &prop))
        ABORT_TRACE_ERROR("obj->lookupProperty failed in JSOP_IN");
    bool cond = prop != NULL;
    if (prop)
        obj2->dropProperty(cx, prop);
    if (wasDeepAborted())
        ABORT_TRACE("deep abort from property lookup");

    



    fuseIf(cx->fp->regs->pc + 1, cond, x);

    





    set(&lval, x);
    return JSRS_CONTINUE;
}

static JSBool FASTCALL
HasInstance(JSContext* cx, JSObject* ctor, jsval val)
{
    JSBool result = JS_FALSE;
    if (!ctor->map->ops->hasInstance(cx, ctor, val, &result))
        js_SetBuiltinError(cx);
    return result;
}
JS_DEFINE_CALLINFO_3(static, BOOL_FAIL, HasInstance, CONTEXT, OBJECT, JSVAL, 0, 0)

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_INSTANCEOF()
{
    
    jsval& ctor = stackval(-1);
    if (JSVAL_IS_PRIMITIVE(ctor))
        ABORT_TRACE("non-object on rhs of instanceof");

    jsval& val = stackval(-2);
    LIns* val_ins = box_jsval(val, get(&val));

    enterDeepBailCall();
    LIns* args[] = {val_ins, get(&ctor), cx_ins};
    stack(-2, lir->insCall(&HasInstance_ci, args));
    LIns* status_ins = lir->insLoad(LIR_ld,
                                    lirbuf->state,
                                    (int) offsetof(InterpState, builtinStatus));
    guard(true, lir->ins_eq0(status_ins), STATUS_EXIT);
    leaveDeepBailCall();

    return JSRS_CONTINUE;
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

        LIns* args[] = { INS_CONSTOBJ(globalObj), proto_ins, INS_CONSTFUN(fun), cx_ins };
        LIns* x = lir->insCall(&js_NewNullClosure_ci, args);
        stack(0, x);
        return JSRS_CONTINUE;
    }
    return JSRS_STOP;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_LAMBDA_FC()
{
    JSFunction* fun;
    JS_GET_SCRIPT_FUNCTION(cx->fp->script, getFullIndex(), fun);

    LIns* scopeChain_ins = get(&cx->fp->argv[-2]);
    JS_ASSERT(scopeChain_ins);

    LIns* args[] = {
        scopeChain_ins,
        INS_CONSTFUN(fun),
        cx_ins
    };
    LIns* call_ins = lir->insCall(&js_AllocFlatClosure_ci, args);
    guard(false,
          addName(lir->ins2(LIR_eq, call_ins, INS_NULL()),
                  "guard(js_AllocFlatClosure)"),
          OOM_EXIT);
    stack(0, call_ins);

    if (fun->u.i.nupvars) {
        JSUpvarArray *uva = JS_SCRIPT_UPVARS(fun->u.i.script);
        for (uint32 i = 0, n = uva->length; i < n; i++) {
            jsval v;
            LIns* upvar_ins = upvar(fun->u.i.script, uva, i, v);
            if (!upvar_ins)
                return JSRS_STOP;
            LIns* dslots_ins = NULL;
            stobj_set_dslot(call_ins, i, dslots_ins, box_jsval(v, upvar_ins));
        }
    }

    return JSRS_CONTINUE;
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
        if (slot < fp->argc)
            stack(0, get(&cx->fp->argv[slot]));
        else
            stack(0, INS_VOID());
        return JSRS_CONTINUE;
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

        LIns* args[] = { INS_CONSTOBJ(globalObj), proto_ins, INS_CONSTFUN(fun), cx_ins };
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
        debug_only_stmt(const char* protoname = NULL;)
        if (JSVAL_IS_STRING(l)) {
            i = JSProto_String;
            debug_only_stmt(protoname = "String.prototype";)
        } else if (JSVAL_IS_NUMBER(l)) {
            i = JSProto_Number;
            debug_only_stmt(protoname = "Number.prototype";)
        } else if (JSVAL_IS_SPECIAL(l)) {
            if (l == JSVAL_VOID)
                ABORT_TRACE("callprop on void");
            guard(false, lir->ins2i(LIR_eq, get(&l), JSVAL_TO_SPECIAL(JSVAL_VOID)), MISMATCH_EXIT);
            i = JSProto_Boolean;
            debug_only_stmt(protoname = "Boolean.prototype";)
        } else {
            JS_ASSERT(JSVAL_IS_NULL(l) || JSVAL_IS_VOID(l));
            ABORT_TRACE("callprop on null or void");
        }

        if (!js_GetClassPrototype(cx, NULL, INT_TO_JSID(i), &obj))
            ABORT_TRACE_ERROR("GetClassPrototype failed!");

        obj_ins = INS_CONSTOBJ(obj);
        debug_only_stmt(obj_ins = addName(obj_ins, protoname);)
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
    stack(-1, INS_CONSTOBJ(PCVAL_TO_OBJECT(pcval)));
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

    putArguments();

    







    if (fp->flags & JSFRAME_CONSTRUCTING) {
        JS_ASSERT(OBJECT_TO_JSVAL(fp->thisp) == fp->argv[-1]);
        rval_ins = get(&fp->argv[-1]);
    } else {
        rval_ins = INS_CONST(JSVAL_TO_SPECIAL(JSVAL_VOID));
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

    jsval* vp;
    LIns* v_ins;
    NameResult nr;
    CHECK_STATUS(name(vp, v_ins, nr));
    stack(-1, v_ins);
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

    LIns* void_ins = INS_CONST(JSVAL_TO_SPECIAL(JSVAL_VOID));
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
    LIns *elt_ins = box_jsval(elt, get(&elt));

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
    stack(1, INS_NULL());
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLLOCAL()
{
    uintN slot = GET_SLOTNO(cx->fp->regs->pc);
    stack(0, var(slot));
    stack(1, INS_NULL());
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_CALLARG()
{
    uintN slot = GET_ARGNO(cx->fp->regs->pc);
    stack(0, arg(slot));
    stack(1, INS_NULL());
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
    stack(-1, INS_CONSTOBJ(obj));
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

    if (STOBJ_GET_CLASS(obj) == &js_ArgumentsClass) {
        unsigned depth;
        JSStackFrame *afp = guardArguments(obj, obj_ins, &depth);
        if (!afp)
            ABORT_TRACE("can't reach arguments object's frame");

        LIns* v_ins = lir->ins1(LIR_i2f, INS_CONST(afp->argc));
        set(&l, v_ins);
        return JSRS_CONTINUE;
    }

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
        LIns* elt_ins = box_jsval(v, get(&v));
        stobj_set_dslot(v_ins, i, dslots_ins, elt_ins);
    }

    if (count > 0)
        stobj_set_fslot(v_ins, JSSLOT_ARRAY_COUNT, INS_CONST(count));

    stack(-int(len), v_ins);
    return JSRS_CONTINUE;
}

JS_REQUIRES_STACK JSRecordingStatus
TraceRecorder::record_JSOP_HOLE()
{
    stack(0, INS_CONST(JSVAL_TO_SPECIAL(JSVAL_HOLE)));
    return JSRS_CONTINUE;
}

JSRecordingStatus
TraceRecorder::record_JSOP_LOOP()
{
    return JSRS_CONTINUE;
}

#define DBG_STUB(OP)                                                          \
    JS_REQUIRES_STACK JSRecordingStatus                                       \
    TraceRecorder::record_##OP()                                              \
    {                                                                         \
        ABORT_TRACE("can't trace " #OP);                                      \
    }

DBG_STUB(JSOP_GETUPVAR_DBG)
DBG_STUB(JSOP_CALLUPVAR_DBG)
DBG_STUB(JSOP_DEFFUN_DBGFC)
DBG_STUB(JSOP_DEFLOCALFUN_DBGFC)
DBG_STUB(JSOP_LAMBDA_DBGFC)

#ifdef JS_JIT_SPEW




void
DumpPeerStability(JSTraceMonitor* tm, const void* ip, JSObject* globalObj, uint32 globalShape,
                  uint32 argc)
{
    Fragment* f;
    TreeInfo* ti;
    bool looped = false;
    unsigned length = 0;

    for (f = getLoop(tm, ip, globalObj, globalShape, argc); f != NULL; f = f->peer) {
        if (!f->vmprivate)
            continue;
        debug_only_printf(LC_TMRecorder, "Stability of fragment %p:\nENTRY STACK=", (void*)f);
        ti = (TreeInfo*)f->vmprivate;
        if (looped)
            JS_ASSERT(ti->nStackTypes == length);
        for (unsigned i = 0; i < ti->nStackTypes; i++)
            debug_only_printf(LC_TMRecorder, "%c", typeChar[ti->stackTypeMap()[i]]);
        debug_only_print0(LC_TMRecorder, " GLOBALS=");
        for (unsigned i = 0; i < ti->nGlobalTypes(); i++)
            debug_only_printf(LC_TMRecorder, "%c", typeChar[ti->globalTypeMap()[i]]);
        debug_only_print0(LC_TMRecorder, "\n");
        UnstableExit* uexit = ti->unstableExits;
        while (uexit != NULL) {
            debug_only_print0(LC_TMRecorder, "EXIT  ");
            JSTraceType* m = uexit->exit->fullTypeMap();
            debug_only_print0(LC_TMRecorder, "STACK=");
            for (unsigned i = 0; i < uexit->exit->numStackSlots; i++)
                debug_only_printf(LC_TMRecorder, "%c", typeChar[m[i]]);
            debug_only_print0(LC_TMRecorder, " GLOBALS=");
            for (unsigned i = 0; i < uexit->exit->numGlobalSlots; i++) {
                debug_only_printf(LC_TMRecorder, "%c",
                                  typeChar[m[uexit->exit->numStackSlots + i]]);
            }
            debug_only_print0(LC_TMRecorder, "\n");
            uexit = uexit->next;
        }
        length = ti->nStackTypes;
        looped = true;
    }
}
#endif

#ifdef MOZ_TRACEVIS

FILE* traceVisLogFile = NULL;
JSHashTable *traceVisScriptTable = NULL;

JS_FRIEND_API(bool)
JS_StartTraceVis(const char* filename = "tracevis.dat")
{
    if (traceVisLogFile) {
        
        JS_StopTraceVis();
    }

    traceVisLogFile = fopen(filename, "wb");
    if (!traceVisLogFile)
        return false;

    return true;
}

JS_FRIEND_API(JSBool)
js_StartTraceVis(JSContext *cx, JSObject *obj,
                 uintN argc, jsval *argv, jsval *rval)
{
    JSBool ok;

    if (argc > 0 && JSVAL_IS_STRING(argv[0])) {
        JSString *str = JSVAL_TO_STRING(argv[0]);
        char *filename = js_DeflateString(cx, str->chars(), str->length());
        if (!filename)
            goto error;
        ok = JS_StartTraceVis(filename);
        cx->free(filename);
    } else {
        ok = JS_StartTraceVis();
    }

    if (ok) {
        fprintf(stderr, "started TraceVis recording\n");
        return JS_TRUE;
    }

  error:
    JS_ReportError(cx, "failed to start TraceVis recording");
    return JS_FALSE;
}

JS_FRIEND_API(bool)
JS_StopTraceVis()
{
    if (!traceVisLogFile)
        return false;

    fclose(traceVisLogFile); 
    traceVisLogFile = NULL;

    return true;
}

JS_FRIEND_API(JSBool)
js_StopTraceVis(JSContext *cx, JSObject *obj,
                uintN argc, jsval *argv, jsval *rval)
{
    JSBool ok = JS_StopTraceVis();

    if (ok)
        fprintf(stderr, "stopped TraceVis recording\n");
    else
        JS_ReportError(cx, "TraceVis isn't running");

    return ok;
}

#endif 

#define UNUSED(n)                                                             \
    JS_REQUIRES_STACK JSRecordingStatus                                       \
    TraceRecorder::record_JSOP_UNUSED##n() {                                  \
        JS_NOT_REACHED("JSOP_UNUSED" #n);                                     \
        return JSRS_STOP;                                                     \
    }
