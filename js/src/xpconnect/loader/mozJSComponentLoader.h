






































#include "plhash.h"
#include "jsapi.h"
#include "mozilla/ModuleLoader.h"
#include "nsIJSRuntimeService.h"
#include "nsIJSContextStack.h"
#include "nsISupports.h"
#include "nsIXPConnect.h"
#include "nsIFile.h"
#include "nsAutoPtr.h"
#include "nsIFastLoadService.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsITimer.h"
#include "nsIObserver.h"
#include "xpcIJSModuleLoader.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#ifndef XPCONNECT_STANDALONE
#include "nsIPrincipal.h"
#endif
#include "xpcIJSGetFactory.h"



#define MOZJSCOMPONENTLOADER_CID \
  {0x6bd13476, 0x1dd2, 0x11b2, \
    { 0xbb, 0xef, 0xf0, 0xcc, 0xb5, 0xfa, 0x64, 0xb6 }}
#define MOZJSCOMPONENTLOADER_CONTRACTID "@mozilla.org/moz/jsloader;1"


class nsXPCFastLoadIO : public nsIFastLoadFileIO
{
 public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIFASTLOADFILEIO

    nsXPCFastLoadIO(nsIFile *file) : mFile(file), mTruncateOutputFile(true) {}

    void SetInputStream(nsIInputStream *stream) { mInputStream = stream; }
    void SetOutputStream(nsIOutputStream *stream) { mOutputStream = stream; }

 private:
    ~nsXPCFastLoadIO() {}

    nsCOMPtr<nsIFile> mFile;
    nsCOMPtr<nsIInputStream> mInputStream;
    nsCOMPtr<nsIOutputStream> mOutputStream;
    bool mTruncateOutputFile;
};


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

    
    const mozilla::Module* LoadModule(nsILocalFile* aFile);
    const mozilla::Module* LoadModuleFromJAR(nsILocalFile* aJARFile,
                                             const nsACString& aPath);

 protected:
    static mozJSComponentLoader* sSelf;

    nsresult ReallyInit();
    void UnloadModules();

    nsresult FileKey(nsILocalFile* aFile, nsAString &aResult);
    nsresult JarKey(nsILocalFile* aFile,
                    const nsACString& aComponentPath,
                    nsAString &aResult);

    const mozilla::Module* LoadModuleImpl(nsILocalFile* aSourceFile,
                                          nsAString &aKey,
                                          nsIURI* aComponentURI);

    nsresult GlobalForLocation(nsILocalFile* aComponentFile,
                               nsIURI *aComponent,
                               JSObject **aGlobal,
                               char **location,
                               jsval *exception);

    nsresult StartFastLoad(nsIFastLoadService *flSvc);
    nsresult ReadScript(nsIFastLoadService *flSvc, const char *nativePath,
                        nsIURI *uri, JSContext *cx, JSScript **script);
    nsresult WriteScript(nsIFastLoadService *flSvc, JSScript *script,
                         nsIFile *component, const char *nativePath,
                         nsIURI *uri, JSContext *cx);
    static void CloseFastLoad(nsITimer *timer, void *closure);
    void CloseFastLoad();

    nsCOMPtr<nsIComponentManager> mCompMgr;
    nsCOMPtr<nsIJSRuntimeService> mRuntimeService;
    nsCOMPtr<nsIThreadJSContextStack> mContextStack;
    nsCOMPtr<nsIFile> mFastLoadFile;
    nsRefPtr<nsXPCFastLoadIO> mFastLoadIO;
    nsCOMPtr<nsIObjectInputStream> mFastLoadInput;
    nsCOMPtr<nsIObjectOutputStream> mFastLoadOutput;
    nsCOMPtr<nsITimer> mFastLoadTimer;
#ifndef XPCONNECT_STANDALONE
    nsCOMPtr<nsIPrincipal> mSystemPrincipal;
#endif
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

            global = nsnull;
            location = nsnull;
        }

        ~ModuleEntry() {
            Clear();
        }

        void Clear() {
            getfactoryobj = NULL;

            if (global) {
                JSAutoRequest ar(sSelf->mContext);
                JS_ClearScope(sSelf->mContext, global);
                JS_RemoveRoot(sSelf->mContext, &global);
            }

            if (location)
                NS_Free(location);

            global = NULL;
            location = NULL;
        }

        static already_AddRefed<nsIFactory> GetFactory(const mozilla::Module& module,
                                                       const mozilla::Module::CIDEntry& entry);

        nsCOMPtr<xpcIJSGetFactory> getfactoryobj;
        JSObject            *global;
        char                *location;
    };

    friend class ModuleEntry;

    
    static PLDHashOperator ClearModules(const nsAString& key, ModuleEntry*& entry, void* cx);
    nsDataHashtable<nsStringHashKey, ModuleEntry*> mModules;

    nsClassHashtable<nsStringHashKey, ModuleEntry> mImports;
    nsDataHashtable<nsStringHashKey, ModuleEntry*> mInProgressImports;

    PRBool mInitialized;
};
