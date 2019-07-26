




#ifndef nsXULPrototypeCache_h__
#define nsXULPrototypeCache_h__

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsXBLDocumentInfo.h"
#include "nsDataHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsURIHashKey.h"
#include "nsXULPrototypeDocument.h"
#include "nsIInputStream.h"
#include "nsIStorageStream.h"

#include "jspubtd.h"

#include "mozilla/scache/StartupCache.h"


class nsCSSStyleSheet;

struct CacheScriptEntry
{
    JSScript*   mScriptObject; 
};









class nsXULPrototypeCache : public nsIObserver
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    bool IsCached(nsIURI* aURI) {
        return GetPrototype(aURI) != nullptr;
    }
    void AbortCaching();


    


    bool IsEnabled();

    



    void Flush();


    
    

    nsXULPrototypeDocument* GetPrototype(nsIURI* aURI);
    nsresult PutPrototype(nsXULPrototypeDocument* aDocument);

    JSScript* GetScript(nsIURI* aURI);
    nsresult PutScript(nsIURI* aURI, JSScript* aScriptObject);

    nsXBLDocumentInfo* GetXBLDocumentInfo(nsIURI* aURL) {
        return mXBLDocTable.GetWeak(aURL);
    }
    nsresult PutXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo);

    



    nsCSSStyleSheet* GetStyleSheet(nsIURI* aURI) {
        return mStyleSheetTable.GetWeak(aURI);
    }

    



    nsresult PutStyleSheet(nsCSSStyleSheet* aStyleSheet);

    



    nsresult WritePrototype(nsXULPrototypeDocument* aPrototypeDocument);

    



    nsresult GetInputStream(nsIURI* aURI, nsIObjectInputStream** objectInput);
    nsresult FinishInputStream(nsIURI* aURI);
    nsresult GetOutputStream(nsIURI* aURI, nsIObjectOutputStream** objectOutput);
    nsresult FinishOutputStream(nsIURI* aURI);
    nsresult HasData(nsIURI* aURI, bool* exists);

    static nsXULPrototypeCache* GetInstance();
    static nsXULPrototypeCache* MaybeGetInstance() { return sInstance; }

    static void ReleaseGlobals()
    {
        NS_IF_RELEASE(sInstance);
    }

    void MarkInCCGeneration(uint32_t aGeneration);
    void MarkInGC(JSTracer* aTrc);
    void FlushScripts();
protected:
    friend nsresult
    NS_NewXULPrototypeCache(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    nsXULPrototypeCache();
    virtual ~nsXULPrototypeCache();

    static nsXULPrototypeCache* sInstance;

    void FlushSkinFiles();

    nsRefPtrHashtable<nsURIHashKey,nsXULPrototypeDocument>  mPrototypeTable; 
    nsRefPtrHashtable<nsURIHashKey,nsCSSStyleSheet>        mStyleSheetTable;
    nsDataHashtable<nsURIHashKey,CacheScriptEntry>         mScriptTable;
    nsRefPtrHashtable<nsURIHashKey,nsXBLDocumentInfo>  mXBLDocTable;

    nsTHashtable<nsURIHashKey> mCacheURITable;

    nsInterfaceHashtable<nsURIHashKey, nsIStorageStream> mOutputStreamTable;
    nsInterfaceHashtable<nsURIHashKey, nsIObjectInputStream> mInputStreamTable;

    
    nsresult BeginCaching(nsIURI* aDocumentURI);
};

#endif 
