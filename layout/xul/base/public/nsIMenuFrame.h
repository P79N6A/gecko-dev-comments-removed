




































#ifndef nsIMenuFrame_h___
#define nsIMenuFrame_h___

#include "nsQueryFrame.h"




class nsIMenuFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsIMenuFrame)

  virtual PRBool IsOpen() = 0;
  virtual PRBool IsMenu() = 0;
  virtual PRBool IsOnMenuBar() = 0;
  virtual PRBool IsOnActiveMenuBar() = 0;
};

#endif

