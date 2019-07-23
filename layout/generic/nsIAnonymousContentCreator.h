









































#ifndef nsIAnonymousContentCreator_h___
#define nsIAnonymousContentCreator_h___

#include "nsISupports.h"
#include "nsIContent.h"

class nsPresContext;
class nsIFrame;
template <class T> class nsTArray;


#define NS_IANONYMOUS_CONTENT_CREATOR_IID \
{ 0x7568a516, 0x3831, 0x4db4, \
  { 0x88, 0xa7, 0xa4, 0x25, 0x78, 0xac, 0xc1, 0x36 } }









class nsIAnonymousContentCreator : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IANONYMOUS_CONTENT_CREATOR_IID)

  








  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements)=0;

  





  virtual nsIFrame* CreateFrameFor(nsIContent* aContent) { return nsnull; }

  



  virtual void PostCreateFrames() {}
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAnonymousContentCreator,
                              NS_IANONYMOUS_CONTENT_CREATOR_IID)

#endif

