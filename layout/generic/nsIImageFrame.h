









































#ifndef nsIImageFrame_h___
#define nsIImageFrame_h___

struct nsSize;
class imgIRequest;
class nsPresContext;
class nsIImageMap;

class nsIImageFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsIImageFrame)

  NS_IMETHOD GetIntrinsicImageSize(nsSize& aSize) = 0;

  NS_IMETHOD GetImageMap(nsPresContext *aPresContext, nsIImageMap **aImageMap) = 0;
};

#endif 
