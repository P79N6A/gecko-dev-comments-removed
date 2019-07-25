






































#ifndef mozilla_IHistory_h_
#define mozilla_IHistory_h_

#include "nsISupports.h"

class nsIURI;

namespace mozilla {

    namespace dom {
        class Link;
    }

#define IHISTORY_IID \
  {0x6f736049, 0x6370, 0x4376, {0xb7, 0x17, 0xfa, 0xfc, 0x0b, 0x4f, 0xd0, 0xf1}}

class IHistory : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IHISTORY_IID)

    





















    NS_IMETHOD RegisterVisitedCallback(nsIURI *aURI, dom::Link *aLink) = 0;

    











    NS_IMETHOD UnregisterVisitedCallback(nsIURI *aURI, dom::Link *aLink) = 0;

    enum VisitFlags {
        


        TOP_LEVEL = 1 << 0,
        


        REDIRECT_PERMANENT = 1 << 1,
        


        REDIRECT_TEMPORARY = 1 << 2
    };

    











    NS_IMETHOD VisitURI(
        nsIURI *aURI,
        nsIURI *aLastVisitedURI,
        PRUint32 aFlags
    ) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(IHistory, IHISTORY_IID)

#define NS_DECL_IHISTORY \
    NS_IMETHOD RegisterVisitedCallback(nsIURI *aURI, \
                                       mozilla::dom::Link *aContent); \
    NS_IMETHOD UnregisterVisitedCallback(nsIURI *aURI, \
                                         mozilla::dom::Link *aContent); \
    NS_IMETHOD VisitURI(nsIURI *aURI, \
                        nsIURI *aLastVisitedURI, \
                        PRUint32 aFlags);

} 

#endif 
