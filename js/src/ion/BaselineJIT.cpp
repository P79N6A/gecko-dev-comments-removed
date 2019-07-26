






#include "BaselineCompiler.h"
#include "BaselineIC.h"
#include "BaselineJIT.h"
#include "CompileInfo.h"

using namespace js;
using namespace js::ion;

BaselineScript::BaselineScript()
  : method_(NULL)
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

static IonExecStatus
EnterBaseline(JSContext *cx, StackFrame *fp, void *jitcode)
{
    JS_CHECK_RECURSION(cx, return IonExec_Error);
    JS_ASSERT(ion::IsEnabled(cx));
    

    EnterIonCode enter = cx->compartment->ionCompartment()->enterJITInfallible();

    
    
    int maxArgc = 0;
    Value *maxArgv = NULL;
    int numActualArgs = 0;

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
        calleeToken = CalleeToToken(fp->script().unsafeGet());
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

MethodStatus
ion::CanEnterBaselineJIT(JSContext *cx, HandleScript script, StackFrame *fp)
{
    if (script->baseline)
        return Method_Compiled;

    if (!cx->compartment->ensureIonCompartmentExists(cx))
        return Method_Error;

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

    MethodStatus status = compiler.compile();
    if (status != Method_Compiled)
        return status;

    
    if (!cx->compartment->ionCompartment()->enterJIT(cx))
        return Method_Error;

    if (!script->baseline)
        return Method_Skipped;

    return Method_Compiled;
}


static const unsigned DataAlignment = sizeof(uintptr_t);

BaselineScript *
BaselineScript::New(JSContext *cx, size_t icEntries)
{
    size_t icEntriesSize = icEntries * sizeof(ICEntry);
    size_t paddedICEntriesSize = AlignBytes(icEntriesSize, DataAlignment);

    size_t allocBytes = sizeof(BaselineScript) + paddedICEntriesSize;

    uint8_t *buffer = (uint8_t *)cx->malloc_(allocBytes);
    if (!buffer)
        return NULL;

    BaselineScript *script = reinterpret_cast<BaselineScript *>(buffer);
    new (script) BaselineScript();

    uint8_t *scriptEnd = buffer + sizeof(BaselineScript);
    uint8_t *icEntryStart = (uint8_t *) AlignBytes((uintptr_t) scriptEnd, DataAlignment);

    script->icEntriesOffset_ = (uint32_t) (icEntryStart - buffer);
    script->icEntries_ = icEntries;

    return script;
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

void
BaselineScript::copyICEntries(const ICEntry *entries, MacroAssembler &masm)
{
    
    
    for (uint32_t i = 0; i < numICEntries(); i++) {
        ICEntry &realEntry = icEntry(i);
        realEntry = entries[i];
        realEntry.fixupReturnOffset(masm);

        
        
        
        
        
        if (realEntry.firstStub()->isFallback())
            realEntry.firstStub()->toFallbackStub()->fixupICEntry(&realEntry);
    }
}
