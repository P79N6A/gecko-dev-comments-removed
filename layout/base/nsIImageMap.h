






































#ifndef nsIImageMap_h___
#define nsIImageMap_h___

#include "nsISupports.h"
class nsIContent;
struct nsRect;

#define NS_IIMAGEMAP_IID \
{ 0x36a48085, 0xc213, 0x4464, { 0xab, 0x60, 0x41, 0x2e, 0xc8, 0xe8, 0xb1, 0xfe } }


class nsIImageMap : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IIMAGEMAP_IID)

  NS_IMETHOD GetBoundsForAreaContent(nsIContent *aContent, 
                                     nsPresContext* aPresContext, 
                                     nsRect& aBounds) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIImageMap, NS_IIMAGEMAP_IID)

#endif 
