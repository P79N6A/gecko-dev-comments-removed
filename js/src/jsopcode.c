










































#include "jsstddef.h"
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsarena.h" 
#include "jsutil.h" 
#include "jsdtoa.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsdbgapi.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jslock.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"

#if JS_HAS_DESTRUCTURING
# include "jsnum.h"
#endif

static const char js_incop_strs[][3] = {"++", "--"};

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

#ifdef DEBUG



static const char *CodeName[] = {
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

    type = (js_CodeSpec[*pc].format & JOF_TYPEMASK);
    if (JOF_TYPE_IS_EXTENDED_JUMP(type))
        return GET_JUMPX_OFFSET(pc2);
    return GET_JUMP_OFFSET(pc2);
}

uintN
js_GetIndexFromBytecode(JSScript *script, jsbytecode *pc, ptrdiff_t pcoff)
{
    JSOp op;
    uintN span, base;

    op = (JSOp)*pc;
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

#ifdef DEBUG

JS_FRIEND_API(JSBool)
js_Disassemble(JSContext *cx, JSScript *script, JSBool lines, FILE *fp)
{
    jsbytecode *pc, *end;
    uintN len;

    pc = script->code;
    end = pc + script->length;
    while (pc < end) {
        if (pc == script->main)
            fputs("main:\n", fp);
        len = js_Disassemble1(cx, script, pc,
                              PTRDIFF(pc, script->code, jsbytecode),
                              lines, fp);
        if (!len)
            return JS_FALSE;
        pc += len;
    }
    return JS_TRUE;
}

const char *
ToDisassemblySource(JSContext *cx, jsval v)
{
    JSObject *obj;
    JSScopeProperty *sprop;
    char *source;
    const char *bytes;
    JSString *str;

    if (!JSVAL_IS_PRIMITIVE(v)) {
        obj = JSVAL_TO_OBJECT(v);
        if (OBJ_GET_CLASS(cx, obj) == &js_BlockClass) {
            source = JS_sprintf_append(NULL, "depth %d {",
                                       OBJ_BLOCK_DEPTH(cx, obj));
            for (sprop = OBJ_SCOPE(obj)->lastProp; sprop;
                 sprop = sprop->parent) {
                bytes = js_AtomToPrintableString(cx, JSID_TO_ATOM(sprop->id));
                if (!bytes)
                    return NULL;
                source = JS_sprintf_append(source, "%s: %d%s",
                                           bytes, sprop->shortid,
                                           sprop->parent ? ", " : "");
            }
            source = JS_sprintf_append(source, "}");
            if (!source)
                return NULL;
            str = JS_NewString(cx, source, strlen(source));
            if (!str)
                return NULL;
            return js_GetStringBytes(cx, str);
        }
    }
    return js_ValueToPrintableSource(cx, v);
}

JS_FRIEND_API(uintN)
js_Disassemble1(JSContext *cx, JSScript *script, jsbytecode *pc,
                uintN loc, JSBool lines, FILE *fp)
{
    JSOp op;
    const JSCodeSpec *cs;
    ptrdiff_t len, off, jmplen;
    uint32 type;
    JSAtom *atom;
    uintN index;
    JSObject *obj;
    jsval v;
    const char *bytes;
    jsint i;

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
    fprintf(fp, "%05u:", loc);
    if (lines)
        fprintf(fp, "%4u", JS_PCToLineNumber(cx, script, pc));
    fprintf(fp, "  %s", CodeName[op]);
    type = cs->format & JOF_TYPEMASK;
    switch (type) {
      case JOF_BYTE:
        if (op == JSOP_TRAP) {
            op = JS_GetTrapOpcode(cx, script, pc);
            if (op == JSOP_LIMIT)
                return 0;
            len = (ptrdiff_t) js_CodeSpec[op].length;
        }
        break;

      case JOF_JUMP:
      case JOF_JUMPX:
        off = GetJumpOffset(pc, pc);
        fprintf(fp, " %u (%d)", loc + off, off);
        break;

      case JOF_ATOM:
      case JOF_OBJECT:
      case JOF_REGEXP:
        index = js_GetIndexFromBytecode(script, pc, 0);
        if (type == JOF_ATOM) {
            JS_GET_SCRIPT_ATOM(script, index, atom);
            v = ATOM_KEY(atom);
        } else {
            if (type == JOF_OBJECT)
                JS_GET_SCRIPT_OBJECT(script, index, obj);
            else
                JS_GET_SCRIPT_REGEXP(script, index, obj);
            v = OBJECT_TO_JSVAL(obj);
        }
        bytes = ToDisassemblySource(cx, v);
        if (!bytes)
            return 0;
        fprintf(fp, " %s", bytes);
        break;

      case JOF_UINT16:
      case JOF_LOCAL:
        i = (jsint)GET_UINT16(pc);
        goto print_int;

      case JOF_2BYTE:
        fprintf(fp, " %u", (uintN)pc[1]);
        break;

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
        fprintf(fp, " defaultOffset %d low %d high %d", off, low, high);
        for (i = low; i <= high; i++) {
            off = GetJumpOffset(pc, pc2);
            fprintf(fp, "\n\t%d: %d", i, off);
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
        fprintf(fp, " offset %d npairs %u", off, (uintN) npairs);
        while (npairs) {
            JS_GET_SCRIPT_ATOM(script, GET_INDEX(pc2), atom);
            pc2 += INDEX_LEN;
            off = GetJumpOffset(pc, pc2);
            pc2 += jmplen;

            bytes = ToDisassemblySource(cx, ATOM_KEY(atom));
            if (!bytes)
                return 0;
            fprintf(fp, "\n\t%s: %d", bytes, off);
            npairs--;
        }
        len = 1 + pc2 - pc;
        break;
      }

      case JOF_QARG:
        fprintf(fp, " %u", GET_ARGNO(pc));
        break;

      case JOF_QVAR:
        fprintf(fp, " %u", GET_VARNO(pc));
        break;

      case JOF_SLOTATOM:
      case JOF_SLOTOBJECT:
        fprintf(fp, " %u", GET_VARNO(pc));
        index = js_GetIndexFromBytecode(script, pc, VARNO_LEN);
        if (type == JOF_ATOM) {
            JS_GET_SCRIPT_ATOM(script, index, atom);
            v = ATOM_KEY(atom);
        } else {
            JS_GET_SCRIPT_OBJECT(script, index, obj);
            v = OBJECT_TO_JSVAL(obj);
        }
        bytes = ToDisassemblySource(cx, v);
        if (!bytes)
            return 0;
        fprintf(fp, " %s", bytes);
        break;

      case JOF_UINT24:
        JS_ASSERT(op == JSOP_UINT24);
        i = (jsint)GET_UINT24(pc);
        goto print_int;

      case JOF_INT8:
        JS_ASSERT(op == JSOP_INT8);
        i = GET_INT8(pc);
        goto print_int;

      case JOF_INT32:
        JS_ASSERT(op == JSOP_INT32);
        i = GET_INT32(pc);
      print_int:
        fprintf(fp, " %d", i);
        break;

      default: {
        char numBuf[12];
        JS_snprintf(numBuf, sizeof numBuf, "%lx", (unsigned long) cs->format);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_UNKNOWN_FORMAT, numBuf);
        return 0;
      }
    }
    fputs("\n", fp);
    return len;
}

#endif 






typedef struct Sprinter {
    JSContext       *context;       
    JSArenaPool     *pool;          
    char            *base;          
    size_t          size;           
    ptrdiff_t       offset;         
} Sprinter;

#define INIT_SPRINTER(cx, sp, ap, off) \
    ((sp)->context = cx, (sp)->pool = ap, (sp)->base = NULL, (sp)->size = 0,  \
     (sp)->offset = off)

#define OFF2STR(sp,off) ((sp)->base + (off))
#define STR2OFF(sp,str) ((str) - (sp)->base)
#define RETRACT(sp,str) ((sp)->offset = STR2OFF(sp, str))

static JSBool
SprintEnsureBuffer(Sprinter *sp, size_t len)
{
    ptrdiff_t nb;

    nb = (sp->offset + len + 1) - sp->size;
    if (nb < 0)
        return JS_TRUE;
    if (!sp->base) {
        JS_ARENA_ALLOCATE_CAST(sp->base, char *, sp->pool, nb);
    } else {
        JS_ARENA_GROW_CAST(sp->base, char *, sp->pool, sp->size, nb);
    }
    if (!sp->base) {
        JS_ReportOutOfMemory(sp->context);
        return JS_FALSE;
    }
    sp->size += nb;
    return JS_TRUE;
}

static ptrdiff_t
SprintPut(Sprinter *sp, const char *s, size_t len)
{
    ptrdiff_t offset;
    char *bp;

    
    if (!SprintEnsureBuffer(sp, len))
        return -1;

    
    offset = sp->offset;
    sp->offset += len;
    bp = sp->base + offset;
    memmove(bp, s, len);
    bp[len] = 0;
    return offset;
}

static ptrdiff_t
SprintCString(Sprinter *sp, const char *s)
{
    return SprintPut(sp, s, strlen(s));
}

static ptrdiff_t
SprintString(Sprinter *sp, JSString *str)
{
    jschar *chars;
    size_t length, size;
    ptrdiff_t offset;

    chars = JSSTRING_CHARS(str);
    length = JSSTRING_LENGTH(str);
    if (length == 0)
        return sp->offset;

    size = js_GetDeflatedStringLength(sp->context, chars, length);
    if (size == (size_t)-1 || !SprintEnsureBuffer(sp, size))
        return -1;

    offset = sp->offset;
    sp->offset += size;
    js_DeflateStringToBuffer(sp->context, chars, length, sp->base + offset,
                             &size);
    sp->base[sp->offset] = 0;
    return offset;
}


static ptrdiff_t
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
    free(bp);
    return offset;
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
    0
};

#define DONT_ESCAPE     0x10000

static char *
QuoteString(Sprinter *sp, JSString *str, uint32 quote)
{
    JSBool dontEscape, ok;
    jschar qc, c;
    ptrdiff_t off, len;
    const jschar *s, *t, *z;
    const char *e;
    char *bp;

    
    dontEscape = (quote & DONT_ESCAPE) != 0;
    qc = (jschar) quote;
    off = sp->offset;
    if (qc && Sprint(sp, "%c", (char)qc) < 0)
        return NULL;

    
    s = JSSTRING_CHARS(str);
    z = s + JSSTRING_LENGTH(str);
    for (t = s; t < z; s = ++t) {
        
        c = *t;
        while (JS_ISPRINT(c) && c != qc && c != '\\' && c != '\t' &&
               !(c >> 8)) {
            c = *++t;
            if (t == z)
                break;
        }
        len = PTRDIFF(t, s, jschar);

        
        if (!SprintEnsureBuffer(sp, len))
            return NULL;

        
        bp = sp->base + sp->offset;
        sp->offset += len;
        while (--len >= 0)
            *bp++ = (char) *s++;
        *bp = '\0';

        if (t == z)
            break;

        
        if (!(c >> 8) && (e = strchr(js_EscapeMap, (int)c)) != NULL) {
            ok = dontEscape
                 ? Sprint(sp, "%c", (char)c) >= 0
                 : Sprint(sp, "\\%c", e[1]) >= 0;
        } else {
#ifdef JS_C_STRINGS_ARE_UTF8
            
            if (c >= 0xD800 && c <= 0xDBFF) {
                jschar buffer[3];
                buffer[0] = c;
                buffer[1] = *++t;
                buffer[2] = 0;
                if (t == z) {
                    char numbuf[10];
                    JS_snprintf(numbuf, sizeof numbuf, "0x%x", c);
                    JS_ReportErrorFlagsAndNumber(sp->context, JSREPORT_ERROR,
                                                 js_GetErrorMessage, NULL,
                                                 JSMSG_BAD_SURROGATE_CHAR,
                                                 numbuf);
                    ok = JS_FALSE;
                    break;
                }
                ok = Sprint(sp, "%hs", buffer) >= 0;
            } else {
                
                ok = Sprint(sp, "%hc", c) >= 0;
            }
#else
            
            ok = Sprint(sp, (c >> 8) ? "\\u%04X" : "\\x%02X", c) >= 0;
#endif
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



#if JS_HAS_BLOCK_SCOPE
typedef enum JSBraceState {
    ALWAYS_BRACE,
    MAYBE_BRACE,
    DONT_BRACE
} JSBraceState;
#endif

struct JSPrinter {
    Sprinter        sprinter;       
    JSArenaPool     pool;           
    uintN           indent;         
    JSPackedBool    pretty;         
    JSPackedBool    grouped;        
    JSScript        *script;        
    jsbytecode      *dvgfence;      
    JSObject        *object;        
#if JS_HAS_BLOCK_SCOPE
    JSBraceState    braceState;     
    ptrdiff_t       spaceOffset;    
#endif
};







#define JS_IN_GROUP_CONTEXT 0x10000

JSPrinter *
JS_NEW_PRINTER(JSContext *cx, const char *name, uintN indent, JSBool pretty)
{
    JSPrinter *jp;

    jp = (JSPrinter *) JS_malloc(cx, sizeof(JSPrinter));
    if (!jp)
        return NULL;
    INIT_SPRINTER(cx, &jp->sprinter, &jp->pool, 0);
    JS_INIT_ARENA_POOL(&jp->pool, name, 256, 1);
    jp->indent = indent & ~JS_IN_GROUP_CONTEXT;
    jp->pretty = pretty;
    jp->grouped = (indent & JS_IN_GROUP_CONTEXT) != 0;
    jp->script = NULL;
    jp->dvgfence = NULL;
    jp->object = NULL;
#if JS_HAS_BLOCK_SCOPE
    jp->braceState = ALWAYS_BRACE;
    jp->spaceOffset = -1;
#endif
    return jp;
}

void
js_DestroyPrinter(JSPrinter *jp)
{
    JS_FinishArenaPool(&jp->pool);
    JS_free(jp->sprinter.context, jp);
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

#if !JS_HAS_BLOCK_SCOPE
# define SET_MAYBE_BRACE(jp)    jp
# define CLEAR_MAYBE_BRACE(jp)  jp
# define MAYBE_SET_DONT_BRACE(jp,pc,endpc,rval)
#else
# define SET_MAYBE_BRACE(jp)    ((jp)->braceState = MAYBE_BRACE, (jp))
# define CLEAR_MAYBE_BRACE(jp)  ((jp)->braceState = ALWAYS_BRACE, (jp))
# define MAYBE_SET_DONT_BRACE   MaybeSetDontBrace

static void
SetDontBrace(JSPrinter *jp)
{
    ptrdiff_t offset;
    const char *bp;

    
    JS_ASSERT(jp->spaceOffset < 0);
    offset = jp->sprinter.offset - (jp->pretty ? 3 : 2);

    
    JS_ASSERT(offset >= 6);
    bp = jp->sprinter.base;
    if (bp[offset+0] == ' ' && bp[offset+1] == '{') {
        JS_ASSERT(!jp->pretty || bp[offset+2] == '\n');
        jp->spaceOffset = offset;
        jp->braceState = DONT_BRACE;
    }
}






static void
MaybeSetDontBrace(JSPrinter *jp, jsbytecode *pc, jsbytecode *endpc,
                  const char *rval)
{
    JS_ASSERT(*pc == JSOP_POP || *pc == JSOP_POPV || *pc == JSOP_POPN);

    if (jp->braceState == MAYBE_BRACE &&
        pc + js_CodeSpec[*pc].length == endpc &&
        !strncmp(rval, var_prefix[SRC_DECL_LET], 4) &&
        rval[4] != '(') {
        SetDontBrace(jp);
    }
}
#endif

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

#if JS_HAS_BLOCK_SCOPE
        if (*format == '}' && jp->braceState != ALWAYS_BRACE) {
            JSBraceState braceState;

            braceState = jp->braceState;
            jp->braceState = ALWAYS_BRACE;
            if (braceState == DONT_BRACE) {
                ptrdiff_t offset, delta, from;

                JS_ASSERT(format[1] == '\n' || format[1] == ' ');
                offset = jp->spaceOffset;
                JS_ASSERT(offset >= 6);

                
                bp = jp->sprinter.base;
                if (bp[offset+0] == ' ' && bp[offset+1] == '{') {
                    delta = 2;
                    if (jp->pretty) {
                        
                        JS_ASSERT(bp[offset+2] == '\n');
                    } else if (bp[offset-1] != ')') {
                        
                        ++offset;
                        delta = 1;
                    }

                    from = offset + delta;
                    memmove(bp + offset, bp + from, jp->sprinter.offset - from);
                    jp->sprinter.offset -= delta;
                    jp->spaceOffset = -1;

                    format += 2;
                    if (*format == '\0')
                        return 0;
                }
            }
        }
#endif

        if (jp->pretty && Sprint(&jp->sprinter, "%*s", jp->indent, "") < 0)
            return -1;
    }

    
    fp = NULL;
    if (!jp->pretty && format[cc = strlen(format) - 1] == '\n') {
        fp = JS_strdup(jp->sprinter.context, format);
        if (!fp)
            return -1;
        fp[cc] = '\0';
        format = fp;
    }

    
    bp = JS_vsmprintf(format, ap);      
    if (fp) {
        JS_free(jp->sprinter.context, fp);
        format = NULL;
    }
    if (!bp) {
        JS_ReportOutOfMemory(jp->sprinter.context);
        return -1;
    }

    cc = strlen(bp);
    if (SprintPut(&jp->sprinter, bp, (size_t)cc) < 0)
        cc = -1;
    free(bp);

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












static ptrdiff_t
GetOff(SprintStack *ss, uintN i)
{
    ptrdiff_t off;
    char *bytes;

    off = ss->offsets[i];
    if (off < 0) {
#if defined DEBUG_brendan || defined DEBUG_mrbkap || defined DEBUG_crowder
        JS_ASSERT(off < -1);
#endif
        if (++off == 0) {
            if (!ss->sprinter.base && SprintPut(&ss->sprinter, "", 0) >= 0)
                memset(ss->sprinter.base, 0, ss->sprinter.offset);
            return 0;
        }

        bytes = js_DecompileValueGenerator(ss->sprinter.context, off,
                                           JSVAL_NULL, NULL);
        if (!bytes)
            return 0;
        off = SprintCString(&ss->sprinter, bytes);
        if (off < 0)
            off = 0;
        ss->offsets[i] = off;
        JS_free(ss->sprinter.context, bytes);
    }
    return off;
}

static const char *
GetStr(SprintStack *ss, uintN i)
{
    ptrdiff_t off;

    



    off = GetOff(ss, i);
    return OFF2STR(&ss->sprinter, off);
}


#define PAREN_SLOP      (2 + 1)






#define JSOP_GETPROP2   256
#define JSOP_GETELEM2   257

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
    JS_ASSERT(top < ss->printer->script->depth);
    if (top >= ss->printer->script->depth) {
        JS_ReportOutOfMemory(ss->sprinter.context);
        return JS_FALSE;
    }

    
    ss->offsets[top] = off;
    ss->opcodes[top] = (op == JSOP_GETPROP2) ? JSOP_GETPROP
                     : (op == JSOP_GETELEM2) ? JSOP_GETELEM
                     : (jsbytecode) op;
    ss->top = ++top;
    AddParenSlop(ss);
    return JS_TRUE;
}

static ptrdiff_t
PopOff(SprintStack *ss, JSOp op)
{
    uintN top;
    const JSCodeSpec *cs, *topcs;
    ptrdiff_t off;

    
    top = ss->top;
    JS_ASSERT(top != 0);
    if (top == 0)
        return 0;

    ss->top = --top;
    off = GetOff(ss, top);
    topcs = &js_CodeSpec[ss->opcodes[top]];
    cs = &js_CodeSpec[op];
    if (topcs->prec != 0 && topcs->prec < cs->prec) {
        ss->sprinter.offset = ss->offsets[top] = off - 2;
        off = Sprint(&ss->sprinter, "(%s)", OFF2STR(&ss->sprinter, off));
    } else {
        ss->sprinter.offset = off;
    }
    return off;
}

static const char *
PopStr(SprintStack *ss, JSOp op)
{
    ptrdiff_t off;

    off = PopOff(ss, op);
    return OFF2STR(&ss->sprinter, off);
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
    char *s, buf[DTOSTR_STANDARD_BUFFER_SIZE];

    JS_ASSERT(JSVAL_IS_DOUBLE(v));
    d = *JSVAL_TO_DOUBLE(v);
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
        s = JS_dtostr(buf, sizeof buf, DTOSTR_STANDARD, 0, d);
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

    js_printf(CLEAR_MAYBE_BRACE(jp), "\tswitch (%s) {\n", lval);

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
                    str = ATOM_TO_STRING(table[i].label);
                    key = JSVAL_VOID;
                } else if (JSVAL_IS_DOUBLE(key)) {
                    JSOp junk;

                    todo = SprintDoubleValue(&ss->sprinter, key, &junk);
                    str = NULL;
                } else {
                    str = js_ValueToString(cx, key);
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

static JSAtom *
GetSlotAtom(JSPrinter *jp, JSPropertyOp getter, uintN slot)
{
    JSObject *obj;
    JSScopeProperty *sprop;

    obj = jp->object;
    while (obj) {
        for (sprop = SCOPE_LAST_PROP(OBJ_SCOPE(obj)); sprop;
             sprop = sprop->parent) {
            if (sprop->getter != getter)
                continue;
            JS_ASSERT(sprop->flags & SPROP_HAS_SHORTID);
            JS_ASSERT(JSID_IS_ATOM(sprop->id));
            if ((uintN) sprop->shortid == slot)
                return JSID_TO_ATOM(sprop->id);
        }
        obj = OBJ_GET_PROTO(jp->sprinter.context, obj);
    }
    return NULL;
}

static JSBool
PushSlotAtom(SprintStack *ss, JSPropertyOp getter, uintN slot, JSOp op)
{
    JSAtom *atom;
    char *lval;

    atom = GetSlotAtom(ss->printer, getter, slot);
    if (!atom)
        return JS_FALSE;
    JS_ASSERT(ATOM_IS_STRING(atom));
    lval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
    if (!lval)
        return JS_FALSE;
    return PushOff(ss, STR2OFF(&ss->sprinter, lval), op);
}

#define LOCAL_ASSERT_RV(expr, rv)                                             \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(expr);                                                      \
        if (!(expr)) return (rv);                                             \
    JS_END_MACRO

const char *
GetLocal(SprintStack *ss, jsint i)
{
    ptrdiff_t off;
    JSContext *cx;
    JSScript *script;
    jsatomid j, n;
    JSAtom *atom;
    JSObject *obj;
    jsint depth, count;
    JSScopeProperty *sprop;
    const char *rval;

#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_RV(expr, "")

    off = ss->offsets[i];
    if (off >= 0)
        return OFF2STR(&ss->sprinter, off);

    




    cx = ss->sprinter.context;
    script = ss->printer->script;
    LOCAL_ASSERT(script->objectsOffset != 0);
    for (j = 0, n = JS_SCRIPT_OBJECTS(script)->length; j < n; j++) {
        JS_GET_SCRIPT_OBJECT(script, j, obj);
        if (OBJ_GET_CLASS(cx, obj) == &js_BlockClass) {
            depth = OBJ_BLOCK_DEPTH(cx, obj);
            count = OBJ_BLOCK_COUNT(cx, obj);
            if ((jsuint)(i - depth) < (jsuint)count)
                break;
        }
    }

    LOCAL_ASSERT(j < n);
    i -= depth;
    for (sprop = OBJ_SCOPE(obj)->lastProp; sprop; sprop = sprop->parent) {
        if (sprop->shortid == i)
            break;
    }

    LOCAL_ASSERT(sprop && JSID_IS_ATOM(sprop->id));
    atom = JSID_TO_ATOM(sprop->id);
    rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
    if (!rval)
        return NULL;
    RETRACT(&ss->sprinter, rval);
    return rval;

#undef LOCAL_ASSERT
}

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
    uintN oplen, i;
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
      case JSOP_SETVAR:
      case JSOP_SETGVAR:
      case JSOP_SETLOCAL:
        LOCAL_ASSERT(pc[oplen] == JSOP_POP || pc[oplen] == JSOP_POPN);
        

      case JSOP_SETLOCALPOP:
        i = GET_UINT16(pc);
        atom = NULL;
        lval = NULL;
        if (op == JSOP_SETARG)
            atom = GetSlotAtom(jp, js_GetArgument, i);
        else if (op == JSOP_SETVAR)
            atom = GetSlotAtom(jp, js_GetLocalVariable, i);
        else if (op == JSOP_SETGVAR)
            GET_ATOM_FROM_BYTECODE(jp->script, pc, 0, atom);
        else
            lval = GetLocal(ss, i);
        if (atom)
            lval = js_AtomToPrintableString(cx, atom);
        LOCAL_ASSERT(lval);
        todo = SprintCString(&ss->sprinter, lval);
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
    ptrdiff_t head, todo;
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
    JSString *str;
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
            GET_ATOM_FROM_BYTECODE(jp->script, pc, 0, atom);
            d = *ATOM_TO_DOUBLE(atom);
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

          case JSOP_CALLPROP:
          case JSOP_GETPROP:
            *OFF2STR(&ss->sprinter, head) = '{';
            GET_ATOM_FROM_BYTECODE(jp->script, pc, 0, atom);
            str = ATOM_TO_STRING(atom);
            if (!QuoteString(&ss->sprinter, str,
                             js_IsIdentifier(str) ? 0 : (jschar)'\'')) {
                return NULL;
            }
            if (SprintPut(&ss->sprinter, ": ", 2) < 0)
                return NULL;
            break;

          default:
            LOCAL_ASSERT(0);
        }

        pc += oplen;
        if (pc == endpc)
            return pc;

        




        pc = DecompileDestructuringLHS(ss, pc, endpc, &hole);
        if (!pc)
            return NULL;
        if (pc == endpc || *pc != JSOP_DUP)
            break;

        




        sn = js_GetSrcNote(jp->script, pc);
        if (sn && SN_TYPE(sn) == SRC_DESTRUCT)
            break;

        if (!hole && SprintPut(&ss->sprinter, ", ", 2) < 0)
            return NULL;

        pc += JSOP_DUP_LENGTH;
    }

out:
    lval = OFF2STR(&ss->sprinter, head);
    todo = SprintPut(&ss->sprinter, (*lval == '[') ? "]" : "}", 1);
    if (todo < 0)
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
    if (!space)
        return JS_FALSE;
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
    jsbytecode *startpc, *endpc, *pc2, *done, *forelem_tail, *forelem_done;
    ptrdiff_t tail, todo, len, oplen, cond, next;
    JSOp op, lastop, saveop;
    const JSCodeSpec *cs;
    jssrcnote *sn, *sn2;
    const char *lval, *rval, *xval, *fmt, *token;
    jsint i, argc;
    char **argv;
    JSAtom *atom;
    JSObject *obj;
    JSFunction *fun;
    JSString *str;
    JSBool ok;
#if JS_HAS_XML_SUPPORT
    JSBool foreach, inXML, quoteAttr;
#else
#define inXML JS_FALSE
#endif
    jsval val;
    int stackDummy;

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




#define DECOMPILE_CODE(pc,nb) if (!Decompile(ss, pc, nb, JSOP_NOP)) return NULL
#define NEXT_OP(pc)           (((pc) + (len) == endpc) ? nextop : pc[len])
#define POP_STR()             PopStr(ss, op)
#define LOCAL_ASSERT(expr)    LOCAL_ASSERT_RV(expr, NULL)





#define ATOM_IS_IDENTIFIER(atom) js_IsIdentifier(ATOM_TO_STRING(atom))
#define ATOM_IS_KEYWORD(atom)                                                 \
    (js_CheckKeyword(JSSTRING_CHARS(ATOM_TO_STRING(atom)),                    \
                     JSSTRING_LENGTH(ATOM_TO_STRING(atom))) != TOK_EOF)






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
        rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), quote_);      \
        if (!rval)                                                            \
            return NULL;                                                      \
    JS_END_MACRO

#define LOAD_ATOM(PCOFF)                                                      \
    GET_ATOM_FROM_BYTECODE(jp->script, pc, PCOFF, atom)

#define LOAD_OBJECT(PCOFF)                                                    \
    GET_OBJECT_FROM_BYTECODE(jp->script, pc, PCOFF, obj)

#define LOAD_FUNCTION(PCOFF)                                                  \
    GET_FUNCTION_FROM_BYTECODE(jp->script, pc, PCOFF, obj)

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

    cx = ss->sprinter.context;
    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return NULL;
    }

    jp = ss->printer;
    startpc = pc;
    endpc = (nb < 0) ? jp->script->code + jp->script->length : pc + nb;
    forelem_tail = forelem_done = NULL;
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

        if (nb < 0 && -(nb + 1) == (intN)ss->top - cs->nuses + cs->ndefs)
            return pc;

        



        token = CodeToken[op];

        if (pc + oplen == jp->dvgfence) {
            JSStackFrame *fp;
            uint32 format, mode, type;

            




            for (fp = cx->fp; fp && !fp->script; fp = fp->down)
                continue;
            format = cs->format;
            if (((fp && pc == fp->pc) ||
                 (pc == startpc && cs->nuses != 0)) &&
                format & (JOF_SET|JOF_DEL|JOF_INCDEC|JOF_IMPORT|JOF_FOR|
                          JOF_VARPROP)) {
                mode = JOF_MODE(format);
                if (mode == JOF_NAME) {
                    





                    type = format & JOF_TYPEMASK;
                    op = (type == JOF_QARG)
                         ? JSOP_GETARG
                         : (type == JOF_QVAR)
                         ? JSOP_GETVAR
                         : (type == JOF_LOCAL)
                         ? JSOP_GETLOCAL
                         : JSOP_NAME;

                    i = cs->nuses - js_CodeSpec[op].nuses;
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
#if JS_HAS_LVALUE_RETURN
                          case JSOP_SETCALL:
                            op = JSOP_CALL;
                            break;
#endif
                          case JSOP_GETARGPROP:
                            op = JSOP_GETARG;
                            break;
                          case JSOP_GETVARPROP:
                            op = JSOP_GETVAR;
                            break;
                          case JSOP_GETLOCALPROP:
                            op = JSOP_GETLOCAL;
                            break;
                          default:
                            LOCAL_ASSERT(0);
                        }
                    }
                }
            }

            saveop = op;
            if (op >= JSOP_LIMIT) {
                switch (op) {
                  case JSOP_GETPROP2:
                    saveop = JSOP_GETPROP;
                    break;
                  case JSOP_GETELEM2:
                    saveop = JSOP_GETELEM;
                    break;
                  default:;
                }
            }
            LOCAL_ASSERT(js_CodeSpec[saveop].length == oplen ||
                         (format & JOF_TYPEMASK) == JOF_SLOTATOM);

            jp->dvgfence = NULL;
        }

        if (token) {
            switch (cs->nuses) {
              case 2:
                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_ASSIGNOP) {
                    




                    op = (JSOp) pc[oplen];
                    LOCAL_ASSERT(op != saveop);
                }
                rval = POP_STR();
                lval = POP_STR();
                if (op != saveop) {
                    
                    todo = SprintCString(&ss->sprinter, rval);
                    op = saveop;
                } else if (!inXML) {
                    todo = Sprint(&ss->sprinter, "%s %s %s",
                                  lval, token, rval);
                } else {
                    
                    LOCAL_ASSERT(op == JSOP_ADD);
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
#define BEGIN_LITOPX_CASE(OP,PCOFF)                                           \
              case OP:                                                        \
                LOAD_ATOM(PCOFF);

#define END_LITOPX_CASE                                                       \
                break;

              case JSOP_NOP:
                




                sn = js_GetSrcNote(jp->script, pc);
                todo = -2;
                switch (sn ? SN_TYPE(sn) : SRC_NULL) {
                  case SRC_WHILE:
                    ++pc;
                    tail = js_GetSrcNoteOffset(sn, 0) - 1;
                    LOCAL_ASSERT(pc[tail] == JSOP_IFNE ||
                                 pc[tail] == JSOP_IFNEX);
                    js_printf(SET_MAYBE_BRACE(jp), "\tdo {\n");
                    jp->indent += 4;
                    DECOMPILE_CODE(pc, tail);
                    jp->indent -= 4;
                    js_printf(jp, "\t} while (%s);\n", POP_STR());
                    pc += tail;
                    len = js_CodeSpec[*pc].length;
                    todo = -2;
                    break;

                  case SRC_FOR:
                    rval = "";

                  do_forloop:
                    
                    pc++;

                    
                    cond = js_GetSrcNoteOffset(sn, 0);
                    next = js_GetSrcNoteOffset(sn, 1);
                    tail = js_GetSrcNoteOffset(sn, 2);
                    LOCAL_ASSERT(tail + GetJumpOffset(pc+tail, pc+tail) == 0);

                    
                    js_printf(jp, "\tfor (%s;", rval);

                    if (pc[cond] == JSOP_IFEQ || pc[cond] == JSOP_IFEQX) {
                        
                        DECOMPILE_CODE(pc, cond);
                        js_printf(jp, " %s", POP_STR());
                    }

                    
                    js_puts(jp, ";");

                    if (pc[next] != JSOP_GOTO && pc[next] != JSOP_GOTOX) {
                        
                        DECOMPILE_CODE(pc + next, tail - next - 1);
                        js_printf(jp, " %s", POP_STR());
                    }

                    
                    js_printf(SET_MAYBE_BRACE(jp), ") {\n");
                    jp->indent += 4;
                    oplen = (cond) ? js_CodeSpec[pc[cond]].length : 0;
                    DECOMPILE_CODE(pc + cond + oplen, next - cond - oplen);
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");

                    
                    len = tail + js_CodeSpec[pc[tail]].length;
                    break;

                  case SRC_LABEL:
                    GET_SOURCE_NOTE_ATOM(sn, atom);
                    jp->indent -= 4;
                    rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
                    if (!rval)
                        return NULL;
                    RETRACT(&ss->sprinter, rval);
                    js_printf(CLEAR_MAYBE_BRACE(jp), "\t%s:\n", rval);
                    jp->indent += 4;
                    break;

                  case SRC_LABELBRACE:
                    GET_SOURCE_NOTE_ATOM(sn, atom);
                    rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
                    if (!rval)
                        return NULL;
                    RETRACT(&ss->sprinter, rval);
                    js_printf(CLEAR_MAYBE_BRACE(jp), "\t%s: {\n", rval);
                    jp->indent += 4;
                    break;

                  case SRC_ENDBRACE:
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");
                    break;

                  case SRC_FUNCDEF:
                    JS_GET_SCRIPT_OBJECT(jp->script, js_GetSrcNoteOffset(sn, 0),
                                         obj);
                  do_function:
                    fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj);
                    jp2 = JS_NEW_PRINTER(cx, "nested_function",
                                         jp->indent, jp->pretty);
                    if (!jp2)
                        return NULL;
                    jp2->object = jp->object;
                    js_puts(jp2, "\n");
                    ok = js_DecompileFunction(jp2, fun);
                    if (ok && jp2->sprinter.base) {
                        js_puts(jp, jp2->sprinter.base);
                        js_puts(jp, "\n");
                    }
                    js_DestroyPrinter(jp2);
                    if (!ok)
                        return NULL;
                    js_puts(jp, "\n");
                    break;

                  case SRC_BRACE:
                    js_printf(CLEAR_MAYBE_BRACE(jp), "\t{\n");
                    jp->indent += 4;
                    len = js_GetSrcNoteOffset(sn, 0);
                    DECOMPILE_CODE(pc + oplen, len - oplen);
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");
                    break;

                  default:;
                }
                break;

              case JSOP_GROUP:
                cs = &js_CodeSpec[lastop];
                if ((cs->prec != 0 &&
                     cs->prec <= js_CodeSpec[NEXT_OP(pc)].prec) ||
                    pc[JSOP_GROUP_LENGTH] == JSOP_NULL ||
                    pc[JSOP_GROUP_LENGTH] == JSOP_GLOBALTHIS ||
                    pc[JSOP_GROUP_LENGTH] == JSOP_DUP ||
                    pc[JSOP_GROUP_LENGTH] == JSOP_IFEQ ||
                    pc[JSOP_GROUP_LENGTH] == JSOP_IFNE) {
                    













                    op = JSOP_NAME;
                    rval = POP_STR();
                    todo = SprintCString(&ss->sprinter, rval);
                } else {
                    




                    LOCAL_ASSERT(ss->top != 0);
                    ss->opcodes[ss->top-1] = saveop = lastop;
                    todo = -2;
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
                todo = Sprint(&ss->sprinter, "");
                break;

              case JSOP_TRY:
                js_printf(CLEAR_MAYBE_BRACE(jp), "\ttry {\n");
                jp->indent += 4;
                todo = -2;
                break;

              case JSOP_FINALLY:
                jp->indent -= 4;
                js_printf(CLEAR_MAYBE_BRACE(jp), "\t} finally {\n");
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

              case JSOP_SWAP:
                




                

              case JSOP_GOSUB:
              case JSOP_GOSUBX:
                




                todo = -2;
                break;

              case JSOP_POPN:
              {
                uintN newtop, oldtop, i;

                





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
                    for (i = newtop; i < oldtop; i++) {
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

                    





                    if (newtop < oldtop) {
                        ss->sprinter.offset = GetOff(ss, newtop);
                        ss->top = newtop;
                    }

                  end_groupassignment:
                    










                    rval = OFF2STR(&ss->sprinter, todo);
                    todo = -2;
                    pc2 = pc + oplen;
                    switch (*pc2) {
                      case JSOP_NOP:
                        
                        sn = js_GetSrcNote(jp->script, pc2);
                        if (sn) {
                            if (SN_TYPE(sn) == SRC_FOR) {
                                pc = pc2;
                                goto do_forloop;
                            }
                            if (SN_TYPE(sn) == SRC_DECL) {
                                if (ss->top == jp->script->depth) {
                                    




                                    pc = pc2 + 1;
                                    len = js_GetSrcNoteOffset(sn, 0);
                                    LOCAL_ASSERT(pc[len] == JSOP_LEAVEBLOCK);
                                    js_printf(jp, "\tlet (%s) {\n", rval);
                                    js_printf(jp, "\t}\n");
                                    goto end_popn;
                                }
                                todo = SprintCString(&ss->sprinter, rval);
                                if (todo < 0 || !PushOff(ss, todo, JSOP_NOP))
                                    return NULL;
                                op = JSOP_POP;
                                pc = pc2 + 1;
                                goto do_letheadbody;
                            }
                        }
                        break;

                      case JSOP_GOTO:
                      case JSOP_GOTOX:
                        
                        cond = GetJumpOffset(pc2, pc2);
                        sn = js_GetSrcNote(jp->script, pc2 + cond - 1);
                        if (sn && SN_TYPE(sn) == SRC_FOR) {
                            todo = SprintCString(&ss->sprinter, rval);
                            saveop = JSOP_NOP;
                        }
                        break;
                    }

                    



                    if (todo == -2) {
                        MAYBE_SET_DONT_BRACE(jp, pc, endpc, rval);
                        js_printf(jp, "\t%s;\n", rval);
                    }
                  end_popn:
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
                        JS_free(cx, (char *)lval);
                        return NULL;
                    }

                    
                    rval = POP_STR();
                    todo = Sprint(&ss->sprinter, "%s, %s", lval, rval);
                    JS_free(cx, (char *)lval);
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
                        js_printf(CLEAR_MAYBE_BRACE(jp), "\tlet (%s) {\n",
                                  POP_STR());
                        jp->indent += 4;
                        DECOMPILE_CODE(pc, len);
                        jp->indent -= 4;
                        js_printf(jp, "\t}\n");
                        todo = -2;
                    } else {
                        LOCAL_ASSERT(pc[len] == JSOP_LEAVEBLOCKEXPR);

                        lval = JS_strdup(cx, POP_STR());
                        if (!lval)
                            return NULL;

                        
                        saveop = JSOP_LEAVEBLOCKEXPR;
                        if (!Decompile(ss, pc, len, saveop)) {
                            JS_free(cx, (char *)lval);
                            return NULL;
                        }
                        rval = POP_STR();
                        todo = Sprint(&ss->sprinter,
                                      (*rval == '{')
                                      ? "let (%s) (%s)"
                                      : "let (%s) %s",
                                      lval, rval);
                        JS_free(cx, (char *)lval);
                    }
                    break;

                  default:
                    
                    if (ss->opcodes[ss->top-1] == JSOP_YIELD)
                        op = JSOP_NOP;

                    rval = POP_STR();

                    





                    if (*rval != '\0' && (rval[0] != '/' || rval[1] != '*')) {
                        MAYBE_SET_DONT_BRACE(jp, pc, endpc, rval);
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

              case JSOP_ENDITER:
                sn = js_GetSrcNote(jp->script, pc);
                todo = -2;
                if (sn && SN_TYPE(sn) == SRC_HIDDEN)
                    break;
                (void) PopOff(ss, op);
                break;

              case JSOP_ENTERWITH:
                LOCAL_ASSERT(!js_GetSrcNote(jp->script, pc));
                rval = POP_STR();
                js_printf(SET_MAYBE_BRACE(jp), "\twith (%s) {\n", rval);
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
                JSScopeProperty *sprop;

                LOAD_OBJECT(0);
                argc = OBJ_BLOCK_COUNT(cx, obj);
                if ((size_t)argc <= sizeof smallv / sizeof smallv[0]) {
                    atomv = smallv;
                } else {
                    atomv = (JSAtom **) JS_malloc(cx, argc * sizeof(JSAtom *));
                    if (!atomv)
                        return NULL;
                }

                
                for (sprop = OBJ_SCOPE(obj)->lastProp; sprop;
                     sprop = sprop->parent) {
                    if (!(sprop->flags & SPROP_HAS_SHORTID))
                        continue;
                    LOCAL_ASSERT(sprop->shortid < argc);
                    atomv[sprop->shortid] = JSID_TO_ATOM(sprop->id);
                }
                ok = JS_TRUE;
                for (i = 0; i < argc; i++) {
                    atom = atomv[i];
                    rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
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
                    js_printf(CLEAR_MAYBE_BRACE(jp), "\t{\n");
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
                    js_printf(CLEAR_MAYBE_BRACE(jp), "\t} catch (");

                    pc2 = pc;
                    pc += oplen;
                    LOCAL_ASSERT(*pc == JSOP_EXCEPTION);
                    pc += JSOP_EXCEPTION_LENGTH;
                    todo = Sprint(&ss->sprinter, exception_cookie);
                    if (todo < 0 || !PushOff(ss, todo, JSOP_EXCEPTION)) {
                        ok = JS_FALSE;
                        goto enterblock_out;
                    }

                    if (*pc == JSOP_DUP) {
                        sn2 = js_GetSrcNote(jp->script, pc);
                        if (!sn2 || SN_TYPE(sn2) != SRC_DESTRUCT) {
                            




                            LOCAL_ASSERT(js_GetSrcNoteOffset(sn, 0) != 0);
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
                        LOCAL_ASSERT(*pc == JSOP_POP);
                        pc += JSOP_POP_LENGTH;
                        lval = PopStr(ss, JSOP_NOP);
                        js_puts(jp, lval);
                    } else {
#endif
                        LOCAL_ASSERT(*pc == JSOP_SETLOCALPOP);
                        i = GET_UINT16(pc);
                        pc += JSOP_SETLOCALPOP_LENGTH;
                        atom = atomv[i - OBJ_BLOCK_DEPTH(cx, obj)];
                        str = ATOM_TO_STRING(atom);
                        if (!QuoteString(&jp->sprinter, str, 0)) {
                            ok = JS_FALSE;
                            goto enterblock_out;
                        }
#if JS_HAS_DESTRUCTURING
                    }
#endif

                    



                    rval = PopStr(ss, JSOP_NOP);
                    LOCAL_ASSERT(strcmp(rval, exception_cookie) == 0);

                    len = js_GetSrcNoteOffset(sn, 0);
                    if (len) {
                        len -= PTRDIFF(pc, pc2, jsbytecode);
                        LOCAL_ASSERT(len > 0);
                        js_printf(jp, " if ");
                        ok = Decompile(ss, pc, len, JSOP_NOP) != NULL;
                        if (!ok)
                            goto enterblock_out;
                        js_printf(jp, "%s", POP_STR());
                        pc += len;
                        LOCAL_ASSERT(*pc == JSOP_IFEQ || *pc == JSOP_IFEQX);
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

              enterblock_out:
                if (atomv != smallv)
                    JS_free(cx, atomv);
                if (!ok)
                    return NULL;
              }
              END_LITOPX_CASE

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

              case JSOP_CALLLOCAL:
              case JSOP_GETLOCAL:
                i = GET_UINT16(pc);
                LOCAL_ASSERT((uintN)i < ss->top);
                sn = js_GetSrcNote(jp->script, pc);

#if JS_HAS_DESTRUCTURING
                if (sn && SN_TYPE(sn) == SRC_GROUPASSIGN) {
                    pc = DecompileGroupAssignment(ss, pc, endpc, sn, &todo);
                    if (!pc)
                        return NULL;
                    LOCAL_ASSERT(*pc == JSOP_POPN);
                    len = oplen = JSOP_POPN_LENGTH;
                    goto end_groupassignment;
                }
#endif

                rval = GetLocal(ss, i);
                todo = Sprint(&ss->sprinter, ss_format, VarPrefix(sn), rval);
                break;

              case JSOP_SETLOCAL:
              case JSOP_SETLOCALPOP:
                i = GET_UINT16(pc);
                lval = GetStr(ss, i);
                rval = POP_STR();
                goto do_setlval;

              case JSOP_INCLOCAL:
              case JSOP_DECLOCAL:
                i = GET_UINT16(pc);
                lval = GetLocal(ss, i);
                goto do_inclval;

              case JSOP_LOCALINC:
              case JSOP_LOCALDEC:
                i = GET_UINT16(pc);
                lval = GetLocal(ss, i);
                goto do_lvalinc;

              case JSOP_FORLOCAL:
                i = GET_UINT16(pc);
                lval = GetStr(ss, i);
                atom = NULL;
                goto do_forlvalinloop;

              case JSOP_RETRVAL:
                todo = -2;
                break;

              case JSOP_RETURN:
                obj = jp->object;
                LOCAL_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_FunctionClass);
                fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj);
                if (fun->flags & JSFUN_EXPR_CLOSURE) {
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
                while ((op = (JSOp) ss->opcodes[--pos]) != JSOP_ENTERBLOCK &&
                       op != JSOP_NEWINIT) {
                    if (pos == 0)
                        break;
                }

                




                forpos = pos + (op == JSOP_ENTERBLOCK || op == JSOP_NEWINIT);
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

                



                LOCAL_ASSERT(ss->opcodes[pos] == JSOP_NEWINIT);
                start = ss->offsets[pos];
                LOCAL_ASSERT(ss->sprinter.base[start] == '[' ||
                             ss->sprinter.base[start] == '#');
                LOCAL_ASSERT(forpos < ss->top);
                xval = OFF2STR(&ss->sprinter, ss->offsets[forpos]);
                lval = OFF2STR(&ss->sprinter, start);
                RETRACT(&ss->sprinter, lval);

                todo = Sprint(&ss->sprinter, "%s%s%.*s",
                              lval, rval, rval - xval, xval);
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

              case JSOP_GOTO:
              case JSOP_GOTOX:
                sn = js_GetSrcNote(jp->script, pc);
                switch (sn ? SN_TYPE(sn) : SRC_NULL) {
                  case SRC_WHILE:
                    cond = GetJumpOffset(pc, pc);
                    tail = js_GetSrcNoteOffset(sn, 0);
                    DECOMPILE_CODE(pc + cond, tail - cond);
                    rval = POP_STR();
                    js_printf(SET_MAYBE_BRACE(jp), "\twhile (%s) {\n", rval);
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
                    rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
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
                    rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
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
                    op = JSOP_NOP;              
                    rval = POP_STR();
                    if (ss->inArrayInit || ss->inGenExp) {
                        LOCAL_ASSERT(SN_TYPE(sn) == SRC_IF);
                        ss->sprinter.offset -= PAREN_SLOP;
                        if (Sprint(&ss->sprinter, " if (%s)", rval) < 0)
                            return NULL;
                        AddParenSlop(ss);
                    } else {
                        js_printf(SET_MAYBE_BRACE(jp),
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
                            DECOMPILE_CODE(pc + oplen, cond - oplen);
                            pc += cond;
                            elseif = JS_TRUE;
                            goto if_again;
                        }

                        js_printf(SET_MAYBE_BRACE(jp), " {\n");
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
                        JS_free(cx, (void *)xval);
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
                    JS_free(cx, (void *)xval);
                    JS_free(cx, (void *)lval);
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
                len = PTRDIFF(done, pc, jsbytecode);
                if (!Decompile(ss, pc, len, op)) {
                    JS_free(cx, (char *)lval);
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
                        JS_free(cx, (char *)rval);
                    }
                    if (tail < 0)
                        todo = -1;
                } else {
                    todo = Sprint(&ss->sprinter, "%s %s %s", lval, xval, rval);
                }
                JS_free(cx, (char *)lval);
                break;

              case JSOP_AND:
              case JSOP_ANDX:
                xval = "&&";
                goto do_logical_connective;

              case JSOP_FORARG:
                atom = GetSlotAtom(jp, js_GetArgument, GET_ARGNO(pc));
                LOCAL_ASSERT(atom);
                goto do_fornameinloop;

              case JSOP_FORVAR:
              case JSOP_FORCONST:
                atom = GetSlotAtom(jp, js_GetLocalVariable, GET_VARNO(pc));
                LOCAL_ASSERT(atom);
                goto do_fornameinloop;

              case JSOP_FORNAME:
                LOAD_ATOM(0);

              do_fornameinloop:
                lval = "";
              do_forlvalinloop:
                sn = js_GetSrcNote(jp->script, pc);
                xval = NULL;
                goto do_forinloop;

              case JSOP_FORPROP:
                xval = NULL;
                LOAD_ATOM(0);
                if (!ATOM_IS_IDENTIFIER(atom)) {
                    xval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom),
                                       (jschar)'\'');
                    if (!xval)
                        return NULL;
                    atom = NULL;
                }
                lval = POP_STR();
                sn = NULL;

              do_forinloop:
                pc += oplen;
                LOCAL_ASSERT(*pc == JSOP_IFEQ || *pc == JSOP_IFEQX);
                oplen = js_CodeSpec[*pc].length;
                len = GetJumpOffset(pc, pc);
                sn2 = js_GetSrcNote(jp->script, pc);
                tail = js_GetSrcNoteOffset(sn2, 0);

              do_forinhead:
                if (!atom && xval) {
                    




                    if (*xval == '\0') {
                        xval = NULL;
                    } else {
                        xval = JS_strdup(cx, xval);
                        if (!xval)
                            return NULL;
                    }
                }

#if JS_HAS_XML_SUPPORT
                if (foreach) {
                    foreach = JS_FALSE;
                    todo = Sprint(&ss->sprinter, "for %s (%s%s",
                                  js_each_str, VarPrefix(sn), lval);
                } else
#endif
                {
                    todo = Sprint(&ss->sprinter, "for (%s%s",
                                  VarPrefix(sn), lval);
                }
                if (atom) {
                    if (*lval && SprintPut(&ss->sprinter, ".", 1) < 0)
                        return NULL;
                    xval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
                    if (!xval)
                        return NULL;
                } else if (xval) {
                    LOCAL_ASSERT(*xval != '\0');
                    ok = (Sprint(&ss->sprinter,
                                 (JOF_OPMODE(lastop) == JOF_XMLNAME)
                                 ? ".%s"
                                 : "[%s]",
                                 xval)
                          >= 0);
                    JS_free(cx, (char *)xval);
                    if (!ok)
                        return NULL;
                }
                if (todo < 0)
                    return NULL;

                lval = OFF2STR(&ss->sprinter, todo);
                rval = GetStr(ss, ss->top-1);
                RETRACT(&ss->sprinter, rval);
                if (ss->inArrayInit || ss->inGenExp) {
                    if (ss->top > 1 &&
                        (js_CodeSpec[ss->opcodes[ss->top-2]].format &
                         JOF_FOR)) {
                        ss->sprinter.offset -= PAREN_SLOP;
                    }
                    todo = Sprint(&ss->sprinter, " %s in %s)", lval, rval);
                    if (todo < 0)
                        return NULL;
                    ss->offsets[ss->top-1] = todo;
                    ss->opcodes[ss->top-1] = op;
                    AddParenSlop(ss);
                    DECOMPILE_CODE(pc + oplen, tail - oplen);
                } else {
                    js_printf(SET_MAYBE_BRACE(jp), "\t%s in %s) {\n",
                              lval, rval);
                    jp->indent += 4;
                    DECOMPILE_CODE(pc + oplen, tail - oplen);
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");
                }
                todo = -2;
                break;

              case JSOP_FORELEM:
                pc++;
                LOCAL_ASSERT(*pc == JSOP_IFEQ || *pc == JSOP_IFEQX);
                len = js_CodeSpec[*pc].length;

                





                sn = js_GetSrcNote(jp->script, pc);
                LOCAL_ASSERT(!forelem_tail);
                forelem_tail = pc + js_GetSrcNoteOffset(sn, 0);

                











                LOCAL_ASSERT(!forelem_done);
                forelem_done = pc + GetJumpOffset(pc, pc);

                
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
                LOCAL_ASSERT(forelem_tail > pc);
                tail = forelem_tail - pc;
                forelem_tail = NULL;
                LOCAL_ASSERT(forelem_done > pc);
                len = forelem_done - pc;
                forelem_done = NULL;
                goto do_forinhead;

#if JS_HAS_GETTER_SETTER
              case JSOP_GETTER:
              case JSOP_SETTER:
                todo = -2;
                break;
#endif

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
                        LOCAL_ASSERT(forelem_tail > pc);
                        tail = forelem_tail - pc;
                        forelem_tail = NULL;
                        LOCAL_ASSERT(forelem_done > pc);
                        len = forelem_done - pc;
                        forelem_done = NULL;
                        xval = NULL;
                        atom = NULL;

                        




                        if (js_GetSrcNoteOffset(sn, 0) == SRC_DECL_VAR)
                            sn = NULL;
                        goto do_forinhead;
                    }

                    todo = Sprint(&ss->sprinter, "%s%s = %s",
                                  VarPrefix(sn), lval, rval);
                    break;
                }
#endif

                rval = GetStr(ss, ss->top-1);
                saveop = (JSOp) ss->opcodes[ss->top-1];
                todo = SprintCString(&ss->sprinter, rval);
                break;

              case JSOP_SETARG:
                atom = GetSlotAtom(jp, js_GetArgument, GET_ARGNO(pc));
                LOCAL_ASSERT(atom);
                goto do_setname;

              case JSOP_SETVAR:
                atom = GetSlotAtom(jp, js_GetLocalVariable, GET_VARNO(pc));
                LOCAL_ASSERT(atom);
                goto do_setname;

              case JSOP_SETCONST:
              case JSOP_SETNAME:
              case JSOP_SETGVAR:
                LOAD_ATOM(0);

              do_setname:
                lval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
                if (!lval)
                    return NULL;
                rval = POP_STR();
                if (op == JSOP_SETNAME)
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
#if JS_HAS_LVALUE_RETURN
              case JSOP_SETCALL:
#endif
                
                argc = GET_ARGC(pc);
                op = (argc == 1) ? JSOP_NOP : JSOP_SETNAME;
                argv = (char **)
                    JS_malloc(cx, (size_t)(argc + 1) * sizeof *argv);
                if (!argv)
                    return NULL;

                ok = JS_TRUE;
                for (i = argc; i > 0; i--) {
                    argv[i] = JS_strdup(cx, POP_STR());
                    if (!argv[i])
                        ok = JS_FALSE;
                }

                
                LOCAL_ASSERT(ss->top >= 2);
                (void) PopOff(ss, op);

                



                op = (JSOp) ss->opcodes[ss->top-1];
                lval = PopStr(ss,
                              (saveop == JSOP_NEW &&
                               (op == JSOP_CALL || op == JSOP_EVAL ||
                                (js_CodeSpec[op].format & JOF_CALLOP)))
                              ? JSOP_NAME
                              : saveop);
                op = saveop;

                argv[0] = JS_strdup(cx, lval);
                if (!argv[i])
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

                for (i = 0; i <= argc; i++) {
                    if (argv[i])
                        JS_free(cx, argv[i]);
                }
                JS_free(cx, argv);
                if (!ok)
                    return NULL;
#if JS_HAS_LVALUE_RETURN
                if (op == JSOP_SETCALL) {
                    if (!PushOff(ss, todo, op))
                        return NULL;
                    todo = Sprint(&ss->sprinter, "");
                }
#endif
                break;

              case JSOP_DELNAME:
                LOAD_ATOM(0);
                lval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
                if (!lval)
                    return NULL;
                RETRACT(&ss->sprinter, lval);
              do_delete_lval:
                todo = Sprint(&ss->sprinter, "%s %s", js_delete_str, lval);
                break;

              case JSOP_DELPROP:
                GET_ATOM_QUOTE_AND_FMT("%s %s[%s]", "%s %s.%s", rval);
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, fmt, js_delete_str, lval, rval);
                break;

              case JSOP_DELELEM:
                op = JSOP_NOP;          
                xval = POP_STR();
                op = saveop;
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
                atom = GetSlotAtom(jp, js_GetArgument, GET_ARGNO(pc));
                LOCAL_ASSERT(atom);
                goto do_incatom;

              case JSOP_INCVAR:
              case JSOP_DECVAR:
                atom = GetSlotAtom(jp, js_GetLocalVariable, GET_VARNO(pc));
                LOCAL_ASSERT(atom);
                goto do_incatom;

              case JSOP_INCNAME:
              case JSOP_DECNAME:
              case JSOP_INCGVAR:
              case JSOP_DECGVAR:
                LOAD_ATOM(0);
              do_incatom:
                lval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
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
                atom = GetSlotAtom(jp, js_GetArgument, GET_ARGNO(pc));
                LOCAL_ASSERT(atom);
                goto do_atominc;

              case JSOP_VARINC:
              case JSOP_VARDEC:
                atom = GetSlotAtom(jp, js_GetLocalVariable, GET_VARNO(pc));
                LOCAL_ASSERT(atom);
                goto do_atominc;

              case JSOP_NAMEINC:
              case JSOP_NAMEDEC:
              case JSOP_GVARINC:
              case JSOP_GVARDEC:
                LOAD_ATOM(0);
              do_atominc:
                lval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
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

              case JSOP_GETPROP2:
                op = JSOP_GETPROP;
                (void) PopOff(ss, lastop);
                

#if JS_HAS_XML_SUPPORT
              case JSOP_CALLPROP:
#endif
              case JSOP_GETPROP:
              case JSOP_GETXPROP:
                LOAD_ATOM(0);

              do_getprop:
                GET_QUOTE_AND_FMT(index_format, dot_format, rval);
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, fmt, lval, rval);
                break;

              BEGIN_LITOPX_CASE(JSOP_GETTHISPROP, 0)
                GET_QUOTE_AND_FMT(index_format, dot_format, rval);
                todo = Sprint(&ss->sprinter, fmt, js_this_str, rval);
              END_LITOPX_CASE

              BEGIN_LITOPX_CASE(JSOP_GETARGPROP, ARGNO_LEN)
                if (!PushSlotAtom(ss, js_GetArgument, GET_ARGNO(pc), op))
                    return NULL;
                goto do_getprop;

              BEGIN_LITOPX_CASE(JSOP_GETVARPROP, VARNO_LEN)
                if (!PushSlotAtom(ss, js_GetLocalVariable, GET_VARNO(pc), op))
                    return NULL;
                goto do_getprop;

              BEGIN_LITOPX_CASE(JSOP_GETLOCALPROP, 2)
                i = GET_UINT16(pc);
                LOCAL_ASSERT((uintN)i < ss->top);
                lval = GetLocal(ss, i);
                if (!lval)
                    return NULL;
                todo = SprintCString(&ss->sprinter, lval);
                if (todo < 0 || !PushOff(ss, todo, op))
                    return NULL;
                goto do_getprop;

              case JSOP_SETPROP:
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
                atom = GetSlotAtom(jp, js_GetArgument, i);
#if JS_HAS_DESTRUCTURING
                if (!atom) {
                    todo = Sprint(&ss->sprinter, "%s[%d]", js_arguments_str, i);
                    break;
                }
#else
                LOCAL_ASSERT(atom);
#endif
                goto do_name;

              case JSOP_CALLVAR:
              case JSOP_GETVAR:
                atom = GetSlotAtom(jp, js_GetLocalVariable, GET_VARNO(pc));
                LOCAL_ASSERT(atom);
                goto do_name;

              case JSOP_CALLNAME:
              case JSOP_NAME:
              case JSOP_GETGVAR:
              case JSOP_CALLGVAR:
                LOAD_ATOM(0);
              do_name:
                lval = "";
              do_qname:
                sn = js_GetSrcNote(jp->script, pc);
                rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom),
                                   inXML ? DONT_ESCAPE : 0);
                if (!rval)
                    return NULL;
                RETRACT(&ss->sprinter, rval);
                todo = Sprint(&ss->sprinter, "%s%s%s",
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

              BEGIN_LITOPX_CASE(JSOP_DOUBLE, 0)
                val = ATOM_KEY(atom);
                JS_ASSERT(JSVAL_IS_DOUBLE(val));
                todo = SprintDoubleValue(&ss->sprinter, val, &saveop);
              END_LITOPX_CASE

              BEGIN_LITOPX_CASE(JSOP_STRING, 0)
                rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom),
                                   inXML ? DONT_ESCAPE : '"');
                if (!rval)
                    return NULL;
                todo = STR2OFF(&ss->sprinter, rval);
              END_LITOPX_CASE

              case JSOP_ANONFUNOBJ:
#if JS_HAS_GENERATOR_EXPRS
                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_GENEXP) {
                    JSScript *inner, *outer;
                    void *mark;
                    SprintStack ss2;

                    LOAD_FUNCTION(0);
                    fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj);
                    LOCAL_ASSERT(FUN_INTERPRETED(fun));
                    inner = fun->u.i.script;

                    





                    mark = JS_ARENA_MARK(&cx->tempPool);
                    if (!InitSprintStack(cx, &ss2, jp, inner->depth))
                        return NULL;
                    ss2.inGenExp = JS_TRUE;

                    






                    outer = jp->script;
                    LOCAL_ASSERT(JS_UPTRDIFF(pc, outer->code) <= outer->length);
                    jp->script = inner;
                    if (!Decompile(&ss2, inner->code, inner->length,
                                   JSOP_NOP)) {
                        JS_ARENA_RELEASE(&cx->tempPool, mark);
                        return NULL;
                    }
                    jp->script = outer;

                    



                    pc += len;
                    LOCAL_ASSERT(*pc == JSOP_GLOBALTHIS);
                    pc += JSOP_NULL_LENGTH;
                    LOCAL_ASSERT(*pc == JSOP_CALL);
                    LOCAL_ASSERT(GET_ARGC(pc) == 0);
                    len = JSOP_CALL_LENGTH;

                    















                    LOCAL_ASSERT(pc + len < endpc ||
                                 endpc < outer->code + outer->length);
                    LOCAL_ASSERT(ss2.top == 1);
                    ss2.opcodes[0] = JSOP_POP;
                    if (pc + len == endpc &&
                        ((JSOp) *endpc != JSOP_IFNE &&
                         (JSOp) *endpc != JSOP_IFNEX)) {
                        op = JSOP_SETNAME;
                    } else {
                        op = (JSOp) pc[len];
                        op = ((js_CodeSpec[op].format & JOF_PARENHEAD) ||
                              ((js_CodeSpec[op].format & JOF_INVOKE) &&
                               GET_ARGC(pc + len) == 1) ||
                              (((op == JSOP_IFEQ || op == JSOP_IFEQX) &&
                               (sn2 = js_GetSrcNote(outer, pc + len)) &&
                               SN_TYPE(sn2) != SRC_COND)))
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
                    JS_free(cx, (void *)rval);
                    break;
                }
#endif 
                

              case JSOP_NAMEDFUNOBJ:
                LOAD_FUNCTION(0);
                {
                    uintN indent = JS_DONT_PRETTY_PRINT;

                    





                    fun = (JSFunction *) OBJ_GET_PRIVATE(cx, obj);
                    if (!(fun->flags & JSFUN_EXPR_CLOSURE))
                        indent |= JS_IN_GROUP_CONTEXT;
                    str = JS_DecompileFunction(cx, fun, indent);
                    if (!str)
                        return NULL;
                }
              sprint_string:
                todo = SprintString(&ss->sprinter, str);
                break;

              case JSOP_OBJECT:
                LOAD_OBJECT(0);
                JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_RegExpClass);
                goto do_regexp;

              case JSOP_REGEXP:
                GET_REGEXP_FROM_BYTECODE(jp->script, pc, 0, obj);
              do_regexp:
                if (!js_regexp_toString(cx, obj, &val))
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
                            JS_malloc(cx, (size_t)n * sizeof *table);
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
                          JS_malloc(cx, (size_t)j * sizeof *table);
                    if (tmp) {
                        ok = js_MergeSort(table, (size_t)j, sizeof(TableEntry),
                                          CompareOffsets, NULL, tmp);
                        JS_free(cx, tmp);
                    } else {
                        ok = JS_FALSE;
                    }
                }

                if (ok) {
                    ok = DecompileSwitch(ss, table, (uintN)j, pc, len, off,
                                         JS_FALSE);
                }
                JS_free(cx, table);
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
                    JS_malloc(cx, (size_t)npairs * sizeof *table);
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
                    JS_GET_SCRIPT_ATOM(jp->script, GET_INDEX(pc2), atom);
                    pc2 += INDEX_LEN;
                    off2 = GetJumpOffset(pc, pc2);
                    pc2 += jmplen;
                    table[k].key = ATOM_KEY(atom);
                    table[k].offset = off2;
                }

                ok = DecompileSwitch(ss, table, (uintN)npairs, pc, len, off,
                                     JS_FALSE);
                JS_free(cx, table);
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
                    JS_malloc(cx, (size_t)ncases * sizeof *table);
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
                JS_free(cx, table);
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

              case JSOP_STRICTEQ:
              case JSOP_STRICTNE:
                rval = POP_STR();
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s %c== %s",
                              lval, (op == JSOP_STRICTEQ) ? '=' : '!', rval);
                break;

              case JSOP_CLOSURE:
                LOAD_FUNCTION(0);
                todo = -2;
                goto do_function;
              END_LITOPX_CASE

#if JS_HAS_EXPORT_IMPORT
              case JSOP_EXPORTALL:
                js_printf(jp, "\texport *;\n");
                todo = -2;
                break;

              BEGIN_LITOPX_CASE(JSOP_EXPORTNAME, 0)
                rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
                if (!rval)
                    return NULL;
                RETRACT(&ss->sprinter, rval);
                js_printf(jp, "\texport %s;\n", rval);
                todo = -2;
              END_LITOPX_CASE

              case JSOP_IMPORTALL:
                lval = POP_STR();
                js_printf(jp, "\timport %s.*;\n", lval);
                todo = -2;
                break;

              case JSOP_IMPORTPROP:
              do_importprop:
                GET_ATOM_QUOTE_AND_FMT("\timport %s[%s];\n",
                                       "\timport %s.%s;\n",
                                       rval);
                lval = POP_STR();
                js_printf(jp, fmt, lval, rval);
                todo = -2;
                break;

              case JSOP_IMPORTELEM:
                xval = POP_STR();
                op = JSOP_GETELEM;
                if (JOF_OPMODE(lastop) == JOF_XMLNAME)
                    goto do_importprop;
                lval = POP_STR();
                js_printf(jp, "\timport %s[%s];\n", lval, xval);
                todo = -2;
                break;
#endif 

              case JSOP_TRAP:
                op = JS_GetTrapOpcode(cx, jp->script, pc);
                if (op == JSOP_LIMIT)
                    return NULL;
                saveop = op;
                *pc = op;
                cs = &js_CodeSpec[op];
                len = cs->length;
                DECOMPILE_CODE(pc, len);
                *pc = JSOP_TRAP;
                todo = -2;
                break;

              case JSOP_NEWINIT:
              {
                JSBool isArray;

                LOCAL_ASSERT(ss->top >= 2);
                (void) PopOff(ss, op);
                lval = POP_STR();
                isArray = (*lval == 'A');
                todo = ss->sprinter.offset;
#if JS_HAS_SHARP_VARS
                op = (JSOp)pc[len];
                if (op == JSOP_DEFSHARP) {
                    pc += len;
                    cs = &js_CodeSpec[op];
                    len = cs->length;
                    i = (jsint) GET_UINT16(pc);
                    if (Sprint(&ss->sprinter, "#%u=", (unsigned) i) < 0)
                        return NULL;
                }
#endif 
                if (isArray) {
                    ++ss->inArrayInit;
                    if (SprintCString(&ss->sprinter, "[") < 0)
                        return NULL;
                } else {
                    if (SprintCString(&ss->sprinter, "{") < 0)
                        return NULL;
                }
                break;
              }

              case JSOP_ENDINIT:
                op = JSOP_NOP;           
                rval = POP_STR();
                sn = js_GetSrcNote(jp->script, pc);

                
                for (xval = rval; *xval != '[' && *xval != '{'; xval++)
                    continue;
                if (*xval == '[')
                    --ss->inArrayInit;
                todo = Sprint(&ss->sprinter, "%s%s%c",
                              rval,
                              (sn && SN_TYPE(sn) == SRC_CONTINUE) ? ", " : "",
                              (*xval == '[') ? ']' : '}');
                break;

              case JSOP_INITPROP:
                LOAD_ATOM(0);
                xval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom),
                                   (jschar)
                                   (ATOM_IS_IDENTIFIER(atom) ? 0 : '\''));
                if (!xval)
                    return NULL;
                rval = POP_STR();
                lval = POP_STR();
              do_initprop:
#ifdef OLD_GETTER_SETTER
                todo = Sprint(&ss->sprinter, "%s%s%s%s%s:%s",
                              lval,
                              (lval[1] != '\0') ? ", " : "",
                              xval,
                              (lastop == JSOP_GETTER || lastop == JSOP_SETTER)
                              ? " " : "",
                              (lastop == JSOP_GETTER) ? js_getter_str :
                              (lastop == JSOP_SETTER) ? js_setter_str :
                              "",
                              rval);
#else
                if (lastop == JSOP_GETTER || lastop == JSOP_SETTER) {
                    if (!atom ||
                        !ATOM_IS_STRING(atom) ||
                        !ATOM_IS_IDENTIFIER(atom) ||
                        ATOM_IS_KEYWORD(atom) ||
                        (ss->opcodes[ss->top+1] != JSOP_ANONFUNOBJ &&
                         ss->opcodes[ss->top+1] != JSOP_NAMEDFUNOBJ)) {
                        todo = Sprint(&ss->sprinter, "%s%s%s %s: %s", lval,
                                      (lval[1] != '\0') ? ", " : "", xval,
                                      (lastop == JSOP_GETTER) ? js_getter_str :
                                      (lastop == JSOP_SETTER) ? js_setter_str :
                                      "",
                                      rval);
                    } else {
                        const char *end = rval + strlen(rval);

                        if (*rval == '(')
                            ++rval, --end;
                        LOCAL_ASSERT(strncmp(rval, js_function_str, 8) == 0);
                        LOCAL_ASSERT(rval[8] == ' ');
                        rval += 8 + 1;
                        LOCAL_ASSERT(*end ? *end == ')' : end[-1] == '}');
                        todo = Sprint(&ss->sprinter, "%s%s%s %s%s%.*s",
                                      lval,
                                      (lval[1] != '\0') ? ", " : "",
                                      (lastop == JSOP_GETTER)
                                      ? js_get_str : js_set_str,
                                      xval,
                                      (rval[0] != '(') ? " " : "",
                                      end - rval, rval);
                    }
                } else {
                    todo = Sprint(&ss->sprinter, "%s%s%s: %s",
                                  lval,
                                  (lval[1] != '\0') ? ", " : "",
                                  xval,
                                  rval);
                }
#endif
                break;

              case JSOP_INITELEM:
                
                LOCAL_ASSERT(pc + len < endpc);
                op = (GetStr(ss, ss->top - 3)[1] == '\0' &&
                      GetStr(ss, ss->top - 2)[0] == '0' &&
                      (JSOp) pc[len] == JSOP_ENDINIT)
                     ? JSOP_NOP
                     : JSOP_SETNAME;
                rval = POP_STR();

                
                op = JSOP_NOP;
                xval = POP_STR();
                lval = POP_STR();
                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_INITPROP) {
                    atom = NULL;
                    goto do_initprop;
                }
                todo = Sprint(&ss->sprinter, "%s%s%s",
                              lval,
                              (lval[1] != '\0' || *xval != '0') ? ", " : "",
                              rval);
                break;

#if JS_HAS_SHARP_VARS
              case JSOP_DEFSHARP:
                i = (jsint) GET_UINT16(pc);
                rval = POP_STR();
                todo = Sprint(&ss->sprinter, "#%u=%s", (unsigned) i, rval);
                break;

              case JSOP_USESHARP:
                i = (jsint) GET_UINT16(pc);
                todo = Sprint(&ss->sprinter, "#%u#", (unsigned) i);
                break;
#endif 

#if JS_HAS_DEBUGGER_KEYWORD
              case JSOP_DEBUGGER:
                js_printf(jp, "\tdebugger;\n");
                todo = -2;
                break;
#endif 

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

              BEGIN_LITOPX_CASE(JSOP_QNAMEPART, 0)
                if (pc[JSOP_QNAMEPART_LENGTH] == JSOP_TOATTRNAME) {
                    saveop = JSOP_TOATTRNAME;
                    len += JSOP_TOATTRNAME_LENGTH;
                    lval = "@";
                    goto do_qname;
                }
                goto do_name;
              END_LITOPX_CASE

              BEGIN_LITOPX_CASE(JSOP_QNAMECONST, 0)
                rval = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0);
                if (!rval)
                    return NULL;
                RETRACT(&ss->sprinter, rval);
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s::%s", lval, rval);
              END_LITOPX_CASE

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

              case JSOP_FOREACH:
                foreach = JS_TRUE;
                todo = -2;
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
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s.(%s)", lval, rval);
                break;

              case JSOP_DESCENDANTS:
                rval = POP_STR();
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, "%s..%s", lval, rval);
                break;

              case JSOP_XMLOBJECT:
                LOAD_OBJECT(0);
                todo = Sprint(&ss->sprinter, "<xml address='%p'>", obj);
                break;

              BEGIN_LITOPX_CASE(JSOP_XMLCDATA, 0)
                todo = SprintPut(&ss->sprinter, "<![CDATA[", 9);
                if (!QuoteString(&ss->sprinter, ATOM_TO_STRING(atom),
                                 DONT_ESCAPE))
                    return NULL;
                SprintPut(&ss->sprinter, "]]>", 3);
              END_LITOPX_CASE

              BEGIN_LITOPX_CASE(JSOP_XMLCOMMENT, 0)
                todo = SprintPut(&ss->sprinter, "<!--", 4);
                if (!QuoteString(&ss->sprinter, ATOM_TO_STRING(atom),
                                 DONT_ESCAPE))
                    return NULL;
                SprintPut(&ss->sprinter, "-->", 3);
              END_LITOPX_CASE

              BEGIN_LITOPX_CASE(JSOP_XMLPI, 0)
                rval = JS_strdup(cx, POP_STR());
                if (!rval)
                    return NULL;
                todo = SprintPut(&ss->sprinter, "<?", 2);
                ok = QuoteString(&ss->sprinter, ATOM_TO_STRING(atom), 0) &&
                     (*rval == '\0' ||
                      (SprintPut(&ss->sprinter, " ", 1) >= 0 &&
                       SprintCString(&ss->sprinter, rval)));
                JS_free(cx, (char *)rval);
                if (!ok)
                    return NULL;
                SprintPut(&ss->sprinter, "?>", 2);
              END_LITOPX_CASE

              case JSOP_GETFUNNS:
                todo = SprintPut(&ss->sprinter, js_function_str, 8);
                break;
#endif 

              default:
                todo = -2;
                break;

#undef BEGIN_LITOPX_CASE
#undef END_LITOPX_CASE
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
#undef POP_STR
#undef LOCAL_ASSERT
#undef ATOM_IS_IDENTIFIER
#undef GET_QUOTE_AND_FMT
#undef GET_ATOM_QUOTE_AND_FMT

    return pc;
}

JSBool
js_DecompileCode(JSPrinter *jp, JSScript *script, jsbytecode *pc, uintN len,
                 uintN pcdepth)
{
    uintN depth, i;
    SprintStack ss;
    JSContext *cx;
    void *mark;
    JSBool ok;
    JSScript *oldscript;
    char *last;

    depth = script->depth;
    JS_ASSERT(pcdepth <= depth);

    
    cx = jp->sprinter.context;
    mark = JS_ARENA_MARK(&cx->tempPool);
    ok = InitSprintStack(cx, &ss, jp, depth);
    if (!ok)
        goto out;

    










    ss.top = pcdepth;
    if (pcdepth != 0) {
        JSStackFrame *fp;
        ptrdiff_t top;

        for (fp = cx->fp; fp && !fp->script; fp = fp->down)
            continue;
        top = fp ? fp->sp - fp->spbase : 0;
        for (i = 0; i < pcdepth; i++) {
            ss.offsets[i] = -1;
            ss.opcodes[i] = JSOP_NOP;
        }
        if (fp && fp->pc == pc && (uintN)top == pcdepth) {
            for (i = 0; i < pcdepth; i++) {
                ptrdiff_t off;
                jsbytecode *genpc;

                off = (intN)i - (intN)depth;
                genpc = (jsbytecode *) fp->spbase[off];
                if (JS_UPTRDIFF(genpc, script->code) < script->length) {
                    ss.offsets[i] += (ptrdiff_t)i - top;
                    ss.opcodes[i] = *genpc;
                }
            }
        }
    }

    
    oldscript = jp->script;
    jp->script = script;
    ok = Decompile(&ss, pc, len, JSOP_NOP) != NULL;
    jp->script = oldscript;

    
    if (ss.top) {
        do {
            last = OFF2STR(&ss.sprinter, PopOff(&ss, JSOP_POP));
        } while (ss.top > pcdepth);
        js_printf(jp, "%s", last);
    }

out:
    
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return ok;
}

JSBool
js_DecompileScript(JSPrinter *jp, JSScript *script)
{
    return js_DecompileCode(jp, script, script->code, (uintN)script->length, 0);
}

static const char native_code_str[] = "\t[native code]\n";

JSBool
js_DecompileFunctionBody(JSPrinter *jp, JSFunction *fun)
{
    JSScript *script;
    JSObject *oldobject;
    JSBool ok;

    if (!FUN_INTERPRETED(fun)) {
        js_printf(jp, native_code_str);
        return JS_TRUE;
    }
    script = fun->u.i.script;
    oldobject = jp->object;
    jp->object = fun->object;
    ok = js_DecompileCode(jp, script, script->code, (uintN)script->length, 0);
    jp->object = oldobject;
    return ok;
}

JSBool
js_DecompileFunction(JSPrinter *jp, JSFunction *fun)
{
    JSContext *cx;
    uintN i, nargs, indent;
    void *mark;
    JSAtom **params;
    JSObject *oldobject;
    JSScopeProperty *sprop;
    jsbytecode *pc, *endpc;
    ptrdiff_t len;
    JSBool ok;

    




    if (jp->pretty) {
        js_printf(jp, "\t");
    } else {
        if (!jp->grouped && (fun->flags & JSFUN_LAMBDA))
            js_puts(jp, "(");
    }
    if (JSFUN_GETTER_TEST(fun->flags))
        js_printf(jp, "%s ", js_getter_str);
    else if (JSFUN_SETTER_TEST(fun->flags))
        js_printf(jp, "%s ", js_setter_str);

    js_printf(jp, "%s ", js_function_str);
    if (fun->atom && !QuoteString(&jp->sprinter, ATOM_TO_STRING(fun->atom), 0))
        return JS_FALSE;
    js_puts(jp, "(");

    if (FUN_INTERPRETED(fun) && fun->object) {
        size_t paramsize;
#ifdef JS_HAS_DESTRUCTURING
        SprintStack ss;
        JSScript *oldscript;
#endif

        









        cx = jp->sprinter.context;
        nargs = fun->nargs;
        mark = JS_ARENA_MARK(&cx->tempPool);
        paramsize = nargs * sizeof(JSAtom *);
        JS_ARENA_ALLOCATE_CAST(params, JSAtom **, &cx->tempPool, paramsize);
        if (!params) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }

        memset(params, 0, paramsize);
        for (sprop = SCOPE_LAST_PROP(OBJ_SCOPE(fun->object)); sprop;
             sprop = sprop->parent) {
            if (sprop->getter != js_GetArgument)
                continue;
            JS_ASSERT(sprop->flags & SPROP_HAS_SHORTID);
            JS_ASSERT((uint16) sprop->shortid < nargs);
            JS_ASSERT(JSID_IS_ATOM(sprop->id));
            params[(uint16) sprop->shortid] = JSID_TO_ATOM(sprop->id);
        }

        pc = fun->u.i.script->main;
        endpc = pc + fun->u.i.script->length;
        ok = JS_TRUE;

#ifdef JS_HAS_DESTRUCTURING
        
        if (*pc == JSOP_GENERATOR)
            pc += JSOP_GENERATOR_LENGTH;

        ss.printer = NULL;
        oldscript = jp->script;
        jp->script = fun->u.i.script;
        oldobject = jp->object;
        jp->object = fun->object;
#endif

        for (i = 0; i < nargs; i++) {
            if (i > 0)
                js_puts(jp, ", ");

#if JS_HAS_DESTRUCTURING
#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_RV(expr, JS_FALSE)

            if (!params[i]) {
                ptrdiff_t todo;
                const char *lval;

                LOCAL_ASSERT(*pc == JSOP_GETARG);
                pc += JSOP_GETARG_LENGTH;
                LOCAL_ASSERT(*pc == JSOP_DUP);
                if (!ss.printer) {
                    ok = InitSprintStack(cx, &ss, jp, fun->u.i.script->depth);
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

            if (!QuoteString(&jp->sprinter, ATOM_TO_STRING(params[i]), 0)) {
                ok = JS_FALSE;
                break;
            }
        }

#ifdef JS_HAS_DESTRUCTURING
        jp->script = oldscript;
        jp->object = oldobject;
#endif
        JS_ARENA_RELEASE(&cx->tempPool, mark);
        if (!ok)
            return JS_FALSE;
#ifdef __GNUC__
    } else {
        pc = NULL;
#endif
    }

    indent = jp->indent;
    if (fun->flags & JSFUN_EXPR_CLOSURE) {
        js_printf(jp, ") ");
    } else {
        js_printf(jp, ") {\n");
        jp->indent += 4;
    }

    if (FUN_INTERPRETED(fun) && fun->object) {
        oldobject = jp->object;
        jp->object = fun->object;
        len = fun->u.i.script->code + fun->u.i.script->length - pc;
        ok = js_DecompileCode(jp, fun->u.i.script, pc, (uintN)len, 0);
        jp->object = oldobject;
        if (!ok) {
            jp->indent = indent;
            return JS_FALSE;
        }
    } else {
        js_printf(jp, native_code_str);
    }

    if (!(fun->flags & JSFUN_EXPR_CLOSURE)) {
        jp->indent -= 4;
        js_printf(jp, "\t}");
    }

    if (!jp->pretty) {
        if (!jp->grouped && (fun->flags & JSFUN_LAMBDA))
            js_puts(jp, ")");
    }
    return JS_TRUE;
}

#undef LOCAL_ASSERT_RV

char *
js_DecompileValueGenerator(JSContext *cx, intN spindex, jsval v,
                           JSString *fallback)
{
    JSStackFrame *fp, *down;
    jsbytecode *pc, *begin, *end;
    jsval *sp, *spbase, *base, *limit;
    intN depth, pcdepth;
    JSScript *script;
    JSOp op;
    const JSCodeSpec *cs;
    jssrcnote *sn;
    ptrdiff_t len, oplen;
    JSPrinter *jp;
    char *name;

    for (fp = cx->fp; fp && !fp->script; fp = fp->down)
        continue;
    if (!fp)
        goto do_fallback;

    
    pc = fp->pc;
    sp = fp->sp;
    spbase = fp->spbase;
    if ((uintN)(sp - spbase) > fp->script->depth) {
        




        goto do_fallback;
    }

    if (spindex == JSDVG_SEARCH_STACK) {
        if (!pc) {
            




            JS_ASSERT(!fp->script && !(fp->fun && FUN_INTERPRETED(fp->fun)));
            down = fp->down;
            if (!down)
                goto do_fallback;
            script = down->script;
            spbase = down->spbase;
            base = fp->argv;
            limit = base + fp->argc;
        } else {
            





            script = fp->script;
            spbase = base = fp->spbase;
            limit = fp->sp;
        }

        




        if (!script || !base || !limit)
            goto do_fallback;

        














        for (sp = limit;;) {
            if (sp <= base)
                goto do_fallback;
            --sp;
            if (*sp == v) {
                depth = (intN)script->depth;
                sp -= depth;
                pc = (jsbytecode *) *sp;
                break;
            }
        }
    } else {
        




        if (!pc)
            goto do_fallback;
        script = fp->script;
        if (!script)
            goto do_fallback;

        if (spindex != JSDVG_IGNORE_STACK) {
            JS_ASSERT(spindex < 0);
            depth = (intN)script->depth;
#if !JS_HAS_NO_SUCH_METHOD
            JS_ASSERT(-depth <= spindex);
#endif
            spindex -= depth;

            base = (jsval *) cx->stackPool.current->base;
            limit = (jsval *) cx->stackPool.current->avail;
            sp = fp->sp + spindex;
            if (JS_UPTRDIFF(sp, base) < JS_UPTRDIFF(limit, base))
                pc = (jsbytecode *) *sp;
        }
    }

    



    if (JS_UPTRDIFF(pc, script->code) >= (jsuword)script->length) {
        pc = fp->pc;
        if (!pc)
            goto do_fallback;
    }
    op = (JSOp) *pc;
    if (op == JSOP_TRAP)
        op = JS_GetTrapOpcode(cx, script, pc);

    
    JS_ASSERT(op != JSOP_CASE && op != JSOP_CASEX &&
              op != JSOP_DUP && op != JSOP_DUP2 &&
              op != JSOP_SWAP);

    



    if (op == JSOP_THIS)
        return JS_strdup(cx, js_this_str);

    





    if (op == JSOP_BINDNAME)
        goto do_fallback;

    
    cs = &js_CodeSpec[op];
    begin = pc;
    end = pc + cs->length;
    switch (JOF_MODE(cs->format)) {
      case JOF_PROP:
      case JOF_ELEM:
      case JOF_XMLNAME:
      case 0:
        sn = js_GetSrcNote(script, pc);
        if (!sn)
            goto do_fallback;
        switch (SN_TYPE(sn)) {
          case SRC_PCBASE:
            begin -= js_GetSrcNoteOffset(sn, 0);
            break;
          case SRC_PCDELTA:
            end = begin + js_GetSrcNoteOffset(sn, 0);
            begin += cs->length;
            break;
          default:
            goto do_fallback;
        }
        break;
      default:;
    }
    len = PTRDIFF(end, begin, jsbytecode);
    if (len <= 0)
        goto do_fallback;

    




    pcdepth = 0;
    for (pc = script->main; pc < begin; pc += oplen) {
        uint32 type;
        intN nuses, ndefs;

        op = (JSOp) *pc;
        if (op == JSOP_TRAP)
            op = JS_GetTrapOpcode(cx, script, pc);
        cs = &js_CodeSpec[op];
        oplen = cs->length;

        if (op == JSOP_POPN) {
            pcdepth -= GET_UINT16(pc);
            continue;
        }

        






        sn = js_GetSrcNote(script, pc);
        if (sn && SN_TYPE(sn) == SRC_COND) {
            ptrdiff_t jmpoff, jmplen;

            jmpoff = js_GetSrcNoteOffset(sn, 0);
            if (pc + jmpoff < begin) {
                pc += jmpoff;
                op = (JSOp) *pc;
                JS_ASSERT(op == JSOP_GOTO || op == JSOP_GOTOX);
                cs = &js_CodeSpec[op];
                oplen = cs->length;
                jmplen = GetJumpOffset(pc, pc);
                if (pc + jmplen < begin) {
                    oplen = (uintN) jmplen;
                    continue;
                }

                



                --pcdepth;
            }
        }

        type = cs->format & JOF_TYPEMASK;
        switch (type) {
          case JOF_TABLESWITCH:
          case JOF_TABLESWITCHX:
          {
            jsint jmplen, i, low, high;
            jsbytecode *pc2;

            jmplen = (type == JOF_TABLESWITCH) ? JUMP_OFFSET_LEN
                                               : JUMPX_OFFSET_LEN;
            pc2 = pc;
            pc2 += jmplen;
            low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            high = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            for (i = low; i <= high; i++)
                pc2 += jmplen;
            oplen = 1 + pc2 - pc;
            break;
          }

          case JOF_LOOKUPSWITCH:
          case JOF_LOOKUPSWITCHX:
          {
            jsint jmplen;
            jsbytecode *pc2;
            jsatomid npairs;

            jmplen = (type == JOF_LOOKUPSWITCH) ? JUMP_OFFSET_LEN
                                                : JUMPX_OFFSET_LEN;
            pc2 = pc;
            pc2 += jmplen;
            npairs = GET_UINT16(pc2);
            pc2 += INDEX_LEN;
            while (npairs) {
                pc2 += INDEX_LEN;
                pc2 += jmplen;
                npairs--;
            }
            oplen = 1 + pc2 - pc;
            break;
          }

          default:;
        }

        if (sn && SN_TYPE(sn) == SRC_HIDDEN)
            continue;

        nuses = cs->nuses;
        if (nuses < 0) {
            
            nuses = 2 + GET_ARGC(pc);
        } else if (op == JSOP_RETSUB) {
            
            JS_ASSERT(nuses == 0);
            nuses = 2;
        } else if (op == JSOP_LEAVEBLOCK || op == JSOP_LEAVEBLOCKEXPR) {
            JS_ASSERT(nuses == 0);
            nuses = GET_UINT16(pc);
        }
        pcdepth -= nuses;
        JS_ASSERT(pcdepth >= 0);

        ndefs = cs->ndefs;
        if (op == JSOP_FINALLY) {
            
            JS_ASSERT(ndefs == 0);
            ndefs = 2;
        } else if (op == JSOP_ENTERBLOCK) {
            JSObject *obj;

            JS_ASSERT(ndefs == 0);
            GET_OBJECT_FROM_BYTECODE(script, pc, 0, obj);
            JS_ASSERT(OBJ_BLOCK_DEPTH(cx, obj) == pcdepth);
            ndefs = OBJ_BLOCK_COUNT(cx, obj);
        }
        pcdepth += ndefs;
    }

    name = NULL;
    jp = JS_NEW_PRINTER(cx, "js_DecompileValueGenerator", 0, JS_FALSE);
    if (jp) {
        if (fp->fun && fp->fun->object) {
            JS_ASSERT(OBJ_IS_NATIVE(fp->fun->object));
            jp->object = fp->fun->object;
        }
        jp->dvgfence = end;
        if (js_DecompileCode(jp, script, begin, (uintN)len, (uintN)pcdepth)) {
            name = (jp->sprinter.base) ? jp->sprinter.base : (char *) "";
            name = JS_strdup(cx, name);
        }
        js_DestroyPrinter(jp);
    }
    return name;

  do_fallback:
    if (!fallback) {
        fallback = js_ValueToSource(cx, v);
        if (!fallback)
            return NULL;
    }
    return js_DeflateString(cx, JSSTRING_CHARS(fallback),
                            JSSTRING_LENGTH(fallback));
}
