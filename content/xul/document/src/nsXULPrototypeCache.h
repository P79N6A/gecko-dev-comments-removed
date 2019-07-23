









































#ifndef nsXULPrototypeCache_h__
#define nsXULPrototypeCache_h__

#include "nsCOMPtr.h"
#include "nsICSSStyleSheet.h"
#include "nsIObserver.h"
#include "nsIXBLDocumentInfo.h"
#include "nsIXULPrototypeCache.h"
#include "nsDataHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsURIHashKey.h"
#include "nsXULPrototypeDocument.h"

class nsIFastLoadService;

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

    nsIXBLDocumentInfo* GetXBLDocumentInfo(nsIURI* aURL) {
        return mXBLDocTable.GetWeak(aURL);
    }
    nsresult PutXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo);

    



    nsICSSStyleSheet* GetStyleSheet(nsIURI* aURI) {
        return mStyleSheetTable.GetWeak(aURI);
    }

    



    nsresult PutStyleSheet(nsICSSStyleSheet* aStyleSheet);


    static nsXULPrototypeCache* GetInstance();
    static nsIFastLoadService* GetFastLoadService();

    static void ReleaseGlobals()
    {
        NS_IF_RELEASE(sInstance);
    }

protected:
    friend NS_IMETHODIMP
    NS_NewXULPrototypeCache(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    nsXULPrototypeCache();
    virtual ~nsXULPrototypeCache();

    static nsXULPrototypeCache* sInstance;

    void FlushScripts();
    void FlushSkinFiles();

    nsRefPtrHashtable<nsURIHashKey,nsXULPrototypeDocument>  mPrototypeTable; 
    nsInterfaceHashtable<nsURIHashKey,nsICSSStyleSheet>    mStyleSheetTable;
    nsDataHashtable<nsURIHashKey,CacheScriptEntry>         mScriptTable;
    nsInterfaceHashtable<nsURIHashKey,nsIXBLDocumentInfo>  mXBLDocTable;

    
    
    
    nsDataHashtable<nsURIHashKey,PRUint32> mFastLoadURITable;

    static nsIFastLoadService*    gFastLoadService;
    static nsIFile*               gFastLoadFile;

    
    nsresult StartFastLoad(nsIURI* aDocumentURI);
    nsresult StartFastLoadingURI(nsIURI* aURI, PRInt32 aDirectionFlags);
};

#endif 
