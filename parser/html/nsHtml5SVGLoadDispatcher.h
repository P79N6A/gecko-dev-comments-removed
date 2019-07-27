



#ifndef nsHtml5SVGLoadDispatcher_h
#define nsHtml5SVGLoadDispatcher_h

#include "nsThreadUtils.h"
#include "nsIContent.h"

class nsHtml5SVGLoadDispatcher : public nsRunnable
{
  private:
    nsCOMPtr<nsIContent> mElement;
    nsCOMPtr<nsIDocument> mDocument;
  public:
    explicit nsHtml5SVGLoadDispatcher(nsIContent* aElement);
    NS_IMETHOD Run();
};

#endif 
