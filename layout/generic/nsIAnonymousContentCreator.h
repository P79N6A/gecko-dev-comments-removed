









#ifndef nsIAnonymousContentCreator_h___
#define nsIAnonymousContentCreator_h___

#include "nsQueryFrame.h"
#include "nsStyleContext.h"
#include "nsTArrayForwardDeclare.h"

class nsIContent;
class nsIFrame;







class nsIAnonymousContentCreator
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIAnonymousContentCreator)

  struct ContentInfo {
    explicit ContentInfo(nsIContent* aContent) :
      mContent(aContent)
    {}

    ContentInfo(nsIContent* aContent, nsStyleContext* aStyleContext) :
      mContent(aContent), mStyleContext(aStyleContext)
    {}

    nsIContent* mContent;
    nsRefPtr<nsStyleContext> mStyleContext;
    nsTArray<ContentInfo> mChildren;
  };

  
















  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements)=0;

  





  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) = 0;

  





  virtual nsIFrame* CreateFrameFor(nsIContent* aContent) { return nullptr; }
};

#endif

