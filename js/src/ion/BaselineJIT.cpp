/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BaselineCompiler.h"
#include "BaselineIC.h"
#include "BaselineJIT.h"
#include "CompileInfo.h"
#include "IonSpewer.h"
#include "IonFrames-inl.h"

#include "vm/Stack-inl.h"

#include "jsopcodeinlines.h"

using namespace js;
using namespace js::ion;

/* static */ ICStubSpace *
ICStubSpace::FallbackStubSpaceFor(JSScript *script)
{
    JS_ASSERT(script->hasBaselineScript());
    return script->baselineScript()->fallbackStubSpace();
}

/* static */ ICStubSpace *
ICStubSpace::StubSpaceFor(JSScript *script)
{
    JS_ASSERT(script->hasBaselineScript());
    return script->baselineScript()->optimizedStubSpace();
}

/* static */ PCMappingEntry::SlotLocation
PCMappingEntry::ToSlotLocation(const StackValue *stackVal)
{
    if (stackVal->kind() == StackValue::Register) {
        if (stackVal->reg() == R0)
            return SlotInR0;
        JS_ASSERT(stackVal->reg() == R1);
        return SlotInR1;
    }
    JS_ASSERT(stackVal->kind() != StackValue::Stack);
    return SlotIgnore;
}

BaselineScript::BaselineScript(uint32_t prologueOffset)
  : method_(NULL),
    fallbackStubSpace_(),
    optimizedStubSpace_(),
    prologueOffset_(prologueOffset),
    active_(false)
{ }

static const size_t BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 1 << 12; //XXX

// XXX copied from Ion.cpp
class AutoDestroyAllocator
{
    LifoAlloc *alloc;

  public:
    AutoDestroyAllocator(LifoAlloc *alloc) : alloc(alloc) {}

    void cancel()
    {
        alloc = NULL;
    }

    ~AutoDestroyAllocator()
    {
        if (alloc)
            js_delete(alloc);
    }
};

static bool
CheckFrame(StackFrame *fp)
{
    if (fp->isEvalFrame()) {
        // Eval frames are not yet supported.
        IonSpew(IonSpew_BaselineAbort, "BASELINE FIXME: eval frame!");
        return false;
    }

    if (fp->isGeneratorFrame()) {
        IonSpew(IonSpew_BaselineAbort, "generator frame");
        return false;
    }

    if (fp->isDebuggerFrame()) {
        // Debugger eval-in-frame. These are likely short-running scripts so
        // don't bother compiling them for now.
        IonSpew(IonSpew_BaselineAbort, "debugger frame");
        return false;
    }

    return true;
}

static IonExecStatus
EnterBaseline(JSContext *cx, StackFrame *fp, void *jitcode)
{
    JS_CHECK_RECURSION(cx, return IonExec_Aborted);
    JS_ASSERT(ion::IsBaselineEnabled(cx));
    JS_ASSERT(CheckFrame(fp));

    EnterIonCode enter = cx->compartment->ionCompartment()->enterJIT();

    // maxArgc is the maximum of arguments between the number of actual
    // arguments and the number of formal arguments. It accounts for |this|.
    int maxArgc = 0;
    Value *maxArgv = NULL;
    int numActualArgs = 0;
    RootedValue thisv(cx);

    void *calleeToken;
    if (fp->isFunctionFrame()) {
        // CountArgSlot include |this| and the |scopeChain|.
        maxArgc = CountArgSlots(fp->fun()) - 1; // -1 = discard |scopeChain|
        maxArgv = fp->formals() - 1;            // -1 = include |this|

        // Formal arguments are the argument corresponding to the function
        // definition and actual arguments are corresponding to the call-site
        // arguments.
        numActualArgs = fp->numActualArgs();

        // We do not need to handle underflow because formal arguments are pad
        // with |undefined| values but we need to distinguish between the
        if (fp->hasOverflowArgs()) {
            int formalArgc = maxArgc;
            Value *formalArgv = maxArgv;
            maxArgc = numActualArgs + 1; // +1 = include |this|
            maxArgv = fp->actuals() - 1; // -1 = include |this|

            // The beginning of the actual args is not updated, so we just copy
            // the formal args into the actual args to get a linear vector which
            // can be copied by generateEnterJit.
            memcpy(maxArgv, formalArgv, formalArgc * sizeof(Value));
        }
        calleeToken = CalleeToToken(&fp->callee());
    } else {
        calleeToken = CalleeToToken(fp->script());
        thisv = fp->thisValue();
        maxArgc = 1;
        maxArgv = thisv.address();
    }

    // Caller must construct |this| before invoking the Ion function.
    JS_ASSERT_IF(fp->isConstructing(), fp->functionThis().isObject());

    Value result = Int32Value(numActualArgs);
    {
        AssertCompartmentUnchanged pcc(cx);
        IonContext ictx(cx, cx->compartment, NULL);
        IonActivation activation(cx, fp);
        JSAutoResolveFlags rf(cx, RESOLVE_INFER);

        // Single transition point from Interpreter to Ion.
        enter(jitcode, maxArgc, maxArgv, fp, calleeToken, &result);
    }

    JS_ASSERT(fp == cx->fp());
    JS_ASSERT(!cx->runtime->hasIonReturnOverride());

    // The trampoline wrote the return value but did not set the HAS_RVAL flag.
    fp->setReturnValue(result);

    // Ion callers wrap primitive constructor return.
    if (!result.isMagic() && fp->isConstructing() && fp->returnValue().isPrimitive())
        fp->setReturnValue(ObjectValue(fp->constructorThis()));

    JS_ASSERT_IF(result.isMagic(), result.isMagic(JS_ION_ERROR));
    return result.isMagic() ? IonExec_Error : IonExec_Ok;
}

IonExecStatus
ion::EnterBaselineMethod(JSContext *cx, StackFrame *fp)
{
    RootedScript script(cx, fp->script());
    BaselineScript *baseline = script->baseline;
    IonCode *code = baseline->method();
    void *jitcode = code->raw();

    return EnterBaseline(cx, fp, jitcode);
}

static MethodStatus
BaselineCompile(JSContext *cx, HandleScript script, StackFrame *fp)
{
    JS_ASSERT(!script->baseline);

    LifoAlloc *alloc = cx->new_<LifoAlloc>(BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE);
    if (!alloc)
        return Method_Error;

    AutoDestroyAllocator autoDestroy(alloc);

    TempAllocator *temp = alloc->new_<TempAllocator>(alloc);
    if (!temp)
        return Method_Error;

    IonContext ictx(cx, cx->compartment, temp);

    BaselineCompiler compiler(cx, script);
    if (!compiler.init())
        return Method_Error;

    AutoFlushCache afc("BaselineJIT", cx->compartment->ionCompartment());
    MethodStatus status = compiler.compile();

    JS_ASSERT_IF(status == Method_Compiled, script->baseline);
    JS_ASSERT_IF(status != Method_Compiled, !script->baseline);

    if (status == Method_CantCompile)
        script->baseline = BASELINE_DISABLED_SCRIPT;

    return status;
}

MethodStatus
ion::CanEnterBaselineJIT(JSContext *cx, JSScript *scriptArg, StackFrame *fp, bool newType)
{
    // Skip if baseline compilation is disabledf in options.
    JS_ASSERT(ion::IsBaselineEnabled(cx));

    // Skip if the script has been disabled.
    if (scriptArg->baseline == BASELINE_DISABLED_SCRIPT)
        return Method_Skipped;

    RootedScript script(cx, scriptArg);

    // If constructing, allocate a new |this| object.
    if (fp->isConstructing() && fp->functionThis().isPrimitive()) {
        RootedObject callee(cx, &fp->callee());
        RootedObject obj(cx, js_CreateThisForFunction(cx, callee, newType));
        if (!obj)
            return Method_Skipped;
        fp->functionThis().setObject(*obj);
    }

    if (!CheckFrame(fp))
        return Method_CantCompile;

    if (!cx->compartment->ensureIonCompartmentExists(cx))
        return Method_Error;

    if (script->hasBaselineScript())
        return Method_Compiled;

    return BaselineCompile(cx, script, fp);
}

// Be safe, align IC entry list to 8 in all cases.
static const unsigned DataAlignment = sizeof(uintptr_t);

BaselineScript *
BaselineScript::New(JSContext *cx, uint32_t prologueOffset, size_t icEntries, size_t pcMappingEntries)
{
    size_t paddedBaselineScriptSize = AlignBytes(sizeof(BaselineScript), DataAlignment);

    size_t icEntriesSize = icEntries * sizeof(ICEntry);
    size_t pcMappingEntriesSize = pcMappingEntries * sizeof(PCMappingEntry);

    size_t paddedICEntriesSize = AlignBytes(icEntriesSize, DataAlignment);
    size_t paddedPCMappingEntriesSize = AlignBytes(pcMappingEntriesSize, DataAlignment);

    size_t allocBytes = paddedBaselineScriptSize +
        paddedICEntriesSize +
        paddedPCMappingEntriesSize;

    uint8_t *buffer = (uint8_t *)cx->malloc_(allocBytes);
    if (!buffer)
        return NULL;

    BaselineScript *script = reinterpret_cast<BaselineScript *>(buffer);
    new (script) BaselineScript(prologueOffset);

    size_t offsetCursor = paddedBaselineScriptSize;

    script->icEntriesOffset_ = offsetCursor;
    script->icEntries_ = icEntries;
    offsetCursor += paddedICEntriesSize;

    script->pcMappingOffset_ = offsetCursor;
    script->pcMappingEntries_ = pcMappingEntries;
    offsetCursor += paddedPCMappingEntriesSize;

    return script;
}

void
BaselineScript::trace(JSTracer *trc)
{
    MarkIonCode(trc, &method_, "baseline-method");

    // Mark all IC stub codes hanging off the IC stub entries.
    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &ent = icEntry(i);
        if (!ent.hasStub())
            continue;
        for (ICStub *stub = ent.firstStub(); stub; stub = stub->next())
            stub->trace(trc);
    }
}

void
BaselineScript::Trace(JSTracer *trc, BaselineScript *script)
{
    script->trace(trc);
}

void
BaselineScript::Destroy(FreeOp *fop, BaselineScript *script)
{
    fop->free_(script);
}

ICEntry &
BaselineScript::icEntry(size_t index)
{
    JS_ASSERT(index < numICEntries());
    return icEntryList()[index];
}

ICEntry &
BaselineScript::icEntryFromReturnOffset(CodeOffsetLabel returnOffset)
{
    // FIXME: Change this to something better than linear search (binary probe)?
    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &entry = icEntry(i);
        if (entry.returnOffset().offset() == returnOffset.offset())
            return entry;
    }

    JS_NOT_REACHED("No cache");
    return icEntry(0);
}

uint8_t *
BaselineScript::returnAddressForIC(const ICEntry &ent)
{
    return method()->raw() + ent.returnOffset().offset();
}

ICEntry &
BaselineScript::icEntryFromPCOffset(uint32_t pcOffset)
{
    // FIXME: Change this to something better than linear search (binary probe)?
    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &entry = icEntry(i);
        if (entry.pcOffset() == pcOffset)
            return entry;
    }

    JS_NOT_REACHED("No cache");
    return icEntry(0);
}

ICEntry &
BaselineScript::icEntryFromReturnAddress(uint8_t *returnAddr)
{
    JS_ASSERT(returnAddr > method_->raw());
    CodeOffsetLabel offset(returnAddr - method_->raw());
    return icEntryFromReturnOffset(offset);
}

void
BaselineScript::copyICEntries(const ICEntry *entries, MacroAssembler &masm)
{
    // Fix up the return offset in the IC entries and copy them in.
    // Also write out the IC entry ptrs in any fallback stubs that were added.
    for (uint32_t i = 0; i < numICEntries(); i++) {
        ICEntry &realEntry = icEntry(i);
        realEntry = entries[i];
        realEntry.fixupReturnOffset(masm);

        if (!realEntry.hasStub()) {
            // VM call without any stubs.
            continue;
        }

        // If the attached stub is a fallback stub, then fix it up with
        // a pointer to the (now available) realEntry.
        if (realEntry.firstStub()->isFallback())
            realEntry.firstStub()->toFallbackStub()->fixupICEntry(&realEntry);
    }
}

void
BaselineScript::adoptFallbackStubs(ICStubSpace *stubSpace)
{
    fallbackStubSpace_.adoptFrom(stubSpace);
}

PCMappingEntry &
BaselineScript::pcMappingEntry(size_t index)
{
    JS_ASSERT(index < numPCMappingEntries());
    return pcMappingEntryList()[index];
}

void
BaselineScript::copyPCMappingEntries(const PCMappingEntry *entries, MacroAssembler &masm)
{
    for (uint32_t i = 0; i < numPCMappingEntries(); i++) {
        PCMappingEntry &ent = pcMappingEntry(i);
        ent = entries[i];
        ent.fixupNativeOffset(masm);
    }
}

uint8_t *
BaselineScript::nativeCodeForPC(HandleScript script, jsbytecode *pc)
{
    JS_ASSERT(script->baseline == this);

    uint32_t pcOffset = pc - script->code;
    for (size_t i = 0; i < numPCMappingEntries(); i++) {
        PCMappingEntry &entry = pcMappingEntry(i);
        if (entry.pcOffset == pcOffset)
            return method_->raw() + entry.nativeOffset;
    }

    JS_NOT_REACHED("Invalid pc");
    return NULL;
}

uint8_t
BaselineScript::slotInfoForPC(HandleScript script, jsbytecode *pc)
{
    JS_ASSERT(script->baseline == this);

    uint32_t pcOffset = pc - script->code;
    for (size_t i = 0; i < numPCMappingEntries(); i++) {
        PCMappingEntry &entry = pcMappingEntry(i);
        if (entry.pcOffset == pcOffset)
            return entry.slotInfo;
    }

    JS_NOT_REACHED("Invalid pc");
    return 0xff;
}

void
BaselineScript::toggleDebugTraps(UnrootedScript script, jsbytecode *pc)
{
    JS_ASSERT(script->baseline == this);

    SrcNoteLineScanner scanner(script->notes(), script->lineno);
    uint32_t pcOffset = pc ? pc - script->code : 0;

    IonContext ictx(NULL, script->compartment(), NULL);
    AutoFlushCache afc("DebugTraps");

    for (size_t i = 0; i < numPCMappingEntries(); i++) {
        PCMappingEntry &entry = pcMappingEntry(i);

        // If we have a pc, only toggle traps at that pc.
        if (pc && pcOffset != entry.pcOffset)
            continue;

        scanner.advanceTo(entry.pcOffset);

        jsbytecode *trapPc = script->code + entry.pcOffset;
        bool enabled = (script->stepModeEnabled() && scanner.isLineHeader()) ||
            script->hasBreakpointsAt(trapPc);

        // Patch the trap.
        CodeLocationLabel label(method(), entry.nativeOffset);
        Assembler::ToggleCall(label, enabled);
    }
}

void
ion::FinishDiscardBaselineScript(FreeOp *fop, UnrootedScript script)
{
    if (!script->hasBaselineScript())
        return;

    if (script->baseline->active()) {
        // Script is live on the stack, don't destroy it. Reset |active| flag so
        // that we don't need a separate script iteration to unmark them.
        script->baseline->resetActive();
        return;
    }

    BaselineScript::Destroy(fop, script->baseline);
    script->baseline = NULL;
}

void
ion::IonCompartment::toggleBaselineStubBarriers(bool enabled)
{
    for (ICStubCodeMap::Enum e(*stubCodes_); !e.empty(); e.popFront()) {
        IonCode *code = *e.front().value.unsafeGet();
        code->togglePreBarriers(enabled);
    }
}
