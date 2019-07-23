









































#include "xpctools_private.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"

NS_IMPL_ISUPPORTS1(nsXPCToolsCompiler, nsIXPCToolsCompiler)

nsXPCToolsCompiler::nsXPCToolsCompiler()
{
}

nsXPCToolsCompiler::~nsXPCToolsCompiler()
{
}


NS_IMETHODIMP nsXPCToolsCompiler::GetBinDir(nsILocalFile * *aBinDir)
{
    *aBinDir = nsnull;
    
    nsCOMPtr<nsIFile> file;
    nsresult rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR, getter_AddRefs(file));
    if(NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsILocalFile> lfile = do_QueryInterface(file);
    NS_ADDREF(*aBinDir = lfile);
    return NS_OK;
}

JS_STATIC_DLL_CALLBACK(void) ErrorReporter(JSContext *cx, const char *message,
                          JSErrorReport *report)
{
    printf("compile error!\n");
}

static JSClass global_class = {
    "nsXPCToolsCompiler::global", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};


NS_IMETHODIMP nsXPCToolsCompiler::CompileFile(nsILocalFile *aFile, PRBool strict)
{
    
    
    
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if(NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsIXPCNativeCallContext> callContext;
    xpc->GetCurrentNativeCallContext(getter_AddRefs(callContext));
    if(!callContext)
        return NS_ERROR_FAILURE;

    
    
    nsCOMPtr<nsISupports> callee;
    callContext->GetCallee(getter_AddRefs(callee));
    if(!callee || callee.get() != (nsISupports*)this)
        return NS_ERROR_FAILURE;

    
    JSContext* cx;
    rv = callContext->GetJSContext(&cx);
    if(NS_FAILED(rv) || !cx)
        return NS_ERROR_FAILURE;

    FILE* handle;
    if(NS_FAILED(aFile->OpenANSIFileDesc("r", &handle)))
        return NS_ERROR_FAILURE;

    JSObject* glob = JS_NewObject(cx, &global_class, NULL, NULL);
    if (!glob)
        return NS_ERROR_FAILURE;
    if (!JS_InitStandardClasses(cx, glob))
        return NS_ERROR_FAILURE;

    nsCAutoString path;
    if(NS_FAILED(aFile->GetNativePath(path)))
        return NS_ERROR_FAILURE;

    uint32 oldoptions = JS_GetOptions(cx);
    JS_SetOptions(cx, JSOPTION_WERROR | (strict ? JSOPTION_STRICT : 0));
    JSErrorReporter older = JS_SetErrorReporter(cx, ErrorReporter);
    JSExceptionState *es =JS_SaveExceptionState(cx);

    if(!JS_CompileFileHandle(cx, glob, path.get(), handle))
    {
        jsval v;
        JSErrorReport* report;
        if(JS_GetPendingException(cx, &v) &&
           nsnull != (report = JS_ErrorFromException(cx, v)))
        {
            JSString* str;
            const char* msg = "Error";
            str = JS_ValueToString(cx, v);
            if(str)
                msg = JS_GetStringBytes(str);
            printf("%s [%s,%d]\n\n",
                    msg,
                    report->filename, 
                    (int)report->lineno);            
        }
        else
        {
            printf("no script and no error report!\n");
        }
        
    }    
    JS_RestoreExceptionState(cx, es);
    JS_SetErrorReporter(cx, older);
    JS_SetOptions(cx, oldoptions);
        
    return NS_OK;
}
