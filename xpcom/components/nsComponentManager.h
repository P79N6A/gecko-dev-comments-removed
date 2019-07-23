




































#ifndef nsComponentManager_h__
#define nsComponentManager_h__

#include "nsXPCOM.h"

#include "xpcom-private.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIComponentManagerObsolete.h"
#include "nsCategoryManager.h"
#include "nsIServiceManager.h"
#include "nsILocalFile.h"
#include "nsIModule.h"
#include "nsIModuleLoader.h"
#include "nsStaticComponentLoader.h"
#include "nsNativeComponentLoader.h"
#include "nsIFactory.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "pldhash.h"
#include "prtime.h"
#include "prmon.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsIFile.h"
#include "plarena.h"
#include "nsCOMArray.h"
#include "nsDataHashtable.h"
#include "nsTArray.h"

struct nsFactoryEntry;
class nsIServiceManager;
struct PRThread;

#define NS_COMPONENTMANAGER_CID                      \
{ /* 91775d60-d5dc-11d2-92fb-00e09805570f */         \
    0x91775d60,                                      \
    0xd5dc,                                          \
    0x11d2,                                          \
    {0x92, 0xfb, 0x00, 0xe0, 0x98, 0x05, 0x57, 0x0f} \
}


extern const char xpcomKeyName[];
extern const char xpcomComponentsKeyName[];
extern const char lastModValueName[];
extern const char fileSizeValueName[];
extern const char nativeComponentType[];
extern const char staticComponentType[];

typedef int LoaderType;


#define NS_LOADER_TYPE_NATIVE  -1
#define NS_LOADER_TYPE_STATIC  -2
#define NS_LOADER_TYPE_INVALID -3

#ifdef DEBUG
#define XPCOM_CHECK_PENDING_CIDS
#endif



struct nsLoaderdata {
    nsCOMPtr<nsIModuleLoader> loader;
    nsCString                 type;
};



struct DeferredModule
{
    DeferredModule() :
        type(nsnull), modTime(0) { }

    const char             *type;
    nsCOMPtr<nsILocalFile>  file;
    nsCString               location;
    nsCOMPtr<nsIModule>     module;
    PRInt64                 modTime;
};

class nsComponentManagerImpl
    : public nsIComponentManager,
      public nsIServiceManager,
      public nsIComponentRegistrar,
      public nsSupportsWeakReference,
      public nsIInterfaceRequestor,
      public nsIServiceManagerObsolete,
      public nsIComponentManagerObsolete
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR
    
    
    
    
    
    
    NS_IMETHOD GetClassObjectByContractID(const char *aContractID,
                                          const nsIID &aIID,
                                          void **_retval);


    NS_DECL_NSICOMPONENTMANAGEROBSOLETE

    
    
    
    NS_IMETHOD AutoRegister(nsIFile *aSpec); 
    NS_IMETHOD AutoUnregister(nsIFile *aSpec); 
    NS_IMETHOD RegisterFactory(const nsCID & aClass, const char *aClassName, const char *aContractID, nsIFactory *aFactory); 
    
    NS_IMETHOD RegisterFactoryLocation(const nsCID & aClass, const char *aClassName, const char *aContractID, nsIFile *aFile, const char *loaderStr, const char *aType); 
    NS_IMETHOD UnregisterFactoryLocation(const nsCID & aClass, nsIFile *aFile); 
    NS_IMETHOD IsCIDRegistered(const nsCID & aClass, PRBool *_retval); 
    NS_IMETHOD IsContractIDRegistered(const char *aClass, PRBool *_retval); 
    NS_IMETHOD EnumerateCIDs(nsISimpleEnumerator **_retval); 
    NS_IMETHOD EnumerateContractIDs(nsISimpleEnumerator **_retval); 
    NS_IMETHOD CIDToContractID(const nsCID & aClass, char **_retval); 
    NS_IMETHOD ContractIDToCID(const char *aContractID, nsCID * *_retval); 
    
    nsresult RegistryLocationForFile(nsIFile* aFile,
                                     nsCString& aResult);
    nsresult FileForRegistryLocation(const nsCString &aLocation,
                                     nsILocalFile **aSpec);

    NS_DECL_NSISERVICEMANAGER
    NS_DECL_NSISERVICEMANAGEROBSOLETE

    
    nsComponentManagerImpl();

    static nsComponentManagerImpl* gComponentManager;
    nsresult Init(nsStaticModuleInfo const *aStaticModules,
                  PRUint32 aStaticModuleCount);
    
    

    nsresult WritePersistentRegistry();
    nsresult ReadPersistentRegistry();

    nsresult Shutdown(void);

    nsresult FreeServices();

    nsresult
    NS_GetService(const char *aContractID, const nsIID& aIID, PRBool aDontCreate, nsISupports** result);

    nsresult RegisterComponentCommon(const nsCID &aClass,
                                     const char *aClassName,
                                     const char *aContractID,
                                     PRUint32 aContractIDLen,
                                     const char *aRegistryName,
                                     PRUint32 aRegistryNameLen,
                                     PRBool aReplace, PRBool aPersist,
                                     const char *aType);
    
    
    
    LoaderType GetLoaderType(const char *typeStr);
    LoaderType AddLoaderType(const char *typeStr);
    const char* StringForLoaderType(LoaderType aType) {
        return mLoaderData[aType].type.get();
    }
    nsIModuleLoader* LoaderForType(LoaderType aType);
    int GetLoaderCount() { return mLoaderData.Length(); }
    void GetAllLoaders();

    nsresult FindFactory(const char *contractID, PRUint32 aContractIDLen, nsIFactory **aFactory) ;
    nsresult LoadFactory(nsFactoryEntry *aEntry, nsIFactory **aFactory);

    nsFactoryEntry *GetFactoryEntry(const char *aContractID,
                                    PRUint32 aContractIDLen);
    nsFactoryEntry *GetFactoryEntry(const nsCID &aClass);

    nsresult SyncComponentsInDir(PRInt32 when, nsIFile *dirSpec);

    
    
    nsresult HashContractID(const char *acontractID, PRUint32 aContractIDLen,
                            nsFactoryEntry *fe_ptr);

    void DeleteContractIDEntriesByCID(const nsCID* aClass, nsIFactory* factory);
    nsresult AutoRegisterImpl(nsIFile*                  inDirSpec,
                              nsCOMArray<nsILocalFile> &aLeftovers,
                              nsTArray<DeferredModule> &aDeferred);
    nsresult AutoRegisterDirectory(nsIFile*                  aComponentFile,
                                   nsCOMArray<nsILocalFile> &aLeftovers,
                                   nsTArray<DeferredModule> &aDeferred);
    nsresult AutoRegisterComponentsList(nsIFile* inDir,
                                        PRFileDesc* fd,
                                        nsCOMArray<nsILocalFile>& aLeftovers,
                                        nsTArray<DeferredModule>& aDeferred);
    nsresult AutoRegisterComponent(nsILocalFile*             aComponentFile,
                                   nsTArray<DeferredModule> &aDeferred,
                                   LoaderType                minLoader = NS_LOADER_TYPE_NATIVE);
    void LoadLeftoverComponents(nsCOMArray<nsILocalFile> &aLeftovers,
                                nsTArray<DeferredModule> &aDeferred,
                                LoaderType                minLoader);

    void LoadDeferredModules(nsTArray<DeferredModule> &aDeferred);

    PLDHashTable        mFactories;
    PLDHashTable        mContractIDs;
    PRMonitor*          mMon;

    nsNativeModuleLoader mNativeModuleLoader;
    nsStaticModuleLoader mStaticModuleLoader;
    nsCOMPtr<nsIFile>   mComponentsDir;
    PRInt32             mComponentsOffset;

    nsCOMPtr<nsIFile>   mGREComponentsDir;
    PRInt32             mGREComponentsOffset;

    nsCOMPtr<nsIFile>   mRegistryFile;

    
    #define NS_SHUTDOWN_NEVERHAPPENED 0
    #define NS_SHUTDOWN_INPROGRESS 1
    #define NS_SHUTDOWN_COMPLETE 2
    PRUint32 mShuttingDown;

    nsTArray<nsLoaderdata> mLoaderData;

    nsDataHashtable<nsHashableHashKey, PRInt64> mAutoRegEntries;

    PRBool              mRegistryDirty;
    nsCOMPtr<nsCategoryManager>  mCategoryManager;

    PLArenaPool   mArena;

    struct PendingServiceInfo {
      const nsCID* cid;
      PRThread* thread;
    };

    inline PendingServiceInfo* AddPendingService(const nsCID& aServiceCID,
                                                 PRThread* aThread);
    inline void RemovePendingService(const nsCID& aServiceCID);
    inline PRThread* GetPendingServiceThread(const nsCID& aServiceCID) const;

    nsTArray<PendingServiceInfo> mPendingServices;

private:
    ~nsComponentManagerImpl();
};


#define NS_MAX_FILENAME_LEN	1024

#define NS_ERROR_IS_DIR NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_XPCOM, 24)
















struct nsFactoryEntry {
    nsFactoryEntry(const nsCID    &aClass,
                   LoaderType      aLoaderType,
                   const char     *aLocationKey,
                   nsFactoryEntry *aParent = nsnull);

    nsFactoryEntry(const nsCID    &aClass,
                   nsIFactory     *aFactory,
                   nsFactoryEntry *aParent = nsnull) :
        mCid(aClass),
        mLoaderType(NS_LOADER_TYPE_INVALID),
        mFactory(aFactory),
        mParent(aParent) {
        mLocationKey = nsnull;
    }

    ~nsFactoryEntry();

    void ReInit(LoaderType  aLoaderType,
                const char *aLocationKey);

    nsresult GetFactory(nsIFactory **aFactory);

    nsCID                  mCid;
    LoaderType             mLoaderType;
    const char            *mLocationKey;
    nsCOMPtr<nsIFactory>   mFactory;
    nsCOMPtr<nsISupports>  mServiceObject;
    nsFactoryEntry        *mParent;
};



struct nsFactoryTableEntry : public PLDHashEntryHdr {
    nsFactoryEntry *mFactoryEntry;    
};

struct nsContractIDTableEntry : public PLDHashEntryHdr {
    char           *mContractID;
    PRUint32        mContractIDLen;
    nsFactoryEntry *mFactoryEntry;    
};

#endif 

