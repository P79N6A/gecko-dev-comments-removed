






































#ifndef nsICanvasFrame_h__
#define nsICanvasFrame_h__

#include "nsQueryFrame.h"

class nsICanvasFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsICanvasFrame)

  


  NS_IMETHOD SetHasFocus(PRBool aHasFocus) = 0;

};

#endif  

