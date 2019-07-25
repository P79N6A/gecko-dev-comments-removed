




































#ifndef nsComponentManager_h__
#define nsComponentManager_h__

#include "nsXPCOM.h"

#include "xpcom-private.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIServiceManager.h"
#include "nsILocalFile.h"
#include "mozilla/Module.h"
#include "mozilla/ModuleLoader.h"
#include "nsXULAppAPI.h"
#include "nsNativeComponentLoader.h"
#include "nsIFactory.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "pldhash.h"
#include "prtime.h"
#include "prmon.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"
#include "nsIFile.h"
#include "plarena.h"
#include "nsCOMArray.h"
#include "nsDataHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsClassHashtable.h"
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
#define NS_LOADER_TYPE_JAR     -3
#define NS_LOADER_TYPE_INVALID -4

#ifdef DEBUG
#define XPCOM_CHECK_PENDING_CIDS
#endif


extern const mozilla::Module kXPCOMModule;


struct nsLoaderdata {
    nsCOMPtr<mozilla::ModuleLoader> loader;
    nsCString                 type;
};

class nsComponentManagerImpl
    : public nsIComponentManager,
      public nsIServiceManager,
      public nsSupportsWeakReference,
      public nsIComponentRegistrar,
      public nsIInterfaceRequestor
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSICOMPONENTMANAGER
    NS_DECL_NSICOMPONENTREGISTRAR

    static nsresult Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    nsresult RegistryLocationForFile(nsIFile* aFile,
                                     nsCString& aResult);
    nsresult FileForRegistryLocation(const nsCString &aLocation,
                                     nsILocalFile **aSpec);

    NS_DECL_NSISERVICEMANAGER

    
    nsComponentManagerImpl();

    static nsComponentManagerImpl* gComponentManager;
    nsresult Init();

    nsresult Shutdown(void);

    nsresult FreeServices();

    already_AddRefed<mozilla::ModuleLoader> LoaderForExtension(const nsACString& aExt);
    nsInterfaceHashtable<nsCStringHashKey, mozilla::ModuleLoader> mLoaderMap;

    already_AddRefed<nsIFactory> FindFactory(const nsCID& aClass);
    already_AddRefed<nsIFactory> FindFactory(const char *contractID,
                                             PRUint32 aContractIDLen);

    already_AddRefed<nsIFactory> LoadFactory(nsFactoryEntry *aEntry);

    nsFactoryEntry *GetFactoryEntry(const char *aContractID,
                                    PRUint32 aContractIDLen);
    nsFactoryEntry *GetFactoryEntry(const nsCID &aClass);

    nsDataHashtable<nsIDHashKey, nsFactoryEntry*> mFactories;
    nsDataHashtable<nsCStringHashKey, nsFactoryEntry*> mContractIDs;

    PRMonitor*          mMon;

    static void InitializeStaticModules();
    static void InitializeModuleLocations();

    struct ComponentLocation
    {
        NSLocationType type;
        nsCOMPtr<nsILocalFile> location;
    };

    static nsTArray<const mozilla::Module*>* sStaticModules;
    static nsTArray<ComponentLocation>* sModuleLocations;

    nsNativeModuleLoader mNativeModuleLoader;

    class KnownModule
    {
    public:
        


        KnownModule(const mozilla::Module* aModule, nsILocalFile* aFile)
            : mModule(aModule)
            , mFile(aFile)
            , mLoaded(false)
            , mFailed(false)
        { }

        KnownModule(nsILocalFile* aFile)
            : mModule(NULL)
            , mFile(aFile)
            , mLoader(NULL)
            , mLoaded(false)
            , mFailed(false)
        { }

        ~KnownModule()
        {
            if (mLoaded && mModule->unloadProc)
                mModule->unloadProc();
        }

        bool EnsureLoader();
        bool Load();

        const mozilla::Module* Module() const
        {
            return mModule;
        }

    private:
        const mozilla::Module* mModule;
        nsCOMPtr<nsILocalFile> mFile;
        nsCOMPtr<mozilla::ModuleLoader> mLoader;
        bool mLoaded;
        bool mFailed;
    };

    
    
    nsTArray< nsAutoPtr<KnownModule> > mKnownStaticModules;
    nsClassHashtable<nsHashableHashKey, KnownModule> mKnownFileModules;

    void RegisterModule(const mozilla::Module* aModule,
                        nsILocalFile* aFile);
    void RegisterCIDEntry(const mozilla::Module::CIDEntry* aEntry,
                          KnownModule* aModule);
    void RegisterContractID(const mozilla::Module::ContractIDEntry* aEntry);

    void RegisterLocation(NSLocationType aType, nsILocalFile* aLocation);

    
    
    
    void RegisterDirectory(NSLocationType aType, nsILocalFile* aDirectory,
                           nsCOMArray<nsILocalFile>& aManifests);
    void RegisterFile(NSLocationType aType, nsILocalFile* aFile,
                      nsCOMArray<nsILocalFile>& aManifests);

    void RegisterManifestFile(NSLocationType aType, nsILocalFile* aFile);

    struct ManifestProcessingContext
    {
        ManifestProcessingContext(NSLocationType aType, nsILocalFile* aFile)
            : mType(aType)
            , mFile(aFile)
        { }
        ~ManifestProcessingContext() { }

        NSLocationType mType;
        nsCOMPtr<nsILocalFile> mFile;
    };

    void ManifestBinaryComponent(ManifestProcessingContext& cx, int lineno, char *const * argv);
    void ManifestComponent(ManifestProcessingContext& cx, int lineno, char *const * argv);
    void ManifestContract(ManifestProcessingContext& cx, int lineno, char* const * argv);
    void ManifestCategory(ManifestProcessingContext& cx, int lineno, char* const * argv);

    void RereadChromeManifests();

    
    enum {
        NOT_INITIALIZED,
        NORMAL,
        SHUTDOWN_IN_PROGRESS,
        SHUTDOWN_COMPLETE
    } mStatus;

    nsTArray<nsLoaderdata> mLoaderData;

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

struct nsFactoryEntry
{
    nsFactoryEntry(const mozilla::Module::CIDEntry* entry,
                   nsComponentManagerImpl::KnownModule* module);

    
    nsFactoryEntry(nsIFactory* factory);

    already_AddRefed<nsIFactory> GetFactory();

    const mozilla::Module::CIDEntry* mCIDEntry;
    nsComponentManagerImpl::KnownModule* mModule;

    nsCOMPtr<nsIFactory>   mFactory;
    nsCOMPtr<nsISupports>  mServiceObject;
};

#endif 

