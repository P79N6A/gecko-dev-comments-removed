





































#ifndef __NS_ISVGSVGFRAME_H__
#define __NS_ISVGSVGFRAME_H__

#include "nsISupports.h"


#define NS_ISVGSVGFRAME_IID \
{ 0xc38fdfc3, 0x7030, 0x47cb, { 0xba, 0x69, 0xd7, 0xc5, 0xf4, 0x5e, 0x65, 0x7c } }

class nsISVGSVGFrame : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGSVGFRAME_IID)

  NS_IMETHOD SuspendRedraw()=0;        
  NS_IMETHOD UnsuspendRedraw()=0;      
  NS_IMETHOD NotifyViewportChange()=0; 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGSVGFrame, NS_ISVGSVGFRAME_IID)

#endif 
