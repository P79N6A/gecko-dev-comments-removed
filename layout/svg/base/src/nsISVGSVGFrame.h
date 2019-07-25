





































#ifndef __NS_ISVGSVGFRAME_H__
#define __NS_ISVGSVGFRAME_H__

#include "nsQueryFrame.h"

class nsISVGSVGFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsISVGSVGFrame)

  virtual void SuspendRedraw()=0;        
  virtual void UnsuspendRedraw()=0;      
  virtual void NotifyViewportChange()=0; 
};

#endif 
