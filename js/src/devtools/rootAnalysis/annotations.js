

"use strict";


var ignoreIndirectCalls = {
    "mallocSizeOf" : true,
    "aMallocSizeOf" : true,
    "_malloc_message" : true,
    "je_malloc_message" : true,
    "chunk_dalloc" : true,
    "chunk_alloc" : true,
    "__conv" : true,
    "__convf" : true,
    "prerrortable.c:callback_newtable" : true,
    "mozalloc_oom.cpp:void (* gAbortHandler)(size_t)" : true
};

function indirectCallCannotGC(fullCaller, fullVariable)
{
    var caller = readable(fullCaller);

    
    
    
    
    var name = readable(fullVariable);

    if (name in ignoreIndirectCalls)
        return true;

    if (name == "mapper" && caller == "ptio.c:pt_MapError")
        return true;

    if (name == "params" && caller == "PR_ExplodeTime")
        return true;

    if (name == "op" && /GetWeakmapKeyDelegate/.test(caller))
        return true;

    var CheckCallArgs = "AsmJSValidate.cpp:uint8 CheckCallArgs(FunctionCompiler*, js::frontend::ParseNode*, (uint8)(FunctionCompiler*,js::frontend::ParseNode*,Type)*, FunctionCompiler::Call*)";
    if (name == "checkArg" && caller == CheckCallArgs)
        return true;

    
    if (/CallDestroyScriptHook/.test(caller))
        return true;

    
    if (name == "op" && caller.indexOf("bool js::WeakMap<Key, Value, HashPolicy>::keyNeedsMark(JSObject*)") != -1)
    {
        return true;
    }

    return false;
}


var ignoreClasses = {
    "JS::CallbackTracer" : true,
    "JSStringFinalizer" : true,
    "SprintfState" : true,
    "SprintfStateStr" : true,
    "JSLocaleCallbacks" : true,
    "JSC::ExecutableAllocator" : true,
    "PRIOMethods": true,
    "XPCOMFunctions" : true, 
    "_MD_IOVector" : true,
    "malloc_table_t": true, 
};



var ignoreCallees = {
    "js::Class.trace" : true,
    "js::Class.finalize" : true,
    "JSRuntime.destroyPrincipals" : true,
    "icu_50::UObject.__deleting_dtor" : true, 
    "mozilla::CycleCollectedJSRuntime.DescribeCustomObjects" : true, 
    "mozilla::CycleCollectedJSRuntime.NoteCustomGCThingXPCOMChildren" : true, 
    "PLDHashTableOps.hashKey" : true,
    "z_stream_s.zfree" : true,
    "GrGLInterface.fCallback" : true,
    "std::strstreambuf._M_alloc_fun" : true,
    "std::strstreambuf._M_free_fun" : true,
};

function fieldCallCannotGC(csu, fullfield)
{
    if (csu in ignoreClasses)
        return true;
    if (fullfield in ignoreCallees)
        return true;
    return false;
}

function ignoreEdgeUse(edge, variable)
{
    
    if (edge.Kind == "Call") {
        var callee = edge.Exp[0];
        if (callee.Kind == "Var") {
            var name = callee.Variable.Name[0];
            if (/~DebugOnly/.test(name))
                return true;
            if (/~ScopedThreadSafeStringInspector/.test(name))
                return true;
        }
    }

    return false;
}

function ignoreEdgeAddressTaken(edge)
{
    
    
    
    
    if (edge.Kind == "Call") {
        var callee = edge.Exp[0];
        if (callee.Kind == "Var") {
            var name = callee.Variable.Name[0];
            if (/js::Invoke\(/.test(name))
                return true;
        }
    }

    return false;
}


var ignoreFunctions = {
    "ptio.c:pt_MapError" : true,
    "je_malloc_printf" : true,
    "PR_ExplodeTime" : true,
    "PR_ErrorInstallTable" : true,
    "PR_SetThreadPrivate" : true,
    "JSObject* js::GetWeakmapKeyDelegate(JSObject*)" : true, 
    "uint8 NS_IsMainThread()" : true,

    
    
    "void* std::_Locale_impl::~_Locale_impl(int32)" : true,

    
    "uint32 nsXPConnect::Release()" : true,

    
    "NS_LogInit": true,
    "NS_LogTerm": true,
    "NS_LogAddRef": true,
    "NS_LogRelease": true,
    "NS_LogCtor": true,
    "NS_LogDtor": true,
    "NS_LogCOMPtrAddRef": true,
    "NS_LogCOMPtrRelease": true,

    
    "NS_DebugBreak": true,

    
    
    "void js::AutoCompartment::~AutoCompartment(int32)" : true,
    "void JSAutoCompartment::~JSAutoCompartment(int32)" : true,

    
    
    
    
    "void mozilla::AutoJSContext::AutoJSContext(mozilla::detail::GuardObjectNotifier*)" : true,
    "void mozilla::AutoSafeJSContext::~AutoSafeJSContext(int32)" : true,

    
    
    "void js::AutoCompartment::AutoCompartment(js::ExclusiveContext*, JSCompartment*)": true,

    
    
    
    "nsGlobalNameStruct* nsScriptNameSpaceManager::LookupNavigatorName(nsAString_internal*)": true,
    "nsGlobalNameStruct* nsScriptNameSpaceManager::LookupName(nsAString_internal*, uint16**)": true,
};

function ignoreGCFunction(mangled)
{
    assert(mangled in readableNames);
    var fun = readableNames[mangled][0];

    if (fun in ignoreFunctions)
        return true;

    
    if (fun.indexOf("void nsCOMPtr<T>::Assert_NoQueryNeeded()") >= 0)
        return true;

    
    if (/refillFreeList/.test(fun) && /\(js::AllowGC\)0u/.test(fun))
        return true;
    return false;
}

function isRootedTypeName(name)
{
    if (name == "mozilla::ErrorResult" ||
        name == "JSErrorResult" ||
        name == "WrappableJSErrorResult" ||
        name == "js::frontend::TokenStream" ||
        name == "js::frontend::TokenStream::Position" ||
        name == "ModuleCompiler" ||
        name == "JSAddonId")
    {
        return true;
    }
    return false;
}

function stripUCSAndNamespace(name)
{
    if (name.startsWith('struct '))
        name = name.substr(7);
    if (name.startsWith('class '))
        name = name.substr(6);
    if (name.startsWith('const '))
        name = name.substr(6);
    if (name.startsWith('js::ctypes::'))
        name = name.substr(12);
    if (name.startsWith('js::'))
        name = name.substr(4);
    if (name.startsWith('JS::'))
        name = name.substr(4);
    if (name.startsWith('mozilla::dom::'))
        name = name.substr(14);
    if (name.startsWith('mozilla::'))
        name = name.substr(9);

    return name;
}

function isRootedPointerTypeName(name)
{
    name = stripUCSAndNamespace(name);

    if (name.startsWith('MaybeRooted<'))
        return /\(js::AllowGC\)1u>::RootType/.test(name);

    return name.startsWith('Rooted') || name.startsWith('PersistentRooted');
}

function isUnsafeStorage(typeName)
{
    typeName = stripUCSAndNamespace(typeName);
    return typeName.startsWith('UniquePtr<');
}

function isSuppressConstructor(name)
{
    return name.indexOf("::AutoSuppressGC") != -1
        || name.indexOf("::AutoAssertGCCallback") != -1
        || name.indexOf("::AutoEnterAnalysis") != -1
        || name.indexOf("::AutoSuppressGCAnalysis") != -1
        || name.indexOf("::AutoIgnoreRootingHazards") != -1;
}




function isOverridableField(initialCSU, csu, field)
{
    if (csu != 'nsISupports')
        return false;
    if (field == 'GetCurrentJSContext')
        return false;
    if (field == 'IsOnCurrentThread')
        return false;
    if (field == 'GetNativeContext')
        return false;
    if (field == "GetGlobalJSObject")
        return false;
    if (field == "GetIsMainThread")
        return false;
    if (initialCSU == 'nsIXPConnectJSObjectHolder' && field == 'GetJSObject')
        return false;
    if (initialCSU == 'nsIXPConnect' && field == 'GetSafeJSContext')
        return false;
    if (initialCSU == 'nsIScriptContext') {
        if (field == 'GetWindowProxy' || field == 'GetWindowProxyPreserveColor')
            return false;
    }

    return true;
}

function listGCTypes() {
    return [
        'JSObject',
        'JSString',
        'js::Shape',
        'js::BaseShape',
        'JSScript',
        'js::LazyScript',
        'js::ion::IonCode',
    ];
}

function listGCPointers() {
    return [
        'JS::Value',
        'jsid',

        
        'JS::AutoCheckCannotGC',
    ];
}

function listNonGCTypes() {
    return [
    ];
}

function listNonGCPointers() {
    return [
        
        
        
        'NPIdentifier',
        'XPCNativeMember',
    ];
}




function isGCPointer(typeName)
{
    return false;
}
