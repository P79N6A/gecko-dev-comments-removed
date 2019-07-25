








































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
#include "jsdate.h"
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
#include "jsstaticcheck.h"
#include "jstracer.h"
#include "jsxml.h"
#include "jstypedarray.h"

#include "jsatominlines.h"
#include "jscntxtinlines.h"
#include "jsfuninlines.h"
#include "jspropertycacheinlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jscntxtinlines.h"

#include "jsautooplen.h"        
#include "imacros.c.out"

#if defined(NANOJIT_ARM) && defined(__GNUC__) && defined(AVMPLUS_LINUX)
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <elf.h>
#endif

namespace nanojit {
using namespace js;



void*
nanojit::Allocator::allocChunk(size_t nbytes)
{
    VMAllocator *vma = (VMAllocator*)this;
    JS_ASSERT(!vma->outOfMemory());
    void *p = js_calloc(nbytes);
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
        js_free(p);
}

void
nanojit::Allocator::postReset() {
    VMAllocator *vma = (VMAllocator*)this;
    vma->mOutOfMemory = false;
    vma->mSize = 0;
}

int
StackFilter::getTop(LIns* guard)
{
    VMSideExit* e = (VMSideExit*)guard->record()->exit;
    return e->sp_adj;
}

#ifdef DEBUG
void ValidateWriter::checkAccSet(LOpcode op, LIns* base, int32_t disp, AccSet accSet)
{
    LIns* sp = (LIns*)checkAccSetExtras[0];
    LIns* rp = (LIns*)checkAccSetExtras[1];

    bool isRstack = base == rp;
    bool isStack =
        base == sp ||
        (base->isop(LIR_addp) && base->oprnd1() == sp && base->oprnd2()->isImmP());
    bool isUnknown = !isStack && !isRstack;

    bool ok;

    NanoAssert(accSet != ACCSET_NONE);
    switch (accSet) {
    case ACCSET_STACK:  ok = isStack;       break;
    case ACCSET_RSTACK: ok = isRstack;      break;
    case ACCSET_OTHER:  ok = isUnknown;     break;
    default:
        
        
        JS_ASSERT(compressAccSet(accSet).val == MINI_ACCSET_MULTIPLE.val);
        ok = true;
        break;
    }

    if (!ok) {
        InsBuf b1, b2;
        printer->formatIns(&b1, base);
        JS_snprintf(b2.buf, b2.len, "but the base pointer (%s) doesn't match", b1.buf);
        errorAccSet(lirNames[op], accSet, b2.buf);
     }
}
#endif

#if defined NJ_VERBOSE
void
LInsPrinter::formatGuard(InsBuf *buf, LIns *ins)
{
    RefBuf b1, b2;
    VMSideExit *x = (VMSideExit *)ins->record()->exit;
    VMPI_snprintf(buf->buf, buf->len,
            "%s: %s %s -> pc=%p imacpc=%p sp%+ld rp%+ld (GuardID=%03d)",
            formatRef(&b1, ins),
            lirNames[ins->opcode()],
            ins->oprnd1() ? formatRef(&b2, ins->oprnd1()) : "",
            (void *)x->pc,
            (void *)x->imacpc,
            (long int)x->sp_adj,
            (long int)x->rp_adj,
            ins->record()->profGuardID);
}

void
LInsPrinter::formatGuardXov(InsBuf *buf, LIns *ins)
{
    RefBuf b1, b2, b3;
    VMSideExit *x = (VMSideExit *)ins->record()->exit;
    VMPI_snprintf(buf->buf, buf->len,
            "%s = %s %s, %s -> pc=%p imacpc=%p sp%+ld rp%+ld (GuardID=%03d)",
            formatRef(&b1, ins),
            lirNames[ins->opcode()],
            formatRef(&b2, ins->oprnd1()),
            formatRef(&b3, ins->oprnd2()),
            (void *)x->pc,
            (void *)x->imacpc,
            (long int)x->sp_adj,
            (long int)x->rp_adj,
            ins->record()->profGuardID);
}

const char*
nanojit::LInsPrinter::accNames[] = {
    "s",    
    "r",    
    "o",    
              "?", "?", "?", "?", "?", "?", "?", "?",   
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",   
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",   
    "?"                                                 
};
#endif

} 

JS_DEFINE_CALLINFO_2(extern, STRING, js_IntToString, CONTEXT, INT32, 1, nanojit::ACCSET_NONE)

namespace js {

using namespace nanojit;

#if JS_HAS_XML_SUPPORT
#define RETURN_VALUE_IF_XML(val, ret)                                         \
    JS_BEGIN_MACRO                                                            \
        if (!val.isPrimitive() && val.toObject().isXML())        \
            RETURN_VALUE("xml detected", ret);                                \
    JS_END_MACRO
#else
#define RETURN_IF_XML(val, ret) ((void) 0)
#endif

#define RETURN_IF_XML_A(val) RETURN_VALUE_IF_XML(val, ARECORD_STOP)
#define RETURN_IF_XML(val)   RETURN_VALUE_IF_XML(val, RECORD_STOP)

JS_STATIC_ASSERT(sizeof(JSValueType) == 1);
JS_STATIC_ASSERT(offsetof(TraceNativeStorage, stack_global_buf) % 16 == 0);


#ifdef DEBUG
static char
TypeToChar(JSValueType type)
{
    switch (type) {
      case JSVAL_TYPE_DOUBLE: return 'D';
      case JSVAL_TYPE_INT32: return 'I';
      case JSVAL_TYPE_STRING: return 'S';
      case JSVAL_TYPE_OBJECT: return '!';
      case JSVAL_TYPE_BOOLEAN: return 'B';
      case JSVAL_TYPE_NULL: return 'N';
      case JSVAL_TYPE_UNDEFINED: return 'U';
      case JSVAL_TYPE_MAGIC: return 'M';
      case JSVAL_TYPE_FUNOBJ: return 'F';
      case JSVAL_TYPE_NONFUNOBJ: return 'O';
      case JSVAL_TYPE_BOXED: return '#';
      case JSVAL_TYPE_STRORNULL: return 's';
      case JSVAL_TYPE_OBJORNULL: return 'o';
      case JSVAL_TYPE_UNINITIALIZED: return '*';
    }
    return '?';
}

static char
ValueToTypeChar(const Value &v)
{
    if (v.isInt32()) return 'I';
    if (v.isDouble()) return 'D';
    if (v.isString()) return 'S';
    if (v.isObject()) return v.toObject().isFunction() ? 'F' : 'O';
    if (v.isBoolean()) return 'B';
    if (v.isNull()) return 'N';
    if (v.isUndefined()) return 'U';
    if (v.isMagic()) return 'M';
    return '?';
}
#endif








#define HOTLOOP 4


#define BL_ATTEMPTS 2


#define BL_BACKOFF 32


#define MAX_LOOP_EXECS 300


#define HOTEXIT 1


#define MAXEXIT 3


#define MAXPEERS 9


#define MAX_CALLDEPTH 10


#define MAX_TABLE_SWITCH 256


#define MAX_INTERP_STACK_BYTES                                                \
    (MAX_NATIVE_STACK_SLOTS * sizeof(Value) +                                 \
     MAX_CALL_STACK_ENTRIES * sizeof(JSInlineFrame) +                         \
     sizeof(JSInlineFrame)) /* possibly slow native frame at top of stack */


#define MAX_BRANCHES 32

#define CHECK_STATUS(expr)                                                    \
    JS_BEGIN_MACRO                                                            \
        RecordingStatus _status = (expr);                                     \
        if (_status != RECORD_CONTINUE)                                       \
          return _status;                                                     \
    JS_END_MACRO

#define CHECK_STATUS_A(expr)                                                  \
    JS_BEGIN_MACRO                                                            \
        AbortableRecordingStatus _status = InjectStatus((expr));              \
        if (_status != ARECORD_CONTINUE)                                      \
          return _status;                                                     \
    JS_END_MACRO

#ifdef JS_JIT_SPEW
#define RETURN_VALUE(msg, value)                                              \
    JS_BEGIN_MACRO                                                            \
        debug_only_printf(LC_TMAbort, "trace stopped: %d: %s\n", __LINE__, (msg)); \
        return (value);                                                       \
    JS_END_MACRO
#else
#define RETURN_VALUE(msg, value)   return (value)
#endif

#define RETURN_STOP(msg)     RETURN_VALUE(msg, RECORD_STOP)
#define RETURN_STOP_A(msg)   RETURN_VALUE(msg, ARECORD_STOP)
#define RETURN_ERROR(msg)    RETURN_VALUE(msg, RECORD_ERROR)
#define RETURN_ERROR_A(msg)  RETURN_VALUE(msg, ARECORD_ERROR)

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

static JSBool
jitstats_getOnTrace(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    *vp = BOOLEAN_TO_JSVAL(JS_ON_TRACE(cx));
    return true;
}

static JSPropertySpec jitstats_props[] = {
#define JITSTAT(x) { #x, STAT ## x ## ID, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT },
#include "jitstats.tbl"
#undef JITSTAT
    { "onTrace", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT, jitstats_getOnTrace, NULL },
    { 0 }
};

static JSBool
jitstats_getProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    int index = -1;

    if (JSID_IS_STRING(id)) {
        JSString* str = JSID_TO_STRING(id);
        if (strcmp(JS_GetStringBytes(str), "HOTLOOP") == 0) {
            *vp = INT_TO_JSVAL(HOTLOOP);
            return JS_TRUE;
        }
    }

    if (JSID_IS_INT(id))
        index = JSID_TO_INT(id);

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
        *vp = INT_TO_JSVAL(jsint(result));
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
InitJITStatsClass(JSContext *cx, JSObject *glob)
{
    JS_InitClass(cx, glob, NULL, &jitstats_class, NULL, 0, jitstats_props, NULL, NULL, NULL);
}

#define AUDIT(x) (jitstats.x++)
#else
#define AUDIT(x) ((void)0)
#endif 







#define INS_CONST(c)          addName(lir->insImmI(c), #c)
#define INS_CONSTU(c)         addName(lir->insImmI((uint32_t)c), #c)
#define INS_CONSTPTR(p)       addName(lir->insImmP(p), #p)
#define INS_CONSTWORD(v)      addName(lir->insImmP((void *) (v)), #v)
#define INS_CONSTQWORD(c)     addName(lir->insImmQ(c), #c)
#define INS_CONSTOBJ(obj)     addName(insImmObj(obj), #obj)
#define INS_CONSTFUN(fun)     addName(insImmFun(fun), #fun)
#define INS_CONSTSTR(str)     addName(insImmStr(str), #str)
#define INS_CONSTSHAPE(shape) addName(insImmShape(shape), #shape)
#define INS_CONSTID(id)       addName(insImmId(id), #id)
#define INS_ATOM(atom)        INS_CONSTSTR(ATOM_TO_STRING(atom))
#define INS_NULL()            INS_CONSTPTR(NULL)
#define INS_UNDEFINED()       INS_CONST(0)

static const size_t sPayloadOffset = offsetof(jsval_layout, s.payload);
#if JS_BITS_PER_WORD == 32
static const size_t sTagOffset = offsetof(jsval_layout, s.tag);
#endif

static avmplus::AvmCore s_core = avmplus::AvmCore();
static avmplus::AvmCore* core = &s_core;

static void OutOfMemoryAbort()
{
    JS_NOT_REACHED("out of memory");
    abort();
}

#ifdef JS_JIT_SPEW
static void
DumpPeerStability(TraceMonitor* tm, const void* ip, JSObject* globalObj, uint32 globalShape, uint32 argc);
#endif








static bool did_we_check_processor_features = false;







LogControl LogController;

#ifdef JS_JIT_SPEW





static bool did_we_set_up_debug_logging = false;

static void
InitJITLogController()
{
    char *tm, *tmf;
    uint32_t bits;

    LogController.lcbits = 0;

    tm = getenv("TRACEMONKEY");
    if (tm) {
        fflush(NULL);
        printf(
            "The environment variable $TRACEMONKEY has been replaced by $TMFLAGS.\n"
            "Try 'TMFLAGS=help js -j' for a list of options.\n"
        );
        exit(0);
    }

    tmf = getenv("TMFLAGS");
    if (!tmf) return;

    
    if (strstr(tmf, "help")) {
        fflush(NULL);
        printf(
            "usage: TMFLAGS=option,option,option,... where options can be:\n"
            "\n"
            "  help         show this message\n"
            "  ------ options for jstracer & jsregexp ------\n"
            "  minimal      ultra-minimalist output; try this first\n"
            "  full         everything except 'treevis' and 'fragprofile'\n"
            "  tracer       tracer lifetime (FIXME:better description)\n"
            "  recorder     trace recording stuff (FIXME:better description)\n"
            "  abort        show trace recording aborts\n"
            "  stats        show trace recording stats\n"
            "  regexp       show compilation & entry for regexps\n"
            "  treevis      spew that tracevis/tree.py can parse\n"
            "  ------ options for Nanojit ------\n"
            "  fragprofile  count entries and exits for each fragment\n"
            "  liveness     show LIR liveness at start of reader pipeline\n"
            "  readlir      show LIR as it enters the reader pipeline\n"
            "  aftersf      show LIR after StackFilter\n"
            "  afterdce     show LIR after dead code elimination\n"
            "  native       show native code (interleaved with 'afterdce')\n"
            "  regalloc     show regalloc state in 'native' output\n"
            "  activation   show activation state in 'native' output\n"
            "\n"
        );
        exit(0);
        
    }

    bits = 0;

    
    if (strstr(tmf, "minimal")  || strstr(tmf, "full")) bits |= LC_TMMinimal;
    if (strstr(tmf, "tracer")   || strstr(tmf, "full")) bits |= LC_TMTracer;
    if (strstr(tmf, "recorder") || strstr(tmf, "full")) bits |= LC_TMRecorder;
    if (strstr(tmf, "abort")    || strstr(tmf, "full")) bits |= LC_TMAbort;
    if (strstr(tmf, "stats")    || strstr(tmf, "full")) bits |= LC_TMStats;
    if (strstr(tmf, "regexp")   || strstr(tmf, "full")) bits |= LC_TMRegexp;
    if (strstr(tmf, "treevis"))                         bits |= LC_TMTreeVis;

    
    if (strstr(tmf, "fragprofile"))                       bits |= LC_FragProfile;
    if (strstr(tmf, "liveness")   || strstr(tmf, "full")) bits |= LC_Liveness;
    if (strstr(tmf, "readlir")    || strstr(tmf, "full")) bits |= LC_ReadLIR;
    if (strstr(tmf, "aftersf")    || strstr(tmf, "full")) bits |= LC_AfterSF;
    if (strstr(tmf, "afterdce")   || strstr(tmf, "full")) bits |= LC_AfterDCE;
    if (strstr(tmf, "native")     || strstr(tmf, "full")) bits |= LC_Native;
    if (strstr(tmf, "regalloc")   || strstr(tmf, "full")) bits |= LC_RegAlloc;
    if (strstr(tmf, "activation") || strstr(tmf, "full")) bits |= LC_Activation;

    LogController.lcbits = bits;
    return;

}
#endif



#ifdef JS_JIT_SPEW








template<class T>
static
Seq<T>* reverseInPlace(Seq<T>* seq)
{
    Seq<T>* prev = NULL;
    Seq<T>* curr = seq;
    while (curr) {
        Seq<T>* next = curr->tail;
        curr->tail = prev;
        prev = curr;
        curr = next;
    }
    return prev;
}


#define N_TOP_BLOCKS 50


struct GuardPI {
    uint32_t guardID; 
    uint32_t count;   
};

struct FragPI {
    uint32_t count;          
    uint32_t nStaticExits;   
    size_t nCodeBytes;       
    size_t nExitBytes;       
    Seq<GuardPI>* guards;    
    uint32_t largestGuardID; 
};

void
FragProfiling_FragFinalizer(Fragment* f, TraceMonitor* tm)
{
    
    
    if (!(LogController.lcbits & LC_FragProfile))
        return;

    NanoAssert(f);
    
    NanoAssert(f->profFragID >= 1);
    
    
    NanoAssert(!tm->profTab->containsKey(f->profFragID));

    FragPI pi = { f->profCount,
                  f->nStaticExits,
                  f->nCodeBytes,
                  f->nExitBytes,
                  NULL, 0 };

    
    SeqBuilder<GuardPI> guardsBuilder(*tm->profAlloc);
    GuardRecord* gr;
    uint32_t nGs = 0;
    uint32_t sumOfDynExits = 0;
    for (gr = f->guardsForFrag; gr; gr = gr->nextInFrag) {
         nGs++;
         
         
         
         
         NanoAssert(gr->profGuardID > 0);
         sumOfDynExits += gr->profCount;
         GuardPI gpi = { gr->profGuardID, gr->profCount };
         guardsBuilder.add(gpi);
         if (gr->profGuardID > pi.largestGuardID)
             pi.largestGuardID = gr->profGuardID;
    }
    pi.guards = guardsBuilder.get();
    
    pi.guards = reverseInPlace(pi.guards);

    
    
    
    
    
    
    NanoAssert(nGs >= f->nStaticExits);

    
    
    
    
    NanoAssert(f->profCount >= sumOfDynExits);

    

    tm->profTab->put(f->profFragID, pi);
}

static void
FragProfiling_showResults(TraceMonitor* tm)
{
    uint32_t topFragID[N_TOP_BLOCKS];
    FragPI   topPI[N_TOP_BLOCKS];
    uint64_t totCount = 0, cumulCount;
    uint32_t totSE = 0;
    size_t   totCodeB = 0, totExitB = 0;
    PodArrayZero(topFragID);
    PodArrayZero(topPI);
    FragStatsMap::Iter iter(*tm->profTab);
    while (iter.next()) {
        uint32_t fragID  = iter.key();
        FragPI   pi      = iter.value();
        uint32_t count   = pi.count;
        totCount += (uint64_t)count;
        
        int r = N_TOP_BLOCKS-1;
        while (true) {
            if (r == -1)
                break;
            if (topFragID[r] == 0) {
                r--;
                continue;
            }
            if (count > topPI[r].count) {
                r--;
                continue;
            }
            break;
        }
        r++;
        NanoAssert(r >= 0 && r <= N_TOP_BLOCKS);
        

        if (r < N_TOP_BLOCKS) {
            for (int s = N_TOP_BLOCKS-1; s > r; s--) {
                topFragID[s] = topFragID[s-1];
                topPI[s]     = topPI[s-1];
            }
            topFragID[r] = fragID;
            topPI[r]     = pi;
        }
    }

    LogController.printf(
        "\n----------------- Per-fragment execution counts ------------------\n");
    LogController.printf(
        "\nTotal count = %llu\n\n", (unsigned long long int)totCount);

    LogController.printf(
        "           Entry counts         Entry counts       ----- Static -----\n");
    LogController.printf(
        "         ------Self------     ----Cumulative---   Exits  Cbytes Xbytes   FragID\n");
    LogController.printf("\n");

    if (totCount == 0)
        totCount = 1; 
    cumulCount = 0;
    int r;
    for (r = 0; r < N_TOP_BLOCKS; r++) {
        if (topFragID[r] == 0)
            break;
        cumulCount += (uint64_t)topPI[r].count;
        LogController.printf("%3d:     %5.2f%% %9u     %6.2f%% %9llu"
                             "     %3d   %5u  %5u   %06u\n",
                             r,
                             (double)topPI[r].count * 100.0 / (double)totCount,
                             topPI[r].count,
                             (double)cumulCount * 100.0 / (double)totCount,
                             (unsigned long long int)cumulCount,
                             topPI[r].nStaticExits,
                             (unsigned int)topPI[r].nCodeBytes,
                             (unsigned int)topPI[r].nExitBytes,
                             topFragID[r]);
        totSE += (uint32_t)topPI[r].nStaticExits;
        totCodeB += topPI[r].nCodeBytes;
        totExitB += topPI[r].nExitBytes;
    }
    LogController.printf("\nTotal displayed code bytes = %u, "
                            "exit bytes = %u\n"
                            "Total displayed static exits = %d\n\n",
                            (unsigned int)totCodeB, (unsigned int)totExitB, totSE);

    LogController.printf("Analysis by exit counts\n\n");

    for (r = 0; r < N_TOP_BLOCKS; r++) {
        if (topFragID[r] == 0)
            break;
        LogController.printf("FragID=%06u, total count %u:\n", topFragID[r],
                                topPI[r].count);
        uint32_t madeItToEnd = topPI[r].count;
        uint32_t totThisFrag = topPI[r].count;
        if (totThisFrag == 0)
            totThisFrag = 1;
        GuardPI gpi;
        
        for (Seq<GuardPI>* guards = topPI[r].guards; guards; guards = guards->tail) {
            gpi = (*guards).head;
            if (gpi.count == 0)
                continue;
            madeItToEnd -= gpi.count;
            LogController.printf("   GuardID=%03u    %7u (%5.2f%%)\n",
                                    gpi.guardID, gpi.count,
                                    100.0 * (double)gpi.count / (double)totThisFrag);
        }
        LogController.printf("   Looped (%03u)   %7u (%5.2f%%)\n",
                                topPI[r].largestGuardID+1,
                                madeItToEnd,
                                100.0 * (double)madeItToEnd /  (double)totThisFrag);
        NanoAssert(madeItToEnd <= topPI[r].count); 
        LogController.printf("\n");
    }

    tm->profTab = NULL;
}

#endif



#ifdef DEBUG
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

static JSBool FASTCALL
PrintOnTrace(char* format, uint32 argc, double *argv)
{
    union {
        struct {
            uint32 lo;
            uint32 hi;
        } i;
        double   d;
        char     *cstr;
        JSObject *o;
        JSString *s;
    } u;

#define GET_ARG() JS_BEGIN_MACRO          \
        if (argi >= argc) { \
        fprintf(out, "[too few args for format]"); \
        break;       \
} \
    u.d = argv[argi++]; \
    JS_END_MACRO

    FILE *out = stderr;

    uint32 argi = 0;
    for (char *p = format; *p; ++p) {
        if (*p != '%') {
            putc(*p, out);
            continue;
        }
        char ch = *++p;
        if (!ch) {
            fprintf(out, "[trailing %%]");
            continue;
        }

        switch (ch) {
        case 'a':
            GET_ARG();
            fprintf(out, "[%u:%u 0x%x:0x%x %f]", u.i.lo, u.i.hi, u.i.lo, u.i.hi, u.d);
            break;
        case 'd':
            GET_ARG();
            fprintf(out, "%d", u.i.lo);
            break;
        case 'u':
            GET_ARG();
            fprintf(out, "%u", u.i.lo);
            break;
        case 'x':
            GET_ARG();
            fprintf(out, "%x", u.i.lo);
            break;
        case 'f':
            GET_ARG();
            fprintf(out, "%f", u.d);
            break;
        case 'o':
            GET_ARG();
            js_DumpObject(u.o);
            break;
        case 's':
            GET_ARG();
            {
                size_t length = u.s->length();
                
                if (length > 1 << 16)
                    length = 1 << 16;
                jschar *chars = u.s->chars();
                for (unsigned i = 0; i < length; ++i) {
                    jschar co = chars[i];
                    if (co < 128)
                        putc(co, out);
                    else if (co < 256)
                        fprintf(out, "\\u%02x", co);
                    else
                        fprintf(out, "\\u%04x", co);
                }
            }
            break;
        case 'S':
            GET_ARG();
            fprintf(out, "%s", u.cstr);
            break;
        case 'v': {
            GET_ARG();
            Value *v = (Value *) u.i.lo;
            js_DumpValue(*v);
            break;
        }
        default:
            fprintf(out, "[invalid %%%c]", *p);
        }
    }

#undef GET_ARG

    return JS_TRUE;
}

JS_DEFINE_CALLINFO_3(extern, BOOL, PrintOnTrace, CHARPTR, UINT32, DOUBLEPTR, 0, ACCSET_STORE_ANY)



void
TraceRecorder::tprint(const char *format, int count, nanojit::LIns *insa[])
{
    size_t size = strlen(format) + 1;
    char* data = (char*) traceMonitor->traceAlloc->alloc(size);
    memcpy(data, format, size);

    double *args = (double*) traceMonitor->traceAlloc->alloc(count * sizeof(double));
    for (int i = 0; i < count; ++i) {
        JS_ASSERT(insa[i]);
        lir->insStore(insa[i], INS_CONSTPTR(args), sizeof(double) * i, ACCSET_OTHER);
    }

    LIns* args_ins[] = { INS_CONSTPTR(args), INS_CONST(count), INS_CONSTPTR(data) };
    LIns* call_ins = lir->insCall(&PrintOnTrace_ci, args_ins);
    guard(false, lir->insEqI_0(call_ins), MISMATCH_EXIT);
}


void
TraceRecorder::tprint(const char *format)
{
    LIns* insa[] = { NULL };
    tprint(format, 0, insa);
}

void
TraceRecorder::tprint(const char *format, LIns *ins)
{
    LIns* insa[] = { ins };
    tprint(format, 1, insa);
}

void
TraceRecorder::tprint(const char *format, LIns *ins1, LIns *ins2)
{
    LIns* insa[] = { ins1, ins2 };
    tprint(format, 2, insa);
}

void
TraceRecorder::tprint(const char *format, LIns *ins1, LIns *ins2, LIns *ins3)
{
    LIns* insa[] = { ins1, ins2, ins3 };
    tprint(format, 3, insa);
}

void
TraceRecorder::tprint(const char *format, LIns *ins1, LIns *ins2, LIns *ins3, LIns *ins4)
{
    LIns* insa[] = { ins1, ins2, ins3, ins4 };
    tprint(format, 4, insa);
}

void
TraceRecorder::tprint(const char *format, LIns *ins1, LIns *ins2, LIns *ins3, LIns *ins4,
                      LIns *ins5)
{
    LIns* insa[] = { ins1, ins2, ins3, ins4, ins5 };
    tprint(format, 5, insa);
}

void
TraceRecorder::tprint(const char *format, LIns *ins1, LIns *ins2, LIns *ins3, LIns *ins4,
                      LIns *ins5, LIns *ins6)
{
    LIns* insa[] = { ins1, ins2, ins3, ins4, ins5, ins6 };
    tprint(format, 6, insa);
}
#endif

Tracker::Tracker()
{
    pagelist = NULL;
}

Tracker::~Tracker()
{
    clear();
}

inline jsuword
Tracker::getTrackerPageBase(const void* v) const
{
    return jsuword(v) & ~TRACKER_PAGE_MASK;
}

inline jsuword
Tracker::getTrackerPageOffset(const void* v) const
{
    return (jsuword(v) & TRACKER_PAGE_MASK) >> 2;
}

struct Tracker::TrackerPage*
Tracker::findTrackerPage(const void* v) const
{
    jsuword base = getTrackerPageBase(v);
    struct Tracker::TrackerPage* p = pagelist;
    while (p) {
        if (p->base == base)
            return p;
        p = p->next;
    }
    return NULL;
}

struct Tracker::TrackerPage*
Tracker::addTrackerPage(const void* v)
{
    jsuword base = getTrackerPageBase(v);
    struct TrackerPage* p = (struct TrackerPage*) js_calloc(sizeof(*p));
    p->base = base;
    p->next = pagelist;
    pagelist = p;
    return p;
}

void
Tracker::clear()
{
    while (pagelist) {
        TrackerPage* p = pagelist;
        pagelist = pagelist->next;
        js_free(p);
    }
}

bool
Tracker::has(const void *v) const
{
    return get(v) != NULL;
}

LIns*
Tracker::get(const void* v) const
{
    struct Tracker::TrackerPage* p = findTrackerPage(v);
    if (!p)
        return NULL;
    return p->map[getTrackerPageOffset(v)];
}

void
Tracker::set(const void* v, LIns* i)
{
    struct Tracker::TrackerPage* p = findTrackerPage(v);
    if (!p)
        p = addTrackerPage(v);
    p->map[getTrackerPageOffset(v)] = i;
}

static inline jsuint
argSlots(JSStackFrame* fp)
{
    return fp->isEvalFrame() ? 0 : JS_MAX(fp->numActualArgs(), fp->numFormalArgs());
}

static inline jsuint
numActualArgs(JSStackFrame* fp)
{
    return fp->isEvalFrame() ? 0 : fp->numActualArgs();
}

static inline bool
hasInt32Repr(const Value &v)
{
    if (!v.isNumber())
        return false;
    if (v.isInt32())
        return true;
    int32_t _;
    return JSDOUBLE_IS_INT32(v.toDouble(), &_);
}

static inline jsint
asInt32(const Value &v)
{
    JS_ASSERT(v.isNumber());
    if (v.isInt32())
        return v.toInt32();
#ifdef DEBUG
    int32_t _;
    JS_ASSERT(JSDOUBLE_IS_INT32(v.toDouble(), &_));
#endif
    return jsint(v.toDouble());
}






static inline JSValueType
getPromotedType(const Value &v)
{
    if (v.isNumber())
        return JSVAL_TYPE_DOUBLE;
    if (v.isObject())
        return v.toObject().isFunction() ? JSVAL_TYPE_FUNOBJ : JSVAL_TYPE_NONFUNOBJ;
    return v.extractNonDoubleObjectTraceType();
}






static inline JSValueType
getCoercedType(const Value &v)
{
    if (v.isNumber()) {
        int32_t _;
        return (v.isInt32() || JSDOUBLE_IS_INT32(v.toDouble(), &_))
               ? JSVAL_TYPE_INT32
               : JSVAL_TYPE_DOUBLE;
    }
    if (v.isObject())
        return v.toObject().isFunction() ? JSVAL_TYPE_FUNOBJ : JSVAL_TYPE_NONFUNOBJ;
    return v.extractNonDoubleObjectTraceType();
}

static inline JSValueType
getFrameObjPtrTraceType(JSObject *obj)
{
    JS_ASSERT_IF(obj, !obj->isFunction());
    return obj ? JSVAL_TYPE_NONFUNOBJ : JSVAL_TYPE_NULL;
}

static inline bool
IsFrameObjPtrTraceType(JSValueType t)
{
    return t == JSVAL_TYPE_NULL || t == JSVAL_TYPE_NONFUNOBJ || t == JSVAL_TYPE_FUNOBJ;
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
StackSlotHash(JSContext* cx, unsigned slot, const void* pc)
{
    uintptr_t h = HASH_SEED;
    HashAccum(h, uintptr_t(cx->fp()->getScript()), ORACLE_MASK);
    HashAccum(h, uintptr_t(pc), ORACLE_MASK);
    HashAccum(h, uintptr_t(slot), ORACLE_MASK);
    return int(h);
}

static JS_REQUIRES_STACK inline int
GlobalSlotHash(JSContext* cx, unsigned slot)
{
    uintptr_t h = HASH_SEED;
    JSStackFrame* fp = cx->fp();

    while (fp->down)
        fp = fp->down;

    HashAccum(h, uintptr_t(fp->maybeScript()), ORACLE_MASK);
    HashAccum(h, uintptr_t(fp->getScopeChain()->getGlobal()->shape()), ORACLE_MASK);
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
Oracle::markStackSlotUndemotable(JSContext* cx, unsigned slot, const void* pc)
{
    _stackDontDemote.set(StackSlotHash(cx, slot, pc));
}

JS_REQUIRES_STACK void
Oracle::markStackSlotUndemotable(JSContext* cx, unsigned slot)
{
    markStackSlotUndemotable(cx, slot, cx->regs->pc);
}


JS_REQUIRES_STACK bool
Oracle::isStackSlotUndemotable(JSContext* cx, unsigned slot, const void* pc) const
{
    return _stackDontDemote.get(StackSlotHash(cx, slot, pc));
}

JS_REQUIRES_STACK bool
Oracle::isStackSlotUndemotable(JSContext* cx, unsigned slot) const
{
    return isStackSlotUndemotable(cx, slot, cx->regs->pc);
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

JS_REQUIRES_STACK void
TraceRecorder::markSlotUndemotable(LinkableFragment* f, unsigned slot)
{
    if (slot < f->nStackTypes) {
        traceMonitor->oracle->markStackSlotUndemotable(cx, slot);
        return;
    }

    uint16* gslots = f->globalSlots->data();
    traceMonitor->oracle->markGlobalSlotUndemotable(cx, gslots[slot - f->nStackTypes]);
}

JS_REQUIRES_STACK void
TraceRecorder::markSlotUndemotable(LinkableFragment* f, unsigned slot, const void* pc)
{
    if (slot < f->nStackTypes) {
        traceMonitor->oracle->markStackSlotUndemotable(cx, slot, pc);
        return;
    }

    uint16* gslots = f->globalSlots->data();
    traceMonitor->oracle->markGlobalSlotUndemotable(cx, gslots[slot - f->nStackTypes]);
}

static JS_REQUIRES_STACK bool
IsSlotUndemotable(Oracle* oracle, JSContext* cx, LinkableFragment* f, unsigned slot, const void* ip)
{
    if (slot < f->nStackTypes)
        return !oracle || oracle->isStackSlotUndemotable(cx, slot, ip);

    uint16* gslots = f->globalSlots->data();
    return !oracle || oracle->isGlobalSlotUndemotable(cx, gslots[slot - f->nStackTypes]);
}

class FrameInfoCache
{
    struct HashPolicy
    {
        typedef FrameInfo *Lookup;
        static HashNumber hash(const FrameInfo* fi) {
            size_t len = sizeof(FrameInfo) + fi->callerHeight * sizeof(JSValueType);
            HashNumber h = 0;
            const unsigned char *s = (const unsigned char*)fi;
            for (size_t i = 0; i < len; i++, s++)
                h = JS_ROTATE_LEFT32(h, 4) ^ *s;
            return h;
        }

        static bool match(const FrameInfo* fi1, const FrameInfo* fi2) {
            if (memcmp(fi1, fi2, sizeof(FrameInfo)) != 0)
                return false;
            return memcmp(fi1->get_typemap(), fi2->get_typemap(),
                          fi1->callerHeight * sizeof(JSValueType)) == 0;
        }
    };

    typedef HashSet<FrameInfo *, HashPolicy, SystemAllocPolicy> FrameSet;

    FrameSet set;
    VMAllocator *allocator;

  public:

    FrameInfoCache(VMAllocator *allocator);

    void reset() {
        set.clear();
    }

    FrameInfo *memoize(FrameInfo *fi) {
        FrameSet::AddPtr p = set.lookupForAdd(fi);
        if (!p) {
            FrameInfo* n = (FrameInfo*)
                allocator->alloc(sizeof(FrameInfo) + fi->callerHeight * sizeof(JSValueType));
            memcpy(n, fi, sizeof(FrameInfo) + fi->callerHeight * sizeof(JSValueType));
            if (!set.add(p, n))
                return NULL;
        }

        return *p;
    }
};

FrameInfoCache::FrameInfoCache(VMAllocator *allocator)
  : allocator(allocator)
{
    if (!set.init())
        OutOfMemoryAbort();
}

#define PC_HASH_COUNT 1024

static void
Blacklist(jsbytecode* pc)
{
    AUDIT(blacklisted);
    JS_ASSERT(*pc == JSOP_TRACE || *pc == JSOP_NOP);
    *pc = JSOP_NOP;
}

static bool
IsBlacklisted(jsbytecode* pc)
{
    if (*pc == JSOP_NOP)
        return true;
    if (*pc == JSOP_CALL)
        return *(pc + JSOP_CALL_LENGTH) == JSOP_NOP;
    return false;
}

static void
Backoff(JSContext *cx, jsbytecode* pc, Fragment* tree = NULL)
{
    
    RecordAttemptMap &table = *JS_TRACE_MONITOR(cx).recordAttempts;
    if (RecordAttemptMap::AddPtr p = table.lookupForAdd(pc)) {
        if (p->value++ > (BL_ATTEMPTS * MAXPEERS)) {
            p->value = 0;
            Blacklist(pc);
            return;
        }
    } else {
        table.add(p, pc, 0);
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
    RecordAttemptMap &table = *JS_TRACE_MONITOR(cx).recordAttempts;
    if (RecordAttemptMap::Ptr p = table.lookup(pc))
        p->value = 0;
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

static void
RawLookupFirstPeer(TraceMonitor* tm, const void *ip, JSObject* globalObj,
                   uint32 globalShape, uint32 argc,
                   TreeFragment*& firstInBucket, TreeFragment**& prevTreeNextp)
{
    size_t h = FragmentHash(ip, globalObj, globalShape, argc);
    TreeFragment** ppf = &tm->vmfragments[h];
    firstInBucket = *ppf;
    for (; TreeFragment* pf = *ppf; ppf = &pf->next) {
        if (pf->globalObj == globalObj &&
            pf->globalShape == globalShape &&
            pf->ip == ip &&
            pf->argc == argc) {
            prevTreeNextp = ppf;
            return;
        }
    }
    prevTreeNextp = ppf;
    return;
}

static TreeFragment*
LookupLoop(TraceMonitor* tm, const void *ip, JSObject* globalObj,
                uint32 globalShape, uint32 argc)
{
    TreeFragment *_, **prevTreeNextp;
    RawLookupFirstPeer(tm, ip, globalObj, globalShape, argc, _, prevTreeNextp);
    return *prevTreeNextp;
}

static TreeFragment*
LookupOrAddLoop(TraceMonitor* tm, const void *ip, JSObject* globalObj,
                uint32 globalShape, uint32 argc)
{
    TreeFragment *firstInBucket, **prevTreeNextp;
    RawLookupFirstPeer(tm, ip, globalObj, globalShape, argc, firstInBucket, prevTreeNextp);
    if (TreeFragment *f = *prevTreeNextp)
        return f;

    verbose_only(
    uint32_t profFragID = (LogController.lcbits & LC_FragProfile)
                          ? (++(tm->lastFragID)) : 0;
    )
    TreeFragment* f = new (*tm->dataAlloc) TreeFragment(ip, tm->dataAlloc, globalObj, globalShape,
                                                        argc verbose_only(, profFragID));
    f->root = f;                
    *prevTreeNextp = f;         
    f->next = NULL;
    f->first = f;               
    f->peer = NULL;
    return f;
}

static TreeFragment*
AddNewPeerToPeerList(TraceMonitor* tm, TreeFragment* peer)
{
    JS_ASSERT(peer);
    verbose_only(
    uint32_t profFragID = (LogController.lcbits & LC_FragProfile)
                          ? (++(tm->lastFragID)) : 0;
    )
    TreeFragment* f = new (*tm->dataAlloc) TreeFragment(peer->ip, tm->dataAlloc, peer->globalObj,
                                                        peer->globalShape, peer->argc
                                                        verbose_only(, profFragID));
    f->root = f;                
    f->first = peer->first;     
    f->peer = peer->peer;
    peer->peer = f;
    
    debug_only(f->next = (TreeFragment*)0xcdcdcdcd);
    return f;
}

JS_REQUIRES_STACK void
TreeFragment::initialize(JSContext* cx, SlotList *globalSlots, bool speculate)
{
    this->dependentTrees.clear();
    this->linkedTrees.clear();
    this->globalSlots = globalSlots;

    
    this->typeMap.captureTypes(cx, globalObj, *globalSlots, 0 , speculate);
    this->nStackTypes = this->typeMap.length() - globalSlots->length();
    this->spOffsetAtEntry = cx->regs->sp - cx->fp()->base();

#ifdef DEBUG
    this->treeFileName = cx->fp()->getScript()->filename;
    this->treeLineNumber = js_FramePCToLineNumber(cx, cx->fp());
    this->treePCOffset = FramePCOffset(cx, cx->fp());
#endif
    this->script = cx->fp()->getScript();
    this->gcthings.clear();
    this->shapes.clear();
    this->unstableExits = NULL;
    this->sideExits.clear();

    
    this->nativeStackBase = (nStackTypes - (cx->regs->sp - cx->fp()->base())) *
                             sizeof(double);
    this->maxNativeStackSlots = nStackTypes;
    this->maxCallDepth = 0;
    this->execs = 0;
}

UnstableExit*
TreeFragment::removeUnstableExit(VMSideExit* exit)
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

#ifdef DEBUG
static void
AssertTreeIsUnique(TraceMonitor* tm, TreeFragment* f)
{
    JS_ASSERT(f->root == f);

    




    for (TreeFragment* peer = LookupLoop(tm, f->ip, f->globalObj, f->globalShape, f->argc);
         peer != NULL;
         peer = peer->peer) {
        if (!peer->code() || peer == f)
            continue;
        JS_ASSERT(!f->typeMap.matches(peer->typeMap));
    }
}
#endif

static void
AttemptCompilation(JSContext *cx, JSObject* globalObj, jsbytecode* pc, uint32 argc)
{
    TraceMonitor *tm = &JS_TRACE_MONITOR(cx);

    
    JS_ASSERT(*pc == JSOP_NOP || *pc == JSOP_TRACE);
    *pc = JSOP_TRACE;
    ResetRecordingAttempts(cx, pc);

    
    TreeFragment* f = LookupLoop(tm, pc, globalObj, globalObj->shape(), argc);
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


static bool
isfop(LIns* i, LOpcode op)
{
    if (i->isop(op))
        return true;
#if NJ_SOFTFLOAT_SUPPORTED
    if (nanojit::AvmCore::config.soft_float &&
        i->isop(LIR_ii2d) &&
        i->oprnd1()->isop(LIR_calli) &&
        i->oprnd2()->isop(LIR_hcalli)) {
        return i->oprnd1()->callInfo() == softFloatOps.opmap[op];
    }
#endif
    return false;
}

static const CallInfo *
fcallinfo(LIns *i)
{
#if NJ_SOFTFLOAT_SUPPORTED
    if (nanojit::AvmCore::config.soft_float) {
        if (!i->isop(LIR_ii2d))
            return NULL;
        i = i->oprnd1();
        return i->isop(LIR_calli) ? i->callInfo() : NULL;
    }
#endif
    return i->isop(LIR_calld) ? i->callInfo() : NULL;
}

static LIns*
fcallarg(LIns* i, int n)
{
#if NJ_SOFTFLOAT_SUPPORTED
    if (nanojit::AvmCore::config.soft_float) {
        NanoAssert(i->isop(LIR_ii2d));
        return i->oprnd1()->callArgN(n);
    }
#endif
    NanoAssert(i->isop(LIR_calld));
    return i->callArgN(n);
}

static LIns*
foprnd1(LIns* i)
{
#if NJ_SOFTFLOAT_SUPPORTED
    if (nanojit::AvmCore::config.soft_float)
        return fcallarg(i, 0);
#endif
    return i->oprnd1();
}

static LIns*
foprnd2(LIns* i)
{
#if NJ_SOFTFLOAT_SUPPORTED
    if (nanojit::AvmCore::config.soft_float)
        return fcallarg(i, 1);
#endif
    return i->oprnd2();
}

static LIns*
demote(LirWriter *out, LIns* ins)
{
    JS_ASSERT(ins->isD());
    if (ins->isCall())
        return ins->callArgN(0);
    if (isfop(ins, LIR_i2d) || isfop(ins, LIR_ui2d))
        return foprnd1(ins);
    JS_ASSERT(ins->isImmD());
    double cf = ins->immD();
    int32_t ci = cf > 0x7fffffff ? uint32_t(cf) : int32_t(cf);
    return out->insImmI(ci);
}

static bool
isPromoteInt(LIns* ins)
{
    if (isfop(ins, LIR_i2d))
        return true;
    if (ins->isImmD()) {
        jsdouble d = ins->immD();
        return d == jsdouble(jsint(d)) && !JSDOUBLE_IS_NEGZERO(d);
    }
    return false;
}

static bool
isPromoteUint(LIns* ins)
{
    if (isfop(ins, LIR_ui2d))
        return true;
    if (ins->isImmD()) {
        jsdouble d = ins->immD();
        return d == jsdouble(jsuint(d)) && !JSDOUBLE_IS_NEGZERO(d);
    }
    return false;
}

static bool
isPromote(LIns* ins)
{
    return isPromoteInt(ins) || isPromoteUint(ins);
}





static bool
IsOverflowSafe(LOpcode op, LIns* i)
{
    LIns* c;
    switch (op) {
      case LIR_addi:
      case LIR_subi:
          return (i->isop(LIR_andi) && ((c = i->oprnd2())->isImmI()) &&
                  ((c->immI() & 0xc0000000) == 0)) ||
                 (i->isop(LIR_rshi) && ((c = i->oprnd2())->isImmI()) &&
                  ((c->immI() > 0)));
    default:
        JS_ASSERT(op == LIR_muli);
    }
    return (i->isop(LIR_andi) && ((c = i->oprnd2())->isImmI()) &&
            ((c->immI() & 0xffff0000) == 0)) ||
           (i->isop(LIR_rshui) && ((c = i->oprnd2())->isImmI()) &&
            ((c->immI() >= 16)));
}

class FuncFilter: public LirWriter
{
public:
    FuncFilter(LirWriter* out):
        LirWriter(out)
    {
    }

    LIns* ins2(LOpcode v, LIns* s0, LIns* s1)
    {
        if (s0 == s1 && v == LIR_eqd) {
            if (isPromote(s0)) {
                
                return insImmI(1);
            }
            if (s0->isop(LIR_muld) || s0->isop(LIR_subd) || s0->isop(LIR_addd)) {
                LIns* lhs = s0->oprnd1();
                LIns* rhs = s0->oprnd2();
                if (isPromote(lhs) && isPromote(rhs)) {
                    
                    return insImmI(1);
                }
            }
        } else if (isCmpDOpcode(v)) {
            if (isPromoteInt(s0) && isPromoteInt(s1)) {
                
                v = cmpOpcodeD2I(v);
                return out->ins2(v, demote(out, s0), demote(out, s1));
            } else if (isPromoteUint(s0) && isPromoteUint(s1)) {
                
                v = cmpOpcodeD2UI(v);
                return out->ins2(v, demote(out, s0), demote(out, s1));
            }
        }
        return out->ins2(v, s0, s1);
    }
};









template <typename Visitor>
static JS_REQUIRES_STACK bool
VisitFrameSlots(Visitor &visitor, JSContext *cx, unsigned depth,
                FrameRegsIter &i, JSStackFrame *up)
{
    JSStackFrame *const fp = i.fp();
    Value *const sp = i.sp();

    if (depth > 0 && !VisitFrameSlots(visitor, cx, depth-1, ++i, fp))
        return false;

    if (fp->argv) {
        if (depth == 0) {
            visitor.setStackSlotKind("args");
            if (!visitor.visitStackSlots(&fp->argv[-2], argSlots(fp) + 2, fp))
                return false;
        }
        visitor.setStackSlotKind("arguments");
        if (!visitor.visitFrameObjPtr(fp->addressArgsObj(), fp))
            return false;
        
        
        
        visitor.setStackSlotKind("scopeChain");
        if (!visitor.visitFrameObjPtr(fp->addressScopeChain(), fp))
            return false;
        visitor.setStackSlotKind("var");
        if (!visitor.visitStackSlots(fp->slots(), fp->getFixedCount(), fp))
            return false;
    }

    visitor.setStackSlotKind("stack");
    Value *base = fp->base();
    JS_ASSERT(sp >= base && sp <= fp->slots() + fp->getSlotCount());
    if (!visitor.visitStackSlots(base, size_t(sp - base), fp))
        return false;
    if (up) {
        int missing = up->numFormalArgs() - up->numActualArgs();
        if (missing > 0) {
            visitor.setStackSlotKind("missing");
            if (!visitor.visitStackSlots(sp, size_t(missing), fp))
                return false;
        }
    }
    return true;
}



const int SPECIAL_FRAME_SLOTS = 2;

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
VisitStackSlots(Visitor &visitor, JSContext *cx, unsigned callDepth)
{
    FrameRegsIter i(cx);
    return VisitFrameSlots(visitor, cx, callDepth, i, NULL);
}

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE void
VisitGlobalSlots(Visitor &visitor, JSContext *cx, JSObject *globalObj,
                 unsigned ngslots, uint16 *gslots)
{
    for (unsigned n = 0; n < ngslots; ++n) {
        unsigned slot = gslots[n];
        visitor.visitGlobalSlot(&globalObj->getSlotRef(slot), n, slot);
    }
}

class AdjustCallerTypeVisitor;

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE void
VisitGlobalSlots(Visitor &visitor, JSContext *cx, SlotList &gslots)
{
    VisitGlobalSlots(visitor, cx, cx->fp()->getScopeChain()->getGlobal(),
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
    VisitSlots(visitor, cx, cx->fp()->getScopeChain()->getGlobal(),
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
    VisitSlots(visitor, cx, cx->fp()->getScopeChain()->getGlobal(),
               callDepth, slots.length(), slots.data());
}


class SlotVisitorBase {
#if defined JS_JIT_SPEW
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
    const void* mStop;
public:
    JS_ALWAYS_INLINE CountSlotsVisitor(const void* stop = NULL) :
        mCount(0),
        mDone(false),
        mStop(stop)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, JSStackFrame* fp) {
        if (mDone)
            return false;
        if (mStop && size_t(((const Value *)mStop) - vp) < count) {
            mCount += size_t(((const Value *)mStop) - vp);
            mDone = true;
            return false;
        }
        mCount += count;
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame* fp) {
        if (mDone)
            return false;
        if (mStop && mStop == p) {
            mDone = true;
            return false;
        }
        mCount++;
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
    FrameRegsIter i(cx);
    unsigned slots = 0;
    unsigned depth = callDepth;
    for (;; ++i) {
        



        JSStackFrame *const fp = i.fp();
        slots += i.sp() - fp->base();
        if (fp->argv)
            slots += fp->getFixedCount() + SPECIAL_FRAME_SLOTS;
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
        int missing = fp->numFormalArgs() - fp->numActualArgs();
        if (missing > 0)
            slots += missing;
    }
    JS_NOT_REACHED("NativeStackSlots");
}

class CaptureTypesVisitor : public SlotVisitorBase
{
    JSContext* mCx;
    JSValueType* mTypeMap;
    JSValueType* mPtr;
    Oracle   * mOracle;

public:
    JS_ALWAYS_INLINE CaptureTypesVisitor(JSContext* cx, JSValueType* typeMap, bool speculate) :
        mCx(cx),
        mTypeMap(typeMap),
        mPtr(typeMap),
        mOracle(speculate ? JS_TRACE_MONITOR(cx).oracle : NULL) {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot) {
            JSValueType type = getCoercedType(*vp);
            if (type == JSVAL_TYPE_INT32 && (!mOracle || mOracle->isGlobalSlotUndemotable(mCx, slot)))
                type = JSVAL_TYPE_DOUBLE;
            JS_ASSERT(type != JSVAL_TYPE_BOXED);
            debug_only_printf(LC_TMTracer,
                              "capture type global%d: %c\n",
                              n, TypeToChar(type));
            *mPtr++ = type;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, int count, JSStackFrame* fp) {
        for (int i = 0; i < count; ++i) {
            JSValueType type = getCoercedType(vp[i]);
            if (type == JSVAL_TYPE_INT32 && (!mOracle || mOracle->isStackSlotUndemotable(mCx, length())))
                type = JSVAL_TYPE_DOUBLE;
            JS_ASSERT(type != JSVAL_TYPE_BOXED);
            debug_only_printf(LC_TMTracer,
                              "capture type %s%d: %c\n",
                              stackSlotKind(), i, TypeToChar(type));
            *mPtr++ = type;
        }
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame* fp) {
        JSValueType type = getFrameObjPtrTraceType(*p);
        debug_only_printf(LC_TMTracer,
                          "capture type %s%d: %c\n",
                          stackSlotKind(), 0, TypeToChar(type));
        *mPtr++ = type;
        return true;
    }

    JS_ALWAYS_INLINE uintptr_t length() {
        return mPtr - mTypeMap;
    }
};

void
TypeMap::set(unsigned stackSlots, unsigned ngslots,
             const JSValueType* stackTypeMap, const JSValueType* globalTypeMap)
{
    setLength(ngslots + stackSlots);
    memcpy(data(), stackTypeMap, stackSlots * sizeof(JSValueType));
    memcpy(data() + stackSlots, globalTypeMap, ngslots * sizeof(JSValueType));
}





JS_REQUIRES_STACK void
TypeMap::captureTypes(JSContext* cx, JSObject* globalObj, SlotList& slots, unsigned callDepth,
                      bool speculate)
{
    setLength(NativeStackSlots(cx, callDepth) + slots.length());
    CaptureTypesVisitor visitor(cx, data(), speculate);
    VisitSlots(visitor, cx, globalObj, callDepth, slots);
    JS_ASSERT(visitor.length() == length());
}

JS_REQUIRES_STACK void
TypeMap::captureMissingGlobalTypes(JSContext* cx, JSObject* globalObj, SlotList& slots, unsigned stackSlots,
                                   bool speculate)
{
    unsigned oldSlots = length() - stackSlots;
    int diff = slots.length() - oldSlots;
    JS_ASSERT(diff >= 0);
    setLength(length() + diff);
    CaptureTypesVisitor visitor(cx, data() + stackSlots + oldSlots, speculate);
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
TypeMap::fromRaw(JSValueType* other, unsigned numSlots)
{
    unsigned oldLength = length();
    setLength(length() + numSlots);
    for (unsigned i = 0; i < numSlots; i++)
        get(oldLength + i) = other[i];
}






static void
MergeTypeMaps(JSValueType** partial, unsigned* plength, JSValueType* complete, unsigned clength, JSValueType* mem)
{
    unsigned l = *plength;
    JS_ASSERT(l < clength);
    memcpy(mem, *partial, l * sizeof(JSValueType));
    memcpy(mem + l, complete + l, (clength - l) * sizeof(JSValueType));
    *partial = mem;
    *plength = clength;
}





static JS_REQUIRES_STACK void
SpecializeTreesToLateGlobals(JSContext* cx, TreeFragment* root, JSValueType* globalTypeMap,
                            unsigned numGlobalSlots)
{
    for (unsigned i = root->nGlobalTypes(); i < numGlobalSlots; i++)
        root->typeMap.add(globalTypeMap[i]);

    JS_ASSERT(root->nGlobalTypes() == numGlobalSlots);

    for (unsigned i = 0; i < root->dependentTrees.length(); i++) {
        TreeFragment* tree = root->dependentTrees[i];
        if (tree->code() && tree->nGlobalTypes() < numGlobalSlots)
            SpecializeTreesToLateGlobals(cx, tree, globalTypeMap, numGlobalSlots);
    }
    for (unsigned i = 0; i < root->linkedTrees.length(); i++) {
        TreeFragment* tree = root->linkedTrees[i];
        if (tree->code() && tree->nGlobalTypes() < numGlobalSlots)
            SpecializeTreesToLateGlobals(cx, tree, globalTypeMap, numGlobalSlots);
    }
}


static JS_REQUIRES_STACK void
SpecializeTreesToMissingGlobals(JSContext* cx, JSObject* globalObj, TreeFragment* root)
{
    
    size_t count = 0;
    for (TreeFragment *f = root->first; f; f = f->peer, ++count);
    bool speculate = count < MAXPEERS-1;

    root->typeMap.captureMissingGlobalTypes(cx, globalObj, *root->globalSlots, root->nStackTypes,
                                            speculate);
    JS_ASSERT(root->globalSlots->length() == root->typeMap.length() - root->nStackTypes);

    SpecializeTreesToLateGlobals(cx, root, root->globalTypeMap(), root->nGlobalTypes());
}

static void
ResetJITImpl(JSContext* cx);

#ifdef MOZ_TRACEVIS
static JS_INLINE void
ResetJIT(JSContext* cx, TraceVisFlushReason r)
{
    LogTraceVisEvent(cx, S_RESET, r);
    ResetJITImpl(cx);
}
#else
# define ResetJIT(cx, reason) ResetJITImpl(cx)
#endif

void
FlushJITCache(JSContext *cx)
{
    ResetJIT(cx, FR_OOM);
}

static void
TrashTree(JSContext* cx, TreeFragment* f);

template <class T>
static T&
InitConst(const T &t)
{
    return const_cast<T &>(t);
}

JS_REQUIRES_STACK
TraceRecorder::TraceRecorder(JSContext* cx, VMSideExit* anchor, VMFragment* fragment,
                             unsigned stackSlots, unsigned ngslots, JSValueType* typeMap,
                             VMSideExit* innermost, jsbytecode* outer, uint32 outerArgc,
                             bool speculate)
  : cx(cx),
    traceMonitor(&JS_TRACE_MONITOR(cx)),
    oracle(speculate ? JS_TRACE_MONITOR(cx).oracle : NULL),
    fragment(fragment),
    tree(fragment->root),
    globalObj(tree->globalObj),
    outer(outer),
    outerArgc(outerArgc),
    lexicalBlock(cx->fp()->maybeBlockChain()),
    anchor(anchor),
    lir(NULL),
    cx_ins(NULL),
    eos_ins(NULL),
    eor_ins(NULL),
    loopLabel(NULL),
    importTypeMap(&tempAlloc()),
    lirbuf(new (tempAlloc()) LirBuffer(tempAlloc())),
    mark(*traceMonitor->traceAlloc),
    numSideExitsBefore(tree->sideExits.length()),
    tracker(),
    nativeFrameTracker(),
    global_dslots(NULL),
    callDepth(anchor ? anchor->calldepth : 0),
    atoms(FrameAtomBase(cx, cx->fp())),
    consts(cx->fp()->getScript()->constOffset
           ? cx->fp()->getScript()->consts()->vector
           : NULL),
    cfgMerges(&tempAlloc()),
    trashSelf(false),
    whichTreesToTrash(&tempAlloc()),
    guardedShapeTable(cx),
    rval_ins(NULL),
    native_rval_ins(NULL),
    newobj_ins(NULL),
    pendingSpecializedNative(NULL),
    pendingUnboxSlot(NULL),
    pendingGuardCondition(NULL),
    pendingLoop(true),
    generatedSpecializedNative(),
    tempTypeMap(cx)
{
    JS_ASSERT(globalObj == cx->fp()->getScopeChain()->getGlobal());
    JS_ASSERT(globalObj->hasOwnShape());
    JS_ASSERT(cx->regs->pc == (jsbytecode*)fragment->ip);

    fragment->lirbuf = lirbuf;
#ifdef DEBUG
    lirbuf->printer = new (tempAlloc()) LInsPrinter(tempAlloc(), TM_NUM_USED_ACCS);
#endif

    




    fragment->lastIns = NULL;
    fragment->setCode(NULL);
    fragment->lirbuf = lirbuf;
    verbose_only( fragment->profCount = 0; )
    verbose_only( fragment->nStaticExits = 0; )
    verbose_only( fragment->nCodeBytes = 0; )
    verbose_only( fragment->nExitBytes = 0; )
    verbose_only( fragment->guardNumberer = 1; )
    verbose_only( fragment->guardsForFrag = NULL; )
    verbose_only( fragment->loopLabel = NULL; )

    




    if (!guardedShapeTable.init())
        abort();

#ifdef JS_JIT_SPEW
    debug_only_print0(LC_TMMinimal, "\n");
    debug_only_printf(LC_TMMinimal, "Recording starting from %s:%u@%u (FragID=%06u)\n",
                      tree->treeFileName, tree->treeLineNumber, tree->treePCOffset,
                      fragment->profFragID);

    debug_only_printf(LC_TMTracer, "globalObj=%p, shape=%d\n",
                      (void*)this->globalObj, this->globalObj->shape());
    debug_only_printf(LC_TMTreeVis, "TREEVIS RECORD FRAG=%p ANCHOR=%p\n", (void*)fragment,
                      (void*)anchor);
#endif

    nanojit::LirWriter*& lir = InitConst(this->lir);
    lir = new (tempAlloc()) LirBufWriter(lirbuf, nanojit::AvmCore::config);
#ifdef DEBUG
    ValidateWriter* validate2;
    lir = validate2 =
        new (tempAlloc()) ValidateWriter(lir, lirbuf->printer, "end of writer pipeline");
#endif
    debug_only_stmt(
        if (LogController.lcbits & LC_TMRecorder) {
           lir = new (tempAlloc()) VerboseWriter(tempAlloc(), lir, lirbuf->printer,
                                               &LogController);
        }
    )
    
    if (avmplus::AvmCore::config.cseopt)
        lir = new (tempAlloc()) CseFilter(lir, TM_NUM_USED_ACCS, tempAlloc());
#if NJ_SOFTFLOAT_SUPPORTED
    if (nanojit::AvmCore::config.soft_float)
        lir = new (tempAlloc()) SoftFloatFilter(lir);
#endif
    lir = new (tempAlloc()) ExprFilter(lir);
    lir = new (tempAlloc()) FuncFilter(lir);
#ifdef DEBUG
    ValidateWriter* validate1;
    lir = validate1 =
        new (tempAlloc()) ValidateWriter(lir, lirbuf->printer, "start of writer pipeline");
#endif
    lir->ins0(LIR_start);

    for (int i = 0; i < NumSavedRegs; ++i)
        lir->insParam(i, 1);
#ifdef DEBUG
    for (int i = 0; i < NumSavedRegs; ++i)
        addName(lirbuf->savedRegs[i], regNames[Assembler::savedRegs[i]]);
#endif

    lirbuf->state = addName(lir->insParam(0, 0), "state");

    if (fragment == fragment->root)
        InitConst(loopLabel) = lir->ins0(LIR_label);

    
    
    
    
    verbose_only( if (LogController.lcbits & LC_FragProfile) {
        LIns* entryLabel = NULL;
        if (fragment == fragment->root) {
            entryLabel = loopLabel;
        } else {
            entryLabel = lir->ins0(LIR_label);
        }
        NanoAssert(entryLabel);
        NanoAssert(!fragment->loopLabel);
        fragment->loopLabel = entryLabel;
    })

#ifdef DEBUG
    
    
    void** extras = new (tempAlloc()) void*[2];
    extras[0] = 0;      
    extras[1] = 0;      
    validate1->setCheckAccSetExtras(extras);
    validate2->setCheckAccSetExtras(extras);
#endif

    lirbuf->sp =
        addName(lir->insLoad(LIR_ldp, lirbuf->state, offsetof(TracerState, sp), ACCSET_OTHER), "sp");
    lirbuf->rp =
        addName(lir->insLoad(LIR_ldp, lirbuf->state, offsetof(TracerState, rp), ACCSET_OTHER), "rp");
    InitConst(cx_ins) =
        addName(lir->insLoad(LIR_ldp, lirbuf->state, offsetof(TracerState, cx), ACCSET_OTHER), "cx");
    InitConst(eos_ins) =
        addName(lir->insLoad(LIR_ldp, lirbuf->state, offsetof(TracerState, eos), ACCSET_OTHER), "eos");
    InitConst(eor_ins) =
        addName(lir->insLoad(LIR_ldp, lirbuf->state, offsetof(TracerState, eor), ACCSET_OTHER), "eor");

#ifdef DEBUG
    
    extras[0] = lirbuf->sp;
    extras[1] = lirbuf->rp;
#endif

    
    if (tree->globalSlots->length() > tree->nGlobalTypes())
        SpecializeTreesToMissingGlobals(cx, globalObj, tree);

    
    import(tree, lirbuf->sp, stackSlots, ngslots, callDepth, typeMap);

    if (fragment == fragment->root) {
        




        LIns* flagptr = INS_CONSTPTR((void *) &JS_THREAD_DATA(cx)->interruptFlags);
        LIns* x = lir->insLoad(LIR_ldi, flagptr, 0, ACCSET_OTHER, LOAD_VOLATILE);
        guard(true, lir->insEqI_0(x), snapshot(TIMEOUT_EXIT));
    }

    



    if (anchor && anchor->exitType == NESTED_EXIT) {
        LIns* nested_ins = addName(lir->insLoad(LIR_ldp, lirbuf->state,
                                                offsetof(TracerState, outermostTreeExitGuard),
                                                ACCSET_OTHER), "outermostTreeExitGuard");
        guard(true, lir->ins2(LIR_eqp, nested_ins, INS_CONSTPTR(innermost)), NESTED_EXIT);
    }
}

TraceRecorder::~TraceRecorder()
{
    
    JS_ASSERT(traceMonitor->recorder != this);

    if (trashSelf)
        TrashTree(cx, fragment->root);

    for (unsigned int i = 0; i < whichTreesToTrash.length(); i++)
        TrashTree(cx, whichTreesToTrash[i]);

    
    tempAlloc().reset();

    forgetGuardedShapes();
}

inline bool
TraceMonitor::outOfMemory() const
{
    return dataAlloc->outOfMemory() ||
           tempAlloc->outOfMemory() ||
           traceAlloc->outOfMemory();
}





AbortableRecordingStatus
TraceRecorder::finishSuccessfully()
{
    JS_ASSERT(traceMonitor->recorder == this);
    JS_ASSERT(fragment->lastIns && fragment->code());

    AUDIT(traceCompleted);
    mark.commit();

    
    JSContext* localcx = cx;
    TraceMonitor* localtm = traceMonitor;

    localtm->recorder = NULL;
    delete this;

    
    if (localtm->outOfMemory() || OverfullJITCache(localtm)) {
        ResetJIT(localcx, FR_OOM);
        return ARECORD_ABORTED;
    }
    return ARECORD_COMPLETED;
}


JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::finishAbort(const char* reason)
{
    JS_ASSERT(traceMonitor->recorder == this);
    JS_ASSERT(!fragment->code());

    AUDIT(recorderAborted);
#ifdef DEBUG
    debug_only_printf(LC_TMAbort,
                      "Abort recording of tree %s:%d@%d at %s:%d@%d: %s.\n",
                      tree->treeFileName,
                      tree->treeLineNumber,
                      tree->treePCOffset,
                      cx->fp()->getScript()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()),
                      reason);
#endif
    Backoff(cx, (jsbytecode*) fragment->root->ip, fragment->root);

    








    if (fragment->root == fragment) {
        TrashTree(cx, fragment->toTreeFragment());
    } else {
        JS_ASSERT(numSideExitsBefore <= fragment->root->sideExits.length());
        fragment->root->sideExits.setLength(numSideExitsBefore);
    }

    
    JSContext* localcx = cx;
    TraceMonitor* localtm = traceMonitor;

    localtm->recorder = NULL;
    delete this;
    if (localtm->outOfMemory() || OverfullJITCache(localtm))
        ResetJIT(localcx, FR_OOM);
    return ARECORD_ABORTED;
}


inline LIns*
TraceRecorder::addName(LIns* ins, const char* name)
{
#ifdef JS_JIT_SPEW
    



    if (LogController.lcbits > 0)
        lirbuf->printer->lirNameMap->addName(ins, name);
#endif
    return ins;
}

inline LIns*
TraceRecorder::insImmObj(JSObject* obj)
{
    tree->gcthings.addUnique(ObjectValue(*obj));
    return lir->insImmP((void*)obj);
}

inline LIns*
TraceRecorder::insImmFun(JSFunction* fun)
{
    tree->gcthings.addUnique(ObjectValue(*fun));
    return lir->insImmP((void*)fun);
}

inline LIns*
TraceRecorder::insImmStr(JSString* str)
{
    tree->gcthings.addUnique(StringValue(str));
    return lir->insImmP((void*)str);
}

inline LIns*
TraceRecorder::insImmShape(const Shape* shape)
{
    tree->shapes.addUnique(shape);
    return lir->insImmP((void*)shape);
}

inline LIns*
TraceRecorder::insImmId(jsid id)
{
    if (JSID_IS_GCTHING(id))
        tree->gcthings.addUnique(IdToValue(id));
    return lir->insImmP((void*)JSID_BITS(id));
}

inline LIns*
TraceRecorder::p2i(nanojit::LIns* ins)
{
#ifdef NANOJIT_64BIT
    return lir->ins1(LIR_q2i, ins);
#else
    return ins;
#endif
}

ptrdiff_t
TraceRecorder::nativeGlobalSlot(const Value* p) const
{
    JS_ASSERT(isGlobal(p));
    if (size_t(p - globalObj->fslots) < JS_INITIAL_NSLOTS)
        return ptrdiff_t(p - globalObj->fslots);
    return ptrdiff_t((p - globalObj->dslots) + JS_INITIAL_NSLOTS);
}


ptrdiff_t
TraceRecorder::nativeGlobalOffset(const Value* p) const
{
    return nativeGlobalSlot(p) * sizeof(double);
}


bool
TraceRecorder::isGlobal(const Value* p) const
{
    return ((size_t(p - globalObj->fslots) < JS_INITIAL_NSLOTS) ||
            (size_t(p - globalObj->dslots) < (globalObj->numSlots() - JS_INITIAL_NSLOTS)));
}

bool
TraceRecorder::isVoidPtrGlobal(const void* p) const
{
    return isGlobal((const Value *)p);
}








JS_REQUIRES_STACK ptrdiff_t
TraceRecorder::nativeStackOffsetImpl(const void* p) const
{
    CountSlotsVisitor visitor(p);
    VisitStackSlots(visitor, cx, callDepth);
    size_t offset = visitor.count() * sizeof(double);

    



    if (!visitor.stopped()) {
        const Value *vp = (const Value *)p;
        JS_ASSERT(size_t(vp - cx->fp()->slots()) < cx->fp()->getSlotCount());
        offset += size_t(vp - cx->regs->sp) * sizeof(double);
    }
    return offset;
}

JS_REQUIRES_STACK inline ptrdiff_t
TraceRecorder::nativeStackOffset(const Value* p) const
{
    return nativeStackOffsetImpl(p);
}

JS_REQUIRES_STACK inline ptrdiff_t
TraceRecorder::nativeStackSlotImpl(const void* p) const
{
    return nativeStackOffsetImpl(p) / sizeof(double);
}

JS_REQUIRES_STACK inline ptrdiff_t
TraceRecorder::nativeStackSlot(const Value* p) const
{
    return nativeStackSlotImpl(p);
}





inline JS_REQUIRES_STACK ptrdiff_t
TraceRecorder::nativespOffsetImpl(const void* p) const
{
    return -tree->nativeStackBase + nativeStackOffsetImpl(p);
}

inline JS_REQUIRES_STACK ptrdiff_t
TraceRecorder::nativespOffset(const Value* p) const
{
    return nativespOffsetImpl(p);
}


inline void
TraceRecorder::trackNativeStackUse(unsigned slots)
{
    if (slots > tree->maxNativeStackSlots)
        tree->maxNativeStackSlots = slots;
}






static inline void
ValueToNative(const Value &v, JSValueType type, double* slot)
{
    JS_ASSERT(type <= JSVAL_UPPER_INCL_TYPE_OF_BOXABLE_SET);
    if (type > JSVAL_UPPER_INCL_TYPE_OF_NUMBER_SET)
        v.unboxNonDoubleTo((uint64 *)slot);
    else if (type == JSVAL_TYPE_INT32)
        *(int32_t *)slot = v.isInt32() ? v.toInt32() : (int32_t)v.toDouble();
    else
        *(double *)slot = v.toNumber();

#ifdef DEBUG
    int32_t _;
    switch (type) {
      case JSVAL_TYPE_NONFUNOBJ: {
        JS_ASSERT(!IsFunctionObject(v));
        debug_only_printf(LC_TMTracer,
                          "object<%p:%s> ", (void*)*(JSObject **)slot,
                          v.toObject().getClass()->name);
        return;
      }

      case JSVAL_TYPE_INT32:
        JS_ASSERT(v.isInt32() || (v.isDouble() && JSDOUBLE_IS_INT32(v.toDouble(), &_)));
        debug_only_printf(LC_TMTracer, "int<%d> ", *(jsint *)slot);
        return;

      case JSVAL_TYPE_DOUBLE:
        JS_ASSERT(v.isNumber());
        debug_only_printf(LC_TMTracer, "double<%g> ", *(jsdouble *)slot);
        return;

      case JSVAL_TYPE_BOXED:
        JS_NOT_REACHED("found jsval type in an entry type map");
        return;

      case JSVAL_TYPE_STRING:
        JS_ASSERT(v.isString());
        debug_only_printf(LC_TMTracer, "string<%p> ", (void*)*(JSString**)slot);
        return;

      case JSVAL_TYPE_NULL:
        JS_ASSERT(v.isNull());
        debug_only_print0(LC_TMTracer, "null ");
        return;

      case JSVAL_TYPE_BOOLEAN:
        JS_ASSERT(v.isBoolean());
        debug_only_printf(LC_TMTracer, "special<%d> ", *(JSBool*)slot);
        return;

      case JSVAL_TYPE_UNDEFINED:
        JS_ASSERT(v.isUndefined());
        debug_only_print0(LC_TMTracer, "undefined ");
        return;

      case JSVAL_TYPE_MAGIC:
        JS_ASSERT(v.isMagic());
        debug_only_print0(LC_TMTracer, "hole ");
        return;

      case JSVAL_TYPE_FUNOBJ: {
        JS_ASSERT(IsFunctionObject(v));
        JSFunction* fun = GET_FUNCTION_PRIVATE(cx, &v.toObject());
        debug_only_printf(LC_TMTracer,
                          "function<%p:%s> ", (void*)*(JSObject **)slot,
                          fun->atom
                          ? JS_GetStringBytes(ATOM_TO_STRING(fun->atom))
                          : "unnamed");
        return;
      }
      default:
        JS_NOT_REACHED("unexpected type");
        break;
    }
#endif
}

void
TraceMonitor::flush()
{
    
    JS_ASSERT(!recorder);
    AUDIT(cacheFlushed);

    
    verbose_only(
        for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
            for (TreeFragment *f = vmfragments[i]; f; f = f->next) {
                JS_ASSERT(f->root == f);
                for (TreeFragment *p = f; p; p = p->peer)
                    FragProfiling_FragFinalizer(p, this);
            }
        }
    )

    verbose_only(
        for (Seq<Fragment*>* f = branches; f; f = f->tail)
            FragProfiling_FragFinalizer(f->head, this);
    )

    frameCache->reset();
    dataAlloc->reset();
    traceAlloc->reset();
    codeAlloc->reset();
    tempAlloc->reset();
    reTempAlloc->reset();
    oracle->clear();

    Allocator& alloc = *dataAlloc;

    for (size_t i = 0; i < MONITOR_N_GLOBAL_STATES; ++i) {
        globalStates[i].globalShape = -1;
        globalStates[i].globalSlots = new (alloc) SlotList(&alloc);
    }

    assembler = new (alloc) Assembler(*codeAlloc, alloc, alloc, core, &LogController, avmplus::AvmCore::config);
    verbose_only( branches = NULL; )

    PodArrayZero(vmfragments);
    reFragments = new (alloc) REHashMap(alloc);

    needFlush = JS_FALSE;
}

static inline void
MarkTree(JSTracer* trc, TreeFragment *f)
{
    Value* vp = f->gcthings.data();
    unsigned len = f->gcthings.length();
    while (len--) {
        Value &v = *vp++;
        JS_SET_TRACING_NAME(trc, "jitgcthing");
        JS_ASSERT(v.isMarkable());
        Mark(trc, v.asGCThing(), v.gcKind());
    }
    const Shape** shapep = f->shapes.data();
    len = f->shapes.length();
    while (len--) {
        const Shape* shape = *shapep++;
        shape->trace(trc);
    }
}

void
TraceMonitor::mark(JSTracer* trc)
{
    if (!trc->context->runtime->gcFlushCodeCaches) {
        for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
            TreeFragment* f = vmfragments[i];
            while (f) {
                if (f->code())
                    MarkTree(trc, f);
                TreeFragment* peer = f->peer;
                while (peer) {
                    if (peer->code())
                        MarkTree(trc, peer);
                    peer = peer->peer;
                }
                f = f->next;
            }
        }
        if (recorder)
            MarkTree(trc, recorder->getTree());
    }
}




static inline void
NativeToValue(JSContext* cx, Value& v, JSValueType type, double* slot)
{
    if (type == JSVAL_TYPE_DOUBLE) {
        v.setNumber(*slot);
    } else if (JS_LIKELY(type <= JSVAL_UPPER_INCL_TYPE_OF_BOXABLE_SET)) {
        v.boxNonDoubleFrom(type, (uint64 *)slot);
    } else if (type == JSVAL_TYPE_STRORNULL) {
        JSString *str = *(JSString **)slot;
        v = str ? StringValue(str) : NullValue();
    } else if (type == JSVAL_TYPE_OBJORNULL) {
        JSObject *obj = *(JSObject **)slot;
        v = obj ? ObjectValue(*obj) : NullValue();
    } else {
        JS_ASSERT(type == JSVAL_TYPE_BOXED);
        JS_STATIC_ASSERT(sizeof(Value) == sizeof(double));
        v = *(Value *)slot;
    }

#ifdef DEBUG
    switch (type) {
      case JSVAL_TYPE_NONFUNOBJ:
        JS_ASSERT(!IsFunctionObject(v));
        debug_only_printf(LC_TMTracer,
                          "object<%p:%s> ",
                          (void*) &v.toObject(),
                          v.toObject().getClass()->name);
        break;
      case JSVAL_TYPE_INT32:
        debug_only_printf(LC_TMTracer, "int<%d> ", v.toInt32());
        break;
      case JSVAL_TYPE_DOUBLE:
        debug_only_printf(LC_TMTracer, "double<%g> ", v.toNumber());
        break;
      case JSVAL_TYPE_STRING:
        debug_only_printf(LC_TMTracer, "string<%p> ", (void*)v.toString());
        break;
      case JSVAL_TYPE_NULL:
        JS_ASSERT(v.isNull());
        debug_only_print0(LC_TMTracer, "null ");
        break;
      case JSVAL_TYPE_BOOLEAN:
        debug_only_printf(LC_TMTracer, "bool<%d> ", v.toBoolean());
        break;
      case JSVAL_TYPE_UNDEFINED:
        JS_ASSERT(v.isUndefined());
        debug_only_print0(LC_TMTracer, "undefined ");
        break;
      case JSVAL_TYPE_MAGIC:
        debug_only_printf(LC_TMTracer, "magic<%d> ", v.whyMagic());
        break;
      case JSVAL_TYPE_FUNOBJ: {
        JS_ASSERT(IsFunctionObject(v));
        JSFunction* fun = GET_FUNCTION_PRIVATE(cx, &v.toObject());
        debug_only_printf(LC_TMTracer,
                          "function<%p:%s> ", (void*) &v.toObject(),
                          fun->atom
                          ? JS_GetStringBytes(ATOM_TO_STRING(fun->atom))
                          : "unnamed");
        break;
      }
      case JSVAL_TYPE_STRORNULL:
        debug_only_printf(LC_TMTracer, "nullablestr<%p> ", v.isNull() ? NULL : (void *)v.toString());
        break;
      case JSVAL_TYPE_OBJORNULL:
        debug_only_printf(LC_TMTracer, "nullablestr<%p> ", v.isNull() ? NULL : (void *)&v.toObject());
        break;
      case JSVAL_TYPE_BOXED:
        debug_only_printf(LC_TMTracer, "box<%llx> ", (long long unsigned int)v.asRawBits());
        break;
      default:
        JS_NOT_REACHED("unexpected type");
        break;
    }
#endif
}

void
ExternNativeToValue(JSContext* cx, Value& v, JSValueType type, double* slot)
{
    return NativeToValue(cx, v, type, slot);
}

class BuildNativeFrameVisitor : public SlotVisitorBase
{
    JSContext *mCx;
    JSValueType *mTypeMap;
    double *mGlobal;
    double *mStack;
public:
    BuildNativeFrameVisitor(JSContext *cx,
                            JSValueType *typemap,
                            double *global,
                            double *stack) :
        mCx(cx),
        mTypeMap(typemap),
        mGlobal(global),
        mStack(stack)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot) {
        debug_only_printf(LC_TMTracer, "global%d: ", n);
        ValueToNative(*vp, *mTypeMap++, &mGlobal[slot]);
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, int count, JSStackFrame* fp) {
        for (int i = 0; i < count; ++i) {
            debug_only_printf(LC_TMTracer, "%s%d: ", stackSlotKind(), i);
            ValueToNative(*vp++, *mTypeMap++, mStack++);
        }
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame* fp) {
        debug_only_printf(LC_TMTracer, "%s%d: ", stackSlotKind(), 0);
        *(JSObject **)mStack = *p;
#ifdef DEBUG
        if (*mTypeMap == JSVAL_TYPE_NULL) {
            JS_ASSERT(*p == NULL);
            debug_only_print0(LC_TMTracer, "null ");
        } else {
            JS_ASSERT(*mTypeMap == JSVAL_TYPE_NONFUNOBJ);
            JS_ASSERT(!(*p)->isFunction());
            debug_only_printf(LC_TMTracer,
                              "object<%p:%s> ", (void*)*p,
                              (*p)->getClass()->name);
        }
#endif
        mTypeMap++;
        mStack++;
        return true;
    }
};

static JS_REQUIRES_STACK void
BuildNativeFrame(JSContext *cx, JSObject *globalObj, unsigned callDepth,
                 unsigned ngslots, uint16 *gslots,
                 JSValueType *typeMap, double *global, double *stack)
{
    BuildNativeFrameVisitor visitor(cx, typeMap, global, stack);
    VisitSlots(visitor, cx, globalObj, callDepth, ngslots, gslots);
    debug_only_print0(LC_TMTracer, "\n");
}

class FlushNativeGlobalFrameVisitor : public SlotVisitorBase
{
    JSContext *mCx;
    JSValueType *mTypeMap;
    double *mGlobal;
public:
    FlushNativeGlobalFrameVisitor(JSContext *cx,
                                  JSValueType *typeMap,
                                  double *global) :
        mCx(cx),
        mTypeMap(typeMap),
        mGlobal(global)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot) {
        debug_only_printf(LC_TMTracer, "global%d=", n);
        JS_ASSERT(JS_THREAD_DATA(mCx)->waiveGCQuota);
        NativeToValue(mCx, *vp, *mTypeMap++, &mGlobal[slot]);
    }
};

class FlushNativeStackFrameVisitor : public SlotVisitorBase
{
    JSContext *mCx;
    const JSValueType *mInitTypeMap;
    const JSValueType *mTypeMap;
    double *mStack;
    Value *mStop;
public:
    FlushNativeStackFrameVisitor(JSContext *cx,
                                 const JSValueType *typeMap,
                                 double *stack,
                                 Value *stop) :
        mCx(cx),
        mInitTypeMap(typeMap),
        mTypeMap(typeMap),
        mStack(stack),
        mStop(stop)
    {}

    const JSValueType* getTypeMap()
    {
        return mTypeMap;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, JSStackFrame* fp) {
        JS_ASSERT(JS_THREAD_DATA(mCx)->waiveGCQuota);
        for (size_t i = 0; i < count; ++i) {
            if (vp == mStop)
                return false;
            debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), unsigned(i));
            NativeToValue(mCx, *vp, *mTypeMap, mStack);
            vp++;
            mTypeMap++;
            mStack++;
        }
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame* fp) {
        JS_ASSERT(JS_THREAD_DATA(mCx)->waiveGCQuota);
        if ((Value *)p == mStop)
            return false;
        debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), 0);
        *p = *(JSObject **)mStack;
#ifdef DEBUG
        JSValueType type = *mTypeMap;
        if (type == JSVAL_TYPE_NULL) {
            debug_only_print0(LC_TMTracer, "null ");
        } else {
            JS_ASSERT(type == JSVAL_TYPE_NONFUNOBJ);
            JS_ASSERT(!(*p)->isFunction());
            debug_only_printf(LC_TMTracer,
                              "object<%p:%s> ",
                              (void*) *p,
                              (*p)->getClass()->name);
        }
#endif
        mTypeMap++;
        mStack++;
        return true;
    }
};


static JS_REQUIRES_STACK void
FlushNativeGlobalFrame(JSContext *cx, JSObject *globalObj, double *global, unsigned ngslots,
                       uint16 *gslots, JSValueType *typemap)
{
    FlushNativeGlobalFrameVisitor visitor(cx, typemap, global);
    VisitGlobalSlots(visitor, cx, globalObj, ngslots, gslots);
    debug_only_print0(LC_TMTracer, "\n");
}






static int32
StackDepthFromCallStack(TracerState* state, uint32 callDepth)
{
    int32 nativeStackFramePos = 0;

    
    for (FrameInfo** fip = state->callstackBase; fip < state->rp + callDepth; fip++)
        nativeStackFramePos += (*fip)->callerHeight;
    return nativeStackFramePos;
}












template<typename T>
inline JSValueType
GetUpvarOnTrace(JSContext* cx, uint32 upvarLevel, int32 slot, uint32 callDepth, double* result)
{
    TracerState* state = cx->tracerState;
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
        uint32 argc = state->outermostTree->argc;
        uint32 native_slot = T::native_slot(argc, slot);
        *result = state->stackBase[native_slot];
        return state->callstackBase[0]->get_typemap()[native_slot];
    }

    



    JS_ASSERT(upvarLevel < UpvarCookie::UPVAR_LEVEL_LIMIT);
    JSStackFrame* fp = cx->findFrameAtLevel(upvarLevel);
    Value v = T::interp_get(fp, slot);
    JSValueType type = getCoercedType(v);
    ValueToNative(v, type, result);
    return type;
}


struct UpvarArgTraits {
    static Value interp_get(JSStackFrame* fp, int32 slot) {
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
    static Value interp_get(JSStackFrame* fp, int32 slot) {
        return fp->slots()[slot];
    }

    static uint32 native_slot(uint32 argc, int32 slot) {
        return 4  + argc + slot;
    }
};

uint32 JS_FASTCALL
GetUpvarVarOnTrace(JSContext* cx, uint32 upvarLevel, int32 slot, uint32 callDepth, double* result)
{
    return GetUpvarOnTrace<UpvarVarTraits>(cx, upvarLevel, slot, callDepth, result);
}






struct UpvarStackTraits {
    static Value interp_get(JSStackFrame* fp, int32 slot) {
        return fp->slots()[slot + fp->getFixedCount()];
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


struct ClosureVarInfo
{
    uint32   slot;
#ifdef DEBUG
    uint32   callDepth;
#endif
};





template<typename T>
inline uint32
GetFromClosure(JSContext* cx, JSObject* call, const ClosureVarInfo* cv, double* result)
{
    JS_ASSERT(call->isCall());

    TracerState* state = cx->tracerState;

#ifdef DEBUG
    FrameInfo** fip = state->rp + cv->callDepth;
    int32 stackOffset = StackDepthFromCallStack(state, cv->callDepth);
    while (--fip > state->callstackBase) {
        FrameInfo* fi = *fip;

        



        stackOffset -= fi->callerHeight;
        JSObject* callee = *(JSObject**)(&state->stackBase[stackOffset]);
        if (callee == call) {
            
            
            
            
            
            
            
            
            JS_NOT_REACHED("JSOP_NAME variable found in outer trace");
        }
    }
#endif

    


    uint32 slot = cv->slot;
    VOUCH_DOES_NOT_REQUIRE_STACK();
    if (cx->fp()->maybeCallObj() == call) {
        slot = T::adj_slot(cx->fp(), slot);
        *result = state->stackBase[slot];
        return state->callstackBase[0]->get_typemap()[slot];
    }

    JSStackFrame* fp = (JSStackFrame*) call->getPrivate();
    Value v;
    if (fp) {
        v = T::get_slot(fp, slot);
    } else {
        






        JS_ASSERT(slot < T::slot_count(call));
        v = T::get_slot(call, slot);
    }
    JSValueType type = getCoercedType(v);
    ValueToNative(v, type, result);
    return type;
}

struct ArgClosureTraits
{
    
    
    static inline uint32 adj_slot(JSStackFrame* fp, uint32 slot) { return 2 + slot; }

    
    static inline LIns* adj_slot_lir(LirWriter* lir, LIns* fp_ins, unsigned slot) {
        return lir->insImmI(2 + slot);
    }

    
    
    static inline Value get_slot(JSStackFrame* fp, unsigned slot) {
        JS_ASSERT(slot < fp->numFormalArgs());
        return fp->argv[slot];
    }

    
    static inline Value get_slot(JSObject* obj, unsigned slot) {
        return obj->getSlot(slot_offset(obj) + slot);
    }

    
    static inline uint32 slot_offset(JSObject* obj) {
        return JSSLOT_START(&js_CallClass) + CALL_CLASS_RESERVED_SLOTS;
    }

    
    static inline uint16 slot_count(JSObject* obj) {
        return js_GetCallObjectFunction(obj)->nargs;
    }

private:
    ArgClosureTraits();
};

uint32 JS_FASTCALL
GetClosureArg(JSContext* cx, JSObject* callee, const ClosureVarInfo* cv, double* result)
{
    return GetFromClosure<ArgClosureTraits>(cx, callee, cv, result);
}

struct VarClosureTraits
{
    
    
    
    static inline uint32 adj_slot(JSStackFrame* fp, uint32 slot) {
        return 4 + fp->numActualArgs() + slot;
    }

    static inline LIns* adj_slot_lir(LirWriter* lir, LIns* fp_ins, unsigned slot) {
        LIns *argc_ins = lir->insLoad(LIR_ldi, fp_ins, JSStackFrame::offsetNumActualArgs(), ACCSET_OTHER);
        return lir->ins2(LIR_addi, lir->insImmI(4 + slot), argc_ins);
    }

    
    static inline Value get_slot(JSStackFrame* fp, unsigned slot) {
        JS_ASSERT(slot < fp->getFunction()->u.i.nvars);
        return fp->slots()[slot];
    }

    static inline Value get_slot(JSObject* obj, unsigned slot) {
        return obj->getSlot(slot_offset(obj) + slot);
    }

    static inline uint32 slot_offset(JSObject* obj) {
        return JSSLOT_START(&js_CallClass) + CALL_CLASS_RESERVED_SLOTS +
               js_GetCallObjectFunction(obj)->nargs;
    }

    static inline uint16 slot_count(JSObject* obj) {
        return js_GetCallObjectFunction(obj)->u.i.nvars;
    }

private:
    VarClosureTraits();
};

uint32 JS_FASTCALL
GetClosureVar(JSContext* cx, JSObject* callee, const ClosureVarInfo* cv, double* result)
{
    return GetFromClosure<VarClosureTraits>(cx, callee, cv, result);
}
















static JS_REQUIRES_STACK int
FlushNativeStackFrame(JSContext* cx, unsigned callDepth, const JSValueType* mp, double* np,
                      JSStackFrame* stopFrame)
{
    Value* stopAt = stopFrame ? &stopFrame->argv[-2] : NULL;

    
    FlushNativeStackFrameVisitor visitor(cx, mp, np, stopAt);
    VisitStackSlots(visitor, cx, callDepth);

    
    
    
    {
        unsigned n = callDepth+1; 
        JSStackFrame* fp = cx->fp();
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
                if (fp->hasArgsObj() &&
                    fp->getArgsObj()->getPrivate() == JS_ARGUMENTS_OBJECT_ON_TRACE)
                {
                    JS_ASSERT(fp->getArgsObj()->isNormalArguments());
                    fp->getArgsObj()->setPrivate(fp);
                }

                JS_ASSERT(fp->argv[-1].isObjectOrNull());
                JS_ASSERT(fp->callee()->isFunction());
                JS_ASSERT(GET_FUNCTION_PRIVATE(cx, fp->callee()) == fp->getFunction());

                if (FUN_INTERPRETED(fp->getFunction()) &&
                    (fp->getFunction()->flags & JSFUN_HEAVYWEIGHT)) {
                    
                    
                    if (!fp->hasCallObj())
                        fp->setCallObj(fp->getScopeChain());

                    
                    
                    
                    
                    
                    if (!fp->getScopeChain()->getPrivate())
                        fp->getScopeChain()->setPrivate(fp);
                }
                fp->setThisValue(fp->argv[-1]);
            }
        }
    }
    debug_only_print0(LC_TMTracer, "\n");
    return visitor.getTypeMap() - mp;
}


JS_REQUIRES_STACK void
TraceRecorder::importImpl(LIns* base, ptrdiff_t offset, const void* p, JSValueType t,
                          const char *prefix, uintN index, JSStackFrame *fp)
{
    LIns* ins;
    AccSet accSet = base == lirbuf->sp ? ACCSET_STACK : ACCSET_OTHER;
    if (t == JSVAL_TYPE_INT32) { 
        JS_ASSERT(hasInt32Repr(*(const Value *)p));

        





        ins = lir->insLoad(LIR_ldi, base, offset, accSet);
        ins = lir->ins1(LIR_i2d, ins);
    } else {
        JS_ASSERT_IF(t != JSVAL_TYPE_BOXED && !IsFrameObjPtrTraceType(t),
                     ((const Value *)p)->isNumber() == (t == JSVAL_TYPE_DOUBLE));
        if (t == JSVAL_TYPE_DOUBLE) {
            ins = lir->insLoad(LIR_ldd, base, offset, accSet);
        } else if (t == JSVAL_TYPE_BOOLEAN) {
            ins = lir->insLoad(LIR_ldi, base, offset, accSet);
        } else if (t == JSVAL_TYPE_UNDEFINED) {
            ins = INS_UNDEFINED();
        } else if (t == JSVAL_TYPE_MAGIC) {
            ins = lir->insLoad(LIR_ldi, base, offset, accSet);
        } else {
            ins = lir->insLoad(LIR_ldp, base, offset, accSet);
        }
    }
    checkForGlobalObjectReallocation();
    tracker.set(p, ins);

#ifdef DEBUG
    char name[64];
    JS_ASSERT(strlen(prefix) < 11);
    void* mark = NULL;
    jsuword* localNames = NULL;
    const char* funName = NULL;
    if (*prefix == 'a' || *prefix == 'v') {
        mark = JS_ARENA_MARK(&cx->tempPool);
        if (fp->getFunction()->hasLocalNames())
            localNames = fp->getFunction()->getLocalNameArray(cx, &cx->tempPool);
        funName = fp->getFunction()->atom
                ? js_AtomToPrintableString(cx, fp->getFunction()->atom)
                : "<anonymous>";
    }
    if (!strcmp(prefix, "argv")) {
        if (index < fp->numFormalArgs()) {
            JSAtom *atom = JS_LOCAL_NAME_TO_ATOM(localNames[index]);
            JS_snprintf(name, sizeof name, "$%s.%s", funName, js_AtomToPrintableString(cx, atom));
        } else {
            JS_snprintf(name, sizeof name, "$%s.<arg%d>", funName, index);
        }
    } else if (!strcmp(prefix, "vars")) {
        JSAtom *atom = JS_LOCAL_NAME_TO_ATOM(localNames[fp->numFormalArgs() + index]);
        JS_snprintf(name, sizeof name, "$%s.%s", funName, js_AtomToPrintableString(cx, atom));
    } else {
        JS_snprintf(name, sizeof name, "$%s%d", prefix, index);
    }

    if (mark)
        JS_ARENA_RELEASE(&cx->tempPool, mark);
    addName(ins, name);

    debug_only_printf(LC_TMTracer, "import vp=%p name=%s type=%c\n",
                      p, name, TypeToChar(t));
#endif
}

JS_REQUIRES_STACK void
TraceRecorder::import(LIns* base, ptrdiff_t offset, const Value* p, JSValueType t,
                          const char *prefix, uintN index, JSStackFrame *fp)
{
    return importImpl(base, offset, p, t, prefix, index, fp);
}

class ImportBoxedStackSlotVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    LIns *mBase;
    ptrdiff_t mStackOffset;
    JSValueType *mTypemap;
    JSStackFrame *mFp;
public:
    ImportBoxedStackSlotVisitor(TraceRecorder &recorder,
                                LIns *base,
                                ptrdiff_t stackOffset,
                                JSValueType *typemap) :
        mRecorder(recorder),
        mBase(base),
        mStackOffset(stackOffset),
        mTypemap(typemap)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            if (*mTypemap == JSVAL_TYPE_BOXED) {
                mRecorder.import(mBase, mStackOffset, vp, JSVAL_TYPE_BOXED,
                                 "jsval", i, fp);
                LIns *vp_ins = mRecorder.unbox_value(*vp, mBase, mStackOffset,
                                                     mRecorder.copy(mRecorder.anchor));
                mRecorder.set(vp, vp_ins);
            }
            vp++;
            mTypemap++;
            mStackOffset += sizeof(double);
        }
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame *fp) {
        JS_ASSERT(*mTypemap != JSVAL_TYPE_BOXED);
        mTypemap++;
        mStackOffset += sizeof(double);
        return true;
    }
};

JS_REQUIRES_STACK void
TraceRecorder::import(TreeFragment* tree, LIns* sp, unsigned stackSlots, unsigned ngslots,
                      unsigned callDepth, JSValueType* typeMap)
{
    














    JSValueType* globalTypeMap = typeMap + stackSlots;
    unsigned length = tree->nGlobalTypes();

    



    if (ngslots < length) {
        MergeTypeMaps(&globalTypeMap , &ngslots ,
                      tree->globalTypeMap(), length,
                      (JSValueType*)alloca(sizeof(JSValueType) * length));
    }
    JS_ASSERT(ngslots == tree->nGlobalTypes());

    



    ImportBoxedStackSlotVisitor boxedStackVisitor(*this, sp, -tree->nativeStackBase, typeMap);
    VisitStackSlots(boxedStackVisitor, cx, callDepth);

    



    importTypeMap.set(importStackSlots = stackSlots,
                      importGlobalSlots = ngslots,
                      typeMap, globalTypeMap);
}

JS_REQUIRES_STACK bool
TraceRecorder::isValidSlot(JSObject *obj, const Shape* shape)
{
    uint32 setflags = (js_CodeSpec[*cx->regs->pc].format & (JOF_SET | JOF_INCDEC | JOF_FOR));

    if (setflags) {
        if (!shape->hasDefaultSetter())
            RETURN_VALUE("non-stub setter", false);
        if (!shape->writable())
            RETURN_VALUE("writing to a read-only property", false);
    }

    
    if (setflags != JOF_SET && !shape->hasDefaultGetter()) {
        JS_ASSERT(!shape->isMethod());
        RETURN_VALUE("non-stub getter", false);
    }

    if (!obj->containsSlot(shape->slot))
        RETURN_VALUE("invalid-slot obj property", false);

    return true;
}


JS_REQUIRES_STACK void
TraceRecorder::importGlobalSlot(unsigned slot)
{
    JS_ASSERT(slot == uint16(slot));
    JS_ASSERT(globalObj->numSlots() <= MAX_GLOBAL_SLOTS);

    Value* vp = &globalObj->getSlotRef(slot);
    JS_ASSERT(!known(vp));

    
    JSValueType type;
    int index = tree->globalSlots->offsetOf(uint16(slot));
    if (index == -1) {
        type = getCoercedType(*vp);
        if (type == JSVAL_TYPE_INT32 && (!oracle || oracle->isGlobalSlotUndemotable(cx, slot)))
            type = JSVAL_TYPE_DOUBLE;
        index = (int)tree->globalSlots->length();
        tree->globalSlots->add(uint16(slot));
        tree->typeMap.add(type);
        SpecializeTreesToMissingGlobals(cx, globalObj, tree);
        JS_ASSERT(tree->nGlobalTypes() == tree->globalSlots->length());
    } else {
        type = importTypeMap[importStackSlots + index];
        JS_ASSERT(type != JSVAL_TYPE_UNINITIALIZED);
    }
    import(eos_ins, slot * sizeof(double), vp, type, "global", index, NULL);
}


JS_REQUIRES_STACK bool
TraceRecorder::lazilyImportGlobalSlot(unsigned slot)
{
    if (slot != uint16(slot)) 
        return false;
    



    if (globalObj->numSlots() > MAX_GLOBAL_SLOTS)
        return false;
    Value* vp = &globalObj->getSlotRef(slot);
    if (known(vp))
        return true; 
    importGlobalSlot(slot);
    return true;
}


LIns*
TraceRecorder::writeBack(LIns* i, LIns* base, ptrdiff_t offset, bool shouldDemote)
{
    




    if (shouldDemote && isPromoteInt(i))
        i = demote(lir, i);
    return lir->insStore(i, base, offset, (base == lirbuf->sp) ? ACCSET_STACK : ACCSET_OTHER);
}


JS_REQUIRES_STACK void
TraceRecorder::setImpl(void* p, LIns* i, bool demote)
{
    JS_ASSERT(i != NULL);
    checkForGlobalObjectReallocation();
    tracker.set(p, i);

    





    LIns* x = nativeFrameTracker.get(p);
    if (!x) {
        if (isVoidPtrGlobal(p))
            x = writeBack(i, eos_ins, nativeGlobalOffset((Value *)p), demote);
        else
            x = writeBack(i, lirbuf->sp, nativespOffsetImpl(p), demote);
        nativeFrameTracker.set(p, x);
    } else {
#if defined NANOJIT_64BIT
        JS_ASSERT( x->isop(LIR_stq) || x->isop(LIR_sti) || x->isop(LIR_std));
#else
        JS_ASSERT( x->isop(LIR_sti) || x->isop(LIR_std));
#endif

        int disp;
        LIns *base = x->oprnd2();
#ifdef NANOJIT_ARM
        if (base->isop(LIR_addp)) {
            disp = base->oprnd2()->immI();
            base = base->oprnd1();
        } else
#endif
        disp = x->disp();

        JS_ASSERT(base == lirbuf->sp || base == eos_ins);
        JS_ASSERT(disp == ((base == lirbuf->sp)
                            ? nativespOffsetImpl(p)
                            : nativeGlobalOffset((Value *)p)));

        writeBack(i, base, disp, demote);
    }
}

JS_REQUIRES_STACK inline void
TraceRecorder::set(Value* p, LIns* i, bool demote)
{
    return setImpl(p, i, demote);
}

JS_REQUIRES_STACK void
TraceRecorder::setFrameObjPtr(JSObject** p, LIns* i, bool demote)
{
    JS_ASSERT(isValidFrameObjPtr(p));
    return setImpl(p, i, demote);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::attemptImport(const Value* p)
{
    if (LIns* i = getFromTracker(p))
        return i;

    
    CountSlotsVisitor countVisitor(p);
    VisitStackSlots(countVisitor, cx, callDepth);

    if (countVisitor.stopped() || size_t(p - cx->fp()->slots()) < cx->fp()->getSlotCount())
        return get(p);

    return NULL;
}

inline nanojit::LIns*
TraceRecorder::getFromTrackerImpl(const void* p)
{
    checkForGlobalObjectReallocation();
    return tracker.get(p);
}

inline nanojit::LIns*
TraceRecorder::getFromTracker(const Value* p)
{
    return getFromTrackerImpl(p);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::getImpl(const void *p)
{
    LIns* x = getFromTrackerImpl(p);
    if (x)
        return x;
    if (isVoidPtrGlobal(p)) {
        unsigned slot = nativeGlobalSlot((const Value *)p);
        JS_ASSERT(tree->globalSlots->offsetOf(uint16(slot)) != -1);
        importGlobalSlot(slot);
    } else {
        unsigned slot = nativeStackSlotImpl(p);
        JSValueType type = importTypeMap[slot];
        JS_ASSERT(type != JSVAL_TYPE_UNINITIALIZED);
        importImpl(lirbuf->sp, -tree->nativeStackBase + slot * sizeof(jsdouble),
                   p, type, "stack", slot, cx->fp());
    }
    JS_ASSERT(knownImpl(p));
    return tracker.get(p);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::get(const Value *p)
{
    return getImpl(p);
}

#ifdef DEBUG
bool
TraceRecorder::isValidFrameObjPtr(JSObject **p)
{
    JSStackFrame *fp = cx->fp();
    for (; fp; fp = fp->down) {
        if (fp->addressScopeChain() == p || fp->addressArgsObj() == p)
            return true;
    }
    return false;
}
#endif

JS_REQUIRES_STACK LIns*
TraceRecorder::getFrameObjPtr(JSObject **p)
{
    JS_ASSERT(isValidFrameObjPtr(p));
    return getImpl(p);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::addr(Value* p)
{
    return isGlobal(p)
           ? lir->ins2(LIR_addp, eos_ins, INS_CONSTWORD(nativeGlobalOffset(p)))
           : lir->ins2(LIR_addp, lirbuf->sp,
                       INS_CONSTWORD(nativespOffset(p)));
}

JS_REQUIRES_STACK inline bool
TraceRecorder::knownImpl(const void* p)
{
    checkForGlobalObjectReallocation();
    return tracker.has(p);
}

JS_REQUIRES_STACK inline bool
TraceRecorder::known(const Value* vp)
{
    return knownImpl(vp);
}

JS_REQUIRES_STACK inline bool
TraceRecorder::known(JSObject** p)
{
    return knownImpl(p);
}






JS_REQUIRES_STACK void
TraceRecorder::checkForGlobalObjectReallocation()
{
    if (global_dslots != globalObj->dslots) {
        debug_only_print0(LC_TMTracer,
                          "globalObj->dslots relocated, updating tracker\n");
        Value* src = global_dslots;
        Value* dst = globalObj->dslots;
        jsuint length = globalObj->dslots[-1].toPrivateUint32() - JS_INITIAL_NSLOTS;
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
    JSValueType *mTypeMap;
public:
    AdjustCallerGlobalTypesVisitor(TraceRecorder &recorder,
                                   JSValueType *typeMap) :
        mRecorder(recorder),
        mCx(mRecorder.cx),
        mLirbuf(mRecorder.lirbuf),
        mLir(mRecorder.lir),
        mTypeMap(typeMap)
    {}

    JSValueType* getTypeMap()
    {
        return mTypeMap;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot) {
        LIns *ins = mRecorder.get(vp);
        bool isPromote = isPromoteInt(ins);
        if (isPromote && *mTypeMap == JSVAL_TYPE_DOUBLE) {
            mLir->insStore(mRecorder.get(vp), mRecorder.eos_ins,
                            mRecorder.nativeGlobalOffset(vp), ACCSET_OTHER);

            



            JS_TRACE_MONITOR(mCx).oracle->markGlobalSlotUndemotable(mCx, slot);
        }
        JS_ASSERT(!(!isPromote && *mTypeMap == JSVAL_TYPE_INT32));
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
    JSValueType *mTypeMap;
public:
    AdjustCallerStackTypesVisitor(TraceRecorder &recorder,
                                  JSValueType *typeMap) :
        mRecorder(recorder),
        mCx(mRecorder.cx),
        mLirbuf(mRecorder.lirbuf),
        mLir(mRecorder.lir),
        mSlotnum(0),
        mTypeMap(typeMap)
    {}

    JSValueType* getTypeMap()
    {
        return mTypeMap;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, JSStackFrame* fp) {
        
        for (size_t i = 0; i < count; ++i) {
            LIns *ins = mRecorder.get(vp);
            bool isPromote = isPromoteInt(ins);
            if (isPromote && *mTypeMap == JSVAL_TYPE_DOUBLE) {
                mLir->insStore(ins, mLirbuf->sp,
                               mRecorder.nativespOffset(vp), ACCSET_STACK);

                



                JS_TRACE_MONITOR(mCx).oracle->markStackSlotUndemotable(mCx, mSlotnum);
            }
            JS_ASSERT(!(!isPromote && *mTypeMap == JSVAL_TYPE_INT32));
            ++vp;
            ++mTypeMap;
            ++mSlotnum;
        }
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame* fp) {
        JS_ASSERT(*mTypeMap != JSVAL_TYPE_BOXED);
        ++mTypeMap;
        ++mSlotnum;
        return true;
    }
};






JS_REQUIRES_STACK void
TraceRecorder::adjustCallerTypes(TreeFragment* f)
{
    AdjustCallerGlobalTypesVisitor globalVisitor(*this, f->globalTypeMap());
    VisitGlobalSlots(globalVisitor, cx, *tree->globalSlots);

    AdjustCallerStackTypesVisitor stackVisitor(*this, f->stackTypeMap());
    VisitStackSlots(stackVisitor, cx, 0);

    JS_ASSERT(f == f->root);
}

JS_REQUIRES_STACK inline JSValueType
TraceRecorder::determineSlotType(Value* vp)
{
    if (vp->isNumber()) {
        LIns *i = getFromTracker(vp);
        JSValueType t;
        if (i) {
            t = isPromoteInt(i) ? JSVAL_TYPE_INT32 : JSVAL_TYPE_DOUBLE;
        } else if (isGlobal(vp)) {
            int offset = tree->globalSlots->offsetOf(uint16(nativeGlobalSlot(vp)));
            JS_ASSERT(offset != -1);
            t = importTypeMap[importStackSlots + offset];
        } else {
            t = importTypeMap[nativeStackSlot(vp)];
        }
        JS_ASSERT(t != JSVAL_TYPE_UNINITIALIZED);
        JS_ASSERT_IF(t == JSVAL_TYPE_INT32, hasInt32Repr(*vp));
        return t;
    }

    if (vp->isObject())
        return vp->toObject().isFunction() ? JSVAL_TYPE_FUNOBJ : JSVAL_TYPE_NONFUNOBJ;
    return vp->extractNonDoubleObjectTraceType();
}

class DetermineTypesVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    JSValueType *mTypeMap;
public:
    DetermineTypesVisitor(TraceRecorder &recorder,
                          JSValueType *typeMap) :
        mRecorder(recorder),
        mTypeMap(typeMap)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot) {
        *mTypeMap++ = mRecorder.determineSlotType(vp);
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i)
            *mTypeMap++ = mRecorder.determineSlotType(vp++);
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame* fp) {
        *mTypeMap++ = getFrameObjPtrTraceType(*p);
        return true;
    }

    JSValueType* getTypeMap()
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
                      (void*)exit->from, (void*)cx->regs->pc, cx->fp()->getScript()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()), FramePCOffset(cx, cx->fp()));
    debug_only_print0(LC_TMTreeVis, " STACK=\"");
    for (unsigned i = 0; i < exit->numStackSlots; i++)
        debug_only_printf(LC_TMTreeVis, "%c", TypeToChar(exit->stackTypeMap()[i]));
    debug_only_print0(LC_TMTreeVis, "\" GLOBALS=\"");
    for (unsigned i = 0; i < exit->numGlobalSlots; i++)
        debug_only_printf(LC_TMTreeVis, "%c", TypeToChar(exit->globalTypeMap()[i]));
    debug_only_print0(LC_TMTreeVis, "\"\n");
}
#endif

JS_REQUIRES_STACK VMSideExit*
TraceRecorder::snapshot(ExitType exitType)
{
    JSStackFrame* const fp = cx->fp();
    JSFrameRegs* const regs = cx->regs;
    jsbytecode* pc = regs->pc;

    



    const JSCodeSpec& cs = js_CodeSpec[*pc];

    




    bool resumeAfter = (pendingSpecializedNative &&
                        JSTN_ERRTYPE(pendingSpecializedNative) == FAIL_STATUS);
    if (resumeAfter) {
        JS_ASSERT(*pc == JSOP_CALL || *pc == JSOP_APPLY || *pc == JSOP_NEW ||
                  *pc == JSOP_SETPROP || *pc == JSOP_SETNAME);
        pc += cs.length;
        regs->pc = pc;
        MUST_FLOW_THROUGH("restore_pc");
    }

    



    unsigned stackSlots = NativeStackSlots(cx, callDepth);

    



    trackNativeStackUse(stackSlots + 1);

    
    unsigned ngslots = tree->globalSlots->length();
    unsigned typemap_size = (stackSlots + ngslots) * sizeof(JSValueType);

    
    JSValueType* typemap = NULL;
    if (tempTypeMap.resize(typemap_size))
        typemap = tempTypeMap.begin(); 

    





    DetermineTypesVisitor detVisitor(*this, typemap);
    VisitSlots(detVisitor, cx, callDepth, ngslots,
               tree->globalSlots->data());
    JS_ASSERT(unsigned(detVisitor.getTypeMap() - typemap) ==
              ngslots + stackSlots);

    






    if (pendingUnboxSlot ||
        (pendingSpecializedNative && (pendingSpecializedNative->flags & JSTN_UNBOX_AFTER))) {
        unsigned pos = stackSlots - 1;
        if (pendingUnboxSlot == cx->regs->sp - 2)
            pos = stackSlots - 2;
        typemap[pos] = JSVAL_TYPE_BOXED;
    } else if (pendingSpecializedNative &&
               (pendingSpecializedNative->flags & JSTN_RETURN_NULLABLE_STR)) {
        typemap[stackSlots - 1] = JSVAL_TYPE_STRORNULL;
    } else if (pendingSpecializedNative &&
               (pendingSpecializedNative->flags & JSTN_RETURN_NULLABLE_OBJ)) {
        typemap[stackSlots - 1] = JSVAL_TYPE_OBJORNULL;
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

    



    VMSideExit** exits = tree->sideExits.data();
    unsigned nexits = tree->sideExits.length();
    if (exitType == LOOP_EXIT) {
        for (unsigned n = 0; n < nexits; ++n) {
            VMSideExit* e = exits[n];
            if (e->pc == pc && (e->imacpc == fp->maybeIMacroPC()) &&
                ngslots == e->numGlobalSlots &&
                !memcmp(exits[n]->fullTypeMap(), typemap, typemap_size)) {
                AUDIT(mergedLoopExits);
#if defined JS_JIT_SPEW
                TreevisLogExit(cx, e);
#endif
                return e;
            }
        }
    }

    
    VMSideExit* exit = (VMSideExit*)
        traceAlloc().alloc(sizeof(VMSideExit) + (stackSlots + ngslots) * sizeof(JSValueType));

    
    exit->from = fragment;
    exit->calldepth = callDepth;
    exit->numGlobalSlots = ngslots;
    exit->numStackSlots = stackSlots;
    exit->numStackSlotsBelowCurrentFrame = cx->fp()->argv ?
                                           nativeStackOffset(&cx->fp()->argv[-2]) / sizeof(double) :
                                           0;
    exit->exitType = exitType;
    exit->block = fp->maybeBlockChain();
    if (fp->hasBlockChain())
        tree->gcthings.addUnique(ObjectValue(*fp->getBlockChain()));
    exit->pc = pc;
    exit->imacpc = fp->maybeIMacroPC();
    exit->sp_adj = (stackSlots * sizeof(double)) - tree->nativeStackBase;
    exit->rp_adj = exit->calldepth * sizeof(FrameInfo*);
    exit->nativeCalleeWord = 0;
    exit->lookupFlags = js_InferFlags(cx, 0);
    memcpy(exit->fullTypeMap(), typemap, typemap_size);

#if defined JS_JIT_SPEW
    TreevisLogExit(cx, exit);
#endif
    return exit;
}

JS_REQUIRES_STACK GuardRecord*
TraceRecorder::createGuardRecord(VMSideExit* exit)
{
#ifdef JS_JIT_SPEW
    
    
    
    
    GuardRecord* gr = new (dataAlloc()) GuardRecord();
#else
    
    GuardRecord* gr = new (traceAlloc()) GuardRecord();
#endif

    gr->exit = exit;
    exit->addGuard(gr);

    
    verbose_only(
        gr->profGuardID = fragment->guardNumberer++;
        gr->nextInFrag = fragment->guardsForFrag;
        fragment->guardsForFrag = gr;
    )

    return gr;
}





JS_REQUIRES_STACK void
TraceRecorder::guard(bool expected, LIns* cond, VMSideExit* exit)
{
    debug_only_printf(LC_TMRecorder,
                      "    About to try emitting guard code for "
                      "SideExit=%p exitType=%s\n",
                      (void*)exit, getExitName(exit->exitType));

    GuardRecord* guardRec = createGuardRecord(exit);

    if (exit->exitType == LOOP_EXIT)
        tree->sideExits.add(exit);

    if (!cond->isCmp()) {
        expected = !expected;
        cond = cond->isI() ? lir->insEqI_0(cond) : lir->insEqP_0(cond);
    }

    LIns* guardIns =
        lir->insGuard(expected ? LIR_xf : LIR_xt, cond, guardRec);
    if (!guardIns) {
        debug_only_print0(LC_TMRecorder,
                          "    redundant guard, eliminated, no codegen\n");
    }
}





JS_REQUIRES_STACK LIns*
TraceRecorder::guard_xov(LOpcode op, LIns* d0, LIns* d1, VMSideExit* exit)
{
    debug_only_printf(LC_TMRecorder,
                      "    About to try emitting guard_xov code for "
                      "SideExit=%p exitType=%s\n",
                      (void*)exit, getExitName(exit->exitType));

    GuardRecord* guardRec = createGuardRecord(exit);
    JS_ASSERT(exit->exitType == OVERFLOW_EXIT);

    switch (op) {
      case LIR_addi:
        op = LIR_addxovi;
        break;
      case LIR_subi:
        op = LIR_subxovi;
        break;
      case LIR_muli:
        op = LIR_mulxovi;
        break;
      default:
        JS_NOT_REACHED("unexpected comparison op");
        break;
    }

    LIns* guardIns = lir->insGuardXov(op, d0, d1, guardRec);
    NanoAssert(guardIns);
    return guardIns;
}

JS_REQUIRES_STACK VMSideExit*
TraceRecorder::copy(VMSideExit* copy)
{
    size_t typemap_size = copy->numGlobalSlots + copy->numStackSlots;
    VMSideExit* exit = (VMSideExit*)
        traceAlloc().alloc(sizeof(VMSideExit) + typemap_size * sizeof(JSValueType));

    
    memcpy(exit, copy, sizeof(VMSideExit) + typemap_size * sizeof(JSValueType));
    exit->guards = NULL;
    exit->from = fragment;
    exit->target = NULL;

    if (exit->exitType == LOOP_EXIT)
        tree->sideExits.add(exit);
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
    if (cx->tracerState) 
        return true;

    JSCList *cl;

#ifdef JS_THREADSAFE
    JSThread* thread = cx->thread;
    for (cl = thread->contextList.next; cl != &thread->contextList; cl = cl->next)
        if (CX_FROM_THREAD_LINKS(cl)->tracerState)
            return true;
#else
    JSRuntime* rt = cx->runtime;
    for (cl = rt->contextList.next; cl != &rt->contextList; cl = cl->next)
        if (js_ContextFromLinkField(cl)->tracerState)
            return true;
#endif
    return false;
}

static void
ResetJITImpl(JSContext* cx)
{
    if (!(cx->jitEnabled || (cx->options & JSOPTION_METHODJIT)))
        return;
    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    debug_only_print0(LC_TMTracer, "Flushing cache.\n");
    if (tm->recorder) {
        JS_ASSERT_NOT_ON_TRACE(cx);
        AbortRecording(cx, "flush cache");
    }
    if (ProhibitFlush(cx)) {
        debug_only_print0(LC_TMTracer, "Deferring JIT flush due to deep bail.\n");
        tm->needFlush = JS_TRUE;
        return;
    }
    tm->flush();
}


JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::compile()
{
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_COMPILE);
#endif

    if (traceMonitor->needFlush) {
        ResetJIT(cx, FR_DEEP_BAIL);
        return ARECORD_ABORTED;
    }
    if (tree->maxNativeStackSlots >= MAX_NATIVE_STACK_SLOTS) {
        debug_only_print0(LC_TMTracer, "Blacklist: excessive stack use.\n");
        Blacklist((jsbytecode*)tree->ip);
        return ARECORD_STOP;
    }
    if (anchor && anchor->exitType != CASE_EXIT)
        ++tree->branchCount;
    if (outOfMemory())
        return ARECORD_STOP;

    
#if defined DEBUG && !defined WIN32
    
    const char* filename = cx->fp()->getScript()->filename;
    char* label = (char*)js_malloc((filename ? strlen(filename) : 7) + 16);
    sprintf(label, "%s:%u", filename ? filename : "<stdin>",
            js_FramePCToLineNumber(cx, cx->fp()));
    lirbuf->printer->addrNameMap->addAddrRange(fragment, sizeof(Fragment), 0, label);
    js_free(label);
#endif

    Assembler *assm = traceMonitor->assembler;
    JS_ASSERT(assm->error() == nanojit::None);
    assm->compile(fragment, tempAlloc(), true verbose_only(, lirbuf->printer));

    if (assm->error() != nanojit::None) {
        assm->setError(nanojit::None);
        debug_only_print0(LC_TMTracer, "Blacklisted: error during compilation\n");
        Blacklist((jsbytecode*)tree->ip);
        return ARECORD_STOP;
    }

    if (outOfMemory())
        return ARECORD_STOP;
    ResetRecordingAttempts(cx, (jsbytecode*)fragment->ip);
    ResetRecordingAttempts(cx, (jsbytecode*)tree->ip);
    if (anchor) {
#ifdef NANOJIT_IA32
        if (anchor->exitType == CASE_EXIT)
            assm->patch(anchor, anchor->switchInfo);
        else
#endif
            assm->patch(anchor);
    }
    JS_ASSERT(fragment->code());
    JS_ASSERT_IF(fragment == fragment->root, fragment->root == tree);

    return ARECORD_CONTINUE;
}

static void
JoinPeers(Assembler* assm, VMSideExit* exit, TreeFragment* target)
{
    exit->target = target;
    assm->patch(exit);

    debug_only_printf(LC_TMTreeVis, "TREEVIS JOIN ANCHOR=%p FRAG=%p\n", (void*)exit, (void*)target);

    if (exit->root() == target)
        return;

    target->dependentTrees.addUnique(exit->root());
    exit->root()->linkedTrees.addUnique(target);
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
          : vp(NULL), promoteInt(false), lastCheck(TypeCheck_Bad)
        {}
        SlotInfo(Value* vp, bool promoteInt)
          : vp(vp), promoteInt(promoteInt), lastCheck(TypeCheck_Bad), type(getCoercedType(*vp))
        {}
        SlotInfo(JSObject** p)
          : vp(vp), promoteInt(false), lastCheck(TypeCheck_Bad), type(getFrameObjPtrTraceType(*p))
        {}
        SlotInfo(Value* vp, JSValueType t)
          : vp(vp), promoteInt(t == JSVAL_TYPE_INT32), lastCheck(TypeCheck_Bad), type(t)
        {}
        void            *vp;
        bool            promoteInt;
        TypeCheckResult lastCheck;
        JSValueType     type;
    };

    SlotMap(TraceRecorder& rec)
        : mRecorder(rec),
          mCx(rec.cx),
          slots(NULL)
    {
    }

    virtual ~SlotMap()
    {
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot)
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
    checkTypes(LinkableFragment* f)
    {
        if (length() != f->typeMap.length())
            return TypeConsensus_Bad;

        bool has_undemotes = false;
        for (unsigned i = 0; i < length(); i++) {
            TypeCheckResult result = checkType(i, f->typeMap[i]);
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
    addSlot(Value* vp)
    {
        bool promoteInt = false;
        if (vp->isNumber()) {
            if (LIns* i = mRecorder.getFromTracker(vp)) {
                promoteInt = isPromoteInt(i);
            } else if (mRecorder.isGlobal(vp)) {
                int offset = mRecorder.tree->globalSlots->offsetOf(uint16(mRecorder.nativeGlobalSlot(vp)));
                JS_ASSERT(offset != -1);
                promoteInt = mRecorder.importTypeMap[mRecorder.importStackSlots + offset] ==
                             JSVAL_TYPE_INT32;
            } else {
                promoteInt = mRecorder.importTypeMap[mRecorder.nativeStackSlot(vp)] ==
                             JSVAL_TYPE_INT32;
            }
        }
        slots.add(SlotInfo(vp, promoteInt));
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    addSlot(JSObject** p)
    {
        slots.add(SlotInfo(p));
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    addSlot(JSValueType t)
    {
        slots.add(SlotInfo(NULL, t));
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    addSlot(Value *vp, JSValueType t)
    {
        slots.add(SlotInfo(vp, t));
    }

    JS_REQUIRES_STACK void
    markUndemotes()
    {
        for (unsigned i = 0; i < length(); i++) {
            if (get(i).lastCheck == TypeCheck_Undemote)
                mRecorder.markSlotUndemotable(mRecorder.tree, i);
        }
    }

    JS_REQUIRES_STACK virtual void
    adjustTail(TypeConsensus consensus)
    {
    }

    JS_REQUIRES_STACK virtual void
    adjustTypes()
    {
        for (unsigned i = 0; i < length(); i++)
            adjustType(get(i));
    }

  protected:
    JS_REQUIRES_STACK virtual void
    adjustType(SlotInfo& info) {
        JS_ASSERT(info.lastCheck != TypeCheck_Undemote && info.lastCheck != TypeCheck_Bad);
#ifdef DEBUG
        if (info.lastCheck == TypeCheck_Promote) {
            JS_ASSERT(info.type == JSVAL_TYPE_INT32 || info.type == JSVAL_TYPE_DOUBLE);
            









            LIns* ins = mRecorder.getFromTrackerImpl(info.vp);
            JS_ASSERT_IF(ins, isPromoteInt(ins));
        } else 
#endif
        if (info.lastCheck == TypeCheck_Demote) {
            JS_ASSERT(info.type == JSVAL_TYPE_INT32 || info.type == JSVAL_TYPE_DOUBLE);
            JS_ASSERT(mRecorder.getImpl(info.vp)->isD());

            
            mRecorder.setImpl(info.vp, mRecorder.getImpl(info.vp), false);
        }
    }

  private:
    TypeCheckResult
    checkType(unsigned i, JSValueType t)
    {
        debug_only_printf(LC_TMTracer,
                          "checkType slot %d: interp=%c typemap=%c isNum=%d promoteInt=%d\n",
                          i,
                          TypeToChar(slots[i].type),
                          TypeToChar(t),
                          slots[i].type == JSVAL_TYPE_INT32 || slots[i].type == JSVAL_TYPE_DOUBLE,
                          slots[i].promoteInt);
        switch (t) {
          case JSVAL_TYPE_INT32:
            if (slots[i].type != JSVAL_TYPE_INT32 && slots[i].type != JSVAL_TYPE_DOUBLE)
                return TypeCheck_Bad; 
            
            if (!slots[i].promoteInt)
                return TypeCheck_Undemote;
            
            JS_ASSERT_IF(slots[i].vp, hasInt32Repr(*(const Value *)slots[i].vp) && slots[i].promoteInt);
            return slots[i].vp ? TypeCheck_Promote : TypeCheck_Okay;
          case JSVAL_TYPE_DOUBLE:
            if (slots[i].type != JSVAL_TYPE_INT32 && slots[i].type != JSVAL_TYPE_DOUBLE)
                return TypeCheck_Bad; 
            if (slots[i].promoteInt)
                return slots[i].vp ? TypeCheck_Demote : TypeCheck_Bad;
            return TypeCheck_Okay;
          default:
            return slots[i].type == t ? TypeCheck_Okay : TypeCheck_Bad;
        }
        JS_NOT_REACHED("shouldn't fall through type check switch");
    }
  protected:
    TraceRecorder& mRecorder;
    JSContext* mCx;
    Queue<SlotInfo> slots;
};

class DefaultSlotMap : public SlotMap
{
  public:
    DefaultSlotMap(TraceRecorder& tr) : SlotMap(tr)
    {
    }
    
    virtual ~DefaultSlotMap()
    {
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, JSStackFrame* fp)
    {
        for (size_t i = 0; i < count; i++)
            addSlot(&vp[i]);
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject** p, JSStackFrame* fp)
    {
        addSlot(p);
        return true;
    }
};

JS_REQUIRES_STACK TypeConsensus
TraceRecorder::selfTypeStability(SlotMap& slotMap)
{
    debug_only_printf(LC_TMTracer, "Checking type stability against self=%p\n", (void*)fragment);
    TypeConsensus consensus = slotMap.checkTypes(tree);

    
    if (consensus == TypeConsensus_Okay)
        return TypeConsensus_Okay;

    


    if (consensus == TypeConsensus_Undemotes)
        slotMap.markUndemotes();

    return consensus;
}

JS_REQUIRES_STACK TypeConsensus
TraceRecorder::peerTypeStability(SlotMap& slotMap, const void* ip, TreeFragment** pPeer)
{
    
    JS_ASSERT(fragment->root == tree);
    TreeFragment* peer = LookupLoop(traceMonitor, ip, tree->globalObj, tree->globalShape, tree->argc);

    
    JS_ASSERT_IF(!peer, tree->ip != ip);
    if (!peer)
        return TypeConsensus_Bad;
    bool onlyUndemotes = false;
    for (; peer != NULL; peer = peer->peer) {
        if (!peer->code() || peer == fragment)
            continue;
        debug_only_printf(LC_TMTracer, "Checking type stability against peer=%p\n", (void*)peer);
        TypeConsensus consensus = slotMap.checkTypes(peer);
        if (consensus == TypeConsensus_Okay) {
            *pPeer = peer;
            


            return TypeConsensus_Okay;
        }
        if (consensus == TypeConsensus_Undemotes)
            onlyUndemotes = true;
    }

    return onlyUndemotes ? TypeConsensus_Undemotes : TypeConsensus_Bad;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::closeLoop()
{
    return closeLoop(snapshot(UNSTABLE_LOOP_EXIT));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::closeLoop(VMSideExit* exit)
{
    DefaultSlotMap slotMap(*this);
    VisitSlots(slotMap, cx, 0, *tree->globalSlots);
    return closeLoop(slotMap, exit);
}






JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::closeLoop(SlotMap& slotMap, VMSideExit* exit)
{
    




    JS_ASSERT((*cx->regs->pc == JSOP_TRACE || *cx->regs->pc == JSOP_NOP) &&
              !cx->fp()->hasIMacroPC());

    if (callDepth != 0) {
        debug_only_print0(LC_TMTracer,
                          "Blacklisted: stack depth mismatch, possible recursion.\n");
        Blacklist((jsbytecode*)tree->ip);
        trashSelf = true;
        return ARECORD_STOP;
    }

    JS_ASSERT_IF(exit->exitType == UNSTABLE_LOOP_EXIT,
                 exit->numStackSlots == tree->nStackTypes);

    JS_ASSERT(fragment->root == tree);

    TreeFragment* peer = NULL;

    TypeConsensus consensus = TypeConsensus_Bad;

    if (exit->exitType == UNSTABLE_LOOP_EXIT)
        consensus = selfTypeStability(slotMap);
    if (consensus != TypeConsensus_Okay) {
        TypeConsensus peerConsensus = peerTypeStability(slotMap, tree->ip, &peer);
        
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

    
    slotMap.adjustTail(consensus);

    if (consensus != TypeConsensus_Okay || peer) {
        fragment->lastIns = lir->insGuard(LIR_x, NULL, createGuardRecord(exit));

        
        JS_ASSERT_IF(peer, consensus == TypeConsensus_Okay);

        
        if (!peer) {
            




            debug_only_print0(LC_TMTracer,
                              "Trace has unstable loop variable with no stable peer, "
                              "compiling anyway.\n");
            UnstableExit* uexit = new (traceAlloc()) UnstableExit;
            uexit->fragment = fragment;
            uexit->exit = exit;
            uexit->next = tree->unstableExits;
            tree->unstableExits = uexit;
        } else {
            JS_ASSERT(peer->code());
            exit->target = peer;
            debug_only_printf(LC_TMTracer,
                              "Joining type-unstable trace to target fragment %p.\n",
                              (void*)peer);
            peer->dependentTrees.addUnique(tree);
            tree->linkedTrees.addUnique(peer);
        }
    } else {
        exit->exitType = LOOP_EXIT;
        debug_only_printf(LC_TMTreeVis, "TREEVIS CHANGEEXIT EXIT=%p TYPE=%s\n", (void*)exit,
                          getExitName(LOOP_EXIT));

        JS_ASSERT((fragment == fragment->root) == !!loopLabel);
        if (loopLabel) {
            lir->insBranch(LIR_j, NULL, loopLabel);
            lir->ins1(LIR_livep, lirbuf->state);
        }

        exit->target = tree;
        





        fragment->lastIns = lir->insGuard(LIR_x, NULL, createGuardRecord(exit));
    }

    CHECK_STATUS_A(compile());

    debug_only_printf(LC_TMTreeVis, "TREEVIS CLOSELOOP EXIT=%p PEER=%p\n", (void*)exit, (void*)peer);

    JS_ASSERT(LookupLoop(traceMonitor, tree->ip, tree->globalObj, tree->globalShape, tree->argc) ==
              tree->first);
    JS_ASSERT(tree->first);

    peer = tree->first;
    joinEdgesToEntry(peer);

    debug_only_stmt(DumpPeerStability(traceMonitor, peer->ip, peer->globalObj,
                                      peer->globalShape, peer->argc);)

    debug_only_print0(LC_TMTracer,
                      "updating specializations on dependent and linked trees\n");
    if (tree->code())
        SpecializeTreesToMissingGlobals(cx, globalObj, tree);

    



    if (outer)
        AttemptCompilation(cx, globalObj, outer, outerArgc);
#ifdef JS_JIT_SPEW
    debug_only_printf(LC_TMMinimal,
                      "Recording completed at  %s:%u@%u via closeLoop (FragID=%06u)\n",
                      cx->fp()->getScript()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()),
                      fragment->profFragID);
    debug_only_print0(LC_TMMinimal, "\n");
#endif

    return finishSuccessfully();
}

static void
FullMapFromExit(TypeMap& typeMap, VMSideExit* exit)
{
    typeMap.setLength(0);
    typeMap.fromRaw(exit->stackTypeMap(), exit->numStackSlots);
    typeMap.fromRaw(exit->globalTypeMap(), exit->numGlobalSlots);
    
    if (exit->numGlobalSlots < exit->root()->nGlobalTypes()) {
        typeMap.fromRaw(exit->root()->globalTypeMap() + exit->numGlobalSlots,
                        exit->root()->nGlobalTypes() - exit->numGlobalSlots);
    }
}

static JS_REQUIRES_STACK TypeConsensus
TypeMapLinkability(JSContext* cx, const TypeMap& typeMap, TreeFragment* peer)
{
    const TypeMap& peerMap = peer->typeMap;
    unsigned minSlots = JS_MIN(typeMap.length(), peerMap.length());
    TypeConsensus consensus = TypeConsensus_Okay;
    for (unsigned i = 0; i < minSlots; i++) {
        if (typeMap[i] == peerMap[i])
            continue;
        if (typeMap[i] == JSVAL_TYPE_INT32 && peerMap[i] == JSVAL_TYPE_DOUBLE &&
            IsSlotUndemotable(JS_TRACE_MONITOR(cx).oracle, cx, peer, i, peer->ip)) {
            consensus = TypeConsensus_Undemotes;
        } else {
            return TypeConsensus_Bad;
        }
    }
    return consensus;
}

JS_REQUIRES_STACK unsigned
TraceRecorder::findUndemotesInTypemaps(const TypeMap& typeMap, LinkableFragment* f,
                        Queue<unsigned>& undemotes)
{
    undemotes.setLength(0);
    unsigned minSlots = JS_MIN(typeMap.length(), f->typeMap.length());
    for (unsigned i = 0; i < minSlots; i++) {
        if (typeMap[i] == JSVAL_TYPE_INT32 && f->typeMap[i] == JSVAL_TYPE_DOUBLE) {
            undemotes.add(i);
        } else if (typeMap[i] != f->typeMap[i]) {
            return 0;
        }
    }
    for (unsigned i = 0; i < undemotes.length(); i++)
        markSlotUndemotable(f, undemotes[i]);
    return undemotes.length();
}

JS_REQUIRES_STACK void
TraceRecorder::joinEdgesToEntry(TreeFragment* peer_root)
{
    if (fragment->root != fragment)
        return;

    TypeMap typeMap(NULL);
    Queue<unsigned> undemotes(NULL);

    for (TreeFragment* peer = peer_root; peer; peer = peer->peer) {
        if (!peer->code())
            continue;
        UnstableExit* uexit = peer->unstableExits;
        while (uexit != NULL) {
            
            FullMapFromExit(typeMap, uexit->exit);
            
            TypeConsensus consensus = TypeMapLinkability(cx, typeMap, tree);
            JS_ASSERT_IF(consensus == TypeConsensus_Okay, peer != fragment);
            if (consensus == TypeConsensus_Okay) {
                debug_only_printf(LC_TMTracer,
                                  "Joining type-stable trace to target exit %p->%p.\n",
                                  (void*)uexit->fragment, (void*)uexit->exit);

                



                TreeFragment* from = uexit->exit->root();
                if (from->nGlobalTypes() < tree->nGlobalTypes()) {
                    SpecializeTreesToLateGlobals(cx, from, tree->globalTypeMap(),
                                                 tree->nGlobalTypes());
                }

                
                JS_ASSERT(tree == fragment);
                JoinPeers(traceMonitor->assembler, uexit->exit, tree);
                uexit = peer->removeUnstableExit(uexit->exit);
            } else {
                
                if (findUndemotesInTypemaps(typeMap, tree, undemotes)) {
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

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::endLoop()
{
    return endLoop(snapshot(LOOP_EXIT));
}


JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::endLoop(VMSideExit* exit)
{
    JS_ASSERT(fragment->root == tree);

    if (callDepth != 0) {
        debug_only_print0(LC_TMTracer, "Blacklisted: stack depth mismatch, possible recursion.\n");
        Blacklist((jsbytecode*)tree->ip);
        trashSelf = true;
        return ARECORD_STOP;
    }

    fragment->lastIns =
        lir->insGuard(LIR_x, NULL, createGuardRecord(exit));

    CHECK_STATUS_A(compile());

    debug_only_printf(LC_TMTreeVis, "TREEVIS ENDLOOP EXIT=%p\n", (void*)exit);

    JS_ASSERT(LookupLoop(traceMonitor, tree->ip, tree->globalObj, tree->globalShape, tree->argc) ==
              tree->first);

    joinEdgesToEntry(tree->first);

    debug_only_stmt(DumpPeerStability(traceMonitor, tree->ip, tree->globalObj,
                                      tree->globalShape, tree->argc);)

    



    debug_only_print0(LC_TMTracer,
                      "updating specializations on dependent and linked trees\n");
    if (tree->code())
        SpecializeTreesToMissingGlobals(cx, globalObj, fragment->root);

    



    if (outer)
        AttemptCompilation(cx, globalObj, outer, outerArgc);
#ifdef JS_JIT_SPEW
    debug_only_printf(LC_TMMinimal,
                      "Recording completed at  %s:%u@%u via endLoop (FragID=%06u)\n",
                      cx->fp()->getScript()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()),
                      fragment->profFragID);
    debug_only_print0(LC_TMTracer, "\n");
#endif

    return finishSuccessfully();
}


JS_REQUIRES_STACK void
TraceRecorder::prepareTreeCall(TreeFragment* inner)
{
    VMSideExit* exit = snapshot(OOM_EXIT);

    






    if (callDepth > 0) {
        




        ptrdiff_t sp_adj = nativeStackOffset(&cx->fp()->argv[-2]);

        
        ptrdiff_t rp_adj = callDepth * sizeof(FrameInfo*);

        



        debug_only_printf(LC_TMTracer,
                          "sp_adj=%lld outer=%lld inner=%lld\n",
                          (long long int)sp_adj,
                          (long long int)tree->nativeStackBase,
                          (long long int)inner->nativeStackBase);
        ptrdiff_t sp_offset =
                - tree->nativeStackBase 
                + sp_adj 
                + inner->maxNativeStackSlots * sizeof(double); 
        LIns* sp_top = lir->ins2(LIR_addp, lirbuf->sp, INS_CONSTWORD(sp_offset));
        guard(true, lir->ins2(LIR_ltp, sp_top, eos_ins), exit);

        
        ptrdiff_t rp_offset = rp_adj + inner->maxCallDepth * sizeof(FrameInfo*);
        LIns* rp_top = lir->ins2(LIR_addp, lirbuf->rp, INS_CONSTWORD(rp_offset));
        guard(true, lir->ins2(LIR_ltp, rp_top, eor_ins), exit);

        sp_offset =
                - tree->nativeStackBase 
                + sp_adj 
                + inner->nativeStackBase; 
        
        lir->insStore(lir->ins2(LIR_addp, lirbuf->sp, INS_CONSTWORD(sp_offset)),
                lirbuf->state, offsetof(TracerState, sp), ACCSET_OTHER);
        lir->insStore(lir->ins2(LIR_addp, lirbuf->rp, INS_CONSTWORD(rp_adj)),
                lirbuf->state, offsetof(TracerState, rp), ACCSET_OTHER);
    }

    





    GuardRecord* guardRec = createGuardRecord(exit);
    lir->insGuard(LIR_xbarrier, NULL, guardRec);
}

static unsigned
BuildGlobalTypeMapFromInnerTree(Queue<JSValueType>& typeMap, VMSideExit* inner)
{
#if defined DEBUG
    unsigned initialSlots = typeMap.length();
#endif
    
    typeMap.add(inner->globalTypeMap(), inner->numGlobalSlots);

    
    TreeFragment* innerFrag = inner->root();
    unsigned slots = inner->numGlobalSlots;
    if (slots < innerFrag->nGlobalTypes()) {
        typeMap.add(innerFrag->globalTypeMap() + slots, innerFrag->nGlobalTypes() - slots);
        slots = innerFrag->nGlobalTypes();
    }
    JS_ASSERT(typeMap.length() - initialSlots == slots);
    return slots;
}


JS_REQUIRES_STACK void
TraceRecorder::emitTreeCall(TreeFragment* inner, VMSideExit* exit)
{
    
    LIns* args[] = { lirbuf->state }; 
    
    CallInfo* ci = new (traceAlloc()) CallInfo();
    ci->_address = uintptr_t(inner->code());
    JS_ASSERT(ci->_address);
    ci->_typesig = CallInfo::typeSig1(ARGTYPE_P, ARGTYPE_P);
    ci->_isPure = 0;
    ci->_storeAccSet = ACCSET_STORE_ANY;
    ci->_abi = ABI_FASTCALL;
#ifdef DEBUG
    ci->_name = "fragment";
#endif
    LIns* rec = lir->insCall(ci, args);
    LIns* lr = lir->insLoad(LIR_ldp, rec, offsetof(GuardRecord, exit), ACCSET_OTHER);
    LIns* nested = lir->insBranch(LIR_jt,
                                  lir->ins2ImmI(LIR_eqi,
                                             lir->insLoad(LIR_ldi, lr,
                                                          offsetof(VMSideExit, exitType),
                                                          ACCSET_OTHER),
                                             NESTED_EXIT),
                                  NULL);

    




    lir->insStore(lr, lirbuf->state, offsetof(TracerState, lastTreeExitGuard), ACCSET_OTHER);
    LIns* done1 = lir->insBranch(LIR_j, NULL, NULL);

    




    nested->setTarget(lir->ins0(LIR_label));
    LIns* done2 = lir->insBranch(LIR_jf,
                                 lir->insEqP_0(lir->insLoad(LIR_ldp,
                                                            lirbuf->state,
                                                            offsetof(TracerState, lastTreeCallGuard),
                                                            ACCSET_OTHER)),
                                 NULL);
    lir->insStore(lr, lirbuf->state, offsetof(TracerState, lastTreeCallGuard), ACCSET_OTHER);
    lir->insStore(lir->ins2(LIR_addp,
                             lir->insLoad(LIR_ldp, lirbuf->state, offsetof(TracerState, rp),
                                          ACCSET_OTHER),
                             lir->insI2P(lir->ins2ImmI(LIR_lshi,
                                                     lir->insLoad(LIR_ldi, lr,
                                                                  offsetof(VMSideExit, calldepth),
                                                                  ACCSET_OTHER),
                                                     sizeof(void*) == 4 ? 2 : 3))),
                   lirbuf->state,
                   offsetof(TracerState, rpAtLastTreeCall), ACCSET_OTHER);
    LIns* label = lir->ins0(LIR_label);
    done1->setTarget(label);
    done2->setTarget(label);

    



    lir->insStore(lr, lirbuf->state, offsetof(TracerState, outermostTreeExitGuard), ACCSET_OTHER);

    
#ifdef DEBUG
    JSValueType* map;
    size_t i;
    map = exit->globalTypeMap();
    for (i = 0; i < exit->numGlobalSlots; i++)
        JS_ASSERT(map[i] != JSVAL_TYPE_BOXED);
    map = exit->stackTypeMap();
    for (i = 0; i < exit->numStackSlots; i++)
        JS_ASSERT(map[i] != JSVAL_TYPE_BOXED);
#endif

    





    clearEntryFrameSlotsFromTracker(tracker);
    clearCurrentFrameSlotsFromTracker(tracker);
    SlotList& gslots = *tree->globalSlots;
    for (unsigned i = 0; i < gslots.length(); i++) {
        unsigned slot = gslots[i];
        Value* vp = &globalObj->getSlotRef(slot);
        tracker.set(vp, NULL);
    }

    
    importTypeMap.setLength(NativeStackSlots(cx, callDepth));
#ifdef DEBUG
    for (unsigned i = importStackSlots; i < importTypeMap.length(); i++)
        importTypeMap[i] = JSVAL_TYPE_UNINITIALIZED;
#endif
    unsigned startOfInnerFrame = importTypeMap.length() - exit->numStackSlots;
    for (unsigned i = 0; i < exit->numStackSlots; i++)
        importTypeMap[startOfInnerFrame + i] = exit->stackTypeMap()[i];
    importStackSlots = importTypeMap.length();
    JS_ASSERT(importStackSlots == NativeStackSlots(cx, callDepth));

    



    BuildGlobalTypeMapFromInnerTree(importTypeMap, exit);

    importGlobalSlots = importTypeMap.length() - importStackSlots;
    JS_ASSERT(importGlobalSlots == tree->globalSlots->length());

    
    if (callDepth > 0) {
        lir->insStore(lirbuf->sp, lirbuf->state, offsetof(TracerState, sp), ACCSET_OTHER);
        lir->insStore(lirbuf->rp, lirbuf->state, offsetof(TracerState, rp), ACCSET_OTHER);
    }

    



    VMSideExit* nestedExit = snapshot(NESTED_EXIT);
    JS_ASSERT(exit->exitType == LOOP_EXIT);
    guard(true, lir->ins2(LIR_eqp, lr, INS_CONSTPTR(exit)), nestedExit);
    debug_only_printf(LC_TMTreeVis, "TREEVIS TREECALL INNER=%p EXIT=%p GUARD=%p\n", (void*)inner,
                      (void*)nestedExit, (void*)exit);

    
    inner->dependentTrees.addUnique(fragment->root);
    tree->linkedTrees.addUnique(inner);
}


JS_REQUIRES_STACK void
TraceRecorder::trackCfgMerges(jsbytecode* pc)
{
    
    JS_ASSERT((*pc == JSOP_IFEQ) || (*pc == JSOP_IFEQX));
    jssrcnote* sn = js_GetSrcNote(cx->fp()->getScript(), pc);
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
    if (IsLoopEdge(pc, (jsbytecode*)tree->ip)) {
        exitType = LOOP_EXIT;

        




        if ((*pc == JSOP_IFEQ || *pc == JSOP_IFEQX) == cond) {
            JS_ASSERT(*pc == JSOP_IFNE || *pc == JSOP_IFNEX || *pc == JSOP_IFEQ || *pc == JSOP_IFEQX);
            debug_only_print0(LC_TMTracer,
                              "Walking out of the loop, terminating it anyway.\n");
            cond = !cond;
        }

        




        if (x->isImmI()) {
            pendingLoop = (x->immI() == int32(cond));
            return;
        }
    } else {
        exitType = BRANCH_EXIT;
    }
    if (!x->isImmI())
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


JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::checkTraceEnd(jsbytecode *pc)
{
    if (IsLoopEdge(pc, (jsbytecode*)tree->ip)) {
        





        if (pendingLoop) {
            JS_ASSERT(!cx->fp()->hasIMacroPC() && (pc == cx->regs->pc || pc == cx->regs->pc + 1));
            JSFrameRegs orig = *cx->regs;

            cx->regs->pc = (jsbytecode*)tree->ip;
            cx->regs->sp = cx->fp()->base() + tree->spOffsetAtEntry;

            JSContext* localcx = cx;
            AbortableRecordingStatus ars = closeLoop();
            *localcx->regs = orig;
            return ars;
        }

        return endLoop();
    }
    return ARECORD_CONTINUE;
}






static JS_REQUIRES_STACK bool
CheckGlobalObjectShape(JSContext* cx, TraceMonitor* tm, JSObject* globalObj,
                       uint32 *shape = NULL, SlotList** slots = NULL)
{
    if (tm->needFlush) {
        ResetJIT(cx, FR_DEEP_BAIL);
        return false;
    }

    if (globalObj->numSlots() > MAX_GLOBAL_SLOTS) {
        if (tm->recorder)
            AbortRecording(cx, "too many slots in global object");
        return false;
    }

    




    if (!globalObj->hasOwnShape()) {
        JS_LOCK_OBJ(cx, globalObj);
        bool ok = globalObj->globalObjectOwnShapeChange(cx);
        JS_UNLOCK_OBJ(cx, globalObj);
        if (!ok) {
            debug_only_print0(LC_TMTracer,
                              "Can't record: failed to give globalObj a unique shape.\n");
            return false;
        }
    }

    uint32 globalShape = globalObj->shape();

    if (tm->recorder) {
        TreeFragment* root = tm->recorder->getFragment()->root;

        
        if (globalObj != root->globalObj || globalShape != root->globalShape) {
            AUDIT(globalShapeMismatchAtEntry);
            debug_only_printf(LC_TMTracer,
                              "Global object/shape mismatch (%p/%u vs. %p/%u), flushing cache.\n",
                              (void*)globalObj, globalShape, (void*)root->globalObj,
                              root->globalShape);
            Backoff(cx, (jsbytecode*) root->ip);
            ResetJIT(cx, FR_GLOBAL_SHAPE_MISMATCH);
            return false;
        }
        if (shape)
            *shape = globalShape;
        if (slots)
            *slots = root->globalSlots;
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
    ResetJIT(cx, FR_GLOBALS_FULL);
    return false;
}





bool JS_REQUIRES_STACK
TraceRecorder::startRecorder(JSContext* cx, VMSideExit* anchor, VMFragment* f,
                             unsigned stackSlots, unsigned ngslots,
                             JSValueType* typeMap, VMSideExit* expectedInnerExit,
                             jsbytecode* outer, uint32 outerArgc, bool speculate)
{
    TraceMonitor *tm = &JS_TRACE_MONITOR(cx);
    JS_ASSERT(!tm->needFlush);
    JS_ASSERT_IF(cx->fp()->hasIMacroPC(), f->root != f);

    tm->recorder = new TraceRecorder(cx, anchor, f, stackSlots, ngslots, typeMap,
                                     expectedInnerExit, outer, outerArgc, speculate);

    if (!tm->recorder || tm->outOfMemory() || OverfullJITCache(tm)) {
        ResetJIT(cx, FR_OOM);
        return false;
    }

    return true;
}

static void
TrashTree(JSContext* cx, TreeFragment* f)
{
    JS_ASSERT(f == f->root);
    debug_only_printf(LC_TMTreeVis, "TREEVIS TRASH FRAG=%p\n", (void*)f);

    if (!f->code())
        return;
    AUDIT(treesTrashed);
    debug_only_print0(LC_TMTracer, "Trashing tree info.\n");
    f->setCode(NULL);
    TreeFragment** data = f->dependentTrees.data();
    unsigned length = f->dependentTrees.length();
    for (unsigned n = 0; n < length; ++n)
        TrashTree(cx, data[n]);
    data = f->linkedTrees.data();
    length = f->linkedTrees.length();
    for (unsigned n = 0; n < length; ++n)
        TrashTree(cx, data[n]);
}

static int
SynthesizeFrame(JSContext* cx, const FrameInfo& fi, JSObject* callee)
{
    VOUCH_DOES_NOT_REQUIRE_STACK();

    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, callee);
    JS_ASSERT(FUN_INTERPRETED(fun));

    
    JSStackFrame* const fp = cx->fp();
    JS_ASSERT_IF(!fi.imacpc,
                 js_ReconstructStackDepth(cx, fp->getScript(), fi.pc) ==
                 uintN(fi.spdist - fp->getFixedCount()));

    
    JSScript* newscript = fun->u.i.script;
    Value* sp = fp->slots() + fi.spdist;
    uintN argc = fi.get_argc();
    Value* vp = sp - (2 + argc);

    
    cx->regs->sp = sp;
    cx->regs->pc = fi.pc;
    if (fi.imacpc)
        fp->setIMacroPC(fi.imacpc);
    fp->setBlockChain(fi.block);

    







    StackSpace &stack = cx->stack();
    uintN nslots = newscript->nslots;
    uintN funargs = fun->nargs;
    Value *argv = vp + 2;
    JSStackFrame *newfp;
    if (argc < funargs) {
        uintN missing = funargs - argc;
        newfp = stack.getInlineFrame(cx, sp, missing, nslots);
        for (Value *v = argv + argc, *end = v + missing; v != end; ++v)
            v->setUndefined();
    } else {
        newfp = stack.getInlineFrame(cx, sp, 0, nslots);
    }

    
    newfp->setCallObj(NULL);
    newfp->setArgsObj(NULL);
    newfp->setScript(newscript);
    newfp->setFunction(fun);
    newfp->setNumActualArgs(argc);
    newfp->argv = argv;
#ifdef DEBUG
    
    
    newfp->argv[-1].setMagic(JS_THIS_POISON);
#endif
    newfp->clearReturnValue();
    newfp->setAnnotation(NULL);
    newfp->setScopeChain(NULL); 
    newfp->flags = fi.is_constructing() ? JSFRAME_CONSTRUCTING : 0;
    newfp->setBlockChain(NULL);
    newfp->setThisValue(NullValue()); 
    JS_ASSERT(!newfp->hasIMacroPC());

    



    newfp->setCallerVersion((JSVersion) fp->getScript()->version);

    
    stack.pushInlineFrame(cx, fp, fi.pc, newfp);

    
    cx->regs->fp = newfp;
    cx->regs->pc = newscript->code;
    cx->regs->sp = newfp->base();

    



    JSInterpreterHook hook = cx->debugHooks->callHook;
    if (hook) {
        newfp->setHookData(hook(cx, newfp, JS_TRUE, 0, cx->debugHooks->callHookData));
    } else {
        newfp->setHookData(NULL);
    }

    







    return (fi.spdist - newfp->down->getFixedCount()) +
          ((fun->nargs > newfp->numActualArgs()) ? fun->nargs - newfp->numActualArgs() : 0) +
           newscript->nfixed + SPECIAL_FRAME_SLOTS;
}

JS_REQUIRES_STACK static void
SynthesizeSlowNativeFrame(TracerState& state, JSContext *cx, VMSideExit *exit)
{
    




    StackSegment *seg;
    JSStackFrame *fp;
    cx->stack().getSynthesizedSlowNativeFrame(cx, seg, fp);

#ifdef DEBUG
    JSObject *callee = &state.nativeVp[0].toObject();
    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, callee);
    JS_ASSERT(!fun->isInterpreted() && !fun->isFastNative());
    JS_ASSERT(fun->u.n.extra == 0);
#endif

    fp->setCallObj(NULL);
    fp->setArgsObj(NULL);
    fp->setScript(NULL);
    fp->setThisValue(state.nativeVp[1]);
    fp->setNumActualArgs(state.nativeVpLen - 2);
    fp->argv = state.nativeVp + 2;
    fp->setFunction(GET_FUNCTION_PRIVATE(cx, fp->callee()));
    fp->clearReturnValue();
    fp->setAnnotation(NULL);
    fp->setScopeChain(cx->fp()->getScopeChain());
    fp->setBlockChain(NULL);
    fp->flags = exit->constructing() ? JSFRAME_CONSTRUCTING : 0;
    JS_ASSERT(!fp->hasIMacroPC());

    state.bailedSlowNativeRegs.fp = fp;
    state.bailedSlowNativeRegs.pc = NULL;
    state.bailedSlowNativeRegs.sp = fp->slots();

    cx->stack().pushSynthesizedSlowNativeFrame(cx, seg, state.bailedSlowNativeRegs);
}

static JS_REQUIRES_STACK bool
RecordTree(JSContext* cx, TreeFragment* first, jsbytecode* outer,
           uint32 outerArgc, SlotList* globalSlots)
{
    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    
    JS_ASSERT(first->first == first);
    TreeFragment* f = NULL;
    size_t count = 0;
    for (TreeFragment* peer = first; peer; peer = peer->peer, ++count) {
        if (!peer->code())
            f = peer;
    }
    if (!f)
        f = AddNewPeerToPeerList(tm, first);
    JS_ASSERT(f->root == f);

    
    bool speculate = count < MAXPEERS-1;

    
    const void* localRootIP = f->root->ip;

    
    if (!CheckGlobalObjectShape(cx, tm, f->globalObj)) {
        Backoff(cx, (jsbytecode*) localRootIP);
        return false;
    }

    AUDIT(recorderStarted);

    if (tm->outOfMemory() || OverfullJITCache(tm)) {
        Backoff(cx, (jsbytecode*) f->root->ip);
        ResetJIT(cx, FR_OOM);
        debug_only_print0(LC_TMTracer,
                          "Out of memory recording new tree, flushing cache.\n");
        return false;
    }

    JS_ASSERT(!f->code());

    f->initialize(cx, globalSlots, speculate);

#ifdef DEBUG
    AssertTreeIsUnique(tm, f);
#endif
#ifdef JS_JIT_SPEW
    debug_only_printf(LC_TMTreeVis, "TREEVIS CREATETREE ROOT=%p PC=%p FILE=\"%s\" LINE=%d OFFS=%d",
                      (void*)f, f->ip, f->treeFileName, f->treeLineNumber,
                      FramePCOffset(cx, cx->fp()));
    debug_only_print0(LC_TMTreeVis, " STACK=\"");
    for (unsigned i = 0; i < f->nStackTypes; i++)
        debug_only_printf(LC_TMTreeVis, "%c", TypeToChar(f->typeMap[i]));
    debug_only_print0(LC_TMTreeVis, "\" GLOBALS=\"");
    for (unsigned i = 0; i < f->nGlobalTypes(); i++)
        debug_only_printf(LC_TMTreeVis, "%c", TypeToChar(f->typeMap[f->nStackTypes + i]));
    debug_only_print0(LC_TMTreeVis, "\"\n");
#endif

    
    return TraceRecorder::startRecorder(cx, NULL, f, f->nStackTypes,
                                        f->globalSlots->length(),
                                        f->typeMap.data(), NULL,
                                        outer, outerArgc, speculate);
}

static JS_REQUIRES_STACK TypeConsensus
FindLoopEdgeTarget(JSContext* cx, VMSideExit* exit, TreeFragment** peerp)
{
    TreeFragment* from = exit->root();

    JS_ASSERT(from->code());
    Oracle* oracle = JS_TRACE_MONITOR(cx).oracle;

    TypeMap typeMap(NULL);
    FullMapFromExit(typeMap, exit);
    JS_ASSERT(typeMap.length() - exit->numStackSlots == from->nGlobalTypes());

    
    uint16* gslots = from->globalSlots->data();
    for (unsigned i = 0; i < typeMap.length(); i++) {
        if (typeMap[i] == JSVAL_TYPE_DOUBLE) {
            if (i < from->nStackTypes)
                oracle->markStackSlotUndemotable(cx, i, from->ip);
            else if (i >= exit->numStackSlots)
                oracle->markGlobalSlotUndemotable(cx, gslots[i - exit->numStackSlots]);
        }
    }

    JS_ASSERT(exit->exitType == UNSTABLE_LOOP_EXIT);

    TreeFragment* firstPeer = from->first;

    for (TreeFragment* peer = firstPeer; peer; peer = peer->peer) {
        if (!peer->code())
            continue;
        JS_ASSERT(peer->argc == from->argc);
        JS_ASSERT(exit->numStackSlots == peer->nStackTypes);
        TypeConsensus consensus = TypeMapLinkability(cx, typeMap, peer);
        if (consensus == TypeConsensus_Okay || consensus == TypeConsensus_Undemotes) {
            *peerp = peer;
            return consensus;
        }
    }

    return TypeConsensus_Bad;
}

static JS_REQUIRES_STACK bool
AttemptToStabilizeTree(JSContext* cx, JSObject* globalObj, VMSideExit* exit, jsbytecode* outer,
                       uint32 outerArgc)
{
    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    if (tm->needFlush) {
        ResetJIT(cx, FR_DEEP_BAIL);
        return false;
    }

    TreeFragment* from = exit->root();

    TreeFragment* peer = NULL;
    TypeConsensus consensus = FindLoopEdgeTarget(cx, exit, &peer);
    if (consensus == TypeConsensus_Okay) {
        JS_ASSERT(from->globalSlots == peer->globalSlots);
        JS_ASSERT_IF(exit->exitType == UNSTABLE_LOOP_EXIT,
                     from->nStackTypes == peer->nStackTypes);
        JS_ASSERT(exit->numStackSlots == peer->nStackTypes);
        
        JoinPeers(tm->assembler, exit, peer);
        



        if (peer->nGlobalTypes() < peer->globalSlots->length())
            SpecializeTreesToMissingGlobals(cx, globalObj, peer);
        JS_ASSERT(from->nGlobalTypes() == from->globalSlots->length());
        
        if (exit->exitType == UNSTABLE_LOOP_EXIT)
            from->removeUnstableExit(exit);
        debug_only_stmt(DumpPeerStability(tm, peer->ip, globalObj, from->globalShape, from->argc);)
        return false;
    } else if (consensus == TypeConsensus_Undemotes) {
        
        TrashTree(cx, peer);
        return false;
    }

    SlotList *globalSlots = from->globalSlots;

    JS_ASSERT(from == from->root);

    
    if (*(jsbytecode*)from->ip == JSOP_NOP)
        return false;

    return RecordTree(cx, from->first, outer, outerArgc, globalSlots);
}

static JS_REQUIRES_STACK VMFragment*
CreateBranchFragment(JSContext* cx, TreeFragment* root, VMSideExit* anchor)
{
    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    verbose_only(
    uint32_t profFragID = (LogController.lcbits & LC_FragProfile)
                          ? (++(tm->lastFragID)) : 0;
    )

    VMFragment* f = new (*tm->dataAlloc) VMFragment(cx->regs->pc verbose_only(, profFragID));

    debug_only_printf(LC_TMTreeVis, "TREEVIS CREATEBRANCH ROOT=%p FRAG=%p PC=%p FILE=\"%s\""
                      " LINE=%d ANCHOR=%p OFFS=%d\n",
                      (void*)root, (void*)f, (void*)cx->regs->pc, cx->fp()->getScript()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()), (void*)anchor,
                      FramePCOffset(cx, cx->fp()));
    verbose_only( tm->branches = new (*tm->dataAlloc) Seq<Fragment*>(f, tm->branches); )

    f->root = root;
    if (anchor)
        anchor->target = f;
    return f;
}

static JS_REQUIRES_STACK bool
AttemptToExtendTree(JSContext* cx, VMSideExit* anchor, VMSideExit* exitedFrom, jsbytecode* outer
#ifdef MOZ_TRACEVIS
    , TraceVisStateObj* tvso = NULL
#endif
    )
{
    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    JS_ASSERT(!tm->recorder);

    if (tm->needFlush) {
        ResetJIT(cx, FR_DEEP_BAIL);
#ifdef MOZ_TRACEVIS
        if (tvso) tvso->r = R_FAIL_EXTEND_FLUSH;
#endif
        return false;
    }

    TreeFragment* f = anchor->root();
    JS_ASSERT(f->code());

    



    if (f->branchCount >= MAX_BRANCHES) {
#ifdef MOZ_TRACEVIS
        if (tvso) tvso->r = R_FAIL_EXTEND_MAX_BRANCHES;
#endif
        return false;
    }

    VMFragment* c = (VMFragment*)anchor->target;
    if (!c) {
        c = CreateBranchFragment(cx, f, anchor);
    } else {
        





        c->ip = cx->regs->pc;
        JS_ASSERT(c->root == f);
    }

    debug_only_printf(LC_TMTracer,
                      "trying to attach another branch to the tree (hits = %d)\n", c->hits());

    int32_t& hits = c->hits();
    int32_t maxHits = HOTEXIT + MAXEXIT;
    if (anchor->exitType == CASE_EXIT)
        maxHits *= anchor->switchInfo->count;
    if (outer || (hits++ >= HOTEXIT && hits <= maxHits)) {
        
        unsigned stackSlots;
        unsigned ngslots;
        JSValueType* typeMap;
        TypeMap fullMap(NULL);
        if (!exitedFrom) {
            



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
        bool rv = TraceRecorder::startRecorder(cx, anchor, c, stackSlots, ngslots, typeMap,
                                               exitedFrom, outer, numActualArgs(cx->fp()),
                                               hits < maxHits);
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

static JS_REQUIRES_STACK bool
ExecuteTree(JSContext* cx, TreeFragment* f, uintN& inlineCallCount,
            VMSideExit** innermostNestedGuardp, VMSideExit** lrp);

static inline MonitorResult
RecordingIfTrue(bool b)
{
    return b ? MONITOR_RECORDING : MONITOR_NOT_RECORDING;
}





JS_REQUIRES_STACK MonitorResult
TraceRecorder::recordLoopEdge(JSContext* cx, TraceRecorder* r, uintN& inlineCallCount)
{
#ifdef JS_THREADSAFE
    if (cx->fp()->getScopeChain()->getGlobal()->title.ownercx != cx) {
        AbortRecording(cx, "Global object not owned by this context");
        return MONITOR_NOT_RECORDING; 
    }
#endif

    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    
    if (tm->needFlush) {
        ResetJIT(cx, FR_DEEP_BAIL);
        return MONITOR_NOT_RECORDING;
    }

    JS_ASSERT(r->fragment && !r->fragment->lastIns);
    TreeFragment* root = r->fragment->root;
    TreeFragment* first = LookupOrAddLoop(tm, cx->regs->pc, root->globalObj,
                                          root->globalShape, numActualArgs(cx->fp()));

    



    JSObject* globalObj = cx->fp()->getScopeChain()->getGlobal();
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;
    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots)) {
        JS_ASSERT(!tm->recorder);
        return MONITOR_NOT_RECORDING;
    }

    debug_only_printf(LC_TMTracer,
                      "Looking for type-compatible peer (%s:%d@%d)\n",
                      cx->fp()->getScript()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()));

    
    TreeFragment* f = r->findNestedCompatiblePeer(first);
    if (!f || !f->code()) {
        AUDIT(noCompatInnerTrees);

        TreeFragment* outerFragment = root;
        jsbytecode* outer = (jsbytecode*) outerFragment->ip;
        uint32 outerArgc = outerFragment->argc;
        JS_ASSERT(numActualArgs(cx->fp()) == first->argc);
        AbortRecording(cx, "No compatible inner tree");

        return RecordingIfTrue(RecordTree(cx, first, outer, outerArgc, globalSlots));
    }

    AbortableRecordingStatus status = r->attemptTreeCall(f, inlineCallCount);
    if (status == ARECORD_CONTINUE)
        return MONITOR_RECORDING;
    if (status == ARECORD_ERROR) {
        if (TRACE_RECORDER(cx))
            AbortRecording(cx, "Error returned while recording loop edge");
        return MONITOR_ERROR;
    }
    JS_ASSERT(status == ARECORD_ABORTED && !tm->recorder);
    return MONITOR_NOT_RECORDING;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::attemptTreeCall(TreeFragment* f, uintN& inlineCallCount)
{
    adjustCallerTypes(f);
    prepareTreeCall(f);

#ifdef DEBUG
    uintN oldInlineCallCount = inlineCallCount;
#endif

    JSContext *localCx = cx;

    
    
    
    
    
    
    
    
    
    importTypeMap.setLength(NativeStackSlots(cx, callDepth));
    DetermineTypesVisitor visitor(*this, importTypeMap.data());
    VisitStackSlots(visitor, cx, callDepth);

    VMSideExit* innermostNestedGuard = NULL;
    VMSideExit* lr;
    bool ok = ExecuteTree(cx, f, inlineCallCount, &innermostNestedGuard, &lr);

    



    JS_ASSERT_IF(TRACE_RECORDER(localCx), TRACE_RECORDER(localCx) == this);
    if (!ok)
        return ARECORD_ERROR;
    if (!TRACE_RECORDER(localCx))
        return ARECORD_ABORTED;

    if (!lr) {
        AbortRecording(cx, "Couldn't call inner tree");
        return ARECORD_ABORTED;
    }

    TreeFragment* outerFragment = tree;
    jsbytecode* outer = (jsbytecode*) outerFragment->ip;
    switch (lr->exitType) {
      case LOOP_EXIT:
        
        if (innermostNestedGuard) {
            AbortRecording(cx, "Inner tree took different side exit, abort current "
                              "recording and grow nesting tree");
            return AttemptToExtendTree(localCx, innermostNestedGuard, lr, outer) ?
                ARECORD_CONTINUE : ARECORD_ABORTED;
        }

        JS_ASSERT(oldInlineCallCount == inlineCallCount);

        
        emitTreeCall(f, lr);
        return ARECORD_CONTINUE;

      case UNSTABLE_LOOP_EXIT:
      {
        
        JSObject* _globalObj = globalObj;
        AbortRecording(cx, "Inner tree is trying to stabilize, abort outer recording");
        return AttemptToStabilizeTree(localCx, _globalObj, lr, outer, outerFragment->argc) ?
            ARECORD_CONTINUE : ARECORD_ABORTED;
      }

      case OVERFLOW_EXIT:
        traceMonitor->oracle->markInstructionUndemotable(cx->regs->pc);
        
      case BRANCH_EXIT:
      case CASE_EXIT: {
        
        AbortRecording(cx, "Inner tree is trying to grow, abort outer recording");
        return AttemptToExtendTree(localCx, lr, NULL, outer) ? ARECORD_CONTINUE : ARECORD_ABORTED;
      }

      case NESTED_EXIT:
          JS_NOT_REACHED("NESTED_EXIT should be replaced by innermost side exit");
      default:
        debug_only_printf(LC_TMTracer, "exit_type=%s\n", getExitName(lr->exitType));
        AbortRecording(cx, "Inner tree not suitable for calling");
        return ARECORD_ABORTED;
    }
}

static inline bool
IsEntryTypeCompatible(const Value &v, JSValueType type)
{
#ifdef DEBUG
    char tag = ValueToTypeChar(v);
    debug_only_printf(LC_TMTracer, "%c/%c ", tag, TypeToChar(type));
#endif

    JS_ASSERT(type <= JSVAL_UPPER_INCL_TYPE_OF_BOXABLE_SET);
    switch (type) {
      case JSVAL_TYPE_DOUBLE:
        if (v.isNumber())
            return true;
        debug_only_printf(LC_TMTracer, "double != tag%c ", tag);
        break;
      case JSVAL_TYPE_INT32: {
        int32_t _;
        if (v.isInt32() || (v.isDouble() && JSDOUBLE_IS_INT32(v.toDouble(), &_)))
            return true;
        debug_only_printf(LC_TMTracer, "int != tag%c ", tag);
        break;
      }
      case JSVAL_TYPE_UNDEFINED:
        if (v.isUndefined())
            return true;
        debug_only_printf(LC_TMTracer, "undefined != tag%c ", tag);
        break;
      case JSVAL_TYPE_BOOLEAN:
        if (v.isBoolean())
            return true;
        debug_only_printf(LC_TMTracer, "bool != tag%c ", tag);
        break;
      case JSVAL_TYPE_MAGIC:
        if (v.isMagic())
            return true;
        debug_only_printf(LC_TMTracer, "magic != tag%c ", tag);
        break;
      case JSVAL_TYPE_STRING:
        if (v.isString())
            return true;
        debug_only_printf(LC_TMTracer, "string != tag%c ", tag);
        break;
      case JSVAL_TYPE_NULL:
        if (v.isNull())
            return true;
        debug_only_printf(LC_TMTracer, "null != tag%c ", tag);
        break;
      case JSVAL_TYPE_OBJECT:
        JS_NOT_REACHED("JSVAL_TYPE_OBJECT does not belong in a type map");
        break;
      case JSVAL_TYPE_NONFUNOBJ:
        if (v.isObject() && !v.toObject().isFunction())
            return true;
        debug_only_printf(LC_TMTracer, "object != tag%c ", tag);
        break;
      case JSVAL_TYPE_FUNOBJ:
        if (v.isObject() && v.toObject().isFunction())
            return true;
        debug_only_printf(LC_TMTracer, "fun != tag%c ", tag);
        break;
      default:
        JS_NOT_REACHED("unexpected type");
    }
    return false;
}

class TypeCompatibilityVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    JSContext *mCx;
    Oracle *mOracle;
    JSValueType *mTypeMap;
    unsigned mStackSlotNum;
    bool mOk;
public:
    TypeCompatibilityVisitor (TraceRecorder &recorder,
                              JSValueType *typeMap) :
        mRecorder(recorder),
        mCx(mRecorder.cx),
        mOracle(JS_TRACE_MONITOR(mCx).oracle),
        mTypeMap(typeMap),
        mStackSlotNum(0),
        mOk(true)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot) {
        debug_only_printf(LC_TMTracer, "global%d=", n);
        if (!IsEntryTypeCompatible(*vp, *mTypeMap)) {
            mOk = false;
        } else if (!isPromoteInt(mRecorder.get(vp)) && *mTypeMap == JSVAL_TYPE_INT32) {
            mOracle->markGlobalSlotUndemotable(mCx, slot);
            mOk = false;
        } else if (vp->isInt32() && *mTypeMap == JSVAL_TYPE_DOUBLE) {
            mOracle->markGlobalSlotUndemotable(mCx, slot);
        }
        mTypeMap++;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), unsigned(i));
            if (!IsEntryTypeCompatible(*vp, *mTypeMap)) {
                mOk = false;
            } else if (!isPromoteInt(mRecorder.get(vp)) && *mTypeMap == JSVAL_TYPE_INT32) {
                mOracle->markStackSlotUndemotable(mCx, mStackSlotNum);
                mOk = false;
            } else if (vp->isInt32() && *mTypeMap == JSVAL_TYPE_DOUBLE) {
                mOracle->markStackSlotUndemotable(mCx, mStackSlotNum);
            }
            vp++;
            mTypeMap++;
            mStackSlotNum++;
        }
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame* fp) {
        debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), 0);
        if (!IsEntryTypeCompatible(ObjectOrNullValue(*p), *mTypeMap))
            mOk = false;
        mTypeMap++;
        mStackSlotNum++;
        return true;
    }

    bool isOk() {
        return mOk;
    }
};

JS_REQUIRES_STACK TreeFragment*
TraceRecorder::findNestedCompatiblePeer(TreeFragment* f)
{
    TraceMonitor* tm;

    tm = &JS_TRACE_MONITOR(cx);
    unsigned int ngslots = tree->globalSlots->length();

    for (; f != NULL; f = f->peer) {
        if (!f->code())
            continue;

        debug_only_printf(LC_TMTracer, "checking nested types %p: ", (void*)f);

        if (ngslots > f->nGlobalTypes())
            SpecializeTreesToMissingGlobals(cx, globalObj, f);

        








        TypeCompatibilityVisitor visitor(*this, f->typeMap.data());
        VisitSlots(visitor, cx, 0, *tree->globalSlots);

        debug_only_printf(LC_TMTracer, " %s\n", visitor.isOk() ? "match" : "");
        if (visitor.isOk())
            return f;
    }

    return NULL;
}

class CheckEntryTypeVisitor : public SlotVisitorBase
{
    bool mOk;
    JSValueType *mTypeMap;
public:
    CheckEntryTypeVisitor(JSValueType *typeMap) :
        mOk(true),
        mTypeMap(typeMap)
    {}

    JS_ALWAYS_INLINE void checkSlot(const Value &v, char const *name, int i) {
        debug_only_printf(LC_TMTracer, "%s%d=", name, i);
        JS_ASSERT(*(uint8_t*)mTypeMap != 0xCD);
        mOk = IsEntryTypeCompatible(v, *mTypeMap++);
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot) {
        if (mOk)
            checkSlot(*vp, "global", n);
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, JSStackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            if (!mOk)
                break;
            checkSlot(*vp++, stackSlotKind(), i);
        }
        return mOk;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(JSObject **p, JSStackFrame *fp) {
        checkSlot(ObjectOrNullValue(*p), stackSlotKind(), 0);
        return mOk;
    }

    bool isOk() {
        return mOk;
    }
};








static JS_REQUIRES_STACK bool
CheckEntryTypes(JSContext* cx, JSObject* globalObj, TreeFragment* f)
{
    unsigned int ngslots = f->globalSlots->length();

    JS_ASSERT(f->nStackTypes == NativeStackSlots(cx, 0));

    if (ngslots > f->nGlobalTypes())
        SpecializeTreesToMissingGlobals(cx, globalObj, f);

    JS_ASSERT(f->typeMap.length() == NativeStackSlots(cx, 0) + ngslots);
    JS_ASSERT(f->typeMap.length() == f->nStackTypes + ngslots);
    JS_ASSERT(f->nGlobalTypes() == ngslots);

    CheckEntryTypeVisitor visitor(f->typeMap.data());
    VisitSlots(visitor, cx, 0, *f->globalSlots);

    debug_only_print0(LC_TMTracer, "\n");
    return visitor.isOk();
}










static JS_REQUIRES_STACK TreeFragment*
FindVMCompatiblePeer(JSContext* cx, JSObject* globalObj, TreeFragment* f, uintN& count)
{
    count = 0;
    for (; f != NULL; f = f->peer) {
        if (!f->code())
            continue;
        debug_only_printf(LC_TMTracer,
                          "checking vm types %p (ip: %p): ", (void*)f, f->ip);
        if (CheckEntryTypes(cx, globalObj, f))
            return f;
        ++count;
    }
    return NULL;
}







JS_ALWAYS_INLINE
TracerState::TracerState(JSContext* cx, TraceMonitor* tm, TreeFragment* f,
                         uintN& inlineCallCount, VMSideExit** innermostNestedGuardp)
  : cx(cx),
    stackBase(tm->storage->stack()),
    sp(stackBase + f->nativeStackBase / sizeof(double)),
    eos(tm->storage->global()),
    callstackBase(tm->storage->callstack()),
    sor(callstackBase),
    rp(callstackBase),
    eor(callstackBase + JS_MIN(MAX_CALL_STACK_ENTRIES,
                               JS_MAX_INLINE_CALL_COUNT - inlineCallCount)),
    lastTreeExitGuard(NULL),
    lastTreeCallGuard(NULL),
    rpAtLastTreeCall(NULL),
    outermostTree(f),
    inlineCallCountp(&inlineCallCount),
    innermostNestedGuardp(innermostNestedGuardp),
#ifdef EXECUTE_TREE_TIMER
    startTime(rdtsc()),
#endif
    builtinStatus(0),
    nativeVp(NULL),
    bailedSlowNativeRegs(bailedSlowNativeRegs)
{
    JS_ASSERT(!tm->tracecx);
    tm->tracecx = cx;
    prev = cx->tracerState;
    cx->tracerState = this;

    JS_ASSERT(eos == stackBase + MAX_NATIVE_STACK_SLOTS);
    JS_ASSERT(sp < eos);

    




    JS_ASSERT(inlineCallCount <= JS_MAX_INLINE_CALL_COUNT);

#ifdef DEBUG
    



    memset(tm->storage->stack(), 0xCD, MAX_NATIVE_STACK_SLOTS * sizeof(double));
    memset(tm->storage->callstack(), 0xCD, MAX_CALL_STACK_ENTRIES * sizeof(FrameInfo*));
#endif
}

JS_ALWAYS_INLINE
TracerState::~TracerState()
{
    JS_ASSERT(!nativeVp);

    cx->tracerState = prev;
    JS_TRACE_MONITOR(cx).tracecx = NULL;
}


static JS_ALWAYS_INLINE VMSideExit*
ExecuteTrace(JSContext* cx, Fragment* f, TracerState& state)
{
    JS_ASSERT(!cx->bailExit);
    union { NIns *code; GuardRecord* (FASTCALL *func)(TracerState*); } u;
    u.code = f->code();
    GuardRecord* rec;
#if defined(JS_NO_FASTCALL) && defined(NANOJIT_IA32)
    SIMULATE_FASTCALL(rec, state, NULL, u.func);
#else
    rec = u.func(&state);
#endif
    JS_ASSERT(!cx->bailExit);
    return (VMSideExit*)rec->exit;
}


static JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
ScopeChainCheck(JSContext* cx, TreeFragment* f)
{
    JS_ASSERT(f->globalObj == cx->fp()->getScopeChain()->getGlobal());

    















    JSObject* child = cx->fp()->getScopeChain();
    while (JSObject* parent = child->getParent()) {
        if (!js_IsCacheableNonGlobalScope(child)) {
            debug_only_print0(LC_TMTracer,"Blacklist: non-cacheable object on scope chain.\n");
            Blacklist((jsbytecode*) f->root->ip);
            return false;
        }
        child = parent;
    }
    JS_ASSERT(child == f->globalObj);

    if (!(f->globalObj->getClass()->flags & JSCLASS_IS_GLOBAL)) {
        debug_only_print0(LC_TMTracer, "Blacklist: non-global at root of scope chain.\n");
        Blacklist((jsbytecode*) f->root->ip);
        return false;
    }

    return true;
}

static void
LeaveTree(TraceMonitor *tm, TracerState&, VMSideExit* lr);


static JS_REQUIRES_STACK bool
ExecuteTree(JSContext* cx, TreeFragment* f, uintN& inlineCallCount,
            VMSideExit** innermostNestedGuardp, VMSideExit **lrp)
{
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_EXECUTE);
#endif
    JS_ASSERT(f->root == f && f->code());
    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    if (!ScopeChainCheck(cx, f) || !cx->stack().ensureEnoughSpaceToEnterTrace() ||
        inlineCallCount + f->maxCallDepth > JS_MAX_INLINE_CALL_COUNT) {
        *lrp = NULL;
        return true;
    }

    
    JS_ASSERT(f->globalObj->numSlots() <= MAX_GLOBAL_SLOTS);
    JS_ASSERT(f->nGlobalTypes() == f->globalSlots->length());
    JS_ASSERT_IF(f->globalSlots->length() != 0,
                 f->globalObj->shape() == f->globalShape);

    
    TracerState state(cx, tm, f, inlineCallCount, innermostNestedGuardp);
    double* stack = tm->storage->stack();
    double* global = tm->storage->global();
    JSObject* globalObj = f->globalObj;
    unsigned ngslots = f->globalSlots->length();
    uint16* gslots = f->globalSlots->data();

    BuildNativeFrame(cx, globalObj, 0 , ngslots, gslots,
                     f->typeMap.data(), global, stack);

    AUDIT(traceTriggered);
    debug_only_printf(LC_TMTracer, "entering trace at %s:%u@%u, execs: %u code: %p\n",
                      cx->fp()->getScript()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()),
           f->execs,
           f->code());

    debug_only_stmt(uint32 globalSlots = globalObj->numSlots();)
    debug_only_stmt(*(uint64*)&tm->storage->global()[globalSlots] = 0xdeadbeefdeadbeefLL;)

    
#ifdef MOZ_TRACEVIS
    VMSideExit* lr = (TraceVisStateObj(cx, S_NATIVE), ExecuteTrace(cx, f, state));
#else
    VMSideExit* lr = ExecuteTrace(cx, f, state);
#endif

    JS_ASSERT(*(uint64*)&tm->storage->global()[globalSlots] == 0xdeadbeefdeadbeefLL);
    JS_ASSERT_IF(lr->exitType == LOOP_EXIT, !lr->calldepth);

    
    LeaveTree(tm, state, lr);

    *lrp = state.innermost;
    bool ok = !(state.builtinStatus & BUILTIN_ERROR);
    JS_ASSERT_IF(cx->throwing, !ok);
    return ok;
}

class Guardian {
    bool *flagp;
public:
    Guardian(bool *flagp) {
        this->flagp = flagp;
        JS_ASSERT(!*flagp);
        *flagp = true;
    }

    ~Guardian() {
        JS_ASSERT(*flagp);
        *flagp = false;
    }
};

static JS_FORCES_STACK void
LeaveTree(TraceMonitor *tm, TracerState& state, VMSideExit* lr)
{
    VOUCH_DOES_NOT_REQUIRE_STACK();

    JSContext* cx = state.cx;

    
    Guardian waiver(&JS_THREAD_DATA(cx)->waiveGCQuota);

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
    bool bailed = innermost->exitType == STATUS_EXIT && (bs & BUILTIN_BAILED);
    if (bailed) {
        








        if (!cx->fp()->hasScript()) {
            JS_ASSERT(cx->regs == &state.bailedSlowNativeRegs);
            cx->stack().popSynthesizedSlowNativeFrame(cx);
        }
        JS_ASSERT(cx->fp()->hasScript());

        if (!(bs & BUILTIN_ERROR)) {
            











            JSFrameRegs* regs = cx->regs;
            JSOp op = (JSOp) *regs->pc;
            JS_ASSERT(op == JSOP_CALL || op == JSOP_APPLY || op == JSOP_NEW ||
                      op == JSOP_GETPROP || op == JSOP_GETTHISPROP || op == JSOP_GETARGPROP ||
                      op == JSOP_GETLOCALPROP || op == JSOP_LENGTH ||
                      op == JSOP_GETELEM || op == JSOP_CALLELEM || op == JSOP_CALLPROP ||
                      op == JSOP_SETPROP || op == JSOP_SETNAME || op == JSOP_SETMETHOD ||
                      op == JSOP_SETELEM || op == JSOP_INITELEM || op == JSOP_ENUMELEM ||
                      op == JSOP_INSTANCEOF ||
                      op == JSOP_ITER || op == JSOP_MOREITER || op == JSOP_ENDITER ||
                      op == JSOP_FORARG || op == JSOP_FORLOCAL ||
                      op == JSOP_FORNAME || op == JSOP_FORPROP || op == JSOP_FORELEM ||
                      op == JSOP_DELPROP || op == JSOP_DELELEM);

            




            if (op == JSOP_SETELEM && JSOp(regs->pc[JSOP_SETELEM_LENGTH]) == JSOP_POP) {
                regs->sp -= js_CodeSpec[JSOP_SETELEM].nuses;
                regs->sp += js_CodeSpec[JSOP_SETELEM].ndefs;
                regs->pc += JSOP_SETELEM_LENGTH;
                op = JSOP_POP;
            }

            const JSCodeSpec& cs = js_CodeSpec[op];
            regs->sp -= (cs.format & JOF_INVOKE) ? GET_ARGC(regs->pc) + 2 : cs.nuses;
            regs->sp += cs.ndefs;
            regs->pc += cs.length;
            JS_ASSERT_IF(!cx->fp()->hasIMacroPC(),
                         cx->fp()->slots() + cx->fp()->getFixedCount() +
                         js_ReconstructStackDepth(cx, cx->fp()->getScript(), regs->pc) ==
                         regs->sp);

            





            JS_ASSERT(state.deepBailSp >= state.stackBase && state.sp <= state.deepBailSp);

            





            JSValueType* typeMap = innermost->stackTypeMap();
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

    while (callstack < rp) {
        FrameInfo* fi = *callstack;
        
        JSObject* callee = *(JSObject**)&stack[fi->callerHeight];

        



        SynthesizeFrame(cx, *fi, callee);
        int slots = FlushNativeStackFrame(cx, 1 , (*callstack)->get_typemap(),
                                          stack, cx->fp());
#ifdef DEBUG
        JSStackFrame* fp = cx->fp();
        debug_only_printf(LC_TMTracer,
                          "synthesized deep frame for %s:%u@%u, slots=%d, fi=%p\n",
                          fp->getScript()->filename,
                          js_FramePCToLineNumber(cx, fp),
                          FramePCOffset(cx, fp),
                          slots,
                          (void*)*callstack);
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
        JSStackFrame* fp = cx->fp();
        debug_only_printf(LC_TMTracer,
                          "synthesized shallow frame for %s:%u@%u\n",
                          fp->getScript()->filename, js_FramePCToLineNumber(cx, fp),
                          FramePCOffset(cx, fp));
#endif
    }

    






    JSStackFrame* const fp = cx->fp();

    fp->setBlockChain(innermost->block);

    



    cx->regs->pc = innermost->pc;
    if (innermost->imacpc)
        fp->setIMacroPC(innermost->imacpc);
    else
        fp->clearIMacroPC();
    cx->regs->sp = fp->base() + (innermost->sp_adj / sizeof(double)) - calldepth_slots;
    JS_ASSERT_IF(!fp->hasIMacroPC(),
                 fp->slots() + fp->getFixedCount() +
                 js_ReconstructStackDepth(cx, fp->getScript(), cx->regs->pc) == cx->regs->sp);

#ifdef EXECUTE_TREE_TIMER
    uint64 cycles = rdtsc() - state.startTime;
#elif defined(JS_JIT_SPEW)
    uint64 cycles = 0;
#endif

    debug_only_printf(LC_TMTracer,
                      "leaving trace at %s:%u@%u, op=%s, lr=%p, exitType=%s, sp=%lld, "
                      "calldepth=%d, cycles=%llu\n",
                      fp->getScript()->filename,
                      js_FramePCToLineNumber(cx, fp),
                      FramePCOffset(cx, fp),
                      js_CodeName[fp->hasIMacroPC() ? *fp->getIMacroPC() : *cx->regs->pc],
                      (void*)lr,
                      getExitName(lr->exitType),
                      (long long int)(cx->regs->sp - fp->base()),
                      calldepth,
                      (unsigned long long int)cycles);

    






    TreeFragment* outermostTree = state.outermostTree;
    uint16* gslots = outermostTree->globalSlots->data();
    unsigned ngslots = outermostTree->globalSlots->length();
    JS_ASSERT(ngslots == outermostTree->nGlobalTypes());
    JSValueType* globalTypeMap;

    
    TypeMap& typeMap = *tm->cachedTempTypeMap;
    typeMap.clear();
    if (innermost->numGlobalSlots == ngslots) {
        
        globalTypeMap = innermost->globalTypeMap();
    } else {
        






        JS_ASSERT(innermost->root()->nGlobalTypes() == ngslots);
        JS_ASSERT(innermost->root()->nGlobalTypes() > innermost->numGlobalSlots);
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
        SynthesizeSlowNativeFrame(state, cx, innermost);

    
    JS_ASSERT(state.eos == state.stackBase + MAX_NATIVE_STACK_SLOTS);
    JSObject* globalObj = outermostTree->globalObj;
    FlushNativeGlobalFrame(cx, globalObj, state.eos, ngslots, gslots, globalTypeMap);
#ifdef DEBUG
    
    for (JSStackFrame* fp = cx->fp(); fp; fp = fp->down) {
        JS_ASSERT_IF(fp->argv, fp->argv[-1].isObjectOrNull());
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

static bool
ApplyBlacklistHeuristics(JSContext *cx, TreeFragment *tree)
{
    if (tree->execs >= MAX_LOOP_EXECS) {
        debug_only_printf(LC_TMTracer, "tree %p executed %d times, blacklisting\n",
                          (void*)tree, tree->execs);
        Blacklist((jsbytecode *)tree->ip);
        return false;
    }
    tree->execs++;
    return true;
}

JS_REQUIRES_STACK MonitorResult
MonitorLoopEdge(JSContext* cx, uintN& inlineCallCount)
{
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_MONITOR);
#endif

    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);

    
    if (tm->recorder) {
        jsbytecode* pc = cx->regs->pc;
        if (pc == tm->recorder->tree->ip) {
            tm->recorder->closeLoop();
        } else {
            MonitorResult r = TraceRecorder::recordLoopEdge(cx, tm->recorder, inlineCallCount);
            JS_ASSERT((r == MONITOR_RECORDING) == (TRACE_RECORDER(cx) != NULL));
            if (r == MONITOR_RECORDING || r == MONITOR_ERROR)
                return r;


            










            if (pc != cx->regs->pc) {
#ifdef MOZ_TRACEVIS
                tvso.r = R_INNER_SIDE_EXIT;
#endif
                return MONITOR_NOT_RECORDING;
            }
        }
    }
    JS_ASSERT(!tm->recorder);

    



    JSObject* globalObj = cx->fp()->getScopeChain()->getGlobal();
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;

    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots)) {
        Backoff(cx, cx->regs->pc);
        return MONITOR_NOT_RECORDING;
    }

    
    if (JS_THREAD_DATA(cx)->interruptFlags) {
#ifdef MOZ_TRACEVIS
        tvso.r = R_CALLBACK_PENDING;
#endif
        return MONITOR_NOT_RECORDING;
    }

    jsbytecode* pc = cx->regs->pc;
    uint32 argc = numActualArgs(cx->fp());

    TreeFragment* f = LookupOrAddLoop(tm, pc, globalObj, globalShape, argc);

    



    if (!f->code() && !f->peer) {
    record:
        if (++f->hits() < HOTLOOP) {
#ifdef MOZ_TRACEVIS
            tvso.r = f->hits() < 1 ? R_BACKED_OFF : R_COLD;
#endif
            return MONITOR_NOT_RECORDING;
        }

        if (!ScopeChainCheck(cx, f)) {
#ifdef MOZ_TRACEVIS
            tvso.r = R_FAIL_SCOPE_CHAIN_CHECK;
#endif
            return MONITOR_NOT_RECORDING;
        }

        




        bool rv = RecordTree(cx, f->first, NULL, 0, globalSlots);
#ifdef MOZ_TRACEVIS
        if (!rv)
            tvso.r = R_FAIL_RECORD_TREE;
#endif
        return RecordingIfTrue(rv);
    }

    debug_only_printf(LC_TMTracer,
                      "Looking for compat peer %d@%d, from %p (ip: %p)\n",
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()), (void*)f, f->ip);

    uintN count;
    TreeFragment* match = FindVMCompatiblePeer(cx, globalObj, f, count);
    if (!match) {
        if (count < MAXPEERS)
            goto record;

        



        debug_only_print0(LC_TMTracer, "Blacklisted: too many peer trees.\n");
        Blacklist((jsbytecode*) f->root->ip);
#ifdef MOZ_TRACEVIS
        tvso.r = R_MAX_PEERS;
#endif
        return MONITOR_NOT_RECORDING;
    }

    VMSideExit* lr = NULL;
    VMSideExit* innermostNestedGuard = NULL;

    if (!ApplyBlacklistHeuristics(cx, match))
        return MONITOR_NOT_RECORDING;

    if (!ExecuteTree(cx, match, inlineCallCount, &innermostNestedGuard, &lr))
        return MONITOR_ERROR;

    if (!lr) {
#ifdef MOZ_TRACEVIS
        tvso.r = R_FAIL_EXECUTE_TREE;
#endif
        return MONITOR_NOT_RECORDING;
    }

    




    bool rv;
    switch (lr->exitType) {
      case UNSTABLE_LOOP_EXIT:
          rv = AttemptToStabilizeTree(cx, globalObj, lr, NULL, 0);
#ifdef MOZ_TRACEVIS
          if (!rv)
              tvso.r = R_FAIL_STABILIZE;
#endif
          return RecordingIfTrue(rv);

      case OVERFLOW_EXIT:
          tm->oracle->markInstructionUndemotable(cx->regs->pc);
        
      case BRANCH_EXIT:
      case CASE_EXIT:
        rv = AttemptToExtendTree(cx, lr, NULL, NULL
#ifdef MOZ_TRACEVIS
                                                   , &tvso
#endif
                                 );
        return RecordingIfTrue(rv);

      case LOOP_EXIT:
        if (innermostNestedGuard) {
            rv = AttemptToExtendTree(cx, innermostNestedGuard, lr, NULL
#ifdef MOZ_TRACEVIS
                                                                       , &tvso
#endif
                                     );
            return RecordingIfTrue(rv);
        }
#ifdef MOZ_TRACEVIS
        tvso.r = R_NO_EXTEND_OUTER;
#endif
        return MONITOR_NOT_RECORDING;

#ifdef MOZ_TRACEVIS
      case MISMATCH_EXIT:
        tvso.r = R_MISMATCH_EXIT;
        return MONITOR_NOT_RECORDING;
      case OOM_EXIT:
        tvso.r = R_OOM_EXIT;
        return MONITOR_NOT_RECORDING;
      case TIMEOUT_EXIT:
        tvso.r = R_TIMEOUT_EXIT;
        return MONITOR_NOT_RECORDING;
      case DEEP_BAIL_EXIT:
        tvso.r = R_DEEP_BAIL_EXIT;
        return MONITOR_NOT_RECORDING;
      case STATUS_EXIT:
        tvso.r = R_STATUS_EXIT;
        return MONITOR_NOT_RECORDING;
#endif

      default:
        



#ifdef MOZ_TRACEVIS
        tvso.r = R_OTHER_EXIT;
#endif
        return MONITOR_NOT_RECORDING;
    }
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::monitorRecording(JSOp op)
{
    TraceMonitor &localtm = JS_TRACE_MONITOR(cx);
    debug_only_stmt( JSContext *localcx = cx; )

    
    if (localtm.needFlush) {
        ResetJIT(cx, FR_DEEP_BAIL);
        return ARECORD_ABORTED;
    }
    JS_ASSERT(!fragment->lastIns);

    



    pendingSpecializedNative = NULL;
    newobj_ins = NULL;

    
    if (pendingGuardCondition) {
        guard(true, pendingGuardCondition, STATUS_EXIT);
        pendingGuardCondition = NULL;
    }

    
    if (pendingUnboxSlot) {
        LIns* val_ins = get(pendingUnboxSlot);
        




        LIns* unboxed_ins = unbox_value(*pendingUnboxSlot,
                                        val_ins->oprnd1(), val_ins->disp(),
                                        snapshot(BRANCH_EXIT));
        set(pendingUnboxSlot, unboxed_ins);
        pendingUnboxSlot = 0;
    }

    debug_only_stmt(
        if (LogController.lcbits & LC_TMRecorder) {
            js_Disassemble1(cx, cx->fp()->getScript(), cx->regs->pc,
                            cx->fp()->hasIMacroPC()
                                ? 0 : cx->regs->pc - cx->fp()->getScript()->code,
                            !cx->fp()->hasIMacroPC(), stdout);
        }
    )

    






    AbortableRecordingStatus status;
#ifdef DEBUG
    bool wasInImacro = (cx->fp()->hasIMacroPC());
#endif
    switch (op) {
      default:
          AbortRecording(cx, "unsupported opcode");
          status = ARECORD_ERROR;
          break;
# define OPDEF(x,val,name,token,length,nuses,ndefs,prec,format)               \
      case x:                                                                 \
        status = this->record_##x();                                          \
        break;
# include "jsopcode.tbl"
# undef OPDEF
    }

    

    if (!JSOP_IS_IMACOP(op)) {
        JS_ASSERT(status != ARECORD_IMACRO);
        JS_ASSERT_IF(!wasInImacro, !localcx->fp()->hasIMacroPC());
    }

    if (localtm.recorder) {
        JS_ASSERT(status != ARECORD_ABORTED);
        JS_ASSERT(localtm.recorder == this);

        
        if (status == ARECORD_COMPLETED)
            return ARECORD_CONTINUE;

        
        if (StatusAbortsRecorderIfActive(status)) {
            AbortRecording(cx, js_CodeName[op]);
            return status == ARECORD_ERROR ? ARECORD_ERROR : ARECORD_ABORTED;
        }

        if (outOfMemory() || OverfullJITCache(&localtm)) {
            ResetJIT(cx, FR_OOM);

            




            return status == ARECORD_IMACRO ? ARECORD_IMACRO_ABORTED : ARECORD_ABORTED;
        }
    } else {
        JS_ASSERT(status == ARECORD_COMPLETED ||
                  status == ARECORD_ABORTED ||
                  status == ARECORD_ERROR);
    }
    return status;
}

JS_REQUIRES_STACK void
AbortRecording(JSContext* cx, const char* reason)
{
#ifdef DEBUG
    JS_ASSERT(TRACE_RECORDER(cx));
    TRACE_RECORDER(cx)->finishAbort(reason);
#else
    TRACE_RECORDER(cx)->finishAbort("[no reason]");
#endif
}

#if defined NANOJIT_IA32
static bool
CheckForSSE2()
{
    char *c = getenv("X86_FORCE_SSE2");
    if (c)
        return (!strcmp(c, "true") ||
                !strcmp(c, "1") ||
                !strcmp(c, "yes"));

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


extern "C" int js_arm_try_armv5_op();
extern "C" int js_arm_try_armv6_op();
extern "C" int js_arm_try_armv7_op();
extern "C" int js_arm_try_vfp_op();

static unsigned int
arm_check_arch()
{
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
arm_check_vfp()
{
#ifdef WINCE_WINDOWS_MOBILE
    return false;
#else
    bool ret = false;
    __try {
        js_arm_try_vfp_op();
        ret = true;
    } __except(GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
        ret = false;
    }
    return ret;
#endif
}

#define HAVE_ENABLE_DISABLE_DEBUGGER_EXCEPTIONS 1




static void
disable_debugger_exceptions()
{
    
    DWORD kctrl = (DWORD) TlsGetValue(2);
    
    kctrl |= 0x12;
    TlsSetValue(2, (LPVOID) kctrl);
}

static void
enable_debugger_exceptions()
{
    
    DWORD kctrl = (DWORD) TlsGetValue(2);
    
    kctrl &= ~0x12;
    TlsSetValue(2, (LPVOID) kctrl);
}

#elif defined(__GNUC__) && defined(AVMPLUS_LINUX)


static unsigned int arm_arch = 4;
static bool arm_has_vfp = false;
static bool arm_has_neon = false;
static bool arm_has_iwmmxt = false;
static bool arm_tests_initialized = false;

#ifdef ANDROID

static void
arm_read_auxv()
{
  char buf[1024];
  char* pos;
  const char* ver_token = "CPU architecture: ";
  FILE* f = fopen("/proc/cpuinfo", "r");
  fread(buf, sizeof(char), 1024, f);
  fclose(f);
  pos = strstr(buf, ver_token);
  if (pos) {
    int ver = *(pos + strlen(ver_token)) - '0';
    arm_arch = ver;
  }
  arm_has_neon = strstr(buf, "neon") != NULL;
  arm_has_vfp = strstr(buf, "vfp") != NULL;
  arm_has_iwmmxt = strstr(buf, "iwmmxt") != NULL;
  arm_tests_initialized = true;
}

#else

static void
arm_read_auxv()
{
    int fd;
    Elf32_auxv_t aux;

    fd = open("/proc/self/auxv", O_RDONLY);
    if (fd > 0) {
        while (read(fd, &aux, sizeof(Elf32_auxv_t))) {
            if (aux.a_type == AT_HWCAP) {
                uint32_t hwcap = aux.a_un.a_val;
                if (getenv("ARM_FORCE_HWCAP"))
                    hwcap = strtoul(getenv("ARM_FORCE_HWCAP"), NULL, 0);
                else if (getenv("_SBOX_DIR"))
                    continue;  
                
                
                arm_has_vfp = (hwcap & 64) != 0;
                arm_has_iwmmxt = (hwcap & 512) != 0;
                
                arm_has_neon = (hwcap & 4096) != 0;
            } else if (aux.a_type == AT_PLATFORM) {
                const char *plat = (const char*) aux.a_un.a_val;
                if (getenv("ARM_FORCE_PLATFORM"))
                    plat = getenv("ARM_FORCE_PLATFORM");
                else if (getenv("_SBOX_DIR"))
                    continue;  
                
                
                
                
                
                if ((plat[0] == 'v') &&
                    (plat[1] >= '4') && (plat[1] <= '9') &&
                    ((plat[2] == 'l') || (plat[2] == 'b')))
                {
                    arm_arch = plat[1] - '0';
                }
            }
        }
        close (fd);

        
        
        if (!getenv("ARM_TRUST_HWCAP") && (arm_arch >= 7))
            arm_has_neon = true;
    }

    arm_tests_initialized = true;
}

#endif

static unsigned int
arm_check_arch()
{
    if (!arm_tests_initialized)
        arm_read_auxv();

    return arm_arch;
}

static bool
arm_check_vfp()
{
    if (!arm_tests_initialized)
        arm_read_auxv();

    return arm_has_vfp;
}

#else
#warning Not sure how to check for architecture variant on your platform. Assuming ARMv4.
static unsigned int
arm_check_arch() { return 4; }
static bool
arm_check_vfp() { return false; }
#endif

#ifndef HAVE_ENABLE_DISABLE_DEBUGGER_EXCEPTIONS
static void
enable_debugger_exceptions() { }
static void
disable_debugger_exceptions() { }
#endif

#endif 

#define K *1024
#define M K K
#define G K M

void
SetMaxCodeCacheBytes(JSContext* cx, uint32 bytes)
{
    TraceMonitor* tm = &JS_THREAD_DATA(cx)->traceMonitor;
    JS_ASSERT(tm->codeAlloc && tm->dataAlloc && tm->traceAlloc);
    if (bytes > 1 G)
        bytes = 1 G;
    if (bytes < 128 K)
        bytes = 128 K;
    tm->maxCodeCacheBytes = bytes;
}

void
InitJIT(TraceMonitor *tm)
{
#if defined JS_JIT_SPEW
    tm->profAlloc = NULL;
    
    if (!did_we_set_up_debug_logging) {
        InitJITLogController();
        did_we_set_up_debug_logging = true;
    }
    
    if (LogController.lcbits & LC_FragProfile) {
        tm->profAlloc = new VMAllocator();
        tm->profTab = new (*tm->profAlloc) FragStatsMap(*tm->profAlloc);
    }
    tm->lastFragID = 0;
#else
    PodZero(&LogController);
#endif

    if (!did_we_check_processor_features) {
#if defined NANOJIT_IA32
        avmplus::AvmCore::config.i386_use_cmov =
            avmplus::AvmCore::config.i386_sse2 = CheckForSSE2();
        avmplus::AvmCore::config.i386_fixed_esp = true;
#endif
#if defined NANOJIT_ARM

        disable_debugger_exceptions();

        bool            arm_vfp     = arm_check_vfp();
        unsigned int    arm_arch    = arm_check_arch();

        enable_debugger_exceptions();

        avmplus::AvmCore::config.arm_vfp        = arm_vfp;
        avmplus::AvmCore::config.soft_float     = !arm_vfp;
        avmplus::AvmCore::config.arm_arch       = arm_arch;

        
        
        JS_ASSERT(arm_arch >= 4);
#endif
        did_we_check_processor_features = true;
    }

    
    tm->maxCodeCacheBytes = 16 M;

    tm->oracle = new Oracle();

    tm->recordAttempts = new RecordAttemptMap;
    if (!tm->recordAttempts->init(PC_HASH_COUNT))
        abort();

    JS_ASSERT(!tm->dataAlloc && !tm->traceAlloc && !tm->codeAlloc);
    tm->dataAlloc = new VMAllocator();
    tm->traceAlloc = new VMAllocator();
    tm->tempAlloc = new VMAllocator();
    tm->reTempAlloc = new VMAllocator();
    tm->codeAlloc = new CodeAlloc();
    tm->frameCache = new FrameInfoCache(tm->dataAlloc);
    tm->storage = new TraceNativeStorage();
    tm->cachedTempTypeMap = new TypeMap(0);
    tm->flush();
    verbose_only( tm->branches = NULL; )

#if !defined XP_WIN
    debug_only(PodZero(&jitstats));
#endif

#ifdef JS_JIT_SPEW
    
    jitstats.archIsIA32 = 0;
    jitstats.archIs64BIT = 0;
    jitstats.archIsARM = 0;
    jitstats.archIsSPARC = 0;
    jitstats.archIsPPC = 0;
#if defined NANOJIT_IA32
    jitstats.archIsIA32 = 1;
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
#if defined NANOJIT_X64
    jitstats.archIsAMD64 = 1;
#endif
#endif
}

void
FinishJIT(TraceMonitor *tm)
{
    JS_ASSERT(!tm->recorder);

#ifdef JS_JIT_SPEW
    if (jitstats.recorderStarted) {
        char sep = ':';
        debug_only_print0(LC_TMStats, "recorder");
#define RECORDER_JITSTAT(_ident, _name)                             \
        debug_only_printf(LC_TMStats, "%c " _name "(%llu)", sep,    \
                          (unsigned long long int)jitstats._ident); \
        sep = ',';
#define JITSTAT(x)
#include "jitstats.tbl"
#undef JITSTAT
#undef RECORDER_JITSTAT
        debug_only_print0(LC_TMStats, "\n");

        sep = ':';
        debug_only_print0(LC_TMStats, "monitor");
#define MONITOR_JITSTAT(_ident, _name)                              \
        debug_only_printf(LC_TMStats, "%c " _name "(%llu)", sep,    \
                          (unsigned long long int)jitstats._ident); \
        sep = ',';
#define JITSTAT(x)
#include "jitstats.tbl"
#undef JITSTAT
#undef MONITOR_JITSTAT
        debug_only_print0(LC_TMStats, "\n");
    }
#endif

    delete tm->recordAttempts;
    delete tm->oracle;

#ifdef DEBUG
    
    
    if (LogController.lcbits & LC_FragProfile) {
        for (Seq<Fragment*>* f = tm->branches; f; f = f->tail) {
            FragProfiling_FragFinalizer(f->head, tm);
        }
        for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
            for (TreeFragment *f = tm->vmfragments[i]; f; f = f->next) {
                JS_ASSERT(f->root == f);
                for (TreeFragment *p = f; p; p = p->peer)
                    FragProfiling_FragFinalizer(p, tm);
            }
        }
        REHashMap::Iter iter(*(tm->reFragments));
        while (iter.next()) {
            VMFragment* frag = (VMFragment*)iter.value();
            FragProfiling_FragFinalizer(frag, tm);
        }

        FragProfiling_showResults(tm);
        delete tm->profAlloc;

    } else {
        NanoAssert(!tm->profTab);
        NanoAssert(!tm->profAlloc);
    }
#endif

    PodArrayZero(tm->vmfragments);

    if (tm->frameCache) {
        delete tm->frameCache;
        tm->frameCache = NULL;
    }

    if (tm->codeAlloc) {
        delete tm->codeAlloc;
        tm->codeAlloc = NULL;
    }

    if (tm->dataAlloc) {
        delete tm->dataAlloc;
        tm->dataAlloc = NULL;
    }

    if (tm->traceAlloc) {
        delete tm->traceAlloc;
        tm->traceAlloc = NULL;
    }

    if (tm->tempAlloc) {
        delete tm->tempAlloc;
        tm->tempAlloc = NULL;
    }

    if (tm->reTempAlloc) {
        delete tm->reTempAlloc;
        tm->reTempAlloc = NULL;
    }

    if (tm->storage) {
        delete tm->storage;
        tm->storage = NULL;
    }

    delete tm->cachedTempTypeMap;
    tm->cachedTempTypeMap = NULL;
}

JS_REQUIRES_STACK void
PurgeScriptFragments(JSContext* cx, JSScript* script)
{
    if (!TRACING_ENABLED(cx))
        return;
    debug_only_printf(LC_TMTracer,
                      "Purging fragments for JSScript %p.\n", (void*)script);

    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
        TreeFragment** fragp = &tm->vmfragments[i];
        while (TreeFragment* frag = *fragp) {
            if (JS_UPTRDIFF(frag->ip, script->code) < script->length) {
                
                debug_only_printf(LC_TMTracer,
                                  "Disconnecting TreeFragment %p "
                                  "with ip %p, in range [%p,%p).\n",
                                  (void*)frag, frag->ip, script->code,
                                  script->code + script->length);

                JS_ASSERT(frag->root == frag);
                *fragp = frag->next;
                do {
                    verbose_only( FragProfiling_FragFinalizer(frag, tm); )
                    TrashTree(cx, frag);
                } while ((frag = frag->peer) != NULL);
                continue;
            }
            fragp = &frag->next;
        }
    }

    RecordAttemptMap &table = *tm->recordAttempts;
    for (RecordAttemptMap::Enum e(table); !e.empty(); e.popFront()) {
        if (JS_UPTRDIFF(e.front().key, script->code) < script->length)
            e.removeFront();
    }
}

bool
OverfullJITCache(TraceMonitor* tm)
{
    

































    jsuint maxsz = tm->maxCodeCacheBytes;
    VMAllocator *dataAlloc = tm->dataAlloc;
    VMAllocator *traceAlloc = tm->traceAlloc;
    CodeAlloc *codeAlloc = tm->codeAlloc;

    return (codeAlloc->size() + dataAlloc->size() + traceAlloc->size() > maxsz);
}

JS_FORCES_STACK JS_FRIEND_API(void)
DeepBail(JSContext *cx)
{
    JS_ASSERT(JS_ON_TRACE(cx));

    



    TraceMonitor *tm = &JS_TRACE_MONITOR(cx);
    JSContext *tracecx = tm->tracecx;

    
    JS_ASSERT(tracecx->bailExit);

    tm->tracecx = NULL;
    debug_only_print0(LC_TMTracer, "Deep bail.\n");
    LeaveTree(tm, *tracecx->tracerState, tracecx->bailExit);
    tracecx->bailExit = NULL;

    TracerState* state = tracecx->tracerState;
    state->builtinStatus |= BUILTIN_BAILED;

    








    state->deepBailSp = state->sp;
}

JS_REQUIRES_STACK Value&
TraceRecorder::argval(unsigned n) const
{
    JS_ASSERT(n < cx->fp()->numFormalArgs());
    return cx->fp()->argv[n];
}

JS_REQUIRES_STACK Value&
TraceRecorder::varval(unsigned n) const
{
    JS_ASSERT(n < cx->fp()->getSlotCount());
    return cx->fp()->slots()[n];
}

JS_REQUIRES_STACK Value&
TraceRecorder::stackval(int n) const
{
    return cx->regs->sp[n];
}

JS_REQUIRES_STACK void
TraceRecorder::updateAtoms()
{
    atoms = FrameAtomBase(cx, cx->fp());
    consts = cx->fp()->hasIMacroPC() || cx->fp()->getScript()->constOffset == 0
           ? 0 
           : cx->fp()->getScript()->consts()->vector;
}

JS_REQUIRES_STACK void
TraceRecorder::updateAtoms(JSScript *script)
{
    atoms = script->atomMap.vector;
    consts = script->constOffset == 0 ? 0 : script->consts()->vector;
}




JS_REQUIRES_STACK LIns*
TraceRecorder::scopeChain()
{
    return cx->fp()->callee()
           ? getFrameObjPtr(cx->fp()->addressScopeChain())
           : entryScopeChain();
}






JS_REQUIRES_STACK LIns*
TraceRecorder::entryScopeChain() const
{
    return lir->insLoad(LIR_ldp, entryFrameIns(),
                        JSStackFrame::offsetScopeChain(), ACCSET_OTHER);
}




JS_REQUIRES_STACK LIns*
TraceRecorder::entryFrameIns() const
{
    LIns *regs_ins = lir->insLoad(LIR_ldp, cx_ins, offsetof(JSContext, regs), ACCSET_OTHER);
    return lir->insLoad(LIR_ldp, regs_ins, offsetof(JSFrameRegs, fp), ACCSET_OTHER);
}






JS_REQUIRES_STACK JSStackFrame*
TraceRecorder::frameIfInRange(JSObject* obj, unsigned* depthp) const
{
    JSStackFrame* ofp = (JSStackFrame*) obj->getPrivate();
    JSStackFrame* fp = cx->fp();
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

JS_DEFINE_CALLINFO_4(extern, UINT32, GetClosureVar, CONTEXT, OBJECT, CVIPTR, DOUBLEPTR,
                     0, ACCSET_STORE_ANY)
JS_DEFINE_CALLINFO_4(extern, UINT32, GetClosureArg, CONTEXT, OBJECT, CVIPTR, DOUBLEPTR,
                     0, ACCSET_STORE_ANY)










JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::scopeChainProp(JSObject* chainHead, Value*& vp, LIns*& ins, NameResult& nr)
{
    JS_ASSERT(chainHead == cx->fp()->getScopeChain());
    JS_ASSERT(chainHead != globalObj);

    TraceMonitor &localtm = *traceMonitor;

    JSAtom* atom = atoms[GET_INDEX(cx->regs->pc)];
    JSObject* obj2;
    JSProperty* prop;
    JSObject *obj = chainHead;
    if (!js_FindProperty(cx, ATOM_TO_JSID(atom), &obj, &obj2, &prop))
        RETURN_ERROR_A("error in js_FindProperty");

    
    if (!localtm.recorder)
        return ARECORD_ABORTED;

    if (!prop)
        RETURN_STOP_A("failed to find name in non-global scope chain");

    if (obj == globalObj) {
        
        
        
        LIns *head_ins;
        if (cx->fp()->argv) {
            
            
            
            chainHead = cx->fp()->callee()->getParent();
            head_ins = stobj_get_parent(get(&cx->fp()->argv[-2]));
        } else {
            head_ins = scopeChain();
        }
        LIns *obj_ins;
        CHECK_STATUS_A(traverseScopeChain(chainHead, head_ins, obj, obj_ins));

        if (obj2 != obj) {
            obj2->dropProperty(cx, prop);
            RETURN_STOP_A("prototype property");
        }

        Shape* shape = (Shape*) prop;
        if (!isValidSlot(obj, shape)) {
            JS_UNLOCK_OBJ(cx, obj2);
            return ARECORD_STOP;
        }
        if (!lazilyImportGlobalSlot(shape->slot)) {
            JS_UNLOCK_OBJ(cx, obj2);
            RETURN_STOP_A("lazy import of global slot failed");
        }
        vp = &obj->getSlotRef(shape->slot);
        ins = get(vp);
        JS_UNLOCK_OBJ(cx, obj2);
        nr.tracked = true;
        return ARECORD_CONTINUE;
    }

    if (obj == obj2 && obj->isCall()) {
        AbortableRecordingStatus status =
            InjectStatus(callProp(obj, prop, ATOM_TO_JSID(atom), vp, ins, nr));
        JS_UNLOCK_OBJ(cx, obj);
        return status;
    }

    obj2->dropProperty(cx, prop);
    RETURN_STOP_A("fp->scopeChain is not global or active call object");
}




JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callProp(JSObject* obj, JSProperty* prop, jsid id, Value*& vp,
                        LIns*& ins, NameResult& nr)
{
    Shape *shape = (Shape*) prop;

    JSOp op = JSOp(*cx->regs->pc);
    uint32 setflags = (js_CodeSpec[op].format & (JOF_SET | JOF_INCDEC | JOF_FOR));
    if (setflags && !shape->writable())
        RETURN_STOP("writing to a read-only property");

    uintN slot = uint16(shape->shortid);

    vp = NULL;
    JSStackFrame* cfp = (JSStackFrame*) obj->getPrivate();
    if (cfp) {
        if (shape->getterOp() == js_GetCallArg) {
            JS_ASSERT(slot < cfp->numFormalArgs());
            vp = &cfp->argv[slot];
            nr.v = *vp;
        } else if (shape->getterOp() == js_GetCallVar ||
                   shape->getterOp() == js_GetCallVarChecked) {
            JS_ASSERT(slot < cfp->getSlotCount());
            vp = &cfp->slots()[slot];
            nr.v = *vp;
        } else {
            RETURN_STOP("dynamic property of Call object");
        }

        
        JS_ASSERT(shape->hasShortID());

        if (frameIfInRange(obj)) {
            
            
            ins = get(vp);
            nr.tracked = true;
            return RECORD_CONTINUE;
        }
    } else {
        
        
        
#ifdef DEBUG
        JSBool rv =
#endif
            js_GetPropertyHelper(cx, obj, shape->id,
                                 (op == JSOP_CALLNAME)
                                 ? JSGET_NO_METHOD_BARRIER
                                 : JSGET_METHOD_BARRIER,
                                 &nr.v);
        JS_ASSERT(rv);
    }

    LIns* obj_ins;
    JSObject* parent = cx->fp()->callee()->getParent();
    LIns* parent_ins = stobj_get_parent(get(&cx->fp()->argv[-2]));
    CHECK_STATUS(traverseScopeChain(parent, parent_ins, obj, obj_ins));

    if (!cfp) {
        
        
        
        
        
        if (shape->getterOp() == js_GetCallArg) {
            JS_ASSERT(slot < ArgClosureTraits::slot_count(obj));
            slot += ArgClosureTraits::slot_offset(obj);
        } else if (shape->getterOp() == js_GetCallVar ||
                   shape->getterOp() == js_GetCallVarChecked) {
            JS_ASSERT(slot < VarClosureTraits::slot_count(obj));
            slot += VarClosureTraits::slot_offset(obj);
        } else {
            RETURN_STOP("dynamic property of Call object");
        }

        
        JS_ASSERT(shape->hasShortID());

        ins = unbox_slot(obj, obj_ins, slot, snapshot(BRANCH_EXIT));
    } else {
        ClosureVarInfo* cv = new (traceAlloc()) ClosureVarInfo();
        cv->slot = slot;
#ifdef DEBUG
        cv->callDepth = callDepth;
#endif

        LIns* outp = lir->insAlloc(sizeof(double));
        LIns* args[] = {
            outp,
            INS_CONSTPTR(cv),
            obj_ins,
            cx_ins
        };
        const CallInfo* ci;
        if (shape->getterOp() == js_GetCallArg) {
            ci = &GetClosureArg_ci;
        } else if (shape->getterOp() == js_GetCallVar ||
                   shape->getterOp() == js_GetCallVarChecked) {
            ci = &GetClosureVar_ci;
        } else {
            RETURN_STOP("dynamic property of Call object");
        }

        
        JS_ASSERT(shape->hasShortID());

        LIns* call_ins = lir->insCall(ci, args);

        JSValueType type = getCoercedType(nr.v);
        guard(true,
              addName(lir->ins2(LIR_eqi, call_ins, lir->insImmI(type)),
                      "guard(type-stable name access)"),
              BRANCH_EXIT);
        ins = stackLoad(outp, ACCSET_OTHER, type);
    }
    nr.tracked = false;
    nr.obj = obj;
    nr.obj_ins = obj_ins;
    nr.shape = shape;
    return RECORD_CONTINUE;
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
    set(&stackval(n), i);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::alu(LOpcode v, jsdouble v0, jsdouble v1, LIns* s0, LIns* s1)
{
    




    if (!oracle || oracle->isInstructionUndemotable(cx->regs->pc) ||
        !isPromoteInt(s0) || !isPromoteInt(s1)) {
    out:
        if (v == LIR_modd) {
            LIns* args[] = { s1, s0 };
            return lir->insCall(&js_dmod_ci, args);
        }
        LIns* result = lir->ins2(v, s0, s1);
        JS_ASSERT_IF(s0->isImmD() && s1->isImmD(), result->isImmD());
        return result;
    }

    jsdouble r;
    switch (v) {
    case LIR_addd:
        r = v0 + v1;
        break;
    case LIR_subd:
        r = v0 - v1;
        break;
    case LIR_muld:
        r = v0 * v1;
        if (r == 0.0)
            goto out;
        break;
#if defined NANOJIT_IA32 || defined NANOJIT_X64
    case LIR_divd:
        if (v1 == 0)
            goto out;
        r = v0 / v1;
        break;
    case LIR_modd:
        if (v0 < 0 || v1 == 0 || (s1->isImmD() && v1 < 0))
            goto out;
        r = js_dmod(v0, v1);
        break;
#endif
    default:
        goto out;
    }

    



    if (jsint(r) != r || JSDOUBLE_IS_NEGZERO(r))
        goto out;

    LIns* d0 = demote(lir, s0);
    LIns* d1 = demote(lir, s1);

    



    VMSideExit* exit;
    LIns* result;
    switch (v) {
#if defined NANOJIT_IA32 || defined NANOJIT_X64
      case LIR_divd:
        if (d0->isImmI() && d1->isImmI())
            return lir->ins1(LIR_i2d, lir->insImmI(jsint(r)));

        exit = snapshot(OVERFLOW_EXIT);

        




        if (!d1->isImmI()) {
            LIns* gt = lir->insBranch(LIR_jt, lir->ins2ImmI(LIR_gti, d1, 0), NULL);
            guard(false, lir->insEqI_0(d1), exit);
            guard(false, lir->ins2(LIR_andi,
                                   lir->ins2ImmI(LIR_eqi, d0, 0x80000000),
                                   lir->ins2ImmI(LIR_eqi, d1, -1)), exit);
            gt->setTarget(lir->ins0(LIR_label));
        } else {
            if (d1->immI() == -1)
                guard(false, lir->ins2ImmI(LIR_eqi, d0, 0x80000000), exit);
        }
        result = lir->ins2(v = LIR_divi, d0, d1);

        
        guard(true, lir->insEqI_0(lir->ins1(LIR_modi, result)), exit);

        
        guard(false, lir->insEqI_0(result), exit);
        break;

      case LIR_modd: {
        if (d0->isImmI() && d1->isImmI())
            return lir->ins1(LIR_i2d, lir->insImmI(jsint(r)));

        exit = snapshot(OVERFLOW_EXIT);

        
        if (!d1->isImmI())
            guard(false, lir->insEqI_0(d1), exit);
        result = lir->ins1(v = LIR_modi, lir->ins2(LIR_divi, d0, d1));

        
        LIns* branch = lir->insBranch(LIR_jf, lir->insEqI_0(result), NULL);

        



        guard(false, lir->ins2ImmI(LIR_lti, d0, 0), exit);
        branch->setTarget(lir->ins0(LIR_label));
        break;
      }
#endif

      default:
        v = arithOpcodeD2I(v);
        JS_ASSERT(v == LIR_addi || v == LIR_muli || v == LIR_subi);

        






        if (!IsOverflowSafe(v, d0) || !IsOverflowSafe(v, d1)) {
            exit = snapshot(OVERFLOW_EXIT);
            result = guard_xov(v, d0, d1, exit);
            if (v == LIR_muli) 
                guard(false, lir->insEqI_0(result), exit);
        } else {
            result = lir->ins2(v, d0, d1);
        }
        break;
    }
    JS_ASSERT_IF(d0->isImmI() && d1->isImmI(),
                 result->isImmI() && result->immI() == jsint(r));
    return lir->ins1(LIR_i2d, result);
}

LIns*
TraceRecorder::i2d(LIns* i)
{
    return lir->ins1(LIR_i2d, i);
}

LIns*
TraceRecorder::d2i(LIns* f, bool resultCanBeImpreciseIfFractional)
{
    if (f->isImmD())
        return lir->insImmI(js_DoubleToECMAInt32(f->immD()));
    if (isfop(f, LIR_i2d) || isfop(f, LIR_ui2d))
        return foprnd1(f);
    if (isfop(f, LIR_addd) || isfop(f, LIR_subd)) {
        LIns* lhs = foprnd1(f);
        LIns* rhs = foprnd2(f);
        if (isPromote(lhs) && isPromote(rhs)) {
            LOpcode op = arithOpcodeD2I(f->opcode());
            return lir->ins2(op, demote(lir, lhs), demote(lir, rhs));
        }
    }
    if (f->isCall()) {
        const CallInfo* ci = f->callInfo();
        if (ci == &js_UnboxDouble_ci) {
#if JS_BITS_PER_WORD == 32
            LIns *tag_ins = fcallarg(f, 0);
            LIns *payload_ins = fcallarg(f, 1);
            LIns* args[] = { payload_ins, tag_ins };
            return lir->insCall(&js_UnboxInt32_ci, args);
#else
            LIns* val_ins = fcallarg(f, 0);
            LIns* args[] = { val_ins };
            return lir->insCall(&js_UnboxInt32_ci, args);
#endif
        }
        if (ci == &js_StringToNumber_ci) {
            LIns* args[] = { fcallarg(f, 1), fcallarg(f, 0) };
            return lir->insCall(&js_StringToInt32_ci, args);
        }
    }
    return resultCanBeImpreciseIfFractional
         ? lir->ins1(LIR_d2i, f)
         : lir->insCall(&js_DoubleToInt32_ci, &f);
}

LIns*
TraceRecorder::f2u(LIns* f)
{
    if (f->isImmD())
        return lir->insImmI(js_DoubleToECMAUint32(f->immD()));
    if (isfop(f, LIR_i2d) || isfop(f, LIR_ui2d))
        return foprnd1(f);
    return lir->insCall(&js_DoubleToUint32_ci, &f);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::makeNumberInt32(LIns* f)
{
    JS_ASSERT(f->isD());
    LIns* x;
    if (!isPromote(f)) {
        
        
        
        x = d2i(f, true);
        guard(true, lir->ins2(LIR_eqd, f, lir->ins1(LIR_i2d, x)), MISMATCH_EXIT);
    } else {
        x = demote(lir, f);
    }
    return x;
}

JS_REQUIRES_STACK LIns*
TraceRecorder::stringify(const Value& v)
{
    LIns* v_ins = get(&v);
    if (v.isString())
        return v_ins;

    LIns* args[] = { v_ins, cx_ins };
    const CallInfo* ci;
    if (v.isNumber()) {
        ci = &js_NumberToString_ci;
    } else if (v.isUndefined()) {
        return INS_ATOM(cx->runtime->atomState.booleanAtoms[2]);
    } else if (v.isBoolean()) {
        ci = &js_BooleanIntToString_ci;
    } else {
        




        JS_ASSERT(v.isNull());
        return INS_ATOM(cx->runtime->atomState.nullAtom);
    }

    v_ins = lir->insCall(ci, args);
    guard(false, lir->insEqP_0(v_ins), OOM_EXIT);
    return v_ins;
}

JS_REQUIRES_STACK bool
TraceRecorder::canCallImacro() const
{
    
    return !cx->fp()->hasIMacroPC();
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callImacro(jsbytecode* imacro)
{
    return canCallImacro() ? callImacroInfallibly(imacro) : RECORD_STOP;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callImacroInfallibly(jsbytecode* imacro)
{
    JSStackFrame* fp = cx->fp();
    JS_ASSERT(!fp->hasIMacroPC());
    JSFrameRegs* regs = cx->regs;
    fp->setIMacroPC(regs->pc);
    regs->pc = imacro;
    updateAtoms();
    return RECORD_IMACRO;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::ifop()
{
    Value& v = stackval(-1);
    LIns* v_ins = get(&v);
    bool cond;
    LIns* x;

    if (v.isNull() || v.isUndefined()) {
        cond = false;
        x = lir->insImmI(0);
    } else if (!v.isPrimitive()) {
        cond = true;
        x = lir->insImmI(1);
    } else if (v.isBoolean()) {
        
        cond = v.isTrue();
        x = lir->ins2ImmI(LIR_eqi, v_ins, 1);
    } else if (v.isNumber()) {
        jsdouble d = v.toNumber();
        cond = !JSDOUBLE_IS_NaN(d) && d;
        x = lir->ins2(LIR_andi,
                      lir->ins2(LIR_eqd, v_ins, v_ins),
                      lir->insEqI_0(lir->ins2(LIR_eqd, v_ins, lir->insImmD(0))));
    } else if (v.isString()) {
        cond = v.toString()->length() != 0;
        x = getStringLength(v_ins);
    } else {
        JS_NOT_REACHED("ifop");
        return ARECORD_STOP;
    }

    jsbytecode* pc = cx->regs->pc;
    emitIf(pc, cond, x);
    return checkTraceEnd(pc);
}

#ifdef NANOJIT_IA32





JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::tableswitch()
{
    Value& v = stackval(-1);

    
    if (!v.isNumber())
        return ARECORD_CONTINUE;

    
    LIns* v_ins = d2i(get(&v));
    if (v_ins->isImmI())
        return ARECORD_CONTINUE;

    jsbytecode* pc = cx->regs->pc;
    
    if (anchor &&
        (anchor->exitType == CASE_EXIT || anchor->exitType == DEFAULT_EXIT) &&
        fragment->ip == pc) {
        return ARECORD_CONTINUE;
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

    




    int count = high + 1 - low;
    if (count == 0)
        return ARECORD_CONTINUE;

    
    if (count > MAX_TABLE_SWITCH)
        return InjectStatus(switchop());

    
    SwitchInfo* si = new (traceAlloc()) SwitchInfo();
    si->count = count;
    si->table = 0;
    si->index = (uint32) -1;
    LIns* diff = lir->ins2(LIR_subi, v_ins, lir->insImmI(low));
    LIns* cmp = lir->ins2(LIR_ltui, diff, lir->insImmI(si->count));
    lir->insGuard(LIR_xf, cmp, createGuardRecord(snapshot(DEFAULT_EXIT)));
    lir->insStore(diff, lir->insImmP(&si->index), 0, ACCSET_OTHER);
    VMSideExit* exit = snapshot(CASE_EXIT);
    exit->switchInfo = si;
    LIns* guardIns = lir->insGuard(LIR_xtbl, diff, createGuardRecord(exit));
    fragment->lastIns = guardIns;
    CHECK_STATUS_A(compile());
    return finishSuccessfully();
}
#endif

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::switchop()
{
    Value& v = stackval(-1);
    LIns* v_ins = get(&v);

    
    if (v_ins->isImmAny())
        return RECORD_CONTINUE;
    if (v.isNumber()) {
        jsdouble d = v.toNumber();
        guard(true,
              addName(lir->ins2(LIR_eqd, v_ins, lir->insImmD(d)),
                      "guard(switch on numeric)"),
              BRANCH_EXIT);
    } else if (v.isString()) {
        LIns* args[] = { INS_CONSTSTR(v.toString()), v_ins };
        guard(true,
              addName(lir->insEqI_0(lir->insEqI_0(lir->insCall(&js_EqualStrings_ci, args))),
                      "guard(switch on string)"),
              BRANCH_EXIT);
    } else if (v.isBoolean()) {
        guard(true,
              addName(lir->ins2(LIR_eqi, v_ins, lir->insImmI(v.isTrue())),
                      "guard(switch on boolean)"),
              BRANCH_EXIT);
    } else if (v.isUndefined()) {
        
    } else {
        RETURN_STOP("switch on object or null");
    }
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::inc(Value& v, jsint incr, bool pre)
{
    LIns* v_ins = get(&v);
    CHECK_STATUS(inc(v, v_ins, incr, pre));
    set(&v, v_ins);
    return RECORD_CONTINUE;
}





JS_REQUIRES_STACK RecordingStatus
TraceRecorder::inc(const Value &v, LIns*& v_ins, jsint incr, bool pre)
{
    LIns* v_after;
    CHECK_STATUS(incHelper(v, v_ins, v_after, incr));

    const JSCodeSpec& cs = js_CodeSpec[*cx->regs->pc];
    JS_ASSERT(cs.ndefs == 1);
    stack(-cs.nuses, pre ? v_after : v_ins);
    v_ins = v_after;
    return RECORD_CONTINUE;
}




JS_REQUIRES_STACK RecordingStatus
TraceRecorder::incHelper(const Value &v, LIns* v_ins, LIns*& v_after, jsint incr)
{
    if (!v.isNumber())
        RETURN_STOP("can only inc numbers");
    v_after = alu(LIR_addd, v.toNumber(), incr, v_ins, lir->insImmD(incr));
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::incProp(jsint incr, bool pre)
{
    Value& l = stackval(-1);
    if (l.isPrimitive())
        RETURN_STOP_A("incProp on primitive");

    JSObject* obj = &l.toObject();
    LIns* obj_ins = get(&l);

    uint32 slot;
    LIns* v_ins;
    CHECK_STATUS_A(prop(obj, obj_ins, &slot, &v_ins, NULL));

    if (slot == SHAPE_INVALID_SLOT)
        RETURN_STOP_A("incProp on invalid slot");

    Value& v = obj->getSlotRef(slot);
    CHECK_STATUS_A(inc(v, v_ins, incr, pre));

    LIns* dslots_ins = NULL;
    stobj_set_slot(obj_ins, slot, dslots_ins, v, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::incElem(jsint incr, bool pre)
{
    Value& r = stackval(-1);
    Value& l = stackval(-2);
    Value* vp;
    LIns* v_ins;
    LIns* addr_ins;

    if (!l.isPrimitive() && l.toObject().isDenseArray() && r.isInt32()) {
        guardDenseArray(get(&l), MISMATCH_EXIT);
        CHECK_STATUS(denseArrayElement(l, r, vp, v_ins, addr_ins));
        if (!addr_ins) 
            return RECORD_STOP;
        CHECK_STATUS(inc(*vp, v_ins, incr, pre));
        box_value_into(*vp, v_ins, addr_ins, 0, ACCSET_OTHER);
        return RECORD_CONTINUE;
    }

    return callImacro((incr == 1)
                      ? pre ? incelem_imacros.incelem : incelem_imacros.eleminc
                      : pre ? decelem_imacros.decelem : decelem_imacros.elemdec);
}

static bool
EvalCmp(LOpcode op, double l, double r)
{
    bool cond;
    switch (op) {
      case LIR_eqd:
        cond = (l == r);
        break;
      case LIR_ltd:
        cond = l < r;
        break;
      case LIR_gtd:
        cond = l > r;
        break;
      case LIR_led:
        cond = l <= r;
        break;
      case LIR_ged:
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
    if (op == LIR_eqd)
        return !!js_EqualStrings(l, r);
    return EvalCmp(op, js_CompareStrings(l, r), 0);
}

JS_REQUIRES_STACK void
TraceRecorder::strictEquality(bool equal, bool cmpCase)
{
    Value& r = stackval(-1);
    Value& l = stackval(-2);
    LIns* l_ins = get(&l);
    LIns* r_ins = get(&r);
    LIns* x;
    bool cond;

    JSValueType ltag = getPromotedType(l);
    if (ltag != getPromotedType(r)) {
        cond = !equal;
        x = lir->insImmI(cond);
    } else if (ltag == JSVAL_TYPE_STRING) {
        LIns* args[] = { r_ins, l_ins };
        x = lir->ins2ImmI(LIR_eqi, lir->insCall(&js_EqualStrings_ci, args), equal);
        cond = !!js_EqualStrings(l.toString(), r.toString());
    } else {
        LOpcode op;
        if (ltag == JSVAL_TYPE_DOUBLE)
            op = LIR_eqd;
        else if (ltag == JSVAL_TYPE_NULL || ltag == JSVAL_TYPE_NONFUNOBJ || ltag == JSVAL_TYPE_FUNOBJ)
            op = LIR_eqp;
        else
            op = LIR_eqi;
        x = lir->ins2(op, l_ins, r_ins);
        if (!equal)
            x = lir->insEqI_0(x);
        cond = (ltag == JSVAL_TYPE_DOUBLE)
               ? l.toNumber() == r.toNumber()
               : l == r;
    }
    cond = (cond == equal);

    if (cmpCase) {
        
        if (!x->isImmI())
            guard(cond, x, BRANCH_EXIT);
        return;
    }

    set(&l, x);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::equality(bool negate, bool tryBranchAfterCond)
{
    Value& rval = stackval(-1);
    Value& lval = stackval(-2);
    LIns* l_ins = get(&lval);
    LIns* r_ins = get(&rval);

    return equalityHelper(lval, rval, l_ins, r_ins, negate, tryBranchAfterCond, lval);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::equalityHelper(Value& l, Value& r, LIns* l_ins, LIns* r_ins,
                              bool negate, bool tryBranchAfterCond,
                              Value& rval)
{
    LOpcode op = LIR_eqi;
    bool cond;
    LIns* args[] = { NULL, NULL };

    










    if (getPromotedType(l) == getPromotedType(r)) {
        if (l.isUndefined() || l.isNull()) {
            cond = true;
            if (l.isNull())
                op = LIR_eqp;
        } else if (l.isObject()) {
            if (l.toObject().getClass()->ext.equality)
                RETURN_STOP_A("Can't trace extended class equality operator");
            op = LIR_eqp;
            cond = (l == r);
        } else if (l.isBoolean()) {
            JS_ASSERT(r.isBoolean());
            cond = (l == r);
        } else if (l.isString()) {
            JSString *l_str = l.toString();
            JSString *r_str = r.toString();
            if (!l_str->isRope() && !r_str->isRope() && l_str->length() == 1 && r_str->length() == 1) {
                VMSideExit *exit = snapshot(BRANCH_EXIT);
                LIns *c = INS_CONSTWORD(1);
                guard(true, lir->ins2(LIR_eqp, getStringLength(l_ins), c), exit);
                guard(true, lir->ins2(LIR_eqp, getStringLength(r_ins), c), exit);
                l_ins = lir->insLoad(LIR_ldus2ui, getStringChars(l_ins), 0, ACCSET_OTHER, LOAD_CONST);
                r_ins = lir->insLoad(LIR_ldus2ui, getStringChars(r_ins), 0, ACCSET_OTHER, LOAD_CONST);
            } else {
                args[0] = r_ins, args[1] = l_ins;
                l_ins = lir->insCall(&js_EqualStrings_ci, args);
                r_ins = lir->insImmI(1);
            }
            cond = !!js_EqualStrings(l.toString(), r.toString());
        } else {
            JS_ASSERT(l.isNumber() && r.isNumber());
            cond = (l.toNumber() == r.toNumber());
            op = LIR_eqd;
        }
    } else if (l.isNull() && r.isUndefined()) {
        l_ins = INS_UNDEFINED();
        cond = true;
    } else if (l.isUndefined() && r.isNull()) {
        r_ins = INS_UNDEFINED();
        cond = true;
    } else if (l.isNumber() && r.isString()) {
        args[0] = r_ins, args[1] = cx_ins;
        r_ins = lir->insCall(&js_StringToNumber_ci, args);
        cond = (l.toNumber() == js_StringToNumber(cx, r.toString()));
        op = LIR_eqd;
    } else if (l.isString() && r.isNumber()) {
        args[0] = l_ins, args[1] = cx_ins;
        l_ins = lir->insCall(&js_StringToNumber_ci, args);
        cond = (js_StringToNumber(cx, l.toString()) == r.toNumber());
        op = LIR_eqd;
    } else {
        
        
        if (l.isBoolean()) {
            l_ins = i2d(l_ins);
            set(&l, l_ins);
            l.setInt32(l.isTrue());
            return equalityHelper(l, r, l_ins, r_ins, negate,
                                  tryBranchAfterCond, rval);
        }
        if (r.isBoolean()) {
            r_ins = i2d(r_ins);
            set(&r, r_ins);
            r.setInt32(r.isTrue());
            return equalityHelper(l, r, l_ins, r_ins, negate,
                                  tryBranchAfterCond, rval);
        }
        if ((l.isString() || l.isNumber()) && !r.isPrimitive()) {
            CHECK_STATUS_A(guardNativeConversion(r));
            return InjectStatus(callImacro(equality_imacros.any_obj));
        }
        if (!l.isPrimitive() && (r.isString() || r.isNumber())) {
            CHECK_STATUS_A(guardNativeConversion(l));
            return InjectStatus(callImacro(equality_imacros.obj_any));
        }

        l_ins = lir->insImmI(0);
        r_ins = lir->insImmI(1);
        cond = false;
    }

    
    LIns* x = lir->ins2(op, l_ins, r_ins);
    if (negate) {
        x = lir->insEqI_0(x);
        cond = !cond;
    }

    jsbytecode* pc = cx->regs->pc;

    




    if (tryBranchAfterCond)
        fuseIf(pc + 1, cond, x);

    



    if (pc[1] == JSOP_IFNE || pc[1] == JSOP_IFEQ)
        CHECK_STATUS_A(checkTraceEnd(pc + 1));

    





    set(&rval, x);

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::relational(LOpcode op, bool tryBranchAfterCond)
{
    Value& r = stackval(-1);
    Value& l = stackval(-2);
    LIns* x = NULL;
    bool cond;
    LIns* l_ins = get(&l);
    LIns* r_ins = get(&r);
    bool fp = false;
    jsdouble lnum, rnum;

    




    if (!l.isPrimitive()) {
        CHECK_STATUS_A(guardNativeConversion(l));
        if (!r.isPrimitive()) {
            CHECK_STATUS_A(guardNativeConversion(r));
            return InjectStatus(callImacro(binary_imacros.obj_obj));
        }
        return InjectStatus(callImacro(binary_imacros.obj_any));
    }
    if (!r.isPrimitive()) {
        CHECK_STATUS_A(guardNativeConversion(r));
        return InjectStatus(callImacro(binary_imacros.any_obj));
    }

    
    if (l.isString() && r.isString()) {
        LIns* args[] = { r_ins, l_ins };
        l_ins = lir->insCall(&js_CompareStrings_ci, args);
        r_ins = lir->insImmI(0);
        cond = EvalCmp(op, l.toString(), r.toString());
        goto do_comparison;
    }

    
    if (!l.isNumber()) {
        LIns* args[] = { l_ins, cx_ins };
        if (l.isBoolean()) {
            l_ins = i2d(l_ins);
        } else if (l.isUndefined()) {
            l_ins = lir->insImmD(js_NaN);
        } else if (l.isString()) {
            l_ins = lir->insCall(&js_StringToNumber_ci, args);
        } else if (l.isNull()) {
            l_ins = lir->insImmD(0.0);
        } else {
            JS_NOT_REACHED("JSVAL_IS_NUMBER if int/double, objects should "
                           "have been handled at start of method");
            RETURN_STOP_A("safety belt");
        }
    }
    if (!r.isNumber()) {
        LIns* args[] = { r_ins, cx_ins };
        if (r.isBoolean()) {
            r_ins = i2d(r_ins);
        } else if (r.isUndefined()) {
            r_ins = lir->insImmD(js_NaN);
        } else if (r.isString()) {
            r_ins = lir->insCall(&js_StringToNumber_ci, args);
        } else if (r.isNull()) {
            r_ins = lir->insImmD(0.0);
        } else {
            JS_NOT_REACHED("JSVAL_IS_NUMBER if int/double, objects should "
                           "have been handled at start of method");
            RETURN_STOP_A("safety belt");
        }
    }
    {
        AutoValueRooter tvr(cx);
        *tvr.addr() = l;
        ValueToNumber(cx, tvr.value(), &lnum);
        *tvr.addr() = r;
        ValueToNumber(cx, tvr.value(), &rnum);
    }
    cond = EvalCmp(op, lnum, rnum);
    fp = true;

    
  do_comparison:
    



    if (!fp) {
        JS_ASSERT(isCmpDOpcode(op));
        op = cmpOpcodeD2I(op);
    }
    x = lir->ins2(op, l_ins, r_ins);

    jsbytecode* pc = cx->regs->pc;

    




    if (tryBranchAfterCond)
        fuseIf(pc + 1, cond, x);

    



    if (pc[1] == JSOP_IFNE || pc[1] == JSOP_IFEQ)
        CHECK_STATUS_A(checkTraceEnd(pc + 1));

    





    set(&l, x);

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::unary(LOpcode op)
{
    Value& v = stackval(-1);
    bool intop = retTypes[op] == LTy_I;
    if (v.isNumber()) {
        LIns* a = get(&v);
        if (intop)
            a = d2i(a);
        a = lir->ins1(op, a);
        if (intop)
            a = lir->ins1(LIR_i2d, a);
        set(&v, a);
        return RECORD_CONTINUE;
    }
    return RECORD_STOP;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::binary(LOpcode op)
{
    Value& r = stackval(-1);
    Value& l = stackval(-2);

    if (!l.isPrimitive()) {
        CHECK_STATUS(guardNativeConversion(l));
        if (!r.isPrimitive()) {
            CHECK_STATUS(guardNativeConversion(r));
            return callImacro(binary_imacros.obj_obj);
        }
        return callImacro(binary_imacros.obj_any);
    }
    if (!r.isPrimitive()) {
        CHECK_STATUS(guardNativeConversion(r));
        return callImacro(binary_imacros.any_obj);
    }

    bool intop = retTypes[op] == LTy_I;
    LIns* a = get(&l);
    LIns* b = get(&r);

    bool leftIsNumber = l.isNumber();
    jsdouble lnum = leftIsNumber ? l.toNumber() : 0;

    bool rightIsNumber = r.isNumber();
    jsdouble rnum = rightIsNumber ? r.toNumber() : 0;

    if (l.isString()) {
        NanoAssert(op != LIR_addd); 
        LIns* args[] = { a, cx_ins };
        a = lir->insCall(&js_StringToNumber_ci, args);
        lnum = js_StringToNumber(cx, l.toString());
        leftIsNumber = true;
    }
    if (r.isString()) {
        NanoAssert(op != LIR_addd); 
        LIns* args[] = { b, cx_ins };
        b = lir->insCall(&js_StringToNumber_ci, args);
        rnum = js_StringToNumber(cx, r.toString());
        rightIsNumber = true;
    }
    if (l.isBoolean()) {
        a = i2d(a);
        lnum = l.toBoolean();
        leftIsNumber = true;
    } else if (l.isUndefined()) {
        a = lir->insImmD(js_NaN);
        lnum = js_NaN;
        leftIsNumber = true;
    }
    if (r.isBoolean()) {
        b = i2d(b);
        rnum = r.toBoolean();
        rightIsNumber = true;
    } else if (r.isUndefined()) {
        b = lir->insImmD(js_NaN);
        rnum = js_NaN;
        rightIsNumber = true;
    }
    if (leftIsNumber && rightIsNumber) {
        if (intop) {
            a = (op == LIR_rshui) ? f2u(a) : d2i(a);
            b = d2i(b);
        }
        a = alu(op, lnum, rnum, a, b);
        if (intop)
            a = lir->ins1(op == LIR_rshui ? LIR_ui2d : LIR_i2d, a);
        set(&l, a);
        return RECORD_CONTINUE;
    }
    return RECORD_STOP;
}

#if defined DEBUG_notme && defined XP_UNIX
#include <stdio.h>

static FILE* shapefp = NULL;

static void
DumpShape(JSObject* obj, const char* prefix)
{
    if (!shapefp) {
        shapefp = fopen("/tmp/shapes.dump", "w");
        if (!shapefp)
            return;
    }

    fprintf(shapefp, "\n%s: shape %u flags %x\n", prefix, obj->shape(), obj->flags);
    for (Shape::Range r = obj->lastProperty()->all(); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();

        if (JSID_IS_ATOM(shape.id)) {
            fprintf(shapefp, " %s", JS_GetStringBytes(JSID_TO_STRING(shape.id)));
        } else {
            JS_ASSERT(!JSID_IS_OBJECT(shape.id));
            fprintf(shapefp, " %d", JSID_TO_INT(shape.id));
        }
        fprintf(shapefp, " %u %p %p %x %x %d\n",
                shape.slot, shape.getter, shape.setter, shape.attrs, shape.flags, shape.shortid);
    }
    fflush(shapefp);
}

void
TraceRecorder::dumpGuardedShapes(const char* prefix)
{
    for (GuardedShapeTable::Range r = guardedShapeTable.all(); !r.empty(); r.popFront())
        DumpShape(r.front().value, prefix);
}
#endif 

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::guardShape(LIns* obj_ins, JSObject* obj, uint32 shape, const char* guardName,
                          VMSideExit* exit)
{
    
    GuardedShapeTable::AddPtr p = guardedShapeTable.lookupForAdd(obj_ins);
    if (p) {
        JS_ASSERT(p->value == obj);
        return RECORD_CONTINUE;
    }
    if (!guardedShapeTable.add(p, obj_ins, obj))
        return RECORD_ERROR;

    if (obj == globalObj) {
        
        guard(true,
              addName(lir->ins2(LIR_eqp, obj_ins, INS_CONSTOBJ(globalObj)), "guard_global"),
              exit);
        return RECORD_CONTINUE;
    }

#if defined DEBUG_notme && defined XP_UNIX
    DumpShape(obj, "guard");
    fprintf(shapefp, "for obj_ins %p\n", obj_ins);
#endif

    
    guard(true, addName(lir->ins2ImmI(LIR_eqi, shape_ins(obj_ins), shape), guardName), exit);
    return RECORD_CONTINUE;
}

void
TraceRecorder::forgetGuardedShapesForObject(JSObject* obj)
{
    for (GuardedShapeTable::Enum e(guardedShapeTable); !e.empty(); e.popFront()) {
        if (e.front().value == obj) {
#if defined DEBUG_notme && defined XP_UNIX
            DumpShape(entry->obj, "forget");
#endif
            e.removeFront();
        }
    }
}

void
TraceRecorder::forgetGuardedShapes()
{
#if defined DEBUG_notme && defined XP_UNIX
    dumpGuardedShapes("forget-all");
#endif
    guardedShapeTable.clear();
}

inline LIns*
TraceRecorder::shape_ins(LIns* obj_ins)
{
    return addName(lir->insLoad(LIR_ldi, obj_ins, int(offsetof(JSObject, objShape)), ACCSET_OTHER),
                   "objShape");
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::test_property_cache(JSObject* obj, LIns* obj_ins, JSObject*& obj2, PCVal& pcval)
{
    jsbytecode* pc = cx->regs->pc;
    JS_ASSERT(*pc != JSOP_INITPROP && *pc != JSOP_INITMETHOD &&
              *pc != JSOP_SETNAME && *pc != JSOP_SETPROP && *pc != JSOP_SETMETHOD);

    
    
    
    JSObject* aobj = obj;
    if (obj->isDenseArray()) {
        guardDenseArray(obj_ins, BRANCH_EXIT);
        aobj = obj->getProto();
        obj_ins = stobj_get_proto(obj_ins);
    }

    if (!aobj->isNative())
        RETURN_STOP_A("non-native object");

    JSAtom* atom;
    PropertyCacheEntry* entry;
    JS_PROPERTY_CACHE(cx).test(cx, pc, aobj, obj2, entry, atom);
    if (atom) {
        
        
        jsid id = ATOM_TO_JSID(atom);

        
        forgetGuardedShapes();

        JSProperty* prop;
        if (JOF_OPMODE(*pc) == JOF_NAME) {
            JS_ASSERT(aobj == obj);

            TraceMonitor &localtm = *traceMonitor;
            entry = js_FindPropertyHelper(cx, id, true, &obj, &obj2, &prop);
            if (!entry)
                RETURN_ERROR_A("error in js_FindPropertyHelper");

            
            if (!localtm.recorder)
                return ARECORD_ABORTED;

            if (entry == JS_NO_PROP_CACHE_FILL)
                RETURN_STOP_A("cannot cache name");
        } else {
            TraceMonitor &localtm = *traceMonitor;
            JSContext *localcx = cx;
            int protoIndex = js_LookupPropertyWithFlags(cx, aobj, id,
                                                        cx->resolveFlags,
                                                        &obj2, &prop);

            if (protoIndex < 0)
                RETURN_ERROR_A("error in js_LookupPropertyWithFlags");

            
            if (!localtm.recorder) {
                if (prop)
                    obj2->dropProperty(localcx, prop);
                return ARECORD_ABORTED;
            }

            if (prop) {
                if (!obj2->isNative())
                    RETURN_STOP_A("property found on non-native object");
                entry = JS_PROPERTY_CACHE(cx).fill(cx, aobj, 0, protoIndex, obj2,
                                                   (Shape*) prop);
                JS_ASSERT(entry);
                if (entry == JS_NO_PROP_CACHE_FILL)
                    entry = NULL;
            }

        }

        if (!prop) {
            
            
            
            obj2 = obj;

            
            pcval.setNull();
            return ARECORD_CONTINUE;
        }

        obj2->dropProperty(cx, prop);
        if (!entry)
            RETURN_STOP_A("failed to fill property cache");
    }

#ifdef JS_THREADSAFE
    
    
    
    
    JS_ASSERT(cx->requestDepth);
#endif

    return InjectStatus(guardPropertyCacheHit(obj_ins, aobj, obj2, entry, pcval));
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::guardPropertyCacheHit(LIns* obj_ins,
                                     JSObject* aobj,
                                     JSObject* obj2,
                                     PropertyCacheEntry* entry,
                                     PCVal& pcval)
{
    VMSideExit* exit = snapshot(BRANCH_EXIT);

    uint32 vshape = entry->vshape();

    
    
    
    
    if (aobj == globalObj) {
        if (entry->adding())
            RETURN_STOP("adding a property to the global object");

        JSOp op = js_GetOpcode(cx, cx->fp()->getScript(), cx->regs->pc);
        if (JOF_OPMODE(op) != JOF_NAME) {
            guard(true,
                  addName(lir->ins2(LIR_eqp, obj_ins, INS_CONSTOBJ(globalObj)), "guard_global"),
                  exit);
        }
    } else {
        CHECK_STATUS(guardShape(obj_ins, aobj, entry->kshape, "guard_kshape", exit));
    }

    if (entry->adding()) {
        LIns *vshape_ins = addName(
            lir->insLoad(LIR_ldi,
                         addName(lir->insLoad(LIR_ldp, cx_ins, offsetof(JSContext, runtime),
                                              ACCSET_OTHER, LOAD_CONST),
                                 "runtime"),
                         offsetof(JSRuntime, protoHazardShape), ACCSET_OTHER),
            "protoHazardShape");

        guard(true,
              addName(lir->ins2ImmI(LIR_eqi, vshape_ins, vshape), "guard_protoHazardShape"),
              MISMATCH_EXIT);
    }

    
    
    if (entry->vcapTag() >= 1) {
        JS_ASSERT(obj2->shape() == vshape);
        if (obj2 == globalObj)
            RETURN_STOP("hitting the global object via a prototype chain");

        LIns* obj2_ins;
        if (entry->vcapTag() == 1) {
            
            obj2_ins = addName(stobj_get_proto(obj_ins), "proto");
            guard(false, lir->insEqP_0(obj2_ins), exit);
        } else {
            obj2_ins = INS_CONSTOBJ(obj2);
        }
        CHECK_STATUS(guardShape(obj2_ins, obj2, vshape, "guard_vshape", exit));
    }

    pcval = entry->vword;
    return RECORD_CONTINUE;
}

void
TraceRecorder::stobj_set_fslot(LIns *obj_ins, unsigned slot, const Value &v, LIns* v_ins)
{
    box_value_into(v, v_ins, obj_ins, offsetof(JSObject, fslots) + slot * sizeof(Value), ACCSET_OTHER);
}

void
TraceRecorder::stobj_set_dslot(LIns *obj_ins, unsigned slot, LIns*& dslots_ins, 
                               const Value &v, LIns* v_ins)
{
    if (!dslots_ins)
        dslots_ins = lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, dslots), ACCSET_OTHER);
    box_value_into(v, v_ins, dslots_ins, slot * sizeof(Value), ACCSET_OTHER);
}

void
TraceRecorder::stobj_set_slot(LIns* obj_ins, unsigned slot, LIns*& dslots_ins,
                              const Value &v, LIns* v_ins)
{
    if (slot < JS_INITIAL_NSLOTS)
        stobj_set_fslot(obj_ins, slot, v, v_ins);
    else
        stobj_set_dslot(obj_ins, slot - JS_INITIAL_NSLOTS, dslots_ins, v, v_ins);
}

#if JS_BITS_PER_WORD == 32 || JS_BITS_PER_WORD == 64
void
TraceRecorder::set_array_fslot(LIns *obj_ins, unsigned slot, uint32 val)
{
    



    lir->insStore(INS_CONSTU(val), obj_ins,
                  offsetof(JSObject, fslots) + slot * sizeof(Value) + sPayloadOffset,
                  ACCSET_OTHER);
}

LIns*
TraceRecorder::stobj_get_fslot_uint32(LIns* obj_ins, unsigned slot)
{
    JS_ASSERT(slot < JS_INITIAL_NSLOTS);
    return lir->insLoad(LIR_ldi, obj_ins,
                        offsetof(JSObject, fslots) + slot * sizeof(Value) + sPayloadOffset,
                        ACCSET_OTHER);
}
#endif

LIns*
TraceRecorder::unbox_slot(JSObject *obj, LIns *obj_ins, uint32 slot, VMSideExit *exit)
{
    LIns *vaddr_ins;
    ptrdiff_t offset;
    if (slot < JS_INITIAL_NSLOTS) {
        vaddr_ins = obj_ins;
        offset = offsetof(JSObject, fslots) + slot * sizeof(Value);
    } else {
        vaddr_ins = lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, dslots), ACCSET_OTHER);
        offset = (slot - JS_INITIAL_NSLOTS) * sizeof(Value);
    }

    const Value &v = obj->getSlot(slot);
    return unbox_value(v, vaddr_ins, offset, exit);
}

#if JS_BITS_PER_WORD == 32

LIns*
TraceRecorder::stobj_get_const_private_ptr(LIns *obj_ins, unsigned slot)
{
    JS_ASSERT(slot < JS_INITIAL_NSLOTS);
    return lir->insLoad(LIR_ldi, obj_ins,
                        offsetof(JSObject, fslots) + slot * sizeof(Value) + sPayloadOffset,
                        ACCSET_OTHER, LOAD_CONST);
}

LIns*
TraceRecorder::stobj_get_fslot_ptr(LIns *obj_ins, unsigned slot)
{
    JS_ASSERT(slot < JS_INITIAL_NSLOTS);
    return lir->insLoad(LIR_ldi, obj_ins,
                        offsetof(JSObject, fslots) + slot * sizeof(Value) + sPayloadOffset,
                        ACCSET_OTHER);
}

void
TraceRecorder::box_undefined_into(LIns *vaddr_ins, ptrdiff_t offset, AccSet accSet)
{
    lir->insStore(INS_CONSTU(JSVAL_TAG_UNDEFINED), vaddr_ins, offset + sTagOffset, accSet);
    lir->insStore(INS_CONST(0), vaddr_ins, offset + sPayloadOffset, accSet);
}

void
TraceRecorder::box_null_into(LIns *dstaddr_ins, ptrdiff_t offset, AccSet accSet)
{
    lir->insStore(INS_CONSTU(JSVAL_TAG_NULL), dstaddr_ins, offset + sTagOffset, accSet);
    lir->insStore(INS_CONST(0), dstaddr_ins, offset + sPayloadOffset, accSet);
}

inline LIns*
TraceRecorder::unbox_number_as_double(LIns *vaddr_ins, ptrdiff_t offset, LIns *tag_ins,
                                      VMSideExit *exit, AccSet accSet)
{
    guard(true, lir->ins2(LIR_leui, tag_ins, INS_CONSTU(JSVAL_UPPER_INCL_TAG_OF_NUMBER_SET)), exit);
    LIns *val_ins = lir->insLoad(LIR_ldi, vaddr_ins, offset + sPayloadOffset, accSet);
    LIns* args[] = { val_ins, tag_ins };
    return lir->insCall(&js_UnboxDouble_ci, args);
}

inline LIns*
TraceRecorder::unbox_non_double_object(LIns* vaddr_ins, ptrdiff_t offset, LIns* tag_ins,
                                       JSValueType type, VMSideExit* exit, AccSet accSet)
{
    LIns *val_ins;
    if (type == JSVAL_TYPE_UNDEFINED) {
        val_ins = INS_UNDEFINED();
    } else if (type == JSVAL_TYPE_NULL) {
        val_ins = INS_NULL();
    } else {
        JS_ASSERT(type == JSVAL_TYPE_INT32 || type == JSVAL_TYPE_OBJECT ||
                  type == JSVAL_TYPE_STRING || type == JSVAL_TYPE_BOOLEAN ||
                  type == JSVAL_TYPE_MAGIC);
        val_ins = lir->insLoad(LIR_ldi, vaddr_ins, offset + sPayloadOffset, accSet);
    }

    guard(true, lir->ins2(LIR_eqi, tag_ins, INS_CONSTU(JSVAL_TYPE_TO_TAG(type))), exit);
    return val_ins;
}

LIns*
TraceRecorder::unbox_object(LIns* vaddr_ins, ptrdiff_t offset, LIns* tag_ins,
                            JSValueType type, VMSideExit* exit, AccSet accSet)
{
    JS_ASSERT(type == JSVAL_TYPE_FUNOBJ || type == JSVAL_TYPE_NONFUNOBJ);
    guard(true, lir->ins2(LIR_eqi, tag_ins, INS_CONSTU(JSVAL_TAG_OBJECT)), exit);
    LIns *payload_ins = lir->insLoad(LIR_ldi, vaddr_ins, offset + sPayloadOffset, accSet);
    if (type == JSVAL_TYPE_FUNOBJ)
        guardClass(payload_ins, &js_FunctionClass, exit, LOAD_NORMAL);
    else
        guardNotClass(payload_ins, &js_FunctionClass, exit, LOAD_NORMAL);
    return payload_ins;
}

LIns*
TraceRecorder::unbox_value(const Value &v, LIns *vaddr_ins, ptrdiff_t offset, VMSideExit *exit,
                           bool force_double)
{
    AccSet accSet = vaddr_ins == lirbuf->sp ? ACCSET_STACK : ACCSET_OTHER;
    LIns *tag_ins = lir->insLoad(LIR_ldi, vaddr_ins, offset + sTagOffset, accSet);

    if (v.isNumber() && force_double)
        return unbox_number_as_double(vaddr_ins, offset, tag_ins, exit, accSet);

    if (v.isInt32()) {
        guard(true, lir->ins2(LIR_eqi, tag_ins, INS_CONSTU(JSVAL_TAG_INT32)), exit);
        return i2d(lir->insLoad(LIR_ldi, vaddr_ins, offset + sPayloadOffset, accSet));
    }

    if (v.isDouble()) {
        guard(true, lir->ins2(LIR_ltui, tag_ins, INS_CONSTU(JSVAL_TAG_CLEAR)), exit);
        return lir->insLoad(LIR_ldd, vaddr_ins, offset, accSet);
    }

    if (v.isObject()) {
        JSValueType type = v.toObject().isFunction() ? JSVAL_TYPE_FUNOBJ : JSVAL_TYPE_NONFUNOBJ;
        return unbox_object(vaddr_ins, offset, tag_ins, type, exit, accSet);
    }

    JSValueType type = v.extractNonDoubleObjectTraceType();
    return unbox_non_double_object(vaddr_ins, offset, tag_ins, type, exit, accSet);
}

void
TraceRecorder::unbox_any_object(LIns *vaddr_ins, LIns **obj_ins, LIns **is_obj_ins, AccSet accSet)
{
    LIns *tag_ins = lir->insLoad(LIR_ldp, vaddr_ins, sTagOffset, accSet);
    *is_obj_ins = lir->ins2(LIR_eqi, tag_ins, INS_CONSTU(JSVAL_TAG_OBJECT));
    *obj_ins = lir->insLoad(LIR_ldp, vaddr_ins, sPayloadOffset, accSet);
}

LIns*
TraceRecorder::is_boxed_true(LIns *vaddr_ins, AccSet accSet)
{
    LIns *tag_ins = lir->insLoad(LIR_ldi, vaddr_ins, sTagOffset, accSet);
    LIns *bool_ins = lir->ins2(LIR_eqi, tag_ins, INS_CONSTU(JSVAL_TAG_BOOLEAN));
    LIns *payload_ins = lir->insLoad(LIR_ldi, vaddr_ins, sPayloadOffset, accSet);
    return lir->ins2(LIR_andi, bool_ins, payload_ins);
}

void
TraceRecorder::box_value_into(const Value &v, LIns *v_ins, LIns *dstaddr_ins, ptrdiff_t offset,
                              AccSet accSet)
{
    if (v.isNumber()) {
        JS_ASSERT(v_ins->isD());
        if (fcallinfo(v_ins) == &js_UnboxDouble_ci) {
            LIns *tag_ins = fcallarg(v_ins, 0);
            LIns *payload_ins = fcallarg(v_ins, 1);
            lir->insStore(tag_ins, dstaddr_ins, offset + sTagOffset, accSet);
            lir->insStore(payload_ins, dstaddr_ins, offset + sPayloadOffset, accSet);
        } else if (isPromoteInt(v_ins)) {
            LIns *int_ins = demote(lir, v_ins);
            lir->insStore(INS_CONSTU(JSVAL_TAG_INT32), dstaddr_ins, 
                          offset + sTagOffset, accSet);
            lir->insStore(int_ins, dstaddr_ins, offset + sPayloadOffset, accSet);
        } else {
            lir->insStore(v_ins, dstaddr_ins, offset, accSet);
        }
        return;
    }

    if (v.isUndefined()) {
        box_undefined_into(dstaddr_ins, offset, accSet);
    } else if (v.isNull()) {
        box_null_into(dstaddr_ins, offset, accSet);
    } else {
        JSValueTag tag = v.isObject() ? JSVAL_TAG_OBJECT : v.extractNonDoubleObjectTraceTag();
        lir->insStore(INS_CONSTU(tag), dstaddr_ins, offset + sTagOffset, accSet);
        lir->insStore(v_ins, dstaddr_ins, offset + sPayloadOffset, accSet);
    }
}

LIns*
TraceRecorder::box_value_into_alloc(const Value &v, LIns *v_ins)
{
    LIns *boxed_ins = lir->insAlloc(sizeof(Value));
    box_value_into(v, v_ins, boxed_ins, 0, ACCSET_OTHER);
    return boxed_ins;
}

LIns*
TraceRecorder::box_value_for_native_call(const Value &v, LIns *v_ins)
{
    return box_value_into_alloc(v, v_ins);
}

#elif JS_BITS_PER_WORD == 64

LIns*
TraceRecorder::stobj_get_const_private_ptr(LIns *obj_ins, unsigned slot)
{
    
    JS_ASSERT(slot < JS_INITIAL_NSLOTS);
    LIns *v_ins = lir->insLoad(LIR_ldq, obj_ins,
                               offsetof(JSObject, fslots) + slot * sizeof(Value),
                               ACCSET_OTHER, LOAD_CONST);
    return lir->ins2ImmI(LIR_lshq, v_ins, 1);
}

LIns*
TraceRecorder::stobj_get_fslot_ptr(LIns *obj_ins, unsigned slot)
{
    JS_ASSERT(slot < JS_INITIAL_NSLOTS);
    LIns *v_ins = lir->insLoad(LIR_ldq, obj_ins,
                               offsetof(JSObject, fslots) + slot * sizeof(Value),
                               ACCSET_OTHER);
    return unpack_ptr(v_ins);
}

void
TraceRecorder::box_undefined_into(LIns *vaddr_ins, ptrdiff_t offset, AccSet accSet)
{
    lir->insStore(INS_CONSTQWORD(JSVAL_BITS(JSVAL_VOID)), vaddr_ins, offset, accSet);
}

inline LIns *
TraceRecorder::non_double_object_value_has_type(LIns *v_ins, JSValueType type)
{
    return lir->ins2(LIR_eqi,
                     lir->ins1(LIR_q2i, lir->ins2ImmI(LIR_rshuq, v_ins, JSVAL_TAG_SHIFT)),
                     INS_CONSTU(JSVAL_TYPE_TO_TAG(type)));
}

inline LIns *
TraceRecorder::unpack_ptr(LIns *v_ins)
{
    return lir->ins2(LIR_andq, v_ins, INS_CONSTQWORD(JSVAL_PAYLOAD_MASK));
}

inline LIns *
TraceRecorder::unbox_number_as_double(LIns *v_ins, VMSideExit *exit)
{
    guard(true,
          lir->ins2(LIR_ltuq, v_ins, INS_CONSTQWORD(JSVAL_UPPER_EXCL_SHIFTED_TAG_OF_NUMBER_SET)),
          exit);
    LIns* args[] = { v_ins };
    return lir->insCall(&js_UnboxDouble_ci, args);
}

inline nanojit::LIns*
TraceRecorder::unbox_non_double_object(LIns* v_ins, JSValueType type, VMSideExit* exit)
{
    JS_ASSERT(type <= JSVAL_UPPER_INCL_TYPE_OF_VALUE_SET);
    LIns *unboxed_ins;
    if (type == JSVAL_TYPE_UNDEFINED) {
        unboxed_ins = INS_UNDEFINED();
    } else if (type == JSVAL_TYPE_NULL) {
        unboxed_ins = INS_NULL();
    } else if (type >= JSVAL_LOWER_INCL_TYPE_OF_GCTHING_SET) {
        unboxed_ins = unpack_ptr(v_ins);
    } else {
        JS_ASSERT(type == JSVAL_TYPE_INT32 || type == JSVAL_TYPE_BOOLEAN || type == JSVAL_TYPE_MAGIC);
        unboxed_ins = lir->ins1(LIR_q2i, v_ins);
    }

    guard(true, non_double_object_value_has_type(v_ins, type), exit);
    return unboxed_ins;
}

LIns*
TraceRecorder::unbox_object(LIns* v_ins, JSValueType type, VMSideExit* exit)
{
    JS_STATIC_ASSERT(JSVAL_TYPE_OBJECT == JSVAL_UPPER_INCL_TYPE_OF_VALUE_SET);
    JS_ASSERT(type == JSVAL_TYPE_FUNOBJ || type == JSVAL_TYPE_NONFUNOBJ);
    guard(true,
          lir->ins2(LIR_geuq, v_ins, INS_CONSTQWORD(JSVAL_SHIFTED_TAG_OBJECT)),
          exit);
    v_ins = unpack_ptr(v_ins);
    if (type == JSVAL_TYPE_FUNOBJ)
        guardClass(v_ins, &js_FunctionClass, exit, LOAD_NORMAL);
    else
        guardNotClass(v_ins, &js_FunctionClass, exit, LOAD_NORMAL);
    return v_ins;
}

LIns*
TraceRecorder::unbox_value(const Value &v, LIns *vaddr_ins, ptrdiff_t offset, VMSideExit *exit,
                           bool force_double)
{
    AccSet accSet = vaddr_ins == lirbuf->sp ? ACCSET_STACK : ACCSET_OTHER;
    LIns *v_ins = lir->insLoad(LIR_ldq, vaddr_ins, offset, accSet);

    if (v.isNumber() && force_double)
        return unbox_number_as_double(v_ins, exit);

    if (v.isInt32()) {
        guard(true, non_double_object_value_has_type(v_ins, JSVAL_TYPE_INT32), exit);
        return i2d(lir->ins1(LIR_q2i, v_ins));
    }

    if (v.isDouble()) {
        guard(true, lir->ins2(LIR_leuq, v_ins, INS_CONSTQWORD(JSVAL_SHIFTED_TAG_MAX_DOUBLE)), exit);
        return lir->ins1(LIR_qasd, v_ins);
    }

    if (v.isObject()) {
        JSValueType type = v.toObject().isFunction() ? JSVAL_TYPE_FUNOBJ : JSVAL_TYPE_NONFUNOBJ;
        return unbox_object(v_ins, type, exit);
    }

    JSValueType type = v.extractNonDoubleObjectTraceType();
    return unbox_non_double_object(v_ins, type, exit);
}

void
TraceRecorder::unbox_any_object(LIns *vaddr_ins, LIns **obj_ins, LIns **is_obj_ins, AccSet accSet)
{
    JS_STATIC_ASSERT(JSVAL_TYPE_OBJECT == JSVAL_UPPER_INCL_TYPE_OF_VALUE_SET);
    LIns *v_ins = lir->insLoad(LIR_ldq, vaddr_ins, 0, accSet);
    *is_obj_ins = lir->ins2(LIR_geuq, v_ins, INS_CONSTQWORD(JSVAL_TYPE_OBJECT));
    *obj_ins = unpack_ptr(v_ins);
}

LIns*
TraceRecorder::is_boxed_true(LIns *vaddr_ins, AccSet accSet)
{
    LIns *v_ins = lir->insLoad(LIR_ldq, vaddr_ins, 0, accSet);
    return lir->ins2(LIR_eqq, v_ins, lir->insImmQ(JSVAL_BITS(JSVAL_TRUE)));
}

LIns*
TraceRecorder::box_value_for_native_call(const Value &v, LIns *v_ins)
{
    if (v.isNumber()) {
        JS_ASSERT(v_ins->isD());
        if (fcallinfo(v_ins) == &js_UnboxDouble_ci)
            return fcallarg(v_ins, 0);
        if (isPromoteInt(v_ins)) {
            return lir->ins2(LIR_orq,
                             lir->ins1(LIR_ui2uq, demote(lir, v_ins)),
                             INS_CONSTQWORD(JSVAL_SHIFTED_TAG_INT32));
        }
        return lir->ins1(LIR_dasq, v_ins);
    }

    if (v.isNull())
        return INS_CONSTQWORD(JSVAL_BITS(JSVAL_NULL));
    if (v.isUndefined())
        return INS_CONSTQWORD(JSVAL_BITS(JSVAL_VOID));

    JSValueTag tag = v.isObject() ? JSVAL_TAG_OBJECT : v.extractNonDoubleObjectTraceTag();
    uint64 shiftedTag = ((uint64)tag) << JSVAL_TAG_SHIFT;
    LIns *shiftedTag_ins = INS_CONSTQWORD(shiftedTag);

    if (v.isGCThing())
        return lir->ins2(LIR_orq, v_ins, shiftedTag_ins);
    return lir->ins2(LIR_orq, lir->ins1(LIR_ui2uq, v_ins), shiftedTag_ins);
}

void
TraceRecorder::box_value_into(const Value &v, LIns *v_ins, LIns *dstaddr_ins, ptrdiff_t offset,
                              AccSet accSet)
{
    LIns *boxed_ins = box_value_for_native_call(v, v_ins);
    lir->insStore(boxed_ins, dstaddr_ins, offset, accSet);
}

LIns*
TraceRecorder::box_value_into_alloc(const Value &v, LIns *v_ins)
{
    LIns *alloc_ins = lir->insAlloc(sizeof(Value));
    box_value_into(v, v_ins, alloc_ins, 0, ACCSET_OTHER);
    return alloc_ins;
}

#endif  

LIns*
TraceRecorder::stobj_get_parent(nanojit::LIns* obj_ins)
{
    return lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, parent), ACCSET_OTHER);
}

LIns*
TraceRecorder::stobj_get_private(nanojit::LIns* obj_ins)
{
    return stobj_get_fslot_ptr(obj_ins, JSSLOT_PRIVATE);
}

LIns*
TraceRecorder::stobj_get_proto(nanojit::LIns* obj_ins)
{
    return lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, proto), ACCSET_OTHER);
}

LIns*
TraceRecorder::is_string_id(LIns *id_ins)
{
    return lir->insEqP_0(lir->ins2(LIR_andp, id_ins, INS_CONSTWORD(JSID_TYPE_MASK)));
}

LIns *
TraceRecorder::unbox_string_id(LIns *id_ins)
{
    JS_STATIC_ASSERT(JSID_TYPE_STRING == 0);
    return id_ins;
}

LIns *
TraceRecorder::unbox_int_id(LIns *id_ins)
{
    return lir->ins2ImmI(LIR_rshi, p2i(id_ins), 1);
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getThis(LIns*& this_ins)
{
    JSStackFrame *fp = cx->fp();
    JS_ASSERT_IF(fp->argv, fp->argv[-1] == fp->getThisValue());

    if (!fp->hasFunction()) {
        
        
        
        

        JS_ASSERT(!fp->argv);
        JS_ASSERT(!fp->getThisValue().isPrimitive());

#ifdef DEBUG
        JSObject *obj = globalObj->thisObject(cx);
        if (!obj)
            RETURN_ERROR("thisObject hook failed");
        JS_ASSERT(fp->getThisValue().toObjectOrNull() == obj);
#endif

        this_ins = INS_CONSTOBJ(fp->getThisValue().toObjectOrNull());
        return RECORD_CONTINUE;
    }

    Value& thisv = fp->argv[-1];
    JS_ASSERT(thisv == fp->getThisValue() || fp->getThisValue().isNull());

    JS_ASSERT(fp->callee()->getGlobal() == globalObj);

    if (!thisv.isNull()) {
        




        this_ins = get(&fp->argv[-1]);
        return RECORD_CONTINUE;
    }

    




    JSObject *obj = fp->getThisObject(cx);
    if (!obj)
        RETURN_ERROR("getThisObject failed");
    JS_ASSERT(fp->argv[-1] == ObjectOrNullValue(obj));
    this_ins = INS_CONSTOBJ(obj);
    set(&fp->argv[-1], this_ins);
    return RECORD_CONTINUE;
}


JS_REQUIRES_STACK void
TraceRecorder::guardClassHelper(bool cond, LIns* obj_ins, Class* clasp, VMSideExit* exit,
                                LoadQual loadQual)
{
    LIns* class_ins =
        lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, clasp), ACCSET_OTHER, loadQual);

#ifdef JS_JIT_SPEW
    char namebuf[32];
    JS_snprintf(namebuf, sizeof namebuf, "guard(class is %s)", clasp->name);
#else
    static const char namebuf[] = "";
#endif
    guard(cond, addName(lir->ins2(LIR_eqp, class_ins, INS_CONSTPTR(clasp)), namebuf), exit);
}

JS_REQUIRES_STACK void
TraceRecorder::guardClass(LIns* obj_ins, Class* clasp, VMSideExit* exit, LoadQual loadQual)
{
    guardClassHelper(true, obj_ins, clasp, exit, loadQual);
}

JS_REQUIRES_STACK void
TraceRecorder::guardNotClass(LIns* obj_ins, Class* clasp, VMSideExit* exit, LoadQual loadQual)
{
    guardClassHelper(false, obj_ins, clasp, exit, loadQual);
}

JS_REQUIRES_STACK void
TraceRecorder::guardDenseArray(LIns* obj_ins, ExitType exitType)
{
    guardClass(obj_ins, &js_ArrayClass, snapshot(exitType), LOAD_NORMAL);
}

JS_REQUIRES_STACK void
TraceRecorder::guardDenseArray(LIns* obj_ins, VMSideExit* exit)
{
    guardClass(obj_ins, &js_ArrayClass, exit, LOAD_NORMAL);
}

JS_REQUIRES_STACK bool
TraceRecorder::guardHasPrototype(JSObject* obj, LIns* obj_ins,
                                 JSObject** pobj, LIns** pobj_ins,
                                 VMSideExit* exit)
{
    *pobj = obj->getProto();
    *pobj_ins = stobj_get_proto(obj_ins);

    bool cond = *pobj == NULL;
    guard(cond, addName(lir->insEqP_0(*pobj_ins), "guard(proto-not-null)"), exit);
    return !cond;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::guardPrototypeHasNoIndexedProperties(JSObject* obj, LIns* obj_ins, ExitType exitType)
{
    



    VMSideExit* exit = snapshot(exitType);

    if (js_PrototypeHasIndexedProperties(cx, obj))
        return RECORD_STOP;

    while (guardHasPrototype(obj, obj_ins, &obj, &obj_ins, exit))
        CHECK_STATUS(guardShape(obj_ins, obj, obj->shape(), "guard(shape)", exit));
    return RECORD_CONTINUE;
}





JS_REQUIRES_STACK RecordingStatus
TraceRecorder::guardNativeConversion(Value& v)
{
    JSObject* obj = &v.toObject();
    LIns* obj_ins = get(&v);

    ConvertOp convert = obj->getClass()->convert;
    if (convert != Valueify(JS_ConvertStub) && convert != js_TryValueOf)
        RETURN_STOP("operand has convert hook");

    VMSideExit* exit = snapshot(BRANCH_EXIT);
    if (obj->isNative()) {
        
        
        
        CHECK_STATUS(guardShape(obj_ins, obj, obj->shape(),
                                "guardNativeConversion", exit));
    } else {
        
        
        guardClass(obj_ins, obj->getClass(), snapshot(MISMATCH_EXIT), LOAD_NORMAL);
    }
    return RECORD_CONTINUE;
}




JS_REQUIRES_STACK void
TraceRecorder::clearFrameSlotsFromTracker(Tracker& which, JSStackFrame* fp, unsigned nslots)
{
    






    Value* vp;
    Value* vpstop;

    




    if (fp->argv) {
        vp = &fp->argv[-2];
        vpstop = &fp->argv[argSlots(fp)];
        while (vp < vpstop)
            which.set(vp++, (LIns*)0);
        which.set(fp->addressArgsObj(), (LIns*)0);
        which.set(fp->addressScopeChain(), (LIns*)0);
    }
    vp = &fp->slots()[0];
    vpstop = &fp->slots()[nslots];
    while (vp < vpstop)
        which.set(vp++, (LIns*)0);
}

JS_REQUIRES_STACK JSStackFrame*
TraceRecorder::entryFrame() const
{
    JSStackFrame *fp = cx->fp();
    for (unsigned i = 0; i < callDepth; ++i)
        fp = fp->down;
    return fp;
}

JS_REQUIRES_STACK void
TraceRecorder::clearEntryFrameSlotsFromTracker(Tracker& which)
{
    JSStackFrame *fp = entryFrame();

    
    clearFrameSlotsFromTracker(which, fp, fp->getFixedCount());
}

JS_REQUIRES_STACK void
TraceRecorder::clearCurrentFrameSlotsFromTracker(Tracker& which)
{
    
    clearFrameSlotsFromTracker(which, cx->fp(), cx->fp()->getSlotCount());
}






JS_REQUIRES_STACK void
TraceRecorder::putActivationObjects()
{
    JSStackFrame *fp = cx->fp();

    bool have_args = fp->hasArgsObj() &&
        !fp->getArgsObj()->isStrictArguments() &&
        fp->numActualArgs() > 0;
    bool have_call = fp->hasFunction() &&
        JSFUN_HEAVYWEIGHT_TEST(fp->getFunction()->flags) &&
        fp->getFunction()->countArgsAndVars();

    if (!have_args && !have_call)
        return;

    int nargs = have_args ? argSlots(fp) : fp->numFormalArgs();

    LIns* args_ins;
    if (nargs) {
        args_ins = lir->insAlloc(sizeof(Value) * nargs);
        for (int i = 0; i < nargs; ++i) {
            box_value_into(fp->argv[i], get(&fp->argv[i]), args_ins, i * sizeof(Value),
                           ACCSET_OTHER);
        }
    } else {
        args_ins = INS_CONSTPTR(0);
    }

    if (have_args) {
        LIns* argsobj_ins = getFrameObjPtr(fp->addressArgsObj());
        LIns* args[] = { args_ins, argsobj_ins, cx_ins };
        lir->insCall(&js_PutArguments_ci, args);
    }

    if (have_call) {
        int nslots = fp->getFunction()->countVars();
        LIns* slots_ins;
        if (nslots) {
            slots_ins = lir->insAlloc(sizeof(Value) * nslots);
            for (int i = 0; i < nslots; ++i) {
                box_value_into(fp->slots()[i], get(&fp->slots()[i]), slots_ins,
                               i * sizeof(Value), ACCSET_OTHER);
            }
        } else {
            slots_ins = INS_CONSTPTR(0);
        }

        LIns* scopeChain_ins = getFrameObjPtr(fp->addressScopeChain());
        LIns* args[] = { slots_ins, INS_CONST(nslots), args_ins,
                         INS_CONST(fp->numFormalArgs()), scopeChain_ins, cx_ins };
        lir->insCall(&js_PutCallObjectOnTrace_ci, args);
    }
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_EnterFrame()
{
    JSStackFrame* const fp = cx->fp();

    if (++callDepth >= MAX_CALLDEPTH)
        RETURN_STOP_A("exceeded maximum call depth");

    debug_only_printf(LC_TMTracer, "EnterFrame %s, callDepth=%d\n",
                      js_AtomToPrintableString(cx, cx->fp()->getFunction()->atom),
                      callDepth);
    debug_only_stmt(
        if (LogController.lcbits & LC_TMRecorder) {
            js_Disassemble(cx, cx->fp()->getScript(), JS_TRUE, stdout);
            debug_only_print0(LC_TMTracer, "----\n");
        }
    )
    LIns* void_ins = INS_UNDEFINED();

    
    
    
    
    
    
    
    
    
    
    
    

    Value* vp = &fp->argv[fp->numActualArgs()];
    Value* vpstop = vp + ptrdiff_t(fp->numFormalArgs()) - ptrdiff_t(fp->numActualArgs());
    for (; vp < vpstop; ++vp) {
        nativeFrameTracker.set(vp, NULL);
        set(vp, void_ins);
    }

    nativeFrameTracker.set(fp->addressArgsObj(), NULL);
    setFrameObjPtr(fp->addressArgsObj(), INS_NULL());
    nativeFrameTracker.set(fp->addressScopeChain(), NULL);

    vp = fp->slots();
    vpstop = vp + fp->getFixedCount();
    for (; vp < vpstop; ++vp) {
        nativeFrameTracker.set(vp, NULL);
        set(vp, void_ins);
    }

    vp = vpstop;
    vpstop = vp + (fp->getSlotCount() - fp->getFixedCount());
    for (; vp < vpstop; ++vp)
        nativeFrameTracker.set(vp, NULL);

    LIns* callee_ins = get(&cx->fp()->argv[-2]);
    LIns* scopeChain_ins = stobj_get_parent(callee_ins);

    if (cx->fp()->hasFunction() && JSFUN_HEAVYWEIGHT_TEST(cx->fp()->getFunction()->flags)) {
        
        
        setFrameObjPtr(fp->addressScopeChain(), INS_NULL());

        if (js_IsNamedLambda(cx->fp()->getFunction()))
            RETURN_STOP_A("can't call named lambda heavyweight on trace");

        LIns* fun_ins = INS_CONSTPTR(cx->fp()->getFunction());

        LIns* args[] = { scopeChain_ins, callee_ins, fun_ins, cx_ins };
        LIns* call_ins = lir->insCall(&js_CreateCallObjectOnTrace_ci, args);
        guard(false, lir->insEqP_0(call_ins), snapshot(OOM_EXIT));

        setFrameObjPtr(fp->addressScopeChain(), call_ins);
    } else {
        setFrameObjPtr(fp->addressScopeChain(), scopeChain_ins);
    }

    
    if (fp->getScript() == fp->down->getScript() &&
        fp->down->down && fp->down->down->getScript() == fp->getScript()) {
        RETURN_STOP_A("recursion started inlining");
    }

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_LeaveFrame()
{
    debug_only_stmt(
        if (cx->fp()->hasFunction())
            debug_only_printf(LC_TMTracer,
                              "LeaveFrame (back to %s), callDepth=%d\n",
                              js_AtomToPrintableString(cx, cx->fp()->getFunction()->atom),
                              callDepth);
        );

    JS_ASSERT(js_CodeSpec[js_GetOpcode(cx, cx->fp()->getScript(),
              cx->regs->pc)].length == JSOP_CALL_LENGTH);

    if (callDepth-- <= 0)
        RETURN_STOP_A("returned out of a loop we started tracing");

    
    
    updateAtoms();
    set(&stackval(-1), rval_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_PUSH()
{
    stack(0, INS_UNDEFINED());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_POPV()
{
    Value& rval = stackval(-1);

    
    
    
    LIns *fp_ins = entryFrameIns();
    box_value_into(rval, get(&rval), fp_ins, JSStackFrame::offsetReturnValue(), ACCSET_OTHER);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ENTERWITH()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LEAVEWITH()
{
    return ARECORD_STOP;
}

#ifdef MOZ_TRACE_JSCALLS





static JSBool JS_FASTCALL
functionProbe(JSContext *cx, JSFunction *fun, JSBool enter)
{
    cx->doFunctionCallback(fun, FUN_SCRIPT(fun), enter);
    return true;
}

JS_DEFINE_CALLINFO_3(static, BOOL, functionProbe, CONTEXT, FUNCTION, BOOL, 0, 0)
#endif

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_RETURN()
{
    
    if (callDepth == 0) {
        AUDIT(returnLoopExits);
        return endLoop();
    }

    putActivationObjects();

#ifdef MOZ_TRACE_JSCALLS
    if (cx->functionCallback) {
        LIns* args[] = { INS_CONST(0), INS_CONSTPTR(cx->fp()->getFunction()), cx_ins };
        LIns* call_ins = lir->insCall(&functionProbe_ci, args);
        guard(false, lir->insEqI_0(call_ins), MISMATCH_EXIT);
    }
#endif

    
    Value& rval = stackval(-1);
    JSStackFrame *fp = cx->fp();
    if ((fp->flags & JSFRAME_CONSTRUCTING) && rval.isPrimitive()) {
        JS_ASSERT(fp->getThisValue() == fp->argv[-1]);
        rval_ins = get(&fp->argv[-1]);
    } else {
        rval_ins = get(&rval);
    }
    debug_only_printf(LC_TMTracer,
                      "returning from %s\n",
                      js_AtomToPrintableString(cx, fp->getFunction()->atom));
    clearCurrentFrameSlotsFromTracker(nativeFrameTracker);

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GOTO()
{
    




    jssrcnote* sn = js_GetSrcNote(cx->fp()->getScript(), cx->regs->pc);

    if (sn && (SN_TYPE(sn) == SRC_BREAK || SN_TYPE(sn) == SRC_CONT2LABEL)) {
        AUDIT(breakLoopExits);
        return endLoop();
    }
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IFEQ()
{
    trackCfgMerges(cx->regs->pc);
    return ifop();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IFNE()
{
    return ifop();
}

LIns*
TraceRecorder::newArguments(LIns* callee_ins, bool strict)
{
    LIns* global_ins = INS_CONSTOBJ(globalObj);
    LIns* argc_ins = INS_CONST(cx->fp()->numActualArgs());

    LIns* args[] = { callee_ins, argc_ins, global_ins, cx_ins };
    LIns* argsobj_ins = lir->insCall(&js_Arguments_ci, args);
    guard(false, lir->insEqP_0(argsobj_ins), OOM_EXIT);

    if (strict) {
        JSStackFrame* fp = cx->fp();
        uintN argc = fp->numActualArgs();
        LIns* argsData_ins = stobj_get_const_private_ptr(argsobj_ins, JSObject::JSSLOT_ARGS_DATA);

        for (uintN i = 0; i < argc; i++) {
            box_value_into(fp->argv[i], get(&fp->argv[i]), argsData_ins,
                           offsetof(ArgumentsData, slots) + i * sizeof(Value),
                           ACCSET_OTHER);
        }
    }

    return argsobj_ins;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARGUMENTS()
{
    JSStackFrame* const fp = cx->fp();

    
    JS_ASSERT(!(fp->flags & JSFRAME_EVAL));

    if (fp->flags & JSFRAME_OVERRIDE_ARGS)
        RETURN_STOP_A("Can't trace |arguments| if |arguments| is assigned to");

    LIns* a_ins = getFrameObjPtr(fp->addressArgsObj());
    LIns* args_ins;
    LIns* callee_ins = get(&fp->argv[-2]);
    bool strict = fp->getFunction()->inStrictMode();
    if (a_ins->isImmP()) {
        
        args_ins = newArguments(callee_ins, strict);
    } else {
        

        LIns* mem_ins = lir->insAlloc(sizeof(JSObject *));

        LIns* br1 = lir->insBranch(LIR_jt, lir->insEqP_0(a_ins), NULL);
        lir->insStore(a_ins, mem_ins, 0, ACCSET_OTHER);
        LIns* br2 = lir->insBranch(LIR_j, NULL, NULL);

        LIns* label1 = lir->ins0(LIR_label);
        br1->setTarget(label1);

        LIns* call_ins = newArguments(callee_ins, strict);
        lir->insStore(call_ins, mem_ins, 0, ACCSET_OTHER);

        LIns* label2 = lir->ins0(LIR_label);
        br2->setTarget(label2);

        args_ins = lir->insLoad(LIR_ldp, mem_ins, 0, ACCSET_OTHER);
    }

    stack(0, args_ins);
    setFrameObjPtr(fp->addressArgsObj(), args_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DUP()
{
    stack(0, get(&stackval(-1)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DUP2()
{
    stack(0, get(&stackval(-2)));
    stack(1, get(&stackval(-1)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SWAP()
{
    Value& l = stackval(-2);
    Value& r = stackval(-1);
    LIns* l_ins = get(&l);
    LIns* r_ins = get(&r);
    set(&r, l_ins);
    set(&l, r_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_PICK()
{
    Value* sp = cx->regs->sp;
    jsint n = cx->regs->pc[1];
    JS_ASSERT(sp - (n+1) >= cx->fp()->base());
    LIns* top = get(sp - (n+1));
    for (jsint i = 0; i < n; ++i)
        set(sp - (n+1) + i, get(sp - n + i));
    set(&sp[-1], top);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETCONST()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BITOR()
{
    return InjectStatus(binary(LIR_ori));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BITXOR()
{
    return InjectStatus(binary(LIR_xori));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BITAND()
{
    return InjectStatus(binary(LIR_andi));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_EQ()
{
    return equality(false, true);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NE()
{
    return equality(true, true);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LT()
{
    return relational(LIR_ltd, true);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LE()
{
    return relational(LIR_led, true);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GT()
{
    return relational(LIR_gtd, true);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GE()
{
    return relational(LIR_ged, true);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LSH()
{
    return InjectStatus(binary(LIR_lshi));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_RSH()
{
    return InjectStatus(binary(LIR_rshi));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_URSH()
{
    return InjectStatus(binary(LIR_rshui));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ADD()
{
    Value& r = stackval(-1);
    Value& l = stackval(-2);

    if (!l.isPrimitive()) {
        CHECK_STATUS_A(guardNativeConversion(l));
        if (!r.isPrimitive()) {
            CHECK_STATUS_A(guardNativeConversion(r));
            return InjectStatus(callImacro(add_imacros.obj_obj));
        }
        return InjectStatus(callImacro(add_imacros.obj_any));
    }
    if (!r.isPrimitive()) {
        CHECK_STATUS_A(guardNativeConversion(r));
        return InjectStatus(callImacro(add_imacros.any_obj));
    }

    if (l.isString() || r.isString()) {
        LIns* args[] = { stringify(r), stringify(l), cx_ins };
        LIns* concat = lir->insCall(&js_ConcatStrings_ci, args);
        guard(false, lir->insEqP_0(concat), OOM_EXIT);
        set(&l, concat);
        return ARECORD_CONTINUE;
    }

    return InjectStatus(binary(LIR_addd));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SUB()
{
    return InjectStatus(binary(LIR_subd));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_MUL()
{
    return InjectStatus(binary(LIR_muld));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DIV()
{
    return InjectStatus(binary(LIR_divd));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_MOD()
{
    return InjectStatus(binary(LIR_modd));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NOT()
{
    Value& v = stackval(-1);
    if (v.isBoolean() || v.isUndefined()) {
        set(&v, lir->insEqI_0(lir->ins2ImmI(LIR_eqi, get(&v), 1)));
        return ARECORD_CONTINUE;
    }
    if (v.isNumber()) {
        LIns* v_ins = get(&v);
        set(&v, lir->ins2(LIR_ori, lir->ins2(LIR_eqd, v_ins, lir->insImmD(0)),
                                  lir->insEqI_0(lir->ins2(LIR_eqd, v_ins, v_ins))));
        return ARECORD_CONTINUE;
    }
    if (v.isObjectOrNull()) {
        set(&v, lir->insEqP_0(get(&v)));
        return ARECORD_CONTINUE;
    }
    JS_ASSERT(v.isString());
    set(&v, lir->insEqP_0(getStringLength(get(&v))));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BITNOT()
{
    return InjectStatus(unary(LIR_noti));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NEG()
{
    Value& v = stackval(-1);

    if (!v.isPrimitive()) {
        CHECK_STATUS_A(guardNativeConversion(v));
        return InjectStatus(callImacro(unary_imacros.sign));
    }

    if (v.isNumber()) {
        LIns* a = get(&v);

        




        if (oracle &&
            !oracle->isInstructionUndemotable(cx->regs->pc) &&
            isPromoteInt(a) &&
            (!v.isInt32() || v.toInt32() != 0) &&
            (!v.isDouble() || v.toDouble() != 0) &&
            -v.toNumber() == (int)-v.toNumber())
        {
            VMSideExit* exit = snapshot(OVERFLOW_EXIT);
            a = guard_xov(LIR_subi, lir->insImmI(0), demote(lir, a), exit);
            if (!a->isImmI() && a->isop(LIR_subxovi)) {
                guard(false, lir->ins2ImmI(LIR_eqi, a, 0), exit); 
            }
            a = lir->ins1(LIR_i2d, a);
        } else {
            a = lir->ins1(LIR_negd, a);
        }

        set(&v, a);
        return ARECORD_CONTINUE;
    }

    if (v.isNull()) {
        set(&v, lir->insImmD(-0.0));
        return ARECORD_CONTINUE;
    }

    if (v.isUndefined()) {
        set(&v, lir->insImmD(js_NaN));
        return ARECORD_CONTINUE;
    }

    if (v.isString()) {
        LIns* args[] = { get(&v), cx_ins };
        set(&v, lir->ins1(LIR_negd,
                          lir->insCall(&js_StringToNumber_ci, args)));
        return ARECORD_CONTINUE;
    }

    JS_ASSERT(v.isBoolean());
    set(&v, lir->ins1(LIR_negd, i2d(get(&v))));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_POS()
{
    Value& v = stackval(-1);

    if (!v.isPrimitive()) {
        CHECK_STATUS_A(guardNativeConversion(v));
        return InjectStatus(callImacro(unary_imacros.sign));
    }

    if (v.isNumber())
        return ARECORD_CONTINUE;

    if (v.isNull()) {
        set(&v, lir->insImmD(0));
        return ARECORD_CONTINUE;
    }
    if (v.isUndefined()) {
        set(&v, lir->insImmD(js_NaN));
        return ARECORD_CONTINUE;
    }

    if (v.isString()) {
        LIns* args[] = { get(&v), cx_ins };
        set(&v, lir->insCall(&js_StringToNumber_ci, args));
        return ARECORD_CONTINUE;
    }

    JS_ASSERT(v.isBoolean());
    set(&v, i2d(get(&v)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_PRIMTOP()
{
    
    
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_OBJTOP()
{
    Value& v = stackval(-1);
    RETURN_IF_XML_A(v);
    return ARECORD_CONTINUE;
}

RecordingStatus
TraceRecorder::getClassPrototype(JSObject* ctor, LIns*& proto_ins)
{
    
#ifdef DEBUG
    Class *clasp = FUN_CLASP(GET_FUNCTION_PRIVATE(cx, ctor));
    JS_ASSERT(clasp);

    TraceMonitor &localtm = JS_TRACE_MONITOR(cx);
#endif

    Value pval;
    if (!ctor->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &pval))
        RETURN_ERROR("error getting prototype from constructor");

    
    
    JS_ASSERT(localtm.recorder);

#ifdef DEBUG
    JSBool ok, found;
    uintN attrs;
    ok = JS_GetPropertyAttributes(cx, ctor, js_class_prototype_str, &attrs, &found);
    JS_ASSERT(ok);
    JS_ASSERT(found);
    JS_ASSERT((~attrs & (JSPROP_READONLY | JSPROP_PERMANENT)) == 0);
#endif

    
    
    JS_ASSERT(!pval.isPrimitive());
    JSObject *proto = &pval.toObject();
    JS_ASSERT_IF(clasp != &js_ArrayClass, proto->emptyShape->getClass() == clasp);

    proto_ins = INS_CONSTOBJ(proto);
    return RECORD_CONTINUE;
}

RecordingStatus
TraceRecorder::getClassPrototype(JSProtoKey key, LIns*& proto_ins)
{
#ifdef DEBUG
    TraceMonitor &localtm = JS_TRACE_MONITOR(cx);
#endif

    JSObject* proto;
    if (!js_GetClassPrototype(cx, globalObj, key, &proto))
        RETURN_ERROR("error in js_GetClassPrototype");

    
    JS_ASSERT(localtm.recorder);

#ifdef DEBUG
    
    if (key != JSProto_Array) {
        JS_ASSERT(proto->isNative());
        EmptyShape *empty = proto->emptyShape;
        JS_ASSERT(empty);
        JS_ASSERT(JSCLASS_CACHED_PROTO_KEY(empty->getClass()) == key);
    }
#endif

    proto_ins = INS_CONSTOBJ(proto);
    return RECORD_CONTINUE;
}

#define IGNORE_NATIVE_CALL_COMPLETE_CALLBACK ((JSSpecializedNative*)1)

RecordingStatus
TraceRecorder::newString(JSObject* ctor, uint32 argc, Value* argv, Value* rval)
{
    JS_ASSERT(argc == 1);

    if (!argv[0].isPrimitive()) {
        CHECK_STATUS(guardNativeConversion(argv[0]));
        return callImacro(new_imacros.String);
    }

    LIns* proto_ins;
    CHECK_STATUS(getClassPrototype(ctor, proto_ins));

    LIns* args[] = { stringify(argv[0]), proto_ins, cx_ins };
    LIns* obj_ins = lir->insCall(&js_String_tn_ci, args);
    guard(false, lir->insEqP_0(obj_ins), OOM_EXIT);

    set(rval, obj_ins);
    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
    return RECORD_CONTINUE;
}

RecordingStatus
TraceRecorder::newArray(JSObject* ctor, uint32 argc, Value* argv, Value* rval)
{
    LIns *proto_ins;
    CHECK_STATUS(getClassPrototype(ctor, proto_ins));

    LIns *arr_ins;
    if (argc == 0 || (argc == 1 && argv[0].isNumber())) {
        LIns *args[] = { argc == 0 ? lir->insImmI(0) : d2i(get(argv)), proto_ins, cx_ins };
        arr_ins = lir->insCall(&js_NewEmptyArray_ci, args);
        guard(false, lir->insEqP_0(arr_ins), OOM_EXIT);
    } else {
        LIns *args[] = { INS_CONST(argc), proto_ins, cx_ins };
        arr_ins = lir->insCall(&js_NewPreallocatedArray_ci, args);
        guard(false, lir->insEqP_0(arr_ins), OOM_EXIT);

        
        LIns *dslots_ins = NULL;
        for (uint32 i = 0; i < argc && !outOfMemory(); i++) {
            stobj_set_dslot(arr_ins, i, dslots_ins, argv[i], get(&argv[i]));
        }
    }

    set(rval, arr_ins);
    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK void
TraceRecorder::propagateFailureToBuiltinStatus(LIns* ok_ins, LIns*& status_ins)
{
    









    JS_STATIC_ASSERT(((JS_TRUE & 1) ^ 1) << 1 == 0);
    JS_STATIC_ASSERT(((JS_FALSE & 1) ^ 1) << 1 == BUILTIN_ERROR);
    status_ins = lir->ins2(LIR_ori,
                           status_ins,
                           lir->ins2ImmI(LIR_lshi,
                                      lir->ins2ImmI(LIR_xori,
                                                 lir->ins2ImmI(LIR_andi, ok_ins, 1),
                                                 1),
                                      1));
    lir->insStore(status_ins, lirbuf->state, (int) offsetof(TracerState, builtinStatus),
                  ACCSET_OTHER);
}

JS_REQUIRES_STACK void
TraceRecorder::emitNativePropertyOp(const Shape* shape, LIns* obj_ins,
                                    bool setflag, LIns* addr_boxed_val_ins)
{
    JS_ASSERT(addr_boxed_val_ins->isop(LIR_allocp));
    JS_ASSERT(setflag ? !shape->hasSetterValue() : !shape->hasGetterValue());
    JS_ASSERT(setflag ? !shape->hasDefaultSetter() : !shape->hasDefaultGetterOrIsMethod());

    enterDeepBailCall();

    lir->insStore(addr_boxed_val_ins, lirbuf->state, offsetof(TracerState, nativeVp), ACCSET_OTHER);
    lir->insStore(INS_CONST(1), lirbuf->state, offsetof(TracerState, nativeVpLen), ACCSET_OTHER);

    CallInfo* ci = new (traceAlloc()) CallInfo();
    ci->_address = uintptr_t(setflag ? shape->setterOp() : shape->getterOp());
    ci->_typesig = CallInfo::typeSig4(ARGTYPE_I, ARGTYPE_P, ARGTYPE_P, ARGTYPE_P, ARGTYPE_P);
    ci->_isPure = 0;
    ci->_storeAccSet = ACCSET_STORE_ANY;
    ci->_abi = ABI_CDECL;
#ifdef DEBUG
    ci->_name = "JSPropertyOp";
#endif
    LIns* args[] = { addr_boxed_val_ins, INS_CONSTID(SHAPE_USERID(shape)), obj_ins, cx_ins };
    LIns* ok_ins = lir->insCall(ci, args);

    
    lir->insStore(INS_NULL(), lirbuf->state, offsetof(TracerState, nativeVp), ACCSET_OTHER);
    leaveDeepBailCall();

    
    
    
    
    LIns* status_ins = lir->insLoad(LIR_ldi, lirbuf->state,
                                    (int) offsetof(TracerState, builtinStatus), ACCSET_OTHER);
    propagateFailureToBuiltinStatus(ok_ins, status_ins);
    guard(true, lir->insEqI_0(status_ins), STATUS_EXIT);
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::emitNativeCall(JSSpecializedNative* sn, uintN argc, LIns* args[], bool rooted)
{
    bool constructing = !!(sn->flags & JSTN_CONSTRUCTOR);

    if (JSTN_ERRTYPE(sn) == FAIL_STATUS) {
        
        
        JS_ASSERT(!pendingSpecializedNative);

        
        
        
        VMSideExit* exit = enterDeepBailCall();
        JSObject* funobj = &stackval(0 - (2 + argc)).toObject();
        if (FUN_SLOW_NATIVE(GET_FUNCTION_PRIVATE(cx, funobj))) {
            exit->setNativeCallee(funobj, constructing);
            tree->gcthings.addUnique(ObjectValue(*funobj));
        }
    }

    LIns* res_ins = lir->insCall(sn->builtin, args);

    
    if (rooted)
        lir->insStore(INS_NULL(), lirbuf->state, offsetof(TracerState, nativeVp), ACCSET_OTHER);

    rval_ins = res_ins;
    switch (JSTN_ERRTYPE(sn)) {
      case FAIL_NULL:
        guard(false, lir->insEqP_0(res_ins), OOM_EXIT);
        break;
      case FAIL_NEG:
        res_ins = lir->ins1(LIR_i2d, res_ins);
        guard(false, lir->ins2(LIR_ltd, res_ins, lir->insImmD(0)), OOM_EXIT);
        break;
      case FAIL_NEITHER:
          guard(false, lir->ins2ImmI(LIR_eqi, res_ins, JS_NEITHER), OOM_EXIT);
        break;
      default:;
    }

    set(&stackval(0 - (2 + argc)), res_ins);

    




    pendingSpecializedNative = sn;

    return RECORD_CONTINUE;
}





JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callSpecializedNative(JSNativeTraceInfo *trcinfo, uintN argc,
                                     bool constructing)
{
    JSStackFrame* const fp = cx->fp();
    jsbytecode *pc = cx->regs->pc;

    Value& fval = stackval(0 - (2 + argc));
    Value& tval = stackval(0 - (1 + argc));

    LIns* this_ins = get(&tval);

    LIns* args[nanojit::MAXARGS];
    JSSpecializedNative *sn = trcinfo->specializations;
    JS_ASSERT(sn);
    do {
        if (((sn->flags & JSTN_CONSTRUCTOR) != 0) != constructing)
            continue;

        uintN knownargc = strlen(sn->argtypes);
        if (argc != knownargc)
            continue;

        intN prefixc = strlen(sn->prefix);
        JS_ASSERT(prefixc <= 3);
        LIns** argp = &args[argc + prefixc - 1];
        char argtype;

#if defined DEBUG
        memset(args, 0xCD, sizeof(args));
#endif

        uintN i;
        for (i = prefixc; i--; ) {
            argtype = sn->prefix[i];
            if (argtype == 'C') {
                *argp = cx_ins;
            } else if (argtype == 'T') { 
                if (tval.isPrimitive())
                    goto next_specialization;
                *argp = this_ins;
            } else if (argtype == 'S') { 
                if (!tval.isString())
                    goto next_specialization;
                *argp = this_ins;
            } else if (argtype == 'f') {
                *argp = INS_CONSTOBJ(&fval.toObject());
            } else if (argtype == 'p') {
                CHECK_STATUS(getClassPrototype(&fval.toObject(), *argp));
            } else if (argtype == 'R') {
                *argp = INS_CONSTPTR(cx->runtime);
            } else if (argtype == 'P') {
                
                
                if ((*pc == JSOP_CALL) &&
                    fp->hasIMacroPC() && *fp->getIMacroPC() == JSOP_GETELEM)
                    *argp = INS_CONSTPTR(fp->getIMacroPC());
                else
                    *argp = INS_CONSTPTR(pc);
            } else if (argtype == 'D') { 
                if (!tval.isNumber())
                    goto next_specialization;
                *argp = this_ins;
            } else {
                JS_NOT_REACHED("unknown prefix arg type");
            }
            argp--;
        }

        for (i = knownargc; i--; ) {
            Value& arg = stackval(0 - (i + 1));
            *argp = get(&arg);

            argtype = sn->argtypes[i];
            if (argtype == 'd' || argtype == 'i') {
                if (!arg.isNumber())
                    goto next_specialization;
                if (argtype == 'i')
                    *argp = d2i(*argp);
            } else if (argtype == 'o') {
                if (arg.isPrimitive())
                    goto next_specialization;
            } else if (argtype == 's') {
                if (!arg.isString())
                    goto next_specialization;
            } else if (argtype == 'r') {
                if (!VALUE_IS_REGEXP(cx, arg))
                    goto next_specialization;
            } else if (argtype == 'f') {
                if (!IsFunctionObject(arg))
                    goto next_specialization;
            } else if (argtype == 'v') {
                *argp = box_value_for_native_call(arg, *argp);
            } else {
                goto next_specialization;
            }
            argp--;
        }
#if defined DEBUG
        JS_ASSERT(args[0] != (LIns *)0xcdcdcdcd);
#endif
        return emitNativeCall(sn, argc, args, false);

next_specialization:;
    } while ((sn++)->flags & JSTN_MORE);

    return RECORD_STOP;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callNative(uintN argc, JSOp mode)
{
    LIns* args[5];

    JS_ASSERT(mode == JSOP_CALL || mode == JSOP_NEW || mode == JSOP_APPLY);

    Value* vp = &stackval(0 - (2 + argc));
    JSObject* funobj = &vp[0].toObject();
    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, funobj);
    FastNative native = (FastNative)fun->u.n.native;

    switch (argc) {
      case 1:
        if (vp[2].isNumber() && mode == JSOP_CALL) {
            if (native == js_math_ceil || native == js_math_floor || native == js_math_round) {
                LIns* a = get(&vp[2]);
                if (isPromote(a)) {
                    set(&vp[0], a);
                    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                    return RECORD_CONTINUE;
                }
            }
            if (vp[1].isString()) {
                JSString *str = vp[1].toString();
                if (native == (FastNative)js_str_charAt) {
                    LIns* str_ins = get(&vp[1]);
                    LIns* idx_ins = get(&vp[2]);
                    set(&vp[0], getCharAt(str, str_ins, idx_ins, mode));
                    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                    return RECORD_CONTINUE;
                } else if (native == (FastNative)js_str_charCodeAt) {
                    jsdouble i = vp[2].toNumber();
                    if (i < 0 || i >= str->length())
                        RETURN_STOP("charCodeAt out of bounds");
                    LIns* str_ins = get(&vp[1]);
                    LIns* idx_ins = get(&vp[2]);
                    set(&vp[0], getCharCodeAt(str, str_ins, idx_ins));
                    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                    return RECORD_CONTINUE;
                }
            }
        }
        break;

      case 2:
        if (vp[2].isNumber() && vp[3].isNumber() && mode == JSOP_CALL &&
            (native == js_math_min || native == js_math_max)) {
            LIns* a = get(&vp[2]);
            LIns* b = get(&vp[3]);
            if (isPromote(a) && isPromote(b)) {
                a = demote(lir, a);
                b = demote(lir, b);
                set(&vp[0],
                    lir->ins1(LIR_i2d,
                              lir->insChoose(lir->ins2((native == js_math_min)
                                                        ? LIR_lti
                                                        : LIR_gti, a, b),
                                              a, b, avmplus::AvmCore::use_cmov())));
                pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                return RECORD_CONTINUE;
            }
        }
        break;
    }

    if (fun->flags & JSFUN_TRCINFO) {
        JSNativeTraceInfo *trcinfo = FUN_TRCINFO(fun);
        JS_ASSERT(trcinfo && (JSFastNative)fun->u.n.native == trcinfo->native);

        
        if (trcinfo->specializations) {
            RecordingStatus status = callSpecializedNative(trcinfo, argc, mode == JSOP_NEW);
            if (status != RECORD_STOP)
                return status;
        }
    }

    if (native == js_fun_apply || native == js_fun_call)
        RETURN_STOP("trying to call native apply or call");

    if (fun->u.n.extra > 0)
        RETURN_STOP("trying to trace slow native with fun->u.n.extra > 0");

    
    uintN vplen = 2 + JS_MAX(argc, unsigned(FUN_MINARGS(fun)));
    if (!(fun->flags & JSFUN_FAST_NATIVE))
        vplen++; 
    LIns* invokevp_ins = lir->insAlloc(vplen * sizeof(Value));

    
    box_value_into(vp[0], INS_CONSTOBJ(funobj), invokevp_ins, 0, ACCSET_OTHER);

    
    LIns* this_ins;
    if (mode == JSOP_NEW) {
        Class* clasp = fun->u.n.clasp;
        JS_ASSERT(clasp != &js_SlowArrayClass);
        if (!clasp)
            clasp = &js_ObjectClass;
        JS_ASSERT(((jsuword) clasp & 3) == 0);

        
        
        
        if (clasp == &js_FunctionClass)
            RETURN_STOP("new Function");

        if (!clasp->isNative())
            RETURN_STOP("new with non-native ops");

        if (fun->isFastConstructor()) {
            vp[1].setMagic(JS_FAST_CONSTRUCTOR);
            newobj_ins = INS_CONST(JS_FAST_CONSTRUCTOR);

            
            mode = JSOP_CALL;
        } else {
            args[0] = INS_CONSTOBJ(funobj);
            args[1] = INS_CONSTPTR(clasp);
            args[2] = cx_ins;
            newobj_ins = lir->insCall(&js_NewInstance_ci, args);
            guard(false, lir->insEqP_0(newobj_ins), OOM_EXIT);

            








            vp[1].setObject(*globalObj);
        }
        this_ins = newobj_ins;
    } else {
        this_ins = get(&vp[1]);

        





        if (!(fun->flags & JSFUN_FAST_NATIVE)) {
            if (vp[1].isNull()) {
                JSObject* thisObj = ComputeThisFromVp(cx, vp);
                if (!thisObj)
                    RETURN_ERROR("error in js_ComputeGlobalThis");
                this_ins = INS_CONSTOBJ(thisObj);
            } else if (!vp[1].isObject()) {
                RETURN_STOP("slow native(primitive, args)");
            } else {
                if (vp[1].toObject().hasClass(&js_WithClass))
                    RETURN_STOP("can't trace slow native invocation on With object");
                guardNotClass(this_ins, &js_WithClass, snapshot(MISMATCH_EXIT), LOAD_CONST);

                this_ins = lir->insChoose(lir->insEqP_0(stobj_get_parent(this_ins)),
                                           INS_CONSTOBJ(globalObj),
                                           this_ins, avmplus::AvmCore::use_cmov());
            }
        }
    }
    set(&vp[1], this_ins);
    box_value_into(vp[1], this_ins, invokevp_ins, 1 * sizeof(Value), ACCSET_OTHER);

    
    for (uintN n = 2; n < 2 + argc; n++) {
        box_value_into(vp[n], get(&vp[n]), invokevp_ins, n * sizeof(Value), ACCSET_OTHER);
        
        
        if (outOfMemory())
            RETURN_STOP("out of memory in argument list");
    }

    
    if (2 + argc < vplen) {
        for (uintN n = 2 + argc; n < vplen; n++) {
            box_undefined_into(invokevp_ins, n * sizeof(Value), ACCSET_OTHER);
            if (outOfMemory())
                RETURN_STOP("out of memory in extra slots");
        }
    }

    
    uint32 typesig;
    if (fun->flags & JSFUN_FAST_NATIVE) {
        if (mode == JSOP_NEW && !(fun->flags & JSFUN_FAST_NATIVE_CTOR))
            RETURN_STOP("untraceable fast native constructor");
        native_rval_ins = invokevp_ins;
        args[0] = invokevp_ins;
        args[1] = lir->insImmI(argc);
        args[2] = cx_ins;
        typesig = CallInfo::typeSig3(ARGTYPE_I, ARGTYPE_P, ARGTYPE_I, ARGTYPE_P);
    } else {
        int32_t offset = (vplen - 1) * sizeof(Value);
        native_rval_ins = lir->ins2(LIR_addp, invokevp_ins, INS_CONSTWORD(offset));
        args[0] = native_rval_ins;
        args[1] = lir->ins2(LIR_addp, invokevp_ins, INS_CONSTWORD(2 * sizeof(Value)));
        args[2] = lir->insImmI(argc);
        args[3] = this_ins;
        args[4] = cx_ins;
        typesig = CallInfo::typeSig5(ARGTYPE_I,
                                     ARGTYPE_P, ARGTYPE_P, ARGTYPE_I, ARGTYPE_P, ARGTYPE_P);
    }

    
    
    

    CallInfo* ci = new (traceAlloc()) CallInfo();
    ci->_address = uintptr_t(fun->u.n.native);
    ci->_isPure = 0;
    ci->_storeAccSet = ACCSET_STORE_ANY;
    ci->_abi = ABI_CDECL;
    ci->_typesig = typesig;
#ifdef DEBUG
    ci->_name = JS_GetFunctionName(fun);
 #endif

    
    generatedSpecializedNative.builtin = ci;
    generatedSpecializedNative.flags = FAIL_STATUS | ((mode == JSOP_NEW)
                                                        ? JSTN_CONSTRUCTOR
                                                        : JSTN_UNBOX_AFTER);
    generatedSpecializedNative.prefix = NULL;
    generatedSpecializedNative.argtypes = NULL;

    
    
    
    
    
    lir->insStore(INS_CONST(vplen), lirbuf->state, offsetof(TracerState, nativeVpLen), ACCSET_OTHER);
    lir->insStore(invokevp_ins, lirbuf->state, offsetof(TracerState, nativeVp), ACCSET_OTHER);

    
    
    return emitNativeCall(&generatedSpecializedNative, argc, args, true);
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::functionCall(uintN argc, JSOp mode)
{
    Value& fval = stackval(0 - (2 + argc));
    JS_ASSERT(&fval >= cx->fp()->base());

    if (!IsFunctionObject(fval))
        RETURN_STOP("callee is not a function");

    Value& tval = stackval(0 - (1 + argc));

    



    if (!get(&fval)->isImmP())
        CHECK_STATUS(guardCallee(fval));

    










    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, &fval.toObject());

#ifdef MOZ_TRACE_JSCALLS
    if (cx->functionCallback) {
        JSScript *script = FUN_SCRIPT(fun);
        if (! script || ! script->isEmpty()) {
            LIns* args[] = { INS_CONST(1), INS_CONSTPTR(fun), cx_ins };
            LIns* call_ins = lir->insCall(&functionProbe_ci, args);
            guard(false, lir->insEqI_0(call_ins), MISMATCH_EXIT);
        }
    }
#endif

    if (FUN_INTERPRETED(fun)) {
        if (mode == JSOP_NEW) {
            LIns* args[] = { get(&fval), INS_CONSTPTR(&js_ObjectClass), cx_ins };
            LIns* tv_ins = lir->insCall(&js_NewInstance_ci, args);
            guard(false, lir->insEqP_0(tv_ins), OOM_EXIT);
            set(&tval, tv_ins);
        }
        return interpretedFunctionCall(fval, fun, argc, mode == JSOP_NEW);
    }

    FastNative native = FUN_FAST_NATIVE(fun);
    Value* argv = &tval + 1;
    if (native == js_Array)
        return newArray(&fval.toObject(), argc, argv, &fval);
    if (native == js_String && argc == 1) {
        if (mode == JSOP_NEW)
            return newString(&fval.toObject(), 1, argv, &fval);
        if (!argv[0].isPrimitive()) {
            CHECK_STATUS(guardNativeConversion(argv[0]));
            return callImacro(call_imacros.String);
        }
        set(&fval, stringify(argv[0]));
        pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
        return RECORD_CONTINUE;
    }

    RecordingStatus rs = callNative(argc, mode);
#ifdef MOZ_TRACE_JSCALLS
    if (cx->functionCallback) {
        LIns* args[] = { INS_CONST(0), INS_CONSTPTR(fun), cx_ins };
        LIns* call_ins = lir->insCall(&functionProbe_ci, args);
        guard(false, lir->insEqI_0(call_ins), MISMATCH_EXIT);
    }
#endif
    return rs;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NEW()
{
    uintN argc = GET_ARGC(cx->regs->pc);
    cx->assertValidStackDepth(argc + 2);
    return InjectStatus(functionCall(argc, JSOP_NEW));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DELNAME()
{
    return ARECORD_STOP;
}

JSBool JS_FASTCALL
DeleteIntKey(JSContext* cx, JSObject* obj, int32 i)
{
    LeaveTraceIfGlobalObject(cx, obj);
    Value v = BooleanValue(false);
    jsid id = INT_TO_JSID(i);
    if (!obj->deleteProperty(cx, id, &v))
        SetBuiltinError(cx);
    return v.toBoolean();
}
JS_DEFINE_CALLINFO_3(extern, BOOL_FAIL, DeleteIntKey, CONTEXT, OBJECT, INT32, 0, ACCSET_STORE_ANY)

JSBool JS_FASTCALL
DeleteStrKey(JSContext* cx, JSObject* obj, JSString* str)
{
    LeaveTraceIfGlobalObject(cx, obj);
    Value v = BooleanValue(false);
    jsid id;

    




    if (!js_ValueToStringId(cx, StringValue(str), &id) || !obj->deleteProperty(cx, id, &v))
        SetBuiltinError(cx);
    return v.toBoolean();
}
JS_DEFINE_CALLINFO_3(extern, BOOL_FAIL, DeleteStrKey, CONTEXT, OBJECT, STRING, 0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DELPROP()
{
    Value& lval = stackval(-1);
    if (lval.isPrimitive())
        RETURN_STOP_A("JSOP_DELPROP on primitive base expression");
    if (&lval.toObject() == globalObj)
        RETURN_STOP_A("JSOP_DELPROP on global property");

    JSAtom* atom = atoms[GET_INDEX(cx->regs->pc)];

    enterDeepBailCall();
    LIns* args[] = { INS_ATOM(atom), get(&lval), cx_ins };
    LIns* rval_ins = lir->insCall(&DeleteStrKey_ci, args);

    LIns* status_ins = lir->insLoad(LIR_ldi,
                                    lirbuf->state,
                                    offsetof(TracerState, builtinStatus), ACCSET_OTHER);
    pendingGuardCondition = lir->insEqI_0(status_ins);
    leaveDeepBailCall();

    set(&lval, rval_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DELELEM()
{
    Value& lval = stackval(-2);
    if (lval.isPrimitive())
        RETURN_STOP_A("JSOP_DELELEM on primitive base expression");
    if (&lval.toObject() == globalObj)
        RETURN_STOP_A("JSOP_DELELEM on global property");

    Value& idx = stackval(-1);
    LIns* rval_ins;

    enterDeepBailCall();
    if (hasInt32Repr(idx)) {
        LIns* args[] = { makeNumberInt32(get(&idx)), get(&lval), cx_ins };
        rval_ins = lir->insCall(&DeleteIntKey_ci, args);
    } else if (idx.isString()) {
        LIns* args[] = { get(&idx), get(&lval), cx_ins };
        rval_ins = lir->insCall(&DeleteStrKey_ci, args);
    } else {
        RETURN_STOP_A("JSOP_DELELEM on non-int, non-string index");
    }

    LIns* status_ins = lir->insLoad(LIR_ldi,
                                    lirbuf->state,
                                    offsetof(TracerState, builtinStatus), ACCSET_OTHER);
    pendingGuardCondition = lir->insEqI_0(status_ins);
    leaveDeepBailCall();

    set(&lval, rval_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TYPEOF()
{
    Value& r = stackval(-1);
    LIns* type;
    if (r.isString()) {
        type = INS_ATOM(cx->runtime->atomState.typeAtoms[JSTYPE_STRING]);
    } else if (r.isNumber()) {
        type = INS_ATOM(cx->runtime->atomState.typeAtoms[JSTYPE_NUMBER]);
    } else if (r.isUndefined()) {
        type = INS_ATOM(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
    } else if (r.isBoolean()) {
        type = INS_ATOM(cx->runtime->atomState.typeAtoms[JSTYPE_BOOLEAN]);
    } else if (r.isNull()) {
        type = INS_ATOM(cx->runtime->atomState.typeAtoms[JSTYPE_OBJECT]);
    } else {
        if (r.toObject().isFunction()) {
            type = INS_ATOM(cx->runtime->atomState.typeAtoms[JSTYPE_FUNCTION]);
        } else {
            LIns* args[] = { get(&r), cx_ins };
            type = lir->insCall(&js_TypeOfObject_ci, args);
        }
    }
    set(&r, type);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_VOID()
{
    stack(-1, INS_UNDEFINED());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INCNAME()
{
    return incName(1);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INCPROP()
{
    return incProp(1);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INCELEM()
{
    return InjectStatus(incElem(1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DECNAME()
{
    return incName(-1);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DECPROP()
{
    return incProp(-1);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DECELEM()
{
    return InjectStatus(incElem(-1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::incName(jsint incr, bool pre)
{
    Value* vp;
    LIns* v_ins;
    LIns* v_after;
    NameResult nr;

    CHECK_STATUS_A(name(vp, v_ins, nr));
    Value v = nr.tracked ? *vp : nr.v;
    CHECK_STATUS_A(incHelper(v, v_ins, v_after, incr));
    LIns* v_result = pre ? v_after : v_ins;
    if (nr.tracked) {
        set(vp, v_after);
        stack(0, v_result);
        return ARECORD_CONTINUE;
    }

    if (!nr.obj->isCall())
        RETURN_STOP_A("incName on unsupported object class");

    CHECK_STATUS_A(setCallProp(nr.obj, nr.obj_ins, nr.shape, v_after, v));
    stack(0, v_result);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NAMEINC()
{
    return incName(1, false);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_PROPINC()
{
    return incProp(1, false);
}


JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ELEMINC()
{
    return InjectStatus(incElem(1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NAMEDEC()
{
    return incName(-1, false);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_PROPDEC()
{
    return incProp(-1, false);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ELEMDEC()
{
    return InjectStatus(incElem(-1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETPROP()
{
    return getProp(stackval(-1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETPROP()
{
    Value& l = stackval(-2);
    if (l.isPrimitive())
        RETURN_STOP_A("primitive this for SETPROP");

    JSObject* obj = &l.toObject();
    if (obj->getOps()->setProperty)
        RETURN_STOP_A("non-native js::ObjectOps::setProperty");
    return ARECORD_CONTINUE;
}


JS_REQUIRES_STACK RecordingStatus
TraceRecorder::nativeSet(JSObject* obj, LIns* obj_ins, const Shape* shape,
                         const Value &v, LIns* v_ins)
{
    uint32 slot = shape->slot;

    

















    JS_ASSERT(shape->hasDefaultSetter() || slot == SHAPE_INVALID_SLOT);

    
    if (!shape->hasDefaultSetter())
        emitNativePropertyOp(shape, obj_ins, true, box_value_into_alloc(v, v_ins));

    
    if (slot != SHAPE_INVALID_SLOT) {
        JS_ASSERT(obj->containsSlot(shape->slot));
        JS_ASSERT(shape->hasSlot());
        if (obj == globalObj) {
            if (!lazilyImportGlobalSlot(slot))
                RETURN_STOP("lazy import of global slot failed");
            set(&obj->getSlotRef(slot), v_ins);
        } else {
            LIns* dslots_ins = NULL;
            stobj_set_slot(obj_ins, slot, dslots_ins, v, v_ins);
        }
    }

    return RECORD_CONTINUE;
}

static JSBool FASTCALL
MethodWriteBarrier(JSContext* cx, JSObject* obj, Shape* shape, JSObject* funobj)
{
    AutoObjectRooter tvr(cx, funobj);

    return obj->methodWriteBarrier(cx, *shape, ObjectValue(*tvr.object()));
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, MethodWriteBarrier, CONTEXT, OBJECT, SHAPE, OBJECT,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::setProp(Value &l, PropertyCacheEntry* entry, const Shape* shape,
                       Value &v, LIns*& v_ins, bool isDefinitelyAtom)
{
    if (entry == JS_NO_PROP_CACHE_FILL)
        RETURN_STOP("can't trace uncacheable property set");
    JS_ASSERT_IF(entry->vcapTag() >= 1, !shape->hasSlot());

    JS_ASSERT(!l.isPrimitive());
    JSObject* obj = &l.toObject();

    if (!shape->hasDefaultSetter() && shape->slot != SHAPE_INVALID_SLOT && !obj->isCall())
        RETURN_STOP("can't trace set of property with setter and slot");
    if (shape->hasSetterValue())
        RETURN_STOP("can't trace JavaScript function setter");

    
    if (shape->hasGetterValue())
        RETURN_STOP("can't assign to property with script getter but no setter");
    if (!shape->writable())
        RETURN_STOP("can't assign to readonly property");

    LIns* obj_ins = get(&l);

    JS_ASSERT_IF(entry->directHit(), obj->nativeContains(*shape));

    
    v_ins = get(&v);
    if (obj->isCall())
        return setCallProp(obj, obj_ins, shape, v_ins, v);

    
    JSObject* obj2 = obj;
    for (jsuword i = entry->scopeIndex(); i; i--)
        obj2 = obj2->getParent();
    for (jsuword j = entry->protoIndex(); j; j--)
        obj2 = obj2->getProto();
    JS_ASSERT_IF(entry->adding(), obj2 == obj);

    
    PCVal pcval;
    CHECK_STATUS(guardPropertyCacheHit(obj_ins, obj, obj2, entry, pcval));
    JS_ASSERT(!obj2->nativeEmpty());
    JS_ASSERT(obj2->nativeContains(*shape));
    JS_ASSERT_IF(obj2 != obj, !shape->hasSlot());

    





    if (obj2->brandedOrHasMethodBarrier() && IsFunctionObject(v) && entry->directHit()) {
        if (obj == globalObj)
            RETURN_STOP("can't trace function-valued property set in branded global scope");

        enterDeepBailCall();
        LIns* args[] = { v_ins, INS_CONSTSHAPE(shape), obj_ins, cx_ins };
        LIns* ok_ins = lir->insCall(&MethodWriteBarrier_ci, args);
        guard(false, lir->insEqI_0(ok_ins), OOM_EXIT);
        leaveDeepBailCall();
    }

    
    if (entry->adding()) {
        JS_ASSERT(shape->hasSlot());
        if (obj == globalObj)
            RETURN_STOP("adding a property to the global object");

        LIns* args[] = { INS_CONSTSHAPE(shape), obj_ins, cx_ins };
        const CallInfo *ci = isDefinitelyAtom ? &js_AddAtomProperty_ci : &js_AddProperty_ci;
        LIns* ok_ins = lir->insCall(ci, args);
        guard(false, lir->insEqI_0(ok_ins), OOM_EXIT);
    }

    return nativeSet(obj, obj_ins, shape, v, v_ins);
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::setUpwardTrackedVar(Value* stackVp, const Value &v, LIns* v_ins)
{
    JSValueType stackT = determineSlotType(stackVp);
    JSValueType otherT = getCoercedType(v);

    bool promote = true;

    if (stackT != otherT) {
        if (stackT == JSVAL_TYPE_DOUBLE && otherT == JSVAL_TYPE_INT32 && isPromoteInt(v_ins))
            promote = false;
        else
            RETURN_STOP("can't trace this upvar mutation");
    }

    set(stackVp, v_ins, promote);

    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::setCallProp(JSObject *callobj, LIns *callobj_ins, const Shape *shape,
                           LIns *v_ins, const Value &v)
{
    
    JSStackFrame *fp = frameIfInRange(callobj);
    if (fp) {
        if (shape->setterOp() == SetCallArg) {
            JS_ASSERT(shape->hasShortID());
            uintN slot = uint16(shape->shortid);
            Value *vp2 = &fp->argv[slot];
            CHECK_STATUS(setUpwardTrackedVar(vp2, v, v_ins));
            return RECORD_CONTINUE;
        }
        if (shape->setterOp() == SetCallVar) {
            JS_ASSERT(shape->hasShortID());
            uintN slot = uint16(shape->shortid);
            Value *vp2 = &fp->slots()[slot];
            CHECK_STATUS(setUpwardTrackedVar(vp2, v, v_ins));
            return RECORD_CONTINUE;
        }
        RETURN_STOP("can't trace special CallClass setter");
    }

    if (!callobj->getPrivate()) {
        
        
        
        
        
        intN slot = uint16(shape->shortid);
        if (shape->setterOp() == SetCallArg) {
            JS_ASSERT(slot < ArgClosureTraits::slot_count(callobj));
            slot += ArgClosureTraits::slot_offset(callobj);
        } else if (shape->setterOp() == SetCallVar) {
            JS_ASSERT(slot < VarClosureTraits::slot_count(callobj));
            slot += VarClosureTraits::slot_offset(callobj);
        } else {
            RETURN_STOP("can't trace special CallClass setter");
        }

        
        
        
        JS_ASSERT(shape->hasShortID());

        LIns* dslots_ins = NULL;
        stobj_set_slot(callobj_ins, slot, dslots_ins, v, v_ins);
        return RECORD_CONTINUE;
    }

    
    
    

    
    const CallInfo* ci = NULL;
    if (shape->setterOp() == SetCallArg)
        ci = &js_SetCallArg_ci;
    else if (shape->setterOp() == SetCallVar)
        ci = &js_SetCallVar_ci;
    else
        RETURN_STOP("can't trace special CallClass setter");

    
    
    
    

    LIns *fp_ins = entryFrameIns();
    LIns *fpcallobj_ins = lir->insLoad(LIR_ldp, fp_ins, JSStackFrame::offsetCallObj(),
                                       ACCSET_OTHER);
    LIns *br1 = lir->insBranch(LIR_jf, lir->ins2(LIR_eqp, fpcallobj_ins, callobj_ins), NULL);

    

    
    unsigned slot = uint16(shape->shortid);
    LIns *slot_ins;
    if (shape->setterOp() == SetCallArg)
        slot_ins = ArgClosureTraits::adj_slot_lir(lir, fp_ins, slot);
    else
        slot_ins = VarClosureTraits::adj_slot_lir(lir, fp_ins, slot);
    LIns *offset_ins = lir->ins2(LIR_muli, slot_ins, INS_CONST(sizeof(double)));

    
    LIns *callstackBase_ins = lir->insLoad(LIR_ldp, lirbuf->state,
                                           offsetof(TracerState, callstackBase), ACCSET_OTHER);
    LIns *frameInfo_ins = lir->insLoad(LIR_ldp, callstackBase_ins, 0, ACCSET_OTHER);
    LIns *typemap_ins = lir->ins2(LIR_addp, frameInfo_ins, INS_CONSTWORD(sizeof(FrameInfo)));
    LIns *type_ins = lir->insLoad(LIR_lduc2ui,
                                  lir->ins2(LIR_addp, typemap_ins, lir->insUI2P(slot_ins)), 0,
                                  ACCSET_OTHER, LOAD_CONST);
    JSValueType type = getCoercedType(v);
    if (type == JSVAL_TYPE_INT32 && !isPromoteInt(v_ins))
        type = JSVAL_TYPE_DOUBLE;
    guard(true,
          addName(lir->ins2(LIR_eqi, type_ins, lir->insImmI(type)),
                  "guard(type-stable set upvar)"),
          BRANCH_EXIT);

    
    LIns *stackBase_ins = lir->insLoad(LIR_ldp, lirbuf->state,
                                       offsetof(TracerState, stackBase), ACCSET_OTHER);
    LIns *storeValue_ins = isPromoteInt(v_ins) ? demote(lir, v_ins) : v_ins;
    lir->insStore(storeValue_ins,
                  lir->ins2(LIR_addp, stackBase_ins, lir->insUI2P(offset_ins)), 0,
                  ACCSET_STORE_ANY);
    LIns *br2 = lir->insBranch(LIR_j, NULL, NULL);

    
    LIns *label1 = lir->ins0(LIR_label);
    br1->setTarget(label1);
    LIns* args[] = {
        box_value_for_native_call(v, v_ins),
        INS_CONSTWORD(JSID_BITS(SHAPE_USERID(shape))),
        callobj_ins,
        cx_ins
    };
    LIns* call_ins = lir->insCall(ci, args);
    guard(false, addName(lir->insEqI_0(call_ins), "guard(set upvar)"), STATUS_EXIT);

    LIns *label2 = lir->ins0(LIR_label);
    br2->setTarget(label2);

    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_SetPropHit(PropertyCacheEntry* entry, const Shape* shape)
{
    Value& r = stackval(-1);
    Value& l = stackval(-2);
    LIns* v_ins;

    jsbytecode* pc = cx->regs->pc;

    bool isDefinitelyAtom = (*pc == JSOP_SETPROP);
    CHECK_STATUS_A(setProp(l, entry, shape, r, v_ins, isDefinitelyAtom));

    switch (*pc) {
      case JSOP_SETPROP:
      case JSOP_SETNAME:
      case JSOP_SETGNAME:
      case JSOP_SETMETHOD:
        if (pc[JSOP_SETPROP_LENGTH] != JSOP_POP)
            set(&l, v_ins);
        break;

      default:;
    }

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK VMSideExit*
TraceRecorder::enterDeepBailCall()
{
    
    VMSideExit* exit = snapshot(DEEP_BAIL_EXIT);
    lir->insStore(INS_CONSTPTR(exit), cx_ins, offsetof(JSContext, bailExit), ACCSET_OTHER);

    
    GuardRecord* guardRec = createGuardRecord(exit);
    lir->insGuard(LIR_xbarrier, NULL, guardRec);

    
    forgetGuardedShapes();
    return exit;
}

JS_REQUIRES_STACK void
TraceRecorder::leaveDeepBailCall()
{
    
    lir->insStore(INS_NULL(), cx_ins, offsetof(JSContext, bailExit), ACCSET_OTHER);
}

JS_REQUIRES_STACK void
TraceRecorder::finishGetProp(LIns* obj_ins, LIns* vp_ins, LIns* ok_ins, Value* outp)
{
    
    
    
    
    LIns* result_ins = lir->insLoad(LIR_ldd, vp_ins, 0, ACCSET_OTHER);
    set(outp, result_ins);
    if (js_CodeSpec[*cx->regs->pc].format & JOF_CALLOP)
        set(outp + 1, obj_ins);

    
    
    pendingGuardCondition = ok_ins;

    
    
    
    pendingUnboxSlot = outp;
}

static inline bool
RootedStringToId(JSContext* cx, JSString** namep, jsid* idp)
{
    JSString* name = *namep;
    if (name->isAtomized()) {
        *idp = INTERNED_STRING_TO_JSID(name);
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
GetPropertyByName(JSContext* cx, JSObject* obj, JSString** namep, Value* vp)
{
    LeaveTraceIfGlobalObject(cx, obj);

    jsid id;
    if (!RootedStringToId(cx, namep, &id) || !obj->getProperty(cx, id, vp)) {
        SetBuiltinError(cx);
        return false;
    }
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, GetPropertyByName, CONTEXT, OBJECT, STRINGPTR, VALUEPTR,
                     0, ACCSET_STORE_ANY)



JS_REQUIRES_STACK RecordingStatus
TraceRecorder::primitiveToStringInPlace(Value* vp)
{
    Value v = *vp;
    JS_ASSERT(v.isPrimitive());

    if (!v.isString()) {
        
        
        JSString *str = js_ValueToString(cx, v);
        JS_ASSERT(TRACE_RECORDER(cx) == this);
        if (!str)
            RETURN_ERROR("failed to stringify element id");
        v.setString(str);
        set(vp, stringify(*vp));

        
        
        *vp = v;
    }
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getPropertyByName(LIns* obj_ins, Value* idvalp, Value* outp)
{
    CHECK_STATUS(primitiveToStringInPlace(idvalp));
    enterDeepBailCall();

    
    
    
    LIns* vp_ins = addName(lir->insAlloc(sizeof(Value)), "vp");
    LIns* idvalp_ins = addName(addr(idvalp), "idvalp");
    LIns* args[] = {vp_ins, idvalp_ins, obj_ins, cx_ins};
    LIns* ok_ins = lir->insCall(&GetPropertyByName_ci, args);

    
    
    
    
    
    tracker.set(idvalp, lir->insLoad(LIR_ldp, idvalp_ins, 0, ACCSET_STACK|ACCSET_OTHER));

    finishGetProp(obj_ins, vp_ins, ok_ins, outp);
    leaveDeepBailCall();
    return RECORD_CONTINUE;
}

static JSBool FASTCALL
GetPropertyByIndex(JSContext* cx, JSObject* obj, int32 index, Value* vp)
{
    LeaveTraceIfGlobalObject(cx, obj);

    AutoIdRooter idr(cx);
    if (!js_Int32ToId(cx, index, idr.addr()) || !obj->getProperty(cx, idr.id(), vp)) {
        SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, GetPropertyByIndex, CONTEXT, OBJECT, INT32, VALUEPTR, 0,
                     ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getPropertyByIndex(LIns* obj_ins, LIns* index_ins, Value* outp)
{
    index_ins = makeNumberInt32(index_ins);

    
    enterDeepBailCall();
    LIns* vp_ins = addName(lir->insAlloc(sizeof(Value)), "vp");
    LIns* args[] = {vp_ins, index_ins, obj_ins, cx_ins};
    LIns* ok_ins = lir->insCall(&GetPropertyByIndex_ci, args);
    finishGetProp(obj_ins, vp_ins, ok_ins, outp);
    leaveDeepBailCall();
    return RECORD_CONTINUE;
}

static JSBool FASTCALL
GetPropertyById(JSContext* cx, JSObject* obj, jsid id, Value* vp)
{
    LeaveTraceIfGlobalObject(cx, obj);
    if (!obj->getProperty(cx, id, vp)) {
        SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, GetPropertyById, CONTEXT, OBJECT, JSID, VALUEPTR,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getPropertyById(LIns* obj_ins, Value* outp)
{
    
    JSAtom* atom;
    jsbytecode* pc = cx->regs->pc;
    const JSCodeSpec& cs = js_CodeSpec[*pc];
    if (*pc == JSOP_LENGTH) {
        atom = cx->runtime->atomState.lengthAtom;
    } else if (JOF_TYPE(cs.format) == JOF_ATOM) {
        atom = atoms[GET_INDEX(pc)];
    } else {
        JS_ASSERT(JOF_TYPE(cs.format) == JOF_SLOTATOM);
        atom = atoms[GET_INDEX(pc + SLOTNO_LEN)];
    }

    JS_STATIC_ASSERT(sizeof(jsid) == sizeof(void *));
    jsid id = ATOM_TO_JSID(atom);

    
    enterDeepBailCall();
    LIns* vp_ins = addName(lir->insAlloc(sizeof(Value)), "vp");
    LIns* args[] = {vp_ins, INS_CONSTWORD(JSID_BITS(id)), obj_ins, cx_ins};
    LIns* ok_ins = lir->insCall(&GetPropertyById_ci, args);
    finishGetProp(obj_ins, vp_ins, ok_ins, outp);
    leaveDeepBailCall();
    return RECORD_CONTINUE;
}


static JSBool FASTCALL
GetPropertyWithNativeGetter(JSContext* cx, JSObject* obj, Shape* shape, Value* vp)
{
    LeaveTraceIfGlobalObject(cx, obj);

#ifdef DEBUG
    JSProperty* prop;
    JSObject* pobj;
    JS_ASSERT(obj->lookupProperty(cx, shape->id, &pobj, &prop));
    JS_ASSERT(prop == (JSProperty*) shape);
    pobj->dropProperty(cx, prop);
#endif

    
    
    
    JS_ASSERT(obj->getClass() != &js_WithClass);

    vp->setUndefined();
    if (!shape->getterOp()(cx, obj, SHAPE_USERID(shape), vp)) {
        SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, GetPropertyWithNativeGetter,
                     CONTEXT, OBJECT, SHAPE, VALUEPTR, 0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getPropertyWithNativeGetter(LIns* obj_ins, const Shape* shape, Value* outp)
{
    JS_ASSERT(!shape->hasGetterValue());
    JS_ASSERT(shape->slot == SHAPE_INVALID_SLOT);
    JS_ASSERT(!shape->hasDefaultGetterOrIsMethod());

    
    
    
    enterDeepBailCall();
    LIns* vp_ins = addName(lir->insAlloc(sizeof(Value)), "vp");
    LIns* args[] = {vp_ins, INS_CONSTPTR(shape), obj_ins, cx_ins};
    LIns* ok_ins = lir->insCall(&GetPropertyWithNativeGetter_ci, args);
    finishGetProp(obj_ins, vp_ins, ok_ins, outp);
    leaveDeepBailCall();
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getPropertyWithScriptGetter(JSObject *obj, LIns* obj_ins, const Shape* shape)
{
    if (!canCallImacro())
        RETURN_STOP("cannot trace script getter, already in imacro");

    
    
    
    Value getter = shape->getterValue();
    Value*& sp = cx->regs->sp;
    switch (*cx->regs->pc) {
      case JSOP_GETPROP:
        sp++;
        sp[-1] = sp[-2];
        set(&sp[-1], get(&sp[-2]));
        sp[-2] = getter;
        set(&sp[-2], INS_CONSTOBJ(&getter.toObject()));
        return callImacroInfallibly(getprop_imacros.scriptgetter);

      case JSOP_CALLPROP:
        sp += 2;
        sp[-2] = getter;
        set(&sp[-2], INS_CONSTOBJ(&getter.toObject()));
        sp[-1] = sp[-3];
        set(&sp[-1], get(&sp[-3]));
        return callImacroInfallibly(callprop_imacros.scriptgetter);

      case JSOP_GETTHISPROP:
      case JSOP_GETARGPROP:
      case JSOP_GETLOCALPROP:
        sp += 2;
        sp[-2] = getter;
        set(&sp[-2], INS_CONSTOBJ(&getter.toObject()));
        sp[-1] = ObjectValue(*obj);
        set(&sp[-1], obj_ins);
        return callImacroInfallibly(getthisprop_imacros.scriptgetter);

      default:
        RETURN_STOP("cannot trace script getter for this opcode");
    }
}

JS_REQUIRES_STACK LIns*
TraceRecorder::getStringLength(LIns* str_ins)
{
    return addName(lir->ins2ImmI(LIR_rshup,
                                 addName(lir->insLoad(LIR_ldp, str_ins,
                                                      offsetof(JSString, mLengthAndFlags),
                                                      ACCSET_OTHER), "mLengthAndFlags"),
                                 JSString::FLAGS_LENGTH_SHIFT), "length");
}

JS_REQUIRES_STACK LIns*
TraceRecorder::getStringChars(LIns* str_ins)
{
    return addName(lir->insLoad(LIR_ldp, str_ins,
                                offsetof(JSString, mChars),
                                ACCSET_OTHER), "chars");
}

JS_REQUIRES_STACK LIns*
TraceRecorder::getCharCodeAt(JSString *str, LIns* str_ins, LIns* idx_ins)
{
    idx_ins = lir->insUI2P(makeNumberInt32(idx_ins));
    LIns *length_ins = lir->insLoad(LIR_ldp, str_ins, offsetof(JSString, mLengthAndFlags),
                                    ACCSET_OTHER);
    LIns *br = lir->insBranch(LIR_jt,
                              lir->insEqP_0(lir->ins2(LIR_andp,
                                                      length_ins,
                                                      INS_CONSTWORD(JSString::ROPE_BIT))),
                              NULL);
    lir->insCall(&js_Flatten_ci, &str_ins);
    br->setTarget(lir->ins0(LIR_label));

    guard(true,
          lir->ins2(LIR_ltup, idx_ins, lir->ins2ImmI(LIR_rshup, length_ins, JSString::FLAGS_LENGTH_SHIFT)),
          snapshot(MISMATCH_EXIT));
    LIns *chars_ins = getStringChars(str_ins);
    return i2d(lir->insLoad(LIR_ldus2ui,
                            lir->ins2(LIR_addp, chars_ins, lir->ins2ImmI(LIR_lshp, idx_ins, 1)), 0,
                            ACCSET_OTHER, LOAD_CONST));
}

JS_STATIC_ASSERT(sizeof(JSString) == 16 || sizeof(JSString) == 32);

JS_REQUIRES_STACK LIns*
TraceRecorder::getCharAt(JSString *str, LIns* str_ins, LIns* idx_ins, JSOp mode)
{
    idx_ins = lir->insUI2P(makeNumberInt32(idx_ins));
    LIns *length_ins = lir->insLoad(LIR_ldp, str_ins, offsetof(JSString, mLengthAndFlags),
                                    ACCSET_OTHER);
    LIns *br = lir->insBranch(LIR_jt,
                              lir->insEqP_0(lir->ins2(LIR_andp,
                                                      length_ins,
                                                      INS_CONSTWORD(JSString::ROPE_BIT))),
                              NULL);
    lir->insCall(&js_Flatten_ci, &str_ins);
    br->setTarget(lir->ins0(LIR_label));

    LIns *phi_ins = NULL;
    if (mode == JSOP_GETELEM) {
        guard(true,
              lir->ins2(LIR_ltup,
                        idx_ins,
                        lir->ins2ImmI(LIR_rshup,
                                      length_ins,
                                      JSString::FLAGS_LENGTH_SHIFT)),
              MISMATCH_EXIT);
    } else {
        phi_ins = lir->insAlloc(sizeof(JSString *));
        lir->insStore(LIR_stp, INS_CONSTSTR(cx->runtime->emptyString), phi_ins, 0, ACCSET_OTHER);

        br = lir->insBranch(LIR_jf,
                            lir->ins2(LIR_ltup,
                                      idx_ins,
                                      lir->ins2ImmI(LIR_rshup,
                                                    length_ins,
                                                    JSString::FLAGS_LENGTH_SHIFT)),
                            NULL);
    }

    LIns *chars_ins = getStringChars(str_ins);
    LIns *ch_ins = lir->insLoad(LIR_ldus2ui,
                                lir->ins2(LIR_addp, chars_ins, lir->ins2ImmI(LIR_lshp, idx_ins, 1)), 0,
                                ACCSET_OTHER, LOAD_CONST);
    guard(true, lir->ins2ImmI(LIR_ltui, ch_ins, UNIT_STRING_LIMIT), snapshot(MISMATCH_EXIT));
    LIns *unitstr_ins = lir->ins2(LIR_addp,
                                  INS_CONSTPTR(JSString::unitStringTable),
                                  lir->ins2ImmI(LIR_lshp,
                                                lir->insUI2P(ch_ins),
                                                (sizeof(JSString) == 16) ? 4 : 5));
    if (mode == JSOP_GETELEM)
        return unitstr_ins;

    lir->insStore(LIR_stp, unitstr_ins, phi_ins, 0, ACCSET_OTHER);
    br->setTarget(lir->ins0(LIR_label));

    return lir->insLoad(LIR_ldp, phi_ins, 0, ACCSET_OTHER);
}


#if NJ_EXPANDED_LOADSTORE_SUPPORTED && NJ_F2I_SUPPORTED
static bool OkToTraceTypedArrays = true;
#else
static bool OkToTraceTypedArrays = false;
#endif

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETELEM()
{
    bool call = *cx->regs->pc == JSOP_CALLELEM;

    Value& idx = stackval(-1);
    Value& lval = stackval(-2);

    LIns* obj_ins = get(&lval);
    LIns* idx_ins = get(&idx);

    
    if (lval.isString() && hasInt32Repr(idx)) {
        if (call)
            RETURN_STOP_A("JSOP_CALLELEM on a string");
        int i = asInt32(idx);
        if (size_t(i) >= lval.toString()->length())
            RETURN_STOP_A("Invalid string index in JSOP_GETELEM");
        set(&lval, getCharAt(lval.toString(), obj_ins, idx_ins, JSOP_GETELEM));
        return ARECORD_CONTINUE;
    }

    if (lval.isPrimitive())
        RETURN_STOP_A("JSOP_GETLEM on a primitive");
    RETURN_IF_XML_A(lval);

    JSObject* obj = &lval.toObject();
    if (obj == globalObj)
        RETURN_STOP_A("JSOP_GETELEM on global");
    LIns* v_ins;

    
    if (!idx.isInt32()) {
        if (!idx.isPrimitive())
            RETURN_STOP_A("object used as index");

        return InjectStatus(getPropertyByName(obj_ins, &idx, &lval));
    }

    if (obj->isArguments()) {
        unsigned depth;
        JSStackFrame *afp = guardArguments(obj, obj_ins, &depth);
        if (afp) {
            uintN int_idx = idx.toInt32();
            Value* vp = &afp->argv[int_idx];
            if (idx_ins->isImmD()) {
                if (int_idx < 0 || int_idx >= afp->numActualArgs())
                    RETURN_STOP_A("cannot trace arguments with out of range index");
                v_ins = get(vp);
            } else {
                
                
                
                idx_ins = makeNumberInt32(idx_ins);
                if (int_idx < 0 || int_idx >= afp->numActualArgs())
                    RETURN_STOP_A("cannot trace arguments with out of range index");

                VMSideExit *exit = snapshot(MISMATCH_EXIT);
                 guard(true,
                      addName(lir->ins2(LIR_gei, idx_ins, INS_CONST(0)),
                              "guard(upvar index >= 0)"),
                       exit);
                guard(true,
                      addName(lir->ins2(LIR_lti, idx_ins, INS_CONST(afp->numActualArgs())),
                              "guard(upvar index in range)"),
                      exit);

                JSValueType type = getCoercedType(*vp);

                
                LIns* typemap_ins;
                if (depth == 0) {
                    
                    
                    
                    unsigned stackSlots = NativeStackSlots(cx, 0 );
                    JSValueType* typemap = new (traceAlloc()) JSValueType[stackSlots];
                    DetermineTypesVisitor detVisitor(*this, typemap);
                    VisitStackSlots(detVisitor, cx, 0);
                    typemap_ins = INS_CONSTPTR(typemap + 2 );
                } else {
                    
                    
                    
                    
                    
                    LIns* fip_ins = lir->insLoad(LIR_ldp, lirbuf->rp,
                                                 (callDepth-depth)*sizeof(FrameInfo*),
                                                 ACCSET_RSTACK);
                    typemap_ins = lir->ins2(LIR_addp, fip_ins, INS_CONSTWORD(sizeof(FrameInfo) + 2 * sizeof(JSValueType)));
                }

                LIns* typep_ins = lir->ins2(LIR_addp, typemap_ins,
                                            lir->insUI2P(lir->ins2(LIR_muli,
                                                                   idx_ins,
                                                                   INS_CONST(sizeof(JSValueType)))));
                LIns* type_ins = lir->insLoad(LIR_lduc2ui, typep_ins, 0, ACCSET_OTHER, LOAD_CONST);
                guard(true,
                      addName(lir->ins2(LIR_eqi, type_ins, lir->insImmI(type)),
                              "guard(type-stable upvar)"),
                      BRANCH_EXIT);

                
                guard(true, lir->ins2(LIR_ltui, idx_ins, INS_CONST(afp->numActualArgs())),
                      snapshot(BRANCH_EXIT));
                size_t stackOffset = nativespOffset(&afp->argv[0]);
                LIns* args_addr_ins = lir->ins2(LIR_addp, lirbuf->sp, INS_CONSTWORD(stackOffset));
                LIns* argi_addr_ins = lir->ins2(LIR_addp,
                                                args_addr_ins,
                                                lir->insUI2P(lir->ins2(LIR_muli,
                                                                       idx_ins,
                                                                       INS_CONST(sizeof(double)))));
                
                
                
                
                v_ins = stackLoad(argi_addr_ins, ACCSET_LOAD_ANY, type);
            }
            JS_ASSERT(v_ins);
            set(&lval, v_ins);
            if (call)
                set(&idx, obj_ins);
            return ARECORD_CONTINUE;
        }
        RETURN_STOP_A("can't reach arguments object's frame");
    }

    if (obj->isDenseArray()) {
        
        Value* vp;
        LIns* addr_ins;

        guardDenseArray(obj_ins, BRANCH_EXIT);
        CHECK_STATUS_A(denseArrayElement(lval, idx, vp, v_ins, addr_ins));
        set(&lval, v_ins);
        if (call)
            set(&idx, obj_ins);
        return ARECORD_CONTINUE;
    }

    if (OkToTraceTypedArrays && js_IsTypedArray(obj)) {
        
        Value* vp;
        LIns* addr_ins;

        guardClass(obj_ins, obj->getClass(), snapshot(BRANCH_EXIT), LOAD_CONST);
        CHECK_STATUS_A(typedArrayElement(lval, idx, vp, v_ins, addr_ins));
        set(&lval, v_ins);
        if (call)
            set(&idx, obj_ins);
        return ARECORD_CONTINUE;
    }

    return InjectStatus(getPropertyByIndex(obj_ins, idx_ins, &lval));
}



static JSBool FASTCALL
SetPropertyByName(JSContext* cx, JSObject* obj, JSString** namep, Value* vp)
{
    LeaveTraceIfGlobalObject(cx, obj);

    jsid id;
    if (!RootedStringToId(cx, namep, &id) || !obj->setProperty(cx, id, vp)) {
        SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, SetPropertyByName, CONTEXT, OBJECT, STRINGPTR, VALUEPTR,
                     0, ACCSET_STORE_ANY)

static JSBool FASTCALL
InitPropertyByName(JSContext* cx, JSObject* obj, JSString** namep, ValueArgType arg)
{
    LeaveTraceIfGlobalObject(cx, obj);

    jsid id;
    if (!RootedStringToId(cx, namep, &id) ||
        !obj->defineProperty(cx, id, ValueArgToConstRef(arg), NULL, NULL, JSPROP_ENUMERATE)) {
        SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, InitPropertyByName, CONTEXT, OBJECT, STRINGPTR, VALUE,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::initOrSetPropertyByName(LIns* obj_ins, Value* idvalp, Value* rvalp, bool init)
{
    CHECK_STATUS(primitiveToStringInPlace(idvalp));

    if (init) {
        LIns* v_ins = box_value_for_native_call(*rvalp, get(rvalp));
        enterDeepBailCall();
        LIns* idvalp_ins = addName(addr(idvalp), "idvalp");
        LIns* args[] = {v_ins, idvalp_ins, obj_ins, cx_ins};
        pendingGuardCondition = lir->insCall(&InitPropertyByName_ci, args);
    } else {
        
        LIns* vp_ins = box_value_into_alloc(*rvalp, get(rvalp));
        enterDeepBailCall();
        LIns* idvalp_ins = addName(addr(idvalp), "idvalp");
        LIns* args[] = {vp_ins, idvalp_ins, obj_ins, cx_ins};
        pendingGuardCondition = lir->insCall(&SetPropertyByName_ci, args);
    }

    leaveDeepBailCall();
    return RECORD_CONTINUE;
}

static JSBool FASTCALL
SetPropertyByIndex(JSContext* cx, JSObject* obj, int32 index, Value* vp)
{
    LeaveTraceIfGlobalObject(cx, obj);

    AutoIdRooter idr(cx);
    if (!js_Int32ToId(cx, index, idr.addr()) || !obj->setProperty(cx, idr.id(), vp)) {
        SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, SetPropertyByIndex, CONTEXT, OBJECT, INT32, VALUEPTR,
                     0, ACCSET_STORE_ANY)

static JSBool FASTCALL
InitPropertyByIndex(JSContext* cx, JSObject* obj, int32 index, ValueArgType arg)
{
    LeaveTraceIfGlobalObject(cx, obj);

    AutoIdRooter idr(cx);
    if (!js_Int32ToId(cx, index, idr.addr()) ||
        !obj->defineProperty(cx, idr.id(), ValueArgToConstRef(arg), NULL, NULL, JSPROP_ENUMERATE)) {
        SetBuiltinError(cx);
        return JS_FALSE;
    }
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, InitPropertyByIndex, CONTEXT, OBJECT, INT32, VALUE,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::initOrSetPropertyByIndex(LIns* obj_ins, LIns* index_ins, Value* rvalp, bool init)
{
    index_ins = makeNumberInt32(index_ins);

    if (init) {
        LIns* rval_ins = box_value_for_native_call(*rvalp, get(rvalp));
        enterDeepBailCall();
        LIns* args[] = {rval_ins, index_ins, obj_ins, cx_ins};
        pendingGuardCondition = lir->insCall(&InitPropertyByIndex_ci, args);
    } else {
        
        LIns* vp_ins = box_value_into_alloc(*rvalp, get(rvalp));
        enterDeepBailCall();
        LIns* args[] = {vp_ins, index_ins, obj_ins, cx_ins};
        pendingGuardCondition = lir->insCall(&SetPropertyByIndex_ci, args);
    }

    leaveDeepBailCall();
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::setElem(int lval_spindex, int idx_spindex, int v_spindex)
{
    Value& v = stackval(v_spindex);
    Value& idx = stackval(idx_spindex);
    Value& lval = stackval(lval_spindex);

    if (lval.isPrimitive())
        RETURN_STOP_A("left JSOP_SETELEM operand is not an object");
    RETURN_IF_XML_A(lval);

    JSObject* obj = &lval.toObject();
    LIns* obj_ins = get(&lval);
    LIns* idx_ins = get(&idx);
    LIns* v_ins = get(&v);

    if (obj->isArguments())
        RETURN_STOP_A("can't trace setting elements of the |arguments| object");

    if (obj == globalObj)
        RETURN_STOP_A("can't trace setting elements on the global object");

    if (!idx.isInt32()) {
        if (!idx.isPrimitive())
            RETURN_STOP_A("non-primitive index");
        CHECK_STATUS_A(initOrSetPropertyByName(obj_ins, &idx, &v,
                                             *cx->regs->pc == JSOP_INITELEM));
    } else if (OkToTraceTypedArrays && js_IsTypedArray(obj)) {
        

        
        guardClass(obj_ins, obj->getClass(), snapshot(BRANCH_EXIT), LOAD_CONST);

        js::TypedArray* tarray = js::TypedArray::fromJSObject(obj);

        LIns* priv_ins = stobj_get_const_private_ptr(obj_ins);

        
        
        idx_ins = makeNumberInt32(idx_ins);            
                                                       
        
        lir->insGuard(LIR_xf,
                      lir->ins2(LIR_ltui,
                                idx_ins,
                                lir->insLoad(LIR_ldi, priv_ins, js::TypedArray::lengthOffset(),
                                             ACCSET_OTHER, LOAD_CONST)),
                      createGuardRecord(snapshot(OVERFLOW_EXIT)));

        
        LIns* data_ins = lir->insLoad(LIR_ldp, priv_ins, js::TypedArray::dataOffset(),
                                      ACCSET_OTHER, LOAD_CONST);
        LIns* pidx_ins = lir->insUI2P(idx_ins);
        LIns* addr_ins = 0;

        LIns* typed_v_ins = v_ins;

        
        
        
        if (!v.isNumber()) {
            if (v.isNull()) {
                typed_v_ins = lir->insImmD(0);
            } else if (v.isUndefined()) {
                typed_v_ins = lir->insImmD(js_NaN);
            } else if (v.isString()) {
                LIns* args[] = { typed_v_ins, cx_ins };
                typed_v_ins = lir->insCall(&js_StringToNumber_ci, args);
            } else if (v.isBoolean()) {
                JS_ASSERT(v.isBoolean());
                typed_v_ins = i2d(typed_v_ins);
            } else {
                typed_v_ins = lir->insImmD(js_NaN);
            }
        }

        switch (tarray->type) {
          case js::TypedArray::TYPE_INT8:
          case js::TypedArray::TYPE_INT16:
          case js::TypedArray::TYPE_INT32:
            typed_v_ins = d2i(typed_v_ins);
            break;
          case js::TypedArray::TYPE_UINT8:
          case js::TypedArray::TYPE_UINT16:
          case js::TypedArray::TYPE_UINT32:
            typed_v_ins = f2u(typed_v_ins);
            break;
          case js::TypedArray::TYPE_UINT8_CLAMPED:
            if (isPromoteInt(typed_v_ins)) {
                typed_v_ins = demote(lir, typed_v_ins);
                typed_v_ins = lir->insChoose(lir->ins2ImmI(LIR_lti, typed_v_ins, 0),
                                             lir->insImmI(0),
                                             lir->insChoose(lir->ins2ImmI(LIR_gti,
                                                                          typed_v_ins,
                                                                          0xff),
                                                            lir->insImmI(0xff),
                                                            typed_v_ins,
                                                            avmplus::AvmCore::use_cmov()),
                                        avmplus::AvmCore::use_cmov());
            } else {
                typed_v_ins = lir->insCall(&js_TypedArray_uint8_clamp_double_ci, &typed_v_ins);
            }
            break;
          case js::TypedArray::TYPE_FLOAT32:
          case js::TypedArray::TYPE_FLOAT64:
            
            break;
          default:
            JS_NOT_REACHED("Unknown typed array type in tracer");       
        }

        switch (tarray->type) {
          case js::TypedArray::TYPE_INT8:
          case js::TypedArray::TYPE_UINT8_CLAMPED:
          case js::TypedArray::TYPE_UINT8:
            addr_ins = lir->ins2(LIR_addp, data_ins, pidx_ins);
            lir->insStore(LIR_sti2c, typed_v_ins, addr_ins, 0, ACCSET_OTHER);
            break;
          case js::TypedArray::TYPE_INT16:
          case js::TypedArray::TYPE_UINT16:
            addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 1));
            lir->insStore(LIR_sti2s, typed_v_ins, addr_ins, 0, ACCSET_OTHER);
            break;
          case js::TypedArray::TYPE_INT32:
          case js::TypedArray::TYPE_UINT32:
            addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 2));
            lir->insStore(LIR_sti, typed_v_ins, addr_ins, 0, ACCSET_OTHER);
            break;
          case js::TypedArray::TYPE_FLOAT32:
            addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 2));
            lir->insStore(LIR_std2f, typed_v_ins, addr_ins, 0, ACCSET_OTHER);
            break;
          case js::TypedArray::TYPE_FLOAT64:
            addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 3));
            lir->insStore(LIR_std, typed_v_ins, addr_ins, 0, ACCSET_OTHER);
            break;
          default:
            JS_NOT_REACHED("Unknown typed array type in tracer");       
        }
    } else if (idx.toInt32() < 0 || !obj->isDenseArray()) {
        CHECK_STATUS_A(initOrSetPropertyByIndex(obj_ins, idx_ins, &v,
                                                *cx->regs->pc == JSOP_INITELEM));
    } else {
        

        
        if (!obj->isDenseArray()) 
            return ARECORD_STOP;
        guardDenseArray(obj_ins, BRANCH_EXIT);

        
        
        idx_ins = makeNumberInt32(idx_ins);

        
        
        
        
        LIns* res_ins;
        LIns* args[] = { NULL, idx_ins, obj_ins, cx_ins };
        if (v.isNumber()) {
            if (fcallinfo(v_ins) == &js_UnboxDouble_ci) {
#if JS_BITS_PER_WORD == 32
                LIns *boxed = lir->insAlloc(sizeof(Value));
                LIns *tag_ins = fcallarg(v_ins, 0);
                LIns *payload_ins = fcallarg(v_ins, 1);
                lir->insStore(tag_ins, boxed, sTagOffset, ACCSET_OTHER);
                lir->insStore(payload_ins, boxed, sPayloadOffset, ACCSET_OTHER);
                args[0] = boxed;
#else
                args[0] = fcallarg(v_ins, 0);
#endif
                res_ins = lir->insCall(&js_Array_dense_setelem_ci, args);
            } else if (isPromoteInt(v_ins)) {
                args[0] = demote(lir, v_ins);
                res_ins = lir->insCall(&js_Array_dense_setelem_int_ci, args);
            } else {
                args[0] = v_ins;
                res_ins = lir->insCall(&js_Array_dense_setelem_double_ci, args);
            }
        } else {
            args[0] = box_value_for_native_call(v, v_ins);
            res_ins = lir->insCall(&js_Array_dense_setelem_ci, args);
        }
        guard(false, lir->insEqI_0(res_ins), MISMATCH_EXIT);
    }

    jsbytecode* pc = cx->regs->pc;
    if (*pc == JSOP_SETELEM && pc[JSOP_SETELEM_LENGTH] != JSOP_POP)
        set(&lval, v_ins);

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETELEM()
{
    return setElem(-3, -2, -1);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLNAME()
{
    JSObject* obj = cx->fp()->getScopeChain();
    if (obj != globalObj) {
        Value* vp;
        LIns* ins;
        NameResult nr;
        CHECK_STATUS_A(scopeChainProp(obj, vp, ins, nr));
        stack(0, ins);
        stack(1, INS_NULL());
        return ARECORD_CONTINUE;
    }

    LIns* obj_ins = INS_CONSTOBJ(globalObj);
    JSObject* obj2;
    PCVal pcval;

    CHECK_STATUS_A(test_property_cache(obj, obj_ins, obj2, pcval));

    if (pcval.isNull() || !pcval.isFunObj())
        RETURN_STOP_A("callee is not an object");

    stack(0, INS_CONSTOBJ(&pcval.toFunObj()));
    stack(1, INS_NULL());
    return ARECORD_CONTINUE;
}

JS_DEFINE_CALLINFO_5(extern, UINT32, GetUpvarArgOnTrace, CONTEXT, UINT32, INT32, UINT32,
                     DOUBLEPTR, 0, ACCSET_STORE_ANY)
JS_DEFINE_CALLINFO_5(extern, UINT32, GetUpvarVarOnTrace, CONTEXT, UINT32, INT32, UINT32,
                     DOUBLEPTR, 0, ACCSET_STORE_ANY)
JS_DEFINE_CALLINFO_5(extern, UINT32, GetUpvarStackOnTrace, CONTEXT, UINT32, INT32, UINT32,
                     DOUBLEPTR, 0, ACCSET_STORE_ANY)






JS_REQUIRES_STACK LIns*
TraceRecorder::upvar(JSScript* script, JSUpvarArray* uva, uintN index, Value& v)
{
    






    UpvarCookie cookie = uva->vector[index];
    const Value& vr = GetUpvar(cx, script->staticLevel, cookie);
    v = vr;

    if (LIns* ins = attemptImport(&vr))
        return ins;

    



    uint32 level = script->staticLevel - cookie.level();
    uint32 cookieSlot = cookie.slot();
    JSStackFrame* fp = cx->findFrameAtLevel(level);
    const CallInfo* ci;
    int32 slot;
    if (!fp->hasFunction() || (fp->flags & JSFRAME_EVAL)) {
        ci = &GetUpvarStackOnTrace_ci;
        slot = cookieSlot;
    } else if (cookieSlot < fp->numFormalArgs()) {
        ci = &GetUpvarArgOnTrace_ci;
        slot = cookieSlot;
    } else if (cookieSlot == UpvarCookie::CALLEE_SLOT) {
        ci = &GetUpvarArgOnTrace_ci;
        slot = -2;
    } else {
        ci = &GetUpvarVarOnTrace_ci;
        slot = cookieSlot - fp->numFormalArgs();
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
    JSValueType type = getCoercedType(v);
    guard(true,
          addName(lir->ins2(LIR_eqi, call_ins, lir->insImmI(type)),
                  "guard(type-stable upvar)"),
          BRANCH_EXIT);
    return stackLoad(outp, ACCSET_OTHER, type);
}





LIns*
TraceRecorder::stackLoad(LIns* base, AccSet accSet, uint8 type)
{
    LOpcode loadOp;
    switch (type) {
      case JSVAL_TYPE_DOUBLE:
        loadOp = LIR_ldd;
        break;
      case JSVAL_TYPE_NONFUNOBJ:
      case JSVAL_TYPE_STRING:
      case JSVAL_TYPE_FUNOBJ:
      case JSVAL_TYPE_NULL:
        loadOp = LIR_ldp;
        break;
      case JSVAL_TYPE_INT32:
      case JSVAL_TYPE_BOOLEAN:
      case JSVAL_TYPE_UNDEFINED:
      case JSVAL_TYPE_MAGIC:
        loadOp = LIR_ldi;
        break;
      case JSVAL_TYPE_BOXED:
      default:
        JS_NOT_REACHED("found jsval type in an upvar type map entry");
        return NULL;
    }

    LIns* result = lir->insLoad(loadOp, base, 0, accSet);
    if (type == JSVAL_TYPE_INT32)
        result = lir->ins1(LIR_i2d, result);
    return result;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETUPVAR()
{
    uintN index = GET_UINT16(cx->regs->pc);
    JSScript *script = cx->fp()->getScript();
    JSUpvarArray* uva = script->upvars();
    JS_ASSERT(index < uva->length);

    Value v;
    LIns* upvar_ins = upvar(script, uva, index, v);
    if (!upvar_ins)
        return ARECORD_STOP;
    stack(0, upvar_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLUPVAR()
{
    CHECK_STATUS_A(record_JSOP_GETUPVAR());
    stack(1, INS_NULL());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETFCSLOT()
{
    JSObject* callee = cx->fp()->callee();
    LIns* callee_ins = get(&cx->fp()->argv[-2]);

    LIns* upvars_ins = stobj_get_const_private_ptr(callee_ins,
                                                   JSObject::JSSLOT_FLAT_CLOSURE_UPVARS);

    unsigned index = GET_UINT16(cx->regs->pc);
    LIns *v_ins = unbox_value(callee->getFlatClosureUpvar(index), upvars_ins, index * sizeof(Value),
                              snapshot(BRANCH_EXIT));
    stack(0, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLFCSLOT()
{
    CHECK_STATUS_A(record_JSOP_GETFCSLOT());
    stack(1, INS_NULL());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::guardCallee(Value& callee)
{
    JSObject* callee_obj = &callee.toObject();
    JS_ASSERT(callee_obj->isFunction());
    JSFunction* callee_fun = (JSFunction*) callee_obj->getPrivate();

    





    VMSideExit* branchExit = snapshot(BRANCH_EXIT);
    LIns* callee_ins = get(&callee);
    tree->gcthings.addUnique(callee);

    guard(true,
          lir->ins2(LIR_eqp,
                    stobj_get_const_private_ptr(callee_ins),
                    INS_CONSTPTR(callee_fun)),
          branchExit);

    





















    if (FUN_INTERPRETED(callee_fun) &&
        (!FUN_NULL_CLOSURE(callee_fun) || callee_fun->u.i.nupvars != 0)) {
        JSObject* parent = callee_obj->getParent();

        if (parent != globalObj) {
            if (!parent->isCall())
                RETURN_STOP("closure scoped by neither the global object nor a Call object");

            guard(true,
                  lir->ins2(LIR_eqp,
                            stobj_get_parent(callee_ins),
                            INS_CONSTOBJ(parent)),
                  branchExit);
        }
    }
    return RECORD_CONTINUE;
}







JS_REQUIRES_STACK JSStackFrame *
TraceRecorder::guardArguments(JSObject *obj, LIns* obj_ins, unsigned *depthp)
{
    JS_ASSERT(obj->isArguments());

    JSStackFrame *afp = frameIfInRange(obj, depthp);
    if (!afp)
        return NULL;

    VMSideExit *exit = snapshot(MISMATCH_EXIT);
    guardClass(obj_ins, obj->getClass(), exit, LOAD_CONST);

    LIns* args_ins = getFrameObjPtr(afp->addressArgsObj());
    LIns* cmp = lir->ins2(LIR_eqp, args_ins, obj_ins);
    lir->insGuard(LIR_xf, cmp, createGuardRecord(exit));
    return afp;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::interpretedFunctionCall(Value& fval, JSFunction* fun, uintN argc, bool constructing)
{
    







    if (fun->u.i.script->isEmpty()) {
        LIns* rval_ins = constructing ? stack(-1 - argc) : INS_UNDEFINED();
        stack(-2 - argc, rval_ins);
        return RECORD_CONTINUE;
    }

    if (fval.toObject().getGlobal() != globalObj)
        RETURN_STOP("JSOP_CALL or JSOP_NEW crosses global scopes");

    JSStackFrame* const fp = cx->fp();

    
    unsigned stackSlots = NativeStackSlots(cx, 0 );
    FrameInfo* fi = (FrameInfo*)
        tempAlloc().alloc(sizeof(FrameInfo) + stackSlots * sizeof(JSValueType));
    JSValueType* typemap = (JSValueType*)(fi + 1);

    DetermineTypesVisitor detVisitor(*this, typemap);
    VisitStackSlots(detVisitor, cx, 0);

    JS_ASSERT(argc < FrameInfo::CONSTRUCTING_FLAG);

    tree->gcthings.addUnique(fval);
    fi->block = fp->maybeBlockChain();
    if (fp->hasBlockChain())
        tree->gcthings.addUnique(ObjectValue(*fp->getBlockChain()));
    fi->pc = cx->regs->pc;
    fi->imacpc = fp->maybeIMacroPC();
    fi->spdist = cx->regs->sp - fp->slots();
    fi->set_argc(uint16(argc), constructing);
    fi->callerHeight = stackSlots - (2 + argc);
    fi->callerArgc = numActualArgs(fp);

    if (callDepth >= tree->maxCallDepth)
        tree->maxCallDepth = callDepth + 1;

    fi = traceMonitor->frameCache->memoize(fi);
    if (!fi)
        RETURN_STOP("out of memory");
    lir->insStore(INS_CONSTPTR(fi), lirbuf->rp, callDepth * sizeof(FrameInfo*), ACCSET_RSTACK);

#if defined JS_JIT_SPEW
    debug_only_printf(LC_TMTracer, "iFC frameinfo=%p, stack=%d, map=", (void*)fi,
                      fi->callerHeight);
    for (unsigned i = 0; i < fi->callerHeight; i++)
        debug_only_printf(LC_TMTracer, "%c", TypeToChar(fi->get_typemap()[i]));
    debug_only_print0(LC_TMTracer, "\n");
#endif

    updateAtoms(fun->u.i.script);
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALL()
{
    uintN argc = GET_ARGC(cx->regs->pc);
    cx->assertValidStackDepth(argc + 2);
    return InjectStatus(functionCall(argc,
                                     (cx->fp()->hasIMacroPC() && *cx->fp()->getIMacroPC() == JSOP_APPLY)
                                        ? JSOP_APPLY
                                        : JSOP_CALL));
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

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_APPLY()
{
    jsbytecode *pc = cx->regs->pc;
    uintN argc = GET_ARGC(pc);
    cx->assertValidStackDepth(argc + 2);

    Value* vp = cx->regs->sp - (argc + 2);
    jsuint length = 0;
    JSObject* aobj = NULL;
    LIns* aobj_ins = NULL;

    JS_ASSERT(!cx->fp()->hasIMacroPC());

    if (!IsFunctionObject(vp[0]))
        return record_JSOP_CALL();
    RETURN_IF_XML_A(vp[0]);

    JSObject* obj = &vp[0].toObject();
    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, obj);
    if (FUN_INTERPRETED(fun))
        return record_JSOP_CALL();

    bool apply = (FastNative)fun->u.n.native == js_fun_apply;
    if (!apply && (FastNative)fun->u.n.native != js_fun_call)
        return record_JSOP_CALL();

    



    if (argc > 0 && !vp[2].isObject())
        return record_JSOP_CALL();

    


    if (!IsFunctionObject(vp[1]))
        RETURN_STOP_A("callee is not a function");
    CHECK_STATUS_A(guardCallee(vp[1]));

    if (apply && argc >= 2) {
        if (argc != 2)
            RETURN_STOP_A("apply with excess arguments");
        if (vp[3].isPrimitive())
            RETURN_STOP_A("arguments parameter of apply is primitive");
        aobj = &vp[3].toObject();
        aobj_ins = get(&vp[3]);

        



        if (aobj->isDenseArray()) {
            guardDenseArray(aobj_ins, MISMATCH_EXIT);
            length = aobj->getArrayLength();
            guard(true,
                  lir->ins2ImmI(LIR_eqi,
                                stobj_get_fslot_uint32(aobj_ins, JSObject::JSSLOT_ARRAY_LENGTH),
                                length),
                  BRANCH_EXIT);
        } else if (aobj->isArguments()) {
            unsigned depth;
            JSStackFrame *afp = guardArguments(aobj, aobj_ins, &depth);
            if (!afp)
                RETURN_STOP_A("can't reach arguments object's frame");
            length = afp->numActualArgs();
        } else {
            RETURN_STOP_A("arguments parameter of apply is not a dense array or argments object");
        }

        if (length >= JS_ARRAY_LENGTH(apply_imacro_table))
            RETURN_STOP_A("too many arguments to apply");

        return InjectStatus(callImacro(apply_imacro_table[length]));
    }

    if (argc >= JS_ARRAY_LENGTH(call_imacro_table))
        RETURN_STOP_A("too many arguments to call");

    return InjectStatus(callImacro(call_imacro_table[argc]));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_NativeCallComplete()
{
    if (pendingSpecializedNative == IGNORE_NATIVE_CALL_COMPLETE_CALLBACK)
        return ARECORD_CONTINUE;

#ifdef DEBUG
    JS_ASSERT(pendingSpecializedNative);
    jsbytecode* pc = cx->regs->pc;
    JS_ASSERT(*pc == JSOP_CALL || *pc == JSOP_APPLY || *pc == JSOP_NEW || *pc == JSOP_SETPROP);
#endif

    Value& v = stackval(-1);
    LIns* v_ins = get(&v);

    













    if (JSTN_ERRTYPE(pendingSpecializedNative) == FAIL_STATUS) {
        
        lir->insStore(INS_NULL(), cx_ins, (int) offsetof(JSContext, bailExit), ACCSET_OTHER);

        LIns* status = lir->insLoad(LIR_ldi, lirbuf->state,
                                    (int) offsetof(TracerState, builtinStatus), ACCSET_OTHER);
        if (pendingSpecializedNative == &generatedSpecializedNative) {
            LIns* ok_ins = v_ins;

            





            if (pendingSpecializedNative->flags & JSTN_CONSTRUCTOR) {
                LIns *cond_ins;
                LIns *x;

                
                
                unbox_any_object(native_rval_ins, &v_ins, &cond_ins, ACCSET_OTHER);
                
                x = lir->insChoose(cond_ins, v_ins, INS_CONSTWORD(0), avmplus::AvmCore::use_cmov());
                
                
                v_ins = lir->insChoose(lir->insEqP_0(x), newobj_ins, x, avmplus::AvmCore::use_cmov());
            } else {
                v_ins = lir->insLoad(LIR_ldd, native_rval_ins, 0, ACCSET_OTHER);
            }
            set(&v, v_ins);

            propagateFailureToBuiltinStatus(ok_ins, status);
        }
        guard(true, lir->insEqI_0(status), STATUS_EXIT);
    }

    if (pendingSpecializedNative->flags & JSTN_UNBOX_AFTER) {
        




        JS_ASSERT(&v == &cx->regs->sp[-1] && get(&v) == v_ins);
        set(&v, unbox_value(v, native_rval_ins, 0, snapshot(BRANCH_EXIT)));
    } else if (pendingSpecializedNative->flags &
               (JSTN_RETURN_NULLABLE_STR | JSTN_RETURN_NULLABLE_OBJ)) {
        guard(v.isNull(),
              addName(lir->insEqP_0(v_ins), "guard(nullness)"),
              BRANCH_EXIT);
    } else if (JSTN_ERRTYPE(pendingSpecializedNative) == FAIL_NEG) {
        
        JS_ASSERT(v.isNumber());
    } else {
        
        if (v.isNumber() &&
            pendingSpecializedNative->builtin->returnType() == ARGTYPE_I) {
            set(&v, lir->ins1(LIR_i2d, v_ins));
        }
    }

    
    
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::name(Value*& vp, LIns*& ins, NameResult& nr)
{
    JSObject* obj = cx->fp()->getScopeChain();
    JSOp op = JSOp(*cx->regs->pc);
    if (js_CodeSpec[op].format & JOF_GNAME)
        obj = obj->getGlobal();
    if (obj != globalObj)
        return scopeChainProp(obj, vp, ins, nr);

    
    LIns* obj_ins = INS_CONSTOBJ(globalObj);
    uint32 slot;

    JSObject* obj2;
    PCVal pcval;

    



    CHECK_STATUS_A(test_property_cache(obj, obj_ins, obj2, pcval));

    
    if (pcval.isNull())
        RETURN_STOP_A("named property not found");

    
    if (obj2 != obj)
        RETURN_STOP_A("name() hit prototype chain");

    
    if (pcval.isShape()) {
        const Shape* shape = pcval.toShape();
        if (!isValidSlot(obj, shape))
            RETURN_STOP_A("name() not accessing a valid slot");
        slot = shape->slot;
    } else {
        if (!pcval.isSlot())
            RETURN_STOP_A("PCE is not a slot");
        slot = pcval.toSlot();
    }

    if (!lazilyImportGlobalSlot(slot))
        RETURN_STOP_A("lazy import of global slot failed");

    vp = &obj->getSlotRef(slot);
    ins = get(vp);
    nr.tracked = true;
    return ARECORD_CONTINUE;
}

static JSObject* FASTCALL
MethodReadBarrier(JSContext* cx, JSObject* obj, Shape* shape, JSObject* funobj)
{
    Value v = ObjectValue(*funobj);
    AutoValueRooter tvr(cx, v);

    if (!obj->methodReadBarrier(cx, *shape, tvr.addr()))
        return NULL;
    return &tvr.value().toObject();
}
JS_DEFINE_CALLINFO_4(static, OBJECT_FAIL, MethodReadBarrier, CONTEXT, OBJECT, SHAPE, OBJECT,
                     0, ACCSET_STORE_ANY)









JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::prop(JSObject* obj, LIns* obj_ins, uint32 *slotp, LIns** v_insp, Value *outp)
{
    






    if (!obj->isDenseArray() && obj->getOps()->getProperty)
        RETURN_STOP_A("non-dense-array, non-native js::ObjectOps::getProperty");

    JS_ASSERT((slotp && v_insp && !outp) || (!slotp && !v_insp && outp));

    



    JSObject* obj2;
    PCVal pcval;
    CHECK_STATUS_A(test_property_cache(obj, obj_ins, obj2, pcval));

    
    if (pcval.isNull()) {
        if (slotp)
            RETURN_STOP_A("property not found");

        



        if (obj->getClass()->getProperty != Valueify(JS_PropertyStub)) {
            RETURN_STOP_A("can't trace through access to undefined property if "
                          "JSClass.getProperty hook isn't stubbed");
        }
        guardClass(obj_ins, obj->getClass(), snapshot(MISMATCH_EXIT), LOAD_NORMAL);

        






        VMSideExit* exit = snapshot(BRANCH_EXIT);
        do {
            if (obj->isNative()) {
                CHECK_STATUS_A(guardShape(obj_ins, obj, obj->shape(), "guard(shape)", exit));
            } else if (obj->isDenseArray()) {
                guardDenseArray(obj_ins, exit);
            } else {
                RETURN_STOP_A("non-native object involved in undefined property access");
            }
        } while (guardHasPrototype(obj, obj_ins, &obj, &obj_ins, exit));

        set(outp, INS_UNDEFINED());
        return ARECORD_CONTINUE;
    }

    return InjectStatus(propTail(obj, obj_ins, obj2, pcval, slotp, v_insp, outp));
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::propTail(JSObject* obj, LIns* obj_ins, JSObject* obj2, PCVal pcval,
                        uint32 *slotp, LIns** v_insp, Value *outp)
{
    const JSCodeSpec& cs = js_CodeSpec[*cx->regs->pc];
    uint32 setflags = (cs.format & (JOF_INCDEC | JOF_FOR));
    JS_ASSERT(!(cs.format & JOF_SET));

    const Shape* shape;
    uint32 slot;
    bool isMethod;

    if (pcval.isShape()) {
        shape = pcval.toShape();
        JS_ASSERT(obj2->nativeContains(*shape));

        if (setflags && !shape->hasDefaultSetter())
            RETURN_STOP("non-stub setter");
        if (setflags && !shape->writable())
            RETURN_STOP("writing to a readonly property");
        if (!shape->hasDefaultGetterOrIsMethod()) {
            if (slotp)
                RETURN_STOP("can't trace non-stub getter for this opcode");
            if (shape->hasGetterValue())
                return getPropertyWithScriptGetter(obj, obj_ins, shape);
            if (shape->slot == SHAPE_INVALID_SLOT)
                return getPropertyWithNativeGetter(obj_ins, shape, outp);
            return getPropertyById(obj_ins, outp);
        }
        if (!obj2->containsSlot(shape->slot))
            RETURN_STOP("no valid slot");
        slot = shape->slot;
        isMethod = shape->isMethod();
        JS_ASSERT_IF(isMethod, obj2->hasMethodBarrier());
    } else {
        if (!pcval.isSlot())
            RETURN_STOP("PCE is not a slot");
        slot = pcval.toSlot();
        shape = NULL;
        isMethod = false;
    }

    
    if (obj2 != obj) {
        if (setflags)
            RETURN_STOP("JOF_INCDEC|JOF_FOR opcode hit prototype chain");

        













        obj_ins = (obj2 == obj->getProto()) ? stobj_get_proto(obj_ins) : INS_CONSTOBJ(obj2);
        obj = obj2;
    }

    LIns* v_ins;
    if (obj2 == globalObj) {
        if (isMethod)
            RETURN_STOP("get global method");
        if (!lazilyImportGlobalSlot(slot))
            RETURN_STOP("lazy import of global slot failed");
        v_ins = get(&globalObj->getSlotRef(slot));
    } else {
        v_ins = unbox_slot(obj, obj_ins, slot, snapshot(BRANCH_EXIT));
    }

    









    if (isMethod && !cx->fp()->hasIMacroPC()) {
        enterDeepBailCall();
        LIns* args[] = { v_ins, INS_CONSTSHAPE(shape), obj_ins, cx_ins };
        v_ins = lir->insCall(&MethodReadBarrier_ci, args);
        leaveDeepBailCall();
    }

    if (slotp) {
        *slotp = slot;
        *v_insp = v_ins;
    }
    if (outp)
        set(outp, v_ins);
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::denseArrayElement(Value& oval, Value& ival, Value*& vp, LIns*& v_ins,
                                 LIns*& addr_ins)
{
    JS_ASSERT(oval.isObject() && ival.isInt32());

    JSObject* obj = &oval.toObject();
    LIns* obj_ins = get(&oval);
    jsint idx = ival.toInt32();
    LIns* idx_ins = makeNumberInt32(get(&ival));

    VMSideExit* exit = snapshot(BRANCH_EXIT);

    






    LIns* capacity_ins =
        addName(stobj_get_fslot_uint32(obj_ins, JSObject::JSSLOT_DENSE_ARRAY_CAPACITY),
                "capacity");
    jsuint capacity = obj->getDenseArrayCapacity();
    bool within = (jsuint(idx) < capacity);
    if (!within) {
        
        guard(true, lir->ins2(LIR_geui, idx_ins, capacity_ins), exit);

        CHECK_STATUS(guardPrototypeHasNoIndexedProperties(obj, obj_ins, MISMATCH_EXIT));

        
        v_ins = INS_UNDEFINED();
        addr_ins = NULL;
        return RECORD_CONTINUE;
    }

    
    guard(true, lir->ins2(LIR_ltui, idx_ins, capacity_ins), exit);

    
    LIns* dslots_ins =
        addName(lir->insLoad(LIR_ldp, obj_ins, offsetof(JSObject, dslots), ACCSET_OTHER), "dslots");
    vp = &obj->dslots[jsuint(idx)];
	JS_ASSERT(sizeof(Value) == 8); 
    addr_ins = lir->ins2(LIR_addp, dslots_ins,
                         lir->ins2ImmI(LIR_lshp, lir->insUI2P(idx_ins), 3));
    v_ins = unbox_value(*vp, addr_ins, 0, exit);

    
    if (vp->isMagic()) {
        CHECK_STATUS(guardPrototypeHasNoIndexedProperties(obj, obj_ins, MISMATCH_EXIT));
        v_ins = INS_UNDEFINED();
    }
    return RECORD_CONTINUE;
}


LIns *
TraceRecorder::canonicalizeNaNs(LIns *dval_ins)
{
    
    LIns *isnonnan_ins = lir->ins2(LIR_eqd, dval_ins, dval_ins);
    return lir->insChoose(isnonnan_ins, dval_ins, lir->insImmD(js_NaN), true);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::typedArrayElement(Value& oval, Value& ival, Value*& vp, LIns*& v_ins,
                                 LIns*& addr_ins)
{
    JS_ASSERT(oval.isObject() && ival.isInt32());

    JSObject* obj = &oval.toObject();
    LIns* obj_ins = get(&oval);
    jsint idx = ival.toInt32();
    LIns* idx_ins = makeNumberInt32(get(&ival));
    LIns* pidx_ins = lir->insUI2P(idx_ins);

    js::TypedArray* tarray = js::TypedArray::fromJSObject(obj);
    JS_ASSERT(tarray);

    
    LIns* priv_ins = stobj_get_const_private_ptr(obj_ins);

    
    if ((jsuint) idx >= tarray->length) {
        guard(false,
              lir->ins2(LIR_ltui,
                        idx_ins,
                        lir->insLoad(LIR_ldi, priv_ins, js::TypedArray::lengthOffset(),
                        ACCSET_OTHER, LOAD_CONST)),
              BRANCH_EXIT);
        v_ins = INS_UNDEFINED();
        return ARECORD_CONTINUE;
    }

    








    guard(true,
          lir->ins2(LIR_ltui,
                    idx_ins,
                    lir->insLoad(LIR_ldi, priv_ins, js::TypedArray::lengthOffset(),
                                 ACCSET_OTHER, LOAD_CONST)),
          BRANCH_EXIT);

    

    LIns* data_ins = lir->insLoad(LIR_ldp, priv_ins, js::TypedArray::dataOffset(),
                                  ACCSET_OTHER, LOAD_CONST);

    switch (tarray->type) {
      case js::TypedArray::TYPE_INT8:
        addr_ins = lir->ins2(LIR_addp, data_ins, pidx_ins);
        v_ins = lir->ins1(LIR_i2d, lir->insLoad(LIR_ldc2i, addr_ins, 0, ACCSET_OTHER));
        break;
      case js::TypedArray::TYPE_UINT8:
      case js::TypedArray::TYPE_UINT8_CLAMPED:
        addr_ins = lir->ins2(LIR_addp, data_ins, pidx_ins);
        v_ins = lir->ins1(LIR_ui2d, lir->insLoad(LIR_lduc2ui, addr_ins, 0, ACCSET_OTHER));
        break;
      case js::TypedArray::TYPE_INT16:
        addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 1));
        v_ins = lir->ins1(LIR_i2d, lir->insLoad(LIR_lds2i, addr_ins, 0, ACCSET_OTHER));
        break;
      case js::TypedArray::TYPE_UINT16:
        addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 1));
        v_ins = lir->ins1(LIR_ui2d, lir->insLoad(LIR_ldus2ui, addr_ins, 0, ACCSET_OTHER));
        break;
      case js::TypedArray::TYPE_INT32:
        addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 2));
        v_ins = lir->ins1(LIR_i2d, lir->insLoad(LIR_ldi, addr_ins, 0, ACCSET_OTHER));
        break;
      case js::TypedArray::TYPE_UINT32:
        addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 2));
        v_ins = lir->ins1(LIR_ui2d, lir->insLoad(LIR_ldi, addr_ins, 0, ACCSET_OTHER));
        break;
      case js::TypedArray::TYPE_FLOAT32:
        addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 2));
        v_ins = canonicalizeNaNs(lir->insLoad(LIR_ldf2d, addr_ins, 0, ACCSET_OTHER));
        break;
      case js::TypedArray::TYPE_FLOAT64:
        addr_ins = lir->ins2(LIR_addp, data_ins, lir->ins2ImmI(LIR_lshp, pidx_ins, 3));
        v_ins = canonicalizeNaNs(lir->insLoad(LIR_ldd, addr_ins, 0, ACCSET_OTHER));
        break;
      default:
        JS_NOT_REACHED("Unknown typed array type in tracer");
    }

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::getProp(JSObject* obj, LIns* obj_ins)
{
    JSOp op = JSOp(*cx->regs->pc);
    const JSCodeSpec& cs = js_CodeSpec[op];

    JS_ASSERT(cs.ndefs == 1);
    return prop(obj, obj_ins, NULL, NULL, &stackval(-cs.nuses));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::getProp(Value& v)
{
    if (v.isPrimitive())
        RETURN_STOP_A("primitive lhs");

    return getProp(&v.toObject(), get(&v));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NAME()
{
    Value* vp;
    LIns* v_ins;
    NameResult nr;
    CHECK_STATUS_A(name(vp, v_ins, nr));
    stack(0, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DOUBLE()
{
    double d = consts[GET_INDEX(cx->regs->pc)].toDouble();
    stack(0, lir->insImmD(d));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_STRING()
{
    JSAtom* atom = atoms[GET_INDEX(cx->regs->pc)];
    stack(0, INS_ATOM(atom));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ZERO()
{
    stack(0, lir->insImmD(0));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ONE()
{
    stack(0, lir->insImmD(1));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NULL()
{
    stack(0, INS_NULL());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_THIS()
{
    LIns* this_ins;
    CHECK_STATUS_A(getThis(this_ins));
    stack(0, this_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FALSE()
{
    stack(0, lir->insImmI(0));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TRUE()
{
    stack(0, lir->insImmI(1));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_OR()
{
    return ifop();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_AND()
{
    return ifop();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TABLESWITCH()
{
#ifdef NANOJIT_IA32
    
    return tableswitch();
#else
    return InjectStatus(switchop());
#endif
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LOOKUPSWITCH()
{
    return InjectStatus(switchop());
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_STRICTEQ()
{
    strictEquality(true, false);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_STRICTNE()
{
    strictEquality(false, false);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_OBJECT()
{
    JSStackFrame* const fp = cx->fp();
    JSScript* script = fp->getScript();
    unsigned index = atoms - script->atomMap.vector + GET_INDEX(cx->regs->pc);

    JSObject* obj;
    obj = script->getObject(index);
    stack(0, INS_CONSTOBJ(obj));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_POP()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TRAP()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETARG()
{
    stack(0, arg(GET_ARGNO(cx->regs->pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETARG()
{
    arg(GET_ARGNO(cx->regs->pc), stack(-1));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETLOCAL()
{
    stack(0, var(GET_SLOTNO(cx->regs->pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETLOCAL()
{
    var(GET_SLOTNO(cx->regs->pc), stack(-1));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_UINT16()
{
    stack(0, lir->insImmD(GET_UINT16(cx->regs->pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NEWINIT()
{
    JSProtoKey key = JSProtoKey(GET_INT8(cx->regs->pc));
    LIns* proto_ins;
    CHECK_STATUS_A(getClassPrototype(key, proto_ins));

    LIns *v_ins;
    if (key == JSProto_Array) {
        LIns *args[] = { lir->insImmI(0), proto_ins, cx_ins };
        v_ins = lir->insCall(&js_NewEmptyArray_ci, args);
    } else {
        LIns *args[] = { proto_ins, cx_ins };
        v_ins = lir->insCall((cx->regs->pc[JSOP_NEWINIT_LENGTH] != JSOP_ENDINIT)
                             ? &js_NonEmptyObject_ci
                             : &js_Object_tn_ci,
                             args);
    }
    guard(false, lir->insEqP_0(v_ins), OOM_EXIT);
    stack(0, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ENDINIT()
{
#ifdef DEBUG
    Value& v = stackval(-1);
    JS_ASSERT(!v.isPrimitive());
#endif
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INITPROP()
{
    
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INITELEM()
{
    return setElem(-3, -2, -1);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFSHARP()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_USESHARP()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INCARG()
{
    return InjectStatus(inc(argval(GET_ARGNO(cx->regs->pc)), 1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INCLOCAL()
{
    return InjectStatus(inc(varval(GET_SLOTNO(cx->regs->pc)), 1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DECARG()
{
    return InjectStatus(inc(argval(GET_ARGNO(cx->regs->pc)), -1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DECLOCAL()
{
    return InjectStatus(inc(varval(GET_SLOTNO(cx->regs->pc)), -1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARGINC()
{
    return InjectStatus(inc(argval(GET_ARGNO(cx->regs->pc)), 1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LOCALINC()
{
    return InjectStatus(inc(varval(GET_SLOTNO(cx->regs->pc)), 1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARGDEC()
{
    return InjectStatus(inc(argval(GET_ARGNO(cx->regs->pc)), -1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LOCALDEC()
{
    return InjectStatus(inc(varval(GET_SLOTNO(cx->regs->pc)), -1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IMACOP()
{
    JS_ASSERT(cx->fp()->hasIMacroPC());
    return ARECORD_CONTINUE;
}

static JSBool FASTCALL
ObjectToIterator(JSContext* cx, JSObject *obj, int32 flags, JSObject **objp)
{
    AutoValueRooter tvr(cx, ObjectValue(*obj));
    bool ok = js_ValueToIterator(cx, flags, tvr.addr());
    if (!ok) {
        SetBuiltinError(cx);
        return false;
    }
    *objp = &tvr.value().toObject();
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, ObjectToIterator, CONTEXT, OBJECT, INT32, OBJECTPTR,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ITER()
{
    Value& v = stackval(-1);
    if (v.isPrimitive())
        RETURN_STOP_A("for-in on a primitive value");

    RETURN_IF_XML_A(v);

    LIns *obj_ins = get(&v);
    jsuint flags = cx->regs->pc[1];

    enterDeepBailCall();

    LIns* objp_ins = lir->insAlloc(sizeof(JSObject*));
    LIns* args[] = { objp_ins, lir->insImmI(flags), obj_ins, cx_ins };
    LIns* ok_ins = lir->insCall(&ObjectToIterator_ci, args);

    
    
    pendingGuardCondition = ok_ins;

    leaveDeepBailCall();

    stack(-1, addName(lir->insLoad(LIR_ldp, objp_ins, 0, ACCSET_OTHER), "iterobj"));

    return ARECORD_CONTINUE;
}

static JSBool FASTCALL
IteratorMore(JSContext *cx, JSObject *iterobj, Value *vp)
{
    AutoValueRooter tvr(cx);
    if (!js_IteratorMore(cx, iterobj, tvr.addr())) {
        SetBuiltinError(cx);
        return false;
    }
    *vp = tvr.value();
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_3(extern, BOOL_FAIL, IteratorMore, CONTEXT, OBJECT, VALUEPTR,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_MOREITER()
{
    Value& iterobj_val = stackval(-1);
    if (iterobj_val.isPrimitive())
        RETURN_STOP_A("for-in on a primitive value");

    RETURN_IF_XML_A(iterobj_val);

    JSObject* iterobj = &iterobj_val.toObject();
    LIns* iterobj_ins = get(&iterobj_val);
    bool cond;
    LIns* cond_ins;

    
    if (iterobj->hasClass(&js_IteratorClass)) {
        guardClass(iterobj_ins, &js_IteratorClass, snapshot(BRANCH_EXIT), LOAD_NORMAL);
        NativeIterator *ni = (NativeIterator *) iterobj->getPrivate();
        void *cursor = ni->props_cursor;
        void *end = ni->props_end;

        LIns *ni_ins = stobj_get_const_private_ptr(iterobj_ins);
        LIns *cursor_ins =
            addName(lir->insLoad(LIR_ldp, ni_ins,
                                 offsetof(NativeIterator, props_cursor), ACCSET_OTHER), "cursor");
        LIns *end_ins = addName(lir->insLoad(LIR_ldp, ni_ins, offsetof(NativeIterator, props_end),
                                ACCSET_OTHER), "end");

        
        cond = cursor < end;
        cond_ins = lir->ins2(LIR_ltp, cursor_ins, end_ins);
    } else {
        guardNotClass(iterobj_ins, &js_IteratorClass, snapshot(BRANCH_EXIT), LOAD_NORMAL);

        enterDeepBailCall();

        LIns* vp_ins = lir->insAlloc(sizeof(Value));
        LIns* args[] = { vp_ins, iterobj_ins, cx_ins };
        LIns* ok_ins = lir->insCall(&IteratorMore_ci, args);

        





        guard(true, ok_ins, OOM_EXIT);

        leaveDeepBailCall();

        



        JSContext *localCx = cx;
        AutoValueRooter rooter(cx);
        if (!js_IteratorMore(cx, iterobj, rooter.addr()))
            RETURN_ERROR_A("error in js_IteratorMore");
        if (!TRACE_RECORDER(localCx))
            return ARECORD_ABORTED;

        cond = (rooter.value().isTrue());
        cond_ins = is_boxed_true(vp_ins, ACCSET_OTHER);
    }

    jsbytecode* pc = cx->regs->pc;

    if (pc[1] == JSOP_IFNE) {
        fuseIf(pc + 1, cond, cond_ins);
        return checkTraceEnd(pc + 1);
    }

    stack(0, cond_ins);

    return ARECORD_CONTINUE;
}

static JSBool FASTCALL
CloseIterator(JSContext *cx, JSObject *iterobj)
{
    if (!js_CloseIterator(cx, iterobj)) {
        SetBuiltinError(cx);
        return false;
    }
    return cx->tracerState->builtinStatus == 0;
}
JS_DEFINE_CALLINFO_2(extern, BOOL_FAIL, CloseIterator, CONTEXT, OBJECT, 0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ENDITER()
{
    JS_ASSERT(!stackval(-1).isPrimitive());

    enterDeepBailCall();

    LIns* args[] = { stack(-1), cx_ins };
    LIns* ok_ins = lir->insCall(&CloseIterator_ci, args);

    
    
    pendingGuardCondition = ok_ins;

    leaveDeepBailCall();

    return ARECORD_CONTINUE;
}

#if JS_BITS_PER_WORD == 32
JS_REQUIRES_STACK void
TraceRecorder::storeMagic(JSWhyMagic why, nanojit::LIns *addr_ins, ptrdiff_t offset, AccSet accSet)
{
    lir->insStore(INS_CONSTU(why), addr_ins, offset + sPayloadOffset, accSet);
    lir->insStore(INS_CONSTU(JSVAL_TAG_MAGIC), addr_ins, offset + sTagOffset, accSet);
}
#elif JS_BITS_PER_WORD == 64
JS_REQUIRES_STACK void
TraceRecorder::storeMagic(JSWhyMagic why, nanojit::LIns *addr_ins, ptrdiff_t offset, AccSet accSet)
{
    LIns *magic = INS_CONSTQWORD(BUILD_JSVAL(JSVAL_TAG_MAGIC, why));
    lir->insStore(magic, addr_ins, offset, accSet);
}
#endif

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::unboxNextValue(LIns* &v_ins)
{
    Value &iterobj_val = stackval(-1);
    JSObject *iterobj = &iterobj_val.toObject();
    LIns* iterobj_ins = get(&iterobj_val);

    if (iterobj->hasClass(&js_IteratorClass)) {
        guardClass(iterobj_ins, &js_IteratorClass, snapshot(BRANCH_EXIT), LOAD_NORMAL);
        NativeIterator *ni = (NativeIterator *) iterobj->getPrivate();

        LIns *ni_ins = stobj_get_const_private_ptr(iterobj_ins);
        LIns *cursor_ins = addName(lir->insLoad(LIR_ldp, ni_ins, offsetof(NativeIterator, props_cursor), ACCSET_OTHER), "cursor");

        
        if (!(((NativeIterator *) iterobj->getPrivate())->flags & JSITER_FOREACH)) {
            
            jsid id = *ni->currentKey();
            LIns *id_ins = addName(lir->insLoad(LIR_ldp, cursor_ins, 0, ACCSET_OTHER), "id");

            



            guard(JSID_IS_STRING(id), is_string_id(id_ins), snapshot(BRANCH_EXIT));

            if (JSID_IS_STRING(id)) {
                v_ins = unbox_string_id(id_ins);
            } else {
                
                JS_ASSERT(JSID_IS_INT(id));
                LIns *id_to_int_ins = unbox_int_id(id_ins);
                LIns* args[] = { id_to_int_ins, cx_ins };
                v_ins = lir->insCall(&js_IntToString_ci, args);
                guard(false, lir->insEqP_0(v_ins), OOM_EXIT);
            }

            
            cursor_ins = lir->ins2(LIR_addp, cursor_ins, INS_CONSTWORD(sizeof(jsid)));
        } else {
            
            Value v = *ni->currentValue();
            v_ins = unbox_value(v, cursor_ins, 0, snapshot(BRANCH_EXIT));

            
            cursor_ins = lir->ins2(LIR_addp, cursor_ins, INS_CONSTWORD(sizeof(Value)));
        }

        lir->insStore(LIR_stp, cursor_ins, ni_ins, offsetof(NativeIterator, props_cursor),
                      ACCSET_OTHER);
    } else {
        guardNotClass(iterobj_ins, &js_IteratorClass, snapshot(BRANCH_EXIT), LOAD_NORMAL);

        v_ins = unbox_value(cx->iterValue, cx_ins, offsetof(JSContext, iterValue),
                            snapshot(BRANCH_EXIT));
        storeMagic(JS_NO_ITER_VALUE, cx_ins, offsetof(JSContext, iterValue), ACCSET_OTHER);
    }

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FORNAME()
{
    Value* vp;
    LIns* x_ins;
    NameResult nr;
    CHECK_STATUS_A(name(vp, x_ins, nr));
    if (!nr.tracked)
        RETURN_STOP_A("forname on non-tracked value not supported");
    LIns* v_ins;
    CHECK_STATUS_A(unboxNextValue(v_ins));
    set(vp, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FORPROP()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FORELEM()
{
    LIns* v_ins;
    CHECK_STATUS_A(unboxNextValue(v_ins));
    stack(0, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FORARG()
{
    LIns* v_ins;
    CHECK_STATUS_A(unboxNextValue(v_ins));
    arg(GET_ARGNO(cx->regs->pc), v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FORLOCAL()
{
    LIns* v_ins;
    CHECK_STATUS_A(unboxNextValue(v_ins));
    var(GET_SLOTNO(cx->regs->pc), v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FORGLOBAL()
{
    LIns* v_ins;
    CHECK_STATUS_A(unboxNextValue(v_ins));

    uint32 slot = cx->fp()->getScript()->getGlobalSlot(GET_SLOTNO(cx->regs->pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    set(&globalObj->getSlotRef(slot), v_ins);
    return ARECORD_CONTINUE;
}


JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_POPN()
{
    return ARECORD_CONTINUE;
}










JS_REQUIRES_STACK RecordingStatus
TraceRecorder::traverseScopeChain(JSObject *obj, LIns *obj_ins, JSObject *targetObj,
                                  LIns *&targetIns)
{
    VMSideExit* exit = NULL;

    



















    bool foundCallObj = false;
    bool foundBlockObj = false;
    JSObject* searchObj = obj;

    for (;;) {
        if (searchObj != globalObj) {
            Class* clasp = searchObj->getClass();
            if (clasp == &js_BlockClass) {
                foundBlockObj = true;
            } else if (clasp == &js_CallClass &&
                       JSFUN_HEAVYWEIGHT_TEST(js_GetCallObjectFunction(searchObj)->flags)) {
                foundCallObj = true;
            }
        }

        if (searchObj == targetObj)
            break;

        searchObj = searchObj->getParent();
        if (!searchObj)
            RETURN_STOP("cannot traverse this scope chain on trace");
    }

    if (!foundCallObj) {
        JS_ASSERT(targetObj == globalObj);
        targetIns = INS_CONSTPTR(globalObj);
        return RECORD_CONTINUE;
    }

    if (foundBlockObj)
        RETURN_STOP("cannot traverse this scope chain on trace");

    
    for (;;) {
        if (obj != globalObj) {
            if (!js_IsCacheableNonGlobalScope(obj))
                RETURN_STOP("scope chain lookup crosses non-cacheable object");

            
            
            
            if (obj->isCall() &&
                JSFUN_HEAVYWEIGHT_TEST(js_GetCallObjectFunction(obj)->flags)) {
                if (!exit)
                    exit = snapshot(BRANCH_EXIT);
                guard(true,
                      addName(lir->ins2ImmI(LIR_eqi, shape_ins(obj_ins), obj->shape()),
                              "guard_shape"),
                      exit);
            }
        }

        JS_ASSERT(obj->getClass() != &js_BlockClass);

        if (obj == targetObj)
            break;

        obj = obj->getParent();
        obj_ins = stobj_get_parent(obj_ins);
    }

    targetIns = obj_ins;
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BINDNAME()
{
    JSStackFrame* const fp = cx->fp();
    JSObject *obj;

    if (!fp->hasFunction()) {
        obj = fp->getScopeChain();

#ifdef DEBUG
        JSStackFrame *fp2 = fp;
#endif

        
        
        while (obj->getClass() == &js_BlockClass) {
            
#ifdef DEBUG
            
            while (obj->getPrivate() != fp2) {
                JS_ASSERT(fp2->flags & JSFRAME_SPECIAL);
                fp2 = fp2->down;
                if (!fp2)
                    JS_NOT_REACHED("bad stack frame");
            }
#endif
            obj = obj->getParent();
            
            JS_ASSERT(obj);
        }

        
        
        
        JS_ASSERT(obj == globalObj);

        






        stack(0, INS_CONSTOBJ(obj));
        return ARECORD_CONTINUE;
    }

    
    
    
    if (JSFUN_HEAVYWEIGHT_TEST(fp->getFunction()->flags))
        RETURN_STOP_A("BINDNAME in heavyweight function.");

    
    
    
    Value *callee = &cx->fp()->argv[-2];
    obj = callee->toObject().getParent();
    if (obj == globalObj) {
        stack(0, INS_CONSTOBJ(obj));
        return ARECORD_CONTINUE;
    }
    LIns *obj_ins = stobj_get_parent(get(callee));

    
    JSAtom *atom = atoms[GET_INDEX(cx->regs->pc)];
    jsid id = ATOM_TO_JSID(atom);
    JSContext *localCx = cx;
    JSObject *obj2 = js_FindIdentifierBase(cx, fp->getScopeChain(), id);
    if (!obj2)
        RETURN_ERROR_A("error in js_FindIdentifierBase");
    if (!TRACE_RECORDER(localCx))
        return ARECORD_ABORTED;
    if (obj2 != globalObj && !obj2->isCall())
        RETURN_STOP_A("BINDNAME on non-global, non-call object");

    
    LIns *obj2_ins;
    CHECK_STATUS_A(traverseScopeChain(obj, obj_ins, obj2, obj2_ins));

    
    
    stack(0, obj2 == globalObj ? INS_CONSTOBJ(obj2) : obj2_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETNAME()
{
    
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_THROW()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IN()
{
    Value& rval = stackval(-1);
    Value& lval = stackval(-2);

    if (rval.isPrimitive())
        RETURN_STOP_A("JSOP_IN on non-object right operand");
    JSObject* obj = &rval.toObject();
    LIns* obj_ins = get(&rval);

    jsid id;
    LIns* x;
    if (lval.isInt32()) {
        if (!js_Int32ToId(cx, lval.toInt32(), &id))
            RETURN_ERROR_A("OOM converting left operand of JSOP_IN to string");
        LIns* args[] = { makeNumberInt32(get(&lval)), obj_ins, cx_ins };
        x = lir->insCall(&js_HasNamedPropertyInt32_ci, args);
    } else if (lval.isString()) {
        if (!js_ValueToStringId(cx, lval, &id))
            RETURN_ERROR_A("left operand of JSOP_IN didn't convert to a string-id");
        LIns* args[] = { get(&lval), obj_ins, cx_ins };
        x = lir->insCall(&js_HasNamedProperty_ci, args);
    } else {
        RETURN_STOP_A("string or integer expected");
    }

    guard(false, lir->ins2ImmI(LIR_eqi, x, JS_NEITHER), OOM_EXIT);
    x = lir->ins2ImmI(LIR_eqi, x, 1);

    TraceMonitor &localtm = *traceMonitor;
    JSContext *localcx = cx;

    JSObject* obj2;
    JSProperty* prop;
    JSBool ok = obj->lookupProperty(cx, id, &obj2, &prop);

    if (!ok)
        RETURN_ERROR_A("obj->lookupProperty failed in JSOP_IN");

    
    if (!localtm.recorder) {
        if (prop)
            obj2->dropProperty(localcx, prop);
        return ARECORD_ABORTED;
    }

    bool cond = prop != NULL;
    if (prop)
        obj2->dropProperty(cx, prop);

    



    fuseIf(cx->regs->pc + 1, cond, x);

    





    set(&lval, x);
    return ARECORD_CONTINUE;
}

static JSBool FASTCALL
HasInstanceOnTrace(JSContext* cx, JSObject* ctor, ValueArgType arg)
{
    const Value &argref = ValueArgToConstRef(arg);
    JSBool result = JS_FALSE;
    if (!HasInstance(cx, ctor, &argref, &result))
        SetBuiltinError(cx);
    return result;
}
JS_DEFINE_CALLINFO_3(static, BOOL_FAIL, HasInstanceOnTrace, CONTEXT, OBJECT, VALUE,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INSTANCEOF()
{
    
    Value& ctor = stackval(-1);
    if (ctor.isPrimitive())
        RETURN_STOP_A("non-object on rhs of instanceof");

    Value& val = stackval(-2);
    LIns* val_ins = box_value_for_native_call(val, get(&val));

    enterDeepBailCall();
    LIns* args[] = {val_ins, get(&ctor), cx_ins};
    stack(-2, lir->insCall(&HasInstanceOnTrace_ci, args));
    LIns* status_ins = lir->insLoad(LIR_ldi,
                                    lirbuf->state,
                                    offsetof(TracerState, builtinStatus), ACCSET_OTHER);
    pendingGuardCondition = lir->insEqI_0(status_ins);
    leaveDeepBailCall();

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEBUGGER()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GOSUB()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_RETSUB()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_EXCEPTION()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LINENO()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CONDSWITCH()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CASE()
{
    strictEquality(true, true);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFAULT()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_EVAL()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ENUMELEM()
{
    



    return setElem(-2, -1, -3);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETTER()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETTER()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFFUN()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFFUN_FC()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFCONST()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFVAR()
{
    return ARECORD_STOP;
}

jsatomid
TraceRecorder::getFullIndex(ptrdiff_t pcoff)
{
    jsatomid index = GET_INDEX(cx->regs->pc + pcoff);
    index += atoms - cx->fp()->getScript()->atomMap.vector;
    return index;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LAMBDA()
{
    JSFunction* fun;
    fun = cx->fp()->getScript()->getFunction(getFullIndex());

    










    if (FUN_NULL_CLOSURE(fun)) {
        if (FUN_OBJECT(fun)->getParent() != globalObj)
            RETURN_STOP_A("Null closure function object parent must be global object");

        jsbytecode *pc2 = cx->regs->pc + JSOP_LAMBDA_LENGTH;
        JSOp op2 = JSOp(*pc2);

        if (op2 == JSOP_INITMETHOD) {
            stack(0, INS_CONSTOBJ(FUN_OBJECT(fun)));
            return ARECORD_CONTINUE;
        }

        if (op2 == JSOP_SETMETHOD) {
            Value lval = stackval(-1);

            if (!lval.isPrimitive() && lval.toObject().canHaveMethodBarrier()) {
                stack(0, INS_CONSTOBJ(FUN_OBJECT(fun)));
                return ARECORD_CONTINUE;
            }
        } else if (fun->joinable()) {
            if (op2 == JSOP_CALL) {
                






                int iargc = GET_ARGC(pc2);

                




                const Value &cref = cx->regs->sp[1 - (iargc + 2)];
                JSObject *callee;

                if (IsFunctionObject(cref, &callee)) {
                    JSFunction *calleeFun = GET_FUNCTION_PRIVATE(cx, callee);
                    FastNative fastNative = FUN_FAST_NATIVE(calleeFun);

                    if ((iargc == 1 && fastNative == array_sort) ||
                        (iargc == 2 && fastNative == str_replace)) {
                        stack(0, INS_CONSTOBJ(FUN_OBJECT(fun)));
                        return ARECORD_CONTINUE;
                    }
                }
            } else if (op2 == JSOP_NULL) {
                pc2 += JSOP_NULL_LENGTH;
                op2 = JSOp(*pc2);

                if (op2 == JSOP_CALL && GET_ARGC(pc2) == 0) {
                    stack(0, INS_CONSTOBJ(FUN_OBJECT(fun)));
                    return ARECORD_CONTINUE;
                }
            }
        }

        LIns *proto_ins;
        CHECK_STATUS_A(getClassPrototype(JSProto_Function, proto_ins));

        LIns* args[] = { INS_CONSTOBJ(globalObj), proto_ins, INS_CONSTFUN(fun), cx_ins };
        LIns* x = lir->insCall(&js_NewNullClosure_ci, args);
        stack(0, x);
        return ARECORD_CONTINUE;
    }

    LIns *proto_ins;
    CHECK_STATUS_A(getClassPrototype(JSProto_Function, proto_ins));
    LIns* scopeChain_ins = scopeChain();
    JS_ASSERT(scopeChain_ins);
    LIns* args[] = { proto_ins, scopeChain_ins, INS_CONSTPTR(fun), cx_ins };
    LIns* call_ins = lir->insCall(&js_CloneFunctionObject_ci, args);
    guard(false,
          addName(lir->insEqP_0(call_ins), "guard(js_CloneFunctionObject)"),
          OOM_EXIT);
    stack(0, call_ins);

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LAMBDA_FC()
{
    JSFunction* fun;
    fun = cx->fp()->getScript()->getFunction(getFullIndex());

    if (FUN_OBJECT(fun)->getParent() != globalObj)
        return ARECORD_STOP;

    LIns* args[] = {
        scopeChain(),
        INS_CONSTFUN(fun),
        cx_ins
    };
    LIns* closure_ins = lir->insCall(&js_AllocFlatClosure_ci, args);
    guard(false,
          addName(lir->ins2(LIR_eqp, closure_ins, INS_NULL()),
                  "guard(js_AllocFlatClosure)"),
          OOM_EXIT);

    if (fun->u.i.nupvars) {
        JSUpvarArray *uva = fun->u.i.script->upvars();
        LIns* upvars_ins = stobj_get_const_private_ptr(closure_ins,
                                                       JSObject::JSSLOT_FLAT_CLOSURE_UPVARS);

        for (uint32 i = 0, n = uva->length; i < n; i++) {
            Value v;
            LIns* v_ins = upvar(fun->u.i.script, uva, i, v);
            if (!v_ins)
                return ARECORD_STOP;

            box_value_into(v, v_ins, upvars_ins, i * sizeof(Value), ACCSET_OTHER);
        }
    }

    stack(0, closure_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLEE()
{
    stack(0, get(&cx->fp()->argv[-2]));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETLOCALPOP()
{
    var(GET_SLOTNO(cx->regs->pc), stack(-1));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IFPRIMTOP()
{
    
    
    
    
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETCALL()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TRY()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FINALLY()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NOP()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARGSUB()
{
    JSStackFrame* const fp = cx->fp();
    if (!(fp->getFunction()->flags & JSFUN_HEAVYWEIGHT)) {
        uintN slot = GET_ARGNO(cx->regs->pc);
        if (slot >= fp->numActualArgs())
            RETURN_STOP_A("can't trace out-of-range arguments");
        stack(0, get(&cx->fp()->argv[slot]));
        return ARECORD_CONTINUE;
    }
    RETURN_STOP_A("can't trace JSOP_ARGSUB hard case");
}

JS_REQUIRES_STACK LIns*
TraceRecorder::guardArgsLengthNotAssigned(LIns* argsobj_ins)
{
    
    
    LIns *len_ins = stobj_get_fslot_uint32(argsobj_ins, JSObject::JSSLOT_ARGS_LENGTH);
    LIns *ovr_ins = lir->ins2(LIR_andi, len_ins, INS_CONST(JSObject::ARGS_LENGTH_OVERRIDDEN_BIT));
    guard(true, lir->insEqI_0(ovr_ins), snapshot(BRANCH_EXIT));
    return len_ins;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARGCNT()
{
    JSStackFrame * const fp = cx->fp();

    if (fp->getFunction()->flags & JSFUN_HEAVYWEIGHT)
        RETURN_STOP_A("can't trace heavyweight JSOP_ARGCNT");

    
    
    
    
    
    
    if (fp->hasArgsObj() && fp->getArgsObj()->isArgsLengthOverridden())
        RETURN_STOP_A("can't trace JSOP_ARGCNT if arguments.length has been modified");
    LIns *a_ins = getFrameObjPtr(fp->addressArgsObj());
    if (callDepth == 0) {
        LIns *br = lir->insBranch(LIR_jt, lir->insEqP_0(a_ins), NULL);
        guardArgsLengthNotAssigned(a_ins);
        LIns *label = lir->ins0(LIR_label);
        br->setTarget(label);
    }
    stack(0, lir->insImmD(fp->numActualArgs()));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_DefLocalFunSetSlot(uint32 slot, JSObject* obj)
{
    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, obj);

    if (FUN_NULL_CLOSURE(fun) && FUN_OBJECT(fun)->getParent() == globalObj) {
        LIns *proto_ins;
        CHECK_STATUS_A(getClassPrototype(JSProto_Function, proto_ins));

        LIns* args[] = { INS_CONSTOBJ(globalObj), proto_ins, INS_CONSTFUN(fun), cx_ins };
        LIns* x = lir->insCall(&js_NewNullClosure_ci, args);
        var(slot, x);
        return ARECORD_CONTINUE;
    }

    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFLOCALFUN()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFLOCALFUN_FC()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GOTOX()
{
    return record_JSOP_GOTO();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IFEQX()
{
    return record_JSOP_IFEQ();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IFNEX()
{
    return record_JSOP_IFNE();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ORX()
{
    return record_JSOP_OR();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ANDX()
{
    return record_JSOP_AND();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GOSUBX()
{
    return record_JSOP_GOSUB();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CASEX()
{
    strictEquality(true, true);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFAULTX()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TABLESWITCHX()
{
    return record_JSOP_TABLESWITCH();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LOOKUPSWITCHX()
{
    return InjectStatus(switchop());
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BACKPATCH()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BACKPATCH_POP()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_THROWING()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETRVAL()
{
    
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_RETRVAL()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_REGEXP()
{
    JSStackFrame* const fp = cx->fp();
    JSScript* script = fp->getScript();
    unsigned index = atoms - script->atomMap.vector + GET_INDEX(cx->regs->pc);

    LIns* proto_ins;
    CHECK_STATUS_A(getClassPrototype(JSProto_RegExp, proto_ins));

    LIns* args[] = {
        proto_ins,
        INS_CONSTOBJ(script->getRegExp(index)),
        cx_ins
    };
    LIns* regex_ins = lir->insCall(&js_CloneRegExpObject_ci, args);
    guard(false, lir->insEqP_0(regex_ins), OOM_EXIT);

    stack(0, regex_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_UNUSED180()
{
    JS_NOT_REACHED("recording JSOP_UNUSED180?!?");
    return ARECORD_ERROR;
}



JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFXMLNS()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ANYNAME()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_QNAMEPART()
{
    return record_JSOP_STRING();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_QNAMECONST()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_QNAME()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TOATTRNAME()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TOATTRVAL()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ADDATTRNAME()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ADDATTRVAL()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BINDXMLNAME()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETXMLNAME()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_XMLNAME()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DESCENDANTS()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FILTER()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ENDFILTER()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TOXML()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TOXMLLIST()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_XMLTAGEXPR()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_XMLELTEXPR()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_XMLCDATA()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_XMLCOMMENT()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_XMLPI()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETFUNNS()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_STARTXML()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_STARTXMLEXPR()
{
    return ARECORD_STOP;
}



JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLPROP()
{
    Value& l = stackval(-1);
    JSObject* obj;
    LIns* obj_ins;
    LIns* this_ins;
    if (!l.isPrimitive()) {
        obj = &l.toObject();
        obj_ins = get(&l);
        this_ins = obj_ins; 
    } else {
        JSProtoKey protoKey;
        debug_only_stmt(const char* protoname = NULL;)
        if (l.isString()) {
            protoKey = JSProto_String;
            debug_only_stmt(protoname = "String.prototype";)
        } else if (l.isNumber()) {
            protoKey = JSProto_Number;
            debug_only_stmt(protoname = "Number.prototype";)
        } else if (l.isBoolean()) {
            protoKey = JSProto_Boolean;
            debug_only_stmt(protoname = "Boolean.prototype";)
        } else {
            JS_ASSERT(l.isNull() || l.isUndefined());
            RETURN_STOP_A("callprop on null or void");
        }

        if (!js_GetClassPrototype(cx, NULL, protoKey, &obj))
            RETURN_ERROR_A("GetClassPrototype failed!");

        obj_ins = INS_CONSTOBJ(obj);
        debug_only_stmt(obj_ins = addName(obj_ins, protoname);)
        this_ins = get(&l); 
    }

    JSObject* obj2;
    PCVal pcval;
    CHECK_STATUS_A(test_property_cache(obj, obj_ins, obj2, pcval));

    if (pcval.isNull())
        RETURN_STOP_A("callprop of missing method");

    if (pcval.isFunObj()) {
        if (l.isPrimitive()) {
            JSFunction* fun = GET_FUNCTION_PRIVATE(cx, &pcval.toFunObj());
            if (!PrimitiveThisTest(fun, l))
                RETURN_STOP_A("callee does not accept primitive |this|");
        }
        set(&l, INS_CONSTOBJ(&pcval.toFunObj()));
    } else {
        if (l.isPrimitive())
            RETURN_STOP_A("callprop of primitive method");
        JS_ASSERT_IF(pcval.isShape(), !pcval.toShape()->isMethod());
        CHECK_STATUS_A(propTail(obj, obj_ins, obj2, pcval, NULL, NULL, &l));
    }
    stack(0, this_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DELDESC()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_UINT24()
{
    stack(0, lir->insImmD(GET_UINT24(cx->regs->pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INDEXBASE()
{
    atoms += GET_INDEXBASE(cx->regs->pc);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_RESETBASE()
{
    updateAtoms();
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_RESETBASE0()
{
    updateAtoms();
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLELEM()
{
    return record_JSOP_GETELEM();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_STOP()
{
    JSStackFrame *fp = cx->fp();

    if (fp->hasIMacroPC()) {
        




        updateAtoms(fp->getScript());
        return ARECORD_CONTINUE;
    }

    putActivationObjects();

#ifdef MOZ_TRACE_JSCALLS
    if (cx->functionCallback) {
        LIns* args[] = { INS_CONST(0), INS_CONSTPTR(cx->fp()->getFunction()), cx_ins };
        LIns* call_ins = lir->insCall(&functionProbe_ci, args);
        guard(false, lir->insEqI_0(call_ins), MISMATCH_EXIT);
    }
#endif

    







    if (fp->flags & JSFRAME_CONSTRUCTING) {
        JS_ASSERT(fp->getThisValue() == fp->argv[-1]);
        rval_ins = get(&fp->argv[-1]);
    } else {
        rval_ins = INS_UNDEFINED();
    }
    clearCurrentFrameSlotsFromTracker(nativeFrameTracker);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETXPROP()
{
    Value& l = stackval(-1);
    if (l.isPrimitive())
        RETURN_STOP_A("primitive-this for GETXPROP?");

    Value* vp;
    LIns* v_ins;
    NameResult nr;
    CHECK_STATUS_A(name(vp, v_ins, nr));
    stack(-1, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLXMLNAME()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TYPEOFEXPR()
{
    return record_JSOP_TYPEOF();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ENTERBLOCK()
{
    JSObject* obj;
    obj = cx->fp()->getScript()->getObject(getFullIndex(0));

    LIns* void_ins = INS_UNDEFINED();
    for (int i = 0, n = OBJ_BLOCK_COUNT(cx, obj); i < n; i++)
        stack(i, void_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LEAVEBLOCK()
{
    
    if (cx->fp()->getBlockChain() == lexicalBlock)
        return ARECORD_STOP;
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GENERATOR()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_YIELD()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARRAYPUSH()
{
    uint32_t slot = GET_UINT16(cx->regs->pc);
    JS_ASSERT(cx->fp()->getFixedCount() <= slot);
    JS_ASSERT(cx->fp()->slots() + slot < cx->regs->sp - 1);
    Value &arrayval = cx->fp()->slots()[slot];
    JS_ASSERT(arrayval.isObject());
    JS_ASSERT(arrayval.toObject().isDenseArray());
    LIns *array_ins = get(&arrayval);
    Value &elt = stackval(-1);
    LIns *elt_ins = box_value_for_native_call(elt, get(&elt));

    LIns *args[] = { elt_ins, array_ins, cx_ins };
    LIns *ok_ins = lir->insCall(&js_ArrayCompPush_tn_ci, args);
    guard(false, lir->insEqI_0(ok_ins), OOM_EXIT);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ENUMCONSTELEM()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LEAVEBLOCKEXPR()
{
    LIns* v_ins = stack(-1);
    int n = -1 - GET_UINT16(cx->regs->pc);
    stack(n, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETTHISPROP()
{
    LIns* this_ins;

    CHECK_STATUS_A(getThis(this_ins));

    



    CHECK_STATUS_A(getProp(&cx->fp()->getThisValue().toObject(), this_ins));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETARGPROP()
{
    return getProp(argval(GET_ARGNO(cx->regs->pc)));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DEFUPVAR()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETLOCALPROP()
{
    return getProp(varval(GET_SLOTNO(cx->regs->pc)));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INDEXBASE1()
{
    atoms += 1 << 16;
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INDEXBASE2()
{
    atoms += 2 << 16;
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INDEXBASE3()
{
    atoms += 3 << 16;
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLLOCAL()
{
    uintN slot = GET_SLOTNO(cx->regs->pc);
    stack(0, var(slot));
    stack(1, INS_NULL());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLARG()
{
    uintN slot = GET_ARGNO(cx->regs->pc);
    stack(0, arg(slot));
    stack(1, INS_NULL());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BINDGNAME()
{
    stack(0, INS_CONSTOBJ(globalObj));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INT8()
{
    stack(0, lir->insImmD(GET_INT8(cx->regs->pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INT32()
{
    stack(0, lir->insImmD(GET_INT32(cx->regs->pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LENGTH()
{
    Value& l = stackval(-1);
    if (l.isPrimitive()) {
        if (!l.isString())
            RETURN_STOP_A("non-string primitive JSOP_LENGTH unsupported");
        set(&l, lir->ins1(LIR_i2d, p2i(getStringLength(get(&l)))));
        return ARECORD_CONTINUE;
    }

    JSObject* obj = &l.toObject();
    LIns* obj_ins = get(&l);

    if (obj->isArguments()) {
        unsigned depth;
        JSStackFrame *afp = guardArguments(obj, obj_ins, &depth);
        if (!afp)
            RETURN_STOP_A("can't reach arguments object's frame");

        
        
        if (obj->isArgsLengthOverridden())
            RETURN_STOP_A("can't trace JSOP_ARGCNT if arguments.length has been modified");
        LIns* slot_ins = guardArgsLengthNotAssigned(obj_ins);

        
        
        LIns* v_ins =
            lir->ins1(LIR_i2d, lir->ins2ImmI(LIR_rshi,
                                             slot_ins, JSObject::ARGS_PACKED_BITS_COUNT));
        set(&l, v_ins);
        return ARECORD_CONTINUE;
    }

    LIns* v_ins;
    if (obj->isArray()) {
        if (obj->isDenseArray()) {
            guardDenseArray(obj_ins, BRANCH_EXIT);
        } else {
            JS_ASSERT(obj->isSlowArray());
            guardClass(obj_ins, &js_SlowArrayClass, snapshot(BRANCH_EXIT), LOAD_NORMAL);
        }
        v_ins = lir->ins1(LIR_i2d, stobj_get_fslot_uint32(obj_ins, JSObject::JSSLOT_ARRAY_LENGTH));
    } else if (OkToTraceTypedArrays && js_IsTypedArray(obj)) {
        
        guardClass(obj_ins, obj->getClass(), snapshot(BRANCH_EXIT), LOAD_NORMAL);
        v_ins = lir->ins1(LIR_i2d, lir->insLoad(LIR_ldi,
                                                stobj_get_const_private_ptr(obj_ins),
                                                js::TypedArray::lengthOffset(),
                                                ACCSET_OTHER, LOAD_CONST));
    } else {
        if (!obj->isNative())
            RETURN_STOP_A("can't trace length property access on non-array, non-native object");
        return getProp(obj, obj_ins);
    }
    set(&l, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NEWARRAY()
{
    LIns *proto_ins;
    CHECK_STATUS_A(getClassPrototype(JSProto_Array, proto_ins));

    uint32 len = GET_UINT16(cx->regs->pc);
    cx->assertValidStackDepth(len);

    LIns* args[] = { lir->insImmI(len), proto_ins, cx_ins };
    LIns* v_ins = lir->insCall(&js_NewPreallocatedArray_ci, args);
    guard(false, lir->insEqP_0(v_ins), OOM_EXIT);

    LIns* dslots_ins = NULL;
    uint32 count = 0;
    for (uint32 i = 0; i < len; i++) {
        Value& v = stackval(int(i) - int(len));
        if (!v.isMagic())
            count++;
        stobj_set_dslot(v_ins, i, dslots_ins, v, get(&v));
    }

    stack(-int(len), v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_HOLE()
{
    stack(0, INS_CONST(JS_ARRAY_HOLE));
    return ARECORD_CONTINUE;
}

AbortableRecordingStatus
TraceRecorder::record_JSOP_TRACE()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETMETHOD()
{
    return record_JSOP_SETPROP();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INITMETHOD()
{
    return record_JSOP_INITPROP();
}

JSBool FASTCALL
js_Unbrand(JSContext *cx, JSObject *obj)
{
    return obj->unbrand(cx);
}

JS_DEFINE_CALLINFO_2(extern, BOOL, js_Unbrand, CONTEXT, OBJECT, 0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_UNBRAND()
{
    LIns* args_ins[] = { stack(-1), cx_ins };
    LIns* call_ins = lir->insCall(&js_Unbrand_ci, args_ins);
    guard(true, call_ins, OOM_EXIT);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_UNBRANDTHIS()
{
    LIns* this_ins;
    RecordingStatus status = getThis(this_ins);
    if (status != RECORD_CONTINUE)
        return InjectStatus(status);

    LIns* args_ins[] = { this_ins, cx_ins };
    LIns* call_ins = lir->insCall(&js_Unbrand_ci, args_ins);
    guard(true, call_ins, OOM_EXIT);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SHARPINIT()
{
    return ARECORD_STOP;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETGLOBAL()
{
    uint32 slot = cx->fp()->getScript()->getGlobalSlot(GET_SLOTNO(cx->regs->pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    stack(0, get(&globalObj->getSlotRef(slot)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETGLOBAL()
{
    uint32 slot = cx->fp()->getScript()->getGlobalSlot(GET_SLOTNO(cx->regs->pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    set(&globalObj->getSlotRef(slot), stack(-1));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLGLOBAL()
{
    uint32 slot = cx->fp()->getScript()->getGlobalSlot(GET_SLOTNO(cx->regs->pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    Value &v = globalObj->getSlotRef(slot);
    stack(0, get(&v));
    stack(1, INS_NULL());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GLOBALDEC()
{
    uint32 slot = cx->fp()->getScript()->getGlobalSlot(GET_SLOTNO(cx->regs->pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    return InjectStatus(inc(globalObj->getSlotRef(slot), -1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DECGLOBAL()
{
    uint32 slot = cx->fp()->getScript()->getGlobalSlot(GET_SLOTNO(cx->regs->pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    return InjectStatus(inc(globalObj->getSlotRef(slot), -1, true));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INCGLOBAL()
{
    uint32 slot = cx->fp()->getScript()->getGlobalSlot(GET_SLOTNO(cx->regs->pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    return InjectStatus(inc(globalObj->getSlotRef(slot), 1, true));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GLOBALINC()
{
    uint32 slot = cx->fp()->getScript()->getGlobalSlot(GET_SLOTNO(cx->regs->pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    return InjectStatus(inc(globalObj->getSlotRef(slot), 1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETGNAME()
{
    return record_JSOP_NAME();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETGNAME()
{
    return record_JSOP_SETNAME();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GNAMEDEC()
{
    return record_JSOP_NAMEDEC();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GNAMEINC()
{
    return record_JSOP_NAMEINC();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DECGNAME()
{
    return record_JSOP_DECNAME();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INCGNAME()
{
    return record_JSOP_INCNAME();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLGNAME()
{
    return record_JSOP_CALLNAME();
}

#define DBG_STUB(OP)                                                          \
    JS_REQUIRES_STACK AbortableRecordingStatus                                \
    TraceRecorder::record_##OP()                                              \
    {                                                                         \
        RETURN_STOP_A("can't trace " #OP);                                    \
    }

DBG_STUB(JSOP_GETUPVAR_DBG)
DBG_STUB(JSOP_CALLUPVAR_DBG)
DBG_STUB(JSOP_DEFFUN_DBGFC)
DBG_STUB(JSOP_DEFLOCALFUN_DBGFC)
DBG_STUB(JSOP_LAMBDA_DBGFC)

#ifdef JS_JIT_SPEW




void
DumpPeerStability(TraceMonitor* tm, const void* ip, JSObject* globalObj, uint32 globalShape,
                  uint32 argc)
{
    TreeFragment* f;
    bool looped = false;
    unsigned length = 0;

    for (f = LookupLoop(tm, ip, globalObj, globalShape, argc); f != NULL; f = f->peer) {
        if (!f->code())
            continue;
        debug_only_printf(LC_TMRecorder, "Stability of fragment %p:\nENTRY STACK=", (void*)f);
        if (looped)
            JS_ASSERT(f->nStackTypes == length);
        for (unsigned i = 0; i < f->nStackTypes; i++)
            debug_only_printf(LC_TMRecorder, "%c", TypeToChar(f->stackTypeMap()[i]));
        debug_only_print0(LC_TMRecorder, " GLOBALS=");
        for (unsigned i = 0; i < f->nGlobalTypes(); i++)
            debug_only_printf(LC_TMRecorder, "%c", TypeToChar(f->globalTypeMap()[i]));
        debug_only_print0(LC_TMRecorder, "\n");
        UnstableExit* uexit = f->unstableExits;
        while (uexit != NULL) {
            debug_only_print0(LC_TMRecorder, "EXIT  ");
            JSValueType* m = uexit->exit->fullTypeMap();
            debug_only_print0(LC_TMRecorder, "STACK=");
            for (unsigned i = 0; i < uexit->exit->numStackSlots; i++)
                debug_only_printf(LC_TMRecorder, "%c", TypeToChar(m[i]));
            debug_only_print0(LC_TMRecorder, " GLOBALS=");
            for (unsigned i = 0; i < uexit->exit->numGlobalSlots; i++) {
                debug_only_printf(LC_TMRecorder, "%c",
                                  TypeToChar(m[uexit->exit->numStackSlots + i]));
            }
            debug_only_print0(LC_TMRecorder, "\n");
            uexit = uexit->next;
        }
        length = f->nStackTypes;
        looped = true;
    }
}
#endif

#ifdef MOZ_TRACEVIS

FILE* traceVisLogFile = NULL;
JSHashTable *traceVisScriptTable = NULL;

JS_FRIEND_API(bool)
StartTraceVis(const char* filename = "tracevis.dat")
{
    if (traceVisLogFile) {
        
        StopTraceVis();
    }

    traceVisLogFile = fopen(filename, "wb");
    if (!traceVisLogFile)
        return false;

    return true;
}

JS_FRIEND_API(JSBool)
StartTraceVisNative(JSContext *cx, JSObject *obj, uintN argc, Value *argv, Value *rval)
{
    JSBool ok;

    if (argc > 0 && argv[0].isString()) {
        JSString *str = JSVAL_TO_STRING(argv[0]);
        char *filename = js_DeflateString(cx, str->chars(), str->length());
        if (!filename)
            goto error;
        ok = StartTraceVis(filename);
        cx->free(filename);
    } else {
        ok = StartTraceVis();
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
StopTraceVis()
{
    if (!traceVisLogFile)
        return false;

    fclose(traceVisLogFile); 
    traceVisLogFile = NULL;

    return true;
}

JS_FRIEND_API(JSBool)
StopTraceVisNative(JSContext *cx, JSObject *obj, uintN argc, Value *argv, Value *rval)
{
    JSBool ok = StopTraceVis();

    if (ok)
        fprintf(stderr, "stopped TraceVis recording\n");
    else
        JS_ReportError(cx, "TraceVis isn't running");

    return ok;
}

#endif 

JS_REQUIRES_STACK void
TraceRecorder::captureStackTypes(unsigned callDepth, JSValueType* typeMap)
{
    CaptureTypesVisitor capVisitor(cx, typeMap, !!oracle);
    VisitStackSlots(capVisitor, cx, callDepth);
}

JS_REQUIRES_STACK void
TraceRecorder::determineGlobalTypes(JSValueType* typeMap)
{
    DetermineTypesVisitor detVisitor(*this, typeMap);
    VisitGlobalSlots(detVisitor, cx, *tree->globalSlots);
}

#ifdef JS_METHODJIT

class AutoRetBlacklist
{
    jsbytecode* pc;
    bool& blacklist;

  public:
    AutoRetBlacklist(jsbytecode* pc, bool& blacklist)
      : pc(pc), blacklist(blacklist)
    { }

    ~AutoRetBlacklist()
    {
        blacklist = IsBlacklisted(pc);
    }
};

JS_REQUIRES_STACK TracePointAction
MonitorTracePoint(JSContext* cx, uintN& inlineCallCount, bool& blacklist)
{
    JSStackFrame* fp = cx->fp();
    TraceMonitor* tm = &JS_TRACE_MONITOR(cx);
    jsbytecode* pc = cx->regs->pc;

    JS_ASSERT(!TRACE_RECORDER(cx));

    JSObject* globalObj = cx->fp()->getScopeChain()->getGlobal();
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;

    AutoRetBlacklist autoRetBlacklist(pc, blacklist);

    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots)) {
        Backoff(cx, pc);
        return TPA_Nothing;
    }

    uint32 argc = cx->fp()->argc;
    TreeFragment* tree = LookupOrAddLoop(tm, pc, globalObj, globalShape, argc);

    debug_only_printf(LC_TMTracer,
                      "Looking for compat peer %d@%d, from %p (ip: %p)\n",
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()), (void*)tree, tree->ip);

    if (tree->code() || tree->peer) {
        uintN count;
        TreeFragment* match = FindVMCompatiblePeer(cx, globalObj, tree, count);
        if (match) {
            VMSideExit* lr = NULL;
            VMSideExit* innermostNestedGuard = NULL;

            if (!ApplyBlacklistHeuristics(cx, match))
                return TPA_Nothing;

            
            if (!ExecuteTree(cx, match, inlineCallCount, &innermostNestedGuard, &lr))
                return TPA_Error;

            if (!lr)
                return TPA_Nothing;

            switch (lr->exitType) {
              case UNSTABLE_LOOP_EXIT:
                if (!AttemptToStabilizeTree(cx, globalObj, lr, NULL, 0))
                    return TPA_RanStuff;
                break;

              case OVERFLOW_EXIT:
                tm->oracle->markInstructionUndemotable(cx->regs->pc);
                
              case BRANCH_EXIT:
              case CASE_EXIT:
                if (!AttemptToExtendTree(cx, lr, NULL, NULL))
                    return TPA_RanStuff;
                break;

              case LOOP_EXIT:
                if (!innermostNestedGuard)
                    return TPA_RanStuff;
                if (!AttemptToExtendTree(cx, innermostNestedGuard, lr, NULL))
                    return TPA_RanStuff;
                break;

              default:
                return TPA_RanStuff;
            }

            JS_ASSERT(TRACE_RECORDER(cx));

            goto interpret;
        }

        if (count >= MAXPEERS) {
            debug_only_print0(LC_TMTracer, "Blacklisted: too many peer trees.\n");
            Blacklist((jsbytecode*)tree->root->ip);
            return TPA_Nothing;
        }
    }

    if (++tree->hits() < HOTLOOP)
        return TPA_Nothing;
    if (!ScopeChainCheck(cx, tree))
        return TPA_Nothing;
    if (!RecordTree(cx, tree->first, NULL, 0, globalSlots))
        return TPA_Nothing;

  interpret:
    JS_ASSERT(TRACE_RECORDER(cx));

    
    fp->flags |= JSFRAME_RECORDING;
    if (!Interpret(cx, fp, inlineCallCount))
        return TPA_Error;

    fp->flags &= ~JSFRAME_RECORDING;

    return TPA_RanStuff;
}

#endif

} 
