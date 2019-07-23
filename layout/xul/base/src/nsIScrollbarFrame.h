





































#ifndef nsIScrollbarFrame_h___
#define nsIScrollbarFrame_h___

#include "nsQueryFrame.h"

class nsIScrollbarMediator;

class nsIScrollbarFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIScrollbarFrame)
  
  
  
  virtual void SetScrollbarMediatorContent(nsIContent* aMediator) = 0;

  
  virtual nsIScrollbarMediator* GetScrollbarMediator() = 0;
};

#endif
