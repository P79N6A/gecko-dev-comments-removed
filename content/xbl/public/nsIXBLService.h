











































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
{ 0x8d3b37f5, 0xde7e, 0x4595,   \
 { 0xb8, 0x56, 0xf7, 0x11, 0xe8, 0xe7, 0xb5, 0x59 } }

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
  NS_IMETHOD DetachGlobalKeyHandler(nsPIDOMEventTarget* aTarget)=0;
  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXBLService, NS_IXBLSERVICE_IID)

#endif 
