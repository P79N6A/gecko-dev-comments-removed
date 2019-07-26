









#include "jsopcodeinlines.h"

#include "mozilla/Util.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "jsanalyze.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jsautooplen.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsfun.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsprf.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jsutil.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/SourceNotes.h"
#include "js/CharacterEncoding.h"
#include "vm/ScopeObject.h"
#include "vm/Shape.h"
#include "vm/StringBuffer.h"

#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::gc;

using js::frontend::IsIdentifier;
using mozilla::ArrayLength;




JS_STATIC_ASSERT(sizeof(uint32_t) * JS_BITS_PER_BYTE >= INDEX_LIMIT_LOG2 + 1);


#define OPDEF(op,val,name,token,length,nuses,ndefs,format)               \
    JS_STATIC_ASSERT(op##_LENGTH == length);
#include "jsopcode.tbl"
#undef OPDEF

const JSCodeSpec js_CodeSpec[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,format) \
    {length,nuses,ndefs,format},
#include "jsopcode.tbl"
#undef OPDEF
};

const unsigned js_NumCodeSpecs = JS_ARRAY_LENGTH(js_CodeSpec);





static const char * const CodeToken[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,format) \
    token,
#include "jsopcode.tbl"
#undef OPDEF
};





const char * const js_CodeName[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,format) \
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
        MOZ_ASSUME_UNREACHABLE("Unexpected op");
    }
}

static uint32_t
NumBlockSlots(JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(*pc == JSOP_ENTERBLOCK ||
              *pc == JSOP_ENTERLET0 || *pc == JSOP_ENTERLET1 || *pc == JSOP_ENTERLET2);
    JS_STATIC_ASSERT(JSOP_ENTERBLOCK_LENGTH == JSOP_ENTERLET0_LENGTH);
    JS_STATIC_ASSERT(JSOP_ENTERBLOCK_LENGTH == JSOP_ENTERLET1_LENGTH);
    JS_STATIC_ASSERT(JSOP_ENTERBLOCK_LENGTH == JSOP_ENTERLET2_LENGTH);

    return script->getObject(GET_UINT32_INDEX(pc))->as<StaticBlockObject>().propertyCountForCompilation();
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
      case JSOP_ENTERLET2:
        return NumBlockSlots(script, pc) + 2;
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
    if (op == JSOP_ENTERLET1)
        return n + 1;
    if (op == JSOP_ENTERLET2)
        return n + 2;
    return n;
}

static const char * const countBaseNames[] = {
    "interp"
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) == PCCounts::BASE_LIMIT);

static const char * const countAccessNames[] = {
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

static const char * const countElementNames[] = {
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

static const char * const countPropertyNames[] = {
    "prop_static",
    "prop_definite",
    "prop_other"
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(countBaseNames) +
                 JS_ARRAY_LENGTH(countAccessNames) +
                 JS_ARRAY_LENGTH(countPropertyNames) == PCCounts::PROP_LIMIT);

static const char * const countArithNames[] = {
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
        MOZ_ASSUME_UNREACHABLE("bad op");
    }

    if (arithOp(op))
        return countArithNames[which - BASE_LIMIT];

    MOZ_ASSUME_UNREACHABLE("bad op");
}

#ifdef JS_ION
void
js::DumpIonScriptCounts(Sprinter *sp, jit::IonScriptCounts *ionCounts)
{
    Sprint(sp, "IonScript [%lu blocks]:\n", ionCounts->numBlocks());
    for (size_t i = 0; i < ionCounts->numBlocks(); i++) {
        const jit::IonBlockCounts &block = ionCounts->block(i);
        if (block.hitCount() < 10)
            continue;
        Sprint(sp, "BB #%lu [%05u]", block.id(), block.offset());
        for (size_t j = 0; j < block.numSuccessors(); j++)
            Sprint(sp, " -> #%lu", block.successor(j));
        Sprint(sp, " :: %llu hits %u instruction bytes %u spill bytes\n",
               block.hitCount(), block.instructionBytes(), block.spillBytes());
        Sprint(sp, "%s\n", block.code());
    }
}
#endif

void
js_DumpPCCounts(JSContext *cx, HandleScript script, js::Sprinter *sp)
{
    JS_ASSERT(script->hasScriptCounts);

#ifdef DEBUG
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
#endif

#ifdef JS_ION
    jit::IonScriptCounts *ionCounts = script->getIonCounts();

    while (ionCounts) {
        DumpIonScriptCounts(sp, ionCounts);
        ionCounts = ionCounts->previous();
    }
#endif
}






static void
AssertStackDepth(JSScript *script, uint32_t offset, uint32_t stackDepth) {
    





    JS_ASSERT_IF(script->hasAnalysis() && script->analysis()->maybeCode(offset),
                 script->analysis()->getCode(offset).stackDepth == stackDepth);
}

namespace {

class BytecodeParser
{
    class Bytecode
    {
      public:
        Bytecode() { mozilla::PodZero(this); }

        
        
        bool parsed : 1;

        
        uint32_t stackDepth;

        
        
        
        
        uint32_t *offsetStack;

        bool captureOffsetStack(LifoAlloc &alloc, const uint32_t *stack, uint32_t depth) {
            stackDepth = depth;
            offsetStack = alloc.newArray<uint32_t>(stackDepth);
            if (stackDepth) {
                if (!offsetStack)
                    return false;
                for (uint32_t n = 0; n < stackDepth; n++)
                    offsetStack[n] = stack[n];
            }
            return true;
        }

        
        
        
        
        
        void mergeOffsetStack(const uint32_t *stack, uint32_t depth) {
            JS_ASSERT(depth == stackDepth);
            for (uint32_t n = 0; n < stackDepth; n++)
                if (offsetStack[n] != stack[n])
                    offsetStack[n] = UINT32_MAX;
        }
    };

    JSContext *cx_;
    LifoAllocScope allocScope_;
    RootedScript script_;

    Bytecode **codeArray_;

  public:
    BytecodeParser(JSContext *cx, JSScript *script)
      : cx_(cx),
        allocScope_(&cx->tempLifoAlloc()),
        script_(cx, script),
        codeArray_(nullptr) { }

    bool parse();

#ifdef DEBUG
    bool isReachable(uint32_t offset) { return maybeCode(offset); }
    bool isReachable(const jsbytecode *pc) { return maybeCode(pc); }
#endif

    uint32_t stackDepthAtPC(uint32_t offset) {
        
        
        
        return getCode(offset).stackDepth;
    }
    uint32_t stackDepthAtPC(const jsbytecode *pc) { return stackDepthAtPC(pc - script_->code); }

    uint32_t offsetForStackOperand(uint32_t offset, int operand) {
        Bytecode &code = getCode(offset);
        if (operand < 0) {
            operand += code.stackDepth;
            JS_ASSERT(operand >= 0);
        }
        JS_ASSERT(uint32_t(operand) < code.stackDepth);
        return code.offsetStack[operand];
    }
    jsbytecode *pcForStackOperand(jsbytecode *pc, int operand) {
        uint32_t offset = offsetForStackOperand(pc - script_->code, operand);
        if (offset == UINT32_MAX)
            return nullptr;
        return script_->code + offsetForStackOperand(pc - script_->code, operand);
    }

  private:
    LifoAlloc &alloc() {
        return allocScope_.alloc();
    }

    void reportOOM() {
        allocScope_.releaseEarly();
        js_ReportOutOfMemory(cx_);
    }

    uint32_t numSlots() {
        return 1 + (script_->function() ? script_->function()->nargs : 0) + script_->nfixed;
    }

    uint32_t maximumStackDepth() {
        return script_->nslots - script_->nfixed;
    }

    Bytecode& getCode(uint32_t offset) {
        JS_ASSERT(offset < script_->length);
        JS_ASSERT(codeArray_[offset]);
        return *codeArray_[offset];
    }
    Bytecode& getCode(const jsbytecode *pc) { return getCode(pc - script_->code); }

    Bytecode* maybeCode(uint32_t offset) {
        JS_ASSERT(offset < script_->length);
        return codeArray_[offset];
    }
    Bytecode* maybeCode(const jsbytecode *pc) { return maybeCode(pc - script_->code); }

    uint32_t simulateOp(JSOp op, uint32_t offset, uint32_t *offsetStack, uint32_t stackDepth);

    inline bool addJump(uint32_t offset, uint32_t *currentOffset,
                        uint32_t stackDepth, const uint32_t *offsetStack);
};

}  

uint32_t
BytecodeParser::simulateOp(JSOp op, uint32_t offset, uint32_t *offsetStack, uint32_t stackDepth)
{
    uint32_t nuses = GetUseCount(script_, offset);
    uint32_t ndefs = GetDefCount(script_, offset);

    JS_ASSERT(stackDepth >= nuses);
    stackDepth -= nuses;
    JS_ASSERT(stackDepth + ndefs <= maximumStackDepth());

    
    
    
    switch (op) {
      default:
        for (uint32_t n = 0; n != ndefs; ++n)
            offsetStack[stackDepth + n] = offset;
        break;

      case JSOP_CASE:
        
        JS_ASSERT(ndefs == 1);
        break;

      case JSOP_DUP:
        JS_ASSERT(ndefs == 2);
        if (offsetStack)
            offsetStack[stackDepth + 1] = offsetStack[stackDepth];
        break;

      case JSOP_DUP2:
        JS_ASSERT(ndefs == 4);
        if (offsetStack) {
            offsetStack[stackDepth + 2] = offsetStack[stackDepth];
            offsetStack[stackDepth + 3] = offsetStack[stackDepth + 1];
        }
        break;

      case JSOP_SWAP:
        JS_ASSERT(ndefs == 2);
        if (offsetStack) {
            uint32_t tmp = offsetStack[stackDepth + 1];
            offsetStack[stackDepth + 1] = offsetStack[stackDepth];
            offsetStack[stackDepth] = tmp;
        }
        break;
    }
    stackDepth += ndefs;
    return stackDepth;
}

bool
BytecodeParser::addJump(uint32_t offset, uint32_t *currentOffset,
                        uint32_t stackDepth, const uint32_t *offsetStack)
{
    JS_ASSERT(offset < script_->length);

    Bytecode *&code = codeArray_[offset];
    if (!code) {
        code = alloc().new_<Bytecode>();
        if (!code)
            return false;
        AssertStackDepth(script_, offset, stackDepth);
        if (!code->captureOffsetStack(alloc(), offsetStack, stackDepth)) {
            reportOOM();
            return false;
        }
    } else {
        code->mergeOffsetStack(offsetStack, stackDepth);
    }

    if (offset < *currentOffset && !code->parsed) {
        
        
        
        *currentOffset = offset;
    }

    return true;
}

bool
BytecodeParser::parse()
{
    JS_ASSERT(!codeArray_);

    uint32_t length = script_->length;
    codeArray_ = alloc().newArray<Bytecode*>(length);

    if (!codeArray_) {
        reportOOM();
        return false;
    }

    mozilla::PodZero(codeArray_, length);

    
    Bytecode *startcode = alloc().new_<Bytecode>();
    if (!startcode) {
        reportOOM();
        return false;
    }

    
    uint32_t *offsetStack = alloc().newArray<uint32_t>(maximumStackDepth());
    if (maximumStackDepth() && !offsetStack) {
        reportOOM();
        return false;
    }

    startcode->stackDepth = 0;
    codeArray_[0] = startcode;

    uint32_t offset, nextOffset = 0;
    while (nextOffset < length) {
        offset = nextOffset;

        Bytecode *code = maybeCode(offset);
        jsbytecode *pc = script_->code + offset;

        JSOp op = (JSOp)*pc;
        JS_ASSERT(op < JSOP_LIMIT);

        
        uint32_t successorOffset = offset + GetBytecodeLength(pc);

        
        
        nextOffset = successorOffset;

        if (!code) {
            
            continue;
        }

        if (code->parsed) {
            
            continue;
        }

        code->parsed = true;

        uint32_t stackDepth = simulateOp(op, offset, offsetStack, code->stackDepth);

        switch (op) {
          case JSOP_TABLESWITCH: {
            uint32_t defaultOffset = offset + GET_JUMP_OFFSET(pc);
            jsbytecode *pc2 = pc + JUMP_OFFSET_LEN;
            int32_t low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            int32_t high = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;

            if (!addJump(defaultOffset, &nextOffset, stackDepth, offsetStack))
                return false;

            for (int32_t i = low; i <= high; i++) {
                uint32_t targetOffset = offset + GET_JUMP_OFFSET(pc2);
                if (targetOffset != offset) {
                    if (!addJump(targetOffset, &nextOffset, stackDepth, offsetStack))
                        return false;
                }
                pc2 += JUMP_OFFSET_LEN;
            }
            break;
          }

          case JSOP_TRY: {
            
            
            
            
            JSTryNote *tn = script_->trynotes()->vector;
            JSTryNote *tnlimit = tn + script_->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                uint32_t startOffset = script_->mainOffset + tn->start;
                if (startOffset == offset + 1) {
                    uint32_t catchOffset = startOffset + tn->length;
                    if (tn->kind != JSTRY_ITER && tn->kind != JSTRY_LOOP) {
                        if (!addJump(catchOffset, &nextOffset, stackDepth, offsetStack))
                            return false;
                    }
                }
            }
            break;
          }

          default:
            break;
        }

        
        if (IsJumpOpcode(op)) {
            
            uint32_t newStackDepth = stackDepth;
            if (op == JSOP_CASE)
                newStackDepth--;

            uint32_t targetOffset = offset + GET_JUMP_OFFSET(pc);
            if (!addJump(targetOffset, &nextOffset, newStackDepth, offsetStack))
                return false;
        }

        
        if (BytecodeFallsThrough(op)) {
            JS_ASSERT(successorOffset < script_->length);

            Bytecode *&nextcode = codeArray_[successorOffset];

            if (!nextcode) {
                nextcode = alloc().new_<Bytecode>();
                if (!nextcode) {
                    reportOOM();
                    return false;
                }
                AssertStackDepth(script_, successorOffset, stackDepth);
                if (!nextcode->captureOffsetStack(alloc(), offsetStack, stackDepth)) {
                    reportOOM();
                    return false;
                }
            } else {
                nextcode->mergeOffsetStack(offsetStack, stackDepth);
            }
        }
    }

    return true;
}

#ifdef DEBUG

bool
js::ReconstructStackDepth(JSContext *cx, JSScript *script, jsbytecode *pc, uint32_t *depth)
{
    BytecodeParser parser(cx, script);
    if (!parser.parse())
        return false;

    if (!parser.isReachable(pc))
        return false;

    *depth = parser.stackDepthAtPC(pc);

    return true;
}






JS_FRIEND_API(bool)
js_DisassembleAtPC(JSContext *cx, JSScript *scriptArg, bool lines,
                   jsbytecode *pc, bool showAll, Sprinter *sp)
{
    RootedScript script(cx, scriptArg);
    BytecodeParser parser(cx, script);

    jsbytecode *next, *end;
    unsigned len;

    if (showAll && !parser.parse())
        return false;

    if (showAll)
        Sprint(sp, "%s:%u\n", script->filename(), script->lineno);

    if (pc != nullptr)
        sp->put("    ");
    if (showAll)
        sp->put("sn stack ");
    sp->put("loc   ");
    if (lines)
        sp->put("line");
    sp->put("  op\n");

    if (pc != nullptr)
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
        if (pc != nullptr) {
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
            if (parser.isReachable(next))
                Sprint(sp, "%05u ", parser.stackDepthAtPC(next));
            else
                Sprint(sp, "      ", parser.stackDepthAtPC(next));
        }
        len = js_Disassemble1(cx, script, next, next - script->code, lines, sp);
        if (!len)
            return false;
        next += len;
    }
    return true;
}

bool
js_Disassemble(JSContext *cx, HandleScript script, bool lines, Sprinter *sp)
{
    return js_DisassembleAtPC(cx, script, lines, nullptr, false, sp);
}

JS_FRIEND_API(bool)
js_DumpPC(JSContext *cx)
{
    js::gc::AutoSuppressGC suppressGC(cx);
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return false;
    ScriptFrameIter iter(cx);
    RootedScript script(cx, iter.script());
    bool ok = js_DisassembleAtPC(cx, script, true, iter.pc(), false, &sprinter);
    fprintf(stdout, "%s", sprinter.string());
    return ok;
}

JS_FRIEND_API(bool)
js_DumpScript(JSContext *cx, JSScript *scriptArg)
{
    js::gc::AutoSuppressGC suppressGC(cx);
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return false;
    RootedScript script(cx, scriptArg);
    bool ok = js_Disassemble(cx, script, true, &sprinter);
    fprintf(stdout, "%s", sprinter.string());
    return ok;
}




JS_FRIEND_API(bool)
js_DumpScriptDepth(JSContext *cx, JSScript *scriptArg, jsbytecode *pc)
{
    js::gc::AutoSuppressGC suppressGC(cx);
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return false;
    RootedScript script(cx, scriptArg);
    bool ok = js_DisassembleAtPC(cx, script, true, pc, true, &sprinter);
    fprintf(stdout, "%s", sprinter.string());
    return ok;
}

static char *
QuoteString(Sprinter *sp, JSString *str, uint32_t quote);

static bool
ToDisassemblySource(JSContext *cx, jsval v, JSAutoByteString *bytes)
{
    if (JSVAL_IS_STRING(v)) {
        Sprinter sprinter(cx);
        if (!sprinter.init())
            return false;
        char *nbytes = QuoteString(&sprinter, JSVAL_TO_STRING(v), '"');
        if (!nbytes)
            return false;
        nbytes = JS_sprintf_append(nullptr, "%s", nbytes);
        if (!nbytes)
            return false;
        bytes->initBytes(nbytes);
        return true;
    }

    if (cx->runtime()->isHeapBusy() || cx->runtime()->noGCOrAllocationCheck) {
        char *source = JS_sprintf_append(nullptr, "<value>");
        if (!source)
            return false;
        bytes->initBytes(source);
        return true;
    }

    if (!JSVAL_IS_PRIMITIVE(v)) {
        JSObject *obj = JSVAL_TO_OBJECT(v);
        if (obj->is<BlockObject>()) {
            char *source = JS_sprintf_append(nullptr, "depth %d {",
                                             obj->as<BlockObject>().stackDepth());
            if (!source)
                return false;

            Shape::Range<CanGC> r(cx, obj->lastProperty());

            while (!r.empty()) {
                Rooted<Shape*> shape(cx, &r.front());
                JSAtom *atom = JSID_IS_INT(shape->propid())
                               ? cx->names().empty
                               : JSID_TO_ATOM(shape->propid());

                JSAutoByteString bytes;
                if (!AtomToPrintableString(cx, atom, &bytes))
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

        if (obj->is<JSFunction>()) {
            JSString *str = JS_DecompileFunction(cx, &obj->as<JSFunction>(), JS_DONT_PRETTY_PRINT);
            if (!str)
                return false;
            return bytes->encodeLatin1(cx, str);
        }

        if (obj->is<RegExpObject>()) {
            JSString *source = obj->as<RegExpObject>().toString(cx);
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
                unsigned loc, bool lines, Sprinter *sp)
{
    JSOp op = (JSOp)*pc;
    if (op >= JSOP_LIMIT) {
        char numBuf1[12], numBuf2[12];
        JS_snprintf(numBuf1, sizeof numBuf1, "%d", op);
        JS_snprintf(numBuf2, sizeof numBuf2, "%d", JSOP_LIMIT);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
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
        Value v = StringValue(ScopeCoordinateName(script, pc));
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
        
        if (script->compartment()->activeAnalysis) {
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
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
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
    char *newBuf = (char *) js_realloc(base, newSize);
    if (!newBuf) {
        reportOutOfMemory();
        return false;
    }
    base = newBuf;
    size = newSize;
    base[size - 1] = 0;
    return true;
}

Sprinter::Sprinter(ExclusiveContext *cx)
  : context(cx),
#ifdef DEBUG
    initialized(false),
#endif
    base(nullptr), size(0), offset(0), reportedOOM(false)
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
    base = (char *) js_malloc(DefaultSize);
    if (!base) {
        reportOutOfMemory();
        return false;
    }
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

char *
Sprinter::reserve(size_t len)
{
    InvariantChecker ic(this);

    while (len + 1 > size - offset) { 
        if (!realloc_(size * 2))
            return nullptr;
    }

    char *sb = base + offset;
    offset += len;
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
    DeflateStringToBuffer(nullptr, chars, length, buffer, &size);
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

ptrdiff_t
Sprinter::getOffset() const
{
    return offset;
}

void
Sprinter::reportOutOfMemory()
{
    if (reportedOOM)
        return;
    if (context)
        js_ReportOutOfMemory(context);
    reportedOOM = true;
}

bool
Sprinter::hadOutOfMemory() const
{
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
    
    bool dontEscape = (quote & DONT_ESCAPE) != 0;
    jschar qc = (jschar) quote;
    ptrdiff_t offset = sp->getOffset();
    if (qc && Sprint(sp, "%c", (char)qc) < 0)
        return nullptr;

    const jschar *s = str->getChars(sp->context);
    if (!s)
        return nullptr;
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
                return nullptr;

            for (ptrdiff_t i = 0; i < len; ++i)
                (*sp)[base + i] = (char) *s++;
            (*sp)[base + len] = 0;
        }

        if (t == z)
            break;

        
        bool ok;
        const char *e;
        if (!(c >> 8) && c != 0 && (e = strchr(js_EscapeMap, (int)c)) != nullptr) {
            ok = dontEscape
                 ? Sprint(sp, "%c", (char)c) >= 0
                 : Sprint(sp, "\\%c", e[1]) >= 0;
        } else {
            




            ok = Sprint(sp, (qc && !(c >> 8)) ? "\\x%02X" : "\\u%04X", c) >= 0;
        }
        if (!ok)
            return nullptr;
    }

    
    if (qc && Sprint(sp, "%c", (char)qc) < 0)
        return nullptr;

    



    if (offset == sp->getOffset() && Sprint(sp, "") < 0)
        return nullptr;

    return sp->stringAt(offset);
}

JSString *
js_QuoteString(ExclusiveContext *cx, JSString *str, jschar quote)
{
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return nullptr;
    char *bytes = QuoteString(&sprinter, str, quote);
    if (!bytes)
        return nullptr;
    return js_NewStringCopyZ<CanGC>(cx, bytes);
}



static JSObject *
GetBlockChainAtPC(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(pc >= script->main() && pc < script->code + script->length);

    ptrdiff_t offset = pc - script->main();

    if (!script->hasBlockScopes())
        return nullptr;

    BlockScopeArray *blockScopes = script->blockScopes();
    JSObject *blockChain = nullptr;
    for (uint32_t n = 0; n < blockScopes->length; n++) {
        const BlockScopeNote *note = &blockScopes->vector[n];
        if (note->start > offset)
            break;
        if (offset <= note->start + note->length)
            blockChain = script->getObject(note->index);
    }

    return blockChain;
}
































struct ExpressionDecompiler
{
    JSContext *cx;
    StackFrame *fp;
    RootedScript script;
    RootedFunction fun;
    BindingVector *localNames;
    BytecodeParser parser;
    Sprinter sprinter;

    ExpressionDecompiler(JSContext *cx, JSScript *script, JSFunction *fun)
        : cx(cx),
          script(cx, script),
          fun(cx, fun),
          localNames(nullptr),
          parser(cx, script),
          sprinter(cx)
    {}
    ~ExpressionDecompiler();
    bool init();
    bool decompilePCForStackOperand(jsbytecode *pc, int i);
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
ExpressionDecompiler::decompilePCForStackOperand(jsbytecode *pc, int i)
{
    pc = parser.pcForStackOperand(pc, i);
    if (!pc)
        return write("(intermediate value)");
    return decompilePC(pc);
}

bool
ExpressionDecompiler::decompilePC(jsbytecode *pc)
{
    JS_ASSERT(script->code <= pc && pc < script->code + script->length);

    JSOp op = (JSOp)*pc;

    if (const char *token = CodeToken[op]) {
        
        switch (js_CodeSpec[op].nuses) {
          case 2: {
            jssrcnote *sn = js_GetSrcNote(cx, script, pc);
            if (!sn || SN_TYPE(sn) != SRC_ASSIGNOP)
                return write("(") &&
                       decompilePCForStackOperand(pc, -2) &&
                       write(" ") &&
                       write(token) &&
                       write(" ") &&
                       decompilePCForStackOperand(pc, -1) &&
                       write(")");
            break;
          }
          case 1:
            return write(token) &&
                   write("(") &&
                   decompilePCForStackOperand(pc, -1) &&
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
            JS_ASSERT(i < unsigned(parser.stackDepthAtPC(pc)));
            atom = findLetVar(pc, i);
            if (!atom)
                return decompilePCForStackOperand(pc, i); 
        } else {
            atom = getVar(i);
        }
        JS_ASSERT(atom);
        return write(atom);
      }
      case JSOP_CALLALIASEDVAR:
      case JSOP_GETALIASEDVAR: {
        JSAtom *atom = ScopeCoordinateName(script, pc);
        JS_ASSERT(atom);
        return write(atom);
      }
      case JSOP_LENGTH:
      case JSOP_GETPROP:
      case JSOP_CALLPROP: {
        RootedAtom prop(cx, (op == JSOP_LENGTH) ? cx->names().length : loadAtom(pc));
        if (!decompilePCForStackOperand(pc, -1))
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
        return decompilePCForStackOperand(pc, -2) &&
               write("[") &&
               decompilePCForStackOperand(pc, -1) &&
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
      case JSOP_INT32:
        return sprinter.printf("%d", GetBytecodeInteger(pc)) >= 0;
      case JSOP_STRING:
        return quote(loadAtom(pc), '"');
      case JSOP_UNDEFINED:
        return write(js_undefined_str);
      case JSOP_THIS:
        
        
        return write(js_this_str);
      case JSOP_CALL:
      case JSOP_FUNCALL:
        return decompilePCForStackOperand(pc, -int32_t(GET_ARGC(pc) + 2)) &&
               write("(...)");
      case JSOP_SPREADCALL:
        return decompilePCForStackOperand(pc, -int32_t(3)) &&
               write("(...)");
      case JSOP_NEWARRAY:
        return write("[]");
      case JSOP_REGEXP:
      case JSOP_OBJECT: {
        JSObject *obj = (op == JSOP_REGEXP)
                        ? script->getRegExp(GET_UINT32_INDEX(pc))
                        : script->getObject(GET_UINT32_INDEX(pc));
        RootedValue objv(cx, ObjectValue(*obj));
        JSString *str = ValueToSource(cx, objv);
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

    if (!parser.parse())
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
            return nullptr;
        JS_ASSERT(chain->is<BlockObject>());
        do {
            BlockObject &block = chain->as<BlockObject>();
            uint32_t blockDepth = block.stackDepth();
            uint32_t blockCount = block.slotCount();
            if (uint32_t(depth - blockDepth) < uint32_t(blockCount)) {
                for (Shape::Range<NoGC> r(block.lastProperty()); !r.empty(); r.popFront()) {
                    const Shape &shape = r.front();
                    if (shape.shortid() == int(depth - blockDepth))
                        return JSID_TO_ATOM(shape.propid());
                }
            }
            chain = chain->getParent();
        } while (chain && chain->is<BlockObject>());
    }
    return nullptr;
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

    



    if (iter.isIon())
        return true;

    *valuepc = nullptr;

    BytecodeParser parser(cx, iter.script());
    if (!parser.parse())
        return false;

    if (spindex < 0 && spindex + int(parser.stackDepthAtPC(current)) < 0)
        spindex = JSDVG_SEARCH_STACK;

    if (spindex == JSDVG_SEARCH_STACK) {
        size_t index = iter.numFrameSlots();
        JS_ASSERT(index >= size_t(parser.stackDepthAtPC(current)));

        
        
        
        int stackHits = 0;
        Value s;
        do {
            if (!index)
                return true;
            s = iter.frameSlotValue(--index);
        } while (s != v || stackHits++ != skipStackHits);

        
        
        
        jsbytecode *pc = nullptr;
        if (index < size_t(parser.stackDepthAtPC(current)))
            pc = parser.pcForStackOperand(current, index);
        *valuepc = pc ? pc : current;
    } else {
        jsbytecode *pc = parser.pcForStackOperand(current, spindex);
        *valuepc = pc ? pc : current;
    }
    return true;
}

static bool
DecompileExpressionFromStack(JSContext *cx, int spindex, int skipStackHits, HandleValue v, char **res)
{
    JS_ASSERT(spindex < 0 ||
              spindex == JSDVG_IGNORE_STACK ||
              spindex == JSDVG_SEARCH_STACK);

    *res = nullptr;

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
                           : nullptr);

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
    RootedString fallback(cx, fallbackArg);
    {
        char *result;
        if (!DecompileExpressionFromStack(cx, spindex, skipStackHits, v, &result))
            return nullptr;
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
            return nullptr;
    }

    Rooted<JSLinearString *> linear(cx, fallback->ensureLinear(cx));
    if (!linear)
        return nullptr;
    TwoByteChars tbchars(linear->chars(), linear->length());
    return LossyTwoByteCharsToNewLatin1CharsZ(cx, tbchars).c_str();
}

static bool
DecompileArgumentFromStack(JSContext *cx, int formalIndex, char **res)
{
    JS_ASSERT(formalIndex >= 0);

    *res = nullptr;

#ifdef JS_MORE_DETERMINISTIC
    
    return true;
#endif

    



    ScriptFrameIter frameIter(cx);
    JS_ASSERT(!frameIter.done());

    



    ++frameIter;
    if (frameIter.done())
        return true;

    RootedScript script(cx, frameIter.script());
    AutoCompartment ac(cx, &script->global());
    jsbytecode *current = frameIter.pc();
    RootedFunction fun(cx, frameIter.isFunctionFrame()
                       ? frameIter.callee()
                       : nullptr);

    JS_ASSERT(script->code <= current && current < script->code + script->length);

    if (current < script->main())
        return true;

    
    if (JSOp(*current) != JSOP_CALL || static_cast<unsigned>(formalIndex) >= GET_ARGC(current))
        return true;

    BytecodeParser parser(cx, script);
    if (!parser.parse())
        return false;

    int formalStackIndex = parser.stackDepthAtPC(current) - GET_ARGC(current) + formalIndex;
    JS_ASSERT(formalStackIndex >= 0);
    if (uint32_t(formalStackIndex) >= parser.stackDepthAtPC(current))
        return true;

    ExpressionDecompiler ed(cx, script, fun);
    if (!ed.init())
        return false;
    if (!ed.decompilePCForStackOperand(current, formalStackIndex))
        return false;

    return ed.getOutput(res);
}

char *
js::DecompileArgument(JSContext *cx, int formalIndex, HandleValue v)
{
    {
        char *result;
        if (!DecompileArgumentFromStack(cx, formalIndex, &result))
            return nullptr;
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
        return nullptr;

    Rooted<JSLinearString *> linear(cx, fallback->ensureLinear(cx));
    if (!linear)
        return nullptr;
    return LossyTwoByteCharsToNewLatin1CharsZ(cx, linear->range()).c_str();
}

bool
js::CallResultEscapes(jsbytecode *pc)
{
    








    if (*pc == JSOP_CALL)
        pc += JSOP_CALL_LENGTH;
    else if (*pc == JSOP_SPREADCALL)
        pc += JSOP_SPREADCALL_LENGTH;
    else
        return true;

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
    JSRuntime *rt = cx->runtime();

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
                          double *values, const char * const *names, unsigned count,
                          MaybeComma &comma)
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
    JSRuntime *rt = cx->runtime();

    if (!rt->scriptAndCountsVector || index >= rt->scriptAndCountsVector->length()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BUFFER_TOO_SMALL);
        return nullptr;
    }

    const ScriptAndCounts &sac = (*rt->scriptAndCountsVector)[index];
    RootedScript script(cx, sac.script);

    




    StringBuffer buf(cx);

    buf.append('{');

    AppendJSONProperty(buf, "file", NO_COMMA);
    JSString *str = JS_NewStringCopyZ(cx, script->filename());
    if (!str || !(str = StringToSource(cx, str)))
        return nullptr;
    buf.append(str);

    AppendJSONProperty(buf, "line");
    NumberValueToStringBuffer(cx, Int32Value(script->lineno), buf);

    if (script->function()) {
        JSAtom *atom = script->function()->displayAtom();
        if (atom) {
            AppendJSONProperty(buf, "name");
            if (!(str = StringToSource(cx, atom)))
                return nullptr;
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
                    MOZ_ASSUME_UNREACHABLE("Bad opcode");
            } else if (PCCounts::arithOp(op)) {
                arithTotals[j - PCCounts::BASE_LIMIT] += value;
            } else {
                MOZ_ASSUME_UNREACHABLE("Bad opcode");
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
    jit::IonScriptCounts *ionCounts = sac.getIonCounts();
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
        return nullptr;

    return buf.finishString();
}

static bool
GetPCCountJSON(JSContext *cx, const ScriptAndCounts &sac, StringBuffer &buf)
{
    RootedScript script(cx, sac.script);

    buf.append('{');
    AppendJSONProperty(buf, "text", NO_COMMA);

    JSString *str = JS_DecompileScript(cx, script, nullptr, 0);
    if (!str || !(str = StringToSource(cx, str)))
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
            if (!str || !(str = StringToSource(cx, str)))
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

    jit::IonScriptCounts *ionCounts = sac.getIonCounts();
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
                const jit::IonBlockCounts &block = ionCounts->block(i);

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
                if (!str || !(str = StringToSource(cx, str)))
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
    JSRuntime *rt = cx->runtime();

    if (!rt->scriptAndCountsVector || index >= rt->scriptAndCountsVector->length()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BUFFER_TOO_SMALL);
        return nullptr;
    }

    const ScriptAndCounts &sac = (*rt->scriptAndCountsVector)[index];
    JSScript *script = sac.script;

    StringBuffer buf(cx);

    if (!script->function() && !script->compileAndGo)
        return buf.finishString();

    {
        AutoCompartment ac(cx, &script->global());
        if (!GetPCCountJSON(cx, sac, buf))
            return nullptr;
    }

    return buf.finishString();
}
