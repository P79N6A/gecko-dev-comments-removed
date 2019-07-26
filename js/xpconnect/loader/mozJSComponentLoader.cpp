





#include "mozilla/Attributes.h"

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include <cstdarg>

#include "prlog.h"
#ifdef ANDROID
#include <android/log.h>
#endif
#ifdef XP_WIN
#include <windows.h>
#endif

#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIComponentManager.h"
#include "mozilla/Module.h"
#include "nsIFile.h"
#include "mozJSComponentLoader.h"
#include "mozJSLoaderUtils.h"
#include "nsIJSRuntimeService.h"
#include "nsIXPConnect.h"
#include "nsIObserverService.h"
#include "nsIScriptSecurityManager.h"
#include "nsIFileURL.h"
#include "nsIJARURI.h"
#include "nsNetUtil.h"
#include "nsDOMBlobBuilder.h"
#include "jsprf.h"
#include "nsJSPrincipals.h"
#include "xpcprivate.h"
#include "xpcpublic.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "WrapperFactory.h"

#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"
#include "mozilla/Preferences.h"

#include "js/OldDebugAPI.h"

using namespace mozilla;
using namespace mozilla::scache;
using namespace xpc;
using namespace JS;




static const JSClass kFakeBackstagePassJSClass =
{
    "FakeBackstagePass",
    0,
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

static const char kJSRuntimeServiceContractID[] = "@mozilla.org/js/xpc/RuntimeService;1";
static const char kXPConnectServiceContractID[] = "@mozilla.org/js/xpc/XPConnect;1";
static const char kObserverServiceContractID[] = "@mozilla.org/observer-service;1";
static const char kJSCachePrefix[] = "jsloader";


#ifndef XP_OS2
#define HAVE_PR_MEMMAP
#endif





#define XPC_SERIALIZATION_BUFFER_SIZE   (64 * 1024)
#define XPC_DESERIALIZATION_BUFFER_SIZE (12 * 8192)

#ifdef PR_LOGGING

static PRLogModuleInfo *gJSCLLog;
#endif

#define LOG(args) PR_LOG(gJSCLLog, PR_LOG_DEBUG, args)


#define ERROR_SCOPE_OBJ "%s - Second argument must be an object."
#define ERROR_NOT_PRESENT "%s - EXPORTED_SYMBOLS is not present."
#define ERROR_NOT_AN_ARRAY "%s - EXPORTED_SYMBOLS is not an array."
#define ERROR_GETTING_ARRAY_LENGTH "%s - Error getting array length of EXPORTED_SYMBOLS."
#define ERROR_ARRAY_ELEMENT "%s - EXPORTED_SYMBOLS[%d] is not a string."
#define ERROR_GETTING_SYMBOL "%s - Could not get symbol '%s'."
#define ERROR_SETTING_SYMBOL "%s - Could not set symbol '%s' on target object."

static bool
Dump(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0)
        return true;

    JSString *str = JS::ToString(cx, args[0]);
    if (!str)
        return false;

    size_t length;
    const jschar *chars = JS_GetStringCharsAndLength(cx, str, &length);
    if (!chars)
        return false;

    NS_ConvertUTF16toUTF8 utf8str(reinterpret_cast<const char16_t*>(chars),
                                  length);
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "Gecko", "%s", utf8str.get());
#endif
#ifdef XP_WIN
    if (IsDebuggerPresent()) {
      OutputDebugStringW(reinterpret_cast<const wchar_t*>(chars));
    }
#endif
    fputs(utf8str.get(), stdout);
    fflush(stdout);
    return true;
}

static bool
Debug(JSContext *cx, unsigned argc, jsval *vp)
{
#ifdef DEBUG
    return Dump(cx, argc, vp);
#else
    return true;
#endif
}

static bool
File(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        XPCThrower::Throw(NS_ERROR_UNEXPECTED, cx);
        return false;
    }

    nsCOMPtr<nsISupports> native;
    nsresult rv = nsDOMMultipartFile::NewFile(getter_AddRefs(native));
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }

    nsCOMPtr<nsIJSNativeInitializer> initializer = do_QueryInterface(native);
    MOZ_ASSERT(initializer);

    rv = initializer->Initialize(nullptr, cx, nullptr, args);
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }

    nsXPConnect *xpc = nsXPConnect::XPConnect();
    JSObject *glob = CurrentGlobalOrNull(cx);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = xpc->WrapNativeToJSVal(cx, glob, native, nullptr,
                                &NS_GET_IID(nsISupports),
                                true, args.rval());
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }
    return true;
}

static bool
Blob(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    nsCOMPtr<nsISupports> native;
    nsresult rv = nsDOMMultipartFile::NewBlob(getter_AddRefs(native));
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }

    nsCOMPtr<nsIJSNativeInitializer> initializer = do_QueryInterface(native);
    MOZ_ASSERT(initializer);

    rv = initializer->Initialize(nullptr, cx, nullptr, args);
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }

    nsXPConnect *xpc = nsXPConnect::XPConnect();
    JSObject *glob = CurrentGlobalOrNull(cx);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = xpc->WrapNativeToJSVal(cx, glob, native, nullptr,
                                &NS_GET_IID(nsISupports),
                                true, args.rval());
    if (NS_FAILED(rv)) {
        XPCThrower::Throw(rv, cx);
        return false;
    }
    return true;
}

static const JSFunctionSpec gGlobalFun[] = {
    JS_FS("dump",    Dump,   1,0),
    JS_FS("debug",   Debug,  1,0),
    JS_FS("atob",    Atob,   1,0),
    JS_FS("btoa",    Btoa,   1,0),
    JS_FS("File",    File,   1,JSFUN_CONSTRUCTOR),
    JS_FS("Blob",    Blob,   2,JSFUN_CONSTRUCTOR),
    JS_FS_END
};

class MOZ_STACK_CLASS JSCLContextHelper
{
public:
    JSCLContextHelper(JSContext* aCx);
    ~JSCLContextHelper();

    void reportErrorAfterPop(char *buf);

    operator JSContext*() const {return mContext;}

private:

    JSContext* mContext;
    nsCxPusher mPusher;
    char*      mBuf;

    
    JSCLContextHelper(const JSCLContextHelper &) MOZ_DELETE;
    const JSCLContextHelper& operator=(const JSCLContextHelper &) MOZ_DELETE;
};


class JSCLAutoErrorReporterSetter
{
public:
    JSCLAutoErrorReporterSetter(JSContext* cx, JSErrorReporter reporter)
        {mContext = cx; mOldReporter = JS_SetErrorReporter(cx, reporter);}
    ~JSCLAutoErrorReporterSetter()
        {JS_SetErrorReporter(mContext, mOldReporter);}
private:
    JSContext* mContext;
    JSErrorReporter mOldReporter;

    JSCLAutoErrorReporterSetter(const JSCLAutoErrorReporterSetter &) MOZ_DELETE;
    const JSCLAutoErrorReporterSetter& operator=(const JSCLAutoErrorReporterSetter &) MOZ_DELETE;
};

static nsresult
ReportOnCaller(JSContext *callerContext,
               const char *format, ...) {
    if (!callerContext) {
        return NS_ERROR_FAILURE;
    }

    va_list ap;
    va_start(ap, format);

    char *buf = JS_vsmprintf(format, ap);
    if (!buf) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    JS_ReportError(callerContext, buf);
    JS_smprintf_free(buf);

    return NS_OK;
}

static nsresult
ReportOnCaller(JSCLContextHelper &helper,
               const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    char *buf = JS_vsmprintf(format, ap);
    if (!buf) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    helper.reportErrorAfterPop(buf);

    return NS_OK;
}

mozJSComponentLoader::mozJSComponentLoader()
    : mRuntime(nullptr),
      mContext(nullptr),
      mModules(32),
      mImports(32),
      mInProgressImports(32),
      mThisObjects(32),
      mInitialized(false),
      mReuseLoaderGlobal(false)
{
    MOZ_ASSERT(!sSelf, "mozJSComponentLoader should be a singleton");

#ifdef PR_LOGGING
    if (!gJSCLLog) {
        gJSCLLog = PR_NewLogModule("JSComponentLoader");
    }
#endif

    sSelf = this;
}

mozJSComponentLoader::~mozJSComponentLoader()
{
    if (mInitialized) {
        NS_ERROR("'xpcom-shutdown-loaders' was not fired before cleaning up mozJSComponentLoader");
        UnloadModules();
    }

    sSelf = nullptr;
}

mozJSComponentLoader*
mozJSComponentLoader::sSelf;

NS_IMPL_ISUPPORTS3(mozJSComponentLoader,
                   mozilla::ModuleLoader,
                   xpcIJSModuleLoader,
                   nsIObserver)

nsresult
mozJSComponentLoader::ReallyInit()
{
    nsresult rv;

    mReuseLoaderGlobal = Preferences::GetBool("jsloader.reuseGlobal");

    
    
#ifdef MOZ_B2G
    mReuseLoaderGlobal = true;
#endif

    





    mRuntimeService = do_GetService(kJSRuntimeServiceContractID, &rv);
    if (NS_FAILED(rv) ||
        NS_FAILED(rv = mRuntimeService->GetRuntime(&mRuntime)))
        return rv;

    
    mContext = JS_NewContext(mRuntime, 256);
    if (!mContext)
        return NS_ERROR_OUT_OF_MEMORY;

    nsCOMPtr<nsIScriptSecurityManager> secman =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
    if (!secman)
        return NS_ERROR_FAILURE;

    rv = secman->GetSystemPrincipal(getter_AddRefs(mSystemPrincipal));
    if (NS_FAILED(rv) || !mSystemPrincipal)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIObserverService> obsSvc =
        do_GetService(kObserverServiceContractID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(this, "xpcom-shutdown-loaders", false);
    NS_ENSURE_SUCCESS(rv, rv);

    mInitialized = true;

    return NS_OK;
}

const mozilla::Module*
mozJSComponentLoader::LoadModule(FileLocation &aFile)
{
    nsCOMPtr<nsIFile> file = aFile.GetBaseFile();

    nsCString spec;
    aFile.GetURIString(spec);

    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri), spec);
    if (NS_FAILED(rv))
        return nullptr;

    if (!mInitialized) {
        rv = ReallyInit();
        if (NS_FAILED(rv))
            return nullptr;
    }

    ModuleEntry* mod;
    if (mModules.Get(spec, &mod))
    return mod;

    nsAutoPtr<ModuleEntry> entry(new ModuleEntry);

    JSAutoRequest ar(mContext);
    RootedValue dummy(mContext);
    rv = ObjectForLocation(file, uri, &entry->obj, &entry->thisObjectKey,
                           &entry->location, false, &dummy);
    if (NS_FAILED(rv)) {
        return nullptr;
    }

    nsCOMPtr<nsIXPConnect> xpc = do_GetService(kXPConnectServiceContractID,
                                               &rv);
    if (NS_FAILED(rv))
        return nullptr;

    nsCOMPtr<nsIComponentManager> cm;
    rv = NS_GetComponentManager(getter_AddRefs(cm));
    if (NS_FAILED(rv))
        return nullptr;

    JSCLContextHelper cx(mContext);
    JSAutoCompartment ac(cx, entry->obj);

    nsCOMPtr<nsIXPConnectJSObjectHolder> cm_holder;
    rv = xpc->WrapNative(cx, entry->obj, cm,
                         NS_GET_IID(nsIComponentManager),
                         getter_AddRefs(cm_holder));

    if (NS_FAILED(rv)) {
        return nullptr;
    }

    JSObject* cm_jsobj = cm_holder->GetJSObject();
    if (!cm_jsobj) {
        return nullptr;
    }

    nsCOMPtr<nsIXPConnectJSObjectHolder> file_holder;
    rv = xpc->WrapNative(cx, entry->obj, file,
                         NS_GET_IID(nsIFile),
                         getter_AddRefs(file_holder));

    if (NS_FAILED(rv)) {
        return nullptr;
    }

    JSObject* file_jsobj = file_holder->GetJSObject();
    if (!file_jsobj) {
        return nullptr;
    }

    JSCLAutoErrorReporterSetter aers(cx, xpc::SystemErrorReporter);

    RootedValue NSGetFactory_val(cx);
    if (!JS_GetProperty(cx, entry->obj, "NSGetFactory", &NSGetFactory_val) ||
        JSVAL_IS_VOID(NSGetFactory_val)) {
        return nullptr;
    }

    if (JS_TypeOfValue(cx, NSGetFactory_val) != JSTYPE_FUNCTION) {
        nsAutoCString spec;
        uri->GetSpec(spec);
        JS_ReportError(cx, "%s has NSGetFactory property that is not a function",
                       spec.get());
        return nullptr;
    }

    RootedObject jsGetFactoryObj(cx);
    if (!JS_ValueToObject(cx, NSGetFactory_val, &jsGetFactoryObj) ||
        !jsGetFactoryObj) {
        
        return nullptr;
    }

    rv = xpc->WrapJS(cx, jsGetFactoryObj,
                     NS_GET_IID(xpcIJSGetFactory), getter_AddRefs(entry->getfactoryobj));
    if (NS_FAILED(rv)) {
        
#ifdef DEBUG
        fprintf(stderr, "mJCL: couldn't get nsIModule from jsval\n");
#endif
        return nullptr;
    }

    
    mModules.Put(spec, entry);

    
    
    if (!mReuseLoaderGlobal) {
        xpc::SetLocationForGlobal(entry->obj, spec);
    }

    
    return entry.forget();
}

nsresult
mozJSComponentLoader::FindTargetObject(JSContext* aCx,
                                       MutableHandleObject aTargetObject)
{
    aTargetObject.set(nullptr);

    RootedObject targetObject(aCx);
    if (mReuseLoaderGlobal) {
        JSScript* script =
            js::GetOutermostEnclosingFunctionOfScriptedCaller(aCx);
        if (script) {
            targetObject = mThisObjects.Get(script);
        }
    }

    
    
    
    if (!targetObject) {
        
        nsresult rv;
        nsCOMPtr<nsIXPConnect> xpc =
            do_GetService(kXPConnectServiceContractID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsAXPCNativeCallContext *cc = nullptr;
        rv = xpc->GetCurrentNativeCallContext(&cc);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIXPConnectWrappedNative> wn;
        rv = cc->GetCalleeWrapper(getter_AddRefs(wn));
        NS_ENSURE_SUCCESS(rv, rv);

        targetObject = wn->GetJSObject();
        if (!targetObject) {
            NS_ERROR("null calling object");
            return NS_ERROR_FAILURE;
        }

        targetObject = JS_GetGlobalForObject(aCx, targetObject);
    }

    aTargetObject.set(targetObject);
    return NS_OK;
}

void
mozJSComponentLoader::NoteSubScript(HandleScript aScript, HandleObject aThisObject)
{
  if (!mInitialized && NS_FAILED(ReallyInit())) {
      MOZ_CRASH();
  }

  mThisObjects.Put(aScript, aThisObject);
}

 size_t
mozJSComponentLoader::DataEntrySizeOfExcludingThis(const nsACString& aKey,
                                                   ModuleEntry* const& aData,
                                                   MallocSizeOf aMallocSizeOf, void*)
{
    return aKey.SizeOfExcludingThisIfUnshared(aMallocSizeOf) +
        aData->SizeOfIncludingThis(aMallocSizeOf);
}

 size_t
mozJSComponentLoader::ClassEntrySizeOfExcludingThis(const nsACString& aKey,
                                                    const nsAutoPtr<ModuleEntry>& aData,
                                                    MallocSizeOf aMallocSizeOf, void*)
{
    return aKey.SizeOfExcludingThisIfUnshared(aMallocSizeOf) +
        aData->SizeOfIncludingThis(aMallocSizeOf);
}

size_t
mozJSComponentLoader::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf)
{
    size_t amount = aMallocSizeOf(this);

    amount += mModules.SizeOfExcludingThis(DataEntrySizeOfExcludingThis, aMallocSizeOf);
    amount += mImports.SizeOfExcludingThis(ClassEntrySizeOfExcludingThis, aMallocSizeOf);
    amount += mInProgressImports.SizeOfExcludingThis(DataEntrySizeOfExcludingThis, aMallocSizeOf);
    amount += mThisObjects.SizeOfExcludingThis(nullptr, aMallocSizeOf);

    return amount;
}


#ifdef HAVE_PR_MEMMAP
class FileAutoCloser
{
 public:
    explicit FileAutoCloser(PRFileDesc *file) : mFile(file) {}
    ~FileAutoCloser() { PR_Close(mFile); }
 private:
    PRFileDesc *mFile;
};

class FileMapAutoCloser
{
 public:
    explicit FileMapAutoCloser(PRFileMap *map) : mMap(map) {}
    ~FileMapAutoCloser() { PR_CloseFileMap(mMap); }
 private:
    PRFileMap *mMap;
};
#else
class ANSIFileAutoCloser
{
 public:
    explicit ANSIFileAutoCloser(FILE *file) : mFile(file) {}
    ~ANSIFileAutoCloser() { fclose(mFile); }
 private:
    FILE *mFile;
};
#endif

JSObject*
mozJSComponentLoader::PrepareObjectForLocation(JSCLContextHelper& aCx,
                                               nsIFile *aComponentFile,
                                               nsIURI *aURI,
                                               bool aReuseLoaderGlobal,
                                               bool *aRealFile)
{
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    if (aReuseLoaderGlobal) {
        holder = mLoaderGlobal;
    }

    nsresult rv = NS_OK;
    nsCOMPtr<nsIXPConnect> xpc =
        do_GetService(kXPConnectServiceContractID, &rv);
    NS_ENSURE_SUCCESS(rv, nullptr);

    if (!mLoaderGlobal) {
        nsRefPtr<BackstagePass> backstagePass;
        rv = NS_NewBackstagePass(getter_AddRefs(backstagePass));
        NS_ENSURE_SUCCESS(rv, nullptr);

        CompartmentOptions options;
        options.setZone(SystemZone)
               .setVersion(JSVERSION_LATEST);
        rv = xpc->InitClassesWithNewWrappedGlobal(aCx,
                                                  static_cast<nsIGlobalObject *>(backstagePass),
                                                  mSystemPrincipal,
                                                  0,
                                                  options,
                                                  getter_AddRefs(holder));
        NS_ENSURE_SUCCESS(rv, nullptr);

        RootedObject global(aCx, holder->GetJSObject());
        NS_ENSURE_TRUE(global, nullptr);

        backstagePass->SetGlobalObject(global);

        JSAutoCompartment ac(aCx, global);
        if (!JS_DefineFunctions(aCx, global, gGlobalFun) ||
            !JS_DefineProfilingFunctions(aCx, global)) {
            return nullptr;
        }

        if (aReuseLoaderGlobal) {
            mLoaderGlobal = holder;
        }
    }

    RootedObject obj(aCx, holder->GetJSObject());
    NS_ENSURE_TRUE(obj, nullptr);

    JSAutoCompartment ac(aCx, obj);

    if (aReuseLoaderGlobal) {
        
        
        obj = JS_NewObject(aCx, &kFakeBackstagePassJSClass, NullPtr(), NullPtr());
        NS_ENSURE_TRUE(obj, nullptr);
    }

    *aRealFile = false;

    
    
    
    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
    nsCOMPtr<nsIFile> testFile;
    if (NS_SUCCEEDED(rv)) {
        fileURL->GetFile(getter_AddRefs(testFile));
    }

    if (testFile) {
        *aRealFile = true;

        nsCOMPtr<nsIXPConnectJSObjectHolder> locationHolder;
        rv = xpc->WrapNative(aCx, obj, aComponentFile,
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(locationHolder));
        NS_ENSURE_SUCCESS(rv, nullptr);

        RootedObject locationObj(aCx, locationHolder->GetJSObject());
        NS_ENSURE_TRUE(locationObj, nullptr);

        if (!JS_DefineProperty(aCx, obj, "__LOCATION__",
                               ObjectValue(*locationObj),
                               nullptr, nullptr, 0)) {
            return nullptr;
        }
    }

    nsAutoCString nativePath;
    rv = aURI->GetSpec(nativePath);
    NS_ENSURE_SUCCESS(rv, nullptr);

    
    
    JSString *exposedUri = JS_NewStringCopyN(aCx, nativePath.get(),
                                             nativePath.Length());
    if (!JS_DefineProperty(aCx, obj, "__URI__",
                           STRING_TO_JSVAL(exposedUri), nullptr, nullptr, 0)) {
        return nullptr;
    }

    return obj;
}

nsresult
mozJSComponentLoader::ObjectForLocation(nsIFile *aComponentFile,
                                        nsIURI *aURI,
                                        JSObject **aObject,
                                        JSScript **aTableScript,
                                        char **aLocation,
                                        bool aPropagateExceptions,
                                        MutableHandleValue aException)
{
    JSCLContextHelper cx(mContext);

    JS_AbortIfWrongThread(JS_GetRuntime(cx));

    JSCLAutoErrorReporterSetter aers(cx, xpc::SystemErrorReporter);

    bool realFile = false;
    RootedObject obj(cx, PrepareObjectForLocation(cx, aComponentFile, aURI,
                                                  mReuseLoaderGlobal, &realFile));
    NS_ENSURE_TRUE(obj, NS_ERROR_FAILURE);

    JSAutoCompartment ac(cx, obj);

    RootedScript script(cx);
    RootedFunction function(cx);

    nsAutoCString nativePath;
    nsresult rv = aURI->GetSpec(nativePath);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    

    bool writeToCache = false;
    StartupCache* cache = StartupCache::GetSingleton();

    nsAutoCString cachePath(kJSCachePrefix);
    rv = PathifyURI(aURI, cachePath);
    NS_ENSURE_SUCCESS(rv, rv);

    if (cache) {
        if (!mReuseLoaderGlobal) {
            rv = ReadCachedScript(cache, cachePath, cx, mSystemPrincipal, &script);
        } else {
            rv = ReadCachedFunction(cache, cachePath, cx, mSystemPrincipal,
                                    function.address());
        }

        if (NS_SUCCEEDED(rv)) {
            LOG(("Successfully loaded %s from startupcache\n", nativePath.get()));
        } else {
            
            
            
            writeToCache = true;
        }
    }

    if (!script && !function) {
        
        LOG(("Slow loading %s\n", nativePath.get()));

        
        
        
        AutoSaveContextOptions asco(cx);
        if (aPropagateExceptions)
            ContextOptionsRef(cx).setDontReportUncaught(true);

        CompileOptions options(cx);
        options.setPrincipals(nsJSPrincipals::get(mSystemPrincipal))
               .setNoScriptRval(mReuseLoaderGlobal ? false : true)
               .setVersion(JSVERSION_LATEST)
               .setFileAndLine(nativePath.get(), 1)
               .setSourcePolicy(mReuseLoaderGlobal ?
                                CompileOptions::NO_SOURCE :
                                CompileOptions::LAZY_SOURCE);

        if (realFile) {
#ifdef HAVE_PR_MEMMAP
            int64_t fileSize;
            rv = aComponentFile->GetFileSize(&fileSize);
            if (NS_FAILED(rv)) {
                return rv;
            }

            int64_t maxSize = UINT32_MAX;
            if (fileSize > maxSize) {
                NS_ERROR("file too large");
                return NS_ERROR_FAILURE;
            }

            PRFileDesc *fileHandle;
            rv = aComponentFile->OpenNSPRFileDesc(PR_RDONLY, 0, &fileHandle);
            if (NS_FAILED(rv)) {
                return NS_ERROR_FILE_NOT_FOUND;
            }

            
            FileAutoCloser fileCloser(fileHandle);

            PRFileMap *map = PR_CreateFileMap(fileHandle, fileSize,
                                              PR_PROT_READONLY);
            if (!map) {
                NS_ERROR("Failed to create file map");
                return NS_ERROR_FAILURE;
            }

            
            FileMapAutoCloser mapCloser(map);

            uint32_t fileSize32 = fileSize;

            char *buf = static_cast<char*>(PR_MemMap(map, 0, fileSize32));
            if (!buf) {
                NS_WARNING("Failed to map file");
                return NS_ERROR_FAILURE;
            }

            if (!mReuseLoaderGlobal) {
                script = Compile(cx, obj, options, buf,
                                     fileSize32);
            } else {
                function = CompileFunction(cx, obj, options,
                                               nullptr, 0, nullptr,
                                               buf, fileSize32);
            }

            PR_MemUnmap(buf, fileSize32);

#else  

            




            FILE *fileHandle;
            rv = aComponentFile->OpenANSIFileDesc("r", &fileHandle);
            if (NS_FAILED(rv)) {
                return NS_ERROR_FILE_NOT_FOUND;
            }

            
            ANSIFileAutoCloser fileCloser(fileHandle);

            int64_t len;
            rv = aComponentFile->GetFileSize(&len);
            if (NS_FAILED(rv) || len < 0) {
                NS_WARNING("Failed to get file size");
                return NS_ERROR_FAILURE;
            }

            char *buf = (char *) malloc(len * sizeof(char));
            if (!buf) {
                return NS_ERROR_FAILURE;
            }

            size_t rlen = fread(buf, 1, len, fileHandle);
            if (rlen != (uint64_t)len) {
                free(buf);
                NS_WARNING("Failed to read file");
                return NS_ERROR_FAILURE;
            }

            if (!mReuseLoaderGlobal) {
                script = Compile(cx, obj, options, buf,
                                     fileSize32);
            } else {
                function = CompileFunction(cx, obj, options,
                                           nullptr, 0, nullptr,
                                           buf, fileSize32);
            }

            free(buf);

#endif 
        } else {
            nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIChannel> scriptChannel;
            rv = ioService->NewChannelFromURI(aURI, getter_AddRefs(scriptChannel));
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIInputStream> scriptStream;
            rv = scriptChannel->Open(getter_AddRefs(scriptStream));
            NS_ENSURE_SUCCESS(rv, rv);

            uint64_t len64;
            uint32_t bytesRead;

            rv = scriptStream->Available(&len64);
            NS_ENSURE_SUCCESS(rv, rv);
            NS_ENSURE_TRUE(len64 < UINT32_MAX, NS_ERROR_FILE_TOO_BIG);
            if (!len64)
                return NS_ERROR_FAILURE;
            uint32_t len = (uint32_t)len64;

            
            nsAutoArrayPtr<char> buf(new char[len + 1]);
            if (!buf)
                return NS_ERROR_OUT_OF_MEMORY;

            
            rv = scriptStream->Read(buf, len, &bytesRead);
            if (bytesRead != len)
                return NS_BASE_STREAM_OSERROR;

            buf[len] = '\0';

            if (!mReuseLoaderGlobal) {
                script = Compile(cx, obj, options, buf, bytesRead);
            } else {
                function = CompileFunction(cx, obj, options,
                                               nullptr, 0, nullptr,
                                               buf, bytesRead);
            }
        }
        
        
        if (!script && !function && aPropagateExceptions) {
            JS_GetPendingException(cx, aException);
            JS_ClearPendingException(cx);
        }
    }

    if (!script && !function) {
        return NS_ERROR_FAILURE;
    }

    if (writeToCache) {
        
        if (script) {
            rv = WriteCachedScript(cache, cachePath, cx, mSystemPrincipal,
                                   script);
        } else {
            rv = WriteCachedFunction(cache, cachePath, cx, mSystemPrincipal,
                                     function);
        }

        
        
        if (NS_SUCCEEDED(rv)) {
            LOG(("Successfully wrote to cache\n"));
        } else {
            LOG(("Failed to write to cache\n"));
        }
    }

    
    
    *aObject = obj;

    RootedScript tableScript(cx, script);
    if (!tableScript) {
        tableScript = JS_GetFunctionScript(cx, function);
        MOZ_ASSERT(tableScript);
    }

    *aTableScript = tableScript;

    
    
    
    mThisObjects.Put(tableScript, obj);
    bool ok = false;

    {
        AutoSaveContextOptions asco(cx);
        if (aPropagateExceptions)
            ContextOptionsRef(cx).setDontReportUncaught(true);
        if (script) {
            ok = JS_ExecuteScriptVersion(cx, obj, script, nullptr, JSVERSION_LATEST);
        } else {
            RootedValue rval(cx);
            ok = JS_CallFunction(cx, obj, function, 0, nullptr, rval.address());
        }
     }

    if (!ok) {
        if (aPropagateExceptions) {
            JS_GetPendingException(cx, aException);
            JS_ClearPendingException(cx);
        }
        *aObject = nullptr;
        *aTableScript = nullptr;
        mThisObjects.Remove(tableScript);
        return NS_ERROR_FAILURE;
    }

    
    *aLocation = ToNewCString(nativePath);
    if (!*aLocation) {
        *aObject = nullptr;
        *aTableScript = nullptr;
        mThisObjects.Remove(tableScript);
        return NS_ERROR_OUT_OF_MEMORY;
    }

    JS_AddNamedObjectRoot(cx, aObject, *aLocation);
    JS_AddNamedScriptRoot(cx, aTableScript, *aLocation);
    return NS_OK;
}

 PLDHashOperator
mozJSComponentLoader::ClearModules(const nsACString& key, ModuleEntry*& entry, void* cx)
{
    entry->Clear();
    return PL_DHASH_REMOVE;
}

void
mozJSComponentLoader::UnloadModules()
{
    mInitialized = false;

    if (mLoaderGlobal) {
        MOZ_ASSERT(mReuseLoaderGlobal, "How did this happen?");

        JSAutoRequest ar(mContext);
        RootedObject global(mContext, mLoaderGlobal->GetJSObject());
        if (global) {
            JSAutoCompartment ac(mContext, global);
            JS_SetAllNonReservedSlotsToUndefined(mContext, global);
        } else {
            NS_WARNING("Going to leak!");
        }

        mLoaderGlobal = nullptr;
    }

    mInProgressImports.Clear();
    mImports.Clear();
    mThisObjects.Clear();

    mModules.Enumerate(ClearModules, nullptr);

    JS_DestroyContextNoGC(mContext);
    mContext = nullptr;

    mRuntimeService = nullptr;
}

NS_IMETHODIMP
mozJSComponentLoader::Import(const nsACString& registryLocation,
                             HandleValue targetValArg,
                             JSContext *cx,
                             uint8_t optionalArgc,
                             MutableHandleValue retval)
{
    MOZ_ASSERT(nsContentUtils::IsCallerChrome());

    RootedValue targetVal(cx, targetValArg);
    RootedObject targetObject(cx, nullptr);
    if (optionalArgc) {
        
        if (targetVal.isObject()) {
            
            
            
            
            
            
            if (WrapperFactory::IsXrayWrapper(&targetVal.toObject()) &&
                !WrapperFactory::WaiveXrayAndWrap(cx, &targetVal))
            {
                return NS_ERROR_FAILURE;
            }
            targetObject = &targetVal.toObject();
        } else if (!targetVal.isNull()) {
            
            
            return ReportOnCaller(cx, ERROR_SCOPE_OBJ,
                                  PromiseFlatCString(registryLocation).get());
        }
    } else {
        nsresult rv = FindTargetObject(cx, &targetObject);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    Maybe<JSAutoCompartment> ac;
    if (targetObject) {
        ac.construct(cx, targetObject);
    }

    RootedObject global(cx);
    nsresult rv = ImportInto(registryLocation, targetObject, cx, &global);

    if (global) {
        if (!JS_WrapObject(cx, &global)) {
            NS_ERROR("can't wrap return value");
            return NS_ERROR_FAILURE;
        }

        retval.setObject(*global);
    }
    return rv;
}



NS_IMETHODIMP
mozJSComponentLoader::ImportInto(const nsACString &aLocation,
                                 JSObject *aTargetObj,
                                 nsAXPCNativeCallContext *cc,
                                 JSObject **_retval)
{
    JSContext *callercx;
    nsresult rv = cc->GetJSContext(&callercx);
    NS_ENSURE_SUCCESS(rv, rv);

    RootedObject targetObject(callercx, aTargetObj);
    RootedObject global(callercx);
    rv = ImportInto(aLocation, targetObject, callercx, &global);
    NS_ENSURE_SUCCESS(rv, rv);
    *_retval = global;
    return NS_OK;
}

nsresult
mozJSComponentLoader::ImportInto(const nsACString &aLocation,
                                 HandleObject targetObj,
                                 JSContext *callercx,
                                 MutableHandleObject vp)
{
    vp.set(nullptr);

    nsresult rv;
    if (!mInitialized) {
        rv = ReallyInit();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIURI> resURI;
    rv = ioService->NewURI(aLocation, nullptr, nullptr, getter_AddRefs(resURI));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIChannel> scriptChannel;
    rv = ioService->NewChannelFromURI(resURI, getter_AddRefs(scriptChannel));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);

    nsCOMPtr<nsIURI> resolvedURI;
    rv = scriptChannel->GetURI(getter_AddRefs(resolvedURI));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIJARURI> jarURI;
    jarURI = do_QueryInterface(resolvedURI, &rv);
    nsCOMPtr<nsIFileURL> baseFileURL;
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIURI> baseURI;
        while (jarURI) {
            jarURI->GetJARFile(getter_AddRefs(baseURI));
            jarURI = do_QueryInterface(baseURI, &rv);
        }
        baseFileURL = do_QueryInterface(baseURI, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
    } else {
        baseFileURL = do_QueryInterface(resolvedURI, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIFile> sourceFile;
    rv = baseFileURL->GetFile(getter_AddRefs(sourceFile));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> sourceLocalFile;
    sourceLocalFile = do_QueryInterface(sourceFile, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString key;
    rv = resolvedURI->GetSpec(key);
    NS_ENSURE_SUCCESS(rv, rv);

    ModuleEntry* mod;
    nsAutoPtr<ModuleEntry> newEntry;
    if (!mImports.Get(key, &mod) && !mInProgressImports.Get(key, &mod)) {
        newEntry = new ModuleEntry;
        if (!newEntry)
            return NS_ERROR_OUT_OF_MEMORY;
        mInProgressImports.Put(key, newEntry);

        RootedValue exception(callercx);
        rv = ObjectForLocation(sourceLocalFile, resURI, &newEntry->obj,
                               &newEntry->thisObjectKey,
                               &newEntry->location, true, &exception);

        mInProgressImports.Remove(key);

        if (NS_FAILED(rv)) {
            if (!exception.isUndefined()) {
                
                
                if (!JS_WrapValue(callercx, &exception))
                    return NS_ERROR_OUT_OF_MEMORY;
                JS_SetPendingException(callercx, exception);
                return NS_OK;
            }

            
            return NS_ERROR_FILE_NOT_FOUND;
        }

        
        
        if (!mReuseLoaderGlobal) {
            xpc::SetLocationForGlobal(newEntry->obj, aLocation);
        }

        mod = newEntry;
    }

    MOZ_ASSERT(mod->obj, "Import table contains entry with no object");
    vp.set(mod->obj);

    if (targetObj) {
        JSCLContextHelper cxhelper(mContext);
        JSAutoCompartment ac(mContext, mod->obj);

        RootedValue symbols(mContext);
        if (!JS_GetProperty(mContext, mod->obj,
                            "EXPORTED_SYMBOLS", &symbols)) {
            return ReportOnCaller(cxhelper, ERROR_NOT_PRESENT,
                                  PromiseFlatCString(aLocation).get());
        }

        if (!symbols.isObject() ||
            !JS_IsArrayObject(mContext, &symbols.toObject())) {
            return ReportOnCaller(cxhelper, ERROR_NOT_AN_ARRAY,
                                  PromiseFlatCString(aLocation).get());
        }

        RootedObject symbolsObj(mContext, &symbols.toObject());

        

        uint32_t symbolCount = 0;
        if (!JS_GetArrayLength(mContext, symbolsObj, &symbolCount)) {
            return ReportOnCaller(cxhelper, ERROR_GETTING_ARRAY_LENGTH,
                                  PromiseFlatCString(aLocation).get());
        }

#ifdef DEBUG
        nsAutoCString logBuffer;
#endif

        RootedValue value(mContext);
        RootedId symbolId(mContext);
        for (uint32_t i = 0; i < symbolCount; ++i) {
            if (!JS_GetElement(mContext, symbolsObj, i, &value) ||
                !value.isString() ||
                !JS_ValueToId(mContext, value, &symbolId)) {
                return ReportOnCaller(cxhelper, ERROR_ARRAY_ELEMENT,
                                      PromiseFlatCString(aLocation).get(), i);
            }

            if (!JS_GetPropertyById(mContext, mod->obj, symbolId, &value)) {
                JSAutoByteString bytes(mContext, JSID_TO_STRING(symbolId));
                if (!bytes)
                    return NS_ERROR_FAILURE;
                return ReportOnCaller(cxhelper, ERROR_GETTING_SYMBOL,
                                      PromiseFlatCString(aLocation).get(),
                                      bytes.ptr());
            }

            JSAutoCompartment target_ac(mContext, targetObj);

            if (!JS_WrapValue(mContext, &value) ||
                !JS_SetPropertyById(mContext, targetObj, symbolId, value)) {
                JSAutoByteString bytes(mContext, JSID_TO_STRING(symbolId));
                if (!bytes)
                    return NS_ERROR_FAILURE;
                return ReportOnCaller(cxhelper, ERROR_SETTING_SYMBOL,
                                      PromiseFlatCString(aLocation).get(),
                                      bytes.ptr());
            }
#ifdef DEBUG
            if (i == 0) {
                logBuffer.AssignLiteral("Installing symbols [ ");
            }
            JSAutoByteString bytes(mContext, JSID_TO_STRING(symbolId));
            if (!!bytes)
                logBuffer.Append(bytes.ptr());
            logBuffer.AppendLiteral(" ");
            if (i == symbolCount - 1) {
                LOG(("%s] from %s\n", logBuffer.get(),
                     PromiseFlatCString(aLocation).get()));
            }
#endif
        }
    }

    
    if (newEntry) {
        mImports.Put(key, newEntry);
        newEntry.forget();
    }

    return NS_OK;
}

NS_IMETHODIMP
mozJSComponentLoader::Unload(const nsACString & aLocation)
{
    nsresult rv;

    if (!mInitialized) {
        return NS_OK;
    }

    nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIURI> resURI;
    rv = ioService->NewURI(aLocation, nullptr, nullptr, getter_AddRefs(resURI));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIChannel> scriptChannel;
    rv = ioService->NewChannelFromURI(resURI, getter_AddRefs(scriptChannel));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);

    nsCOMPtr<nsIURI> resolvedURI;
    rv = scriptChannel->GetURI(getter_AddRefs(resolvedURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString key;
    rv = resolvedURI->GetSpec(key);
    NS_ENSURE_SUCCESS(rv, rv);

    ModuleEntry* mod;
    if (mImports.Get(key, &mod)) {
        mImports.Remove(key);
    }

    return NS_OK;
}

NS_IMETHODIMP
mozJSComponentLoader::Observe(nsISupports *subject, const char *topic,
                              const char16_t *data)
{
    if (!strcmp(topic, "xpcom-shutdown-loaders")) {
        UnloadModules();
    } else {
        NS_ERROR("Unexpected observer topic.");
    }

    return NS_OK;
}

size_t
mozJSComponentLoader::ModuleEntry::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
    size_t n = aMallocSizeOf(this);
    n += aMallocSizeOf(location);

    return n;
}

 already_AddRefed<nsIFactory>
mozJSComponentLoader::ModuleEntry::GetFactory(const mozilla::Module& module,
                                              const mozilla::Module::CIDEntry& entry)
{
    const ModuleEntry& self = static_cast<const ModuleEntry&>(module);
    MOZ_ASSERT(self.getfactoryobj, "Handing out an uninitialized module?");

    nsCOMPtr<nsIFactory> f;
    nsresult rv = self.getfactoryobj->Get(*entry.cid, getter_AddRefs(f));
    if (NS_FAILED(rv))
        return nullptr;

    return f.forget();
}



JSCLContextHelper::JSCLContextHelper(JSContext* aCx)
    : mContext(aCx)
    , mBuf(nullptr)
{
    mPusher.Push(mContext);
    JS_BeginRequest(mContext);
}

JSCLContextHelper::~JSCLContextHelper()
{
    JS_EndRequest(mContext);
    mPusher.Pop();
    JSContext *restoredCx = nsContentUtils::GetCurrentJSContext();
    if (restoredCx && mBuf) {
        JS_ReportError(restoredCx, mBuf);
    }

    if (mBuf) {
        JS_smprintf_free(mBuf);
    }
}

void
JSCLContextHelper::reportErrorAfterPop(char *buf)
{
    MOZ_ASSERT(!mBuf, "Already called reportErrorAfterPop");
    mBuf = buf;
}
