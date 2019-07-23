





































#ifndef __NS_ISVGSVGFRAME_H__
#define __NS_ISVGSVGFRAME_H__

#include "nsQueryFrame.h"

class nsISVGSVGFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsISVGSVGFrame)

  NS_IMETHOD SuspendRedraw()=0;        
  NS_IMETHOD UnsuspendRedraw()=0;      
  NS_IMETHOD NotifyViewportChange()=0; 
};

#endif 
