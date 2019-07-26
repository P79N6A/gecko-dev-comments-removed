






#ifndef nsXBLService_h_
#define nsXBLService_h_

#include "nsString.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "jsapi.h"              
#include "jsclist.h"            
#include "nsTArray.h"

class nsXBLBinding;
class nsXBLDocumentInfo;
class nsIContent;
class nsIDocument;
class nsString;
class nsIURI;
class nsIPrincipal;
class nsSupportsHashtable;
class nsHashtable;
class nsIDOMEventTarget;

class nsXBLService : public nsIObserver,
                     public nsSupportsWeakReference
{
  NS_DECL_ISUPPORTS

  static nsXBLService* gInstance;

  static void Init();

  static void Shutdown() {
    NS_IF_RELEASE(gInstance);
  }

  static nsXBLService* GetInstance() { return gInstance; }

  static bool IsChromeOrResourceURI(nsIURI* aURI);

  
  
  nsresult LoadBindings(nsIContent* aContent, nsIURI* aURL,
                        nsIPrincipal* aOriginPrincipal,
                        nsXBLBinding** aBinding, bool* aResolveStyle);

  
  nsresult BindingReady(nsIContent* aBoundElement, nsIURI* aURI, bool* aIsReady);

  
  
  
  nsresult LoadBindingDocumentInfo(nsIContent* aBoundElement,
                                   nsIDocument* aBoundDocument,
                                   nsIURI* aBindingURI,
                                   nsIPrincipal* aOriginPrincipal,
                                   bool aForceSyncLoad,
                                   nsXBLDocumentInfo** aResult);

  
  static nsresult AttachGlobalKeyHandler(nsIDOMEventTarget* aTarget);
  static nsresult DetachGlobalKeyHandler(nsIDOMEventTarget* aTarget);

  NS_DECL_NSIOBSERVER

private:
  nsXBLService();
  virtual ~nsXBLService();

protected:
  
  nsresult FlushStyleBindings(nsIContent* aContent);

  
  nsresult FlushMemory();
  
  
  nsresult FetchBindingDocument(nsIContent* aBoundElement, nsIDocument* aBoundDocument,
                                nsIURI* aDocumentURI, nsIURI* aBindingURI, 
                                bool aForceSyncLoad, nsIDocument** aResult);

  


  nsresult GetBinding(nsIContent* aBoundElement, nsIURI* aURI,
                      bool aPeekFlag, nsIPrincipal* aOriginPrincipal,
                      bool* aIsReady, nsXBLBinding** aResult);

  
















  nsresult GetBinding(nsIContent* aBoundElement, nsIURI* aURI,
                      bool aPeekFlag, nsIPrincipal* aOriginPrincipal,
                      bool* aIsReady, nsXBLBinding** aResult,
                      nsTArray<nsIURI*>& aDontExtendURIs);


public:
  static bool gDisableChromeCache;

  static nsHashtable* gClassTable;           

  static JSCList  gClassLRUList;             
  static uint32_t gClassLRUListLength;       
  static uint32_t gClassLRUListQuota;        
  static bool     gAllowDataURIs;            
                                             
                                             
};

class nsXBLJSClass : public JSCList, public JSClass
{
private:
  nsrefcnt mRefCnt;
  nsCString mKey;
  static uint64_t sIdCount;
  nsrefcnt Destroy();

public:
  nsXBLJSClass(const nsAFlatCString& aClassName, const nsCString& aKey);
  ~nsXBLJSClass() { nsMemory::Free((void*) name); }

  static uint64_t NewId() { return ++sIdCount; }

  nsCString& Key() { return mKey; }
  void SetKey(const nsCString& aKey) { mKey = aKey; }

  nsrefcnt Hold() { return ++mRefCnt; }
  nsrefcnt Drop() { return --mRefCnt ? mRefCnt : Destroy(); }
  nsrefcnt AddRef() { return Hold(); }
  nsrefcnt Release() { return Drop(); }
};

#endif
