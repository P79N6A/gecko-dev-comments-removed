











































#ifndef nsIXBLService_h__
#define nsIXBLService_h__

#include "nsISupports.h"

class nsIContent;
class nsIDocument;
class nsIDOMEventReceiver;
class nsIDOMNodeList;
class nsXBLBinding;
class nsIXBLDocumentInfo;
class nsIURI;
class nsIAtom;

#define NS_IXBLSERVICE_IID      \
  { 0x7157b300, 0xf49b, 0x4e7d, \
    { 0xac, 0x3a, 0xef, 0x8f, 0x20, 0x69, 0x6e, 0xb1 } }

class nsIXBLService : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXBLSERVICE_IID)

  
  
  NS_IMETHOD LoadBindings(nsIContent* aContent, nsIURI* aURL, PRBool aAugmentFlag,
                          nsXBLBinding** aBinding, PRBool* aResolveStyle) = 0;

  
  NS_IMETHOD BindingReady(nsIContent* aBoundElement, nsIURI* aURI, PRBool* aIsReady) = 0;

  
  NS_IMETHOD ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID, nsIAtom** aResult) = 0;

  
  NS_IMETHOD LoadBindingDocumentInfo(nsIContent* aBoundElement, nsIDocument* aBoundDocument,
                                     nsIURI* aBindingURI,
                                     PRBool aForceSyncLoad, nsIXBLDocumentInfo** aResult) = 0;

  
  NS_IMETHOD AttachGlobalKeyHandler(nsIDOMEventReceiver* aElement)=0;
  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXBLService, NS_IXBLSERVICE_IID)

#endif 
