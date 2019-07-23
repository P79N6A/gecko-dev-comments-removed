




































#include "jsapi.h"
#include "nscore.h"
#include "nsIScriptContext.h"

#include "nsString.h"
#include "nsInstall.h"
#include "nsWinProfile.h"
#include "nsJSWinProfile.h"

extern void ConvertJSValToStr(nsString&  aString,
                             JSContext* aContext,
                             jsval      aValue);

extern void ConvertStrToJSVal(const nsString& aProp,
                             JSContext* aContext,
                             jsval* aReturn);

extern PRBool ConvertJSValToBool(PRBool* aProp,
                                JSContext* aContext,
                                jsval aValue);


static void PR_CALLBACK
WinProfileCleanup(JSContext *cx, JSObject *obj);





JSClass WinProfileClass = {
  "WinProfile",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub,
  WinProfileCleanup
};


static void PR_CALLBACK WinProfileCleanup(JSContext *cx, JSObject *obj)
{
    nsWinProfile *nativeThis = (nsWinProfile*)JS_GetPrivate(cx, obj);
    delete nativeThis;
}







PR_STATIC_CALLBACK(JSBool)
WinProfileGetString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinProfile *nativeThis =
    (nsWinProfile*)JS_GetInstancePrivate(cx, obj, &WinProfileClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  nsString     nativeRet;
  nsAutoString b0;
  nsAutoString b1;

  *rval = JSVAL_NULL;

  if(argc >= 2)                             
  {
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);

    nativeThis->GetString(b0, b1, &nativeRet);

    ConvertStrToJSVal(nativeRet, cx, rval);
  }
  else
  {
    JS_ReportWarning(cx, "WinProfile.getString() parameters error");
  }

  return JS_TRUE;
}





PR_STATIC_CALLBACK(JSBool)
WinProfileWriteString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsWinProfile *nativeThis =
    (nsWinProfile*)JS_GetInstancePrivate(cx, obj, &WinProfileClass, argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32 nativeRet;
  nsAutoString b0;
  nsAutoString b1;
  nsAutoString b2;

  *rval = JSVAL_ZERO;

  if(argc >= 3)
  {
    
    
    

    ConvertJSValToStr(b0, cx, argv[0]);
    ConvertJSValToStr(b1, cx, argv[1]);
    ConvertJSValToStr(b2, cx, argv[2]);

    if(NS_OK == nativeThis->WriteString(b0, b1, b2, &nativeRet))
    {
      *rval = INT_TO_JSVAL(nativeRet);
    }
  }
  else
  {
    JS_ReportWarning(cx, "WinProfile.writeString() parameters error");
  }

  return JS_TRUE;
}




PR_STATIC_CALLBACK(JSBool)
WinProfile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_FALSE;
}

static JSConstDoubleSpec winprofile_constants[] = 
{
    {0}
};




static JSFunctionSpec WinProfileMethods[] = 
{
  {"getString",         WinProfileGetString,    2,0,0},
  {"writeString",       WinProfileWriteString,  3,0,0},
  {nsnull,nsnull,0,0,0}
};

PRInt32
InitWinProfilePrototype(JSContext *jscontext, JSObject *global, JSObject **winProfilePrototype)
{
  *winProfilePrototype = JS_InitClass( jscontext,          
                                       global,             
                                       nsnull,             
                                       &WinProfileClass,   
                                       nsnull,             
                                       0,                  
                                       nsnull,             
                                       nsnull,             
                                       nsnull,             
                                       WinProfileMethods); 

  if(nsnull == *winProfilePrototype) 
  {
    return NS_ERROR_FAILURE;
  }

  if(PR_FALSE == JS_DefineConstDoubles(jscontext, *winProfilePrototype, winprofile_constants))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

