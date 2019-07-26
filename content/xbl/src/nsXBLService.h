






#ifndef nsXBLService_h_
#define nsXBLService_h_

#include "mozilla/LinkedList.h"
#include "nsString.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "js/Class.h"           
#include "nsTArray.h"

class nsCStringKey;
class nsXBLBinding;
class nsXBLDocumentInfo;
class nsXBLJSClass;
class nsIContent;
class nsIDocument;
class nsString;
class nsIURI;
class nsIPrincipal;
class nsSupportsHashtable;
class nsHashtable;

namespace mozilla {
namespace dom {
class EventTarget;
}
}

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

  
  static nsresult AttachGlobalKeyHandler(mozilla::dom::EventTarget* aTarget);
  static nsresult DetachGlobalKeyHandler(mozilla::dom::EventTarget* aTarget);

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

  static mozilla::LinkedList<nsXBLJSClass>* gClassLRUList;
                                             
  static uint32_t gClassLRUListLength;       
  static uint32_t gClassLRUListQuota;        
  static bool     gAllowDataURIs;            
                                             
                                             

  
  static nsXBLJSClass *getClass(const nsCString &key);
  static nsXBLJSClass *getClass(nsCStringKey *key);
};

class nsXBLJSClass : public mozilla::LinkedListElement<nsXBLJSClass>
                   , public JSClass
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

  
  static nsXBLJSClass*
  fromJSClass(JSClass* c)
  {
    nsXBLJSClass* x = static_cast<nsXBLJSClass*>(c);
    MOZ_ASSERT(nsXBLService::getClass(x->mKey) == x);
    return x;
  }
};

#endif
