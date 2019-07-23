









































#include "xpctools_private.h"



ProfilerFile::ProfilerFile(const char* filename)
    :   mName(filename ? strdup(filename) : nsnull)
{
    mFunctionTable.Init();
}

ProfilerFile::~ProfilerFile()
{
    if(mName)
        free(mName);
}

ProfilerFunction* 
ProfilerFile::FindOrAddFunction(const char* aName,
                                uintN aBaseLineNumber,
                                uintN aLineExtent,
                                size_t aTotalSize)
{
    FunctionID key(aBaseLineNumber, aLineExtent);
    ProfilerFunction* fun;

    if (!mFunctionTable.Get(key, &fun))
    {
        fun = new ProfilerFunction(aName, aBaseLineNumber, aLineExtent,
                                   aTotalSize, this);
        if(fun)
            mFunctionTable.Put(key, fun);
    }
    return fun;
}

void
ProfilerFile::EnumerateFunctions(EnumeratorType aFunc, void *aClosure) const
{
    mFunctionTable.EnumerateRead(aFunc, aClosure);
}

ProfilerFunction::ProfilerFunction(const char* name, 
                                   uintN lineno, uintn extent, size_t totalsize,
                                   ProfilerFile* file)
    :   mName(name ? strdup(name) : nsnull),
        mBaseLineNumber(lineno),
        mLineExtent(extent),
        mTotalSize(totalsize),
        mFile(file),
        mCallCount(0),
        mCompileCount(0),
        mQuickTime((PRUint32) -1),
        mLongTime(0),
        mStartTime(0),
        mSum(0)
{
    
}

ProfilerFunction::~ProfilerFunction()
{
    if(mName)
        free(mName);
}




NS_IMPL_ISUPPORTS1(nsXPCToolsProfiler, nsIXPCToolsProfiler)

nsXPCToolsProfiler::nsXPCToolsProfiler()
    :   mLock(PR_NewLock()),
        mRuntime(nsnull)
{
    InitializeRuntime();
    mFileTable.Init();
    mScriptTable.Init();
}

nsXPCToolsProfiler::~nsXPCToolsProfiler()
{
    Stop();
    if(mLock)
        PR_DestroyLock(mLock);
}





JS_STATIC_DLL_CALLBACK(void)
xpctools_JSNewScriptHook(JSContext  *cx,
                         const char *filename,  
                         uintN      lineno,     
                         JSScript   *script,
                         JSFunction *fun,
                         void       *callerdata)
{
    if(!script)
        return;
    if(!filename)
        filename = "<<<!!! has no name so may represent many different pages !!!>>>";
    nsXPCToolsProfiler* self = (nsXPCToolsProfiler*) callerdata;    

    PR_Lock(self->mLock);

    ProfilerFile* file;
    if (!self->mFileTable.Get(filename, &file))
    {
        file = new ProfilerFile(filename);
        if (file)
            self->mFileTable.Put(filename, file);
    }
    if(file)
    {
        ProfilerFunction* function = 
            file->FindOrAddFunction(fun ? JS_GetFunctionName(fun) : nsnull, 
                                    JS_GetScriptBaseLineNumber(cx, script),
                                    JS_GetScriptLineExtent(cx, script),
                                    fun
                                    ? JS_GetFunctionTotalSize(cx, fun)
                                    : JS_GetScriptTotalSize(cx, script));
        if(function)
        {
            function->IncrementCompileCount();
            self->mScriptTable.Put(script, function);
        }
    }
    PR_Unlock(self->mLock);
}


JS_STATIC_DLL_CALLBACK(void)
xpctools_JSDestroyScriptHook(JSContext  *cx,
                             JSScript   *script,
                             void       *callerdata)
{
    if(!script)
        return;
    nsXPCToolsProfiler* self = (nsXPCToolsProfiler*) callerdata;

    PR_Lock(self->mLock);
    self->mScriptTable.Remove(script);
    PR_Unlock(self->mLock);
}



JS_STATIC_DLL_CALLBACK(void*)
xpctools_InterpreterHook(JSContext *cx, JSStackFrame *fp, JSBool before,
                         JSBool *ok, void *closure)
{
    
    NS_ASSERTION(fp, "bad frame pointer!");

    JSScript* script = fp->script;
    if(script)
    {
        nsXPCToolsProfiler* self = (nsXPCToolsProfiler*) closure;    

        PR_Lock(self->mLock);

        ProfilerFunction* fun;
        if (self->mScriptTable.Get(script, &fun))
        {
            if(before == PR_TRUE)
            {
                fun->IncrementCallCount();
                fun->SetStartTime();
            }
            else
            {
                PRUint32 delta = fun->SetEndTime();

                JSStackFrame* down = fp->down;
                if (down) {
                    JSScript* caller = down->script;
                    if (caller) {
                        ProfilerFunction* callfun;
                        if (self->mScriptTable.Get(caller, &callfun))
                        {
                            callfun->AddNotSelfTime(delta);
                        }
                    }
                }
            }
        }

        PR_Unlock(self->mLock);
    }   
    return closure;
}





NS_IMETHODIMP nsXPCToolsProfiler::Start()
{
    PR_Lock(mLock);

    if(!VerifyRuntime())
    {
        PR_Unlock(mLock);
        return NS_ERROR_UNEXPECTED; 
    }

    JS_SetNewScriptHook(mRuntime, xpctools_JSNewScriptHook, this);
    JS_SetDestroyScriptHook(mRuntime, xpctools_JSDestroyScriptHook, this);
    JS_SetExecuteHook(mRuntime, xpctools_InterpreterHook, this);
    JS_SetCallHook(mRuntime, xpctools_InterpreterHook, this);

    PR_Unlock(mLock);
    return NS_OK;
}


NS_IMETHODIMP nsXPCToolsProfiler::Stop()
{
    PR_Lock(mLock);

    if(!VerifyRuntime())
    {
        PR_Unlock(mLock);
        return NS_ERROR_UNEXPECTED; 
    }

    JS_SetNewScriptHook(mRuntime, nsnull, nsnull);
    JS_SetDestroyScriptHook(mRuntime, nsnull, nsnull);
    JS_SetExecuteHook(mRuntime, nsnull, this);
    JS_SetCallHook(mRuntime, nsnull, this);
    
    PR_Unlock(mLock);
    return NS_OK;
}


NS_IMETHODIMP nsXPCToolsProfiler::Clear()
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}


static PLDHashOperator
xpctools_FunctionNamePrinter(const FunctionID &aKey,
                             ProfilerFunction* fun, void* closure)
{
    FILE* out = (FILE*) closure;
    const char* name = fun->GetName();
    PRUint32 average;
    PRUint32 count;

    count = fun->GetCallCount();
    if (count != 0)
        average = fun->GetSum() / count;
    if(!name)
        name = "<top level>"; 
    fprintf(out,
        "    [%lu,%lu] %s() {%d-%d} %lu ",
        (unsigned long) fun->GetCompileCount(),
        (unsigned long) fun->GetCallCount(),
        name,
        (int) fun->GetBaseLineNumber(),
        (int)(fun->GetBaseLineNumber()+fun->GetLineExtent()-1),
        (unsigned long) fun->GetTotalSize());
    if(count != 0)
        fprintf(out,
            "{min %lu, max %lu, avg %lu, sum %lu, self %lu}\n",
            (unsigned long) fun->GetQuickTime(),
            (unsigned long) fun->GetLongTime(),
            (unsigned long) average,
            (unsigned long) fun->GetSum(),
            (unsigned long) fun->GetSelf());
    else
        fprintf(out, "\n" );
    return PL_DHASH_NEXT;
}        

static PLDHashOperator
xpctools_FilenamePrinter(const char *keyname, ProfilerFile* file,
                         void *closure)
{
    FILE* out = (FILE*) closure;
    fprintf(out, "%s\n", file->GetName());
    file->EnumerateFunctions(xpctools_FunctionNamePrinter, closure);
    return PL_DHASH_NEXT;
}        


NS_IMETHODIMP nsXPCToolsProfiler::WriteResults(nsILocalFile *aFile)
{
    NS_ENSURE_ARG(aFile);

    FILE* out;
    if(NS_FAILED(aFile->OpenANSIFileDesc("w", &out)) || ! out)
        return NS_ERROR_FAILURE;

    fprintf(out, "[CompileCount, CallCount] Name {StartLine, EndLine} TotalSize {MinTime, MaxTime, Average, Total, Self}\n\n");

    PR_Lock(mLock);
    mFileTable.EnumerateRead(xpctools_FilenamePrinter, out);
    PR_Unlock(mLock);
    
    return NS_OK;
}




JSBool
nsXPCToolsProfiler::VerifyRuntime()
{
    JSRuntime* rt;
    nsCOMPtr<nsIJSRuntimeService> rts = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
    return rts && NS_SUCCEEDED(rts->GetRuntime(&rt)) && rt && rt == mRuntime;
}

JSBool 
nsXPCToolsProfiler::InitializeRuntime()
{
    NS_ASSERTION(!mRuntime, "can't init runtime twice");
    JSRuntime* rt;
    nsCOMPtr<nsIJSRuntimeService> rts = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
    if(rts && NS_SUCCEEDED(rts->GetRuntime(&rt)) && rt)
        mRuntime = rt;
    return mRuntime != nsnull;
}
