






































#include "plhash.h"
#include "jsapi.h"
#include "nsIModuleLoader.h"
#include "nsIJSRuntimeService.h"
#include "nsIJSContextStack.h"
#include "nsISupports.h"
#include "nsIXPConnect.h"
#include "nsIModule.h"
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



#define MOZJSCOMPONENTLOADER_CID \
  {0x6bd13476, 0x1dd2, 0x11b2, \
    { 0xbb, 0xef, 0xf0, 0xcc, 0xb5, 0xfa, 0x64, 0xb6 }}
#define MOZJSCOMPONENTLOADER_CONTRACTID "@mozilla.org/moz/jsloader;1"
#define MOZJSCOMPONENTLOADER_TYPE_NAME "text/javascript"


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


class mozJSComponentLoader : public nsIModuleLoader,
                             public xpcIJSModuleLoader,
                             public nsIObserver
{
    friend class JSCLContextHelper;
 public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMODULELOADER
    NS_DECL_XPCIJSMODULELOADER
    NS_DECL_NSIOBSERVER

    mozJSComponentLoader();
    virtual ~mozJSComponentLoader();

 protected:
    static mozJSComponentLoader* sSelf;

    nsresult ReallyInit();
    void UnloadModules();

    nsresult GlobalForLocation(nsILocalFile *aComponent,
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

    class ModuleEntry
    {
    public:
        ModuleEntry() {
            global = nsnull;
            location = nsnull;
        }

        ~ModuleEntry() {
            module = nsnull;

            if (global) {
                JSAutoRequest ar(sSelf->mContext);
                JS_ClearScope(sSelf->mContext, global);
                JS_RemoveRoot(sSelf->mContext, &global);
            }

            if (location)
                NS_Free(location);
        }

        nsCOMPtr<nsIModule>  module;
        JSObject            *global;
        char                *location;
    };

    friend class ModuleEntry;

    nsClassHashtable<nsHashableHashKey, ModuleEntry> mModules;
    nsClassHashtable<nsHashableHashKey, ModuleEntry> mImports;
    nsDataHashtable<nsHashableHashKey, ModuleEntry*> mInProgressImports;

    PRBool mInitialized;
};
