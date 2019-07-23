






































#ifndef nsIImageMap_h___
#define nsIImageMap_h___

#include "nsISupports.h"
class nsIContent;
struct nsRect;

#define NS_IIMAGEMAP_IID \
{ 0x2fca3d7e, 0x5b1f, 0x4ecf, \
 { 0xb5, 0x7a, 0x84, 0x24, 0x97, 0x81, 0x2e, 0x62 } }

class nsIImageMap : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IIMAGEMAP_IID)

  NS_IMETHOD GetBoundsForAreaContent(nsIContent *aContent, 
                                     nsRect& aBounds) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIImageMap, NS_IIMAGEMAP_IID)

#endif 
