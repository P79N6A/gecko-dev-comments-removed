




































#ifndef nsHtml5SVGLoadDispatcher_h_
#define nsHtml5SVGLoadDispatcher_h_

#include "nsThreadUtils.h"
#include "nsIContent.h"

class nsHtml5SVGLoadDispatcher : public nsRunnable
{
  private:
    nsCOMPtr<nsIContent> mElement;
    nsCOMPtr<nsIDocument> mDocument;
  public:
    nsHtml5SVGLoadDispatcher(nsIContent* aElement);
    NS_IMETHOD Run();
};

#endif 
