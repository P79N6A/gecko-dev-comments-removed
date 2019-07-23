





































#include "jsapi.h"
#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsString.h"
#include "nsIDOMInstallVersion.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsIComponentManager.h"
#include "nsDOMCID.h"

#include "nsSoftwareUpdateIIDs.h"


PR_STATIC_CALLBACK(JSBool)
GetInstallVersionProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

PR_STATIC_CALLBACK(JSBool)
SetInstallVersionProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

PR_STATIC_CALLBACK(JSBool)
EnumerateInstallVersion(JSContext *cx, JSObject *obj);

PR_STATIC_CALLBACK(JSBool)
ResolveInstallVersion(JSContext *cx, JSObject *obj, jsval id);

PR_STATIC_CALLBACK(void)
FinalizeInstallVersion(JSContext *cx, JSObject *obj);





JSClass InstallVersionClass = {
  "InstallVersion",
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub,
  JS_PropertyStub,
  GetInstallVersionProperty,
  SetInstallVersionProperty,
  EnumerateInstallVersion,
  ResolveInstallVersion,
  JS_ConvertStub,
  FinalizeInstallVersion
};


extern void ConvertJSValToStr(nsString&  aString,
                             JSContext* aContext,
                             jsval      aValue);

extern void ConvertStrToJSVal(const nsString& aProp,
                             JSContext* aContext,
                             jsval* aReturn);

extern PRBool ConvertJSValToBool(PRBool* aProp,
                                JSContext* aContext,
                                jsval aValue);

extern PRBool ConvertJSValToObj(nsISupports** aSupports,
                               REFNSIID aIID,
                               JSClass* aClass,
                               JSContext* aContext,
                               jsval aValue);

void ConvertJSvalToVersionString(nsString& versionString, JSContext* cx, jsval* argument);





enum InstallVersion_slots {
  INSTALLVERSION_MAJOR = -1,
  INSTALLVERSION_MINOR = -2,
  INSTALLVERSION_RELEASE = -3,
  INSTALLVERSION_BUILD = -4
};





JS_STATIC_DLL_CALLBACK(JSBool)
GetInstallVersionProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMInstallVersion *a = (nsIDOMInstallVersion*)JS_GetPrivate(cx, obj);

  
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case INSTALLVERSION_MAJOR:
      {
        PRInt32 prop;
        if (NS_OK == a->GetMajor(&prop)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case INSTALLVERSION_MINOR:
      {
        PRInt32 prop;
        if (NS_OK == a->GetMinor(&prop)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case INSTALLVERSION_RELEASE:
      {
        PRInt32 prop;
        if (NS_OK == a->GetRelease(&prop)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
      case INSTALLVERSION_BUILD:
      {
        PRInt32 prop;
        if (NS_OK == a->GetBuild(&prop)) {
          *vp = INT_TO_JSVAL(prop);
        }
        else {
          return JS_FALSE;
        }
        break;
      }
    }
  }

  return JS_TRUE;
}





JS_STATIC_DLL_CALLBACK(JSBool)
SetInstallVersionProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  nsIDOMInstallVersion *a = (nsIDOMInstallVersion*)JS_GetPrivate(cx, obj);

  
  if (nsnull == a) {
    return JS_TRUE;
  }

  if (JSVAL_IS_INT(id)) {
    switch(JSVAL_TO_INT(id)) {
      case INSTALLVERSION_MAJOR:
      {
        PRInt32 prop;
        int32 temp;
        if (JSVAL_IS_NUMBER(*vp) && JS_ValueToInt32(cx, *vp, &temp)) {
          prop = (PRInt32)temp;
        }
        else {
          JS_ReportError(cx, "Parameter must be a number");
          return JS_FALSE;
        }
      
        a->SetMajor(prop);
        
        break;
      }
      case INSTALLVERSION_MINOR:
      {
        PRInt32 prop;
        int32 temp;
        if (JSVAL_IS_NUMBER(*vp) && JS_ValueToInt32(cx, *vp, &temp)) {
          prop = (PRInt32)temp;
        }
        else {
          JS_ReportError(cx, "Parameter must be a number");
          return JS_FALSE;
        }
      
        a->SetMinor(prop);
        
        break;
      }
      case INSTALLVERSION_RELEASE:
      {
        PRInt32 prop;
        int32 temp;
        if (JSVAL_IS_NUMBER(*vp) && JS_ValueToInt32(cx, *vp, &temp)) {
          prop = (PRInt32)temp;
        }
        else {
          JS_ReportError(cx, "Parameter must be a number");
          return JS_FALSE;
        }
      
        a->SetRelease(prop);
        
        break;
      }
      case INSTALLVERSION_BUILD:
      {
        PRInt32 prop;
        int32 temp;
        if (JSVAL_IS_NUMBER(*vp) && JS_ValueToInt32(cx, *vp, &temp)) {
          prop = (PRInt32)temp;
        }
        else {
          JS_ReportError(cx, "Parameter must be a number");
          return JS_FALSE;
        }
      
        a->SetBuild(prop);
        
        break;
      }
    }
  }

  return JS_TRUE;
}





JS_STATIC_DLL_CALLBACK(void)
FinalizeInstallVersion(JSContext *cx, JSObject *obj)
{
  nsISupports *nativeThis = (nsISupports*)JS_GetPrivate(cx, obj);

  if (nsnull != nativeThis) {
    
    nsIScriptObjectOwner *owner = nsnull;
    if (NS_OK == nativeThis->QueryInterface(NS_GET_IID(nsIScriptObjectOwner), 
                                            (void**)&owner)) {
      owner->SetScriptObject(nsnull);
      NS_RELEASE(owner);
    }
    
    
    NS_RELEASE(nativeThis);
  }
}





JS_STATIC_DLL_CALLBACK(JSBool)
EnumerateInstallVersion(JSContext *cx, JSObject *obj)
{
  return JS_TRUE;
}





JS_STATIC_DLL_CALLBACK(JSBool)
ResolveInstallVersion(JSContext *cx, JSObject *obj, jsval id)
{
  return JS_TRUE;
}





JS_STATIC_DLL_CALLBACK(JSBool)
InstallVersionInit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallVersion *nativeThis =
    (nsIDOMInstallVersion*)JS_GetInstancePrivate(cx, obj, &InstallVersionClass,
                                                 argv);
  if (!nativeThis)
    return JS_FALSE;

  nsAutoString b0;

  *rval = JSVAL_NULL;

  
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (argc == 1) 
  {
    JSString *jsstring;
    if ((jsstring = JS_ValueToString(cx, argv[0])) != nsnull) {
      b0.Assign(reinterpret_cast<const PRUnichar*>
                                (JS_GetStringChars(jsstring)));
    }
  }
  else 
  {
      b0.AssignLiteral("0.0.0.0");
  }
    
  if (NS_OK != nativeThis->Init(b0)) 
        return JS_FALSE;
  
  *rval = JSVAL_VOID;

  return JS_TRUE;
}





JS_STATIC_DLL_CALLBACK(JSBool)
InstallVersionToString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallVersion *nativeThis =
    (nsIDOMInstallVersion*)JS_GetInstancePrivate(cx, obj, &InstallVersionClass,
                                                 argv);
  if (!nativeThis)
    return JS_FALSE;

  nsAutoString nativeRet;

  *rval = JSVAL_NULL;

  
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if (NS_OK != nativeThis->ToString(nativeRet)) {
    return JS_TRUE;
  }

  JSString *jsstring =
    JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar*>
                                            (nativeRet.get()),
                        nativeRet.Length());

  
  *rval = STRING_TO_JSVAL(jsstring);
  return JS_TRUE;
}





JS_STATIC_DLL_CALLBACK(JSBool)
InstallVersionCompareTo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallVersion *nativeThis =
    (nsIDOMInstallVersion*)JS_GetInstancePrivate(cx, obj, &InstallVersionClass,
                                                 argv);
  if (!nativeThis)
    return JS_FALSE;

  PRInt32                 nativeRet;
  nsString                b0str;
  PRInt32                 b0int;
  PRInt32                 b1int;
  PRInt32                 b2int;
  PRInt32                 b3int;

  *rval = JSVAL_NULL;

  
  if (nsnull == nativeThis) {
    return JS_TRUE;
  }

  if(argc >= 4)
  {
    
    
    
    

    if(!JSVAL_IS_INT(argv[0]))
    {
        JS_ReportError(cx, "1st parameter must be a number");
        return JS_FALSE;
    }
    else if(!JSVAL_IS_INT(argv[1]))
    {
        JS_ReportError(cx, "2nd parameter must be a number");
        return JS_FALSE;
    }
    else if(!JSVAL_IS_INT(argv[2]))
    {
        JS_ReportError(cx, "3rd parameter must be a number");
        return JS_FALSE;
    }
    else if(!JSVAL_IS_INT(argv[3]))
    {
        JS_ReportError(cx, "4th parameter must be a number");
        return JS_FALSE;
    }

    b0int = JSVAL_TO_INT(argv[0]);
    b1int = JSVAL_TO_INT(argv[1]);
    b2int = JSVAL_TO_INT(argv[2]);
    b3int = JSVAL_TO_INT(argv[3]);

    if(NS_OK != nativeThis->CompareTo(b0int, b1int, b2int, b3int, &nativeRet))
    {
      return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else if(argc >= 1)
  {
     

    if(JSVAL_IS_OBJECT(argv[0]))
    {
        nsCOMPtr<nsIDOMInstallVersion> versionObj;

        if(JS_FALSE == ConvertJSValToObj(getter_AddRefs(versionObj),
                                         NS_GET_IID(nsIDOMInstallVersion),
                                         &InstallVersionClass,
                                         cx,
                                         argv[0]))
        {
          return JS_FALSE;
        }

        if (!versionObj)
        {
          JS_ReportError(cx, "Function compareTo expects a non null object.");
          return JS_FALSE;
        }

        if(NS_OK != nativeThis->CompareTo(versionObj, &nativeRet))
        {
          return JS_FALSE;
        }
    }
    else
    {
        ConvertJSValToStr(b0str, cx, argv[0]);

        if(NS_OK != nativeThis->CompareTo(b0str, &nativeRet))
        {
          return JS_FALSE;
        }
    }

    *rval = INT_TO_JSVAL(nativeRet);
  }
  else
  {
    JS_ReportError(cx, "Function compareTo requires 4 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}





static JSPropertySpec InstallVersionProperties[] =
{
  {"major",    INSTALLVERSION_MAJOR,    JSPROP_ENUMERATE},
  {"minor",    INSTALLVERSION_MINOR,    JSPROP_ENUMERATE},
  {"release",  INSTALLVERSION_RELEASE,  JSPROP_ENUMERATE},
  {"build",    INSTALLVERSION_BUILD,    JSPROP_ENUMERATE},
  {0}
};





static JSFunctionSpec InstallVersionMethods[] = 
{
  {"init",      InstallVersionInit,             1,0,0},
  {"toString",  InstallVersionToString,         0,0,0},
  {"compareTo", InstallVersionCompareTo,        1,0,0},
  {nsnull,nsnull,0,0,0}
};

static JSConstDoubleSpec version_constants[] = 
{
    { nsIDOMInstallVersion::EQUAL,                  "EQUAL"              },
    { nsIDOMInstallVersion::BLD_DIFF,               "BLD_DIFF"           },
    { nsIDOMInstallVersion::BLD_DIFF_MINUS,         "BLD_DIFF_MINUS"     },
    { nsIDOMInstallVersion::REL_DIFF,               "REL_DIFF"           },
    { nsIDOMInstallVersion::REL_DIFF_MINUS,         "REL_DIFF_MINUS"     },
    { nsIDOMInstallVersion::MINOR_DIFF,             "MINOR_DIFF"         },
    { nsIDOMInstallVersion::MINOR_DIFF_MINUS,       "MINOR_DIFF_MINUS"   },
    { nsIDOMInstallVersion::MAJOR_DIFF,             "MAJOR_DIFF"         },
    { nsIDOMInstallVersion::MAJOR_DIFF_MINUS,       "MAJOR_DIFF_MINUS"   },
    {0,nsnull}
};






JS_STATIC_DLL_CALLBACK(JSBool)
InstallVersion(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsresult result;
  
  nsIDOMInstallVersion *nativeThis;
  nsIScriptObjectOwner *owner = nsnull;

  static NS_DEFINE_IID(kInstallVersion_CID, NS_SoftwareUpdateInstallVersion_CID);

  result = CallCreateInstance(kInstallVersion_CID, &nativeThis);
  if (NS_FAILED(result)) return JS_FALSE;

            
  result = nativeThis->QueryInterface(NS_GET_IID(nsIScriptObjectOwner),
                                      (void **)&owner);
  if (NS_FAILED(result)) {
    NS_RELEASE(nativeThis);
    return JS_FALSE;
  }

  owner->SetScriptObject((void *)obj);
  JS_SetPrivate(cx, obj, nativeThis);

  NS_RELEASE(owner);
   
  jsval ignore;
  InstallVersionInit(cx, obj, argc, argv, &ignore);

  return JS_TRUE;
}

nsresult InitInstallVersionClass(JSContext *jscontext, JSObject *global, void** prototype)
{
  JSObject *proto = nsnull;

  if (prototype != nsnull)
    *prototype = nsnull;

  proto = JS_InitClass(jscontext,     
                       global,        
                       nsnull,  
                       &InstallVersionClass,      
                       InstallVersion,            
                       0,             
                       InstallVersionProperties,  
                       InstallVersionMethods,     
                       nsnull,        
                       nsnull);       
  
  if (nsnull == proto)
      return NS_ERROR_FAILURE;
  
  
  if ( PR_FALSE == JS_DefineConstDoubles(jscontext, proto, version_constants) )
            return NS_ERROR_FAILURE;
  
  if (prototype != nsnull)
      *prototype = proto;
  
  return NS_OK;
}




nsresult NS_InitInstallVersionClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "InstallVersion", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) || 
      !JSVAL_IS_OBJECT(vp)) 
  {
    nsresult rv = InitInstallVersionClass(jscontext, global, (void**)&proto);
    if (NS_FAILED(rv)) return rv;
  }
  else if ((nsnull != constructor) && JSVAL_IS_OBJECT(vp)) 
  {
    proto = JSVAL_TO_OBJECT(vp);
  }
  else 
  {
    return NS_ERROR_FAILURE;
  }

  if (aPrototype) 
    *aPrototype = proto;
  
  return NS_OK;
}





nsresult
NS_NewScriptInstallVersion(nsIScriptContext *aContext, nsISupports *aSupports,
                           nsISupports *aParent, void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports &&
                  nsnull != aReturn,
                  "null argument to NS_NewScriptInstallVersion");

  JSObject *proto;
  JSObject *parent;
  nsIScriptObjectOwner *owner;
  JSContext *jscontext = (JSContext *)aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMInstallVersion *installVersion;

  if (nsnull == aParent) {
    parent = nsnull;
  }
  else if (NS_OK == aParent->QueryInterface(NS_GET_IID(nsIScriptObjectOwner),
                                            (void**)&owner)) {
    if (NS_OK != owner->GetScriptObject(aContext, (void **)&parent)) {
      NS_RELEASE(owner);
      return NS_ERROR_FAILURE;
    }
    NS_RELEASE(owner);
  }
  else {
    return NS_ERROR_FAILURE;
  }

  if (NS_OK != NS_InitInstallVersionClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = aSupports->QueryInterface(NS_GET_IID(nsIDOMInstallVersion), (void **)&installVersion);
  if (NS_OK != result) {
    return result;
  }

  
  *aReturn = JS_NewObject(jscontext, &InstallVersionClass, proto, parent);
  if (nsnull != *aReturn) {
    
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, installVersion);
  }
  else {
    NS_RELEASE(installVersion);
    return NS_ERROR_FAILURE; 
  }

  return NS_OK;
}

