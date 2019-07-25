



































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
 { 0x71627c30, 0xd3c5, 0x4ad0,{0xb5, 0x33, 0x6e, 0x01, 0x91, 0xf2, 0x79, 0x32}}




class nsILinkHandler : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINKHANDLER_IID)

  









  NS_IMETHOD OnLinkClick(nsIContent* aContent, 
                         nsIURI* aURI,
                         const PRUnichar* aTargetSpec,
                         nsIInputStream* aPostDataStream = 0,
                         nsIInputStream* aHeadersDataStream = 0) = 0;

  














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
