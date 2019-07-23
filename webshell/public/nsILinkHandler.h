



































#ifndef nsILinkHandler_h___
#define nsILinkHandler_h___

#include "nsISupports.h"

class nsIInputStream;
class nsIDocShell;
class nsIRequest;
class nsIContent;
class nsString;
class nsGUIEvent;


#define NS_ILINKHANDLER_IID \
 { 0x514bc565, 0x8d38, 0x4dde,{0xb4, 0xeb, 0xe7, 0xb5, 0x01, 0x2b, 0xf4, 0x64}}

enum nsLinkState {
  eLinkState_Unknown    = 0,
  eLinkState_Unvisited  = 1,
  eLinkState_Visited    = 2, 
  eLinkState_NotLink    = 3
};




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
      

  


  NS_IMETHOD GetLinkState(nsIURI* aLinkURI, nsLinkState& aState) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILinkHandler, NS_ILINKHANDLER_IID)

#endif 
