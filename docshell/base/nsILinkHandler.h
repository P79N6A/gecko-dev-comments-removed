



































#ifndef nsILinkHandler_h___
#define nsILinkHandler_h___

#include "nsISupports.h"
#include "nsIContent.h"

class nsIInputStream;
class nsIDocShell;
class nsIRequest;
class nsString;
class nsGUIEvent;


#define NS_ILINKHANDLER_IID \
  { 0xd85670a1, 0x224a, 0x4562, \
    { 0x87, 0xa9, 0x43, 0xa5, 0x24, 0xe7, 0xd0, 0x1b } }




class nsILinkHandler : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINKHANDLER_IID)

  










  NS_IMETHOD OnLinkClick(nsIContent* aContent, 
                         nsIURI* aURI,
                         const PRUnichar* aTargetSpec,
                         nsIInputStream* aPostDataStream,
                         nsIInputStream* aHeadersDataStream,
                         bool aIsTrusted) = 0;

  














  NS_IMETHOD OnLinkClickSync(nsIContent* aContent, 
                             nsIURI* aURI,
                             const PRUnichar* aTargetSpec,
                             nsIInputStream* aPostDataStream = 0,
                             nsIInputStream* aHeadersDataStream = 0,
                             nsIDocShell** aDocShell = 0,
                             nsIRequest** aRequest = 0) = 0;

  







  NS_IMETHOD OnOverLink(nsIContent* aContent, 
                        nsIURI* aURLSpec,
                        const PRUnichar* aTargetSpec) = 0;

  


  NS_IMETHOD OnLeaveLink() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILinkHandler, NS_ILINKHANDLER_IID)

#endif 
