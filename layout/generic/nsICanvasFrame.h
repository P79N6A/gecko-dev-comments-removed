






































#ifndef nsICanvasFrame_h__
#define nsICanvasFrame_h__

#include "nsISupports.h"


#define NS_ICANVASFRAME_IID       \
{ 0x9df7db77, 0x49a2, 0x11d5, {0x97, 0x92, 0x0, 0x60, 0xb0, 0xfb, 0x99, 0x56} }

class nsICanvasFrame : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICANVASFRAME_IID)

  


  NS_IMETHOD SetHasFocus(PRBool aHasFocus) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasFrame, NS_ICANVASFRAME_IID)

#endif  

