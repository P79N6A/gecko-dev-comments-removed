









































#include "nsIXBLService.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "jsapi.h"              
#include "jsclist.h"            
#include "nsFixedSizeAllocator.h"
#include "nsTArray.h"

class nsXBLBinding;
class nsIXBLDocumentInfo;
class nsIContent;
class nsIDocument;
class nsIAtom;
class nsString;
class nsIURI;
class nsSupportsHashtable;
class nsHashtable;

class nsXBLService : public nsIXBLService,
                     public nsIObserver,
                     public nsSupportsWeakReference
{
  NS_DECL_ISUPPORTS

  
  
  NS_IMETHOD LoadBindings(nsIContent* aContent, nsIURI* aURL, PRBool aAugmentFlag,
                          nsXBLBinding** aBinding, PRBool* aResolveStyle);

  
  NS_IMETHOD BindingReady(nsIContent* aBoundElement, nsIURI* aURI, PRBool* aIsReady);

  
  NS_IMETHOD ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID, nsIAtom** aResult);

  
  NS_IMETHOD LoadBindingDocumentInfo(nsIContent* aBoundElement, nsIDocument* aBoundDocument,
                                     nsIURI* aBindingURI,
                                     PRBool aForceSyncLoad, nsIXBLDocumentInfo** aResult);

  
  NS_IMETHOD AttachGlobalKeyHandler(nsPIDOMEventTarget* aTarget);

  NS_DECL_NSIOBSERVER

public:
  nsXBLService();
  virtual ~nsXBLService();

protected:
  
  nsresult FlushStyleBindings(nsIContent* aContent);

  
  nsresult FlushMemory();
  
  
  nsresult FetchBindingDocument(nsIContent* aBoundElement, nsIDocument* aBoundDocument,
                                nsIURI* aDocumentURI, nsIURI* aBindingURI, 
                                PRBool aForceSyncLoad, nsIDocument** aResult);

  nsIXBLDocumentInfo* GetXBLDocumentInfo(nsIURI* aURI,
                                         nsIContent* aBoundElement);

  


  nsresult GetBinding(nsIContent* aBoundElement, nsIURI* aURI,
                      PRBool aPeekFlag, PRBool* aIsReady,
                      nsXBLBinding** aResult);

  













  nsresult GetBinding(nsIContent* aBoundElement, nsIURI* aURI,
                      PRBool aPeekFlag, PRBool* aIsReady,
                      nsXBLBinding** aResult,
                      nsTArray<nsIURI*>& aDontExtendURIs);


public:
  static PRUint32 gRefCnt;                   

  static PRBool gDisableChromeCache;

  static nsHashtable* gClassTable;           

  static JSCList  gClassLRUList;             
  static PRUint32 gClassLRUListLength;       
  static PRUint32 gClassLRUListQuota;        

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

