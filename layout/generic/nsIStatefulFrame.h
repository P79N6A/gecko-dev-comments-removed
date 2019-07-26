








#ifndef _nsIStatefulFrame_h
#define _nsIStatefulFrame_h

#include "nsQueryFrame.h"

class nsPresState;

class nsIStatefulFrame
{
 public: 
  NS_DECL_QUERYFRAME_TARGET(nsIStatefulFrame)

  
  
  NS_IMETHOD SaveState(nsPresState** aState) = 0;

  
  NS_IMETHOD RestoreState(nsPresState* aState) = 0;
};

#endif 
