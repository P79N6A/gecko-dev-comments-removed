









































#ifndef nsXULPrototypeCache_h__
#define nsXULPrototypeCache_h__

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsXBLDocumentInfo.h"
#include "nsIXULPrototypeCache.h"
#include "nsDataHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsURIHashKey.h"
#include "nsXULPrototypeDocument.h"

class nsIFastLoadService;
class nsCSSStyleSheet;

struct CacheScriptEntry
{
    PRUint32    mScriptTypeID; 
    void*       mScriptObject; 
};









class nsXULPrototypeCache : public nsIXULPrototypeCache,
                                   nsIObserver
{
public:
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    
    virtual PRBool IsCached(nsIURI* aURI) {
        return GetPrototype(aURI) != nsnull;
    }
    virtual void AbortFastLoads();


    


    PRBool IsEnabled();

    



    void Flush();

    


    void RemoveFromFastLoadSet(nsIURI* aDocumentURI);

    



    nsresult WritePrototype(nsXULPrototypeDocument* aPrototypeDocument);

    
    

    nsXULPrototypeDocument* GetPrototype(nsIURI* aURI);
    nsresult PutPrototype(nsXULPrototypeDocument* aDocument);

    void* GetScript(nsIURI* aURI, PRUint32* langID);
    nsresult PutScript(nsIURI* aURI, PRUint32 langID, void* aScriptObject);

    nsXBLDocumentInfo* GetXBLDocumentInfo(nsIURI* aURL) {
        return mXBLDocTable.GetWeak(aURL);
    }
    nsresult PutXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo);

    



    nsCSSStyleSheet* GetStyleSheet(nsIURI* aURI) {
        return mStyleSheetTable.GetWeak(aURI);
    }

    



    nsresult PutStyleSheet(nsCSSStyleSheet* aStyleSheet);


    static nsXULPrototypeCache* GetInstance();
    static nsIFastLoadService* GetFastLoadService();

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

    
    
    
    nsDataHashtable<nsURIHashKey,PRUint32> mFastLoadURITable;

    static nsIFastLoadService*    gFastLoadService;
    static nsIFile*               gFastLoadFile;

    
    nsresult StartFastLoad(nsIURI* aDocumentURI);
    nsresult StartFastLoadingURI(nsIURI* aURI, PRInt32 aDirectionFlags);
};

#endif 
