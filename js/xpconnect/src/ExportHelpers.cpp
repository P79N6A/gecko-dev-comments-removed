





#include "xpcprivate.h"
#include "WrapperFactory.h"
#include "jsfriendapi.h"
#include "jsproxy.h"
#include "jswrapper.h"
#include "js/StructuredClone.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsGlobalWindow.h"
#include "nsJSUtils.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"

using namespace mozilla;
using namespace JS;
using namespace js;

namespace xpc {

bool
IsReflector(JSObject *obj)
{
    return IS_WN_REFLECTOR(obj) || dom::IsDOMObject(obj);
}

enum StackScopedCloneTags {
    SCTAG_BASE = JS_SCTAG_USER_MIN,
    SCTAG_REFLECTOR,
    SCTAG_FUNCTION
};

class MOZ_STACK_CLASS StackScopedCloneData {
public:
    StackScopedCloneData(JSContext *aCx, StackScopedCloneOptions *aOptions)
        : mOptions(aOptions)
        , mReflectors(aCx)
        , mFunctions(aCx)
    {}

    StackScopedCloneOptions *mOptions;
    AutoObjectVector mReflectors;
    AutoObjectVector mFunctions;
};

static JSObject *
StackScopedCloneRead(JSContext *cx, JSStructuredCloneReader *reader, uint32_t tag,
                     uint32_t data, void *closure)
{
    MOZ_ASSERT(closure, "Null pointer!");
    StackScopedCloneData *cloneData = static_cast<StackScopedCloneData *>(closure);
    if (tag == SCTAG_REFLECTOR) {
        MOZ_ASSERT(!data);

        size_t idx;
        if (!JS_ReadBytes(reader, &idx, sizeof(size_t)))
            return nullptr;

        RootedObject reflector(cx, cloneData->mReflectors[idx]);
        MOZ_ASSERT(reflector, "No object pointer?");
        MOZ_ASSERT(IsReflector(reflector), "Object pointer must be a reflector!");

        if (!JS_WrapObject(cx, &reflector))
            return nullptr;

        return reflector;
    }

    if (tag == SCTAG_FUNCTION) {
      MOZ_ASSERT(data < cloneData->mFunctions.length());

      RootedValue functionValue(cx);
      RootedObject obj(cx, cloneData->mFunctions[data]);

      if (!JS_WrapObject(cx, &obj))
          return nullptr;

      FunctionForwarderOptions forwarderOptions(cx);
      if (!xpc::NewFunctionForwarder(cx, JSID_VOIDHANDLE, obj, forwarderOptions, &functionValue))
          return nullptr;

      return &functionValue.toObject();
    }

    MOZ_ASSERT_UNREACHABLE("Encountered garbage in the clone stream!");
    return nullptr;
}












bool IsBlobOrFileList(JSObject *obj)
{
    nsISupports *supports = UnwrapReflectorToISupports(obj);
    if (!supports)
        return false;
    nsCOMPtr<nsIDOMBlob> blob = do_QueryInterface(supports);
    if (blob)
        return true;
    nsCOMPtr<nsIDOMFileList> fileList = do_QueryInterface(supports);
    if (fileList)
        return true;
    return false;
}

static bool
StackScopedCloneWrite(JSContext *cx, JSStructuredCloneWriter *writer,
                      Handle<JSObject *> objArg, void *closure)
{
    MOZ_ASSERT(closure, "Null pointer!");
    StackScopedCloneData *cloneData = static_cast<StackScopedCloneData *>(closure);

    
    
    
    RootedObject obj(cx, JS_ObjectToInnerObject(cx, objArg));
    if ((cloneData->mOptions->wrapReflectors && IsReflector(obj)) ||
        IsBlobOrFileList(obj))
    {
        if (!cloneData->mReflectors.append(obj))
            return false;

        size_t idx = cloneData->mReflectors.length() - 1;
        if (!JS_WriteUint32Pair(writer, SCTAG_REFLECTOR, 0))
            return false;
        if (!JS_WriteBytes(writer, &idx, sizeof(size_t)))
            return false;
        return true;
    }

    if (JS_ObjectIsCallable(cx, obj)) {
        if (cloneData->mOptions->cloneFunctions) {
            cloneData->mFunctions.append(obj);
            return JS_WriteUint32Pair(writer, SCTAG_FUNCTION, cloneData->mFunctions.length() - 1);
        } else {
            JS_ReportError(cx, "Permission denied to pass a Function via structured clone");
            return false;
        }
    }

    JS_ReportError(cx, "Encountered unsupported value type writing stack-scoped structured clone");
    return false;
}

static const JSStructuredCloneCallbacks gStackScopedCloneCallbacks = {
    StackScopedCloneRead,
    StackScopedCloneWrite,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};












bool
StackScopedClone(JSContext *cx, StackScopedCloneOptions &options,
                 MutableHandleValue val)
{
    JSAutoStructuredCloneBuffer buffer;
    StackScopedCloneData data(cx, &options);
    {
        
        
        Maybe<JSAutoCompartment> ac;
        if (val.isObject()) {
            ac.construct(cx, &val.toObject());
        } else if (val.isString() && !JS_WrapValue(cx, val)) {
            return false;
        }

        if (!buffer.write(cx, val, &gStackScopedCloneCallbacks, &data))
            return false;
    }

    
    return buffer.read(cx, val, &gStackScopedCloneCallbacks, &data);
}





static bool
CloningFunctionForwarder(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject optionsObj(cx, &js::GetFunctionNativeReserved(&args.callee(), 1).toObject());
    FunctionForwarderOptions options(cx, optionsObj);
    if (!options.Parse())
        return false;

    
    RootedObject forwarderObj(cx, &js::GetFunctionNativeReserved(&args.callee(), 0).toObject());
    RootedObject origFunObj(cx, UncheckedUnwrap(forwarderObj));
    {
        JSAutoCompartment ac(cx, origFunObj);
        
        
        StackScopedCloneOptions cloneOptions;
        cloneOptions.wrapReflectors = true;
        for (unsigned i = 0; i < args.length(); i++) {
            RootedObject argObj(cx, args[i].isObject() ? &args[i].toObject() : nullptr);
            if (options.allowCallbacks && argObj && JS_ObjectIsCallable(cx, argObj)) {
                FunctionForwarderOptions innerOptions(cx);
                if (!JS_WrapObject(cx, &argObj))
                    return false;
                if (!xpc::NewFunctionForwarder(cx, JSID_VOIDHANDLE, argObj, innerOptions, args[i]))
                    return false;
            } else if (!StackScopedClone(cx, cloneOptions, args[i])) {
                return false;
            }
        }

        
        
        RootedValue functionVal(cx, ObjectValue(*origFunObj));

        if (!JS_CallFunctionValue(cx, JS::NullPtr(), functionVal, args, args.rval()))
            return false;
    }

    
    return JS_WrapValue(cx, args.rval());
}

static bool
NonCloningFunctionForwarder(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedValue v(cx, js::GetFunctionNativeReserved(&args.callee(), 0));
    MOZ_ASSERT(v.isObject(), "weird function");

    RootedObject obj(cx, JS_THIS_OBJECT(cx, vp));
    if (!obj) {
        return false;
    }
    return JS_CallFunctionValue(cx, obj, v, args, args.rval());
}
bool
NewFunctionForwarder(JSContext *cx, HandleId idArg, HandleObject callable,
                     FunctionForwarderOptions &options, MutableHandleValue vp)
{
    RootedId id(cx, idArg);
    if (id == JSID_VOIDHANDLE)
        id = GetRTIdByIndex(cx, XPCJSRuntime::IDX_EMPTYSTRING);

    JSFunction *fun = js::NewFunctionByIdWithReserved(cx, CloningFunctionForwarder, 0,0,
                                                      JS::CurrentGlobalOrNull(cx), id);
    if (!fun)
        return false;

    
    AssertSameCompartment(cx, callable);
    RootedObject funObj(cx, JS_GetFunctionObject(fun));
    js::SetFunctionNativeReserved(funObj, 0, ObjectValue(*callable));

    
    RootedObject optionsObj(cx, options.ToJSObject(cx));
    if (!optionsObj)
        return false;
    js::SetFunctionNativeReserved(funObj, 1, ObjectValue(*optionsObj));

    vp.setObject(*funObj);
    return true;
}

bool
NewNonCloningFunctionForwarder(JSContext *cx, HandleId id, HandleObject callable,
                               MutableHandleValue vp)
{
    JSFunction *fun = js::NewFunctionByIdWithReserved(cx, NonCloningFunctionForwarder,
                                                      0,0, JS::CurrentGlobalOrNull(cx), id);
    if (!fun)
        return false;

    JSObject *funobj = JS_GetFunctionObject(fun);
    js::SetFunctionNativeReserved(funobj, 0, ObjectValue(*callable));
    vp.setObject(*funobj);
    return true;
}

bool
ExportFunction(JSContext *cx, HandleValue vfunction, HandleValue vscope, HandleValue voptions,
               MutableHandleValue rval)
{
    bool hasOptions = !voptions.isUndefined();
    if (!vscope.isObject() || !vfunction.isObject() || (hasOptions && !voptions.isObject())) {
        JS_ReportError(cx, "Invalid argument");
        return false;
    }

    RootedObject funObj(cx, &vfunction.toObject());
    RootedObject targetScope(cx, &vscope.toObject());
    ExportFunctionOptions options(cx, hasOptions ? &voptions.toObject() : nullptr);
    if (hasOptions && !options.Parse())
        return false;

    
    
    targetScope = CheckedUnwrap(targetScope);
    if (!targetScope) {
        JS_ReportError(cx, "Permission denied to export function into scope");
        return false;
    }

    if (js::IsScriptedProxy(targetScope)) {
        JS_ReportError(cx, "Defining property on proxy object is not allowed");
        return false;
    }

    {
        
        
        JSAutoCompartment ac(cx, targetScope);

        
        funObj = UncheckedUnwrap(funObj);
        if (!JS_ObjectIsCallable(cx, funObj)) {
            JS_ReportError(cx, "First argument must be a function");
            return false;
        }

        RootedId id(cx, options.defineAs);
        if (JSID_IS_VOID(id)) {
            
            
            JSFunction *fun = JS_GetObjectFunction(funObj);
            RootedString funName(cx, JS_GetFunctionId(fun));
            if (!funName)
                funName = JS_InternString(cx, "");

            if (!JS_StringToId(cx, funName, &id))
                return false;
        }
        MOZ_ASSERT(JSID_IS_STRING(id));

        
        
        
        if (!JS_WrapObject(cx, &funObj))
            return false;

        
        
        FunctionForwarderOptions forwarderOptions(cx);
        forwarderOptions.allowCallbacks = options.allowCallbacks;
        if (!NewFunctionForwarder(cx, id, funObj, forwarderOptions, rval)) {
            JS_ReportError(cx, "Exporting function failed");
            return false;
        }

        
        
        
        if (!JSID_IS_VOID(options.defineAs)) {
            if (!JS_DefinePropertyById(cx, targetScope, id, rval, JSPROP_ENUMERATE,
                                       JS_PropertyStub, JS_StrictPropertyStub)) {
                return false;
            }
        }
    }

    
    if (!JS_WrapValue(cx, rval))
        return false;

    return true;
}

static bool
GetFilenameAndLineNumber(JSContext *cx, nsACString &filename, unsigned &lineno)
{
    JS::AutoFilename scriptFilename;
    if (JS::DescribeScriptedCaller(cx, &scriptFilename, &lineno)) {
        if (const char *cfilename = scriptFilename.get()) {
            filename.Assign(nsDependentCString(cfilename));
            return true;
        }
    }
    return false;
}

bool
EvalInWindow(JSContext *cx, const nsAString &source, HandleObject scope, MutableHandleValue rval)
{
    
    RootedObject targetScope(cx, CheckedUnwrap(scope));
    if (!targetScope) {
        JS_ReportError(cx, "Permission denied to eval in target scope");
        return false;
    }

    
    RootedObject inner(cx, CheckedUnwrap(targetScope,  false));
    nsCOMPtr<nsIGlobalObject> global;
    nsCOMPtr<nsPIDOMWindow> window;
    if (!JS_IsGlobalObject(inner) ||
        !(global = GetNativeForGlobal(inner)) ||
        !(window = do_QueryInterface(global)))
    {
        JS_ReportError(cx, "Second argument must be a window");
        return false;
    }

    nsCOMPtr<nsIScriptContext> context =
        (static_cast<nsGlobalWindow*>(window.get()))->GetScriptContext();
    if (!context) {
        JS_ReportError(cx, "Script context needed");
        return false;
    }

    nsCString filename;
    unsigned lineNo;
    if (!GetFilenameAndLineNumber(cx, filename, lineNo)) {
        
        filename.AssignLiteral("Unknown");
        lineNo = 0;
    }

    RootedObject cxGlobal(cx, JS::CurrentGlobalOrNull(cx));
    {
        
        
        JSContext *wndCx = context->GetNativeContext();
        AutoCxPusher pusher(wndCx);
        JS::CompileOptions compileOptions(wndCx);
        compileOptions.setFileAndLine(filename.get(), lineNo);

        
        
        nsJSUtils::EvaluateOptions evaluateOptions;
        evaluateOptions.setReportUncaught(false);

        nsresult rv = nsJSUtils::EvaluateString(wndCx,
                                                source,
                                                targetScope,
                                                compileOptions,
                                                evaluateOptions,
                                                rval);

        if (NS_FAILED(rv)) {
            
            
            
            MOZ_ASSERT(!JS_IsExceptionPending(wndCx),
                       "Exception should be delivered as return value.");
            if (rval.isUndefined()) {
                MOZ_ASSERT(rv == NS_ERROR_OUT_OF_MEMORY);
                return false;
            }

            
            
            RootedValue exn(wndCx, rval);
            
            rval.set(UndefinedValue());

            
            JSAutoCompartment ac(wndCx, cxGlobal);
            StackScopedCloneOptions cloneOptions;
            cloneOptions.wrapReflectors = true;
            if (StackScopedClone(wndCx, cloneOptions, &exn))
                js::SetPendingExceptionCrossContext(cx, exn);

            return false;
        }
    }

    
    StackScopedCloneOptions cloneOptions;
    cloneOptions.wrapReflectors = true;
    if (!StackScopedClone(cx, cloneOptions, rval)) {
        rval.set(UndefinedValue());
        return false;
    }

    return true;
}

bool
CreateObjectIn(JSContext *cx, HandleValue vobj, CreateObjectInOptions &options,
               MutableHandleValue rval)
{
    if (!vobj.isObject()) {
        JS_ReportError(cx, "Expected an object as the target scope");
        return false;
    }

    RootedObject scope(cx, js::CheckedUnwrap(&vobj.toObject()));
    if (!scope) {
        JS_ReportError(cx, "Permission denied to create object in the target scope");
        return false;
    }

    bool define = !JSID_IS_VOID(options.defineAs);

    if (define && js::IsScriptedProxy(scope)) {
        JS_ReportError(cx, "Defining property on proxy object is not allowed");
        return false;
    }

    RootedObject obj(cx);
    {
        JSAutoCompartment ac(cx, scope);
        obj = JS_NewObject(cx, nullptr, JS::NullPtr(), scope);
        if (!obj)
            return false;

        if (define) {
            if (!JS_DefinePropertyById(cx, scope, options.defineAs, obj, JSPROP_ENUMERATE,
                                       JS_PropertyStub, JS_StrictPropertyStub))
                return false;
        }
    }

    rval.setObject(*obj);
    if (!WrapperFactory::WaiveXrayAndWrap(cx, rval))
        return false;

    return true;
}

} 
