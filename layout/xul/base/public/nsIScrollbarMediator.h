






































#ifndef nsIScrollbarMediator_h___
#define nsIScrollbarMediator_h___

#include "nsQueryFrame.h"

class nsScrollbarFrame;

class nsIScrollbarMediator
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIScrollbarMediator)

  
  NS_IMETHOD PositionChanged(nsScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex) = 0;
  NS_IMETHOD ScrollbarButtonPressed(nsScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex) = 0;

  NS_IMETHOD VisibilityChanged(bool aVisible) = 0;
};

#endif

