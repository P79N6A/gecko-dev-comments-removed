









































#ifndef nsIAnonymousContentCreator_h___
#define nsIAnonymousContentCreator_h___

#include "nsQueryFrame.h"
#include "nsIContent.h"

class nsIFrame;
template <class T> class nsTArray;







class nsIAnonymousContentCreator
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsIAnonymousContentCreator)

  








  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements)=0;

  





  virtual nsIFrame* CreateFrameFor(nsIContent* aContent) { return nsnull; }
};

#endif

