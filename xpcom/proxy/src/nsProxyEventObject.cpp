






































#include "prprf.h"
#include "prmem.h"

#include "nscore.h"
#include "nsProxyEventPrivate.h"
#include "nsIThreadInternal.h"

#include "nsServiceManagerUtils.h"

#include "nsHashtable.h"

#include "nsIInterfaceInfoManager.h"
#include "xptcall.h"

#include "nsAutoLock.h"

nsProxyEventObject::nsProxyEventObject(nsProxyObject *aParent,
                            nsProxyEventClass* aClass,
                            already_AddRefed<nsISomeInterface> aRealInterface,
                            nsresult *rv)
    : mRealInterface(aRealInterface),
      mClass(aClass),
      mProxyObject(aParent),
      mNext(nsnull)
{
    *rv = InitStub(aClass->GetProxiedIID());
}

nsProxyEventObject::~nsProxyEventObject()
{
    
    

    mProxyObject->LockedRemove(this);
}





NS_IMPL_THREADSAFE_ADDREF(nsProxyEventObject)

NS_IMETHODIMP_(nsrefcnt)
nsProxyEventObject::Release(void)
{
    nsAutoMonitor mon(nsProxyObjectManager::GetInstance()->GetMonitor());

    nsrefcnt count;
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    
    
    count = PR_AtomicDecrement((PRInt32 *)&mRefCnt);
    NS_LOG_RELEASE(this, count, "nsProxyEventObject");
    if (0 == count) {
        mRefCnt = 1; 
        
        
        
        
        
        NS_DELETEXPCOM(this);
        return 0;
    }
    return count;
}

NS_IMETHODIMP
nsProxyEventObject::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if( aIID.Equals(GetClass()->GetProxiedIID()) )
    {
        *aInstancePtr = NS_STATIC_CAST(nsISupports*, mXPTCStub);
        NS_ADDREF_THIS();
        return NS_OK;
    }
        
    return mProxyObject->QueryInterface(aIID, aInstancePtr);
}





nsresult
nsProxyEventObject::convertMiniVariantToVariant(const XPTMethodDescriptor *methodInfo, 
                                                nsXPTCMiniVariant * params, 
                                                nsXPTCVariant **fullParam, 
                                                uint8 *outParamCount)
{
    uint8 paramCount = methodInfo->num_args;
    *outParamCount = paramCount;
    *fullParam = nsnull;

    if (!paramCount) return NS_OK;
        
    *fullParam = (nsXPTCVariant*)malloc(sizeof(nsXPTCVariant) * paramCount);
    
    if (*fullParam == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    
    for (int i = 0; i < paramCount; i++)
    {
        const nsXPTParamInfo& paramInfo = methodInfo->params[i];
        if ((GetProxyType() & NS_PROXY_ASYNC) && paramInfo.IsDipper())
        {
            NS_WARNING("Async proxying of out parameters is not supported"); 
            free(*fullParam);
            return NS_ERROR_PROXY_INVALID_OUT_PARAMETER;
        }
        uint8 flags = paramInfo.IsOut() ? nsXPTCVariant::PTR_IS_DATA : 0;
        (*fullParam)[i].Init(params[i], paramInfo.GetType(), flags);
    }
    
    return NS_OK;
}

class nsProxyThreadFilter : public nsIThreadEventFilter
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSITHREADEVENTFILTER
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsProxyThreadFilter, nsIThreadEventFilter)

NS_DEFINE_IID(kFilterIID, NS_PROXYEVENT_FILTER_IID);

NS_IMETHODIMP_(PRBool)
nsProxyThreadFilter::AcceptEvent(nsIRunnable *event)
{
    PROXY_LOG(("PROXY(%p): filter event [%p]\n", this, event));

    
    
    
    
    
    nsRefPtr<nsProxyObjectCallInfo> poci;
    event->QueryInterface(kFilterIID, getter_AddRefs(poci));
    return poci && poci->IsSync();
}

NS_IMETHODIMP
nsProxyEventObject::CallMethod(PRUint16 methodIndex,
                               const XPTMethodDescriptor* methodInfo,
                               nsXPTCMiniVariant * params)
{
    NS_ASSERTION(methodIndex > 2,
                 "Calling QI/AddRef/Release through CallMethod");
    nsresult rv;

    if (XPT_MD_IS_NOTXPCOM(methodInfo->flags))
        return NS_ERROR_PROXY_INVALID_IN_PARAMETER;

    nsXPTCVariant *fullParam;
    uint8 paramCount;
    rv = convertMiniVariantToVariant(methodInfo, params,
                                     &fullParam, &paramCount);
    if (NS_FAILED(rv))
        return rv;

    PRBool callDirectly = PR_FALSE;
    if (GetProxyType() & NS_PROXY_SYNC &&
        NS_SUCCEEDED(GetTarget()->IsOnCurrentThread(&callDirectly)) &&
        callDirectly) {

        
        rv = NS_InvokeByIndex(mRealInterface, methodIndex,
                              paramCount, fullParam);

        if (fullParam)
            free(fullParam);

        return rv;
    }

    nsRefPtr<nsProxyObjectCallInfo> proxyInfo =
        new nsProxyObjectCallInfo(this, methodInfo, methodIndex,
                                  fullParam, paramCount);
    if (!proxyInfo)
        return NS_ERROR_OUT_OF_MEMORY;

    if (! (GetProxyType() & NS_PROXY_SYNC)) {
        return GetTarget()->Dispatch(proxyInfo, NS_DISPATCH_NORMAL);
    }

    

    nsIThread *thread = NS_GetCurrentThread();
    nsCOMPtr<nsIThreadInternal> threadInt = do_QueryInterface(thread);
    NS_ENSURE_STATE(threadInt);

    
    
    nsRefPtr<nsProxyThreadFilter> filter = new nsProxyThreadFilter();
    if (!filter)
        return NS_ERROR_OUT_OF_MEMORY;
    threadInt->PushEventQueue(filter);

    proxyInfo->SetCallersTarget(thread);
    
    
    rv = GetTarget()->Dispatch(proxyInfo, NS_DISPATCH_NORMAL);
    if (NS_SUCCEEDED(rv)) {
        while (!proxyInfo->GetCompleted()) {
            if (!NS_ProcessNextEvent(thread)) {
                rv = NS_ERROR_UNEXPECTED;
                break;
            }
        }
        rv = proxyInfo->GetResult();
    } else {
        NS_WARNING("Failed to dispatch nsProxyCallEvent");
    }

    threadInt->PopEventQueue();

    PROXY_LOG(("PROXY(%p): PostAndWait exit [%p %x]\n", this, proxyInfo.get(), rv));
    return rv;
}
