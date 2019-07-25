





































#include "jsapi.h"
#include "nscore.h"
#include "nsAutoPtr.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsIDOMInstallTriggerGlobal.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDocShell.h"
#include "nsIObserverService.h"
#include "nsInstallTrigger.h"
#include "nsXPITriggerInfo.h"
#include "nsDOMJSUtils.h"
#include "nsXPIInstallInfo.h"

#include "nsIComponentManager.h"
#include "nsNetUtil.h"
#include "nsIScriptSecurityManager.h"

#include "nsSoftwareUpdateIIDs.h"

void ConvertJSValToStr(nsString&  aString,
                      JSContext* aContext,
                      jsval      aValue)
{
  JSString *jsstring;

  if ( !JSVAL_IS_NULL(aValue) &&
       (jsstring = JS_ValueToString(aContext, aValue)) != nsnull)
  {
    aString.Assign(reinterpret_cast<const PRUnichar*>(JS_GetStringChars(jsstring)));
  }
  else
  {
    aString.Truncate();
  }
}

static void
FinalizeInstallTriggerGlobal(JSContext *cx, JSObject *obj);





JSClass InstallTriggerGlobalClass = {
  "InstallTrigger",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub,
  FinalizeInstallTriggerGlobal
};





static void
FinalizeInstallTriggerGlobal(JSContext *cx, JSObject *obj)
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

static JSBool CreateNativeObject(JSContext *cx, JSObject *obj, nsIDOMInstallTriggerGlobal **aResult)
{
    nsresult result;
    nsIScriptObjectOwner *owner = nsnull;
    nsIDOMInstallTriggerGlobal *nativeThis;

    static NS_DEFINE_CID(kInstallTrigger_CID,
                         NS_SoftwareUpdateInstallTrigger_CID);

    result = CallCreateInstance(kInstallTrigger_CID, &nativeThis);
    if (NS_FAILED(result)) return JS_FALSE;

    result = nativeThis->QueryInterface(NS_GET_IID(nsIScriptObjectOwner),
                                        (void **)&owner);

    if (NS_OK != result)
    {
        NS_RELEASE(nativeThis);
        return JS_FALSE;
    }

    owner->SetScriptObject((void *)obj);
    JS_SetPrivate(cx, obj, nativeThis);

    *aResult = nativeThis;

    NS_RELEASE(nativeThis);  
    return JS_TRUE;
}




static nsresult
InstallTriggerCheckLoadURIFromScript(JSContext *cx, const nsAString& uriStr)
{
    nsresult rv;
    nsCOMPtr<nsIScriptSecurityManager> secman(
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID,&rv));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIPrincipal> principal;
    rv = secman->GetSubjectPrincipal(getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!principal)
        return NS_ERROR_FAILURE;

    
    
    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), uriStr);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = secman->CheckLoadURIWithPrincipal(principal, uri,
                    nsIScriptSecurityManager::DISALLOW_INHERIT_PRINCIPAL);
    return rv;
}







static nsIDOMInstallTriggerGlobal* getTriggerNative(JSContext *cx, JSObject *obj)
{
  if (!JS_InstanceOf(cx, obj, &InstallTriggerGlobalClass, nsnull))
    return nsnull;

  nsIDOMInstallTriggerGlobal *native = (nsIDOMInstallTriggerGlobal*)JS_GetPrivate(cx, obj);
  if (!native) {
    
    CreateNativeObject(cx, obj, &native);
  }
  return native;
}




static JSBool
InstallTriggerGlobalUpdateEnabled(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallTriggerGlobal *nativeThis = getTriggerNative(cx, obj);
  if (!nativeThis)
    return JS_FALSE;

  *rval = JSVAL_FALSE;

  nsIScriptGlobalObject *globalObject = nsnull;
  nsIScriptContext *scriptContext = GetScriptContextFromJSContext(cx);
  if (scriptContext)
    globalObject = scriptContext->GetGlobalObject();

  PRBool nativeRet = PR_FALSE;
  if (globalObject)
    nativeThis->UpdateEnabled(globalObject, XPI_GLOBAL, &nativeRet);

  *rval = BOOLEAN_TO_JSVAL(nativeRet);
  return JS_TRUE;
}





static JSBool
InstallTriggerGlobalInstall(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallTriggerGlobal *nativeThis = getTriggerNative(cx, obj);
  if (!nativeThis)
    return JS_FALSE;

  *rval = JSVAL_FALSE;

  
  nsIScriptGlobalObject *globalObject = nsnull;
  nsIScriptContext *scriptContext = GetScriptContextFromJSContext(cx);
  if (scriptContext)
    globalObject = scriptContext->GetGlobalObject();

  if (!globalObject)
      return JS_TRUE;

  nsCOMPtr<nsIScriptSecurityManager> secman(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  if (!secman)
  {
    JS_ReportError(cx, "Could not the script security manager service.");
    return JS_FALSE;
  }
  
  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = secman->GetSubjectPrincipal(getter_AddRefs(principal));

  if (NS_FAILED(rv) || !principal)
  {
    JS_ReportError(cx, "Could not get the Subject Principal during InstallTrigger.Install()");
    return JS_FALSE;
  }

  
  nsCOMPtr<nsIURI> baseURL;
  JSObject* global = JS_GetGlobalObject(cx);
  if (global)
  {
    jsval v;
    if (JS_GetProperty(cx,global,"location",&v))
    {
      nsAutoString location;
      ConvertJSValToStr( location, cx, v );
      NS_NewURI(getter_AddRefs(baseURL), location);
    }
  }

  PRBool abortLoad = PR_FALSE;

  
  if ( argc >= 1 && JSVAL_IS_OBJECT(argv[0]) && JSVAL_TO_OBJECT(argv[0]) )
  {
    nsXPITriggerInfo *trigger = new nsXPITriggerInfo();
    if (!trigger)
      return JS_FALSE;

    trigger->SetPrincipal(principal);

    JSIdArray *ida = JS_Enumerate( cx, JSVAL_TO_OBJECT(argv[0]) );
    if ( ida )
    {
      jsval v;
      const PRUnichar *name, *URL;
      const PRUnichar *iconURL = nsnull;

      for (int i = 0; i < ida->length && !abortLoad; i++ )
      {
        JS_IdToValue( cx, ida->vector[i], &v );
        JSString * str = JS_ValueToString( cx, v );
        if (!str)
        {
          abortLoad = PR_TRUE;
          break;
        }

        name = reinterpret_cast<const PRUnichar*>(JS_GetStringChars( str ));

        URL = iconURL = nsnull;
        JSAutoByteString hash;
        JS_GetUCProperty( cx, JSVAL_TO_OBJECT(argv[0]), reinterpret_cast<const jschar*>(name), nsCRT::strlen(name), &v );
        if ( JSVAL_IS_OBJECT(v) && JSVAL_TO_OBJECT(v) )
        {
          jsval v2;
          if (JS_GetProperty( cx, JSVAL_TO_OBJECT(v), "URL", &v2 ) && !JSVAL_IS_VOID(v2)) {
            JSString *str = JS_ValueToString(cx, v2);
            if (!str) {
              abortLoad = PR_TRUE;
              break;
            }
            URL = reinterpret_cast<const PRUnichar*>(JS_GetStringChars(str));
          }

          if (JS_GetProperty( cx, JSVAL_TO_OBJECT(v), "IconURL", &v2 ) && !JSVAL_IS_VOID(v2)) {
            JSString *str = JS_ValueToString(cx, v2);
            if (!str) {
              abortLoad = PR_TRUE;
              break;
            }
            iconURL = reinterpret_cast<const PRUnichar*>(JS_GetStringChars(str));
          }

          if (JS_GetProperty( cx, JSVAL_TO_OBJECT(v), "Hash", &v2) && !JSVAL_IS_VOID(v2)) {
            JSString *str = JS_ValueToString(cx, v2);
            if (!str || !hash.encode(cx, str)) {
              abortLoad = PR_TRUE;
              break;
            }
          }
        }
        else
        {
          JSString *str = JS_ValueToString(cx, v);
          if (!str) {
            abortLoad = PR_TRUE;
            break;
          }
          URL = reinterpret_cast<const PRUnichar*>(JS_GetStringChars(str));
        }

        if ( URL )
        {
            
            nsAutoString xpiURL(URL);
            if (baseURL)
            {
                nsCAutoString resolvedURL;
                baseURL->Resolve(NS_ConvertUTF16toUTF8(xpiURL), resolvedURL);
                xpiURL = NS_ConvertUTF8toUTF16(resolvedURL);
            }

            nsAutoString icon(iconURL);
            if (iconURL && baseURL)
            {
                nsCAutoString resolvedIcon;
                baseURL->Resolve(NS_ConvertUTF16toUTF8(icon), resolvedIcon);
                icon = NS_ConvertUTF8toUTF16(resolvedIcon);
            }

            
            rv = InstallTriggerCheckLoadURIFromScript(cx, xpiURL);
            if (NS_FAILED(rv))
                abortLoad = PR_TRUE;

            if (!abortLoad && iconURL)
            {
                rv = InstallTriggerCheckLoadURIFromScript(cx, icon);
                if (NS_FAILED(rv))
                    abortLoad = PR_TRUE;
            }

            if (!abortLoad)
            {
                
                nsXPITriggerItem *item =
                    new nsXPITriggerItem( name, xpiURL.get(), icon.get(), hash );
                if ( item )
                {
                    trigger->Add( item );
                }
                else
                    abortLoad = PR_TRUE;
            }
        }
        else
            abortLoad = PR_TRUE;
      }
      JS_DestroyIdArray( cx, ida );
    }


    
    if (!abortLoad && trigger->Size() > 0)
    {
        nsCOMPtr<nsIURI> checkuri;
        rv = nativeThis->GetOriginatingURI(globalObject,
                                           getter_AddRefs(checkuri));
        if (NS_SUCCEEDED(rv))
        {
            nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(globalObject);
            nsCOMPtr<nsIXPIInstallInfo> installInfo =
                new nsXPIInstallInfo(win, checkuri, trigger, 0);
            if (installInfo)
            {
                
                PRBool enabled = PR_FALSE;
                nativeThis->UpdateEnabled(checkuri, XPI_WHITELIST, &enabled);
                if (!enabled)
                {
                    nsCOMPtr<nsIObserverService> os =
                      mozilla::services::GetObserverService();
                    if (os)
                        os->NotifyObservers(installInfo,
                                            "xpinstall-install-blocked",
                                            nsnull);
                }
                else
                {
                    
                    if ( argc >= 2 && JS_TypeOfValue(cx,argv[1]) == JSTYPE_FUNCTION )
                    {
                        trigger->SaveCallback( cx, argv[1] );
                    }

                    PRBool result;
                    nativeThis->StartInstall(installInfo, &result);
                    *rval = BOOLEAN_TO_JSVAL(result);
                }
                return JS_TRUE;
            }
        }
    }
    
    delete trigger;
  }

  JS_ReportError(cx, "Incorrect arguments to InstallTrigger.Install()");
  return JS_FALSE;
}





static JSBool
InstallTriggerGlobalInstallChrome(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallTriggerGlobal *nativeThis = getTriggerNative(cx, obj);
  if (!nativeThis)
    return JS_FALSE;

  uint32       chromeType = NOT_CHROME;
  nsAutoString sourceURL;
  nsAutoString name;

  *rval = JSVAL_FALSE;

  
  if (argc >=1)
  {
      if (!JS_ValueToECMAUint32(cx, argv[0], &chromeType))
          return JS_FALSE;
  }

  
  nsIScriptGlobalObject *globalObject = nsnull;
  nsIScriptContext *scriptContext = GetScriptContextFromJSContext(cx);
  if (scriptContext)
      globalObject = scriptContext->GetGlobalObject();

  if (!globalObject)
      return JS_TRUE;

  
  nsCOMPtr<nsIURI> baseURL;
  JSObject* global = JS_GetGlobalObject(cx);
  if (global)
  {
    jsval v;
    if (JS_GetProperty(cx,global,"location",&v))
    {
      nsAutoString location;
      ConvertJSValToStr( location, cx, v );
      NS_NewURI(getter_AddRefs(baseURL), location);
    }
  }


  if ( argc >= 3 )
  {
    ConvertJSValToStr(sourceURL, cx, argv[1]);
    ConvertJSValToStr(name, cx, argv[2]);

    if (baseURL)
    {
        nsCAutoString resolvedURL;
        baseURL->Resolve(NS_ConvertUTF16toUTF8(sourceURL), resolvedURL);
        sourceURL = NS_ConvertUTF8toUTF16(resolvedURL);
    }

    
    nsresult rv = InstallTriggerCheckLoadURIFromScript(cx, sourceURL);
    if (NS_FAILED(rv))
        return JS_FALSE;

    if ( chromeType & CHROME_ALL )
    {
        
        nsCOMPtr<nsIURI> checkuri;
        nsresult rv = nativeThis->GetOriginatingURI(globalObject,
                                                    getter_AddRefs(checkuri));
        if (NS_SUCCEEDED(rv))
        {
            nsAutoPtr<nsXPITriggerInfo> trigger(new nsXPITriggerInfo());
            nsAutoPtr<nsXPITriggerItem> item(new nsXPITriggerItem(name.get(),
                                                                  sourceURL.get(),
                                                                  nsnull));
            if (trigger && item)
            {
                
                trigger->Add(item.forget());
                nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(globalObject);
                nsCOMPtr<nsIXPIInstallInfo> installInfo =
                    new nsXPIInstallInfo(win, checkuri, trigger, chromeType);
                if (installInfo)
                {
                    
                    trigger.forget();
                    PRBool enabled = PR_FALSE;
                    nativeThis->UpdateEnabled(checkuri, XPI_WHITELIST,
                                              &enabled);
                    if (!enabled)
                    {
                        nsCOMPtr<nsIObserverService> os =
                          mozilla::services::GetObserverService();
                        if (os)
                            os->NotifyObservers(installInfo,
                                                "xpinstall-install-blocked",
                                                nsnull);
                    }
                    else
                    {
                        PRBool nativeRet = PR_FALSE;
                        nativeThis->StartInstall(installInfo, &nativeRet);
                        *rval = BOOLEAN_TO_JSVAL(nativeRet);
                    }
                }
            }
        }
    }
  }
  return JS_TRUE;
}





static JSBool
InstallTriggerGlobalStartSoftwareUpdate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsIDOMInstallTriggerGlobal *nativeThis = getTriggerNative(cx, obj);
  if (!nativeThis)
    return JS_FALSE;

  PRBool       nativeRet;
  PRInt32      flags = 0;

  *rval = JSVAL_FALSE;

  nsIScriptGlobalObject *globalObject = nsnull;
  nsIScriptContext *scriptContext = GetScriptContextFromJSContext(cx);
  if (scriptContext)
      globalObject = scriptContext->GetGlobalObject();

  if (!globalObject)
      return JS_TRUE;

  
  nsCOMPtr<nsIURI> baseURL;
  JSObject* global = JS_GetGlobalObject(cx);
  if (global)
  {
    jsval v;
    if (JS_GetProperty(cx,global,"location",&v))
    {
      nsAutoString location;
      ConvertJSValToStr( location, cx, v );
      NS_NewURI(getter_AddRefs(baseURL), location);
    }
  }


  if ( argc >= 1 )
  {
    nsAutoString xpiURL;
    ConvertJSValToStr(xpiURL, cx, argv[0]);
    if (baseURL)
    {
        nsCAutoString resolvedURL;
        baseURL->Resolve(NS_ConvertUTF16toUTF8(xpiURL), resolvedURL);
        xpiURL = NS_ConvertUTF8toUTF16(resolvedURL);
    }

    
    nsresult rv = InstallTriggerCheckLoadURIFromScript(cx, xpiURL);
    if (NS_FAILED(rv))
        return JS_FALSE;

    if (argc >= 2 && !JS_ValueToInt32(cx, argv[1], (int32 *)&flags))
    {
        JS_ReportError(cx, "StartSoftwareUpdate() 2nd parameter must be a number");
        return JS_FALSE;
    }

    nsCOMPtr<nsIURI> checkuri;
    rv = nativeThis->GetOriginatingURI(globalObject, getter_AddRefs(checkuri));
    if (NS_SUCCEEDED(rv))
    {
        nsAutoPtr<nsXPITriggerInfo> trigger(new nsXPITriggerInfo());
        nsAutoPtr<nsXPITriggerItem> item(new nsXPITriggerItem(0,
                                                              xpiURL.get(),
                                                              nsnull));
        if (trigger && item)
        {
            
            trigger->Add(item.forget());
            nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(globalObject);
            nsCOMPtr<nsIXPIInstallInfo> installInfo =
                                new nsXPIInstallInfo(win, checkuri, trigger, 0);
            if (installInfo)
            {
                
                trigger.forget();
                PRBool enabled = PR_FALSE;
                nativeThis->UpdateEnabled(checkuri, XPI_WHITELIST, &enabled);
                if (!enabled)
                {
                    nsCOMPtr<nsIObserverService> os =
                      mozilla::services::GetObserverService();
                    if (os)
                        os->NotifyObservers(installInfo,
                                            "xpinstall-install-blocked",
                                            nsnull);
                }
                else
                {
                    nativeThis->StartInstall(installInfo, &nativeRet);
                    *rval = BOOLEAN_TO_JSVAL(nativeRet);
                }
            }
        }
    }
  }
  else
  {
    JS_ReportError(cx, "Function StartSoftwareUpdate requires 1 parameters");
    return JS_FALSE;
  }

  return JS_TRUE;
}





static JSFunctionSpec InstallTriggerGlobalMethods[] =
{
  {"UpdateEnabled",         InstallTriggerGlobalUpdateEnabled,         0,0,0},
  {"StartSoftwareUpdate",   InstallTriggerGlobalStartSoftwareUpdate,   2,0,0},
  {"updateEnabled",         InstallTriggerGlobalUpdateEnabled,         0,0,0},
  {"enabled",               InstallTriggerGlobalUpdateEnabled,         0,0,0},
  {"install",               InstallTriggerGlobalInstall,               2,0,0},
  {"installChrome",         InstallTriggerGlobalInstallChrome,         2,0,0},
  {"startSoftwareUpdate",   InstallTriggerGlobalStartSoftwareUpdate,   2,0,0},
  {nsnull,nsnull,0,0,0}
};


static JSConstDoubleSpec diff_constants[] =
{
    { nsIDOMInstallTriggerGlobal::MAJOR_DIFF,    "MAJOR_DIFF" },
    { nsIDOMInstallTriggerGlobal::MINOR_DIFF,    "MINOR_DIFF" },
    { nsIDOMInstallTriggerGlobal::REL_DIFF,      "REL_DIFF"   },
    { nsIDOMInstallTriggerGlobal::BLD_DIFF,      "BLD_DIFF"   },
    { nsIDOMInstallTriggerGlobal::EQUAL,         "EQUAL"      },
    { nsIDOMInstallTriggerGlobal::NOT_FOUND,     "NOT_FOUND"  },
    { CHROME_SKIN,                               "SKIN"       },
    { CHROME_LOCALE,                             "LOCALE"     },
    { CHROME_CONTENT,                            "CONTENT"    },
    { CHROME_ALL,                                "PACKAGE"    },
    {0,nsnull}
};



nsresult InitInstallTriggerGlobalClass(JSContext *jscontext, JSObject *global, void** prototype)
{
  JSObject *proto = nsnull;

  if (prototype != nsnull)
    *prototype = nsnull;

    proto = JS_InitClass(jscontext,                       
                         global,                          
                         nsnull,                          
                         &InstallTriggerGlobalClass,      
                         nsnull,                          
                         nsnull,                          
                         nsnull,                          
                         nsnull,                          
                         nsnull,                          
                         InstallTriggerGlobalMethods);    


    if (nsnull == proto) return NS_ERROR_FAILURE;

    if ( PR_FALSE == JS_DefineConstDoubles(jscontext, proto, diff_constants) )
            return NS_ERROR_FAILURE;

    if (prototype != nsnull)
      *prototype = proto;

  return NS_OK;
}






nsresult NS_InitInstallTriggerGlobalClass(nsIScriptContext *aContext, void **aPrototype)
{
  JSContext *jscontext = aContext->GetNativeContext();
  JSObject *proto = nsnull;
  JSObject *constructor = nsnull;
  JSObject *global = JS_GetGlobalObject(jscontext);
  jsval vp;

  if ((PR_TRUE != JS_LookupProperty(jscontext, global, "InstallTriggerGlobal", &vp)) ||
      !JSVAL_IS_OBJECT(vp) ||
      ((constructor = JSVAL_TO_OBJECT(vp)) == nsnull) ||
      (PR_TRUE != JS_LookupProperty(jscontext, JSVAL_TO_OBJECT(vp), "prototype", &vp)) ||
      !JSVAL_IS_OBJECT(vp))
  {
    nsresult rv = InitInstallTriggerGlobalClass(jscontext, global, (void**)&proto);
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
NS_NewScriptInstallTriggerGlobal(nsIScriptContext *aContext,
                                 nsISupports *aSupports, nsISupports *aParent,
                                 void **aReturn)
{
  NS_PRECONDITION(nsnull != aContext && nsnull != aSupports &&
                  nsnull != aReturn,
                  "null argument to NS_NewScriptInstallTriggerGlobal");

  JSObject *proto;
  JSObject *parent = nsnull;
  JSContext *jscontext = aContext->GetNativeContext();
  nsresult result = NS_OK;
  nsIDOMInstallTriggerGlobal *installTriggerGlobal;

  nsCOMPtr<nsIScriptObjectOwner> owner(do_QueryInterface(aParent));

  if (owner) {
    if (NS_OK != owner->GetScriptObject(aContext, (void **)&parent)) {
      return NS_ERROR_FAILURE;
    }
  } else {
    nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(aParent));

    if (sgo) {
      parent = sgo->GetGlobalJSObject();
    } else {
      return NS_ERROR_FAILURE;
    }
  }

  if (NS_OK != NS_InitInstallTriggerGlobalClass(aContext, (void **)&proto)) {
    return NS_ERROR_FAILURE;
  }

  result = CallQueryInterface(aSupports, &installTriggerGlobal);
  if (NS_OK != result) {
    return result;
  }

  
  *aReturn = JS_NewObject(jscontext, &InstallTriggerGlobalClass, proto, parent);
  if (nsnull != *aReturn) {
    
    JS_SetPrivate(jscontext, (JSObject *)*aReturn, installTriggerGlobal);
  }
  else {
    NS_RELEASE(installTriggerGlobal);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

