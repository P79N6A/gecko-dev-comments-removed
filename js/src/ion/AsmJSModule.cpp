





#include "ion/AsmJSModule.h"
#include "ion/IonCode.h"

#include "jsobjinlines.h"

using namespace js;

static void AsmJSModuleObject_finalize(FreeOp *fop, JSObject *obj);
static void AsmJSModuleObject_trace(JSTracer *trc, JSObject *obj);

static const unsigned ASM_CODE_RESERVED_SLOT = 0;
static const unsigned ASM_CODE_NUM_RESERVED_SLOTS = 1;

static Class AsmJSModuleClass = {
    "AsmJSModuleObject",
    JSCLASS_IS_ANONYMOUS | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(ASM_CODE_NUM_RESERVED_SLOTS),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    NULL,                    
    AsmJSModuleObject_finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    AsmJSModuleObject_trace
};

bool
js::IsAsmJSModuleObject(JSObject *obj)
{
    return obj->getClass() == &AsmJSModuleClass;
}

AsmJSModule &
js::AsmJSModuleObjectToModule(JSObject *obj)
{
    JS_ASSERT(IsAsmJSModuleObject(obj));
    return *(AsmJSModule *)obj->getReservedSlot(ASM_CODE_RESERVED_SLOT).toPrivate();
}

static void
AsmJSModuleObject_finalize(FreeOp *fop, JSObject *obj)
{
    fop->delete_(&AsmJSModuleObjectToModule(obj));
}

static void
AsmJSModuleObject_trace(JSTracer *trc, JSObject *obj)
{
    AsmJSModuleObjectToModule(obj).trace(trc);
}

JSObject *
js::NewAsmJSModuleObject(JSContext *cx, ScopedJSDeletePtr<AsmJSModule> *module)
{
    JSObject *obj = NewObjectWithGivenProto(cx, &AsmJSModuleClass, NULL, NULL);
    if (!obj)
        return NULL;

    obj->setReservedSlot(ASM_CODE_RESERVED_SLOT, PrivateValue(module->forget()));
    return obj;
}

void
AsmJSModule::patchHeapAccesses(ArrayBufferObject *heap, JSContext *cx)
{
    JS_ASSERT(IsPowerOfTwo(heap->byteLength()));
#if defined(JS_CPU_X86)
    void *heapOffset = (void*)heap->dataPointer();
    void *heapLength = (void*)heap->byteLength();
    for (unsigned i = 0; i < heapAccesses_.length(); i++) {
        JSC::X86Assembler::setPointer(heapAccesses_[i].patchLengthAt(code_), heapLength);
        JSC::X86Assembler::setPointer(heapAccesses_[i].patchOffsetAt(code_), heapOffset);
    }
#elif defined(JS_CPU_ARM)
    ion::IonContext ic(cx, NULL);
    ion::AutoFlushCache afc("patchBoundsCheck");
    uint32_t bits = mozilla::CeilingLog2(heap->byteLength());
    for (unsigned i = 0; i < heapAccesses_.length(); i++)
        ion::Assembler::updateBoundsCheck(bits, (ion::Instruction*)(heapAccesses_[i].offset() + code_));
#endif
}

AsmJSModule::~AsmJSModule()
{
    if (code_) {
        for (unsigned i = 0; i < numExits(); i++) {
            AsmJSModule::ExitDatum &exitDatum = exitIndexToGlobalDatum(i);
            if (!exitDatum.fun)
                continue;

            if (!exitDatum.fun->hasScript())
                continue;

            JSScript *script = exitDatum.fun->nonLazyScript();
            if (!script->hasIonScript())
                continue;

            ion::DependentAsmJSModuleExit exit(this, i);
            script->ionScript()->removeDependentAsmJSModule(exit);
        }
    }

    for (size_t i = 0; i < numFunctionCounts(); i++)
        js_delete(functionCounts(i));
}
