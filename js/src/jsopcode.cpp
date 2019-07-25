










































#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsfun.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"

#include "ds/Sort.h"

#include "frontend/BytecodeEmitter.h"
#include "frontend/TokenStream.h"
#include "vm/Debugger.h"

#include "jscntxtinlines.h"
#include "jsobjinlines.h"
#include "jsopcodeinlines.h"
#include "jsscriptinlines.h"

#include "jsautooplen.h"

#include "vm/RegExpObject-inl.h"

using namespace mozilla;
using namespace js;
using namespace js::gc;




JS_STATIC_ASSERT(sizeof(uint32_t) * JS_BITS_PER_BYTE >= INDEX_LIMIT_LOG2 + 1);


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





const char *js_CodeName[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    name,
#include "jsopcode.tbl"
#undef OPDEF
};



#define COUNTS_LEN 16

typedef Vector<char, 8> DupBuffer;

static bool
Dup(const char *chars, DupBuffer *cb)
{
    return cb->append(chars, strlen(chars) + 1);
}

static ptrdiff_t
GetJumpOffset(jsbytecode *pc, jsbytecode *pc2)
{
    uint32_t type;

    type = JOF_OPTYPE(*pc);
    if (JOF_TYPE_IS_EXTENDED_JUMP(type))
        return GET_JUMPX_OFFSET(pc2);
    return GET_JUMP_OFFSET(pc2);
}

uintN
js_GetIndexFromBytecode(JSScript *script, jsbytecode *pc, ptrdiff_t pcoff)
{
    JSOp op = JSOp(*pc);
    JS_ASSERT(js_CodeSpec[op].length >= 1 + pcoff + UINT16_LEN);

    



    uintN span = js_CodeSpec[op].length;
    uintN base = 0;
    if (pc - script->code + span < script->length) {
        JSOp next = JSOp(pc[span]);
        if (next == JSOP_RESETBASE) {
            JS_ASSERT(JSOp(pc[-JSOP_INDEXBASE_LENGTH]) == JSOP_INDEXBASE);
            base = GET_INDEXBASE(pc - JSOP_INDEXBASE_LENGTH);
        } else if (next == JSOP_RESETBASE0) {
            JSOp prev = JSOp(pc[-1]);
            JS_ASSERT(JSOP_INDEXBASE1 <= prev && prev <= JSOP_INDEXBASE3);
            base = (prev - JSOP_INDEXBASE1 + 1) << 16;
        }
    }
    return base + GET_UINT16(pc + pcoff);
}

size_t
js_GetVariableBytecodeLength(jsbytecode *pc)
{
    uintN jmplen, ncases;
    jsint low, high;

    JSOp op = JSOp(*pc);
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

static uint32_t
NumBlockSlots(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(*pc == JSOP_ENTERBLOCK || *pc == JSOP_ENTERLET0 || *pc == JSOP_ENTERLET1);
    JS_STATIC_ASSERT(JSOP_ENTERBLOCK_LENGTH == JSOP_ENTERLET0_LENGTH);
    JS_STATIC_ASSERT(JSOP_ENTERBLOCK_LENGTH == JSOP_ENTERLET1_LENGTH);

    JSObject *obj = NULL;
    GET_OBJECT_FROM_BYTECODE(script, pc, 0, obj);
    return obj->asStaticBlock().slotCount();
}

uintN
js::StackUses(JSScript *script, jsbytecode *pc)
{
    JSOp op = (JSOp) *pc;
    const JSCodeSpec &cs = js_CodeSpec[op];
    if (cs.nuses >= 0)
        return cs.nuses;

    JS_ASSERT(js_CodeSpec[op].nuses == -1);
    switch (op) {
      case JSOP_POPN:
        return GET_UINT16(pc);
      case JSOP_LEAVEBLOCK:
        return GET_UINT16(pc);
      case JSOP_LEAVEBLOCKEXPR:
        return GET_UINT16(pc) + 1;
      case JSOP_ENTERLET0:
        return NumBlockSlots(script, pc);
      case JSOP_ENTERLET1:
        return NumBlockSlots(script, pc) + 1;
      default:
        
        JS_ASSERT(op == JSOP_NEW || op == JSOP_CALL || op == JSOP_EVAL ||
                  op == JSOP_FUNCALL || op == JSOP_FUNAPPLY);
        return 2 + GET_ARGC(pc);
    }
}

uintN
js::StackDefs(JSScript *script, jsbytecode *pc)
{
    JSOp op = (JSOp) *pc;
    const JSCodeSpec &cs = js_CodeSpec[op];
    if (cs.ndefs >= 0)
        return cs.ndefs;

    uint32_t n = NumBlockSlots(script, pc);
    return op == JSOP_ENTERLET1 ? n + 1 : n;
}

static const char * countBaseNames[] = {
    "interp",
    "mjit",
    "mjit_calls",
    "mjit_code",
    "mjit_pics"
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) == OpcodeCounts::BASE_COUNT);

static const char * countAccessNames[] = {
    "infer_mono",
    "infer_di",
    "infer_poly",
    "infer_barrier",
    "infer_nobarrier",
    "observe_undefined",
    "observe_null",
    "observe_boolean",
    "observe_int32",
    "observe_double",
    "observe_string",
    "observe_object"
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) +
                 JS_ARRAY_LENGTH(countAccessNames) == OpcodeCounts::ACCESS_COUNT);

static const char * countElementNames[] = {
    "id_int",
    "id_double",
    "id_other",
    "id_unknown",
    "elem_typed",
    "elem_packed",
    "elem_dense",
    "elem_other"
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) +
                 JS_ARRAY_LENGTH(countAccessNames) +
                 JS_ARRAY_LENGTH(countElementNames) == OpcodeCounts::ELEM_COUNT);

static const char * countPropertyNames[] = {
    "prop_static",
    "prop_definite",
    "prop_other"
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) +
                 JS_ARRAY_LENGTH(countAccessNames) +
                 JS_ARRAY_LENGTH(countPropertyNames) == OpcodeCounts::PROP_COUNT);

static const char * countArithNames[] = {
    "arith_int",
    "arith_double",
    "arith_other",
    "arith_unknown",
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) +
                 JS_ARRAY_LENGTH(countArithNames) == OpcodeCounts::ARITH_COUNT);

 const char *
OpcodeCounts::countName(JSOp op, size_t which)
{
    JS_ASSERT(which < numCounts(op));

    if (which < BASE_COUNT)
        return countBaseNames[which];

    if (accessOp(op)) {
        if (which < ACCESS_COUNT)
            return countAccessNames[which - BASE_COUNT];
        if (elementOp(op))
            return countElementNames[which - ACCESS_COUNT];
        if (propertyOp(op))
            return countPropertyNames[which - ACCESS_COUNT];
        JS_NOT_REACHED("bad op");
        return NULL;
    }

    if (arithOp(op))
        return countArithNames[which - BASE_COUNT];

    JS_NOT_REACHED("bad op");
    return NULL;
}

#ifdef DEBUG

JS_FRIEND_API(void)
js_DumpPCCounts(JSContext *cx, JSScript *script, js::Sprinter *sp)
{
    JS_ASSERT(script->pcCounters);

    jsbytecode *pc = script->code;
    while (pc < script->code + script->length) {
        JSOp op = JSOp(*pc);

        int len = js_CodeSpec[op].length;
        jsbytecode *next = (len != -1) ? pc + len : pc + js_GetVariableBytecodeLength(pc);

        if (!js_Disassemble1(cx, script, pc, pc - script->code, true, sp))
            return;

        size_t total = OpcodeCounts::numCounts(op);
        double *raw = script->getCounts(pc).rawCounts();

        Sprint(sp, "                  {");
        bool printed = false;
        for (size_t i = 0; i < total; i++) {
            double val = raw[i];
            if (val) {
                if (printed)
                    Sprint(sp, ", ");
                Sprint(sp, "\"%s\": %.0f", OpcodeCounts::countName(op, i), val);
                printed = true;
            }
        }
        Sprint(sp, "}\n");

        pc = next;
    }
}





JS_FRIEND_API(JSBool)
js_DisassembleAtPC(JSContext *cx, JSScript *script, JSBool lines, jsbytecode *pc, Sprinter *sp)
{
    jsbytecode *next, *end;
    uintN len;

    SprintCString(sp, "loc   ");
    if (lines)
        SprintCString(sp, "line");
    SprintCString(sp, "  op\n");
    SprintCString(sp, "----- ");
    if (lines)
        SprintCString(sp, "----");
    SprintCString(sp, "  --\n");

    next = script->code;
    end = next + script->length;
    while (next < end) {
        if (next == script->main())
            SprintCString(sp, "main:\n");
        if (pc != NULL) {
            if (pc == next)
                SprintCString(sp, "--> ");
            else
                SprintCString(sp, "    ");
        }
        len = js_Disassemble1(cx, script, next, next - script->code, lines, sp);
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
    LifoAllocScope las(&cx->tempLifoAlloc());
    Sprinter sprinter;
    INIT_SPRINTER(cx, &sprinter, &cx->tempLifoAlloc(), 0);
    JSBool ok = js_DisassembleAtPC(cx, cx->fp()->script(), true, cx->regs().pc, &sprinter);
    fprintf(stdout, "%s", sprinter.base);
    return ok;
}

JSBool
js_DumpScript(JSContext *cx, JSScript *script)
{
    LifoAllocScope las(&cx->tempLifoAlloc());
    Sprinter sprinter;
    INIT_SPRINTER(cx, &sprinter, &cx->tempLifoAlloc(), 0);
    JSBool ok = js_Disassemble(cx, script, true, &sprinter);
    fprintf(stdout, "%s", sprinter.base);
    return ok;
}

static char *
QuoteString(Sprinter *sp, JSString *str, uint32_t quote);

static bool
ToDisassemblySource(JSContext *cx, jsval v, JSAutoByteString *bytes)
{
    if (JSVAL_IS_STRING(v)) {
        Sprinter sprinter;
        LifoAlloc &tla = cx->tempLifoAlloc();
        LifoAllocScope las(&tla);
        INIT_SPRINTER(cx, &sprinter, &tla, 0);
        char *nbytes = QuoteString(&sprinter, JSVAL_TO_STRING(v), '"');
        if (!nbytes)
            return false;
        nbytes = JS_sprintf_append(NULL, "%s", nbytes);
        if (!nbytes)
            return false;
        bytes->initBytes(nbytes);
        return true;
    }

    if (cx->runtime->gcRunning || JS_THREAD_DATA(cx)->noGCOrAllocationCheck) {
        char *source = JS_sprintf_append(NULL, "<value>");
        if (!source)
            return false;
        bytes->initBytes(source);
        return true;
    }

    if (!JSVAL_IS_PRIMITIVE(v)) {
        JSObject *obj = JSVAL_TO_OBJECT(v);
        if (obj->isBlock()) {
            char *source = JS_sprintf_append(NULL, "depth %d {", obj->asBlock().stackDepth());
            if (!source)
                return false;

            Shape::Range r = obj->lastProperty()->all();
            while (!r.empty()) {
                const Shape &shape = r.front();
                JSAtom *atom = JSID_IS_INT(shape.propid())
                               ? cx->runtime->atomState.emptyAtom
                               : JSID_TO_ATOM(shape.propid());

                JSAutoByteString bytes;
                if (!js_AtomToPrintableString(cx, atom, &bytes))
                    return false;

                r.popFront();
                source = JS_sprintf_append(source, "%s: %d%s",
                                           bytes.ptr(), shape.shortid(),
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

        if (obj->isFunction()) {
            JSString *str = JS_DecompileFunction(cx, obj->toFunction(), JS_DONT_PRETTY_PRINT);
            if (!str)
                return false;
            return bytes->encode(cx, str);
        }

        if (obj->isRegExp()) {
            JSString *source = obj->asRegExp().toString(cx);
            if (!source)
                return false;
            JS::Anchor<JSString *> anchor(source);
            return bytes->encode(cx, source);
        }
    }

    return !!js_ValueToPrintable(cx, v, bytes, true);
}

JS_FRIEND_API(uintN)
js_Disassemble1(JSContext *cx, JSScript *script, jsbytecode *pc,
                uintN loc, JSBool lines, Sprinter *sp)
{
    JSOp op = (JSOp)*pc;
    if (op >= JSOP_LIMIT) {
        char numBuf1[12], numBuf2[12];
        JS_snprintf(numBuf1, sizeof numBuf1, "%d", op);
        JS_snprintf(numBuf2, sizeof numBuf2, "%d", JSOP_LIMIT);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BYTECODE_TOO_BIG, numBuf1, numBuf2);
        return 0;
    }
    const JSCodeSpec *cs = &js_CodeSpec[op];
    ptrdiff_t len = (ptrdiff_t) cs->length;
    Sprint(sp, "%05u:", loc);
    if (lines)
        Sprint(sp, "%4u", JS_PCToLineNumber(cx, script, pc));
    Sprint(sp, "  %s", js_CodeName[op]);

    switch (uint32_t type = JOF_TYPE(cs->format)) {
      case JOF_BYTE:
          
          
          
          
          if (op == JSOP_TRY) {
              JSTryNoteArray *trynotes = script->trynotes();
              uint32_t i;
              for(i = 0; i < trynotes->length; i++) {
                  JSTryNote note = trynotes->vector[i];
                  if (note.kind == JSTRY_CATCH && note.start == loc + 1) {
                      Sprint(sp, " %u (%+d)",
                             (unsigned int) (loc+note.length+1),
                             (int) (note.length+1));
                      break;
                  }
              }
          }
        break;

      case JOF_JUMP:
      case JOF_JUMPX: {
        ptrdiff_t off = GetJumpOffset(pc, pc);
        Sprint(sp, " %u (%+d)", loc + (intN) off, (intN) off);
        break;
      }

      case JOF_ATOM:
      case JOF_OBJECT:
      case JOF_REGEXP: {
        uintN index = js_GetIndexFromBytecode(script, pc, 0);
        jsval v;
        if (type == JOF_ATOM) {
            if (op == JSOP_DOUBLE) {
                v = script->getConst(index);
            } else {
                JSAtom *atom = script->getAtom(index);
                v = STRING_TO_JSVAL(atom);
            }
        } else {
            JSObject *obj;
            if (type == JOF_OBJECT) {
                
                if (cx->compartment->activeAnalysis) {
                    Sprint(sp, " object");
                    break;
                }
                obj = script->getObject(index);
            } else {
                obj = script->getRegExp(index);
            }
            v = OBJECT_TO_JSVAL(obj);
        }
        {
            JSAutoByteString bytes;
            if (!ToDisassemblySource(cx, v, &bytes))
                return 0;
            Sprint(sp, " %s", bytes.ptr());
        }
        break;
      }

      case JOF_TABLESWITCH:
      case JOF_TABLESWITCHX:
      {
        jsbytecode *pc2;
        jsint i, low, high;

        ptrdiff_t jmplen = (type == JOF_TABLESWITCH) ? JUMP_OFFSET_LEN : JUMPX_OFFSET_LEN;
        pc2 = pc;
        ptrdiff_t off = GetJumpOffset(pc, pc2);
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

        ptrdiff_t jmplen = (type == JOF_LOOKUPSWITCH) ? JUMP_OFFSET_LEN : JUMPX_OFFSET_LEN;
        pc2 = pc;
        ptrdiff_t off = GetJumpOffset(pc, pc2);
        pc2 += jmplen;
        npairs = GET_UINT16(pc2);
        pc2 += UINT16_LEN;
        Sprint(sp, " offset %d npairs %u", intN(off), uintN(npairs));
        while (npairs) {
            uint16_t constIndex = GET_INDEX(pc2);
            pc2 += INDEX_LEN;
            off = GetJumpOffset(pc, pc2);
            pc2 += jmplen;

            JSAutoByteString bytes;
            if (!ToDisassemblySource(cx, script->getConst(constIndex), &bytes))
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
        uintN index = js_GetIndexFromBytecode(script, pc, SLOTNO_LEN);
        jsval v;
        if (type == JOF_SLOTATOM) {
            JSAtom *atom = script->getAtom(index);
            v = STRING_TO_JSVAL(atom);
        } else {
            v = OBJECT_TO_JSVAL(script->getObject(index));
        }

        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, v, &bytes))
            return 0;
        Sprint(sp, " %s", bytes.ptr());
        break;
      }

      {
        int i;

      case JOF_UINT16PAIR:
        i = (jsint)GET_UINT16(pc);
        Sprint(sp, " %d", i);
        pc += UINT16_LEN;
        

      case JOF_UINT16:
        i = (jsint)GET_UINT16(pc);
        goto print_int;

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
      }

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
    if (sp->offset + len < sp->size)
        return JS_TRUE;

    ptrdiff_t nb = Max(Max((size_t)1024, sp->size * 2), sp->offset + len + 1);
    char *base = sp->base;
    if (!base)
        base = static_cast<char *>(sp->pool->allocUnaligned(nb));
    else
        base = static_cast<char *>(sp->pool->reallocUnaligned(base, sp->size, nb - sp->size));
    if (!base) {
        js_ReportOutOfMemory(sp->context);
        return JS_FALSE;
    }
    sp->base = base;
    sp->size = nb;
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

    size_t size = GetDeflatedStringLength(sp->context, chars, length);
    if (size == (size_t)-1 || !SprintEnsureBuffer(sp, size))
        return -1;

    ptrdiff_t offset = sp->offset;
    sp->offset += size;
    DeflateStringToBuffer(sp->context, chars, length, sp->base + offset, &size);
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
    '\0'
};

#define DONT_ESCAPE     0x10000

static char *
QuoteString(Sprinter *sp, JSString *str, uint32_t quote)
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
        while (c < 127 && isprint(c) && c != qc && c != '\\' && c != '\t') {
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
        if (!(c >> 8) && c != 0 && (e = strchr(js_EscapeMap, (int)c)) != NULL) {
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
    LifoAllocScope las(&cx->tempLifoAlloc());
    Sprinter sprinter;
    INIT_SPRINTER(cx, &sprinter, &cx->tempLifoAlloc(), 0);
    char *bytes = QuoteString(&sprinter, str, quote);
    JSString *escstr = bytes ? JS_NewStringCopyZ(cx, bytes) : NULL;
    return escstr;
}








struct DecompiledOpcode
{
    
    const char *text;

    
    jsbytecode *parent;

    




    int32_t parentOffset;

    



    bool parenthesized;

    DecompiledOpcode()
        : text(NULL), parent(NULL), parentOffset(-1), parenthesized(false)
    {}
};

struct JSPrinter
{
    Sprinter        sprinter;       
    LifoAlloc       pool;           
    uintN           indent;         
    bool            pretty;         
    bool            grouped;        
    bool            strict;         
    JSScript        *script;        
    jsbytecode      *dvgfence;      
    jsbytecode      **pcstack;      
    JSFunction      *fun;           
    Vector<JSAtom *> *localNames;   
    Vector<DecompiledOpcode> *decompiledOpcodes; 

    DecompiledOpcode &decompiled(jsbytecode *pc) {
        JS_ASSERT(decompiledOpcodes);
        return (*decompiledOpcodes)[pc - script->code];
    }
};

JSPrinter *
js_NewPrinter(JSContext *cx, const char *name, JSFunction *fun,
              uintN indent, JSBool pretty, JSBool grouped, JSBool strict)
{
    JSPrinter *jp = (JSPrinter *) cx->malloc_(sizeof(JSPrinter));
    if (!jp)
        return NULL;
    INIT_SPRINTER(cx, &jp->sprinter, &jp->pool, 0);
    new (&jp->pool) LifoAlloc(1024);
    jp->indent = indent;
    jp->pretty = !!pretty;
    jp->grouped = !!grouped;
    jp->strict = !!strict;
    jp->script = NULL;
    jp->dvgfence = NULL;
    jp->pcstack = NULL;
    jp->fun = fun;
    jp->localNames = NULL;
    jp->decompiledOpcodes = NULL;
    if (fun && fun->isInterpreted() && fun->script()->bindings.hasLocalNames()) {
        jp->localNames = cx->new_<Vector<JSAtom *> >(cx);
        if (!jp->localNames || !fun->script()->bindings.getLocalNameArray(cx, jp->localNames)) {
            js_DestroyPrinter(jp);
            return NULL;
        }
    }
    return jp;
}

void
js_DestroyPrinter(JSPrinter *jp)
{
    jp->pool.freeAll();
    Foreground::delete_(jp->localNames);
    jp->sprinter.context->free_(jp);
}

JSString *
js_GetPrinterOutput(JSPrinter *jp)
{
    JSContext *cx = jp->sprinter.context;
    if (!jp->sprinter.base)
        return cx->runtime->emptyString;
    return JS_NewStringCopyZ(cx, jp->sprinter.base);
}


static inline void
UpdateDecompiledParent(JSPrinter *jp, jsbytecode *pc, jsbytecode *parent, size_t offset)
{
    if (jp->decompiledOpcodes && pc) {
        jp->decompiled(pc).parent = parent;
        jp->decompiled(pc).parentOffset = offset;
    }
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
        jp->sprinter.context->free_(fp);
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
    jp->sprinter.context->free_(bp);

    va_end(ap);
    return cc;
}

JSBool
js_puts(JSPrinter *jp, const char *s)
{
    return SprintCString(&jp->sprinter, s) >= 0;
}



struct SprintStack
{
    Sprinter    sprinter;       
    ptrdiff_t   *offsets;       
    jsbytecode  *opcodes;       
    jsbytecode  **bytecodes;    
    uintN       top;            
    uintN       inArrayInit;    
    JSBool      inGenExp;       
    JSPrinter   *printer;       
};





static inline bool
UpdateDecompiledText(SprintStack *ss, jsbytecode *pc, ptrdiff_t todo)
{
    JSPrinter *jp = ss->printer;

    if (jp->decompiledOpcodes && jp->decompiled(pc).text == NULL) {
        const char *text = OFF2STR(&ss->sprinter, todo);
        size_t len = strlen(text) + 1;

        const char *ntext = ss->printer->pool.newArrayUninitialized<char>(len);
        if (!ntext) {
            js_ReportOutOfMemory(ss->sprinter.context);
            return false;
        }

        memcpy((char *) ntext, text, len);
        jp->decompiled(pc).text = ntext;
    }

    return true;
}

static inline const char *
SprintDupeStr(SprintStack *ss, const char *str)
{
    size_t len = strlen(str) + 1;

    const char *nstr = ss->printer->pool.newArrayUninitialized<char>(len);
    if (nstr) {
        memcpy((char *) nstr, str, len);
    } else {
        js_ReportOutOfMemory(ss->sprinter.context);
        nstr = "";
    }

    return nstr;
}


static inline void
SprintOpcodePermanent(JSPrinter *jp, const char *str, jsbytecode *pc)
{
    UpdateDecompiledParent(jp, pc, NULL, jp->sprinter.offset);
    js_printf(jp, "%s", str);
}






static inline void
SprintOpcode(SprintStack *ss, const char *str, jsbytecode *pc,
             jsbytecode *parentpc, ptrdiff_t startOffset)
{
    if (startOffset < 0) {
        JS_ASSERT(ss->sprinter.context->isExceptionPending());
        return;
    }
    UpdateDecompiledParent(ss->printer, pc, parentpc, ss->sprinter.offset - startOffset);
    SprintCString(&ss->sprinter, str);
}





static inline void
CopyDecompiledTextForDecomposedOp(JSPrinter *jp, jsbytecode *pc)
{
    JS_ASSERT(js_CodeSpec[*pc].format & JOF_DECOMPOSE);

    if (jp->decompiledOpcodes) {
        size_t len = GetDecomposeLength(pc, js_CodeSpec[*pc].length);

        const char *text = jp->decompiled(pc).text;

        jsbytecode *pc2 = pc + GetBytecodeLength(pc);
        for (; pc2 < pc + len; pc2 += GetBytecodeLength(pc2)) {
            jp->decompiled(pc2).text = text;
            jp->decompiled(pc2).parent = pc;
            jp->decompiled(pc2).parentOffset = 0;
        }
    }
}












static intN
ReconstructPCStack(JSContext *cx, JSScript *script, jsbytecode *pc,
                   jsbytecode **pcstack, jsbytecode **lastDecomposedPC);

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
            ss->sprinter.context->free_(bytes);
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
PushOff(SprintStack *ss, ptrdiff_t off, JSOp op, jsbytecode *pc = NULL)
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
    ss->bytecodes[top] = pc;
    ss->top = ++top;
    AddParenSlop(ss);
    return JS_TRUE;
}

static bool
PushStr(SprintStack *ss, const char *str, JSOp op)
{
    ptrdiff_t off = SprintCString(&ss->sprinter, str);
    if (off < 0)
        return false;
    return PushOff(ss, off, op);
}

static ptrdiff_t
PopOffPrec(SprintStack *ss, uint8_t prec, jsbytecode **ppc = NULL)
{
    uintN top;
    const JSCodeSpec *topcs;
    ptrdiff_t off;

    if (ppc)
        *ppc = NULL;

    
    top = ss->top;
    JS_ASSERT(top != 0);
    if (top == 0)
        return 0;

    ss->top = --top;
    off = GetOff(ss, top);
    topcs = &js_CodeSpec[ss->opcodes[top]];

    jsbytecode *pc = ss->bytecodes[top];
    if (ppc)
        *ppc = pc;

    if (topcs->prec != 0 && topcs->prec < prec) {
        ss->sprinter.offset = ss->offsets[top] = off - 2;
        off = Sprint(&ss->sprinter, "(%s)", OFF2STR(&ss->sprinter, off));
        if (ss->printer->decompiledOpcodes && pc)
            ss->printer->decompiled(pc).parenthesized = true;
    } else {
        ss->sprinter.offset = off;
    }
    return off;
}

static const char *
PopStrPrec(SprintStack *ss, uint8_t prec, jsbytecode **ppc = NULL)
{
    ptrdiff_t off;

    off = PopOffPrec(ss, prec, ppc);
    return OFF2STR(&ss->sprinter, off);
}






static const char *
PopStrPrecDupe(SprintStack *ss, uint8_t prec, jsbytecode **ppc = NULL)
{
    const char *str = PopStrPrec(ss, prec, ppc);
    return SprintDupeStr(ss, str);
}

static ptrdiff_t
PopOff(SprintStack *ss, JSOp op, jsbytecode **ppc = NULL)
{
    return PopOffPrec(ss, js_CodeSpec[op].prec, ppc);
}

static const char *
PopStr(SprintStack *ss, JSOp op, jsbytecode **ppc = NULL)
{
    return PopStrPrec(ss, js_CodeSpec[op].prec, ppc);
}

static const char *
PopStrDupe(SprintStack *ss, JSOp op, jsbytecode **ppc = NULL)
{
    return PopStrPrecDupe(ss, js_CodeSpec[op].prec, ppc);
}





static const char *
PopCondStr(SprintStack *ss, jsbytecode **ppc = NULL)
{
    JSOp op = (js_CodeSpec[ss->opcodes[ss->top - 1]].format & JOF_SET)
              ? JSOP_IFEQ
              : JSOP_NOP;
    return PopStr(ss, op, ppc);
}

static inline bool
IsInitializerOp(unsigned char op)
{
    return op == JSOP_NEWINIT || op == JSOP_NEWARRAY || op == JSOP_NEWOBJECT;
}

struct TableEntry {
    jsval       key;
    ptrdiff_t   offset;
    JSAtom      *label;
    jsint       order;          
};

inline bool
CompareTableEntries(const TableEntry &a, const TableEntry &b, bool *lessOrEqualp)
{
    *lessOrEqualp = (a.offset != b.offset) ? a.offset <= b.offset : a.order <= b.order;
    return true;
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
Decompile(SprintStack *ss, jsbytecode *pc, intN nb);

static JSBool
DecompileSwitch(SprintStack *ss, TableEntry *table, uintN tableLength,
                jsbytecode *pc, ptrdiff_t switchLength,
                ptrdiff_t defaultOffset, JSBool isCondSwitch)
{
    JSContext *cx;
    JSPrinter *jp;
    ptrdiff_t off, off2, diff, caseExprOff, todo;
    char *rval;
    uintN i;
    jsval key;
    JSString *str;

    cx = ss->sprinter.context;
    jp = ss->printer;

    jsbytecode *lvalpc;
    const char *lval = PopStr(ss, JSOP_NOP, &lvalpc);

    
    if (isCondSwitch)
        ss->top++;

    js_printf(jp, "\tswitch (");
    SprintOpcodePermanent(jp, lval, lvalpc);
    js_printf(jp, ") {\n");

    if (tableLength) {
        diff = table[0].offset - defaultOffset;
        if (diff > 0) {
            jp->indent += 2;
            js_printf(jp, "\t%s:\n", js_default_str);
            jp->indent += 2;
            if (!Decompile(ss, pc + defaultOffset, diff))
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
                if (!Decompile(ss, pc + caseExprOff, nextCaseExprOff - caseExprOff))
                    return JS_FALSE;
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
                    if (todo < 0)
                        return JS_FALSE;
                    str = NULL;
                } else {
                    str = ToString(cx, key);
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
                    if (!Decompile(ss, pc + off, diff))
                        return JS_FALSE;
                    off = defaultOffset;
                }
                jp->indent -= 2;
                js_printf(jp, "\t%s:\n", js_default_str);
                jp->indent += 2;
            }
            if (!Decompile(ss, pc + off, off2 - off))
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
    LOCAL_ASSERT_RV(jp->fun, NULL);
    LOCAL_ASSERT_RV(slot < jp->fun->script()->bindings.countLocalNames(), NULL);
    JSAtom *name = (*jp->localNames)[slot];
#if !JS_HAS_DESTRUCTURING
    LOCAL_ASSERT_RV(name, NULL);
#endif
    return name;
}

#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_RV(expr, "")

static const char *
GetLocalInSlot(SprintStack *ss, jsint i, jsint slot, JSObject *obj)
{
    for (Shape::Range r(obj->lastProperty()); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();

        if (shape.shortid() == slot) {
            
            if (!JSID_IS_ATOM(shape.propid()))
                continue;

            JSAtom *atom = JSID_TO_ATOM(shape.propid());
            const char *rval = QuoteString(&ss->sprinter, atom, 0);
            if (!rval)
                return NULL;

            RETRACT(&ss->sprinter, rval);
            return rval;
        }
    }

    return GetStr(ss, i);
}

const char *
GetLocal(SprintStack *ss, jsint i)
{
    ptrdiff_t off = ss->offsets[i];
    if (off >= 0)
        return OFF2STR(&ss->sprinter, off);

    









    JSScript *script = ss->printer->script;
    if (!JSScript::isValidOffset(script->objectsOffset))
        return GetStr(ss, i);

    
    
    
    if (off <= -2 && ss->printer->pcstack) {
        jsbytecode *pc = ss->printer->pcstack[-2 - off];

        JS_ASSERT(ss->printer->script->code <= pc);
        JS_ASSERT(pc < (ss->printer->script->code + ss->printer->script->length));

        if (JSOP_ENTERBLOCK == (JSOp)*pc) {
            jsatomid j = js_GetIndexFromBytecode(ss->printer->script, pc, 0);
            JSObject *obj = script->getObject(j);

            if (obj->isBlock()) {
                uint32_t depth = obj->asBlock().stackDepth();
                uint32_t count = obj->asBlock().slotCount();
                if (jsuint(i - depth) < jsuint(count))
                    return GetLocalInSlot(ss, i, jsint(i - depth), obj);
            }
        }
    }

    
    for (jsatomid j = 0, n = script->objects()->length; j != n; j++) {
        JSObject *obj = script->getObject(j);

        if (obj->isBlock()) {
            uint32_t depth = obj->asBlock().stackDepth();
            uint32_t count = obj->asBlock().slotCount();
            if (jsuint(i - depth) < jsuint(count))
                return GetLocalInSlot(ss, i, jsint(i - depth), obj);
        }
    }

    return GetStr(ss, i);
}

#undef LOCAL_ASSERT

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

typedef Vector<JSAtom *, 8> AtomVector;
typedef AtomVector::Range AtomRange;

#if JS_HAS_DESTRUCTURING

#define LOCAL_ASSERT(expr)  LOCAL_ASSERT_RV(expr, NULL)
#define LOAD_OP_DATA(pc)    (oplen = (cs = &js_CodeSpec[op=(JSOp)*pc])->length)

static jsbytecode *
DecompileDestructuring(SprintStack *ss, jsbytecode *pc, jsbytecode *endpc,
                       AtomRange *letNames = NULL);












static jsbytecode *
DecompileDestructuringLHS(SprintStack *ss, jsbytecode *pc, jsbytecode *endpc, JSBool *hole,
                          AtomRange *letNames = NULL)
{
    JSPrinter *jp;
    JSOp op;
    const JSCodeSpec *cs;
    uintN oplen;
    jsint i;
    const char *lval, *xval;
    JSAtom *atom;

    *hole = JS_FALSE;
    jp = ss->printer;
    LOAD_OP_DATA(pc);

    switch (op) {
      case JSOP_POP:
        *hole = JS_TRUE;
        if (SprintPut(&ss->sprinter, ", ", 2) < 0)
            return NULL;
        break;

      case JSOP_PICK:
        

















        LOCAL_ASSERT(letNames && !letNames->empty());
        if (!QuoteString(&ss->sprinter, letNames->popCopyFront(), 0))
            return NULL;
        break;

      case JSOP_DUP:
      {
        
        pc = DecompileDestructuring(ss, pc, endpc, letNames);
        if (!pc)
            return NULL;
        if (pc == endpc)
            return pc;
        LOAD_OP_DATA(pc);

        




        lval = PopStr(ss, JSOP_NOP);
        if (SprintCString(&ss->sprinter, lval) < 0)
            return NULL;

        LOCAL_ASSERT(*pc == JSOP_POP);

        





        jsbytecode *nextpc = pc + JSOP_POP_LENGTH;
        LOCAL_ASSERT(nextpc <= endpc);
        if (letNames && *nextpc == JSOP_PICK) {
            LOCAL_ASSERT(nextpc < endpc);
            pc = nextpc;
            LOAD_OP_DATA(pc);
        }
        break;
      }

      case JSOP_SETARG:
      case JSOP_SETLOCAL:
        LOCAL_ASSERT(!letNames);
        LOCAL_ASSERT(pc[oplen] == JSOP_POP || pc[oplen] == JSOP_POPN);
        
      case JSOP_SETLOCALPOP:
        LOCAL_ASSERT(!letNames);
        if (op == JSOP_SETARG) {
            atom = GetArgOrVarAtom(jp, GET_SLOTNO(pc));
            LOCAL_ASSERT(atom);
            if (!QuoteString(&ss->sprinter, atom, 0))
                return NULL;
        } else if (IsVarSlot(jp, pc, &i)) {
            atom = GetArgOrVarAtom(jp, i);
            LOCAL_ASSERT(atom);
            if (!QuoteString(&ss->sprinter, atom, 0))
                return NULL;
        } else {
            lval = GetLocal(ss, i);
            if (!lval || SprintCString(&ss->sprinter, lval) < 0)
                return NULL;
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

      default: {
        LOCAL_ASSERT(!letNames);
        






        ptrdiff_t todo = ss->sprinter.offset;
        ss->sprinter.offset = todo + PAREN_SLOP;
        pc = Decompile(ss, pc, -((intN)ss->top));
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
        if (todo < 0)
            return NULL;
        break;
      }
    }

    LOCAL_ASSERT(pc < endpc);
    pc += oplen;
    return pc;
}









static jsbytecode *
DecompileDestructuring(SprintStack *ss, jsbytecode *pc, jsbytecode *endpc,
                       AtomRange *letNames)
{
    LOCAL_ASSERT(*pc == JSOP_DUP);
    pc += JSOP_DUP_LENGTH;

    JSContext *cx = ss->sprinter.context;
    JSPrinter *jp = ss->printer;
    jsbytecode *startpc = pc;

    




    ptrdiff_t head = SprintPut(&ss->sprinter, "[", 1);
    if (head < 0 || !PushOff(ss, head, JSOP_NOP))
        return NULL;
    ss->sprinter.offset -= PAREN_SLOP;
    LOCAL_ASSERT(head == ss->sprinter.offset - 1);
    LOCAL_ASSERT(*OFF2STR(&ss->sprinter, head) == '[');

    int lasti = -1;

    while (pc < endpc) {
#if JS_HAS_DESTRUCTURING_SHORTHAND
        ptrdiff_t nameoff = -1;
#endif

        const JSCodeSpec *cs;
        uintN oplen;
        JSOp op;
        LOAD_OP_DATA(pc);

        int i;
        double d;
        switch (op) {
          case JSOP_POP:
            
            LOCAL_ASSERT(startpc == pc);
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
          {
            jssrcnote *sn = js_GetSrcNote(jp->script, pc);
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
          }

          case JSOP_GETPROP:
          case JSOP_LENGTH:
          {
            JSAtom *atom;
            LOAD_ATOM(0);
            *OFF2STR(&ss->sprinter, head) = '{';
#if JS_HAS_DESTRUCTURING_SHORTHAND
            nameoff = ss->sprinter.offset;
#endif
            if (!QuoteString(&ss->sprinter, atom, IsIdentifier(atom) ? 0 : (jschar)'\''))
                return NULL;
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

        




        JSBool hole;
        pc = DecompileDestructuringLHS(ss, pc, endpc, &hole, letNames);
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

        







        jssrcnote *sn = js_GetSrcNote(jp->script, pc);
        if (!sn)
            break;
        if (SN_TYPE(sn) != SRC_CONTINUE) {
            LOCAL_ASSERT(SN_TYPE(sn) == SRC_DESTRUCT || SN_TYPE(sn) == SRC_DESTRUCTLET);
            break;
        }

        if (!hole && SprintPut(&ss->sprinter, ", ", 2) < 0)
            return NULL;

        pc += JSOP_DUP_LENGTH;
    }

out:
    const char *lval = OFF2STR(&ss->sprinter, head);
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
    LOCAL_ASSERT(op == JSOP_GETLOCAL);

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
        if (op != JSOP_GETLOCAL)
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

#define LOCAL_ASSERT(expr)    LOCAL_ASSERT_RV(expr, false)







static bool
GetBlockNames(JSContext *cx, StaticBlockObject &blockObj, AtomVector *atoms)
{
    size_t numAtoms = blockObj.slotCount();
    LOCAL_ASSERT(numAtoms > 0);
    if (!atoms->resize(numAtoms))
        return false;

    uintN i = numAtoms;
    for (Shape::Range r = blockObj.lastProperty()->all(); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();
        LOCAL_ASSERT(shape.hasShortID());
        --i;
        LOCAL_ASSERT((uintN)shape.shortid() == i);
        (*atoms)[i] = JSID_IS_INT(shape.propid())
                      ? cx->runtime->atomState.emptyAtom
                      : JSID_TO_ATOM(shape.propid());
    }

    LOCAL_ASSERT(i == 0);
    return true;
}

static bool
PushBlockNames(JSContext *cx, SprintStack *ss, const AtomVector &atoms)
{
    for (size_t i = 0; i < atoms.length(); i++) {
        const char *name = QuoteString(&ss->sprinter, atoms[i], 0);
        if (!name || !PushOff(ss, STR2OFF(&ss->sprinter, name), JSOP_ENTERBLOCK))
            return false;
    }
    return true;
}






static bool
AssignBlockNamesToPushedSlots(JSContext *cx, SprintStack *ss, const AtomVector &atoms)
{
    
    LOCAL_ASSERT(atoms.length() <= (uintN)ss->top);
    for (size_t i = 0; i < atoms.length(); ++i)
        PopStr(ss, JSOP_NOP);
    return PushBlockNames(cx, ss, atoms);
}

static const char SkipString[] = "/*skip*/";
static const char DestructuredString[] = "/*destructured*/";
static const unsigned DestructuredStringLength = ArrayLength(DestructuredString) - 1;

static ptrdiff_t
SprintLetBody(JSContext *cx, JSPrinter *jp, SprintStack *ss, jsbytecode *pc, ptrdiff_t bodyLength,
              const char *headChars)
{
    if (pc[bodyLength] == JSOP_LEAVEBLOCK) {
        js_printf(jp, "\tlet (%s) {\n", headChars);
        jp->indent += 4;
        if (!Decompile(ss, pc, bodyLength))
            return -1;
        jp->indent -= 4;
        js_printf(jp, "\t}\n");
        return -2;
    }

    LOCAL_ASSERT_RV(pc[bodyLength] == JSOP_LEAVEBLOCKEXPR, -1);
    if (!Decompile(ss, pc, bodyLength))
        return -1;

    const char *bodyChars = PopStr(ss, JSOP_SETNAME);
    const char *format = *bodyChars == '{' ? "let (%s) (%s)" : "let (%s) %s";
    return Sprint(&ss->sprinter, format, headChars, bodyChars);
}










static const char *
GetTokenForAssignment(JSPrinter *jp, jssrcnote *sn, JSOp lastop,
                      jsbytecode *pc, jsbytecode *rvalpc,
                      jsbytecode **lastlvalpc, jsbytecode **lastrvalpc)
{
    const char *token;
    if (sn && SN_TYPE(sn) == SRC_ASSIGNOP) {
        if (lastop == JSOP_GETTER) {
            token = js_getter_str;
        } else if (lastop == JSOP_SETTER) {
            token = js_setter_str;
        } else {
            token = CodeToken[lastop];
            if (*lastlvalpc && *lastrvalpc) {
                UpdateDecompiledParent(jp, *lastlvalpc, pc, 0);
                UpdateDecompiledParent(jp, *lastrvalpc, rvalpc, 0);
            }
        }
    } else {
        token = "";
    }
    *lastlvalpc = NULL;
    *lastrvalpc = NULL;
    return token;
}

static ptrdiff_t
SprintNormalFor(JSContext *cx, JSPrinter *jp, SprintStack *ss, const char *initPrefix,
                const char *init, jsbytecode *initpc, jsbytecode **ppc, ptrdiff_t *plen)
{
    jsbytecode *pc = *ppc;
    jssrcnote *sn = js_GetSrcNote(jp->script, pc);
    JS_ASSERT(SN_TYPE(sn) == SRC_FOR);

    
    js_printf(jp, "\tfor (%s", initPrefix);
    SprintOpcodePermanent(jp, init, initpc);
    js_printf(jp, ";");

    
    JS_ASSERT(*pc == JSOP_NOP || *pc == JSOP_POP);
    pc += JSOP_NOP_LENGTH;

    
    ptrdiff_t cond = js_GetSrcNoteOffset(sn, 0);
    ptrdiff_t next = js_GetSrcNoteOffset(sn, 1);
    ptrdiff_t tail = js_GetSrcNoteOffset(sn, 2);

    



    jsbytecode *pc2 = pc;
    if (cond != tail) {
        LOCAL_ASSERT(*pc == JSOP_GOTO || *pc == JSOP_GOTOX);
        pc2 += (*pc == JSOP_GOTO) ? JSOP_GOTO_LENGTH : JSOP_GOTOX_LENGTH;
    }
    LOCAL_ASSERT(tail + GetJumpOffset(pc+tail, pc+tail) == pc2 - pc);

    if (cond != tail) {
        
        if (!Decompile(ss, pc + cond, tail - cond))
            return -1;
        js_printf(jp, " ");
        jsbytecode *condpc;
        const char *cond = PopStr(ss, JSOP_NOP, &condpc);
        SprintOpcodePermanent(jp, cond, condpc);
    }

    
    js_puts(jp, ";");

    if (next != cond) {
        







        uintN saveTop = ss->top;

        if (!Decompile(ss, pc + next, cond - next - JSOP_POP_LENGTH))
            return -1;
        LOCAL_ASSERT(ss->top - saveTop <= 1U);
        jsbytecode *updatepc = NULL;
        const char *update = (ss->top == saveTop)
                             ? ss->sprinter.base + ss->sprinter.offset
                             : PopStr(ss, JSOP_NOP, &updatepc);
        js_printf(jp, " ");
        SprintOpcodePermanent(jp, update, updatepc);
    }

    
    js_printf(jp, ") {\n");
    jp->indent += 4;
    next -= pc2 - pc;
    if (!Decompile(ss, pc2, next))
        return -1;
    jp->indent -= 4;
    js_printf(jp, "\t}\n");

    
    *ppc = pc;
    *plen = tail + js_CodeSpec[pc[tail]].length;
    return -2;
}

#undef LOCAL_ASSERT

static JSBool
InitSprintStack(JSContext *cx, SprintStack *ss, JSPrinter *jp, uintN depth)
{
    INIT_SPRINTER(cx, &ss->sprinter, &cx->tempLifoAlloc(), PAREN_SLOP);

    
    size_t offsetsz = depth * sizeof(ptrdiff_t);
    size_t opcodesz = depth * sizeof(jsbytecode);
    size_t bytecodesz = depth * sizeof(jsbytecode *);
    void *space = cx->tempLifoAlloc().alloc(offsetsz + opcodesz + bytecodesz);
    if (!space) {
        js_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    ss->offsets = (ptrdiff_t *) space;
    ss->opcodes = (jsbytecode *) ((char *)space + offsetsz);
    ss->bytecodes = (jsbytecode **) ((char *)space + offsetsz + opcodesz);

    ss->top = ss->inArrayInit = 0;
    ss->inGenExp = JS_FALSE;
    ss->printer = jp;
    return JS_TRUE;
}

template <typename T>
void
Swap(T &a, T &b)
{
    T tmp = a;
    a = b;
    b = tmp;
}






static jsbytecode *
Decompile(SprintStack *ss, jsbytecode *pc, intN nb)
{
    JSContext *cx;
    JSPrinter *jp, *jp2;
    jsbytecode *startpc, *endpc, *pc2, *done, *lvalpc, *rvalpc, *xvalpc;
    ptrdiff_t tail, todo, len, oplen, cond, next;
    JSOp op, lastop, saveop;
    const JSCodeSpec *cs;
    jssrcnote *sn, *sn2;
    const char *lval, *rval, *xval, *fmt, *token;
    uintN nuses;
    jsint i, argc;
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
#define DECOMPILE_CODE_CLEANUP(pc,nb,cleanup) if (!Decompile(ss, pc, nb)) cleanup
#define DECOMPILE_CODE(pc,nb) DECOMPILE_CODE_CLEANUP(pc,nb,return NULL)
#define TOP_STR()             GetStr(ss, ss->top - 1)
#define POP_STR()             PopStr(ss, op)
#define POP_STR_PREC(prec)    PopStrPrec(ss, prec)






#define GET_QUOTE_AND_FMT(qfmt, ufmt, rval)                                   \
    JS_BEGIN_MACRO                                                            \
        jschar quote_;                                                        \
        if (!IsIdentifier(atom)) {                                            \
            quote_ = '\'';                                                    \
            fmt = qfmt;                                                       \
        } else {                                                              \
            quote_ = 0;                                                       \
            fmt = ufmt;                                                       \
        }                                                                     \
        rval = QuoteString(&ss->sprinter, atom, quote_);                      \
        rval = SprintDupeStr(ss, rval);                                       \
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
        LOCAL_ASSERT(atomIndex_ < jp->script->natoms);                        \
        (atom) = jp->script->atoms[atomIndex_];                               \
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

    jsbytecode *lastlvalpc = NULL, *lastrvalpc = NULL;

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
        nuses = StackUses(jp->script, pc);

        




        if (nb < 0) {
            LOCAL_ASSERT(ss->top >= nuses);
            uintN ndefs = StackDefs(jp->script, pc);
            if ((uintN) -(nb + 1) == ss->top - nuses + ndefs)
                return pc;
        }

        



        token = CodeToken[op];

        if (pc + oplen == jp->dvgfence) {
            





            uint32_t format = cs->format;
            bool matchPC = false;
            if (StackFrame *fp = js_GetScriptedCaller(cx, NULL)) {
                jsbytecode *npc = fp->pcQuadratic(cx);
                if (pc == npc) {
                    matchPC = true;
                } else if (format & JOF_DECOMPOSE) {
                    if (unsigned(npc - pc) < GetDecomposeLength(pc, js_CodeSpec[*pc].length))
                        matchPC = true;
                }
            }
            if ((matchPC || (pc == startpc && nuses != 0)) &&
                format & (JOF_SET|JOF_DEL|JOF_INCDEC|JOF_VARPROP)) {
                uint32_t mode = JOF_MODE(format);
                if (mode == JOF_NAME) {
                    





                    uint32_t type = JOF_TYPE(format);
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

            jp->dvgfence = NULL;
        }

        jsbytecode *pushpc = pc;

        if (token) {
            switch (nuses) {
              case 2:
                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_ASSIGNOP) {
                    




                    op = (JSOp) pc[oplen];
                    rval = PopStr(ss, op, &lastrvalpc);
                    (void)PopStr(ss, op, &lastlvalpc);

                    
                    todo = SprintCString(&ss->sprinter, rval);
                } else if (!inXML) {
                    rval = PopStrPrecDupe(ss, cs->prec + !!(cs->format & JOF_LEFTASSOC), &rvalpc);
                    lval = PopStrPrec(ss, cs->prec + !(cs->format & JOF_LEFTASSOC), &lvalpc);
                    todo = ss->sprinter.offset;
                    SprintOpcode(ss, lval, lvalpc, pc, todo);
                    Sprint(&ss->sprinter, " %s ", token);
                    SprintOpcode(ss, rval, rvalpc, pc, todo);
                } else {
                    
                    LOCAL_ASSERT(op == JSOP_ADD);
                    rval = POP_STR();
                    lval = POP_STR();
                    todo = Sprint(&ss->sprinter, ss_format, lval, rval);
                }
                break;

              case 1:
                rval = PopStrDupe(ss, op, &rvalpc);
                todo = SprintCString(&ss->sprinter, token);
                SprintOpcode(ss, rval, rvalpc, pc, todo);
                break;

              case 0:
                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_CONTINUE) {
                    
                    todo = SprintCString(&ss->sprinter, SkipString);
                    break;
                }
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
                    
                    sn = js_GetSrcNote(jp->script, pc);
                    tail = js_GetSrcNoteOffset(sn, 0);
                    LOCAL_ASSERT(pc[tail] == JSOP_IFNE ||
                                 pc[tail] == JSOP_IFNEX);
                    js_printf(jp, "\tdo {\n");
                    jp->indent += 4;
                    DECOMPILE_CODE(pc, tail);
                    jp->indent -= 4;
                    js_printf(jp, "\t} while (");
                    rval = PopCondStr(ss, &rvalpc);
                    SprintOpcodePermanent(jp, rval, rvalpc);
                    js_printf(jp, ");\n");
                    pc += tail;
                    len = js_CodeSpec[*pc].length;
                    todo = -2;
                    break;

                  case SRC_FOR:
                    
                    todo = SprintNormalFor(cx, jp, ss, "", "", NULL, &pc, &len);
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

              case JSOP_LABEL:
              case JSOP_LABELX:
                sn = js_GetSrcNote(jp->script, pc);
                todo = -2;
                switch (sn ? SN_TYPE(sn) : SRC_NULL) {
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

                  default:
                    JS_NOT_REACHED("JSOP_LABEL without source note");
                    break;
                }
                break;

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
                    rvalpc = NULL;
                    todo = -2;
                    pc2 = pc + oplen;

                    if (*pc2 == JSOP_NOP) {
                        sn = js_GetSrcNote(jp->script, pc2);
                        if (sn) {
                            if (SN_TYPE(sn) == SRC_FOR) {
                                op = JSOP_NOP;
                                pc = pc2;
                                todo = SprintNormalFor(cx, jp, ss, "", rval, rvalpc, &pc, &len);
                                break;
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
                    rval = PopStr(ss, op, &rvalpc);
                    todo = SprintNormalFor(cx, jp, ss, "", rval, rvalpc, &pc, &len);
                    break;

                  case SRC_PCDELTA:
                    
                    op = JSOP_POP;

                    
                    lval = PopStrDupe(ss, op, &lvalpc);

                    



                    pushpc = pc;
                    done = pc + len;
                    pc += js_GetSrcNoteOffset(sn, 0);
                    len = 0;

                    if (!Decompile(ss, done, pc - done))
                        return NULL;

                    
                    rval = PopStrDupe(ss, op, &rvalpc);
                    todo = ss->sprinter.offset;
                    SprintOpcode(ss, lval, lvalpc, pushpc, todo);
                    SprintCString(&ss->sprinter, ", ");
                    SprintOpcode(ss, rval, rvalpc, pushpc, todo);
                    break;

                  case SRC_HIDDEN:
                    
                    todo = -2;
                    break;

                  case SRC_CONTINUE:
                    
                    (void) PopOff(ss, op);
                    todo = -2;
                    break;

                  default:
                  {
                    
                    if (ss->opcodes[ss->top-1] == JSOP_YIELD)
                        op = JSOP_NOP;

                    jsbytecode *rvalpc;
                    rval = PopStr(ss, op, &rvalpc);

                    





                    if (*rval != '\0' && (rval[0] != '/' || rval[1] != '*')) {
                        bool parens =
                            *rval == '{' ||
                            (strncmp(rval, js_function_str, 8) == 0 &&
                             rval[8] == ' ');
                        js_printf(jp, parens ? "\t(" : "\t");
                        SprintOpcodePermanent(jp, rval, rvalpc);
                        js_printf(jp, parens ? ");\n" : ";\n");
                    } else {
                        LOCAL_ASSERT(*rval == '\0' ||
                                     strcmp(rval, exception_cookie) == 0);
                    }
                    todo = -2;
                    break;
                  }
                }
                sn = NULL;
                break;

              case JSOP_ENTERWITH:
                LOCAL_ASSERT(!js_GetSrcNote(jp->script, pc));
                rval = PopStr(ss, op, &rvalpc);
                js_printf(jp, "\twith (");
                SprintOpcodePermanent(jp, rval, rvalpc);
                js_printf(jp, ") {\n");
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
                LOAD_OBJECT(0);
                AtomVector atoms(cx);
                StaticBlockObject &blockObj = obj->asStaticBlock();

                if (!GetBlockNames(cx, blockObj, &atoms) || !PushBlockNames(cx, ss, atoms))
                    return NULL;

                sn = js_GetSrcNote(jp->script, pc);
                switch (sn ? SN_TYPE(sn) : SRC_NULL) {
#if JS_HAS_BLOCK_SCOPE
                  case SRC_BRACE:
                    js_printf(jp, "\t{\n");
                    jp->indent += 4;
                    len = js_GetSrcNoteOffset(sn, 0);
                    if (!Decompile(ss, pc + oplen, len - oplen))
                        return NULL;
                    jp->indent -= 4;
                    js_printf(jp, "\t}\n");
                    break;
#endif

                  case SRC_CATCH:
                    jp->indent -= 4;
                    js_printf(jp, "\t} catch (");

                    pc2 = pc;
                    pc += oplen;
                    LOCAL_ASSERT(*pc == JSOP_EXCEPTION);
                    pc += JSOP_EXCEPTION_LENGTH;
                    todo = Sprint(&ss->sprinter, exception_cookie);
                    if (todo < 0 || !PushOff(ss, todo, JSOP_EXCEPTION))
                        return NULL;

                    if (*pc == JSOP_DUP) {
                        sn2 = js_GetSrcNote(jp->script, pc);
                        if (!sn2 || SN_TYPE(sn2) != SRC_DESTRUCT) {
                            




                            LOCAL_ASSERT(js_GetSrcNoteOffset(sn, 0) != 0);
                            pc += JSOP_DUP_LENGTH;
                            todo = Sprint(&ss->sprinter, exception_cookie);
                            if (todo < 0 || !PushOff(ss, todo, JSOP_EXCEPTION))
                                return NULL;
                        }
                    }

#if JS_HAS_DESTRUCTURING
                    if (*pc == JSOP_DUP) {
                        pc = DecompileDestructuring(ss, pc, endpc);
                        if (!pc)
                            return NULL;
                        LOCAL_ASSERT(*pc == JSOP_POP);
                        pc += JSOP_POP_LENGTH;
                        lval = PopStr(ss, JSOP_NOP);
                        js_puts(jp, lval);
                    } else {
#endif
                        LOCAL_ASSERT(*pc == JSOP_SETLOCALPOP);
                        pc += JSOP_SETLOCALPOP_LENGTH;
                        LOCAL_ASSERT(blockObj.slotCount() >= 1);
                        if (!QuoteString(&jp->sprinter, atoms[0], 0))
                            return NULL;
#if JS_HAS_DESTRUCTURING
                    }
#endif

                    



                    rval = PopStr(ss, JSOP_NOP);
                    LOCAL_ASSERT(strcmp(rval, exception_cookie) == 0);

                    len = js_GetSrcNoteOffset(sn, 0);
                    if (len) {
                        len -= pc - pc2;
                        LOCAL_ASSERT(len > 0);
                        js_printf(jp, " if ");
                        if (!Decompile(ss, pc, len))
                            return NULL;
                        js_printf(jp, "%s", POP_STR());
                        pc += len;
                        LOCAL_ASSERT(*pc == JSOP_IFEQ || *pc == JSOP_IFEQX);
                        pc += js_CodeSpec[*pc].length;
                    }

                    js_printf(jp, ") {\n");
                    jp->indent += 4;
                    len = 0;
                    break;
                  default:;
                }

                todo = -2;
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

              case JSOP_ENTERLET0:
              {
                LOAD_OBJECT(0);
                StaticBlockObject &blockObj = obj->asStaticBlock();

                AtomVector atoms(cx);
                if (!GetBlockNames(cx, blockObj, &atoms))
                    return NULL;

                sn = js_GetSrcNote(jp->script, pc);
                LOCAL_ASSERT(SN_TYPE(sn) == SRC_DECL);
                ptrdiff_t letData = js_GetSrcNoteOffset(sn, 0);
                bool groupAssign = LetDataToGroupAssign(letData);
                uintN letDepth = blockObj.stackDepth();
                LOCAL_ASSERT(letDepth == (uintN)ss->top - blockObj.slotCount());
                LOCAL_ASSERT(atoms.length() == blockObj.slotCount());

                




                Vector<const char *> rhsExprs(cx);
                if (!rhsExprs.resize(atoms.length()))
                    return NULL;
                for (size_t i = 0; i < rhsExprs.length(); ++i) {
                    rhsExprs[i] = GetStr(ss, letDepth + i);
                    if (!rhsExprs[i])
                        return NULL;
                }

                
                ptrdiff_t headBegin = ss->sprinter.offset;

                



                if (groupAssign) {
                    if (Sprint(&ss->sprinter, "[") < 0)
                        return NULL;
                    for (size_t i = 0; i < atoms.length(); ++i) {
                        if (i && Sprint(&ss->sprinter, ", ") < 0)
                            return NULL;
                        if (!QuoteString(&ss->sprinter, atoms[i], 0))
                            return NULL;
                    }
                    if (Sprint(&ss->sprinter, "] = [") < 0)
                        return NULL;
                }

                for (size_t i = 0; i < atoms.length(); ++i) {
                    const char *rhs = rhsExprs[i];
                    if (!strcmp(rhs, SkipString))
                        continue;

                    if (i && Sprint(&ss->sprinter, ", ") < 0)
                        return NULL;

                    if (groupAssign) {
                        if (SprintCString(&ss->sprinter, rhs) < 0)
                            return NULL;
                    } else if (!strncmp(rhs, DestructuredString, DestructuredStringLength)) {
                        if (SprintCString(&ss->sprinter, rhs + DestructuredStringLength) < 0)
                            return NULL;
                    } else {
                        JS_ASSERT(atoms[i] != cx->runtime->atomState.emptyAtom);
                        if (!QuoteString(&ss->sprinter, atoms[i], 0))
                            return NULL;
                        if (*rhs) {
                            uint8_t prec = js_CodeSpec[ss->opcodes[letDepth + i]].prec;
                            const char *fmt = prec && prec < js_CodeSpec[JSOP_SETLOCAL].prec
                                              ? " = (%s)"
                                              : " = %s";
                            if (Sprint(&ss->sprinter, fmt, rhs) < 0)
                                return NULL;
                        }
                    }
                }

                if (groupAssign && Sprint(&ss->sprinter, "]") < 0)
                    return NULL;

                
                DupBuffer head(cx);
                if (!Dup(OFF2STR(&ss->sprinter, headBegin), &head))
                    return NULL;
                if (!AssignBlockNamesToPushedSlots(cx, ss, atoms))
                    return NULL;

                
                jsbytecode *nextpc = pc + JSOP_ENTERLET0_LENGTH;
                if (*nextpc == JSOP_NOP) {
                    jssrcnote *nextsn = js_GetSrcNote(jp->script, nextpc);
                    if (nextsn && SN_TYPE(nextsn) == SRC_FOR) {
                        pc = nextpc;
                        todo = SprintNormalFor(cx, jp, ss, "let ", head.begin(), pc, &pc, &len);
                        break;
                    }
                }

                
                len = LetDataToOffset(letData);
                pc = nextpc;
                saveop = (JSOp) pc[len];
                todo = SprintLetBody(cx, jp, ss, pc, len, head.begin());
                break;
              }

              












              case JSOP_ENTERLET1:
              {
                LOAD_OBJECT(0);
                StaticBlockObject &blockObj = obj->asStaticBlock();

                AtomVector atoms(cx);
                if (!GetBlockNames(cx, blockObj, &atoms))
                    return NULL;

                LOCAL_ASSERT(js_GetSrcNote(jp->script, pc) == NULL);
                LOCAL_ASSERT(ss->top - 1 == blockObj.stackDepth() + blockObj.slotCount());
                jsbytecode *nextpc = pc + JSOP_ENTERLET1_LENGTH;
                if (*nextpc == JSOP_GOTO || *nextpc == JSOP_GOTOX) {
                    LOCAL_ASSERT(SN_TYPE(js_GetSrcNote(jp->script, nextpc)) == SRC_FOR_IN);
                } else {
                    LOCAL_ASSERT(*nextpc == JSOP_CONDSWITCH ||
                                 *nextpc == JSOP_TABLESWITCH || *nextpc == JSOP_TABLESWITCHX ||
                                 *nextpc == JSOP_LOOKUPSWITCH || *nextpc == JSOP_LOOKUPSWITCHX);
                }

                DupBuffer rhs(cx);
                if (!Dup(PopStr(ss, JSOP_NOP), &rhs))
                    return NULL;
                if (!AssignBlockNamesToPushedSlots(cx, ss, atoms))
                    return NULL;
                if (!PushStr(ss, rhs.begin(), op))
                    return NULL;
                todo = -2;
                break;
              }

              case JSOP_GETFCSLOT:
              case JSOP_CALLFCSLOT:
              {
                if (!jp->fun)
                    jp->fun = jp->script->getCallerFunction();

                if (!jp->localNames) {
                    JS_ASSERT(fun == jp->fun);
                    jp->localNames = cx->new_<Vector<JSAtom *> >(cx);
                    if (!jp->localNames ||
                        !jp->fun->script()->bindings.getLocalNameArray(cx, jp->localNames))
                    {
                        return NULL;
                    }
                }

                uintN index = GET_UINT16(pc);
                if (index < jp->fun->script()->bindings.countUpvars()) {
                    index += jp->fun->script()->bindings.countArgsAndVars();
                } else {
                    JSUpvarArray *uva;
#ifdef DEBUG
                    








                    StackFrame *fp = js_GetTopStackFrame(cx, FRAME_EXPAND_NONE);
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
                rval = PopStrDupe(ss, op, &rvalpc);
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
                    rval = PopStr(ss, op, &rvalpc);
                    bool parens = (*rval == '{');
                    if (parens)
                        js_printf(jp, "(");
                    SprintOpcodePermanent(jp, rval, rvalpc);
                    js_printf(jp, parens ? ")%s" : "%s",
                              ((fun->flags & JSFUN_LAMBDA) || !fun->atom)
                              ? ""
                              : ";");
                    todo = -2;
                    break;
                }
                

              case JSOP_SETRVAL:
                rval = PopStr(ss, op, &rvalpc);
                if (*rval != '\0') {
                    js_printf(jp, "\t%s ", js_return_str);
                    SprintOpcodePermanent(jp, rval, rvalpc);
                    js_printf(jp, ";\n");
                } else {
                    js_printf(jp, "\t%s;\n", js_return_str);
                }
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
                
                op = JSOP_SETNAME;

                
                rval = POP_STR();

                




                uintN pos = ss->top;
                while (pos != 0) {
                    op = (JSOp) ss->opcodes[--pos];
                    if (op == JSOP_ENTERBLOCK)
                        break;
                }
                JS_ASSERT(op == JSOP_ENTERBLOCK);

                




                uintN forpos = pos + 1;
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

                ptrdiff_t start = ss->offsets[pos];
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
                rval = PopStr(ss, op, &rvalpc);
                js_printf(jp, "\t%s ", js_throw_str);
                SprintOpcodePermanent(jp, rval, rvalpc);
                js_printf(jp, ";\n");
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
                    JS_ASSERT(pc[next] == JSOP_POP);
                    JS_ASSERT(pc[cond] == JSOP_MOREITER);
                    DECOMPILE_CODE(pc + oplen, next - oplen);
                    lval = POP_STR();

                    



                    JS_ASSERT(strcmp(lval + strlen(lval) - 9, " = <next>") == 0);
                    const_cast<char *>(lval)[strlen(lval) - 9] = '\0';
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
                        DECOMPILE_CODE(pc + next + JSOP_POP_LENGTH, cond - next - JSOP_POP_LENGTH);
                    } else {
                        



                        rval = GetStr(ss, ss->top - 1);
                        xval = VarPrefix(js_GetSrcNote(jp->script, pc + next));
                        js_printf(jp, "\t%s (%s%s in %s) {\n",
                                  foreach ? js_for_each_str : js_for_str,
                                  xval, lval, rval);
                        jp->indent += 4;
                        DECOMPILE_CODE(pc + next + JSOP_POP_LENGTH, cond - next - JSOP_POP_LENGTH);
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
                    js_printf(jp, "\twhile (");
                    rval = PopCondStr(ss, &rvalpc);
                    SprintOpcodePermanent(jp, rval, rvalpc);
                    js_printf(jp, ") {\n");
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
                    rval = PopCondStr(ss, &rvalpc);
                    if (ss->inArrayInit || ss->inGenExp) {
                        LOCAL_ASSERT(SN_TYPE(sn) == SRC_IF);
                        ss->sprinter.offset -= PAREN_SLOP;
                        if (Sprint(&ss->sprinter, " if (%s)", rval) < 0)
                            return NULL;
                        AddParenSlop(ss);
                    } else {
                        js_printf(jp, elseif ? " if (" : "\tif (");
                        SprintOpcodePermanent(jp, rval, rvalpc);
                        js_printf(jp, ") {\n");
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
                    xval = PopStrDupe(ss, op, &xvalpc);
                    len = js_GetSrcNoteOffset(sn, 0);
                    DECOMPILE_CODE(pc + oplen, len - oplen);
                    lval = PopStrDupe(ss, op, &lvalpc);
                    pushpc = pc;
                    pc += len;
                    LOCAL_ASSERT(*pc == JSOP_GOTO || *pc == JSOP_GOTOX);
                    oplen = js_CodeSpec[*pc].length;
                    len = GetJumpOffset(pc, pc);
                    DECOMPILE_CODE(pc + oplen, len - oplen);
                    rval = PopStrDupe(ss, op, &rvalpc);
                    todo = ss->sprinter.offset;
                    SprintOpcode(ss, xval, xvalpc, pushpc, todo);
                    SprintCString(&ss->sprinter, " ? ");
                    SprintOpcode(ss, lval, lvalpc, pushpc, todo);
                    SprintCString(&ss->sprinter, " : ");
                    SprintOpcode(ss, rval, rvalpc, pushpc, todo);
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
                
                lval = PopStrDupe(ss, op, &lvalpc);
                done = pc + GetJumpOffset(pc, pc);
                pushpc = pc;
                pc += len;
                JS_ASSERT(*pc == JSOP_POP);
                pc += JSOP_POP_LENGTH;
                len = done - pc;
                if (!Decompile(ss, pc, len))
                    return NULL;
                rval = PopStrDupe(ss, op, &rvalpc);
                if (!rval)
                    return NULL;
                todo = ss->sprinter.offset;
                SprintOpcode(ss, lval, lvalpc, pushpc, todo);
                if (jp->pretty &&
                    jp->indent + 4 + strlen(lval) + 4 + strlen(rval) > 75) {
                    Sprint(&ss->sprinter, " %s\n", xval);
                    Sprint(&ss->sprinter, "%*s", jp->indent + 4, "");
                } else {
                    Sprint(&ss->sprinter, " %s ", xval);
                }
                SprintOpcode(ss, rval, rvalpc, pushpc, todo);
                break;

              case JSOP_AND:
              case JSOP_ANDX:
                xval = "&&";
                goto do_logical_connective;

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
                    if (SN_TYPE(sn) == SRC_DESTRUCT) {
                        pc = DecompileDestructuring(ss, pc, endpc);
                        if (!pc)
                            return NULL;

                        lval = POP_STR();  
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

                        op = saveop = JSOP_ENUMELEM;
                        len = 0;
                    } else {
                        LOCAL_ASSERT(SN_TYPE(sn) == SRC_DESTRUCTLET);

                        ptrdiff_t offsetToLet = js_GetSrcNoteOffset(sn, 0);
                        LOCAL_ASSERT(*(pc + offsetToLet) == JSOP_ENTERLET0);

                        GET_OBJECT_FROM_BYTECODE(jp->script, pc + offsetToLet, 0, obj);
                        StaticBlockObject &blockObj = obj->asStaticBlock();

                        uint32_t blockDepth = blockObj.stackDepth();
                        LOCAL_ASSERT(blockDepth < ss->top);
                        LOCAL_ASSERT(ss->top <= blockDepth + blockObj.slotCount());

                        AtomVector atoms(cx);
                        if (!GetBlockNames(cx, blockObj, &atoms))
                            return NULL;

                        




                        AtomRange letNames = atoms.all();
                        uint32_t curDepth = ss->top - 1 ;
                        for (uint32_t i = blockDepth; i < curDepth; ++i)
                            letNames.popFront();

                        



                        DupBuffer rhs(cx);
                        if (!Dup(PopStr(ss, JSOP_SETLOCAL), &rhs))
                            return NULL;

                        
                        size_t remainBefore = letNames.remain();
                        pc = DecompileDestructuring(ss, pc, endpc, &letNames);
                        if (!pc)
                            return NULL;
                        size_t remainAfter = letNames.remain();

                        



                        const char *lhs = PopStr(ss, JSOP_NOP);
                        ptrdiff_t off = Sprint(&ss->sprinter, "%s%s = %s",
                                               DestructuredString, lhs, rhs.begin());
                        if (off < 0 || !PushOff(ss, off, JSOP_NOP))
                            return NULL;

                        








                        LOCAL_ASSERT(remainBefore >= remainAfter);
                        LOCAL_ASSERT(remainBefore > remainAfter || remainAfter > 0);
                        for (size_t i = remainBefore - 1; i > remainAfter; --i) {
                            if (!PushStr(ss, SkipString, JSOP_NOP))
                                return NULL;
                        }

                        LOCAL_ASSERT(*pc == JSOP_POP);
                        pc += JSOP_POP_LENGTH;

                        
                        if (remainBefore == remainAfter) {
                            LOCAL_ASSERT(*pc == JSOP_UNDEFINED);
                            pc += JSOP_UNDEFINED_LENGTH;
                        }

                        len = 0;
                        todo = -2;
                    }
                    break;
                }
#endif

                rval = GetStr(ss, ss->top-1);
                saveop = (JSOp) ss->opcodes[ss->top-1];
                todo = SprintCString(&ss->sprinter, rval);
                break;

              case JSOP_SWAP:
                Swap(ss->offsets[ss->top-1], ss->offsets[ss->top-2]);
                Swap(ss->opcodes[ss->top-1], ss->opcodes[ss->top-2]);
                Swap(ss->bytecodes[ss->top-1], ss->bytecodes[ss->top-2]);
                todo = -2;
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
                rval = PopStrDupe(ss, op, &rvalpc);
                if (op == JSOP_SETNAME || op == JSOP_SETGNAME)
                    (void) PopOff(ss, op);

              do_setlval:
                sn = js_GetSrcNote(jp->script, pc - 1);
                todo = ss->sprinter.offset;
                if (sn && SN_TYPE(sn) == SRC_ASSIGNOP) {
                    const char *token =
                        GetTokenForAssignment(jp, sn, lastop, pc, rvalpc,
                                              &lastlvalpc, &lastrvalpc);
                    Sprint(&ss->sprinter, "%s %s= ", lval, token);
                    SprintOpcode(ss, rval, rvalpc, pc, todo);
                } else {
                    sn = js_GetSrcNote(jp->script, pc);
                    const char *prefix = VarPrefix(sn);
                    Sprint(&ss->sprinter, "%s%s = ", prefix, lval);
                    SprintOpcode(ss, rval, rvalpc, pc, todo);
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
              {
                argc = GET_ARGC(pc);
                const char **argv = (const char **)
                    cx->malloc_((size_t)(argc + 1) * sizeof *argv);
                if (!argv)
                    return NULL;
                jsbytecode **argbytecodes = (jsbytecode **)
                    cx->malloc_((size_t)(argc + 1) * sizeof *argbytecodes);
                if (!argbytecodes) {
                    cx->free_(argv);
                    return NULL;
                }

                op = JSOP_SETNAME;
                for (i = argc; i > 0; i--)
                    argv[i] = PopStrDupe(ss, op, &argbytecodes[i]);

                
                LOCAL_ASSERT(ss->top >= 2);
                (void) PopOff(ss, op);

                




                op = (JSOp) ss->opcodes[ss->top - 1];
                argv[0] = PopStrDupe(ss,
                                     (saveop == JSOP_NEW &&
                                      (op == JSOP_CALL ||
                                       op == JSOP_EVAL ||
                                       op == JSOP_FUNCALL ||
                                       op == JSOP_FUNAPPLY ||
                                       op == JSOP_CALLPROP ||
                                       op == JSOP_CALLELEM))
                                     ? JSOP_NAME
                                     : saveop,
                                     &lvalpc);
                op = saveop;

                lval = "(", rval = ")";
                todo = ss->sprinter.offset;
                if (op == JSOP_NEW) {
                    if (argc == 0)
                        lval = rval = "";
                    Sprint(&ss->sprinter, "%s ", js_new_str);
                }
                SprintOpcode(ss, argv[0], lvalpc, pc, todo);
                SprintCString(&ss->sprinter, lval);

                for (i = 1; i <= argc; i++) {
                    SprintOpcode(ss, argv[i], argbytecodes[i], pc, todo);
                    if (i < argc)
                        SprintCString(&ss->sprinter, ", ");
                }
                SprintCString(&ss->sprinter, rval);

                cx->free_(argv);
                cx->free_(argbytecodes);

                break;
              }

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
              {
                const char *prefix = (op == JSOP_VOID) ? js_void_str : js_typeof_str;
                rval = PopStrDupe(ss, op, &rvalpc);
                todo = ss->sprinter.offset;
                Sprint(&ss->sprinter, "%s ", prefix);
                SprintOpcode(ss, rval, rvalpc, pc, todo);
                break;
              }

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
                if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
                    len += GetDecomposeLength(pc, js_CodeSpec[*pc].length);
                break;

              case JSOP_INCPROP:
              case JSOP_DECPROP:
                GET_ATOM_QUOTE_AND_FMT(preindex_format, predot_format, rval);

                




                op = JSOP_GETPROP;
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, fmt,
                              js_incop_strs[!(cs->format & JOF_INC)],
                              lval, rval);
                len += GetDecomposeLength(pc, JSOP_INCPROP_LENGTH);
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
                len += GetDecomposeLength(pc, JSOP_INCELEM_LENGTH);
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
                if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
                    len += GetDecomposeLength(pc, js_CodeSpec[*pc].length);
                break;

              case JSOP_PROPINC:
              case JSOP_PROPDEC:
                GET_ATOM_QUOTE_AND_FMT(postindex_format, postdot_format, rval);

                




                op = JSOP_GETPROP;
                lval = POP_STR();
                todo = Sprint(&ss->sprinter, fmt, lval, rval,
                              js_incop_strs[!(cs->format & JOF_INC)]);
                len += GetDecomposeLength(pc, JSOP_PROPINC_LENGTH);
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
                len += GetDecomposeLength(pc, JSOP_ELEMINC_LENGTH);
                break;

              case JSOP_GETPROP2:
                op = JSOP_GETPROP;
                (void) PopOff(ss, lastop);
                

              case JSOP_CALLPROP:
              case JSOP_GETPROP:
              case JSOP_GETXPROP:
              case JSOP_LENGTH:
                LOAD_ATOM(0);

                GET_QUOTE_AND_FMT("[%s]", ".%s", rval);
                PROPAGATE_CALLNESS();
                lval = PopStr(ss, op, &lvalpc);
                todo = ss->sprinter.offset;
                SprintOpcode(ss, lval, lvalpc, pc, todo);
                Sprint(&ss->sprinter, fmt, rval);
                break;

              case JSOP_SETPROP:
              case JSOP_SETMETHOD:
              {
                LOAD_ATOM(0);
                GET_QUOTE_AND_FMT("[%s] %s= ", ".%s %s= ", xval);
                rval = PopStrDupe(ss, op, &rvalpc);

                




                op = JSOP_GETPROP;
                lval = PopStr(ss, op, &lvalpc);
                sn = js_GetSrcNote(jp->script, pc - 1);
                const char *token =
                    GetTokenForAssignment(jp, sn, lastop, pc, rvalpc,
                                          &lastlvalpc, &lastrvalpc);
                todo = ss->sprinter.offset;
                SprintOpcode(ss, lval, lvalpc, pc, todo);
                Sprint(&ss->sprinter, fmt, xval, token);
                SprintOpcode(ss, rval, rvalpc, pc, todo);
                break;
              }

              case JSOP_GETELEM2:
                (void) PopOff(ss, lastop);
                
              case JSOP_CALLELEM:
              case JSOP_GETELEM:
                op = JSOP_NOP;          
                xval = PopStrDupe(ss, op, &xvalpc);
                op = saveop;
                PROPAGATE_CALLNESS();
                lval = PopStr(ss, op, &lvalpc);
                todo = ss->sprinter.offset;
                SprintOpcode(ss, lval, lvalpc, pc, todo);
                if (*xval != '\0') {
                    bool xml = (JOF_OPMODE(lastop) == JOF_XMLNAME);
                    SprintCString(&ss->sprinter, xml ? "." : "[");
                    SprintOpcode(ss, xval, xvalpc, pc, todo);
                    SprintCString(&ss->sprinter, xml ? "" : "]");
                }
                break;

              case JSOP_SETELEM:
              {
                rval = PopStrDupe(ss, op, &rvalpc);
                op = JSOP_NOP;          
                xval = PopStrDupe(ss, op, &xvalpc);
                cs = &js_CodeSpec[ss->opcodes[ss->top]];
                op = JSOP_GETELEM;      
                lval = PopStr(ss, op, &lvalpc);
                op = saveop;
                if (*xval == '\0')
                    goto do_setlval;
                sn = js_GetSrcNote(jp->script, pc - 1);
                bool xml = (JOF_MODE(cs->format) == JOF_XMLNAME);
                const char *token =
                    GetTokenForAssignment(jp, sn, lastop, pc, rvalpc,
                                          &lastlvalpc, &lastrvalpc);
                todo = ss->sprinter.offset;
                SprintOpcode(ss, lval, lvalpc, pc, todo);
                SprintCString(&ss->sprinter, xml ? "." : "[");
                SprintOpcode(ss, xval, xvalpc, pc, todo);
                SprintCString(&ss->sprinter, xml ? "" : "]");
                Sprint(&ss->sprinter, " %s= ", token);
                SprintOpcode(ss, rval, rvalpc, pc, todo);
                break;
              }

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
#if JS_HAS_GENERATOR_EXPRS
                sn = js_GetSrcNote(jp->script, pc);
                if (sn && SN_TYPE(sn) == SRC_GENEXP) {
                    Vector<JSAtom *> *innerLocalNames;
                    Vector<JSAtom *> *outerLocalNames;
                    JSScript *inner, *outer;
                    Vector<DecompiledOpcode> *decompiledOpcodes;
                    SprintStack ss2;
                    JSFunction *outerfun;

                    LOAD_FUNCTION(0);

                    





                    LifoAllocScope las(&cx->tempLifoAlloc());
                    if (fun->script()->bindings.hasLocalNames()) {
                        innerLocalNames = cx->new_<Vector<JSAtom *> >(cx);
                        if (!innerLocalNames ||
                            !fun->script()->bindings.getLocalNameArray(cx, innerLocalNames))
                        {
                            return NULL;
                        }
                    } else {
                        innerLocalNames = NULL;
                    }
                    inner = fun->script();
                    if (!InitSprintStack(cx, &ss2, jp, StackDepth(inner)))
                        return NULL;
                    ss2.inGenExp = JS_TRUE;

                    






                    outer = jp->script;
                    outerfun = jp->fun;
                    outerLocalNames = jp->localNames;
                    decompiledOpcodes = jp->decompiledOpcodes;
                    LOCAL_ASSERT(UnsignedPtrDiff(pc, outer->code) <= outer->length);
                    jp->script = inner;
                    jp->fun = fun;
                    jp->localNames = innerLocalNames;
                    jp->decompiledOpcodes = NULL;

                    



                    ok = Decompile(&ss2, inner->main(), inner->length - inner->mainOffset)
                         != NULL;
                    jp->script = outer;
                    jp->fun = outerfun;
                    jp->localNames = outerLocalNames;
                    jp->decompiledOpcodes = decompiledOpcodes;
                    if (!ok)
                        return NULL;

                    



                    pc += len;
                    LOCAL_ASSERT(*pc == JSOP_UNDEFINED);
                    pc += JSOP_UNDEFINED_LENGTH;
                    LOCAL_ASSERT(*pc == JSOP_CALL);
                    LOCAL_ASSERT(GET_ARGC(pc) == 0);
                    len = JSOP_CALL_LENGTH;

                    















                    pc2 = pc + len;
                    op = JSOp(*pc2);
                    if (op == JSOP_LOOPHEAD || op == JSOP_NOP)
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
                    las.releaseEarly();
                    if (!rval)
                        return NULL;
                    todo = SprintCString(&ss->sprinter, rval);
                    cx->free_((void *)rval);
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
                str = obj->asRegExp().toString(cx);
                if (!str)
                    return NULL;
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
                jmplen = (op == JSOP_TABLESWITCH) ? JUMP_OFFSET_LEN : JUMPX_OFFSET_LEN;
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
                    ok = true;
                } else {
                    table = (TableEntry *)
                            cx->malloc_((size_t)n * sizeof *table);
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
                          cx->malloc_((size_t)j * sizeof *table);
                    if (tmp) {
                        MergeSort(table, size_t(j), tmp, CompareTableEntries);
                        Foreground::free_(tmp);
                        ok = true;
                    } else {
                        ok = false;
                    }
                }

                if (ok)
                    ok = DecompileSwitch(ss, table, (uintN)j, pc, len, off, false);
                cx->free_(table);
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
                    cx->malloc_((size_t)npairs * sizeof *table);
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
                    uint16_t constIndex = GET_INDEX(pc2);
                    pc2 += INDEX_LEN;
                    off2 = GetJumpOffset(pc, pc2);
                    pc2 += jmplen;
                    table[k].key = jp->script->getConst(constIndex);
                    table[k].offset = off2;
                }

                ok = DecompileSwitch(ss, table, (uintN)npairs, pc, len, off,
                                     JS_FALSE);
                cx->free_(table);
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
                    cx->malloc_((size_t)ncases * sizeof *table);
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
                cx->free_(table);
                if (!ok)
                    return NULL;
                todo = -2;
                break;
              }

              case JSOP_CASE:
              case JSOP_CASEX:
              {
                lval = PopStr(ss, op, &lvalpc);
                if (!lval)
                    return NULL;
                js_printf(jp, "\tcase ");
                SprintOpcodePermanent(jp, lval, lvalpc);
                js_printf(jp, ":\n");
                todo = -2;
                break;
              }

              case JSOP_DEFFUN:
              case JSOP_DEFFUN_FC:
                LOAD_FUNCTION(0);
                todo = -2;
                goto do_function;
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
                rval = PopStr(ss, op, &rvalpc);
                sn = js_GetSrcNote(jp->script, pc);

                
                for (xval = rval; *xval != '[' && *xval != '{'; xval++)
                    continue;
                inArray = (*xval == '[');
                if (inArray)
                    --ss->inArrayInit;
                todo = ss->sprinter.offset;
                SprintOpcode(ss, rval, rvalpc, pc, todo);
                Sprint(&ss->sprinter, "%s%c",
                       (sn && SN_TYPE(sn) == SRC_CONTINUE) ? ", " : "",
                       inArray ? ']' : '}');
                break;
              }

              {
                JSBool isFirst;
                const char *maybeComma;

              case JSOP_INITELEM:
                isFirst = IsInitializerOp(ss->opcodes[ss->top - 3]);

                
                rval = PopStr(ss, JSOP_SETNAME, &rvalpc);

                
                xval = PopStr(ss, JSOP_NOP);
                lval = PopStr(ss, JSOP_NOP, &lvalpc);
                sn = js_GetSrcNote(jp->script, pc);

                if (sn && SN_TYPE(sn) == SRC_INITPROP) {
                    atom = NULL;
                    goto do_initprop;
                }
                maybeComma = isFirst ? "" : ", ";
                todo = Sprint(&ss->sprinter, "%s%s", lval, maybeComma);
                SprintOpcode(ss, rval, rvalpc, pc, todo);
                break;

              case JSOP_INITPROP:
              case JSOP_INITMETHOD:
                LOAD_ATOM(0);
                xval = QuoteString(&ss->sprinter, atom, jschar(IsIdentifier(atom) ? 0 : '\''));
                if (!xval)
                    return NULL;
                isFirst = IsInitializerOp(ss->opcodes[ss->top - 2]);
                rval = PopStrDupe(ss, op, &rvalpc);
                lval = PopStr(ss, op, &lvalpc);
                

              do_initprop:
                todo = ss->sprinter.offset;
                SprintOpcode(ss, lval, lvalpc, pc, todo);
                maybeComma = isFirst ? "" : ", ";
                if (lastop == JSOP_GETTER || lastop == JSOP_SETTER) {
                    const char *end = rval + strlen(rval);

                    if (*rval == '(')
                        ++rval, --end;
                    LOCAL_ASSERT(strncmp(rval, js_function_str, 8) == 0);
                    LOCAL_ASSERT(rval[8] == ' ');
                    rval += 8 + 1;
                    LOCAL_ASSERT(*end ? *end == ')' : end[-1] == '}');
                    Sprint(&ss->sprinter, "%s%s %s%s%.*s",
                           maybeComma,
                           (lastop == JSOP_GETTER)
                           ? js_get_str : js_set_str,
                           xval,
                           (rval[0] != '(') ? " " : "",
                           end - rval, rval);
                } else {
                    Sprint(&ss->sprinter, "%s%s: ", maybeComma, xval);
                    SprintOpcode(ss, rval, rvalpc, pc, todo);
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
                
                rval = PopStrDupe(ss, op, &rvalpc);
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
                cx->free_((char *)rval);
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

        if (cx->isExceptionPending()) {
            
            return NULL;
        }

        if (todo < 0) {
            
            JS_ASSERT(todo == -2);
            if (todo == -1)
                return NULL;
        } else {
            if (!UpdateDecompiledText(ss, pushpc, todo))
                return NULL;
            if (!PushOff(ss, todo, saveop, pushpc))
                return NULL;
            if (js_CodeSpec[*pc].format & JOF_DECOMPOSE)
                CopyDecompiledTextForDecomposedOp(jp, pc);
        }

        pc += len;
    }




#undef inXML
#undef DECOMPILE_CODE
#undef TOP_STR
#undef POP_STR
#undef POP_STR_PREC
#undef LOCAL_ASSERT
#undef GET_QUOTE_AND_FMT
#undef GET_ATOM_QUOTE_AND_FMT

    return pc;
}

static JSBool
DecompileCode(JSPrinter *jp, JSScript *script, jsbytecode *pc, uintN len,
              uintN pcdepth)
{
    JSContext *cx = jp->sprinter.context;

    uintN depth = StackDepth(script);
    JS_ASSERT(pcdepth <= depth);

    
    LifoAllocScope las(&cx->tempLifoAlloc());
    SprintStack ss;
    if (!InitSprintStack(cx, &ss, jp, depth))
        return false;

    










    ss.top = pcdepth;
    if (pcdepth != 0) {
        for (uintN i = 0; i < pcdepth; i++) {
            ss.offsets[i] = -2 - (ptrdiff_t)i;
            ss.opcodes[i] = *jp->pcstack[i];
        }
    }

    
    JSScript *oldscript = jp->script;
    jp->script = script;
    bool ok = Decompile(&ss, pc, len) != NULL;
    jp->script = oldscript;

    
    if (ok && ss.top) {
        char *last;
        do {
            last = OFF2STR(&ss.sprinter, PopOff(&ss, JSOP_POP));
        } while (ss.top > pcdepth);
        js_printf(jp, "%s", last);
    }

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
    if (!jp->fun->isInterpreted()) {
        js_printf(jp, native_code_str);
        return JS_TRUE;
    }

    script = jp->fun->script();
    return DecompileBody(jp, script, script->code);
}

JSBool
js_DecompileFunction(JSPrinter *jp)
{
    JSFunction *fun = jp->fun;
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

    if (!fun->isInterpreted()) {
        js_printf(jp, ") {\n");
        jp->indent += 4;
        js_printf(jp, native_code_str);
        jp->indent -= 4;
        js_printf(jp, "\t}");
    } else {
        JSScript *script = fun->script();
#if JS_HAS_DESTRUCTURING
        SprintStack ss;
#endif

        
        jsbytecode *pc = script->main();
        jsbytecode *endpc = pc + script->length;
        JSBool ok = JS_TRUE;

#if JS_HAS_DESTRUCTURING
        ss.printer = NULL;
        jp->script = script;
        LifoAllocScope las(&jp->sprinter.context->tempLifoAlloc());
#endif

        for (uintN i = 0; i < fun->nargs; i++) {
            if (i > 0)
                js_puts(jp, ", ");

            JSAtom *param = GetArgOrVarAtom(jp, i);

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
        las.releaseEarly();
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
js_DecompileValueGenerator(JSContext *cx, intN spindex, jsval v,
                           JSString *fallback)
{
    StackFrame *fp;
    JSScript *script;
    jsbytecode *pc;

    JS_ASSERT(spindex < 0 ||
              spindex == JSDVG_IGNORE_STACK ||
              spindex == JSDVG_SEARCH_STACK);

    if (!cx->hasfp() || !cx->fp()->isScriptFrame())
        goto do_fallback;

    fp = js_GetTopStackFrame(cx, FRAME_EXPAND_ALL);
    script = fp->script();
    pc = cx->regs().pc;
    JS_ASSERT(script->code <= pc && pc < script->code + script->length);

    if (pc < script->main())
        goto do_fallback;
    
    if (spindex != JSDVG_IGNORE_STACK) {
        jsbytecode **pcstack;

        



        pcstack = (jsbytecode **)
                  cx->malloc_(StackDepth(script) * sizeof *pcstack);
        if (!pcstack)
            return NULL;
        jsbytecode *lastDecomposedPC = NULL;
        intN pcdepth = ReconstructPCStack(cx, script, pc, pcstack, &lastDecomposedPC);
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
            Value *sp = cx->regs().sp;
            do {
                if (sp == stackBase) {
                    pcdepth = -1;
                    goto release_pcstack;
                }
            } while (*--sp != v);

            













            if (sp < stackBase + pcdepth) {
                pc = pcstack[sp - stackBase];
                if (lastDecomposedPC) {
                    size_t len = GetDecomposeLength(lastDecomposedPC,
                                                    js_CodeSpec[*lastDecomposedPC].length);
                    if (unsigned(pc - lastDecomposedPC) < len)
                        pc = lastDecomposedPC;
                }
            }
        }

      release_pcstack:
        cx->free_(pcstack);
        if (pcdepth < 0)
            goto do_fallback;
    }

    {
        char *name = DecompileExpression(cx, script, fp->maybeFun(), pc);
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
    return DeflateString(cx, chars, length);
}

static char *
DecompileExpression(JSContext *cx, JSScript *script, JSFunction *fun,
                    jsbytecode *pc)
{
    JS_ASSERT(script->code <= pc && pc < script->code + script->length);

    JSOp op = (JSOp) *pc;

    
    JS_ASSERT(op != JSOP_CASE && op != JSOP_CASEX &&
              op != JSOP_DUP && op != JSOP_DUP2);

    



    if (op == JSOP_THIS)
        return JS_strdup(cx, js_this_str);

    





    if (op == JSOP_BINDNAME)
        return FAILED_EXPRESSION_DECOMPILER;

    
    const JSCodeSpec *cs = &js_CodeSpec[op];
    jsbytecode *begin = pc;
    jsbytecode *end = pc + cs->length;
    switch (JOF_MODE(cs->format)) {
      case JOF_PROP:
      case JOF_ELEM:
      case JOF_XMLNAME:
      case 0: {
        jssrcnote *sn = js_GetSrcNote(script, pc);
        if (!sn)
            return FAILED_EXPRESSION_DECOMPILER;
        switch (SN_TYPE(sn)) {
          case SRC_PCBASE:
            begin -= js_GetSrcNoteOffset(sn, 0);
            break;
          case SRC_PCDELTA:
            end = begin + js_GetSrcNoteOffset(sn, 0);
            begin += cs->length;
            break;
          default:
            return FAILED_EXPRESSION_DECOMPILER;
        }
        break;
      }
      default:;
    }

    



    if (op == JSOP_CALLPROP || op == JSOP_CALLELEM) {
        JS_ASSERT(*end == JSOP_SWAP);
        end += JSOP_SWAP_LENGTH;
    }

    ptrdiff_t len = end - begin;
    if (len <= 0)
        return FAILED_EXPRESSION_DECOMPILER;

    struct Guard {
        jsbytecode **pcstack;
        JSPrinter *printer;
        Guard() : pcstack(NULL), printer(NULL) {}
        ~Guard() {
            if (printer)
                js_DestroyPrinter(printer);
            Foreground::free_(pcstack);
        }
    } g;

    g.pcstack = (jsbytecode **)OffTheBooks::malloc_(StackDepth(script) * sizeof *g.pcstack);
    if (!g.pcstack)
        return NULL;

    intN pcdepth = ReconstructPCStack(cx, script, begin, g.pcstack, NULL);
    if (pcdepth < 0)
         return FAILED_EXPRESSION_DECOMPILER;

    g.printer = js_NewPrinter(cx, "js_DecompileValueGenerator", fun, 0, false, false, false);
    if (!g.printer)
        return NULL;

    g.printer->dvgfence = end;
    g.printer->pcstack = g.pcstack;
    if (!DecompileCode(g.printer, script, begin, (uintN) len, (uintN) pcdepth))
        return NULL;

    const char *name = (g.printer->sprinter.base) ? g.printer->sprinter.base : "";
    return JS_strdup(cx, name);
}

uintN
js_ReconstructStackDepth(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    return ReconstructPCStack(cx, script, pc, NULL, NULL);
}

#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_RV(expr, -1);

static intN
SimulateOp(JSContext *cx, JSScript *script, JSOp op, const JSCodeSpec *cs,
           jsbytecode *pc, jsbytecode **pcstack, uintN &pcdepth)
{
    uintN nuses = StackUses(script, pc);
    uintN ndefs = StackDefs(script, pc);
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

static intN
ReconstructPCStack(JSContext *cx, JSScript *script, jsbytecode *target,
                   jsbytecode **pcstack, jsbytecode **lastDecomposedPC)
{
    







    LOCAL_ASSERT(script->code <= target && target < script->code + script->length);
    jsbytecode *pc = script->code;
    uintN pcdepth = 0;
    ptrdiff_t oplen;
    for (; pc < target; pc += oplen) {
        JSOp op = JSOp(*pc);
        const JSCodeSpec *cs = &js_CodeSpec[op];
        oplen = cs->length;
        if (oplen < 0)
            oplen = js_GetVariableBytecodeLength(pc);

        if (cs->format & JOF_DECOMPOSE) {
            if (lastDecomposedPC)
                *lastDecomposedPC = pc;
            continue;
        }

        






        jssrcnote *sn = js_GetSrcNote(script, pc);
        if (sn && SN_TYPE(sn) == SRC_COND) {
            ptrdiff_t jmpoff = js_GetSrcNoteOffset(sn, 0);
            if (pc + jmpoff < target) {
                pc += jmpoff;
                op = JSOp(*pc);
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

    if (*pc == JSOP_POP)
        return false;

    if (*pc == JSOP_NOT)
        pc += JSOP_NOT_LENGTH;

    return (*pc != JSOP_IFEQ);
}

extern bool
IsValidBytecodeOffset(JSContext *cx, JSScript *script, size_t offset)
{
    
    for (BytecodeRange r(script); !r.empty(); r.popFront()) {
        size_t here = r.frontOffset();
        if (here >= offset)
            return here == offset;
    }
    return false;
}

JS_FRIEND_API(size_t)
GetPCCountScriptCount(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (!rt->scriptPCCounters)
        return 0;

    return rt->scriptPCCounters->length();
}

enum MaybeComma {NO_COMMA, COMMA};

static void
AppendJSONProperty(StringBuffer &buf, const char *name, MaybeComma comma = COMMA)
{
    if (comma)
        buf.append(',');

    buf.append('\"');
    buf.appendInflated(name, strlen(name));
    buf.appendInflated("\":", 2);
}

static void
AppendArrayJSONProperties(JSContext *cx, StringBuffer &buf,
                          double *values, const char **names, unsigned count, MaybeComma &comma)
{
    for (unsigned i = 0; i < count; i++) {
        if (values[i]) {
            AppendJSONProperty(buf, names[i], comma);
            comma = COMMA;
            NumberValueToStringBuffer(cx, DoubleValue(values[i]), buf);
        }
    }
}

JS_FRIEND_API(JSString *)
GetPCCountScriptSummary(JSContext *cx, size_t index)
{
    JSRuntime *rt = cx->runtime;

    if (!rt->scriptPCCounters || index >= rt->scriptPCCounters->length()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BUFFER_TOO_SMALL);
        return NULL;
    }

    ScriptOpcodeCountsPair info = (*rt->scriptPCCounters)[index];
    JSScript *script = info.script;

    




    StringBuffer buf(cx);

    buf.append('{');

    AppendJSONProperty(buf, "file", NO_COMMA);
    JSString *str = JS_NewStringCopyZ(cx, script->filename);
    if (!str || !(str = JS_ValueToSource(cx, StringValue(str))))
        return NULL;
    buf.append(str);

    AppendJSONProperty(buf, "line");
    NumberValueToStringBuffer(cx, Int32Value(script->lineno), buf);

    if (script->function()) {
        JSAtom *atom = script->function()->atom;
        if (atom) {
            AppendJSONProperty(buf, "name");
            if (!(str = JS_ValueToSource(cx, StringValue(atom))))
                return NULL;
            buf.append(str);
        }
    }

    double baseTotals[OpcodeCounts::BASE_COUNT] = {0.0};
    double accessTotals[OpcodeCounts::ACCESS_COUNT - OpcodeCounts::BASE_COUNT] = {0.0};
    double elementTotals[OpcodeCounts::ELEM_COUNT - OpcodeCounts::ACCESS_COUNT] = {0.0};
    double propertyTotals[OpcodeCounts::PROP_COUNT - OpcodeCounts::ACCESS_COUNT] = {0.0};
    double arithTotals[OpcodeCounts::ARITH_COUNT - OpcodeCounts::BASE_COUNT] = {0.0};

    for (unsigned i = 0; i < script->length; i++) {
        OpcodeCounts &counts = info.getCounts(script->code + i);
        if (!counts)
            continue;

        JSOp op = (JSOp)script->code[i];
        unsigned numCounts = OpcodeCounts::numCounts(op);

        for (unsigned j = 0; j < numCounts; j++) {
            double value = counts.get(j);
            if (j < OpcodeCounts::BASE_COUNT) {
                baseTotals[j] += value;
            } else if (OpcodeCounts::accessOp(op)) {
                if (j < OpcodeCounts::ACCESS_COUNT)
                    accessTotals[j - OpcodeCounts::BASE_COUNT] += value;
                else if (OpcodeCounts::elementOp(op))
                    elementTotals[j - OpcodeCounts::ACCESS_COUNT] += value;
                else if (OpcodeCounts::propertyOp(op))
                    propertyTotals[j - OpcodeCounts::ACCESS_COUNT] += value;
                else
                    JS_NOT_REACHED("Bad opcode");
            } else if (OpcodeCounts::arithOp(op)) {
                arithTotals[j - OpcodeCounts::BASE_COUNT] += value;
            } else {
                JS_NOT_REACHED("Bad opcode");
            }
        }
    }

    AppendJSONProperty(buf, "totals");
    buf.append('{');

    MaybeComma comma = NO_COMMA;

    AppendArrayJSONProperties(cx, buf, baseTotals, countBaseNames,
                              JS_ARRAY_LENGTH(baseTotals), comma);
    AppendArrayJSONProperties(cx, buf, accessTotals, countAccessNames,
                              JS_ARRAY_LENGTH(accessTotals), comma);
    AppendArrayJSONProperties(cx, buf, elementTotals, countElementNames,
                              JS_ARRAY_LENGTH(elementTotals), comma);
    AppendArrayJSONProperties(cx, buf, propertyTotals, countPropertyNames,
                              JS_ARRAY_LENGTH(propertyTotals), comma);
    AppendArrayJSONProperties(cx, buf, arithTotals, countArithNames,
                              JS_ARRAY_LENGTH(arithTotals), comma);

    buf.append('}');
    buf.append('}');

    if (cx->isExceptionPending())
        return NULL;

    return buf.finishString();
}

struct AutoDestroyPrinter
{
    JSPrinter *jp;
    AutoDestroyPrinter(JSPrinter *jp) : jp(jp) {}
    ~AutoDestroyPrinter() { js_DestroyPrinter(jp); }
};

static bool
GetPCCountJSON(JSContext *cx, const ScriptOpcodeCountsPair &info, StringBuffer &buf)
{
    JSScript *script = info.script;

    buf.append('{');
    AppendJSONProperty(buf, "text", NO_COMMA);

    Vector<DecompiledOpcode> decompiledOpcodes(cx);
    if (!decompiledOpcodes.reserve(script->length))
        return false;

    for (unsigned i = 0; i < script->length; i++)
        decompiledOpcodes.infallibleAppend(DecompiledOpcode());

    JSFunction *fun = script->function();
    JSPrinter *jp = js_NewPrinter(cx, "", fun, 4, true, false, false);
    if (!jp)
        return false;
    AutoDestroyPrinter destroy(jp);

    jp->decompiledOpcodes = &decompiledOpcodes;

    if (fun) {
        if (!js_DecompileFunction(jp))
            return false;
    } else {
        if (!js_DecompileScript(jp, script))
            return false;
    }
    JSString *str = js_GetPrinterOutput(jp);
    if (!str || !(str = JS_ValueToSource(cx, StringValue(str))))
        return false;

    buf.append(str);

    AppendJSONProperty(buf, "opcodes");
    buf.append('[');
    bool comma = false;

    SrcNoteLineScanner scanner(script->notes(), script->lineno);

    for (jsbytecode *pc = script->code;
         pc < script->code + script->length;
         pc += GetBytecodeLength(pc))
    {
        size_t offset = pc - script->code;

        JSOp op = (JSOp) *pc;

        if (comma)
            buf.append(',');
        comma = true;

        buf.append('{');

        AppendJSONProperty(buf, "id", NO_COMMA);
        NumberValueToStringBuffer(cx, Int32Value(pc - script->code), buf);

        scanner.advanceTo(offset);

        AppendJSONProperty(buf, "line");
        NumberValueToStringBuffer(cx, Int32Value(scanner.getLine()), buf);

        {
            const char *name = js_CodeName[op];
            AppendJSONProperty(buf, "name");
            buf.append('\"');
            buf.appendInflated(name, strlen(name));
            buf.append('\"');
        }

        DecompiledOpcode *search = &decompiledOpcodes[offset];
        size_t textBias = 0;
        while (search->parent) {
            textBias += search->parentOffset;
            if (search->parenthesized)
                textBias++;
            search = &decompiledOpcodes[search->parent - script->code];
        }

        int32_t printedOffset = search->parentOffset;
        if (printedOffset != -1) {
            printedOffset += textBias;
            if (search->parenthesized)
                printedOffset++;
            AppendJSONProperty(buf, "textOffset");
            NumberValueToStringBuffer(cx, Int32Value(printedOffset), buf);
        }

        const char *text = decompiledOpcodes[offset].text;
        if (text && *text != 0) {
            AppendJSONProperty(buf, "text");
            JSString *str = JS_NewStringCopyZ(cx, text);
            if (!str || !(str = JS_ValueToSource(cx, StringValue(str))))
                return false;
            buf.append(str);
        }

        OpcodeCounts &counts = info.getCounts(pc);
        unsigned numCounts = OpcodeCounts::numCounts(op);

        AppendJSONProperty(buf, "counts");
        buf.append('{');

        MaybeComma comma = NO_COMMA;
        for (unsigned i = 0; i < numCounts; i++) {
            double value = counts.get(i);
            if (value > 0) {
                AppendJSONProperty(buf, OpcodeCounts::countName(op, i), comma);
                comma = COMMA;
                NumberValueToStringBuffer(cx, DoubleValue(value), buf);
            }
        }

        buf.append('}');
        buf.append('}');
    }

    buf.append(']');
    buf.append('}');

    return !cx->isExceptionPending();
}

JS_FRIEND_API(JSString *)
GetPCCountScriptContents(JSContext *cx, size_t index)
{
    JSRuntime *rt = cx->runtime;

    if (!rt->scriptPCCounters || index >= rt->scriptPCCounters->length()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BUFFER_TOO_SMALL);
        return NULL;
    }

    const ScriptOpcodeCountsPair &info = (*rt->scriptPCCounters)[index];
    JSScript *script = info.script;

    StringBuffer buf(cx);

    if (!script->function() && !script->compileAndGo)
        return buf.finishString();

    {
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, script->function() ? (JSObject *) script->function() : script->global()))
            return NULL;

        if (!GetPCCountJSON(cx, info, buf))
            return NULL;
    }

    return buf.finishString();
}

} 
