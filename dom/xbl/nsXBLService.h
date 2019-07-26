






#ifndef nsXBLService_h_
#define nsXBLService_h_

#include "nsString.h"
#include "nsWeakReference.h"
#include "js/Class.h"           
#include "nsTArray.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"

class nsXBLBinding;
class nsXBLDocumentInfo;
class nsXBLJSClass;
class nsIContent;
class nsIDocument;
class nsString;
class nsIURI;
class nsIPrincipal;

namespace mozilla {
namespace dom {
class EventTarget;
}
}

class nsXBLService MOZ_FINAL : public nsSupportsWeakReference
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

private:
  nsXBLService();
  virtual ~nsXBLService();

protected:
  
  nsresult FlushStyleBindings(nsIContent* aContent);

  
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

  typedef nsDataHashtable<nsCStringHashKey, nsXBLJSClass*> ClassTable;
  static ClassTable* gClassTable;           

  static bool     gAllowDataURIs;            
                                             
                                             

  
  static nsXBLJSClass *getClass(const nsCString &key);
};

class nsXBLJSClass : public JSClass
{
private:
  nsrefcnt mRefCnt;
  nsCString mKey;
  static uint64_t sIdCount;

public:
  nsXBLJSClass(const nsAFlatCString& aClassName, const nsCString& aKey);
  ~nsXBLJSClass();

  static uint64_t NewId() { return ++sIdCount; }

  nsCString& Key() { return mKey; }
  void SetKey(const nsCString& aKey) { mKey = aKey; }

  nsrefcnt Hold() { return ++mRefCnt; }
  nsrefcnt Drop() { nsrefcnt curr = --mRefCnt; if (!curr) delete this; return curr; }
  nsrefcnt AddRef() { return Hold(); }
  nsrefcnt Release() { return Drop(); }

  static bool IsXBLJSClass(const JSClass* aClass);

  
  
  
  
  
  
  
  
  
  static nsXBLJSClass*
  fromJSClass(const JSClass* c)
  {
    MOZ_ASSERT(IsXBLJSClass(c));
    nsXBLJSClass* x = const_cast<nsXBLJSClass*>(static_cast<const nsXBLJSClass*>(c));
    MOZ_ASSERT(nsXBLService::getClass(x->mKey) == x);
    return x;
  }
};

#endif
