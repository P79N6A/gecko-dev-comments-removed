















































#include "nsProxyEventPrivate.h"

#include "nsIComponentManager.h"
#include "nsIProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsIThread.h"

#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "xptiprivate.h"

using namespace mozilla;

#ifdef PR_LOGGING
PRLogModuleInfo *nsProxyObjectManager::sLog = PR_NewLogModule("xpcomproxy");
#endif

class nsProxyEventKey : public nsHashKey
{
public:
    nsProxyEventKey(void* rootObjectKey, void* targetKey, PRInt32 proxyType)
        : mRootObjectKey(rootObjectKey), mTargetKey(targetKey), mProxyType(proxyType) {
    }
  
    PRUint32 HashCode(void) const {
        return NS_PTR_TO_INT32(mRootObjectKey) ^ 
            NS_PTR_TO_INT32(mTargetKey) ^ mProxyType;
    }

    bool Equals(const nsHashKey *aKey) const {
        const nsProxyEventKey* other = (const nsProxyEventKey*)aKey;
        return mRootObjectKey == other->mRootObjectKey
            && mTargetKey == other->mTargetKey
            && mProxyType == other->mProxyType;
    }

    nsHashKey *Clone() const {
        return new nsProxyEventKey(mRootObjectKey, mTargetKey, mProxyType);
    }

protected:
    void*       mRootObjectKey;
    void*       mTargetKey;
    PRInt32     mProxyType;
};





nsProxyObjectManager* nsProxyObjectManager::gInstance = nsnull;

NS_IMPL_QUERY_INTERFACE1(nsProxyObjectManager, nsIProxyObjectManager)

NS_IMETHODIMP_(nsrefcnt)
nsProxyObjectManager::AddRef()
{
    return 2;
}

NS_IMETHODIMP_(nsrefcnt)
nsProxyObjectManager::Release()
{
    return 1;
}

nsProxyObjectManager::nsProxyObjectManager()
    : mProxyObjectMap(256, false)
    , mProxyCreationLock("nsProxyObjectManager.mProxyCreationLock")
{
    mProxyClassMap.Init(256);
}

nsProxyObjectManager::~nsProxyObjectManager()
{
    mProxyClassMap.Clear();

    nsProxyObjectManager::gInstance = nsnull;
}

bool
nsProxyObjectManager::IsManagerShutdown()
{
    return gInstance == nsnull;
}

nsProxyObjectManager *
nsProxyObjectManager::GetInstance()
{
    if (!gInstance) 
        gInstance = new nsProxyObjectManager();
    return gInstance;
}

void
nsProxyObjectManager::Shutdown()
{
    delete gInstance;
    NS_ASSERTION(!gInstance, "Destructor didn't null gInstance?");
}

nsresult
nsProxyObjectManager::Create(nsISupports* outer, const nsIID& aIID,
                             void* *aInstancePtr)
{
    nsProxyObjectManager *proxyObjectManager = GetInstance();
    if (!proxyObjectManager)
        return NS_ERROR_OUT_OF_MEMORY;

    return proxyObjectManager->QueryInterface(aIID, aInstancePtr);
}

class nsProxyLockedRefPtr
{
public:
    nsProxyLockedRefPtr(nsProxyObject* aPtr) :
        mProxyObject(aPtr)
    {
        if (mProxyObject)
            mProxyObject->LockedAddRef();
    }

    ~nsProxyLockedRefPtr()
    {
        if (mProxyObject)
            mProxyObject->LockedRelease();
    }

    operator nsProxyObject*() const
    {
        return mProxyObject;
    }

    nsProxyObject* operator->() const
    {
        return mProxyObject;
    }

private:
    nsProxyObject *mProxyObject;
};

NS_IMETHODIMP 
nsProxyObjectManager::GetProxyForObject(nsIEventTarget* aTarget, 
                                        REFNSIID aIID, 
                                        nsISupports* aObj, 
                                        PRInt32 proxyType, 
                                        void** aProxyObject)
{
    NS_ENSURE_ARG_POINTER(aObj);

    *aProxyObject = nsnull;

    
    nsCOMPtr<nsIThread> thread;
    if (aTarget == NS_PROXY_TO_CURRENT_THREAD) {
      aTarget = NS_GetCurrentThread();
    } else if (aTarget == NS_PROXY_TO_MAIN_THREAD) {
      thread = do_GetMainThread();
      aTarget = thread.get();
    }

    
    
    
    if (!(proxyType & NS_PROXY_ASYNC) && !(proxyType & NS_PROXY_ALWAYS))
    {
        bool result;
        aTarget->IsOnCurrentThread(&result);
     
        if (result)
            return aObj->QueryInterface(aIID, aProxyObject);
    }
    
    nsCOMPtr<nsISupports> realObj = do_QueryInterface(aObj);

    
    
    nsCOMPtr<nsProxyObject> po = do_QueryInterface(aObj);
    if (po) {
        realObj = po->GetRealObject();
    }

    nsCOMPtr<nsISupports> realEQ = do_QueryInterface(aTarget);

    nsProxyEventKey rootKey(realObj, realEQ, proxyType);

    {
        MutexAutoLock lock(mProxyCreationLock);
        nsProxyLockedRefPtr root =
            (nsProxyObject*) mProxyObjectMap.Get(&rootKey);
        if (root)
            return root->LockedFind(aIID, aProxyObject);
    }

    
    nsProxyObject *newRoot = new nsProxyObject(aTarget, proxyType, realObj);
    if (!newRoot)
        return NS_ERROR_OUT_OF_MEMORY;

    
    {
        MutexAutoLock lock(mProxyCreationLock);
        nsProxyLockedRefPtr root = 
            (nsProxyObject*) mProxyObjectMap.Get(&rootKey);
        if (root) {
            delete newRoot;
            return root->LockedFind(aIID, aProxyObject);
        }

        mProxyObjectMap.Put(&rootKey, newRoot);

        nsProxyLockedRefPtr kungFuDeathGrip(newRoot);
        return newRoot->LockedFind(aIID, aProxyObject);
    }
}

void
nsProxyObjectManager::LockedRemove(nsProxyObject *aProxy)
{
    nsCOMPtr<nsISupports> realEQ = do_QueryInterface(aProxy->GetTarget());

    nsProxyEventKey rootKey(aProxy->GetRealObject(), realEQ, aProxy->GetProxyType());

    if (!mProxyObjectMap.Remove(&rootKey)) {
        NS_ERROR("nsProxyObject not found in global hash.");
    }
}

nsresult
nsProxyObjectManager::GetClass(REFNSIID aIID, nsProxyEventClass **aResult)
{
    {
        MutexAutoLock lock(mProxyCreationLock);
        if (mProxyClassMap.Get(aIID, aResult)) {
            NS_ASSERTION(*aResult, "Null data in mProxyClassMap");
            return NS_OK;
        }
    }

    nsIInterfaceInfoManager *iim =
        xptiInterfaceInfoManager::GetSingleton();
    if (!iim)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIInterfaceInfo> ii;
    nsresult rv = iim->GetInfoForIID(&aIID, getter_AddRefs(ii));
    if (NS_FAILED(rv))
        return rv;

    nsProxyEventClass *pec = new nsProxyEventClass(aIID, ii);
    if (!pec)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    MutexAutoLock lock(mProxyCreationLock);

    if (mProxyClassMap.Get(aIID, aResult)) {
        NS_ASSERTION(*aResult, "Null data in mProxyClassMap");
        delete pec;
        return NS_OK;
    }

    if (!mProxyClassMap.Put(aIID, pec)) {
        delete pec;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    *aResult = pec;
    return NS_OK;
}








nsresult
NS_GetProxyForObject(nsIEventTarget *target, 
                     REFNSIID aIID, 
                     nsISupports* aObj, 
                     PRInt32 proxyType, 
                     void** aProxyObject) 
{
    static NS_DEFINE_CID(proxyObjMgrCID, NS_PROXYEVENT_MANAGER_CID);

    nsresult rv;

    
    
    nsCOMPtr<nsIProxyObjectManager> proxyObjMgr = 
        do_GetService(proxyObjMgrCID, &rv);
    if (NS_FAILED(rv))
        return rv;
    
    
    
    return proxyObjMgr->GetProxyForObject(target, aIID, aObj,
                                          proxyType, aProxyObject);
}
