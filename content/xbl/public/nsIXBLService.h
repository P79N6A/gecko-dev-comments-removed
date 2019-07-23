











































#ifndef nsIXBLService_h__
#define nsIXBLService_h__

#include "nsISupports.h"

class nsIContent;
class nsIDocument;
class nsPIDOMEventTarget;
class nsIDOMNodeList;
class nsXBLBinding;
class nsIXBLDocumentInfo;
class nsIURI;
class nsIAtom;
class nsIPrincipal;

#define NS_IXBLSERVICE_IID      \
{ 0x98b28f4e, 0x698f, 0x4f77,   \
 { 0xa8, 0x9e, 0x65, 0xf5, 0xd0, 0xde, 0x6a, 0xbf } }

class nsIXBLService : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXBLSERVICE_IID)

  
  
  NS_IMETHOD LoadBindings(nsIContent* aContent, nsIURI* aURL,
                          nsIPrincipal* aOriginPrincipal, PRBool aAugmentFlag,
                          nsXBLBinding** aBinding, PRBool* aResolveStyle) = 0;

  
  NS_IMETHOD BindingReady(nsIContent* aBoundElement, nsIURI* aURI, PRBool* aIsReady) = 0;

  
  NS_IMETHOD ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID, nsIAtom** aResult) = 0;

  
  
  
  NS_IMETHOD LoadBindingDocumentInfo(nsIContent* aBoundElement,
                                     nsIDocument* aBoundDocument,
                                     nsIURI* aBindingURI,
                                     nsIPrincipal* aOriginPrincipal,
                                     PRBool aForceSyncLoad,
                                     nsIXBLDocumentInfo** aResult) = 0;

  
  NS_IMETHOD AttachGlobalKeyHandler(nsPIDOMEventTarget* aTarget)=0;
  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXBLService, NS_IXBLSERVICE_IID)

#endif 
