




#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_IO_H
#include <io.h>     
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>     
#endif

#include "base/basictypes.h"

#include "jsapi.h"
#include "jsdbgapi.h"
#include "jsprf.h"

#include "xpcpublic.h"

#include "XPCShellEnvironment.h"

#include "mozilla/XPCOM.h"

#include "nsIChannel.h"
#include "nsIClassInfo.h"
#include "nsIDirectoryService.h"
#include "nsIJSRuntimeService.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsIURI.h"
#include "nsIXPConnect.h"
#include "nsIXPCScriptable.h"

#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsJSUtils.h"
#include "nsJSPrincipals.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#include "BackstagePass.h"

#include "TestShellChild.h"
#include "TestShellParent.h"

using mozilla::ipc::XPCShellEnvironment;
using mozilla::ipc::TestShellChild;
using mozilla::ipc::TestShellParent;
using mozilla::AutoSafeJSContext;
using namespace JS;

namespace {

static const char kDefaultRuntimeScriptFilename[] = "xpcshell.js";

class XPCShellDirProvider : public nsIDirectoryServiceProvider
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER

    XPCShellDirProvider() { }
    ~XPCShellDirProvider() { }

    bool SetGREDir(const char *dir);
    void ClearGREDir() { mGREDir = nullptr; }

private:
    nsCOMPtr<nsIFile> mGREDir;
};

inline XPCShellEnvironment*
Environment(Handle<JSObject*> global)
{
    AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, global);
    Rooted<Value> v(cx);
    if (!JS_GetProperty(cx, global, "__XPCShellEnvironment", &v) ||
        !v.get().isDouble())
    {
        return nullptr;
    }
    return static_cast<XPCShellEnvironment*>(v.get().toPrivate());
}

static bool
Print(JSContext *cx,
      unsigned argc,
      JS::Value *vp)
{
    unsigned i, n;
    JSString *str;

    JS::Value *argv = JS_ARGV(cx, vp);
    for (i = n = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return false;
        JSAutoByteString bytes(cx, str);
        if (!bytes)
            return false;
        fprintf(stdout, "%s%s", i ? " " : "", bytes.ptr());
        fflush(stdout);
    }
    n++;
    if (n)
        fputc('\n', stdout);
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return true;
}

static JSBool
GetLine(char *bufp,
        FILE *file,
        const char *prompt)
{
    char line[256];
    fputs(prompt, stdout);
    fflush(stdout);
    if (!fgets(line, sizeof line, file))
        return JS_FALSE;
    strcpy(bufp, line);
    return JS_TRUE;
}

static bool
Dump(JSContext *cx,
     unsigned argc,
     JS::Value *vp)
{
    JS_SET_RVAL(cx, vp, JSVAL_VOID);

    JSString *str;
    if (!argc)
        return true;

    str = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
    if (!str)
        return false;
    JSAutoByteString bytes(cx, str);
    if (!bytes)
      return false;

    fputs(bytes.ptr(), stdout);
    fflush(stdout);
    return true;
}

static bool
Load(JSContext *cx,
     unsigned argc,
     JS::Value *vp)
{
    JS::Rooted<JS::Value> result(cx);

    JS::Rooted<JSObject*> obj(cx, JS_THIS_OBJECT(cx, vp));
    if (!obj)
        return false;

    JS::Value *argv = JS_ARGV(cx, vp);
    for (unsigned i = 0; i < argc; i++) {
        JSString *str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return false;
        argv[i] = STRING_TO_JSVAL(str);
        JSAutoByteString filename(cx, str);
        if (!filename)
            return false;
        FILE *file = fopen(filename.ptr(), "r");
        if (!file) {
            JS_ReportError(cx, "cannot open file '%s' for reading", filename.ptr());
            return false;
        }
        Rooted<JSObject*> global(cx, JS::CurrentGlobalOrNull(cx));
        JS::CompileOptions options(cx);
        options.setUTF8(true)
               .setFileAndLine(filename.ptr(), 1)
               .setPrincipals(Environment(global)->GetPrincipal());
        JS::RootedObject rootedObj(cx, obj);
        JSScript *script = JS::Compile(cx, rootedObj, options, file);
        fclose(file);
        if (!script)
            return false;

        if (!JS_ExecuteScript(cx, obj, script, result.address())) {
            return false;
        }
    }
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return true;
}

static bool
Version(JSContext *cx,
        unsigned argc,
        JS::Value *vp)
{
    JS::Value *argv = JS_ARGV(cx, vp);
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(JS_GetVersion(cx)));
    if (argc > 0 && JSVAL_IS_INT(argv[0]))
        JS_SetVersionForCompartment(js::GetContextCompartment(cx),
                                    JSVersion(JSVAL_TO_INT(argv[0])));
    return true;
}

static bool
BuildDate(JSContext *cx, unsigned argc, JS::Value *vp)
{
    fprintf(stdout, "built on %s at %s\n", __DATE__, __TIME__);
    return true;
}

static bool
Quit(JSContext *cx,
     unsigned argc,
     JS::Value *vp)
{
    Rooted<JSObject*> global(cx, JS::CurrentGlobalOrNull(cx));
    XPCShellEnvironment* env = Environment(global);
    env->SetIsQuitting();

    return false;
}

static bool
DumpXPC(JSContext *cx,
        unsigned argc,
        JS::Value *vp)
{
    int32_t depth = 2;

    if (argc > 0) {
        if (!JS_ValueToInt32(cx, JS_ARGV(cx, vp)[0], &depth))
            return false;
    }

    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());
    if(xpc)
        xpc->DebugDump(int16_t(depth));
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return true;
}

static bool
GC(JSContext *cx,
   unsigned argc,
   JS::Value *vp)
{
    JSRuntime *rt = JS_GetRuntime(cx);
    JS_GC(rt);
#ifdef JS_GCMETER
    js_DumpGCStats(rt, stdout);
#endif
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return true;
}

#ifdef JS_GC_ZEAL
static bool
GCZeal(JSContext *cx, 
       unsigned argc,
       JS::Value *vp)
{
  JS::Value* argv = JS_ARGV(cx, vp);

  uint32_t zeal;
  if (!JS_ValueToECMAUint32(cx, argv[0], &zeal))
    return false;

  JS_SetGCZeal(cx, uint8_t(zeal), JS_DEFAULT_ZEAL_FREQ);
  return true;
}
#endif

#ifdef DEBUG

static bool
DumpHeap(JSContext *cx,
         unsigned argc,
         JS::Value *vp)
{
    JSAutoByteString fileName;
    void* startThing = NULL;
    JSGCTraceKind startTraceKind = JSTRACE_OBJECT;
    void *thingToFind = NULL;
    size_t maxDepth = (size_t)-1;
    void *thingToIgnore = NULL;
    FILE *dumpFile;
    JSBool ok;

    JS::Value *argv = JS_ARGV(cx, vp);
    JS_SET_RVAL(cx, vp, JSVAL_VOID);

    vp = argv + 0;
    if (argc > 0 && *vp != JSVAL_NULL && *vp != JSVAL_VOID) {
        JSString *str;

        str = JS_ValueToString(cx, *vp);
        if (!str)
            return false;
        *vp = STRING_TO_JSVAL(str);
        if (!fileName.encodeLatin1(cx, str))
            return false;
    }

    vp = argv + 1;
    if (argc > 1 && *vp != JSVAL_NULL && *vp != JSVAL_VOID) {
        if (!JSVAL_IS_TRACEABLE(*vp))
            goto not_traceable_arg;
        startThing = JSVAL_TO_TRACEABLE(*vp);
        startTraceKind = JSVAL_TRACE_KIND(*vp);
    }

    vp = argv + 2;
    if (argc > 2 && *vp != JSVAL_NULL && *vp != JSVAL_VOID) {
        if (!JSVAL_IS_TRACEABLE(*vp))
            goto not_traceable_arg;
        thingToFind = JSVAL_TO_TRACEABLE(*vp);
    }

    vp = argv + 3;
    if (argc > 3 && *vp != JSVAL_NULL && *vp != JSVAL_VOID) {
        uint32_t depth;

        if (!JS_ValueToECMAUint32(cx, *vp, &depth))
            return false;
        maxDepth = depth;
    }

    vp = argv + 4;
    if (argc > 4 && *vp != JSVAL_NULL && *vp != JSVAL_VOID) {
        if (!JSVAL_IS_TRACEABLE(*vp))
            goto not_traceable_arg;
        thingToIgnore = JSVAL_TO_TRACEABLE(*vp);
    }

    if (!fileName) {
        dumpFile = stdout;
    } else {
        dumpFile = fopen(fileName.ptr(), "w");
        if (!dumpFile) {
            fprintf(stderr, "dumpHeap: can't open %s: %s\n",
                    fileName.ptr(), strerror(errno));
            return false;
        }
    }

    ok = JS_DumpHeap(JS_GetRuntime(cx), dumpFile, startThing, startTraceKind, thingToFind,
                     maxDepth, thingToIgnore);
    if (dumpFile != stdout)
        fclose(dumpFile);
    if (!ok)
        JS_ReportOutOfMemory(cx);
    return ok;

  not_traceable_arg:
    fprintf(stderr,
            "dumpHeap: argument %u is not null or a heap-allocated thing\n",
            (unsigned)(vp - argv));
    return false;
}

#endif 

const JSFunctionSpec gGlobalFunctions[] =
{
    JS_FS("print",           Print,          0,0),
    JS_FS("load",            Load,           1,0),
    JS_FS("quit",            Quit,           0,0),
    JS_FS("version",         Version,        1,0),
    JS_FS("build",           BuildDate,      0,0),
    JS_FS("dumpXPC",         DumpXPC,        1,0),
    JS_FS("dump",            Dump,           1,0),
    JS_FS("gc",              GC,             0,0),
 #ifdef JS_GC_ZEAL
    JS_FS("gczeal",          GCZeal,         1,0),
 #endif
 #ifdef DEBUG
    JS_FS("dumpHeap",        DumpHeap,       5,0),
 #endif
    JS_FS_END
};

typedef enum JSShellErrNum
{
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "jsshell.msg"
#undef MSG_DEF
    JSShellErr_Limit
#undef MSGDEF
} JSShellErrNum;

} 

void
XPCShellEnvironment::ProcessFile(JSContext *cx,
                                 JS::Handle<JSObject*> obj,
                                 const char *filename,
                                 FILE *file,
                                 JSBool forceTTY)
{
    XPCShellEnvironment* env = this;

    JSScript *script;
    JS::Rooted<JS::Value> result(cx);
    int lineno, startline;
    JSBool ok, hitEOF;
    char *bufp, buffer[4096];
    JSString *str;

    if (forceTTY) {
        file = stdin;
    }
    else
#ifdef HAVE_ISATTY
    if (!isatty(fileno(file)))
#endif
    {
        







        int ch = fgetc(file);
        if (ch == '#') {
            while((ch = fgetc(file)) != EOF) {
                if(ch == '\n' || ch == '\r')
                    break;
            }
        }
        ungetc(ch, file);

        JSAutoRequest ar(cx);
        JSAutoCompartment ac(cx, obj);

        JS::CompileOptions options(cx);
        options.setUTF8(true)
               .setFileAndLine(filename, 1)
               .setPrincipals(env->GetPrincipal());
        JSScript* script = JS::Compile(cx, obj, options, file);
        if (script)
            (void)JS_ExecuteScript(cx, obj, script, result.address());

        return;
    }

    
    lineno = 1;
    hitEOF = JS_FALSE;
    do {
        bufp = buffer;
        *bufp = '\0';

        JSAutoRequest ar(cx);
        JSAutoCompartment ac(cx, obj);

        





        startline = lineno;
        do {
            if (!GetLine(bufp, file, startline == lineno ? "js> " : "")) {
                hitEOF = JS_TRUE;
                break;
            }
            bufp += strlen(bufp);
            lineno++;
        } while (!JS_BufferIsCompilableUnit(cx, obj, buffer, strlen(buffer)));

        
        JS_ClearPendingException(cx);
        script =
            JS_CompileScriptForPrincipals(cx, obj, env->GetPrincipal(), buffer,
                                          strlen(buffer), "typein", startline);
        if (script) {
            JSErrorReporter older;

            ok = JS_ExecuteScript(cx, obj, script, result.address());
            if (ok && result != JSVAL_VOID) {
                
                older = JS_SetErrorReporter(cx, NULL);
                str = JS_ValueToString(cx, result);
                JSAutoByteString bytes;
                if (str)
                    bytes.encodeLatin1(cx, str);
                JS_SetErrorReporter(cx, older);

                if (!!bytes)
                    fprintf(stdout, "%s\n", bytes.ptr());
                else
                    ok = JS_FALSE;
            }
        }
    } while (!hitEOF && !env->IsQuitting());

    fprintf(stdout, "\n");
}

NS_IMETHODIMP_(nsrefcnt)
XPCShellDirProvider::AddRef()
{
    return 2;
}

NS_IMETHODIMP_(nsrefcnt)
XPCShellDirProvider::Release()
{
    return 1;
}

NS_IMPL_QUERY_INTERFACE1(XPCShellDirProvider, nsIDirectoryServiceProvider)

bool
XPCShellDirProvider::SetGREDir(const char *dir)
{
    nsresult rv = XRE_GetFileFromPath(dir, getter_AddRefs(mGREDir));
    return NS_SUCCEEDED(rv);
}

NS_IMETHODIMP
XPCShellDirProvider::GetFile(const char *prop,
                             bool *persistent,
                             nsIFile* *result)
{
    if (mGREDir && !strcmp(prop, NS_GRE_DIR)) {
        *persistent = true;
        NS_ADDREF(*result = mGREDir);
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}


XPCShellEnvironment*
XPCShellEnvironment::CreateEnvironment()
{
    XPCShellEnvironment* env = new XPCShellEnvironment();
    if (env && !env->Init()) {
        delete env;
        env = nullptr;
    }
    return env;
}

XPCShellEnvironment::XPCShellEnvironment()
:   mQuitting(JS_FALSE)
{
}

XPCShellEnvironment::~XPCShellEnvironment()
{

    AutoSafeJSContext cx;
    JSObject* global = GetGlobalObject();
    if (global) {
        {
            JSAutoCompartment ac(cx, global);
            JS_SetAllNonReservedSlotsToUndefined(cx, global);
        }
        mGlobalHolder.Release();

        JSRuntime *rt = JS_GetRuntime(cx);
        JS_GC(rt);
    }
}

bool
XPCShellEnvironment::Init()
{
    nsresult rv;

#ifdef HAVE_SETBUF
    
    
    setbuf(stdout, 0);
#endif

    nsCOMPtr<nsIJSRuntimeService> rtsvc =
        do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
    if (!rtsvc) {
        NS_ERROR("failed to get nsJSRuntimeService!");
        return false;
    }

    JSRuntime *rt;
    if (NS_FAILED(rtsvc->GetRuntime(&rt)) || !rt) {
        NS_ERROR("failed to get JSRuntime from nsJSRuntimeService!");
        return false;
    }

    if (!mGlobalHolder.Hold(rt)) {
        NS_ERROR("Can't protect global object!");
        return false;
    }

    AutoSafeJSContext cx;

    JS_SetContextPrivate(cx, this);

    nsCOMPtr<nsIXPConnect> xpc =
      do_GetService(nsIXPConnect::GetCID());
    if (!xpc) {
        NS_ERROR("failed to get nsXPConnect service!");
        return false;
    }

    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIScriptSecurityManager> securityManager =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv) && securityManager) {
        rv = securityManager->GetSystemPrincipal(getter_AddRefs(principal));
        if (NS_FAILED(rv)) {
            fprintf(stderr, "+++ Failed to obtain SystemPrincipal from ScriptSecurityManager service.\n");
        }
    } else {
        fprintf(stderr, "+++ Failed to get ScriptSecurityManager service, running without principals");
    }

    nsRefPtr<BackstagePass> backstagePass;
    rv = NS_NewBackstagePass(getter_AddRefs(backstagePass));
    if (NS_FAILED(rv)) {
        NS_ERROR("Failed to create backstage pass!");
        return false;
    }

    JS::CompartmentOptions options;
    options.setZone(JS::SystemZone)
           .setVersion(JSVERSION_LATEST);
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = xpc->InitClassesWithNewWrappedGlobal(cx,
                                              static_cast<nsIGlobalObject *>(backstagePass),
                                              principal, 0,
                                              options,
                                              getter_AddRefs(holder));
    if (NS_FAILED(rv)) {
        NS_ERROR("InitClassesWithNewWrappedGlobal failed!");
        return false;
    }

    JS::Rooted<JSObject*> globalObj(cx, holder->GetJSObject());
    if (!globalObj) {
        NS_ERROR("Failed to get global JSObject!");
        return false;
    }
    JSAutoCompartment ac(cx, globalObj);

    backstagePass->SetGlobalObject(globalObj);

    if (!JS_DefineProperty(cx, globalObj, "__XPCShellEnvironment",
                           PRIVATE_TO_JSVAL(this), JS_PropertyStub,
                           JS_StrictPropertyStub,
                           JSPROP_READONLY | JSPROP_PERMANENT) ||
        !JS_DefineFunctions(cx, globalObj, gGlobalFunctions) ||
        !JS_DefineProfilingFunctions(cx, globalObj))
    {
        NS_ERROR("JS_DefineFunctions failed!");
        return false;
    }

    mGlobalHolder = globalObj;

    FILE* runtimeScriptFile = fopen(kDefaultRuntimeScriptFilename, "r");
    if (runtimeScriptFile) {
        fprintf(stdout, "[loading '%s'...]\n", kDefaultRuntimeScriptFilename);
        ProcessFile(cx, globalObj, kDefaultRuntimeScriptFilename,
                    runtimeScriptFile, JS_FALSE);
        fclose(runtimeScriptFile);
    }

    return true;
}

bool
XPCShellEnvironment::EvaluateString(const nsString& aString,
                                    nsString* aResult)
{
  AutoSafeJSContext cx;
  JS::Rooted<JSObject*> global(cx, GetGlobalObject());
  JSAutoCompartment ac(cx, global);

  JSScript* script =
      JS_CompileUCScriptForPrincipals(cx, global, GetPrincipal(),
                                      aString.get(), aString.Length(),
                                      "typein", 0);
  if (!script) {
     return false;
  }

  if (aResult) {
      aResult->Truncate();
  }

  JS::Rooted<JS::Value> result(cx);
  JSBool ok = JS_ExecuteScript(cx, global, script, result.address());
  if (ok && result != JSVAL_VOID) {
      JSErrorReporter old = JS_SetErrorReporter(cx, NULL);
      JSString* str = JS_ValueToString(cx, result);
      nsDependentJSString depStr;
      if (str)
          depStr.init(cx, str);
      JS_SetErrorReporter(cx, old);

      if (!depStr.IsEmpty() && aResult) {
          aResult->Assign(depStr);
      }
  }

  return true;
}
