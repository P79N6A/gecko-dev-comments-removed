





#include "jit/AsmJSLink.h"

#ifdef MOZ_VTUNE
# include "vtune/VTuneWrapper.h"
#endif

#include "jscntxt.h"
#include "jsmath.h"
#include "jswrapper.h"

#include "frontend/BytecodeCompiler.h"
#include "jit/AsmJSModule.h"
#include "jit/Ion.h"
#include "jit/PerfSpewer.h"

#include "jsobjinlines.h"

using namespace js;
using namespace js::jit;

using mozilla::IsNaN;

static const unsigned MODULE_FUN_SLOT = 0;

static bool
LinkFail(JSContext *cx, const char *str)
{
    JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING, js_GetErrorMessage,
                                 NULL, JSMSG_USE_ASM_LINK_FAIL, str);
    return false;
}

static bool
GetDataProperty(JSContext *cx, const Value &objVal, HandlePropertyName field, MutableHandleValue v)
{
    if (!objVal.isObject())
        return LinkFail(cx, "accessing property of non-object");

    Rooted<JSPropertyDescriptor> desc(cx);
    if (!JS_GetPropertyDescriptorById(cx, &objVal.toObject(), NameToId(field), 0, &desc))
        return false;

    if (!desc.object())
        return LinkFail(cx, "property not present on object");

    if (desc.hasGetterOrSetterObject())
        return LinkFail(cx, "property is not a data property");

    v.set(desc.value());
    return true;
}

static bool
ValidateGlobalVariable(JSContext *cx, const AsmJSModule &module, AsmJSModule::Global &global,
                       HandleValue importVal)
{
    JS_ASSERT(global.which() == AsmJSModule::Global::Variable);

    void *datum = module.globalVarIndexToGlobalDatum(global.varIndex());

    switch (global.varInitKind()) {
      case AsmJSModule::Global::InitConstant: {
        const Value &v = global.varInitConstant();
        if (v.isInt32())
            *(int32_t *)datum = v.toInt32();
        else
            *(double *)datum = v.toDouble();
        break;
      }
      case AsmJSModule::Global::InitImport: {
        RootedPropertyName field(cx, global.varImportField());
        RootedValue v(cx);
        if (!GetDataProperty(cx, importVal, field, &v))
            return false;

        switch (global.varImportCoercion()) {
          case AsmJS_ToInt32:
            if (!ToInt32(cx, v, (int32_t *)datum))
                return false;
            break;
          case AsmJS_ToNumber:
            if (!ToNumber(cx, v, (double *)datum))
                return false;
            break;
        }
        break;
      }
    }

    return true;
}

static bool
ValidateFFI(JSContext *cx, AsmJSModule::Global &global, HandleValue importVal,
            AutoObjectVector *ffis)
{
    RootedPropertyName field(cx, global.ffiField());
    RootedValue v(cx);
    if (!GetDataProperty(cx, importVal, field, &v))
        return false;

    if (!v.isObject() || !v.toObject().is<JSFunction>())
        return LinkFail(cx, "FFI imports must be functions");

    (*ffis)[global.ffiIndex()] = &v.toObject().as<JSFunction>();
    return true;
}

static bool
ValidateArrayView(JSContext *cx, AsmJSModule::Global &global, HandleValue globalVal,
                  HandleValue bufferVal)
{
    RootedPropertyName field(cx, global.viewName());
    RootedValue v(cx);
    if (!GetDataProperty(cx, globalVal, field, &v))
        return false;

    if (!IsTypedArrayConstructor(v, global.viewType()))
        return LinkFail(cx, "bad typed array constructor");

    return true;
}

static bool
ValidateMathBuiltin(JSContext *cx, AsmJSModule::Global &global, HandleValue globalVal)
{
    RootedValue v(cx);
    if (!GetDataProperty(cx, globalVal, cx->names().Math, &v))
        return false;
    RootedPropertyName field(cx, global.mathName());
    if (!GetDataProperty(cx, v, field, &v))
        return false;

    Native native = NULL;
    switch (global.mathBuiltin()) {
      case AsmJSMathBuiltin_sin: native = math_sin; break;
      case AsmJSMathBuiltin_cos: native = math_cos; break;
      case AsmJSMathBuiltin_tan: native = math_tan; break;
      case AsmJSMathBuiltin_asin: native = math_asin; break;
      case AsmJSMathBuiltin_acos: native = math_acos; break;
      case AsmJSMathBuiltin_atan: native = math_atan; break;
      case AsmJSMathBuiltin_ceil: native = js_math_ceil; break;
      case AsmJSMathBuiltin_floor: native = js_math_floor; break;
      case AsmJSMathBuiltin_exp: native = math_exp; break;
      case AsmJSMathBuiltin_log: native = math_log; break;
      case AsmJSMathBuiltin_pow: native = js_math_pow; break;
      case AsmJSMathBuiltin_sqrt: native = js_math_sqrt; break;
      case AsmJSMathBuiltin_abs: native = js_math_abs; break;
      case AsmJSMathBuiltin_atan2: native = math_atan2; break;
      case AsmJSMathBuiltin_imul: native = math_imul; break;
    }

    if (!IsNativeFunction(v, native))
        return LinkFail(cx, "bad Math.* builtin");

    return true;
}

static bool
ValidateGlobalConstant(JSContext *cx, AsmJSModule::Global &global, HandleValue globalVal)
{
    RootedPropertyName field(cx, global.constantName());
    RootedValue v(cx);
    if (!GetDataProperty(cx, globalVal, field, &v))
        return false;

    if (!v.isNumber())
        return LinkFail(cx, "global constant value needs to be a number");

    
    if (IsNaN(global.constantValue())) {
        if (!IsNaN(v.toNumber()))
            return LinkFail(cx, "global constant value needs to be NaN");
    } else {
        if (v.toNumber() != global.constantValue())
            return LinkFail(cx, "global constant value mismatch");
    }

    return true;
}

static bool
DynamicallyLinkModule(JSContext *cx, CallArgs args, AsmJSModule &module)
{
    if (module.isLinked())
        return LinkFail(cx, "As a temporary limitation, modules cannot be linked more than "
                            "once. This limitation should be removed in a future release. To "
                            "work around this, compile a second module (e.g., using the "
                            "Function constructor).");
    module.setIsLinked();

    RootedValue globalVal(cx, UndefinedValue());
    if (args.length() > 0)
        globalVal = args[0];

    RootedValue importVal(cx, UndefinedValue());
    if (args.length() > 1)
        importVal = args[1];

    RootedValue bufferVal(cx, UndefinedValue());
    if (args.length() > 2)
        bufferVal = args[2];

    Rooted<ArrayBufferObject*> heap(cx);
    if (module.hasArrayView()) {
        if (!IsTypedArrayBuffer(bufferVal))
            return LinkFail(cx, "bad ArrayBuffer argument");

        heap = &bufferVal.toObject().as<ArrayBufferObject>();

        if (!IsPowerOfTwo(heap->byteLength()) || heap->byteLength() < AsmJSAllocationGranularity)
            return LinkFail(cx, "ArrayBuffer byteLength must be a power of two greater than or equal to 4096");

        
        
        JS_ASSERT((module.minHeapLength() - 1) <= INT32_MAX);
        if (heap->byteLength() < module.minHeapLength())
            return LinkFail(cx, "ArrayBuffer byteLength less than the largest source code heap length constraint.");

        if (!ArrayBufferObject::prepareForAsmJS(cx, heap))
            return LinkFail(cx, "Unable to prepare ArrayBuffer for asm.js use");

        module.initHeap(heap, cx);
    }

    AutoObjectVector ffis(cx);
    if (!ffis.resize(module.numFFIs()))
        return false;

    for (unsigned i = 0; i < module.numGlobals(); i++) {
        AsmJSModule::Global &global = module.global(i);
        switch (global.which()) {
          case AsmJSModule::Global::Variable:
            if (!ValidateGlobalVariable(cx, module, global, importVal))
                return false;
            break;
          case AsmJSModule::Global::FFI:
            if (!ValidateFFI(cx, global, importVal, &ffis))
                return false;
            break;
          case AsmJSModule::Global::ArrayView:
            if (!ValidateArrayView(cx, global, globalVal, bufferVal))
                return false;
            break;
          case AsmJSModule::Global::MathBuiltin:
            if (!ValidateMathBuiltin(cx, global, globalVal))
                return false;
            break;
          case AsmJSModule::Global::Constant:
            if (!ValidateGlobalConstant(cx, global, globalVal))
                return false;
            break;
        }
    }

    for (unsigned i = 0; i < module.numExits(); i++)
        module.exitIndexToGlobalDatum(i).fun = &ffis[module.exit(i).ffiIndex()]->as<JSFunction>();

    return true;
}

AsmJSActivation::AsmJSActivation(JSContext *cx, AsmJSModule &module)
  : cx_(cx),
    module_(module),
    errorRejoinSP_(NULL),
    profiler_(NULL),
    resumePC_(NULL)
{
    if (cx->runtime()->spsProfiler.enabled()) {
        
        
        
        
        profiler_ = &cx->runtime()->spsProfiler;
        profiler_->enterNative("asm.js code :0", this);
    }

    prev_ = cx_->runtime()->mainThread.asmJSActivationStack_;

    JSRuntime::AutoLockForOperationCallback lock(cx_->runtime());
    cx_->runtime()->mainThread.asmJSActivationStack_ = this;

    (void) errorRejoinSP_;  
}

AsmJSActivation::~AsmJSActivation()
{
    if (profiler_)
        profiler_->exitNative();

    JS_ASSERT(cx_->runtime()->mainThread.asmJSActivationStack_ == this);

    JSRuntime::AutoLockForOperationCallback lock(cx_->runtime());
    cx_->runtime()->mainThread.asmJSActivationStack_ = prev_;
}

static const unsigned ASM_MODULE_SLOT = 0;
static const unsigned ASM_EXPORT_INDEX_SLOT = 1;



static bool
CallAsmJS(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs callArgs = CallArgsFromVp(argc, vp);
    RootedFunction callee(cx, &callArgs.callee().as<JSFunction>());

    
    
    
    RootedObject moduleObj(cx, &callee->getExtendedSlot(ASM_MODULE_SLOT).toObject());
    AsmJSModule &module = moduleObj->as<AsmJSModuleObject>().module();

    
    
    
    unsigned exportIndex = callee->getExtendedSlot(ASM_EXPORT_INDEX_SLOT).toInt32();
    const AsmJSModule::ExportedFunction &func = module.exportedFunction(exportIndex);

    
    
    
    
    
    
    

    Vector<uint64_t, 8> coercedArgs(cx);
    if (!coercedArgs.resize(Max<size_t>(1, func.numArgs())))
        return false;

    RootedValue v(cx);
    for (unsigned i = 0; i < func.numArgs(); ++i) {
        v = i < callArgs.length() ? callArgs[i] : UndefinedValue();
        switch (func.argCoercion(i)) {
          case AsmJS_ToInt32:
            if (!ToInt32(cx, v, (int32_t*)&coercedArgs[i]))
                return false;
            break;
          case AsmJS_ToNumber:
            if (!ToNumber(cx, v, (double*)&coercedArgs[i]))
                return false;
            break;
        }
    }

    {
        
        
        
        
        AsmJSActivation activation(cx, module);

        
        
        
        jit::IonContext ictx(cx, NULL);
        JitActivation jitActivation(cx,  false,  false);

        
        if (!module.entryTrampoline(func)(coercedArgs.begin(), module.globalData()))
            return false;
    }

    switch (func.returnType()) {
      case AsmJSModule::Return_Void:
        callArgs.rval().set(UndefinedValue());
        break;
      case AsmJSModule::Return_Int32:
        callArgs.rval().set(Int32Value(*(int32_t*)&coercedArgs[0]));
        break;
      case AsmJSModule::Return_Double:
        callArgs.rval().set(NumberValue(*(double*)&coercedArgs[0]));
        break;
    }

    return true;
}

static JSFunction *
NewExportedFunction(JSContext *cx, const AsmJSModule::ExportedFunction &func,
                    HandleObject moduleObj, unsigned exportIndex)
{
    RootedPropertyName name(cx, func.name());
    JSFunction *fun = NewFunction(cx, NullPtr(), CallAsmJS, func.numArgs(),
                                  JSFunction::NATIVE_FUN, cx->global(), name,
                                  JSFunction::ExtendedFinalizeKind);
    if (!fun)
        return NULL;

    fun->setExtendedSlot(ASM_MODULE_SLOT, ObjectValue(*moduleObj));
    fun->setExtendedSlot(ASM_EXPORT_INDEX_SLOT, Int32Value(exportIndex));
    return fun;
}

static bool
HandleDynamicLinkFailure(JSContext *cx, CallArgs args, AsmJSModule &module, HandlePropertyName name)
{
    if (cx->isExceptionPending())
        return false;

    const AsmJSModuleSourceDesc &desc= module.sourceDesc();
    uint32_t length = desc.bufEnd() - desc.bufStart();
    Rooted<JSStableString*> src(cx, desc.scriptSource()->substring(cx, desc.bufStart(), desc.bufEnd()));
    if (!src)
        return false;

    RootedFunction fun(cx, NewFunction(cx, NullPtr(), NULL, 0, JSFunction::INTERPRETED,
                                       cx->global(), name, JSFunction::FinalizeKind,
                                       TenuredObject));
    if (!fun)
        return false;

    AutoNameVector formals(cx);
    formals.reserve(3);
    if (module.globalArgumentName())
        formals.infallibleAppend(module.globalArgumentName());
    if (module.importArgumentName())
        formals.infallibleAppend(module.importArgumentName());
    if (module.bufferArgumentName())
        formals.infallibleAppend(module.bufferArgumentName());

    CompileOptions options(cx);
    options.setPrincipals(cx->compartment()->principals)
           .setOriginPrincipals(desc.scriptSource()->originPrincipals())
           .setCompileAndGo(false)
           .setNoScriptRval(false);

    if (!frontend::CompileFunctionBody(cx, &fun, options, formals, src->chars().get(), length))
        return false;

    

    unsigned argc = args.length();

    InvokeArgs args2(cx);
    if (!args2.init(argc))
        return false;

    args2.setCallee(ObjectValue(*fun));
    args2.setThis(args.thisv());
    for (unsigned i = 0; i < argc; i++)
        args2[i].set(args[i]);

    if (!Invoke(cx, args2))
        return false;

    args.rval().set(args2.rval());

    return true;
}

#ifdef MOZ_VTUNE
static bool
SendFunctionsToVTune(JSContext *cx, AsmJSModule &module)
{
    uint8_t *base = module.functionCode();

    for (unsigned i = 0; i < module.numProfiledFunctions(); i++) {
        const AsmJSModule::ProfiledFunction &func = module.profiledFunction(i);

        uint8_t *start = base + func.startCodeOffset;
        uint8_t *end   = base + func.endCodeOffset;
        JS_ASSERT(end >= start);

        unsigned method_id = iJIT_GetNewMethodID();
        if (method_id == 0)
            return false;

        JSAutoByteString bytes;
        const char *method_name = AtomToPrintableString(cx, func.name, &bytes);
        if (!method_name)
            return false;

        iJIT_Method_Load method;
        method.method_id = method_id;
        method.method_name = const_cast<char *>(method_name);
        method.method_load_address = (void *)start;
        method.method_size = unsigned(end - start);
        method.line_number_size = 0;
        method.line_number_table = NULL;
        method.class_id = 0;
        method.class_file_name = NULL;
        method.source_file_name = NULL;

        iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, (void *)&method);
    }

    return true;
}
#endif

#ifdef JS_ION_PERF
static bool
SendFunctionsToPerf(JSContext *cx, AsmJSModule &module)
{
    if (!PerfFuncEnabled())
        return true;

    uintptr_t base = (uintptr_t) module.functionCode();
    const char *filename = module.sourceDesc().scriptSource()->filename();

    for (unsigned i = 0; i < module.numPerfFunctions(); i++) {
        const AsmJSModule::ProfiledFunction &func = module.perfProfiledFunction(i);
        uintptr_t start = base + (unsigned long) func.startCodeOffset;
        uintptr_t end   = base + (unsigned long) func.endCodeOffset;
        JS_ASSERT(end >= start);
        size_t size = end - start;

        JSAutoByteString bytes;
        const char *name = AtomToPrintableString(cx, func.name, &bytes);
        if (!name)
            return false;

        writePerfSpewerAsmJSFunctionMap(start, size, filename, func.lineno, func.columnIndex, name);
    }

    return true;
}

static bool
SendBlocksToPerf(JSContext *cx, AsmJSModule &module)
{
    if (!PerfBlockEnabled())
        return true;

    unsigned long funcBaseAddress = (unsigned long) module.functionCode();
    const char *filename = module.sourceDesc().scriptSource()->filename();

    for (unsigned i = 0; i < module.numPerfBlocksFunctions(); i++) {
        const AsmJSModule::ProfiledBlocksFunction &func = module.perfProfiledBlocksFunction(i);

        size_t size = func.endCodeOffset - func.startCodeOffset;

        JSAutoByteString bytes;
        const char *name = AtomToPrintableString(cx, func.name, &bytes);
        if (!name)
            return false;

        writePerfSpewerAsmJSBlocksMap(funcBaseAddress, func.startCodeOffset,
                                      func.endInlineCodeOffset, size, filename, name, func.blocks);
    }

    return true;
}
#endif

static bool
SendModuleToAttachedProfiler(JSContext *cx, AsmJSModule &module)
{
#if defined(MOZ_VTUNE)
    if (IsVTuneProfilingActive() && !SendFunctionsToVTune(cx, module))
        return false;
#endif

#if defined(JS_ION_PERF)
    if (module.numExportedFunctions() > 0) {
        size_t firstEntryCode = (size_t) module.entryTrampoline(module.exportedFunction(0));
        writePerfSpewerAsmJSEntriesAndExits(firstEntryCode, (size_t) module.globalData() - firstEntryCode);
    }
    if (!SendBlocksToPerf(cx, module))
        return false;
    if (!SendFunctionsToPerf(cx, module))
        return false;
#endif

    return true;
}


static JSObject *
CreateExportObject(JSContext *cx, HandleObject moduleObj)
{
    AsmJSModule &module = moduleObj->as<AsmJSModuleObject>().module();

    if (module.numExportedFunctions() == 1) {
        const AsmJSModule::ExportedFunction &func = module.exportedFunction(0);
        if (!func.maybeFieldName())
            return NewExportedFunction(cx, func, moduleObj, 0);
    }

    gc::AllocKind allocKind = gc::GetGCObjectKind(module.numExportedFunctions());
    RootedObject obj(cx, NewBuiltinClassInstance(cx, &JSObject::class_, allocKind));
    if (!obj)
        return NULL;

    for (unsigned i = 0; i < module.numExportedFunctions(); i++) {
        const AsmJSModule::ExportedFunction &func = module.exportedFunction(i);

        RootedFunction fun(cx, NewExportedFunction(cx, func, moduleObj, i));
        if (!fun)
            return NULL;

        JS_ASSERT(func.maybeFieldName() != NULL);
        RootedId id(cx, NameToId(func.maybeFieldName()));
        RootedValue val(cx, ObjectValue(*fun));
        if (!DefineNativeProperty(cx, obj, id, val, NULL, NULL, JSPROP_ENUMERATE, 0, 0))
            return NULL;
    }

    return obj;
}


static bool
LinkAsmJS(JSContext *cx, unsigned argc, JS::Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    
    RootedFunction fun(cx, &args.callee().as<JSFunction>());
    RootedObject moduleObj(cx,  &fun->getExtendedSlot(MODULE_FUN_SLOT).toObject());
    AsmJSModule &module = moduleObj->as<AsmJSModuleObject>().module();

    
    
    
    
    if (!DynamicallyLinkModule(cx, args, module)) {
        
        
        RootedPropertyName name(cx, fun->name());
        return HandleDynamicLinkFailure(cx, args, module, name);
    }

    
    
    if (!SendModuleToAttachedProfiler(cx, module))
        return false;

    
    
    JSObject *obj = CreateExportObject(cx, moduleObj);
    if (!obj)
        return false;

    args.rval().set(ObjectValue(*obj));
    return true;
}

JSFunction *
js::NewAsmJSModuleFunction(ExclusiveContext *cx, JSFunction *origFun, HandleObject moduleObj)
{
    RootedPropertyName name(cx, origFun->name());
    JSFunction *moduleFun = NewFunction(cx, NullPtr(), LinkAsmJS, origFun->nargs,
                                        JSFunction::NATIVE_FUN, NullPtr(), name,
                                        JSFunction::ExtendedFinalizeKind, TenuredObject);
    if (!moduleFun)
        return NULL;

    moduleFun->setExtendedSlot(MODULE_FUN_SLOT, ObjectValue(*moduleObj));
    return moduleFun;
}

bool
js::IsAsmJSModuleNative(js::Native native)
{
    return native == LinkAsmJS;
}

static bool
IsMaybeWrappedNativeFunction(const Value &v, Native native)
{
    if (!v.isObject())
        return false;

    JSObject *obj = CheckedUnwrap(&v.toObject());
    if (!obj)
        return false;

    return obj->is<JSFunction>() && obj->as<JSFunction>().maybeNative() == native;
}

bool
js::IsAsmJSModule(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    bool rval = args.hasDefined(0) && IsMaybeWrappedNativeFunction(args[0], LinkAsmJS);
    args.rval().set(BooleanValue(rval));
    return true;
}

bool
js::IsAsmJSFunction(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    bool rval = args.hasDefined(0) && IsMaybeWrappedNativeFunction(args[0], CallAsmJS);
    args.rval().set(BooleanValue(rval));
    return true;
}
