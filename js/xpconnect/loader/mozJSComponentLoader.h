





#ifndef mozJSComponentLoader_h
#define mozJSComponentLoader_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/ModuleLoader.h"
#include "nsISupports.h"
#include "nsIObserver.h"
#include "nsIURI.h"
#include "xpcIJSModuleLoader.h"
#include "nsClassHashtable.h"
#include "nsCxPusher.h"
#include "nsDataHashtable.h"
#include "jsapi.h"
#include "js/HashTable.h"
#include "js/RootingAPI.h"

#include "xpcIJSGetFactory.h"

class nsIFile;
class nsIJSRuntimeService;
class nsIPrincipal;
class nsIXPConnectJSObjectHolder;



#define MOZJSCOMPONENTLOADER_CID                                              \
  {0x6bd13476, 0x1dd2, 0x11b2,                                                \
    { 0xbb, 0xef, 0xf0, 0xcc, 0xb5, 0xfa, 0x64, 0xb6 }}
#define MOZJSCOMPONENTLOADER_CONTRACTID "@mozilla.org/moz/jsloader;1"

class JSCLContextHelper;

class mozJSComponentLoader : public mozilla::ModuleLoader,
                             public xpcIJSModuleLoader,
                             public nsIObserver
{
    friend class JSCLContextHelper;
 public:
    NS_DECL_ISUPPORTS
    NS_DECL_XPCIJSMODULELOADER
    NS_DECL_NSIOBSERVER

    mozJSComponentLoader();
    virtual ~mozJSComponentLoader();

    
    const mozilla::Module* LoadModule(mozilla::FileLocation &aFile);

    nsresult FindTargetObject(JSContext* aCx,
                              JS::MutableHandleObject aTargetObject);

    static mozJSComponentLoader* Get() { return sSelf; }

    void NoteSubScript(JS::HandleScript aScript, JS::HandleObject aThisObject);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

    void Trace(JSTracer *trc);

 protected:
    static mozJSComponentLoader* sSelf;

    nsresult ReallyInit();
    void UnloadModules();

    JSObject* PrepareObjectForLocation(JSCLContextHelper& aCx,
                                       nsIFile* aComponentFile,
                                       nsIURI *aComponent,
                                       bool aReuseLoaderGlobal,
                                       bool *aRealFile);

    nsresult ObjectForLocation(nsIFile* aComponentFile,
                               nsIURI *aComponent,
                               JS::MutableHandle<JSObject*> aObject,
                               JS::MutableHandle<JSScript*> aTableScript,
                               char **location,
                               bool aCatchException,
                               JS::MutableHandleValue aException);

    nsresult ImportInto(const nsACString &aLocation,
                        JS::HandleObject targetObj,
                        JSContext *callercx,
                        JS::MutableHandleObject vp);

    nsCOMPtr<nsIComponentManager> mCompMgr;
    nsCOMPtr<nsIJSRuntimeService> mRuntimeService;
    nsCOMPtr<nsIPrincipal> mSystemPrincipal;
    nsCOMPtr<nsIXPConnectJSObjectHolder> mLoaderGlobal;
    JSRuntime *mRuntime;
    JSContext *mContext;

    class ModuleEntry : public mozilla::Module
    {
    public:
        ModuleEntry(JSContext *cx) : mozilla::Module(), obj(cx), thisObjectKey(cx) {
            mVersion = mozilla::Module::kVersion;
            mCIDs = nullptr;
            mContractIDs = nullptr;
            mCategoryEntries = nullptr;
            getFactoryProc = GetFactory;
            loadProc = nullptr;
            unloadProc = nullptr;

            obj = nullptr;
            thisObjectKey = nullptr;
            location = nullptr;
        }

        ~ModuleEntry() {
            Clear();
        }

        void Clear() {
            getfactoryobj = nullptr;

            if (obj) {
                mozilla::AutoJSContext cx;
                JSAutoCompartment ac(cx, obj);

                JS_SetAllNonReservedSlotsToUndefined(cx, obj);
            }

            if (location)
                NS_Free(location);

            obj = nullptr;
            thisObjectKey = nullptr;
            location = nullptr;
        }

        size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

        static already_AddRefed<nsIFactory> GetFactory(const mozilla::Module& module,
                                                       const mozilla::Module::CIDEntry& entry);

        nsCOMPtr<xpcIJSGetFactory> getfactoryobj;
        JS::PersistentRooted<JSObject*> obj;
        JS::PersistentRooted<JSScript*> thisObjectKey;
        char *location;
    };

    friend class ModuleEntry;

    static size_t DataEntrySizeOfExcludingThis(const nsACString& aKey, ModuleEntry* const& aData,
                                               mozilla::MallocSizeOf aMallocSizeOf, void* arg);
    static size_t ClassEntrySizeOfExcludingThis(const nsACString& aKey,
                                                const nsAutoPtr<ModuleEntry>& aData,
                                                mozilla::MallocSizeOf aMallocSizeOf, void* arg);

    
    static PLDHashOperator ClearModules(const nsACString& key, ModuleEntry*& entry, void* cx);
    nsDataHashtable<nsCStringHashKey, ModuleEntry*> mModules;

    nsClassHashtable<nsCStringHashKey, ModuleEntry> mImports;
    nsDataHashtable<nsCStringHashKey, ModuleEntry*> mInProgressImports;
    typedef js::HashMap<JSScript*,
                        JS::Heap<JSObject*>,
                        js::PointerHasher<JSScript*, 3>,
                        js::SystemAllocPolicy> ThisObjectsMap;
    ThisObjectsMap mThisObjects;

    bool mInitialized;
    bool mReuseLoaderGlobal;
};

#endif
