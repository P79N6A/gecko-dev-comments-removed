






































#ifndef nsICanvasFrame_h__
#define nsICanvasFrame_h__

#include "nsQueryFrame.h"

class nsICanvasFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsICanvasFrame)

  


  NS_IMETHOD SetHasFocus(PRBool aHasFocus) = 0;

};

#endif  

