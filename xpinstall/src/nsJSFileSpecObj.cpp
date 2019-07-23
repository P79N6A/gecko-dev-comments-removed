





































#include "jsapi.h"
#include "nscore.h"
#include "nsIScriptContext.h"

#include "nsString.h"
#include "nsInstall.h"
#include "nsJSFileSpecObj.h"

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
FileSpecObjectCleanup(JSContext *cx, JSObject *obj);





JSClass FileSpecObjectClass = {
  "FileSpecObject",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub,
  FileSpecObjectCleanup
};








JS_STATIC_DLL_CALLBACK(JSBool)
fso_ToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsInstallFolder *nativeThis =
    (nsInstallFolder*)JS_GetInstancePrivate(cx, obj, &FileSpecObjectClass,
                                            argv);
  if (!nativeThis)
    return JS_FALSE;

  nsAutoString stringReturned;

  *rval = JSVAL_NULL;

  if(NS_FAILED( nativeThis->ToString(&stringReturned)))
    return JS_TRUE;


  JSString *jsstring =
    JS_NewUCStringCopyN(cx, NS_REINTERPRET_CAST(const jschar*,
                                                stringReturned.get()),
                        stringReturned.Length());

  
  *rval = STRING_TO_JSVAL(jsstring);

  return JS_TRUE;
}





JS_STATIC_DLL_CALLBACK(JSBool)
fso_AppendPath(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_TRUE;
}





JS_STATIC_DLL_CALLBACK(void) FileSpecObjectCleanup(JSContext *cx, JSObject *obj)
{
  nsInstallFolder *nativeThis = (nsInstallFolder*)JS_GetPrivate(cx, obj);
  if (nativeThis != nsnull)
    delete nativeThis;
}




static JSFunctionSpec fileSpecObjMethods[] = 
{
  {"appendPath",        fso_AppendPath,         1,0,0},
  {"toString",          fso_ToString,           0,0,0},
  {nsnull,nsnull,0,0,0}
};


PRInt32  InitFileSpecObjectPrototype(JSContext *jscontext, 
                                      JSObject *global, 
                                      JSObject **fileSpecObjectPrototype)
{
  *fileSpecObjectPrototype  = JS_InitClass( jscontext,         
                                            global,            
                                            nsnull,            
                                            &FileSpecObjectClass, 
                                            nsnull,            
                                            0,                 
                                            nsnull,            
                                            fileSpecObjMethods,
                                            nsnull,            
                                            nsnull);           

  if (nsnull == *fileSpecObjectPrototype) 
  {
      return NS_ERROR_FAILURE;
  }

 
  return NS_OK;
}


