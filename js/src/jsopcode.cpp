










#include "mozilla/FloatingPoint.h"
#include "mozilla/Util.h"

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jstypes.h"
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
#include "jsscript.h"
#include "jsstr.h"

#include "ds/Sort.h"
#include "frontend/BytecodeEmitter.h"
#include "frontend/TokenStream.h"
#include "js/CharacterEncoding.h"
#include "vm/Debugger.h"
#include "vm/Shape.h"
#include "vm/StringBuffer.h"

#include "jscntxtinlines.h"
#include "jsobjinlines.h"
#include "jsopcodeinlines.h"

#include "jsautooplen.h"

#include "vm/RegExpObject-inl.h"

using namespace js;
using namespace js::gc;

using js::frontend::IsIdentifier;
using mozilla::ArrayLength;




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

unsigned js_NumCodeSpecs = JS_ARRAY_LENGTH(js_CodeSpec);





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

size_t
js_GetVariableBytecodeLength(jsbytecode *pc)
{
    JSOp op = JSOp(*pc);
    JS_ASSERT(js_CodeSpec[op].length == -1);
    switch (op) {
      case JSOP_TABLESWITCH: {
        
        pc += JUMP_OFFSET_LEN;
        int32_t low = GET_JUMP_OFFSET(pc);
        pc += JUMP_OFFSET_LEN;
        int32_t high = GET_JUMP_OFFSET(pc);
        unsigned ncases = unsigned(high - low + 1);
        return 1 + 3 * JUMP_OFFSET_LEN + ncases * JUMP_OFFSET_LEN;
      }
      default:
        JS_NOT_REACHED("Unexpected op");
        return 0;
    }
}

static uint32_t
NumBlockSlots(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(*pc == JSOP_ENTERBLOCK || *pc == JSOP_ENTERLET0 || *pc == JSOP_ENTERLET1);
    JS_STATIC_ASSERT(JSOP_ENTERBLOCK_LENGTH == JSOP_ENTERLET0_LENGTH);
    JS_STATIC_ASSERT(JSOP_ENTERBLOCK_LENGTH == JSOP_ENTERLET1_LENGTH);

    return script->getObject(GET_UINT32_INDEX(pc))->asStaticBlock().slotCount();
}

unsigned
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

unsigned
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

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) == PCCounts::BASE_LIMIT);

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
                 JS_ARRAY_LENGTH(countAccessNames) == PCCounts::ACCESS_LIMIT);

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
                 JS_ARRAY_LENGTH(countElementNames) == PCCounts::ELEM_LIMIT);

static const char * countPropertyNames[] = {
    "prop_static",
    "prop_definite",
    "prop_other"
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) +
                 JS_ARRAY_LENGTH(countAccessNames) +
                 JS_ARRAY_LENGTH(countPropertyNames) == PCCounts::PROP_LIMIT);

static const char * countArithNames[] = {
    "arith_int",
    "arith_double",
    "arith_other",
    "arith_unknown",
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) +
                 JS_ARRAY_LENGTH(countArithNames) == PCCounts::ARITH_LIMIT);

 const char *
PCCounts::countName(JSOp op, size_t which)
{
    JS_ASSERT(which < numCounts(op));

    if (which < BASE_LIMIT)
        return countBaseNames[which];

    if (accessOp(op)) {
        if (which < ACCESS_LIMIT)
            return countAccessNames[which - BASE_LIMIT];
        if (elementOp(op))
            return countElementNames[which - ACCESS_LIMIT];
        if (propertyOp(op))
            return countPropertyNames[which - ACCESS_LIMIT];
        JS_NOT_REACHED("bad op");
        return NULL;
    }

    if (arithOp(op))
        return countArithNames[which - BASE_LIMIT];

    JS_NOT_REACHED("bad op");
    return NULL;
}

#ifdef DEBUG

void
js_DumpPCCounts(JSContext *cx, HandleScript script, js::Sprinter *sp)
{
    JS_ASSERT(script->hasScriptCounts);

    jsbytecode *pc = script->code;
    while (pc < script->code + script->length) {
        JSOp op = JSOp(*pc);

        int len = js_CodeSpec[op].length;
        jsbytecode *next = (len != -1) ? pc + len : pc + js_GetVariableBytecodeLength(pc);

        if (!js_Disassemble1(cx, script, pc, pc - script->code, true, sp))
            return;

        size_t total = PCCounts::numCounts(op);
        double *raw = script->getPCCounts(pc).rawCounts();

        Sprint(sp, "                  {");
        bool printed = false;
        for (size_t i = 0; i < total; i++) {
            double val = raw[i];
            if (val) {
                if (printed)
                    Sprint(sp, ", ");
                Sprint(sp, "\"%s\": %.0f", PCCounts::countName(op, i), val);
                printed = true;
            }
        }
        Sprint(sp, "}\n");

        pc = next;
    }

    ion::IonScriptCounts *ionCounts = script->getIonCounts();

    while (ionCounts) {
        Sprint(sp, "IonScript [%lu blocks]:\n", ionCounts->numBlocks());
        for (size_t i = 0; i < ionCounts->numBlocks(); i++) {
            const ion::IonBlockCounts &block = ionCounts->block(i);
            Sprint(sp, "BB #%lu [%05u]", block.id(), block.offset());
            for (size_t j = 0; j < block.numSuccessors(); j++)
                Sprint(sp, " -> #%lu", block.successor(j));
            Sprint(sp, " :: %llu hits %u instruction bytes %u spill bytes\n",
                   block.hitCount(), block.instructionBytes(), block.spillBytes());
            Sprint(sp, "%s\n", block.code());
        }
        ionCounts = ionCounts->previous();
    }
}





JS_FRIEND_API(JSBool)
js_DisassembleAtPC(JSContext *cx, JSScript *scriptArg, JSBool lines,
                   jsbytecode *pc, bool showAll, Sprinter *sp)
{
    AssertCanGC();
    RootedScript script(cx, scriptArg);

    jsbytecode *next, *end;
    unsigned len;

    if (showAll)
        Sprint(sp, "%s:%u\n", script->filename, script->lineno);

    if (pc != NULL)
        sp->put("    ");
    if (showAll)
        sp->put("sn stack ");
    sp->put("loc   ");
    if (lines)
        sp->put("line");
    sp->put("  op\n");

    if (pc != NULL)
        sp->put("    ");
    if (showAll)
        sp->put("-- ----- ");
    sp->put("----- ");
    if (lines)
        sp->put("----");
    sp->put("  --\n");

    next = script->code;
    end = next + script->length;
    while (next < end) {
        if (next == script->main())
            sp->put("main:\n");
        if (pc != NULL) {
            if (pc == next)
                sp->put("--> ");
            else
                sp->put("    ");
        }
        if (showAll) {
            jssrcnote *sn = js_GetSrcNote(cx, script, next);
            if (sn) {
                JS_ASSERT(!SN_IS_TERMINATOR(sn));
                jssrcnote *next = SN_NEXT(sn);
                while (!SN_IS_TERMINATOR(next) && SN_DELTA(next) == 0) {
                    Sprint(sp, "%02u\n    ", SN_TYPE(sn));
                    sn = next;
                    next = SN_NEXT(sn);
                }
                Sprint(sp, "%02u ", SN_TYPE(sn));
            }
            else
                sp->put("   ");
            if (script->hasAnalysis() && script->analysis()->maybeCode(next))
                Sprint(sp, "%05u ", script->analysis()->getCode(next).stackDepth);
            else
                sp->put("      ");
        }
        len = js_Disassemble1(cx, script, next, next - script->code, lines, sp);
        if (!len)
            return JS_FALSE;
        next += len;
    }
    return JS_TRUE;
}

JSBool
js_Disassemble(JSContext *cx, HandleScript script, JSBool lines, Sprinter *sp)
{
    return js_DisassembleAtPC(cx, script, lines, NULL, false, sp);
}

JS_FRIEND_API(JSBool)
js_DumpPC(JSContext *cx)
{
    js::gc::AutoSuppressGC suppressGC(cx);
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return JS_FALSE;
    RootedScript script(cx, cx->fp()->script());
    JSBool ok = js_DisassembleAtPC(cx, script, true, cx->regs().pc, false, &sprinter);
    fprintf(stdout, "%s", sprinter.string());
    return ok;
}

JS_FRIEND_API(JSBool)
js_DumpScript(JSContext *cx, JSScript *scriptArg)
{
    js::gc::AutoSuppressGC suppressGC(cx);
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return JS_FALSE;
    RootedScript script(cx, scriptArg);
    JSBool ok = js_Disassemble(cx, script, true, &sprinter);
    fprintf(stdout, "%s", sprinter.string());
    return ok;
}




JS_FRIEND_API(JSBool)
js_DumpScriptDepth(JSContext *cx, JSScript *scriptArg, jsbytecode *pc)
{
    js::gc::AutoSuppressGC suppressGC(cx);
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return JS_FALSE;
    RootedScript script(cx, scriptArg);
    JSBool ok = js_DisassembleAtPC(cx, script, true, pc, true, &sprinter);
    fprintf(stdout, "%s", sprinter.string());
    return ok;
}

static char *
QuoteString(Sprinter *sp, JSString *str, uint32_t quote);

static bool
ToDisassemblySource(JSContext *cx, jsval v, JSAutoByteString *bytes)
{
    AssertCanGC();
    if (JSVAL_IS_STRING(v)) {
        Sprinter sprinter(cx);
        if (!sprinter.init())
            return false;
        char *nbytes = QuoteString(&sprinter, JSVAL_TO_STRING(v), '"');
        if (!nbytes)
            return false;
        nbytes = JS_sprintf_append(NULL, "%s", nbytes);
        if (!nbytes)
            return false;
        bytes->initBytes(nbytes);
        return true;
    }

    if (cx->runtime->isHeapBusy() || cx->runtime->noGCOrAllocationCheck) {
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
            Shape::Range::AutoRooter root(cx, &r);

            while (!r.empty()) {
                Rooted<Shape*> shape(cx, &r.front());
                JSAtom *atom = JSID_IS_INT(shape->propid())
                               ? cx->names().empty
                               : JSID_TO_ATOM(shape->propid());

                JSAutoByteString bytes;
                if (!js_AtomToPrintableString(cx, atom, &bytes))
                    return false;

                r.popFront();
                source = JS_sprintf_append(source, "%s: %d%s",
                                           bytes.ptr(), shape->shortid(),
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
            return bytes->encodeLatin1(cx, str);
        }

        if (obj->isRegExp()) {
            JSString *source = obj->asRegExp().toString(cx);
            if (!source)
                return false;
            JS::Anchor<JSString *> anchor(source);
            return bytes->encodeLatin1(cx, source);
        }
    }

    return !!js_ValueToPrintable(cx, v, bytes, true);
}

unsigned
js_Disassemble1(JSContext *cx, HandleScript script, jsbytecode *pc,
                unsigned loc, JSBool lines, Sprinter *sp)
{
    AssertCanGC();
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

    switch (JOF_TYPE(cs->format)) {
      case JOF_BYTE:
          
          
          
          
          if (op == JSOP_TRY) {
              TryNoteArray *trynotes = script->trynotes();
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

      case JOF_JUMP: {
        ptrdiff_t off = GET_JUMP_OFFSET(pc);
        Sprint(sp, " %u (%+d)", loc + (int) off, (int) off);
        break;
      }

      case JOF_SCOPECOORD: {
        Value v = StringValue(ScopeCoordinateName(cx, script, pc));
        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, v, &bytes))
            return 0;
        ScopeCoordinate sc(pc);
        Sprint(sp, " %s (hops = %u, slot = %u)", bytes.ptr(), sc.hops, sc.slot);
        break;
      }

      case JOF_ATOM: {
        Value v = StringValue(script->getAtom(GET_UINT32_INDEX(pc)));
        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, v, &bytes))
            return 0;
        Sprint(sp, " %s", bytes.ptr());
        break;
      }

      case JOF_DOUBLE: {
        Value v = script->getConst(GET_UINT32_INDEX(pc));
        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, v, &bytes))
            return 0;
        Sprint(sp, " %s", bytes.ptr());
        break;
      }

      case JOF_OBJECT: {
        
        if (cx->compartment->activeAnalysis) {
            Sprint(sp, " object");
            break;
        }

        JSObject *obj = script->getObject(GET_UINT32_INDEX(pc));
        {
            JSAutoByteString bytes;
            if (!ToDisassemblySource(cx, ObjectValue(*obj), &bytes))
                return 0;
            Sprint(sp, " %s", bytes.ptr());
        }
        break;
      }

      case JOF_REGEXP: {
        JSObject *obj = script->getRegExp(GET_UINT32_INDEX(pc));
        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, ObjectValue(*obj), &bytes))
            return 0;
        Sprint(sp, " %s", bytes.ptr());
        break;
      }

      case JOF_TABLESWITCH:
      {
        int32_t i, low, high;

        ptrdiff_t off = GET_JUMP_OFFSET(pc);
        jsbytecode *pc2 = pc + JUMP_OFFSET_LEN;
        low = GET_JUMP_OFFSET(pc2);
        pc2 += JUMP_OFFSET_LEN;
        high = GET_JUMP_OFFSET(pc2);
        pc2 += JUMP_OFFSET_LEN;
        Sprint(sp, " defaultOffset %d low %d high %d", int(off), low, high);
        for (i = low; i <= high; i++) {
            off = GET_JUMP_OFFSET(pc2);
            Sprint(sp, "\n\t%d: %d", i, int(off));
            pc2 += JUMP_OFFSET_LEN;
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

      case JOF_SLOTOBJECT: {
        Sprint(sp, " %u", GET_SLOTNO(pc));
        JSObject *obj = script->getObject(GET_UINT32_INDEX(pc + SLOTNO_LEN));
        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, ObjectValue(*obj), &bytes))
            return 0;
        Sprint(sp, " %s", bytes.ptr());
        break;
      }

      {
        int i;

      case JOF_UINT16PAIR:
        i = (int)GET_UINT16(pc);
        Sprint(sp, " %d", i);
        pc += UINT16_LEN;
        

      case JOF_UINT16:
        i = (int)GET_UINT16(pc);
        goto print_int;

      case JOF_UINT24:
        JS_ASSERT(op == JSOP_UINT24 || op == JSOP_NEWARRAY || op == JSOP_INITELEM_ARRAY);
        i = (int)GET_UINT24(pc);
        goto print_int;

      case JOF_UINT8:
        i = GET_UINT8(pc);
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
    sp->put("\n");
    return len;
}

#endif 



const size_t Sprinter::DefaultSize = 64;

bool
Sprinter::realloc_(size_t newSize)
{
    JS_ASSERT(newSize > (size_t) offset);
    char *newBuf = (char *) context->realloc_(base, newSize);
    if (!newBuf)
        return false;
    base = newBuf;
    size = newSize;
    base[size - 1] = 0;
    return true;
}

Sprinter::Sprinter(JSContext *cx)
  : context(cx),
#ifdef DEBUG
    initialized(false),
#endif
    base(NULL), size(0), offset(0), reportedOOM(false)
{ }

Sprinter::~Sprinter()
{
#ifdef DEBUG
    if (initialized)
        checkInvariants();
#endif
    js_free(base);
}

bool
Sprinter::init()
{
    JS_ASSERT(!initialized);
    base = (char *) context->malloc_(DefaultSize);
    if (!base)
        return false;
#ifdef DEBUG
    initialized = true;
#endif
    *base = 0;
    size = DefaultSize;
    base[size - 1] = 0;
    return true;
}

void
Sprinter::checkInvariants() const
{
    JS_ASSERT(initialized);
    JS_ASSERT((size_t) offset < size);
    JS_ASSERT(base[size - 1] == 0);
}

const char *
Sprinter::string() const
{
    return base;
}

const char *
Sprinter::stringEnd() const
{
    return base + offset;
}

char *
Sprinter::stringAt(ptrdiff_t off) const
{
    JS_ASSERT(off >= 0 && (size_t) off < size);
    return base + off;
}

char &
Sprinter::operator[](size_t off)
{
    JS_ASSERT(off < size);
    return *(base + off);
}

bool
Sprinter::empty() const
{
    return *base == 0;
}

char *
Sprinter::reserve(size_t len)
{
    InvariantChecker ic(this);

    while (len + 1 > size - offset) { 
        if (!realloc_(size * 2))
            return NULL;
    }

    char *sb = base + offset;
    offset += len;
    return sb;
}

char *
Sprinter::reserveAndClear(size_t len)
{
    char *sb = reserve(len);
    if (sb)
        memset(sb, 0, len);
    return sb;
}

ptrdiff_t
Sprinter::put(const char *s, size_t len)
{
    InvariantChecker ic(this);

    const char *oldBase = base;
    const char *oldEnd = base + size;

    ptrdiff_t oldOffset = offset;
    char *bp = reserve(len);
    if (!bp)
        return -1;

    
    if (s >= oldBase && s < oldEnd) {
        
        if (base != oldBase)
            s = stringAt(s - oldBase);  
        memmove(bp, s, len);
    } else {
        js_memcpy(bp, s, len);
    }

    bp[len] = 0;
    return oldOffset;
}

ptrdiff_t
Sprinter::put(const char *s)
{
    return put(s, strlen(s));
}

ptrdiff_t
Sprinter::putString(JSString *s)
{
    InvariantChecker ic(this);

    size_t length = s->length();
    const jschar *chars = s->getChars(context);
    if (!chars)
        return -1;

    size_t size = length;
    if (size == (size_t) -1)
        return -1;

    ptrdiff_t oldOffset = offset;
    char *buffer = reserve(size);
    if (!buffer)
        return -1;
    DeflateStringToBuffer(context, chars, length, buffer, &size);
    buffer[size] = 0;

    return oldOffset;
}

int
Sprinter::printf(const char *fmt, ...)
{
    InvariantChecker ic(this);

    do {
        va_list va;
        va_start(va, fmt);
        int i = vsnprintf(base + offset, size - offset, fmt, va);
        va_end(va);

        if (i > -1 && (size_t) i < size - offset) {
            offset += i;
            return i;
        }
    } while (realloc_(size * 2));

    return -1;
}

void
Sprinter::setOffset(const char *end)
{
    JS_ASSERT(end >= base && end < base + size);
    offset = end - base;
}

void
Sprinter::setOffset(ptrdiff_t off)
{
    JS_ASSERT(off >= 0 && (size_t) off < size);
    offset = off;
}

ptrdiff_t
Sprinter::getOffset() const
{
    return offset;
}

ptrdiff_t
Sprinter::getOffsetOf(const char *string) const
{
    JS_ASSERT(string >= base && string < base + size);
    return string - base;
}

void
Sprinter::reportOutOfMemory() {
    if (reportedOOM)
        return;
    js_ReportOutOfMemory(context);
    reportedOOM = true;
}

bool
Sprinter::hadOutOfMemory() const {
    return reportedOOM;
}

ptrdiff_t
js::Sprint(Sprinter *sp, const char *format, ...)
{
    va_list ap;
    char *bp;
    ptrdiff_t offset;

    va_start(ap, format);
    bp = JS_vsmprintf(format, ap);      
    va_end(ap);
    if (!bp) {
        sp->reportOutOfMemory();
        return -1;
    }
    offset = sp->put(bp);
    js_free(bp);
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
    '\0'
};

#define DONT_ESCAPE     0x10000

static char *
QuoteString(Sprinter *sp, JSString *str, uint32_t quote)
{
    
    JSBool dontEscape = (quote & DONT_ESCAPE) != 0;
    jschar qc = (jschar) quote;
    ptrdiff_t offset = sp->getOffset();
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

        {
            ptrdiff_t len = t - s;
            ptrdiff_t base = sp->getOffset();
            char *bp = sp->reserve(len);
            if (!bp)
                return NULL;

            for (ptrdiff_t i = 0; i < len; ++i)
                (*sp)[base + i] = (char) *s++;
            (*sp)[base + len] = 0;
        }

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

    



    if (offset == sp->getOffset() && Sprint(sp, "") < 0)
        return NULL;

    return sp->stringAt(offset);
}

JSString *
js_QuoteString(JSContext *cx, JSString *str, jschar quote)
{
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return NULL;
    char *bytes = QuoteString(&sprinter, str, quote);
    JSString *escstr = bytes ? JS_NewStringCopyZ(cx, bytes) : NULL;
    return escstr;
}



static int ReconstructPCStack(JSContext*, JSScript*, jsbytecode*, jsbytecode**);

static unsigned
StackDepth(JSScript *script)
{
    return script->nslots - script->nfixed;
}

static JSObject *
GetBlockChainAtPC(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    jsbytecode *start = script->main();

    JS_ASSERT(pc >= start && pc < script->code + script->length);

    JSObject *blockChain = NULL;
    for (jsbytecode *p = start; p < pc; p += GetBytecodeLength(p)) {
        JSOp op = JSOp(*p);

        switch (op) {
          case JSOP_ENTERBLOCK:
          case JSOP_ENTERLET0:
          case JSOP_ENTERLET1: {
            JSObject *child = script->getObject(p);
            JS_ASSERT_IF(blockChain, child->asBlock().stackDepth() >= blockChain->asBlock().stackDepth());
            blockChain = child;
            break;
          }
          case JSOP_LEAVEBLOCK:
          case JSOP_LEAVEBLOCKEXPR:
          case JSOP_LEAVEFORLETIN: {
            
            
            
            
            jssrcnote *sn = js_GetSrcNote(cx, script, p);
            if (!(sn && SN_TYPE(sn) == SRC_HIDDEN)) {
                JS_ASSERT(blockChain);
                blockChain = blockChain->asStaticBlock().enclosingBlock();
                JS_ASSERT_IF(blockChain, blockChain->isBlock());
            }
            break;
          }
          default:
            break;
        }
    }

    return blockChain;
}

class PCStack
{
    jsbytecode **stack;
    int depth_;

  public:
    PCStack() : stack(NULL), depth_(0) {}
    ~PCStack();
    bool init(JSContext *cx, JSScript *script, jsbytecode *pc);
    int depth() const { return depth_; }
    jsbytecode *operator[](int i) const;
};

PCStack::~PCStack()
{
    js_free(stack);
}

bool
PCStack::init(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    stack = cx->pod_malloc<jsbytecode*>(StackDepth(script));
    if (!stack)
        return false;
    depth_ = ReconstructPCStack(cx, script, pc, stack);
    JS_ASSERT(depth_ >= 0);
    return true;
}


jsbytecode *
PCStack::operator[](int i) const
{
    if (i < 0) {
        i += depth_;
        JS_ASSERT(i >= 0);
    }
    JS_ASSERT(i < depth_);
    return stack[i];
}
































struct ExpressionDecompiler
{
    JSContext *cx;
    StackFrame *fp;
    RootedScript script;
    RootedFunction fun;
    BindingVector *localNames;
    Sprinter sprinter;

    ExpressionDecompiler(JSContext *cx, JSScript *script, JSFunction *fun)
        : cx(cx),
          script(cx, script),
          fun(cx, fun),
          localNames(NULL),
          sprinter(cx)
    {}
    ~ExpressionDecompiler();
    bool init();
    bool decompilePC(jsbytecode *pc);
    JSAtom *getVar(unsigned slot);
    JSAtom *getArg(unsigned slot);
    JSAtom *findLetVar(jsbytecode *pc, unsigned depth);
    JSAtom *loadAtom(jsbytecode *pc);
    bool quote(JSString *s, uint32_t quote);
    bool write(const char *s);
    bool write(JSString *str);
    bool getOutput(char **out);
};

bool
ExpressionDecompiler::decompilePC(jsbytecode *pc)
{
    JS_ASSERT(script->code <= pc && pc < script->code + script->length);

    PCStack pcstack;
    if (!pcstack.init(cx, script, pc))
        return false;

    JSOp op = (JSOp)*pc;

    if (const char *token = CodeToken[op]) {
        
        switch (js_CodeSpec[op].nuses) {
          case 2: {
            jssrcnote *sn = js_GetSrcNote(cx, script, pc);
            if (!sn || SN_TYPE(sn) != SRC_ASSIGNOP)
                return write("(") &&
                       decompilePC(pcstack[-2]) &&
                       write(" ") &&
                       write(token) &&
                       write(" ") &&
                       decompilePC(pcstack[-1]) &&
                       write(")");
            break;
          }
          case 1:
            return write(token) &&
                   write("(") &&
                   decompilePC(pcstack[-1]) &&
                   write(")");
          default:
            break;
        }
    }

    switch (op) {
      case JSOP_GETGNAME:
      case JSOP_CALLGNAME:
      case JSOP_NAME:
      case JSOP_CALLNAME:
        return write(loadAtom(pc));
      case JSOP_GETARG:
      case JSOP_CALLARG: {
        unsigned slot = GET_ARGNO(pc);
        JSAtom *atom = getArg(slot);
        return write(atom);
      }
      case JSOP_GETLOCAL:
      case JSOP_CALLLOCAL: {
        unsigned i = GET_SLOTNO(pc);
        JSAtom *atom;
        if (i >= script->nfixed) {
            i -= script->nfixed;
            JS_ASSERT(i < unsigned(pcstack.depth()));
            atom = findLetVar(pc, i);
            if (!atom)
                return decompilePC(pcstack[i]); 
        } else {
            atom = getVar(i);
        }
        JS_ASSERT(atom);
        return write(atom);
      }
      case JSOP_CALLALIASEDVAR:
      case JSOP_GETALIASEDVAR: {
        JSAtom *atom = ScopeCoordinateName(cx, script, pc);
        JS_ASSERT(atom);
        return write(atom);
      }
      case JSOP_LENGTH:
      case JSOP_GETPROP:
      case JSOP_CALLPROP: {
        RootedAtom prop(cx, (op == JSOP_LENGTH) ? cx->names().length : loadAtom(pc));
        if (!decompilePC(pcstack[-1]))
            return false;
        if (IsIdentifier(prop)) {
            return write(".") &&
                   quote(prop, '\0');
        }
        return write("[") &&
               quote(prop, '\'') &&
               write("]");
      }
      case JSOP_GETELEM:
      case JSOP_CALLELEM:
        return decompilePC(pcstack[-2]) &&
               write("[") &&
               decompilePC(pcstack[-1]) &&
               write("]");
      case JSOP_NULL:
        return write(js_null_str);
      case JSOP_TRUE:
        return write(js_true_str);
      case JSOP_FALSE:
        return write(js_false_str);
      case JSOP_ZERO:
      case JSOP_ONE:
      case JSOP_INT8:
      case JSOP_UINT16:
      case JSOP_UINT24:
      case JSOP_INT32: {
        int32_t i;
        switch (op) {
          case JSOP_ZERO:
            i = 0;
            break;
          case JSOP_ONE:
            i = 1;
            break;
          case JSOP_INT8:
            i = GET_INT8(pc);
            break;
          case JSOP_UINT16:
            i = GET_UINT16(pc);
            break;
          case JSOP_UINT24:
            i = GET_UINT24(pc);
            break;
          case JSOP_INT32:
            i = GET_INT32(pc);
            break;
          default:
            JS_NOT_REACHED("wat?");
        }
        return sprinter.printf("%d", i) >= 0;
      }
      case JSOP_STRING:
        return quote(loadAtom(pc), '"');
      case JSOP_UNDEFINED:
        return write(js_undefined_str);
      case JSOP_THIS:
        
        
        return write(js_this_str);
      case JSOP_CALL:
      case JSOP_FUNCALL:
        return decompilePC(pcstack[-int32_t(GET_ARGC(pc) + 2)]) &&
               write("(...)");
      case JSOP_NEWARRAY:
        return write("[]");
      case JSOP_REGEXP:
      case JSOP_OBJECT: {
        JSObject *obj = (op == JSOP_REGEXP)
                        ? script->getRegExp(GET_UINT32_INDEX(pc))
                        : script->getObject(GET_UINT32_INDEX(pc));
        JSString *str = ValueToSource(cx, ObjectValue(*obj));
        if (!str)
            return false;
        return write(str);
      }
      default:
        break;
    }
    return write("(intermediate value)");
}

ExpressionDecompiler::~ExpressionDecompiler()
{
    js_delete<BindingVector>(localNames);
}

bool
ExpressionDecompiler::init()
{
    assertSameCompartment(cx, script);

    if (!sprinter.init())
        return false;

    localNames = cx->new_<BindingVector>(cx);
    if (!localNames)
        return false;
    RootedScript script_(cx, script);
    if (!FillBindingVector(script_, localNames))
        return false;

    return true;
}

bool
ExpressionDecompiler::write(const char *s)
{
    return sprinter.put(s) >= 0;
}

bool
ExpressionDecompiler::write(JSString *str)
{
    return sprinter.putString(str) >= 0;
}

bool
ExpressionDecompiler::quote(JSString *s, uint32_t quote)
{
    return QuoteString(&sprinter, s, quote) >= 0;
}

JSAtom *
ExpressionDecompiler::loadAtom(jsbytecode *pc)
{
    return script->getAtom(GET_UINT32_INDEX(pc));
}

JSAtom *
ExpressionDecompiler::findLetVar(jsbytecode *pc, unsigned depth)
{
    if (script->hasObjects()) {
        JSObject *chain = GetBlockChainAtPC(cx, script, pc);
        if (!chain)
            return NULL;
        JS_ASSERT(chain->isBlock());
        do {
            BlockObject &block = chain->asBlock();
            uint32_t blockDepth = block.stackDepth();
            uint32_t blockCount = block.slotCount();
            if (uint32_t(depth - blockDepth) < uint32_t(blockCount)) {
                for (Shape::Range r(block.lastProperty()); !r.empty(); r.popFront()) {
                    const Shape &shape = r.front();
                    if (shape.shortid() == int(depth - blockDepth))
                        return JSID_TO_ATOM(shape.propid());
                }
            }
            chain = chain->getParent();
        } while (chain && chain->isBlock());
    }
    return NULL;
}

JSAtom *
ExpressionDecompiler::getArg(unsigned slot)
{
    JS_ASSERT(fun);
    JS_ASSERT(slot < script->bindings.count());
    return (*localNames)[slot].name();
}

JSAtom *
ExpressionDecompiler::getVar(unsigned slot)
{
    JS_ASSERT(fun);
    slot += fun->nargs;
    JS_ASSERT(slot < script->bindings.count());
    return (*localNames)[slot].name();
}

bool
ExpressionDecompiler::getOutput(char **res)
{
    ptrdiff_t len = sprinter.stringEnd() - sprinter.stringAt(0);
    *res = cx->pod_malloc<char>(len + 1);
    if (!*res)
        return false;
    js_memcpy(*res, sprinter.stringAt(0), len);
    (*res)[len] = 0;
    return true;
}

static bool
FindStartPC(JSContext *cx, ScriptFrameIter &iter, int spindex, int skipStackHits, Value v,
            jsbytecode **valuepc)
{
    jsbytecode *current = *valuepc;

    if (spindex == JSDVG_IGNORE_STACK)
        return true;

    








    if (iter.isIon() || iter.interpFrame()->jitRevisedStack())
        return true;

    *valuepc = NULL;

    PCStack pcstack;
    if (!pcstack.init(cx, iter.script(), current))
        return false;

    if (spindex < 0 && spindex + pcstack.depth() < 0)
        spindex = JSDVG_SEARCH_STACK;

    if (spindex == JSDVG_SEARCH_STACK) {
        size_t index = iter.numFrameSlots();
        JS_ASSERT(index >= size_t(pcstack.depth()));

        
        
        
        int stackHits = 0;
        Value s;
        do {
            if (!index)
                return true;
            s = iter.frameSlotValue(--index);
        } while (s != v || stackHits++ != skipStackHits);

        
        
        if (index < size_t(pcstack.depth()))
            *valuepc = pcstack[index];
        else
            *valuepc = current;
    } else {
        *valuepc = pcstack[spindex];
    }
    return true;
}

static bool
DecompileExpressionFromStack(JSContext *cx, int spindex, int skipStackHits, HandleValue v, char **res)
{
    AssertCanGC();
    JS_ASSERT(spindex < 0 ||
              spindex == JSDVG_IGNORE_STACK ||
              spindex == JSDVG_SEARCH_STACK);

    *res = NULL;

#ifdef JS_MORE_DETERMINISTIC
    




    return true;
#endif

    ScriptFrameIter frameIter(cx);

    if (frameIter.done())
        return true;

    RootedScript script(cx, frameIter.script());
    AutoCompartment ac(cx, &script->global());
    jsbytecode *valuepc = frameIter.pc();
    RootedFunction fun(cx, frameIter.isFunctionFrame()
                           ? frameIter.callee()
                           : NULL);

    JS_ASSERT(script->code <= valuepc && valuepc < script->code + script->length);

    
    if (valuepc < script->main())
        return true;

    if (!FindStartPC(cx, frameIter, spindex, skipStackHits, v, &valuepc))
        return false;
    if (!valuepc)
        return true;

    ExpressionDecompiler ed(cx, script, fun);
    if (!ed.init())
        return false;
    if (!ed.decompilePC(valuepc))
        return false;

    return ed.getOutput(res);
}

char *
js::DecompileValueGenerator(JSContext *cx, int spindex, HandleValue v,
                            HandleString fallbackArg, int skipStackHits)
{
    AssertCanGC();
    RootedString fallback(cx, fallbackArg);
    {
        char *result;
        if (!DecompileExpressionFromStack(cx, spindex, skipStackHits, v, &result))
            return NULL;
        if (result) {
            if (strcmp(result, "(intermediate value)"))
                return result;
            js_free(result);
        }
    }
    if (!fallback) {
        if (v.isUndefined())
            return JS_strdup(cx, js_undefined_str); 
        fallback = ValueToSource(cx, v);
        if (!fallback)
            return NULL;
    }

    Rooted<JSLinearString *> linear(cx, fallback->ensureLinear(cx));
    if (!linear)
        return NULL;
    TwoByteChars tbchars(linear->chars(), linear->length());
    return LossyTwoByteCharsToNewLatin1CharsZ(cx, tbchars).c_str();
}

static bool
DecompileArgumentFromStack(JSContext *cx, int formalIndex, char **res)
{
    JS_ASSERT(formalIndex >= 0);

    *res = NULL;

#ifdef JS_MORE_DETERMINISTIC
    
    return true;
#endif

    



    StackIter frameIter(cx);
    while (!frameIter.done() && !frameIter.isScript())
        ++frameIter;
    JS_ASSERT(!frameIter.done());

    



    ++frameIter;

    




    if (frameIter.done() || frameIter.poppedCallDuringSettle() || !frameIter.isScript())
        return true;

    RootedScript script(cx, frameIter.script());
    AutoCompartment ac(cx, &script->global());
    jsbytecode *current = frameIter.pc();
    RootedFunction fun(cx, frameIter.isFunctionFrame()
                       ? frameIter.callee()
                       : NULL);

    JS_ASSERT(script->code <= current && current < script->code + script->length);

    if (current < script->main())
        return true;

    PCStack pcStack;
    if (!pcStack.init(cx, script, current))
        return false;

    int formalStackIndex = pcStack.depth() - GET_ARGC(current) + formalIndex;
    JS_ASSERT(formalStackIndex >= 0);
    if (formalStackIndex >= pcStack.depth())
        return true;

    ExpressionDecompiler ed(cx, script, fun);
    if (!ed.init())
        return false;
    if (!ed.decompilePC(pcStack[formalStackIndex]))
        return false;

    return ed.getOutput(res);
}

char *
js::DecompileArgument(JSContext *cx, int formalIndex, HandleValue v)
{
    AssertCanGC();
    {
        char *result;
        if (!DecompileArgumentFromStack(cx, formalIndex, &result))
            return NULL;
        if (result) {
            if (strcmp(result, "(intermediate value)"))
                return result;
            js_free(result);
        }
    }
    if (v.isUndefined())
        return JS_strdup(cx, js_undefined_str); 
    RootedString fallback(cx, ValueToSource(cx, v));
    if (!fallback)
        return NULL;

    Rooted<JSLinearString *> linear(cx, fallback->ensureLinear(cx));
    if (!linear)
        return NULL;
    return LossyTwoByteCharsToNewLatin1CharsZ(cx, linear->range()).c_str();
}

unsigned
js_ReconstructStackDepth(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    return ReconstructPCStack(cx, script, pc, NULL);
}

#define LOCAL_ASSERT_CUSTOM(expr, BAD_EXIT)                                   \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(expr);                                                      \
        if (!(expr)) { BAD_EXIT; }                                            \
    JS_END_MACRO
#define LOCAL_ASSERT_RV(expr, rv)                                             \
    LOCAL_ASSERT_CUSTOM(expr, return (rv))
#define LOCAL_ASSERT(expr)      LOCAL_ASSERT_RV(expr, -1);

static int
SimulateOp(JSScript *script, JSOp op, const JSCodeSpec *cs,
           jsbytecode *pc, jsbytecode **pcstack, unsigned &pcdepth)
{
    if (cs->format & JOF_DECOMPOSE)
        return pcdepth;

    unsigned nuses = StackUses(script, pc);
    unsigned ndefs = StackDefs(script, pc);
    LOCAL_ASSERT(pcdepth >= nuses);
    pcdepth -= nuses;
    LOCAL_ASSERT(pcdepth + ndefs <= StackDepth(script));

    




    switch (op) {
      default:
        if (pcstack) {
            for (unsigned i = 0; i != ndefs; ++i)
                pcstack[pcdepth + i] = pc;
        }
        break;

      case JSOP_CASE:
        
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


static void
AssertPCDepth(JSScript *script, jsbytecode *pc, unsigned pcdepth) {
    





    JS_ASSERT_IF(script->hasAnalysis() && script->analysis()->maybeCode(pc),
                 script->analysis()->getCode(pc).stackDepth == pcdepth);
}

static int
ReconstructPCStack(JSContext *cx, JSScript *script, jsbytecode *target, jsbytecode **pcstack)
{
    








































    LOCAL_ASSERT(script->code <= target && target < script->code + script->length);

    



    unsigned hpcdepth = unsigned(-1);

    



    unsigned cpcdepth = unsigned(-1);

    jsbytecode *pc;
    unsigned pcdepth;
    ptrdiff_t oplen;
    for (pc = script->code, pcdepth = 0; ; pc += oplen) {
        JSOp op = JSOp(*pc);
        oplen = GetBytecodeLength(pc);
        const JSCodeSpec *cs = &js_CodeSpec[op];
        jssrcnote *sn = js_GetSrcNote(cx, script, pc);

        
























































        bool exitPath =
            op == JSOP_GOTO ||
            op == JSOP_RETRVAL ||
            op == JSOP_THROW;

        bool isConditionalCatchExit = false;

        if (sn && SN_TYPE(sn) == SRC_HIDDEN) {
            
            isConditionalCatchExit = op == JSOP_GOTO;
            if (hpcdepth == unsigned(-1))
                hpcdepth = pcdepth;
        } else if (!exitPath) {
            hpcdepth = unsigned(-1);
        }


        






























        if (op == JSOP_LEAVEBLOCK && sn && SN_TYPE(sn) == SRC_CATCH) {
            LOCAL_ASSERT(cpcdepth == unsigned(-1));
            cpcdepth = pcdepth;
        } else if (sn && SN_TYPE(sn) == SRC_HIDDEN &&
                   (op == JSOP_THROW || op == JSOP_THROWING))
        {
            





            LOCAL_ASSERT(cpcdepth != unsigned(-1));
            pcdepth = cpcdepth + 1;
            cpcdepth = unsigned(-1);
        } else if (!(op == JSOP_GOTO && sn && SN_TYPE(sn) == SRC_HIDDEN) &&
                   !(op == JSOP_GOSUB && cpcdepth != unsigned(-1)))
        {
            













            if (cpcdepth != unsigned(-1))
                LOCAL_ASSERT(op == JSOP_NOP || op == JSOP_FINALLY);
            cpcdepth = unsigned(-1);
        }

        
        AssertPCDepth(script, pc, pcdepth);
        if (pc >= target)
            break;

        
        if (SimulateOp(script, op, cs, pc, pcstack, pcdepth) < 0)
            return -1;

        

        





        if (exitPath && hpcdepth != unsigned(-1)) {
            pcdepth = hpcdepth;

            
            if (!isConditionalCatchExit)
                hpcdepth = unsigned(-1);
        }

        












        if (sn && SN_TYPE(sn) == SRC_COND) {
            ptrdiff_t jmplen = GET_JUMP_OFFSET(pc);
            if (pc + jmplen <= target) {
                
                oplen = jmplen;
            }
        }
    }
    LOCAL_ASSERT(pc == target);
    AssertPCDepth(script, pc, pcdepth);
    return pcdepth;
}

#undef LOCAL_ASSERT
#undef LOCAL_ASSERT_RV

bool
js::CallResultEscapes(jsbytecode *pc)
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
js::IsValidBytecodeOffset(JSContext *cx, JSScript *script, size_t offset)
{
    
    for (BytecodeRange r(cx, script); !r.empty(); r.popFront()) {
        size_t here = r.frontOffset();
        if (here >= offset)
            return here == offset;
    }
    return false;
}

JS_FRIEND_API(size_t)
js::GetPCCountScriptCount(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (!rt->scriptAndCountsVector)
        return 0;

    return rt->scriptAndCountsVector->length();
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
js::GetPCCountScriptSummary(JSContext *cx, size_t index)
{
    JSRuntime *rt = cx->runtime;

    if (!rt->scriptAndCountsVector || index >= rt->scriptAndCountsVector->length()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BUFFER_TOO_SMALL);
        return NULL;
    }

    const ScriptAndCounts &sac = (*rt->scriptAndCountsVector)[index];
    RootedScript script(cx, sac.script);

    




    StringBuffer buf(cx);

    buf.append('{');

    AppendJSONProperty(buf, "file", NO_COMMA);
    JSString *str = JS_NewStringCopyZ(cx, script->filename);
    if (!str || !(str = ValueToSource(cx, StringValue(str))))
        return NULL;
    buf.append(str);

    AppendJSONProperty(buf, "line");
    NumberValueToStringBuffer(cx, Int32Value(script->lineno), buf);

    if (script->function()) {
        JSAtom *atom = script->function()->displayAtom();
        if (atom) {
            AppendJSONProperty(buf, "name");
            if (!(str = ValueToSource(cx, StringValue(atom))))
                return NULL;
            buf.append(str);
        }
    }

    double baseTotals[PCCounts::BASE_LIMIT] = {0.0};
    double accessTotals[PCCounts::ACCESS_LIMIT - PCCounts::BASE_LIMIT] = {0.0};
    double elementTotals[PCCounts::ELEM_LIMIT - PCCounts::ACCESS_LIMIT] = {0.0};
    double propertyTotals[PCCounts::PROP_LIMIT - PCCounts::ACCESS_LIMIT] = {0.0};
    double arithTotals[PCCounts::ARITH_LIMIT - PCCounts::BASE_LIMIT] = {0.0};

    for (unsigned i = 0; i < script->length; i++) {
        PCCounts &counts = sac.getPCCounts(script->code + i);
        if (!counts)
            continue;

        JSOp op = (JSOp)script->code[i];
        unsigned numCounts = PCCounts::numCounts(op);

        for (unsigned j = 0; j < numCounts; j++) {
            double value = counts.get(j);
            if (j < PCCounts::BASE_LIMIT) {
                baseTotals[j] += value;
            } else if (PCCounts::accessOp(op)) {
                if (j < PCCounts::ACCESS_LIMIT)
                    accessTotals[j - PCCounts::BASE_LIMIT] += value;
                else if (PCCounts::elementOp(op))
                    elementTotals[j - PCCounts::ACCESS_LIMIT] += value;
                else if (PCCounts::propertyOp(op))
                    propertyTotals[j - PCCounts::ACCESS_LIMIT] += value;
                else
                    JS_NOT_REACHED("Bad opcode");
            } else if (PCCounts::arithOp(op)) {
                arithTotals[j - PCCounts::BASE_LIMIT] += value;
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

    uint64_t ionActivity = 0;
    ion::IonScriptCounts *ionCounts = sac.getIonCounts();
    while (ionCounts) {
        for (size_t i = 0; i < ionCounts->numBlocks(); i++)
            ionActivity += ionCounts->block(i).hitCount();
        ionCounts = ionCounts->previous();
    }
    if (ionActivity) {
        AppendJSONProperty(buf, "ion", comma);
        NumberValueToStringBuffer(cx, DoubleValue(ionActivity), buf);
    }

    buf.append('}');
    buf.append('}');

    if (cx->isExceptionPending())
        return NULL;

    return buf.finishString();
}

static bool
GetPCCountJSON(JSContext *cx, const ScriptAndCounts &sac, StringBuffer &buf)
{
    RootedScript script(cx, sac.script);

    buf.append('{');
    AppendJSONProperty(buf, "text", NO_COMMA);

    JSString *str = JS_DecompileScript(cx, script, NULL, 0);
    if (!str || !(str = ValueToSource(cx, StringValue(str))))
        return false;

    buf.append(str);

    AppendJSONProperty(buf, "line");
    NumberValueToStringBuffer(cx, Int32Value(script->lineno), buf);

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

        {
            ExpressionDecompiler ed(cx, script, script->function());
            if (!ed.init())
                return false;
            if (!ed.decompilePC(pc))
                return false;
            char *text;
            if (!ed.getOutput(&text))
                return false;
            AppendJSONProperty(buf, "text");
            JSString *str = JS_NewStringCopyZ(cx, text);
            js_free(text);
            if (!str || !(str = ValueToSource(cx, StringValue(str))))
                return false;
            buf.append(str);
        }

        PCCounts &counts = sac.getPCCounts(pc);
        unsigned numCounts = PCCounts::numCounts(op);

        AppendJSONProperty(buf, "counts");
        buf.append('{');

        MaybeComma comma = NO_COMMA;
        for (unsigned i = 0; i < numCounts; i++) {
            double value = counts.get(i);
            if (value > 0) {
                AppendJSONProperty(buf, PCCounts::countName(op, i), comma);
                comma = COMMA;
                NumberValueToStringBuffer(cx, DoubleValue(value), buf);
            }
        }

        buf.append('}');
        buf.append('}');
    }

    buf.append(']');

    ion::IonScriptCounts *ionCounts = sac.getIonCounts();
    if (ionCounts) {
        AppendJSONProperty(buf, "ion");
        buf.append('[');
        bool comma = false;
        while (ionCounts) {
            if (comma)
                buf.append(',');
            comma = true;

            buf.append('[');
            for (size_t i = 0; i < ionCounts->numBlocks(); i++) {
                if (i)
                    buf.append(',');
                const ion::IonBlockCounts &block = ionCounts->block(i);

                buf.append('{');
                AppendJSONProperty(buf, "id", NO_COMMA);
                NumberValueToStringBuffer(cx, Int32Value(block.id()), buf);
                AppendJSONProperty(buf, "offset");
                NumberValueToStringBuffer(cx, Int32Value(block.offset()), buf);
                AppendJSONProperty(buf, "successors");
                buf.append('[');
                for (size_t j = 0; j < block.numSuccessors(); j++) {
                    if (j)
                        buf.append(',');
                    NumberValueToStringBuffer(cx, Int32Value(block.successor(j)), buf);
                }
                buf.append(']');
                AppendJSONProperty(buf, "hits");
                NumberValueToStringBuffer(cx, DoubleValue(block.hitCount()), buf);

                AppendJSONProperty(buf, "code");
                JSString *str = JS_NewStringCopyZ(cx, block.code());
                if (!str || !(str = ValueToSource(cx, StringValue(str))))
                    return false;
                buf.append(str);

                AppendJSONProperty(buf, "instructionBytes");
                NumberValueToStringBuffer(cx, Int32Value(block.instructionBytes()), buf);

                AppendJSONProperty(buf, "spillBytes");
                NumberValueToStringBuffer(cx, Int32Value(block.spillBytes()), buf);

                buf.append('}');
            }
            buf.append(']');

            ionCounts = ionCounts->previous();
        }
        buf.append(']');
    }

    buf.append('}');

    return !cx->isExceptionPending();
}

JS_FRIEND_API(JSString *)
js::GetPCCountScriptContents(JSContext *cx, size_t index)
{
    JSRuntime *rt = cx->runtime;

    if (!rt->scriptAndCountsVector || index >= rt->scriptAndCountsVector->length()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BUFFER_TOO_SMALL);
        return NULL;
    }

    const ScriptAndCounts &sac = (*rt->scriptAndCountsVector)[index];
    JSScript *script = sac.script;

    StringBuffer buf(cx);

    if (!script->function() && !script->compileAndGo)
        return buf.finishString();

    {
        AutoCompartment ac(cx, &script->global());
        if (!GetPCCountJSON(cx, sac, buf))
            return NULL;
    }

    return buf.finishString();
}
