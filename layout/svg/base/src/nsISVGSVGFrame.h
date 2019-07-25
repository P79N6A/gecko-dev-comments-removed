




#ifndef __NS_ISVGSVGFRAME_H__
#define __NS_ISVGSVGFRAME_H__

#include "nsQueryFrame.h"

class nsISVGSVGFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsISVGSVGFrame)

  






  virtual void NotifyViewportOrTransformChanged(uint32_t aFlags)=0; 
};

#endif 
