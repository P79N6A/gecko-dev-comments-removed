









































#ifndef nsIImageFrame_h___
#define nsIImageFrame_h___

#include "nsISupports.h"
struct nsSize;
class imgIRequest;
class nsPresContext;
class nsIImageMap;


#define NS_IIMAGEFRAME_IID \
{ 0xb261a0d5, 0xe696, 0x11d4, { 0x98, 0x85, 0x0, 0xc0, 0x4f, 0xa0, 0xcf, 0x4b } }

class nsIImageFrame : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IIMAGEFRAME_IID)

  NS_IMETHOD GetIntrinsicImageSize(nsSize& aSize) = 0;

  NS_IMETHOD GetImageMap(nsPresContext *aPresContext, nsIImageMap **aImageMap) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIImageFrame, NS_IIMAGEFRAME_IID)

#endif 
