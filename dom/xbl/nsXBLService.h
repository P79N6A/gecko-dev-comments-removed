






#ifndef nsXBLService_h_
#define nsXBLService_h_

#include "nsString.h"
#include "nsWeakReference.h"
#include "nsTArray.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"

class nsXBLBinding;
class nsXBLDocumentInfo;
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
                                nsIPrincipal* aOriginPrincipal, bool aForceSyncLoad,
                                nsIDocument** aResult);

  


  nsresult GetBinding(nsIContent* aBoundElement, nsIURI* aURI,
                      bool aPeekFlag, nsIPrincipal* aOriginPrincipal,
                      bool* aIsReady, nsXBLBinding** aResult);

  
















  nsresult GetBinding(nsIContent* aBoundElement, nsIURI* aURI,
                      bool aPeekFlag, nsIPrincipal* aOriginPrincipal,
                      bool* aIsReady, nsXBLBinding** aResult,
                      nsTArray<nsIURI*>& aDontExtendURIs);


public:
  static bool gDisableChromeCache;
  static bool     gAllowDataURIs;            
                                             
                                             
};

#endif
