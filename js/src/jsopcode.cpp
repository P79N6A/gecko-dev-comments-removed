










































#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsarena.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsstaticcheck.h"
#include "jsvector.h"

#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"
#include "jscntxtinlines.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::gc;




JS_STATIC_ASSERT(sizeof(uint32) * JS_BITS_PER_BYTE >= INDEX_LIMIT_LOG2 + 1);


#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format)               \
    JS_STATIC_ASSERT(op##_LENGTH == length);
#include "jsopcode.tbl"
#undef OPDEF

static const char js_incop_strs[][3] = {"++", "--"};
static const char js_for_each_str[]  = "for each";

const JSCodeSpec js_CodeSpec[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    {length,nuses,ndefs,prec,format},
#include "jsopcode.tbl"
#undef OPDEF
};

uintN js_NumCodeSpecs = JS_ARRAY_LENGTH(js_CodeSpec);





static const char *CodeToken[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    token,
#include "jsopcode.tbl"
#undef OPDEF
};

#if defined(DEBUG) || defined(JS_JIT_SPEW) || defined(JS_METHODJIT_SPEW)




const char *js_CodeName[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    name,
#include "jsopcode.tbl"
#undef OPDEF
};
#endif



static ptrdiff_t
GetJumpOffset(jsbytecode *pc, jsbytecode *pc2)
{
    uint32 type;

    type = JOF_OPTYPE(*pc);
    if (JOF_TYPE_IS_EXTENDED_JUMP(type))
        return GET_JUMPX_OFFSET(pc2);
    return GET_JUMP_OFFSET(pc2);
}

uintN
js_GetIndexFromBytecode(JSContext *cx, JSScript *script, jsbytecode *pc,
                        ptrdiff_t pcoff)
{
    JSOp op;
    uintN span, base;

    op = js_GetOpcode(cx, script, pc);
    JS_ASSERT(js_CodeSpec[op].length >= 1 + pcoff + UINT16_LEN);

    



    span = js_CodeSpec[op].length;
    base = 0;
    if (pc - script->code + span < script->length) {
        if (pc[span] == JSOP_RESETBASE) {
            base = GET_INDEXBASE(pc - JSOP_INDEXBASE_LENGTH);
        } else if (pc[span] == JSOP_RESETBASE0) {
            JS_ASSERT(JSOP_INDEXBASE1 <= pc[-1] || pc[-1] <= JSOP_INDEXBASE3);
            base = (pc[-1] - JSOP_INDEXBASE1 + 1) << 16;
        }
    }
    return base + GET_UINT16(pc + pcoff);
}

uintN
js_GetVariableBytecodeLength(jsbytecode *pc)
{
    JSOp op;
    uintN jmplen, ncases;
    jsint low, high;

    op = (JSOp) *pc;
    JS_ASSERT(js_CodeSpec[op].length == -1);
    switch (op) {
      case JSOP_TABLESWITCHX:
        jmplen = JUMPX_OFFSET_LEN;
        goto do_table;
      case JSOP_TABLESWITCH:
        jmplen = JUMP_OFFSET_LEN;
      do_table:
        
        pc += jmplen;
        low = GET_JUMP_OFFSET(pc);
        pc += JUMP_OFFSET_LEN;
        high = GET_JUMP_OFFSET(pc);
        ncases = (uintN)(high - low + 1);
        return 1 + jmplen + INDEX_LEN + INDEX_LEN + ncases * jmplen;

      case JSOP_LOOKUPSWITCHX:
        jmplen = JUMPX_OFFSET_LEN;
        goto do_lookup;
      default:
        JS_ASSERT(op == JSOP_LOOKUPSWITCH);
        jmplen = JUMP_OFFSET_LEN;
      do_lookup:
        
        pc += jmplen;
        ncases = GET_UINT16(pc);
        return 1 + jmplen + INDEX_LEN + ncases * (INDEX_LEN + jmplen);
    }
}

uintN
js_GetVariableStackUses(JSOp op, jsbytecode *pc)
{
    JS_ASSERT(*pc == op || *pc == JSOP_TRAP);
    JS_ASSERT(js_CodeSpec[op].nuses == -1);
    switch (op) {
      case JSOP_POPN:
        return GET_UINT16(pc);
      case JSOP_LEAVEBLOCK:
        return GET_UINT16(pc);
      case JSOP_LEAVEBLOCKEXPR:
        return GET_UINT16(pc) + 1;
      default:
        
        JS_ASSERT(op == JSOP_NEW || op == JSOP_CALL || op == JSOP_EVAL ||
                  op == JSOP_FUNCALL || op == JSOP_FUNAPPLY);
        return 2 + GET_ARGC(pc);
    }
}

uintN
js_GetEnterBlockStackDefs(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JSObject *obj;

    JS_ASSERT(*pc == JSOP_ENTERBLOCK || *pc == JSOP_TRAP);
    GET_OBJECT_FROM_BYTECODE(script, pc, 0, obj);
    return OBJ_BLOCK_COUNT(cx, obj);
}

class AutoScriptUntrapper {
    JSContext *cx;
    JSScript *script;
    jsbytecode *origPC;
    jsbytecode *newPC;

public:
    AutoScriptUntrapper(JSContext *cx, JSScript *script, jsbytecode **pc)
        : cx(cx), script(script), origPC(*pc)
    {
        jsbytecode *newCode = js_UntrapScriptCode(cx, script);
        if (newCode == script->code) {
            
            newPC = origPC;
        } else {
            script->main += newCode - script->code;
            *pc = newPC = origPC + (newCode - script->code);
            script->code = newCode;
        }
    }
    ~AutoScriptUntrapper()
    {
        ptrdiff_t delta = newPC - origPC;
        if (delta) {
            jsbytecode *oldCode = script->code - delta;
            cx->free(script->code);
            script->code = oldCode;
            script->main -= delta;
        }
    }
};

#ifdef DEBUG


JS_FRIEND_API(JSBool)
js_DisassembleAtPC(JSContext *cx, JSScript *script, JSBool lines, jsbytecode *pc, Sprinter *sp)
{
    jsbytecode *next, *end;
    uintN len;

    next = script->code;
    end = next + script->length;
    while (next < end) {
        if (next == script->main)
            SprintCString(sp, "main:\n");
        if (pc != NULL) {
            if (pc == next)
                SprintCString(sp, "--> ");
            else
                SprintCString(sp, "    ");
        }
        len = js_Disassemble1(cx, script, next,
                              next - script->code,
                              lines, sp);
        if (!len)
            return JS_FALSE;
        next += len;
    }
    return JS_TRUE;
}

JS_FRIEND_API(JSBool)
js_Disassemble(JSContext *cx, JSScript *script, JSBool lines, Sprinter *sp)
{
    return js_DisassembleAtPC(cx, script, lines, NULL, sp);
}

JS_FRIEND_API(JSBool)
js_DumpPC(JSContext *cx)
{
    void *mark = JS_ARENA_MARK(&cx->tempPool);
    Sprinter sprinter;
    INIT_SPRINTER(cx, &sprinter, &cx->tempPool, 0);
    JSBool ok = js_DisassembleAtPC(cx, cx->fp()->script(), true, cx->regs->pc, &sprinter);
    fprintf(stdout, "%s", sprinter.base);
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return ok;
}

JSBool
js_DumpScript(JSContext *cx, JSScript *script)
{
    void *mark = JS_ARENA_MARK(&cx->tempPool);
    Sprinter sprinter;
    INIT_SPRINTER(cx, &sprinter, &cx->tempPool, 0);
    JSBool ok = js_Disassemble(cx, script, true, &sprinter);
    fprintf(stdout, "%s", sprinter.base);
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return ok;
}

static bool
ToDisassemblySource(JSContext *cx, jsval v, JSAutoByteString *bytes)
{
    if (!JSVAL_IS_PRIMITIVE(v)) {
        JSObject *obj = JSVAL_TO_OBJECT(v);
        Class *clasp = obj->getClass();

        if (clasp == &js_BlockClass) {
            char *source = JS_sprintf_append(NULL, "depth %d {", OBJ_BLOCK_DEPTH(cx, obj));
            if (!source)
                return false;

            Shape::Range r = obj->lastProperty()->all();
            while (!r.empty()) {
                const Shape &shape = r.front();
                JSAutoByteString bytes;
                if (!js_AtomToPrintableString(cx, JSID_TO_ATOM(shape.id), &bytes))
                    return false;

                r.popFront();
                source = JS_sprintf_append(source, "%s: %d%s",
                                           bytes.ptr(), shape.shortid,
                                           !r.empty() ? ", " : "");
                if (!source)
                    return false;
            }

            source = JS_sprintf_append(source, "}");
            if (!source)
                return false;
            bytes->initBytes(source);
            return true;
        }

        if (clasp == &js_FunctionClass) {
            JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);
            JSString *str = JS_DecompileFunction(cx, fun, JS_DONT_PRETTY_PRINT);
            if (!str)
                return false;
            return bytes->encode(cx, str);
        }

        if (clasp == &js_RegExpClass) {
            AutoValueRooter tvr(cx);
            if (!js_regexp_toString(cx, obj, tvr.addr()))
                return false;
            return bytes->encode(cx, JSVAL_TO_STRING(Jsvalify(tvr.value())));
        }
    }

    return !!js_ValueToPrintable(cx, Valueify(v), bytes, true);
}

JS_FRIEND_API(uintN)
js_Disassemble1(JSContext *cx, JSScript *script, jsbytecode *pc,
                uintN loc, JSBool lines, Sprinter *sp)
{
    JSOp op;
    const JSCodeSpec *cs;
    ptrdiff_t len, off, jmplen;
    uint32 type;
    JSAtom *atom;
    uintN index;
    JSObject *obj;
    jsval v;
    jsint i;

    AutoScriptUntrapper untrapper(cx, script, &pc);

    op = (JSOp)*pc;
    if (op >= JSOP_LIMIT) {
        char numBuf1[12], numBuf2[12];
        JS_snprintf(numBuf1, sizeof numBuf1, "%d", op);
        JS_snprintf(numBuf2, sizeof numBuf2, "%d", JSOP_LIMIT);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BYTECODE_TOO_BIG, numBuf1, numBuf2);
        return 0;
    }
    cs = &js_CodeSpec[op];
    len = (ptrdiff_t) cs->length;
    Sprint(sp, "%05u:", loc);
    if (lines)
        Sprint(sp, "%4u", JS_PCToLineNumber(cx, script, pc));
    Sprint(sp, "  %s", js_CodeName[op]);
    type = JOF_TYPE(cs->format);
    switch (type) {
      case JOF_BYTE:
        break;

      case JOF_JUMP:
      case JOF_JUMPX:
        off = GetJumpOffset(pc, pc);
        Sprint(sp, " %u (%d)", loc + intN(off), intN(off));
        break;

      case JOF_ATOM:
      case JOF_OBJECT:
      case JOF_REGEXP:
        index = js_GetIndexFromBytecode(cx, script, pc, 0);
        if (type == JOF_ATOM) {
            if (op == JSOP_DOUBLE) {
                v = Jsvalify(script->getConst(index));
            } else {
                JS_GET_SCRIPT_ATOM(script, pc, index, atom);
                v = STRING_TO_JSVAL(atom);
            }
        } else {
            if (type == JOF_OBJECT)
                obj = script->getObject(index);
            else
                obj = script->getRegExp(index);
            v = OBJECT_TO_JSVAL(obj);
        }
        {
            JSAutoByteString bytes;
            if (!ToDisassemblySource(cx, v, &bytes))
                return 0;
            Sprint(sp, " %s", bytes.ptr());
        }
        break;

      case JOF_GLOBAL:
        atom = script->getGlobalAtom(GET_SLOTNO(pc));
        v = STRING_TO_JSVAL(atom);
        {
            JSAutoByteString bytes;
            if (!ToDisassemblySource(cx, v, &bytes))
                return 0;
            Sprint(sp, " %s", bytes.ptr());
        }
        break;

      case JOF_UINT16PAIR:
        i = (jsint)GET_UINT16(pc);
        Sprint(sp, " %d", i);
        pc += UINT16_LEN;
        

      case JOF_UINT16:
        i = (jsint)GET_UINT16(pc);
        goto print_int;

      case JOF_TABLESWITCH:
      case JOF_TABLESWITCHX:
      {
        jsbytecode *pc2;
        jsint i, low, high;

        jmplen = (type == JOF_TABLESWITCH) ? JUMP_OFFSET_LEN
                                           : JUMPX_OFFSET_LEN;
        pc2 = pc;
        off = GetJumpOffset(pc, pc2);
        pc2 += jmplen;
        low = GET_JUMP_OFFSET(pc2);
        pc2 += JUMP_OFFSET_LEN;
        high = GET_JUMP_OFFSET(pc2);
        pc2 += JUMP_OFFSET_LEN;
        Sprint(sp, " defaultOffset %d low %d high %d", intN(off), low, high);
        for (i = low; i <= high; i++) {
            off = GetJumpOffset(pc, pc2);
            Sprint(sp, "\n\t%d: %d", i, intN(off));
            pc2 += jmplen;
        }
        len = 1 + pc2 - pc;
        break;
      }

      case JOF_LOOKUPSWITCH:
      case JOF_LOOKUPSWITCHX:
      {
        jsbytecode *pc2;
        jsatomid npairs;

        jmplen = (type == JOF_LOOKUPSWITCH) ? JUMP_OFFSET_LEN
                                            : JUMPX_OFFSET_LEN;
        pc2 = pc;
        off = GetJumpOffset(pc, pc2);
        pc2 += jmplen;
        npairs = GET_UINT16(pc2);
        pc2 += UINT16_LEN;
        Sprint(sp, " offset %d npairs %u", intN(off), uintN(npairs));
        while (npairs) {
            uint16 constIndex = GET_INDEX(pc2);
            pc2 += INDEX_LEN;
            off = GetJumpOffset(pc, pc2);
            pc2 += jmplen;

            JSAutoByteString bytes;
            if (!ToDisassemblySource(cx, Jsvalify(script->getConst(constIndex)), &bytes))
                return 0;
            Sprint(sp, "\n\t%s: %d", bytes.ptr(), intN(off));
            npairs--;
        }
        len = 1 + pc2 - pc;
        break;
      }

      case JOF_QARG:
        Sprint(sp, " %u", GET_ARGNO(pc));
        break;

      case JOF_LOCAL:
        Sprint(sp, " %u", GET_SLOTNO(pc));
        break;

      case JOF_SLOTATOM:
      case JOF_SLOTOBJECT: {
        Sprint(sp, " %u", GET_SLOTNO(pc));
        index = js_GetIndexFromBytecode(cx, script, pc, SLOTNO_LEN);
        if (type == JOF_SLOTATOM) {
            JS_GET_SCRIPT_ATOM(script, pc, index, atom);
            v = STRING_TO_JSVAL(atom);
        } else {
            obj = script->getObject(index);
            v = OBJECT_TO_JSVAL(obj);
        }

        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, v, &bytes))
            return 0;
        Sprint(sp, " %s", bytes.ptr());
        break;
      }

      case JOF_UINT24:
        JS_ASSERT(op == JSOP_UINT24 || op == JSOP_NEWARRAY);
        i = (jsint)GET_UINT24(pc);
        goto print_int;

      case JOF_UINT8:
        i = pc[1];
        goto print_int;

      case JOF_INT8:
        i = GET_INT8(pc);
        goto print_int;

      case JOF_INT32:
        JS_ASSERT(op == JSOP_INT32);
        i = GET_INT32(pc);
      print_int:
        Sprint(sp, " %d", i);
        break;

      default: {
        char numBuf[12];
        JS_snprintf(numBuf, sizeof numBuf, "%lx", (unsigned long) cs->format);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_UNKNOWN_FORMAT, numBuf);
        return 0;
      }
    }
    SprintCString(sp, "\n");
    return len;
}

#endif 



#define OFF2STR(sp,off) ((sp)->base + (off))
#define STR2OFF(sp,str) ((str) - (sp)->base)
#define RETRACT(sp,str) ((sp)->offset = STR2OFF(sp, str))

static JSBool
SprintEnsureBuffer(Sprinter *sp, size_t len)
{
    ptrdiff_t nb;
    char *base;

    nb = (sp->offset + len + 1) - sp->size;
    if (nb < 0)
        return JS_TRUE;
    base = sp->base;
    if (!base) {
        JS_ARENA_ALLOCATE_CAST(base, char *, sp->pool, nb);
    } else {
        JS_ARENA_GROW_CAST(base, char *, sp->pool, sp->size, nb);
    }
    if (!base) {
        js_ReportOutOfScriptQuota(sp->context);
        return JS_FALSE;
    }
    sp->base = base;
    sp->size += nb;
    return JS_TRUE;
}

namespace js {

char *
SprintReserveAmount(Sprinter *sp, size_t len)
{
    
    if (!SprintEnsureBuffer(sp, len))
        return NULL;

    
    ptrdiff_t offset = sp->offset;
    sp->offset += len;
    return sp->base + offset;
}

ptrdiff_t
SprintPut(Sprinter *sp, const char *s, size_t len)
{
    ptrdiff_t offset = sp->size; 
    char *bp = sp->base;         

    
    if (!SprintEnsureBuffer(sp, len))
        return -1;

    if (sp->base != bp &&               
        s >= bp && s < bp + offset) {   
        s = sp->base + (s - bp);        
    }

    
    offset = sp->offset;
    sp->offset += len;
    bp = sp->base + offset;
    memmove(bp, s, len);
    bp[len] = 0;
    return offset;
}

ptrdiff_t
SprintCString(Sprinter *sp, const char *s)
{
    return SprintPut(sp, s, strlen(s));
}

ptrdiff_t
SprintString(Sprinter *sp, JSString *str)
{
    size_t length = str->length();
    const jschar *chars = str->getChars(sp->context);
    if (!chars)
        return -1;

    size_t size = js_GetDeflatedStringLength(sp->context, chars, length);
    if (size == (size_t)-1 || !SprintEnsureBuffer(sp, size))
        return -1;

    ptrdiff_t offset = sp->offset;
    sp->offset += size;
    js_DeflateStringToBuffer(sp->context, chars, length, sp->base + offset,
                             &size);
    sp->base[sp->offset] = 0;
    return offset;
}

ptrdiff_t
Sprint(Sprinter *sp, const char *format, ...)
{
    va_list ap;
    char *bp;
    ptrdiff_t offset;

    va_start(ap, format);
    bp = JS_vsmprintf(format, ap);      
    va_end(ap);
    if (!bp) {
        JS_ReportOutOfMemory(sp->context);
        return -1;
    }
    offset = SprintCString(sp, bp);
    sp->context->free_(bp);
    return offset;
}

} 

const char js_EscapeMap[] = {
    '\b', 'b',
    '\f', 'f',
    '\n', 'n',
    '\r', 'r',
    '\t', 't',
    '\v', 'v',
    '"',  '"',
    '\'', '\'',
    '\\', '\\',
    '\0', '0'
};

#define DONT_ESCAPE     0x10000

static char *
QuoteString(Sprinter *sp, JSString *str, uint32 quote)
{
    
    JSBool dontEscape = (quote & DONT_ESCAPE) != 0;
    jschar qc = (jschar) quote;
    ptrdiff_t off = sp->offset;
    if (qc && Sprint(sp, "%c", (char)qc) < 0)
        return NULL;

    const jschar *s = str->getChars(sp->context);
    if (!s)
        return NULL;
    const jschar *z = s + str->length();

    
    for (const jschar *t = s; t < z; s = ++t) {
        
        jschar c = *t;
        while (JS_ISPRINT(c) && c != qc && c != '\\' && c != '\t' &&
               !(c >> 8)) {
            c = *++t;
            if (t == z)
                break;
        }
        ptrdiff_t len = t - s;

        
        if (!SprintEnsureBuffer(sp, len))
            return NULL;

        
        char *bp = sp->base + sp->offset;
        sp->offset += len;
        while (--len >= 0)
            *bp++ = (char) *s++;
        *bp = '\0';

        if (t == z)
            break;

        
        bool ok;
        const char *e;
        if (!(c >> 8) && (e = strchr(js_EscapeMap, (int)c)) != NULL) {
            ok = dontEscape
                 ? Sprint(sp, "%c", (char)c) >= 0
                 : Sprint(sp, "\\%c", e[1]) >= 0;
        } else {
            




            ok = Sprint(sp, (qc && !(c >> 8)) ? "\\x%02X" : "\\u%04X", c) >= 0;
        }
        if (!ok)
            return NULL;
    }

    
    if (qc && Sprint(sp, "%c", (char)qc) < 0)
        return NULL;

    



    if (off == sp->offset && Sprint(sp, "") < 0)
        return NULL;
    return OFF2STR(sp, off);
}

JSString *
js_QuoteString(JSContext *cx, JSString *str, jschar quote)
{
    void *mark;
    Sprinter sprinter;
    char *bytes;
    JSString *escstr;

    mark = JS_ARENA_MARK(&cx->tempPool);
    INIT_SPRINTER(cx, &sprinter, &cx->tempPool, 0);
    bytes = QuoteString(&sprinter, str, quote);
    escstr = bytes ? JS_NewStringCopyZ(cx, bytes) : NULL;
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return escstr;
}



struct JSPrinter {
    Sprinter        sprinter;       
    JSArenaPool     pool;           
    uintN           indent;         
    bool            pretty;         
    bool            grouped;        
    bool            strict;         
    JSScript        *script;        
    jsbytecode      *dvgfence;      
    jsbytecode      **pcstack;      
    JSFunction      *fun;           
    jsuword         *localNames;    
};

JSPrinter *
js_NewPrinter(JSContext *cx, const char *name, JSFunction *fun,
              uintN indent, JSBool pretty, JSBool grouped, JSBool strict)
{
    JSPrinter *jp;

    jp = (JSPrinter *) cx->malloc(sizeof(JSPrinter));
    if (!jp)
        return NULL;
    INIT_SPRINTER(cx, &jp->sprinter, &jp->pool, 0);
    JS_InitArenaPool(&jp->pool, name, 256, 1, &cx->scriptStackQuota);
    jp->indent = indent;
    jp->pretty = !!pretty;
    jp->grouped = !!grouped;
    jp->strict = !!strict;
    jp->script = NULL;
    jp->dvgfence = NULL;
    jp->pcstack = NULL;
    jp->fun = fun;
    jp->localNames = NULL;
    if (fun && fun->isInterpreted() && fun->script()->bindings.hasLocalNames()) {
        jp->localNames = fun->script()->bindings.getLocalNameArray(cx, &jp->pool);
        if (!jp->localNames) {
            js_DestroyPrinter(jp);
            return NULL;
        }
    }
    return jp;
}

void
js_DestroyPrinter(JSPrinter *jp)
{
    JS_FinishArenaPool(&jp->pool);
    jp->sprinter.context->free(jp);
}

JSString *
js_GetPrinterOutput(JSPrinter *jp)
{
    JSContext *cx;
    JSString *str;

    cx = jp->sprinter.context;
    if (!jp->sprinter.base)
        return cx->runtime->emptyString;
    str = JS_NewStringCopyZ(cx, jp->sprinter.base);
    if (!str)
        return NULL;
    JS_FreeArenaPool(&jp->pool);
    INIT_SPRINTER(cx, &jp->sprinter, &jp->pool, 0);
    return str;
}




static const char * const var_prefix[] = {"var ", "const ", "let "};

static const char *
VarPrefix(jssrcnote *sn)
{
    if (sn && (SN_TYPE(sn) == SRC_DECL || SN_TYPE(sn) == SRC_GROUPASSIGN)) {
        ptrdiff_t type = js_GetSrcNoteOffset(sn, 0);
        if ((uintN)type <= SRC_DECL_LET)
            return var_prefix[type];
    }
    return "";
}

int
js_printf(JSPrinter *jp, const char *format, ...)
{
    va_list ap;
    char *bp, *fp;
    int cc;

    if (*format == '\0')
        return 0;

    va_start(ap, format);

    
    if (*format == '\t') {
        format++;
        if (jp->pretty && Sprint(&jp->sprinter, "%*s", jp->indent, "") < 0) {
            va_end(ap);
            return -1;
        }
    }

    
    fp = NULL;
    if (!jp->pretty && format[cc = strlen(format) - 1] == '\n') {
        fp = JS_strdup(jp->sprinter.context, format);
        if (!fp) {
            va_end(ap);
            return -1;
        }
        fp[cc] = '\0';
        format = fp;
    }

    
    bp = JS_vsmprintf(format, ap);      
    if (fp) {
        jp->sprinter.context->free(fp);
        format = NULL;
    }
    if (!bp) {
        JS_ReportOutOfMemory(jp->sprinter.context);
        va_end(ap);
        return -1;
    }

    cc = strlen(bp);
    if (SprintPut(&jp->sprinter, bp, (size_t)cc) < 0)
        cc = -1;
    jp->sprinter.context->free(bp);

    va_end(ap);
    return cc;
}

JSBool
js_puts(JSPrinter *jp, const char *s)
{
    return SprintCString(&jp->sprinter, s) >= 0;
}



typedef struct SprintStack {
    Sprinter    sprinter;       
    ptrdiff_t   *offsets;       
    jsbytecode  *opcodes;       
    uintN       top;            
    uintN       inArrayInit;    
    JSBool      inGenExp;       
    JSPrinter   *printer;       
} SprintStack;












static intN
ReconstructPCStack(JSContext *cx, JSScript *script, jsbytecode *pc,
                   jsbytecode **pcstack);

#define FAILED_EXPRESSION_DECOMPILER ((char *) 1)







static char *
DecompileExpression(JSContext *cx, JSScript *script, JSFunction *fun,
                    jsbytecode *pc);












static ptrdiff_t
GetOff(SprintStack *ss, uintN i)
{
    ptrdiff_t off;
    jsbytecode *pc;
    char *bytes;

    off = ss->offsets[i];
    if (off >= 0)
        return off;

    JS_ASSERT(off <= -2);
    JS_ASSERT(ss->printer->pcstack);
    if (off <= -2 && ss->printer->pcstack) {
        pc = ss->printer->pcstack[-2 - off];
        bytes = DecompileExpression(ss->sprinter.context, ss->printer->script,
                                    ss->printer->fun, pc);
        if (!bytes)
            return 0;
        if (bytes != FAILED_EXPRESSION_DECOMPILER) {
            off = SprintCString(&ss->sprinter, bytes);
            if (off < 0)
                off = 0;
            ss->offsets[i] = off;
            ss->sprinter.context->free(bytes);
            return off;
        }
        if (!ss->sprinter.base && SprintPut(&ss->sprinter, "", 0) >= 0) {
            memset(ss->sprinter.base, 0, ss->sprinter.offset);
            ss->offsets[i] = -1;
        }
    }
    return 0;
}

static const char *
GetStr(SprintStack *ss, uintN i)
{
    ptrdiff_t off;

    



    off = GetOff(ss, i);
    return OFF2STR(&ss->sprinter, off);
}





#define PAREN_SLOP      (2 + 1)


JS_STATIC_ASSERT(JSOP_FAKE_LIMIT <= 255);

static void
AddParenSlop(SprintStack *ss)
{
    memset(OFF2STR(&ss->sprinter, ss->sprinter.offset), 0, PAREN_SLOP);
    ss->sprinter.offset += PAREN_SLOP;
}

static JSBool
PushOff(SprintStack *ss, ptrdiff_t off, JSOp op)
{
    uintN top;

    if (!SprintEnsureBuffer(&ss->sprinter, PAREN_SLOP))
        return JS_FALSE;

    
    top = ss->top;
    JS_ASSERT(top < StackDepth(ss->printer->script));
    if (top >= StackDepth(ss->printer->script)) {
        JS_ReportOutOfMemory(ss->sprinter.context);
        return JS_FALSE;
    }

    
    ss->offsets[top] = off;
    ss->opcodes[top] = jsbytecode((op == JSOP_GETPROP2) ? JSOP_GETPROP
                                : (op == JSOP_GETELEM2) ? JSOP_GETELEM
                                : op);
    ss->top = ++top;
    AddParenSlop(ss);
    return JS_TRUE;
}

static ptrdiff_t
PopOffPrec(SprintStack *ss, uint8 prec)
{
    uintN top;
    const JSCodeSpec *topcs;
    ptrdiff_t off;

    
    top = ss->top;
    JS_ASSERT(top != 0);
    if (top == 0)
        return 0;

    ss->top = --top;
    off = GetOff(ss, top);
    topcs = &js_CodeSpec[ss->opcodes[top]];
    if (topcs->prec != 0 && topcs->prec < prec) {
        ss->sprinter.offset = ss->offsets[top] = off - 2;
        off = Sprint(&ss->sprinter, "(%s)", OFF2STR(&ss->sprinter, off));
    } else {
        ss->sprinter.offset = off;
    }
    return off;
}

static const char *
PopStrPrec(SprintStack *ss, uint8 prec)
{
    ptrdiff_t off;

    off = PopOffPrec(ss, prec);
    return OFF2STR(&ss->sprinter, off);
}

static ptrdiff_t
PopOff(SprintStack *ss, JSOp op)
{
    return PopOffPrec(ss, js_CodeSpec[op].prec);
}

static const char *
PopStr(SprintStack *ss, JSOp op)
{
    return PopStrPrec(ss, js_CodeSpec[op].prec);
}

static inline bool
IsInitializerOp(unsigned char op)
{
    return op == JSOP_NEWINIT || op == JSOP_NEWARRAY || op == JSOP_NEWOBJECT;
}

typedef struct TableEntry {
    jsval       key;
    ptrdiff_t   offset;
    JSAtom      *label;
    jsint       order;          
} TableEntry;

static JSBool
CompareOffsets(void *arg, const void *v1, const void *v2, int *result)
{
    ptrdiff_t offset_diff;
    const TableEntry *te1 = (const TableEntry *) v1,
                     *te2 = (const TableEntry *) v2;

    offset_diff = te1->offset - te2->offset;
    *result = (offset_diff == 0 ? te1->order - te2->order
               : offset_diff < 0 ? -1
               : 1);
    return JS_TRUE;
}

static ptrdiff_t
SprintDoubleValue(Sprinter *sp, jsval v, JSOp *opp)
{
    jsdouble d;
    ptrdiff_t todo;
    char *s;

    JS_ASSERT(JSVAL_IS_DOUBLE(v));
    d = JSVAL_TO_DOUBLE(v);
    if (JSDOUBLE_IS_NEGZERO(d)) {
        todo = SprintCString(sp, "-0");
        *opp = JSOP_NEG;
    } else if (!JSDOUBLE_IS_FINITE(d)) {
        
        todo = SprintCString(sp,
                             JSDOUBLE_IS_NaN(d)
                             ? "0 / 0"
                             : (d < 0)
                             ? "1 / -0"
                             : "1 / 0");
        *opp = JSOP_DIV;
    } else {
        ToCStringBuf cbuf;
        s = NumberToCString(sp->context, &cbuf, d);
        if (!s) {
            JS_ReportOutOfMemory(sp->context);
            return -1;
        }
        JS_ASSERT(strcmp(s, js_Infinity_str) &&
                  (*s != '-' ||
                   strcmp(s + 1, js_Infinity_str)) &&
                  strcmp(s, js_NaN_str));
        todo = Sprint(sp, s);
    }
    return todo;
}

static jsbytecode *
Decompile(SprintStack *ss, jsbytecode *pc, intN nb, JSOp nextop);

static JSBool
DecompileSwitch(SprintStack *ss, TableEntry *table, uintN tableLength,
                jsbytecode *pc, ptrdiff_t switchLength,
                ptrdiff_t defaultOffset, JSBool isCondSwitch)
{
    JSContext *cx;
    JSPrinter *jp;
    ptrdiff_t off, off2, diff, caseExprOff, todo;
    char *lval, *rval;
    uintN i;
    jsval key;
    JSString *str;

    cx = ss->sprinter.context;
    jp = ss->printer;

    
    off = isCondSwitch ? GetOff(ss, ss->top-1) : PopOff(ss, JSOP_NOP);
    lval = OFF2STR(&ss->sprinter, off);

    js_printf(jp, "\tswitch (%s) {\n", lval);

    if (tableLength) {
        diff = table[0].offset - defaultOffset;
        if (diff > 0) {
            jp->indent += 2;
            js_printf(jp, "\t%s:\n", js_default_str);
            jp->indent += 2;
            if (!Decompile(ss, pc + defaultOffset, diff, JSOP_NOP))
                return JS_FALSE;
            jp->indent -= 4;
        }

        caseExprOff = isCondSwitch ? JSOP_CONDSWITCH_LENGTH : 0;

        for (i = 0; i < tableLength; i++) {
            off = table[i].offset;
            off2 = (i + 1 < tableLength) ? table[i + 1].offset : switchLength;

            key = table[i].key;
            if (isCondSwitch) {
                ptrdiff_t nextCaseExprOff;

                




                nextCaseExprOff = (ptrdiff_t)JSVAL_TO_INT(key);
                nextCaseExprOff += js_CodeSpec[pc[nextCaseExprOff]].length;
                jp->indent += 2;
                if (!Decompile(ss, pc + caseExprOff,
                               nextCaseExprOff - caseExprOff, JSOP_NOP)) {
                    return JS_FALSE;
                }
                caseExprOff = nextCaseExprOff;

                
                --ss->top;
            } else {
                







                todo = -1;
                if (table[i].label) {
                    str = table[i].label;
                    key = JSVAL_VOID;
                } else if (JSVAL_IS_DOUBLE(key)) {
                    JSOp junk;

                    todo = SprintDoubleValue(&ss->sprinter, key, &junk);
                    str = NULL;
                } else {
                    str = js_ValueToString(cx, Valueify(key));
                    if (!str)
                        return JS_FALSE;
                }
                if (todo >= 0) {
                    rval = OFF2STR(&ss->sprinter, todo);
                } else {
                    rval = QuoteString(&ss->sprinter, str, (jschar)
                                       (JSVAL_IS_STRING(key) ? '"' : 0));
                    if (!rval)
                        return JS_FALSE;
                }
                RETRACT(&ss->sprinter, rval);
                jp->indent += 2;
                js_printf(jp, "\tcase %s:\n", rval);
            }

            jp->indent += 2;
            if (off <= defaultOffset && defaultOffset < off2) {
                diff = defaultOffset - off;
                if (diff != 0) {
                    if (!Decompile(ss, pc + off, diff, JSOP_NOP))
                        return JS_FALSE;
                    off = defaultOffset;
                }
                jp->indent -= 2;
                js_printf(jp, "\t%s:\n", js_default_str);
                jp->indent += 2;
            }
            if (!Decompile(ss, pc + off, off2 - off, JSOP_NOP))
                return JS_FALSE;
            jp->indent -= 4;

            
            if (isCondSwitch)
                ++ss->top;
        }
    }

    if (defaultOffset == switchLength) {
        jp->indent += 2;
        js_printf(jp, "\t%s:;\n", js_default_str);
        jp->indent -= 2;
    }
    js_printf(jp, "\t}\n");

    
    if (isCondSwitch)
        --ss->top;
    return JS_TRUE;
}

#define LOCAL_ASSERT_CUSTOM(expr, BAD_EXIT)                                   \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(expr);                                                      \
        if (!(expr)) { BAD_EXIT; }                                            \
    JS_END_MACRO

#define LOCAL_ASSERT_RV(expr, rv)                                             \
    LOCAL_ASSERT_CUSTOM(expr, return (rv))

static JSAtom *
GetArgOrVarAtom(JSPrinter *jp, uintN slot)
{
    JSAtom *name;

    LOCAL_ASSERT_RV(jp->fun, NULL);
    LOCAL_ASSERT_RV(slot < jp->fun->script()->bindings.countLocalNames(), NULL);
    name = JS_LOCAL_NAME_TO_ATOM(jp->localNames[slot]);
#if !JS_HAS_DESTRUCTURING
    LOCAL_ASSERT_RV(name, NULL);
#endif
    return name;
}

const char *
GetLocal(SprintStack *ss, jsint i)
{
#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_RV(expr, "")

    ptrdiff_t off = ss->offsets[i];
    if (off >= 0)
        return OFF2STR(&ss->sprinter, off);

    









    JSScript *script = ss->printer->script;
    if (!JSScript::isValidOffset(script->objectsOffset))
        return GetStr(ss, i);

    for (jsatomid j = 0, n = script->objects()->length; j != n; j++) {
        JSObject *obj = script->getObject(j);
        if (obj->isBlock()) {
            jsint depth = OBJ_BLOCK_DEPTH(cx, obj);
            jsint count = OBJ_BLOCK_COUNT(cx, obj);

            if (jsuint(i - depth) < jsuint(count)) {
                jsint slot = i - depth;

                for (Shape::Range r(obj->lastProperty()); !r.empty(); r.popFront()) {
                    const Shape &shape = r.front();

                    if (shape.shortid == slot) {
                        LOCAL_ASSERT(JSID_IS_ATOM(shape.id));

                        JSAtom *atom = JSID_TO_ATOM(shape.id);
                        const char *rval = QuoteString(&ss->sprinter, atom, 0);
                        if (!rval)
                            return NULL;

                        RETRACT(&ss->sprinter, rval);
                        return rval;
                    }
                }

                break;
            }
        }
    }

    return GetStr(ss, i);

#undef LOCAL_ASSERT
}

static JSBool
IsVarSlot(JSPrinter *jp, jsbytecode *pc, jsint *indexp)
{
    uintN slot;

    slot = GET_SLOTNO(pc);
    if (slot < jp->script->nfixed) {
        
        *indexp = jp->fun->nargs + slot;
        return JS_TRUE;
    }

    
    slot -= jp->script->nfixed;
    JS_ASSERT(slot < StackDepth(jp->script));
    *indexp = slot;
    return JS_FALSE;
}

#define LOAD_ATOM(PCOFF)                                                      \
    GET_ATOM_FROM_BYTECODE(jp->script, pc, PCOFF, atom)

#if JS_HAS_DESTRUCTURING

#define LOCAL_ASSERT(expr)  LOCAL_ASSERT_RV(expr, NULL)
#define LOAD_OP_DATA(pc)    (oplen = (cs = &js_CodeSpec[op=(JSOp)*pc])->length)

static jsbytecode *
DecompileDestructuring(SprintStack *ss, jsbytecode *pc, jsbytecode *endpc);

static jsbytecode *
DecompileDestructuringLHS(SprintStack *ss, jsbytecode *pc, jsbytecode *endpc,
                          JSBool *hole)
{
    JSContext *cx;
    JSPrinter *jp;
    JSOp op;
    const JSCodeSpec *cs;
    uintN oplen;
    jsint i;
    const char *lval, *xval;
    ptrdiff_t todo;
    JSAtom *atom;

    *hole = JS_FALSE;
    cx = ss->sprinter.context;
    jp = ss->printer;
    LOAD_OP_DATA(pc);

    switch (op) {
      case JSOP_POP:
        *hole = JS_TRUE;
        todo = SprintPut(&ss->sprinter, ", ", 2);
        break;

      case JSOP_DUP:
        pc = DecompileDestructuring(ss, pc, endpc);
        if (!pc)
            return NULL;
        if (pc == endpc)
            return pc;
        LOAD_OP_DATA(pc);
        lval = PopStr(ss, JSOP_NOP);
        todo = SprintCString(&ss->sprinter, lval);
        if (op == JSOP_POPN)
            return pc;
        LOCAL_ASSERT(*pc == JSOP_POP);
        break;

      case JSOP_SETARG:
      case JSOP_SETLOCAL:
        LOCAL_ASSERT(pc[oplen] == JSOP_POP || pc[oplen] == JSOP_POPN);
        

      case JSOP_SETLOCALPOP:
        atom = NULL;
        lval = NULL;
        if (op == JSOP_SETARG) {
            atom = GetArgOrVarAtom(jp, GET_SLOTNO(pc));
            LOCAL_ASSERT(atom);
        } else if (IsVarSlot(jp, pc, &i)) {
            atom = GetArgOrVarAtom(jp, i);
            LOCAL_ASSERT(atom);
        } else {
            lval = GetLocal(ss, i);
        }
        {
            JSAutoByteString bytes;
            if (atom)
                lval = js_AtomToPrintableString(cx, atom, &bytes);
            LOCAL_ASSERT(lval);
            todo = SprintCString(&ss->sprinter, lval);
        }
        if (op != JSOP_SETLOCALPOP) {
            pc += oplen;
            if (pc == endpc)
                return pc;
            LOAD_OP_DATA(pc);
            if (op == JSOP_POPN)
                return pc;
            LOCAL_ASSERT(op == JSOP_POP);
        }
        break;

      default:
        






        todo = ss->sprinter.offset;
        ss->sprinter.offset = todo + PAREN_SLOP;
        pc = Decompile(ss, pc, -((intN)ss->top), JSOP_NOP);
        if (!pc)
            return NULL;
        if (pc == endpc)
            return pc;
        LOAD_OP_DATA(pc);
        LOCAL_ASSERT(op == JSOP_ENUMELEM || op == JSOP_ENUMCONSTELEM);
        xval = PopStr(ss, JSOP_NOP);
        lval = PopStr(ss, JSOP_GETPROP);
        ss->sprinter.offset = todo;
        if (*lval == '\0') {
            
            todo = SprintCString(&ss->sprinter, xval);
        } else if (*xval == '\0') {
            
            todo = SprintCString(&ss->sprinter, lval);
        } else {
            todo = Sprint(&ss->sprinter,
                          (JOF_OPMODE(ss->opcodes[ss->top+1]) == JOF_XMLNAME)
                          ? "%s.%s"
                          : "%s[%s]",
                          lval, xval);
        }
        break;
    }

    if (todo < 0)
        return NULL;

    LOCAL_ASSERT(pc < endpc);
    pc += oplen;
    return pc;
}











static jsbytecode *
DecompileDestructuring(SprintStack *ss, jsbytecode *pc, jsbytecode *endpc)
{
    ptrdiff_t head;
    JSContext *cx;
    JSPrinter *jp;
    JSOp op, saveop;
    const JSCodeSpec *cs;
    uintN oplen;
    jsint i, lasti;
    jsdouble d;
    const char *lval;
    JSAtom *atom;
    jssrcnote *sn;
    JSBool hole;

    LOCAL_ASSERT(*pc == JSOP_DUP);
    pc += JSOP_DUP_LENGTH;

    




    head = SprintPut(&ss->sprinter, "[", 1);
    if (head < 0 || !PushOff(ss, head, JSOP_NOP))
        return NULL;
    ss->sprinter.offset -= PAREN_SLOP;
    LOCAL_ASSERT(head == ss->sprinter.offset - 1);
    LOCAL_ASSERT(*OFF2STR(&ss->sprinter, head) == '[');

    cx = ss->sprinter.context;
    jp = ss->printer;
    lasti = -1;

    while (pc < endpc) {
#if JS_HAS_DESTRUCTURING_SHORTHAND
        ptrdiff_t nameoff = -1;
#endif

        LOAD_OP_DATA(pc);
        saveop = op;

        switch (op) {
          case JSOP_POP:
            pc += oplen;
            goto out;

          
          case JSOP_ZERO:   d = i = 0; goto do_getelem;
          case JSOP_ONE:    d = i = 1; goto do_getelem;
          case JSOP_UINT16: d = i = GET_UINT16(pc); goto do_getelem;
          case JSOP_UINT24: d = i = GET_UINT24(pc); goto do_getelem;
          case JSOP_INT8:   d = i = GET_INT8(pc);   goto do_getelem;
          case JSOP_INT32:  d = i = GET_INT32(pc);  goto do_getelem;

          case JSOP_DOUBLE:
            GET_DOUBLE_FROM_BYTECODE(jp->script, pc, 0, d);
            LOCAL_ASSERT(JSDOUBLE_IS_FINITE(d) && !JSDOUBLE_IS_NEGZERO(d));
            i = (jsint)d;

          do_getelem:
            sn = js_GetSrcNote(jp->script, pc);
            pc += oplen;
            if (pc == endpc)
                return pc;
            LOAD_OP_DATA(pc);
            LOCAL_ASSERT(op == JSOP_GETELEM);

            
            if (sn && SN_TYPE(sn) == SRC_INITPROP) {
                *OFF2STR(&ss->sprinter, head) = '{';
                if (Sprint(&ss->sprinter, "%g: ", d) < 0)
                    return NULL;
            } else {
                
                LOCAL_ASSERT(i == d);

                
                while (++lasti < i) {
                    if (SprintPut(&ss->sprinter, ", ", 2) < 0)
                        return NULL;
                }
            }
            break;

          case JSOP_LENGTH:
            atom = cx->runtime->atomState.lengthAtom;
            goto do_destructure_atom;

          case JSOP_CALLPROP:
          case JSOP_GETPROP:
            LOAD_ATOM(0);
          do_destructure_atom:
          {
            *OFF2STR(&ss->sprinter, head) = '{';
#if JS_HAS_DESTRUCTURING_SHORTHAND
            nameoff = ss->sprinter.offset;
#endif
            if (!QuoteString(&ss->sprinter, atom,
                             js_IsIdentifier(atom) ? 0 : (jschar)'\'')) {
                return NULL;
            }
            if (SprintPut(&ss->sprinter, ": ", 2) < 0)
                return NULL;
            break;
          }

          default:
            LOCAL_ASSERT(0);
        }

        pc += oplen;
        if (pc == endpc)
            return pc;

        




        pc = DecompileDestructuringLHS(ss, pc, endpc, &hole);
        if (!pc)
            return NULL;

#if JS_HAS_DESTRUCTURING_SHORTHAND
        if (nameoff >= 0) {
            ptrdiff_t offset, initlen;

            offset = ss->sprinter.offset;
            LOCAL_ASSERT(*OFF2STR(&ss->sprinter, offset) == '\0');
            initlen = offset - nameoff;
            LOCAL_ASSERT(initlen >= 4);

            
            if (((size_t)initlen & 1) == 0) {
                size_t namelen;
                const char *name;

                



                namelen = (size_t)(initlen - 2) >> 1;
                name = OFF2STR(&ss->sprinter, nameoff);
                if (!strncmp(name + namelen, ": ", 2) &&
                    !strncmp(name, name + namelen + 2, namelen)) {
                    offset -= namelen + 2;
                    *OFF2STR(&ss->sprinter, offset) = '\0';
                    ss->sprinter.offset = offset;
                }
            }
        }
#endif

        if (pc == endpc || *pc != JSOP_DUP)
            break;

        







        sn = js_GetSrcNote(jp->script, pc);
        if (!sn)
            break;
        if (SN_TYPE(sn) != SRC_CONTINUE) {
            LOCAL_ASSERT(SN_TYPE(sn) == SRC_DESTRUCT);
            break;
        }

        if (!hole && SprintPut(&ss->sprinter, ", ", 2) < 0)
            return NULL;

        pc += JSOP_DUP_LENGTH;
    }

out:
    lval = OFF2STR(&ss->sprinter, head);
    if (SprintPut(&ss->sprinter, (*lval == '[') ? "]" : "}", 1) < 0)
        return NULL;
    return pc;
}

static jsbytecode *
DecompileGroupAssignment(SprintStack *ss, jsbytecode *pc, jsbytecode *endpc,
                         jssrcnote *sn, ptrdiff_t *todop)
{
    JSOp op;
    const JSCodeSpec *cs;
    uintN oplen, start, end, i;
    ptrdiff_t todo;
    JSBool hole;
    const char *rval;

    LOAD_OP_DATA(pc);
    LOCAL_ASSERT(op == JSOP_PUSH || op == JSOP_GETLOCAL);

    todo = Sprint(&ss->sprinter, "%s[", VarPrefix(sn));
    if (todo < 0 || !PushOff(ss, todo, JSOP_NOP))
        return NULL;
    ss->sprinter.offset -= PAREN_SLOP;

    for (;;) {
        pc += oplen;
        if (pc == endpc)
            return pc;
        pc = DecompileDestructuringLHS(ss, pc, endpc, &hole);
        if (!pc)
            return NULL;
        if (pc == endpc)
            return pc;
        LOAD_OP_DATA(pc);
        if (op != JSOP_PUSH && op != JSOP_GETLOCAL)
            break;
        if (!hole && SprintPut(&ss->sprinter, ", ", 2) < 0)
            return NULL;
    }

    LOCAL_ASSERT(op == JSOP_POPN);
    if (SprintPut(&ss->sprinter, "] = [", 5) < 0)
        return NULL;

    end = ss->top - 1;
    start = end - GET_UINT16(pc);
    for (i = start; i < end; i++) {
        rval = GetStr(ss, i);
        if (Sprint(&ss->sprinter,
                   (i == start) ? "%s" : ", %s",
                   (i == end - 1 && *rval == '\0') ? ", " : rval) < 0) {
            return NULL;
        }
    }

    if (SprintPut(&ss->sprinter, "]", 1) < 0)
        return NULL;
    ss->sprinter.offset = ss->offsets[i];
    ss->top = start;
    *todop = todo;
    return pc;
}

#undef LOCAL_ASSERT
#undef LOAD_OP_DATA

#endif 

static JSBool
InitSprintStack(JSContext *cx, SprintStack *ss, JSPrinter *jp, uintN depth)
{
    size_t offsetsz, opcodesz;
    void *space;

    INIT_SPRINTER(cx, &ss->sprinter, &cx->tempPool, PAREN_SLOP);

    
    offsetsz = depth * sizeof(ptrdiff_t);
    opcodesz = depth * sizeof(jsbytecode);
    JS_ARENA_ALLOCATE(space, &cx->tempPool, offsetsz + opcodesz);
    if (!space) {
        js_ReportOutOfScriptQuota(cx);
        return JS_FALSE;
    }
    ss->offsets = (ptrdiff_t *) space;
    ss->opcodes = (jsbytecode *) ((char *)space + offsetsz);

    ss->top = ss->inArrayInit = 0;
    ss->inGenExp = JS_FALSE;
    ss->printer = jp;
    return JS_TRUE;
}











static jsbytecode *
Decompile(SprintStack *ss, jsbytecode *pc, intN nb, JSOp nextop)
{
    JSContext *cx;
    JSPrinter *jp, *jp2;
    jsbytecode *startpc, *endpc, *pc2, *done;
    ptrdiff_t tail, todo, len, oplen, cond, next;
    JSOp op, lastop, saveop;
    const JSCodeSpec *cs;
    jssrcnote *sn, *sn2;
    const char *lval, *rval, *xval, *fmt, *token;
    uintN nuses;
    jsint i, argc;
    char **argv;
    JSAtom *atom;
    JSObject *obj;
    JSFunction *fun = NULL; 
    JSString *str;
    JSBool ok;
#if JS_HAS_XML_SUPPORT
    JSBool foreach, inXML, quoteAttr;
#else
#define inXML JS_FALSE
#endif
    jsval val;

    static const char exception_cookie[] = "/*EXCEPTION*/";
    static const char retsub_pc_cookie[] = "/*RETSUB_PC*/";
    static const char forelem_cookie[]   = "/*FORELEM*/";
    static const char with_cookie[]      = "/*WITH*/";
    static const char dot_format[]       = "%s.%s";
    static const char index_format[]     = "%s[%s]";
    static const char predot_format[]    = "%s%s.%s";
    static const char postdot_format[]   = "%s.%s%s";
    static const char preindex_format[]  = "%s%s[%s]";
    static const char postindex_format[] = "%s[%s]%s";
    static const char ss_format[]        = "%s%s";
    static const char sss_format[]       = "%s%s%s";

    
    JS_STATIC_ASSERT(ARGNO_LEN == SLOTNO_LEN);




#define LOCAL_ASSERT(expr)    LOCAL_ASSERT_RV(expr, NULL)
#define DECOMPILE_CODE(pc,nb) if (!Decompile(ss, pc, nb, JSOP_NOP)) return NULL
#define NEXT_OP(pc)           (((pc) + (len) == endpc) ? nextop : pc[len])
#define TOP_STR()             GetStr(ss, ss->top - 1)
#define POP_STR()             PopStr(ss, op)
#define POP_STR_PREC(prec)    PopStrPrec(ss, prec)





#define POP_COND_STR()                                                        \
    PopStr(ss, (js_CodeSpec[ss->opcodes[ss->top - 1]].format & JOF_SET)       \
               ? JSOP_IFEQ                                                    \
               : JSOP_NOP)

#define ATOM_IS_IDENTIFIER(atom) js_IsIdentifier(atom)






#define GET_QUOTE_AND_FMT(qfmt, ufmt, rval)                                   \
    JS_BEGIN_MACRO                                                            \
        jschar quote_;                                                        \
        if (!ATOM_IS_IDENTIFIER(atom)) {                                      \
            quote_ = '\'';                                                    \
            fmt = qfmt;                                                       \
        } else {                                                              \
            quote_ = 0;                                                       \
            fmt = ufmt;                                                       \
        }                                                                     \
        rval = QuoteString(&ss->sprinter, atom, quote_);                      \
        if (!rval)                                                            \
            return NULL;                                                      \
    JS_END_MACRO

#define LOAD_OBJECT(PCOFF)                                                    \
    GET_OBJECT_FROM_BYTECODE(jp->script, pc, PCOFF, obj)

#define LOAD_FUNCTION(PCOFF)                                                  \
    GET_FUNCTION_FROM_BYTECODE(jp->script, pc, PCOFF, fun)

#define LOAD_REGEXP(PCOFF)                                                    \
    GET_REGEXP_FROM_BYTECODE(jp->script, pc, PCOFF, obj)

#define GET_SOURCE_NOTE_ATOM(sn, atom)                                        \
    JS_BEGIN_MACRO                                                            \
        jsatomid atomIndex_ = (jsatomid) js_GetSrcNoteOffset((sn), 0);        \
                                                                              \
        LOCAL_ASSERT(atomIndex_ < jp->script->atomMap.length);                \
        (atom) = jp->script->atomMap.vector[atomIndex_];                      \
    JS_END_MACRO





#define GET_ATOM_QUOTE_AND_FMT(qfmt, ufmt, rval)                              \
    JS_BEGIN_MACRO                                                            \
        LOAD_ATOM(0);                                                         \
        GET_QUOTE_AND_FMT(qfmt, ufmt, rval);                                  \
    JS_END_MACRO












#define PROPAGATE_CALLNESS()                                                  \
    JS_BEGIN_MACRO                                                            \
        if (ss->opcodes[ss->top - 1] == JSOP_CALL ||                          \
            ss->opcodes[ss->top - 1] == JSOP_EVAL ||                          \
            ss->opcodes[ss->top - 1] == JSOP_FUNCALL ||                       \
            ss->opcodes[ss->top - 1] == JSOP_FUNAPPLY) {                      \
            saveop = JSOP_CALL;                                               \
        }                                                                     \
    JS_END_MACRO

    cx = ss->sprinter.context;
    JS_CHECK_RECURSION(cx, return NULL);

    jp = ss->printer;
    startpc = pc;
    endpc = (nb < 0) ? jp->script->code + jp->script->length : pc + nb;
    tail = -1;
    todo = -2;                  
    saveop = JSOP_NOP;
    sn = NULL;
    rval = NULL;
#if JS_HAS_XML_SUPPORT
    foreach = inXML = quoteAttr = JS_FALSE;
#endif

    while (nb < 0 || pc < endpc) {
        






        lastop = saveop;
        op = (JSOp) *pc;
        cs = &js_CodeSpec[op];
        if (cs->format & JOF_INDEXBASE) {
            





            pc += cs->length;
            if (pc >= endpc)
                break;
            op = (JSOp) *pc;
            cs = &js_CodeSpec[op];
        }
        saveop = op;
        len = oplen = cs->length;
        nuses = js_GetStackUses(cs, op, pc);

        




        if (nb < 0) {
            LOCAL_ASSERT(ss->top >= nuses);
            uintN ndefs = js_GetStackDefs(cx, cs, op, jp->script, pc);
            if ((uintN) -(nb + 1) == ss->top - nuses + ndefs)
                return pc;
        }

        



        token = CodeToken[op];

        if (pc + oplen == jp->dvgfence) {
            JSStackFrame *fp;
            uint32 format, mode, type;

            




            fp = js_GetScriptedCaller(cx, NULL);
            format = cs->format;
            if (((fp && pc == fp->pc(cx)) ||
                 (pc == startpc && nuses != 0)) &&
                format & (JOF_SET|JOF_DEL|JOF_INCDEC|JOF_FOR|JOF_VARPROP)) {
                mode = JOF_MODE(format);
                if (mode == JOF_NAME) {
                    





                    type = JOF_TYPE(format);
                    op = (type == JOF_QARG)
                         ? JSOP_GETARG
                         : (type == JOF_LOCAL)
                         ? JSOP_GETLOCAL
                         : JSOP_NAME;

                    JS_ASSERT(js_CodeSpec[op].nuses >= 0);
                    i = nuses - js_CodeSpec[op].nuses;
                    while (--i >= 0)
                        PopOff(ss, JSOP_NOP);
                } else {
                    







                    if (mode == JOF_PROP) {
                        op = (JSOp) ((format & JOF_SET)
                                     ? JSOP_GETPROP2
                                     : JSOP_GETPROP);
                    } else if (mode == JOF_ELEM) {
                        op = (JSOp) ((format & JOF_SET)
                                     ? JSOP_GETELEM2
                                     : JSOP_GETELEM);
                    } else {
                        




                        switch (op) {
                          case JSOP_ENUMELEM:
                          case JSOP_ENUMCONSTELEM:
                            op = JSOP_GETELEM;
                            break;
                          case JSOP_GETTHISPROP:
                            













                            break;
                          case JSOP_GETARGPROP:
                            op = JSOP_GETARG;
                            break;
                          case JSOP_GETLOCALPROP:
                            op = JSOP_GETLOCAL;
                            break;
                          case JSOP_SETXMLNAME:
                            op = JSOp(JSOP_GETELEM2);
                            break;
                          default:
                            LOCAL_ASSERT(0);
                        }
                    }
                }
            }

            saveop = op;
            if (op >= JSOP_LIMIT) {
                if (op == JSOP_GETPROP2)
                    saveop = JSOP_GETPROP;
                else if (op == JSOP_GETELEM2)
                    saveop = JSOP_GETELEM;
            }
            LOCAL_ASSERT(js_CodeSpec[saveop].length == oplen ||
                         JOF_TYPE(format) == JOF_SLOTATOM);

            jp->dvgfence = NULL;
        }

        if (token) {
            switch (nuses) {
              case 2:
                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_ASSIGNOP) {
                    




                    op = (JSOp) pc[oplen];
                    rval = POP_STR();
                    lval = POP_STR();
                    
                    todo = SprintCString(&ss->sprinter, rval);
                    op = saveop;
                } else if (!inXML) {
                    rval = POP_STR_PREC(cs->prec + !!(cs->format & JOF_LEFTASSOC));
                    lval = POP_STR_PREC(cs->prec + !(cs->format & JOF_LEFTASSOC));
                    todo = Sprint(&ss->sprinter, "%s %s %s",
                                  lval, token, rval);
                } else {
                    
                    LOCAL_ASSERT(op == JSOP_ADD);
                    rval = POP_STR();
                    lval = POP_STR();
                    todo = Sprint(&ss->sprinter, ss_format, lval, rval);
                }
                break;

              case 1:
                rval = POP_STR();
                todo = Sprint(&ss->sprinter, ss_format, token, rval);
                break;

              case 0:
                todo = SprintCString(&ss->sprinter, token);
                break;

              default:
                todo = -2;
                break;
            }
        } else {
            switch (op) {
              case JSOP_NOP:
                




                sn = js_GetSrcNote(jp->script, pc);
                todo = -2;
                switch (sn ? SN_TYPE(sn) : SRC_NULL) {
                  case SRC_WHILE:
                    ++pc;
                    tail = js_GetSrcNoteOffset(sn, 0) - 1;
                    LOCAL_ASSERT(pc[tail] == JSOP_IFNE ||
                                 pc[tail] == JSOP_IFNEX);
                    js_printf(jp, "\tdo {\n");
                    jp->indent += 4;
                    DECOMPILE_CODE(pc, tail);
                    jp->indent -= 4;
                    js_printf(jp, "\t} while (%s);\n", POP_COND_STR());
                    pc += tail;
                    len = js_CodeSpec[*pc].length;
                    todo = -2;
                    break;

                  case SRC_FOR:
                    rval = "";

                  do_forloop:
                    JS_ASSERT(SN_TYPE(sn) == SRC_FOR);

                    
                    pc += JSOP_NOP_LENGTH;

                    
                    cond = js_GetSrcNoteOffset(sn, 0);
                    next = js_GetSrcNoteOffset(sn, 1);
                    tail = js_GetSrcNoteOffset(sn, 2);

                    



                    pc2 = pc;
                    if (cond != tail) {
                        LOCAL_ASSERT(*pc == JSOP_GOTO || *pc == JSOP_GOTOX);
                        pc2 += (*pc == JSOP_GOTO) ? JSOP_GOTO_LENGTH : JSOP_GOTOX_LENGTH;
                    }
                    LOCAL_ASSERT(tail + GetJumpOffset(pc+tail, pc+tail) == pc2 - pc);

                    
                    js_printf(jp, "\tfor (%s;", rval);

                    if (cond != tail) {
                        
                        DECOMPILE_CODE(pc + cond, tail - cond);
                        js_printf(jp, " %s", POP_STR());
                    }

                    
                    js_puts(jp, ";");

                    if (next != cond) {
                        







                        uintN saveTop = ss->top;

                        DECOMPILE_CODE(pc + next, cond - next - JSOP_POP_LENGTH);
                        LOCAL_ASSERT(ss->top - saveTop <= 1U);
                        rval = (ss->top == saveTop)
                               ? ss->sprinter.base + ss->sprinter.offset
                               : POP_STR();
                        js_printf(jp, " %s", rval);
                    }

                    
                    js_printf(jp, ") {\n");
                    jp->indent += 4;
                    next -= pc2 - pc;
                    DECOMPILE_CODE(pc2, next);
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");

                    
                    len = tail + js_CodeSpec[pc[tail]].length;
                    break;

                  case SRC_LABEL:
                    GET_SOURCE_NOTE_ATOM(sn, atom);
                    jp->indent -= 4;
                    rval = QuoteString(&ss->sprinter, atom, 0);
                    if (!rval)
                        return NULL;
                    RETRACT(&ss->sprinter, rval);
                    js_printf(jp, "\t%s:\n", rval);
                    jp->indent += 4;
                    break;

                  case SRC_LABELBRACE:
                    GET_SOURCE_NOTE_ATOM(sn, atom);
                    rval = QuoteString(&ss->sprinter, atom, 0);
                    if (!rval)
                        return NULL;
                    RETRACT(&ss->sprinter, rval);
                    js_printf(jp, "\t%s: {\n", rval);
                    jp->indent += 4;
                    break;

                  case SRC_ENDBRACE:
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");
                    break;

                  case SRC_FUNCDEF:
                    fun = jp->script->getFunction(js_GetSrcNoteOffset(sn, 0));
                  do_function:
                    js_puts(jp, "\n");
                    jp2 = js_NewPrinter(cx, "nested_function", fun,
                                        jp->indent, jp->pretty, jp->grouped,
                                        jp->strict);
                    if (!jp2)
                        return NULL;
                    ok = js_DecompileFunction(jp2);
                    if (ok && jp2->sprinter.base)
                        js_puts(jp, jp2->sprinter.base);
                    js_DestroyPrinter(jp2);
                    if (!ok)
                        return NULL;
                    js_puts(jp, "\n\n");
                    break;

                  case SRC_BRACE:
                    js_printf(jp, "\t{\n");
                    jp->indent += 4;
                    len = js_GetSrcNoteOffset(sn, 0);
                    DECOMPILE_CODE(pc + oplen, len - oplen);
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");
                    break;

                  default:;
                }
                break;

              case JSOP_PUSH:
#if JS_HAS_DESTRUCTURING
                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_GROUPASSIGN) {
                    pc = DecompileGroupAssignment(ss, pc, endpc, sn, &todo);
                    if (!pc)
                        return NULL;
                    LOCAL_ASSERT(*pc == JSOP_POPN);
                    len = oplen = JSOP_POPN_LENGTH;
                    goto end_groupassignment;
                }
#endif
                

              case JSOP_BINDNAME:
              case JSOP_BINDGNAME:
                todo = Sprint(&ss->sprinter, "");
                break;

              case JSOP_TRY:
                js_printf(jp, "\ttry {\n");
                jp->indent += 4;
                todo = -2;
                break;

              case JSOP_FINALLY:
                jp->indent -= 4;
                js_printf(jp, "\t} finally {\n");
                jp->indent += 4;

                




                todo = Sprint(&ss->sprinter, exception_cookie);
                if (todo < 0 || !PushOff(ss, todo, op))
                    return NULL;
                todo = Sprint(&ss->sprinter, retsub_pc_cookie);
                break;

              case JSOP_RETSUB:
                rval = POP_STR();
                LOCAL_ASSERT(strcmp(rval, retsub_pc_cookie) == 0);
                lval = POP_STR();
                LOCAL_ASSERT(strcmp(lval, exception_cookie) == 0);
                todo = -2;
                break;

              case JSOP_GOSUB:
              case JSOP_GOSUBX:
                




                todo = -2;
                break;

              case JSOP_POPN:
              {
                uintN newtop, oldtop;

                





                oldtop = ss->top;
                newtop = oldtop - GET_UINT16(pc);
                LOCAL_ASSERT(newtop <= oldtop);
                todo = -2;

                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_HIDDEN)
                    break;
#if JS_HAS_DESTRUCTURING
                if (sn && SN_TYPE(sn) == SRC_GROUPASSIGN) {
                    todo = Sprint(&ss->sprinter, "%s[] = [",
                                  VarPrefix(sn));
                    if (todo < 0)
                        return NULL;
                    for (uintN i = newtop; i < oldtop; i++) {
                        rval = OFF2STR(&ss->sprinter, ss->offsets[i]);
                        if (Sprint(&ss->sprinter, ss_format,
                                   (i == newtop) ? "" : ", ",
                                   (i == oldtop - 1 && *rval == '\0')
                                   ? ", " : rval) < 0) {
                            return NULL;
                        }
                    }
                    if (SprintPut(&ss->sprinter, "]", 1) < 0)
                        return NULL;

                    





                    if (newtop == oldtop) {
                        ss->sprinter.offset = todo;
                    } else {
                        





                        LOCAL_ASSERT(newtop < oldtop);
                        ss->sprinter.offset = GetOff(ss, newtop);
                        ss->top = newtop;
                    }

                  end_groupassignment:
                    LOCAL_ASSERT(*pc == JSOP_POPN);

                    










                    rval = OFF2STR(&ss->sprinter, todo);
                    todo = -2;
                    pc2 = pc + oplen;
                    if (*pc2 == JSOP_NOP) {
                        sn = js_GetSrcNote(jp->script, pc2);
                        if (sn) {
                            if (SN_TYPE(sn) == SRC_FOR) {
                                op = JSOP_NOP;
                                pc = pc2;
                                goto do_forloop;
                            }

                            if (SN_TYPE(sn) == SRC_DECL) {
                                if (ss->top == StackDepth(jp->script)) {
                                    




                                    pc = pc2 + JSOP_NOP_LENGTH;
                                    len = js_GetSrcNoteOffset(sn, 0);
                                    LOCAL_ASSERT(pc[len] == JSOP_LEAVEBLOCK);
                                    js_printf(jp, "\tlet (%s) {\n", rval);
                                    js_printf(jp, "\t}\n");
                                    break;
                                }
                                todo = SprintCString(&ss->sprinter, rval);
                                if (todo < 0 || !PushOff(ss, todo, JSOP_NOP))
                                    return NULL;
                                op = JSOP_POP;
                                pc = pc2 + JSOP_NOP_LENGTH;
                                goto do_letheadbody;
                            }
                        } else {
                            







                            if (GET_UINT16(pc) == 0)
                                break;
                            todo = SprintCString(&ss->sprinter, rval);
                            saveop = JSOP_NOP;
                        }
                    }

                    



                    if (todo == -2)
                        js_printf(jp, "\t%s;\n", rval);
                    break;
                }
#endif
                if (newtop < oldtop) {
                    ss->sprinter.offset = GetOff(ss, newtop);
                    ss->top = newtop;
                }
                break;
              }

              case JSOP_EXCEPTION:
                
                LOCAL_ASSERT(JS_FALSE);
                break;

              case JSOP_POP:
                





                op = JSOP_POPV;
                

              case JSOP_POPV:
                sn = js_GetSrcNote(jp->script, pc);
                switch (sn ? SN_TYPE(sn) : SRC_NULL) {
                  case SRC_FOR:
                    
                    if (ss->opcodes[ss->top-1] == JSOP_IN)
                        op = JSOP_LSH;
                    rval = POP_STR();
                    todo = -2;
                    goto do_forloop;

                  case SRC_PCDELTA:
                    
                    op = JSOP_POP;

                    
                    lval = JS_strdup(cx, POP_STR());
                    if (!lval)
                        return NULL;

                    



                    done = pc + len;
                    pc += js_GetSrcNoteOffset(sn, 0);
                    len = 0;

                    if (!Decompile(ss, done, pc - done, JSOP_POP)) {
                        cx->free((char *)lval);
                        return NULL;
                    }

                    
                    rval = POP_STR();
                    todo = Sprint(&ss->sprinter, "%s, %s", lval, rval);
                    cx->free((char *)lval);
                    break;

                  case SRC_HIDDEN:
                    
                    todo = -2;
                    break;

                  case SRC_DECL:
                    
                    pc += JSOP_POP_LENGTH;
#if JS_HAS_DESTRUCTURING
                  do_letheadbody:
#endif
                    len = js_GetSrcNoteOffset(sn, 0);
                    if (pc[len] == JSOP_LEAVEBLOCK) {
                        js_printf(jp, "\tlet (%s) {\n", POP_STR());
                        jp->indent += 4;
                        DECOMPILE_CODE(pc, len);
                        jp->indent -= 4;
                        js_printf(jp, "\t}\n");
                        todo = -2;
                    } else {
                        LOCAL_ASSERT(pc[len] == JSOP_LEAVEBLOCKEXPR);

                        lval = JS_strdup(cx, PopStr(ss, JSOP_NOP));
                        if (!lval)
                            return NULL;

                        
                        saveop = JSOP_LEAVEBLOCKEXPR;
                        if (!Decompile(ss, pc, len, saveop)) {
                            cx->free((char *)lval);
                            return NULL;
                        }
                        rval = PopStr(ss, JSOP_SETNAME);
                        todo = Sprint(&ss->sprinter,
                                      (*rval == '{')
                                      ? "let (%s) (%s)"
                                      : "let (%s) %s",
                                      lval, rval);
                        cx->free((char *)lval);
                    }
                    break;

                  default:
                    
                    if (ss->opcodes[ss->top-1] == JSOP_YIELD)
                        op = JSOP_NOP;

                    rval = POP_STR();

                    





                    if (*rval != '\0' && (rval[0] != '/' || rval[1] != '*')) {
                        js_printf(jp,
                                  (*rval == '{' ||
                                   (strncmp(rval, js_function_str, 8) == 0 &&
                                    rval[8] == ' '))
                                  ? "\t(%s);\n"
                                  : "\t%s;\n",
                                  rval);
                    } else {
                        LOCAL_ASSERT(*rval == '\0' ||
                                     strcmp(rval, exception_cookie) == 0);
                    }
                    todo = -2;
                    break;
                }
                sn = NULL;
                break;

              case JSOP_ENTERWITH:
                LOCAL_ASSERT(!js_GetSrcNote(jp->script, pc));
                rval = POP_STR();
                js_printf(jp, "\twith (%s) {\n", rval);
                jp->indent += 4;
                todo = Sprint(&ss->sprinter, with_cookie);
                break;

              case JSOP_LEAVEWITH:
                sn = js_GetSrcNote(jp->script, pc);
                todo = -2;
                if (sn && SN_TYPE(sn) == SRC_HIDDEN)
                    break;
                rval = POP_STR();
                LOCAL_ASSERT(strcmp(rval, with_cookie) == 0);
                jp->indent -= 4;
                js_printf(jp, "\t}\n");
                break;

              case JSOP_ENTERBLOCK:
              {
                JSAtom **atomv, *smallv[5];

                LOAD_OBJECT(0);
                argc = OBJ_BLOCK_COUNT(cx, obj);
                if ((size_t)argc <= JS_ARRAY_LENGTH(smallv)) {
                    atomv = smallv;
                } else {
                    atomv = (JSAtom **) cx->malloc(argc * sizeof(JSAtom *));
                    if (!atomv)
                        return NULL;
                }

                MUST_FLOW_THROUGH("enterblock_out");
#define LOCAL_ASSERT_OUT(expr) LOCAL_ASSERT_CUSTOM(expr, ok = JS_FALSE; \
                                                   goto enterblock_out)
                for (Shape::Range r = obj->lastProperty()->all(); !r.empty(); r.popFront()) {
                    const Shape &shape = r.front();

                    if (!shape.hasShortID())
                        continue;
                    LOCAL_ASSERT_OUT(shape.shortid < argc);
                    atomv[shape.shortid] = JSID_TO_ATOM(shape.id);
                }
                ok = JS_TRUE;
                for (i = 0; i < argc; i++) {
                    atom = atomv[i];
                    rval = QuoteString(&ss->sprinter, atom, 0);
                    if (!rval ||
                        !PushOff(ss, STR2OFF(&ss->sprinter, rval), op)) {
                        ok = JS_FALSE;
                        goto enterblock_out;
                    }
                }

                sn = js_GetSrcNote(jp->script, pc);
                switch (sn ? SN_TYPE(sn) : SRC_NULL) {
#if JS_HAS_BLOCK_SCOPE
                  case SRC_BRACE:
                    js_printf(jp, "\t{\n");
                    jp->indent += 4;
                    len = js_GetSrcNoteOffset(sn, 0);
                    ok = Decompile(ss, pc + oplen, len - oplen, JSOP_NOP)
                         != NULL;
                    if (!ok)
                        goto enterblock_out;
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");
                    break;
#endif

                  case SRC_CATCH:
                    jp->indent -= 4;
                    js_printf(jp, "\t} catch (");

                    pc2 = pc;
                    pc += oplen;
                    LOCAL_ASSERT_OUT(*pc == JSOP_EXCEPTION);
                    pc += JSOP_EXCEPTION_LENGTH;
                    todo = Sprint(&ss->sprinter, exception_cookie);
                    if (todo < 0 || !PushOff(ss, todo, JSOP_EXCEPTION)) {
                        ok = JS_FALSE;
                        goto enterblock_out;
                    }

                    if (*pc == JSOP_DUP) {
                        sn2 = js_GetSrcNote(jp->script, pc);
                        if (!sn2 || SN_TYPE(sn2) != SRC_DESTRUCT) {
                            




                            LOCAL_ASSERT_OUT(js_GetSrcNoteOffset(sn, 0) != 0);
                            pc += JSOP_DUP_LENGTH;
                            todo = Sprint(&ss->sprinter, exception_cookie);
                            if (todo < 0 ||
                                !PushOff(ss, todo, JSOP_EXCEPTION)) {
                                ok = JS_FALSE;
                                goto enterblock_out;
                            }
                        }
                    }

#if JS_HAS_DESTRUCTURING
                    if (*pc == JSOP_DUP) {
                        pc = DecompileDestructuring(ss, pc, endpc);
                        if (!pc) {
                            ok = JS_FALSE;
                            goto enterblock_out;
                        }
                        LOCAL_ASSERT_OUT(*pc == JSOP_POP);
                        pc += JSOP_POP_LENGTH;
                        lval = PopStr(ss, JSOP_NOP);
                        js_puts(jp, lval);
                    } else {
#endif
                        LOCAL_ASSERT_OUT(*pc == JSOP_SETLOCALPOP);
                        i = GET_SLOTNO(pc) - jp->script->nfixed;
                        pc += JSOP_SETLOCALPOP_LENGTH;
                        atom = atomv[i - OBJ_BLOCK_DEPTH(cx, obj)];
                        str = atom;
                        if (!QuoteString(&jp->sprinter, str, 0)) {
                            ok = JS_FALSE;
                            goto enterblock_out;
                        }
#if JS_HAS_DESTRUCTURING
                    }
#endif

                    



                    rval = PopStr(ss, JSOP_NOP);
                    LOCAL_ASSERT_OUT(strcmp(rval, exception_cookie) == 0);

                    len = js_GetSrcNoteOffset(sn, 0);
                    if (len) {
                        len -= pc - pc2;
                        LOCAL_ASSERT_OUT(len > 0);
                        js_printf(jp, " if ");
                        ok = Decompile(ss, pc, len, JSOP_NOP) != NULL;
                        if (!ok)
                            goto enterblock_out;
                        js_printf(jp, "%s", POP_STR());
                        pc += len;
                        LOCAL_ASSERT_OUT(*pc == JSOP_IFEQ || *pc == JSOP_IFEQX);
                        pc += js_CodeSpec[*pc].length;
                    }

                    js_printf(jp, ") {\n");
                    jp->indent += 4;
                    len = 0;
                    break;
                  default:
                    break;
                }

                todo = -2;

#undef LOCAL_ASSERT_OUT
              enterblock_out:
                if (atomv != smallv)
                    cx->free(atomv);
                if (!ok)
                    return NULL;
              }
              break;

              case JSOP_LEAVEBLOCK:
              case JSOP_LEAVEBLOCKEXPR:
              {
                uintN top, depth;

                sn = js_GetSrcNote(jp->script, pc);
                todo = -2;
                if (op == JSOP_LEAVEBLOCKEXPR) {
                    LOCAL_ASSERT(SN_TYPE(sn) == SRC_PCBASE);
                    rval = POP_STR();
                } else if (sn) {
                    LOCAL_ASSERT(op == JSOP_LEAVEBLOCK);
                    if (SN_TYPE(sn) == SRC_HIDDEN)
                        break;

                    






                    LOCAL_ASSERT(SN_TYPE(sn) == SRC_CATCH);
                    if ((uintN)js_GetSrcNoteOffset(sn, 0) != ss->top) {
                        LOCAL_ASSERT((uintN)js_GetSrcNoteOffset(sn, 0)
                                     == ss->top - 1);
                        rval = POP_STR();
                        LOCAL_ASSERT(strcmp(rval, exception_cookie) == 0);
                    }
                }
                top = ss->top;
                depth = GET_UINT16(pc);
                LOCAL_ASSERT(top >= depth);
                top -= depth;
                ss->top = top;
                ss->sprinter.offset = GetOff(ss, top);
                if (op == JSOP_LEAVEBLOCKEXPR)
                    todo = SprintCString(&ss->sprinter, rval);
                break;
              }

              case JSOP_GETUPVAR_DBG:
              case JSOP_CALLUPVAR_DBG:
              case JSOP_GETFCSLOT:
              case JSOP_CALLFCSLOT:
              {
                if (!jp->fun) {
                    JS_ASSERT(jp->script->savedCallerFun);
                    jp->fun = jp->script->getFunction(0);
                }

                if (!jp->localNames) {
                    JS_ASSERT(fun == jp->fun);
                    jp->localNames =
                        jp->fun->script()->bindings.getLocalNameArray(cx, &jp->pool);
                }

                uintN index = GET_UINT16(pc);
                if (index < jp->fun->script()->bindings.countUpvars()) {
                    index += jp->fun->script()->bindings.countArgsAndVars();
                } else {
                    JSUpvarArray *uva;
#ifdef DEBUG
                    








                    JSStackFrame *fp = js_GetTopStackFrame(cx);
                    if (fp) {
                        while (!fp->isEvalFrame())
                            fp = fp->prev();
                        JS_ASSERT(fp->script() == jp->script);
                        JS_ASSERT(fp->prev()->fun() == jp->fun);
                        JS_ASSERT(jp->fun->isInterpreted());
                        JS_ASSERT(jp->script != jp->fun->script());
                        JS_ASSERT(JSScript::isValidOffset(jp->script->upvarsOffset));
                    }
#endif
                    uva = jp->script->upvars();
                    index = uva->vector[index].slot();
                }
                atom = GetArgOrVarAtom(jp, index);
                goto do_name;
              }

              case JSOP_CALLLOCAL:
              case JSOP_GETLOCAL:
                if (IsVarSlot(jp, pc, &i)) {
                    atom = GetArgOrVarAtom(jp, i);
                    LOCAL_ASSERT(atom);
                    goto do_name;
                }
                LOCAL_ASSERT((uintN)i < ss->top);
                sn = js_GetSrcNote(jp->script, pc);

#if JS_HAS_DESTRUCTURING
                if (sn && SN_TYPE(sn) == SRC_GROUPASSIGN) {
                    





                    if (endpc - pc > JSOP_GETLOCAL_LENGTH || pc > startpc) {
                        pc = DecompileGroupAssignment(ss, pc, endpc, sn, &todo);
                        if (!pc)
                            return NULL;
                        LOCAL_ASSERT(*pc == JSOP_POPN);
                        len = oplen = JSOP_POPN_LENGTH;
                        goto end_groupassignment;
                    }

                    
                    sn = NULL;
                }
#endif

                rval = GetLocal(ss, i);
                todo = Sprint(&ss->sprinter, ss_format, VarPrefix(sn), rval);
                break;

              case JSOP_SETLOCAL:
              case JSOP_SETLOCALPOP:
                if (IsVarSlot(jp, pc, &i)) {
                    atom = GetArgOrVarAtom(jp, i);
                    LOCAL_ASSERT(atom);
                    goto do_setname;
                }
                lval = GetLocal(ss, i);
                rval = POP_STR();
                goto do_setlval;

              case JSOP_INCLOCAL:
              case JSOP_DECLOCAL:
                if (IsVarSlot(jp, pc, &i)) {
                    atom = GetArgOrVarAtom(jp, i);
                    LOCAL_ASSERT(atom);
                    goto do_incatom;
                }
                lval = GetLocal(ss, i);
                goto do_inclval;

              case JSOP_LOCALINC:
              case JSOP_LOCALDEC:
                if (IsVarSlot(jp, pc, &i)) {
                    atom = GetArgOrVarAtom(jp, i);
                    LOCAL_ASSERT(atom);
                    goto do_atominc;
                }
                lval = GetLocal(ss, i);
                goto do_lvalinc;

              case JSOP_RETRVAL:
                todo = -2;
                break;

              case JSOP_RETURN:
                LOCAL_ASSERT(jp->fun);
                fun = jp->fun;
                if (fun->flags & JSFUN_EXPR_CLOSURE) {
                    
                    op = JSOP_SETNAME;
                    rval = POP_STR();
                    js_printf(jp, (*rval == '{') ? "(%s)%s" : ss_format,
                              rval,
                              ((fun->flags & JSFUN_LAMBDA) || !fun->atom)
                              ? ""
                              : ";");
                    todo = -2;
                    break;
                }
                

              case JSOP_SETRVAL:
                rval = POP_STR();
                if (*rval != '\0')
                    js_printf(jp, "\t%s %s;\n", js_return_str, rval);
                else
                    js_printf(jp, "\t%s;\n", js_return_str);
                todo = -2;
                break;

#if JS_HAS_GENERATORS
              case JSOP_YIELD:
#if JS_HAS_GENERATOR_EXPRS
                if (!ss->inGenExp || !(sn = js_GetSrcNote(jp->script, pc)))
#endif
                {
                    
                    op = JSOP_SETNAME;
                    rval = POP_STR();
                    todo = (*rval != '\0')
                           ? Sprint(&ss->sprinter,
                                    (strncmp(rval, js_yield_str, 5) == 0 &&
                                     (rval[5] == ' ' || rval[5] == '\0'))
                                    ? "%s (%s)"
                                    : "%s %s",
                                    js_yield_str, rval)
                           : SprintCString(&ss->sprinter, js_yield_str);
                    break;
                }

#if JS_HAS_GENERATOR_EXPRS
                LOCAL_ASSERT(SN_TYPE(sn) == SRC_HIDDEN);
                
#endif

              case JSOP_ARRAYPUSH:
              {
                uintN pos, forpos;
                ptrdiff_t start;

                
                op = JSOP_SETNAME;

                
                rval = POP_STR();

                




                pos = ss->top;
                while (pos != 0) {
                    op = (JSOp) ss->opcodes[--pos];
                    if (op == JSOP_ENTERBLOCK)
                        break;
                }
                JS_ASSERT(op == JSOP_ENTERBLOCK);

                




                forpos = pos + 1;
                LOCAL_ASSERT(forpos < ss->top);

                



                while (ss->opcodes[pos] == JSOP_ENTERBLOCK) {
                    if (pos == 0)
                        break;
                    --pos;
                }

#if JS_HAS_GENERATOR_EXPRS
                if (saveop == JSOP_YIELD) {
                    







                    LOCAL_ASSERT(pos == 0);
                    xval = OFF2STR(&ss->sprinter, ss->offsets[forpos]);
                    ss->sprinter.offset = PAREN_SLOP;
                    todo = Sprint(&ss->sprinter, ss_format, rval, xval);
                    if (todo < 0)
                        return NULL;
                    ss->offsets[0] = todo;
                    ++ss->top;
                    return pc;
                }
#endif 

                



                JS_ASSERT(jp->script->nfixed + pos == GET_UINT16(pc));
                LOCAL_ASSERT(ss->opcodes[pos] == JSOP_NEWINIT);

                start = ss->offsets[pos];
                LOCAL_ASSERT(ss->sprinter.base[start] == '[' ||
                             ss->sprinter.base[start] == '#');
                LOCAL_ASSERT(forpos < ss->top);
                xval = OFF2STR(&ss->sprinter, ss->offsets[forpos]);
                lval = OFF2STR(&ss->sprinter, start);
                RETRACT(&ss->sprinter, lval);

                todo = Sprint(&ss->sprinter, sss_format, lval, rval, xval);
                if (todo < 0)
                    return NULL;
                ss->offsets[pos] = todo;
                todo = -2;
                break;
              }
#endif 

              case JSOP_THROWING:
                todo = -2;
                break;

              case JSOP_THROW:
                sn = js_GetSrcNote(jp->script, pc);
                todo = -2;
                if (sn && SN_TYPE(sn) == SRC_HIDDEN)
                    break;
                rval = POP_STR();
                js_printf(jp, "\t%s %s;\n", js_throw_str, rval);
                break;

              case JSOP_ITER:
                foreach = (pc[1] & (JSITER_FOREACH | JSITER_KEYVALUE)) ==
                          JSITER_FOREACH;
                todo = -2;
                break;

              case JSOP_MOREITER:
                JS_NOT_REACHED("JSOP_MOREITER");
                break;

              case JSOP_ENDITER:
                sn = js_GetSrcNote(jp->script, pc);
                todo = -2;
                if (sn && SN_TYPE(sn) == SRC_HIDDEN)
                    break;
                (void) PopOff(ss, op);
                break;

              case JSOP_GOTO:
              case JSOP_GOTOX:
                sn = js_GetSrcNote(jp->script, pc);
                switch (sn ? SN_TYPE(sn) : SRC_NULL) {
                  case SRC_FOR_IN:
                    






                    cond = GetJumpOffset(pc, pc);
                    next = js_GetSrcNoteOffset(sn, 0);
                    tail = js_GetSrcNoteOffset(sn, 1);
                    JS_ASSERT(pc[cond] == JSOP_MOREITER);
                    DECOMPILE_CODE(pc + oplen, next - oplen);
                    lval = POP_STR();
                    LOCAL_ASSERT(ss->top >= 1);

                    if (ss->inArrayInit || ss->inGenExp) {
                        rval = POP_STR();
                        if (ss->top >= 1 && ss->opcodes[ss->top - 1] == JSOP_FORLOCAL) {
                            ss->sprinter.offset = ss->offsets[ss->top] - PAREN_SLOP;
                            if (Sprint(&ss->sprinter, " %s (%s in %s)",
                                       foreach ? js_for_each_str : js_for_str,
                                       lval, rval) < 0) {
                                return NULL;
                            }

                            





                            todo = ss->offsets[ss->top - 1];
                        } else {
                            todo = Sprint(&ss->sprinter, " %s (%s in %s)",
                                          foreach ? js_for_each_str : js_for_str,
                                          lval, rval);
                        }
                        if (todo < 0 || !PushOff(ss, todo, JSOP_FORLOCAL))
                            return NULL;
                        DECOMPILE_CODE(pc + next, cond - next);
                    } else {
                        



                        rval = GetStr(ss, ss->top - 1);
                        js_printf(jp, "\t%s (%s in %s) {\n",
                                  foreach ? js_for_each_str : js_for_str,
                                  lval, rval);
                        jp->indent += 4;
                        DECOMPILE_CODE(pc + next, cond - next);
                        jp->indent -= 4;
                        js_printf(jp, "\t}\n");
                    }

                    pc += tail;
                    LOCAL_ASSERT(*pc == JSOP_IFNE || *pc == JSOP_IFNEX);
                    len = js_CodeSpec[*pc].length;
                    break;

                  case SRC_WHILE:
                    cond = GetJumpOffset(pc, pc);
                    tail = js_GetSrcNoteOffset(sn, 0);
                    DECOMPILE_CODE(pc + cond, tail - cond);
                    js_printf(jp, "\twhile (%s) {\n", POP_COND_STR());
                    jp->indent += 4;
                    DECOMPILE_CODE(pc + oplen, cond - oplen);
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");
                    pc += tail;
                    LOCAL_ASSERT(*pc == JSOP_IFNE || *pc == JSOP_IFNEX);
                    len = js_CodeSpec[*pc].length;
                    todo = -2;
                    break;

                  case SRC_CONT2LABEL:
                    GET_SOURCE_NOTE_ATOM(sn, atom);
                    rval = QuoteString(&ss->sprinter, atom, 0);
                    if (!rval)
                        return NULL;
                    RETRACT(&ss->sprinter, rval);
                    js_printf(jp, "\tcontinue %s;\n", rval);
                    break;

                  case SRC_CONTINUE:
                    js_printf(jp, "\tcontinue;\n");
                    break;

                  case SRC_BREAK2LABEL:
                    GET_SOURCE_NOTE_ATOM(sn, atom);
                    rval = QuoteString(&ss->sprinter, atom, 0);
                    if (!rval)
                        return NULL;
                    RETRACT(&ss->sprinter, rval);
                    js_printf(jp, "\tbreak %s;\n", rval);
                    break;

                  case SRC_HIDDEN:
                    break;

                  default:
                    js_printf(jp, "\tbreak;\n");
                    break;
                }
                todo = -2;
                break;

              case JSOP_IFEQ:
              case JSOP_IFEQX:
              {
                JSBool elseif = JS_FALSE;

              if_again:
                len = GetJumpOffset(pc, pc);
                sn = js_GetSrcNote(jp->script, pc);

                switch (sn ? SN_TYPE(sn) : SRC_NULL) {
                  case SRC_IF:
                  case SRC_IF_ELSE:
                    rval = POP_COND_STR();
                    if (ss->inArrayInit || ss->inGenExp) {
                        LOCAL_ASSERT(SN_TYPE(sn) == SRC_IF);
                        ss->sprinter.offset -= PAREN_SLOP;
                        if (Sprint(&ss->sprinter, " if (%s)", rval) < 0)
                            return NULL;
                        AddParenSlop(ss);
                    } else {
                        js_printf(jp,
                                  elseif ? " if (%s) {\n" : "\tif (%s) {\n",
                                  rval);
                        jp->indent += 4;
                    }

                    if (SN_TYPE(sn) == SRC_IF) {
                        DECOMPILE_CODE(pc + oplen, len - oplen);
                    } else {
                        LOCAL_ASSERT(!ss->inArrayInit && !ss->inGenExp);
                        tail = js_GetSrcNoteOffset(sn, 0);
                        DECOMPILE_CODE(pc + oplen, tail - oplen);
                        jp->indent -= 4;
                        pc += tail;
                        LOCAL_ASSERT(*pc == JSOP_GOTO || *pc == JSOP_GOTOX);
                        oplen = js_CodeSpec[*pc].length;
                        len = GetJumpOffset(pc, pc);
                        js_printf(jp, "\t} else");

                        







                        cond = js_GetSrcNoteOffset(sn, 1);
                        if (cond != 0) {
                            cond -= tail;
                            DECOMPILE_CODE(pc + oplen, cond - oplen);
                            pc += cond;
                            oplen = js_CodeSpec[*pc].length;
                            elseif = JS_TRUE;
                            goto if_again;
                        }

                        js_printf(jp, " {\n");
                        jp->indent += 4;
                        DECOMPILE_CODE(pc + oplen, len - oplen);
                    }

                    if (!ss->inArrayInit && !ss->inGenExp) {
                        jp->indent -= 4;
                        js_printf(jp, "\t}\n");
                    }
                    todo = -2;
                    break;

                  case SRC_COND:
                    xval = JS_strdup(cx, POP_STR());
                    if (!xval)
                        return NULL;
                    len = js_GetSrcNoteOffset(sn, 0);
                    DECOMPILE_CODE(pc + oplen, len - oplen);
                    lval = JS_strdup(cx, POP_STR());
                    if (!lval) {
                        cx->free((void *)xval);
                        return NULL;
                    }
                    pc += len;
                    LOCAL_ASSERT(*pc == JSOP_GOTO || *pc == JSOP_GOTOX);
                    oplen = js_CodeSpec[*pc].length;
                    len = GetJumpOffset(pc, pc);
                    DECOMPILE_CODE(pc + oplen, len - oplen);
                    rval = POP_STR();
                    todo = Sprint(&ss->sprinter, "%s ? %s : %s",
                                  xval, lval, rval);
                    cx->free((void *)xval);
                    cx->free((void *)lval);
                    break;

                  default:
                    break;
                }
                break;
              }

              case JSOP_IFNE:
              case JSOP_IFNEX:
                LOCAL_ASSERT(0);
                break;

              case JSOP_OR:
              case JSOP_ORX:
                xval = "||";

              do_logical_connective:
                
                lval = JS_strdup(cx, POP_STR());
                if (!lval)
                    return NULL;
                done = pc + GetJumpOffset(pc, pc);
                pc += len;
                len = done - pc;
                if (!Decompile(ss, pc, len, op)) {
                    cx->free((char *)lval);
                    return NULL;
                }
                rval = POP_STR();
                if (jp->pretty &&
                    jp->indent + 4 + strlen(lval) + 4 + strlen(rval) > 75) {
                    rval = JS_strdup(cx, rval);
                    if (!rval) {
                        tail = -1;
                    } else {
                        todo = Sprint(&ss->sprinter, "%s %s\n", lval, xval);
                        tail = Sprint(&ss->sprinter, "%*s%s",
                                      jp->indent + 4, "", rval);
                        cx->free((char *)rval);
                    }
                    if (tail < 0)
                        todo = -1;
                } else {
                    todo = Sprint(&ss->sprinter, "%s %s %s", lval, xval, rval);
                }
                cx->free((char *)lval);
                break;

              case JSOP_AND:
              case JSOP_ANDX:
                xval = "&&";
                goto do_logical_connective;

              case JSOP_FORARG:
                sn = NULL;
                i = GET_ARGNO(pc);
                goto do_forvarslot;

              case JSOP_FORLOCAL:
                sn = js_GetSrcNote(jp->script, pc);
                if (!IsVarSlot(jp, pc, &i)) {
                    JS_ASSERT(op == JSOP_FORLOCAL);
                    todo = Sprint(&ss->sprinter, ss_format, VarPrefix(sn), GetStr(ss, i));
                    break;
                }

              do_forvarslot:
                atom = GetArgOrVarAtom(jp, i);
                LOCAL_ASSERT(atom);
                todo = SprintCString(&ss->sprinter, VarPrefix(sn));
                if (todo < 0 || !QuoteString(&ss->sprinter, atom, 0))
                    return NULL;
                break;

              case JSOP_FORNAME:
              case JSOP_FORGNAME:
                LOAD_ATOM(0);

                sn = js_GetSrcNote(jp->script, pc);
                todo = SprintCString(&ss->sprinter, VarPrefix(sn));
                if (todo < 0 || !QuoteString(&ss->sprinter, atom, 0))
                    return NULL;
                break;

              case JSOP_FORPROP:
                xval = NULL;
                LOAD_ATOM(0);
                if (!ATOM_IS_IDENTIFIER(atom)) {
                    xval = QuoteString(&ss->sprinter, atom,
                                       (jschar)'\'');
                    if (!xval)
                        return NULL;
                }
                lval = POP_STR();
                if (xval) {
                    JS_ASSERT(*lval);
                    todo = Sprint(&ss->sprinter, index_format, lval, xval);
                } else {
                    todo = Sprint(&ss->sprinter, ss_format, lval, *lval ? "." : "");
                    if (todo < 0)
                        return NULL;
                    if (!QuoteString(&ss->sprinter, atom, 0))
                        return NULL;
                }
                break;

              case JSOP_FORELEM:
                todo = SprintCString(&ss->sprinter, forelem_cookie);
                break;

              case JSOP_ENUMELEM:
              case JSOP_ENUMCONSTELEM:
                






                atom = NULL;
                op = JSOP_NOP;          
                xval = POP_STR();
                op = JSOP_GETELEM;      
                lval = POP_STR();
                op = saveop;
                rval = POP_STR();
                LOCAL_ASSERT(strcmp(rval, forelem_cookie) == 0);
                if (*xval == '\0') {
                    todo = SprintCString(&ss->sprinter, lval);
                } else {
                    todo = Sprint(&ss->sprinter,
                                  (JOF_OPMODE(lastop) == JOF_XMLNAME)
                                  ? dot_format
                                  : index_format,
                                  lval, xval);
                }
                break;

              case JSOP_GETTER:
              case JSOP_SETTER:
                todo = -2;
                break;

              case JSOP_DUP2:
                rval = GetStr(ss, ss->top-2);
                todo = SprintCString(&ss->sprinter, rval);
                if (todo < 0 || !PushOff(ss, todo,
                                         (JSOp) ss->opcodes[ss->top-2])) {
                    return NULL;
                }
                

              case JSOP_DUP:
#if JS_HAS_DESTRUCTURING
                sn = js_GetSrcNote(jp->script, pc);
                if (sn) {
                    LOCAL_ASSERT(SN_TYPE(sn) == SRC_DESTRUCT);
                    pc = DecompileDestructuring(ss, pc, endpc);
                    if (!pc)
                        return NULL;
                    len = 0;
                    lval = POP_STR();
                    op = saveop = JSOP_ENUMELEM;
                    rval = POP_STR();

                    if (strcmp(rval, forelem_cookie) == 0) {
                        todo = Sprint(&ss->sprinter, ss_format,
                                      VarPrefix(sn), lval);

                        
                        if (*pc == JSOP_POP)
                            len = JSOP_POP_LENGTH;
                    } else {
                        todo = Sprint(&ss->sprinter, "%s%s = %s",
                                      VarPrefix(sn), lval, rval);
                    }
                    break;
                }
#endif

                rval = GetStr(ss, ss->top-1);
                saveop = (JSOp) ss->opcodes[ss->top-1];
                todo = SprintCString(&ss->sprinter, rval);
                break;

              case JSOP_SETARG:
                atom = GetArgOrVarAtom(jp, GET_ARGNO(pc));
                LOCAL_ASSERT(atom);
                goto do_setname;

              case JSOP_SETCONST:
              case JSOP_SETNAME:
              case JSOP_SETGNAME:
                LOAD_ATOM(0);

              do_setname:
                lval = QuoteString(&ss->sprinter, atom, 0);
                if (!lval)
                    return NULL;
                rval = POP_STR();
                if (op == JSOP_SETNAME || op == JSOP_SETGNAME)
                    (void) PopOff(ss, op);

              do_setlval:
                sn = js_GetSrcNote(jp->script, pc - 1);
                if (sn && SN_TYPE(sn) == SRC_ASSIGNOP) {
                    todo = Sprint(&ss->sprinter, "%s %s= %s",
                                  lval,
                                  (lastop == JSOP_GETTER)
                                  ? js_getter_str
                                  : (lastop == JSOP_SETTER)
                                  ? js_setter_str
                                  : CodeToken[lastop],
                                  rval);
                } else {
                    sn = js_GetSrcNote(jp->script, pc);
                    todo = Sprint(&ss->sprinter, "%s%s = %s",
                                  VarPrefix(sn), lval, rval);
                }
                if (op == JSOP_SETLOCALPOP) {
                    if (!PushOff(ss, todo, saveop))
                        return NULL;
                    rval = POP_STR();
                    LOCAL_ASSERT(*rval != '\0');
                    js_printf(jp, "\t%s;\n", rval);
                    todo = -2;
                }
                break;

              case JSOP_NEW:
              case JSOP_CALL:
              case JSOP_EVAL:
              case JSOP_FUNCALL:
              case JSOP_FUNAPPLY:
                argc = GET_ARGC(pc);
                argv = (char **)
                    cx->malloc((size_t)(argc + 1) * sizeof *argv);
                if (!argv)
                    return NULL;

                op = JSOP_SETNAME;
                ok = JS_TRUE;
                for (i = argc; i > 0; i--)
                    argv[i] = JS_strdup(cx, POP_STR());

                
                LOCAL_ASSERT(ss->top >= 2);
                (void) PopOff(ss, op);

                




                op = (JSOp) ss->opcodes[ss->top - 1];
                lval = PopStr(ss,
                              (saveop == JSOP_NEW &&
                               (op == JSOP_CALL ||
                                op == JSOP_EVAL ||
                                op == JSOP_FUNCALL ||
                                op == JSOP_FUNAPPLY ||
                                (js_CodeSpec[op].format & JOF_CALLOP)))
                              ? JSOP_NAME
                              : saveop);
                op = saveop;

                argv[0] = JS_strdup(cx, lval);
                if (!argv[0])
                    ok = JS_FALSE;

                lval = "(", rval = ")";
                if (op == JSOP_NEW) {
                    if (argc == 0)
                        lval = rval = "";
                    todo = Sprint(&ss->sprinter, "%s %s%s",
                                  js_new_str, argv[0], lval);
                } else {
                    todo = Sprint(&ss->sprinter, ss_format,
                                  argv[0], lval);
                }
                if (todo < 0)
                    ok = JS_FALSE;

                for (i = 1; i <= argc; i++) {
                    if (!argv[i] ||
                        Sprint(&ss->sprinter, ss_format,
                               argv[i], (i < argc) ? ", " : "") < 0) {
                        ok = JS_FALSE;
                        break;
                    }
                }
                if (Sprint(&ss->sprinter, rval) < 0)
                    ok = JS_FALSE;

                for (i = 0; i <= argc; i++)
                    cx->free(argv[i]);
                cx->free(argv);
                if (!ok)
                    return NULL;
                break;

              case JSOP_SETCALL:
                todo = Sprint(&ss->sprinter, "");
                break;

              case JSOP_DELNAME:
                LOAD_ATOM(0);
                lval = QuoteString(&ss->sprinter, atom, 0);
                if (!lval)
                    return NULL;
                RETRACT(&ss->sprinter, lval);
              do_delete_lval:
                todo = Sprint(&ss->sprinter, "%s %s", js_delete_str, lval);
                break;

              case JSOP_DELPROP:
                GET_ATOM_QUOTE_AND_FMT("%s %s[%s]", "%s %s.%s", rval);
                op = JSOP_GETPROP;
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, fmt, js_delete_str, lval, rval);
                break;

              case JSOP_DELELEM:
                op = JSOP_NOP;          
                xval = POP_STR();
                op = JSOP_GETPROP;
                lval = POP_STR();
                if (*xval == '\0')
                    goto do_delete_lval;
                todo = Sprint(&ss->sprinter,
                              (JOF_OPMODE(lastop) == JOF_XMLNAME)
                              ? "%s %s.%s"
                              : "%s %s[%s]",
                              js_delete_str, lval, xval);
                break;

#if JS_HAS_XML_SUPPORT
              case JSOP_DELDESC:
                xval = POP_STR();
                op = JSOP_GETPROP;
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s %s..%s",
                              js_delete_str, lval, xval);
                break;
#endif

              case JSOP_TYPEOFEXPR:
              case JSOP_TYPEOF:
              case JSOP_VOID:
                rval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s %s",
                              (op == JSOP_VOID) ? js_void_str : js_typeof_str,
                              rval);
                break;

              case JSOP_INCARG:
              case JSOP_DECARG:
                atom = GetArgOrVarAtom(jp, GET_ARGNO(pc));
                LOCAL_ASSERT(atom);
                goto do_incatom;

              case JSOP_INCNAME:
              case JSOP_DECNAME:
              case JSOP_INCGNAME:
              case JSOP_DECGNAME:
                LOAD_ATOM(0);
              do_incatom:
                lval = QuoteString(&ss->sprinter, atom, 0);
                if (!lval)
                    return NULL;
                RETRACT(&ss->sprinter, lval);
              do_inclval:
                todo = Sprint(&ss->sprinter, ss_format,
                              js_incop_strs[!(cs->format & JOF_INC)], lval);
                break;

              case JSOP_INCPROP:
              case JSOP_DECPROP:
                GET_ATOM_QUOTE_AND_FMT(preindex_format, predot_format, rval);

                




                op = JSOP_GETPROP;
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, fmt,
                              js_incop_strs[!(cs->format & JOF_INC)],
                              lval, rval);
                break;

              case JSOP_INCELEM:
              case JSOP_DECELEM:
                op = JSOP_NOP;          
                xval = POP_STR();
                op = JSOP_GETELEM;
                lval = POP_STR();
                if (*xval != '\0') {
                    todo = Sprint(&ss->sprinter,
                                  (JOF_OPMODE(lastop) == JOF_XMLNAME)
                                  ? predot_format
                                  : preindex_format,
                                  js_incop_strs[!(cs->format & JOF_INC)],
                                  lval, xval);
                } else {
                    todo = Sprint(&ss->sprinter, ss_format,
                                  js_incop_strs[!(cs->format & JOF_INC)], lval);
                }
                break;

              case JSOP_ARGINC:
              case JSOP_ARGDEC:
                atom = GetArgOrVarAtom(jp, GET_ARGNO(pc));
                LOCAL_ASSERT(atom);
                goto do_atominc;

              case JSOP_NAMEINC:
              case JSOP_NAMEDEC:
              case JSOP_GNAMEINC:
              case JSOP_GNAMEDEC:
                LOAD_ATOM(0);
              do_atominc:
                lval = QuoteString(&ss->sprinter, atom, 0);
                if (!lval)
                    return NULL;
                RETRACT(&ss->sprinter, lval);
              do_lvalinc:
                todo = Sprint(&ss->sprinter, ss_format,
                              lval, js_incop_strs[!(cs->format & JOF_INC)]);
                break;

              case JSOP_PROPINC:
              case JSOP_PROPDEC:
                GET_ATOM_QUOTE_AND_FMT(postindex_format, postdot_format, rval);

                




                op = JSOP_GETPROP;
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, fmt, lval, rval,
                              js_incop_strs[!(cs->format & JOF_INC)]);
                break;

              case JSOP_ELEMINC:
              case JSOP_ELEMDEC:
                op = JSOP_NOP;          
                xval = POP_STR();
                op = JSOP_GETELEM;
                lval = POP_STR();
                if (*xval != '\0') {
                    todo = Sprint(&ss->sprinter,
                                  (JOF_OPMODE(lastop) == JOF_XMLNAME)
                                  ? postdot_format
                                  : postindex_format,
                                  lval, xval,
                                  js_incop_strs[!(cs->format & JOF_INC)]);
                } else {
                    todo = Sprint(&ss->sprinter, ss_format,
                                  lval, js_incop_strs[!(cs->format & JOF_INC)]);
                }
                break;

              case JSOP_LENGTH:
                fmt = dot_format;
                rval = js_length_str;
                goto do_getprop_lval;

              case JSOP_GETPROP2:
                op = JSOP_GETPROP;
                (void) PopOff(ss, lastop);
                

              case JSOP_CALLPROP:
              case JSOP_GETPROP:
              case JSOP_GETXPROP:
                LOAD_ATOM(0);

              do_getprop:
                GET_QUOTE_AND_FMT(index_format, dot_format, rval);
              do_getprop_lval:
                PROPAGATE_CALLNESS();
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, fmt, lval, rval);
                break;

              case JSOP_GETTHISPROP:
                LOAD_ATOM(0);
                GET_QUOTE_AND_FMT(index_format, dot_format, rval);
                todo = Sprint(&ss->sprinter, fmt, js_this_str, rval);
                break;

              case JSOP_GETARGPROP:
                
                i = GET_ARGNO(pc);

              do_getarg_prop:
                atom = GetArgOrVarAtom(ss->printer, i);
                LOCAL_ASSERT(atom);
                lval = QuoteString(&ss->sprinter, atom, 0);
                if (!lval || !PushOff(ss, STR2OFF(&ss->sprinter, lval), op))
                    return NULL;

                
                LOAD_ATOM(ARGNO_LEN);
                goto do_getprop;

              case JSOP_GETLOCALPROP:
                if (IsVarSlot(jp, pc, &i))
                    goto do_getarg_prop;
                LOCAL_ASSERT((uintN)i < ss->top);
                lval = GetLocal(ss, i);
                if (!lval)
                    return NULL;
                todo = SprintCString(&ss->sprinter, lval);
                if (todo < 0 || !PushOff(ss, todo, op))
                    return NULL;
                LOAD_ATOM(2);
                goto do_getprop;

              case JSOP_SETPROP:
              case JSOP_SETMETHOD:
                LOAD_ATOM(0);
                GET_QUOTE_AND_FMT("%s[%s] %s= %s", "%s.%s %s= %s", xval);
                rval = POP_STR();

                




                op = JSOP_GETPROP;
                lval = POP_STR();
                sn = js_GetSrcNote(jp->script, pc - 1);
                todo = Sprint(&ss->sprinter, fmt, lval, xval,
                              (sn && SN_TYPE(sn) == SRC_ASSIGNOP)
                              ? (lastop == JSOP_GETTER)
                                ? js_getter_str
                                : (lastop == JSOP_SETTER)
                                ? js_setter_str
                                : CodeToken[lastop]
                              : "",
                              rval);
                break;

              case JSOP_GETELEM2:
                op = JSOP_GETELEM;
                (void) PopOff(ss, lastop);
                

              case JSOP_CALLELEM:
              case JSOP_GETELEM:
                op = JSOP_NOP;          
                xval = POP_STR();
                op = saveop;
                PROPAGATE_CALLNESS();
                lval = POP_STR();
                if (*xval == '\0') {
                    todo = Sprint(&ss->sprinter, "%s", lval);
                } else {
                    todo = Sprint(&ss->sprinter,
                                  (JOF_OPMODE(lastop) == JOF_XMLNAME)
                                  ? dot_format
                                  : index_format,
                                  lval, xval);
                }
                break;

              case JSOP_SETELEM:
                rval = POP_STR();
                op = JSOP_NOP;          
                xval = POP_STR();
                cs = &js_CodeSpec[ss->opcodes[ss->top]];
                op = JSOP_GETELEM;      
                lval = POP_STR();
                op = saveop;
                if (*xval == '\0')
                    goto do_setlval;
                sn = js_GetSrcNote(jp->script, pc - 1);
                todo = Sprint(&ss->sprinter,
                              (JOF_MODE(cs->format) == JOF_XMLNAME)
                              ? "%s.%s %s= %s"
                              : "%s[%s] %s= %s",
                              lval, xval,
                              (sn && SN_TYPE(sn) == SRC_ASSIGNOP)
                              ? (lastop == JSOP_GETTER)
                                ? js_getter_str
                                : (lastop == JSOP_SETTER)
                                ? js_setter_str
                                : CodeToken[lastop]
                              : "",
                              rval);
                break;

              case JSOP_ARGSUB:
                i = (jsint) GET_ARGNO(pc);
                todo = Sprint(&ss->sprinter, "%s[%d]",
                              js_arguments_str, (int) i);
                break;

              case JSOP_ARGCNT:
                todo = Sprint(&ss->sprinter, dot_format,
                              js_arguments_str, js_length_str);
                break;

              case JSOP_CALLARG:
              case JSOP_GETARG:
                i = GET_ARGNO(pc);
                atom = GetArgOrVarAtom(jp, i);
#if JS_HAS_DESTRUCTURING
                if (!atom) {
                    todo = Sprint(&ss->sprinter, "%s[%d]", js_arguments_str, i);
                    break;
                }
#else
                LOCAL_ASSERT(atom);
#endif
                goto do_name;

              case JSOP_CALLGLOBAL:
              case JSOP_GETGLOBAL:
                atom = jp->script->getGlobalAtom(GET_SLOTNO(pc));
                goto do_name;

              case JSOP_CALLNAME:
              case JSOP_NAME:
              case JSOP_GETGNAME:
              case JSOP_CALLGNAME:
                LOAD_ATOM(0);
              do_name:
                lval = "";
#if JS_HAS_XML_SUPPORT
              do_qname:
#endif
                sn = js_GetSrcNote(jp->script, pc);
                rval = QuoteString(&ss->sprinter, atom, inXML ? DONT_ESCAPE : 0);
                if (!rval)
                    return NULL;
                RETRACT(&ss->sprinter, rval);
                todo = Sprint(&ss->sprinter, sss_format,
                              VarPrefix(sn), lval, rval);
                break;

              case JSOP_UINT16:
                i = (jsint) GET_UINT16(pc);
                goto do_sprint_int;

              case JSOP_UINT24:
                i = (jsint) GET_UINT24(pc);
                goto do_sprint_int;

              case JSOP_INT8:
                i = GET_INT8(pc);
                goto do_sprint_int;

              case JSOP_INT32:
                i = GET_INT32(pc);
              do_sprint_int:
                todo = Sprint(&ss->sprinter, "%d", i);
                break;

              case JSOP_DOUBLE:
              {
                double d;
                GET_DOUBLE_FROM_BYTECODE(jp->script, pc, 0, d);
                val = DOUBLE_TO_JSVAL(d);
                todo = SprintDoubleValue(&ss->sprinter, val, &saveop);
                break;
              }

              case JSOP_STRING:
                LOAD_ATOM(0);
                rval = QuoteString(&ss->sprinter, atom, inXML ? DONT_ESCAPE : '"');
                if (!rval)
                    return NULL;
                todo = STR2OFF(&ss->sprinter, rval);
                break;

              case JSOP_LAMBDA:
              case JSOP_LAMBDA_FC:
              case JSOP_LAMBDA_DBGFC:
#if JS_HAS_GENERATOR_EXPRS
                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_GENEXP) {
                    void *mark;
                    jsuword *innerLocalNames, *outerLocalNames;
                    JSScript *inner, *outer;
                    SprintStack ss2;
                    JSFunction *outerfun;

                    LOAD_FUNCTION(0);

                    





                    mark = JS_ARENA_MARK(&cx->tempPool);
                    if (fun->script()->bindings.hasLocalNames()) {
                        innerLocalNames =
                            fun->script()->bindings.getLocalNameArray(cx, &cx->tempPool);
                        if (!innerLocalNames)
                            return NULL;
                    } else {
                        innerLocalNames = NULL;
                    }
                    inner = fun->script();
                    if (!InitSprintStack(cx, &ss2, jp, StackDepth(inner))) {
                        JS_ARENA_RELEASE(&cx->tempPool, mark);
                        return NULL;
                    }
                    ss2.inGenExp = JS_TRUE;

                    






                    outer = jp->script;
                    outerfun = jp->fun;
                    outerLocalNames = jp->localNames;
                    LOCAL_ASSERT(JS_UPTRDIFF(pc, outer->code) <= outer->length);
                    jp->script = inner;
                    jp->fun = fun;
                    jp->localNames = innerLocalNames;

                    



                    ok = Decompile(&ss2, inner->main,
                                   inner->length - (inner->main - inner->code),
                                   JSOP_NOP)
                         != NULL;
                    jp->script = outer;
                    jp->fun = outerfun;
                    jp->localNames = outerLocalNames;
                    if (!ok) {
                        JS_ARENA_RELEASE(&cx->tempPool, mark);
                        return NULL;
                    }

                    



                    pc += len;
                    if (*pc == JSOP_BLOCKCHAIN) {
                        pc += JSOP_BLOCKCHAIN_LENGTH;
                    } else {
                        LOCAL_ASSERT(*pc == JSOP_NULLBLOCKCHAIN);
                        pc += JSOP_NULLBLOCKCHAIN_LENGTH;
                    }
                    LOCAL_ASSERT(*pc == JSOP_PUSH);
                    pc += JSOP_PUSH_LENGTH;
                    LOCAL_ASSERT(*pc == JSOP_CALL);
                    LOCAL_ASSERT(GET_ARGC(pc) == 0);
                    len = JSOP_CALL_LENGTH;

                    















                    pc2 = pc + len;
                    op = JSOp(*pc2);
                    if (op == JSOP_TRACE || op == JSOP_NOP)
                        pc2 += JSOP_NOP_LENGTH;
                    LOCAL_ASSERT(pc2 < endpc ||
                                 endpc < outer->code + outer->length);
                    LOCAL_ASSERT(ss2.top == 1);
                    ss2.opcodes[0] = JSOP_POP;
                    if (pc2 == endpc) {
                        op = JSOP_SETNAME;
                    } else {
                        op = (JSOp) *pc2;
                        op = ((js_CodeSpec[op].format & JOF_PARENHEAD) ||
                              ((js_CodeSpec[op].format & JOF_INVOKE) && GET_ARGC(pc2) == 1))
                             ? JSOP_POP
                             : JSOP_SETNAME;

                        






                        LOCAL_ASSERT(js_CodeSpec[JSOP_NAME].prec ==
                                     js_CodeSpec[saveop].prec);
                        saveop = JSOP_NAME;
                    }

                    




                    rval = JS_strdup(cx, PopStr(&ss2, op));
                    JS_ARENA_RELEASE(&cx->tempPool, mark);
                    if (!rval)
                        return NULL;
                    todo = SprintCString(&ss->sprinter, rval);
                    cx->free((void *)rval);
                    break;
                }
#endif 
                

                LOAD_FUNCTION(0);
                {
                    





                    bool grouped = !(fun->flags & JSFUN_EXPR_CLOSURE);
                    bool strict = jp->script->strictModeCode;
                    str = js_DecompileToString(cx, "lambda", fun, 0, 
                                               false, grouped, strict,
                                               js_DecompileFunction);
                    if (!str)
                        return NULL;
                }
              sprint_string:
                todo = SprintString(&ss->sprinter, str);
                break;

              case JSOP_CALLEE:
                JS_ASSERT(jp->fun && jp->fun->atom);
                todo = SprintString(&ss->sprinter, jp->fun->atom);
                break;

              case JSOP_OBJECT:
                LOAD_OBJECT(0);
                str = js_ValueToSource(cx, ObjectValue(*obj));
                if (!str)
                    return NULL;
                goto sprint_string;

              case JSOP_REGEXP:
                GET_REGEXP_FROM_BYTECODE(jp->script, pc, 0, obj);
                if (!js_regexp_toString(cx, obj, Valueify(&val)))
                    return NULL;
                str = JSVAL_TO_STRING(val);
                goto sprint_string;

              case JSOP_TABLESWITCH:
              case JSOP_TABLESWITCHX:
              {
                ptrdiff_t jmplen, off, off2;
                jsint j, n, low, high;
                TableEntry *table, *tmp;

                sn = js_GetSrcNote(jp->script, pc);
                LOCAL_ASSERT(sn && SN_TYPE(sn) == SRC_SWITCH);
                len = js_GetSrcNoteOffset(sn, 0);
                jmplen = (op == JSOP_TABLESWITCH) ? JUMP_OFFSET_LEN
                                                  : JUMPX_OFFSET_LEN;
                pc2 = pc;
                off = GetJumpOffset(pc, pc2);
                pc2 += jmplen;
                low = GET_JUMP_OFFSET(pc2);
                pc2 += JUMP_OFFSET_LEN;
                high = GET_JUMP_OFFSET(pc2);
                pc2 += JUMP_OFFSET_LEN;

                n = high - low + 1;
                if (n == 0) {
                    table = NULL;
                    j = 0;
                    ok = JS_TRUE;
                } else {
                    table = (TableEntry *)
                            cx->malloc((size_t)n * sizeof *table);
                    if (!table)
                        return NULL;
                    for (i = j = 0; i < n; i++) {
                        table[j].label = NULL;
                        off2 = GetJumpOffset(pc, pc2);
                        if (off2) {
                            sn = js_GetSrcNote(jp->script, pc2);
                            if (sn) {
                                LOCAL_ASSERT(SN_TYPE(sn) == SRC_LABEL);
                                GET_SOURCE_NOTE_ATOM(sn, table[j].label);
                            }
                            table[j].key = INT_TO_JSVAL(low + i);
                            table[j].offset = off2;
                            table[j].order = j;
                            j++;
                        }
                        pc2 += jmplen;
                    }
                    tmp = (TableEntry *)
                          cx->malloc((size_t)j * sizeof *table);
                    if (tmp) {
                        VOUCH_DOES_NOT_REQUIRE_STACK();
                        ok = js_MergeSort(table, (size_t)j, sizeof(TableEntry),
                                          CompareOffsets, NULL, tmp,
                                          JS_SORTING_GENERIC);
                        cx->free(tmp);
                    } else {
                        ok = JS_FALSE;
                    }
                }

                if (ok) {
                    ok = DecompileSwitch(ss, table, (uintN)j, pc, len, off,
                                         JS_FALSE);
                }
                cx->free(table);
                if (!ok)
                    return NULL;
                todo = -2;
                break;
              }

              case JSOP_LOOKUPSWITCH:
              case JSOP_LOOKUPSWITCHX:
              {
                ptrdiff_t jmplen, off, off2;
                jsatomid npairs, k;
                TableEntry *table;

                sn = js_GetSrcNote(jp->script, pc);
                LOCAL_ASSERT(sn && SN_TYPE(sn) == SRC_SWITCH);
                len = js_GetSrcNoteOffset(sn, 0);
                jmplen = (op == JSOP_LOOKUPSWITCH) ? JUMP_OFFSET_LEN
                                                   : JUMPX_OFFSET_LEN;
                pc2 = pc;
                off = GetJumpOffset(pc, pc2);
                pc2 += jmplen;
                npairs = GET_UINT16(pc2);
                pc2 += UINT16_LEN;

                table = (TableEntry *)
                    cx->malloc((size_t)npairs * sizeof *table);
                if (!table)
                    return NULL;
                for (k = 0; k < npairs; k++) {
                    sn = js_GetSrcNote(jp->script, pc2);
                    if (sn) {
                        LOCAL_ASSERT(SN_TYPE(sn) == SRC_LABEL);
                        GET_SOURCE_NOTE_ATOM(sn, table[k].label);
                    } else {
                        table[k].label = NULL;
                    }
                    uint16 constIndex = GET_INDEX(pc2);
                    pc2 += INDEX_LEN;
                    off2 = GetJumpOffset(pc, pc2);
                    pc2 += jmplen;
                    table[k].key = Jsvalify(jp->script->getConst(constIndex));
                    table[k].offset = off2;
                }

                ok = DecompileSwitch(ss, table, (uintN)npairs, pc, len, off,
                                     JS_FALSE);
                cx->free(table);
                if (!ok)
                    return NULL;
                todo = -2;
                break;
              }

              case JSOP_CONDSWITCH:
              {
                ptrdiff_t off, off2, caseOff;
                jsint ncases;
                TableEntry *table;

                sn = js_GetSrcNote(jp->script, pc);
                LOCAL_ASSERT(sn && SN_TYPE(sn) == SRC_SWITCH);
                len = js_GetSrcNoteOffset(sn, 0);
                off = js_GetSrcNoteOffset(sn, 1);

                



                pc2 = pc;
                off2 = off;
                for (ncases = 0; off2 != 0; ncases++) {
                    pc2 += off2;
                    LOCAL_ASSERT(*pc2 == JSOP_CASE || *pc2 == JSOP_DEFAULT ||
                                 *pc2 == JSOP_CASEX || *pc2 == JSOP_DEFAULTX);
                    if (*pc2 == JSOP_DEFAULT || *pc2 == JSOP_DEFAULTX) {
                        
                        off2 = 0;
                    } else {
                        sn = js_GetSrcNote(jp->script, pc2);
                        LOCAL_ASSERT(sn && SN_TYPE(sn) == SRC_PCDELTA);
                        off2 = js_GetSrcNoteOffset(sn, 0);
                    }
                }

                




                table = (TableEntry *)
                    cx->malloc((size_t)ncases * sizeof *table);
                if (!table)
                    return NULL;
                pc2 = pc;
                off2 = off;
                for (i = 0; i < ncases; i++) {
                    pc2 += off2;
                    LOCAL_ASSERT(*pc2 == JSOP_CASE || *pc2 == JSOP_DEFAULT ||
                                 *pc2 == JSOP_CASEX || *pc2 == JSOP_DEFAULTX);
                    caseOff = pc2 - pc;
                    table[i].key = INT_TO_JSVAL((jsint) caseOff);
                    table[i].offset = caseOff + GetJumpOffset(pc2, pc2);
                    if (*pc2 == JSOP_CASE || *pc2 == JSOP_CASEX) {
                        sn = js_GetSrcNote(jp->script, pc2);
                        LOCAL_ASSERT(sn && SN_TYPE(sn) == SRC_PCDELTA);
                        off2 = js_GetSrcNoteOffset(sn, 0);
                    }
                }

                




                off = JSVAL_TO_INT(table[ncases-1].key);
                pc2 = pc + off;
                off += GetJumpOffset(pc2, pc2);

                ok = DecompileSwitch(ss, table, (uintN)ncases, pc, len, off,
                                     JS_TRUE);
                cx->free(table);
                if (!ok)
                    return NULL;
                todo = -2;
                break;
              }

              case JSOP_CASE:
              case JSOP_CASEX:
              {
                lval = POP_STR();
                if (!lval)
                    return NULL;
                js_printf(jp, "\tcase %s:\n", lval);
                todo = -2;
                break;
              }

              case JSOP_DEFFUN:
              case JSOP_DEFFUN_FC:
              case JSOP_DEFFUN_DBGFC:
                LOAD_FUNCTION(0);
                todo = -2;
                goto do_function;
                break;

              case JSOP_TRAP:
                saveop = op = JS_GetTrapOpcode(cx, jp->script, pc);
                *pc = op;
                cs = &js_CodeSpec[op];
                len = cs->length;
                DECOMPILE_CODE(pc, len);
                *pc = JSOP_TRAP;
                todo = -2;
                break;

              case JSOP_HOLE:
                todo = SprintPut(&ss->sprinter, "", 0);
                break;

              case JSOP_NEWINIT:
              {
                i = pc[1];
                LOCAL_ASSERT(i == JSProto_Array || i == JSProto_Object);

                todo = ss->sprinter.offset;
#if JS_HAS_SHARP_VARS
                op = (JSOp)pc[len];
                if (op == JSOP_SHARPINIT)
                    op = (JSOp)pc[len += JSOP_SHARPINIT_LENGTH];
                if (op == JSOP_DEFSHARP) {
                    pc += len;
                    cs = &js_CodeSpec[op];
                    len = cs->length;
                    if (Sprint(&ss->sprinter, "#%u=",
                               (unsigned) (jsint) GET_UINT16(pc + UINT16_LEN))
                        < 0) {
                        return NULL;
                    }
                }
#endif 
                if (i == JSProto_Array) {
                    ++ss->inArrayInit;
                    if (SprintCString(&ss->sprinter, "[") < 0)
                        return NULL;
                } else {
                    if (SprintCString(&ss->sprinter, "{") < 0)
                        return NULL;
                }
                break;
              }

              case JSOP_NEWARRAY:
              {
                todo = ss->sprinter.offset;
                ++ss->inArrayInit;
                if (SprintCString(&ss->sprinter, "[") < 0)
                    return NULL;
                break;
              }

              case JSOP_NEWOBJECT:
              {
                todo = ss->sprinter.offset;
                if (SprintCString(&ss->sprinter, "{") < 0)
                    return NULL;
                break;
              }

              case JSOP_ENDINIT:
              {
                JSBool inArray;

                op = JSOP_NOP;           
                rval = POP_STR();
                sn = js_GetSrcNote(jp->script, pc);

                
                for (xval = rval; *xval != '[' && *xval != '{'; xval++)
                    continue;
                inArray = (*xval == '[');
                if (inArray)
                    --ss->inArrayInit;
                todo = Sprint(&ss->sprinter, "%s%s%c",
                              rval,
                              (sn && SN_TYPE(sn) == SRC_CONTINUE) ? ", " : "",
                              inArray ? ']' : '}');
                break;
              }

              {
                JSBool isFirst;
                const char *maybeComma;

              case JSOP_INITELEM:
                isFirst = IsInitializerOp(ss->opcodes[ss->top - 3]);

                
                op = JSOP_SETNAME;
                rval = POP_STR();

                
                op = JSOP_NOP;
                xval = POP_STR();
                lval = POP_STR();
                sn = js_GetSrcNote(jp->script, pc);

                if (sn && SN_TYPE(sn) == SRC_INITPROP) {
                    atom = NULL;
                    goto do_initprop;
                }
                maybeComma = isFirst ? "" : ", ";
                todo = Sprint(&ss->sprinter, sss_format,
                              lval,
                              maybeComma,
                              rval);
                break;

              case JSOP_INITPROP:
              case JSOP_INITMETHOD:
                LOAD_ATOM(0);
                xval = QuoteString(&ss->sprinter, atom,
                                   jschar(ATOM_IS_IDENTIFIER(atom) ? 0 : '\''));
                if (!xval)
                    return NULL;
                isFirst = IsInitializerOp(ss->opcodes[ss->top - 2]);
                rval = POP_STR();
                lval = POP_STR();
                

              do_initprop:
                maybeComma = isFirst ? "" : ", ";
                if (lastop == JSOP_GETTER || lastop == JSOP_SETTER) {
                    const char *end = rval + strlen(rval);

                    if (*rval == '(')
                        ++rval, --end;
                    LOCAL_ASSERT(strncmp(rval, js_function_str, 8) == 0);
                    LOCAL_ASSERT(rval[8] == ' ');
                    rval += 8 + 1;
                    LOCAL_ASSERT(*end ? *end == ')' : end[-1] == '}');
                    todo = Sprint(&ss->sprinter, "%s%s%s %s%s%.*s",
                                    lval,
                                    maybeComma,
                                    (lastop == JSOP_GETTER)
                                    ? js_get_str : js_set_str,
                                    xval,
                                    (rval[0] != '(') ? " " : "",
                                    end - rval, rval);
                } else {
                    todo = Sprint(&ss->sprinter, "%s%s%s: %s",
                                  lval, maybeComma, xval, rval);
                }
                break;
              }

#if JS_HAS_SHARP_VARS
              case JSOP_DEFSHARP:
                i = (jsint) GET_UINT16(pc + UINT16_LEN);
                rval = POP_STR();
                todo = Sprint(&ss->sprinter, "#%u=%s", (unsigned) i, rval);
                break;

              case JSOP_USESHARP:
                i = (jsint) GET_UINT16(pc + UINT16_LEN);
                todo = Sprint(&ss->sprinter, "#%u#", (unsigned) i);
                break;
#endif 

              case JSOP_DEBUGGER:
                js_printf(jp, "\tdebugger;\n");
                todo = -2;
                break;

#if JS_HAS_XML_SUPPORT
              case JSOP_STARTXML:
              case JSOP_STARTXMLEXPR:
                inXML = op == JSOP_STARTXML;
                todo = -2;
                break;

              case JSOP_DEFXMLNS:
                rval = POP_STR();
                js_printf(jp, "\t%s %s %s = %s;\n",
                          js_default_str, js_xml_str, js_namespace_str, rval);
                todo = -2;
                break;

              case JSOP_ANYNAME:
                if (pc[JSOP_ANYNAME_LENGTH] == JSOP_TOATTRNAME) {
                    len += JSOP_TOATTRNAME_LENGTH;
                    todo = SprintPut(&ss->sprinter, "@*", 2);
                } else {
                    todo = SprintPut(&ss->sprinter, "*", 1);
                }
                break;

              case JSOP_QNAMEPART:
                LOAD_ATOM(0);
                if (pc[JSOP_QNAMEPART_LENGTH] == JSOP_TOATTRNAME) {
                    saveop = JSOP_TOATTRNAME;
                    len += JSOP_TOATTRNAME_LENGTH;
                    lval = "@";
                    goto do_qname;
                }
                goto do_name;

              case JSOP_QNAMECONST:
                LOAD_ATOM(0);
                rval = QuoteString(&ss->sprinter, atom, 0);
                if (!rval)
                    return NULL;
                RETRACT(&ss->sprinter, rval);
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s::%s", lval, rval);
                break;

              case JSOP_QNAME:
                rval = POP_STR();
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s::[%s]", lval, rval);
                break;

              case JSOP_TOATTRNAME:
                op = JSOP_NOP;           
                rval = POP_STR();
                todo = Sprint(&ss->sprinter, "@[%s]", rval);
                break;

              case JSOP_TOATTRVAL:
                todo = -2;
                break;

              case JSOP_ADDATTRNAME:
                rval = POP_STR();
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s %s", lval, rval);
                
                quoteAttr = JS_TRUE;
                break;

              case JSOP_ADDATTRVAL:
                rval = POP_STR();
                lval = POP_STR();
                if (quoteAttr)
                    todo = Sprint(&ss->sprinter, "%s=\"%s\"", lval, rval);
                else
                    todo = Sprint(&ss->sprinter, "%s=%s", lval, rval);
                break;

              case JSOP_BINDXMLNAME:
                
                todo = Sprint(&ss->sprinter, "");
                break;

              case JSOP_SETXMLNAME:
                
                rval = POP_STR();
                (void) PopOff(ss, op);
                lval = POP_STR();
                goto do_setlval;

              case JSOP_XMLELTEXPR:
              case JSOP_XMLTAGEXPR:
                todo = Sprint(&ss->sprinter, "{%s}", POP_STR());
                inXML = JS_TRUE;
                
                quoteAttr = JS_FALSE;
                break;

              case JSOP_TOXMLLIST:
                op = JSOP_NOP;           
                todo = Sprint(&ss->sprinter, "<>%s</>", POP_STR());
                inXML = JS_FALSE;
                break;

              case JSOP_TOXML:
              case JSOP_CALLXMLNAME:
              case JSOP_XMLNAME:
              case JSOP_FILTER:
                
                inXML = JS_FALSE;
                todo = -2;
                break;

              case JSOP_ENDFILTER:
                rval = POP_STR();
                PROPAGATE_CALLNESS();
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s.(%s)", lval, rval);
                break;

              case JSOP_DESCENDANTS:
                rval = POP_STR();
                PROPAGATE_CALLNESS();
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s..%s", lval, rval);
                break;

              case JSOP_XMLCDATA:
                LOAD_ATOM(0);
                todo = SprintPut(&ss->sprinter, "<![CDATA[", 9);
                if (!QuoteString(&ss->sprinter, atom, DONT_ESCAPE))
                    return NULL;
                SprintPut(&ss->sprinter, "]]>", 3);
                break;

              case JSOP_XMLCOMMENT:
                LOAD_ATOM(0);
                todo = SprintPut(&ss->sprinter, "<!--", 4);
                if (!QuoteString(&ss->sprinter, atom, DONT_ESCAPE))
                    return NULL;
                SprintPut(&ss->sprinter, "-->", 3);
                break;

              case JSOP_XMLPI:
                LOAD_ATOM(0);
                rval = JS_strdup(cx, POP_STR());
                if (!rval)
                    return NULL;
                todo = SprintPut(&ss->sprinter, "<?", 2);
                ok = QuoteString(&ss->sprinter, atom, 0) &&
                     (*rval == '\0' ||
                      (SprintPut(&ss->sprinter, " ", 1) >= 0 &&
                       SprintCString(&ss->sprinter, rval)));
                cx->free((char *)rval);
                if (!ok)
                    return NULL;
                SprintPut(&ss->sprinter, "?>", 2);
                break;

              case JSOP_GETFUNNS:
                todo = SprintPut(&ss->sprinter, js_function_str, 8);
                break;
#endif 

              default:
                todo = -2;
                break;
            }
        }

        if (todo < 0) {
            
            if (todo == -1)
                return NULL;
        } else {
            if (!PushOff(ss, todo, saveop))
                return NULL;
        }

        if (cs->format & JOF_CALLOP) {
            todo = Sprint(&ss->sprinter, "");
            if (todo < 0 || !PushOff(ss, todo, saveop))
                return NULL;
        }

        pc += len;
    }




#undef inXML
#undef DECOMPILE_CODE
#undef NEXT_OP
#undef TOP_STR
#undef POP_STR
#undef POP_STR_PREC
#undef LOCAL_ASSERT
#undef ATOM_IS_IDENTIFIER
#undef GET_QUOTE_AND_FMT
#undef GET_ATOM_QUOTE_AND_FMT

    return pc;
}

static JSBool
DecompileCode(JSPrinter *jp, JSScript *script, jsbytecode *pc, uintN len,
              uintN pcdepth)
{
    uintN depth, i;
    SprintStack ss;
    JSContext *cx;
    void *mark;
    JSBool ok;
    JSScript *oldscript;
    char *last;

    depth = StackDepth(script);
    JS_ASSERT(pcdepth <= depth);
    cx = jp->sprinter.context;

    AutoScriptUntrapper untrapper(cx, script, &pc);

    
    mark = JS_ARENA_MARK(&cx->tempPool);
    ok = InitSprintStack(cx, &ss, jp, depth);
    if (!ok)
        goto out;

    










    ss.top = pcdepth;
    if (pcdepth != 0) {
        for (i = 0; i < pcdepth; i++) {
            ss.offsets[i] = -2 - (ptrdiff_t)i;
            ss.opcodes[i] = *jp->pcstack[i];
        }
    }

    
    oldscript = jp->script;
    jp->script = script;
    ok = Decompile(&ss, pc, len, JSOP_NOP) != NULL;
    jp->script = oldscript;

    
    if (ok && ss.top) {
        do {
            last = OFF2STR(&ss.sprinter, PopOff(&ss, JSOP_POP));
        } while (ss.top > pcdepth);
        js_printf(jp, "%s", last);
    }

out:
    
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return ok;
}






static JSBool
DecompileBody(JSPrinter *jp, JSScript *script, jsbytecode *pc)
{
    
    if (script->strictModeCode && !jp->strict) {
        if (jp->fun && (jp->fun->flags & JSFUN_EXPR_CLOSURE)) {
            



            js_printf(jp, "\t/* use strict */ \n");
        } else {
            js_printf(jp, "\t\"use strict\";\n");
        }
        jp->strict = true;
    }

    jsbytecode *end = script->code + script->length;
    return DecompileCode(jp, script, pc, end - pc, 0);
}

JSBool
js_DecompileScript(JSPrinter *jp, JSScript *script)
{
    return DecompileBody(jp, script, script->code);
}

JSString *
js_DecompileToString(JSContext *cx, const char *name, JSFunction *fun,
                     uintN indent, JSBool pretty, JSBool grouped, JSBool strict,
                     JSDecompilerPtr decompiler)
{
    JSPrinter *jp;
    JSString *str;

    jp = js_NewPrinter(cx, name, fun, indent, pretty, grouped, strict);
    if (!jp)
        return NULL;
    if (decompiler(jp))
        str = js_GetPrinterOutput(jp);
    else
        str = NULL;
    js_DestroyPrinter(jp);
    return str;
}

static const char native_code_str[] = "\t[native code]\n";

JSBool
js_DecompileFunctionBody(JSPrinter *jp)
{
    JSScript *script;

    JS_ASSERT(jp->fun);
    JS_ASSERT(!jp->script);
    if (!FUN_INTERPRETED(jp->fun)) {
        js_printf(jp, native_code_str);
        return JS_TRUE;
    }

    script = jp->fun->u.i.script;
    return DecompileBody(jp, script, script->code);
}

JSBool
js_DecompileFunction(JSPrinter *jp)
{
    JSFunction *fun;
    uintN i;
    JSAtom *param;
    jsbytecode *pc, *endpc;
    JSBool ok;

    fun = jp->fun;
    JS_ASSERT(fun);
    JS_ASSERT(!jp->script);

    




    if (jp->pretty) {
        js_printf(jp, "\t");
    } else {
        if (!jp->grouped && (fun->flags & JSFUN_LAMBDA))
            js_puts(jp, "(");
    }

    js_printf(jp, "%s ", js_function_str);
    if (fun->atom && !QuoteString(&jp->sprinter, fun->atom, 0))
        return JS_FALSE;
    js_puts(jp, "(");

    if (!FUN_INTERPRETED(fun)) {
        js_printf(jp, ") {\n");
        jp->indent += 4;
        js_printf(jp, native_code_str);
        jp->indent -= 4;
        js_printf(jp, "\t}");
    } else {
        JSScript *script = fun->u.i.script;
#if JS_HAS_DESTRUCTURING
        SprintStack ss;
        void *mark;
#endif

        
        pc = script->main;
        AutoScriptUntrapper untrapper(jp->sprinter.context, script, &pc);
        endpc = pc + script->length;
        ok = JS_TRUE;

#if JS_HAS_DESTRUCTURING
        ss.printer = NULL;
        jp->script = script;
        mark = JS_ARENA_MARK(&jp->sprinter.context->tempPool);
#endif

        for (i = 0; i < fun->nargs; i++) {
            if (i > 0)
                js_puts(jp, ", ");

            param = GetArgOrVarAtom(jp, i);

#if JS_HAS_DESTRUCTURING
#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_RV(expr, JS_FALSE)

            if (!param) {
                ptrdiff_t todo;
                const char *lval;

                LOCAL_ASSERT(*pc == JSOP_GETARG);
                pc += JSOP_GETARG_LENGTH;
                LOCAL_ASSERT(*pc == JSOP_DUP);
                if (!ss.printer) {
                    ok = InitSprintStack(jp->sprinter.context, &ss, jp, StackDepth(script));
                    if (!ok)
                        break;
                }
                pc = DecompileDestructuring(&ss, pc, endpc);
                if (!pc) {
                    ok = JS_FALSE;
                    break;
                }
                LOCAL_ASSERT(*pc == JSOP_POP);
                pc += JSOP_POP_LENGTH;
                lval = PopStr(&ss, JSOP_NOP);
                todo = SprintCString(&jp->sprinter, lval);
                if (todo < 0) {
                    ok = JS_FALSE;
                    break;
                }
                continue;
            }

#undef LOCAL_ASSERT
#endif

            if (!QuoteString(&jp->sprinter, param, 0)) {
                ok = JS_FALSE;
                break;
            }
        }

#if JS_HAS_DESTRUCTURING
        jp->script = NULL;
        JS_ARENA_RELEASE(&jp->sprinter.context->tempPool, mark);
#endif
        if (!ok)
            return JS_FALSE;
        js_printf(jp, ") ");
        if (!(fun->flags & JSFUN_EXPR_CLOSURE)) {
            js_printf(jp, "{\n");
            jp->indent += 4;
        }

        ok = DecompileBody(jp, script, pc);
        if (!ok)
            return JS_FALSE;

        if (!(fun->flags & JSFUN_EXPR_CLOSURE)) {
            jp->indent -= 4;
            js_printf(jp, "\t}");
        }
    }

    if (!jp->pretty && !jp->grouped && (fun->flags & JSFUN_LAMBDA))
        js_puts(jp, ")");

    return JS_TRUE;
}

char *
js_DecompileValueGenerator(JSContext *cx, intN spindex, jsval v_in,
                           JSString *fallback)
{
    JSStackFrame *fp;
    JSScript *script;
    jsbytecode *pc;

    Value v = Valueify(v_in);

    JS_ASSERT(spindex < 0 ||
              spindex == JSDVG_IGNORE_STACK ||
              spindex == JSDVG_SEARCH_STACK);

    LeaveTrace(cx);
    
    if (!cx->regs || !cx->regs->fp || !cx->regs->fp->isScriptFrame())
        goto do_fallback;

    fp = cx->regs->fp;
    script = fp->script();
    pc = fp->hasImacropc() ? fp->imacropc() : cx->regs->pc;
    JS_ASSERT(script->code <= pc && pc < script->code + script->length);

    if (pc < script->main)
        goto do_fallback;
    
    if (spindex != JSDVG_IGNORE_STACK) {
        jsbytecode **pcstack;

        



        pcstack = (jsbytecode **)
                  cx->malloc(StackDepth(script) * sizeof *pcstack);
        if (!pcstack)
            return NULL;
        intN pcdepth = ReconstructPCStack(cx, script, pc, pcstack);
        if (pcdepth < 0)
            goto release_pcstack;

        if (spindex != JSDVG_SEARCH_STACK) {
            JS_ASSERT(spindex < 0);
            pcdepth += spindex;
            if (pcdepth < 0)
                goto release_pcstack;
            pc = pcstack[pcdepth];
        } else {
            




            Value *stackBase = fp->base();
            Value *sp = cx->regs->sp;
            do {
                if (sp == stackBase) {
                    pcdepth = -1;
                    goto release_pcstack;
                }
            } while (*--sp != v);

            








            if (sp < stackBase + pcdepth)
                pc = pcstack[sp - stackBase];
        }

      release_pcstack:
        cx->free(pcstack);
        if (pcdepth < 0)
            goto do_fallback;
    }

    {
        jsbytecode* basepc = cx->regs->pc;
        jsbytecode* savedImacropc = fp->maybeImacropc();
        if (savedImacropc) {
            JS_ASSERT(cx->hasfp());
            cx->regs->pc = savedImacropc;
            fp->clearImacropc();
        }

        



        char *name;
        if (savedImacropc && size_t(pc - script->code) >= script->length)
            name = FAILED_EXPRESSION_DECOMPILER;
        else
            name = DecompileExpression(cx, script, fp->maybeFun(), pc);

        if (savedImacropc) {
            JS_ASSERT(cx->hasfp());
            cx->regs->pc = basepc;
            fp->setImacropc(savedImacropc);
        }

        if (name != FAILED_EXPRESSION_DECOMPILER)
            return name;
    }

  do_fallback:
    if (!fallback) {
        fallback = js_ValueToSource(cx, v);
        if (!fallback)
            return NULL;
    }
    size_t length = fallback->length();
    const jschar *chars = fallback->getChars(cx);
    if (!chars)
        return NULL;
    return js_DeflateString(cx, chars, length);
}

static char *
DecompileExpression(JSContext *cx, JSScript *script, JSFunction *fun,
                    jsbytecode *pc)
{
    JSOp op;
    const JSCodeSpec *cs;
    jsbytecode *begin, *end;
    jssrcnote *sn;
    ptrdiff_t len;
    jsbytecode **pcstack;
    intN pcdepth;
    JSPrinter *jp;
    char *name;

    JS_ASSERT(script->code <= pc && pc < script->code + script->length);

    pcstack = NULL;
    AutoScriptUntrapper untrapper(cx, script, &pc);
    op = (JSOp) *pc;

    
    JS_ASSERT(op != JSOP_CASE && op != JSOP_CASEX &&
              op != JSOP_DUP && op != JSOP_DUP2);

    
    if (op == JSOP_PUSH) {
        name = JS_strdup(cx, js_undefined_str);
        goto out;
    }

    



    if (op == JSOP_THIS) {
        name = JS_strdup(cx, js_this_str);
        goto out;
    }

    





    if (op == JSOP_BINDNAME) {
        name = FAILED_EXPRESSION_DECOMPILER;
        goto out;
    }

    
    cs = &js_CodeSpec[op];
    begin = pc;
    end = pc + cs->length;
    switch (JOF_MODE(cs->format)) {
      case JOF_PROP:
      case JOF_ELEM:
      case JOF_XMLNAME:
      case 0:
        sn = js_GetSrcNote(script, pc);
        if (!sn) {
            name = FAILED_EXPRESSION_DECOMPILER;
            goto out;
        }
        switch (SN_TYPE(sn)) {
          case SRC_PCBASE:
            begin -= js_GetSrcNoteOffset(sn, 0);
            break;
          case SRC_PCDELTA:
            end = begin + js_GetSrcNoteOffset(sn, 0);
            begin += cs->length;
            break;
          default:
            name = FAILED_EXPRESSION_DECOMPILER;
            goto out;
        }
        break;
      default:;
    }
    len = end - begin;
    if (len <= 0) {
        name = FAILED_EXPRESSION_DECOMPILER;
        goto out;
    }

    pcstack = (jsbytecode **)
              cx->malloc(StackDepth(script) * sizeof *pcstack);
    if (!pcstack) {
        name = NULL;
        goto out;
    }

    MUST_FLOW_THROUGH("out");
    pcdepth = ReconstructPCStack(cx, script, begin, pcstack);
    if (pcdepth < 0) {
         name = FAILED_EXPRESSION_DECOMPILER;
         goto out;
    }

    name = NULL;
    jp = js_NewPrinter(cx, "js_DecompileValueGenerator", fun, 0,
                       false, false, false);
    if (jp) {
        jp->dvgfence = end;
        jp->pcstack = pcstack;
        if (DecompileCode(jp, script, begin, (uintN) len, (uintN) pcdepth)) {
            name = (jp->sprinter.base) ? jp->sprinter.base : (char *) "";
            name = JS_strdup(cx, name);
        }
        js_DestroyPrinter(jp);
    }

out:
    cx->free(pcstack);
    return name;
}

uintN
js_ReconstructStackDepth(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    return ReconstructPCStack(cx, script, pc, NULL);
}

#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_RV(expr, -1);

static intN
SimulateOp(JSContext *cx, JSScript *script, JSOp op, const JSCodeSpec *cs,
           jsbytecode *pc, jsbytecode **pcstack, uintN &pcdepth)
{
    uintN nuses = js_GetStackUses(cs, op, pc);
    uintN ndefs = js_GetStackDefs(cx, cs, op, script, pc);
    LOCAL_ASSERT(pcdepth >= nuses);
    pcdepth -= nuses;
    LOCAL_ASSERT(pcdepth + ndefs <= StackDepth(script));

    




    switch (op) {
      default:
        if (pcstack) {
            for (uintN i = 0; i != ndefs; ++i)
                pcstack[pcdepth + i] = pc;
        }
        break;

      case JSOP_CASE:
      case JSOP_CASEX:
        
        JS_ASSERT(ndefs == 1);
        break;

      case JSOP_DUP:
        JS_ASSERT(ndefs == 2);
        if (pcstack)
            pcstack[pcdepth + 1] = pcstack[pcdepth];
        break;

      case JSOP_DUP2:
        JS_ASSERT(ndefs == 4);
        if (pcstack) {
            pcstack[pcdepth + 2] = pcstack[pcdepth];
            pcstack[pcdepth + 3] = pcstack[pcdepth + 1];
        }
        break;

      case JSOP_SWAP:
        JS_ASSERT(ndefs == 2);
        if (pcstack) {
            jsbytecode *tmp = pcstack[pcdepth + 1];
            pcstack[pcdepth + 1] = pcstack[pcdepth];
            pcstack[pcdepth] = tmp;
        }
        break;
    }
    pcdepth += ndefs;
    return pcdepth;
}

#ifdef JS_TRACER

#undef LOCAL_ASSERT
#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_CUSTOM(expr, goto failure);

static intN
SimulateImacroCFG(JSContext *cx, JSScript *script,
                  uintN pcdepth, jsbytecode *pc, jsbytecode *target,
                  jsbytecode **pcstack)
{
    size_t nbytes = 0;
    jsbytecode** tmp_pcstack = NULL;
    if (pcstack) {
        nbytes = StackDepth(script) * sizeof *pcstack;
        tmp_pcstack = (jsbytecode **) cx->malloc(nbytes);
        if (!tmp_pcstack)
            return -1;
        memcpy(tmp_pcstack, pcstack, nbytes);
    }

    ptrdiff_t oplen;
    for (; pc < target; pc += oplen) {
        JSOp op = js_GetOpcode(cx, script, pc);
        const JSCodeSpec *cs = &js_CodeSpec[op];
        oplen = cs->length;
        if (oplen < 0)
            oplen = js_GetVariableBytecodeLength(pc);

        if (SimulateOp(cx, script, op, cs, pc, tmp_pcstack, pcdepth) < 0)
            goto failure;

        uint32 type = cs->format & JOF_TYPEMASK;
        if (type == JOF_JUMP || type == JOF_JUMPX) {
            ptrdiff_t jmpoff = (type == JOF_JUMP) ? GET_JUMP_OFFSET(pc)
                                                  : GET_JUMPX_OFFSET(pc);
            LOCAL_ASSERT(jmpoff >= 0);
            intN tmp_pcdepth = SimulateImacroCFG(cx, script, pcdepth, pc + jmpoff,
                                                 target, tmp_pcstack);
            if (tmp_pcdepth >= 0) {
                pcdepth = uintN(tmp_pcdepth);
                goto success;
            }

            if (op == JSOP_GOTO || op == JSOP_GOTOX)
                goto failure;
        }
    }

    if (pc > target)
        goto failure;

    LOCAL_ASSERT(pc == target);

  success:
    if (tmp_pcstack) {
        memcpy(pcstack, tmp_pcstack, nbytes);
        cx->free(tmp_pcstack);
    }
    return pcdepth;

  failure:
    if (tmp_pcstack)
        cx->free(tmp_pcstack);
    return -1;
}

#undef LOCAL_ASSERT
#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_RV(expr, -1);

static intN
ReconstructImacroPCStack(JSContext *cx, JSScript *script,
                         jsbytecode *imacstart, jsbytecode *target,
                         jsbytecode **pcstack)
{
    



    JSStackFrame *fp = js_GetScriptedCaller(cx, NULL);
    JS_ASSERT(fp->hasImacropc());
    intN pcdepth = ReconstructPCStack(cx, script, fp->imacropc(), pcstack);
    if (pcdepth < 0)
        return pcdepth;
    return SimulateImacroCFG(cx, script, pcdepth, imacstart, target, pcstack);
}

extern jsbytecode* js_GetImacroStart(jsbytecode* pc);
#endif

static intN
ReconstructPCStack(JSContext *cx, JSScript *script, jsbytecode *target,
                   jsbytecode **pcstack)
{
    






#ifdef JS_TRACER
    jsbytecode *imacstart = js_GetImacroStart(target);

    if (imacstart)
        return ReconstructImacroPCStack(cx, script, imacstart, target, pcstack);
#endif

    LOCAL_ASSERT(script->code <= target && target < script->code + script->length);
    jsbytecode *pc = script->code;
    uintN pcdepth = 0;
    ptrdiff_t oplen;
    for (; pc < target; pc += oplen) {
        JSOp op = js_GetOpcode(cx, script, pc);
        const JSCodeSpec *cs = &js_CodeSpec[op];
        oplen = cs->length;
        if (oplen < 0)
            oplen = js_GetVariableBytecodeLength(pc);

        






        jssrcnote *sn = js_GetSrcNote(script, pc);
        if (sn && SN_TYPE(sn) == SRC_COND) {
            ptrdiff_t jmpoff = js_GetSrcNoteOffset(sn, 0);
            if (pc + jmpoff < target) {
                pc += jmpoff;
                op = js_GetOpcode(cx, script, pc);
                JS_ASSERT(op == JSOP_GOTO || op == JSOP_GOTOX);
                cs = &js_CodeSpec[op];
                oplen = cs->length;
                JS_ASSERT(oplen > 0);
                ptrdiff_t jmplen = GetJumpOffset(pc, pc);
                if (pc + jmplen < target) {
                    oplen = (uintN) jmplen;
                    continue;
                }

                



                LOCAL_ASSERT(pcdepth != 0);
                --pcdepth;
            }
        }

        
        if (sn && SN_TYPE(sn) == SRC_HIDDEN)
            continue;

        if (SimulateOp(cx, script, op, cs, pc, pcstack, pcdepth) < 0)
            return -1;

    }
    LOCAL_ASSERT(pc == target);
    return pcdepth;
}

#undef LOCAL_ASSERT
#undef LOCAL_ASSERT_RV

namespace js {

bool
CallResultEscapes(jsbytecode *pc)
{
    











    if (*pc != JSOP_CALL)
        return true;

    pc += JSOP_CALL_LENGTH;

    if (*pc == JSOP_TRACE)
        pc += JSOP_TRACE_LENGTH;

    if (*pc == JSOP_POP)
        return false;

    if (*pc == JSOP_NOT)
        pc += JSOP_NOT_LENGTH;

    return (*pc != JSOP_IFEQ);
}

} 
