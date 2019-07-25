








































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
#include "jscompartment.h"
#include "jsdate.h"
#include "jsdbgapi.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jsmath.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jstl.h"
#include "jstracer.h"
#include "jsxml.h"
#include "jstypedarray.h"

#include "jsatominlines.h"
#include "jscntxtinlines.h"
#include "jsfuninlines.h"
#include "jsinterpinlines.h"
#include "jspropertycacheinlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jscntxtinlines.h"
#include "jsopcodeinlines.h"

#include "vm/Stack-inl.h"

#ifdef JS_METHODJIT
#include "methodjit/MethodJIT.h"
#endif

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

#ifdef DEBUG
namespace js {
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
}
#endif 

namespace nanojit {
using namespace js;
using namespace js::gc;
using namespace js::tjit;











#define OUT_OF_MEMORY_ABORT(msg)    JS_Assert(msg, __FILE__, __LINE__);
































static const size_t DataReserveSize  = 12500 * sizeof(uintptr_t);
static const size_t TraceReserveSize =  5000 * sizeof(uintptr_t);
static const size_t TempReserveSize  =  1000 * sizeof(uintptr_t);

void*
nanojit::Allocator::allocChunk(size_t nbytes, bool fallible)
{
    VMAllocator *vma = (VMAllocator*)this;
    





    void *p = vma->mRt->calloc_(nbytes);
    if (p) {
        vma->mSize += nbytes;
    } else {
        vma->mOutOfMemory = true;
        if (!fallible) {
            p = (void *)vma->mReserveCurr;
            vma->mReserveCurr += nbytes;
            if (vma->mReserveCurr > vma->mReserveLimit)
                OUT_OF_MEMORY_ABORT("nanojit::Allocator::allocChunk: out of memory");
            memset(p, 0, nbytes);
            vma->mSize += nbytes;
        }
    }
    return p;
}

void
nanojit::Allocator::freeChunk(void *p) {
    VMAllocator *vma = (VMAllocator*)this;
    if (p < vma->mReserve || uintptr_t(p) >= vma->mReserveLimit)
        UnwantedForeground::free_(p);
}

void
nanojit::Allocator::postReset() {
    VMAllocator *vma = (VMAllocator*)this;
    vma->mOutOfMemory = false;
    vma->mSize = 0;
    vma->mReserveCurr = uintptr_t(vma->mReserve);
}

int
StackFilter::getTop(LIns* guard)
{
    VMSideExit* e = (VMSideExit*)guard->record()->exit;
    return e->sp_adj;
}

#if defined NJ_VERBOSE
static void
formatGuardExit(InsBuf *buf, LIns *ins)
{
    VMSideExit *x = (VMSideExit *)ins->record()->exit;
    RefBuf b1;
    if (LogController.lcbits & LC_FragProfile)
        VMPI_snprintf(b1.buf, b1.len, " (GuardID=%03d)", ins->record()->profGuardID);
    else
        b1.buf[0] = '\0';
    VMPI_snprintf(buf->buf, buf->len,
                  " -> exit=%p pc=%p imacpc=%p sp%+ld rp%+ld %s%s",
                  (void *)x,
                  (void *)x->pc,
                  (void *)x->imacpc,
                  (long int)x->sp_adj,
                  (long int)x->rp_adj,
                  getExitName(x->exitType),
                  b1.buf);
}

void
LInsPrinter::formatGuard(InsBuf *buf, LIns *ins)
{
    RefBuf b1, b2;
    InsBuf b3;
    formatGuardExit(&b3, ins);
    VMPI_snprintf(buf->buf, buf->len,
                  "%s: %s %s%s",
                  formatRef(&b1, ins),
                  lirNames[ins->opcode()],
                  ins->oprnd1() ? formatRef(&b2, ins->oprnd1()) : "",
                  b3.buf);
}

void
LInsPrinter::formatGuardXov(InsBuf *buf, LIns *ins)
{
    RefBuf b1, b2, b3;
    InsBuf b4;
    formatGuardExit(&b4, ins);
    VMPI_snprintf(buf->buf, buf->len,
                  "%s = %s %s, %s%s",
                  formatRef(&b1, ins),
                  lirNames[ins->opcode()],
                  formatRef(&b2, ins->oprnd1()),
                  formatRef(&b3, ins->oprnd2()),
                  b4.buf);
}

const char*
nanojit::LInsPrinter::accNames[] = {
    "state",        
    "sp",           
    "rp",           
    "cx",           
    "tm",           
    "eos",          
    "alloc",        
    "regs",         
    "sf",           
    "rt",           

    "objclasp",     
    "objflags",     
    "objshape",     
    "objproto",     
    "objparent",    
    "objprivate",   
    "objcapacity",  
    "objslots",     

    "slots",        
    "tarray",       
    "tdata",        
    "iter",         
    "iterprops",    
    "str",          
    "strmchars",    
    "typemap",      
    "fcslots",      
    "argsdata",     

    "?!"            
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(nanojit::LInsPrinter::accNames) == TM_NUM_USED_ACCS + 1);
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








#define HOTLOOP 8


#define BL_ATTEMPTS 2


#define BL_BACKOFF 32





#define MIN_LOOP_ITERS 200
#define LOOP_CHECK_ITERS 10

#ifdef DEBUG
#define LOOP_COUNT_MAX 100000000
#else
#define LOOP_COUNT_MAX MIN_LOOP_ITERS
#endif


#define HOTEXIT 1


#define MAXEXIT 3


#define MAXPEERS 9


#define MAX_CALLDEPTH 10


#define MAX_TABLE_SWITCH 256


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
        JSAtom* str = JSID_TO_ATOM(id);
        if (StringEqualsAscii(str, "HOTLOOP")) {
            *vp = INT_TO_JSVAL(HOTLOOP);
            return JS_TRUE;
        }

        if (StringEqualsAscii(str, "adaptive")) {
#ifdef JS_METHODJIT
            *vp = BOOLEAN_TO_JSVAL(cx->profilingEnabled ||
                                   (cx->methodJitEnabled &&
                                    !cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS)));
#else
            *vp = BOOLEAN_TO_JSVAL(false);
#endif
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
    jitstats_getProperty,  JS_StrictPropertyStub,
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

#ifdef JS_JIT_SPEW
static void
DumpPeerStability(TraceMonitor* tm, const void* ip, JSObject* globalObj, uint32 globalShape, uint32 argc);
#endif








static bool did_we_check_processor_features = false;

nanojit::Config NJConfig;







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
            "  profiler     show loop profiles as they are profiled\n"
            "  treevis      spew that tracevis/tree.py can parse\n"
            "  ------ options for Nanojit ------\n"
            "  fragprofile  count entries and exits for each fragment\n"
            "  liveness     show LIR liveness at start of reader pipeline\n"
            "  readlir      show LIR as it enters the reader pipeline\n"
            "  aftersf      show LIR after StackFilter\n"
            "  afterdce     show LIR after dead code elimination\n"
            "  native       show native code (interleaved with 'afterdce')\n"
            "  nativebytes  show native code bytes in 'native' output\n"
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
    if (strstr(tmf, "profiler") || strstr(tmf, "full")) bits |= LC_TMProfiler;
    if (strstr(tmf, "treevis"))                         bits |= LC_TMTreeVis;

    
    if (strstr(tmf, "fragprofile"))                       bits |= LC_FragProfile;
    if (strstr(tmf, "liveness")   || strstr(tmf, "full")) bits |= LC_Liveness;
    if (strstr(tmf, "readlir")    || strstr(tmf, "full")) bits |= LC_ReadLIR;
    if (strstr(tmf, "aftersf")    || strstr(tmf, "full")) bits |= LC_AfterSF;
    if (strstr(tmf, "afterdce")   || strstr(tmf, "full")) bits |= LC_AfterDCE;
    if (strstr(tmf, "native")     || strstr(tmf, "full")) bits |= LC_Native;
    if (strstr(tmf, "nativebytes")|| strstr(tmf, "full")) bits |= LC_Bytes;
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
JSBool FASTCALL
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
                if (u.s->isRope()) {
                    fprintf(out, "<rope>");
                    break;
                }
                if (u.s->isRope()) {
                    fprintf(out, "<rope: length %d>", (int)u.s->asRope().length());
                } else {
                    const jschar *chars = u.s->asLinear().chars();
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
    LIns* argsp_ins = w.nameImmpNonGC(args);
    for (int i = 0; i < count; ++i)
        w.stTprintArg(insa, argsp_ins, i);

    LIns* args_ins[] = { w.nameImmpNonGC(args), w.nameImmi(count), w.nameImmpNonGC(data) };
    LIns* call_ins = w.call(&PrintOnTrace_ci, args_ins);
    guard(false, w.eqi0(call_ins), MISMATCH_EXIT);
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

Tracker::Tracker(JSContext *cx)
    : cx(cx)
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
    struct TrackerPage* p = (struct TrackerPage*) cx->calloc_(sizeof(*p));
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
        cx->free_(p);
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
getFrameObjPtrTraceType(void *p, StackFrame *fp)
{
    if (p == fp->addressOfScopeChain()) {
        JS_ASSERT(*(JSObject **)p != NULL);
        return JSVAL_TYPE_NONFUNOBJ;
    }
    JS_ASSERT(p == fp->addressOfArgs());
    return fp->hasArgsObj() ? JSVAL_TYPE_NONFUNOBJ : JSVAL_TYPE_NULL;
}

static inline bool
isFrameObjPtrTraceType(JSValueType t)
{
    return t == JSVAL_TYPE_NULL || t == JSVAL_TYPE_NONFUNOBJ;
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
    HashAccum(h, uintptr_t(cx->fp()->script()), ORACLE_MASK);
    HashAccum(h, uintptr_t(pc), ORACLE_MASK);
    HashAccum(h, uintptr_t(slot), ORACLE_MASK);
    return int(h);
}

static JS_REQUIRES_STACK inline int
GlobalSlotHash(JSContext* cx, unsigned slot)
{
    uintptr_t h = HASH_SEED;
    StackFrame* fp = cx->fp();

    while (fp->prev())
        fp = fp->prev();

    HashAccum(h, uintptr_t(fp->maybeScript()), ORACLE_MASK);
    HashAccum(h, uintptr_t(fp->scopeChain().getGlobal()->shape()), ORACLE_MASK);
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
    markStackSlotUndemotable(cx, slot, cx->regs().pc);
}


JS_REQUIRES_STACK bool
Oracle::isStackSlotUndemotable(JSContext* cx, unsigned slot, const void* pc) const
{
    return _stackDontDemote.get(StackSlotHash(cx, slot, pc));
}

JS_REQUIRES_STACK bool
Oracle::isStackSlotUndemotable(JSContext* cx, unsigned slot) const
{
    return isStackSlotUndemotable(cx, slot, cx->regs().pc);
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
Oracle::markInstructionSlowZeroTest(jsbytecode* pc)
{
    _pcSlowZeroTest.set(PCHash(pc));
}


bool
Oracle::isInstructionSlowZeroTest(jsbytecode* pc) const
{
    return _pcSlowZeroTest.get(PCHash(pc));
}

void
Oracle::clearDemotability()
{
    _stackDontDemote.reset();
    _globalDontDemote.reset();
    _pcDontDemote.reset();
    _pcSlowZeroTest.reset();
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
        OUT_OF_MEMORY_ABORT("FrameInfoCache::FrameInfoCache(): out of memory");
}

#define PC_HASH_COUNT 1024

static void
Blacklist(jsbytecode* pc)
{
    AUDIT(blacklisted);
    JS_ASSERT(*pc == JSOP_TRACE || *pc == JSOP_NOTRACE);
    *pc = JSOP_NOTRACE;
}

static void
Unblacklist(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(*pc == JSOP_NOTRACE || *pc == JSOP_TRACE);
    if (*pc == JSOP_NOTRACE) {
        *pc = JSOP_TRACE;

#ifdef JS_METHODJIT
        
        js::mjit::ResetTraceHint(script, pc, GET_UINT16(pc), false);
#endif
    }
}

#ifdef JS_METHODJIT
static bool
IsBlacklisted(jsbytecode* pc)
{
    if (*pc == JSOP_NOTRACE)
        return true;
    if (*pc == JSOP_CALL)
        return *(pc + JSOP_CALL_LENGTH) == JSOP_NOTRACE;
    return false;
}
#endif

static void
Backoff(TraceMonitor *tm, jsbytecode* pc, Fragment* tree = NULL)
{
    
    RecordAttemptMap &table = *tm->recordAttempts;
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
ResetRecordingAttempts(TraceMonitor *tm, jsbytecode* pc)
{
    RecordAttemptMap &table = *tm->recordAttempts;
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
    TreeFragment* f = new (*tm->dataAlloc) TreeFragment(ip, tm->dataAlloc, tm->oracle,
                                                        globalObj, globalShape,
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
    TreeFragment* f = new (*tm->dataAlloc) TreeFragment(peer->ip, tm->dataAlloc, tm->oracle,
                                                        peer->globalObj, peer->globalShape,
                                                        peer->argc verbose_only(, profFragID));
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
    this->spOffsetAtEntry = cx->regs().sp - cx->fp()->base();

#ifdef DEBUG
    this->treeFileName = cx->fp()->script()->filename;
    this->treeLineNumber = js_FramePCToLineNumber(cx, cx->fp());
    this->treePCOffset = FramePCOffset(cx, cx->fp());
#endif
    this->script = cx->fp()->script();
    this->gcthings.clear();
    this->shapes.clear();
    this->unstableExits = NULL;
    this->sideExits.clear();

    
    this->nativeStackBase = (nStackTypes - (cx->regs().sp - cx->fp()->base())) *
                             sizeof(double);
    this->maxNativeStackSlots = nStackTypes;
    this->maxCallDepth = 0;
    this->execs = 0;
    this->iters = 0;
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
AttemptCompilation(TraceMonitor *tm, JSObject* globalObj,
                   JSScript* script, jsbytecode* pc, uint32 argc)
{
    
    Unblacklist(script, pc);
    ResetRecordingAttempts(tm, pc);

    
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

static const CallInfo *
fcallinfo(LIns *ins)
{
    return ins->isop(LIR_calld) ? ins->callInfo() : NULL;
}









static inline uintN
entryFrameArgc(JSContext *cx)
{
    StackFrame *fp = cx->fp();
    return fp->hasArgs() ? fp->numActualArgs() : 0;
}

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
VisitStackAndArgs(Visitor &visitor, StackFrame *fp, StackFrame *next, Value *stack)
{
    if (JS_LIKELY(!next->hasOverflowArgs()))
        return visitor.visitStackSlots(stack, next->formalArgsEnd() - stack, fp);

    





    uintN nactual = next->numActualArgs();
    Value *actuals = next->actualArgs();
    size_t nstack = (actuals - 2 ) - stack;
    if (!visitor.visitStackSlots(stack, nstack, fp))
        return false;
    uintN nformal = next->numFormalArgs();
    Value *formals = next->formalArgs();
    if (!visitor.visitStackSlots(formals - 2, 2 + nformal, fp))
        return false;
    return visitor.visitStackSlots(actuals + nformal, nactual - nformal, fp);
}









template <typename Visitor>
static JS_REQUIRES_STACK bool
VisitFrameSlots(Visitor &visitor, JSContext *cx, unsigned depth, StackFrame *fp,
                StackFrame *next)
{
    JS_ASSERT_IF(!next, cx->fp() == fp);

    if (depth > 0 && !VisitFrameSlots(visitor, cx, depth-1, fp->prev(), fp))
        return false;

    if (depth == 0) {
        if (fp->isGlobalFrame()) {
            visitor.setStackSlotKind("global");
            Value *base = fp->slots() + fp->globalScript()->nfixed;
            if (next)
                return VisitStackAndArgs(visitor, fp, next, base);
            return visitor.visitStackSlots(base, cx->regs().sp - base, fp);
        }

        if (JS_UNLIKELY(fp->isEvalFrame())) {
            visitor.setStackSlotKind("eval");
            if (!visitor.visitStackSlots(&fp->calleev(), 2, fp))
                return false;
        } else {
            



            visitor.setStackSlotKind("args");
            uintN nformal = fp->numFormalArgs();
            if (!visitor.visitStackSlots(fp->formalArgs() - 2, 2 + nformal, fp))
                return false;
            if (JS_UNLIKELY(fp->hasOverflowArgs())) {
                if (!visitor.visitStackSlots(fp->actualArgs() + nformal,
                                             fp->numActualArgs() - nformal, fp))
                    return false;
            }
        }
    }

    JS_ASSERT(fp->isFunctionFrame());

    








    visitor.setStackSlotKind("arguments");
    if (!visitor.visitFrameObjPtr(fp->addressOfArgs(), fp))
        return false;
    visitor.setStackSlotKind("scopeChain");
    if (!visitor.visitFrameObjPtr(fp->addressOfScopeChain(), fp))
        return false;

    visitor.setStackSlotKind("slots");
    if (next)
        return VisitStackAndArgs(visitor, fp, next, fp->slots());
    return visitor.visitStackSlots(fp->slots(), cx->regs().sp - fp->slots(), fp);
}



const int SPECIAL_FRAME_SLOTS = 2;

template <typename Visitor>
static JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
VisitStackSlots(Visitor &visitor, JSContext *cx, unsigned callDepth)
{
    return VisitFrameSlots(visitor, cx, callDepth, cx->fp(), NULL);
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
    VisitGlobalSlots(visitor, cx, cx->fp()->scopeChain().getGlobal(),
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
    VisitSlots(visitor, cx, cx->fp()->scopeChain().getGlobal(),
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
    VisitSlots(visitor, cx, cx->fp()->scopeChain().getGlobal(),
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
    visitStackSlots(Value *vp, size_t count, StackFrame* fp) {
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
    visitFrameObjPtr(void* p, StackFrame* fp) {
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

static JS_REQUIRES_STACK JS_ALWAYS_INLINE unsigned
CountStackAndArgs(StackFrame *next, Value *stack)
{
    if (JS_LIKELY(!next->hasOverflowArgs()))
        return (Value *)next - stack;
    size_t nvals = (next->formalArgs() - 2 ) - stack;
    JS_ASSERT(nvals == unsigned((next->actualArgs() - 2) - stack) + (2 + next->numActualArgs()));
    return nvals;
}

static JS_ALWAYS_INLINE uintN
NumSlotsBeforeFixed(StackFrame *fp)
{
    uintN numArgs = fp->isEvalFrame() ? 0 : Max(fp->numActualArgs(), fp->numFormalArgs());
    return 2 + numArgs + SPECIAL_FRAME_SLOTS;
}







JS_REQUIRES_STACK unsigned
NativeStackSlots(JSContext *cx, unsigned callDepth)
{
    StackFrame *fp = cx->fp();
    StackFrame *next = NULL;
    unsigned slots = 0;
    unsigned depth = callDepth;

    for (; depth > 0; --depth, next = fp, fp = fp->prev()) {
        JS_ASSERT(fp->isNonEvalFunctionFrame());
        slots += SPECIAL_FRAME_SLOTS;
        if (next)
            slots += CountStackAndArgs(next, fp->slots());
        else
            slots += cx->regs().sp - fp->slots();
    }

    Value *start;
    if (fp->isGlobalFrame()) {
        start = fp->slots() + fp->globalScript()->nfixed;
    } else {
        start = fp->slots();
        slots += NumSlotsBeforeFixed(fp);
    }
    if (next)
        slots += CountStackAndArgs(next, start);
    else
        slots += cx->regs().sp - start;

#ifdef DEBUG
    CountSlotsVisitor visitor;
    VisitStackSlots(visitor, cx, callDepth);
    JS_ASSERT(visitor.count() == slots && !visitor.stopped());
#endif
    return slots;
}

class CaptureTypesVisitor : public SlotVisitorBase
{
    JSContext* mCx;
    JSValueType* mTypeMap;
    JSValueType* mPtr;
    Oracle   * mOracle;

public:
    JS_ALWAYS_INLINE CaptureTypesVisitor(JSContext* cx, Oracle *oracle,
                                         JSValueType* typeMap, bool speculate)
      : mCx(cx),
        mTypeMap(typeMap),
        mPtr(typeMap),
        mOracle(speculate ? oracle : NULL)
    {}

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
    visitStackSlots(Value *vp, int count, StackFrame* fp) {
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
    visitFrameObjPtr(void* p, StackFrame* fp) {
        JSValueType type = getFrameObjPtrTraceType(p, fp);
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
    CaptureTypesVisitor visitor(cx, oracle, data(), speculate);
    VisitSlots(visitor, cx, globalObj, callDepth, slots);
    JS_ASSERT(visitor.length() == length());
}

JS_REQUIRES_STACK void
TypeMap::captureMissingGlobalTypes(JSContext* cx,
                                   JSObject* globalObj, SlotList& slots, unsigned stackSlots,
                                   bool speculate)
{
    unsigned oldSlots = length() - stackSlots;
    int diff = slots.length() - oldSlots;
    JS_ASSERT(diff >= 0);
    setLength(length() + diff);
    CaptureTypesVisitor visitor(cx, oracle, data() + stackSlots + oldSlots, speculate);
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
ResetJITImpl(JSContext* cx, TraceMonitor *tm);

#ifdef MOZ_TRACEVIS
static JS_INLINE void
ResetJIT(JSContext* cx, TraceMonitor *tm, TraceVisFlushReason r)
{
    LogTraceVisEvent(cx, S_RESET, r);
    ResetJITImpl(cx, tm);
}
#else
# define ResetJIT(cx, tm, reason) ResetJITImpl(cx, tm)
#endif

void
FlushJITCache(JSContext *cx, TraceMonitor *tm)
{
    ResetJIT(cx, tm, FR_OOM);
}

static void
TrashTree(TreeFragment* f);

JS_REQUIRES_STACK
TraceRecorder::TraceRecorder(JSContext* cx, TraceMonitor *tm,
                             VMSideExit* anchor, VMFragment* fragment,
                             unsigned stackSlots, unsigned ngslots, JSValueType* typeMap,
                             VMSideExit* innermost, JSScript* outerScript, jsbytecode* outerPC,
                             uint32 outerArgc, bool speculate)
  : cx(cx),
    traceMonitor(tm),
    oracle(speculate ? tm->oracle : NULL),
    fragment(fragment),
    tree(fragment->root),
    globalObj(tree->globalObj),
    outerScript(outerScript),
    outerPC(outerPC),
    outerArgc(outerArgc),
    anchor(anchor),
    cx_ins(NULL),
    eos_ins(NULL),
    eor_ins(NULL),
    loopLabel(NULL),
    importTypeMap(&tempAlloc(), tm->oracle),
    lirbuf(new (tempAlloc()) LirBuffer(tempAlloc())),
    mark(*traceMonitor->traceAlloc),
    numSideExitsBefore(tree->sideExits.length()),
    tracker(cx),
    nativeFrameTracker(cx),
    global_slots(NULL),
    callDepth(anchor ? anchor->calldepth : 0),
    atoms(FrameAtomBase(cx, cx->fp())),
    consts(JSScript::isValidOffset(cx->fp()->script()->constOffset)
           ? cx->fp()->script()->consts()->vector
           : NULL),
    strictModeCode_ins(NULL),
    cfgMerges(&tempAlloc()),
    trashSelf(false),
    whichTreesToTrash(&tempAlloc()),
    guardedShapeTable(cx),
    initDepth(0),
    hadNewInit(false),
#ifdef DEBUG
    addPropShapeBefore(NULL),
#endif
    rval_ins(NULL),
    native_rval_ins(NULL),
    newobj_ins(NULL),
    pendingSpecializedNative(NULL),
    pendingUnboxSlot(NULL),
    pendingGuardCondition(NULL),
    pendingGlobalSlotsToSet(cx),
    pendingLoop(true),
    generatedSpecializedNative(),
    tempTypeMap(cx),
    w(&tempAlloc(), lirbuf)
{
    JS_ASSERT(globalObj == cx->fp()->scopeChain().getGlobal());
    JS_ASSERT(globalObj->hasOwnShape());
    JS_ASSERT(cx->regs().pc == (jsbytecode*)fragment->ip);

#ifdef JS_METHODJIT
    if (TRACE_PROFILER(cx))
        AbortProfiling(cx);
#endif

    JS_ASSERT(JS_THREAD_DATA(cx)->onTraceCompartment == NULL);
    JS_ASSERT(JS_THREAD_DATA(cx)->profilingCompartment == NULL);
    JS_ASSERT(JS_THREAD_DATA(cx)->recordingCompartment == NULL);
    JS_THREAD_DATA(cx)->recordingCompartment = cx->compartment;

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
        OUT_OF_MEMORY_ABORT("TraceRecorder::TraceRecorder: out of memory");

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

    
    w.init(&LogController, &NJConfig);

    w.start();

    for (int i = 0; i < NumSavedRegs; ++i)
        w.paramp(i, 1);
#ifdef DEBUG
    for (int i = 0; i < NumSavedRegs; ++i)
        w.name(lirbuf->savedRegs[i], regNames[REGNUM(Assembler::savedRegs[i])]);
#endif

    lirbuf->state = w.name(w.paramp(0, 0), "state");

    if (fragment == fragment->root) {
        w.comment("begin-loop");
        InitConst(loopLabel) = w.label();
    }
    w.comment("begin-setup");

    
    
    
    
    verbose_only( if (LogController.lcbits & LC_FragProfile) {
        LIns* entryLabel = NULL;
        if (fragment == fragment->root) {
            entryLabel = loopLabel;
        } else {
            entryLabel = w.label();
        }
        NanoAssert(entryLabel);
        NanoAssert(!fragment->loopLabel);
        fragment->loopLabel = entryLabel;
    })

    lirbuf->sp = w.name(w.ldpStateField(sp), "sp");
    lirbuf->rp = w.name(w.ldpStateField(rp), "rp");
    InitConst(cx_ins) = w.name(w.ldpStateField(cx), "cx");
    InitConst(eos_ins) = w.name(w.ldpStateField(eos), "eos");
    InitConst(eor_ins) = w.name(w.ldpStateField(eor), "eor");

    strictModeCode_ins = w.name(w.immi(cx->fp()->script()->strictModeCode), "strict");

    
    if (tree->globalSlots->length() > tree->nGlobalTypes())
        SpecializeTreesToMissingGlobals(cx, globalObj, tree);

    
    import(tree, lirbuf->sp, stackSlots, ngslots, callDepth, typeMap);

    if (fragment == fragment->root) {
        





        w.comment("begin-interruptFlags-check");
        
#ifdef JS_THREADSAFE
        void *interrupt = (void*) &cx->runtime->interruptCounter;
#else
        void *interrupt = (void*) &JS_THREAD_DATA(cx)->interruptFlags;
#endif
        LIns* flagptr = w.nameImmpNonGC(interrupt);
        LIns* x = w.ldiVolatile(flagptr);
        guard(true, w.eqi0(x), TIMEOUT_EXIT);
        w.comment("end-interruptFlags-check");

        




#ifdef JS_METHODJIT
        if (cx->methodJitEnabled) {
            w.comment("begin-count-loop-iterations");
            LIns* counterPtr = w.nameImmpNonGC((void *) &traceMonitor->iterationCounter);
            LIns* counterValue = w.ldiVolatile(counterPtr);
            LIns* test = w.ltiN(counterValue, LOOP_COUNT_MAX);
            LIns *branch = w.jfUnoptimizable(test);
            



            w.stiVolatile(w.addi(counterValue, w.immi(1)), counterPtr);
            w.label(branch);
            w.comment("end-count-loop-iterations");
        }
#endif
    }

    



    if (anchor && anchor->exitType == NESTED_EXIT) {
        LIns* nested_ins = w.ldpStateField(outermostTreeExitGuard);
        guard(true, w.eqp(nested_ins, w.nameImmpNonGC(innermost)), NESTED_EXIT);
    }

    w.comment("end-setup");
}

TraceRecorder::~TraceRecorder()
{
    
    JS_ASSERT(traceMonitor->recorder != this);

    JS_ASSERT(JS_THREAD_DATA(cx)->profilingCompartment == NULL);
    JS_ASSERT(&JS_THREAD_DATA(cx)->recordingCompartment->traceMonitor == traceMonitor);
    JS_THREAD_DATA(cx)->recordingCompartment = NULL;

    if (trashSelf)
        TrashTree(fragment->root);

    for (unsigned int i = 0; i < whichTreesToTrash.length(); i++)
        TrashTree(whichTreesToTrash[i]);

    
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
    JS_ASSERT(!traceMonitor->profile);
    JS_ASSERT(traceMonitor->recorder == this);
    JS_ASSERT(fragment->lastIns && fragment->code());

    AUDIT(traceCompleted);
    mark.commit();

    
    JSContext* localcx = cx;
    TraceMonitor* localtm = traceMonitor;

    localtm->recorder = NULL;
    cx->delete_(this);

    
    if (localtm->outOfMemory() || OverfullJITCache(localcx, localtm)) {
        ResetJIT(localcx, localtm, FR_OOM);
        return ARECORD_ABORTED;
    }
    return ARECORD_COMPLETED;
}


JS_REQUIRES_STACK TraceRecorder::AbortResult
TraceRecorder::finishAbort(const char* reason)
{
    JS_ASSERT(!traceMonitor->profile);
    JS_ASSERT(traceMonitor->recorder == this);

    AUDIT(recorderAborted);
#ifdef DEBUG
    debug_only_printf(LC_TMMinimal | LC_TMAbort,
                      "Abort recording of tree %s:%d@%d at %s:%d@%d: %s.\n",
                      tree->treeFileName,
                      tree->treeLineNumber,
                      tree->treePCOffset,
                      cx->fp()->script()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()),
                      reason);
#endif
    Backoff(traceMonitor, (jsbytecode*) fragment->root->ip, fragment->root);

    








    if (fragment->root == fragment) {
        TrashTree(fragment->toTreeFragment());
    } else {
        JS_ASSERT(numSideExitsBefore <= fragment->root->sideExits.length());
        fragment->root->sideExits.setLength(numSideExitsBefore);
    }

    
    JSContext* localcx = cx;
    TraceMonitor* localtm = traceMonitor;

    localtm->recorder = NULL;
    cx->delete_(this);

    
    if (localtm->outOfMemory() || OverfullJITCache(localcx, localtm)) {
        ResetJIT(localcx, localtm, FR_OOM);
        return JIT_RESET;
    }
    return NORMAL_ABORT;
}

inline LIns*
TraceRecorder::w_immpObjGC(JSObject* obj)
{
    JS_ASSERT(obj);
    tree->gcthings.addUnique(ObjectValue(*obj));
    return w.immpNonGC((void*)obj);
}

inline LIns*
TraceRecorder::w_immpFunGC(JSFunction* fun)
{
    JS_ASSERT(fun);
    tree->gcthings.addUnique(ObjectValue(*fun));
    return w.immpNonGC((void*)fun);
}

inline LIns*
TraceRecorder::w_immpStrGC(JSString* str)
{
    JS_ASSERT(str);
    tree->gcthings.addUnique(StringValue(str));
    return w.immpNonGC((void*)str);
}

inline LIns*
TraceRecorder::w_immpShapeGC(const Shape* shape)
{
    JS_ASSERT(shape);
    tree->shapes.addUnique(shape);
    return w.immpNonGC((void*)shape);
}

inline LIns*
TraceRecorder::w_immpIdGC(jsid id)
{
    if (JSID_IS_GCTHING(id))
        tree->gcthings.addUnique(IdToValue(id));
    return w.immpNonGC((void*)JSID_BITS(id));
}

ptrdiff_t
TraceRecorder::nativeGlobalSlot(const Value* p) const
{
    JS_ASSERT(isGlobal(p));
    return ptrdiff_t(p - globalObj->slots);
}


ptrdiff_t
TraceRecorder::nativeGlobalOffset(const Value* p) const
{
    return nativeGlobalSlot(p) * sizeof(double);
}


bool
TraceRecorder::isGlobal(const Value* p) const
{
    return (size_t(p - globalObj->slots) < globalObj->numSlots());
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
        JS_ASSERT(size_t(vp - cx->fp()->slots()) < cx->fp()->numSlots());
        offset += size_t(vp - cx->regs().sp) * sizeof(double);
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
#if defined JS_JIT_SPEW
        if (LogController.lcbits & LC_TMTracer) {
            char funName[40];
            if (fun->atom)
                JS_PutEscapedFlatString(funName, sizeof funName, fun->atom, 0);
            else
                strcpy(funName, "unnamed");
            LogController.printf("function<%p:%s> ", (void*)*(JSObject **)slot, funName);
        }
#endif
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
    JS_ASSERT(!profile);
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

    flushEpoch++;

#ifdef JS_METHODJIT
    if (loopProfiles) {
        for (LoopProfileMap::Enum e(*loopProfiles); !e.empty(); e.popFront()) {
            jsbytecode *pc = e.front().key;
            LoopProfile *prof = e.front().value;
            
            js::mjit::ResetTraceHint(prof->entryScript, pc, GET_UINT16(pc), true);
        }
    }
#endif

    frameCache->reset();
    dataAlloc->reset();
    traceAlloc->reset();
    codeAlloc->reset();
    tempAlloc->reset();
    oracle->clear();
    loopProfiles->clear();

    for (size_t i = 0; i < MONITOR_N_GLOBAL_STATES; ++i) {
        globalStates[i].globalShape = -1;
        globalStates[i].globalSlots = new (*dataAlloc) SlotList(dataAlloc);
    }

    assembler = new (*dataAlloc) Assembler(*codeAlloc, *dataAlloc, *dataAlloc,
                                           &LogController, NJConfig);
    verbose_only( branches = NULL; )

    PodArrayZero(vmfragments);
    tracedScripts.clear();

    needFlush = JS_FALSE;
}

inline bool
HasUnreachableGCThings(JSContext *cx, TreeFragment *f)
{
    




    if (IsAboutToBeFinalized(cx, f->globalObj))
        return true;
    Value* vp = f->gcthings.data();
    for (unsigned len = f->gcthings.length(); len; --len) {
        Value &v = *vp++;
        JS_ASSERT(v.isMarkable());
        if (IsAboutToBeFinalized(cx, v.toGCThing()))
            return true;
    }
    const Shape** shapep = f->shapes.data();
    for (unsigned len = f->shapes.length(); len; --len) {
        const Shape* shape = *shapep++;
        if (IsAboutToBeFinalized(cx, shape))
            return true;
    }
    return false;
}

void
TraceMonitor::sweep(JSContext *cx)
{
    JS_ASSERT(!ontrace());
    debug_only_print0(LC_TMTracer, "Purging fragments with dead things");

    bool shouldAbortRecording = false;
    TreeFragment *recorderTree = NULL;
    if (recorder) {
        recorderTree = recorder->getTree();
        shouldAbortRecording = HasUnreachableGCThings(cx, recorderTree);
    }
        
    for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; ++i) {
        TreeFragment** fragp = &vmfragments[i];
        while (TreeFragment* frag = *fragp) {
            TreeFragment* peer = frag;
            do {
                if (HasUnreachableGCThings(cx, peer))
                    break;
                peer = peer->peer;
            } while (peer);
            if (peer) {
                debug_only_printf(LC_TMTracer,
                                  "TreeFragment peer %p has dead gc thing."
                                  "Disconnecting tree %p with ip %p\n",
                                  (void *) peer, (void *) frag, frag->ip);
                JS_ASSERT(frag->root == frag);
                *fragp = frag->next;
                do {
                    verbose_only( FragProfiling_FragFinalizer(frag, this); );
                    if (recorderTree == frag)
                        shouldAbortRecording = true;
                    TrashTree(frag);
                    frag = frag->peer;
                } while (frag);
            } else {
                fragp = &frag->next;
            }
        }
    }

    if (shouldAbortRecording)
        recorder->finishAbort("dead GC things");
}

void
TraceMonitor::mark(JSTracer *trc)
{
    TracerState* state = tracerState;
    while (state) {
        if (state->nativeVp)
            MarkValueRange(trc, state->nativeVpLen, state->nativeVp, "nativeVp");
        state = state->prev;
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
      case JSVAL_TYPE_FUNOBJ:
        JS_ASSERT(IsFunctionObject(v));
#if defined JS_JIT_SPEW
        if (LogController.lcbits & LC_TMTracer) {
            JSFunction* fun = GET_FUNCTION_PRIVATE(cx, &v.toObject());
            char funName[40];
            if (fun->atom)
                JS_PutEscapedFlatString(funName, sizeof funName, fun->atom, 0);
            else
                strcpy(funName, "unnamed");
            LogController.printf("function<%p:%s> ", (void*) &v.toObject(), funName);
        }
#endif
        break;
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
    visitStackSlots(Value *vp, int count, StackFrame* fp) {
        for (int i = 0; i < count; ++i) {
            debug_only_printf(LC_TMTracer, "%s%d: ", stackSlotKind(), i);
            ValueToNative(*vp++, *mTypeMap++, mStack++);
        }
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(void* p, StackFrame* fp) {
        debug_only_printf(LC_TMTracer, "%s%d: ", stackSlotKind(), 0);
        if (p == fp->addressOfScopeChain())
            *(JSObject **)mStack = &fp->scopeChain();
        else
            *(JSObject **)mStack = fp->hasArgsObj() ? &fp->argsObj() : NULL;
#ifdef DEBUG
        if (*mTypeMap == JSVAL_TYPE_NULL) {
            JS_ASSERT(*(JSObject **)mStack == NULL);
            debug_only_print0(LC_TMTracer, "null ");
        } else {
            JS_ASSERT(*mTypeMap == JSVAL_TYPE_NONFUNOBJ);
            JS_ASSERT(!(*(JSObject **)p)->isFunction());
            debug_only_printf(LC_TMTracer,
                              "object<%p:%s> ", *(void **)p,
                              (*(JSObject **)p)->getClass()->name);
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
public:
    FlushNativeStackFrameVisitor(JSContext *cx,
                                 const JSValueType *typeMap,
                                 double *stack) :
        mCx(cx),
        mInitTypeMap(typeMap),
        mTypeMap(typeMap),
        mStack(stack)
    {}

    const JSValueType* getTypeMap()
    {
        return mTypeMap;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, StackFrame* fp) {
        JS_ASSERT(JS_THREAD_DATA(mCx)->waiveGCQuota);
        for (size_t i = 0; i < count; ++i) {
            debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), unsigned(i));
            NativeToValue(mCx, *vp, *mTypeMap, mStack);
            vp++;
            mTypeMap++;
            mStack++;
        }
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(void* p, StackFrame* fp) {
        JS_ASSERT(JS_THREAD_DATA(mCx)->waiveGCQuota);
        debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), 0);
        JSObject *frameobj = *(JSObject **)mStack;
        JS_ASSERT((frameobj == NULL) == (*mTypeMap == JSVAL_TYPE_NULL));
        if (p == fp->addressOfArgs()) {
            if (frameobj) {
                JS_ASSERT_IF(fp->hasArgsObj(), frameobj == &fp->argsObj());
                fp->setArgsObj(*frameobj);
                JS_ASSERT(frameobj->isArguments());
                if (frameobj->isNormalArguments())
                    frameobj->setPrivate(fp);
                else
                    JS_ASSERT(!frameobj->getPrivate());
                debug_only_printf(LC_TMTracer,
                                  "argsobj<%p> ",
                                  (void *)frameobj);
            } else {
                JS_ASSERT(!fp->hasArgsObj());
                debug_only_print0(LC_TMTracer,
                                  "argsobj<null> ");
            }
            
        } else {
            JS_ASSERT(p == fp->addressOfScopeChain());
            if (frameobj->isCall() &&
                !frameobj->getPrivate() &&
                fp->maybeCallee() == frameobj->getCallObjCallee())
            {
                JS_ASSERT(&fp->scopeChain() == StackFrame::sInvalidScopeChain);
                frameobj->setPrivate(fp);
                fp->setScopeChainWithOwnCallObj(*frameobj);
            } else {
                fp->setScopeChainNoCallObj(*frameobj);
            }
            debug_only_printf(LC_TMTracer,
                              "scopechain<%p> ",
                              (void *)frameobj);
        }
#ifdef DEBUG
        JSValueType type = *mTypeMap;
        if (type == JSVAL_TYPE_NULL) {
            debug_only_print0(LC_TMTracer, "null ");
        } else {
            JS_ASSERT(type == JSVAL_TYPE_NONFUNOBJ);
            JS_ASSERT(!frameobj->isFunction());
            debug_only_printf(LC_TMTracer,
                              "object<%p:%s> ",
                              *(void **)p,
                              frameobj->getClass()->name);
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
    TracerState* state = JS_TRACE_MONITOR_ON_TRACE(cx)->tracerState;
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
    StackFrame* fp = cx->stack.findFrameAtLevel(upvarLevel);
    Value v = T::interp_get(fp, slot);
    JSValueType type = getCoercedType(v);
    ValueToNative(v, type, result);
    return type;
}


struct UpvarArgTraits {
    static Value interp_get(StackFrame* fp, int32 slot) {
        return fp->formalArg(slot);
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
    static Value interp_get(StackFrame* fp, int32 slot) {
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
    static Value interp_get(StackFrame* fp, int32 slot) {
        return fp->slots()[slot + fp->numFixed()];
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

#ifdef DEBUG
    TracerState* state = JS_TRACE_MONITOR_ON_TRACE(cx)->tracerState;
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

    
    VOUCH_DOES_NOT_REQUIRE_STACK();
    StackFrame* fp = (StackFrame*) call->getPrivate();
    JS_ASSERT(fp != cx->fp());

    Value v;
    if (fp) {
        v = T::get_slot(fp, cv->slot);
    } else {
        






        JS_ASSERT(cv->slot < T::slot_count(call));
        v = T::get_slot(call, cv->slot);
    }
    JSValueType type = getCoercedType(v);
    ValueToNative(v, type, result);
    return type;
}

struct ArgClosureTraits
{
    
    
    static inline Value get_slot(StackFrame* fp, unsigned slot) {
        JS_ASSERT(slot < fp->numFormalArgs());
        return fp->formalArg(slot);
    }

    
    static inline Value get_slot(JSObject* obj, unsigned slot) {
        return obj->getSlot(slot_offset(obj) + slot);
    }

    
    static inline uint32 slot_offset(JSObject* obj) {
        return JSObject::CALL_RESERVED_SLOTS;
    }

    
    static inline uint16 slot_count(JSObject* obj) {
        return obj->getCallObjCalleeFunction()->nargs;
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
    
    static inline Value get_slot(StackFrame* fp, unsigned slot) {
        JS_ASSERT(slot < fp->fun()->script()->bindings.countVars());
        return fp->slots()[slot];
    }

    static inline Value get_slot(JSObject* obj, unsigned slot) {
        return obj->getSlot(slot_offset(obj) + slot);
    }

    static inline uint32 slot_offset(JSObject* obj) {
        return JSObject::CALL_RESERVED_SLOTS +
               obj->getCallObjCalleeFunction()->nargs;
    }

    static inline uint16 slot_count(JSObject* obj) {
        return obj->getCallObjCalleeFunction()->script()->bindings.countVars();
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
FlushNativeStackFrame(JSContext* cx, unsigned callDepth, const JSValueType* mp, double* np)
{
    
    FlushNativeStackFrameVisitor visitor(cx, mp, np);
    VisitStackSlots(visitor, cx, callDepth);

    debug_only_print0(LC_TMTracer, "\n");
    return visitor.getTypeMap() - mp;
}


JS_REQUIRES_STACK void
TraceRecorder::importImpl(Address addr, const void* p, JSValueType t,
                          const char *prefix, uintN index, StackFrame *fp)
{
    LIns* ins;
    if (t == JSVAL_TYPE_INT32) { 
        JS_ASSERT(hasInt32Repr(*(const Value *)p));

        





        ins = w.ldi(addr);
        ins = w.i2d(ins);
    } else {
        JS_ASSERT_IF(t != JSVAL_TYPE_BOXED && !isFrameObjPtrTraceType(t),
                     ((const Value *)p)->isNumber() == (t == JSVAL_TYPE_DOUBLE));
        if (t == JSVAL_TYPE_DOUBLE) {
            ins = w.ldd(addr);
        } else if (t == JSVAL_TYPE_BOOLEAN) {
            ins = w.ldi(addr);
        } else if (t == JSVAL_TYPE_UNDEFINED) {
            ins = w.immiUndefined();
        } else if (t == JSVAL_TYPE_MAGIC) {
            ins = w.ldi(addr);
        } else {
            ins = w.ldp(addr);
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
    JSAutoByteString funNameBytes;
    if (*prefix == 'a' || *prefix == 'v') {
        mark = JS_ARENA_MARK(&cx->tempPool);
        JSFunction *fun = fp->fun();
        Bindings &bindings = fun->script()->bindings;
        if (bindings.hasLocalNames())
            localNames = bindings.getLocalNameArray(cx, &cx->tempPool);
        funName = fun->atom
                  ? js_AtomToPrintableString(cx, fun->atom, &funNameBytes)
                  : "<anonymous>";
    }
    if (!strcmp(prefix, "argv")) {
        if (index < fp->numFormalArgs()) {
            JSAtom *atom = JS_LOCAL_NAME_TO_ATOM(localNames[index]);
            JSAutoByteString atomBytes;
            JS_snprintf(name, sizeof name, "$%s.%s", funName,
                        js_AtomToPrintableString(cx, atom, &atomBytes));
        } else {
            JS_snprintf(name, sizeof name, "$%s.<arg%d>", funName, index);
        }
    } else if (!strcmp(prefix, "vars")) {
        JSAtom *atom = JS_LOCAL_NAME_TO_ATOM(localNames[fp->numFormalArgs() + index]);
        JSAutoByteString atomBytes;
        JS_snprintf(name, sizeof name, "$%s.%s", funName,
                    js_AtomToPrintableString(cx, atom, &atomBytes));
    } else {
        JS_snprintf(name, sizeof name, "$%s%d", prefix, index);
    }

    if (mark)
        JS_ARENA_RELEASE(&cx->tempPool, mark);
    w.name(ins, name);

    debug_only_printf(LC_TMTracer, "import vp=%p name=%s type=%c\n",
                      p, name, TypeToChar(t));
#endif
}

JS_REQUIRES_STACK void
TraceRecorder::import(Address addr, const Value* p, JSValueType t,
                      const char *prefix, uintN index, StackFrame *fp)
{
    return importImpl(addr, p, t, prefix, index, fp);
}

class ImportBoxedStackSlotVisitor : public SlotVisitorBase
{
    TraceRecorder &mRecorder;
    LIns *mBase;
    ptrdiff_t mStackOffset;
    JSValueType *mTypemap;
    StackFrame *mFp;
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
    visitStackSlots(Value *vp, size_t count, StackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            if (*mTypemap == JSVAL_TYPE_BOXED) {
                mRecorder.import(StackAddress(mBase, mStackOffset), vp, JSVAL_TYPE_BOXED,
                                 "jsval", i, fp);
                LIns *vp_ins = mRecorder.unbox_value(*vp,
                                                     StackAddress(mBase, mStackOffset),
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
    visitFrameObjPtr(void* p, StackFrame *fp) {
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
    uint32 setflags = (js_CodeSpec[*cx->regs().pc].format & (JOF_SET | JOF_INCDEC | JOF_FOR));

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
    }
    import(EosAddress(eos_ins, slot * sizeof(double)), vp, type, "global", index, NULL);
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
TraceRecorder::writeBack(LIns* ins, LIns* base, ptrdiff_t offset, bool shouldDemoteToInt32)
{
    




    JS_ASSERT(base == lirbuf->sp || base == eos_ins);
    if (shouldDemoteToInt32 && IsPromotedInt32(ins))
        ins = w.demoteToInt32(ins);

    Address addr;
    if (base == lirbuf->sp) {
        addr = StackAddress(base, offset);
    } else {
        addr = EosAddress(base, offset);
        unsigned slot = unsigned(offset / sizeof(double));
        (void)pendingGlobalSlotsToSet.append(slot);  
    }
    return w.st(ins, addr);
}


JS_REQUIRES_STACK void
TraceRecorder::setImpl(void* p, LIns* i, bool shouldDemoteToInt32)
{
    JS_ASSERT(i != NULL);
    checkForGlobalObjectReallocation();
    tracker.set(p, i);

    





    LIns* x = nativeFrameTracker.get(p);
    if (!x) {
        if (isVoidPtrGlobal(p))
            x = writeBack(i, eos_ins, nativeGlobalOffset((Value *)p), shouldDemoteToInt32);
        else
            x = writeBack(i, lirbuf->sp, nativespOffsetImpl(p), shouldDemoteToInt32);
        nativeFrameTracker.set(p, x);
    } else {
#if defined NANOJIT_64BIT
        JS_ASSERT( x->isop(LIR_stq) || x->isop(LIR_sti) || x->isop(LIR_std));
#else
        JS_ASSERT( x->isop(LIR_sti) || x->isop(LIR_std));
#endif

        ptrdiff_t disp;
        LIns *base = x->oprnd2();
        if (base->isop(LIR_addp) && base->oprnd2()->isImmP()) {
            disp = ptrdiff_t(base->oprnd2()->immP());
            base = base->oprnd1();
        } else {
            disp = x->disp();
        }

        JS_ASSERT(base == lirbuf->sp || base == eos_ins);
        JS_ASSERT(disp == ((base == lirbuf->sp)
                            ? nativespOffsetImpl(p)
                            : nativeGlobalOffset((Value *)p)));

        writeBack(i, base, disp, shouldDemoteToInt32);
    }
}

JS_REQUIRES_STACK inline void
TraceRecorder::set(Value* p, LIns* i, bool shouldDemoteToInt32)
{
    return setImpl(p, i, shouldDemoteToInt32);
}

JS_REQUIRES_STACK void
TraceRecorder::setFrameObjPtr(void* p, LIns* i, bool shouldDemoteToInt32)
{
    JS_ASSERT(isValidFrameObjPtr(p));
    return setImpl(p, i, shouldDemoteToInt32);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::attemptImport(const Value* p)
{
    if (LIns* i = getFromTracker(p))
        return i;

    
    CountSlotsVisitor countVisitor(p);
    VisitStackSlots(countVisitor, cx, callDepth);

    if (countVisitor.stopped() || size_t(p - cx->fp()->slots()) < cx->fp()->numSlots())
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
        importImpl(StackAddress(lirbuf->sp, -tree->nativeStackBase + slot * sizeof(jsdouble)),
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
TraceRecorder::isValidFrameObjPtr(void *p)
{
    StackFrame *fp = cx->fp();
    for (; fp; fp = fp->prev()) {
        if (fp->addressOfScopeChain() == p || fp->addressOfArgs() == p)
            return true;
    }
    return false;
}
#endif

JS_REQUIRES_STACK LIns*
TraceRecorder::getFrameObjPtr(void *p)
{
    JS_ASSERT(isValidFrameObjPtr(p));
    return getImpl(p);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::addr(Value* p)
{
    return isGlobal(p)
           ? w.addp(eos_ins, w.nameImmw(nativeGlobalOffset(p)))
           : w.addp(lirbuf->sp, w.nameImmw(nativespOffset(p)));
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
TraceRecorder::checkForGlobalObjectReallocationHelper()
{
    debug_only_print0(LC_TMTracer, "globalObj->slots relocated, updating tracker\n");
    Value* src = global_slots;
    Value* dst = globalObj->getSlots();
    jsuint length = globalObj->capacity;
    LIns** map = (LIns**)alloca(sizeof(LIns*) * length);
    for (jsuint n = 0; n < length; ++n) {
        map[n] = tracker.get(src);
        tracker.set(src++, NULL);
    }
    for (jsuint n = 0; n < length; ++n)
        tracker.set(dst++, map[n]);
    global_slots = globalObj->getSlots();
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
    JSValueType *mTypeMap;
public:
    AdjustCallerGlobalTypesVisitor(TraceRecorder &recorder,
                                   JSValueType *typeMap) :
        mRecorder(recorder),
        mCx(mRecorder.cx),
        mLirbuf(mRecorder.lirbuf),
        mTypeMap(typeMap)
    {}

    JSValueType* getTypeMap()
    {
        return mTypeMap;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot) {
        LIns *ins = mRecorder.get(vp);
        bool isPromote = IsPromotedInt32(ins);
        if (isPromote && *mTypeMap == JSVAL_TYPE_DOUBLE) {
            mRecorder.w.st(mRecorder.get(vp),
                           EosAddress(mRecorder.eos_ins, mRecorder.nativeGlobalOffset(vp)));
            



            mRecorder.traceMonitor->oracle->markGlobalSlotUndemotable(mCx, slot);
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
    unsigned mSlotnum;
    JSValueType *mTypeMap;
public:
    AdjustCallerStackTypesVisitor(TraceRecorder &recorder,
                                  JSValueType *typeMap) :
        mRecorder(recorder),
        mCx(mRecorder.cx),
        mLirbuf(mRecorder.lirbuf),
        mSlotnum(0),
        mTypeMap(typeMap)
    {}

    JSValueType* getTypeMap()
    {
        return mTypeMap;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, StackFrame* fp) {
        
        for (size_t i = 0; i < count; ++i) {
            LIns *ins = mRecorder.get(vp);
            bool isPromote = IsPromotedInt32(ins);
            if (isPromote && *mTypeMap == JSVAL_TYPE_DOUBLE) {
                mRecorder.w.st(ins, StackAddress(mLirbuf->sp, mRecorder.nativespOffset(vp)));
                



                mRecorder.traceMonitor->oracle->markStackSlotUndemotable(mCx, mSlotnum);
            }
            JS_ASSERT(!(!isPromote && *mTypeMap == JSVAL_TYPE_INT32));
            ++vp;
            ++mTypeMap;
            ++mSlotnum;
        }
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(void* p, StackFrame* fp) {
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
            t = IsPromotedInt32(i) ? JSVAL_TYPE_INT32 : JSVAL_TYPE_DOUBLE;
        } else if (isGlobal(vp)) {
            int offset = tree->globalSlots->offsetOf(uint16(nativeGlobalSlot(vp)));
            JS_ASSERT(offset != -1);
            t = importTypeMap[importStackSlots + offset];
        } else {
            t = importTypeMap[nativeStackSlot(vp)];
        }
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
    visitStackSlots(Value *vp, size_t count, StackFrame* fp) {
        for (size_t i = 0; i < count; ++i)
            *mTypeMap++ = mRecorder.determineSlotType(vp++);
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(void* p, StackFrame* fp) {
        *mTypeMap++ = getFrameObjPtrTraceType(p, fp);
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
                      (void*)exit->from, (void*)cx->regs().pc, cx->fp()->script()->filename,
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
    StackFrame* const fp = cx->fp();
    FrameRegs& regs = cx->regs();
    jsbytecode* pc = regs.pc;

    



    const JSCodeSpec& cs = js_CodeSpec[*pc];

    




    bool resumeAfter = (pendingSpecializedNative &&
                        JSTN_ERRTYPE(pendingSpecializedNative) == FAIL_STATUS);
    if (resumeAfter) {
        JS_ASSERT(*pc == JSOP_CALL || *pc == JSOP_FUNAPPLY || *pc == JSOP_FUNCALL ||
                  *pc == JSOP_NEW || *pc == JSOP_SETPROP || *pc == JSOP_SETNAME);
        pc += cs.length;
        regs.pc = pc;
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
        if (pendingUnboxSlot == regs.sp - 2)
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
        regs.pc = pc - cs.length;
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
            if (e->pc == pc && (e->imacpc == fp->maybeImacropc()) &&
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
    exit->numStackSlotsBelowCurrentFrame = cx->fp()->isFunctionFrame() ?
                                           nativeStackOffset(&cx->fp()->calleev()) / sizeof(double) :
                                           0;
    exit->exitType = exitType;
    exit->pc = pc;
    exit->imacpc = fp->maybeImacropc();
    exit->sp_adj = (stackSlots * sizeof(double)) - tree->nativeStackBase;
    exit->rp_adj = exit->calldepth * sizeof(FrameInfo*);
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


static bool
isCond(LIns* ins)
{
    return ins->isCmp() || ins->isImmI(0) || ins->isImmI(1);
}


void
TraceRecorder::ensureCond(LIns** ins, bool* cond)
{
    if (!isCond(*ins)) {
        *cond = !*cond;
        *ins = (*ins)->isI() ? w.eqi0(*ins) : w.eqp0(*ins);
    }
}


























JS_REQUIRES_STACK RecordingStatus
TraceRecorder::guard(bool expected, LIns* cond, VMSideExit* exit,
                     bool abortIfAlwaysExits)
{
    if (exit->exitType == LOOP_EXIT)
        tree->sideExits.add(exit);

    JS_ASSERT(isCond(cond));

    if ((cond->isImmI(0) && expected) || (cond->isImmI(1) && !expected)) {
        if (abortIfAlwaysExits) {
            
            RETURN_STOP("Constantly false guard detected");
        }
        









        JS_NOT_REACHED("unexpected constantly false guard detected");
    }

    



    GuardRecord* guardRec = createGuardRecord(exit);
    expected ? w.xf(cond, guardRec) : w.xt(cond, guardRec);
    return RECORD_CONTINUE;
}






JS_REQUIRES_STACK RecordingStatus
TraceRecorder::guard(bool expected, LIns* cond, ExitType exitType,
                     bool abortIfAlwaysExits)
{
    return guard(expected, cond, snapshot(exitType), abortIfAlwaysExits);
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





static inline bool
ProhibitFlush(TraceMonitor *tm)
{
    return !!tm->tracerState; 
}

static void
ResetJITImpl(JSContext* cx, TraceMonitor* tm)
{
    if (!cx->traceJitEnabled)
        return;
    debug_only_print0(LC_TMTracer, "Flushing cache.\n");
    if (tm->recorder) {
        JS_ASSERT_NOT_ON_TRACE(cx);
        AbortRecording(cx, "flush cache");
    }
#if JS_METHODJIT
    if (tm->profile)
        AbortProfiling(cx);
#endif
    if (ProhibitFlush(tm)) {
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
        ResetJIT(cx, traceMonitor, FR_DEEP_BAIL);
        return ARECORD_ABORTED;
    }
    if (tree->maxNativeStackSlots >= TraceNativeStorage::MAX_NATIVE_STACK_SLOTS) {
        debug_only_print0(LC_TMTracer, "Blacklist: excessive stack use.\n");
        Blacklist((jsbytecode*)tree->ip);
        return ARECORD_STOP;
    }
    if (anchor)
        ++tree->branchCount;
    if (outOfMemory())
        return ARECORD_STOP;

    
#if defined DEBUG && !defined WIN32
    
    const char* filename = cx->fp()->script()->filename;
    char* label = (char*) cx->malloc_((filename ? strlen(filename) : 7) + 16);
    if (label) {
        sprintf(label, "%s:%u", filename ? filename : "<stdin>",
                js_FramePCToLineNumber(cx, cx->fp()));
        lirbuf->printer->addrNameMap->addAddrRange(fragment, sizeof(Fragment), 0, label);
        cx->free_(label);
    }
#endif

    Assembler *assm = traceMonitor->assembler;
    JS_ASSERT(!assm->error());
    assm->compile(fragment, tempAlloc(), true verbose_only(, lirbuf->printer));

    if (assm->error()) {
        assm->setError(nanojit::None);
        debug_only_print0(LC_TMTracer, "Blacklisted: error during compilation\n");
        Blacklist((jsbytecode*)tree->ip);
        return ARECORD_STOP;
    }

    if (outOfMemory())
        return ARECORD_STOP;
    ResetRecordingAttempts(traceMonitor, (jsbytecode*)fragment->ip);
    ResetRecordingAttempts(traceMonitor, (jsbytecode*)tree->ip);
    JS_ASSERT(!assm->error());
    if (anchor)
        assm->patch(anchor);
    if (assm->error())
        return ARECORD_STOP;
    JS_ASSERT(fragment->code());
    JS_ASSERT_IF(fragment == fragment->root, fragment->root == tree);

    return ARECORD_CONTINUE;
}

static bool
JoinPeers(Assembler* assm, VMSideExit* exit, TreeFragment* target)
{
    exit->target = target;
    JS_ASSERT(!assm->error());
    assm->patch(exit);
    if (assm->error())
        return false;

    debug_only_printf(LC_TMTreeVis, "TREEVIS JOIN ANCHOR=%p FRAG=%p\n", (void*)exit, (void*)target);

    if (exit->root() == target)
        return true;

    target->dependentTrees.addUnique(exit->root());
    exit->root()->linkedTrees.addUnique(target);
    return true;
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
          : vp(NULL), isPromotedInt32(false), lastCheck(TypeCheck_Bad)
        {}
        SlotInfo(Value* vp, bool isPromotedInt32)
          : vp(vp), isPromotedInt32(isPromotedInt32), lastCheck(TypeCheck_Bad),
            type(getCoercedType(*vp))
        {}
        SlotInfo(JSValueType t)
          : vp(NULL), isPromotedInt32(false), lastCheck(TypeCheck_Bad), type(t)
        {}
        SlotInfo(Value* vp, JSValueType t)
          : vp(vp), isPromotedInt32(t == JSVAL_TYPE_INT32), lastCheck(TypeCheck_Bad), type(t)
        {}
        void            *vp;
        bool            isPromotedInt32;
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
        bool isPromotedInt32 = false;
        if (vp->isNumber()) {
            if (LIns* i = mRecorder.getFromTracker(vp)) {
                isPromotedInt32 = IsPromotedInt32(i);
            } else if (mRecorder.isGlobal(vp)) {
                int offset = mRecorder.tree->globalSlots->offsetOf(uint16(mRecorder.nativeGlobalSlot(vp)));
                JS_ASSERT(offset != -1);
                isPromotedInt32 = mRecorder.importTypeMap[mRecorder.importStackSlots + offset] ==
                                  JSVAL_TYPE_INT32;
            } else {
                isPromotedInt32 = mRecorder.importTypeMap[mRecorder.nativeStackSlot(vp)] ==
                                  JSVAL_TYPE_INT32;
            }
        }
        slots.add(SlotInfo(vp, isPromotedInt32));
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
            JS_ASSERT_IF(ins, IsPromotedInt32(ins));
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
                          "checkType slot %d: interp=%c typemap=%c isNum=%d isPromotedInt32=%d\n",
                          i,
                          TypeToChar(slots[i].type),
                          TypeToChar(t),
                          slots[i].type == JSVAL_TYPE_INT32 || slots[i].type == JSVAL_TYPE_DOUBLE,
                          slots[i].isPromotedInt32);
        switch (t) {
          case JSVAL_TYPE_INT32:
            if (slots[i].type != JSVAL_TYPE_INT32 && slots[i].type != JSVAL_TYPE_DOUBLE)
                return TypeCheck_Bad; 
            
            if (!slots[i].isPromotedInt32)
                return TypeCheck_Undemote;
            
            JS_ASSERT_IF(slots[i].vp,
                         hasInt32Repr(*(const Value *)slots[i].vp) && slots[i].isPromotedInt32);
            return slots[i].vp ? TypeCheck_Promote : TypeCheck_Okay;
          case JSVAL_TYPE_DOUBLE:
            if (slots[i].type != JSVAL_TYPE_INT32 && slots[i].type != JSVAL_TYPE_DOUBLE)
                return TypeCheck_Bad; 
            if (slots[i].isPromotedInt32)
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
    visitStackSlots(Value *vp, size_t count, StackFrame* fp)
    {
        for (size_t i = 0; i < count; i++)
            addSlot(&vp[i]);
        return true;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(void* p, StackFrame* fp)
    {
        addSlot(getFrameObjPtrTraceType(p, fp));
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
    JS_ASSERT(tree->first == LookupLoop(traceMonitor, ip, tree->globalObj, tree->globalShape, tree->argc));

    
    bool onlyUndemotes = false;
    for (TreeFragment *peer = tree->first; peer != NULL; peer = peer->peer) {
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
    VMSideExit *exit = snapshot(UNSTABLE_LOOP_EXIT);

    DefaultSlotMap slotMap(*this);
    VisitSlots(slotMap, cx, 0, *tree->globalSlots);

    




    JS_ASSERT(*cx->regs().pc == JSOP_TRACE || *cx->regs().pc == JSOP_NOTRACE);
    JS_ASSERT(!cx->fp()->hasImacropc());

    if (callDepth != 0) {
        debug_only_print0(LC_TMTracer,
                          "Blacklisted: stack depth mismatch, possible recursion.\n");
        Blacklist((jsbytecode*)tree->ip);
        trashSelf = true;
        return ARECORD_STOP;
    }

    JS_ASSERT(exit->numStackSlots == tree->nStackTypes);
    JS_ASSERT(fragment->root == tree);
    JS_ASSERT(!trashSelf);

    TreeFragment* peer = NULL;

    TypeConsensus consensus = selfTypeStability(slotMap);
    if (consensus != TypeConsensus_Okay) {
        TypeConsensus peerConsensus = peerTypeStability(slotMap, tree->ip, &peer);
        
        if (peerConsensus != TypeConsensus_Bad)
            consensus = peerConsensus;
    }

#if DEBUG
    if (consensus != TypeConsensus_Okay || peer)
        AUDIT(unstableLoopVariable);
#endif

    



    if (consensus == TypeConsensus_Okay)
        slotMap.adjustTypes();

    if (consensus != TypeConsensus_Okay || peer) {
        fragment->lastIns = w.x(createGuardRecord(exit));

        
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
            w.j(loopLabel);
            w.comment("end-loop");
            w.livep(lirbuf->state);
        }

        exit->target = tree;
        





        fragment->lastIns = w.x(createGuardRecord(exit));
    }

    CHECK_STATUS_A(compile());

    debug_only_printf(LC_TMTreeVis, "TREEVIS CLOSELOOP EXIT=%p PEER=%p\n", (void*)exit, (void*)peer);

    JS_ASSERT(LookupLoop(traceMonitor, tree->ip, tree->globalObj, tree->globalShape, tree->argc) ==
              tree->first);
    JS_ASSERT(tree->first);

    peer = tree->first;
    if (!joinEdgesToEntry(peer))
        return ARECORD_STOP;

    debug_only_stmt(DumpPeerStability(traceMonitor, peer->ip, peer->globalObj,
                                      peer->globalShape, peer->argc);)

    debug_only_print0(LC_TMTracer,
                      "updating specializations on dependent and linked trees\n");
    if (tree->code())
        SpecializeTreesToMissingGlobals(cx, globalObj, tree);

    



    if (outerPC)
        AttemptCompilation(traceMonitor, globalObj, outerScript, outerPC, outerArgc);
#ifdef JS_JIT_SPEW
    debug_only_printf(LC_TMMinimal,
                      "Recording completed at  %s:%u@%u via closeLoop (FragID=%06u)\n",
                      cx->fp()->script()->filename,
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
TypeMapLinkability(JSContext* cx, TraceMonitor *tm, const TypeMap& typeMap, TreeFragment* peer)
{
    const TypeMap& peerMap = peer->typeMap;
    unsigned minSlots = JS_MIN(typeMap.length(), peerMap.length());
    TypeConsensus consensus = TypeConsensus_Okay;
    for (unsigned i = 0; i < minSlots; i++) {
        if (typeMap[i] == peerMap[i])
            continue;
        if (typeMap[i] == JSVAL_TYPE_INT32 && peerMap[i] == JSVAL_TYPE_DOUBLE &&
            IsSlotUndemotable(tm->oracle, cx, peer, i, peer->ip)) {
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

JS_REQUIRES_STACK bool
TraceRecorder::joinEdgesToEntry(TreeFragment* peer_root)
{
    if (fragment->root != fragment)
        return true;

    TypeMap typeMap(NULL, traceMonitor->oracle);
    Queue<unsigned> undemotes(NULL);

    for (TreeFragment* peer = peer_root; peer; peer = peer->peer) {
        if (!peer->code())
            continue;
        UnstableExit* uexit = peer->unstableExits;
        while (uexit != NULL) {
            
            FullMapFromExit(typeMap, uexit->exit);
            
            TypeConsensus consensus = TypeMapLinkability(cx, traceMonitor, typeMap, tree);
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
                if (!JoinPeers(traceMonitor->assembler, uexit->exit, tree))
                    return false;
                uexit = peer->removeUnstableExit(uexit->exit);
            } else {
                
                if (findUndemotesInTypemaps(typeMap, tree, undemotes)) {
                    JS_ASSERT(peer == uexit->fragment->root);
                    if (fragment == peer)
                        trashSelf = true;
                    else
                        whichTreesToTrash.addUnique(uexit->fragment->root);
                    break;
                }
                uexit = uexit->next;
            }
        }
    }
    return true;
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

    fragment->lastIns = w.x(createGuardRecord(exit));

    CHECK_STATUS_A(compile());

    debug_only_printf(LC_TMTreeVis, "TREEVIS ENDLOOP EXIT=%p\n", (void*)exit);

    JS_ASSERT(LookupLoop(traceMonitor, tree->ip, tree->globalObj, tree->globalShape, tree->argc) ==
              tree->first);

    if (!joinEdgesToEntry(tree->first))
        return ARECORD_STOP;

    debug_only_stmt(DumpPeerStability(traceMonitor, tree->ip, tree->globalObj,
                                      tree->globalShape, tree->argc);)

    



    debug_only_print0(LC_TMTracer,
                      "updating specializations on dependent and linked trees\n");
    if (tree->code())
        SpecializeTreesToMissingGlobals(cx, globalObj, fragment->root);

    



    if (outerPC)
        AttemptCompilation(traceMonitor, globalObj, outerScript, outerPC, outerArgc);
#ifdef JS_JIT_SPEW
    debug_only_printf(LC_TMMinimal,
                      "Recording completed at  %s:%u@%u via endLoop (FragID=%06u)\n",
                      cx->fp()->script()->filename,
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
        




        ptrdiff_t sp_adj = nativeStackOffset(&cx->fp()->calleev());

        
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
        LIns* sp_top = w.addp(lirbuf->sp, w.nameImmw(sp_offset));
        guard(true, w.ltp(sp_top, eos_ins), exit);

        
        ptrdiff_t rp_offset = rp_adj + inner->maxCallDepth * sizeof(FrameInfo*);
        LIns* rp_top = w.addp(lirbuf->rp, w.nameImmw(rp_offset));
        guard(true, w.ltp(rp_top, eor_ins), exit);

        sp_offset =
                - tree->nativeStackBase 
                + sp_adj 
                + inner->nativeStackBase; 
        
        w.stStateField(w.addp(lirbuf->sp, w.nameImmw(sp_offset)), sp);
        w.stStateField(w.addp(lirbuf->rp, w.nameImmw(rp_adj)), rp);
    }

    





    w.xbarrier(createGuardRecord(exit));
}

class ClearSlotsVisitor : public SlotVisitorBase
{
    Tracker &tracker;
  public:
    ClearSlotsVisitor(Tracker &tracker)
      : tracker(tracker)
    {}

    JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, StackFrame *) {
        for (Value *vpend = vp + count; vp != vpend; ++vp)
            tracker.set(vp, NULL);
        return true;
    }

    JS_ALWAYS_INLINE bool
    visitFrameObjPtr(void *p, StackFrame *) {
        tracker.set(p, NULL);
        return true;
    }
};

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
    LIns* rec = w.call(ci, args);
    LIns* lr = w.ldpGuardRecordExit(rec);
    LIns* nested = w.jtUnoptimizable(w.eqiN(w.ldiVMSideExitField(lr, exitType), NESTED_EXIT));

    




    w.stStateField(lr, lastTreeExitGuard);
    LIns* done1 = w.j(NULL);

    




    w.label(nested);
    LIns* done2 = w.jfUnoptimizable(w.eqp0(w.ldpStateField(lastTreeCallGuard)));
    w.stStateField(lr, lastTreeCallGuard);
    w.stStateField(w.addp(w.ldpStateField(rp),
                          w.i2p(w.lshiN(w.ldiVMSideExitField(lr, calldepth),
                                        sizeof(void*) == 4 ? 2 : 3))),
                   rpAtLastTreeCall);
    w.label(done1, done2);

    



    w.stStateField(lr, outermostTreeExitGuard);

    
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

    
    ClearSlotsVisitor visitor(tracker);
    VisitStackSlots(visitor, cx, callDepth);
    SlotList& gslots = *tree->globalSlots;
    for (unsigned i = 0; i < gslots.length(); i++) {
        unsigned slot = gslots[i];
        Value* vp = &globalObj->getSlotRef(slot);
        tracker.set(vp, NULL);
    }

    
    importTypeMap.setLength(NativeStackSlots(cx, callDepth));
    unsigned startOfInnerFrame = importTypeMap.length() - exit->numStackSlots;
    for (unsigned i = 0; i < exit->numStackSlots; i++)
        importTypeMap[startOfInnerFrame + i] = exit->stackTypeMap()[i];
    importStackSlots = importTypeMap.length();
    JS_ASSERT(importStackSlots == NativeStackSlots(cx, callDepth));

    



    BuildGlobalTypeMapFromInnerTree(importTypeMap, exit);

    importGlobalSlots = importTypeMap.length() - importStackSlots;
    JS_ASSERT(importGlobalSlots == tree->globalSlots->length());

    
    if (callDepth > 0) {
        w.stStateField(lirbuf->sp, sp);
        w.stStateField(lirbuf->rp, rp);
    }

    



    VMSideExit* nestedExit = snapshot(NESTED_EXIT);
    JS_ASSERT(exit->exitType == LOOP_EXIT);
    guard(true, w.eqp(lr, w.nameImmpNonGC(exit)), nestedExit);
    debug_only_printf(LC_TMTreeVis, "TREEVIS TREECALL INNER=%p EXIT=%p GUARD=%p\n", (void*)inner,
                      (void*)nestedExit, (void*)exit);

    
    inner->dependentTrees.addUnique(fragment->root);
    tree->linkedTrees.addUnique(inner);
}


JS_REQUIRES_STACK void
TraceRecorder::trackCfgMerges(jsbytecode* pc)
{
    
    JS_ASSERT((*pc == JSOP_IFEQ) || (*pc == JSOP_IFEQX));
    jssrcnote* sn = js_GetSrcNote(cx->fp()->script(), pc);
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
    JS_ASSERT(isCond(x));
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
            JS_ASSERT(!cx->fp()->hasImacropc() && (pc == cx->regs().pc || pc == cx->regs().pc + 1));
            FrameRegs orig = cx->regs();

            cx->regs().pc = (jsbytecode*)tree->ip;
            cx->regs().sp = cx->fp()->base() + tree->spOffsetAtEntry;

            JSContext* localcx = cx;
            AbortableRecordingStatus ars = closeLoop();
            localcx->regs() = orig;
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
        ResetJIT(cx, tm, FR_DEEP_BAIL);
        return false;
    }

    if (globalObj->numSlots() > MAX_GLOBAL_SLOTS) {
        if (tm->recorder)
            AbortRecording(cx, "too many slots in global object");
        return false;
    }

    




    if (!globalObj->hasOwnShape()) {
        if (!globalObj->globalObjectOwnShapeChange(cx)) {
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
            Backoff(tm, (jsbytecode*) root->ip);
            ResetJIT(cx, tm, FR_GLOBAL_SHAPE_MISMATCH);
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
    ResetJIT(cx, tm, FR_GLOBALS_FULL);
    return false;
}





bool JS_REQUIRES_STACK
TraceRecorder::startRecorder(JSContext* cx, TraceMonitor *tm, VMSideExit* anchor, VMFragment* f,
                             unsigned stackSlots, unsigned ngslots,
                             JSValueType* typeMap, VMSideExit* expectedInnerExit,
                             JSScript* outerScript, jsbytecode* outerPC, uint32 outerArgc,
                             bool speculate)
{
    JS_ASSERT(!tm->needFlush);
    JS_ASSERT_IF(cx->fp()->hasImacropc(), f->root != f);

    tm->recorder = cx->new_<TraceRecorder>(cx, tm, anchor, f, stackSlots, ngslots, typeMap,
                                             expectedInnerExit, outerScript, outerPC, outerArgc,
                                             speculate);

    if (!tm->recorder || tm->outOfMemory() || OverfullJITCache(cx, tm)) {
        ResetJIT(cx, tm, FR_OOM);
        return false;
    }

    return true;
}

static void
TrashTree(TreeFragment* f)
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
        TrashTree(data[n]);
    data = f->linkedTrees.data();
    length = f->linkedTrees.length();
    for (unsigned n = 0; n < length; ++n)
        TrashTree(data[n]);
}

static void
SynthesizeFrame(JSContext* cx, const FrameInfo& fi, JSObject* callee)
{
    VOUCH_DOES_NOT_REQUIRE_STACK();

    
    StackFrame* const fp = cx->fp();
    JS_ASSERT_IF(!fi.imacpc,
                 js_ReconstructStackDepth(cx, fp->script(), fi.pc) ==
                 uintN(fi.spdist - fp->numFixed()));

    
    JSFunction* newfun = callee->getFunctionPrivate();
    JSScript* newscript = newfun->script();

    
    FrameRegs &regs = cx->regs();
    regs.sp = fp->slots() + fi.spdist;
    regs.pc = fi.pc;
    if (fi.imacpc)
        fp->setImacropc(fi.imacpc);

    
    uintN argc = fi.get_argc();
    uint32 flags = fi.is_constructing() ? StackFrame::CONSTRUCTING : 0;

    
    StackFrame *newfp = cx->stack.getInlineFrame(cx, regs.sp, argc, newfun,
                                                 newscript, &flags);

    
    newfp->initCallFrame(cx, *callee, newfun, argc, flags);

#ifdef DEBUG
    
    if (newfp->hasOverflowArgs()) {
        Value *beg = newfp->actualArgs() - 2;
        Value *end = newfp->actualArgs() + newfp->numFormalArgs();
        for (Value *p = beg; p != end; ++p)
            p->setMagic(JS_ARG_POISON);
    }

    
    newfp->thisValue().setMagic(JS_THIS_POISON);
    newfp->setScopeChainNoCallObj(*StackFrame::sInvalidScopeChain);
#endif

    
    cx->stack.pushInlineFrame(newscript, newfp, cx->regs());

    

    
    JSInterpreterHook hook = cx->debugHooks->callHook;
    if (hook) {
        newfp->setHookData(hook(cx, Jsvalify(newfp), JS_TRUE, 0,
                                cx->debugHooks->callHookData));
    }
}

static JS_REQUIRES_STACK bool
RecordTree(JSContext* cx, TraceMonitor* tm, TreeFragment* first,
           JSScript* outerScript, jsbytecode* outerPC,
           uint32 outerArgc, SlotList* globalSlots)
{
    
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
        Backoff(tm, (jsbytecode*) localRootIP);
        return false;
    }

    AUDIT(recorderStarted);

    if (tm->outOfMemory() ||
        OverfullJITCache(cx, tm) ||
        !tm->tracedScripts.put(cx->fp()->script()))
    {
        if (!OverfullJITCache(cx, tm))
            js_ReportOutOfMemory(cx);
        Backoff(tm, (jsbytecode*) f->root->ip);
        ResetJIT(cx, tm, FR_OOM);
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

    
    return TraceRecorder::startRecorder(cx, tm, NULL, f, f->nStackTypes,
                                        f->globalSlots->length(),
                                        f->typeMap.data(), NULL,
                                        outerScript, outerPC, outerArgc, speculate);
}

static JS_REQUIRES_STACK TypeConsensus
FindLoopEdgeTarget(JSContext* cx, TraceMonitor* tm, VMSideExit* exit, TreeFragment** peerp)
{
    TreeFragment* from = exit->root();

    JS_ASSERT(from->code());
    Oracle* oracle = tm->oracle;

    TypeMap typeMap(NULL, oracle);
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
        TypeConsensus consensus = TypeMapLinkability(cx, tm, typeMap, peer);
        if (consensus == TypeConsensus_Okay || consensus == TypeConsensus_Undemotes) {
            *peerp = peer;
            return consensus;
        }
    }

    return TypeConsensus_Bad;
}

static JS_REQUIRES_STACK bool
AttemptToStabilizeTree(JSContext* cx, TraceMonitor* tm, JSObject* globalObj, VMSideExit* exit,
                       JSScript* outerScript, jsbytecode* outerPC, uint32 outerArgc)
{
    if (tm->needFlush) {
        ResetJIT(cx, tm, FR_DEEP_BAIL);
        return false;
    }

    TreeFragment* from = exit->root();

    TreeFragment* peer = NULL;
    TypeConsensus consensus = FindLoopEdgeTarget(cx, tm, exit, &peer);
    if (consensus == TypeConsensus_Okay) {
        JS_ASSERT(from->globalSlots == peer->globalSlots);
        JS_ASSERT_IF(exit->exitType == UNSTABLE_LOOP_EXIT,
                     from->nStackTypes == peer->nStackTypes);
        JS_ASSERT(exit->numStackSlots == peer->nStackTypes);
        
        if (!JoinPeers(tm->assembler, exit, peer))
            return false;
        



        if (peer->nGlobalTypes() < peer->globalSlots->length())
            SpecializeTreesToMissingGlobals(cx, globalObj, peer);
        JS_ASSERT(from->nGlobalTypes() == from->globalSlots->length());
        
        if (exit->exitType == UNSTABLE_LOOP_EXIT)
            from->removeUnstableExit(exit);
        debug_only_stmt(DumpPeerStability(tm, peer->ip, globalObj, from->globalShape, from->argc);)
        return false;
    } else if (consensus == TypeConsensus_Undemotes) {
        
        TrashTree(peer);
        return false;
    }

    SlotList *globalSlots = from->globalSlots;

    JS_ASSERT(from == from->root);

    
    if (*(jsbytecode*)from->ip == JSOP_NOTRACE)
        return false;

    return RecordTree(cx, tm, from->first, outerScript, outerPC, outerArgc, globalSlots);
}

static JS_REQUIRES_STACK VMFragment*
CreateBranchFragment(JSContext* cx, TraceMonitor* tm, TreeFragment* root, VMSideExit* anchor)
{
    verbose_only(
    uint32_t profFragID = (LogController.lcbits & LC_FragProfile)
                          ? (++(tm->lastFragID)) : 0;
    )

    VMFragment* f = new (*tm->dataAlloc) VMFragment(cx->regs().pc verbose_only(, profFragID));

    debug_only_printf(LC_TMTreeVis, "TREEVIS CREATEBRANCH ROOT=%p FRAG=%p PC=%p FILE=\"%s\""
                      " LINE=%d ANCHOR=%p OFFS=%d\n",
                      (void*)root, (void*)f, (void*)cx->regs().pc, cx->fp()->script()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()), (void*)anchor,
                      FramePCOffset(cx, cx->fp()));
    verbose_only( tm->branches = new (*tm->dataAlloc) Seq<Fragment*>(f, tm->branches); )

    f->root = root;
    if (anchor)
        anchor->target = f;
    return f;
}

static JS_REQUIRES_STACK bool
AttemptToExtendTree(JSContext* cx, TraceMonitor* tm, VMSideExit* anchor, VMSideExit* exitedFrom,
                    JSScript *outerScript, jsbytecode* outerPC
#ifdef MOZ_TRACEVIS
    , TraceVisStateObj* tvso = NULL
#endif
    )
{
    JS_ASSERT(!tm->recorder);

    if (tm->needFlush) {
        ResetJIT(cx, tm, FR_DEEP_BAIL);
#ifdef MOZ_TRACEVIS
        if (tvso) tvso->r = R_FAIL_EXTEND_FLUSH;
#endif
        return false;
    }

    TreeFragment* f = anchor->root();
    JS_ASSERT(f->code());

    



    if (f->branchCount >= MAX_BRANCHES) {
#ifdef JS_METHODJIT
        if (cx->methodJitEnabled && cx->profilingEnabled)
            Blacklist((jsbytecode *)f->ip);
#endif
#ifdef MOZ_TRACEVIS
        if (tvso) tvso->r = R_FAIL_EXTEND_MAX_BRANCHES;
#endif
        return false;
    }

    VMFragment* c = (VMFragment*)anchor->target;
    if (!c) {
        c = CreateBranchFragment(cx, tm, f, anchor);
    } else {
        





        c->ip = cx->regs().pc;
        JS_ASSERT(c->root == f);
    }

    debug_only_printf(LC_TMTracer,
                      "trying to attach another branch to the tree (hits = %d)\n", c->hits());

    int32_t& hits = c->hits();
    int32_t maxHits = HOTEXIT + MAXEXIT;
    if (outerPC || (hits++ >= HOTEXIT && hits <= maxHits)) {
        
        unsigned stackSlots;
        unsigned ngslots;
        JSValueType* typeMap;
        TypeMap fullMap(NULL, tm->oracle);
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
        bool rv = TraceRecorder::startRecorder(cx, tm, anchor, c, stackSlots, ngslots, typeMap,
                                               exitedFrom, outerScript, outerPC, f->argc,
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
ExecuteTree(JSContext* cx, TraceMonitor* tm, TreeFragment* f, uintN& inlineCallCount,
            VMSideExit** innermostNestedGuardp, VMSideExit** lrp);

static inline MonitorResult
RecordingIfTrue(bool b)
{
    return b ? MONITOR_RECORDING : MONITOR_NOT_RECORDING;
}





JS_REQUIRES_STACK MonitorResult
TraceRecorder::recordLoopEdge(JSContext* cx, TraceRecorder* r, uintN& inlineCallCount)
{
    TraceMonitor* tm = r->traceMonitor;

    
    if (tm->needFlush) {
        ResetJIT(cx, tm, FR_DEEP_BAIL);
        return MONITOR_NOT_RECORDING;
    }

    JS_ASSERT(r->fragment && !r->fragment->lastIns);
    TreeFragment* root = r->fragment->root;
    TreeFragment* first = LookupOrAddLoop(tm, cx->regs().pc, root->globalObj,
                                          root->globalShape, entryFrameArgc(cx));

    



    JSObject* globalObj = cx->fp()->scopeChain().getGlobal();
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;
    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots)) {
        JS_ASSERT(!tm->recorder);
        return MONITOR_NOT_RECORDING;
    }

    debug_only_printf(LC_TMTracer,
                      "Looking for type-compatible peer (%s:%d@%d)\n",
                      cx->fp()->script()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()));

    
    TreeFragment* f = r->findNestedCompatiblePeer(first);
    if (!f || !f->code()) {
        AUDIT(noCompatInnerTrees);

        TreeFragment* outerFragment = root;
        JSScript* outerScript = outerFragment->script;
        jsbytecode* outerPC = (jsbytecode*) outerFragment->ip;
        uint32 outerArgc = outerFragment->argc;
        JS_ASSERT(entryFrameArgc(cx) == first->argc);

        if (AbortRecording(cx, "No compatible inner tree") == JIT_RESET)
            return MONITOR_NOT_RECORDING;

        return RecordingIfTrue(RecordTree(cx, tm, first,
                                          outerScript, outerPC, outerArgc, globalSlots));
    }

    AbortableRecordingStatus status = r->attemptTreeCall(f, inlineCallCount);
    if (status == ARECORD_CONTINUE)
        return MONITOR_RECORDING;
    if (status == ARECORD_ERROR) {
        if (tm->recorder)
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
    TraceMonitor *localtm = traceMonitor;

    
    
    
    
    
    
    
    
    
    importTypeMap.setLength(NativeStackSlots(cx, callDepth));
    DetermineTypesVisitor visitor(*this, importTypeMap.data());
    VisitStackSlots(visitor, cx, callDepth);

    VMSideExit* innermostNestedGuard = NULL;
    VMSideExit* lr;
    bool ok = ExecuteTree(cx, traceMonitor, f, inlineCallCount, &innermostNestedGuard, &lr);

    



    JS_ASSERT_IF(localtm->recorder, localtm->recorder == this);
    if (!ok)
        return ARECORD_ERROR;
    if (!localtm->recorder)
        return ARECORD_ABORTED;

    if (!lr) {
        AbortRecording(cx, "Couldn't call inner tree");
        return ARECORD_ABORTED;
    }

    TreeFragment* outerFragment = tree;
    JSScript* outerScript = outerFragment->script;
    jsbytecode* outerPC = (jsbytecode*) outerFragment->ip;
    switch (lr->exitType) {
      case LOOP_EXIT:
        
        if (innermostNestedGuard) {
            if (AbortRecording(cx, "Inner tree took different side exit, abort current "
                                   "recording and grow nesting tree") == JIT_RESET) {
                return ARECORD_ABORTED;
            }
            return AttemptToExtendTree(localCx, localtm,
                                       innermostNestedGuard, lr, outerScript, outerPC)
                   ? ARECORD_CONTINUE
                   : ARECORD_ABORTED;
        }

        JS_ASSERT(oldInlineCallCount == inlineCallCount);

        
        emitTreeCall(f, lr);
        return ARECORD_CONTINUE;

      case UNSTABLE_LOOP_EXIT:
      {
        
        JSObject* _globalObj = globalObj;
        if (AbortRecording(cx, "Inner tree is trying to stabilize, "
                               "abort outer recording") == JIT_RESET) {
            return ARECORD_ABORTED;
        }
        return AttemptToStabilizeTree(localCx, localtm, _globalObj, lr, outerScript, outerPC,
                                      outerFragment->argc)
               ? ARECORD_CONTINUE
               : ARECORD_ABORTED;
      }

      case MUL_ZERO_EXIT:
      case OVERFLOW_EXIT:
        if (lr->exitType == MUL_ZERO_EXIT)
            traceMonitor->oracle->markInstructionSlowZeroTest(cx->regs().pc);
        else
            traceMonitor->oracle->markInstructionUndemotable(cx->regs().pc);
        
      case BRANCH_EXIT:
        
        if (AbortRecording(cx, "Inner tree is trying to grow, "
                               "abort outer recording") == JIT_RESET) {
            return ARECORD_ABORTED;
        }
        return AttemptToExtendTree(localCx, localtm, lr, NULL, outerScript, outerPC)
               ? ARECORD_CONTINUE
               : ARECORD_ABORTED;

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
    bool ok;

    JS_ASSERT(type <= JSVAL_UPPER_INCL_TYPE_OF_BOXABLE_SET);
    JS_ASSERT(type != JSVAL_TYPE_OBJECT);   

    if (v.isInt32()) {
        ok = (type == JSVAL_TYPE_INT32 || type == JSVAL_TYPE_DOUBLE);

    } else if (v.isDouble()) {
        int32_t _;
        ok = (type == JSVAL_TYPE_DOUBLE) || 
             (type == JSVAL_TYPE_INT32 && JSDOUBLE_IS_INT32(v.toDouble(), &_));

    } else if (v.isObject()) {
        ok = v.toObject().isFunction()
           ? type == JSVAL_TYPE_FUNOBJ
           : type == JSVAL_TYPE_NONFUNOBJ;

    } else {
        ok = v.extractNonDoubleObjectTraceType() == type;
    }
#ifdef DEBUG
    char ttag = TypeToChar(type);
    char vtag = ValueToTypeChar(v);
    debug_only_printf(LC_TMTracer, "%c/%c ", vtag, ttag);
    if (!ok)
        debug_only_printf(LC_TMTracer, "%s", "(incompatible types)");
#endif
    return ok;
}

static inline bool
IsFrameObjPtrTypeCompatible(void *p, StackFrame *fp, JSValueType type)
{
    debug_only_printf(LC_TMTracer, "%c/%c ", TypeToChar(type),
                      (p == fp->addressOfScopeChain() || fp->hasArgsObj())
                      ? TypeToChar(JSVAL_TYPE_NONFUNOBJ)
                      : TypeToChar(JSVAL_TYPE_NULL));
    if (p == fp->addressOfScopeChain())
        return type == JSVAL_TYPE_NONFUNOBJ;
    JS_ASSERT(p == fp->addressOfArgs());
    JS_ASSERT(type == JSVAL_TYPE_NONFUNOBJ || type == JSVAL_TYPE_NULL);
    return fp->hasArgsObj() == (type == JSVAL_TYPE_NONFUNOBJ);
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
        mOracle(recorder.traceMonitor->oracle),
        mTypeMap(typeMap),
        mStackSlotNum(0),
        mOk(true)
    {}

    JS_REQUIRES_STACK JS_ALWAYS_INLINE void
    visitGlobalSlot(Value *vp, unsigned n, unsigned slot) {
        debug_only_printf(LC_TMTracer, "global%d=", n);
        if (!IsEntryTypeCompatible(*vp, *mTypeMap)) {
            mOk = false;
        } else if (!IsPromotedInt32(mRecorder.get(vp)) && *mTypeMap == JSVAL_TYPE_INT32) {
            mOracle->markGlobalSlotUndemotable(mCx, slot);
            mOk = false;
        } else if (vp->isInt32() && *mTypeMap == JSVAL_TYPE_DOUBLE) {
            mOracle->markGlobalSlotUndemotable(mCx, slot);
        }
        mTypeMap++;
    }

    






    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitStackSlots(Value *vp, size_t count, StackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), unsigned(i));
            if (!IsEntryTypeCompatible(*vp, *mTypeMap)) {
                mOk = false;
            } else if (!IsPromotedInt32(mRecorder.get(vp)) && *mTypeMap == JSVAL_TYPE_INT32) {
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
    visitFrameObjPtr(void* p, StackFrame* fp) {
        debug_only_printf(LC_TMTracer, "%s%u=", stackSlotKind(), 0);
        if (!IsFrameObjPtrTypeCompatible(p, fp, *mTypeMap))
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
    visitStackSlots(Value *vp, size_t count, StackFrame* fp) {
        for (size_t i = 0; i < count; ++i) {
            if (!mOk)
                break;
            checkSlot(*vp++, stackSlotKind(), i);
        }
        return mOk;
    }

    JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
    visitFrameObjPtr(void* p, StackFrame *fp) {
        debug_only_printf(LC_TMTracer, "%s%d=", stackSlotKind(), 0);
        JS_ASSERT(*(uint8_t*)mTypeMap != 0xCD);
        return mOk = IsFrameObjPtrTypeCompatible(p, fp, *mTypeMap++);
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
    traceMonitor(tm),
    stackBase(tm->storage->stack()),
    sp(stackBase + f->nativeStackBase / sizeof(double)),
    eos(tm->storage->global()),
    callstackBase(tm->storage->callstack()),
    sor(callstackBase),
    rp(callstackBase),
    eor(callstackBase + JS_MIN(TraceNativeStorage::MAX_CALL_STACK_ENTRIES,
                               StackSpace::MAX_INLINE_CALLS - inlineCallCount)),
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
    nativeVp(NULL)
{
    JS_ASSERT(!tm->tracecx);
    tm->tracecx = cx;
    prev = tm->tracerState;
    tm->tracerState = this;

#ifdef JS_METHODJIT
    if (TRACE_PROFILER(cx))
        AbortProfiling(cx);
#endif

    JS_ASSERT(JS_THREAD_DATA(cx)->onTraceCompartment == NULL);
    JS_ASSERT(JS_THREAD_DATA(cx)->recordingCompartment == NULL ||
              JS_THREAD_DATA(cx)->recordingCompartment == cx->compartment);
    JS_ASSERT(JS_THREAD_DATA(cx)->profilingCompartment == NULL);
    JS_THREAD_DATA(cx)->onTraceCompartment = cx->compartment;

    JS_ASSERT(eos == stackBase + TraceNativeStorage::MAX_NATIVE_STACK_SLOTS);
    JS_ASSERT(sp < eos);

    




    JS_ASSERT(inlineCallCount <= StackSpace::MAX_INLINE_CALLS);

#ifdef DEBUG
    



    memset(tm->storage->stack(), 0xCD, TraceNativeStorage::MAX_NATIVE_STACK_SLOTS * sizeof(double));
    memset(tm->storage->callstack(), 0xCD, TraceNativeStorage::MAX_CALL_STACK_ENTRIES * sizeof(FrameInfo*));
#endif
}

JS_ALWAYS_INLINE
TracerState::~TracerState()
{
    JS_ASSERT(!nativeVp);

    if (traceMonitor->tracecx) {
        
        JS_ASSERT(JS_THREAD_DATA(cx)->recordingCompartment == NULL ||
                  JS_THREAD_DATA(cx)->recordingCompartment == cx->compartment);
        JS_ASSERT(JS_THREAD_DATA(cx)->profilingCompartment == NULL);
        JS_ASSERT(JS_THREAD_DATA(cx)->onTraceCompartment == cx->compartment);
        JS_THREAD_DATA(cx)->onTraceCompartment = NULL;
    }
    
    traceMonitor->tracerState = prev;
    traceMonitor->tracecx = NULL;
}


static JS_ALWAYS_INLINE VMSideExit*
ExecuteTrace(JSContext* cx, TraceMonitor* tm, Fragment* f, TracerState& state)
{
    JS_ASSERT(!tm->bailExit);
#ifdef JS_METHODJIT
    JS_ASSERT(!TRACE_PROFILER(cx));
#endif
    union { NIns *code; GuardRecord* (FASTCALL *func)(TracerState*); } u;
    u.code = f->code();
    GuardRecord* rec;
#if defined(JS_NO_FASTCALL) && defined(NANOJIT_IA32)
    SIMULATE_FASTCALL(rec, state, NULL, u.func);
#else
    rec = u.func(&state);
#endif
    JS_ASSERT(!tm->bailExit);
    return (VMSideExit*)rec->exit;
}


static JS_REQUIRES_STACK JS_ALWAYS_INLINE bool
ScopeChainCheck(JSContext* cx, TreeFragment* f)
{
    JS_ASSERT(f->globalObj == cx->fp()->scopeChain().getGlobal());

    















    JSObject* child = &cx->fp()->scopeChain();
    while (JSObject* parent = child->getParent()) {
        if (!IsCacheableNonGlobalScope(child)) {
            debug_only_print0(LC_TMTracer,"Blacklist: non-cacheable object on scope chain.\n");
            Blacklist((jsbytecode*) f->root->ip);
            return false;
        }
        child = parent;
    }
    JS_ASSERT(child == f->globalObj);

    if (!f->globalObj->isGlobal()) {
        debug_only_print0(LC_TMTracer, "Blacklist: non-global at root of scope chain.\n");
        Blacklist((jsbytecode*) f->root->ip);
        return false;
    }

    return true;
}

enum LEAVE_TREE_STATUS {
  NO_DEEP_BAIL = 0,
  DEEP_BAILED = 1
};

static LEAVE_TREE_STATUS
LeaveTree(TraceMonitor *tm, TracerState&, VMSideExit *lr);


static JS_REQUIRES_STACK bool
ExecuteTree(JSContext* cx, TraceMonitor* tm, TreeFragment* f, uintN& inlineCallCount,
            VMSideExit** innermostNestedGuardp, VMSideExit **lrp)
{
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_EXECUTE);
#endif
    JS_ASSERT(f->root == f && f->code());

    if (!ScopeChainCheck(cx, f) || !cx->stack.space().ensureEnoughSpaceToEnterTrace() ||
        inlineCallCount + f->maxCallDepth > StackSpace::MAX_INLINE_CALLS) {
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
                      cx->fp()->script()->filename,
                      js_FramePCToLineNumber(cx, cx->fp()),
                      FramePCOffset(cx, cx->fp()),
           f->execs,
           (void *) f->code());

    debug_only_stmt(uint32 globalSlots = globalObj->numSlots();)
    debug_only_stmt(*(uint64*)&tm->storage->global()[globalSlots] = 0xdeadbeefdeadbeefLL;)

    
    tm->iterationCounter = 0;
    debug_only(int64 t0 = PRMJ_Now();)
#ifdef MOZ_TRACEVIS
    VMSideExit* lr = (TraceVisStateObj(cx, S_NATIVE), ExecuteTrace(cx, tm, f, state));
#else
    VMSideExit* lr = ExecuteTrace(cx, tm, f, state);
#endif
    debug_only(int64 t1 = PRMJ_Now();)

    JS_ASSERT_IF(lr->exitType == LOOP_EXIT, !lr->calldepth);

    
    DebugOnly<LEAVE_TREE_STATUS> lts = LeaveTree(tm, state, lr);
    JS_ASSERT_IF(lts == NO_DEEP_BAIL,
                 *(uint64*)&tm->storage->global()[globalSlots] == 0xdeadbeefdeadbeefLL);

    *lrp = state.innermost;
    bool ok = !(state.builtinStatus & BUILTIN_ERROR);
    JS_ASSERT_IF(cx->isExceptionPending(), !ok);

    size_t iters = tm->iterationCounter;

    f->execs++;
    f->iters += iters;

#ifdef DEBUG
    StackFrame *fp = cx->fp();
    const char *prefix = "";
    if (iters == LOOP_COUNT_MAX)
        prefix = ">";
    debug_only_printf(LC_TMMinimal, "  [%.3f ms] Tree at line %u executed for %s%u iterations;"
                      " executed %u times; leave for %s at %s:%u (%s)\n",
                      double(t1-t0) / PRMJ_USEC_PER_MSEC,
                      f->treeLineNumber, prefix, (uintN)iters, f->execs,
                      getExitName(lr->exitType),
                      fp->script()->filename,
                      js_FramePCToLineNumber(cx, fp),
                      js_CodeName[fp->hasImacropc() ? *fp->imacropc() : *cx->regs().pc]);
#endif
    
#ifdef JS_METHODJIT
    if (cx->methodJitEnabled) {
        if (lr->exitType == LOOP_EXIT && f->iters < MIN_LOOP_ITERS
            && f->execs >= LOOP_CHECK_ITERS)
        {
            debug_only_printf(LC_TMMinimal, "  Blacklisting at line %u (executed only %d iters)\n",
                              f->treeLineNumber, f->iters);
            Blacklist((jsbytecode *)f->ip);
        }
    }
#endif
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

static JS_FORCES_STACK LEAVE_TREE_STATUS
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
        







        if (!(bs & BUILTIN_ERROR)) {
            











            FrameRegs* regs = &cx->regs();
            JSOp op = (JSOp) *regs->pc;

            




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
            JS_ASSERT_IF(!cx->fp()->hasImacropc(),
                         cx->fp()->slots() + cx->fp()->numFixed() +
                         js_ReconstructStackDepth(cx, cx->fp()->script(), regs->pc) ==
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
        return DEEP_BAILED;
    }

    while (callstack < rp) {
        FrameInfo* fi = *callstack;
        
        JSObject* callee = *(JSObject**)&stack[fi->callerHeight];

        







        cx->regs().sp = cx->fp()->slots() + (fi->spdist - (2 + fi->get_argc()));
        int slots = FlushNativeStackFrame(cx, 0 , fi->get_typemap(), stack);

        
        SynthesizeFrame(cx, *fi, callee);
#ifdef DEBUG
        StackFrame* fp = cx->fp();
        debug_only_printf(LC_TMTracer,
                          "synthesized deep frame for %s:%u@%u, slots=%d, fi=%p\n",
                          fp->script()->filename,
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
    unsigned calleeOffset = 0;
    for (unsigned n = 0; n < calldepth; ++n) {
        
        calleeOffset += callstack[n]->callerHeight;
        JSObject* callee = *(JSObject**)&stack[calleeOffset];

        
        SynthesizeFrame(cx, *callstack[n], callee);
        ++*state.inlineCallCountp;
#ifdef DEBUG
        StackFrame* fp = cx->fp();
        debug_only_printf(LC_TMTracer,
                          "synthesized shallow frame for %s:%u@%u\n",
                          fp->script()->filename, js_FramePCToLineNumber(cx, fp),
                          FramePCOffset(cx, fp));
#endif
    }

    




    StackFrame* const fp = cx->fp();

    



    cx->regs().pc = innermost->pc;
    if (innermost->imacpc)
        fp->setImacropc(innermost->imacpc);
    else
        fp->clearImacropc();

    







    uintN slotOffset = innermost->numStackSlots - innermost->numStackSlotsBelowCurrentFrame;
    if (fp->isGlobalFrame()) {
        
        slotOffset += fp->globalScript()->nfixed;
    } else {
        
        slotOffset -= NumSlotsBeforeFixed(fp);
    }
    cx->regs().sp = fp->slots() + slotOffset;

    
    JS_ASSERT_IF(!fp->hasImacropc(),
                 fp->slots() + fp->numFixed() +
                 js_ReconstructStackDepth(cx, fp->script(), cx->regs().pc) == cx->regs().sp);

#ifdef EXECUTE_TREE_TIMER
    uint64 cycles = rdtsc() - state.startTime;
#elif defined(JS_JIT_SPEW)
    uint64 cycles = 0;
#endif
    debug_only_printf(LC_TMTracer,
                      "leaving trace at %s:%u@%u, op=%s, lr=%p, exitType=%s, sp=%lld, "
                      "calldepth=%d, cycles=%llu\n",
                      fp->script()->filename,
                      js_FramePCToLineNumber(cx, fp),
                      FramePCOffset(cx, fp),
                      js_CodeName[fp->hasImacropc() ? *fp->imacropc() : *cx->regs().pc],
                      (void*)lr,
                      getExitName(lr->exitType),
                      (long long int)(cx->regs().sp - fp->base()),
                      calldepth,
                      (unsigned long long int)cycles);

    DebugOnly<int> slots = FlushNativeStackFrame(cx, innermost->calldepth, innermost->stackTypeMap(), stack);
    JS_ASSERT(unsigned(slots) == innermost->numStackSlots);

    






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
        DebugOnly<unsigned> check_ngslots = BuildGlobalTypeMapFromInnerTree(typeMap, innermost);
        JS_ASSERT(check_ngslots == ngslots);
        globalTypeMap = typeMap.data();
    }

    
    JS_ASSERT(state.eos == state.stackBase + TraceNativeStorage::MAX_NATIVE_STACK_SLOTS);
    JSObject* globalObj = outermostTree->globalObj;
    FlushNativeGlobalFrame(cx, globalObj, state.eos, ngslots, gslots, globalTypeMap);

#ifdef JS_JIT_SPEW
    if (innermost->exitType != TIMEOUT_EXIT)
        AUDIT(sideExitIntoInterpreter);
    else
        AUDIT(timeoutIntoInterpreter);
#endif

    state.innermost = innermost;
    return NO_DEEP_BAIL;
}

#if defined(DEBUG) || defined(JS_METHODJIT)
static jsbytecode *
GetLoopBottom(JSContext *cx, jsbytecode *pc)
{
    JS_ASSERT(*pc == JSOP_TRACE || *pc == JSOP_NOTRACE);
    JSScript *script = cx->fp()->script();
    jssrcnote *sn = js_GetSrcNote(script, pc);
    if (!sn)
        return NULL;
    return pc + js_GetSrcNoteOffset(sn, 0);
}
#endif

JS_ALWAYS_INLINE void
TraceRecorder::assertInsideLoop()
{
#ifdef DEBUG
    
    if (callDepth > 0)
        return;

    jsbytecode *pc = cx->fp()->hasImacropc() ? cx->fp()->imacropc() : cx->regs().pc;
    jsbytecode *beg = (jsbytecode *)tree->ip;
    jsbytecode *end = GetLoopBottom(cx, beg);

    




    JS_ASSERT(pc >= beg - JSOP_GOTO_LENGTH && pc <= end);
#endif
}

JS_REQUIRES_STACK MonitorResult
RecordLoopEdge(JSContext* cx, TraceMonitor* tm, uintN& inlineCallCount)
{
#ifdef MOZ_TRACEVIS
    TraceVisStateObj tvso(cx, S_MONITOR);
#endif

    
    if (tm->recorder) {
        tm->recorder->assertInsideLoop();
        jsbytecode* pc = cx->regs().pc;
        if (pc == tm->recorder->tree->ip) {
            AbortableRecordingStatus status = tm->recorder->closeLoop();
            if (status != ARECORD_COMPLETED) {
                if (tm->recorder)
                    AbortRecording(cx, "closeLoop failed");
                return MONITOR_NOT_RECORDING;
            }
        } else {
            MonitorResult r = TraceRecorder::recordLoopEdge(cx, tm->recorder, inlineCallCount);
            JS_ASSERT((r == MONITOR_RECORDING) == (tm->recorder != NULL));
            if (r == MONITOR_RECORDING || r == MONITOR_ERROR)
                return r;

            










            if (pc != cx->regs().pc) {
#ifdef MOZ_TRACEVIS
                tvso.r = R_INNER_SIDE_EXIT;
#endif
                return MONITOR_NOT_RECORDING;
            }
        }
    }
    JS_ASSERT(!tm->recorder);

    



    JSObject* globalObj = cx->fp()->scopeChain().getGlobal();
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;

    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots)) {
        Backoff(tm, cx->regs().pc);
        return MONITOR_NOT_RECORDING;
    }

    
    if (JS_THREAD_DATA(cx)->interruptFlags) {
#ifdef MOZ_TRACEVIS
        tvso.r = R_CALLBACK_PENDING;
#endif
        return MONITOR_NOT_RECORDING;
    }

    jsbytecode* pc = cx->regs().pc;
    uint32 argc = entryFrameArgc(cx);

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

        




        bool rv = RecordTree(cx, tm, f->first, NULL, NULL, 0, globalSlots);
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

    if (!ExecuteTree(cx, tm, match, inlineCallCount, &innermostNestedGuard, &lr))
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
        rv = AttemptToStabilizeTree(cx, tm, globalObj, lr, NULL, NULL, 0);
#ifdef MOZ_TRACEVIS
        if (!rv)
            tvso.r = R_FAIL_STABILIZE;
#endif
        return RecordingIfTrue(rv);

      case MUL_ZERO_EXIT:
      case OVERFLOW_EXIT:
        if (lr->exitType == MUL_ZERO_EXIT)
            tm->oracle->markInstructionSlowZeroTest(cx->regs().pc);
        else
            tm->oracle->markInstructionUndemotable(cx->regs().pc);
        
      case BRANCH_EXIT:
        rv = AttemptToExtendTree(cx, tm, lr, NULL, NULL, NULL
#ifdef MOZ_TRACEVIS
                                                   , &tvso
#endif
                                 );
        return RecordingIfTrue(rv);

      case LOOP_EXIT:
        if (innermostNestedGuard) {
            rv = AttemptToExtendTree(cx, tm, innermostNestedGuard, lr, NULL, NULL
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
    JS_ASSERT(!addPropShapeBefore);

    JS_ASSERT(traceMonitor == &cx->compartment->traceMonitor);
    
    TraceMonitor &localtm = *traceMonitor;
    debug_only_stmt( JSContext *localcx = cx; )
    assertInsideLoop();
    JS_ASSERT(!localtm.profile);

    
    if (localtm.needFlush) {
        ResetJIT(cx, &localtm, FR_DEEP_BAIL);
        return ARECORD_ABORTED;
    }
    JS_ASSERT(!fragment->lastIns);

    



    pendingSpecializedNative = NULL;
    newobj_ins = NULL;
    pendingGlobalSlotsToSet.clear();

    
    if (pendingGuardCondition) {
        LIns* cond = pendingGuardCondition;
        bool expected = true;

        
        ensureCond(&cond, &expected);
        guard(expected, cond, STATUS_EXIT);
        pendingGuardCondition = NULL;
    }

    
    if (pendingUnboxSlot) {
        LIns* val_ins = get(pendingUnboxSlot);
        




        LIns* unboxed_ins = unbox_value(*pendingUnboxSlot,
                                        AnyAddress(val_ins->oprnd1(), val_ins->disp()),
                                        snapshot(BRANCH_EXIT));
        set(pendingUnboxSlot, unboxed_ins);
        pendingUnboxSlot = 0;
    }

    debug_only_stmt(
        if (LogController.lcbits & LC_TMRecorder) {
            void *mark = JS_ARENA_MARK(&cx->tempPool);
            Sprinter sprinter;
            INIT_SPRINTER(cx, &sprinter, &cx->tempPool, 0);

            debug_only_print0(LC_TMRecorder, "\n");
            js_Disassemble1(cx, cx->fp()->script(), cx->regs().pc,
                            cx->fp()->hasImacropc()
                                ? 0 : cx->regs().pc - cx->fp()->script()->code,
                            !cx->fp()->hasImacropc(), &sprinter);

            fprintf(stdout, "%s", sprinter.base);
            JS_ARENA_RELEASE(&cx->tempPool, mark);
        }
    )

    






    AbortableRecordingStatus status;
#ifdef DEBUG
    bool wasInImacro = (cx->fp()->hasImacropc());
#endif
    switch (op) {
      default:
          AbortRecording(cx, "unsupported opcode");
          status = ARECORD_ERROR;
          break;
# define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format)              \
      case op:                                                                \
        w.comment(#op);                                                       \
        status = this->record_##op();                                         \
        break;
# include "jsopcode.tbl"
# undef OPDEF
    }

    

    if (!JSOP_IS_IMACOP(op)) {
        JS_ASSERT(status != ARECORD_IMACRO);
        JS_ASSERT_IF(!wasInImacro, !localcx->fp()->hasImacropc());
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

        if (outOfMemory() || OverfullJITCache(cx, &localtm)) {
            ResetJIT(cx, &localtm, FR_OOM);

            




            return status == ARECORD_IMACRO ? ARECORD_IMACRO_ABORTED : ARECORD_ABORTED;
        }
    } else {
        JS_ASSERT(status == ARECORD_COMPLETED ||
                  status == ARECORD_ABORTED ||
                  status == ARECORD_ERROR);
    }
    return status;
}

JS_REQUIRES_STACK TraceRecorder::AbortResult
AbortRecording(JSContext* cx, const char* reason)
{
#ifdef DEBUG
    JS_ASSERT(TRACE_RECORDER(cx));
    return TRACE_RECORDER(cx)->finishAbort(reason);
#else
    return TRACE_RECORDER(cx)->finishAbort("[no reason]");
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

#if defined(__GNUC__) && defined(AVMPLUS_LINUX)


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
    if (bytes > 1 G)
        bytes = 1 G;
    if (bytes < 128 K)
        bytes = 128 K;
    JS_THREAD_DATA(cx)->maxCodeCacheBytes = bytes;
}

TraceMonitor::TraceMonitor()
  : tracecx(NULL),
    tracerState(NULL),
    bailExit(NULL),
    iterationCounter(0),
    storage(NULL),
    dataAlloc(NULL),
    traceAlloc(NULL),
    tempAlloc(NULL),
    codeAlloc(NULL),
    assembler(NULL),
    frameCache(NULL),
    flushEpoch(0),
    oracle(NULL),
    recorder(NULL),
    profile(NULL),
    recordAttempts(NULL),
    loopProfiles(NULL),
    maxCodeCacheBytes(0),
    needFlush(false),
    cachedTempTypeMap(NULL)
#ifdef DEBUG
    , branches(NULL),
    lastFragID(0),
    profAlloc(NULL),
    profTab(NULL)
#endif
{
    PodZero(&globalStates);
    PodZero(&vmfragments);
}

#ifdef DEBUG
void
TraceMonitor::logFragProfile()
{
    
    
    if (LogController.lcbits & LC_FragProfile) {

        for (Seq<Fragment*>* f = branches; f; f = f->tail)
            FragProfiling_FragFinalizer(f->head, this);

        for (size_t i = 0; i < FRAGMENT_TABLE_SIZE; i++) {
            for (TreeFragment *f = vmfragments[i]; f; f = f->next) {
                JS_ASSERT(f->root == f);
                for (TreeFragment *p = f; p; p = p->peer)
                    FragProfiling_FragFinalizer(p, this);
            }
        }

        if (profTab)
            FragProfiling_showResults(this);

    }
}
#endif

TraceMonitor::~TraceMonitor ()
{

#ifdef DEBUG
    logFragProfile();

    Foreground::delete_(profAlloc); 
    profAlloc = NULL;
#endif

    Foreground::delete_(recordAttempts);
    Foreground::delete_(loopProfiles);
    Foreground::delete_(oracle);

    PodArrayZero(vmfragments);

    Foreground::delete_(frameCache);
    frameCache = NULL;

    Foreground::delete_(codeAlloc);
    codeAlloc = NULL;

    Foreground::delete_(dataAlloc);
    dataAlloc = NULL;

    Foreground::delete_(traceAlloc);
    traceAlloc = NULL;

    Foreground::delete_(tempAlloc);
    tempAlloc = NULL;

    Foreground::delete_(storage);
    storage = NULL;

    Foreground::delete_(cachedTempTypeMap);
    cachedTempTypeMap = NULL;
}

bool
TraceMonitor::init(JSRuntime* rt)
{
#define CHECK_NEW(lhs, type, args) \
    do { lhs = rt->new_<type> args; if (!lhs) return false; } while (0)
#define CHECK_MALLOC(lhs, conversion, size) \
    do { lhs = (conversion)(rt->malloc_(size)); if (!lhs) return false; } while (0)
#define CHECK_ALLOC(lhs, rhs) \
    do { lhs = (rhs); if (!lhs) return false; } while (0)

#if defined JS_JIT_SPEW
    
    CHECK_NEW(profAlloc, VMAllocator, (rt, (char*)NULL, 0));
    CHECK_ALLOC(profTab, new (*profAlloc) FragStatsMap(*profAlloc));
#endif 
    CHECK_NEW(oracle, Oracle, ());

    CHECK_NEW(recordAttempts, RecordAttemptMap, ());
    if (!recordAttempts->init(PC_HASH_COUNT))
        return false;

    CHECK_NEW(loopProfiles, LoopProfileMap, ());
    if (!loopProfiles->init(PC_HASH_COUNT))
        return false;

    char *dataReserve, *traceReserve, *tempReserve;
    CHECK_MALLOC(dataReserve, char*, DataReserveSize);
    CHECK_MALLOC(traceReserve, char*, TraceReserveSize);
    CHECK_MALLOC(tempReserve, char*, TempReserveSize);
    CHECK_NEW(dataAlloc, VMAllocator, (rt, dataReserve, DataReserveSize));
    CHECK_NEW(traceAlloc, VMAllocator, (rt, traceReserve, TraceReserveSize));
    CHECK_NEW(tempAlloc, VMAllocator, (rt, tempReserve, TempReserveSize));
    CHECK_NEW(codeAlloc, CodeAlloc, ());
    CHECK_NEW(frameCache, FrameInfoCache, (dataAlloc));
    CHECK_NEW(storage, TraceNativeStorage, ());
    CHECK_NEW(cachedTempTypeMap, TypeMap, ((Allocator*)NULL, oracle));
    verbose_only( branches = NULL; )

    if (!tracedScripts.init())
        return false;

    flush();

    return true;
}

void
InitJIT()
{
#if defined JS_JIT_SPEW
    
    if (!did_we_set_up_debug_logging) {
        InitJITLogController();
        did_we_set_up_debug_logging = true;
    }
#else
    PodZero(&LogController);
#endif

    if (!did_we_check_processor_features) {
#if defined NANOJIT_IA32
        NJConfig.i386_use_cmov = NJConfig.i386_sse2 = CheckForSSE2();
        NJConfig.i386_fixed_esp = true;
#endif
#if defined NANOJIT_ARM

        disable_debugger_exceptions();

        bool            arm_vfp     = arm_check_vfp();
        unsigned int    arm_arch    = arm_check_arch();

        enable_debugger_exceptions();

        NJConfig.arm_vfp            = arm_vfp;
        NJConfig.soft_float         = !arm_vfp;
        NJConfig.arm_arch           = arm_arch;

        
        
        JS_ASSERT(arm_arch >= 4);
#endif
        did_we_check_processor_features = true;
    }


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
FinishJIT()
{
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

}

JS_REQUIRES_STACK void
PurgeScriptFragments(TraceMonitor* tm, JSScript* script)
{
    debug_only_printf(LC_TMTracer,
                      "Purging fragments for JSScript %p.\n", (void*)script);

    
    JS_ASSERT_IF(tm->recorder, 
                 JS_UPTRDIFF(tm->recorder->getTree()->ip, script->code) >= script->length);

    for (LoopProfileMap::Enum e(*tm->loopProfiles); !e.empty(); e.popFront()) {
        if (JS_UPTRDIFF(e.front().key, script->code) < script->length)
            e.removeFront();
    }

    TracedScriptSet::Ptr found = tm->tracedScripts.lookup(script);
    if (!found)
        return;
    tm->tracedScripts.remove(found);

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
                    TrashTree(frag);
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
OverfullJITCache(JSContext *cx, TraceMonitor* tm)
{
    

































    jsuint maxsz = JS_THREAD_DATA(cx)->maxCodeCacheBytes;
    return (tm->codeAlloc->size() + tm->dataAlloc->size() + tm->traceAlloc->size() > maxsz);
}

JS_FORCES_STACK JS_FRIEND_API(void)
DeepBail(JSContext *cx)
{
    JS_ASSERT(JS_ON_TRACE(cx));

    



    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    JS_ASSERT(JS_THREAD_DATA(cx)->profilingCompartment == NULL);
    JS_THREAD_DATA(cx)->onTraceCompartment = NULL;

    
    JS_ASSERT(tm->bailExit);

    tm->tracecx = NULL;
    debug_only_print0(LC_TMTracer, "Deep bail.\n");
    LeaveTree(tm, *tm->tracerState, tm->bailExit);
    tm->bailExit = NULL;

    TracerState* state = tm->tracerState;
    state->builtinStatus |= BUILTIN_BAILED;

    























    state->deepBailSp = state->sp;
}

JS_REQUIRES_STACK Value&
TraceRecorder::argval(unsigned n) const
{
    JS_ASSERT(n < cx->fp()->numFormalArgs());
    return cx->fp()->formalArg(n);
}

JS_REQUIRES_STACK Value&
TraceRecorder::varval(unsigned n) const
{
    JS_ASSERT(n < cx->fp()->numSlots());
    return cx->fp()->slots()[n];
}

JS_REQUIRES_STACK Value&
TraceRecorder::stackval(int n) const
{
    return cx->regs().sp[n];
}

JS_REQUIRES_STACK void
TraceRecorder::updateAtoms()
{
    JSScript *script = cx->fp()->script();
    atoms = FrameAtomBase(cx, cx->fp());
    consts = (cx->fp()->hasImacropc() || !JSScript::isValidOffset(script->constOffset))
             ? 0
             : script->consts()->vector;
    strictModeCode_ins = w.name(w.immi(script->strictModeCode), "strict");
}

JS_REQUIRES_STACK void
TraceRecorder::updateAtoms(JSScript *script)
{
    atoms = script->atomMap.vector;
    consts = JSScript::isValidOffset(script->constOffset) ? script->consts()->vector : 0;
    strictModeCode_ins = w.name(w.immi(script->strictModeCode), "strict");
}




JS_REQUIRES_STACK LIns*
TraceRecorder::scopeChain()
{
    return cx->fp()->isFunctionFrame()
           ? getFrameObjPtr(cx->fp()->addressOfScopeChain())
           : entryScopeChain();
}






JS_REQUIRES_STACK LIns*
TraceRecorder::entryScopeChain() const
{
    return w.ldpStackFrameScopeChain(entryFrameIns());
}




JS_REQUIRES_STACK LIns*
TraceRecorder::entryFrameIns() const
{
    return w.ldpFrameFp(w.ldpContextRegs(cx_ins));
}






JS_REQUIRES_STACK StackFrame*
TraceRecorder::frameIfInRange(JSObject* obj, unsigned* depthp) const
{
    StackFrame* ofp = (StackFrame*) obj->getPrivate();
    StackFrame* fp = cx->fp();
    for (unsigned depth = 0; depth <= callDepth; ++depth) {
        if (fp == ofp) {
            if (depthp)
                *depthp = depth;
            return ofp;
        }
        if (!(fp = fp->prev()))
            break;
    }
    return NULL;
}

JS_DEFINE_CALLINFO_4(extern, UINT32, GetClosureVar, CONTEXT, OBJECT, CVIPTR, DOUBLEPTR,
                     0, ACCSET_STORE_ANY)
JS_DEFINE_CALLINFO_4(extern, UINT32, GetClosureArg, CONTEXT, OBJECT, CVIPTR, DOUBLEPTR,
                     0, ACCSET_STORE_ANY)










JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::scopeChainProp(JSObject* chainHead, Value*& vp, LIns*& ins, NameResult& nr,
                              JSObject** scopeObjp)
{
    JS_ASSERT(chainHead == &cx->fp()->scopeChain());
    JS_ASSERT(chainHead != globalObj);

    TraceMonitor &localtm = *traceMonitor;

    JSAtom* atom = atoms[GET_INDEX(cx->regs().pc)];
    JSObject* obj2;
    JSProperty* prop;
    JSObject *obj = chainHead;
    if (!js_FindProperty(cx, ATOM_TO_JSID(atom), &obj, &obj2, &prop))
        RETURN_ERROR_A("error in js_FindProperty");

    
    if (!localtm.recorder)
        return ARECORD_ABORTED;

    if (!prop)
        RETURN_STOP_A("failed to find name in non-global scope chain");

    if (scopeObjp)
        *scopeObjp = obj;

    if (obj == globalObj) {
        
        
        
        LIns *head_ins;
        if (cx->fp()->isFunctionFrame()) {
            
            
            
            chainHead = cx->fp()->callee().getParent();
            head_ins = w.ldpObjParent(get(&cx->fp()->calleev()));
        } else {
            head_ins = scopeChain();
        }
        LIns *obj_ins;
        CHECK_STATUS_A(traverseScopeChain(chainHead, head_ins, obj, obj_ins));

        if (obj2 != obj)
            RETURN_STOP_A("prototype property");

        Shape* shape = (Shape*) prop;
        if (!isValidSlot(obj, shape))
            return ARECORD_STOP;
        if (!lazilyImportGlobalSlot(shape->slot))
            RETURN_STOP_A("lazy import of global slot failed");
        vp = &obj->getSlotRef(shape->slot);
        ins = get(vp);
        nr.tracked = true;
        return ARECORD_CONTINUE;
    }

    if (obj == obj2 && obj->isCall()) {
        AbortableRecordingStatus status =
            InjectStatus(callProp(obj, prop, ATOM_TO_JSID(atom), vp, ins, nr));
        return status;
    }

    RETURN_STOP_A("fp->scopeChain is not global or active call object");
}




JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callProp(JSObject* obj, JSProperty* prop, jsid id, Value*& vp,
                        LIns*& ins, NameResult& nr)
{
    Shape *shape = (Shape*) prop;

    JSOp op = JSOp(*cx->regs().pc);
    uint32 setflags = (js_CodeSpec[op].format & (JOF_SET | JOF_INCDEC | JOF_FOR));
    if (setflags && !shape->writable())
        RETURN_STOP("writing to a read-only property");

    uintN slot = uint16(shape->shortid);

    vp = NULL;
    StackFrame* cfp = (StackFrame*) obj->getPrivate();
    if (cfp) {
        if (shape->getterOp() == GetCallArg) {
            JS_ASSERT(slot < cfp->numFormalArgs());
            vp = &cfp->formalArg(slot);
            nr.v = *vp;
        } else if (shape->getterOp() == GetCallVar ||
                   shape->getterOp() == GetCallVarChecked) {
            JS_ASSERT(slot < cfp->numSlots());
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
        
        
        
        DebugOnly<JSBool> rv =
            js_GetPropertyHelper(cx, obj, shape->propid,
                                 (op == JSOP_CALLNAME)
                                 ? JSGET_NO_METHOD_BARRIER
                                 : JSGET_METHOD_BARRIER,
                                 &nr.v);
        JS_ASSERT(rv);
    }

    LIns* obj_ins;
    JSObject* parent = cx->fp()->callee().getParent();
    LIns* parent_ins = w.ldpObjParent(get(&cx->fp()->calleev()));
    CHECK_STATUS(traverseScopeChain(parent, parent_ins, obj, obj_ins));

    if (!cfp) {
        
        
        
        
        
        if (shape->getterOp() == GetCallArg) {
            JS_ASSERT(slot < ArgClosureTraits::slot_count(obj));
            slot += ArgClosureTraits::slot_offset(obj);
        } else if (shape->getterOp() == GetCallVar ||
                   shape->getterOp() == GetCallVarChecked) {
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

        
        
        
        guard(false,
              w.eqp(entryFrameIns(), w.ldpObjPrivate(obj_ins)),
              MISMATCH_EXIT);

        LIns* outp = w.allocp(sizeof(double));
        LIns* args[] = {
            outp,
            w.nameImmpNonGC(cv),
            obj_ins,
            cx_ins
        };
        const CallInfo* ci;
        if (shape->getterOp() == GetCallArg) {
            ci = &GetClosureArg_ci;
        } else if (shape->getterOp() == GetCallVar ||
                   shape->getterOp() == GetCallVarChecked) {
            ci = &GetClosureVar_ci;
        } else {
            RETURN_STOP("dynamic property of Call object");
        }

        
        JS_ASSERT(shape->hasShortID());

        LIns* call_ins = w.call(ci, args);

        JSValueType type = getCoercedType(nr.v);
        guard(true,
              w.name(w.eqi(call_ins, w.immi(type)), "guard(type-stable name access)"),
              BRANCH_EXIT);
        ins = stackLoad(AllocSlotsAddress(outp), type);
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


JS_REQUIRES_STACK void
TraceRecorder::guardNonNeg(LIns* d0, LIns* d1, VMSideExit* exit)
{
    if (d0->isImmI())
        JS_ASSERT(d0->immI() >= 0);
    else
        guard(false, w.ltiN(d0, 0), exit);

    if (d1->isImmI())
        JS_ASSERT(d1->immI() >= 0);
    else
        guard(false, w.ltiN(d1, 0), exit);
}

JS_REQUIRES_STACK LIns*
TraceRecorder::tryToDemote(LOpcode op, jsdouble v0, jsdouble v1, LIns* s0, LIns* s1)
{
    













    if (!oracle || oracle->isInstructionUndemotable(cx->regs().pc) ||
        !IsPromotedInt32(s0) || !IsPromotedInt32(s1))
    {
      undemotable:
        if (op == LIR_modd) {
            



            LIns* args[] = { s1, s0 };
            return w.call(&js_dmod_ci, args);
        }
        LIns* result = w.ins2(op, s0, s1);
        JS_ASSERT_IF(s0->isImmD() && s1->isImmD(), result->isImmD());
        return result;
    }

    LIns* d0 = w.demoteToInt32(s0);
    LIns* d1 = w.demoteToInt32(s1);
    jsdouble r = 0;     
    VMSideExit* exit = NULL;
    LIns* result;

    switch (op) {
      case LIR_addd: {
        r = v0 + v1;
        if (jsint(r) != r || JSDOUBLE_IS_NEGZERO(r))
            goto undemotable;

        Interval i0 = Interval::of(d0, 3);
        Interval i1 = Interval::of(d1, 3);
        result = Interval::add(i0, i1).hasOverflowed
               ? w.addxovi(d0, d1, createGuardRecord(snapshot(OVERFLOW_EXIT)))
               : w.addi(d0, d1);
        break;
      }

      case LIR_subd: {
        r = v0 - v1;
        if (jsint(r) != r || JSDOUBLE_IS_NEGZERO(r))
            goto undemotable;

        Interval i0 = Interval::of(d0, 3);
        Interval i1 = Interval::of(d1, 3);
        result = Interval::sub(i0, i1).hasOverflowed
               ? w.subxovi(d0, d1, createGuardRecord(snapshot(OVERFLOW_EXIT)))
               : w.subi(d0, d1);
        break;
      }

      case LIR_muld: {
        r = v0 * v1;
        if (r == 0.0 && (v0 < 0.0 || v1 < 0.0))
            goto undemotable;

        if (jsint(r) != r || JSDOUBLE_IS_NEGZERO(r))
            goto undemotable;

        Interval i0 = Interval::of(d0, 3);
        Interval i1 = Interval::of(d1, 3);
        if (Interval::mul(i0, i1).hasOverflowed) {
            exit = snapshot(OVERFLOW_EXIT);
            result = w.mulxovi(d0, d1, createGuardRecord(exit));
        } else {
            result = w.muli(d0, d1);
        }

        









        bool needsNegZeroCheck = (i0.canBeZero() && i1.canBeNegative()) ||
                                 (i1.canBeZero() && i0.canBeNegative());
        if (needsNegZeroCheck) {
            





            if (v0 < 0.0 || v1 < 0.0 || oracle->isInstructionSlowZeroTest(cx->regs().pc)) {
                if (!exit)
                    exit = snapshot(OVERFLOW_EXIT);

                guard(true,
                      w.eqi0(w.andi(w.eqi0(result),
                                    w.ori(w.ltiN(d0, 0),
                                          w.ltiN(d1, 0)))),
                      exit);
            } else {
                guardNonNeg(d0, d1, snapshot(MUL_ZERO_EXIT));
            }
        }
        break;
      }

      case LIR_divd: {
#if defined NANOJIT_IA32 || defined NANOJIT_X64
        if (v1 == 0)
            goto undemotable;
        r = v0 / v1;
        if (jsint(r) != r || JSDOUBLE_IS_NEGZERO(r))
            goto undemotable;

        
        if (d0->isImmI() && d1->isImmI())
            return w.i2d(w.immi(jsint(r)));

        exit = snapshot(OVERFLOW_EXIT);

        




        if (!d1->isImmI()) {
            if (MaybeBranch mbr = w.jt(w.gtiN(d1, 0))) {
                guard(false, w.eqi0(d1), exit);
                guard(true,  w.eqi0(w.andi(w.eqiN(d0, 0x80000000),
                                           w.eqiN(d1, -1))), exit);
                w.label(mbr);
            }
        } else if (d1->immI() == -1) {
            guard(false, w.eqiN(d0, 0x80000000), exit);
        }
        result = w.divi(d0, d1);

        
        guard(true, w.eqi0(w.modi(result)), exit);

        
        guard(false, w.eqi0(result), exit);

        break;
#else
        goto undemotable;
#endif
      }

      case LIR_modd: {
#if defined NANOJIT_IA32 || defined NANOJIT_X64
        if (v0 < 0 || v1 == 0 || (s1->isImmD() && v1 < 0))
            goto undemotable;
        r = js_dmod(v0, v1);
        if (jsint(r) != r || JSDOUBLE_IS_NEGZERO(r))
            goto undemotable;

        
        if (d0->isImmI() && d1->isImmI())
            return w.i2d(w.immi(jsint(r)));

        exit = snapshot(OVERFLOW_EXIT);

        
        if (!d1->isImmI())
            guard(false, w.eqi0(d1), exit);
        result = w.modi(w.divi(d0, d1));

        




        if (MaybeBranch mbr = w.jf(w.eqi0(result))) {
            guard(false, w.ltiN(d0, 0), exit);
            w.label(mbr);
        }
        break;
#else
        goto undemotable;
#endif
      }

      default:
        JS_NOT_REACHED("tryToDemote");
        result = NULL;
        break;
    }

    



    JS_ASSERT_IF(d0->isImmI() && d1->isImmI(), result->isImmI(jsint(r)));
    return w.i2d(result);
}

LIns*
TraceRecorder::d2i(LIns* d, bool resultCanBeImpreciseIfFractional)
{
    if (d->isImmD())
        return w.immi(js_DoubleToECMAInt32(d->immD()));
    if (d->isop(LIR_i2d) || d->isop(LIR_ui2d)) {
        
        
        
        
        
        
        
        
        
        return d->oprnd1();
    }
    if (d->isop(LIR_addd) || d->isop(LIR_subd)) {
        
        
        
        
        
        
        
        
        
        LIns* lhs = d->oprnd1();
        LIns* rhs = d->oprnd2();
        if (IsPromotedInt32(lhs) && IsPromotedInt32(rhs))
            return w.ins2(arithOpcodeD2I(d->opcode()), w.demoteToInt32(lhs), w.demoteToInt32(rhs));
    }
    if (d->isCall()) {
        const CallInfo* ci = d->callInfo();
        if (ci == &js_UnboxDouble_ci) {
#if JS_BITS_PER_WORD == 32
            LIns *tag_ins = d->callArgN(0);
            LIns *payload_ins = d->callArgN(1);
            LIns* args[] = { payload_ins, tag_ins };
            return w.call(&js_UnboxInt32_ci, args);
#else
            LIns* val_ins = d->callArgN(0);
            LIns* args[] = { val_ins };
            return w.call(&js_UnboxInt32_ci, args);
#endif
        }
        if (ci == &js_StringToNumber_ci) {
            LIns* ok_ins = w.allocp(sizeof(JSBool));
            LIns* args[] = { ok_ins, d->callArgN(1), d->callArgN(0) };
            LIns* ret_ins = w.call(&js_StringToInt32_ci, args);
            guard(false,
                  w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
                  OOM_EXIT);
            return ret_ins;
        }
    }
    return resultCanBeImpreciseIfFractional
         ? w.rawD2i(d)
         : w.call(&js_DoubleToInt32_ci, &d);
}

LIns*
TraceRecorder::d2u(LIns* d)
{
    if (d->isImmD())
        return w.immi(js_DoubleToECMAUint32(d->immD()));
    if (d->isop(LIR_i2d) || d->isop(LIR_ui2d))
        return d->oprnd1();
    return w.call(&js_DoubleToUint32_ci, &d);
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::makeNumberInt32(LIns* d, LIns** out)
{
    JS_ASSERT(d->isD());
    if (IsPromotedInt32(d)) {
        *out = w.demoteToInt32(d);
        return RECORD_CONTINUE;
    }

    
    
    
    
    *out = d2i(d, true);
    return guard(true, w.eqd(d, w.i2d(*out)), MISMATCH_EXIT, true);
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::makeNumberUint32(LIns* d, LIns** out)
{
    JS_ASSERT(d->isD());
    if (IsPromotedUint32(d)) {
        *out = w.demoteToUint32(d);
        return RECORD_CONTINUE;
    }

    
    
    
    
    *out = d2u(d);
    return guard(true, w.eqd(d, w.ui2d(*out)), MISMATCH_EXIT, true);
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
        return w.immpAtomGC(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
    } else if (v.isBoolean()) {
        ci = &js_BooleanIntToString_ci;
    } else {
        




        JS_ASSERT(v.isNull());
        return w.immpAtomGC(cx->runtime->atomState.nullAtom);
    }

    v_ins = w.call(ci, args);
    guard(false, w.eqp0(v_ins), OOM_EXIT);
    return v_ins;
}

JS_REQUIRES_STACK bool
TraceRecorder::canCallImacro() const
{
    
    return !cx->fp()->hasImacropc();
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callImacro(jsbytecode* imacro)
{
    return canCallImacro() ? callImacroInfallibly(imacro) : RECORD_STOP;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callImacroInfallibly(jsbytecode* imacro)
{
    StackFrame* fp = cx->fp();
    JS_ASSERT(!fp->hasImacropc());
    FrameRegs& regs = cx->regs();
    fp->setImacropc(regs.pc);
    regs.pc = imacro;
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
        x = w.immi(0);
    } else if (!v.isPrimitive()) {
        cond = true;
        x = w.immi(1);
    } else if (v.isBoolean()) {
        
        cond = v.isTrue();
        x = w.eqiN(v_ins, 1);
    } else if (v.isNumber()) {
        jsdouble d = v.toNumber();
        cond = !JSDOUBLE_IS_NaN(d) && d;
        x = w.eqi0(w.eqi0(w.andi(w.eqd(v_ins, v_ins), w.eqi0(w.eqd0(v_ins)))));
    } else if (v.isString()) {
        cond = v.toString()->length() != 0;
        x = w.eqi0(w.eqp0(w.getStringLength(v_ins)));
    } else {
        JS_NOT_REACHED("ifop");
        return ARECORD_STOP;
    }

    jsbytecode* pc = cx->regs().pc;
    emitIf(pc, cond, x);
    return checkTraceEnd(pc);
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::switchop()
{
    Value& v = stackval(-1);
    LIns* v_ins = get(&v);

    
    if (v_ins->isImmAny())
        return RECORD_CONTINUE;
    if (v.isNumber()) {
        jsdouble d = v.toNumber();
        CHECK_STATUS(guard(true,
                     w.name(w.eqd(v_ins, w.immd(d)), "guard(switch on numeric)"),
                     BRANCH_EXIT,
                     true));
    } else if (v.isString()) {
        LIns* args[] = { w.immpStrGC(v.toString()), v_ins, cx_ins };
        LIns* equal_rval = w.call(&js_EqualStringsOnTrace_ci, args);
        guard(false,
              w.name(w.eqiN(equal_rval, JS_NEITHER), "guard(oom)"),
              OOM_EXIT);
        guard(false,
              w.name(w.eqi0(equal_rval), "guard(switch on string)"),
              BRANCH_EXIT);
    } else if (v.isBoolean()) {
        guard(true,
              w.name(w.eqi(v_ins, w.immi(v.isTrue())), "guard(switch on boolean)"),
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
    Value dummy;
    CHECK_STATUS(inc(v, v_ins, dummy, incr, pre));
    set(&v, v_ins);
    return RECORD_CONTINUE;
}






JS_REQUIRES_STACK RecordingStatus
TraceRecorder::inc(const Value &v, LIns*& v_ins, Value &v_out, jsint incr, bool pre)
{
    LIns* v_after;
    CHECK_STATUS(incHelper(v, v_ins, v_out, v_after, incr));

    const JSCodeSpec& cs = js_CodeSpec[*cx->regs().pc];
    JS_ASSERT(cs.ndefs == 1);
    stack(-cs.nuses, pre ? v_after : v_ins);
    v_ins = v_after;
    return RECORD_CONTINUE;
}







JS_REQUIRES_STACK RecordingStatus
TraceRecorder::incHelper(const Value &v, LIns*& v_ins, Value &v_after,
                         LIns*& v_ins_after, jsint incr)
{
    
    if (!v.isPrimitive())
        RETURN_STOP("can inc primitives only");

    
    
    if (v.isUndefined()) {
        v_ins_after = w.immd(js_NaN);
        v_after.setDouble(js_NaN);
        v_ins = w.immd(js_NaN);
    } else if (v.isNull()) {
        v_ins_after = w.immd(incr);
        v_after.setDouble(incr);
        v_ins = w.immd(0.0);
    } else {
        if (v.isBoolean()) {
            v_ins = w.i2d(v_ins);
        } else if (v.isString()) {
            LIns* ok_ins = w.allocp(sizeof(JSBool));
            LIns* args[] = { ok_ins, v_ins, cx_ins };
            v_ins = w.call(&js_StringToNumber_ci, args);
            guard(false,
                  w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
                  OOM_EXIT);
        } else {
            JS_ASSERT(v.isNumber());
        }

        jsdouble num;
        AutoValueRooter tvr(cx);
        *tvr.addr() = v;
        ValueToNumber(cx, tvr.value(), &num);
        v_ins_after = tryToDemote(LIR_addd, num, incr, v_ins, w.immd(incr));
        v_after.setDouble(num + incr);
    }

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
    Value v_after;
    CHECK_STATUS_A(inc(v, v_ins, v_after, incr, pre));

    LIns* slots_ins = NULL;
    stobj_set_slot(obj, obj_ins, slot, slots_ins, v_after, v_ins);
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
        CHECK_STATUS(denseArrayElement(l, r, vp, v_ins, addr_ins, snapshot(BRANCH_EXIT)));
        if (!addr_ins) 
            return RECORD_STOP;
        Value v_after;
        CHECK_STATUS(inc(*vp, v_ins, v_after, incr, pre));
        box_value_into(v_after, v_ins, DSlotsAddress(addr_ins));
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
EvalCmp(JSContext *cx, LOpcode op, JSString* l, JSString* r, JSBool *ret)
{
    if (op == LIR_eqd)
        return EqualStrings(cx, l, r, ret);
    JSBool cmp;
    if (!CompareStrings(cx, l, r, &cmp))
        return false;
    *ret = EvalCmp(op, cmp, 0);
    return true;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::strictEquality(bool equal, bool cmpCase)
{
    Value& r = stackval(-1);
    Value& l = stackval(-2);
    LIns* l_ins = get(&l);
    LIns* r_ins = get(&r);
    LIns* x;
    JSBool cond;

    JSValueType ltag = getPromotedType(l);
    if (ltag != getPromotedType(r)) {
        cond = !equal;
        x = w.immi(cond);
    } else if (ltag == JSVAL_TYPE_STRING) {
        LIns* args[] = { r_ins, l_ins, cx_ins };
        LIns* equal_ins = w.call(&js_EqualStringsOnTrace_ci, args);
        guard(false,
              w.name(w.eqiN(equal_ins, JS_NEITHER), "guard(oom)"),
              OOM_EXIT);
        x = w.eqiN(equal_ins, equal);
        if (!EqualStrings(cx, l.toString(), r.toString(), &cond))
            RETURN_ERROR("oom");
    } else {
        if (ltag == JSVAL_TYPE_DOUBLE)
            x = w.eqd(l_ins, r_ins);
        else if (ltag == JSVAL_TYPE_NULL || ltag == JSVAL_TYPE_NONFUNOBJ || ltag == JSVAL_TYPE_FUNOBJ)
            x = w.eqp(l_ins, r_ins);
        else
            x = w.eqi(l_ins, r_ins);
        if (!equal)
            x = w.eqi0(x);
        cond = (ltag == JSVAL_TYPE_DOUBLE)
               ? l.toNumber() == r.toNumber()
               : l == r;
    }
    cond = (!!cond == equal);

    if (cmpCase) {
        
        if (!x->isImmI())
            guard(cond, x, BRANCH_EXIT);
        return RECORD_CONTINUE;
    }

    set(&l, x);
    return RECORD_CONTINUE;
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
    JSBool cond;
    LIns* args[] = { NULL, NULL, NULL };

    










    if (getPromotedType(l) == getPromotedType(r)) {
        if (l.isUndefined() || l.isNull()) {
            cond = true;
            if (l.isNull())
                op = LIR_eqp;
        } else if (l.isObject()) {
            if (l.toObject().getClass()->ext.equality)
                RETURN_STOP_A("Can't trace extended class equality operator");
            LIns* flags_ins = w.ldiObjFlags(l_ins);
            LIns* flag_ins = w.andi(flags_ins, w.nameImmui(JSObject::HAS_EQUALITY));
            guard(true, w.eqi0(flag_ins), BRANCH_EXIT);

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
                LIns *c = w.immw(1);
                guard(true, w.eqp(w.getStringLength(l_ins), c), exit);
                guard(true, w.eqp(w.getStringLength(r_ins), c), exit);
                l_ins = w.getStringChar(l_ins, w.immpNonGC(0));
                r_ins = w.getStringChar(r_ins, w.immpNonGC(0));
            } else {
                args[0] = r_ins, args[1] = l_ins, args[2] = cx_ins;
                LIns *equal_ins = w.call(&js_EqualStringsOnTrace_ci, args);
                guard(false,
                      w.name(w.eqiN(equal_ins, JS_NEITHER), "guard(oom)"),
                      OOM_EXIT);
                l_ins = equal_ins;
                r_ins = w.immi(1);
            }
            if (!EqualStrings(cx, l.toString(), r.toString(), &cond))
                RETURN_ERROR_A("oom");
        } else {
            JS_ASSERT(l.isNumber() && r.isNumber());
            cond = (l.toNumber() == r.toNumber());
            op = LIR_eqd;
        }
    } else if (l.isNull() && r.isUndefined()) {
        l_ins = w.immiUndefined();
        cond = true;
    } else if (l.isUndefined() && r.isNull()) {
        r_ins = w.immiUndefined();
        cond = true;
    } else if (l.isNumber() && r.isString()) {
        LIns* ok_ins = w.allocp(sizeof(JSBool));
        args[0] = ok_ins, args[1] = r_ins, args[2] = cx_ins;
        r_ins = w.call(&js_StringToNumber_ci, args);
        guard(false,
              w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
              OOM_EXIT);
        JSBool ok;
        double d = js_StringToNumber(cx, r.toString(), &ok);
        if (!ok)
            RETURN_ERROR_A("oom");
        cond = (l.toNumber() == d);
        op = LIR_eqd;
    } else if (l.isString() && r.isNumber()) {
        LIns* ok_ins = w.allocp(sizeof(JSBool));
        args[0] = ok_ins, args[1] = l_ins, args[2] = cx_ins;
        l_ins = w.call(&js_StringToNumber_ci, args);
        guard(false,
              w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
              OOM_EXIT);
        JSBool ok;
        double d = js_StringToNumber(cx, l.toString(), &ok);
        if (!ok)
            RETURN_ERROR_A("oom");
        cond = (d == r.toNumber());
        op = LIR_eqd;
    } else {
        
        
        if (l.isBoolean()) {
            l_ins = w.i2d(l_ins);
            set(&l, l_ins);
            l.setInt32(l.isTrue());
            return equalityHelper(l, r, l_ins, r_ins, negate,
                                  tryBranchAfterCond, rval);
        }
        if (r.isBoolean()) {
            r_ins = w.i2d(r_ins);
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

        l_ins = w.immi(0);
        r_ins = w.immi(1);
        cond = false;
    }

    
    LIns* x = w.ins2(op, l_ins, r_ins);
    if (negate) {
        x = w.eqi0(x);
        cond = !cond;
    }

    jsbytecode* pc = cx->regs().pc;

    




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
    JSBool cond;
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
        LIns* args[] = { r_ins, l_ins, cx_ins };
        LIns* result_ins = w.call(&js_CompareStringsOnTrace_ci, args);
        guard(false,
              w.name(w.eqiN(result_ins, INT32_MIN), "guard(oom)"),
              OOM_EXIT);
        l_ins = result_ins;
        r_ins = w.immi(0);
        if (!EvalCmp(cx, op, l.toString(), r.toString(), &cond))
            RETURN_ERROR_A("oom");
        goto do_comparison;
    }

    
    if (!l.isNumber()) {
        if (l.isBoolean()) {
            l_ins = w.i2d(l_ins);
        } else if (l.isUndefined()) {
            l_ins = w.immd(js_NaN);
        } else if (l.isString()) {
            LIns* ok_ins = w.allocp(sizeof(JSBool));
            LIns* args[] = { ok_ins, l_ins, cx_ins };
            l_ins = w.call(&js_StringToNumber_ci, args);
            guard(false,
                  w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
                  OOM_EXIT);
        } else if (l.isNull()) {
            l_ins = w.immd(0.0);
        } else {
            JS_NOT_REACHED("JSVAL_IS_NUMBER if int/double, objects should "
                           "have been handled at start of method");
            RETURN_STOP_A("safety belt");
        }
    }
    if (!r.isNumber()) {
        if (r.isBoolean()) {
            r_ins = w.i2d(r_ins);
        } else if (r.isUndefined()) {
            r_ins = w.immd(js_NaN);
        } else if (r.isString()) {
            LIns* ok_ins = w.allocp(sizeof(JSBool));
            LIns* args[] = { ok_ins, r_ins, cx_ins };
            r_ins = w.call(&js_StringToNumber_ci, args);
            guard(false,
                  w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
                  OOM_EXIT);
        } else if (r.isNull()) {
            r_ins = w.immd(0.0);
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
    x = w.ins2(op, l_ins, r_ins);

    jsbytecode* pc = cx->regs().pc;

    




    if (tryBranchAfterCond)
        fuseIf(pc + 1, cond, x);

    



    if (pc[1] == JSOP_IFNE || pc[1] == JSOP_IFEQ)
        CHECK_STATUS_A(checkTraceEnd(pc + 1));

    





    set(&l, x);

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::unaryIntOp(LOpcode op)
{
    Value& v = stackval(-1);
    JS_ASSERT(retTypes[op] == LTy_I);
    if (v.isNumber()) {
        LIns* a = get(&v);
        a = w.i2d(w.ins1(op, d2i(a)));
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
        LIns* ok_ins = w.allocp(sizeof(JSBool));
        LIns* args[] = { ok_ins, a, cx_ins };
        a = w.call(&js_StringToNumber_ci, args);
        guard(false,
              w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
              OOM_EXIT);
        JSBool ok;
        lnum = js_StringToNumber(cx, l.toString(), &ok);
        if (!ok)
            RETURN_ERROR("oom");
        leftIsNumber = true;
    }
    if (r.isString()) {
        NanoAssert(op != LIR_addd); 
        LIns* ok_ins = w.allocp(sizeof(JSBool));
        LIns* args[] = { ok_ins, b, cx_ins };
        b = w.call(&js_StringToNumber_ci, args);
        guard(false,
              w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
              OOM_EXIT);
        JSBool ok;
        rnum = js_StringToNumber(cx, r.toString(), &ok);
        if (!ok)
            RETURN_ERROR("oom");
        rightIsNumber = true;
    }
    if (l.isBoolean()) {
        a = w.i2d(a);
        lnum = l.toBoolean();
        leftIsNumber = true;
    } else if (l.isUndefined()) {
        a = w.immd(js_NaN);
        lnum = js_NaN;
        leftIsNumber = true;
    }
    if (r.isBoolean()) {
        b = w.i2d(b);
        rnum = r.toBoolean();
        rightIsNumber = true;
    } else if (r.isUndefined()) {
        b = w.immd(js_NaN);
        rnum = js_NaN;
        rightIsNumber = true;
    }
    if (leftIsNumber && rightIsNumber) {
        if (intop) {
            a = (op == LIR_rshui)
              ? w.ui2d(w.ins2(op, d2u(a), d2i(b)))
              : w.i2d(w.ins2(op, d2i(a), d2i(b)));
        } else {
            a = tryToDemote(op, lnum, rnum, a, b);
        }
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
            putc(' ', shapefp);
            JS_PutString(JSID_TO_STRING(shape.id), shapefp);
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
              w.name(w.eqp(obj_ins, w.immpObjGC(globalObj)), "guard_global"),
              exit);
        return RECORD_CONTINUE;
    }

#if defined DEBUG_notme && defined XP_UNIX
    DumpShape(obj, "guard");
    fprintf(shapefp, "for obj_ins %p\n", obj_ins);
#endif

    
    guard(true, w.name(w.eqiN(w.ldiObjShape(obj_ins), shape), guardName), exit);
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

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::test_property_cache(JSObject* obj, LIns* obj_ins, JSObject*& obj2, PCVal& pcval)
{
    jsbytecode* pc = cx->regs().pc;
    JS_ASSERT(*pc != JSOP_INITPROP && *pc != JSOP_INITMETHOD &&
              *pc != JSOP_SETNAME && *pc != JSOP_SETPROP && *pc != JSOP_SETMETHOD);

    
    
    
    JSObject* aobj = obj;
    if (obj->isDenseArray()) {
        guardDenseArray(obj_ins, BRANCH_EXIT);
        aobj = obj->getProto();
        obj_ins = w.ldpObjProto(obj_ins);
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
            int protoIndex = js_LookupPropertyWithFlags(cx, aobj, id,
                                                        cx->resolveFlags,
                                                        &obj2, &prop);

            if (protoIndex < 0)
                RETURN_ERROR_A("error in js_LookupPropertyWithFlags");

            
            if (!localtm.recorder)
                return ARECORD_ABORTED;

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

        if (!entry)
            RETURN_STOP_A("failed to fill property cache");
    }

#ifdef JS_THREADSAFE
    
    
    
    
    JS_ASSERT(cx->thread()->data.requestDepth);
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

        JSOp op = js_GetOpcode(cx, cx->fp()->script(), cx->regs().pc);
        if (JOF_OPMODE(op) != JOF_NAME) {
            guard(true,
                  w.name(w.eqp(obj_ins, w.immpObjGC(globalObj)), "guard_global"),
                  exit);
        }
    } else {
        CHECK_STATUS(guardShape(obj_ins, aobj, entry->kshape, "guard_kshape", exit));
    }

    if (entry->adding()) {
        LIns *vshape_ins =
            w.ldiRuntimeProtoHazardShape(w.ldpConstContextField(runtime));

        guard(true,
              w.name(w.eqiN(vshape_ins, vshape), "guard_protoHazardShape"),
              BRANCH_EXIT);
    }

    
    
    if (entry->vcapTag() >= 1) {
        JS_ASSERT(obj2->shape() == vshape);
        if (obj2 == globalObj)
            RETURN_STOP("hitting the global object via a prototype chain");

        LIns* obj2_ins;
        if (entry->vcapTag() == 1) {
            
            obj2_ins = w.ldpObjProto(obj_ins);
            guard(false, w.eqp0(obj2_ins), exit);
        } else {
            obj2_ins = w.immpObjGC(obj2);
        }
        CHECK_STATUS(guardShape(obj2_ins, obj2, vshape, "guard_vshape", exit));
    }

    pcval = entry->vword;
    return RECORD_CONTINUE;
}

void
TraceRecorder::stobj_set_fslot(LIns *obj_ins, unsigned slot, const Value &v, LIns* v_ins)
{
    box_value_into(v, v_ins, FSlotsAddress(obj_ins, slot));
}

void
TraceRecorder::stobj_set_dslot(LIns *obj_ins, unsigned slot, LIns*& slots_ins, 
                               const Value &v, LIns* v_ins)
{
    if (!slots_ins)
        slots_ins = w.ldpObjSlots(obj_ins);
    box_value_into(v, v_ins, DSlotsAddress(slots_ins, slot));
}

void
TraceRecorder::stobj_set_slot(JSObject *obj, LIns* obj_ins, unsigned slot, LIns*& slots_ins,
                              const Value &v, LIns* v_ins)
{
    



    if (!obj->hasSlotsArray()) {
        JS_ASSERT(slot < obj->numSlots());
        stobj_set_fslot(obj_ins, slot, v, v_ins);
    } else {
        stobj_set_dslot(obj_ins, slot, slots_ins, v, v_ins);
    }
}

LIns*
TraceRecorder::unbox_slot(JSObject *obj, LIns *obj_ins, uint32 slot, VMSideExit *exit)
{
    
    Address addr = (!obj->hasSlotsArray())
                 ? (Address)FSlotsAddress(obj_ins, slot)
                 : (Address)DSlotsAddress(w.ldpObjSlots(obj_ins), slot);

    return unbox_value(obj->getSlot(slot), addr, exit);
}

#if JS_BITS_PER_WORD == 32

void
TraceRecorder::box_undefined_into(Address addr)
{
    w.stiValueTag(w.nameImmui(JSVAL_TAG_UNDEFINED), addr);
    w.stiValuePayload(w.immi(0), addr);
}

void
TraceRecorder::box_null_into(Address addr)
{
    w.stiValueTag(w.nameImmui(JSVAL_TAG_NULL), addr);
    w.stiValuePayload(w.immi(0), addr);
}

inline LIns*
TraceRecorder::unbox_number_as_double(Address addr, LIns *tag_ins, VMSideExit *exit)
{
    guard(true, w.leui(tag_ins, w.nameImmui(JSVAL_UPPER_INCL_TAG_OF_NUMBER_SET)), exit);
    LIns *val_ins = w.ldiValuePayload(addr);
    LIns* args[] = { val_ins, tag_ins };
    return w.call(&js_UnboxDouble_ci, args);
}

inline LIns*
TraceRecorder::unbox_non_double_object(Address addr, LIns* tag_ins,
                                       JSValueType type, VMSideExit* exit)
{
    LIns *val_ins;
    if (type == JSVAL_TYPE_UNDEFINED) {
        val_ins = w.immiUndefined();
    } else if (type == JSVAL_TYPE_NULL) {
        val_ins = w.immpNull();
    } else {
        JS_ASSERT(type == JSVAL_TYPE_INT32 || type == JSVAL_TYPE_OBJECT ||
                  type == JSVAL_TYPE_STRING || type == JSVAL_TYPE_BOOLEAN ||
                  type == JSVAL_TYPE_MAGIC);
        val_ins = w.ldiValuePayload(addr);
    }

    guard(true, w.eqi(tag_ins, w.nameImmui(JSVAL_TYPE_TO_TAG(type))), exit);
    return val_ins;
}

LIns*
TraceRecorder::unbox_object(Address addr, LIns* tag_ins, JSValueType type, VMSideExit* exit)
{
    JS_ASSERT(type == JSVAL_TYPE_FUNOBJ || type == JSVAL_TYPE_NONFUNOBJ);
    guard(true, w.name(w.eqi(tag_ins, w.nameImmui(JSVAL_TAG_OBJECT)), "isObj"), exit);
    LIns *payload_ins = w.ldiValuePayload(addr);
    if (type == JSVAL_TYPE_FUNOBJ)
        guardClass(payload_ins, &js_FunctionClass, exit, LOAD_NORMAL);
    else
        guardNotClass(payload_ins, &js_FunctionClass, exit, LOAD_NORMAL);
    return payload_ins;
}

LIns*
TraceRecorder::unbox_value(const Value &v, Address addr, VMSideExit *exit, bool force_double)
{
    LIns *tag_ins = w.ldiValueTag(addr);

    if (v.isNumber() && force_double)
        return unbox_number_as_double(addr, tag_ins, exit);

    if (v.isInt32()) {
        guard(true, w.name(w.eqi(tag_ins, w.nameImmui(JSVAL_TAG_INT32)), "isInt"), exit);
        return w.i2d(w.ldiValuePayload(addr));
    }

    if (v.isDouble()) {
        guard(true, w.name(w.ltui(tag_ins, w.nameImmui(JSVAL_TAG_CLEAR)), "isDouble"), exit);
        return w.ldd(addr);
    }

    if (v.isObject()) {
        JSValueType type = v.toObject().isFunction() ? JSVAL_TYPE_FUNOBJ : JSVAL_TYPE_NONFUNOBJ;
        return unbox_object(addr, tag_ins, type, exit);
    }

    JSValueType type = v.extractNonDoubleObjectTraceType();
    return unbox_non_double_object(addr, tag_ins, type, exit);
}

void
TraceRecorder::unbox_any_object(Address addr, LIns **obj_ins, LIns **is_obj_ins)
{
    LIns *tag_ins = w.ldiValueTag(addr);
    *is_obj_ins = w.eqi(tag_ins, w.nameImmui(JSVAL_TAG_OBJECT));
    *obj_ins = w.ldiValuePayload(addr);
}

LIns*
TraceRecorder::is_boxed_true(Address addr)
{
    LIns *tag_ins = w.ldiValueTag(addr);
    LIns *bool_ins = w.eqi(tag_ins, w.nameImmui(JSVAL_TAG_BOOLEAN));
    LIns *payload_ins = w.ldiValuePayload(addr);
    return w.gtiN(w.andi(bool_ins, payload_ins), 0);
}

LIns*
TraceRecorder::is_boxed_magic(Address addr, JSWhyMagic why)
{
    LIns *tag_ins = w.ldiValueTag(addr);
    return w.eqi(tag_ins, w.nameImmui(JSVAL_TAG_MAGIC));
}

void
TraceRecorder::box_value_into(const Value &v, LIns *v_ins, Address addr)
{
    if (v.isNumber()) {
        JS_ASSERT(v_ins->isD());
        if (fcallinfo(v_ins) == &js_UnboxDouble_ci) {
            w.stiValueTag(v_ins->callArgN(0), addr);
            w.stiValuePayload(v_ins->callArgN(1), addr);
        } else if (IsPromotedInt32(v_ins)) {
            LIns *int_ins = w.demoteToInt32(v_ins);
            w.stiValueTag(w.nameImmui(JSVAL_TAG_INT32), addr);
            w.stiValuePayload(int_ins, addr);
        } else {
            w.std(v_ins, addr);
        }
        return;
    }

    if (v.isUndefined()) {
        box_undefined_into(addr);
    } else if (v.isNull()) {
        box_null_into(addr);
    } else {
        JSValueTag tag = v.isObject() ? JSVAL_TAG_OBJECT : v.extractNonDoubleObjectTraceTag();
        w.stiValueTag(w.nameImmui(tag), addr);
        w.stiValuePayload(v_ins, addr);
    }
}

LIns*
TraceRecorder::box_value_for_native_call(const Value &v, LIns *v_ins)
{
    return box_value_into_alloc(v, v_ins);
}

#elif JS_BITS_PER_WORD == 64

void
TraceRecorder::box_undefined_into(Address addr)
{
    w.stq(w.nameImmq(JSVAL_BITS(JSVAL_VOID)), addr);
}

inline LIns *
TraceRecorder::non_double_object_value_has_type(LIns *v_ins, JSValueType type)
{
    return w.eqi(w.q2i(w.rshuqN(v_ins, JSVAL_TAG_SHIFT)),
                 w.nameImmui(JSVAL_TYPE_TO_TAG(type)));
}

inline LIns *
TraceRecorder::unpack_ptr(LIns *v_ins)
{
    return w.andq(v_ins, w.nameImmq(JSVAL_PAYLOAD_MASK));
}

inline LIns *
TraceRecorder::unbox_number_as_double(LIns *v_ins, VMSideExit *exit)
{
    guard(true,
          w.ltuq(v_ins, w.nameImmq(JSVAL_UPPER_EXCL_SHIFTED_TAG_OF_NUMBER_SET)),
          exit);
    LIns* args[] = { v_ins };
    return w.call(&js_UnboxDouble_ci, args);
}

inline nanojit::LIns*
TraceRecorder::unbox_non_double_object(LIns* v_ins, JSValueType type, VMSideExit* exit)
{
    JS_ASSERT(type <= JSVAL_UPPER_INCL_TYPE_OF_VALUE_SET);
    LIns *unboxed_ins;
    if (type == JSVAL_TYPE_UNDEFINED) {
        unboxed_ins = w.immiUndefined();
    } else if (type == JSVAL_TYPE_NULL) {
        unboxed_ins = w.immpNull();
    } else if (type >= JSVAL_LOWER_INCL_TYPE_OF_PTR_PAYLOAD_SET) {
        unboxed_ins = unpack_ptr(v_ins);
    } else {
        JS_ASSERT(type == JSVAL_TYPE_INT32 || type == JSVAL_TYPE_BOOLEAN || type == JSVAL_TYPE_MAGIC);
        unboxed_ins = w.q2i(v_ins);
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
          w.geuq(v_ins, w.nameImmq(JSVAL_SHIFTED_TAG_OBJECT)),
          exit);
    v_ins = unpack_ptr(v_ins);
    if (type == JSVAL_TYPE_FUNOBJ)
        guardClass(v_ins, &js_FunctionClass, exit, LOAD_NORMAL);
    else
        guardNotClass(v_ins, &js_FunctionClass, exit, LOAD_NORMAL);
    return v_ins;
}

LIns*
TraceRecorder::unbox_value(const Value &v, Address addr, VMSideExit *exit, bool force_double)
{
    LIns *v_ins = w.ldq(addr);

    if (v.isNumber() && force_double)
        return unbox_number_as_double(v_ins, exit);

    if (v.isInt32()) {
        guard(true, non_double_object_value_has_type(v_ins, JSVAL_TYPE_INT32), exit);
        return w.i2d(w.q2i(v_ins));
    }

    if (v.isDouble()) {
        guard(true, w.leuq(v_ins, w.nameImmq(JSVAL_SHIFTED_TAG_MAX_DOUBLE)), exit);
        return w.qasd(v_ins);
    }

    if (v.isObject()) {
        JSValueType type = v.toObject().isFunction() ? JSVAL_TYPE_FUNOBJ : JSVAL_TYPE_NONFUNOBJ;
        return unbox_object(v_ins, type, exit);
    }

    JSValueType type = v.extractNonDoubleObjectTraceType();
    return unbox_non_double_object(v_ins, type, exit);
}

void
TraceRecorder::unbox_any_object(Address addr, LIns **obj_ins, LIns **is_obj_ins)
{
    JS_STATIC_ASSERT(JSVAL_TYPE_OBJECT == JSVAL_UPPER_INCL_TYPE_OF_VALUE_SET);
    LIns *v_ins = w.ldq(addr);
    *is_obj_ins = w.geuq(v_ins, w.nameImmq(JSVAL_TYPE_OBJECT));
    *obj_ins = unpack_ptr(v_ins);
}

LIns*
TraceRecorder::is_boxed_true(Address addr)
{
    LIns *v_ins = w.ldq(addr);
    return w.eqq(v_ins, w.immq(JSVAL_BITS(JSVAL_TRUE)));
}

LIns*
TraceRecorder::is_boxed_magic(Address addr, JSWhyMagic why)
{
    LIns *v_ins = w.ldq(addr);
    return w.eqq(v_ins, w.nameImmq(BUILD_JSVAL(JSVAL_TAG_MAGIC, why)));
}

LIns*
TraceRecorder::box_value_for_native_call(const Value &v, LIns *v_ins)
{
    if (v.isNumber()) {
        JS_ASSERT(v_ins->isD());
        if (fcallinfo(v_ins) == &js_UnboxDouble_ci)
            return v_ins->callArgN(0);
        if (IsPromotedInt32(v_ins)) {
            return w.orq(w.ui2uq(w.demoteToInt32(v_ins)),
                         w.nameImmq(JSVAL_SHIFTED_TAG_INT32));
        }
        return w.dasq(v_ins);
    }

    if (v.isNull())
        return w.nameImmq(JSVAL_BITS(JSVAL_NULL));
    if (v.isUndefined())
        return w.nameImmq(JSVAL_BITS(JSVAL_VOID));

    JSValueTag tag = v.isObject() ? JSVAL_TAG_OBJECT : v.extractNonDoubleObjectTraceTag();
    uint64 shiftedTag = ((uint64)tag) << JSVAL_TAG_SHIFT;
    LIns *shiftedTag_ins = w.nameImmq(shiftedTag);

    if (v.hasPtrPayload())
        return w.orq(v_ins, shiftedTag_ins);
    return w.orq(w.ui2uq(v_ins), shiftedTag_ins);
}

void
TraceRecorder::box_value_into(const Value &v, LIns *v_ins, Address addr)
{
    LIns *boxed_ins = box_value_for_native_call(v, v_ins);
    w.st(boxed_ins, addr);
}

#endif  

LIns*
TraceRecorder::box_value_into_alloc(const Value &v, LIns *v_ins)
{
    LIns *alloc_ins = w.allocp(sizeof(Value));
    box_value_into(v, v_ins, AllocSlotsAddress(alloc_ins));
    return alloc_ins;
}

LIns*
TraceRecorder::is_string_id(LIns *id_ins)
{
    return w.eqp0(w.andp(id_ins, w.nameImmw(JSID_TYPE_MASK)));
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
    return w.rshiN(w.p2i(id_ins), 1);
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getThis(LIns*& this_ins)
{
    StackFrame *fp = cx->fp();

    if (fp->isGlobalFrame()) {
        
        
        
        
        JS_ASSERT(!fp->thisValue().isPrimitive());

#ifdef DEBUG
        JSObject *obj = globalObj->thisObject(cx);
        if (!obj)
            RETURN_ERROR("thisObject hook failed");
        JS_ASSERT(&fp->thisValue().toObject() == obj);
#endif

        this_ins = w.immpObjGC(&fp->thisValue().toObject());
        return RECORD_CONTINUE;
    }

    JS_ASSERT(fp->callee().getGlobal() == globalObj);    
    Value& thisv = fp->thisValue();

    if (thisv.isObject() || fp->fun()->inStrictMode()) {
        





        this_ins = get(&fp->thisValue());
        return RECORD_CONTINUE;
    }

    
    if (!thisv.isNullOrUndefined())
        RETURN_STOP("wrapping primitive |this|");

    




    if (!ComputeThis(cx, fp))
        RETURN_ERROR("computeThis failed");

    
    this_ins = w.immpObjGC(globalObj);
    set(&thisv, this_ins);
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK void
TraceRecorder::guardClassHelper(bool cond, LIns* obj_ins, Class* clasp, VMSideExit* exit,
                                LoadQual loadQual)
{
    LIns* class_ins = w.ldpObjClasp(obj_ins, loadQual);

#ifdef JS_JIT_SPEW
    char namebuf[32];
    JS_snprintf(namebuf, sizeof namebuf, "%s_clasp", clasp->name);
    LIns* clasp_ins = w.name(w.immpNonGC(clasp), namebuf);
    JS_snprintf(namebuf, sizeof namebuf, "guard(class is %s)", clasp->name);
    LIns* cmp_ins = w.name(w.eqp(class_ins, clasp_ins), namebuf);
#else
    LIns* clasp_ins = w.immpNonGC(clasp);
    LIns* cmp_ins = w.eqp(class_ins, clasp_ins);
#endif
    guard(cond, cmp_ins, exit);
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
    *pobj_ins = w.ldpObjProto(obj_ins);

    bool cond = *pobj == NULL;
    guard(cond, w.name(w.eqp0(*pobj_ins), "guard(proto-not-null)"), exit);
    return !cond;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::guardPrototypeHasNoIndexedProperties(JSObject* obj, LIns* obj_ins, VMSideExit *exit)
{
    



    if (js_PrototypeHasIndexedProperties(cx, obj))
        return RECORD_STOP;

    JS_ASSERT(obj->isDenseArray());

    




    obj = obj->getProto();
    JS_ASSERT(obj);

    obj_ins = w.immpObjGC(obj);

    






    do {
        CHECK_STATUS(guardShape(obj_ins, obj, obj->shape(), "guard(shape)", exit));
        obj = obj->getProto();
        obj_ins = w.ldpObjProto(obj_ins);
    } while (obj);

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
TraceRecorder::clearReturningFrameFromNativeTracker()
{
    





    ClearSlotsVisitor visitor(nativeFrameTracker);
    VisitStackSlots(visitor, cx, 0);
    Value *vp = cx->regs().sp;
    Value *vpend = cx->fp()->slots() + cx->fp()->script()->nslots;
    for (; vp < vpend; ++vp)
        nativeFrameTracker.set(vp, NULL);
}

class BoxArg
{
  public:
    BoxArg(TraceRecorder *tr, Address addr)
        : tr(tr), addr(addr) {}
    TraceRecorder *tr;
    Address addr;
    bool operator()(uintN argi, Value *src) {
        tr->box_value_into(*src, tr->get(src), OffsetAddress(addr, argi * sizeof(Value)));
        return true;
    }
};






JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::putActivationObjects()
{
    StackFrame *const fp = cx->fp();
    bool have_args = fp->hasArgsObj() && !fp->argsObj().isStrictArguments();
    bool have_call = fp->isFunctionFrame() && fp->fun()->isHeavyweight();

    if (!have_args && !have_call)
        return ARECORD_CONTINUE;

    if (have_args && !fp->script()->usesArguments) {
        








        RETURN_STOP_A("dodgy arguments access");
    }

    uintN nformal = fp->numFormalArgs();
    uintN nactual = fp->numActualArgs();
    uintN nargs = have_args && have_call ? Max(nformal, nactual)
                                         : have_args ? nactual : nformal;

    LIns *args_ins;
    if (nargs > 0) {
        args_ins = w.allocp(nargs * sizeof(Value));
        
        Address addr = AllocSlotsAddress(args_ins);
        if (nargs == nactual)
            fp->forEachCanonicalActualArg(BoxArg(this, addr));
        else
            fp->forEachFormalArg(BoxArg(this, addr));
    } else {
        args_ins = w.immpNonGC(0);
    }

    if (have_args) {
        LIns* argsobj_ins = getFrameObjPtr(fp->addressOfArgs());
        LIns* args[] = { args_ins, argsobj_ins, cx_ins };
        w.call(&js_PutArgumentsOnTrace_ci, args);
    }

    if (have_call) {
        int nslots = fp->fun()->script()->bindings.countVars();
        LIns* slots_ins;
        if (nslots) {
            slots_ins = w.allocp(sizeof(Value) * nslots);
            for (int i = 0; i < nslots; ++i) {
                box_value_into(fp->slots()[i], get(&fp->slots()[i]), 
                               AllocSlotsAddress(slots_ins, i));
            }
        } else {
            slots_ins = w.immpNonGC(0);
        }

        LIns* scopeChain_ins = getFrameObjPtr(fp->addressOfScopeChain());
        LIns* args[] = { slots_ins, w.nameImmi(nslots), args_ins,
                         w.nameImmi(fp->numFormalArgs()), scopeChain_ins };
        w.call(&js_PutCallObjectOnTrace_ci, args);
    }

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_EnterFrame()
{
    StackFrame* const fp = cx->fp();

    if (++callDepth >= MAX_CALLDEPTH)
        RETURN_STOP_A("exceeded maximum call depth");

    debug_only_stmt(JSAutoByteString funBytes);
    debug_only_printf(LC_TMTracer, "EnterFrame %s, callDepth=%d\n",
                      cx->fp()->fun()->atom ?
                        js_AtomToPrintableString(cx, cx->fp()->fun()->atom, &funBytes) :
                        "<anonymous>",
                      callDepth);
    debug_only_stmt(
        if (LogController.lcbits & LC_TMRecorder) {
            void *mark = JS_ARENA_MARK(&cx->tempPool);
            Sprinter sprinter;
            INIT_SPRINTER(cx, &sprinter, &cx->tempPool, 0);

            js_Disassemble(cx, cx->fp()->script(), JS_TRUE, &sprinter);

            debug_only_printf(LC_TMTracer, "%s", sprinter.base);
            JS_ARENA_RELEASE(&cx->tempPool, mark);
            debug_only_print0(LC_TMTracer, "----\n");
        }
    )
    LIns* void_ins = w.immiUndefined();

    
    
    
    
    
    
    
    
    
    

    
    uintN nactual = fp->numActualArgs();
    uintN nformal = fp->numFormalArgs();
    if (nactual < nformal) {
        
        JS_ASSERT(fp->actualArgs() == fp->formalArgs());
        Value *beg = fp->formalArgs() + nactual;
        Value *end = fp->formalArgsEnd();
        for (Value *vp = beg; vp != end; ++vp) {
            nativeFrameTracker.set(vp, NULL);
            set(vp, void_ins);
        }
    } else if (nactual > nformal) {
        
        
        
        
        
        
        JS_ASSERT(fp->actualArgs() != fp->formalArgs());
        JS_ASSERT(fp->hasOverflowArgs());
        Value *srcbeg = fp->actualArgs() - 2;
        Value *srcend = fp->actualArgs() + nformal;
        Value *dstbeg = fp->formalArgs() - 2;
        for (Value *src = srcbeg, *dst = dstbeg; src != srcend; ++src, ++dst) {
            nativeFrameTracker.set(dst, NULL);
            tracker.set(dst, tracker.get(src));
            nativeFrameTracker.set(src, NULL);
            tracker.set(src, NULL);
        }
    }

    
    nativeFrameTracker.set(fp->addressOfArgs(), NULL);
    setFrameObjPtr(fp->addressOfArgs(), w.immpNull());

    
    nativeFrameTracker.set(fp->addressOfScopeChain(), NULL);
    setFrameObjPtr(fp->addressOfScopeChain(), w.immpNull());

    
    Value *vp = fp->slots();
    Value *vpstop = vp + fp->numFixed();
    for (; vp < vpstop; ++vp) {
        nativeFrameTracker.set(vp, NULL);
        set(vp, void_ins);
    }

    
    vp = fp->base();
    vpstop = fp->slots() + fp->numSlots();
    for (; vp < vpstop; ++vp)
        nativeFrameTracker.set(vp, NULL);

    LIns* callee_ins = get(&cx->fp()->calleev());
    LIns* scopeChain_ins = w.ldpObjParent(callee_ins);

    
    if (cx->fp()->fun()->isHeavyweight()) {
        if (js_IsNamedLambda(cx->fp()->fun()))
            RETURN_STOP_A("can't call named lambda heavyweight on trace");

        LIns* fun_ins = w.nameImmpNonGC(cx->fp()->fun());

        LIns* args[] = { scopeChain_ins, callee_ins, fun_ins, cx_ins };
        LIns* call_ins = w.call(&js_CreateCallObjectOnTrace_ci, args);
        guard(false, w.eqp0(call_ins), OOM_EXIT);

        setFrameObjPtr(fp->addressOfScopeChain(), call_ins);
    } else {
        setFrameObjPtr(fp->addressOfScopeChain(), scopeChain_ins);
    }

    
    if (fp->script() == fp->prev()->script() &&
        fp->prev()->prev() && fp->prev()->prev()->script() == fp->script()) {
        RETURN_STOP_A("recursion started inlining");
    }

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_LeaveFrame()
{
    debug_only_stmt(StackFrame *fp = cx->fp();)

    JS_ASSERT(js_CodeSpec[js_GetOpcode(cx, fp->script(),
              cx->regs().pc)].length == JSOP_CALL_LENGTH);

    if (callDepth-- <= 0)
        RETURN_STOP_A("returned out of a loop we started tracing");

    
    
    updateAtoms();
    set(&stackval(-1), rval_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_PUSH()
{
    stack(0, w.immiUndefined());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_POPV()
{
    Value& rval = stackval(-1);

    
    
    
    LIns *fp_ins = entryFrameIns();
    box_value_into(rval, get(&rval), StackFrameAddress(fp_ins,
                                                       StackFrame::offsetOfReturnValue()));
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

static JSBool JS_FASTCALL
functionProbe(JSContext *cx, JSFunction *fun, int enter)
{
#ifdef MOZ_TRACE_JSCALLS
    JSScript *script = fun ? FUN_SCRIPT(fun) : NULL;
    if (enter > 0)
        Probes::enterJSFun(cx, fun, script, enter);
    else
        Probes::exitJSFun(cx, fun, script, enter);
#endif
    return true;
}

JS_DEFINE_CALLINFO_3(static, BOOL, functionProbe, CONTEXT, FUNCTION, INT32, 0, ACCSET_ALL)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_RETURN()
{
    
    if (callDepth == 0) {
        AUDIT(returnLoopExits);
        return endLoop();
    }

    CHECK_STATUS_A(putActivationObjects());

    if (Probes::callTrackingActive(cx)) {
        LIns* args[] = { w.immi(0), w.nameImmpNonGC(cx->fp()->fun()), cx_ins };
        LIns* call_ins = w.call(&functionProbe_ci, args);
        guard(false, w.eqi0(call_ins), MISMATCH_EXIT);
    }

    
    Value& rval = stackval(-1);
    StackFrame *fp = cx->fp();
    if (fp->isConstructing() && rval.isPrimitive()) {
        rval_ins = get(&fp->thisValue());
    } else {
        rval_ins = get(&rval);
    }
    debug_only_stmt(JSAutoByteString funBytes);
    debug_only_printf(LC_TMTracer,
                      "returning from %s\n",
                      fp->fun()->atom ?
                        js_AtomToPrintableString(cx, fp->fun()->atom, &funBytes) :
                        "<anonymous>");
    clearReturningFrameFromNativeTracker();

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GOTO()
{
    




    jssrcnote* sn = js_GetSrcNote(cx->fp()->script(), cx->regs().pc);

    if (sn) {
        if (SN_TYPE(sn) == SRC_BREAK) {
            AUDIT(breakLoopExits);
            return endLoop();
        }

        



        if (SN_TYPE(sn) == SRC_BREAK2LABEL || SN_TYPE(sn) == SRC_CONT2LABEL)
            RETURN_STOP_A("labeled break");
    }
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IFEQ()
{
    trackCfgMerges(cx->regs().pc);
    return ifop();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IFNE()
{
    return ifop();
}

LIns*
TraceRecorder::newArguments(LIns* callee_ins)
{
    LIns* global_ins = w.immpObjGC(globalObj);
    LIns* argc_ins = w.nameImmi(cx->fp()->numActualArgs());

    LIns* args[] = { callee_ins, argc_ins, global_ins, cx_ins };
    LIns* argsobj_ins = w.call(&js_NewArgumentsOnTrace_ci, args);
    guard(false, w.eqp0(argsobj_ins), OOM_EXIT);

    return argsobj_ins;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARGUMENTS()
{
    StackFrame* const fp = cx->fp();

    
    JS_ASSERT(!fp->isEvalFrame());

    if (fp->hasOverriddenArgs())
        RETURN_STOP_A("Can't trace |arguments| if |arguments| is assigned to");
    if (fp->fun()->inStrictMode())
        RETURN_STOP_A("Can't trace strict-mode arguments");

    LIns* a_ins = getFrameObjPtr(fp->addressOfArgs());
    LIns* args_ins;
    LIns* callee_ins = get(&fp->calleev());
    if (a_ins->isImmP()) {
        
        args_ins = newArguments(callee_ins);
    } else {
        

        LIns* mem_ins = w.allocp(sizeof(JSObject *));

        LIns* isZero_ins = w.eqp0(a_ins);
        if (isZero_ins->isImmI(0)) {
            w.stAlloc(a_ins, mem_ins);
        } else if (isZero_ins->isImmI(1)) {
            LIns* call_ins = newArguments(callee_ins);
            w.stAlloc(call_ins, mem_ins);
        } else {
            LIns* br1 = w.jtUnoptimizable(isZero_ins);
            w.stAlloc(a_ins, mem_ins);
            LIns* br2 = w.j(NULL);
            w.label(br1);

            LIns* call_ins = newArguments(callee_ins);
            w.stAlloc(call_ins, mem_ins);
            w.label(br2);
        }
        args_ins = w.ldpAlloc(mem_ins);
    }

    stack(0, args_ins);
    setFrameObjPtr(fp->addressOfArgs(), args_ins);
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
    Value* sp = cx->regs().sp;
    jsint n = cx->regs().pc[1];
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
        LIns* concat = w.call(&js_ConcatStrings_ci, args);
        guard(false, w.eqp0(concat), OOM_EXIT);
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
        set(&v, w.eqi0(w.eqiN(get(&v), 1)));
        return ARECORD_CONTINUE;
    }
    if (v.isNumber()) {
        LIns* v_ins = get(&v);
        set(&v, w.ori(w.eqd0(v_ins), w.eqi0(w.eqd(v_ins, v_ins))));
        return ARECORD_CONTINUE;
    }
    if (v.isObjectOrNull()) {
        set(&v, w.eqp0(get(&v)));
        return ARECORD_CONTINUE;
    }
    JS_ASSERT(v.isString());
    set(&v, w.eqp0(w.getStringLength(get(&v))));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BITNOT()
{
    return InjectStatus(unaryIntOp(LIR_noti));
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
            !oracle->isInstructionUndemotable(cx->regs().pc) &&
            IsPromotedInt32(a) &&
            (!v.isInt32() || v.toInt32() != 0) &&
            (!v.isDouble() || v.toDouble() != 0) &&
            -v.toNumber() == (int)-v.toNumber())
        {
            VMSideExit* exit = snapshot(OVERFLOW_EXIT);
            a = w.subxovi(w.immi(0), w.demoteToInt32(a), createGuardRecord(exit));
            if (!a->isImmI() && a->isop(LIR_subxovi)) {
                guard(false, w.eqiN(a, 0), exit); 
            }
            a = w.i2d(a);
        } else {
            a = w.negd(a);
        }

        set(&v, a);
        return ARECORD_CONTINUE;
    }

    if (v.isNull()) {
        set(&v, w.immd(-0.0));
        return ARECORD_CONTINUE;
    }

    if (v.isUndefined()) {
        set(&v, w.immd(js_NaN));
        return ARECORD_CONTINUE;
    }

    if (v.isString()) {
        LIns* ok_ins = w.allocp(sizeof(JSBool));
        LIns* args[] = { ok_ins, get(&v), cx_ins };
        LIns* num_ins = w.call(&js_StringToNumber_ci, args);
        guard(false,
              w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
              OOM_EXIT);
        set(&v, w.negd(num_ins));
        return ARECORD_CONTINUE;
    }

    JS_ASSERT(v.isBoolean());
    set(&v, w.negd(w.i2d(get(&v))));
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
        set(&v, w.immd(0));
        return ARECORD_CONTINUE;
    }
    if (v.isUndefined()) {
        set(&v, w.immd(js_NaN));
        return ARECORD_CONTINUE;
    }

    if (v.isString()) {
        LIns* ok_ins = w.allocp(sizeof(JSBool));
        LIns* args[] = { ok_ins, get(&v), cx_ins };
        LIns* num_ins = w.call(&js_StringToNumber_ci, args);
        guard(false,
              w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
              OOM_EXIT);
        set(&v, num_ins);
        return ARECORD_CONTINUE;
    }

    JS_ASSERT(v.isBoolean());
    set(&v, w.i2d(get(&v)));
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

    TraceMonitor &localtm = *traceMonitor;
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
    JS_ASSERT_IF(clasp != &js_ArrayClass, proto->emptyShapes[0]->getClass() == clasp);

    proto_ins = w.immpObjGC(proto);
    return RECORD_CONTINUE;
}

RecordingStatus
TraceRecorder::getClassPrototype(JSProtoKey key, LIns*& proto_ins)
{
#ifdef DEBUG
    TraceMonitor &localtm = *traceMonitor;
#endif

    JSObject* proto;
    if (!js_GetClassPrototype(cx, globalObj, key, &proto))
        RETURN_ERROR("error in js_GetClassPrototype");

    
    JS_ASSERT(localtm.recorder);

#ifdef DEBUG
    
    if (key != JSProto_Array) {
        JS_ASSERT(proto->isNative());
        JS_ASSERT(proto->emptyShapes);
        EmptyShape *empty = proto->emptyShapes[0];
        JS_ASSERT(empty);
        JS_ASSERT(JSCLASS_CACHED_PROTO_KEY(empty->getClass()) == key);
    }
#endif

    proto_ins = w.immpObjGC(proto);
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
    LIns* obj_ins = w.call(&js_String_tn_ci, args);
    guard(false, w.eqp0(obj_ins), OOM_EXIT);

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
    if (argc == 0) {
        LIns *args[] = { proto_ins, cx_ins };
        arr_ins = w.call(&js::NewDenseEmptyArray_ci, args);
        guard(false, w.eqp0(arr_ins), OOM_EXIT);

    } else if (argc == 1 && argv[0].isNumber()) {
        
        LIns *len_ins;
        CHECK_STATUS(makeNumberUint32(get(argv), &len_ins));
        LIns *args[] = { proto_ins, len_ins, cx_ins };
        arr_ins = w.call(&js::NewDenseUnallocatedArray_ci, args);
        guard(false, w.eqp0(arr_ins), OOM_EXIT);

    } else {
        LIns *args[] = { proto_ins, w.nameImmi(argc), cx_ins };
        arr_ins = w.call(&js::NewDenseAllocatedArray_ci, args);
        guard(false, w.eqp0(arr_ins), OOM_EXIT);

        
        LIns *slots_ins = NULL;
        for (uint32 i = 0; i < argc && !outOfMemory(); i++) {
            stobj_set_dslot(arr_ins, i, slots_ins, argv[i], get(&argv[i]));
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
    status_ins = w.ori(status_ins, w.lshiN(w.xoriN(w.andiN(ok_ins, 1), 1), 1));
    w.stStateField(status_ins, builtinStatus);
}

JS_REQUIRES_STACK void
TraceRecorder::emitNativePropertyOp(const Shape* shape, LIns* obj_ins,
                                    bool setflag, LIns* addr_boxed_val_ins)
{
    JS_ASSERT(addr_boxed_val_ins->isop(LIR_allocp));
    JS_ASSERT(setflag ? !shape->hasSetterValue() : !shape->hasGetterValue());
    JS_ASSERT(setflag ? !shape->hasDefaultSetter() : !shape->hasDefaultGetterOrIsMethod());

    enterDeepBailCall();

    w.stStateField(addr_boxed_val_ins, nativeVp);
    w.stStateField(w.immi(1), nativeVpLen);

    CallInfo* ci = new (traceAlloc()) CallInfo();
    
    LIns* possibleArgs[] = { NULL, NULL, w.immpIdGC(SHAPE_USERID(shape)), obj_ins, cx_ins };
    LIns** args;
    if (setflag) {
        ci->_address = uintptr_t(shape->setterOp());
        ci->_typesig = CallInfo::typeSig5(ARGTYPE_I, ARGTYPE_P, ARGTYPE_P, ARGTYPE_P, ARGTYPE_B,
                                          ARGTYPE_P);
        possibleArgs[0] = addr_boxed_val_ins;
        possibleArgs[1] = strictModeCode_ins;
        args = possibleArgs;
    } else {
        ci->_address = uintptr_t(shape->getterOp());
        ci->_typesig = CallInfo::typeSig4(ARGTYPE_I, ARGTYPE_P, ARGTYPE_P, ARGTYPE_P, ARGTYPE_P);
        possibleArgs[1] = addr_boxed_val_ins;
        args = possibleArgs + 1;
    }
    ci->_isPure = 0;
    ci->_storeAccSet = ACCSET_STORE_ANY;
    ci->_abi = ABI_CDECL;
#ifdef DEBUG
    ci->_name = "JSPropertyOp";
#endif
    LIns* ok_ins = w.call(ci, args);

    
    w.stStateField(w.immpNull(), nativeVp);
    leaveDeepBailCall();

    
    
    
    
    LIns* status_ins = w.ldiStateField(builtinStatus);
    propagateFailureToBuiltinStatus(ok_ins, status_ins);
    guard(true, w.eqi0(status_ins), STATUS_EXIT);
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::emitNativeCall(JSSpecializedNative* sn, uintN argc, LIns* args[], bool rooted)
{
    if (JSTN_ERRTYPE(sn) == FAIL_STATUS) {
        
        
        JS_ASSERT(!pendingSpecializedNative);

        
        enterDeepBailCall();
    }

    LIns* res_ins = w.call(sn->builtin, args);

    
    if (rooted)
        w.stStateField(w.immpNull(), nativeVp);

    rval_ins = res_ins;
    switch (JSTN_ERRTYPE(sn)) {
      case FAIL_NULL:
        guard(false, w.eqp0(res_ins), OOM_EXIT);
        break;
      case FAIL_NEG:
        res_ins = w.i2d(res_ins);
        guard(false, w.ltdN(res_ins, 0), OOM_EXIT);
        break;
      case FAIL_NEITHER:
          guard(false, w.eqiN(res_ins, JS_NEITHER), OOM_EXIT);
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
    StackFrame* const fp = cx->fp();
    jsbytecode *pc = cx->regs().pc;

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
                *argp = w.immpObjGC(&fval.toObject());
            } else if (argtype == 'p') {
                CHECK_STATUS(getClassPrototype(&fval.toObject(), *argp));
            } else if (argtype == 'R') {
                *argp = w.nameImmpNonGC(cx->runtime);
            } else if (argtype == 'P') {
                
                
                if ((*pc == JSOP_CALL) &&
                    fp->hasImacropc() && *fp->imacropc() == JSOP_GETELEM)
                    *argp = w.nameImmpNonGC(fp->imacropc());
                else
                    *argp = w.nameImmpNonGC(pc);
            } else if (argtype == 'D') { 
                if (!tval.isNumber())
                    goto next_specialization;
                *argp = this_ins;
            } else if (argtype == 'M') {
                MathCache *mathCache = GetMathCache(cx);
                if (!mathCache)
                    return RECORD_ERROR;
                *argp = w.nameImmpNonGC(mathCache);
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

static JSBool FASTCALL
ceilReturningInt(jsdouble x, int32 *out)
{
    jsdouble r = js_math_ceil_impl(x);
    return JSDOUBLE_IS_INT32(r, out);
}

static JSBool FASTCALL
floorReturningInt(jsdouble x, int32 *out)
{
    jsdouble r = js_math_floor_impl(x);
    return JSDOUBLE_IS_INT32(r, out);
}

static JSBool FASTCALL
roundReturningInt(jsdouble x, int32 *out)
{
    jsdouble r = js_math_round_impl(x);
    return JSDOUBLE_IS_INT32(r, out);
}






JS_DEFINE_CALLINFO_2(static, BOOL, ceilReturningInt, DOUBLE, INT32PTR, 0, ACCSET_STORE_ANY)
JS_DEFINE_CALLINFO_2(static, BOOL, floorReturningInt, DOUBLE, INT32PTR, 0, ACCSET_STORE_ANY)
JS_DEFINE_CALLINFO_2(static, BOOL, roundReturningInt, DOUBLE, INT32PTR, 0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callFloatReturningInt(uintN argc, const nanojit::CallInfo *ci)
{
    Value& arg = stackval(-1);
    LIns* resptr_ins = w.allocp(sizeof(int32));
    LIns* args[] = { resptr_ins, get(&arg) };
    LIns* fits_ins = w.call(ci, args);

    guard(false, w.eqi0(fits_ins), OVERFLOW_EXIT);

    LIns* res_ins = w.ldiAlloc(resptr_ins);

    set(&stackval(0 - (2 + argc)), w.i2d(res_ins));

    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;

    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::callNative(uintN argc, JSOp mode)
{
    LIns* args[5];

    JS_ASSERT(mode == JSOP_CALL || mode == JSOP_NEW || mode == JSOP_FUNAPPLY ||
              mode == JSOP_FUNCALL);

    Value* vp = &stackval(0 - (2 + argc));
    JSObject* funobj = &vp[0].toObject();
    JSFunction* fun = funobj->getFunctionPrivate();
    JS_ASSERT(fun->isNative());
    Native native = fun->u.n.native;

    switch (argc) {
      case 1:
        if (vp[2].isNumber() && mode == JSOP_CALL) {
            if (native == js_math_ceil || native == js_math_floor || native == js_math_round) {
                LIns* a = get(&vp[2]);
                int32 result;
                if (IsPromotedInt32OrUint32(a)) {
                    set(&vp[0], a);
                    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                    return RECORD_CONTINUE;
                }
                if (native == js_math_floor) {
                    if (floorReturningInt(vp[2].toNumber(), &result))
                        return callFloatReturningInt(argc, &floorReturningInt_ci);
                } else if (native == js_math_ceil) {
                    if (ceilReturningInt(vp[2].toNumber(), &result))
                        return callFloatReturningInt(argc, &ceilReturningInt_ci);
                } else if (native == js_math_round) {
                    if (roundReturningInt(vp[2].toNumber(), &result))
                        return callFloatReturningInt(argc, &roundReturningInt_ci);
                }
            } else if (native == js_math_abs) {
                LIns* a = get(&vp[2]);
                if (IsPromotedInt32(a) && vp[2].toNumber() != INT_MIN) {
                    a = w.demoteToInt32(a);
                    
                    LIns* intMin_ins = w.name(w.immi(0x80000000), "INT_MIN");
                    LIns* isIntMin_ins = w.name(w.eqi(a, intMin_ins), "isIntMin");
                    guard(false, isIntMin_ins, MISMATCH_EXIT);
                    LIns* neg_ins = w.negi(a);
                    LIns* isNeg_ins = w.name(w.ltiN(a, 0), "isNeg");
                    LIns* abs_ins = w.name(w.cmovi(isNeg_ins, neg_ins, a), "abs");
                    set(&vp[0], w.i2d(abs_ins));
                    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                    return RECORD_CONTINUE;
                }
            }
            if (vp[1].isString()) {
                JSString *str = vp[1].toString();
                if (native == js_str_charAt) {
                    jsdouble i = vp[2].toNumber();
                    if (JSDOUBLE_IS_NaN(i))
                      i = 0;
                    if (i < 0 || i >= str->length())
                        RETURN_STOP("charAt out of bounds");
                    LIns* str_ins = get(&vp[1]);
                    LIns* idx_ins = get(&vp[2]);
                    LIns* char_ins;
                    CHECK_STATUS(getCharAt(str, str_ins, idx_ins, mode, &char_ins));
                    set(&vp[0], char_ins);
                    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                    return RECORD_CONTINUE;
                } else if (native == js_str_charCodeAt) {
                    jsdouble i = vp[2].toNumber();
                    if (JSDOUBLE_IS_NaN(i))
                      i = 0;
                    if (i < 0 || i >= str->length())
                        RETURN_STOP("charCodeAt out of bounds");
                    LIns* str_ins = get(&vp[1]);
                    LIns* idx_ins = get(&vp[2]);
                    LIns* charCode_ins;
                    CHECK_STATUS(getCharCodeAt(str, str_ins, idx_ins, &charCode_ins));
                    set(&vp[0], charCode_ins);
                    pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                    return RECORD_CONTINUE;
                }
            }
        } else if (vp[2].isString() && mode == JSOP_CALL) {
            if (native == js_regexp_exec) {
                







                if (!CallResultEscapes(cx->regs().pc)) {
                    JSObject* proto;
                    jsid id = ATOM_TO_JSID(cx->runtime->atomState.testAtom);
                    
                    if (js_GetClassPrototype(cx, NULL, JSProto_RegExp, &proto)) {
                        if (JSObject *tmp = HasNativeMethod(proto, id, js_regexp_test)) {
                            vp[0] = ObjectValue(*tmp);
                            funobj = tmp;
                            fun = tmp->getFunctionPrivate();
                            native = js_regexp_test;
                        }
                    }
                }
            }
        }
        break;

      case 2:
        if (vp[2].isNumber() && vp[3].isNumber() && mode == JSOP_CALL &&
            (native == js_math_min || native == js_math_max)) {
            LIns* a = get(&vp[2]);
            LIns* b = get(&vp[3]);
            if (IsPromotedInt32(a) && IsPromotedInt32(b)) {
                a = w.demoteToInt32(a);
                b = w.demoteToInt32(b);
                LIns* cmp = (native == js_math_min) ? w.lti(a, b) : w.gti(a, b);
                set(&vp[0], w.i2d(w.cmovi(cmp, a, b)));
                pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                return RECORD_CONTINUE;
            }
            if (IsPromotedUint32(a) && IsPromotedUint32(b)) {
                a = w.demoteToUint32(a);
                b = w.demoteToUint32(b);
                LIns* cmp = (native == js_math_min) ? w.ltui(a, b) : w.gtui(a, b);
                set(&vp[0], w.ui2d(w.cmovi(cmp, a, b)));
                pendingSpecializedNative = IGNORE_NATIVE_CALL_COMPLETE_CALLBACK;
                return RECORD_CONTINUE;
            }
        }
        break;
    }

    if (fun->flags & JSFUN_TRCINFO) {
        JSNativeTraceInfo *trcinfo = FUN_TRCINFO(fun);
        JS_ASSERT(trcinfo && fun->u.n.native == trcinfo->native);

        
        if (trcinfo->specializations) {
            RecordingStatus status = callSpecializedNative(trcinfo, argc, mode == JSOP_NEW);
            if (status != RECORD_STOP)
                return status;
        }
    }

    if (native == js_fun_apply || native == js_fun_call)
        RETURN_STOP("trying to call native apply or call");

    
    uintN vplen = 2 + argc;
    LIns* invokevp_ins = w.allocp(vplen * sizeof(Value));

    
    box_value_into(vp[0], w.immpObjGC(funobj), AllocSlotsAddress(invokevp_ins));

    
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

        
        if (!fun->isConstructor())
            RETURN_STOP("new with non-constructor native function");

        vp[1].setMagicWithObjectOrNullPayload(NULL);
        newobj_ins = w.immpMagicNull();

        
        mode = JSOP_CALL;
        this_ins = newobj_ins;
    } else {
        this_ins = get(&vp[1]);
    }
    set(&vp[1], this_ins);
    box_value_into(vp[1], this_ins, AllocSlotsAddress(invokevp_ins, 1));

    
    for (uintN n = 2; n < 2 + argc; n++) {
        box_value_into(vp[n], get(&vp[n]), AllocSlotsAddress(invokevp_ins, n));
        
        
        if (outOfMemory())
            RETURN_STOP("out of memory in argument list");
    }

    
    if (2 + argc < vplen) {
        for (uintN n = 2 + argc; n < vplen; n++) {
            box_undefined_into(AllocSlotsAddress(invokevp_ins, n));
            if (outOfMemory())
                RETURN_STOP("out of memory in extra slots");
        }
    }

    
    if (mode == JSOP_NEW)
        RETURN_STOP("untraceable fast native constructor");
    native_rval_ins = invokevp_ins;
    args[0] = invokevp_ins;
    args[1] = w.immi(argc);
    args[2] = cx_ins;
    uint32 typesig = CallInfo::typeSig3(ARGTYPE_I, ARGTYPE_P, ARGTYPE_I, ARGTYPE_P);

    
    
    

    CallInfo* ci = new (traceAlloc()) CallInfo();
    ci->_address = uintptr_t(fun->u.n.native);
    ci->_isPure = 0;
    ci->_storeAccSet = ACCSET_STORE_ANY;
    ci->_abi = ABI_CDECL;
    ci->_typesig = typesig;
#ifdef DEBUG
    ci->_name = js_anonymous_str;
    if (fun->atom) {
        JSAutoByteString bytes(cx, fun->atom);
        if (!!bytes) {
            size_t n = strlen(bytes.ptr()) + 1;
            char *buffer = new (traceAlloc()) char[n];
            memcpy(buffer, bytes.ptr(), n);
            ci->_name = buffer;
        }
    }
 #endif

    
    generatedSpecializedNative.builtin = ci;
    generatedSpecializedNative.flags = FAIL_STATUS | ((mode == JSOP_NEW)
                                                        ? JSTN_CONSTRUCTOR
                                                        : JSTN_UNBOX_AFTER);
    generatedSpecializedNative.prefix = NULL;
    generatedSpecializedNative.argtypes = NULL;

    
    
    
    
    
    w.stStateField(w.nameImmi(vplen), nativeVpLen);
    w.stStateField(invokevp_ins, nativeVp);

    
    
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

    if (Probes::callTrackingActive(cx)) {
        JSScript *script = FUN_SCRIPT(fun);
        if (!script || !script->isEmpty()) {
            LIns* args[] = { w.immi(1), w.nameImmpNonGC(fun), cx_ins };
            LIns* call_ins = w.call(&functionProbe_ci, args);
            guard(false, w.eqi0(call_ins), MISMATCH_EXIT);
        }
    }

    if (FUN_INTERPRETED(fun))
        return interpretedFunctionCall(fval, fun, argc, mode == JSOP_NEW);

    Native native = fun->maybeNative();
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
    if (Probes::callTrackingActive(cx)) {
        LIns* args[] = { w.immi(0), w.nameImmpNonGC(fun), cx_ins };
        LIns* call_ins = w.call(&functionProbe_ci, args);
        guard(false, w.eqi0(call_ins), MISMATCH_EXIT);
    }
    return rs;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NEW()
{
    uintN argc = GET_ARGC(cx->regs().pc);
    cx->assertValidStackDepth(argc + 2);
    return InjectStatus(functionCall(argc, JSOP_NEW));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DELNAME()
{
    return ARECORD_STOP;
}

static JSBool JS_FASTCALL
DeleteIntKey(JSContext* cx, JSObject* obj, int32 i, JSBool strict)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);
    LeaveTraceIfArgumentsObject(cx, obj);
    Value v = BooleanValue(false);
    jsid id;
    if (INT_FITS_IN_JSID(i)) {
        id = INT_TO_JSID(i);
    } else {
        if (!js_ValueToStringId(cx, Int32Value(i), &id)) {
            SetBuiltinError(tm);
            return false;
        }
    }

    if (!obj->deleteProperty(cx, id, &v, strict))
        SetBuiltinError(tm);
    return v.toBoolean();
}
JS_DEFINE_CALLINFO_4(extern, BOOL_FAIL, DeleteIntKey, CONTEXT, OBJECT, INT32, BOOL,
                     0, ACCSET_STORE_ANY)

static JSBool JS_FASTCALL
DeleteStrKey(JSContext* cx, JSObject* obj, JSString* str, JSBool strict)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);
    LeaveTraceIfArgumentsObject(cx, obj);
    Value v = BooleanValue(false);
    jsid id;

    




    if (!js_ValueToStringId(cx, StringValue(str), &id) || !obj->deleteProperty(cx, id, &v, strict))
        SetBuiltinError(tm);
    return v.toBoolean();
}
JS_DEFINE_CALLINFO_4(extern, BOOL_FAIL, DeleteStrKey, CONTEXT, OBJECT, STRING, BOOL,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DELPROP()
{
    Value& lval = stackval(-1);
    if (lval.isPrimitive())
        RETURN_STOP_A("JSOP_DELPROP on primitive base expression");
    if (&lval.toObject() == globalObj)
        RETURN_STOP_A("JSOP_DELPROP on global property");

    JSAtom* atom = atoms[GET_INDEX(cx->regs().pc)];

    enterDeepBailCall();
    LIns* args[] = { strictModeCode_ins, w.immpAtomGC(atom), get(&lval), cx_ins };
    LIns* rval_ins = w.call(&DeleteStrKey_ci, args);

    LIns* status_ins = w.ldiStateField(builtinStatus);
    pendingGuardCondition = w.eqi0(status_ins);
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
    if (lval.toObject().isArguments())
        RETURN_STOP_A("JSOP_DELELEM on the |arguments| object");

    Value& idx = stackval(-1);
    LIns* rval_ins;

    enterDeepBailCall();
    if (hasInt32Repr(idx)) {
        LIns* num_ins;
        CHECK_STATUS_A(makeNumberInt32(get(&idx), &num_ins));
        LIns* args[] = { strictModeCode_ins, num_ins, get(&lval), cx_ins };
        rval_ins = w.call(&DeleteIntKey_ci, args);
    } else if (idx.isString()) {
        LIns* args[] = { strictModeCode_ins, get(&idx), get(&lval), cx_ins };
        rval_ins = w.call(&DeleteStrKey_ci, args);
    } else {
        RETURN_STOP_A("JSOP_DELELEM on non-int, non-string index");
    }

    LIns* status_ins = w.ldiStateField(builtinStatus);
    pendingGuardCondition = w.eqi0(status_ins);
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
        type = w.immpAtomGC(cx->runtime->atomState.typeAtoms[JSTYPE_STRING]);
    } else if (r.isNumber()) {
        type = w.immpAtomGC(cx->runtime->atomState.typeAtoms[JSTYPE_NUMBER]);
    } else if (r.isUndefined()) {
        type = w.immpAtomGC(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
    } else if (r.isBoolean()) {
        type = w.immpAtomGC(cx->runtime->atomState.typeAtoms[JSTYPE_BOOLEAN]);
    } else if (r.isNull()) {
        type = w.immpAtomGC(cx->runtime->atomState.typeAtoms[JSTYPE_OBJECT]);
    } else {
        if (r.toObject().isFunction()) {
            type = w.immpAtomGC(cx->runtime->atomState.typeAtoms[JSTYPE_FUNCTION]);
        } else {
            LIns* args[] = { get(&r), cx_ins };
            type = w.call(&js_TypeOfObject_ci, args);
        }
    }
    set(&r, type);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_VOID()
{
    stack(-1, w.immiUndefined());
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
    LIns* v_ins_after;
    NameResult nr;

    CHECK_STATUS_A(name(vp, v_ins, nr));
    Value v = nr.tracked ? *vp : nr.v;
    Value v_after;
    CHECK_STATUS_A(incHelper(v, v_ins, v_after, v_ins_after, incr));
    LIns* v_ins_result = pre ? v_ins_after : v_ins;
    if (nr.tracked) {
        set(vp, v_ins_after);
        stack(0, v_ins_result);
        return ARECORD_CONTINUE;
    }

    if (!nr.obj->isCall())
        RETURN_STOP_A("incName on unsupported object class");

    CHECK_STATUS_A(setCallProp(nr.obj, nr.obj_ins, nr.shape, v_ins_after, v_after));
    stack(0, v_ins_result);
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









static bool
SafeLookup(JSContext *cx, JSObject* obj, jsid id, JSObject** pobjp, const Shape** shapep)
{
    do {
        
        if (obj->getOps()->lookupProperty)
            return false;

        if (const Shape *shape = obj->nativeLookup(id)) {
            *pobjp = obj;
            *shapep = shape;
            return true;
        }

        
        if (obj->getClass()->resolve != JS_ResolveStub)
            return false;
    } while ((obj = obj->getProto()) != NULL);
    *pobjp = NULL;
    *shapep = NULL;
    return true;
}





JS_REQUIRES_STACK RecordingStatus
TraceRecorder::lookupForSetPropertyOp(JSObject* obj, LIns* obj_ins, jsid id,
                                      bool* safep, JSObject** pobjp, const Shape** shapep)
{
    
    
    
    *safep = SafeLookup(cx, obj, id, pobjp, shapep);
    if (!*safep)
        return RECORD_CONTINUE;

    VMSideExit *exit = snapshot(BRANCH_EXIT);
    if (*shapep) {
        CHECK_STATUS(guardShape(obj_ins, obj, obj->shape(), "guard_kshape", exit));
        if (obj != *pobjp && *pobjp != globalObj) {
            CHECK_STATUS(guardShape(w.immpObjGC(*pobjp), *pobjp, (*pobjp)->shape(),
                                    "guard_vshape", exit));
        }
    } else {
        for (;;) {
            if (obj != globalObj)
                CHECK_STATUS(guardShape(obj_ins, obj, obj->shape(), "guard_proto_chain", exit));
            obj = obj->getProto();
            if (!obj)
                break;
            obj_ins = w.immpObjGC(obj);
        }
    }
    return RECORD_CONTINUE;
}

static JSBool FASTCALL
MethodWriteBarrier(JSContext* cx, JSObject* obj, uint32 slot, const Value* v)
{
#ifdef DEBUG
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);
#endif

    bool ok = obj->methodWriteBarrier(cx, slot, *v);
    JS_ASSERT(WasBuiltinSuccessful(tm));
    return ok;
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, MethodWriteBarrier, CONTEXT, OBJECT, UINT32, CVALUEPTR,
                     0, ACCSET_STORE_ANY)


JS_REQUIRES_STACK RecordingStatus
TraceRecorder::nativeSet(JSObject* obj, LIns* obj_ins, const Shape* shape,
                         const Value &v, LIns* v_ins)
{
    uint32 slot = shape->slot;
    JS_ASSERT((slot != SHAPE_INVALID_SLOT) == shape->hasSlot());
    JS_ASSERT_IF(shape->hasSlot(), obj->nativeContains(*shape));

    




















    if (!shape->hasDefaultSetter() && slot != SHAPE_INVALID_SLOT)
        RETURN_STOP("can't trace set of property with setter and slot");

    
    if (shape->hasGetterValue() && shape->hasDefaultSetter())
        RETURN_STOP("can't set a property that has only a getter");
    if (shape->isDataDescriptor() && !shape->writable())
        RETURN_STOP("can't assign to readonly property");

    
    if (!shape->hasDefaultSetter()) {
        if (shape->hasSetterValue())
            RETURN_STOP("can't trace JavaScript function setter yet");
        emitNativePropertyOp(shape, obj_ins, true, box_value_into_alloc(v, v_ins));
    }

    if (slot != SHAPE_INVALID_SLOT) {
        if (obj->brandedOrHasMethodBarrier()) {
            if (obj == globalObj) {
                
                
                
                JS_ASSERT(obj->nativeContains(*shape));
                if (IsFunctionObject(obj->getSlot(slot)))
                    RETURN_STOP("can't trace set of function-valued global property");
            } else {
                
                
                
                enterDeepBailCall();
                LIns* args[] = {box_value_into_alloc(v, v_ins), w.immi(slot), obj_ins, cx_ins};
                LIns* ok_ins = w.call(&MethodWriteBarrier_ci, args);
                guard(false, w.eqi0(ok_ins), OOM_EXIT);
                leaveDeepBailCall();
            }
        }

        
        if (obj == globalObj) {
            if (!lazilyImportGlobalSlot(slot))
                RETURN_STOP("lazy import of global slot failed");
            set(&obj->getSlotRef(slot), v_ins);
        } else {
            LIns* slots_ins = NULL;
            stobj_set_slot(obj, obj_ins, slot, slots_ins, v, v_ins);
        }
    }

    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::addDataProperty(JSObject* obj)
{
    if (!obj->isExtensible())
        RETURN_STOP("assignment adds property to non-extensible object");

    
    
    
    
    if (obj == globalObj)
        RETURN_STOP("set new property of global object"); 

    
    Class* clasp = obj->getClass();
    if (clasp->addProperty != Valueify(JS_PropertyStub))
        RETURN_STOP("set new property of object with addProperty hook");

    
    
    if (clasp->setProperty != Valueify(JS_StrictPropertyStub))
        RETURN_STOP("set new property with setter and slot");

#ifdef DEBUG
    addPropShapeBefore = obj->lastProperty();
#endif
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_AddProperty(JSObject *obj)
{
    Value& objv = stackval(-2);
    JS_ASSERT(&objv.toObject() == obj);
    LIns* obj_ins = get(&objv);
    Value& v = stackval(-1);
    LIns* v_ins = get(&v);
    const Shape* shape = obj->lastProperty();

    if (!shape->hasDefaultSetter()) {
        JS_ASSERT(IsWatchedProperty(cx, shape));
        RETURN_STOP_A("assignment adds property with watchpoint");
    }

#ifdef DEBUG
    JS_ASSERT(addPropShapeBefore);
    if (obj->inDictionaryMode())
        JS_ASSERT(shape->previous()->matches(addPropShapeBefore));
    else
        JS_ASSERT(shape->previous() == addPropShapeBefore);
    JS_ASSERT(shape->isDataDescriptor());
    JS_ASSERT(shape->hasDefaultSetter());
    addPropShapeBefore = NULL;
#endif

    if (obj->inDictionaryMode())
        RETURN_STOP_A("assignment adds property to dictionary"); 

    
    LIns* args[] = { w.immpShapeGC(shape), obj_ins, cx_ins };
    jsbytecode op = *cx->regs().pc;
    bool isDefinitelyAtom = (op == JSOP_SETPROP);
    const CallInfo *ci = isDefinitelyAtom ? &js_AddAtomProperty_ci : &js_AddProperty_ci;
    LIns* ok_ins = w.call(ci, args);
    guard(false, w.eqi0(ok_ins), OOM_EXIT);

    
    CHECK_STATUS_A(InjectStatus(nativeSet(obj, obj_ins, shape, v, v_ins)));

    
    if (op == JSOP_SETPROP || op == JSOP_SETNAME || op == JSOP_SETMETHOD)
        set(&objv, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::setUpwardTrackedVar(Value* stackVp, const Value &v, LIns* v_ins)
{
    JSValueType stackT = determineSlotType(stackVp);
    JSValueType otherT = getCoercedType(v);

    bool promote = true;

    if (stackT != otherT) {
        if (stackT == JSVAL_TYPE_DOUBLE && otherT == JSVAL_TYPE_INT32 && IsPromotedInt32(v_ins))
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
    
    StackFrame *fp = frameIfInRange(callobj);
    if (fp) {
        if (shape->setterOp() == SetCallArg) {
            JS_ASSERT(shape->hasShortID());
            uintN slot = uint16(shape->shortid);
            Value *vp2 = &fp->formalArg(slot);
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

        LIns* slots_ins = NULL;
        stobj_set_dslot(callobj_ins, slot, slots_ins, v, v_ins);
        return RECORD_CONTINUE;
    }

    
    
    

    
    const CallInfo* ci = NULL;
    if (shape->setterOp() == SetCallArg)
        ci = &js_SetCallArg_ci;
    else if (shape->setterOp() == SetCallVar)
        ci = &js_SetCallVar_ci;
    else
        RETURN_STOP("can't trace special CallClass setter");

    
    
    
    guard(false,
          w.eqp(entryFrameIns(), w.ldpObjPrivate(callobj_ins)),
          MISMATCH_EXIT);

    LIns* args[] = {
        box_value_for_native_call(v, v_ins),
        w.nameImmw(JSID_BITS(SHAPE_USERID(shape))),
        callobj_ins,
        cx_ins
    };
    LIns* call_ins = w.call(ci, args);
    guard(false, w.name(w.eqi0(call_ins), "guard(set upvar)"), STATUS_EXIT);

    return RECORD_CONTINUE;
}






JS_REQUIRES_STACK RecordingStatus
TraceRecorder::setProperty(JSObject* obj, LIns* obj_ins, const Value &v, LIns* v_ins,
                           bool* deferredp)
{
    *deferredp = false;

    JSAtom *atom = atoms[GET_INDEX(cx->regs().pc)];
    jsid id = ATOM_TO_JSID(atom);

    if (obj->getOps()->setProperty)
        RETURN_STOP("non-native object");  

    bool safe;
    JSObject* pobj;
    const Shape* shape;
    CHECK_STATUS(lookupForSetPropertyOp(obj, obj_ins, id, &safe, &pobj, &shape));
    if (!safe)
        RETURN_STOP("setprop: lookup fail"); 

    
    
    
    if (obj->isCall())
        return setCallProp(obj, obj_ins, shape, v_ins, v);

    
    
    if (!shape) {
        *deferredp = true;
        return addDataProperty(obj);
    }

    
    if (shape->isAccessorDescriptor()) {
        if (shape->hasDefaultSetter())
            RETURN_STOP("setting accessor property with no setter");
    } else if (!shape->writable()) {
        RETURN_STOP("setting readonly data property");
    }

    
    if (pobj == obj) {
        if (*cx->regs().pc == JSOP_SETMETHOD) {
            if (shape->isMethod() && shape->methodObject() == v.toObject())
                return RECORD_CONTINUE;
            RETURN_STOP("setmethod: property exists");
        }
        return nativeSet(obj, obj_ins, shape, v, v_ins);
    }

    
    
    if (shape->hasSlot()) {
        
        
        if (shape->hasShortID() && !shape->hasDefaultSetter())
            RETURN_STOP("shadowing assignment with shortid");
        *deferredp = true;
        return addDataProperty(obj);
    }

    
    
    if (shape->hasDefaultSetter() && !shape->hasGetterValue())
        return RECORD_CONTINUE;
    return nativeSet(obj, obj_ins, shape, v, v_ins);
}


JS_REQUIRES_STACK RecordingStatus
TraceRecorder::recordSetPropertyOp()
{
    Value& l = stackval(-2);
    if (!l.isObject())
        RETURN_STOP("set property of primitive");
    JSObject* obj = &l.toObject();
    LIns* obj_ins = get(&l);

    Value& r = stackval(-1);
    LIns* r_ins = get(&r);

    bool deferred;
    CHECK_STATUS(setProperty(obj, obj_ins, r, r_ins, &deferred));

    
    
    
    if (!deferred)
        set(&l, r_ins);
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETPROP()
{
    return InjectStatus(recordSetPropertyOp());
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETMETHOD()
{
    return InjectStatus(recordSetPropertyOp());
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETNAME()
{
    return InjectStatus(recordSetPropertyOp());
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::recordInitPropertyOp(jsbytecode op)
{
    Value& l = stackval(-2);
    JSObject* obj = &l.toObject();
    LIns* obj_ins = get(&l);
    JS_ASSERT(obj->getClass() == &js_ObjectClass);

    Value& v = stackval(-1);
    LIns* v_ins = get(&v);

    JSAtom* atom = atoms[GET_INDEX(cx->regs().pc)];
    jsid id = js_CheckForStringIndex(ATOM_TO_JSID(atom));

    
    
    
    
    if (const Shape* shape = obj->nativeLookup(id)) {
        
        
        
        if (op == JSOP_INITMETHOD)
            RETURN_STOP("initmethod: property exists");
        JS_ASSERT(shape->isDataDescriptor());
        JS_ASSERT(shape->hasSlot());
        JS_ASSERT(shape->hasDefaultSetter());
        return nativeSet(obj, obj_ins, shape, v, v_ins);
    }

    
    
    if (atom == cx->runtime->atomState.protoAtom) {
        bool deferred;
        return setProperty(obj, obj_ins, v, v_ins, &deferred);
    }

    
    return addDataProperty(obj);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INITPROP()
{
    return InjectStatus(recordInitPropertyOp(JSOP_INITPROP));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INITMETHOD()
{
    return InjectStatus(recordInitPropertyOp(JSOP_INITMETHOD));
}

JS_REQUIRES_STACK VMSideExit*
TraceRecorder::enterDeepBailCall()
{
    
    VMSideExit* exit = snapshot(DEEP_BAIL_EXIT);
    w.stTraceMonitorField(w.nameImmpNonGC(exit), bailExit);

    
    w.xbarrier(createGuardRecord(exit));

    
    forgetGuardedShapes();
    return exit;
}

JS_REQUIRES_STACK void
TraceRecorder::leaveDeepBailCall()
{
    
    w.stTraceMonitorField(w.immpNull(), bailExit);
}

JS_REQUIRES_STACK void
TraceRecorder::finishGetProp(LIns* obj_ins, LIns* vp_ins, LIns* ok_ins, Value* outp)
{
    
    
    
    
    JS_ASSERT(vp_ins->isop(LIR_allocp));
    LIns* result_ins = w.lddAlloc(vp_ins);
    set(outp, result_ins);
    if (js_CodeSpec[*cx->regs().pc].format & JOF_CALLOP)
        set(outp + 1, obj_ins);

    
    
    pendingGuardCondition = ok_ins;

    
    
    
    pendingUnboxSlot = outp;
}

static inline bool
RootedStringToId(JSContext* cx, JSString** namep, jsid* idp)
{
    JSString* name = *namep;
    if (name->isAtom()) {
        *idp = INTERNED_STRING_TO_JSID(name);
        return true;
    }

    JSAtom* atom = js_AtomizeString(cx, name, 0);
    if (!atom)
        return false;
    *namep = atom; 
    *idp = ATOM_TO_JSID(atom);
    return true;
}

static const size_t PIC_TABLE_ENTRY_COUNT = 32;

struct PICTableEntry
{
    jsid    id;
    uint32  shape;
    uint32  slot;
};

struct PICTable
{
    PICTable() : entryCount(0) {}

    PICTableEntry   entries[PIC_TABLE_ENTRY_COUNT];
    uint32          entryCount;

    bool scan(uint32 shape, jsid id, uint32 *slotOut) {
        for (size_t i = 0; i < entryCount; ++i) {
            PICTableEntry &entry = entries[i];
            if (entry.shape == shape && entry.id == id) {
                *slotOut = entry.slot;
                return true;
            }
        }
        return false;
    }

    void update(uint32 shape, jsid id, uint32 slot) {
        if (entryCount >= PIC_TABLE_ENTRY_COUNT)
            return;
        PICTableEntry &newEntry = entries[entryCount++];
        newEntry.shape = shape;
        newEntry.id = id;
        newEntry.slot = slot;
    }
};

static JSBool FASTCALL
GetPropertyByName(JSContext* cx, JSObject* obj, JSString** namep, Value* vp, PICTable *picTable)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);

    jsid id;
    if (!RootedStringToId(cx, namep, &id)) {
        SetBuiltinError(tm);
        return false;
    }
    
    
    PropertyIdOp op = obj->getOps()->getProperty;
    if (op) {
        bool result = op(cx, obj, obj, id, vp);
        if (!result)
            SetBuiltinError(tm);
        return WasBuiltinSuccessful(tm);
    }

    
    uint32 slot;
    if (picTable->scan(obj->shape(), id, &slot)) {
        *vp = obj->getSlot(slot);
        return WasBuiltinSuccessful(tm);
    }

    const Shape *shape;
    JSObject *holder;
    if (!js_GetPropertyHelperWithShape(cx, obj, obj, id, JSGET_METHOD_BARRIER, vp, &shape,
                                       &holder)) {
        SetBuiltinError(tm);
        return false;
    }

    
    if (obj == holder && shape->hasSlot() && shape->hasDefaultGetter()) {
        



        picTable->update(obj->shape(), id, shape->slot);
    }
    
    return WasBuiltinSuccessful(tm);
}
JS_DEFINE_CALLINFO_5(static, BOOL_FAIL, GetPropertyByName, CONTEXT, OBJECT, STRINGPTR, VALUEPTR,
                     PICTABLE,
                     0, ACCSET_STORE_ANY)



JS_REQUIRES_STACK RecordingStatus
TraceRecorder::primitiveToStringInPlace(Value* vp)
{
    Value v = *vp;
    JS_ASSERT(v.isPrimitive());

    if (!v.isString()) {
        
        
#ifdef DEBUG
        TraceMonitor *localtm = traceMonitor;
#endif
        JSString *str = js_ValueToString(cx, v);
        JS_ASSERT(localtm->recorder == this);
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

    
    
    
    LIns* vp_ins = w.name(w.allocp(sizeof(Value)), "vp");
    LIns* idvalp_ins = w.name(addr(idvalp), "idvalp");
    PICTable *picTable = new (traceAlloc()) PICTable();
    LIns* pic_ins = w.nameImmpNonGC(picTable);
    LIns* args[] = {pic_ins, vp_ins, idvalp_ins, obj_ins, cx_ins};
    LIns* ok_ins = w.call(&GetPropertyByName_ci, args);

    
    
    
    
    
    tracker.set(idvalp, w.ldp(AnyAddress(idvalp_ins)));

    finishGetProp(obj_ins, vp_ins, ok_ins, outp);
    leaveDeepBailCall();
    return RECORD_CONTINUE;
}

static JSBool FASTCALL
GetPropertyByIndex(JSContext* cx, JSObject* obj, int32 index, Value* vp)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);

    AutoIdRooter idr(cx);
    if (!js_Int32ToId(cx, index, idr.addr()) || !obj->getProperty(cx, idr.id(), vp)) {
        SetBuiltinError(tm);
        return JS_FALSE;
    }
    return WasBuiltinSuccessful(tm);
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, GetPropertyByIndex, CONTEXT, OBJECT, INT32, VALUEPTR, 0,
                     ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getPropertyByIndex(LIns* obj_ins, LIns* index_ins, Value* outp)
{
    CHECK_STATUS(makeNumberInt32(index_ins, &index_ins));

    
    enterDeepBailCall();
    LIns* vp_ins = w.name(w.allocp(sizeof(Value)), "vp");
    LIns* args[] = {vp_ins, index_ins, obj_ins, cx_ins};
    LIns* ok_ins = w.call(&GetPropertyByIndex_ci, args);
    finishGetProp(obj_ins, vp_ins, ok_ins, outp);
    leaveDeepBailCall();
    return RECORD_CONTINUE;
}

static JSBool FASTCALL
GetPropertyById(JSContext* cx, JSObject* obj, jsid id, Value* vp)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);
    if (!obj->getProperty(cx, id, vp)) {
        SetBuiltinError(tm);
        return JS_FALSE;
    }
    return WasBuiltinSuccessful(tm);
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, GetPropertyById, CONTEXT, OBJECT, JSID, VALUEPTR,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getPropertyById(LIns* obj_ins, Value* outp)
{
    
    JSAtom* atom;
    jsbytecode* pc = cx->regs().pc;
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
    LIns* vp_ins = w.name(w.allocp(sizeof(Value)), "vp");
    LIns* args[] = {vp_ins, w.nameImmw(JSID_BITS(id)), obj_ins, cx_ins};
    LIns* ok_ins = w.call(&GetPropertyById_ci, args);
    finishGetProp(obj_ins, vp_ins, ok_ins, outp);
    leaveDeepBailCall();
    return RECORD_CONTINUE;
}


static JSBool FASTCALL
GetPropertyWithNativeGetter(JSContext* cx, JSObject* obj, Shape* shape, Value* vp)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);

#ifdef DEBUG
    JSProperty* prop;
    JSObject* pobj;
    JS_ASSERT(obj->lookupProperty(cx, shape->propid, &pobj, &prop));
    JS_ASSERT(prop == (JSProperty*) shape);
#endif

    
    
    
    JS_ASSERT(obj->getClass() != &js_WithClass);

    vp->setUndefined();
    if (!shape->getterOp()(cx, obj, SHAPE_USERID(shape), vp)) {
        SetBuiltinError(tm);
        return JS_FALSE;
    }
    return WasBuiltinSuccessful(tm);
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
    LIns* vp_ins = w.name(w.allocp(sizeof(Value)), "vp");
    LIns* args[] = {vp_ins, w.nameImmpNonGC(shape), obj_ins, cx_ins};
    LIns* ok_ins = w.call(&GetPropertyWithNativeGetter_ci, args);
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
    Value*& sp = cx->regs().sp;
    switch (*cx->regs().pc) {
      case JSOP_GETPROP:
        sp++;
        sp[-1] = sp[-2];
        set(&sp[-1], get(&sp[-2]));
        sp[-2] = getter;
        set(&sp[-2], w.immpObjGC(&getter.toObject()));
        return callImacroInfallibly(getprop_imacros.scriptgetter);

      case JSOP_CALLPROP:
        sp += 2;
        sp[-2] = getter;
        set(&sp[-2], w.immpObjGC(&getter.toObject()));
        sp[-1] = sp[-3];
        set(&sp[-1], get(&sp[-3]));
        return callImacroInfallibly(callprop_imacros.scriptgetter);

      case JSOP_GETTHISPROP:
      case JSOP_GETARGPROP:
      case JSOP_GETLOCALPROP:
        sp += 2;
        sp[-2] = getter;
        set(&sp[-2], w.immpObjGC(&getter.toObject()));
        sp[-1] = ObjectValue(*obj);
        set(&sp[-1], obj_ins);
        return callImacroInfallibly(getthisprop_imacros.scriptgetter);

      default:
        RETURN_STOP("cannot trace script getter for this opcode");
    }
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getCharCodeAt(JSString *str, LIns* str_ins, LIns* idx_ins, LIns** out)
{
    CHECK_STATUS(makeNumberInt32(idx_ins, &idx_ins));
    idx_ins = w.ui2p(idx_ins);
    LIns *lengthAndFlags_ins = w.ldpStringLengthAndFlags(str_ins);
    if (MaybeBranch mbr = w.jt(w.eqp0(w.andp(lengthAndFlags_ins, w.nameImmw(JSString::ROPE_BIT)))))
    {
        LIns *args[] = { str_ins, cx_ins };
        LIns *ok_ins = w.call(&js_FlattenOnTrace_ci, args);
        guard(false, w.eqi0(ok_ins), OOM_EXIT);
        w.label(mbr);
    }

    guard(true,
          w.ltup(idx_ins, w.rshupN(lengthAndFlags_ins, JSString::LENGTH_SHIFT)),
          snapshot(MISMATCH_EXIT));
    *out = w.i2d(w.getStringChar(str_ins, idx_ins));
    return RECORD_CONTINUE;
}

JS_STATIC_ASSERT(sizeof(JSString) == 16 || sizeof(JSString) == 32);


JS_REQUIRES_STACK LIns*
TraceRecorder::getUnitString(LIns* str_ins, LIns* idx_ins)
{
    LIns *ch_ins = w.getStringChar(str_ins, idx_ins);
    guard(true, w.ltuiN(ch_ins, JSAtom::UNIT_STATIC_LIMIT), MISMATCH_EXIT);
    JS_STATIC_ASSERT(sizeof(JSString) == 16 || sizeof(JSString) == 32);
    return w.addp(w.nameImmpNonGC(JSAtom::unitStaticTable),
                  w.lshpN(w.ui2p(ch_ins), (sizeof(JSString) == 16) ? 4 : 5));
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::getCharAt(JSString *str, LIns* str_ins, LIns* idx_ins, JSOp mode, LIns** out)
{
    CHECK_STATUS(makeNumberInt32(idx_ins, &idx_ins));
    idx_ins = w.ui2p(idx_ins);
    LIns *lengthAndFlags_ins = w.ldpStringLengthAndFlags(str_ins);
    if (MaybeBranch mbr = w.jt(w.eqp0(w.andp(lengthAndFlags_ins,
                                             w.nameImmw(JSString::ROPE_BIT)))))
    {
        LIns *args[] = { str_ins, cx_ins };
        LIns *ok_ins = w.call(&js_FlattenOnTrace_ci, args);
        guard(false, w.eqi0(ok_ins), OOM_EXIT);
        w.label(mbr);
    }

    LIns* inRange = w.ltup(idx_ins, w.rshupN(lengthAndFlags_ins, JSString::LENGTH_SHIFT));

    if (mode == JSOP_GETELEM) {
        guard(true, inRange, MISMATCH_EXIT);

        *out = getUnitString(str_ins, idx_ins);
    } else {
        LIns *phi_ins = w.allocp(sizeof(JSString *));
        w.stAlloc(w.nameImmpNonGC(cx->runtime->emptyString), phi_ins);

        if (MaybeBranch mbr = w.jf(inRange)) {
            LIns *unitstr_ins = getUnitString(str_ins, idx_ins);
            w.stAlloc(unitstr_ins, phi_ins);
            w.label(mbr);
        }
        *out = w.ldpAlloc(phi_ins);
    }
    return RECORD_CONTINUE;
}


#if NJ_EXPANDED_LOADSTORE_SUPPORTED && NJ_F2I_SUPPORTED
static bool OkToTraceTypedArrays = true;
#else
static bool OkToTraceTypedArrays = false;
#endif

JS_REQUIRES_STACK void
TraceRecorder::guardNotHole(LIns *argsobj_ins, LIns *idx_ins)
{
    
    LIns* argsData_ins = w.getObjPrivatizedSlot(argsobj_ins, JSObject::JSSLOT_ARGS_DATA);
    LIns* slotOffset_ins = w.addp(w.nameImmw(offsetof(ArgumentsData, slots)),
                                  w.ui2p(w.muliN(idx_ins, sizeof(Value))));
    LIns* vp_ins = w.addp(argsData_ins, slotOffset_ins);

    guard(false,
          w.name(is_boxed_magic(ArgsSlotOffsetAddress(vp_ins), JS_ARGS_HOLE),
                 "guard(not deleted arg)"),
          MISMATCH_EXIT);
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETELEM()
{
    bool call = *cx->regs().pc == JSOP_CALLELEM;

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
        LIns* char_ins;
        CHECK_STATUS_A(getCharAt(lval.toString(), obj_ins, idx_ins, JSOP_GETELEM, &char_ins));
        set(&lval, char_ins);
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
        
        int32 int_idx = idx.toInt32();
        if (int_idx < 0 || int_idx >= (int32)obj->getArgsInitialLength())
            RETURN_STOP_A("cannot trace arguments with out of range index");
        if (obj->getArgsElement(int_idx).isMagic(JS_ARGS_HOLE))
            RETURN_STOP_A("reading deleted args element");

        
        unsigned depth;
        StackFrame *afp = guardArguments(obj, obj_ins, &depth);
        if (afp) {
            Value* vp = &afp->canonicalActualArg(int_idx);
            if (idx_ins->isImmD()) {
                JS_ASSERT(int_idx == (int32)idx_ins->immD());
                guardNotHole(obj_ins, w.nameImmi(int_idx));
                v_ins = get(vp);
            } else {
                
                
                
                CHECK_STATUS_A(makeNumberInt32(idx_ins, &idx_ins));

                



                guard(true,
                      w.name(w.ltui(idx_ins, w.nameImmui(afp->numActualArgs())),
                             "guard(upvar index in range)"),
                      MISMATCH_EXIT);

                guardNotHole(obj_ins, idx_ins);

                JSValueType type = getCoercedType(*vp);

                
                LIns* typemap_ins;
                if (depth == 0) {
                    
                    
                    
                    unsigned stackSlots = NativeStackSlots(cx, 0 );
                    JSValueType* typemap = new (traceAlloc()) JSValueType[stackSlots];
                    DetermineTypesVisitor detVisitor(*this, typemap);
                    VisitStackSlots(detVisitor, cx, 0);
                    typemap_ins = w.nameImmpNonGC(typemap + 2 );
                } else {
                    
                    
                    
                    
                    
                    LIns* fip_ins = w.ldpRstack(lirbuf->rp, (callDepth-depth)*sizeof(FrameInfo*));
                    typemap_ins = w.addp(fip_ins, w.nameImmw(sizeof(FrameInfo) + 2 * sizeof(JSValueType)));
                }

                LIns* type_ins = w.lduc2uiConstTypeMapEntry(typemap_ins, idx_ins);
                guard(true,
                      w.name(w.eqi(type_ins, w.immi(type)), "guard(type-stable upvar)"),
                      BRANCH_EXIT);

                
                size_t stackOffset = nativespOffset(&afp->canonicalActualArg(0));
                LIns* args_addr_ins = w.addp(lirbuf->sp, w.nameImmw(stackOffset));
                LIns* argi_addr_ins = w.addp(args_addr_ins,
                                             w.ui2p(w.muli(idx_ins, w.nameImmi(sizeof(double)))));

                
                
                
                
                v_ins = stackLoad(AnyAddress(argi_addr_ins), type);
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

        VMSideExit* branchExit = snapshot(BRANCH_EXIT);
        guardDenseArray(obj_ins, branchExit);
        CHECK_STATUS_A(denseArrayElement(lval, idx, vp, v_ins, addr_ins, branchExit));
        set(&lval, v_ins);
        if (call)
            set(&idx, obj_ins);
        return ARECORD_CONTINUE;
    }

    if (OkToTraceTypedArrays && js_IsTypedArray(obj)) {
        
        Value* vp;
        guardClass(obj_ins, obj->getClass(), snapshot(BRANCH_EXIT), LOAD_CONST);
        CHECK_STATUS_A(typedArrayElement(lval, idx, vp, v_ins));
        set(&lval, v_ins);
        if (call)
            set(&idx, obj_ins);
        return ARECORD_CONTINUE;
    }

    return InjectStatus(getPropertyByIndex(obj_ins, idx_ins, &lval));
}



static JSBool FASTCALL
SetPropertyByName(JSContext* cx, JSObject* obj, JSString** namep, Value* vp, JSBool strict)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);

    jsid id;
    if (!RootedStringToId(cx, namep, &id) || !obj->setProperty(cx, id, vp, strict)) {
        SetBuiltinError(tm);
        return false;
    }
    return WasBuiltinSuccessful(tm);
}
JS_DEFINE_CALLINFO_5(static, BOOL_FAIL, SetPropertyByName, 
                     CONTEXT, OBJECT, STRINGPTR, VALUEPTR, BOOL,
                     0, ACCSET_STORE_ANY)

static JSBool FASTCALL
InitPropertyByName(JSContext* cx, JSObject* obj, JSString** namep, ValueArgType arg)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);

    jsid id;
    if (!RootedStringToId(cx, namep, &id) ||
        !obj->defineProperty(cx, id, ValueArgToConstRef(arg), NULL, NULL, JSPROP_ENUMERATE)) {
        SetBuiltinError(tm);
        return JS_FALSE;
    }
    return WasBuiltinSuccessful(tm);
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
        LIns* idvalp_ins = w.name(addr(idvalp), "idvalp");
        LIns* args[] = {v_ins, idvalp_ins, obj_ins, cx_ins};
        pendingGuardCondition = w.call(&InitPropertyByName_ci, args);
    } else {
        
        LIns* vp_ins = box_value_into_alloc(*rvalp, get(rvalp));
        enterDeepBailCall();
        LIns* idvalp_ins = w.name(addr(idvalp), "idvalp");
        LIns* args[] = { strictModeCode_ins, vp_ins, idvalp_ins, obj_ins, cx_ins };
        pendingGuardCondition = w.call(&SetPropertyByName_ci, args);
    }

    leaveDeepBailCall();
    return RECORD_CONTINUE;
}

static JSBool FASTCALL
SetPropertyByIndex(JSContext* cx, JSObject* obj, int32 index, Value* vp, JSBool strict)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);

    AutoIdRooter idr(cx);
    if (!js_Int32ToId(cx, index, idr.addr()) || !obj->setProperty(cx, idr.id(), vp, strict)) {
        SetBuiltinError(tm);
        return false;
    }
    return WasBuiltinSuccessful(tm);
}
JS_DEFINE_CALLINFO_5(static, BOOL_FAIL, SetPropertyByIndex, CONTEXT, OBJECT, INT32, VALUEPTR, BOOL,
                     0, ACCSET_STORE_ANY)

static JSBool FASTCALL
InitPropertyByIndex(JSContext* cx, JSObject* obj, int32 index, ValueArgType arg)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    LeaveTraceIfGlobalObject(cx, obj);

    AutoIdRooter idr(cx);
    if (!js_Int32ToId(cx, index, idr.addr()) ||
        !obj->defineProperty(cx, idr.id(), ValueArgToConstRef(arg), NULL, NULL, JSPROP_ENUMERATE)) {
        SetBuiltinError(tm);
        return JS_FALSE;
    }
    return WasBuiltinSuccessful(tm);
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, InitPropertyByIndex, CONTEXT, OBJECT, INT32, VALUE,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::initOrSetPropertyByIndex(LIns* obj_ins, LIns* index_ins, Value* rvalp, bool init)
{
    CHECK_STATUS(makeNumberInt32(index_ins, &index_ins));

    if (init) {
        LIns* rval_ins = box_value_for_native_call(*rvalp, get(rvalp));
        enterDeepBailCall();
        LIns* args[] = {rval_ins, index_ins, obj_ins, cx_ins};
        pendingGuardCondition = w.call(&InitPropertyByIndex_ci, args);
    } else {
        
        LIns* vp_ins = box_value_into_alloc(*rvalp, get(rvalp));
        enterDeepBailCall();
        LIns* args[] = {strictModeCode_ins, vp_ins, index_ins, obj_ins, cx_ins};
        pendingGuardCondition = w.call(&SetPropertyByIndex_ci, args);
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
                                             *cx->regs().pc == JSOP_INITELEM));
    } else if (OkToTraceTypedArrays && js_IsTypedArray(obj)) {
        
        VMSideExit* branchExit = snapshot(BRANCH_EXIT);

        
        guardClass(obj_ins, obj->getClass(), branchExit, LOAD_CONST);

        js::TypedArray* tarray = js::TypedArray::fromJSObject(obj);

        LIns* priv_ins = w.ldpObjPrivate(obj_ins);

        
        
        CHECK_STATUS_A(makeNumberInt32(idx_ins, &idx_ins));

        
        CHECK_STATUS_A(guard(true,
                             w.name(w.ltui(idx_ins, w.ldiConstTypedArrayLength(priv_ins)),
                                    "inRange"),
                             OVERFLOW_EXIT, true));

        
        LIns* data_ins = w.ldpConstTypedArrayData(priv_ins);
        LIns* pidx_ins = w.ui2p(idx_ins);
        LIns* typed_v_ins = v_ins;

        
        
        
        if (!v.isNumber()) {
            if (v.isNull()) {
                typed_v_ins = w.immd(0);
            } else if (v.isUndefined()) {
                typed_v_ins = w.immd(js_NaN);
            } else if (v.isString()) {
                LIns* ok_ins = w.allocp(sizeof(JSBool));
                LIns* args[] = { ok_ins, typed_v_ins, cx_ins };
                typed_v_ins = w.call(&js_StringToNumber_ci, args);
                guard(false,
                      w.name(w.eqi0(w.ldiAlloc(ok_ins)), "guard(oom)"),
                      OOM_EXIT);
            } else if (v.isBoolean()) {
                JS_ASSERT(v.isBoolean());
                typed_v_ins = w.i2d(typed_v_ins);
            } else {
                typed_v_ins = w.immd(js_NaN);
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
            typed_v_ins = d2u(typed_v_ins);
            break;
          case js::TypedArray::TYPE_UINT8_CLAMPED:
            if (IsPromotedInt32(typed_v_ins)) {
                typed_v_ins = w.demoteToInt32(typed_v_ins);
                typed_v_ins = w.cmovi(w.ltiN(typed_v_ins, 0),
                                      w.immi(0),
                                      w.cmovi(w.gtiN(typed_v_ins, 0xff),
                                                     w.immi(0xff),
                                                     typed_v_ins));
            } else {
                typed_v_ins = w.call(&js_TypedArray_uint8_clamp_double_ci, &typed_v_ins);
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
            w.sti2cTypedArrayElement(typed_v_ins, data_ins, pidx_ins);
            break;
          case js::TypedArray::TYPE_INT16:
          case js::TypedArray::TYPE_UINT16:
            w.sti2sTypedArrayElement(typed_v_ins, data_ins, pidx_ins);
            break;
          case js::TypedArray::TYPE_INT32:
          case js::TypedArray::TYPE_UINT32:
            w.stiTypedArrayElement(typed_v_ins, data_ins, pidx_ins);
            break;
          case js::TypedArray::TYPE_FLOAT32:
            w.std2fTypedArrayElement(typed_v_ins, data_ins, pidx_ins);
            break;
          case js::TypedArray::TYPE_FLOAT64:
            w.stdTypedArrayElement(typed_v_ins, data_ins, pidx_ins);
            break;
          default:
            JS_NOT_REACHED("Unknown typed array type in tracer");       
        }
    } else if (idx.toInt32() < 0 || !obj->isDenseArray()) {
        CHECK_STATUS_A(initOrSetPropertyByIndex(obj_ins, idx_ins, &v,
                                                *cx->regs().pc == JSOP_INITELEM));
    } else {
        
        VMSideExit* branchExit = snapshot(BRANCH_EXIT);
        VMSideExit* mismatchExit = snapshot(MISMATCH_EXIT);

        
        if (!obj->isDenseArray()) 
            return ARECORD_STOP;
        guardDenseArray(obj_ins, branchExit);

        
        
        CHECK_STATUS_A(makeNumberInt32(idx_ins, &idx_ins));

        if (!js_EnsureDenseArrayCapacity(cx, obj, idx.toInt32()))
            RETURN_STOP_A("couldn't ensure dense array capacity for setelem");

        
        
        LIns* capacity_ins = w.ldiDenseArrayCapacity(obj_ins);
        




        w.pauseAddingCSEValues();
        if (MaybeBranch mbr = w.jt(w.name(w.ltui(idx_ins, capacity_ins), "inRange"))) {
            LIns* args[] = { idx_ins, obj_ins, cx_ins };
            LIns* res_ins = w.call(&js_EnsureDenseArrayCapacity_ci, args);
            guard(false, w.eqi0(res_ins), mismatchExit);
            w.label(mbr);
        }
        w.resumeAddingCSEValues();

        
        LIns *elemp_ins = w.name(w.getDslotAddress(obj_ins, idx_ins), "elemp");

        
        
        
        
        
        Address dslotAddr = DSlotsAddress(elemp_ins);
        LIns* isHole_ins = w.name(is_boxed_magic(dslotAddr, JS_ARRAY_HOLE),
                                  "isHole");
        w.pauseAddingCSEValues();
        if (MaybeBranch mbr1 = w.jf(isHole_ins)) {
            




            CHECK_STATUS_A(guardPrototypeHasNoIndexedProperties(obj, obj_ins, branchExit));
            LIns* length_ins = w.lduiObjPrivate(obj_ins);
            if (MaybeBranch mbr2 = w.jt(w.ltui(idx_ins, length_ins))) {
                LIns* newLength_ins = w.name(w.addiN(idx_ins, 1), "newLength");
                w.stuiObjPrivate(obj_ins, newLength_ins);
                w.label(mbr2);
            }
            w.label(mbr1);
        }
        w.resumeAddingCSEValues();

        
        box_value_into(v, v_ins, dslotAddr);
    }

    jsbytecode* pc = cx->regs().pc;
    if (*pc == JSOP_SETELEM && pc[JSOP_SETELEM_LENGTH] != JSOP_POP)
        set(&lval, v_ins);

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETELEM()
{
    return setElem(-3, -2, -1);
}

static JSBool FASTCALL
CheckSameGlobal(JSObject *obj, JSObject *globalObj)
{
    return obj->getGlobal() == globalObj;
}
JS_DEFINE_CALLINFO_2(static, BOOL, CheckSameGlobal, OBJECT, OBJECT, 0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLNAME()
{
    JSObject* scopeObj = &cx->fp()->scopeChain();
    LIns *funobj_ins;
    JSObject *funobj;
    if (scopeObj != globalObj) {
        Value* vp;
        NameResult nr;
        CHECK_STATUS_A(scopeChainProp(scopeObj, vp, funobj_ins, nr, &scopeObj));
        if (!nr.tracked)
            vp = &nr.v;
        if (!vp->isObject())
            RETURN_STOP_A("callee is not an object");
        funobj = &vp->toObject();
        if (!funobj->isFunction())
            RETURN_STOP_A("callee is not a function");
    } else {
        LIns* obj_ins = w.immpObjGC(globalObj);
        JSObject* obj2;
        PCVal pcval;

        CHECK_STATUS_A(test_property_cache(scopeObj, obj_ins, obj2, pcval));

        if (pcval.isNull() || !pcval.isFunObj())
            RETURN_STOP_A("callee is not a function");

        funobj = &pcval.toFunObj();
        funobj_ins = w.immpObjGC(funobj);
    }

    
    
    
    
    
    if (scopeObj == globalObj) {
        JSFunction *fun = funobj->getFunctionPrivate();
        if (!fun->isInterpreted() || !fun->inStrictMode()) {
            if (funobj->getGlobal() != globalObj)
                RETURN_STOP_A("callee crosses globals");

            
            
            
            if (!funobj_ins->isImmP() && !tree->script->compileAndGo) {
                LIns* args[] = { w.immpObjGC(globalObj), funobj_ins };
                guard(false, w.eqi0(w.call(&CheckSameGlobal_ci, args)), MISMATCH_EXIT);
            }
        }
    }

    stack(0, funobj_ins);
    stack(1, w.immiUndefined());
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
    StackFrame* fp = cx->stack.findFrameAtLevel(level);
    const CallInfo* ci;
    int32 slot;
    if (!fp->isFunctionFrame() || fp->isEvalFrame()) {
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

    LIns* outp = w.allocp(sizeof(double));
    LIns* args[] = {
        outp,
        w.nameImmi(callDepth),
        w.nameImmi(slot),
        w.nameImmi(level),
        cx_ins
    };
    LIns* call_ins = w.call(ci, args);
    JSValueType type = getCoercedType(v);
    guard(true,
          w.name(w.eqi(call_ins, w.immi(type)), "guard(type-stable upvar)"),
          BRANCH_EXIT);
    return stackLoad(AllocSlotsAddress(outp), type);
}





LIns*
TraceRecorder::stackLoad(Address addr, uint8 type)
{
    switch (type) {
      case JSVAL_TYPE_DOUBLE:
        return w.ldd(addr);
      case JSVAL_TYPE_NONFUNOBJ:
      case JSVAL_TYPE_STRING:
      case JSVAL_TYPE_FUNOBJ:
      case JSVAL_TYPE_NULL:
        return w.ldp(addr);
      case JSVAL_TYPE_INT32:
        return w.i2d(w.ldi(addr));
      case JSVAL_TYPE_BOOLEAN:
      case JSVAL_TYPE_UNDEFINED:
      case JSVAL_TYPE_MAGIC:
        return w.ldi(addr);
      case JSVAL_TYPE_BOXED:
      default:
        JS_NOT_REACHED("found jsval type in an upvar type map entry");
        return NULL;
    }
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETFCSLOT()
{
    JSObject& callee = cx->fp()->callee();
    LIns* callee_ins = get(&cx->fp()->calleev());

    LIns* upvars_ins = w.getObjPrivatizedSlot(callee_ins, JSObject::JSSLOT_FLAT_CLOSURE_UPVARS);

    unsigned index = GET_UINT16(cx->regs().pc);
    LIns *v_ins = unbox_value(callee.getFlatClosureUpvar(index),
                              FCSlotsAddress(upvars_ins, index),
                              snapshot(BRANCH_EXIT));
    stack(0, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLFCSLOT()
{
    CHECK_STATUS_A(record_JSOP_GETFCSLOT());
    stack(1, w.immiUndefined());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::guardCallee(Value& callee)
{
    JSObject& callee_obj = callee.toObject();
    JS_ASSERT(callee_obj.isFunction());
    JSFunction* callee_fun = (JSFunction*) callee_obj.getPrivate();

    





    VMSideExit* branchExit = snapshot(BRANCH_EXIT);
    LIns* callee_ins = get(&callee);
    tree->gcthings.addUnique(callee);

    guard(true,
          w.eqp(w.ldpObjPrivate(callee_ins), w.nameImmpNonGC(callee_fun)),
          branchExit);

    





















    if (callee_fun->isInterpreted() &&
        (!FUN_NULL_CLOSURE(callee_fun) || callee_fun->script()->bindings.hasUpvars())) {
        JSObject* parent = callee_obj.getParent();

        if (parent != globalObj) {
            if (!parent->isCall())
                RETURN_STOP("closure scoped by neither the global object nor a Call object");

            guard(true,
                  w.eqp(w.ldpObjParent(callee_ins), w.immpObjGC(parent)),
                  branchExit);
        }
    }
    return RECORD_CONTINUE;
}







JS_REQUIRES_STACK StackFrame *
TraceRecorder::guardArguments(JSObject *obj, LIns* obj_ins, unsigned *depthp)
{
    JS_ASSERT(obj->isArguments());

    StackFrame *afp = frameIfInRange(obj, depthp);
    if (!afp)
        return NULL;

    VMSideExit *exit = snapshot(MISMATCH_EXIT);
    guardClass(obj_ins, obj->getClass(), exit, LOAD_CONST);

    LIns* args_ins = getFrameObjPtr(afp->addressOfArgs());
    LIns* cmp = w.eqp(args_ins, obj_ins);
    guard(true, cmp, exit);
    return afp;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::createThis(JSObject& ctor, LIns* ctor_ins, LIns** thisobj_insp)
{
    JS_ASSERT(ctor.getFunctionPrivate()->isInterpreted());
    if (ctor.getFunctionPrivate()->isFunctionPrototype())
        RETURN_STOP("new Function.prototype");
    if (ctor.isBoundFunction())
        RETURN_STOP("new applied to bound function");

    
    
    const Shape *shape = LookupInterpretedFunctionPrototype(cx, &ctor);
    if (!shape)
        RETURN_ERROR("new f: error resolving f.prototype");

    
    
    
    
    
    
    
    if (!ctor_ins->isImmP())
        guardShape(ctor_ins, &ctor, ctor.shape(), "ctor_shape", snapshot(MISMATCH_EXIT));

    
    
    
    uintN protoSlot = shape->slot;
    LIns* args[] = { w.nameImmw(protoSlot), ctor_ins, cx_ins };
    *thisobj_insp = w.call(&js_CreateThisFromTrace_ci, args);
    guard(false, w.eqp0(*thisobj_insp), OOM_EXIT);
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::interpretedFunctionCall(Value& fval, JSFunction* fun, uintN argc, bool constructing)
{
    





    if (fun->script()->isEmpty()) {
        LIns* rval_ins;
        if (constructing)
            CHECK_STATUS(createThis(fval.toObject(), get(&fval), &rval_ins));
        else
            rval_ins = w.immiUndefined();
        stack(-2 - argc, rval_ins);
        return RECORD_CONTINUE;
    }

    if (fval.toObject().getGlobal() != globalObj)
        RETURN_STOP("JSOP_CALL or JSOP_NEW crosses global scopes");

    StackFrame* const fp = cx->fp();

    if (constructing) {
        LIns* thisobj_ins;
        CHECK_STATUS(createThis(fval.toObject(), get(&fval), &thisobj_ins));
        stack(-int(argc) - 1, thisobj_ins);
    }

    
    unsigned stackSlots = NativeStackSlots(cx, 0 );
    FrameInfo* fi = (FrameInfo*)
        tempAlloc().alloc(sizeof(FrameInfo) + stackSlots * sizeof(JSValueType));
    JSValueType* typemap = (JSValueType*)(fi + 1);

    DetermineTypesVisitor detVisitor(*this, typemap);
    VisitStackSlots(detVisitor, cx, 0);

    JS_ASSERT(argc < FrameInfo::CONSTRUCTING_FLAG);

    tree->gcthings.addUnique(fval);
    fi->pc = cx->regs().pc;
    fi->imacpc = fp->maybeImacropc();
    fi->spdist = cx->regs().sp - fp->slots();
    fi->set_argc(uint16(argc), constructing);
    fi->callerHeight = stackSlots - (2 + argc);
    fi->callerArgc = fp->hasArgs() ? fp->numActualArgs() : 0;

    if (callDepth >= tree->maxCallDepth)
        tree->maxCallDepth = callDepth + 1;

    fi = traceMonitor->frameCache->memoize(fi);
    if (!fi)
        RETURN_STOP("out of memory");
    w.stRstack(w.nameImmpNonGC(fi), lirbuf->rp, callDepth * sizeof(FrameInfo*));

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




static inline JSOp
GetCallMode(StackFrame *fp)
{
    if (fp->hasImacropc()) {
        JSOp op = (JSOp) *fp->imacropc();
        if (op == JSOP_FUNAPPLY || op == JSOP_FUNCALL)
            return op;
    }
    return JSOP_CALL;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALL()
{
    uintN argc = GET_ARGC(cx->regs().pc);
    cx->assertValidStackDepth(argc + 2);
    return InjectStatus(functionCall(argc, GetCallMode(cx->fp())));
}

static jsbytecode* funapply_imacro_table[] = {
    funapply_imacros.apply0,
    funapply_imacros.apply1,
    funapply_imacros.apply2,
    funapply_imacros.apply3,
    funapply_imacros.apply4,
    funapply_imacros.apply5,
    funapply_imacros.apply6,
    funapply_imacros.apply7,
    funapply_imacros.apply8
};

static jsbytecode* funcall_imacro_table[] = {
    funcall_imacros.call0,
    funcall_imacros.call1,
    funcall_imacros.call2,
    funcall_imacros.call3,
    funcall_imacros.call4,
    funcall_imacros.call5,
    funcall_imacros.call6,
    funcall_imacros.call7,
    funcall_imacros.call8
};

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FUNCALL()
{
    return record_JSOP_FUNAPPLY();
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FUNAPPLY()
{
    jsbytecode *pc = cx->regs().pc;
    uintN argc = GET_ARGC(pc);
    cx->assertValidStackDepth(argc + 2);

    Value* vp = cx->regs().sp - (argc + 2);
    jsuint length = 0;
    JSObject* aobj = NULL;
    LIns* aobj_ins = NULL;

    JS_ASSERT(!cx->fp()->hasImacropc());

    if (!IsFunctionObject(vp[0]))
        return record_JSOP_CALL();
    RETURN_IF_XML_A(vp[0]);

    JSObject* obj = &vp[0].toObject();
    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, obj);
    if (FUN_INTERPRETED(fun))
        return record_JSOP_CALL();

    bool apply = fun->u.n.native == js_fun_apply;
    if (!apply && fun->u.n.native != js_fun_call)
        return record_JSOP_CALL();

    



    if (argc > 0 && !vp[2].isObjectOrNull())
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
                  w.eqiN(w.lduiObjPrivate(aobj_ins), length),
                  BRANCH_EXIT);
        } else if (aobj->isArguments()) {
            unsigned depth;
            StackFrame *afp = guardArguments(aobj, aobj_ins, &depth);
            if (!afp)
                RETURN_STOP_A("can't reach arguments object's frame");
            if (aobj->isArgsLengthOverridden())
                RETURN_STOP_A("can't trace arguments with overridden length");
            guardArgsLengthNotAssigned(aobj_ins);
            length = afp->numActualArgs();
        } else {
            RETURN_STOP_A("arguments parameter of apply is not a dense array or argments object");
        }

        if (length >= JS_ARRAY_LENGTH(funapply_imacro_table))
            RETURN_STOP_A("too many arguments to apply");

        return InjectStatus(callImacro(funapply_imacro_table[length]));
    }

    if (argc >= JS_ARRAY_LENGTH(funcall_imacro_table))
        RETURN_STOP_A("too many arguments to call");

    return InjectStatus(callImacro(funcall_imacro_table[argc]));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_NativeCallComplete()
{
    if (pendingSpecializedNative == IGNORE_NATIVE_CALL_COMPLETE_CALLBACK)
        return ARECORD_CONTINUE;

#ifdef DEBUG
    JS_ASSERT(pendingSpecializedNative);
    jsbytecode* pc = cx->regs().pc;
    JS_ASSERT(*pc == JSOP_CALL || *pc == JSOP_FUNCALL || *pc == JSOP_FUNAPPLY ||
              *pc == JSOP_NEW || *pc == JSOP_SETPROP);
#endif

    Value& v = stackval(-1);
    LIns* v_ins = get(&v);

    













    if (JSTN_ERRTYPE(pendingSpecializedNative) == FAIL_STATUS) {
        leaveDeepBailCall();

        LIns* status = w.ldiStateField(builtinStatus);
        if (pendingSpecializedNative == &generatedSpecializedNative) {
            LIns* ok_ins = v_ins;

            





            Address nativeRvalAddr = AllocSlotsAddress(native_rval_ins);
            if (pendingSpecializedNative->flags & JSTN_CONSTRUCTOR) {
                LIns *cond_ins;
                LIns *x;

                
                
                unbox_any_object(nativeRvalAddr, &v_ins, &cond_ins);
                
                x = w.cmovp(cond_ins, v_ins, w.immw(0));
                
                
                v_ins = w.cmovp(w.eqp0(x), newobj_ins, x);
            } else {
                v_ins = w.ldd(nativeRvalAddr);
            }
            set(&v, v_ins);

            propagateFailureToBuiltinStatus(ok_ins, status);
        }
        guard(true, w.eqi0(status), STATUS_EXIT);
    }

    if (pendingSpecializedNative->flags & JSTN_UNBOX_AFTER) {
        




        JS_ASSERT(&v == &cx->regs().sp[-1] && get(&v) == v_ins);
        set(&v, unbox_value(v, AllocSlotsAddress(native_rval_ins), snapshot(BRANCH_EXIT)));
    } else if (pendingSpecializedNative->flags &
               (JSTN_RETURN_NULLABLE_STR | JSTN_RETURN_NULLABLE_OBJ)) {
        guard(v.isNull(),
              w.name(w.eqp0(v_ins), "guard(nullness)"),
              BRANCH_EXIT);
    } else if (JSTN_ERRTYPE(pendingSpecializedNative) == FAIL_NEG) {
        
        JS_ASSERT(v.isNumber());
    } else {
        
        if (v.isNumber() &&
            pendingSpecializedNative->builtin->returnType() == ARGTYPE_I) {
            set(&v, w.i2d(v_ins));
        }
    }

    
    
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::name(Value*& vp, LIns*& ins, NameResult& nr)
{
    JSObject* obj = &cx->fp()->scopeChain();
    JSOp op = JSOp(*cx->regs().pc);
    if (js_CodeSpec[op].format & JOF_GNAME)
        obj = obj->getGlobal();
    if (obj != globalObj)
        return scopeChainProp(obj, vp, ins, nr);

    
    LIns* obj_ins = w.immpObjGC(globalObj);
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

        set(outp, w.immiUndefined());
        return ARECORD_CONTINUE;
    }

    return InjectStatus(propTail(obj, obj_ins, obj2, pcval, slotp, v_insp, outp));
}

JS_REQUIRES_STACK RecordingStatus
TraceRecorder::propTail(JSObject* obj, LIns* obj_ins, JSObject* obj2, PCVal pcval,
                        uint32 *slotp, LIns** v_insp, Value *outp)
{
    const JSCodeSpec& cs = js_CodeSpec[*cx->regs().pc];
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

        













        obj_ins = (obj2 == obj->getProto()) ? w.ldpObjProto(obj_ins) : w.immpObjGC(obj2);
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

    









    if (isMethod && !cx->fp()->hasImacropc()) {
        enterDeepBailCall();
        LIns* args[] = { v_ins, w.immpShapeGC(shape), obj_ins, cx_ins };
        v_ins = w.call(&MethodReadBarrier_ci, args);
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
                                 LIns*& addr_ins, VMSideExit* branchExit)
{
    JS_ASSERT(oval.isObject() && ival.isInt32());

    JSObject* obj = &oval.toObject();
    LIns* obj_ins = get(&oval);
    jsint idx = ival.toInt32();
    LIns* idx_ins;
    CHECK_STATUS(makeNumberInt32(get(&ival), &idx_ins));

    






    LIns* capacity_ins = w.ldiDenseArrayCapacity(obj_ins);
    jsuint capacity = obj->getDenseArrayCapacity();
    bool within = (jsuint(idx) < capacity);
    if (!within) {
        
        guard(true, w.geui(idx_ins, capacity_ins), branchExit);

        CHECK_STATUS(guardPrototypeHasNoIndexedProperties(obj, obj_ins, snapshot(MISMATCH_EXIT)));

        v_ins = w.immiUndefined();
        addr_ins = NULL;
        return RECORD_CONTINUE;
    }

    
    guard(true, w.name(w.ltui(idx_ins, capacity_ins), "inRange"), branchExit);

    
    vp = &obj->slots[jsuint(idx)];
	JS_ASSERT(sizeof(Value) == 8); 
    addr_ins = w.name(w.getDslotAddress(obj_ins, idx_ins), "elemp");
    v_ins = unbox_value(*vp, DSlotsAddress(addr_ins), branchExit);

    
    if (vp->isMagic()) {
        CHECK_STATUS(guardPrototypeHasNoIndexedProperties(obj, obj_ins, snapshot(MISMATCH_EXIT)));
        v_ins = w.immiUndefined();
        addr_ins = NULL;
    }
    return RECORD_CONTINUE;
}


LIns *
TraceRecorder::canonicalizeNaNs(LIns *dval_ins)
{
    
    LIns *isnonnan_ins = w.eqd(dval_ins, dval_ins);
    return w.cmovd(isnonnan_ins, dval_ins, w.immd(js_NaN));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::typedArrayElement(Value& oval, Value& ival, Value*& vp, LIns*& v_ins)
{
    JS_ASSERT(oval.isObject() && ival.isInt32());

    JSObject* obj = &oval.toObject();
    LIns* obj_ins = get(&oval);
    jsint idx = ival.toInt32();
    LIns* idx_ins;
    CHECK_STATUS_A(makeNumberInt32(get(&ival), &idx_ins));
    LIns* pidx_ins = w.ui2p(idx_ins);

    js::TypedArray* tarray = js::TypedArray::fromJSObject(obj);
    JS_ASSERT(tarray);

    
    LIns* priv_ins = w.ldpObjPrivate(obj_ins);

    
    if ((jsuint) idx >= tarray->length)
        RETURN_STOP_A("out-of-range index on typed array");

    








    guard(true,
          w.name(w.ltui(idx_ins, w.ldiConstTypedArrayLength(priv_ins)), "inRange"),
          BRANCH_EXIT);

    

    LIns* data_ins = w.ldpConstTypedArrayData(priv_ins);

    switch (tarray->type) {
      case js::TypedArray::TYPE_INT8:
        v_ins = w.i2d(w.ldc2iTypedArrayElement(data_ins, pidx_ins));
        break;
      case js::TypedArray::TYPE_UINT8:
      case js::TypedArray::TYPE_UINT8_CLAMPED:
        
        
        v_ins = w.i2d(w.lduc2uiTypedArrayElement(data_ins, pidx_ins));
        break;
      case js::TypedArray::TYPE_INT16:
        v_ins = w.i2d(w.lds2iTypedArrayElement(data_ins, pidx_ins));
        break;
      case js::TypedArray::TYPE_UINT16:
        
        
        v_ins = w.i2d(w.ldus2uiTypedArrayElement(data_ins, pidx_ins));
        break;
      case js::TypedArray::TYPE_INT32:
        v_ins = w.i2d(w.ldiTypedArrayElement(data_ins, pidx_ins));
        break;
      case js::TypedArray::TYPE_UINT32:
        v_ins = w.ui2d(w.ldiTypedArrayElement(data_ins, pidx_ins));
        break;
      case js::TypedArray::TYPE_FLOAT32:
        v_ins = canonicalizeNaNs(w.ldf2dTypedArrayElement(data_ins, pidx_ins));
        break;
      case js::TypedArray::TYPE_FLOAT64:
        v_ins = canonicalizeNaNs(w.lddTypedArrayElement(data_ins, pidx_ins));
        break;
      default:
        JS_NOT_REACHED("Unknown typed array type in tracer");
    }

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::getProp(JSObject* obj, LIns* obj_ins)
{
    JSOp op = JSOp(*cx->regs().pc);
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
    double d = consts[GET_INDEX(cx->regs().pc)].toDouble();
    stack(0, w.immd(d));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_STRING()
{
    JSAtom* atom = atoms[GET_INDEX(cx->regs().pc)];
    stack(0, w.immpAtomGC(atom));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ZERO()
{
    stack(0, w.immd(0));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ONE()
{
    stack(0, w.immd(1));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NULL()
{
    stack(0, w.immpNull());
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
    stack(0, w.immi(0));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_TRUE()
{
    stack(0, w.immi(1));
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
    return InjectStatus(switchop());
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LOOKUPSWITCH()
{
    return InjectStatus(switchop());
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_STRICTEQ()
{
    CHECK_STATUS_A(strictEquality(true, false));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_STRICTNE()
{
    CHECK_STATUS_A(strictEquality(false, false));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_OBJECT()
{
    StackFrame* const fp = cx->fp();
    JSScript* script = fp->script();
    unsigned index = atoms - script->atomMap.vector + GET_INDEX(cx->regs().pc);

    JSObject* obj;
    obj = script->getObject(index);
    stack(0, w.immpObjGC(obj));
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
    stack(0, arg(GET_ARGNO(cx->regs().pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETARG()
{
    arg(GET_ARGNO(cx->regs().pc), stack(-1));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETLOCAL()
{
    stack(0, var(GET_SLOTNO(cx->regs().pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETLOCAL()
{
    var(GET_SLOTNO(cx->regs().pc), stack(-1));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_UINT16()
{
    stack(0, w.immd(GET_UINT16(cx->regs().pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NEWINIT()
{
    initDepth++;
    hadNewInit = true;

    JSProtoKey key = JSProtoKey(cx->regs().pc[1]);

    LIns* proto_ins;
    CHECK_STATUS_A(getClassPrototype(key, proto_ins));

    LIns *v_ins;
    if (key == JSProto_Array) {
        LIns *args[] = { proto_ins, cx_ins };
        v_ins = w.call(&NewDenseEmptyArray_ci, args);
    } else {
        LIns *args[] = { w.immpNull(), proto_ins, cx_ins };
        v_ins = w.call(&js_InitializerObject_ci, args);
    }
    guard(false, w.eqp0(v_ins), OOM_EXIT);
    stack(0, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NEWARRAY()
{
    initDepth++;

    LIns* proto_ins;
    CHECK_STATUS_A(getClassPrototype(JSProto_Array, proto_ins));

    unsigned count = GET_UINT24(cx->regs().pc);
    LIns *args[] = { proto_ins, w.immi(count), cx_ins };
    LIns *v_ins = w.call(&NewDenseAllocatedArray_ci, args);

    guard(false, w.eqp0(v_ins), OOM_EXIT);
    stack(0, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NEWOBJECT()
{
    initDepth++;

    LIns* proto_ins;
    CHECK_STATUS_A(getClassPrototype(JSProto_Object, proto_ins));

    JSObject* baseobj = cx->fp()->script()->getObject(getFullIndex(0));

    LIns *args[] = { w.immpObjGC(baseobj), proto_ins, cx_ins };
    LIns *v_ins = w.call(&js_InitializerObject_ci, args);

    guard(false, w.eqp0(v_ins), OOM_EXIT);
    stack(0, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ENDINIT()
{
    initDepth--;
    if (initDepth == 0)
        hadNewInit = false;

#ifdef DEBUG
    Value& v = stackval(-1);
    JS_ASSERT(!v.isPrimitive());
#endif
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INITELEM()
{
    Value& v = stackval(-1);
    Value& idx = stackval(-2);
    Value& lval = stackval(-3);

    
    
    if (!lval.toObject().isDenseArray() || hadNewInit)
        return setElem(-3, -2, -1);

    
    JS_ASSERT(idx.isInt32());

    
    if (v.isMagic(JS_ARRAY_HOLE))
        return ARECORD_CONTINUE;

    LIns* obj_ins = get(&lval);
    LIns* v_ins = get(&v);

    
    LIns *slots_ins = w.ldpObjSlots(obj_ins);
    box_value_into(v, v_ins, DSlotsAddress(slots_ins, idx.toInt32()));

    return ARECORD_CONTINUE;
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
    return InjectStatus(inc(argval(GET_ARGNO(cx->regs().pc)), 1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INCLOCAL()
{
    return InjectStatus(inc(varval(GET_SLOTNO(cx->regs().pc)), 1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DECARG()
{
    return InjectStatus(inc(argval(GET_ARGNO(cx->regs().pc)), -1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_DECLOCAL()
{
    return InjectStatus(inc(varval(GET_SLOTNO(cx->regs().pc)), -1));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARGINC()
{
    return InjectStatus(inc(argval(GET_ARGNO(cx->regs().pc)), 1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LOCALINC()
{
    return InjectStatus(inc(varval(GET_SLOTNO(cx->regs().pc)), 1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARGDEC()
{
    return InjectStatus(inc(argval(GET_ARGNO(cx->regs().pc)), -1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LOCALDEC()
{
    return InjectStatus(inc(varval(GET_SLOTNO(cx->regs().pc)), -1, false));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_IMACOP()
{
    JS_ASSERT(cx->fp()->hasImacropc());
    return ARECORD_CONTINUE;
}

static JSBool FASTCALL
ObjectToIterator(JSContext* cx, JSObject *obj, int32 flags, Value* vp)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    vp->setObject(*obj);
    bool ok = js_ValueToIterator(cx, flags, vp);
    if (!ok) {
        SetBuiltinError(tm);
        return false;
    }
    return WasBuiltinSuccessful(tm);
}
JS_DEFINE_CALLINFO_4(static, BOOL_FAIL, ObjectToIterator, CONTEXT, OBJECT, INT32, VALUEPTR,
                     0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ITER()
{
    Value& v = stackval(-1);
    if (v.isPrimitive())
        RETURN_STOP_A("for-in on a primitive value");

    RETURN_IF_XML_A(v);

    LIns *obj_ins = get(&v);
    jsuint flags = cx->regs().pc[1];

    enterDeepBailCall();

    LIns* vp_ins = w.allocp(sizeof(Value));
    LIns* args[] = { vp_ins, w.immi(flags), obj_ins, cx_ins };
    LIns* ok_ins = w.call(&ObjectToIterator_ci, args);

    
    
    pendingGuardCondition = ok_ins;

    
    
    
    
    pendingUnboxSlot = cx->regs().sp - 1;
    set(pendingUnboxSlot, w.name(w.lddAlloc(vp_ins), "iterval"));

    leaveDeepBailCall();

    return ARECORD_CONTINUE;
}

static JSBool FASTCALL
IteratorMore(JSContext *cx, JSObject *iterobj, Value *vp)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    if (!js_IteratorMore(cx, iterobj, vp)) {
        SetBuiltinError(tm);
        return false;
    }
    return WasBuiltinSuccessful(tm);
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
    LIns* cond_ins;

    





    if (iterobj->hasClass(&js_IteratorClass)) {
        guardClass(iterobj_ins, &js_IteratorClass, snapshot(BRANCH_EXIT), LOAD_NORMAL);

        NativeIterator *ni = (NativeIterator *) iterobj->getPrivate();
        if (ni->isKeyIter()) {
            LIns *ni_ins = w.ldpObjPrivate(iterobj_ins);
            LIns *cursor_ins = w.ldpIterCursor(ni_ins);
            LIns *end_ins = w.ldpIterEnd(ni_ins);

            cond_ins = w.ltp(cursor_ins, end_ins);
            stack(0, cond_ins);
            return ARECORD_CONTINUE;
        }
    } else {
        guardNotClass(iterobj_ins, &js_IteratorClass, snapshot(BRANCH_EXIT), LOAD_NORMAL);
    }

    enterDeepBailCall();

    LIns* vp_ins = w.allocp(sizeof(Value));
    LIns* args[] = { vp_ins, iterobj_ins, cx_ins };
    pendingGuardCondition = w.call(&IteratorMore_ci, args);

    leaveDeepBailCall();

    cond_ins = is_boxed_true(AllocSlotsAddress(vp_ins));
    stack(0, cond_ins);

    
    
    stack(-1, iterobj_ins);

    return ARECORD_CONTINUE;
}

static JSBool FASTCALL
CloseIterator(JSContext *cx, JSObject *iterobj)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    if (!js_CloseIterator(cx, iterobj)) {
        SetBuiltinError(tm);
        return false;
    }
    return WasBuiltinSuccessful(tm);
}
JS_DEFINE_CALLINFO_2(extern, BOOL_FAIL, CloseIterator, CONTEXT, OBJECT, 0, ACCSET_STORE_ANY)

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ENDITER()
{
    JS_ASSERT(!stackval(-1).isPrimitive());

    enterDeepBailCall();

    LIns* args[] = { stack(-1), cx_ins };
    LIns* ok_ins = w.call(&CloseIterator_ci, args);

    
    
    pendingGuardCondition = ok_ins;

    leaveDeepBailCall();

    return ARECORD_CONTINUE;
}

#if JS_BITS_PER_WORD == 32
JS_REQUIRES_STACK void
TraceRecorder::storeMagic(JSWhyMagic why, Address addr)
{
    w.stiValuePayload(w.immpMagicWhy(why), addr);
    w.stiValueTag(w.immpMagicWhy(JSVAL_TAG_MAGIC), addr);
}
#elif JS_BITS_PER_WORD == 64
JS_REQUIRES_STACK void
TraceRecorder::storeMagic(JSWhyMagic why, Address addr)
{
    LIns *magic = w.nameImmq(BUILD_JSVAL(JSVAL_TAG_MAGIC, why));
    w.stq(magic, addr);
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

        LIns *ni_ins = w.ldpObjPrivate(iterobj_ins);
        LIns *cursor_ins = w.ldpIterCursor(ni_ins);

        
        Address cursorAddr = IterPropsAddress(cursor_ins);
        if (ni->isKeyIter()) {
            
            jsid id = *ni->current();
            LIns *id_ins = w.name(w.ldp(cursorAddr), "id");

            



            guard(JSID_IS_STRING(id), is_string_id(id_ins), BRANCH_EXIT);

            if (JSID_IS_STRING(id)) {
                v_ins = unbox_string_id(id_ins);
            } else if (JSID_IS_INT(id)) {
                
                LIns *id_to_int_ins = unbox_int_id(id_ins);
                LIns* args[] = { id_to_int_ins, cx_ins };
                v_ins = w.call(&js_IntToString_ci, args);
                guard(false, w.eqp0(v_ins), OOM_EXIT);
            } else {
#if JS_HAS_XML_SUPPORT
                JS_ASSERT(JSID_IS_OBJECT(id));
                JS_ASSERT(JSID_TO_OBJECT(id)->isXMLId());
                RETURN_STOP_A("iterated over a property with an XML id");
#else
                JS_NEVER_REACHED("unboxNextValue");
#endif
            }

            
            cursor_ins = w.addp(cursor_ins, w.nameImmw(sizeof(jsid)));
            w.stpIterCursor(cursor_ins, ni_ins);
            return ARECORD_CONTINUE;
        }
    } else {
        guardNotClass(iterobj_ins, &js_IteratorClass, snapshot(BRANCH_EXIT), LOAD_NORMAL);
    }


    Address iterValueAddr = CxAddress(iterValue);
    v_ins = unbox_value(cx->iterValue, iterValueAddr, snapshot(BRANCH_EXIT));
    storeMagic(JS_NO_ITER_VALUE, iterValueAddr);

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
TraceRecorder::record_JSOP_FORGNAME()
{
    return record_JSOP_FORNAME();
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
    arg(GET_ARGNO(cx->regs().pc), v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_FORLOCAL()
{
    LIns* v_ins;
    CHECK_STATUS_A(unboxNextValue(v_ins));
    var(GET_SLOTNO(cx->regs().pc), v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_POPN()
{
    return ARECORD_CONTINUE;
}

static inline bool
IsFindableCallObj(JSObject *obj)
{
    return obj->isCall() &&
           (obj->callIsForEval() || obj->getCallObjCalleeFunction()->isHeavyweight());
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
            if (searchObj->isBlock())
                foundBlockObj = true;
            else if (IsFindableCallObj(searchObj))
                foundCallObj = true;
        }

        if (searchObj == targetObj)
            break;

        searchObj = searchObj->getParent();
        if (!searchObj)
            RETURN_STOP("cannot traverse this scope chain on trace");
    }

    if (!foundCallObj) {
        JS_ASSERT(targetObj == globalObj);
        targetIns = w.nameImmpNonGC(globalObj);
        return RECORD_CONTINUE;
    }

    if (foundBlockObj)
        RETURN_STOP("cannot traverse this scope chain on trace");

    
    for (;;) {
        if (obj != globalObj) {
            if (!IsCacheableNonGlobalScope(obj))
                RETURN_STOP("scope chain lookup crosses non-cacheable object");

            
            
            
            if (IsFindableCallObj(obj)) {
                if (!exit)
                    exit = snapshot(BRANCH_EXIT);
                guard(true,
                      w.name(w.eqiN(w.ldiObjShape(obj_ins), obj->shape()), "guard_shape"),
                      exit);
            }
        }

        JS_ASSERT(!obj->isBlock());

        if (obj == targetObj)
            break;

        obj = obj->getParent();
        obj_ins = w.ldpObjParent(obj_ins);
    }

    targetIns = obj_ins;
    return RECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BINDNAME()
{
    TraceMonitor *localtm = traceMonitor;
    StackFrame* const fp = cx->fp();
    JSObject *obj;

    if (!fp->isFunctionFrame()) {
        obj = &fp->scopeChain();

#ifdef DEBUG
        StackFrame *fp2 = fp;
#endif

        



        while (obj->isBlock()) {
            
#ifdef DEBUG
            
            while (obj->getPrivate() != fp2) {
                JS_ASSERT(fp2->isEvalOrDebuggerFrame());
                fp2 = fp2->prev();
                if (!fp2)
                    JS_NOT_REACHED("bad stack frame");
            }
#endif
            obj = obj->getParent();
            
            JS_ASSERT(obj);
        }

        



        if (obj != globalObj) {
            JS_ASSERT(obj->isCall());
            JS_ASSERT(obj->callIsForEval());
            RETURN_STOP_A("BINDNAME within strict eval code");
        }

        







        stack(0, w.immpObjGC(obj));
        return ARECORD_CONTINUE;
    }

    
    
    
    if (JSFUN_HEAVYWEIGHT_TEST(fp->fun()->flags))
        RETURN_STOP_A("BINDNAME in heavyweight function.");

    
    
    
    Value *callee = &cx->fp()->calleev();
    obj = callee->toObject().getParent();
    if (obj == globalObj) {
        stack(0, w.immpObjGC(obj));
        return ARECORD_CONTINUE;
    }
    LIns *obj_ins = w.ldpObjParent(get(callee));

    
    JSAtom *atom = atoms[GET_INDEX(cx->regs().pc)];
    jsid id = ATOM_TO_JSID(atom);
    JSObject *obj2 = js_FindIdentifierBase(cx, &fp->scopeChain(), id);
    if (!obj2)
        RETURN_ERROR_A("error in js_FindIdentifierBase");
    if (!localtm->recorder)
        return ARECORD_ABORTED;
    if (obj2 != globalObj && !obj2->isCall())
        RETURN_STOP_A("BINDNAME on non-global, non-call object");

    
    LIns *obj2_ins;
    CHECK_STATUS_A(traverseScopeChain(obj, obj_ins, obj2, obj2_ins));

    
    
    stack(0, obj2 == globalObj ? w.immpObjGC(obj2) : obj2_ins);
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

        if (obj->isDenseArray()) {
            
            VMSideExit* branchExit = snapshot(BRANCH_EXIT);
            guardDenseArray(obj_ins, branchExit);

            
            
            
            CHECK_STATUS_A(guardPrototypeHasNoIndexedProperties(obj, obj_ins,
                                                                snapshot(MISMATCH_EXIT)));

            LIns* idx_ins;
            CHECK_STATUS_A(makeNumberInt32(get(&lval), &idx_ins));
            idx_ins = w.name(idx_ins, "index");
            LIns* capacity_ins = w.ldiDenseArrayCapacity(obj_ins);
            LIns* inRange = w.ltui(idx_ins, capacity_ins);

            if (jsuint(lval.toInt32()) < obj->getDenseArrayCapacity()) {
                guard(true, inRange, branchExit);

                LIns *elem_ins = w.getDslotAddress(obj_ins, idx_ins);
                
                LIns *is_hole_ins =
                    is_boxed_magic(DSlotsAddress(elem_ins), JS_ARRAY_HOLE);

                
                x = w.eqi0(is_hole_ins);
            } else {
                guard(false, inRange, branchExit);
                x = w.nameImmi(0);
            }
        } else {
            LIns* num_ins;
            CHECK_STATUS_A(makeNumberInt32(get(&lval), &num_ins));
            LIns* args[] = { num_ins, obj_ins, cx_ins };
            x = w.call(&js_HasNamedPropertyInt32_ci, args);
        }
    } else if (lval.isString()) {
        if (!js_ValueToStringId(cx, lval, &id))
            RETURN_ERROR_A("left operand of JSOP_IN didn't convert to a string-id");
        LIns* args[] = { get(&lval), obj_ins, cx_ins };
        x = w.call(&js_HasNamedProperty_ci, args);
    } else {
        RETURN_STOP_A("string or integer expected");
    }

    guard(false, w.eqiN(x, JS_NEITHER), OOM_EXIT);
    x = w.eqiN(x, 1);

    TraceMonitor &localtm = *traceMonitor;

    JSObject* obj2;
    JSProperty* prop;
    JSBool ok = obj->lookupProperty(cx, id, &obj2, &prop);

    if (!ok)
        RETURN_ERROR_A("obj->lookupProperty failed in JSOP_IN");

    
    if (!localtm.recorder)
        return ARECORD_ABORTED;

    bool cond = prop != NULL;

    



    jsbytecode *pc = cx->regs().pc;
    fuseIf(pc + 1, cond, x);

    
    if (pc[1] == JSOP_IFNE || pc[1] == JSOP_IFEQ)
        CHECK_STATUS_A(checkTraceEnd(pc + 1));

    





    set(&lval, x);
    return ARECORD_CONTINUE;
}

static JSBool FASTCALL
HasInstanceOnTrace(JSContext* cx, JSObject* ctor, ValueArgType arg)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_ON_TRACE(cx);

    const Value &argref = ValueArgToConstRef(arg);
    JSBool result = JS_FALSE;
    if (!HasInstance(cx, ctor, &argref, &result))
        SetBuiltinError(tm);
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
    stack(-2, w.call(&HasInstanceOnTrace_ci, args));
    LIns* status_ins = w.ldiStateField(builtinStatus);
    pendingGuardCondition = w.eqi0(status_ins);
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
TraceRecorder::record_JSOP_BLOCKCHAIN()
{
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_NULLBLOCKCHAIN()
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
    CHECK_STATUS_A(strictEquality(true, true));
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
    jsatomid index = GET_INDEX(cx->regs().pc + pcoff);
    index += atoms - cx->fp()->script()->atomMap.vector;
    return index;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LAMBDA()
{
    JSFunction* fun;
    fun = cx->fp()->script()->getFunction(getFullIndex());

    if (FUN_NULL_CLOSURE(fun) && FUN_OBJECT(fun)->getParent() != globalObj)
        RETURN_STOP_A("Null closure function object parent must be global object");

    










    if (FUN_NULL_CLOSURE(fun) && FUN_OBJECT(fun)->getParent() == &cx->fp()->scopeChain()) {
        jsbytecode *pc2 = AdvanceOverBlockchainOp(cx->regs().pc + JSOP_LAMBDA_LENGTH);
        JSOp op2 = JSOp(*pc2);

        if (op2 == JSOP_INITMETHOD) {
            stack(0, w.immpObjGC(FUN_OBJECT(fun)));
            return ARECORD_CONTINUE;
        }

        if (op2 == JSOP_SETMETHOD) {
            Value lval = stackval(-1);

            if (!lval.isPrimitive() && lval.toObject().canHaveMethodBarrier()) {
                stack(0, w.immpObjGC(FUN_OBJECT(fun)));
                return ARECORD_CONTINUE;
            }
        } else if (fun->joinable()) {
            if (op2 == JSOP_CALL) {
                






                int iargc = GET_ARGC(pc2);

                




                const Value &cref = cx->regs().sp[1 - (iargc + 2)];
                JSObject *callee;

                if (IsFunctionObject(cref, &callee)) {
                    JSFunction *calleeFun = callee->getFunctionPrivate();
                    Native native = calleeFun->maybeNative();

                    if ((iargc == 1 && native == array_sort) ||
                        (iargc == 2 && native == str_replace)) {
                        stack(0, w.immpObjGC(FUN_OBJECT(fun)));
                        return ARECORD_CONTINUE;
                    }
                }
            } else if (op2 == JSOP_NULL) {
                pc2 += JSOP_NULL_LENGTH;
                op2 = JSOp(*pc2);

                if (op2 == JSOP_CALL && GET_ARGC(pc2) == 0) {
                    stack(0, w.immpObjGC(FUN_OBJECT(fun)));
                    return ARECORD_CONTINUE;
                }
            }
        }

        LIns *proto_ins;
        CHECK_STATUS_A(getClassPrototype(JSProto_Function, proto_ins));

        LIns* args[] = { w.immpObjGC(globalObj), proto_ins, w.immpFunGC(fun), cx_ins };
        LIns* x = w.call(&js_NewNullClosure_ci, args);
        stack(0, x);
        return ARECORD_CONTINUE;
    }

    if (GetBlockChainFast(cx, cx->fp(), JSOP_LAMBDA, JSOP_LAMBDA_LENGTH))
        RETURN_STOP_A("Unable to trace creating lambda in let");

    LIns *proto_ins;
    CHECK_STATUS_A(getClassPrototype(JSProto_Function, proto_ins));
    LIns* scopeChain_ins = scopeChain();
    JS_ASSERT(scopeChain_ins);
    LIns* args[] = { proto_ins, scopeChain_ins, w.nameImmpNonGC(fun), cx_ins };
    LIns* call_ins = w.call(&js_CloneFunctionObject_ci, args);
    guard(false,
          w.name(w.eqp0(call_ins), "guard(js_CloneFunctionObject)"),
          OOM_EXIT);
    stack(0, call_ins);

    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LAMBDA_FC()
{
    JSFunction* fun;
    fun = cx->fp()->script()->getFunction(getFullIndex());

    if (FUN_OBJECT(fun)->getParent() != globalObj)
        return ARECORD_STOP;

    if (GetBlockChainFast(cx, cx->fp(), JSOP_LAMBDA_FC, JSOP_LAMBDA_FC_LENGTH))
        RETURN_STOP_A("Unable to trace creating lambda in let");

    LIns* args[] = { scopeChain(), w.immpFunGC(fun), cx_ins };
    LIns* closure_ins = w.call(&js_AllocFlatClosure_ci, args);
    guard(false,
          w.name(w.eqp(closure_ins, w.immpNull()), "guard(js_AllocFlatClosure)"),
          OOM_EXIT);

    JSScript *script = fun->script();
    if (script->bindings.hasUpvars()) {
        JSUpvarArray *uva = script->upvars();
        LIns* upvars_ins = w.getObjPrivatizedSlot(closure_ins,
                                                  JSObject::JSSLOT_FLAT_CLOSURE_UPVARS);

        for (uint32 i = 0, n = uva->length; i < n; i++) {
            Value v;
            LIns* v_ins = upvar(script, uva, i, v);
            if (!v_ins)
                return ARECORD_STOP;

            box_value_into(v, v_ins, FCSlotsAddress(upvars_ins, i));
        }
    }

    stack(0, closure_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLEE()
{
    stack(0, get(&cx->fp()->calleev()));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_SETLOCALPOP()
{
    var(GET_SLOTNO(cx->regs().pc), stack(-1));
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
    StackFrame* const fp = cx->fp();

    





    if (!fp->hasArgsObj() && !fp->fun()->isHeavyweight()) {
        uintN slot = GET_ARGNO(cx->regs().pc);
        if (slot >= fp->numActualArgs())
            RETURN_STOP_A("can't trace out-of-range arguments");

        stack(0, get(&cx->fp()->canonicalActualArg(slot)));
        return ARECORD_CONTINUE;
    }
    RETURN_STOP_A("can't trace JSOP_ARGSUB hard case");
}

JS_REQUIRES_STACK LIns*
TraceRecorder::guardArgsLengthNotAssigned(LIns* argsobj_ins)
{
    
    
    LIns *len_ins = w.getArgsLength(argsobj_ins);
    LIns *ovr_ins = w.andi(len_ins, w.nameImmi(JSObject::ARGS_LENGTH_OVERRIDDEN_BIT));
    guard(true, w.eqi0(ovr_ins), MISMATCH_EXIT);
    return len_ins;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_ARGCNT()
{
    StackFrame * const fp = cx->fp();

    if (fp->fun()->flags & JSFUN_HEAVYWEIGHT)
        RETURN_STOP_A("can't trace heavyweight JSOP_ARGCNT");

    
    
    
    
    
    
    if (fp->hasArgsObj() && fp->argsObj().isArgsLengthOverridden())
        RETURN_STOP_A("can't trace JSOP_ARGCNT if arguments.length has been modified");
    LIns *a_ins = getFrameObjPtr(fp->addressOfArgs());
    if (callDepth == 0) {
        if (MaybeBranch mbr = w.jt(w.eqp0(a_ins))) {
            guardArgsLengthNotAssigned(a_ins);
            w.label(mbr);
        }
    }
    stack(0, w.immd(fp->numActualArgs()));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_DefLocalFunSetSlot(uint32 slot, JSObject* obj)
{
    JSFunction* fun = GET_FUNCTION_PRIVATE(cx, obj);

    if (FUN_NULL_CLOSURE(fun) && FUN_OBJECT(fun)->getParent() == globalObj) {
        LIns *proto_ins;
        CHECK_STATUS_A(getClassPrototype(JSProto_Function, proto_ins));

        LIns* args[] = { w.immpObjGC(globalObj), proto_ins, w.immpFunGC(fun), cx_ins };
        LIns* x = w.call(&js_NewNullClosure_ci, args);
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
    CHECK_STATUS_A(strictEquality(true, true));
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
    StackFrame* const fp = cx->fp();
    JSScript* script = fp->script();
    unsigned index = atoms - script->atomMap.vector + GET_INDEX(cx->regs().pc);

    LIns* proto_ins;
    CHECK_STATUS_A(getClassPrototype(JSProto_RegExp, proto_ins));

    LIns* args[] = {
        proto_ins,
        w.immpObjGC(script->getRegExp(index)),
        cx_ins
    };
    LIns* regex_ins = w.call(&js_CloneRegExpObject_ci, args);
    guard(false, w.eqp0(regex_ins), OOM_EXIT);

    stack(0, regex_ins);
    return ARECORD_CONTINUE;
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

        obj_ins = w.immpObjGC(obj);
        debug_only_stmt(obj_ins = w.name(obj_ins, protoname);)
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
            if (fun->isInterpreted() && !fun->inStrictMode())
                RETURN_STOP_A("callee does not accept primitive |this|");
        }
        set(&l, w.immpObjGC(&pcval.toFunObj()));
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
    stack(0, w.immd(GET_UINT24(cx->regs().pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INDEXBASE()
{
    atoms += GET_INDEXBASE(cx->regs().pc);
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
    StackFrame *fp = cx->fp();

    
    if (callDepth == 0 && !fp->hasImacropc()) {
        AUDIT(returnLoopExits);
        return endLoop();
    }

    if (fp->hasImacropc()) {
        




        updateAtoms(fp->script());
        return ARECORD_CONTINUE;
    }

    CHECK_STATUS_A(putActivationObjects());

    if (Probes::callTrackingActive(cx)) {
        LIns* args[] = { w.immi(0), w.nameImmpNonGC(cx->fp()->fun()), cx_ins };
        LIns* call_ins = w.call(&functionProbe_ci, args);
        guard(false, w.eqi0(call_ins), MISMATCH_EXIT);
    }

    







    if (fp->isConstructing()) {
        rval_ins = get(&fp->thisValue());
    } else {
        rval_ins = w.immiUndefined();
    }
    clearReturningFrameFromNativeTracker();
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
    obj = cx->fp()->script()->getObject(getFullIndex(0));

    LIns* void_ins = w.immiUndefined();
    for (int i = 0, n = OBJ_BLOCK_COUNT(cx, obj); i < n; i++)
        stack(i, void_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LEAVEBLOCK()
{
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
    uint32_t slot = GET_UINT16(cx->regs().pc);
    JS_ASSERT(cx->fp()->numFixed() <= slot);
    JS_ASSERT(cx->fp()->slots() + slot < cx->regs().sp - 1);
    Value &arrayval = cx->fp()->slots()[slot];
    JS_ASSERT(arrayval.isObject());
    LIns *array_ins = get(&arrayval);
    Value &elt = stackval(-1);
    LIns *elt_ins = box_value_for_native_call(elt, get(&elt));

    enterDeepBailCall();

    LIns *args[] = { elt_ins, array_ins, cx_ins };
    pendingGuardCondition = w.call(&js_ArrayCompPush_tn_ci, args);

    leaveDeepBailCall();
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
    int n = -1 - GET_UINT16(cx->regs().pc);
    stack(n, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETTHISPROP()
{
    LIns* this_ins;

    CHECK_STATUS_A(getThis(this_ins));

    



    const Value &thisv = cx->fp()->thisValue();
    if (!thisv.isObject())
        RETURN_STOP_A("primitive this for GETTHISPROP");

    CHECK_STATUS_A(getProp(&thisv.toObject(), this_ins));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETARGPROP()
{
    return getProp(argval(GET_ARGNO(cx->regs().pc)));
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_GETLOCALPROP()
{
    return getProp(varval(GET_SLOTNO(cx->regs().pc)));
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
    uintN slot = GET_SLOTNO(cx->regs().pc);
    stack(0, var(slot));
    stack(1, w.immiUndefined());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLARG()
{
    uintN slot = GET_ARGNO(cx->regs().pc);
    stack(0, arg(slot));
    stack(1, w.immiUndefined());
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_BINDGNAME()
{
    stack(0, w.immpObjGC(globalObj));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INT8()
{
    stack(0, w.immd(GET_INT8(cx->regs().pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_INT32()
{
    stack(0, w.immd(GET_INT32(cx->regs().pc)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_LENGTH()
{
    Value& l = stackval(-1);
    if (l.isPrimitive()) {
        if (!l.isString())
            RETURN_STOP_A("non-string primitive JSOP_LENGTH unsupported");
        set(&l, w.i2d(w.p2i(w.getStringLength(get(&l)))));
        return ARECORD_CONTINUE;
    }

    JSObject* obj = &l.toObject();
    LIns* obj_ins = get(&l);

    if (obj->isArguments()) {
        unsigned depth;
        StackFrame *afp = guardArguments(obj, obj_ins, &depth);
        if (!afp)
            RETURN_STOP_A("can't reach arguments object's frame");

        
        
        if (obj->isArgsLengthOverridden())
            RETURN_STOP_A("can't trace JSOP_ARGCNT if arguments.length has been modified");
        LIns* slot_ins = guardArgsLengthNotAssigned(obj_ins);

        
        
        LIns* v_ins = w.i2d(w.rshiN(slot_ins, JSObject::ARGS_PACKED_BITS_COUNT));
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
        v_ins = w.lduiObjPrivate(obj_ins);
        if (obj->getArrayLength() <= JSVAL_INT_MAX) {
            guard(true, w.leui(v_ins, w.immi(JSVAL_INT_MAX)), BRANCH_EXIT);
            v_ins = w.i2d(v_ins);
        } else {
            v_ins = w.ui2d(v_ins);
        }
    } else if (OkToTraceTypedArrays && js_IsTypedArray(obj)) {
        
        guardClass(obj_ins, obj->getClass(), snapshot(BRANCH_EXIT), LOAD_NORMAL);
        v_ins = w.i2d(w.ldiConstTypedArrayLength(w.ldpObjPrivate(obj_ins)));
    } else {
        if (!obj->isNative())
            RETURN_STOP_A("can't trace length property access on non-array, non-native object");
        return getProp(obj, obj_ins);
    }
    set(&l, v_ins);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_HOLE()
{
    stack(0, w.immpMagicWhy(JS_ARRAY_HOLE));
    return ARECORD_CONTINUE;
}

AbortableRecordingStatus
TraceRecorder::record_JSOP_TRACE()
{
    return ARECORD_CONTINUE;
}

AbortableRecordingStatus
TraceRecorder::record_JSOP_NOTRACE()
{
    return ARECORD_CONTINUE;
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
    LIns* call_ins = w.call(&js_Unbrand_ci, args_ins);
    guard(false, w.eqi0(call_ins), OOM_EXIT);
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_UNBRANDTHIS()
{
    
    StackFrame *fp = cx->fp();
    if (fp->fun()->inStrictMode() && !fp->thisValue().isObject())
        return ARECORD_CONTINUE;

    LIns* this_ins;
    RecordingStatus status = getThis(this_ins);
    if (status != RECORD_CONTINUE)
        return InjectStatus(status);

    LIns* args_ins[] = { this_ins, cx_ins };
    LIns* call_ins = w.call(&js_Unbrand_ci, args_ins);
    guard(false, w.eqi0(call_ins), OOM_EXIT);
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
    uint32 slot = cx->fp()->script()->getGlobalSlot(GET_SLOTNO(cx->regs().pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    stack(0, get(&globalObj->getSlotRef(slot)));
    return ARECORD_CONTINUE;
}

JS_REQUIRES_STACK AbortableRecordingStatus
TraceRecorder::record_JSOP_CALLGLOBAL()
{
    uint32 slot = cx->fp()->script()->getGlobalSlot(GET_SLOTNO(cx->regs().pc));
    if (!lazilyImportGlobalSlot(slot))
         RETURN_STOP_A("lazy import of global slot failed");

    Value &v = globalObj->getSlotRef(slot);
    stack(0, get(&v));
    stack(1, w.immiUndefined());
    return ARECORD_CONTINUE;
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
StartTraceVisNative(JSContext *cx, uintN argc, jsval *vp)
{
    JSBool ok;

    if (argc > 0 && JSVAL_IS_STRING(JS_ARGV(cx, vp)[0])) {
        JSString *str = JSVAL_TO_STRING(JS_ARGV(cx, vp)[0]);
        char *filename = js_DeflateString(cx, str->chars(), str->length());
        if (!filename)
            goto error;
        ok = StartTraceVis(filename);
        cx->free_(filename);
    } else {
        ok = StartTraceVis();
    }

    if (ok) {
        fprintf(stderr, "started TraceVis recording\n");
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
        return true;
    }

  error:
    JS_ReportError(cx, "failed to start TraceVis recording");
    return false;
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
StopTraceVisNative(JSContext *cx, uintN argc, jsval *vp)
{
    JSBool ok = StopTraceVis();

    if (ok) {
        fprintf(stderr, "stopped TraceVis recording\n");
        JS_SET_RVAL(cx, vp, JSVAL_VOID);
    } else {
        JS_ReportError(cx, "TraceVis isn't running");
    }

    return ok;
}

#endif 

JS_REQUIRES_STACK void
TraceRecorder::captureStackTypes(unsigned callDepth, JSValueType* typeMap)
{
    CaptureTypesVisitor capVisitor(cx, traceMonitor->oracle, typeMap, !!oracle);
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
    bool* blacklist;

  public:
    AutoRetBlacklist(jsbytecode* pc, bool* blacklist)
      : pc(pc), blacklist(blacklist)
    { }

    ~AutoRetBlacklist()
    {
        *blacklist = IsBlacklisted(pc);
    }
};

JS_REQUIRES_STACK TracePointAction
RecordTracePoint(JSContext* cx, TraceMonitor* tm,
                 uintN& inlineCallCount, bool* blacklist, bool execAllowed)
{
    StackFrame* fp = cx->fp();
    jsbytecode* pc = cx->regs().pc;

    JS_ASSERT(!tm->recorder);
    JS_ASSERT(!tm->profile);

    JSObject* globalObj = cx->fp()->scopeChain().getGlobal();
    uint32 globalShape = -1;
    SlotList* globalSlots = NULL;

    AutoRetBlacklist autoRetBlacklist(pc, blacklist);

    if (!CheckGlobalObjectShape(cx, tm, globalObj, &globalShape, &globalSlots)) {
        Backoff(tm, pc);
        return TPA_Nothing;
    }

    uint32 argc = entryFrameArgc(cx);
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

            if (!execAllowed) {
                
                Blacklist((jsbytecode*)tree->root->ip);
                return TPA_Nothing;
            }

            
            if (!ExecuteTree(cx, tm, match, inlineCallCount, &innermostNestedGuard, &lr))
                return TPA_Error;

            if (!lr)
                return TPA_Nothing;

            switch (lr->exitType) {
              case UNSTABLE_LOOP_EXIT:
                if (!AttemptToStabilizeTree(cx, tm, globalObj, lr, NULL, NULL, 0))
                    return TPA_RanStuff;
                break;

              case MUL_ZERO_EXIT:
              case OVERFLOW_EXIT:
                if (lr->exitType == MUL_ZERO_EXIT)
                    tm->oracle->markInstructionSlowZeroTest(cx->regs().pc);
                else
                    tm->oracle->markInstructionUndemotable(cx->regs().pc);
                
              case BRANCH_EXIT:
                if (!AttemptToExtendTree(cx, tm, lr, NULL, NULL, NULL))
                    return TPA_RanStuff;
                break;

              case LOOP_EXIT:
                if (!innermostNestedGuard)
                    return TPA_RanStuff;
                if (!AttemptToExtendTree(cx, tm, innermostNestedGuard, lr, NULL, NULL))
                    return TPA_RanStuff;
                break;

              default:
                return TPA_RanStuff;
            }

            JS_ASSERT(tm->recorder);

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
    if (!RecordTree(cx, tm, tree->first, NULL, NULL, 0, globalSlots))
        return TPA_Nothing;

  interpret:
    JS_ASSERT(tm->recorder);

    
    if (!Interpret(cx, fp, inlineCallCount, JSINTERP_RECORD))
        return TPA_Error;

    JS_ASSERT(!cx->isExceptionPending());
    
    return TPA_RanStuff;
}

LoopProfile::LoopProfile(TraceMonitor *tm, StackFrame *entryfp,
                         jsbytecode *top, jsbytecode *bottom)
    : traceMonitor(tm),
      entryScript(entryfp->script()),
      entryfp(entryfp),
      top(top),
      bottom(bottom),
      hits(0),
      undecided(false),
      unprofitable(false)
{
    reset();
}

void
LoopProfile::reset()
{
    profiled = false;
    traceOK = false;
    numAllOps = 0;
    numSelfOps = 0;
    numSelfOpsMult = 0;
    branchMultiplier = 1;
    shortLoop = false;
    maybeShortLoop = false;
    numInnerLoops = 0;
    loopStackDepth = 0;
    sp = 0;

    PodArrayZero(allOps);
    PodArrayZero(selfOps);
}

MonitorResult
LoopProfile::profileLoopEdge(JSContext* cx, uintN& inlineCallCount)
{
    if (cx->regs().pc == top) {
        debug_only_print0(LC_TMProfiler, "Profiling complete (edge)\n");
        decide(cx);
    } else {
        
        StackFrame *fp = cx->fp();
        jsbytecode *pc = cx->regs().pc;
        bool found = false;

        
        for (int i = int(numInnerLoops)-1; i >= 0; i--) {
            if (innerLoops[i].entryfp == fp && innerLoops[i].top == pc) {
                innerLoops[i].iters++;
                found = true;
                break;
            }
        }

        if (!found && numInnerLoops < PROFILE_MAX_INNER_LOOPS)
            innerLoops[numInnerLoops++] = InnerLoop(fp, pc, NULL);
    }

    return MONITOR_NOT_RECORDING;
}


static const uintN PROFILE_HOTLOOP = 61;
static const uintN MAX_PROFILE_OPS = 4096;

static jsbytecode *
GetLoopBottom(JSContext *cx)
{
    return GetLoopBottom(cx, cx->regs().pc);
}

static LoopProfile *
LookupOrAddProfile(JSContext *cx, TraceMonitor *tm, void** traceData, uintN *traceEpoch)
{
    LoopProfile *prof;

    















#if JS_MONOIC
    if (*traceData && *traceEpoch == tm->flushEpoch) {
        prof = (LoopProfile *)*traceData;
    } else {
        jsbytecode* pc = cx->regs().pc;
        jsbytecode* bottom = GetLoopBottom(cx);
        if (!bottom)
            return NULL;
        prof = new (*tm->dataAlloc) LoopProfile(tm, cx->fp(), pc, bottom);
        *traceData = prof;
        *traceEpoch = tm->flushEpoch;
        tm->loopProfiles->put(pc, prof);
    }
#else
    LoopProfileMap &table = *tm->loopProfiles;
    jsbytecode* pc = cx->regs().pc;
    if (LoopProfileMap::AddPtr p = table.lookupForAdd(pc)) {
        prof = p->value;
    } else {
        jsbytecode* bottom = GetLoopBottom(cx);
        if (!bottom)
            return NULL;
        prof = new (*tm->dataAlloc) LoopProfile(tm, cx->fp(), pc, bottom);
        table.add(p, pc, prof);
    }
#endif

    return prof;
}

static LoopProfile *
LookupLoopProfile(TraceMonitor *tm, jsbytecode *pc)
{
    LoopProfileMap &table = *tm->loopProfiles;
    if (LoopProfileMap::Ptr p = table.lookup(pc)) {
        JS_ASSERT(p->value->top == pc);
        return p->value;
    } else
        return NULL;
}

void
LoopProfile::stopProfiling(JSContext *cx)
{
    JS_ASSERT(JS_THREAD_DATA(cx)->recordingCompartment == NULL);
    JS_THREAD_DATA(cx)->profilingCompartment = NULL;

    traceMonitor->profile = NULL;
}

JS_REQUIRES_STACK TracePointAction
MonitorTracePoint(JSContext *cx, uintN& inlineCallCount, bool* blacklist,
                  void** traceData, uintN *traceEpoch, uint32 *loopCounter, uint32 hits)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_FROM_CONTEXT(cx);

    if (!cx->profilingEnabled)
        return RecordTracePoint(cx, tm, inlineCallCount, blacklist, true);

    *blacklist = false;

    







    if (TRACE_PROFILER(cx))
        return TPA_Nothing;

    jsbytecode* pc = cx->regs().pc;
    LoopProfile *prof = LookupOrAddProfile(cx, tm, traceData, traceEpoch);
    if (!prof) {
        *blacklist = true;
        return TPA_Nothing;
    }

    prof->hits += hits;
    if (prof->hits < PROFILE_HOTLOOP)
        return TPA_Nothing;

    AutoRetBlacklist autoRetBlacklist(cx->regs().pc, blacklist);

    if (prof->profiled) {
        if (prof->traceOK) {
            return RecordTracePoint(cx, tm, inlineCallCount, blacklist, prof->execOK);
        } else {
            return TPA_Nothing;
        }
    }

    debug_only_printf(LC_TMProfiler, "Profiling at line %d\n",
                      js_FramePCToLineNumber(cx, cx->fp()));

    tm->profile = prof;

    JS_ASSERT(JS_THREAD_DATA(cx)->profilingCompartment == NULL);
    JS_ASSERT(JS_THREAD_DATA(cx)->recordingCompartment == NULL);
    JS_THREAD_DATA(cx)->profilingCompartment = cx->compartment;

    if (!Interpret(cx, cx->fp(), inlineCallCount, JSINTERP_PROFILE))
        return TPA_Error;

    JS_ASSERT(!cx->isExceptionPending());

    
    prof = LookupLoopProfile(tm, pc);
    if (prof && prof->undecided) {
        *loopCounter = 3000;
        prof->reset();
    }

    return TPA_RanStuff;
}







template<class T>
static inline bool
PCWithinLoop(StackFrame *fp, jsbytecode *pc, T& loop)
{
    return fp > loop.entryfp || (fp == loop.entryfp && pc >= loop.top && pc <= loop.bottom);
}

LoopProfile::ProfileAction
LoopProfile::profileOperation(JSContext* cx, JSOp op)
{
    TraceMonitor* tm = JS_TRACE_MONITOR_FROM_CONTEXT(cx);

    JS_ASSERT(tm == traceMonitor);
    JS_ASSERT(&entryScript->compartment->traceMonitor == tm);

    if (profiled) {
        stopProfiling(cx);
        return ProfComplete;
    }

    jsbytecode *pc = cx->regs().pc;
    StackFrame *fp = cx->fp();
    JSScript *script = fp->script();

    if (!PCWithinLoop(fp, pc, *this)) {
        debug_only_printf(LC_TMProfiler, "Profiling complete (loop exit) at line %u\n",
                          js_FramePCToLineNumber(cx, cx->fp()));
        tm->profile->decide(cx);
        stopProfiling(cx);
        return ProfComplete;
    }

    while (loopStackDepth > 0 && !PCWithinLoop(fp, pc, loopStack[loopStackDepth-1])) {
        debug_only_print0(LC_TMProfiler, "Profiler: Exiting inner loop\n");
        loopStackDepth--;
    }

    if (op == JSOP_TRACE || op == JSOP_NOTRACE) {
        if (pc != top && (loopStackDepth == 0 || pc != loopStack[loopStackDepth-1].top)) {
            if (loopStackDepth == PROFILE_MAX_INNER_LOOPS) {
                debug_only_print0(LC_TMProfiler, "Profiling complete (maxnest)\n");
                tm->profile->decide(cx);
                stopProfiling(cx);
                return ProfComplete;
            }

            debug_only_printf(LC_TMProfiler, "Profiler: Entering inner loop at line %d\n",
                              js_FramePCToLineNumber(cx, cx->fp()));
            loopStack[loopStackDepth++] = InnerLoop(fp, pc, GetLoopBottom(cx));
        }
    }

    numAllOps++;
    if (loopStackDepth == 0) {
        numSelfOps++;
        numSelfOpsMult += branchMultiplier;
    }

    if (op == JSOP_ADD || op == JSOP_SUB || op == JSOP_MUL || op == JSOP_DIV) {
        Value& v1 = cx->regs().sp[-1];
        Value& v2 = cx->regs().sp[-2];

        
        if (v1.isDouble() || v2.isDouble())
            increment(OP_FLOAT);
        else if (v1.isInt32() || v2.isInt32())
            increment(OP_INT);
    }

    if (op == JSOP_EQ || op == JSOP_NE)
        increment(OP_EQ);

    if (op == JSOP_BITOR || op == JSOP_BITXOR || op == JSOP_BITAND
        || op == JSOP_LSH || op == JSOP_RSH || op == JSOP_URSH || op == JSOP_BITNOT)
    {
        increment(OP_BIT);
    }

    if (op == JSOP_EVAL)
        increment(OP_EVAL);

    if (op == JSOP_NEW)
        increment(OP_NEW);

    if (op == JSOP_GETELEM || op == JSOP_SETELEM) {
        Value& lval = cx->regs().sp[op == JSOP_GETELEM ? -2 : -3];
        if (lval.isObject() && js_IsTypedArray(&lval.toObject()))
            increment(OP_TYPED_ARRAY);
        else if (lval.isObject() && lval.toObject().isDenseArray() && op == JSOP_GETELEM)
            increment(OP_ARRAY_READ);
    }

    if (op == JSOP_GETPROP || op == JSOP_CALLPROP ||
        op == JSOP_GETARGPROP || op == JSOP_GETLOCALPROP)
    {
        
        Value v = UndefinedValue();
        if (op == JSOP_GETPROP || op == JSOP_CALLPROP) {
            v = cx->regs().sp[-1];
        } else if (op == JSOP_GETARGPROP) {
            uint32 slot = GET_ARGNO(pc);
            JS_ASSERT(slot < fp->numFormalArgs());
            v = fp->formalArg(slot);
        } else if (op == JSOP_GETLOCALPROP) {
            uint32 slot = GET_SLOTNO(pc);
            JS_ASSERT(slot < script->nslots);
            v = fp->slots()[slot];
        } else {
            JS_NOT_REACHED("no else");
        }

        if (v.isObject()) {
            JSObject *aobj = js_GetProtoIfDenseArray(&v.toObject());
            PropertyCacheEntry *entry;
            JSObject *obj2;
            JSAtom *atom;
            JS_PROPERTY_CACHE(cx).test(cx, pc, aobj, obj2, entry, atom);
            if (!atom && entry->vword.isShape()) {
                const Shape *shape = entry->vword.toShape();
                if (shape->hasGetterValue())
                    increment(OP_SCRIPTED_GETTER);
            }
        }
    }

    if (op == JSOP_CALL) {
        increment(OP_CALL);

        uintN argc = GET_ARGC(cx->regs().pc);
        Value &v = cx->regs().sp[-((int)argc + 2)];
        JSObject *callee;
        if (IsFunctionObject(v, &callee)) {
            JSFunction *fun = callee->getFunctionPrivate();
            if (fun->isInterpreted()) {
                if (cx->fp()->isFunctionFrame() && fun == cx->fp()->fun())
                    increment(OP_RECURSIVE);
            } else {
                js::Native native = fun->u.n.native;
                if (js_IsMathFunction(JS_JSVALIFY_NATIVE(native)))
                    increment(OP_FLOAT);
            }
        }
    }

    if (op == JSOP_CALLPROP && loopStackDepth == 0)
        branchMultiplier *= mjit::GetCallTargetCount(script, pc);

    if (op == JSOP_TABLESWITCH) {
        jsint low = GET_JUMP_OFFSET(pc + JUMP_OFFSET_LEN);
        jsint high = GET_JUMP_OFFSET(pc + JUMP_OFFSET_LEN*2);
        branchMultiplier *= high - low + 1;
    }

    if (op == JSOP_LOOKUPSWITCH)
        branchMultiplier *= GET_UINT16(pc + JUMP_OFFSET_LEN);
    
    if (numAllOps >= MAX_PROFILE_OPS) {
        debug_only_print0(LC_TMProfiler, "Profiling complete (maxops)\n");
        tm->profile->decide(cx);
        stopProfiling(cx);
        return ProfComplete;
    }

    
    jsbytecode *testPC = cx->regs().pc;
    if (op == JSOP_EQ || op == JSOP_NE || op == JSOP_LT || op == JSOP_GT
        || op == JSOP_LE || op == JSOP_GE || op == JSOP_IN || op == JSOP_MOREITER)
    {
        const JSCodeSpec *cs = &js_CodeSpec[op];
        ptrdiff_t oplen = cs->length;
        JS_ASSERT(oplen != -1);

        if (cx->regs().pc - script->code + oplen < ptrdiff_t(script->length))
            if (cx->regs().pc[oplen] == JSOP_IFEQ || cx->regs().pc[oplen] == JSOP_IFNE)
                testPC = cx->regs().pc + oplen;
    }

    
    JSOp testOp = js_GetOpcode(cx, script, testPC);
    if (testOp == JSOP_IFEQ || testOp == JSOP_IFNE || testOp == JSOP_GOTO
        || testOp == JSOP_AND || testOp == JSOP_OR)
    {
        ptrdiff_t len = GET_JUMP_OFFSET(testPC);
        if (testPC + len == top && (op == JSOP_LT || op == JSOP_LE)) {
            StackValue v = stackAt(-1);
            if (v.hasValue && v.value < 8)
                shortLoop = true;
        }

        if (testPC + len == top && (op == JSOP_LT || op == JSOP_LE)
            && cx->regs().sp[-2].isInt32() && cx->regs().sp[-2].toInt32() < 16)
        {
            maybeShortLoop = true;
        }

        if (testOp != JSOP_GOTO && len > 0) {
            bool isConst;
            if (testOp == JSOP_IFEQ || testOp == JSOP_IFNE)
                isConst = stackAt(-1).isConst && stackAt(-2).isConst;
            else
                isConst = stackAt(-1).isConst;

            increment(OP_FWDJUMP);
            if (loopStackDepth == 0 && !isConst)
                branchMultiplier *= 2;
        }
    }

    if (op == JSOP_INT8) {
        stackPush(StackValue(true, GET_INT8(cx->regs().pc)));
    } else if (op == JSOP_STRING) {
        stackPush(StackValue(true));
    } else if (op == JSOP_TYPEOF || op == JSOP_TYPEOFEXPR) {
        stackPush(StackValue(true));
    } else if (op == JSOP_EQ || op == JSOP_NE) {
        StackValue v1 = stackAt(-1);
        StackValue v2 = stackAt(-2);
        stackPush(StackValue(v1.isConst && v2.isConst));
    } else if (op == JSOP_AND) {
        bool b = !!js_ValueToBoolean(cx->regs().sp[-1]);
        StackValue v = stackAt(-1);
        if (b)
            stackPop();
    } else {
        stackClear();
    }
    
    return ProfContinue;
}





bool
LoopProfile::isCompilationExpensive(JSContext *cx, uintN depth)
{
    if (depth == 0)
        return true;

    if (!profiled)
        return false;

    
    if (numSelfOps == MAX_PROFILE_OPS)
        return true;

    
    if (numSelfOpsMult > numSelfOps*100000)
        return true;

    
    for (uintN i=0; i<numInnerLoops; i++) {
        LoopProfile *prof = LookupLoopProfile(traceMonitor, innerLoops[i].top);
        if (!prof || prof->isCompilationExpensive(cx, depth-1))
            return true;
    }

    return false;
}







bool
LoopProfile::isCompilationUnprofitable(JSContext *cx, uintN goodOps)
{
    if (!profiled)
        return false;

    if (goodOps <= 22 && allOps[OP_FWDJUMP])
        return true;
    
    
    for (uintN i=0; i<numInnerLoops; i++) {
        LoopProfile *prof = LookupLoopProfile(traceMonitor, innerLoops[i].top);
        if (!prof || prof->unprofitable)
            return true;
    }

    return false;
}


void
LoopProfile::decide(JSContext *cx)
{
    bool wasUndecided = undecided;
    bool wasTraceOK = traceOK;
    
    profiled = true;
    traceOK = false;
    undecided = false;

#ifdef DEBUG
    uintN line = js_PCToLineNumber(cx, entryScript, top);

    debug_only_printf(LC_TMProfiler, "LOOP %s:%d\n", entryScript->filename, line);

    for (uintN i=0; i<numInnerLoops; i++) {
        InnerLoop &loop = innerLoops[i];
        if (LoopProfile *prof = LookupLoopProfile(traceMonitor, loop.top)) {
            uintN line = js_PCToLineNumber(cx, prof->entryScript, prof->top);
            debug_only_printf(LC_TMProfiler, "NESTED %s:%d (%d iters)\n",
                              prof->entryScript->filename, line, loop.iters);
        }
    }
    debug_only_printf(LC_TMProfiler, "FEATURE float %d\n", allOps[OP_FLOAT]);
    debug_only_printf(LC_TMProfiler, "FEATURE int %d\n", allOps[OP_INT]);
    debug_only_printf(LC_TMProfiler, "FEATURE bit %d\n", allOps[OP_BIT]);
    debug_only_printf(LC_TMProfiler, "FEATURE equality %d\n", allOps[OP_EQ]);
    debug_only_printf(LC_TMProfiler, "FEATURE eval %d\n", allOps[OP_EVAL]);
    debug_only_printf(LC_TMProfiler, "FEATURE new %d\n", allOps[OP_NEW]);
    debug_only_printf(LC_TMProfiler, "FEATURE call %d\n", allOps[OP_CALL]);
    debug_only_printf(LC_TMProfiler, "FEATURE arrayread %d\n", allOps[OP_ARRAY_READ]);
    debug_only_printf(LC_TMProfiler, "FEATURE typedarray %d\n", allOps[OP_TYPED_ARRAY]);
    debug_only_printf(LC_TMProfiler, "FEATURE scriptedgetter %d\n", allOps[OP_SCRIPTED_GETTER]);
    debug_only_printf(LC_TMProfiler, "FEATURE fwdjump %d\n", allOps[OP_FWDJUMP]);
    debug_only_printf(LC_TMProfiler, "FEATURE recursive %d\n", allOps[OP_RECURSIVE]);
    debug_only_printf(LC_TMProfiler, "FEATURE shortLoop %d\n", shortLoop);
    debug_only_printf(LC_TMProfiler, "FEATURE maybeShortLoop %d\n", maybeShortLoop);
    debug_only_printf(LC_TMProfiler, "FEATURE numAllOps %d\n", numAllOps);
    debug_only_printf(LC_TMProfiler, "FEATURE selfOps %d\n", numSelfOps);
    debug_only_printf(LC_TMProfiler, "FEATURE selfOpsMult %g\n", numSelfOpsMult);
#endif

    if (count(OP_RECURSIVE)) {
        debug_only_print0(LC_TMProfiler, "NOTRACE: recursive\n");
    } else if (count(OP_EVAL)) {
        debug_only_print0(LC_TMProfiler, "NOTRACE: eval\n");
    } else if (numInnerLoops > 7) {
        debug_only_print0(LC_TMProfiler, "NOTRACE: >3 inner loops\n");
    } else if (shortLoop) {
        debug_only_print0(LC_TMProfiler, "NOTRACE: short\n");
    } else if (isCompilationExpensive(cx, 4)) {
        debug_only_print0(LC_TMProfiler, "NOTRACE: expensive\n");
    } else if (maybeShortLoop && numInnerLoops < 2) {
        if (wasUndecided) {
            debug_only_print0(LC_TMProfiler, "NOTRACE: maybe short\n");
        } else {
            debug_only_print0(LC_TMProfiler, "UNDECIDED: maybe short\n");
            undecided = true; 
        }
    } else {
        uintN goodOps = 0;

        
        goodOps += count(OP_FLOAT)*10 + count(OP_BIT)*11 + count(OP_INT)*5 + count(OP_EQ)*15;

        
        goodOps += (count(OP_CALL) + count(OP_NEW))*20;

        
        goodOps += count(OP_TYPED_ARRAY)*10;

        
        goodOps += count(OP_SCRIPTED_GETTER)*40;

        
        goodOps += count(OP_ARRAY_READ)*15;

        debug_only_printf(LC_TMProfiler, "FEATURE goodOps %u\n", goodOps);

        unprofitable = isCompilationUnprofitable(cx, goodOps);
        if (unprofitable)
            debug_only_print0(LC_TMProfiler, "NOTRACE: unprofitable\n");
        else if (goodOps >= numAllOps)
            traceOK = true;
    }

    debug_only_printf(LC_TMProfiler, "TRACE %s:%d = %d\n", entryScript->filename, line, traceOK);

    if (traceOK) {
        
        for (uintN i=0; i<numInnerLoops; i++) {
            InnerLoop &loop = innerLoops[i];
            LoopProfile *prof = LookupLoopProfile(traceMonitor, loop.top);
            if (prof) {
                




                prof->traceOK = true;
                if (IsBlacklisted(loop.top)) {
                    debug_only_printf(LC_TMProfiler, "Unblacklisting at %d\n",
                                      js_PCToLineNumber(cx, prof->entryScript, loop.top));
                    Unblacklist(prof->entryScript, loop.top);
                }
            }
        }
    }

    execOK = traceOK;
    traceOK = wasTraceOK || traceOK;

    if (!traceOK && !undecided) {
        debug_only_printf(LC_TMProfiler, "Blacklisting at %d\n", line);
        Blacklist(top);
    }

    debug_only_print0(LC_TMProfiler, "\n");
}

JS_REQUIRES_STACK MonitorResult
MonitorLoopEdge(JSContext* cx, uintN& inlineCallCount, InterpMode interpMode)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_FROM_CONTEXT(cx);
    if (interpMode == JSINTERP_PROFILE && tm->profile)
        return tm->profile->profileLoopEdge(cx, inlineCallCount);
    else
        return RecordLoopEdge(cx, tm, inlineCallCount);
}

void
AbortProfiling(JSContext *cx)
{
    JS_ASSERT(TRACE_PROFILER(cx));
    LoopProfile *prof = TRACE_PROFILER(cx);
    
    debug_only_print0(LC_TMProfiler, "Profiling complete (aborted)\n");
    prof->profiled = true;
    prof->traceOK = false;
    prof->execOK = false;
    prof->stopProfiling(cx);
}

#else 

JS_REQUIRES_STACK MonitorResult
MonitorLoopEdge(JSContext* cx, uintN& inlineCallCount, InterpMode interpMode)
{
    TraceMonitor *tm = JS_TRACE_MONITOR_FROM_CONTEXT(cx);
    return RecordLoopEdge(cx, tm, inlineCallCount);
}

#endif 

uint32
GetHotloop(JSContext *cx)
{
#ifdef JS_METHODJIT
    if (cx->profilingEnabled)
        return PROFILE_HOTLOOP;
    else
#endif
        return 1;
}

} 

