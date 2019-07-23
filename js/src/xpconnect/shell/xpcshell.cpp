













































#include <stdio.h>
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIXPConnect.h"
#include "nsIXPCScriptable.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIXPCScriptable.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "jsapi.h"
#include "jsprf.h"
#include "nscore.h"
#include "nsMemory.h"
#include "nsIGenericFactory.h"
#include "nsIJSRuntimeService.h"
#include "nsCOMPtr.h"
#include "nsIXPCSecurityManager.h"

#ifndef XPCONNECT_STANDALONE
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#endif


#include <stdlib.h>
#include <errno.h>
#if defined(XP_WIN) || defined(XP_OS2)
#include <io.h>     
#elif defined(XP_UNIX) || defined(XP_BEOS)
#include <unistd.h>     
#endif

#include "nsIJSContextStack.h"



#ifdef JS_THREADSAFE
#define DoBeginRequest(cx) JS_BeginRequest((cx))
#define DoEndRequest(cx)   JS_EndRequest((cx))
#else
#define DoBeginRequest(cx) ((void)0)
#define DoEndRequest(cx)   ((void)0)
#endif



#define EXITCODE_RUNTIME_ERROR 3
#define EXITCODE_FILE_NOT_FOUND 4

FILE *gOutFile = NULL;
FILE *gErrFile = NULL;

int gExitCode = 0;
JSBool gQuitting = JS_FALSE;
static JSBool reportWarnings = JS_TRUE;
static JSBool compileOnly = JS_FALSE;

JSPrincipals *gJSPrincipals = nsnull;

JS_STATIC_DLL_CALLBACK(void)
my_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    int i, j, k, n;
    char *prefix = NULL, *tmp;
    const char *ctmp;

    if (!report) {
        fprintf(gErrFile, "%s\n", message);
        return;
    }

    
    if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings)
        return;

    if (report->filename)
        prefix = JS_smprintf("%s:", report->filename);
    if (report->lineno) {
        tmp = prefix;
        prefix = JS_smprintf("%s%u: ", tmp ? tmp : "", report->lineno);
        JS_free(cx, tmp);
    }
    if (JSREPORT_IS_WARNING(report->flags)) {
        tmp = prefix;
        prefix = JS_smprintf("%s%swarning: ",
                             tmp ? tmp : "",
                             JSREPORT_IS_STRICT(report->flags) ? "strict " : "");
        JS_free(cx, tmp);
    }

    
    while ((ctmp = strchr(message, '\n')) != 0) {
        ctmp++;
        if (prefix) fputs(prefix, gErrFile);
        fwrite(message, 1, ctmp - message, gErrFile);
        message = ctmp;
    }
    
    if (prefix)
        fputs(prefix, gErrFile);
    fputs(message, gErrFile);

    if (!report->linebuf) {
        fputc('\n', gErrFile);
        goto out;
    }

    fprintf(gErrFile, ":\n%s%s\n%s", prefix, report->linebuf, prefix);
    n = report->tokenptr - report->linebuf;
    for (i = j = 0; i < n; i++) {
        if (report->linebuf[i] == '\t') {
            for (k = (j + 8) & ~7; j < k; j++) {
                fputc('.', gErrFile);
            }
            continue;
        }
        fputc('.', gErrFile);
        j++;
    }
    fputs("^\n", gErrFile);
 out:
    if (!JSREPORT_IS_WARNING(report->flags))
        gExitCode = EXITCODE_RUNTIME_ERROR;
    JS_free(cx, prefix);
}

JS_STATIC_DLL_CALLBACK(JSBool)
Print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i, n;
    JSString *str;

    for (i = n = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        fprintf(gOutFile, "%s%s", i ? " " : "", JS_GetStringBytes(str));
    }
    n++;
    if (n)
        fputc('\n', gOutFile);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Dump(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    if (!argc)
        return JS_TRUE;
    
    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;

    char *bytes = JS_GetStringBytes(str);
    bytes = strdup(bytes);

    fputs(bytes, gOutFile);
    free(bytes);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;
    const char *filename;
    JSScript *script;
    JSBool ok;
    jsval result;
    FILE *file;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        argv[i] = STRING_TO_JSVAL(str);
        filename = JS_GetStringBytes(str);
        file = fopen(filename, "r");
        script = JS_CompileFileHandleForPrincipals(cx, obj, filename, file,
                                                   gJSPrincipals);
        if (!script)
            ok = JS_FALSE;
        else {
            ok = !compileOnly
                 ? JS_ExecuteScript(cx, obj, script, &result)
                 : JS_TRUE;
            JS_DestroyScript(cx, script);
        }
        if (!ok)
            return JS_FALSE;
    }
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Version(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (argc > 0 && JSVAL_IS_INT(argv[0]))
        *rval = INT_TO_JSVAL(JS_SetVersion(cx, JSVersion(JSVAL_TO_INT(argv[0]))));
    else
        *rval = INT_TO_JSVAL(JS_GetVersion(cx));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
BuildDate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    fprintf(gOutFile, "built on %s at %s\n", __DATE__, __TIME__);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Quit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#ifdef LIVECONNECT
    JSJ_SimpleShutdown();
#endif

    gExitCode = 0;
    JS_ConvertArguments(cx, argc, argv,"/ i", &gExitCode);

    gQuitting = JS_TRUE;

    return JS_FALSE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
DumpXPC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int32 depth = 2;

    if (argc > 0) {
        if (!JS_ValueToInt32(cx, argv[0], &depth))
            return JS_FALSE;
    }

    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());
    if(xpc)
        xpc->DebugDump((int16)depth);
    return JS_TRUE;
}


#include "jscntxt.h"

#ifdef GC_MARK_DEBUG
extern "C" JS_FRIEND_DATA(FILE *) js_DumpGCHeap;
#endif

JS_STATIC_DLL_CALLBACK(JSBool)
GC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSRuntime *rt;
    uint32 preBytes;

    rt = cx->runtime;
    preBytes = rt->gcBytes;
#ifdef GC_MARK_DEBUG
    if (argc && JSVAL_IS_STRING(argv[0])) {
        char *name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
        FILE *file = fopen(name, "w");
        if (!file) {
            fprintf(gErrFile, "gc: can't open %s: %s\n", strerror(errno));
            return JS_FALSE;
        }
        js_DumpGCHeap = file;
    } else {
        js_DumpGCHeap = stdout;
    }
#endif
    JS_GC(cx);
#ifdef GC_MARK_DEBUG
    if (js_DumpGCHeap != stdout)
        fclose(js_DumpGCHeap);
    js_DumpGCHeap = NULL;
#endif
    fprintf(gOutFile, "before %lu, after %lu, break %08lx\n",
           (unsigned long)preBytes, (unsigned long)rt->gcBytes,
#ifdef XP_UNIX
           (unsigned long)sbrk(0)
#else
           0
#endif
           );
#ifdef JS_GCMETER
    js_DumpGCStats(rt, stdout);
#endif
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (argc > 0 && !JSVAL_IS_PRIMITIVE(argv[0])) {
        JS_ClearScope(cx, JSVAL_TO_OBJECT(argv[0]));
    } else {
        JS_ReportError(cx, "'clear' requires an object");
        return JS_FALSE;
    }    
    return JS_TRUE;
}

static JSFunctionSpec glob_functions[] = {
    {"print",           Print,          0,0,0},
    {"load",            Load,           1,0,0},
    {"quit",            Quit,           0,0,0},
    {"version",         Version,        1,0,0},
    {"build",           BuildDate,      0,0,0},
    {"dumpXPC",         DumpXPC,        1,0,0},
    {"dump",            Dump,           1,0,0},
    {"gc",              GC,             0,0,0},
    {"clear",           Clear,          1,0,0},
    {nsnull,nsnull,0,0,0}
};

JSClass global_class = {
    "global", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};

static JSBool
env_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{

#if !defined XP_BEOS && !defined XP_OS2 && !defined SOLARIS
    JSString *idstr, *valstr;
    const char *name, *value;
    int rv;

    idstr = JS_ValueToString(cx, id);
    valstr = JS_ValueToString(cx, *vp);
    if (!idstr || !valstr)
        return JS_FALSE;
    name = JS_GetStringBytes(idstr);
    value = JS_GetStringBytes(valstr);
#if defined XP_WIN || defined HPUX || defined OSF1 || defined IRIX \
    || defined SCO
    {
        char *waste = JS_smprintf("%s=%s", name, value);
        if (!waste) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
        rv = putenv(waste);
#ifdef XP_WIN
        






        free(waste);
#endif
    }
#else
    rv = setenv(name, value, 1);
#endif
    if (rv < 0) {
        JS_ReportError(cx, "can't set envariable %s to %s", name, value);
        return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(valstr);
#endif 
    return JS_TRUE;
}

static JSBool
env_enumerate(JSContext *cx, JSObject *obj)
{
    static JSBool reflected;
    char **evp, *name, *value;
    JSString *valstr;
    JSBool ok;

    if (reflected)
        return JS_TRUE;

    for (evp = (char **)JS_GetPrivate(cx, obj); (name = *evp) != NULL; evp++) {
        value = strchr(name, '=');
        if (!value)
            continue;
        *value++ = '\0';
        valstr = JS_NewStringCopyZ(cx, value);
        if (!valstr) {
            ok = JS_FALSE;
        } else {
            ok = JS_DefineProperty(cx, obj, name, STRING_TO_JSVAL(valstr),
                                   NULL, NULL, JSPROP_ENUMERATE);
        }
        value[-1] = '=';
        if (!ok)
            return JS_FALSE;
    }

    reflected = JS_TRUE;
    return JS_TRUE;
}

static JSBool
env_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
            JSObject **objp)
{
    JSString *idstr, *valstr;
    const char *name, *value;

    if (flags & JSRESOLVE_ASSIGNING)
        return JS_TRUE;

    idstr = JS_ValueToString(cx, id);
    if (!idstr)
        return JS_FALSE;
    name = JS_GetStringBytes(idstr);
    value = getenv(name);
    if (value) {
        valstr = JS_NewStringCopyZ(cx, value);
        if (!valstr)
            return JS_FALSE;
        if (!JS_DefineProperty(cx, obj, name, STRING_TO_JSVAL(valstr),
                               NULL, NULL, JSPROP_ENUMERATE)) {
            return JS_FALSE;
        }
        *objp = obj;
    }
    return JS_TRUE;
}

static JSClass env_class = {
    "environment", JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  env_setProperty,
    env_enumerate, (JSResolveOp) env_resolve,
    JS_ConvertStub,   JS_FinalizeStub
};



typedef enum JSShellErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "jsshell.msg"
#undef MSG_DEF
    JSShellErr_Limit
#undef MSGDEF
} JSShellErrNum;

JSErrorFormatString jsShell_ErrorFormatString[JSErr_Limit] = {
#if JS_HAS_DFLT_MSG_STRINGS
#define MSG_DEF(name, number, count, exception, format) \
    { format, count } ,
#else
#define MSG_DEF(name, number, count, exception, format) \
    { NULL, count } ,
#endif
#include "jsshell.msg"
#undef MSG_DEF
};

JS_STATIC_DLL_CALLBACK(const JSErrorFormatString *)
my_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber)
{
    if ((errorNumber > 0) && (errorNumber < JSShellErr_Limit))
            return &jsShell_ErrorFormatString[errorNumber];
        else
            return NULL;
}

#ifdef EDITLINE
extern "C" {
extern char     *readline(const char *prompt);
extern void     add_history(char *line);
}
#endif

static JSBool
GetLine(JSContext *cx, char *bufp, FILE *file, const char *prompt) {
#ifdef EDITLINE
    



    if (file == stdin) {
        char *linep = readline(prompt);
        if (!linep)
            return JS_FALSE;
        if (*linep)
            add_history(linep);
        strcpy(bufp, linep);
        JS_free(cx, linep);
        bufp += strlen(bufp);
        *bufp++ = '\n';
        *bufp = '\0';
    } else
#endif
    {
        char line[256];
        fprintf(gOutFile, prompt);
        fflush(gOutFile);
        if (!fgets(line, sizeof line, file))
            return JS_FALSE;
        strcpy(bufp, line);
    }
    return JS_TRUE;
}

static void
ProcessFile(JSContext *cx, JSObject *obj, const char *filename, FILE *file,
            JSBool forceTTY)
{
    JSScript *script;
    jsval result;
    int lineno, startline;
    JSBool ok, hitEOF;
    char *bufp, buffer[4096];
    JSString *str;

    if (forceTTY) {
        file = stdin;
    } else if (!isatty(fileno(file))) {
        







        int ch = fgetc(file);
        if (ch == '#') {
            while((ch = fgetc(file)) != EOF) {
                if(ch == '\n' || ch == '\r')
                    break;
            }
        }
        ungetc(ch, file);
        DoBeginRequest(cx);

        script = JS_CompileFileHandleForPrincipals(cx, obj, filename, file,
                                                   gJSPrincipals);

        if (script) {
            if (!compileOnly)
                (void)JS_ExecuteScript(cx, obj, script, &result);
            JS_DestroyScript(cx, script);
        }
        DoEndRequest(cx);

        return;
    }

    
    lineno = 1;
    hitEOF = JS_FALSE;
    do {
        bufp = buffer;
        *bufp = '\0';

        





        startline = lineno;
        do {
            if (!GetLine(cx, bufp, file, startline == lineno ? "js> " : "")) {
                hitEOF = JS_TRUE;
                break;
            }
            bufp += strlen(bufp);
            lineno++;
        } while (!JS_BufferIsCompilableUnit(cx, obj, buffer, strlen(buffer)));
        
        DoBeginRequest(cx);
        
        JS_ClearPendingException(cx);
        script = JS_CompileScriptForPrincipals(cx, obj, gJSPrincipals, buffer,
                                               strlen(buffer), "typein", startline);
        if (script) {
            JSErrorReporter older;

            if (!compileOnly) {
                ok = JS_ExecuteScript(cx, obj, script, &result);
                if (ok && result != JSVAL_VOID) {
                    
                    older = JS_SetErrorReporter(cx, NULL);
                    str = JS_ValueToString(cx, result);
                    JS_SetErrorReporter(cx, older);
    
                    if (str)
                        fprintf(gOutFile, "%s\n", JS_GetStringBytes(str));
                    else
                        ok = JS_FALSE;
                }
            }
            JS_DestroyScript(cx, script);
        }
        DoEndRequest(cx);
    } while (!hitEOF && !gQuitting);

    fprintf(gOutFile, "\n");
}

static void
Process(JSContext *cx, JSObject *obj, const char *filename, JSBool forceTTY)
{
    FILE *file;

    if (forceTTY || !filename || strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL,
                                 JSSMSG_CANT_OPEN,
                                 filename, strerror(errno));
            gExitCode = EXITCODE_FILE_NOT_FOUND;
            return;
        }
    }

    ProcessFile(cx, obj, filename, file, forceTTY);
}

static int
usage(void)
{
    fprintf(gErrFile, "%s\n", JS_GetImplementationVersion());
    fprintf(gErrFile, "usage: xpcshell [-PswWxCi] [-v version] [-f scriptfile] [-e script] [scriptfile] [scriptarg...]\n");
    return 2;
}

extern JSClass global_class;

static int
ProcessArgs(JSContext *cx, JSObject *obj, char **argv, int argc)
{
    const char rcfilename[] = "xpcshell.js";
    FILE *rcfile;
    int i, j, length;
    JSObject *argsObj;
    char *filename = NULL;
    JSBool isInteractive = JS_TRUE;
    JSBool forceTTY = JS_FALSE;

    rcfile = fopen(rcfilename, "r");
    if (rcfile) {
        printf("[loading '%s'...]\n", rcfilename);
        ProcessFile(cx, obj, rcfilename, rcfile, JS_FALSE);
    }

    





    for (i = 0; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') {
            ++i;
            break;
        }
        switch (argv[i][1]) {
          case 'v':
          case 'f':
          case 'e':
            ++i;
            break;
          default:;
        }
    }

    



    argsObj = JS_NewArrayObject(cx, 0, NULL);
    if (!argsObj)
        return 1;
    if (!JS_DefineProperty(cx, obj, "arguments", OBJECT_TO_JSVAL(argsObj),
                           NULL, NULL, 0)) {
        return 1;
    }

    length = argc - i;
    for (j = 0; j < length; j++) {
        JSString *str = JS_NewStringCopyZ(cx, argv[i++]);
        if (!str)
            return 1;
        if (!JS_DefineElement(cx, argsObj, j, STRING_TO_JSVAL(str),
                              NULL, NULL, JSPROP_ENUMERATE)) {
            return 1;
        }
    }

    for (i = 0; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') {
            filename = argv[i++];
            isInteractive = JS_FALSE;
            break;
        }
        switch (argv[i][1]) {
        case 'v':
            if (++i == argc) {
                return usage();
            }
            JS_SetVersion(cx, JSVersion(atoi(argv[i])));
            break;
        case 'W':
            reportWarnings = JS_FALSE;
            break;
        case 'w':
            reportWarnings = JS_TRUE;
            break;
        case 's':
            JS_ToggleOptions(cx, JSOPTION_STRICT);
            break;
        case 'x':
            JS_ToggleOptions(cx, JSOPTION_XML);
            break;
        case 'P':
            if (JS_GET_CLASS(cx, JS_GetPrototype(cx, obj)) != &global_class) {
                JSObject *gobj;

                if (!JS_SealObject(cx, obj, JS_TRUE))
                    return JS_FALSE;
                gobj = JS_NewObject(cx, &global_class, NULL, NULL);
                if (!gobj)
                    return JS_FALSE;
                if (!JS_SetPrototype(cx, gobj, obj))
                    return JS_FALSE;
                JS_SetParent(cx, gobj, NULL);
                JS_SetGlobalObject(cx, gobj);
                obj = gobj;
            }
            break;
        case 'f':
            if (++i == argc) {
                return usage();
            }
            Process(cx, obj, argv[i], JS_FALSE);
            




            isInteractive = JS_FALSE;
            break;
        case 'i':
            isInteractive = forceTTY = JS_TRUE;
            break;
        case 'e':
        {
            jsval rval;

            if (++i == argc) {
                return usage();
            }

            JS_EvaluateScript(cx, obj, argv[i], strlen(argv[i]), 
                              "-e", 1, &rval);

            isInteractive = JS_FALSE;
            break;
        }
        case 'C':
            compileOnly = JS_TRUE;
            isInteractive = JS_FALSE;
            break;

        default:
            return usage();
        }
    }

    if (filename || isInteractive)
        Process(cx, obj, filename, forceTTY);
    return gExitCode;
}



class FullTrustSecMan : public nsIXPCSecurityManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSECURITYMANAGER
  FullTrustSecMan();
};

NS_IMPL_ISUPPORTS1(FullTrustSecMan, nsIXPCSecurityManager)

FullTrustSecMan::FullTrustSecMan()
{
}

NS_IMETHODIMP
FullTrustSecMan::CanCreateWrapper(JSContext * aJSContext, const nsIID & aIID, nsISupports *aObj, nsIClassInfo *aClassInfo, void * *aPolicy)
{
    return NS_OK;
}

NS_IMETHODIMP
FullTrustSecMan::CanCreateInstance(JSContext * aJSContext, const nsCID & aCID)
{
    return NS_OK;
}

NS_IMETHODIMP
FullTrustSecMan::CanGetService(JSContext * aJSContext, const nsCID & aCID)
{
    return NS_OK;
}


NS_IMETHODIMP 
FullTrustSecMan::CanAccess(PRUint32 aAction, nsIXPCNativeCallContext *aCallContext, JSContext * aJSContext, JSObject * aJSObject, nsISupports *aObj, nsIClassInfo *aClassInfo, jsval aName, void * *aPolicy)
{
    return NS_OK;
}





#ifdef TEST_InitClassesWithNewWrappedGlobal

#include "xpctest.h"

class TestGlobal : public nsIXPCTestNoisy, public nsIXPCScriptable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTNOISY
    NS_DECL_NSIXPCSCRIPTABLE

    TestGlobal(){}
};

NS_IMPL_ISUPPORTS2(TestGlobal, nsIXPCTestNoisy, nsIXPCScriptable)


#define XPC_MAP_CLASSNAME           TestGlobal
#define XPC_MAP_QUOTED_CLASSNAME   "TestGlobal"
#define XPC_MAP_FLAGS               nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY |\
                                    nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY |\
                                    nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY
#include "xpc_map_end.h" 

NS_IMETHODIMP TestGlobal::Squawk() {return NS_OK;}

#endif




#ifdef TEST_TranslateThis

#include "xpctest.h"

class nsXPCFunctionThisTranslator : public nsIXPCFunctionThisTranslator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCFUNCTIONTHISTRANSLATOR

  nsXPCFunctionThisTranslator();
  virtual ~nsXPCFunctionThisTranslator();
  
};


NS_IMPL_ISUPPORTS1(nsXPCFunctionThisTranslator, nsIXPCFunctionThisTranslator)

nsXPCFunctionThisTranslator::nsXPCFunctionThisTranslator()
{
  
}

nsXPCFunctionThisTranslator::~nsXPCFunctionThisTranslator()
{
  
#ifdef DEBUG_jband
    printf("destroying nsXPCFunctionThisTranslator\n");
#endif
}


NS_IMETHODIMP 
nsXPCFunctionThisTranslator::TranslateThis(nsISupports *aInitialThis, 
                                           nsIInterfaceInfo *aInterfaceInfo, 
                                           PRUint16 aMethodIndex, 
                                           PRBool *aHideFirstParamFromJS, 
                                           nsIID * *aIIDOfResult, 
                                           nsISupports **_retval)
{
    NS_IF_ADDREF(aInitialThis);
    *_retval = aInitialThis;
    *aHideFirstParamFromJS = JS_FALSE;
    *aIIDOfResult = nsnull;
    return NS_OK;
}

#endif

int
main(int argc, char **argv, char **envp)
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject *glob, *envobj;
    int result;
    nsresult rv;

    
    
    setbuf(stdout, 0);

    gErrFile = stderr;
    gOutFile = stdout;
    {
        nsCOMPtr<nsIServiceManager> servMan;
        rv = NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
        if (NS_FAILED(rv)) {
            printf("NS_InitXPCOM failed!\n");
            return 1;
        }
        {
            nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
            NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
            if (registrar)
                registrar->AutoRegister(nsnull);
        }

        nsCOMPtr<nsIJSRuntimeService> rtsvc = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
        
        if (!rtsvc) {
            printf("failed to get nsJSRuntimeService!\n");
            return 1;
        }
    
        if (NS_FAILED(rtsvc->GetRuntime(&rt)) || !rt) {
            printf("failed to get JSRuntime from nsJSRuntimeService!\n");
            return 1;
        }

        cx = JS_NewContext(rt, 8192);
        if (!cx) {
            printf("JS_NewContext failed!\n");
            return 1;
        }

        JS_SetErrorReporter(cx, my_ErrorReporter);

        nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());
        if (!xpc) {
            printf("failed to get nsXPConnect service!\n");
            return 1;
        }

        
        
        
        
        
        nsCOMPtr<nsIXPCSecurityManager> secman =
            NS_STATIC_CAST(nsIXPCSecurityManager*, new FullTrustSecMan());
        xpc->SetSecurityManagerForJSContext(cx, secman, 0);

        
        

#ifndef XPCONNECT_STANDALONE
        
        
        
        {
            nsCOMPtr<nsIPrincipal> princ;

            nsCOMPtr<nsIScriptSecurityManager> securityManager =
                do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
            if (NS_SUCCEEDED(rv) && securityManager) {
                rv = securityManager->GetSystemPrincipal(getter_AddRefs(princ));
                if (NS_FAILED(rv)) {
                    fprintf(gErrFile, "+++ Failed to obtain SystemPrincipal from ScriptSecurityManager service.\n");
                } else {
                    
                    rv = princ->GetJSPrincipals(cx, &gJSPrincipals);
                    if (NS_FAILED(rv)) {
                        fprintf(gErrFile, "+++ Failed to obtain JS principals from SystemPrincipal.\n");
                    }
                }
            } else {
                fprintf(gErrFile, "+++ Failed to get ScriptSecurityManager service, running without principals");
            }
        }
#endif

#ifdef TEST_TranslateThis
        nsCOMPtr<nsIXPCFunctionThisTranslator>
            translator(new nsXPCFunctionThisTranslator);
        xpc->SetFunctionThisTranslator(NS_GET_IID(nsITestXPCFunctionCallback), translator, nsnull);
#endif
    
        nsCOMPtr<nsIJSContextStack> cxstack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");
        if (!cxstack) {
            printf("failed to get the nsThreadJSContextStack service!\n");
            return 1;
        }

        if(NS_FAILED(cxstack->Push(cx))) {
            printf("failed to push the current JSContext on the nsThreadJSContextStack!\n");
            return 1;
        }

        nsCOMPtr<nsIXPCScriptable> backstagePass;
        nsresult rv = rtsvc->GetBackstagePass(getter_AddRefs(backstagePass));
        if (NS_FAILED(rv)) {
            fprintf(gErrFile, "+++ Failed to get backstage pass from rtsvc: %8x\n",
                    rv);
            return 1;
        }

        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        rv = xpc->InitClassesWithNewWrappedGlobal(cx, backstagePass,
                                                  NS_GET_IID(nsISupports),
                                                  nsIXPConnect::
                                                      FLAG_SYSTEM_GLOBAL_OBJECT,
                                                  getter_AddRefs(holder));
        if (NS_FAILED(rv))
            return 1;
        
        rv = holder->GetJSObject(&glob);
        if (NS_FAILED(rv)) {
            NS_ASSERTION(glob == nsnull, "bad GetJSObject?");
            return 1;
        }

        JS_BeginRequest(cx);

        if (!JS_DefineFunctions(cx, glob, glob_functions)) {
            JS_EndRequest(cx);
            return 1;
        }

        envobj = JS_DefineObject(cx, glob, "environment", &env_class, NULL, 0);
        if (!envobj || !JS_SetPrivate(cx, envobj, envp)) {
            JS_EndRequest(cx);
            return 1;
        }

        argc--;
        argv++;

        result = ProcessArgs(cx, glob, argv, argc);




#ifdef TEST_CALL_ON_WRAPPED_JS_AFTER_SHUTDOWN
        
        nsCOMPtr<nsIJSContextStack> bogus;
        xpc->WrapJS(cx, glob, NS_GET_IID(nsIJSContextStack),
                    (void**) getter_AddRefs(bogus));
#endif

        JS_ClearScope(cx, glob);
        JS_GC(cx);
        JSContext *oldcx;
        cxstack->Pop(&oldcx);
        NS_ASSERTION(oldcx == cx, "JS thread context push/pop mismatch");
        cxstack = nsnull;
        JS_GC(cx);
        JS_DestroyContext(cx);
        xpc->SyncJSContexts();
    } 
    
    rv = NS_ShutdownXPCOM( NULL );
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");

#ifdef TEST_CALL_ON_WRAPPED_JS_AFTER_SHUTDOWN
    
    JSContext* bogusCX;
    bogus->Peek(&bogusCX);
    bogus = nsnull;
#endif

    return result;
}
