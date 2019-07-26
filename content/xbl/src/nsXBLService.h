






#include "nsString.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "jsapi.h"              
#include "jsclist.h"            
#include "nsFixedSizeAllocator.h"
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
                        nsIPrincipal* aOriginPrincipal, bool aAugmentFlag,
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
  static PRUint32 gClassLRUListLength;       
  static PRUint32 gClassLRUListQuota;        
  static bool     gAllowDataURIs;            
                                             
                                             

  nsFixedSizeAllocator mPool;
};

class nsXBLJSClass : public JSCList, public JSClass
{
private:
  nsrefcnt mRefCnt;
  nsrefcnt Destroy();

public:
  nsXBLJSClass(const nsAFlatCString& aClassName);
  ~nsXBLJSClass() { nsMemory::Free((void*) name); }

  nsrefcnt Hold() { return ++mRefCnt; }
  nsrefcnt Drop() { return --mRefCnt ? mRefCnt : Destroy(); }
};

