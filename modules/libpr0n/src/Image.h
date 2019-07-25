




































#ifndef MOZILLA_IMAGELIB_IMAGE_H_
#define MOZILLA_IMAGELIB_IMAGE_H_

#include "imgIContainer.h"
#include "imgStatusTracker.h"
#include "prtypes.h"

namespace mozilla {
namespace imagelib {

class Image : public imgIContainer
{
public:
  imgStatusTracker& GetStatusTracker() { return mStatusTracker; }
  PRBool IsInitialized() const { return mInitialized; }

protected:
  Image();

  
  imgStatusTracker   mStatusTracker;
  PRPackedBool       mInitialized;   
};

} 
} 

#endif 
