





#include "plhash.h"
#include "jsapi.h"
#include "mozilla/ModuleLoader.h"
#include "nsIJSRuntimeService.h"
#include "nsIJSContextStack.h"
#include "nsISupports.h"
#include "nsIXPConnect.h"
#include "nsIFile.h"
#include "nsAutoPtr.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsITimer.h"
#include "nsIObserver.h"
#include "xpcIJSModuleLoader.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsIPrincipal.h"
#include "mozilla/scache/StartupCache.h"

#include "xpcIJSGetFactory.h"



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
                               JSObject **aObject,
                               char **location,
                               jsval *exception);

    nsresult ImportInto(const nsACString & aLocation,
                        JSObject * targetObj,
                        JSContext * callercx,
                        JSObject * *_retval);

    nsCOMPtr<nsIComponentManager> mCompMgr;
    nsCOMPtr<nsIJSRuntimeService> mRuntimeService;
    nsCOMPtr<nsIThreadJSContextStack> mContextStack;
    nsCOMPtr<nsIPrincipal> mSystemPrincipal;
    nsCOMPtr<nsIXPConnectJSObjectHolder> mLoaderGlobal;
    JSRuntime *mRuntime;
    JSContext *mContext;

    class ModuleEntry : public mozilla::Module
    {
    public:
        ModuleEntry() : mozilla::Module() {
            mVersion = mozilla::Module::kVersion;
            mCIDs = NULL;
            mContractIDs = NULL;
            mCategoryEntries = NULL;
            getFactoryProc = GetFactory;
            loadProc = NULL;
            unloadProc = NULL;

            obj = nullptr;
            location = nullptr;
        }

        ~ModuleEntry() {
            Clear();
        }

        void Clear() {
            getfactoryobj = NULL;

            if (obj) {
                JSAutoRequest ar(sSelf->mContext);

                JSAutoCompartment ac(sSelf->mContext, obj);

                JS_SetAllNonReservedSlotsToUndefined(sSelf->mContext, obj);
                JS_RemoveObjectRoot(sSelf->mContext, &obj);
            }

            if (location)
                NS_Free(location);

            obj = NULL;
            location = NULL;
        }

        static already_AddRefed<nsIFactory> GetFactory(const mozilla::Module& module,
                                                       const mozilla::Module::CIDEntry& entry);

        nsCOMPtr<xpcIJSGetFactory> getfactoryobj;
        JSObject            *obj;
        char                *location;
    };

    friend class ModuleEntry;

    
    static PLDHashOperator ClearModules(const nsACString& key, ModuleEntry*& entry, void* cx);
    nsDataHashtable<nsCStringHashKey, ModuleEntry*> mModules;

    nsClassHashtable<nsCStringHashKey, ModuleEntry> mImports;
    nsDataHashtable<nsCStringHashKey, ModuleEntry*> mInProgressImports;

    bool mInitialized;
    bool mReuseLoaderGlobal;
};
