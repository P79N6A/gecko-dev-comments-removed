





































#ifndef nsProxyEventPrivate_h__
#define nsProxyEventPrivate_h__

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIEventTarget.h"
#include "nsIInterfaceInfo.h"
#include "nsIProxyObjectManager.h"

#include "nsXPTCUtils.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"

#include "nsClassHashtable.h"
#include "nsHashtable.h"

#include "prmon.h"
#include "prlog.h"

class nsProxyEventObject;






typedef nsISupports nsISomeInterface;

#define NS_PROXYOBJECT_CLASS_IID \
{ 0xeea90d45, 0xb059, 0x11d2,                       \
  { 0x91, 0x5e, 0xc1, 0x2b, 0x69, 0x6c, 0x93, 0x33 } }




#define NS_PROXYEVENT_FILTER_IID \
{ 0xec373590, 0x9164, 0x11d3,    \
{0x8c, 0x73, 0x00, 0x00, 0x64, 0x65, 0x73, 0x74} }





class nsProxyEventClass
{
public:
    nsIInterfaceInfo*        GetInterfaceInfo() const {return mInfo;}
    const nsIID&             GetProxiedIID()    const {return mIID; }

    nsProxyEventClass(REFNSIID aIID, nsIInterfaceInfo* aInfo);
    ~nsProxyEventClass();

    nsIID                      mIID;
    nsCOMPtr<nsIInterfaceInfo> mInfo;
    uint32*                    mDescriptors;
};






class nsProxyObject : public nsISupports
{
public:
    NS_DECL_ISUPPORTS

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_PROXYOBJECT_CLASS_IID)

    nsProxyObject(nsIEventTarget *destQueue, PRInt32 proxyType,
                  nsISupports *realObject);
 
    nsISupports*        GetRealObject() const { return mRealObject; }
    nsIEventTarget*     GetTarget() const { return mTarget; }
    PRInt32             GetProxyType() const { return mProxyType; }

    nsresult LockedFind(REFNSIID iid, void **aResult);
    void LockedRemove(nsProxyEventObject* aObject);

    friend class nsProxyObjectManager;
private:
    ~nsProxyObject();

    PRInt32                   mProxyType;
    nsCOMPtr<nsIEventTarget>  mTarget;           
    nsCOMPtr<nsISupports>     mRealObject;       

    nsProxyEventObject       *mFirst;

    class nsProxyObjectDestructorEvent : public nsRunnable
    {
        nsProxyObjectDestructorEvent(nsProxyObject *doomed) :
            mDoomed(doomed)
        {}

        NS_DECL_NSIRUNNABLE

        friend class nsProxyObject;
    private:
        nsProxyObject *mDoomed;
    };

    friend class nsProxyObjectDestructorEvent;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsProxyObject, NS_PROXYOBJECT_CLASS_IID)






class nsProxyEventObject : protected nsAutoXPTCStub
{
public:

    NS_DECL_ISUPPORTS

    
    NS_IMETHOD CallMethod(PRUint16 methodIndex,
                          const XPTMethodDescriptor* info,
                          nsXPTCMiniVariant* params);

    nsProxyEventClass*    GetClass()            const { return mClass; }
    nsISomeInterface*     GetProxiedInterface() const { return mRealInterface; }
    nsIEventTarget*       GetTarget()           const { return mProxyObject->GetTarget(); }
    PRInt32               GetProxyType()        const { return mProxyObject->GetProxyType(); } 

    nsresult convertMiniVariantToVariant(const XPTMethodDescriptor *methodInfo,
                                         nsXPTCMiniVariant *params,
                                         nsXPTCVariant **fullParam,
                                         uint8 *outParamCount);

    nsProxyEventObject(nsProxyObject *aParent,
                       nsProxyEventClass *aClass,
                       already_AddRefed<nsISomeInterface> aRealInterface,
                       nsresult *rv);

    friend class nsProxyObject;

private:
    ~nsProxyEventObject();

    
    nsProxyEventClass          *mClass;
    nsCOMPtr<nsProxyObject>     mProxyObject;
    nsCOMPtr<nsISomeInterface>  mRealInterface;

    
    nsProxyEventObject         *mNext;
};

#define NS_PROXYEVENT_IID                             \
{ /* 9a24dc5e-2b42-4a5a-aeca-37b8c8fd8ccd */          \
    0x9a24dc5e,                                       \
    0x2b42,                                           \
    0x4a5a,                                           \
    {0xae, 0xca, 0x37, 0xb8, 0xc8, 0xfd, 0x8c, 0xcd}  \
}




class nsProxyObjectCallInfo : public nsRunnable
{
public:

    NS_DECL_NSIRUNNABLE

    NS_IMETHOD QueryInterface(REFNSIID aIID, void **aResult);

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_PROXYEVENT_IID)
    
    nsProxyObjectCallInfo(nsProxyEventObject* owner,
                          const XPTMethodDescriptor *methodInfo,
                          PRUint32 methodIndex, 
                          nsXPTCVariant* parameterList, 
                          PRUint32 parameterCount);

    ~nsProxyObjectCallInfo();

    PRUint32            GetMethodIndex() const { return mMethodIndex; }
    nsXPTCVariant*      GetParameterList() const { return mParameterList; }
    PRUint32            GetParameterCount() const { return mParameterCount; }
    nsresult            GetResult() const { return mResult; }
    
    PRBool              GetCompleted();
    void                SetCompleted();
    void                PostCompleted();

    void                SetResult(nsresult rv) { mResult = rv; }
    
    nsIEventTarget*     GetCallersTarget();
    void                SetCallersTarget(nsIEventTarget* target);
    PRBool              IsSync() const
    {
        return mOwner->GetProxyType() & NS_PROXY_SYNC;
    }

private:
    
    nsresult         mResult;                    
    const XPTMethodDescriptor *mMethodInfo;
    PRUint32         mMethodIndex;               
    nsXPTCVariant   *mParameterList;             
    PRUint32         mParameterCount;            
    PRInt32          mCompleted;                 
       
    nsCOMPtr<nsIEventTarget> mCallersTarget;     


    nsRefPtr<nsProxyEventObject>   mOwner;       
   
    void RefCountInInterfacePointers(PRBool addRef);
    void CopyStrings(PRBool copy);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsProxyObjectCallInfo, NS_PROXYEVENT_IID)





class nsProxyObjectManager: public nsIProxyObjectManager
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROXYOBJECTMANAGER
        
    static NS_METHOD Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);
    
    nsProxyObjectManager();
    
    static nsProxyObjectManager *GetInstance();
    static PRBool IsManagerShutdown();

    static void Shutdown();

    nsresult GetClass(REFNSIID aIID, nsProxyEventClass **aResult);

    void Remove(nsProxyObject* aProxy);

    PRMonitor*   GetMonitor() const { return mProxyCreationMonitor; }

#ifdef PR_LOGGING
    static PRLogModuleInfo *sLog;
#endif

private:
    ~nsProxyObjectManager();

    static nsProxyObjectManager* mInstance;
    nsHashtable  mProxyObjectMap;
    nsClassHashtable<nsIDHashKey, nsProxyEventClass> mProxyClassMap;
    PRMonitor   *mProxyCreationMonitor;
};

#define NS_XPCOMPROXY_CLASSNAME "nsProxyObjectManager"
#define NS_PROXYEVENT_MANAGER_CID                 \
{ 0xeea90d41,                                     \
  0xb059,                                         \
  0x11d2,                                         \
 {0x91, 0x5e, 0xc1, 0x2b, 0x69, 0x6c, 0x93, 0x33} \
}

#define PROXY_LOG(args) PR_LOG(nsProxyObjectManager::sLog, PR_LOG_DEBUG, args)

#endif  
