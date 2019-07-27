





#ifndef mozilla_IHistory_h_
#define mozilla_IHistory_h_

#include "nsISupports.h"

class nsIURI;
class nsString;

namespace mozilla {

namespace dom {
class Link;
}


#define IHISTORY_IID \
  {0x0057c9d3, 0xb98e, 0x4933, {0xbd, 0xc5, 0x02, 0x75, 0xd0, 0x67, 0x05, 0xe1}}

class IHistory : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(IHISTORY_IID)

  





















  NS_IMETHOD RegisterVisitedCallback(nsIURI* aURI, dom::Link* aLink) = 0;

  











  NS_IMETHOD UnregisterVisitedCallback(nsIURI* aURI, dom::Link* aLink) = 0;

  enum VisitFlags
  {
    


    TOP_LEVEL = 1 << 0,
    


    REDIRECT_PERMANENT = 1 << 1,
    


    REDIRECT_TEMPORARY = 1 << 2,
    


    REDIRECT_SOURCE = 1 << 3,
    



    UNRECOVERABLE_ERROR = 1 << 4
  };

  











  NS_IMETHOD VisitURI(nsIURI* aURI,
                      nsIURI* aLastVisitedURI,
                      uint32_t aFlags) = 0;

  









  NS_IMETHOD SetURITitle(nsIURI* aURI, const nsAString& aTitle) = 0;

  





  NS_IMETHOD NotifyVisited(nsIURI* aURI) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(IHistory, IHISTORY_IID)

#define NS_DECL_IHISTORY \
  NS_IMETHOD RegisterVisitedCallback(nsIURI* aURI, \
                                     mozilla::dom::Link* aContent) MOZ_OVERRIDE; \
  NS_IMETHOD UnregisterVisitedCallback(nsIURI* aURI, \
                                       mozilla::dom::Link* aContent) MOZ_OVERRIDE; \
  NS_IMETHOD VisitURI(nsIURI* aURI, \
                      nsIURI* aLastVisitedURI, \
                      uint32_t aFlags) MOZ_OVERRIDE; \
  NS_IMETHOD SetURITitle(nsIURI* aURI, const nsAString& aTitle) MOZ_OVERRIDE; \
  NS_IMETHOD NotifyVisited(nsIURI* aURI) MOZ_OVERRIDE;

} 

#endif 
