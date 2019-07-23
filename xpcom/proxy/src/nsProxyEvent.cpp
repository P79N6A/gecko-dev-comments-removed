



















































#include "nsProxyEventPrivate.h"
#include "nsProxyRelease.h"
#include "nsIProxyObjectManager.h"
#include "nsCRT.h"

#include "pratom.h"
#include "prmem.h"
#include "xptcall.h"

#include "nsAutoLock.h"
#include "nsXPCOMCID.h"
#include "nsServiceManagerUtils.h"
#include "nsIComponentManager.h"
#include "nsThreadUtils.h"
#include "nsEventQueue.h"
#include "nsMemory.h"






#define nsAUTF8String nsACString
#define nsUTF8String nsCString

class nsProxyCallCompletedEvent : public nsRunnable
{
public:
    nsProxyCallCompletedEvent(nsProxyObjectCallInfo *info)
        : mInfo(info)
    {}

    NS_DECL_NSIRUNNABLE

    NS_IMETHOD QueryInterface(REFNSIID aIID, void **aResult);

private:
    nsProxyObjectCallInfo *mInfo;
};

NS_IMETHODIMP
nsProxyCallCompletedEvent::Run()
{
    NS_ASSERTION(mInfo, "no info");
    mInfo->SetCompleted();
    return NS_OK;
}

NS_DEFINE_IID(kFilterIID, NS_PROXYEVENT_FILTER_IID);

NS_IMETHODIMP
nsProxyCallCompletedEvent::QueryInterface(REFNSIID aIID, void **aResult)
{
    
    
    
    
    if (aIID.Equals(kFilterIID)) {
        *aResult = mInfo;
        mInfo->AddRef();
        return NS_OK;
    }
    return nsRunnable::QueryInterface(aIID, aResult);
}



NS_IMETHODIMP
nsProxyObject::nsProxyObjectDestructorEvent::Run()
{
    delete mDoomed;
    return NS_OK;
}



nsProxyObjectCallInfo::nsProxyObjectCallInfo(nsProxyEventObject* owner,
                                             const XPTMethodDescriptor *methodInfo,
                                             PRUint32 methodIndex, 
                                             nsXPTCVariant* parameterList, 
                                             PRUint32 parameterCount) :
    mResult(NS_ERROR_FAILURE),
    mMethodInfo(methodInfo),
    mMethodIndex(methodIndex),
    mParameterList(parameterList),
    mParameterCount(parameterCount),
    mCompleted(0),
    mOwner(owner)
{
    NS_ASSERTION(owner, "No nsProxyObject!");
    NS_ASSERTION(methodInfo, "No nsXPTMethodInfo!");

    RefCountInInterfacePointers(PR_TRUE);
    if (mOwner->GetProxyType() & NS_PROXY_ASYNC)
        CopyStrings(PR_TRUE);
}

nsProxyObjectCallInfo::~nsProxyObjectCallInfo()
{
    RefCountInInterfacePointers(PR_FALSE);
    if (mOwner->GetProxyType() & NS_PROXY_ASYNC)
        CopyStrings(PR_FALSE);

    mOwner = nsnull;
    
    if (mParameterList)  
        free(mParameterList);
}

NS_IMETHODIMP
nsProxyObjectCallInfo::QueryInterface(REFNSIID aIID, void **aResult)
{
    if (aIID.Equals(kFilterIID)) {
        *aResult = this;
        AddRef();
        return NS_OK;
    }
    return nsRunnable::QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
nsProxyObjectCallInfo::Run()
{
    PROXY_LOG(("PROXY(%p): Run\n", this));

    mResult = NS_InvokeByIndex(mOwner->GetProxiedInterface(),
                               mMethodIndex,
                               mParameterCount,
                               mParameterList);

    if (IsSync()) {
        PostCompleted();
    }

    return NS_OK;
}

void
nsProxyObjectCallInfo::RefCountInInterfacePointers(PRBool addRef)
{
    for (PRUint32 i = 0; i < mParameterCount; i++)
    {
        nsXPTParamInfo paramInfo = mMethodInfo->params[i];

        if (paramInfo.GetType().IsInterfacePointer() )
        {
            nsISupports* anInterface = nsnull;

            if (paramInfo.IsIn())
            {
                anInterface = ((nsISupports*)mParameterList[i].val.p);
                
                if (anInterface)
                {
                    if (addRef)
                        anInterface->AddRef();
                    else
                        anInterface->Release();
            
                }
            }
        }
    }
}

void
nsProxyObjectCallInfo::CopyStrings(PRBool copy)
{
    for (PRUint32 i = 0; i < mParameterCount; i++)
    {
        const nsXPTParamInfo paramInfo = mMethodInfo->params[i];

        if (paramInfo.IsIn())
        {
            const nsXPTType& type = paramInfo.GetType();
            uint8 type_tag = type.TagPart();
            void *ptr = mParameterList[i].val.p;

            if (!ptr)
                continue;

            if (copy)
            {                
                switch (type_tag) 
                {
                    case nsXPTType::T_CHAR_STR:                                
                        mParameterList[i].val.p =
                            PL_strdup((const char *)ptr);
                        break;
                    case nsXPTType::T_WCHAR_STR:
                        mParameterList[i].val.p =
                            nsCRT::strdup((const PRUnichar *)ptr);
                        break;
                    case nsXPTType::T_DOMSTRING:
                    case nsXPTType::T_ASTRING:
                        mParameterList[i].val.p = 
                            new nsString(*((nsAString*) ptr));
                        break;
                    case nsXPTType::T_CSTRING:
                        mParameterList[i].val.p = 
                            new nsCString(*((nsACString*) ptr));
                        break;
                    case nsXPTType::T_UTF8STRING:                        
                        mParameterList[i].val.p = 
                            new nsUTF8String(*((nsAUTF8String*) ptr));
                        break;
                    default:
                        
                        break;                    
                }
            }
            else
            {
                switch (type_tag) 
                {
                    case nsXPTType::T_CHAR_STR:
                        PL_strfree((char*) ptr);
                        break;
                    case nsXPTType::T_WCHAR_STR:
                        nsCRT::free((PRUnichar*)ptr);
                        break;
                    case nsXPTType::T_DOMSTRING:
                    case nsXPTType::T_ASTRING:
                        delete (nsString*) ptr;
                        break;
                    case nsXPTType::T_CSTRING:
                        delete (nsCString*) ptr;
                        break;
                    case nsXPTType::T_UTF8STRING:
                        delete (nsUTF8String*) ptr;
                        break;
                    default:
                        
                        break;
                }
            }
        }
    }
}

PRBool                
nsProxyObjectCallInfo::GetCompleted()
{
    return (PRBool)mCompleted;
}

void
nsProxyObjectCallInfo::SetCompleted()
{
    PROXY_LOG(("PROXY(%p): SetCompleted\n", this));
    PR_AtomicSet(&mCompleted, 1);
}

void                
nsProxyObjectCallInfo::PostCompleted()
{
    PROXY_LOG(("PROXY(%p): PostCompleted\n", this));

    if (mCallersTarget) {
        nsCOMPtr<nsIRunnable> event =
                new nsProxyCallCompletedEvent(this);
        if (event &&
            NS_SUCCEEDED(mCallersTarget->Dispatch(event, NS_DISPATCH_NORMAL)))
            return;
    }

    
    NS_WARNING("Failed to dispatch nsProxyCallCompletedEvent");
    SetCompleted();
}
  
nsIEventTarget*      
nsProxyObjectCallInfo::GetCallersTarget() 
{ 
    return mCallersTarget;
}

void
nsProxyObjectCallInfo::SetCallersTarget(nsIEventTarget* target)
{
    mCallersTarget = target;
}   

nsProxyObject::nsProxyObject(nsIEventTarget *target, PRInt32 proxyType,
                             nsISupports *realObject) :
  mProxyType(proxyType),
  mTarget(target),
  mRealObject(realObject),
  mFirst(nsnull)
{
    MOZ_COUNT_CTOR(nsProxyObject);

    nsProxyObjectManager *pom = nsProxyObjectManager::GetInstance();
    NS_ASSERTION(pom, "Creating a proxy without a global proxy-object-manager.");
    pom->AddRef();
}

nsProxyObject::~nsProxyObject()
{
    
    
    nsISupports *doomed = nsnull;
    mRealObject.swap(doomed);
    NS_ProxyRelease(mTarget, doomed);

    MOZ_COUNT_DTOR(nsProxyObject);
}

NS_IMPL_THREADSAFE_ADDREF(nsProxyObject)

NS_IMETHODIMP_(nsrefcnt)
nsProxyObject::Release()
{
    nsrefcnt count;
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    count = PR_AtomicDecrement((PRInt32*) &mRefCnt);
    NS_LOG_RELEASE(this, count, "nsProxyObject");
    if (count)
        return count;

    nsProxyObjectManager *pom = nsProxyObjectManager::GetInstance();
    NS_ASSERTION(pom, "Deleting a proxy without a global proxy-object-manager.");

    pom->Remove(this);
    pom->Release();
    delete this;

    return 0;
}

NS_IMETHODIMP
nsProxyObject::QueryInterface(REFNSIID aIID, void **aResult)
{
    if (aIID.Equals(GetIID())) {
        *aResult = this;
        AddRef();
        return NS_OK;
    }

    if (aIID.Equals(NS_GET_IID(nsISupports))) {
        *aResult = NS_STATIC_CAST(nsISupports*, this);
        AddRef();
        return NS_OK;
    }

    nsProxyObjectManager *pom = nsProxyObjectManager::GetInstance();
    NS_ASSERTION(pom, "Deleting a proxy without a global proxy-object-manager.");

    nsAutoMonitor mon(pom->GetMonitor());

    return LockedFind(aIID, aResult);
}

nsresult
nsProxyObject::LockedFind(REFNSIID aIID, void **aResult)
{
    
    

    nsProxyEventObject *peo;

    for (peo = mFirst; peo; peo = peo->mNext) {
        if (peo->GetClass()->GetProxiedIID().Equals(aIID)) {
            *aResult = NS_STATIC_CAST(nsISupports*, peo->mXPTCStub);
            peo->AddRef();
            return NS_OK;
        }
    }

    nsProxyEventClass *pec;
    nsresult rv = nsProxyObjectManager::GetInstance()->GetClass(aIID, &pec);
    if (NS_FAILED(rv))
        return rv;

    nsISomeInterface* newInterface;
    rv = mRealObject->QueryInterface(aIID, (void**) &newInterface);
    if (NS_FAILED(rv))
        return rv;

    peo = new nsProxyEventObject(this, pec, 
                already_AddRefed<nsISomeInterface>(newInterface), &rv);
    if (!peo) {
        NS_RELEASE(newInterface);
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (NS_FAILED(rv)) {
        delete peo;
        return rv;
    }

    peo->mNext = mFirst;
    mFirst = peo;

    NS_ADDREF(peo);

    *aResult = NS_STATIC_CAST(nsISupports*, peo->mXPTCStub);
    return NS_OK;
}

void
nsProxyObject::LockedRemove(nsProxyEventObject *peo)
{
    nsProxyEventObject **i;
    for (i = &mFirst; *i; i = &((*i)->mNext)) {
        if (*i == peo) {
            *i = peo->mNext;
            break;
        }
    }
}

