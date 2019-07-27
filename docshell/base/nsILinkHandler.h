




#ifndef nsILinkHandler_h___
#define nsILinkHandler_h___

#include "nsISupports.h"
#include "mozilla/EventForwards.h"

class nsIContent;
class nsIDocShell;
class nsIInputStream;
class nsIRequest;


#define NS_ILINKHANDLER_IID \
  { 0xceb9aade, 0x43da, 0x4f1a, \
    { 0xac, 0x8a, 0xc7, 0x09, 0xfb, 0x22, 0x46, 0x64 } }




class nsILinkHandler : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINKHANDLER_IID)

  











  NS_IMETHOD OnLinkClick(nsIContent* aContent,
                         nsIURI* aURI,
                         const char16_t* aTargetSpec,
                         const nsAString& aFileName,
                         nsIInputStream* aPostDataStream,
                         nsIInputStream* aHeadersDataStream,
                         bool aIsTrusted) = 0;

  















  NS_IMETHOD OnLinkClickSync(nsIContent* aContent,
                             nsIURI* aURI,
                             const char16_t* aTargetSpec,
                             const nsAString& aFileName,
                             nsIInputStream* aPostDataStream = 0,
                             nsIInputStream* aHeadersDataStream = 0,
                             nsIDocShell** aDocShell = 0,
                             nsIRequest** aRequest = 0) = 0;

  







  NS_IMETHOD OnOverLink(nsIContent* aContent,
                        nsIURI* aURLSpec,
                        const char16_t* aTargetSpec) = 0;

  


  NS_IMETHOD OnLeaveLink() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILinkHandler, NS_ILINKHANDLER_IID)

#endif 
