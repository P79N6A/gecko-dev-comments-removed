





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
  {0x6f733924, 0x6321, 0x4384, {0x01, 0xee, 0x8e, 0x7d, 0xfb, 0xde, 0xe7, 0xa8}}

class IHistory : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IHISTORY_IID)

    





















    NS_IMETHOD RegisterVisitedCallback(nsIURI *aURI, dom::Link *aLink) = 0;

    











    NS_IMETHOD UnregisterVisitedCallback(nsIURI *aURI, dom::Link *aLink) = 0;

    enum VisitFlags {
        


        TOP_LEVEL = 1 << 0,
        


        REDIRECT_PERMANENT = 1 << 1,
        


        REDIRECT_TEMPORARY = 1 << 2,
        


        REDIRECT_SOURCE = 1 << 3,
        



        UNRECOVERABLE_ERROR = 1 << 4
    };

    











    NS_IMETHOD VisitURI(
        nsIURI *aURI,
        nsIURI *aLastVisitedURI,
        uint32_t aFlags
    ) = 0;

    









    NS_IMETHOD SetURITitle(nsIURI* aURI, const nsAString& aTitle) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(IHistory, IHISTORY_IID)

#define NS_DECL_IHISTORY \
    NS_IMETHOD RegisterVisitedCallback(nsIURI *aURI, \
                                       mozilla::dom::Link *aContent); \
    NS_IMETHOD UnregisterVisitedCallback(nsIURI *aURI, \
                                         mozilla::dom::Link *aContent); \
    NS_IMETHOD VisitURI(nsIURI *aURI, \
                        nsIURI *aLastVisitedURI, \
                        uint32_t aFlags); \
    NS_IMETHOD SetURITitle(nsIURI* aURI, const nsAString& aTitle);

} 

#endif 
