









#include "jsopcodeinlines.h"

#include "mozilla/SizePrintfMacros.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "jsapi.h"
#include "jsatom.h"
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

#include "asmjs/AsmJSModule.h"
#include "frontend/BytecodeCompiler.h"
#include "frontend/SourceNotes.h"
#include "js/CharacterEncoding.h"
#include "vm/Opcodes.h"
#include "vm/ScopeObject.h"
#include "vm/Shape.h"
#include "vm/StringBuffer.h"

#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::gc;

using JS::AutoCheckCannotGC;

using js::frontend::IsIdentifier;




JS_STATIC_ASSERT(sizeof(uint32_t) * JS_BITS_PER_BYTE >= INDEX_LIMIT_LOG2 + 1);

const JSCodeSpec js_CodeSpec[] = {
#define MAKE_CODESPEC(op,val,name,token,length,nuses,ndefs,format)  {length,nuses,ndefs,format},
    FOR_EACH_OPCODE(MAKE_CODESPEC)
#undef MAKE_CODESPEC
};

const unsigned js_NumCodeSpecs = JS_ARRAY_LENGTH(js_CodeSpec);





static const char * const CodeToken[] = {
#define TOKEN(op, val, name, token, ...)  token,
    FOR_EACH_OPCODE(TOKEN)
#undef TOKEN
};





const char * const js_CodeName[] = {
#define OPNAME(op, val, name, ...)  name,
    FOR_EACH_OPCODE(OPNAME)
#undef OPNAME
};



#define COUNTS_LEN 16

size_t
js::GetVariableBytecodeLength(jsbytecode* pc)
{
    JSOp op = JSOp(*pc);
    MOZ_ASSERT(js_CodeSpec[op].length == -1);
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
        MOZ_CRASH("Unexpected op");
    }
}

unsigned
js::StackUses(JSScript* script, jsbytecode* pc)
{
    JSOp op = (JSOp) *pc;
    const JSCodeSpec& cs = js_CodeSpec[op];
    if (cs.nuses >= 0)
        return cs.nuses;

    MOZ_ASSERT(js_CodeSpec[op].nuses == -1);
    switch (op) {
      case JSOP_POPN:
        return GET_UINT16(pc);
      case JSOP_NEW:
        return 2 + GET_ARGC(pc) + 1;
      default:
        
        MOZ_ASSERT(op == JSOP_CALL || op == JSOP_EVAL ||
                   op == JSOP_STRICTEVAL || op == JSOP_FUNCALL || op == JSOP_FUNAPPLY);
        return 2 + GET_ARGC(pc);
    }
}

unsigned
js::StackDefs(JSScript* script, jsbytecode* pc)
{
    JSOp op = (JSOp) *pc;
    const JSCodeSpec& cs = js_CodeSpec[op];
    MOZ_ASSERT(cs.ndefs >= 0);
    return cs.ndefs;
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

 const char*
PCCounts::countName(JSOp op, size_t which)
{
    MOZ_ASSERT(which < numCounts(op));

    if (which < BASE_LIMIT)
        return countBaseNames[which];

    if (accessOp(op)) {
        if (which < ACCESS_LIMIT)
            return countAccessNames[which - BASE_LIMIT];
        if (elementOp(op))
            return countElementNames[which - ACCESS_LIMIT];
        if (propertyOp(op))
            return countPropertyNames[which - ACCESS_LIMIT];
        MOZ_CRASH("bad op");
    }

    if (arithOp(op))
        return countArithNames[which - BASE_LIMIT];

    MOZ_CRASH("bad op");
}

void
js::DumpIonScriptCounts(Sprinter* sp, jit::IonScriptCounts* ionCounts)
{
    Sprint(sp, "IonScript [%lu blocks]:\n", ionCounts->numBlocks());
    for (size_t i = 0; i < ionCounts->numBlocks(); i++) {
        const jit::IonBlockCounts& block = ionCounts->block(i);
        Sprint(sp, "BB #%lu [%05u]", block.id(), block.offset());
        if (block.description())
            Sprint(sp, " [inlined %s]", block.description());
        for (size_t j = 0; j < block.numSuccessors(); j++)
            Sprint(sp, " -> #%lu", block.successor(j));
        Sprint(sp, " :: %llu hits\n", block.hitCount());
        Sprint(sp, "%s\n", block.code());
    }
}

void
js::DumpPCCounts(JSContext* cx, HandleScript script, Sprinter* sp)
{
    MOZ_ASSERT(script->hasScriptCounts());

#ifdef DEBUG
    jsbytecode* pc = script->code();
    while (pc < script->codeEnd()) {
        JSOp op = JSOp(*pc);
        jsbytecode* next = GetNextPc(pc);

        if (!Disassemble1(cx, script, pc, script->pcToOffset(pc), true, sp))
            return;

        size_t total = PCCounts::numCounts(op);
        double* raw = script->getPCCounts(pc).rawCounts();

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

    jit::IonScriptCounts* ionCounts = script->getIonCounts();

    while (ionCounts) {
        DumpIonScriptCounts(sp, ionCounts);
        ionCounts = ionCounts->previous();
    }
}

void
js::DumpCompartmentPCCounts(JSContext* cx)
{
    for (ZoneCellIter i(cx->zone(), gc::AllocKind::SCRIPT); !i.done(); i.next()) {
        RootedScript script(cx, i.get<JSScript>());
        if (script->compartment() != cx->compartment())
            continue;

        if (script->hasScriptCounts()) {
            Sprinter sprinter(cx);
            if (!sprinter.init())
                return;

            fprintf(stdout, "--- SCRIPT %s:%" PRIuSIZE " ---\n", script->filename(), script->lineno());
            DumpPCCounts(cx, script, &sprinter);
            fputs(sprinter.string(), stdout);
            fprintf(stdout, "--- END SCRIPT %s:%" PRIuSIZE " ---\n", script->filename(), script->lineno());
        }
    }

    for (auto thingKind : ObjectAllocKinds()) {
        for (ZoneCellIter i(cx->zone(), thingKind); !i.done(); i.next()) {
            JSObject* obj = i.get<JSObject>();
            if (obj->compartment() != cx->compartment())
                continue;

            if (obj->is<AsmJSModuleObject>()) {
                AsmJSModule& module = obj->as<AsmJSModuleObject>().module();

                Sprinter sprinter(cx);
                if (!sprinter.init())
                    return;

                fprintf(stdout, "--- Asm.js Module ---\n");

                for (size_t i = 0; i < module.numFunctionCounts(); i++) {
                    jit::IonScriptCounts* counts = module.functionCounts(i);
                    DumpIonScriptCounts(&sprinter, counts);
                }

                fputs(sprinter.string(), stdout);
                fprintf(stdout, "--- END Asm.js Module ---\n");
            }
        }
    }
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

        
        
        
        
        uint32_t* offsetStack;

        bool captureOffsetStack(LifoAlloc& alloc, const uint32_t* stack, uint32_t depth) {
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

        
        
        
        
        
        void mergeOffsetStack(const uint32_t* stack, uint32_t depth) {
            MOZ_ASSERT(depth == stackDepth);
            for (uint32_t n = 0; n < stackDepth; n++)
                if (offsetStack[n] != stack[n])
                    offsetStack[n] = UINT32_MAX;
        }
    };

    JSContext* cx_;
    LifoAllocScope allocScope_;
    RootedScript script_;

    Bytecode** codeArray_;

  public:
    BytecodeParser(JSContext* cx, JSScript* script)
      : cx_(cx),
        allocScope_(&cx->tempLifoAlloc()),
        script_(cx, script),
        codeArray_(nullptr) { }

    bool parse();

#ifdef DEBUG
    bool isReachable(uint32_t offset) { return maybeCode(offset); }
    bool isReachable(const jsbytecode* pc) { return maybeCode(pc); }
#endif

    uint32_t stackDepthAtPC(uint32_t offset) {
        
        
        
        return getCode(offset).stackDepth;
    }
    uint32_t stackDepthAtPC(const jsbytecode* pc) { return stackDepthAtPC(script_->pcToOffset(pc)); }

    uint32_t offsetForStackOperand(uint32_t offset, int operand) {
        Bytecode& code = getCode(offset);
        if (operand < 0) {
            operand += code.stackDepth;
            MOZ_ASSERT(operand >= 0);
        }
        MOZ_ASSERT(uint32_t(operand) < code.stackDepth);
        return code.offsetStack[operand];
    }
    jsbytecode* pcForStackOperand(jsbytecode* pc, int operand) {
        uint32_t offset = offsetForStackOperand(script_->pcToOffset(pc), operand);
        if (offset == UINT32_MAX)
            return nullptr;
        return script_->offsetToPC(offsetForStackOperand(script_->pcToOffset(pc), operand));
    }

  private:
    LifoAlloc& alloc() {
        return allocScope_.alloc();
    }

    void reportOOM() {
        allocScope_.releaseEarly();
        ReportOutOfMemory(cx_);
    }

    uint32_t numSlots() {
        return 1 + script_->nfixed() +
               (script_->functionNonDelazifying() ? script_->functionNonDelazifying()->nargs() : 0);
    }

    uint32_t maximumStackDepth() {
        return script_->nslots() - script_->nfixed();
    }

    Bytecode& getCode(uint32_t offset) {
        MOZ_ASSERT(offset < script_->length());
        MOZ_ASSERT(codeArray_[offset]);
        return *codeArray_[offset];
    }
    Bytecode& getCode(const jsbytecode* pc) { return getCode(script_->pcToOffset(pc)); }

    Bytecode* maybeCode(uint32_t offset) {
        MOZ_ASSERT(offset < script_->length());
        return codeArray_[offset];
    }
    Bytecode* maybeCode(const jsbytecode* pc) { return maybeCode(script_->pcToOffset(pc)); }

    uint32_t simulateOp(JSOp op, uint32_t offset, uint32_t* offsetStack, uint32_t stackDepth);

    inline bool addJump(uint32_t offset, uint32_t* currentOffset,
                        uint32_t stackDepth, const uint32_t* offsetStack);
};

}  

uint32_t
BytecodeParser::simulateOp(JSOp op, uint32_t offset, uint32_t* offsetStack, uint32_t stackDepth)
{
    uint32_t nuses = GetUseCount(script_, offset);
    uint32_t ndefs = GetDefCount(script_, offset);

    MOZ_ASSERT(stackDepth >= nuses);
    stackDepth -= nuses;
    MOZ_ASSERT(stackDepth + ndefs <= maximumStackDepth());

    
    
    
    switch (op) {
      default:
        for (uint32_t n = 0; n != ndefs; ++n)
            offsetStack[stackDepth + n] = offset;
        break;

      case JSOP_CASE:
        
        MOZ_ASSERT(ndefs == 1);
        break;

      case JSOP_DUP:
        MOZ_ASSERT(ndefs == 2);
        if (offsetStack)
            offsetStack[stackDepth + 1] = offsetStack[stackDepth];
        break;

      case JSOP_DUP2:
        MOZ_ASSERT(ndefs == 4);
        if (offsetStack) {
            offsetStack[stackDepth + 2] = offsetStack[stackDepth];
            offsetStack[stackDepth + 3] = offsetStack[stackDepth + 1];
        }
        break;

      case JSOP_DUPAT: {
        MOZ_ASSERT(ndefs == 1);
        jsbytecode* pc = script_->offsetToPC(offset);
        unsigned n = GET_UINT24(pc);
        MOZ_ASSERT(n < stackDepth);
        if (offsetStack)
            offsetStack[stackDepth] = offsetStack[stackDepth - 1 - n];
        break;
      }

      case JSOP_SWAP:
        MOZ_ASSERT(ndefs == 2);
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
BytecodeParser::addJump(uint32_t offset, uint32_t* currentOffset,
                        uint32_t stackDepth, const uint32_t* offsetStack)
{
    MOZ_ASSERT(offset < script_->length());

    Bytecode*& code = codeArray_[offset];
    if (!code) {
        code = alloc().new_<Bytecode>();
        if (!code ||
            !code->captureOffsetStack(alloc(), offsetStack, stackDepth))
        {
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
    MOZ_ASSERT(!codeArray_);

    uint32_t length = script_->length();
    codeArray_ = alloc().newArray<Bytecode*>(length);

    if (!codeArray_) {
        reportOOM();
        return false;
    }

    mozilla::PodZero(codeArray_, length);

    
    Bytecode* startcode = alloc().new_<Bytecode>();
    if (!startcode) {
        reportOOM();
        return false;
    }

    
    uint32_t* offsetStack = alloc().newArray<uint32_t>(maximumStackDepth());
    if (maximumStackDepth() && !offsetStack) {
        reportOOM();
        return false;
    }

    startcode->stackDepth = 0;
    codeArray_[0] = startcode;

    uint32_t offset, nextOffset = 0;
    while (nextOffset < length) {
        offset = nextOffset;

        Bytecode* code = maybeCode(offset);
        jsbytecode* pc = script_->offsetToPC(offset);

        JSOp op = (JSOp)*pc;
        MOZ_ASSERT(op < JSOP_LIMIT);

        
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
            jsbytecode* pc2 = pc + JUMP_OFFSET_LEN;
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
            
            
            
            
            JSTryNote* tn = script_->trynotes()->vector;
            JSTryNote* tnlimit = tn + script_->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                uint32_t startOffset = script_->mainOffset() + tn->start;
                if (startOffset == offset + 1) {
                    uint32_t catchOffset = startOffset + tn->length;
                    if (tn->kind == JSTRY_CATCH || tn->kind == JSTRY_FINALLY) {
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
            MOZ_ASSERT(successorOffset < script_->length());

            Bytecode*& nextcode = codeArray_[successorOffset];

            if (!nextcode) {
                nextcode = alloc().new_<Bytecode>();
                if (!nextcode) {
                    reportOOM();
                    return false;
                }
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
js::ReconstructStackDepth(JSContext* cx, JSScript* script, jsbytecode* pc, uint32_t* depth, bool* reachablePC)
{
    BytecodeParser parser(cx, script);
    if (!parser.parse())
        return false;

    *reachablePC = parser.isReachable(pc);

    if (*reachablePC)
        *depth = parser.stackDepthAtPC(pc);

    return true;
}






static bool
DisassembleAtPC(JSContext* cx, JSScript* scriptArg, bool lines,
                jsbytecode* pc, bool showAll, Sprinter* sp)
{
    RootedScript script(cx, scriptArg);
    BytecodeParser parser(cx, script);

    if (showAll && !parser.parse())
        return false;

    if (showAll)
        Sprint(sp, "%s:%" PRIuSIZE "\n", script->filename(), script->lineno());

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

    jsbytecode* next = script->code();
    jsbytecode* end = script->codeEnd();
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
            jssrcnote* sn = GetSrcNote(cx, script, next);
            if (sn) {
                MOZ_ASSERT(!SN_IS_TERMINATOR(sn));
                jssrcnote* next = SN_NEXT(sn);
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
                Sprint(sp, "      ");
        }
        unsigned len = Disassemble1(cx, script, next, script->pcToOffset(next), lines, sp);
        if (!len)
            return false;
        next += len;
    }
    return true;
}

bool
js::Disassemble(JSContext* cx, HandleScript script, bool lines, Sprinter* sp)
{
    return DisassembleAtPC(cx, script, lines, nullptr, false, sp);
}

JS_FRIEND_API(bool)
js::DumpPC(JSContext* cx)
{
    gc::AutoSuppressGC suppressGC(cx);
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return false;
    ScriptFrameIter iter(cx);
    if (iter.done()) {
        fprintf(stdout, "Empty stack.\n");
        return true;
    }
    RootedScript script(cx, iter.script());
    bool ok = DisassembleAtPC(cx, script, true, iter.pc(), false, &sprinter);
    fprintf(stdout, "%s", sprinter.string());
    return ok;
}

JS_FRIEND_API(bool)
js::DumpScript(JSContext* cx, JSScript* scriptArg)
{
    gc::AutoSuppressGC suppressGC(cx);
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return false;
    RootedScript script(cx, scriptArg);
    bool ok = Disassemble(cx, script, true, &sprinter);
    fprintf(stdout, "%s", sprinter.string());
    return ok;
}

static bool
ToDisassemblySource(JSContext* cx, HandleValue v, JSAutoByteString* bytes)
{
    if (v.isString()) {
        Sprinter sprinter(cx);
        if (!sprinter.init())
            return false;
        char* nbytes = QuoteString(&sprinter, v.toString(), '"');
        if (!nbytes)
            return false;
        nbytes = JS_sprintf_append(nullptr, "%s", nbytes);
        if (!nbytes) {
            ReportOutOfMemory(cx);
            return false;
        }
        bytes->initBytes(nbytes);
        return true;
    }

    JSRuntime* rt = cx->runtime();
    if (rt->isHeapBusy() || !rt->gc.isAllocAllowed()) {
        char* source = JS_sprintf_append(nullptr, "<value>");
        if (!source) {
            ReportOutOfMemory(cx);
            return false;
        }
        bytes->initBytes(source);
        return true;
    }

    if (v.isObject()) {
        JSObject& obj = v.toObject();
        if (obj.is<StaticBlockObject>()) {
            Rooted<StaticBlockObject*> block(cx, &obj.as<StaticBlockObject>());
            char* source = JS_sprintf_append(nullptr, "depth %d {", block->localOffset());
            if (!source) {
                ReportOutOfMemory(cx);
                return false;
            }

            Shape::Range<CanGC> r(cx, block->lastProperty());

            while (!r.empty()) {
                Rooted<Shape*> shape(cx, &r.front());
                JSAtom* atom = JSID_IS_INT(shape->propid())
                               ? cx->names().empty
                               : JSID_TO_ATOM(shape->propid());

                JSAutoByteString bytes;
                if (!AtomToPrintableString(cx, atom, &bytes))
                    return false;

                r.popFront();
                source = JS_sprintf_append(source, "%s: %d%s",
                                           bytes.ptr(),
                                           block->shapeToIndex(*shape),
                                           !r.empty() ? ", " : "");
                if (!source) {
                    ReportOutOfMemory(cx);
                    return false;
                }
            }

            source = JS_sprintf_append(source, "}");
            if (!source) {
                ReportOutOfMemory(cx);
                return false;
            }
            bytes->initBytes(source);
            return true;
        }

        if (obj.is<JSFunction>()) {
            RootedFunction fun(cx, &obj.as<JSFunction>());
            JSString* str = JS_DecompileFunction(cx, fun, JS_DONT_PRETTY_PRINT);
            if (!str)
                return false;
            return bytes->encodeLatin1(cx, str);
        }

        if (obj.is<RegExpObject>()) {
            JSString* source = obj.as<RegExpObject>().toString(cx);
            if (!source)
                return false;
            return bytes->encodeLatin1(cx, source);
        }
    }

    return !!ValueToPrintable(cx, v, bytes, true);
}

unsigned
js::Disassemble1(JSContext* cx, HandleScript script, jsbytecode* pc,
                 unsigned loc, bool lines, Sprinter* sp)
{
    JSOp op = (JSOp)*pc;
    if (op >= JSOP_LIMIT) {
        char numBuf1[12], numBuf2[12];
        JS_snprintf(numBuf1, sizeof numBuf1, "%d", op);
        JS_snprintf(numBuf2, sizeof numBuf2, "%d", JSOP_LIMIT);
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_BYTECODE_TOO_BIG, numBuf1, numBuf2);
        return 0;
    }
    const JSCodeSpec* cs = &js_CodeSpec[op];
    ptrdiff_t len = (ptrdiff_t) cs->length;
    Sprint(sp, "%05u:", loc);
    if (lines)
        Sprint(sp, "%4u", PCToLineNumber(script, pc));
    Sprint(sp, "  %s", js_CodeName[op]);

    switch (JOF_TYPE(cs->format)) {
      case JOF_BYTE:
          
          
          
          
          if (op == JSOP_TRY) {
              TryNoteArray* trynotes = script->trynotes();
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
        RootedValue v(cx,
            StringValue(ScopeCoordinateName(cx->runtime()->scopeCoordinateNameCache, script, pc)));
        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, v, &bytes))
            return 0;
        ScopeCoordinate sc(pc);
        Sprint(sp, " %s (hops = %u, slot = %u)", bytes.ptr(), sc.hops(), sc.slot());
        break;
      }

      case JOF_ATOM: {
        RootedValue v(cx, StringValue(script->getAtom(GET_UINT32_INDEX(pc))));
        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, v, &bytes))
            return 0;
        Sprint(sp, " %s", bytes.ptr());
        break;
      }

      case JOF_DOUBLE: {
        RootedValue v(cx, script->getConst(GET_UINT32_INDEX(pc)));
        JSAutoByteString bytes;
        if (!ToDisassemblySource(cx, v, &bytes))
            return 0;
        Sprint(sp, " %s", bytes.ptr());
        break;
      }

      case JOF_OBJECT: {
        
        if (script->zone()->types.activeAnalysis) {
            Sprint(sp, " object");
            break;
        }

        JSObject* obj = script->getObject(GET_UINT32_INDEX(pc));
        {
            JSAutoByteString bytes;
            RootedValue v(cx, ObjectValue(*obj));
            if (!ToDisassemblySource(cx, v, &bytes))
                return 0;
            Sprint(sp, " %s", bytes.ptr());
        }
        break;
      }

      case JOF_REGEXP: {
        JSObject* obj = script->getRegExp(GET_UINT32_INDEX(pc));
        JSAutoByteString bytes;
        RootedValue v(cx, ObjectValue(*obj));
        if (!ToDisassemblySource(cx, v, &bytes))
            return 0;
        Sprint(sp, " %s", bytes.ptr());
        break;
      }

      case JOF_TABLESWITCH:
      {
        int32_t i, low, high;

        ptrdiff_t off = GET_JUMP_OFFSET(pc);
        jsbytecode* pc2 = pc + JUMP_OFFSET_LEN;
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
        Sprint(sp, " %u", GET_LOCALNO(pc));
        break;

      case JOF_UINT32:
        Sprint(sp, " %u", GET_UINT32(pc));
        break;

      {
        int i;

      case JOF_UINT16:
        i = (int)GET_UINT16(pc);
        goto print_int;

      case JOF_UINT24:
        MOZ_ASSERT(len == 4);
        i = (int)GET_UINT24(pc);
        goto print_int;

      case JOF_UINT8:
        i = GET_UINT8(pc);
        goto print_int;

      case JOF_INT8:
        i = GET_INT8(pc);
        goto print_int;

      case JOF_INT32:
        MOZ_ASSERT(op == JSOP_INT32);
        i = GET_INT32(pc);
      print_int:
        Sprint(sp, " %d", i);
        break;
      }

      default: {
        char numBuf[12];
        JS_snprintf(numBuf, sizeof numBuf, "%lx", (unsigned long) cs->format);
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_UNKNOWN_FORMAT, numBuf);
        return 0;
      }
    }
    sp->put("\n");
    return len;
}

#endif 

namespace {































struct ExpressionDecompiler
{
    JSContext* cx;
    RootedScript script;
    RootedFunction fun;
    BytecodeParser parser;
    Sprinter sprinter;

    ExpressionDecompiler(JSContext* cx, JSScript* script, JSFunction* fun)
        : cx(cx),
          script(cx, script),
          fun(cx, fun),
          parser(cx, script),
          sprinter(cx)
    {}
    bool init();
    bool decompilePCForStackOperand(jsbytecode* pc, int i);
    bool decompilePC(jsbytecode* pc);
    JSAtom* getLocal(uint32_t local, jsbytecode* pc);
    JSAtom* getArg(unsigned slot);
    JSAtom* loadAtom(jsbytecode* pc);
    bool quote(JSString* s, uint32_t quote);
    bool write(const char* s);
    bool write(JSString* str);
    bool getOutput(char** out);
};

bool
ExpressionDecompiler::decompilePCForStackOperand(jsbytecode* pc, int i)
{
    pc = parser.pcForStackOperand(pc, i);
    if (!pc)
        return write("(intermediate value)");
    return decompilePC(pc);
}

bool
ExpressionDecompiler::decompilePC(jsbytecode* pc)
{
    MOZ_ASSERT(script->containsPC(pc));

    JSOp op = (JSOp)*pc;

    if (const char* token = CodeToken[op]) {
        
        switch (js_CodeSpec[op].nuses) {
          case 2: {
            jssrcnote* sn = GetSrcNote(cx, script, pc);
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
      case JSOP_GETNAME:
      case JSOP_GETINTRINSIC:
        return write(loadAtom(pc));
      case JSOP_GETARG: {
        unsigned slot = GET_ARGNO(pc);
        JSAtom* atom = getArg(slot);
        return write(atom);
      }
      case JSOP_GETLOCAL: {
        uint32_t i = GET_LOCALNO(pc);
        if (JSAtom* atom = getLocal(i, pc))
            return write(atom);
        return write("(intermediate value)");
      }
      case JSOP_GETALIASEDVAR: {
        JSAtom* atom = ScopeCoordinateName(cx->runtime()->scopeCoordinateNameCache, script, pc);
        MOZ_ASSERT(atom);
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
      case JSOP_GETPROP_SUPER:
      {
        RootedAtom prop(cx, loadAtom(pc));
        return write("super.") &&
               quote(prop, '\0');
      }
      case JSOP_GETELEM:
      case JSOP_CALLELEM:
        return decompilePCForStackOperand(pc, -2) &&
               write("[") &&
               decompilePCForStackOperand(pc, -1) &&
               write("]");
      case JSOP_GETELEM_SUPER:
        return write("super[") &&
               decompilePCForStackOperand(pc, -3) &&
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
      case JSOP_SYMBOL: {
        unsigned i = uint8_t(pc[1]);
        MOZ_ASSERT(i < JS::WellKnownSymbolLimit);
        if (i < JS::WellKnownSymbolLimit)
            return write(cx->names().wellKnownSymbolDescriptions()[i]);
        break;
      }
      case JSOP_UNDEFINED:
        return write(js_undefined_str);
      case JSOP_THIS:
        
        
        return write(js_this_str);
      case JSOP_NEWTARGET:
        return write("new.target");
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
      case JSOP_OBJECT:
      case JSOP_NEWARRAY_COPYONWRITE: {
        JSObject* obj = (op == JSOP_REGEXP)
                        ? script->getRegExp(GET_UINT32_INDEX(pc))
                        : script->getObject(GET_UINT32_INDEX(pc));
        RootedValue objv(cx, ObjectValue(*obj));
        JSString* str = ValueToSource(cx, objv);
        if (!str)
            return false;
        return write(str);
      }
      case JSOP_VOID:
        return write("void ") && decompilePCForStackOperand(pc, -1);
      default:
        break;
    }
    return write("(intermediate value)");
}

bool
ExpressionDecompiler::init()
{
    assertSameCompartment(cx, script);

    if (!sprinter.init())
        return false;

    if (!parser.parse())
        return false;

    return true;
}

bool
ExpressionDecompiler::write(const char* s)
{
    return sprinter.put(s) >= 0;
}

bool
ExpressionDecompiler::write(JSString* str)
{
    return sprinter.putString(str) >= 0;
}

bool
ExpressionDecompiler::quote(JSString* s, uint32_t quote)
{
    return QuoteString(&sprinter, s, quote) != nullptr;
}

JSAtom*
ExpressionDecompiler::loadAtom(jsbytecode* pc)
{
    return script->getAtom(GET_UINT32_INDEX(pc));
}

JSAtom*
ExpressionDecompiler::getArg(unsigned slot)
{
    MOZ_ASSERT(fun);
    MOZ_ASSERT(slot < script->bindings.numArgs());

    for (BindingIter bi(script); bi; bi++) {
        MOZ_ASSERT(bi->kind() == Binding::ARGUMENT);
        if (bi.argIndex() == slot)
            return bi->name();
    }

    MOZ_CRASH("No binding");
}

JSAtom*
ExpressionDecompiler::getLocal(uint32_t local, jsbytecode* pc)
{
    MOZ_ASSERT(local < script->nfixed());
    if (local < script->nbodyfixed()) {
        for (BindingIter bi(script); bi; bi++) {
            if (bi->kind() != Binding::ARGUMENT && !bi->aliased() && bi.frameIndex() == local)
                return bi->name();
        }

        MOZ_CRASH("No binding");
    }
    for (NestedScopeObject* chain = script->getStaticBlockScope(pc);
         chain;
         chain = chain->enclosingNestedScope())
    {
        if (!chain->is<StaticBlockObject>())
            continue;
        StaticBlockObject& block = chain->as<StaticBlockObject>();
        if (local < block.localOffset())
            continue;
        local -= block.localOffset();
        if (local >= block.numVariables())
            return nullptr;
        for (Shape::Range<NoGC> r(block.lastProperty()); !r.empty(); r.popFront()) {
            const Shape& shape = r.front();
            if (block.shapeToIndex(shape) == local)
                return JSID_TO_ATOM(shape.propid());
        }
        break;
    }
    return nullptr;
}

bool
ExpressionDecompiler::getOutput(char** res)
{
    ptrdiff_t len = sprinter.stringEnd() - sprinter.stringAt(0);
    *res = cx->pod_malloc<char>(len + 1);
    if (!*res)
        return false;
    js_memcpy(*res, sprinter.stringAt(0), len);
    (*res)[len] = 0;
    return true;
}

}  

static bool
FindStartPC(JSContext* cx, const FrameIter& iter, int spindex, int skipStackHits, Value v,
            jsbytecode** valuepc)
{
    jsbytecode* current = *valuepc;

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

        
        
        
        
        if (index < size_t(parser.stackDepthAtPC(current)))
            return true;

        
        
        
        int stackHits = 0;
        Value s;
        do {
            if (!index)
                return true;
            s = iter.frameSlotValue(--index);
        } while (s != v || stackHits++ != skipStackHits);

        
        
        
        jsbytecode* pc = nullptr;
        if (index < size_t(parser.stackDepthAtPC(current)))
            pc = parser.pcForStackOperand(current, index);
        *valuepc = pc ? pc : current;
    } else {
        jsbytecode* pc = parser.pcForStackOperand(current, spindex);
        *valuepc = pc ? pc : current;
    }
    return true;
}

static bool
DecompileExpressionFromStack(JSContext* cx, int spindex, int skipStackHits, HandleValue v, char** res)
{
    MOZ_ASSERT(spindex < 0 ||
               spindex == JSDVG_IGNORE_STACK ||
               spindex == JSDVG_SEARCH_STACK);

    *res = nullptr;

#ifdef JS_MORE_DETERMINISTIC
    




    return true;
#endif

    FrameIter frameIter(cx);

    if (frameIter.done() || !frameIter.hasScript())
        return true;

    RootedScript script(cx, frameIter.script());
    AutoCompartment ac(cx, &script->global());
    jsbytecode* valuepc = frameIter.pc();
    RootedFunction fun(cx, frameIter.isFunctionFrame()
                           ? frameIter.calleeTemplate()
                           : nullptr);

    MOZ_ASSERT(script->containsPC(valuepc));

    
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

typedef mozilla::UniquePtr<char[], JS::FreePolicy> UniquePtrChars;

UniquePtrChars
js::DecompileValueGenerator(JSContext* cx, int spindex, HandleValue v,
                            HandleString fallbackArg, int skipStackHits)
{
    RootedString fallback(cx, fallbackArg);
    {
        char* result;
        if (!DecompileExpressionFromStack(cx, spindex, skipStackHits, v, &result))
            return nullptr;
        if (result) {
            if (strcmp(result, "(intermediate value)"))
                return UniquePtrChars(result);
            js_free(result);
        }
    }
    if (!fallback) {
        if (v.isUndefined())
            return UniquePtrChars(JS_strdup(cx, js_undefined_str)); 
        fallback = ValueToSource(cx, v);
        if (!fallback)
            return UniquePtrChars(nullptr);
    }

    return UniquePtrChars(JS_EncodeString(cx, fallback));
}

static bool
DecompileArgumentFromStack(JSContext* cx, int formalIndex, char** res)
{
    MOZ_ASSERT(formalIndex >= 0);

    *res = nullptr;

#ifdef JS_MORE_DETERMINISTIC
    
    return true;
#endif

    



    FrameIter frameIter(cx);
    MOZ_ASSERT(!frameIter.done());

    



    ++frameIter;
    if (frameIter.done() || !frameIter.hasScript())
        return true;

    RootedScript script(cx, frameIter.script());
    AutoCompartment ac(cx, &script->global());
    jsbytecode* current = frameIter.pc();
    RootedFunction fun(cx, frameIter.isFunctionFrame()
                           ? frameIter.calleeTemplate()
                           : nullptr);

    MOZ_ASSERT(script->containsPC(current));

    if (current < script->main())
        return true;

    
    if (JSOp(*current) != JSOP_CALL || static_cast<unsigned>(formalIndex) >= GET_ARGC(current))
        return true;

    BytecodeParser parser(cx, script);
    if (!parser.parse())
        return false;

    int formalStackIndex = parser.stackDepthAtPC(current) - GET_ARGC(current) + formalIndex;
    MOZ_ASSERT(formalStackIndex >= 0);
    if (uint32_t(formalStackIndex) >= parser.stackDepthAtPC(current))
        return true;

    ExpressionDecompiler ed(cx, script, fun);
    if (!ed.init())
        return false;
    if (!ed.decompilePCForStackOperand(current, formalStackIndex))
        return false;

    return ed.getOutput(res);
}

char*
js::DecompileArgument(JSContext* cx, int formalIndex, HandleValue v)
{
    {
        char* result;
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

    return JS_EncodeString(cx, fallback);
}

bool
js::CallResultEscapes(jsbytecode* pc)
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

    return *pc != JSOP_IFEQ;
}

extern bool
js::IsValidBytecodeOffset(JSContext* cx, JSScript* script, size_t offset)
{
    
    for (BytecodeRange r(cx, script); !r.empty(); r.popFront()) {
        size_t here = r.frontOffset();
        if (here >= offset)
            return here == offset;
    }
    return false;
}

























static void
ReleaseScriptCounts(FreeOp* fop)
{
    JSRuntime* rt = fop->runtime();
    MOZ_ASSERT(rt->scriptAndCountsVector);

    ScriptAndCountsVector& vec = *rt->scriptAndCountsVector;

    for (size_t i = 0; i < vec.length(); i++)
        vec[i].scriptCounts.destroy(fop);

    fop->delete_(rt->scriptAndCountsVector);
    rt->scriptAndCountsVector = nullptr;
}

JS_FRIEND_API(void)
js::StartPCCountProfiling(JSContext* cx)
{
    JSRuntime* rt = cx->runtime();

    if (rt->profilingScripts)
        return;

    if (rt->scriptAndCountsVector)
        ReleaseScriptCounts(rt->defaultFreeOp());

    ReleaseAllJITCode(rt->defaultFreeOp());

    rt->profilingScripts = true;
}

JS_FRIEND_API(void)
js::StopPCCountProfiling(JSContext* cx)
{
    JSRuntime* rt = cx->runtime();

    if (!rt->profilingScripts)
        return;
    MOZ_ASSERT(!rt->scriptAndCountsVector);

    ReleaseAllJITCode(rt->defaultFreeOp());

    ScriptAndCountsVector* vec = cx->new_<ScriptAndCountsVector>(SystemAllocPolicy());
    if (!vec)
        return;

    for (ZonesIter zone(rt, SkipAtoms); !zone.done(); zone.next()) {
        for (ZoneCellIter i(zone, AllocKind::SCRIPT); !i.done(); i.next()) {
            JSScript* script = i.get<JSScript>();
            if (script->hasScriptCounts() && script->types()) {
                ScriptAndCounts sac;
                sac.script = script;
                sac.scriptCounts.set(script->releaseScriptCounts());
                if (!vec->append(sac))
                    sac.scriptCounts.destroy(rt->defaultFreeOp());
            }
        }
    }

    rt->profilingScripts = false;
    rt->scriptAndCountsVector = vec;
}

JS_FRIEND_API(void)
js::PurgePCCounts(JSContext* cx)
{
    JSRuntime* rt = cx->runtime();

    if (!rt->scriptAndCountsVector)
        return;
    MOZ_ASSERT(!rt->profilingScripts);

    ReleaseScriptCounts(rt->defaultFreeOp());
}

JS_FRIEND_API(size_t)
js::GetPCCountScriptCount(JSContext* cx)
{
    JSRuntime* rt = cx->runtime();

    if (!rt->scriptAndCountsVector)
        return 0;

    return rt->scriptAndCountsVector->length();
}

enum MaybeComma {NO_COMMA, COMMA};

static void
AppendJSONProperty(StringBuffer& buf, const char* name, MaybeComma comma = COMMA)
{
    if (comma)
        buf.append(',');

    buf.append('\"');
    buf.append(name, strlen(name));
    buf.append("\":", 2);
}

static void
AppendArrayJSONProperties(JSContext* cx, StringBuffer& buf,
                          double* values, const char * const* names, unsigned count,
                          MaybeComma& comma)
{
    for (unsigned i = 0; i < count; i++) {
        if (values[i]) {
            AppendJSONProperty(buf, names[i], comma);
            comma = COMMA;
            NumberValueToStringBuffer(cx, DoubleValue(values[i]), buf);
        }
    }
}

JS_FRIEND_API(JSString*)
js::GetPCCountScriptSummary(JSContext* cx, size_t index)
{
    JSRuntime* rt = cx->runtime();

    if (!rt->scriptAndCountsVector || index >= rt->scriptAndCountsVector->length()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BUFFER_TOO_SMALL);
        return nullptr;
    }

    const ScriptAndCounts& sac = (*rt->scriptAndCountsVector)[index];
    RootedScript script(cx, sac.script);

    




    StringBuffer buf(cx);

    buf.append('{');

    AppendJSONProperty(buf, "file", NO_COMMA);
    JSString* str = JS_NewStringCopyZ(cx, script->filename());
    if (!str || !(str = StringToSource(cx, str)))
        return nullptr;
    buf.append(str);

    AppendJSONProperty(buf, "line");
    NumberValueToStringBuffer(cx, Int32Value(script->lineno()), buf);

    if (script->functionNonDelazifying()) {
        JSAtom* atom = script->functionNonDelazifying()->displayAtom();
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

    for (unsigned i = 0; i < script->length(); i++) {
        PCCounts& counts = sac.getPCCounts(script->offsetToPC(i));
        if (!counts)
            continue;

        JSOp op = (JSOp)script->code()[i];
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
                    MOZ_CRASH("Bad opcode");
            } else if (PCCounts::arithOp(op)) {
                arithTotals[j - PCCounts::BASE_LIMIT] += value;
            } else {
                MOZ_CRASH("Bad opcode");
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
    jit::IonScriptCounts* ionCounts = sac.getIonCounts();
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
GetPCCountJSON(JSContext* cx, const ScriptAndCounts& sac, StringBuffer& buf)
{
    RootedScript script(cx, sac.script);

    buf.append('{');
    AppendJSONProperty(buf, "text", NO_COMMA);

    JSString* str = JS_DecompileScript(cx, script, nullptr, 0);
    if (!str || !(str = StringToSource(cx, str)))
        return false;

    buf.append(str);

    AppendJSONProperty(buf, "line");
    NumberValueToStringBuffer(cx, Int32Value(script->lineno()), buf);

    AppendJSONProperty(buf, "opcodes");
    buf.append('[');
    bool comma = false;

    SrcNoteLineScanner scanner(script->notes(), script->lineno());

    for (jsbytecode* pc = script->code(); pc < script->codeEnd(); pc += GetBytecodeLength(pc)) {
        size_t offset = script->pcToOffset(pc);

        JSOp op = (JSOp) *pc;

        if (comma)
            buf.append(',');
        comma = true;

        buf.append('{');

        AppendJSONProperty(buf, "id", NO_COMMA);
        NumberValueToStringBuffer(cx, Int32Value(offset), buf);

        scanner.advanceTo(offset);

        AppendJSONProperty(buf, "line");
        NumberValueToStringBuffer(cx, Int32Value(scanner.getLine()), buf);

        {
            const char* name = js_CodeName[op];
            AppendJSONProperty(buf, "name");
            buf.append('\"');
            buf.append(name, strlen(name));
            buf.append('\"');
        }

        {
            ExpressionDecompiler ed(cx, script, script->functionDelazifying());
            if (!ed.init())
                return false;
            if (!ed.decompilePC(pc))
                return false;
            char* text;
            if (!ed.getOutput(&text))
                return false;
            AppendJSONProperty(buf, "text");
            JSString* str = JS_NewStringCopyZ(cx, text);
            js_free(text);
            if (!str || !(str = StringToSource(cx, str)))
                return false;
            buf.append(str);
        }

        PCCounts& counts = sac.getPCCounts(pc);
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

    jit::IonScriptCounts* ionCounts = sac.getIonCounts();
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
                const jit::IonBlockCounts& block = ionCounts->block(i);

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
                JSString* str = JS_NewStringCopyZ(cx, block.code());
                if (!str || !(str = StringToSource(cx, str)))
                    return false;
                buf.append(str);
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

JS_FRIEND_API(JSString*)
js::GetPCCountScriptContents(JSContext* cx, size_t index)
{
    JSRuntime* rt = cx->runtime();

    if (!rt->scriptAndCountsVector || index >= rt->scriptAndCountsVector->length()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BUFFER_TOO_SMALL);
        return nullptr;
    }

    const ScriptAndCounts& sac = (*rt->scriptAndCountsVector)[index];
    JSScript* script = sac.script;

    StringBuffer buf(cx);

    {
        AutoCompartment ac(cx, &script->global());
        if (!GetPCCountJSON(cx, sac, buf))
            return nullptr;
    }

    return buf.finishString();
}
