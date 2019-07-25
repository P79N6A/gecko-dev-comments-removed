









































#ifndef nsIAnonymousContentCreator_h___
#define nsIAnonymousContentCreator_h___

#include "nsQueryFrame.h"
#include "nsIContent.h"
#include "nsStyleContext.h"

class nsIFrame;
template <class T, class A> class nsTArray;







class nsIAnonymousContentCreator
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIAnonymousContentCreator)

  struct ContentInfo {
    ContentInfo(nsIContent* aContent) :
      mContent(aContent)
    {}

    ContentInfo(nsIContent* aContent, nsStyleContext* aStyleContext) :
      mContent(aContent), mStyleContext(aStyleContext)
    {}

    nsIContent* mContent;
    nsRefPtr<nsStyleContext> mStyleContext;
  };

  








  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements)=0;

  





  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter) = 0;

  





  virtual nsIFrame* CreateFrameFor(nsIContent* aContent) { return nsnull; }
};

#endif

