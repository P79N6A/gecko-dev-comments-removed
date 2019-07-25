









































#include "xpcprivate.h"
#include "nsScriptSecurityManager.h"
#include "nsIServiceManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptContext.h"
#include "nsIURL.h"
#include "nsINestedURI.h"
#include "nspr.h"
#include "nsJSPrincipals.h"
#include "nsSystemPrincipal.h"
#include "nsPrincipal.h"
#include "nsNullPrincipal.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"
#include "nsCRTGlue.h"
#include "nsIJSContextStack.h"
#include "nsDOMError.h"
#include "nsDOMCID.h"
#include "jsdbgapi.h"
#include "jsarena.h"
#include "jsfun.h"
#include "jsobj.h"
#include "nsIXPConnect.h"
#include "nsIXPCSecurityManager.h"
#include "nsTextFormatter.h"
#include "nsIStringBundle.h"
#include "nsNetUtil.h"
#include "nsIProperties.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsIFileURL.h"
#include "nsIZipReader.h"
#include "nsIXPConnect.h"
#include "nsIScriptGlobalObject.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"
#include "nsIConsoleService.h"
#include "nsISecurityCheckedComponent.h"
#include "nsIJSRuntimeService.h"
#include "nsIObserverService.h"
#include "nsIContent.h"
#include "nsAutoPtr.h"
#include "nsDOMJSUtils.h"
#include "nsAboutProtocolUtils.h"
#include "nsIClassInfo.h"
#include "nsIURIFixup.h"
#include "nsCDefaultURIFixup.h"
#include "nsIChromeRegistry.h"
#include "nsPrintfCString.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);

nsIIOService    *nsScriptSecurityManager::sIOService = nsnull;
nsIXPConnect    *nsScriptSecurityManager::sXPConnect = nsnull;
nsIThreadJSContextStack *nsScriptSecurityManager::sJSContextStack = nsnull;
nsIStringBundle *nsScriptSecurityManager::sStrBundle = nsnull;
JSRuntime       *nsScriptSecurityManager::sRuntime   = 0;
PRBool nsScriptSecurityManager::sStrictFileOriginPolicy = PR_TRUE;





static inline const PRUnichar *
IDToString(JSContext *cx, jsid id)
{
    if (JSID_IS_STRING(id))
        return JS_GetInternedStringChars(JSID_TO_STRING(id));

    JSAutoRequest ar(cx);
    jsval idval;
    if (!JS_IdToValue(cx, id, &idval))
        return nsnull;
    JSString *str = JS_ValueToString(cx, idval);
    if(!str)
        return nsnull;
    return JS_GetStringCharsZ(cx, str);
}

class nsAutoInPrincipalDomainOriginSetter {
public:
    nsAutoInPrincipalDomainOriginSetter() {
        ++sInPrincipalDomainOrigin;
    }
    ~nsAutoInPrincipalDomainOriginSetter() {
        --sInPrincipalDomainOrigin;
    }
    static PRUint32 sInPrincipalDomainOrigin;
};
PRUint32 nsAutoInPrincipalDomainOriginSetter::sInPrincipalDomainOrigin;

static
nsresult
GetOriginFromURI(nsIURI* aURI, nsACString& aOrigin)
{
  if (nsAutoInPrincipalDomainOriginSetter::sInPrincipalDomainOrigin > 1) {
      
      
      
      
      return NS_ERROR_NOT_AVAILABLE;
  }

  nsAutoInPrincipalDomainOriginSetter autoSetter;

  nsCOMPtr<nsIURI> uri = NS_GetInnermostURI(aURI);
  NS_ENSURE_TRUE(uri, NS_ERROR_UNEXPECTED);

  nsCAutoString hostPort;

  nsresult rv = uri->GetHostPort(hostPort);
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString scheme;
    rv = uri->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);
    aOrigin = scheme + NS_LITERAL_CSTRING("://") + hostPort;
  }
  else {
    
    
    rv = uri->GetSpec(aOrigin);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

static
nsresult
GetPrincipalDomainOrigin(nsIPrincipal* aPrincipal,
                         nsACString& aOrigin)
{

  nsCOMPtr<nsIURI> uri;
  aPrincipal->GetDomain(getter_AddRefs(uri));
  if (!uri) {
    aPrincipal->GetURI(getter_AddRefs(uri));
  }
  NS_ENSURE_TRUE(uri, NS_ERROR_UNEXPECTED);

  return GetOriginFromURI(uri, aOrigin);
}

static nsIScriptContext *
GetScriptContext(JSContext *cx)
{
    return GetScriptContextFromJSContext(cx);
}

inline void SetPendingException(JSContext *cx, const char *aMsg)
{
    JSAutoRequest ar(cx);
    JS_ReportError(cx, "%s", aMsg);
}

inline void SetPendingException(JSContext *cx, const PRUnichar *aMsg)
{
    JSAutoRequest ar(cx);
    JS_ReportError(cx, "%hs", aMsg);
}


#ifdef DEBUG_CAPS_DomainPolicyLifeCycle
PRUint32 DomainPolicy::sObjects=0;
void DomainPolicy::_printPopulationInfo()
{
    printf("CAPS.DomainPolicy: Gen. %d, %d DomainPolicy objects.\n",
        sGeneration, sObjects);
}
#endif
PRUint32 DomainPolicy::sGeneration = 0;



class ClassInfoData
{
public:
    ClassInfoData(nsIClassInfo *aClassInfo, const char *aName)
        : mClassInfo(aClassInfo),
          mName(const_cast<char *>(aName)),
          mDidGetFlags(PR_FALSE),
          mMustFreeName(PR_FALSE)
    {
    }

    ~ClassInfoData()
    {
        if (mMustFreeName)
            nsMemory::Free(mName);
    }

    PRUint32 GetFlags()
    {
        if (!mDidGetFlags) {
            if (mClassInfo) {
                nsresult rv = mClassInfo->GetFlags(&mFlags);
                if (NS_FAILED(rv)) {
                    mFlags = 0;
                }
            } else {
                mFlags = 0;
            }

            mDidGetFlags = PR_TRUE;
        }

        return mFlags;
    }

    PRBool IsDOMClass()
    {
        return !!(GetFlags() & nsIClassInfo::DOM_OBJECT);
    }

    const char* GetName()
    {
        if (!mName) {
            if (mClassInfo) {
                mClassInfo->GetClassDescription(&mName);
            }

            if (mName) {
                mMustFreeName = PR_TRUE;
            } else {
                mName = const_cast<char *>("UnnamedClass");
            }
        }

        return mName;
    }

private:
    nsIClassInfo *mClassInfo; 
    PRUint32 mFlags;
    char *mName;
    PRPackedBool mDidGetFlags;
    PRPackedBool mMustFreeName;
};

class AutoCxPusher {
public:
    AutoCxPusher(nsIJSContextStack *aStack, JSContext *cx)
        : mStack(aStack), mContext(cx)
    {
        if (NS_FAILED(mStack->Push(mContext))) {
            mStack = nsnull;
        }
    }

    ~AutoCxPusher()
    {
        if (mStack) {
            mStack->Pop(nsnull);
        }
    }

private:
    nsCOMPtr<nsIJSContextStack> mStack;
    JSContext *mContext;
};

JSContext *
nsScriptSecurityManager::GetCurrentJSContext()
{
    
    JSContext *cx;
    if (NS_FAILED(sJSContextStack->Peek(&cx)))
        return nsnull;
    return cx;
}

JSContext *
nsScriptSecurityManager::GetSafeJSContext()
{
    
    JSContext *cx;
    if (NS_FAILED(sJSContextStack->GetSafeJSContext(&cx)))
        return nsnull;
    return cx;
}


PRBool
nsScriptSecurityManager::SecurityCompareURIs(nsIURI* aSourceURI,
                                             nsIURI* aTargetURI)
{
    return NS_SecurityCompareURIs(aSourceURI, aTargetURI, sStrictFileOriginPolicy);
}



PRUint32
nsScriptSecurityManager::SecurityHashURI(nsIURI* aURI)
{
    return NS_SecurityHashURI(aURI);
}

NS_IMETHODIMP
nsScriptSecurityManager::GetChannelPrincipal(nsIChannel* aChannel,
                                             nsIPrincipal** aPrincipal)
{
    NS_PRECONDITION(aChannel, "Must have channel!");
    nsCOMPtr<nsISupports> owner;
    aChannel->GetOwner(getter_AddRefs(owner));
    if (owner) {
        CallQueryInterface(owner, aPrincipal);
        if (*aPrincipal) {
            return NS_OK;
        }
    }

    
    
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_GetFinalChannelURI(aChannel, getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);

    return GetCodebasePrincipal(uri, aPrincipal);
}

NS_IMETHODIMP
nsScriptSecurityManager::IsSystemPrincipal(nsIPrincipal* aPrincipal,
                                           PRBool* aIsSystem)
{
    *aIsSystem = (aPrincipal == mSystemPrincipal);
    return NS_OK;
}

NS_IMETHODIMP_(nsIPrincipal *)
nsScriptSecurityManager::GetCxSubjectPrincipal(JSContext *cx)
{
    NS_ASSERTION(cx == GetCurrentJSContext(),
                 "Uh, cx is not the current JS context!");

    nsresult rv = NS_ERROR_FAILURE;
    nsIPrincipal *principal = GetSubjectPrincipal(cx, &rv);
    if (NS_FAILED(rv))
        return nsnull;

    return principal;
}

NS_IMETHODIMP_(nsIPrincipal *)
nsScriptSecurityManager::GetCxSubjectPrincipalAndFrame(JSContext *cx, JSStackFrame **fp)
{
    NS_ASSERTION(cx == GetCurrentJSContext(),
                 "Uh, cx is not the current JS context!");

    nsresult rv = NS_ERROR_FAILURE;
    nsIPrincipal *principal = GetPrincipalAndFrame(cx, fp, &rv);
    if (NS_FAILED(rv))
        return nsnull;

    return principal;
}

NS_IMETHODIMP
nsScriptSecurityManager::PushContextPrincipal(JSContext *cx,
                                              JSStackFrame *fp,
                                              nsIPrincipal *principal)
{
    NS_ASSERTION(principal, "Must pass a non-null principal");

    ContextPrincipal *cp = new ContextPrincipal(mContextPrincipals, cx, fp,
                                                principal);
    if (!cp)
        return NS_ERROR_OUT_OF_MEMORY;

    mContextPrincipals = cp;
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::PopContextPrincipal(JSContext *cx)
{
    NS_ASSERTION(mContextPrincipals->mCx == cx, "Mismatched push/pop");

    ContextPrincipal *next = mContextPrincipals->mNext;
    delete mContextPrincipals;
    mContextPrincipals = next;

    return NS_OK;
}






static PRBool
DeleteCapability(nsHashKey *aKey, void *aData, void* closure)
{
    NS_Free(aData);
    return PR_TRUE;
}


struct DomainEntry
{
    DomainEntry(const char* aOrigin,
                DomainPolicy* aDomainPolicy) : mOrigin(aOrigin),
                                               mDomainPolicy(aDomainPolicy),
                                               mNext(nsnull)
    {
        mDomainPolicy->Hold();
    }

    ~DomainEntry()
    {
        mDomainPolicy->Drop();
    }

    PRBool Matches(const char *anOrigin)
    {
        int len = strlen(anOrigin);
        int thisLen = mOrigin.Length();
        if (len < thisLen)
            return PR_FALSE;
        if (mOrigin.RFindChar(':', thisLen-1, 1) != -1)
        
            return mOrigin.EqualsIgnoreCase(anOrigin, thisLen);

        
        if (!mOrigin.Equals(anOrigin + (len - thisLen)))
            return PR_FALSE;
        if (len == thisLen)
            return PR_TRUE;
        char charBefore = anOrigin[len-thisLen-1];
        return (charBefore == '.' || charBefore == ':' || charBefore == '/');
    }

    nsCString         mOrigin;
    DomainPolicy*     mDomainPolicy;
    DomainEntry*      mNext;
#if defined(DEBUG) || defined(DEBUG_CAPS_HACKER)
    nsCString         mPolicyName_DEBUG;
#endif
};

static PRBool
DeleteDomainEntry(nsHashKey *aKey, void *aData, void* closure)
{
    DomainEntry *entry = (DomainEntry*) aData;
    do
    {
        DomainEntry *next = entry->mNext;
        delete entry;
        entry = next;
    } while (entry);
    return PR_TRUE;
}








NS_IMPL_ISUPPORTS4(nsScriptSecurityManager,
                   nsIScriptSecurityManager,
                   nsIXPCSecurityManager,
                   nsIChannelEventSink,
                   nsIObserver)






JSBool
nsScriptSecurityManager::ContentSecurityPolicyPermitsJSAction(JSContext *cx)
{
    
    nsScriptSecurityManager *ssm =
        nsScriptSecurityManager::GetScriptSecurityManager();

    NS_ASSERTION(ssm, "Failed to get security manager service");
    if (!ssm)
        return JS_FALSE;

    nsresult rv;
    nsIPrincipal* subjectPrincipal = ssm->GetSubjectPrincipal(cx, &rv);

    NS_ASSERTION(NS_SUCCEEDED(rv), "CSP: Failed to get nsIPrincipal from js context");
    if (NS_FAILED(rv))
        return JS_FALSE; 

    if (!subjectPrincipal) {
        
        NS_ASSERTION(!JS_GetSecurityCallbacks(cx)->findObjectPrincipals,
                     "CSP: Should have been able to find subject principal. "
                     "Reluctantly granting access.");
        return JS_TRUE;
    }

    nsCOMPtr<nsIContentSecurityPolicy> csp;
    rv = subjectPrincipal->GetCsp(getter_AddRefs(csp));
    NS_ASSERTION(NS_SUCCEEDED(rv), "CSP: Failed to get CSP from principal.");

    
    if (!csp)
        return JS_TRUE;

    PRBool evalOK = PR_TRUE;
    rv = csp->GetAllowsEval(&evalOK);

    if (NS_FAILED(rv))
    {
        NS_WARNING("CSP: failed to get allowsEval");
        return JS_TRUE; 
    }

    if (!evalOK) {
        
        
        JSStackFrame *fp = nsnull;
        nsAutoString fileName;
        PRUint32 lineNum = 0;
        NS_NAMED_LITERAL_STRING(scriptSample, "call to eval() or related function blocked by CSP");

        fp = JS_FrameIterator(cx, &fp);
        if (fp) {
            JSScript *script = JS_GetFrameScript(cx, fp);
            if (script) {
                const char *file = JS_GetScriptFilename(cx, script);
                if (file) {
                    CopyUTF8toUTF16(nsDependentCString(file), fileName);
                }
                jsbytecode *pc = JS_GetFramePC(cx, fp);
                if (pc) {
                    lineNum = JS_PCToLineNumber(cx, script, pc);
                }
            }
        }

        csp->LogViolationDetails(nsIContentSecurityPolicy::VIOLATION_TYPE_EVAL,
                                 fileName,
                                 scriptSample,
                                 lineNum);
    }

    return evalOK;
}


JSBool
nsScriptSecurityManager::CheckObjectAccess(JSContext *cx, JSObject *obj,
                                           jsid id, JSAccessMode mode,
                                           jsval *vp)
{
    
    nsScriptSecurityManager *ssm =
        nsScriptSecurityManager::GetScriptSecurityManager();

    NS_ASSERTION(ssm, "Failed to get security manager service");
    if (!ssm)
        return JS_FALSE;

    
    
    
    
    
    
    
    
    JSObject* target = JSVAL_IS_PRIMITIVE(*vp) ? obj : JSVAL_TO_OBJECT(*vp);

    
    
    nsresult rv =
        ssm->CheckPropertyAccess(cx, target, obj->getClass()->name, id,
                                 (mode & JSACC_WRITE) ?
                                 (PRInt32)nsIXPCSecurityManager::ACCESS_SET_PROPERTY :
                                 (PRInt32)nsIXPCSecurityManager::ACCESS_GET_PROPERTY);

    if (NS_FAILED(rv))
        return JS_FALSE; 

    return JS_TRUE;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckPropertyAccess(JSContext* cx,
                                             JSObject* aJSObject,
                                             const char* aClassName,
                                             jsid aProperty,
                                             PRUint32 aAction)
{
    return CheckPropertyAccessImpl(aAction, nsnull, cx, aJSObject,
                                   nsnull, nsnull, nsnull,
                                   aClassName, aProperty, nsnull);
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckSameOrigin(JSContext* cx,
                                         nsIURI* aTargetURI)
{
    nsresult rv;

    
    if (!cx)
    {
        cx = GetCurrentJSContext();
        if (!cx)
            return NS_OK; 
    }

    
    nsIPrincipal* sourcePrincipal = GetSubjectPrincipal(cx, &rv);
    if (NS_FAILED(rv))
        return rv;

    if (!sourcePrincipal)
    {
        NS_WARNING("CheckSameOrigin called on script w/o principals; should this happen?");
        return NS_OK;
    }

    if (sourcePrincipal == mSystemPrincipal)
    {
        
        return NS_OK;
    }

    
    
    
    nsCOMPtr<nsIURI> sourceURI;
    sourcePrincipal->GetDomain(getter_AddRefs(sourceURI));
    if (!sourceURI) {
      sourcePrincipal->GetURI(getter_AddRefs(sourceURI));
      NS_ENSURE_TRUE(sourceURI, NS_ERROR_FAILURE);
    }

    
    if (!SecurityCompareURIs(sourceURI, aTargetURI))
    {
         ReportError(cx, NS_LITERAL_STRING("CheckSameOriginError"), sourceURI, aTargetURI);
         return NS_ERROR_DOM_BAD_URI;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckSameOriginURI(nsIURI* aSourceURI,
                                            nsIURI* aTargetURI,
                                            PRBool reportError)
{
    if (!SecurityCompareURIs(aSourceURI, aTargetURI))
    {
         if (reportError) {
            ReportError(nsnull, NS_LITERAL_STRING("CheckSameOriginError"),
                     aSourceURI, aTargetURI);
         }
         return NS_ERROR_DOM_BAD_URI;
    }
    return NS_OK;
}

nsresult
nsScriptSecurityManager::CheckPropertyAccessImpl(PRUint32 aAction,
                                                 nsAXPCNativeCallContext* aCallContext,
                                                 JSContext* cx, JSObject* aJSObject,
                                                 nsISupports* aObj, nsIURI* aTargetURI,
                                                 nsIClassInfo* aClassInfo,
                                                 const char* aClassName, jsid aProperty,
                                                 void** aCachedClassPolicy)
{
    nsresult rv;
    nsIPrincipal* subjectPrincipal = GetSubjectPrincipal(cx, &rv);
    if (NS_FAILED(rv))
        return rv;

    if (!subjectPrincipal || subjectPrincipal == mSystemPrincipal)
        
        return NS_OK;

    nsCOMPtr<nsIPrincipal> objectPrincipal;

    
    
    ClassInfoData classInfoData(aClassInfo, aClassName);
#ifdef DEBUG_CAPS_CheckPropertyAccessImpl
    nsCAutoString propertyName;
    propertyName.AssignWithConversion((PRUnichar*)IDToString(cx, aProperty));
    printf("### CanAccess(%s.%s, %i) ", classInfoData.GetName(), 
           propertyName.get(), aAction);
#endif

    
    SecurityLevel securityLevel;
    rv = LookupPolicy(subjectPrincipal, classInfoData, aProperty, aAction, 
                      (ClassPolicy**)aCachedClassPolicy, &securityLevel);
    if (NS_FAILED(rv))
        return rv;

    if (securityLevel.level == SCRIPT_SECURITY_UNDEFINED_ACCESS)
    {   
        
        
        
        
        if (!aCallContext || classInfoData.IsDOMClass())
            securityLevel.level = SCRIPT_SECURITY_SAME_ORIGIN_ACCESS;
        else
            securityLevel.level = SCRIPT_SECURITY_NO_ACCESS;
    }

    if (SECURITY_ACCESS_LEVEL_FLAG(securityLevel))
    
    {
        switch (securityLevel.level)
        {
        case SCRIPT_SECURITY_NO_ACCESS:
#ifdef DEBUG_CAPS_CheckPropertyAccessImpl
            printf("noAccess ");
#endif
            rv = NS_ERROR_DOM_PROP_ACCESS_DENIED;
            break;

        case SCRIPT_SECURITY_ALL_ACCESS:
#ifdef DEBUG_CAPS_CheckPropertyAccessImpl
            printf("allAccess ");
#endif
            rv = NS_OK;
            break;

        case SCRIPT_SECURITY_SAME_ORIGIN_ACCESS:
            {
#ifdef DEBUG_CAPS_CheckPropertyAccessImpl
                printf("sameOrigin ");
#endif
                nsCOMPtr<nsIPrincipal> principalHolder;
                if(aJSObject)
                {
                    objectPrincipal = doGetObjectPrincipal(aJSObject);
                    if (!objectPrincipal)
                        rv = NS_ERROR_DOM_SECURITY_ERR;
                }
                else if(aTargetURI)
                {
                    if (NS_FAILED(GetCodebasePrincipal(
                          aTargetURI, getter_AddRefs(objectPrincipal))))
                        return NS_ERROR_FAILURE;
                }
                else
                {
                    NS_ERROR("CheckPropertyAccessImpl called without a target object or URL");
                    return NS_ERROR_FAILURE;
                }
                if(NS_SUCCEEDED(rv))
                    rv = CheckSameOriginDOMProp(subjectPrincipal, objectPrincipal,
                                                aAction);
                break;
            }
        default:
#ifdef DEBUG_CAPS_CheckPropertyAccessImpl
                printf("ERROR ");
#endif
            NS_ERROR("Bad Security Level Value");
            return NS_ERROR_FAILURE;
        }
    }
    else 
    {
#ifdef DEBUG_CAPS_CheckPropertyAccessImpl
        printf("Cap:%s ", securityLevel.capability);
#endif
        PRBool capabilityEnabled = PR_FALSE;
        rv = IsCapabilityEnabled(securityLevel.capability, &capabilityEnabled);
        if (NS_FAILED(rv) || !capabilityEnabled)
            rv = NS_ERROR_DOM_SECURITY_ERR;
        else
            rv = NS_OK;
    }

    if (NS_SUCCEEDED(rv))
    {
#ifdef DEBUG_CAPS_CheckPropertyAccessImpl
    printf(" GRANTED.\n");
#endif
        return rv;
    }

    
    
    nsCOMPtr<nsISecurityCheckedComponent> checkedComponent =
        do_QueryInterface(aObj);

    nsXPIDLCString objectSecurityLevel;
    if (checkedComponent)
    {
        nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
        nsCOMPtr<nsIInterfaceInfo> interfaceInfo;
        const nsIID* objIID = nsnull;
        rv = aCallContext->GetCalleeWrapper(getter_AddRefs(wrapper));
        if (NS_SUCCEEDED(rv) && wrapper)
            rv = wrapper->FindInterfaceWithMember(aProperty, getter_AddRefs(interfaceInfo));
        if (NS_SUCCEEDED(rv) && interfaceInfo)
            rv = interfaceInfo->GetIIDShared(&objIID);
        if (NS_SUCCEEDED(rv) && objIID)
        {
            switch (aAction)
            {
            case nsIXPCSecurityManager::ACCESS_GET_PROPERTY:
                checkedComponent->CanGetProperty(objIID,
                                                 IDToString(cx, aProperty),
                                                 getter_Copies(objectSecurityLevel));
                break;
            case nsIXPCSecurityManager::ACCESS_SET_PROPERTY:
                checkedComponent->CanSetProperty(objIID,
                                                 IDToString(cx, aProperty),
                                                 getter_Copies(objectSecurityLevel));
                break;
            case nsIXPCSecurityManager::ACCESS_CALL_METHOD:
                checkedComponent->CanCallMethod(objIID,
                                                IDToString(cx, aProperty),
                                                getter_Copies(objectSecurityLevel));
            }
        }
    }
    rv = CheckXPCPermissions(cx, aObj, aJSObject, subjectPrincipal,
                             objectSecurityLevel);
#ifdef DEBUG_CAPS_CheckPropertyAccessImpl
    if(NS_SUCCEEDED(rv))
        printf("CheckXPCPerms GRANTED.\n");
    else
        printf("CheckXPCPerms DENIED.\n");
#endif

    if (NS_FAILED(rv)) 
    {
        nsAutoString stringName;
        switch(aAction)
        {
        case nsIXPCSecurityManager::ACCESS_GET_PROPERTY:
            stringName.AssignLiteral("GetPropertyDeniedOrigins");
            break;
        case nsIXPCSecurityManager::ACCESS_SET_PROPERTY:
            stringName.AssignLiteral("SetPropertyDeniedOrigins");
            break;
        case nsIXPCSecurityManager::ACCESS_CALL_METHOD:
            stringName.AssignLiteral("CallMethodDeniedOrigins");
        }

        
        
        
        objectPrincipal = nsnull;

        NS_ConvertUTF8toUTF16 className(classInfoData.GetName());
        nsCAutoString subjectOrigin;
        nsCAutoString subjectDomain;
        if (!nsAutoInPrincipalDomainOriginSetter::sInPrincipalDomainOrigin) {
            nsCOMPtr<nsIURI> uri, domain;
            subjectPrincipal->GetURI(getter_AddRefs(uri));
            
            
            NS_ASSERTION(uri, "How did that happen?");
            GetOriginFromURI(uri, subjectOrigin);
            subjectPrincipal->GetDomain(getter_AddRefs(domain));
            if (domain) {
                GetOriginFromURI(domain, subjectDomain);
            }
        } else {
            subjectOrigin.AssignLiteral("the security manager");
        }
        NS_ConvertUTF8toUTF16 subjectOriginUnicode(subjectOrigin);
        NS_ConvertUTF8toUTF16 subjectDomainUnicode(subjectDomain);

        nsCAutoString objectOrigin;
        nsCAutoString objectDomain;
        if (!nsAutoInPrincipalDomainOriginSetter::sInPrincipalDomainOrigin &&
            objectPrincipal) {
            nsCOMPtr<nsIURI> uri, domain;
            objectPrincipal->GetURI(getter_AddRefs(uri));
            if (uri) { 
                GetOriginFromURI(uri, objectOrigin);
            }
            objectPrincipal->GetDomain(getter_AddRefs(domain));
            if (domain) {
                GetOriginFromURI(domain, objectDomain);
            }
        }
        NS_ConvertUTF8toUTF16 objectOriginUnicode(objectOrigin);
        NS_ConvertUTF8toUTF16 objectDomainUnicode(objectDomain);

        nsXPIDLString errorMsg;
        const PRUnichar *formatStrings[] =
        {
            subjectOriginUnicode.get(),
            className.get(),
            IDToString(cx, aProperty),
            objectOriginUnicode.get(),
            subjectDomainUnicode.get(),
            objectDomainUnicode.get()
        };

        PRUint32 length = NS_ARRAY_LENGTH(formatStrings);

        
        
        
        
        
        if (nsAutoInPrincipalDomainOriginSetter::sInPrincipalDomainOrigin ||
            !objectPrincipal) {
            stringName.AppendLiteral("OnlySubject");
            length -= 3;
        } else {
            
            
            length -= 2;
            if (!subjectDomainUnicode.IsEmpty()) {
                stringName.AppendLiteral("SubjectDomain");
                length += 1;
            }
            if (!objectDomainUnicode.IsEmpty()) {
                stringName.AppendLiteral("ObjectDomain");
                length += 1;
                if (length != NS_ARRAY_LENGTH(formatStrings)) {
                    
                    
                    
                    formatStrings[length-1] = formatStrings[length];
                }
            }
        }
        
        
        
        
        nsresult rv2 = sStrBundle->FormatStringFromName(stringName.get(),
                                                        formatStrings,
                                                        length,
                                                        getter_Copies(errorMsg));
        if (NS_FAILED(rv2)) {
            
            errorMsg = stringName;
        }

        SetPendingException(cx, errorMsg.get());
    }

    return rv;
}


nsresult
nsScriptSecurityManager::CheckSameOriginPrincipal(nsIPrincipal* aSubject,
                                                  nsIPrincipal* aObject)
{
    


    if (aSubject == aObject)
        return NS_OK;

    
    PRBool subjectSetDomain = PR_FALSE;
    PRBool objectSetDomain = PR_FALSE;
    
    nsCOMPtr<nsIURI> subjectURI;
    nsCOMPtr<nsIURI> objectURI;

    aSubject->GetDomain(getter_AddRefs(subjectURI));
    if (!subjectURI) {
        aSubject->GetURI(getter_AddRefs(subjectURI));
    } else {
        subjectSetDomain = PR_TRUE;
    }

    aObject->GetDomain(getter_AddRefs(objectURI));
    if (!objectURI) {
        aObject->GetURI(getter_AddRefs(objectURI));
    } else {
        objectSetDomain = PR_TRUE;
    }

    if (SecurityCompareURIs(subjectURI, objectURI))
    {   
        
        
        

        
        if (subjectSetDomain == objectSetDomain)
            return NS_OK;
    }

    


    return NS_ERROR_DOM_PROP_ACCESS_DENIED;
}













 PRUint32
nsScriptSecurityManager::HashPrincipalByOrigin(nsIPrincipal* aPrincipal)
{
    nsCOMPtr<nsIURI> uri;
    aPrincipal->GetDomain(getter_AddRefs(uri));
    if (!uri)
        aPrincipal->GetURI(getter_AddRefs(uri));
    return SecurityHashURI(uri);
}

nsresult
nsScriptSecurityManager::CheckSameOriginDOMProp(nsIPrincipal* aSubject,
                                                nsIPrincipal* aObject,
                                                PRUint32 aAction)
{
    nsresult rv;
    PRBool subsumes;
    rv = aSubject->Subsumes(aObject, &subsumes);
    if (NS_SUCCEEDED(rv) && !subsumes) {
        rv = NS_ERROR_DOM_PROP_ACCESS_DENIED;
    }
    
    if (NS_SUCCEEDED(rv))
        return NS_OK;

    


    if (aObject == mSystemPrincipal)
        return NS_ERROR_DOM_PROP_ACCESS_DENIED;

    




    PRBool capabilityEnabled = PR_FALSE;
    const char* cap = aAction == nsIXPCSecurityManager::ACCESS_SET_PROPERTY ?
                      "UniversalBrowserWrite" : "UniversalBrowserRead";
    rv = IsCapabilityEnabled(cap, &capabilityEnabled);
    NS_ENSURE_SUCCESS(rv, rv);
    if (capabilityEnabled)
        return NS_OK;

    


    return NS_ERROR_DOM_PROP_ACCESS_DENIED;
}

nsresult
nsScriptSecurityManager::LookupPolicy(nsIPrincipal* aPrincipal,
                                      ClassInfoData& aClassData,
                                      jsid aProperty,
                                      PRUint32 aAction,
                                      ClassPolicy** aCachedClassPolicy,
                                      SecurityLevel* result)
{
    nsresult rv;
    result->level = SCRIPT_SECURITY_UNDEFINED_ACCESS;

    DomainPolicy* dpolicy = nsnull;
    
    if (mPolicyPrefsChanged)
    {
        if (!mPrefInitialized) {
            rv = InitPrefs();
            NS_ENSURE_SUCCESS(rv, rv);
        }
        rv = InitPolicies();
        if (NS_FAILED(rv))
            return rv;
    }
    else
    {
        aPrincipal->GetSecurityPolicy((void**)&dpolicy);
    }

    if (!dpolicy && mOriginToPolicyMap)
    {
        
#ifdef DEBUG_CAPS_LookupPolicy
        printf("DomainLookup ");
#endif

        nsCAutoString origin;
        rv = GetPrincipalDomainOrigin(aPrincipal, origin);
        NS_ENSURE_SUCCESS(rv, rv);
 
        char *start = origin.BeginWriting();
        const char *nextToLastDot = nsnull;
        const char *lastDot = nsnull;
        const char *colon = nsnull;
        char *p = start;

        
        for (PRUint32 slashes=0; *p; p++)
        {
            if (*p == '/' && ++slashes == 3) 
            {
                *p = '\0'; 
                break;
            }
            if (*p == '.')
            {
                nextToLastDot = lastDot;
                lastDot = p;
            } 
            else if (!colon && *p == ':')
                colon = p;
        }

        nsCStringKey key(nextToLastDot ? nextToLastDot+1 : start);
        DomainEntry *de = (DomainEntry*) mOriginToPolicyMap->Get(&key);
        if (!de)
        {
            nsCAutoString scheme(start, colon-start+1);
            nsCStringKey schemeKey(scheme);
            de = (DomainEntry*) mOriginToPolicyMap->Get(&schemeKey);
        }

        while (de)
        {
            if (de->Matches(start))
            {
                dpolicy = de->mDomainPolicy;
                break;
            }
            de = de->mNext;
        }

        if (!dpolicy)
            dpolicy = mDefaultPolicy;

        aPrincipal->SetSecurityPolicy((void*)dpolicy);
    }

    ClassPolicy* cpolicy = nsnull;

    if ((dpolicy == mDefaultPolicy) && aCachedClassPolicy)
    {
        
        
        cpolicy = *aCachedClassPolicy;
    }

    if (!cpolicy)
    { 
#ifdef DEBUG_CAPS_LookupPolicy
        printf("ClassLookup ");
#endif

        cpolicy = static_cast<ClassPolicy*>
                             (PL_DHashTableOperate(dpolicy,
                                                      aClassData.GetName(),
                                                      PL_DHASH_LOOKUP));

        if (PL_DHASH_ENTRY_IS_FREE(cpolicy))
            cpolicy = NO_POLICY_FOR_CLASS;

        if ((dpolicy == mDefaultPolicy) && aCachedClassPolicy)
            *aCachedClassPolicy = cpolicy;
    }

    NS_ASSERTION(JSID_IS_INT(aProperty) || JSID_IS_OBJECT(aProperty) ||
                 JSID_IS_STRING(aProperty), "Property must be a valid id");

    
    if (!JSID_IS_STRING(aProperty))
        return NS_OK;

    JSString *propertyKey = JSID_TO_STRING(aProperty);

    
    
    
    
    
    PropertyPolicy* ppolicy = nsnull;
    if (cpolicy != NO_POLICY_FOR_CLASS)
    {
        ppolicy = static_cast<PropertyPolicy*>
                             (PL_DHashTableOperate(cpolicy->mPolicy,
                                                      propertyKey,
                                                      PL_DHASH_LOOKUP));
    }

    
    
    if (dpolicy->mWildcardPolicy &&
        (!ppolicy || PL_DHASH_ENTRY_IS_FREE(ppolicy)))
    {
        ppolicy =
            static_cast<PropertyPolicy*>
                       (PL_DHashTableOperate(dpolicy->mWildcardPolicy->mPolicy,
                                                propertyKey,
                                                PL_DHASH_LOOKUP));
    }

    
    
    
    if (dpolicy != mDefaultPolicy &&
        (!ppolicy || PL_DHASH_ENTRY_IS_FREE(ppolicy)))
    {
        cpolicy = static_cast<ClassPolicy*>
                             (PL_DHashTableOperate(mDefaultPolicy,
                                                      aClassData.GetName(),
                                                      PL_DHASH_LOOKUP));

        if (PL_DHASH_ENTRY_IS_BUSY(cpolicy))
        {
            ppolicy =
                static_cast<PropertyPolicy*>
                           (PL_DHashTableOperate(cpolicy->mPolicy,
                                                    propertyKey,
                                                    PL_DHASH_LOOKUP));
        }

        if ((!ppolicy || PL_DHASH_ENTRY_IS_FREE(ppolicy)) &&
            mDefaultPolicy->mWildcardPolicy)
        {
            ppolicy =
              static_cast<PropertyPolicy*>
                         (PL_DHashTableOperate(mDefaultPolicy->mWildcardPolicy->mPolicy,
                                                  propertyKey,
                                                  PL_DHASH_LOOKUP));
        }
    }

    if (!ppolicy || PL_DHASH_ENTRY_IS_FREE(ppolicy))
        return NS_OK;

    
    if (aAction == nsIXPCSecurityManager::ACCESS_SET_PROPERTY)
        *result = ppolicy->mSet;
    else
        *result = ppolicy->mGet;

    return NS_OK;
}


NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURIFromScript(JSContext *cx, nsIURI *aURI)
{
    
    nsresult rv;
    nsIPrincipal* principal = GetSubjectPrincipal(cx, &rv);
    if (NS_FAILED(rv))
        return rv;

    
    if (!principal)
        return NS_OK;

    rv = CheckLoadURIWithPrincipal(principal, aURI,
                                   nsIScriptSecurityManager::STANDARD);
    if (NS_SUCCEEDED(rv)) {
        
        return NS_OK;
    }

    
    
    PRBool isFile = PR_FALSE;
    PRBool isRes = PR_FALSE;
    if (NS_FAILED(aURI->SchemeIs("file", &isFile)) ||
        NS_FAILED(aURI->SchemeIs("resource", &isRes)))
        return NS_ERROR_FAILURE;
    if (isFile || isRes)
    {
        PRBool enabled;
        if (NS_FAILED(IsCapabilityEnabled("UniversalFileRead", &enabled)))
            return NS_ERROR_FAILURE;
        if (enabled)
            return NS_OK;
    }

    
    nsCAutoString spec;
    if (NS_FAILED(aURI->GetAsciiSpec(spec)))
        return NS_ERROR_FAILURE;
    nsCAutoString msg("Access to '");
    msg.Append(spec);
    msg.AppendLiteral("' from script denied");
    SetPendingException(cx, msg.get());
    return NS_ERROR_DOM_BAD_URI;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURI(nsIURI *aSourceURI, nsIURI *aTargetURI,
                                      PRUint32 aFlags)
{
    
    NS_PRECONDITION(aSourceURI, "CheckLoadURI called with null source URI");
    NS_ENSURE_ARG_POINTER(aSourceURI);

    
    
    
    
    nsCOMPtr<nsIPrincipal> sourcePrincipal;
    nsresult rv = CreateCodebasePrincipal(aSourceURI,
                                          getter_AddRefs(sourcePrincipal));
    NS_ENSURE_SUCCESS(rv, rv);
    return CheckLoadURIWithPrincipal(sourcePrincipal, aTargetURI, aFlags);
}







static nsresult
DenyAccessIfURIHasFlags(nsIURI* aURI, PRUint32 aURIFlags)
{
    NS_PRECONDITION(aURI, "Must have URI!");
    
    PRBool uriHasFlags;
    nsresult rv =
        NS_URIChainHasFlags(aURI, aURIFlags, &uriHasFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    if (uriHasFlags) {
        return NS_ERROR_DOM_BAD_URI;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURIWithPrincipal(nsIPrincipal* aPrincipal,
                                                   nsIURI *aTargetURI,
                                                   PRUint32 aFlags)
{
    NS_PRECONDITION(aPrincipal, "CheckLoadURIWithPrincipal must have a principal");
    
    
    
    NS_ENSURE_FALSE(aFlags & ~(nsIScriptSecurityManager::LOAD_IS_AUTOMATIC_DOCUMENT_REPLACEMENT |
                               nsIScriptSecurityManager::ALLOW_CHROME |
                               nsIScriptSecurityManager::DISALLOW_SCRIPT |
                               nsIScriptSecurityManager::DISALLOW_INHERIT_PRINCIPAL),
                    NS_ERROR_UNEXPECTED);
    NS_ENSURE_ARG_POINTER(aPrincipal);
    NS_ENSURE_ARG_POINTER(aTargetURI);

    if (aPrincipal == mSystemPrincipal) {
        
        return NS_OK;
    }
    
    nsCOMPtr<nsIURI> sourceURI;
    aPrincipal->GetURI(getter_AddRefs(sourceURI));
    if (!sourceURI) {
        NS_ERROR("Non-system principals passed to CheckLoadURIWithPrincipal "
                 "must have a URI!");
        return NS_ERROR_UNEXPECTED;
    }
    
    
    if (aFlags & nsIScriptSecurityManager::LOAD_IS_AUTOMATIC_DOCUMENT_REPLACEMENT) {
        nsresult rv =
            DenyAccessIfURIHasFlags(sourceURI,
                                    nsIProtocolHandler::URI_FORBIDS_AUTOMATIC_DOCUMENT_REPLACEMENT);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    
    if (aFlags & nsIScriptSecurityManager::DISALLOW_INHERIT_PRINCIPAL) {
        nsresult rv =
            DenyAccessIfURIHasFlags(aTargetURI,
                                    nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    nsCOMPtr<nsIURI> sourceBaseURI = NS_GetInnermostURI(sourceURI);
    nsCOMPtr<nsIURI> targetBaseURI = NS_GetInnermostURI(aTargetURI);

    
    nsCAutoString targetScheme;
    nsresult rv = targetBaseURI->GetScheme(targetScheme);
    if (NS_FAILED(rv)) return rv;

    
    if ((aFlags & nsIScriptSecurityManager::DISALLOW_SCRIPT) &&
         targetScheme.EqualsLiteral("javascript"))
    {
       return NS_ERROR_DOM_BAD_URI;
    }

    NS_NAMED_LITERAL_STRING(errorTag, "CheckLoadURIError");

    
    PRBool hasFlags;
    rv = NS_URIChainHasFlags(targetBaseURI,
                             nsIProtocolHandler::URI_LOADABLE_BY_SUBSUMERS,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    if (hasFlags) {
        return aPrincipal->CheckMayLoad(targetBaseURI, PR_TRUE);
    }

    
    nsCAutoString sourceScheme;
    rv = sourceBaseURI->GetScheme(sourceScheme);
    if (NS_FAILED(rv)) return rv;

    if (sourceScheme.LowerCaseEqualsLiteral(NS_NULLPRINCIPAL_SCHEME)) {
        
        if (sourceURI == aTargetURI) {
            return NS_OK;
        }
    }
    else if (targetScheme.Equals(sourceScheme,
                                 nsCaseInsensitiveCStringComparator()))
    {
        
        
        return NS_OK;
    }

    
    
    
    
    
    

    
    rv = DenyAccessIfURIHasFlags(targetBaseURI,
                                 nsIProtocolHandler::URI_DANGEROUS_TO_LOAD);
    if (NS_FAILED(rv)) {
        
        ReportError(nsnull, errorTag, sourceURI, aTargetURI);
        return rv;
    }

    
    rv = NS_URIChainHasFlags(targetBaseURI,
                             nsIProtocolHandler::URI_IS_UI_RESOURCE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) {
        if (aFlags & nsIScriptSecurityManager::ALLOW_CHROME) {
            if (!targetScheme.EqualsLiteral("chrome")) {
                
                return NS_OK;
            }

            
            nsCOMPtr<nsIXULChromeRegistry> reg(do_GetService(
                                                 NS_CHROMEREGISTRY_CONTRACTID));
            if (reg) {
                PRBool accessAllowed = PR_FALSE;
                reg->AllowContentToAccess(targetBaseURI, &accessAllowed);
                if (accessAllowed) {
                    return NS_OK;
                }
            }
        }

        
        
        
        PRBool sourceIsChrome;
        rv = NS_URIChainHasFlags(sourceBaseURI,
                                 nsIProtocolHandler::URI_IS_UI_RESOURCE,
                                 &sourceIsChrome);
        NS_ENSURE_SUCCESS(rv, rv);
        if (sourceIsChrome) {
            return NS_OK;
        }
        ReportError(nsnull, errorTag, sourceURI, aTargetURI);
        return NS_ERROR_DOM_BAD_URI;
    }

    
    rv = NS_URIChainHasFlags(targetBaseURI,
                             nsIProtocolHandler::URI_IS_LOCAL_FILE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) {
        
        
        
        PRBool sourceIsChrome;
        rv = NS_URIChainHasFlags(sourceURI,
                                 nsIProtocolHandler::URI_IS_UI_RESOURCE,
                                 &sourceIsChrome);
        NS_ENSURE_SUCCESS(rv, rv);
        if (sourceIsChrome) {
            return NS_OK;
        }

        
        static const char loadURIPrefGroup[] = "checkloaduri";
        ClassInfoData nameData(nsnull, loadURIPrefGroup);

        SecurityLevel secLevel;
        rv = LookupPolicy(aPrincipal, nameData, sEnabledID,
                          nsIXPCSecurityManager::ACCESS_GET_PROPERTY, 
                          nsnull, &secLevel);
        if (NS_SUCCEEDED(rv) && secLevel.level == SCRIPT_SECURITY_ALL_ACCESS)
        {
            
            return NS_OK;
        }

        ReportError(nsnull, errorTag, sourceURI, aTargetURI);
        return NS_ERROR_DOM_BAD_URI;
    }

    
    
    
    
    rv = NS_URIChainHasFlags(targetBaseURI,
                             nsIProtocolHandler::URI_LOADABLE_BY_ANYONE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasFlags) {
        nsXPIDLString message;
        NS_ConvertASCIItoUTF16 ucsTargetScheme(targetScheme);
        const PRUnichar* formatStrings[] = { ucsTargetScheme.get() };
        rv = sStrBundle->
            FormatStringFromName(NS_LITERAL_STRING("ProtocolFlagError").get(),
                                 formatStrings,
                                 NS_ARRAY_LENGTH(formatStrings),
                                 getter_Copies(message));
        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIConsoleService> console(
              do_GetService("@mozilla.org/consoleservice;1"));
            NS_ENSURE_TRUE(console, NS_ERROR_FAILURE);

            console->LogStringMessage(message.get());
#ifdef DEBUG
            fprintf(stderr, "%s\n", NS_ConvertUTF16toUTF8(message).get());
#endif
        }
    }
    
    return NS_OK;
}

nsresult
nsScriptSecurityManager::ReportError(JSContext* cx, const nsAString& messageTag,
                                     nsIURI* aSource, nsIURI* aTarget)
{
    nsresult rv;
    NS_ENSURE_TRUE(aSource && aTarget, NS_ERROR_NULL_POINTER);

    
    nsCAutoString sourceSpec;
    rv = aSource->GetAsciiSpec(sourceSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCAutoString targetSpec;
    rv = aTarget->GetAsciiSpec(targetSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsXPIDLString message;
    NS_ConvertASCIItoUTF16 ucsSourceSpec(sourceSpec);
    NS_ConvertASCIItoUTF16 ucsTargetSpec(targetSpec);
    const PRUnichar *formatStrings[] = { ucsSourceSpec.get(), ucsTargetSpec.get() };
    rv = sStrBundle->FormatStringFromName(PromiseFlatString(messageTag).get(),
                                          formatStrings,
                                          NS_ARRAY_LENGTH(formatStrings),
                                          getter_Copies(message));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    if (cx)
    {
        SetPendingException(cx, message.get());
    }
    else 
    {
        nsCOMPtr<nsIConsoleService> console(
            do_GetService("@mozilla.org/consoleservice;1"));
        NS_ENSURE_TRUE(console, NS_ERROR_FAILURE);

        console->LogStringMessage(message.get());
#ifdef DEBUG
        fprintf(stderr, "%s\n", NS_LossyConvertUTF16toASCII(message).get());
#endif
    }
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURIStr(const nsACString& aSourceURIStr,
                                         const nsACString& aTargetURIStr,
                                         PRUint32 aFlags)
{
    
    nsCOMPtr<nsIURI> source;
    nsresult rv = NS_NewURI(getter_AddRefs(source), aSourceURIStr,
                            nsnull, nsnull, sIOService);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    nsCOMPtr<nsIPrincipal> sourcePrincipal;
    rv = CreateCodebasePrincipal(source,
                                 getter_AddRefs(sourcePrincipal));
    NS_ENSURE_SUCCESS(rv, rv);

    return CheckLoadURIStrWithPrincipal(sourcePrincipal, aTargetURIStr,
                                        aFlags);
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckLoadURIStrWithPrincipal(nsIPrincipal* aPrincipal,
                                                      const nsACString& aTargetURIStr,
                                                      PRUint32 aFlags)
{
    nsresult rv;
    nsCOMPtr<nsIURI> target;
    rv = NS_NewURI(getter_AddRefs(target), aTargetURIStr,
                   nsnull, nsnull, sIOService);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CheckLoadURIWithPrincipal(aPrincipal, target, aFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    nsCOMPtr<nsIURIFixup> fixup = do_GetService(NS_URIFIXUP_CONTRACTID);
    if (!fixup) {
        return rv;
    }

    PRUint32 flags[] = {
        nsIURIFixup::FIXUP_FLAG_NONE,
        nsIURIFixup::FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP,
        nsIURIFixup::FIXUP_FLAGS_MAKE_ALTERNATE_URI,
        nsIURIFixup::FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP |
        nsIURIFixup::FIXUP_FLAGS_MAKE_ALTERNATE_URI
    };

    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(flags); ++i) {
        rv = fixup->CreateFixupURI(aTargetURIStr, flags[i],
                                   getter_AddRefs(target));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = CheckLoadURIWithPrincipal(aPrincipal, target, aFlags);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return rv;
}

NS_IMETHODIMP
nsScriptSecurityManager::CheckFunctionAccess(JSContext *aCx, void *aFunObj,
                                             void *aTargetObj)
{
    
    nsresult rv;
    nsIPrincipal* subject =
        GetFunctionObjectPrincipal(aCx, (JSObject *)aFunObj, nsnull, &rv);

    
    if (NS_SUCCEEDED(rv) && !subject)
    {
#ifdef DEBUG
        {
            JSFunction *fun = GET_FUNCTION_PRIVATE(cx, (JSObject *)aFunObj);
            JSScript *script = JS_GetFunctionScript(aCx, fun);

            NS_ASSERTION(!script, "Null principal for non-native function!");
        }
#endif

        subject = doGetObjectPrincipal((JSObject*)aFunObj);
    }

    if (!subject)
        return NS_ERROR_FAILURE;

    if (subject == mSystemPrincipal)
        
        return NS_OK;

    
    

    PRBool result;
    rv = CanExecuteScripts(aCx, subject, &result);
    if (NS_FAILED(rv))
      return rv;

    if (!result)
      return NS_ERROR_DOM_SECURITY_ERR;

    


    JSObject* obj = (JSObject*)aTargetObj;
    nsIPrincipal* object = doGetObjectPrincipal(obj);

    if (!object)
        return NS_ERROR_FAILURE;        

    PRBool subsumes;
    rv = subject->Subsumes(object, &subsumes);
    if (NS_SUCCEEDED(rv) && !subsumes) {
        rv = NS_ERROR_DOM_PROP_ACCESS_DENIED;
    }
    return rv;
}

NS_IMETHODIMP
nsScriptSecurityManager::CanExecuteScripts(JSContext* cx,
                                           nsIPrincipal *aPrincipal,
                                           PRBool *result)
{
    *result = PR_FALSE; 

    if (aPrincipal == mSystemPrincipal)
    {
        
        *result = PR_TRUE;
        return NS_OK;
    }

    
    nsIScriptContext *scriptContext = GetScriptContext(cx);
    if (!scriptContext) return NS_ERROR_FAILURE;

    if (!scriptContext->GetScriptsEnabled()) {
        
        *result = PR_FALSE;
        return NS_OK;
    }
    
    nsIScriptGlobalObject *sgo = scriptContext->GetGlobalObject();

    if (!sgo) {
        return NS_ERROR_FAILURE;
    }

    
    
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(sgo);
    nsCOMPtr<nsIDocShell> docshell;
    nsresult rv;

    if (window) {
        docshell = window->GetDocShell();
    }

    if (docshell) {
      rv = docshell->GetCanExecuteScripts(result);
      if (NS_FAILED(rv)) return rv;
      if (!*result) return NS_OK;
    }

    
    
    
    
    
    nsCOMPtr<nsIURI> principalURI;
    aPrincipal->GetURI(getter_AddRefs(principalURI));
    if (!principalURI) {
        
        *result = PR_FALSE;
        return NS_ERROR_UNEXPECTED;
    }
        
    PRBool isAbout;
    rv = principalURI->SchemeIs("about", &isAbout);
    if (NS_SUCCEEDED(rv) && isAbout) {
        nsCOMPtr<nsIAboutModule> module;
        rv = NS_GetAboutModule(principalURI, getter_AddRefs(module));
        if (NS_SUCCEEDED(rv)) {
            PRUint32 flags;
            rv = module->GetURIFlags(principalURI, &flags);
            if (NS_SUCCEEDED(rv) &&
                (flags & nsIAboutModule::ALLOW_SCRIPT)) {
                *result = PR_TRUE;
                return NS_OK;              
            }
        }
    }

    *result = mIsJavaScriptEnabled;
    if (!*result)
        return NS_OK; 

    
    static const char jsPrefGroupName[] = "javascript";
    ClassInfoData nameData(nsnull, jsPrefGroupName);

    SecurityLevel secLevel;
    rv = LookupPolicy(aPrincipal, nameData, sEnabledID,
                      nsIXPCSecurityManager::ACCESS_GET_PROPERTY, 
                      nsnull, &secLevel);
    if (NS_FAILED(rv) || secLevel.level == SCRIPT_SECURITY_NO_ACCESS)
    {
        *result = PR_FALSE;
        return rv;
    }

    
    *result = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsScriptSecurityManager::GetSubjectPrincipal(nsIPrincipal **aSubjectPrincipal)
{
    nsresult rv;
    *aSubjectPrincipal = doGetSubjectPrincipal(&rv);
    if (NS_SUCCEEDED(rv))
        NS_IF_ADDREF(*aSubjectPrincipal);
    return rv;
}

nsIPrincipal*
nsScriptSecurityManager::doGetSubjectPrincipal(nsresult* rv)
{
    NS_PRECONDITION(rv, "Null out param");
    JSContext *cx = GetCurrentJSContext();
    if (!cx)
    {
        *rv = NS_OK;
        return nsnull;
    }
    return GetSubjectPrincipal(cx, rv);
}

NS_IMETHODIMP
nsScriptSecurityManager::GetSystemPrincipal(nsIPrincipal **result)
{
    NS_ADDREF(*result = mSystemPrincipal);

    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::SubjectPrincipalIsSystem(PRBool* aIsSystem)
{
    NS_ENSURE_ARG_POINTER(aIsSystem);
    *aIsSystem = PR_FALSE;

    if (!mSystemPrincipal)
        return NS_OK;

    nsCOMPtr<nsIPrincipal> subject;
    nsresult rv = GetSubjectPrincipal(getter_AddRefs(subject));
    if (NS_FAILED(rv))
        return rv;

    if(!subject)
    {
        
        
        *aIsSystem = PR_TRUE;
        return NS_OK;
    }

    return mSystemPrincipal->Equals(subject, aIsSystem);
}

NS_IMETHODIMP
nsScriptSecurityManager::GetCertificatePrincipal(const nsACString& aCertFingerprint,
                                                 const nsACString& aSubjectName,
                                                 const nsACString& aPrettyName,
                                                 nsISupports* aCertificate,
                                                 nsIURI* aURI,
                                                 nsIPrincipal **result)
{
    *result = nsnull;
    
    NS_ENSURE_ARG(!aCertFingerprint.IsEmpty() &&
                  !aSubjectName.IsEmpty() &&
                  aCertificate);

    return DoGetCertificatePrincipal(aCertFingerprint, aSubjectName,
                                     aPrettyName, aCertificate, aURI, PR_TRUE,
                                     result);
}

nsresult
nsScriptSecurityManager::DoGetCertificatePrincipal(const nsACString& aCertFingerprint,
                                                   const nsACString& aSubjectName,
                                                   const nsACString& aPrettyName,
                                                   nsISupports* aCertificate,
                                                   nsIURI* aURI,
                                                   PRBool aModifyTable,
                                                   nsIPrincipal **result)
{
    NS_ENSURE_ARG(!aCertFingerprint.IsEmpty());
    
    
    
    
    nsRefPtr<nsPrincipal> certificate = new nsPrincipal();
    if (!certificate)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = certificate->Init(aCertFingerprint, aSubjectName,
                                    aPrettyName, aCertificate, aURI);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIPrincipal> fromTable;
    mPrincipals.Get(certificate, getter_AddRefs(fromTable));
    if (fromTable) {
        
        

        if (aModifyTable) {
            
            
            
            rv = static_cast<nsPrincipal*>
                            (static_cast<nsIPrincipal*>(fromTable))
                ->EnsureCertData(aSubjectName, aPrettyName, aCertificate);
            if (NS_FAILED(rv)) {
                
                
                
                NS_ADDREF(*result = certificate);
                return NS_OK;
            }                
        }
        
        if (!aURI) {
            
            
            certificate = static_cast<nsPrincipal*>
                                     (static_cast<nsIPrincipal*>
                                                 (fromTable));
        } else {
            
            
            
            
            nsXPIDLCString prefName;
            nsXPIDLCString id;
            nsXPIDLCString subjectName;
            nsXPIDLCString granted;
            nsXPIDLCString denied;
            PRBool isTrusted;
            rv = fromTable->GetPreferences(getter_Copies(prefName),
                                           getter_Copies(id),
                                           getter_Copies(subjectName),
                                           getter_Copies(granted),
                                           getter_Copies(denied),
                                           &isTrusted);
            
            if (NS_SUCCEEDED(rv)) {
                NS_ASSERTION(!isTrusted, "Shouldn't have isTrusted true here");
                
                certificate = new nsPrincipal();
                if (!certificate)
                    return NS_ERROR_OUT_OF_MEMORY;

                rv = certificate->InitFromPersistent(prefName, id,
                                                     subjectName, aPrettyName,
                                                     granted, denied,
                                                     aCertificate,
                                                     PR_TRUE, PR_FALSE);
                if (NS_FAILED(rv))
                    return rv;
                
                certificate->SetURI(aURI);
            }
        }
    }

    NS_ADDREF(*result = certificate);

    return rv;
}

nsresult
nsScriptSecurityManager::CreateCodebasePrincipal(nsIURI* aURI, nsIPrincipal **result)
{
    
    
    

    nsCOMPtr<nsIURIWithPrincipal> uriPrinc = do_QueryInterface(aURI);
    if (uriPrinc) {
        nsCOMPtr<nsIPrincipal> principal;
        uriPrinc->GetPrincipal(getter_AddRefs(principal));
        if (!principal || principal == mSystemPrincipal) {
            return CallCreateInstance(NS_NULLPRINCIPAL_CONTRACTID, result);
        }

        principal.forget(result);

        return NS_OK;
    }

    nsRefPtr<nsPrincipal> codebase = new nsPrincipal();
    if (!codebase)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = codebase->Init(EmptyCString(), EmptyCString(),
                                 EmptyCString(), nsnull, aURI);
    if (NS_FAILED(rv))
        return rv;

    NS_ADDREF(*result = codebase);

    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::GetCodebasePrincipal(nsIURI *aURI,
                                              nsIPrincipal **result)
{
    NS_ENSURE_ARG(aURI);
    
    PRBool inheritsPrincipal;
    nsresult rv =
        NS_URIChainHasFlags(aURI,
                            nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                            &inheritsPrincipal);
    if (NS_FAILED(rv) || inheritsPrincipal) {
        return CallCreateInstance(NS_NULLPRINCIPAL_CONTRACTID, result);
    }
    
    nsCOMPtr<nsIPrincipal> principal;
    rv = CreateCodebasePrincipal(aURI, getter_AddRefs(principal));
    if (NS_FAILED(rv)) return rv;

    if (mPrincipals.Count() > 0)
    {
        
        nsCOMPtr<nsIPrincipal> fromTable;
        mPrincipals.Get(principal, getter_AddRefs(fromTable));
        if (fromTable) {
            
            
            
            
            
            
            nsXPIDLCString prefName;
            nsXPIDLCString id;
            nsXPIDLCString subjectName;
            nsXPIDLCString granted;
            nsXPIDLCString denied;
            PRBool isTrusted;
            rv = fromTable->GetPreferences(getter_Copies(prefName),
                                           getter_Copies(id),
                                           getter_Copies(subjectName),
                                           getter_Copies(granted),
                                           getter_Copies(denied),
                                           &isTrusted);
            if (NS_SUCCEEDED(rv)) {
                nsRefPtr<nsPrincipal> codebase = new nsPrincipal();
                if (!codebase)
                    return NS_ERROR_OUT_OF_MEMORY;

                rv = codebase->InitFromPersistent(prefName, id,
                                                  subjectName, EmptyCString(),
                                                  granted, denied,
                                                  nsnull, PR_FALSE,
                                                  isTrusted);
                if (NS_FAILED(rv))
                    return rv;
                
                codebase->SetURI(aURI);
                principal = codebase;
            }

        }
    }

    NS_IF_ADDREF(*result = principal);

    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::GetPrincipalFromContext(JSContext *cx,
                                                 nsIPrincipal **result)
{
    *result = nsnull;

    nsIScriptContextPrincipal* scp =
        GetScriptContextPrincipalFromJSContext(cx);

    if (!scp)
    {
        return NS_ERROR_FAILURE;
    }

    nsIScriptObjectPrincipal* globalData = scp->GetObjectPrincipal();
    if (globalData)
        NS_IF_ADDREF(*result = globalData->GetPrincipal());

    return NS_OK;
}


nsIPrincipal*
nsScriptSecurityManager::GetScriptPrincipal(JSContext *cx,
                                            JSScript *script,
                                            nsresult* rv)
{
    NS_PRECONDITION(rv, "Null out param");
    *rv = NS_OK;
    if (!script)
    {
        return nsnull;
    }
    JSPrincipals *jsp = JS_GetScriptPrincipals(cx, script);
    if (!jsp) {
        *rv = NS_ERROR_FAILURE;
        NS_ERROR("Script compiled without principals!");
        return nsnull;
    }
    nsJSPrincipals *nsJSPrin = static_cast<nsJSPrincipals *>(jsp);
    nsIPrincipal* result = nsJSPrin->nsIPrincipalPtr;
    if (!result)
        *rv = NS_ERROR_FAILURE;
    return result;
}


nsIPrincipal*
nsScriptSecurityManager::GetFunctionObjectPrincipal(JSContext *cx,
                                                    JSObject *obj,
                                                    JSStackFrame *fp,
                                                    nsresult *rv)
{
    NS_PRECONDITION(rv, "Null out param");

    *rv = NS_OK;

    if (!JS_ObjectIsFunction(cx, obj))
    {
        
        nsIPrincipal *result = doGetObjectPrincipal(obj);
        if (!result)
            *rv = NS_ERROR_FAILURE;
        return result;
    }

    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);
    JSScript *script = JS_GetFunctionScript(cx, fun);

    if (!script)
    {
        
        return nsnull;
    }

    JSScript *frameScript = fp ? JS_GetFrameScript(cx, fp) : nsnull;

    if (frameScript && frameScript != script)
    {
        
        
        
        
        
        
        
        

        script = frameScript;
    }
    else if (JS_GetFunctionObject(fun) != obj)
    {
        
        
        
        
        
        
        
        
        
        
        

        nsIPrincipal *result = doGetObjectPrincipal(obj);
        if (!result)
            *rv = NS_ERROR_FAILURE;
        return result;
    }

    return GetScriptPrincipal(cx, script, rv);
}

nsIPrincipal*
nsScriptSecurityManager::GetFramePrincipal(JSContext *cx,
                                           JSStackFrame *fp,
                                           nsresult *rv)
{
    NS_PRECONDITION(rv, "Null out param");
    JSObject *obj = JS_GetFrameFunctionObject(cx, fp);
    if (!obj)
    {
        
        JSScript *script = JS_GetFrameScript(cx, fp);
        return GetScriptPrincipal(cx, script, rv);
    }

    nsIPrincipal* result = GetFunctionObjectPrincipal(cx, obj, fp, rv);

#ifdef DEBUG
    if (NS_SUCCEEDED(*rv) && !result)
    {
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);
        JSScript *script = JS_GetFunctionScript(cx, fun);

        NS_ASSERTION(!script, "Null principal for non-native function!");
    }
#endif

    return result;
}

nsIPrincipal*
nsScriptSecurityManager::GetPrincipalAndFrame(JSContext *cx,
                                              JSStackFrame **frameResult,
                                              nsresult* rv)
{
    NS_PRECONDITION(rv, "Null out param");
    
    
    *rv = NS_OK;

    if (cx)
    {
        JSStackFrame *target = nsnull;
        nsIPrincipal *targetPrincipal = nsnull;
        for (ContextPrincipal *cp = mContextPrincipals; cp; cp = cp->mNext)
        {
            if (cp->mCx == cx)
            {
                target = cp->mFp;
                targetPrincipal = cp->mPrincipal;
                break;
            }
        }

        
        JSStackFrame *fp = nsnull; 
        for (fp = JS_FrameIterator(cx, &fp); fp; fp = JS_FrameIterator(cx, &fp))
        {
            if (fp == target)
                break;
            nsIPrincipal* result = GetFramePrincipal(cx, fp, rv);
            if (result)
            {
                NS_ASSERTION(NS_SUCCEEDED(*rv), "Weird return");
                *frameResult = fp;
                return result;
            }
        }

        
        
        
        
        if (targetPrincipal)
        {
            if (fp && fp == target)
            {
                *frameResult = fp;
            }
            else
            {
                JSStackFrame *inner = nsnull;
                *frameResult = JS_FrameIterator(cx, &inner);
            }

            return targetPrincipal;
        }

        nsIScriptContextPrincipal* scp =
            GetScriptContextPrincipalFromJSContext(cx);
        if (scp)
        {
            nsIScriptObjectPrincipal* globalData = scp->GetObjectPrincipal();
            if (!globalData)
            {
                *rv = NS_ERROR_FAILURE;
                return nsnull;
            }

            
            
            nsIPrincipal* result = globalData->GetPrincipal();
            if (result)
            {
                JSStackFrame *inner = nsnull;
                *frameResult = JS_FrameIterator(cx, &inner);
                return result;
            }
        }
    }

    return nsnull;
}

nsIPrincipal*
nsScriptSecurityManager::GetSubjectPrincipal(JSContext *cx,
                                             nsresult* rv)
{
    NS_PRECONDITION(rv, "Null out param");
    JSStackFrame *fp;
    return GetPrincipalAndFrame(cx, &fp, rv);
}

NS_IMETHODIMP
nsScriptSecurityManager::GetObjectPrincipal(JSContext *aCx, JSObject *aObj,
                                            nsIPrincipal **result)
{
    *result = doGetObjectPrincipal(aObj);
    if (!*result)
        return NS_ERROR_FAILURE;
    NS_ADDREF(*result);
    return NS_OK;
}


nsIPrincipal*
nsScriptSecurityManager::doGetObjectPrincipal(JSObject *aObj
#ifdef DEBUG
                                              , PRBool aAllowShortCircuit
#endif
                                              )
{
    NS_ASSERTION(aObj, "Bad call to doGetObjectPrincipal()!");
    nsIPrincipal* result = nsnull;

#ifdef DEBUG
    JSObject* origObj = aObj;
#endif
    
    js::Class *jsClass = aObj->getClass();

    
    
    
    
    
    

    if (jsClass == &js_FunctionClass) {
        aObj = aObj->getParent();

        if (!aObj)
            return nsnull;

        jsClass = aObj->getClass();

        if (jsClass == &js_CallClass) {
            aObj = aObj->getParent();

            if (!aObj)
                return nsnull;

            jsClass = aObj->getClass();
        }
    }

    do {
        
        
        
        if (IS_WRAPPER_CLASS(jsClass)) {
            result = sXPConnect->GetPrincipal(aObj,
#ifdef DEBUG
                                              aAllowShortCircuit
#else
                                              PR_TRUE
#endif
                                              );
            if (result) {
                break;
            }
        } else if (!(~jsClass->flags & (JSCLASS_HAS_PRIVATE |
                                        JSCLASS_PRIVATE_IS_NSISUPPORTS))) {
            nsISupports *priv = (nsISupports *) aObj->getPrivate();

#ifdef DEBUG
            if (aAllowShortCircuit) {
                nsCOMPtr<nsIXPConnectWrappedNative> xpcWrapper =
                    do_QueryInterface(priv);

                NS_ASSERTION(!xpcWrapper ||
                             !strcmp(jsClass->name, "XPCNativeWrapper"),
                             "Uh, an nsIXPConnectWrappedNative with the "
                             "wrong JSClass or getObjectOps hooks!");
            }
#endif

            nsCOMPtr<nsIScriptObjectPrincipal> objPrin =
                do_QueryInterface(priv);

            if (objPrin) {
                result = objPrin->GetPrincipal();

                if (result) {
                    break;
                }
            }
        }

        aObj = aObj->getParent();

        if (!aObj)
            break;

        jsClass = aObj->getClass();
    } while (1);

#ifdef DEBUG
    if (aAllowShortCircuit) {
        nsIPrincipal *principal = doGetObjectPrincipal(origObj, PR_FALSE);

        
        
        NS_ASSERTION(strcmp(jsClass->name, "Location") == 0 ?
                     NS_SUCCEEDED(CheckSameOriginPrincipal(result, principal)) :
                     result == principal,
                     "Principal mismatch.  Not good");
    }
#endif

    return result;
}

nsresult
nsScriptSecurityManager::SavePrincipal(nsIPrincipal* aToSave)
{
    
    mPrincipals.Put(aToSave, aToSave);

    
    nsXPIDLCString idPrefName;
    nsXPIDLCString id;
    nsXPIDLCString subjectName;
    nsXPIDLCString grantedList;
    nsXPIDLCString deniedList;
    PRBool isTrusted;
    nsresult rv = aToSave->GetPreferences(getter_Copies(idPrefName),
                                          getter_Copies(id),
                                          getter_Copies(subjectName),
                                          getter_Copies(grantedList),
                                          getter_Copies(deniedList),
                                          &isTrusted);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    nsCAutoString grantedPrefName;
    nsCAutoString deniedPrefName;
    nsCAutoString subjectNamePrefName;
    rv = GetPrincipalPrefNames( idPrefName,
                                grantedPrefName,
                                deniedPrefName,
                                subjectNamePrefName );
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    mIsWritingPrefs = PR_TRUE;
    if (grantedList) {
        Preferences::SetCString(grantedPrefName.get(), grantedList);
    } else {
        Preferences::ClearUser(grantedPrefName.get());
    }

    if (deniedList) {
        Preferences::SetCString(deniedPrefName.get(), deniedList);
    } else {
        Preferences::ClearUser(deniedPrefName.get());
    }

    if (grantedList || deniedList) {
        Preferences::SetCString(idPrefName, id);
        Preferences::SetCString(subjectNamePrefName.get(), subjectName);
    } else {
        Preferences::ClearUser(idPrefName);
        Preferences::ClearUser(subjectNamePrefName.get());
    }

    mIsWritingPrefs = PR_FALSE;

    nsIPrefService* prefService = Preferences::GetService();
    NS_ENSURE_TRUE(prefService, NS_ERROR_FAILURE);
    return prefService->SavePrefFile(nsnull);
}


NS_IMETHODIMP
nsScriptSecurityManager::IsCapabilityEnabled(const char *capability,
                                             PRBool *result)
{
    nsresult rv;
    JSStackFrame *fp = nsnull;
    JSContext *cx = GetCurrentJSContext();
    fp = cx ? JS_FrameIterator(cx, &fp) : nsnull;

    JSStackFrame *target = nsnull;
    nsIPrincipal *targetPrincipal = nsnull;
    for (ContextPrincipal *cp = mContextPrincipals; cp; cp = cp->mNext)
    {
        if (cp->mCx == cx)
        {
            target = cp->mFp;
            targetPrincipal = cp->mPrincipal;
            break;
        }
    }

    if (!fp)
    {
        
        
        

        *result = (targetPrincipal && !target)
                  ? (targetPrincipal == mSystemPrincipal)
                  : PR_TRUE;

        return NS_OK;
    }

    *result = PR_FALSE;
    nsIPrincipal* previousPrincipal = nsnull;
    do
    {
        nsIPrincipal* principal = GetFramePrincipal(cx, fp, &rv);
        if (NS_FAILED(rv))
            return rv;
        if (!principal)
            continue;
        
        if(previousPrincipal)
        {
            PRBool isEqual = PR_FALSE;
            if(NS_FAILED(previousPrincipal->Equals(principal, &isEqual)) || !isEqual)
                break;
        }
        else
            previousPrincipal = principal;

        
        
        PRInt16 canEnable;
        rv = principal->CanEnableCapability(capability, &canEnable);
        if (NS_FAILED(rv)) return rv;
        if (canEnable != nsIPrincipal::ENABLE_GRANTED &&
            canEnable != nsIPrincipal::ENABLE_WITH_USER_PERMISSION)
            return NS_OK;

        
        void *annotation = JS_GetFrameAnnotation(cx, fp);
        rv = principal->IsCapabilityEnabled(capability, annotation, result);
        if (NS_FAILED(rv)) return rv;
        if (*result)
            return NS_OK;

        
        
        if (JS_IsGlobalFrame(cx, fp))
            break;
    } while ((fp = JS_FrameIterator(cx, &fp)) != nsnull);

    if (!previousPrincipal)
    {
        
        

        return SubjectPrincipalIsSystem(result);
    }

    return NS_OK;
}

void
nsScriptSecurityManager::FormatCapabilityString(nsAString& aCapability)
{
    nsAutoString newcaps;
    nsAutoString rawcap;
    NS_NAMED_LITERAL_STRING(capdesc, "capdesc.");
    PRInt32 pos;
    PRInt32 index = kNotFound;
    nsresult rv;

    NS_ASSERTION(kNotFound == -1, "Basic constant changed, algorithm broken!");

    do {
        pos = index+1;
        index = aCapability.FindChar(' ', pos);
        rawcap = Substring(aCapability, pos,
                           (index == kNotFound) ? index : index - pos);

        nsXPIDLString capstr;
        rv = sStrBundle->GetStringFromName(
                            nsPromiseFlatString(capdesc+rawcap).get(),
                            getter_Copies(capstr));
        if (NS_SUCCEEDED(rv))
            newcaps += capstr;
        else
        {
            nsXPIDLString extensionCap;
            const PRUnichar* formatArgs[] = { rawcap.get() };
            rv = sStrBundle->FormatStringFromName(
                                NS_LITERAL_STRING("ExtensionCapability").get(),
                                formatArgs,
                                NS_ARRAY_LENGTH(formatArgs),
                                getter_Copies(extensionCap));
            if (NS_SUCCEEDED(rv))
                newcaps += extensionCap;
            else
                newcaps += rawcap;
        }

        newcaps += NS_LITERAL_STRING("\n");
    } while (index != kNotFound);

    aCapability = newcaps;
}

PRBool
nsScriptSecurityManager::CheckConfirmDialog(JSContext* cx, nsIPrincipal* aPrincipal,
                                            const char* aCapability, PRBool *checkValue)
{
    nsresult rv;
    *checkValue = PR_FALSE;

    
    nsCOMPtr<nsIPrompt> prompter;
    if (cx)
    {
        nsIScriptContext *scriptContext = GetScriptContext(cx);
        if (scriptContext)
        {
            nsCOMPtr<nsIDOMWindowInternal> domWin =
                do_QueryInterface(scriptContext->GetGlobalObject());
            if (domWin)
                domWin->GetPrompter(getter_AddRefs(prompter));
        }
    }

    if (!prompter)
    {
        
        nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
        if (wwatch)
          wwatch->GetNewPrompter(0, getter_AddRefs(prompter));
        if (!prompter)
            return PR_FALSE;
    }

    
    nsXPIDLString check;
    rv = sStrBundle->GetStringFromName(NS_LITERAL_STRING("CheckMessage").get(),
                                       getter_Copies(check));
    if (NS_FAILED(rv))
        return PR_FALSE;

    nsXPIDLString title;
    rv = sStrBundle->GetStringFromName(NS_LITERAL_STRING("Titleline").get(),
                                       getter_Copies(title));
    if (NS_FAILED(rv))
        return PR_FALSE;

    nsXPIDLString yesStr;
    rv = sStrBundle->GetStringFromName(NS_LITERAL_STRING("Yes").get(),
                                       getter_Copies(yesStr));
    if (NS_FAILED(rv))
        return PR_FALSE;

    nsXPIDLString noStr;
    rv = sStrBundle->GetStringFromName(NS_LITERAL_STRING("No").get(),
                                       getter_Copies(noStr));
    if (NS_FAILED(rv))
        return PR_FALSE;

    nsCAutoString val;
    PRBool hasCert;
    aPrincipal->GetHasCertificate(&hasCert);
    if (hasCert)
        rv = aPrincipal->GetPrettyName(val);
    else
        rv = GetPrincipalDomainOrigin(aPrincipal, val);

    if (NS_FAILED(rv))
        return PR_FALSE;

    NS_ConvertUTF8toUTF16 location(val);
    NS_ConvertASCIItoUTF16 capability(aCapability);
    FormatCapabilityString(capability);
    const PRUnichar *formatStrings[] = { location.get(), capability.get() };

    nsXPIDLString message;
    rv = sStrBundle->FormatStringFromName(NS_LITERAL_STRING("EnableCapabilityQuery").get(),
                                          formatStrings,
                                          NS_ARRAY_LENGTH(formatStrings),
                                          getter_Copies(message));
    if (NS_FAILED(rv))
        return PR_FALSE;

    PRInt32 buttonPressed = 1; 
    rv = prompter->ConfirmEx(title.get(), message.get(),
                             (nsIPrompt::BUTTON_DELAY_ENABLE) +
                             (nsIPrompt::BUTTON_POS_1_DEFAULT) +
                             (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_0) +
                             (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_1),
                             yesStr.get(), noStr.get(), nsnull, check.get(), checkValue, &buttonPressed);

    if (NS_FAILED(rv))
        *checkValue = PR_FALSE;
    return (buttonPressed == 0);
}

NS_IMETHODIMP
nsScriptSecurityManager::RequestCapability(nsIPrincipal* aPrincipal,
                                           const char *capability, PRInt16* canEnable)
{
    if (NS_FAILED(aPrincipal->CanEnableCapability(capability, canEnable)))
        return NS_ERROR_FAILURE;
    if (*canEnable == nsIPrincipal::ENABLE_WITH_USER_PERMISSION)
    {
        
        JSContext* cx = GetCurrentJSContext();
        
        
        PRBool remember = PR_FALSE;
        if (CheckConfirmDialog(cx, aPrincipal, capability, &remember))
            *canEnable = nsIPrincipal::ENABLE_GRANTED;
        else
            *canEnable = nsIPrincipal::ENABLE_DENIED;
        if (remember)
        {
            
            if (NS_FAILED(aPrincipal->SetCanEnableCapability(capability, *canEnable)))
                return NS_ERROR_FAILURE;
            if (NS_FAILED(SavePrincipal(aPrincipal)))
                return NS_ERROR_FAILURE;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::EnableCapability(const char *capability)
{
    JSContext *cx = GetCurrentJSContext();
    JSStackFrame *fp;

    
    if(PL_strlen(capability)>200)
    {
        static const char msg[] = "Capability name too long";
        SetPendingException(cx, msg);
        return NS_ERROR_FAILURE;
    }

    
    
    
    
    
    
    
    
    
    
    
    for (const char *ch = capability; *ch; ++ch)
    {
        if (!NS_IS_ALPHA(*ch) && *ch != ' ' && !NS_IS_DIGIT(*ch)
            && *ch != '_' && *ch != '-' && *ch != '.')
        {
            static const char msg[] = "Invalid character in capability name";
            SetPendingException(cx, msg);
            return NS_ERROR_FAILURE;
        }
    }

    nsresult rv;
    nsIPrincipal* principal = GetPrincipalAndFrame(cx, &fp, &rv);
    if (NS_FAILED(rv))
        return rv;
    if (!principal)
        return NS_ERROR_NOT_AVAILABLE;

    void *annotation = JS_GetFrameAnnotation(cx, fp);
    PRBool enabled;
    if (NS_FAILED(principal->IsCapabilityEnabled(capability, annotation,
                                                 &enabled)))
        return NS_ERROR_FAILURE;
    if (enabled)
        return NS_OK;

    PRInt16 canEnable;
    if (NS_FAILED(RequestCapability(principal, capability, &canEnable)))
        return NS_ERROR_FAILURE;

    if (canEnable != nsIPrincipal::ENABLE_GRANTED)
    {
        nsCAutoString val;
        PRBool hasCert;
        nsresult rv;
        principal->GetHasCertificate(&hasCert);
        if (hasCert)
            rv = principal->GetPrettyName(val);
        else
            rv = GetPrincipalDomainOrigin(principal, val);

        if (NS_FAILED(rv))
            return rv;

        NS_ConvertUTF8toUTF16 location(val);
        NS_ConvertUTF8toUTF16 cap(capability);
        const PRUnichar *formatStrings[] = { location.get(), cap.get() };

        nsXPIDLString message;
        rv = sStrBundle->FormatStringFromName(NS_LITERAL_STRING("EnableCapabilityDenied").get(),
                                              formatStrings,
                                              NS_ARRAY_LENGTH(formatStrings),
                                              getter_Copies(message));
        if (NS_FAILED(rv))
            return rv;

        SetPendingException(cx, message.get());

        return NS_ERROR_FAILURE; 
    }
    if (NS_FAILED(principal->EnableCapability(capability, &annotation)))
        return NS_ERROR_FAILURE;
    JS_SetFrameAnnotation(cx, fp, annotation);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::RevertCapability(const char *capability)
{
    JSContext *cx = GetCurrentJSContext();
    JSStackFrame *fp;
    nsresult rv;
    nsIPrincipal* principal = GetPrincipalAndFrame(cx, &fp, &rv);
    if (NS_FAILED(rv))
        return rv;
    if (!principal)
        return NS_ERROR_NOT_AVAILABLE;
    void *annotation = JS_GetFrameAnnotation(cx, fp);
    principal->RevertCapability(capability, &annotation);
    JS_SetFrameAnnotation(cx, fp, annotation);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptSecurityManager::DisableCapability(const char *capability)
{
    JSContext *cx = GetCurrentJSContext();
    JSStackFrame *fp;
    nsresult rv;
    nsIPrincipal* principal = GetPrincipalAndFrame(cx, &fp, &rv);
    if (NS_FAILED(rv))
        return rv;
    if (!principal)
        return NS_ERROR_NOT_AVAILABLE;
    void *annotation = JS_GetFrameAnnotation(cx, fp);
    principal->DisableCapability(capability, &annotation);
    JS_SetFrameAnnotation(cx, fp, annotation);
    return NS_OK;
}


NS_IMETHODIMP
nsScriptSecurityManager::SetCanEnableCapability(const nsACString& certFingerprint,
                                                const char* capability,
                                                PRInt16 canEnable)
{
    NS_ENSURE_ARG(!certFingerprint.IsEmpty());
    
    nsresult rv;
    nsIPrincipal* subjectPrincipal = doGetSubjectPrincipal(&rv);
    if (NS_FAILED(rv))
        return rv;

    
    if (!mSystemCertificate)
    {
        nsCOMPtr<nsIFile> systemCertFile;
        nsCOMPtr<nsIProperties> directoryService =
                 do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
        if (!directoryService) return NS_ERROR_FAILURE;
        rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsIFile),
                              getter_AddRefs(systemCertFile));
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        systemCertFile->AppendNative(NS_LITERAL_CSTRING("systemSignature.jar"));
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        nsCOMPtr<nsIZipReader> systemCertZip = do_CreateInstance(kZipReaderCID, &rv);
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        rv = systemCertZip->Open(systemCertFile);
        if (NS_SUCCEEDED(rv))
        {
            rv = systemCertZip->GetCertificatePrincipal(nsnull,
                                                        getter_AddRefs(mSystemCertificate));
            if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
        }
    }

    
    PRBool isEqual = PR_FALSE;
    if (mSystemCertificate)
    {
        rv = mSystemCertificate->Equals(subjectPrincipal, &isEqual);
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    }
    if (!isEqual)
    {
        JSContext* cx = GetCurrentJSContext();
        if (!cx) return NS_ERROR_FAILURE;
        static const char msg1[] = "Only code signed by the system certificate may call SetCanEnableCapability or Invalidate";
        static const char msg2[] = "Attempt to call SetCanEnableCapability or Invalidate when no system certificate has been established";
        SetPendingException(cx, mSystemCertificate ? msg1 : msg2);
        return NS_ERROR_FAILURE;
    }

    
    nsCOMPtr<nsIPrincipal> objectPrincipal;
    rv = DoGetCertificatePrincipal(certFingerprint, EmptyCString(),
                                   EmptyCString(), nsnull,
                                   nsnull, PR_FALSE,
                                   getter_AddRefs(objectPrincipal));
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    rv = objectPrincipal->SetCanEnableCapability(capability, canEnable);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    return SavePrincipal(objectPrincipal);
}





NS_IMETHODIMP
nsScriptSecurityManager::CanCreateWrapper(JSContext *cx,
                                          const nsIID &aIID,
                                          nsISupports *aObj,
                                          nsIClassInfo *aClassInfo,
                                          void **aPolicy)
{
#ifdef DEBUG_CAPS_CanCreateWrapper
    char* iidStr = aIID.ToString();
    printf("### CanCreateWrapper(%s) ", iidStr);
    NS_Free(iidStr);
#endif

    ClassInfoData objClassInfo = ClassInfoData(aClassInfo, nsnull);
    if (objClassInfo.IsDOMClass())
    {
#ifdef DEBUG_CAPS_CanCreateWrapper
        printf("DOM class - GRANTED.\n");
#endif
        return NS_OK;
    }

    
    
    nsCOMPtr<nsISecurityCheckedComponent> checkedComponent =
        do_QueryInterface(aObj);

    nsXPIDLCString objectSecurityLevel;
    if (checkedComponent)
        checkedComponent->CanCreateWrapper((nsIID *)&aIID, getter_Copies(objectSecurityLevel));

    nsresult rv = CheckXPCPermissions(cx, aObj, nsnull, nsnull, objectSecurityLevel);
    if (NS_FAILED(rv))
    {
        
        NS_ConvertUTF8toUTF16 strName("CreateWrapperDenied");
        nsCAutoString origin;
        nsresult rv2;
        nsIPrincipal* subjectPrincipal = doGetSubjectPrincipal(&rv2);
        if (NS_SUCCEEDED(rv2) && subjectPrincipal) {
            GetPrincipalDomainOrigin(subjectPrincipal, origin);
        }
        NS_ConvertUTF8toUTF16 originUnicode(origin);
        NS_ConvertUTF8toUTF16 className(objClassInfo.GetName());
        const PRUnichar* formatStrings[] = {
            className.get(),
            originUnicode.get()
        };
        PRUint32 length = NS_ARRAY_LENGTH(formatStrings);
        if (originUnicode.IsEmpty()) {
            --length;
        } else {
            strName.AppendLiteral("ForOrigin");
        }
        nsXPIDLString errorMsg;
        
        
        
        rv2 = sStrBundle->FormatStringFromName(strName.get(),
                                               formatStrings,
                                               length,
                                               getter_Copies(errorMsg));
        NS_ENSURE_SUCCESS(rv2, rv2);

        SetPendingException(cx, errorMsg.get());

#ifdef DEBUG_CAPS_CanCreateWrapper
        printf("DENIED.\n");
    }
    else
    {
        printf("GRANTED.\n");
#endif
    }

    return rv;
}

NS_IMETHODIMP
nsScriptSecurityManager::CanCreateInstance(JSContext *cx,
                                           const nsCID &aCID)
{
#ifdef DEBUG_CAPS_CanCreateInstance
    char* cidStr = aCID.ToString();
    printf("### CanCreateInstance(%s) ", cidStr);
    NS_Free(cidStr);
#endif

    nsresult rv = CheckXPCPermissions(nsnull, nsnull, nsnull, nsnull, nsnull);
    if (NS_FAILED(rv))
    {
        
        nsCAutoString errorMsg("Permission denied to create instance of class. CID=");
        char cidStr[NSID_LENGTH];
        aCID.ToProvidedString(cidStr);
        errorMsg.Append(cidStr);
        SetPendingException(cx, errorMsg.get());

#ifdef DEBUG_CAPS_CanCreateInstance
        printf("DENIED\n");
    }
    else
    {
        printf("GRANTED\n");
#endif
    }
    return rv;
}

NS_IMETHODIMP
nsScriptSecurityManager::CanGetService(JSContext *cx,
                                       const nsCID &aCID)
{
#ifdef DEBUG_CAPS_CanGetService
    char* cidStr = aCID.ToString();
    printf("### CanGetService(%s) ", cidStr);
    NS_Free(cidStr);
#endif

    nsresult rv = CheckXPCPermissions(nsnull, nsnull, nsnull, nsnull, nsnull);
    if (NS_FAILED(rv))
    {
        
        nsCAutoString errorMsg("Permission denied to get service. CID=");
        char cidStr[NSID_LENGTH];
        aCID.ToProvidedString(cidStr);
        errorMsg.Append(cidStr);
        SetPendingException(cx, errorMsg.get());

#ifdef DEBUG_CAPS_CanGetService
        printf("DENIED\n");
    }
    else
    {
        printf("GRANTED\n");
#endif
    }

    return rv;
}


NS_IMETHODIMP
nsScriptSecurityManager::CanAccess(PRUint32 aAction,
                                   nsAXPCNativeCallContext* aCallContext,
                                   JSContext* cx,
                                   JSObject* aJSObject,
                                   nsISupports* aObj,
                                   nsIClassInfo* aClassInfo,
                                   jsid aPropertyName,
                                   void** aPolicy)
{
    return CheckPropertyAccessImpl(aAction, aCallContext, cx,
                                   aJSObject, aObj, nsnull, aClassInfo,
                                   nsnull, aPropertyName, aPolicy);
}

nsresult
nsScriptSecurityManager::CheckXPCPermissions(JSContext* cx,
                                             nsISupports* aObj, JSObject* aJSObject,
                                             nsIPrincipal* aSubjectPrincipal,
                                             const char* aObjectSecurityLevel)
{
    
    PRBool ok = PR_FALSE;
    if (NS_SUCCEEDED(IsCapabilityEnabled("UniversalXPConnect", &ok)) && ok)
        return NS_OK;

    
    if (aObjectSecurityLevel)
    {
        if (PL_strcasecmp(aObjectSecurityLevel, "allAccess") == 0)
            return NS_OK;
        if (cx && PL_strcasecmp(aObjectSecurityLevel, "sameOrigin") == 0)
        {
            nsresult rv;
            if (!aJSObject)
            {
                nsCOMPtr<nsIXPConnectWrappedJS> xpcwrappedjs =
                    do_QueryInterface(aObj);
                if (xpcwrappedjs)
                {
                    rv = xpcwrappedjs->GetJSObject(&aJSObject);
                    NS_ENSURE_SUCCESS(rv, rv);
                }
            }

            if (!aSubjectPrincipal)
            {
                
                aSubjectPrincipal = GetSubjectPrincipal(cx, &rv);
                NS_ENSURE_SUCCESS(rv, rv);
            }
            if (aSubjectPrincipal && aJSObject)
            {
                nsIPrincipal* objectPrincipal = doGetObjectPrincipal(aJSObject);

                
                
                if (objectPrincipal)
                {
                    PRBool subsumes;
                    rv = aSubjectPrincipal->Subsumes(objectPrincipal, &subsumes);
                    NS_ENSURE_SUCCESS(rv, rv);
                    if (subsumes)
                        return NS_OK;
                }
            }
        }
        else if (PL_strcasecmp(aObjectSecurityLevel, "noAccess") != 0)
        {
            PRBool canAccess = PR_FALSE;
            if (NS_SUCCEEDED(IsCapabilityEnabled(aObjectSecurityLevel, &canAccess)) &&
                canAccess)
                return NS_OK;
        }
    }

    
    return NS_ERROR_DOM_XPCONNECT_ACCESS_DENIED;
}




NS_IMETHODIMP
nsScriptSecurityManager::AsyncOnChannelRedirect(nsIChannel* oldChannel, 
                                                nsIChannel* newChannel,
                                                PRUint32 redirFlags,
                                                nsIAsyncVerifyRedirectCallback *cb)
{
    nsCOMPtr<nsIPrincipal> oldPrincipal;
    GetChannelPrincipal(oldChannel, getter_AddRefs(oldPrincipal));

    nsCOMPtr<nsIURI> newURI;
    newChannel->GetURI(getter_AddRefs(newURI));
    nsCOMPtr<nsIURI> newOriginalURI;
    newChannel->GetOriginalURI(getter_AddRefs(newOriginalURI));

    NS_ENSURE_STATE(oldPrincipal && newURI && newOriginalURI);

    const PRUint32 flags =
        nsIScriptSecurityManager::LOAD_IS_AUTOMATIC_DOCUMENT_REPLACEMENT |
        nsIScriptSecurityManager::DISALLOW_SCRIPT;
    nsresult rv = CheckLoadURIWithPrincipal(oldPrincipal, newURI, flags);
    if (NS_SUCCEEDED(rv) && newOriginalURI != newURI) {
        rv = CheckLoadURIWithPrincipal(oldPrincipal, newOriginalURI, flags);
    }

    if (NS_FAILED(rv))
        return rv;

    cb->OnRedirectVerifyCallback(NS_OK);
    return NS_OK;
}





const char sJSEnabledPrefName[] = "javascript.enabled";
const char sFileOriginPolicyPrefName[] =
    "security.fileuri.strict_origin_policy";
static const char sPrincipalPrefix[] = "capability.principal";
static const char sPolicyPrefix[] = "capability.policy.";

static const char* kObservedPrefs[] = {
  sJSEnabledPrefName,
  sFileOriginPolicyPrefName,
  sPolicyPrefix,
  sPrincipalPrefix,
  nsnull
};


NS_IMETHODIMP
nsScriptSecurityManager::Observe(nsISupports* aObject, const char* aTopic,
                                 const PRUnichar* aMessage)
{
    nsresult rv = NS_OK;
    NS_ConvertUTF16toUTF8 messageStr(aMessage);
    const char *message = messageStr.get();

    static const char jsPrefix[] = "javascript.";
    static const char securityPrefix[] = "security.";
    if ((PL_strncmp(message, jsPrefix, sizeof(jsPrefix)-1) == 0) ||
        (PL_strncmp(message, securityPrefix, sizeof(securityPrefix)-1) == 0) )
    {
        ScriptSecurityPrefChanged();
    }
    else if (PL_strncmp(message, sPolicyPrefix, sizeof(sPolicyPrefix)-1) == 0)
    {
        
        mPolicyPrefsChanged = PR_TRUE;
    }
    else if ((PL_strncmp(message, sPrincipalPrefix, sizeof(sPrincipalPrefix)-1) == 0) &&
             !mIsWritingPrefs)
    {
        static const char id[] = "id";
        char* lastDot = PL_strrchr(message, '.');
        
        if(PL_strlen(lastDot) >= sizeof(id))
        {
            PL_strcpy(lastDot + 1, id);
            const char** idPrefArray = (const char**)&message;
            rv = InitPrincipals(1, idPrefArray);
        }
    }
    return rv;
}




nsScriptSecurityManager::nsScriptSecurityManager(void)
    : mOriginToPolicyMap(nsnull),
      mDefaultPolicy(nsnull),
      mCapabilities(nsnull),
      mContextPrincipals(nsnull),
      mPrefInitialized(PR_FALSE),
      mIsJavaScriptEnabled(PR_FALSE),
      mIsWritingPrefs(PR_FALSE),
      mPolicyPrefsChanged(PR_TRUE)
{
    NS_ASSERTION(sizeof(PRWord) == sizeof(void*),
                 "PRWord and void* have different lengths on this platform. "
                 "This may cause a security failure with the SecurityLevel union.");
    mPrincipals.Init(31);
}


nsresult nsScriptSecurityManager::Init()
{
    nsXPConnect* xpconnect = nsXPConnect::GetXPConnect();
     if (!xpconnect)
        return NS_ERROR_FAILURE;

    NS_ADDREF(sXPConnect = xpconnect);
    NS_ADDREF(sJSContextStack = xpconnect);

    JSContext* cx = GetSafeJSContext();
    if (!cx) return NS_ERROR_FAILURE;   
    
    ::JS_BeginRequest(cx);
    if (sEnabledID == JSID_VOID)
        sEnabledID = INTERNED_STRING_TO_JSID(cx, ::JS_InternString(cx, "enabled"));
    ::JS_EndRequest(cx);

    InitPrefs();

    nsresult rv = CallGetService(NS_IOSERVICE_CONTRACTID, &sIOService);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStringBundleService> bundleService =
        mozilla::services::GetStringBundleService();
    if (!bundleService)
        return NS_ERROR_FAILURE;

    rv = bundleService->CreateBundle("chrome://global/locale/security/caps.properties", &sStrBundle);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsRefPtr<nsSystemPrincipal> system = new nsSystemPrincipal();
    NS_ENSURE_TRUE(system, NS_ERROR_OUT_OF_MEMORY);

    JSPrincipals *jsprin;
    rv = system->Init(&jsprin);
    NS_ENSURE_SUCCESS(rv, rv);

    mSystemPrincipal = system;

    
    
    nsCOMPtr<nsIJSRuntimeService> runtimeService =
        do_QueryInterface(sXPConnect, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = runtimeService->GetRuntime(&sRuntime);
    NS_ENSURE_SUCCESS(rv, rv);

    static JSSecurityCallbacks securityCallbacks = {
        CheckObjectAccess,
        NULL,
        NULL,
        ContentSecurityPolicyPermitsJSAction
    };

#ifdef DEBUG
    JSSecurityCallbacks *oldcallbacks =
#endif
    JS_SetRuntimeSecurityCallbacks(sRuntime, &securityCallbacks);
    NS_ASSERTION(!oldcallbacks, "Someone else set security callbacks!");

    JS_SetTrustedPrincipals(sRuntime, jsprin);

    return NS_OK;
}

static nsScriptSecurityManager *gScriptSecMan = nsnull;

jsid nsScriptSecurityManager::sEnabledID   = JSID_VOID;

nsScriptSecurityManager::~nsScriptSecurityManager(void)
{
    Preferences::RemoveObservers(this, kObservedPrefs);
    NS_ASSERTION(!mContextPrincipals, "Leaking mContextPrincipals");
    delete mOriginToPolicyMap;
    if(mDefaultPolicy)
        mDefaultPolicy->Drop();
    delete mCapabilities;
    gScriptSecMan = nsnull;
}

void
nsScriptSecurityManager::Shutdown()
{
    if (sRuntime) {
        JS_SetRuntimeSecurityCallbacks(sRuntime, NULL);
        JS_SetTrustedPrincipals(sRuntime, NULL);
        sRuntime = nsnull;
    }
    sEnabledID = JSID_VOID;

    NS_IF_RELEASE(sIOService);
    NS_IF_RELEASE(sXPConnect);
    NS_IF_RELEASE(sJSContextStack);
    NS_IF_RELEASE(sStrBundle);
}

nsScriptSecurityManager *
nsScriptSecurityManager::GetScriptSecurityManager()
{
    if (!gScriptSecMan)
    {
        nsScriptSecurityManager* ssManager = new nsScriptSecurityManager();
        if (!ssManager)
            return nsnull;
        nsresult rv;
        rv = ssManager->Init();
        NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to initialize nsScriptSecurityManager");
        if (NS_FAILED(rv)) {
            delete ssManager;
            return nsnull;
        }
 
        rv = nsJSPrincipals::Startup();
        if (NS_FAILED(rv)) {
            NS_WARNING("can't initialize JS engine security protocol glue!");
            delete ssManager;
            return nsnull;
        }
 
        rv = sXPConnect->SetDefaultSecurityManager(ssManager,
                                                   nsIXPCSecurityManager::HOOK_ALL);
        if (NS_FAILED(rv)) {
            NS_WARNING("Failed to install xpconnect security manager!");
            delete ssManager;
            return nsnull;
        }

        gScriptSecMan = ssManager;
    }
    return gScriptSecMan;
}




nsSystemPrincipal *
nsScriptSecurityManager::SystemPrincipalSingletonConstructor()
{
    nsIPrincipal *sysprin = nsnull;
    if (gScriptSecMan)
        NS_ADDREF(sysprin = gScriptSecMan->mSystemPrincipal);
    return static_cast<nsSystemPrincipal*>(sysprin);
}

nsresult
nsScriptSecurityManager::InitPolicies()
{
    
    NS_ENSURE_STATE(sXPConnect);
    nsresult rv = sXPConnect->ClearAllWrappedNativeSecurityPolicies();
    if (NS_FAILED(rv)) return rv;

    
    
    delete mOriginToPolicyMap;
    
    
    
    
    DomainPolicy::InvalidateAll();
    
    
    if(mDefaultPolicy) {
        mDefaultPolicy->Drop();
        mDefaultPolicy = nsnull;
    }
    
    
    mOriginToPolicyMap =
      new nsObjectHashtable(nsnull, nsnull, DeleteDomainEntry, nsnull);
    if (!mOriginToPolicyMap)
        return NS_ERROR_OUT_OF_MEMORY;

    
    mDefaultPolicy = new DomainPolicy();
    if (!mDefaultPolicy)
        return NS_ERROR_OUT_OF_MEMORY;

    mDefaultPolicy->Hold();
    if (!mDefaultPolicy->Init())
        return NS_ERROR_UNEXPECTED;

    
    if (!mCapabilities)
    {
        mCapabilities = 
          new nsObjectHashtable(nsnull, nsnull, DeleteCapability, nsnull);
        if (!mCapabilities)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    
    JSContext* cx = GetSafeJSContext();
    NS_ASSERTION(cx, "failed to get JS context");
    AutoCxPusher autoPusher(sJSContextStack, cx);
    rv = InitDomainPolicy(cx, "default", mDefaultPolicy);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAdoptingCString policyNames =
        Preferences::GetCString("capability.policy.policynames");

    nsAdoptingCString defaultPolicyNames =
        Preferences::GetCString("capability.policy.default_policynames");
    policyNames += NS_LITERAL_CSTRING(" ") + defaultPolicyNames;

    
    char* policyCurrent = policyNames.BeginWriting();
    PRBool morePolicies = PR_TRUE;
    while (morePolicies)
    {
        while(*policyCurrent == ' ' || *policyCurrent == ',')
            policyCurrent++;
        if (*policyCurrent == '\0')
            break;
        char* nameBegin = policyCurrent;

        while(*policyCurrent != '\0' && *policyCurrent != ' ' && *policyCurrent != ',')
            policyCurrent++;

        morePolicies = (*policyCurrent != '\0');
        *policyCurrent = '\0';
        policyCurrent++;

        nsCAutoString sitesPrefName(
            NS_LITERAL_CSTRING(sPolicyPrefix) +
            nsDependentCString(nameBegin) +
            NS_LITERAL_CSTRING(".sites"));
        nsAdoptingCString domainList =
            Preferences::GetCString(sitesPrefName.get());
        if (!domainList) {
            continue;
        }

        DomainPolicy* domainPolicy = new DomainPolicy();
        if (!domainPolicy)
            return NS_ERROR_OUT_OF_MEMORY;

        if (!domainPolicy->Init())
        {
            delete domainPolicy;
            return NS_ERROR_UNEXPECTED;
        }
        domainPolicy->Hold();
        
        char* domainStart = domainList.BeginWriting();
        char* domainCurrent = domainStart;
        char* lastDot = nsnull;
        char* nextToLastDot = nsnull;
        PRBool moreDomains = PR_TRUE;
        while (moreDomains)
        {
            if (*domainCurrent == ' ' || *domainCurrent == '\0')
            {
                moreDomains = (*domainCurrent != '\0');
                *domainCurrent = '\0';
                nsCStringKey key(nextToLastDot ? nextToLastDot+1 : domainStart);
                DomainEntry *newEntry = new DomainEntry(domainStart, domainPolicy);
                if (!newEntry)
                {
                    domainPolicy->Drop();
                    return NS_ERROR_OUT_OF_MEMORY;
                }
#ifdef DEBUG
                newEntry->mPolicyName_DEBUG = nameBegin;
#endif
                DomainEntry *existingEntry = (DomainEntry *)
                    mOriginToPolicyMap->Get(&key);
                if (!existingEntry)
                    mOriginToPolicyMap->Put(&key, newEntry);
                else
                {
                    if (existingEntry->Matches(domainStart))
                    {
                        newEntry->mNext = existingEntry;
                        mOriginToPolicyMap->Put(&key, newEntry);
                    }
                    else
                    {
                        while (existingEntry->mNext)
                        {
                            if (existingEntry->mNext->Matches(domainStart))
                            {
                                newEntry->mNext = existingEntry->mNext;
                                existingEntry->mNext = newEntry;
                                break;
                            }
                            existingEntry = existingEntry->mNext;
                        }
                        if (!existingEntry->mNext)
                            existingEntry->mNext = newEntry;
                    }
                }
                domainStart = domainCurrent + 1;
                lastDot = nextToLastDot = nsnull;
            }
            else if (*domainCurrent == '.')
            {
                nextToLastDot = lastDot;
                lastDot = domainCurrent;
            }
            domainCurrent++;
        }

        rv = InitDomainPolicy(cx, nameBegin, domainPolicy);
        domainPolicy->Drop();
        if (NS_FAILED(rv))
            return rv;
    }

    
    mPolicyPrefsChanged = PR_FALSE;

#ifdef DEBUG_CAPS_HACKER
    PrintPolicyDB();
#endif
    return NS_OK;
}


nsresult
nsScriptSecurityManager::InitDomainPolicy(JSContext* cx,
                                          const char* aPolicyName,
                                          DomainPolicy* aDomainPolicy)
{
    nsresult rv;
    nsCAutoString policyPrefix(NS_LITERAL_CSTRING(sPolicyPrefix) +
                               nsDependentCString(aPolicyName) +
                               NS_LITERAL_CSTRING("."));
    PRUint32 prefixLength = policyPrefix.Length() - 1; 

    PRUint32 prefCount;
    char** prefNames;
    nsIPrefBranch* branch = Preferences::GetRootBranch();
    NS_ASSERTION(branch, "failed to get the root pref branch");
    rv = branch->GetChildList(policyPrefix.get(), &prefCount, &prefNames);
    if (NS_FAILED(rv)) return rv;
    if (prefCount == 0)
        return NS_OK;

    
    PRUint32 currentPref = 0;
    for (; currentPref < prefCount; currentPref++)
    {
        
        const char* start = prefNames[currentPref] + prefixLength + 1;
        char* end = PL_strchr(start, '.');
        if (!end) 
            continue;
        static const char sitesStr[] = "sites";

        
        
        if (PL_strncmp(start, sitesStr, sizeof(sitesStr)-1) == 0)
            continue;

        
        nsAdoptingCString prefValue =
            Preferences::GetCString(prefNames[currentPref]);
        if (!prefValue) {
            continue;
        }

        SecurityLevel secLevel;
        if (PL_strcasecmp(prefValue, "noAccess") == 0)
            secLevel.level = SCRIPT_SECURITY_NO_ACCESS;
        else if (PL_strcasecmp(prefValue, "allAccess") == 0)
            secLevel.level = SCRIPT_SECURITY_ALL_ACCESS;
        else if (PL_strcasecmp(prefValue, "sameOrigin") == 0)
            secLevel.level = SCRIPT_SECURITY_SAME_ORIGIN_ACCESS;
        else 
        {  
            nsCStringKey secLevelKey(prefValue);
            secLevel.capability =
                reinterpret_cast<char*>(mCapabilities->Get(&secLevelKey));
            if (!secLevel.capability)
            {
                secLevel.capability = NS_strdup(prefValue);
                if (!secLevel.capability)
                    break;
                mCapabilities->Put(&secLevelKey, 
                                   secLevel.capability);
            }
        }

        *end = '\0';
        
        ClassPolicy* cpolicy = 
          static_cast<ClassPolicy*>
                     (PL_DHashTableOperate(aDomainPolicy, start,
                                              PL_DHASH_ADD));
        if (!cpolicy)
            break;

        
        
        if ((*start == '*') && (end == start + 1)) {
            aDomainPolicy->mWildcardPolicy = cpolicy;

            
            
            
            cpolicy->mDomainWeAreWildcardFor = aDomainPolicy;
        }

        
        start = end + 1;
        end = PL_strchr(start, '.');
        if (end)
            *end = '\0';

        JSAutoRequest ar(cx);

        JSString* propertyKey = ::JS_InternString(cx, start);
        if (!propertyKey)
            return NS_ERROR_OUT_OF_MEMORY;

        
        PropertyPolicy* ppolicy = 
          static_cast<PropertyPolicy*>
                     (PL_DHashTableOperate(cpolicy->mPolicy, propertyKey,
                                              PL_DHASH_ADD));
        if (!ppolicy)
            break;

        if (end) 
        {
            start = end + 1;
            if (PL_strcasecmp(start, "set") == 0)
                ppolicy->mSet = secLevel;
            else
                ppolicy->mGet = secLevel;
        }
        else
        {
            if (ppolicy->mGet.level == SCRIPT_SECURITY_UNDEFINED_ACCESS)
                ppolicy->mGet = secLevel;
            if (ppolicy->mSet.level == SCRIPT_SECURITY_UNDEFINED_ACCESS)
                ppolicy->mSet = secLevel;
        }
    }

    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(prefCount, prefNames);
    if (currentPref < prefCount) 
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}



nsresult
nsScriptSecurityManager::GetPrincipalPrefNames(const char* prefBase,
                                               nsCString& grantedPref,
                                               nsCString& deniedPref,
                                               nsCString& subjectNamePref)
{
    char* lastDot = PL_strrchr(prefBase, '.');
    if (!lastDot) return NS_ERROR_FAILURE;
    PRInt32 prefLen = lastDot - prefBase + 1;

    grantedPref.Assign(prefBase, prefLen);
    deniedPref.Assign(prefBase, prefLen);
    subjectNamePref.Assign(prefBase, prefLen);

#define GRANTED "granted"
#define DENIED "denied"
#define SUBJECTNAME "subjectName"

    grantedPref.AppendLiteral(GRANTED);
    if (grantedPref.Length() != prefLen + sizeof(GRANTED) - 1) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    deniedPref.AppendLiteral(DENIED);
    if (deniedPref.Length() != prefLen + sizeof(DENIED) - 1) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    subjectNamePref.AppendLiteral(SUBJECTNAME);
    if (subjectNamePref.Length() != prefLen + sizeof(SUBJECTNAME) - 1) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

#undef SUBJECTNAME
#undef DENIED
#undef GRANTED
    
    return NS_OK;
}

nsresult
nsScriptSecurityManager::InitPrincipals(PRUint32 aPrefCount, const char** aPrefNames)
{
    







    




    static const char idSuffix[] = ".id";
    for (PRUint32 c = 0; c < aPrefCount; c++)
    {
        PRInt32 prefNameLen = PL_strlen(aPrefNames[c]) - 
            (NS_ARRAY_LENGTH(idSuffix) - 1);
        if (PL_strcasecmp(aPrefNames[c] + prefNameLen, idSuffix) != 0)
            continue;

        nsAdoptingCString id = Preferences::GetCString(aPrefNames[c]);
        if (!id) {
            return NS_ERROR_FAILURE;
        }

        nsCAutoString grantedPrefName;
        nsCAutoString deniedPrefName;
        nsCAutoString subjectNamePrefName;
        nsresult rv = GetPrincipalPrefNames(aPrefNames[c],
                                            grantedPrefName,
                                            deniedPrefName,
                                            subjectNamePrefName);
        if (rv == NS_ERROR_OUT_OF_MEMORY)
            return rv;
        if (NS_FAILED(rv))
            continue;

        nsAdoptingCString grantedList =
            Preferences::GetCString(grantedPrefName.get());
        nsAdoptingCString deniedList =
            Preferences::GetCString(deniedPrefName.get());
        nsAdoptingCString subjectName =
            Preferences::GetCString(subjectNamePrefName.get());

        
        if (id.IsEmpty() || (grantedList.IsEmpty() && deniedList.IsEmpty()))
        {
            Preferences::ClearUser(aPrefNames[c]);
            Preferences::ClearUser(grantedPrefName.get());
            Preferences::ClearUser(deniedPrefName.get());
            Preferences::ClearUser(subjectNamePrefName.get());
            continue;
        }

        
        static const char certificateName[] = "capability.principal.certificate";
        static const char codebaseName[] = "capability.principal.codebase";
        static const char codebaseTrustedName[] = "capability.principal.codebaseTrusted";

        PRBool isCert = PR_FALSE;
        PRBool isTrusted = PR_FALSE;
        
        if (PL_strncmp(aPrefNames[c], certificateName,
                       sizeof(certificateName) - 1) == 0)
        {
            isCert = PR_TRUE;
        }
        else if (PL_strncmp(aPrefNames[c], codebaseName,
                            sizeof(codebaseName) - 1) == 0)
        {
            isTrusted = (PL_strncmp(aPrefNames[c], codebaseTrustedName,
                                    sizeof(codebaseTrustedName) - 1) == 0);
        }
        else
        {
          NS_ERROR("Not a codebase or a certificate?!");
        }

        nsRefPtr<nsPrincipal> newPrincipal = new nsPrincipal();
        if (!newPrincipal)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = newPrincipal->InitFromPersistent(aPrefNames[c], id, subjectName,
                                              EmptyCString(),
                                              grantedList, deniedList, nsnull, 
                                              isCert, isTrusted);
        if (NS_SUCCEEDED(rv))
            mPrincipals.Put(newPrincipal, newPrincipal);
    }
    return NS_OK;
}

inline void
nsScriptSecurityManager::ScriptSecurityPrefChanged()
{
    
    mIsJavaScriptEnabled = PR_TRUE;

    sStrictFileOriginPolicy = PR_TRUE;

    nsresult rv;
    if (!mPrefInitialized) {
        rv = InitPrefs();
        if (NS_FAILED(rv))
            return;
    }

    mIsJavaScriptEnabled =
        Preferences::GetBool(sJSEnabledPrefName, mIsJavaScriptEnabled);

    sStrictFileOriginPolicy =
        Preferences::GetBool(sFileOriginPolicyPrefName, PR_FALSE);
}

nsresult
nsScriptSecurityManager::InitPrefs()
{
    nsresult rv;
    nsIPrefBranch* branch = Preferences::GetRootBranch();
    NS_ENSURE_TRUE(branch, NS_ERROR_FAILURE);

    mPrefInitialized = PR_TRUE;

    
    ScriptSecurityPrefChanged();

    
    Preferences::AddStrongObservers(this, kObservedPrefs);

    PRUint32 prefCount;
    char** prefNames;
    
    rv = branch->GetChildList(sPrincipalPrefix, &prefCount, &prefNames);
    if (NS_SUCCEEDED(rv) && prefCount > 0)
    {
        rv = InitPrincipals(prefCount, (const char**)prefNames);
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(prefCount, prefNames);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}



#ifdef DEBUG_CAPS_HACKER




static PLDHashOperator
PrintPropertyPolicy(PLDHashTable *table, PLDHashEntryHdr *entry,
                    PRUint32 number, void *arg)
{
    PropertyPolicy* pp = (PropertyPolicy*)entry;
    nsCAutoString prop("        ");
    JSContext* cx = (JSContext*)arg;
    prop.AppendInt((PRUint32)pp->key);
    prop += ' ';
    prop.AppendWithConversion((PRUnichar*)JS_GetStringChars(pp->key));
    prop += ": Get=";
    if (SECURITY_ACCESS_LEVEL_FLAG(pp->mGet))
        prop.AppendInt(pp->mGet.level);
    else
        prop += pp->mGet.capability;

    prop += " Set=";
    if (SECURITY_ACCESS_LEVEL_FLAG(pp->mSet))
        prop.AppendInt(pp->mSet.level);
    else
        prop += pp->mSet.capability;
        
    printf("%s.\n", prop.get());
    return PL_DHASH_NEXT;
}

static PLDHashOperator
PrintClassPolicy(PLDHashTable *table, PLDHashEntryHdr *entry,
                 PRUint32 number, void *arg)
{
    ClassPolicy* cp = (ClassPolicy*)entry;
    printf("    %s\n", cp->key);

    PL_DHashTableEnumerate(cp->mPolicy, PrintPropertyPolicy, arg);
    return PL_DHASH_NEXT;
}



static PRBool
PrintDomainPolicy(nsHashKey *aKey, void *aData, void* aClosure)
{
    DomainEntry* de = (DomainEntry*)aData;
    printf("----------------------------\n");
    printf("Domain: %s Policy Name: %s.\n", de->mOrigin.get(),
           de->mPolicyName_DEBUG.get());
    PL_DHashTableEnumerate(de->mDomainPolicy, PrintClassPolicy, aClosure);
    return PR_TRUE;
}

static PRBool
PrintCapability(nsHashKey *aKey, void *aData, void* aClosure)
{
    char* cap = (char*)aData;
    printf("    %s.\n", cap);
    return PR_TRUE;
}

void
nsScriptSecurityManager::PrintPolicyDB()
{
    printf("############## Security Policies ###############\n");
    if(mOriginToPolicyMap)
    {
        JSContext* cx = GetCurrentJSContext();
        if (!cx)
            cx = GetSafeJSContext();
        printf("----------------------------\n");
        printf("Domain: Default.\n");
        PL_DHashTableEnumerate(mDefaultPolicy, PrintClassPolicy, (void*)cx);
        mOriginToPolicyMap->Enumerate(PrintDomainPolicy, (void*)cx);
    }
    printf("############ End Security Policies #############\n\n");
    printf("############## Capabilities ###############\n");
    mCapabilities->Enumerate(PrintCapability);
    printf("############## End Capabilities ###############\n");
}
#endif
