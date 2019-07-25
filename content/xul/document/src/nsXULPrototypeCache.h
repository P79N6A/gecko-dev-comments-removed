









































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

using namespace mozilla::scache;

class nsCSSStyleSheet;

struct CacheScriptEntry
{
    PRUint32    mScriptTypeID; 
    JSScript*   mScriptObject; 
};









class nsXULPrototypeCache : public nsIObserver
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    bool IsCached(nsIURI* aURI) {
        return GetPrototype(aURI) != nsnull;
    }
    void AbortCaching();


    


    bool IsEnabled();

    



    void Flush();


    
    

    nsXULPrototypeDocument* GetPrototype(nsIURI* aURI);
    nsresult PutPrototype(nsXULPrototypeDocument* aDocument);

    JSScript* GetScript(nsIURI* aURI, PRUint32* langID);
    nsresult PutScript(nsIURI* aURI, PRUint32 langID, JSScript* aScriptObject);

    nsXBLDocumentInfo* GetXBLDocumentInfo(nsIURI* aURL) {
        return mXBLDocTable.GetWeak(aURL);
    }
    nsresult PutXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo);

    



    nsCSSStyleSheet* GetStyleSheet(nsIURI* aURI) {
        return mStyleSheetTable.GetWeak(aURI);
    }

    



    nsresult PutStyleSheet(nsCSSStyleSheet* aStyleSheet);

    


    void RemoveFromCacheSet(nsIURI* aDocumentURI);

    



    nsresult WritePrototype(nsXULPrototypeDocument* aPrototypeDocument);

    



    nsresult GetInputStream(nsIURI* aURI, nsIObjectInputStream** objectInput);
    nsresult FinishInputStream(nsIURI* aURI);
    nsresult GetOutputStream(nsIURI* aURI, nsIObjectOutputStream** objectOutput);
    nsresult FinishOutputStream(nsIURI* aURI);
    nsresult HasData(nsIURI* aURI, bool* exists);

    static StartupCache* GetStartupCache();

    static nsXULPrototypeCache* GetInstance();

    static void ReleaseGlobals()
    {
        NS_IF_RELEASE(sInstance);
    }

protected:
    friend nsresult
    NS_NewXULPrototypeCache(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    nsXULPrototypeCache();
    virtual ~nsXULPrototypeCache();

    static nsXULPrototypeCache* sInstance;

    void FlushScripts();
    void FlushSkinFiles();

    nsRefPtrHashtable<nsURIHashKey,nsXULPrototypeDocument>  mPrototypeTable; 
    nsRefPtrHashtable<nsURIHashKey,nsCSSStyleSheet>        mStyleSheetTable;
    nsDataHashtable<nsURIHashKey,CacheScriptEntry>         mScriptTable;
    nsRefPtrHashtable<nsURIHashKey,nsXBLDocumentInfo>  mXBLDocTable;

    
    
    
    nsDataHashtable<nsURIHashKey,PRUint32> mCacheURITable;

    static StartupCache* gStartupCache;
    nsInterfaceHashtable<nsURIHashKey, nsIStorageStream> mOutputStreamTable;
    nsInterfaceHashtable<nsURIHashKey, nsIObjectInputStream> mInputStreamTable;
 
    
    nsresult BeginCaching(nsIURI* aDocumentURI);
};

#endif 
