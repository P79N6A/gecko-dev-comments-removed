




#ifndef _nsIStatefulFrame_h
#define _nsIStatefulFrame_h

#include "nsQueryFrame.h"

class nsPresContext;
class nsPresState;

class nsIStatefulFrame
{
 public: 
  NS_DECL_QUERYFRAME_TARGET(nsIStatefulFrame)

  
  
  
  enum SpecialStateID {eNoID=0, eDocumentScrollState};

  
  
  
  
  NS_IMETHOD SaveState(SpecialStateID aStateID, nsPresState** aState) = 0;

  
  NS_IMETHOD RestoreState(nsPresState* aState) = 0;
};

#endif 
