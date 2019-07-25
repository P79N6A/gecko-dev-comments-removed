






































#ifndef mozilla_IHistory_h_
#define mozilla_IHistory_h_

#include "nsISupports.h"

class nsIURI;

namespace mozilla {

    namespace dom {
        class Link;
    }

#define IHISTORY_IID \
  {0xaf27265d, 0x5672, 0x4d23, {0xa0, 0x75, 0x34, 0x8e, 0xb9, 0x73, 0x5a, 0x9a}}

class IHistory : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IHISTORY_IID)

    





















    NS_IMETHOD RegisterVisitedCallback(nsIURI *aURI, dom::Link *aLink) = 0;

    











    NS_IMETHOD UnregisterVisitedCallback(nsIURI *aURI, dom::Link *aLink) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(IHistory, IHISTORY_IID)

#define NS_DECL_IHISTORY \
    NS_IMETHOD RegisterVisitedCallback(nsIURI *aURI, \
                                       mozilla::dom::Link *aContent); \
    NS_IMETHOD UnregisterVisitedCallback(nsIURI *aURI, \
                                         mozilla::dom::Link *aContent);

} 

#endif 
