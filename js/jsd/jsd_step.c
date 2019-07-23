








































#include "jsd.h"





#ifdef JSD_TRACE

static char*
_indentSpaces(int i)
{
#define MAX_INDENT 63
    static char* p = NULL;
    if(!p)
    {
        p = calloc(1, MAX_INDENT+1);
        if(!p) return "";
        memset(p, ' ', MAX_INDENT);
    }
    if(i > MAX_INDENT) return p;
    return p + MAX_INDENT-i;
}

static void
_interpreterTrace(JSDContext* jsdc, JSContext *cx, JSStackFrame *fp,
                  JSBool before)
{
    JSDScript* jsdscript = NULL;
    JSScript * script;
    static indent = 0;
    char* buf;
    const char* funName = NULL;

    script = JS_GetFrameScript(cx, fp);
    if(script)
    {
        JSD_LOCK_SCRIPTS(jsdc);
        jsdscript = jsd_FindJSDScript(jsdc, script);
        JSD_UNLOCK_SCRIPTS(jsdc);
        if(jsdscript)
            funName = JSD_GetScriptFunctionName(jsdc, jsdscript);
    }
    if(!funName)
        funName = "TOP_LEVEL";

    if(before)
    {
        buf = JS_smprintf("%sentering %s %s this: %0x\n",
                _indentSpaces(indent++),
                funName,
                JS_IsConstructorFrame(cx, fp) ? "constructing":"",
                (int)JS_GetFrameThis(cx, fp));
    }
    else
    {
        buf = JS_smprintf("%sleaving %s\n",
                _indentSpaces(--indent),
                funName);
    }
    JS_ASSERT(indent >= 0);

    if(!buf)
        return;

    printf(buf);
    free(buf);
}
#endif

JSBool
_callHook(JSDContext *jsdc, JSContext *cx, JSStackFrame *fp, JSBool before,
          uintN type, JSD_CallHookProc hook, void *hookData)
{
    JSDScript*        jsdscript;
    JSScript*         jsscript;
    JSBool            hookresult = JS_TRUE;
    
    if (!jsdc || !jsdc->inited)
        return JS_FALSE;

    if (!hook && !(jsdc->flags & JSD_COLLECT_PROFILE_DATA) &&
        jsdc->flags & JSD_DISABLE_OBJECT_TRACE)
    {
        



        return hookresult;
    }
    
    if (before && JS_IsConstructorFrame(cx, fp))
        jsd_Constructing(jsdc, cx, JS_GetFrameThis(cx, fp), fp);

    jsscript = JS_GetFrameScript(cx, fp);
    if (jsscript)
    {
        JSD_LOCK_SCRIPTS(jsdc);
        jsdscript = jsd_FindJSDScript(jsdc, jsscript);
        JSD_UNLOCK_SCRIPTS(jsdc);
    
        if (jsdscript)
        {
            if (JSD_IS_PROFILE_ENABLED(jsdc, jsdscript))
            {
                JSDProfileData *pdata;
                pdata = jsd_GetScriptProfileData (jsdc, jsdscript);
                if (pdata)
                {
                    if (before)
                    {
                        if (JSLL_IS_ZERO(pdata->lastCallStart))
                        {
                            int64 now;
                            JSDProfileData *callerpdata;
                            
                            
                            now = JS_Now();
                            

                            callerpdata = jsdc->callingFunctionPData;
                            if (callerpdata)
                            {
                                int64 ll_delta;
                                pdata->caller = callerpdata;
                                

                                if (JSLL_IS_ZERO(jsdc->lastReturnTime))
                                {
                                    JSLL_SUB(ll_delta, now, callerpdata->lastCallStart);
                                } else {
                                    JSLL_SUB(ll_delta, now, jsdc->lastReturnTime);
                                }
                                JSLL_ADD(callerpdata->runningTime, callerpdata->runningTime, ll_delta);
                            }
                            

                            jsdc->callingFunctionPData = pdata;
                            jsdc->lastReturnTime = JSLL_ZERO;
                            

                            pdata->runningTime = JSLL_ZERO;
                            pdata->lastCallStart = now;
                        } else {
                            if (++pdata->recurseDepth > pdata->maxRecurseDepth)
                                pdata->maxRecurseDepth = pdata->recurseDepth;
                        }
                        
                        hookresult = JS_TRUE;
                    } else if (!pdata->recurseDepth &&
                               !JSLL_IS_ZERO(pdata->lastCallStart)) {
                        int64 now, ll_delta;
                        jsdouble delta;
                        now = JS_Now();
                        JSLL_SUB(ll_delta, now, pdata->lastCallStart);
                        JSLL_L2D(delta, ll_delta);
                        delta /= 1000.0;
                        pdata->totalExecutionTime += delta;
                        

                        if ((0 == pdata->callCount) ||
                            delta < pdata->minExecutionTime)
                        {
                            pdata->minExecutionTime = delta;
                        }
                        if (delta > pdata->maxExecutionTime)
                            pdata->maxExecutionTime = delta;
                        
                        




                        if (!JSLL_IS_ZERO(jsdc->lastReturnTime))
                        {
                            

                            JSLL_SUB(ll_delta, now, jsdc->lastReturnTime);
                            JSLL_ADD(pdata->runningTime, pdata->runningTime, ll_delta);
                            JSLL_L2D(delta, pdata->runningTime);
                            delta /= 1000.0;
                        }
                        
                        pdata->totalOwnExecutionTime += delta;
                        
                        if ((0 == pdata->callCount) ||
                            delta < pdata->minOwnExecutionTime)
                        {
                            pdata->minOwnExecutionTime = delta;
                        }
                        if (delta > pdata->maxOwnExecutionTime)
                            pdata->maxOwnExecutionTime = delta;
                        
                        
                        jsdc->callingFunctionPData = pdata->caller;
                        
                        pdata->caller = NULL;
                        

                        jsdc->lastReturnTime = now;
                        pdata->lastCallStart = JSLL_ZERO;
                        ++pdata->callCount;
                    } else if (pdata->recurseDepth) {
                        --pdata->recurseDepth;
                        ++pdata->callCount;
                    }
                }
                if (hook)
                    jsd_CallCallHook (jsdc, cx, type, hook, hookData);
            } else {
                if (hook)
                    hookresult = 
                        jsd_CallCallHook (jsdc, cx, type, hook, hookData);
                else
                    hookresult = JS_TRUE;
            }
        }
    }

#ifdef JSD_TRACE
    _interpreterTrace(jsdc, cx, fp, before);
    return JS_TRUE;
#else
    return hookresult;
#endif

}

void * JS_DLL_CALLBACK
jsd_FunctionCallHook(JSContext *cx, JSStackFrame *fp, JSBool before,
                     JSBool *ok, void *closure)
{
    JSDContext*       jsdc;
    JSD_CallHookProc  hook;
    void*             hookData;

    jsdc = (JSDContext*) closure;
    
    
    JSD_LOCK();
    hook     = jsdc->functionHook;
    hookData = jsdc->functionHookData;
    JSD_UNLOCK();
    
    if (_callHook (jsdc, cx, fp, before,
                   (before) ? JSD_HOOK_FUNCTION_CALL : JSD_HOOK_FUNCTION_RETURN,
                   hook, hookData))
    {
        return closure;
    }
    
    return NULL;
}

void * JS_DLL_CALLBACK
jsd_TopLevelCallHook(JSContext *cx, JSStackFrame *fp, JSBool before,
                     JSBool *ok, void *closure)
{
    JSDContext*       jsdc;
    JSD_CallHookProc  hook;
    void*             hookData;

    jsdc = (JSDContext*) closure;

    
    JSD_LOCK();
    hook     = jsdc->toplevelHook;
    hookData = jsdc->toplevelHookData;
    JSD_UNLOCK();
    
    if (_callHook (jsdc, cx, fp, before,
                   (before) ? JSD_HOOK_TOPLEVEL_START : JSD_HOOK_TOPLEVEL_END,
                   hook, hookData))
    {
        return closure;
    }
    
    return NULL;
    
}
