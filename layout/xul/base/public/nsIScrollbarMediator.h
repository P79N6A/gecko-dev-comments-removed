






































#ifndef nsIScrollbarMediator_h___
#define nsIScrollbarMediator_h___

#include "nsQueryFrame.h"

class nsIScrollbarFrame;

class nsIScrollbarMediator
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIScrollbarMediator)

  
  
  

  NS_IMETHOD PositionChanged(nsIScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex) = 0;
  NS_IMETHOD ScrollbarButtonPressed(nsIScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex) = 0;

  NS_IMETHOD VisibilityChanged(PRBool aVisible) = 0;
};

#endif

