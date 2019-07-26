






#include "BaselineCompiler.h"
#include "BaselineIC.h"
#include "BaselineJIT.h"
#include "CompileInfo.h"
#include "IonSpewer.h"

#include "vm/Stack-inl.h"

#include "jsopcodeinlines.h"

using namespace js;
using namespace js::ion;

 ICStubSpace *
ICStubSpace::FallbackStubSpaceFor(JSScript *script)
{
    JS_ASSERT(script->hasBaselineScript());
    return script->baselineScript()->fallbackStubSpace();
}

 ICStubSpace *
ICStubSpace::StubSpaceFor(JSScript *script)
{
    JS_ASSERT(script->hasBaselineScript());
    return script->baselineScript()->optimizedStubSpace();
}


BaselineScript::BaselineScript()
  : method_(NULL),
    fallbackStubSpace_(),
    optimizedStubSpace_(),
    active_(false)
{ }

static const size_t BUILDER_LIFO_ALLOC_PRIMARY_CHUNK_SIZE = 1 << 12; 


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
        
        IonSpew(IonSpew_BaselineAbort, "BASELINE FIXME: eval frame!");
        return false;
    }

    if (fp->isGeneratorFrame()) {
        IonSpew(IonSpew_BaselineAbort, "generator frame");
        return false;
    }

    if (fp->isDebuggerFrame()) {
        IonSpew(IonSpew_BaselineAbort, "BASELINE FIXME: debugger frame!");
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

    
    
    int maxArgc = 0;
    Value *maxArgv = NULL;
    int numActualArgs = 0;
    RootedValue thisv(cx);

    void *calleeToken;
    if (fp->isFunctionFrame()) {
        
        maxArgc = CountArgSlots(fp->fun()) - 1; 
        maxArgv = fp->formals() - 1;            

        
        
        
        numActualArgs = fp->numActualArgs();

        
        
        if (fp->hasOverflowArgs()) {
            int formalArgc = maxArgc;
            Value *formalArgv = maxArgv;
            maxArgc = numActualArgs + 1; 
            maxArgv = fp->actuals() - 1; 

            
            
            
            memcpy(maxArgv, formalArgv, formalArgc * sizeof(Value));
        }
        calleeToken = CalleeToToken(&fp->callee());
    } else {
        calleeToken = CalleeToToken(fp->script());
        thisv = fp->thisValue();
        maxArgc = 1;
        maxArgv = thisv.address();
    }

    
    JS_ASSERT_IF(fp->isConstructing(), fp->functionThis().isObject());

    Value result = Int32Value(numActualArgs);
    {
        AssertCompartmentUnchanged pcc(cx);
        IonContext ictx(cx, cx->compartment, NULL);
        IonActivation activation(cx, fp);
        JSAutoResolveFlags rf(cx, RESOLVE_INFER);

        
        enter(jitcode, maxArgc, maxArgv, fp, calleeToken, &result);
    }

    JS_ASSERT(fp == cx->fp());
    JS_ASSERT(!cx->runtime->hasIonReturnOverride());

    
    fp->setReturnValue(result);

    
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
ion::CanEnterBaselineJIT(JSContext *cx, HandleScript script, StackFrame *fp, bool newType)
{
    
    JS_ASSERT(ion::IsBaselineEnabled(cx));

    
    if (script->baseline == BASELINE_DISABLED_SCRIPT)
        return Method_Skipped;

    if (cx->compartment->debugMode()) {
        IonSpew(IonSpew_BaselineAbort, "BASELINE FIXME: Not compiling in debug mode!");
        return Method_CantCompile;
    }

    
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


static const unsigned DataAlignment = sizeof(uintptr_t);

BaselineScript *
BaselineScript::New(JSContext *cx, size_t icEntries, size_t pcMappingEntries)
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
    new (script) BaselineScript();

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
    
    for (size_t i = 0; i < numICEntries(); i++) {
        ICEntry &entry = icEntry(i);
        if (entry.returnOffset().offset() == returnOffset.offset())
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
    
    
    for (uint32_t i = 0; i < numICEntries(); i++) {
        ICEntry &realEntry = icEntry(i);
        realEntry = entries[i];
        realEntry.fixupReturnOffset(masm);

        if (!realEntry.hasStub()) {
            
            continue;
        }

        
        
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
BaselineScript::copyPCMappingEntries(const PCMappingEntry *entries)
{
    for (uint32_t i = 0; i < numPCMappingEntries(); i++)
        pcMappingEntry(i) = entries[i];
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

        
        if (pc && pcOffset != entry.pcOffset)
            continue;

        scanner.advanceTo(entry.pcOffset);

        jsbytecode *trapPc = script->code + entry.pcOffset;
        bool enabled = (script->stepModeEnabled() && scanner.isLineHeader()) ||
            script->hasBreakpointsAt(trapPc);

        
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
        
        
        script->baseline->resetActive();
        return;
    }

    BaselineScript::Destroy(fop, script->baseline);
    script->baseline = NULL;
}
