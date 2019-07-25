




































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

  















  static const PRUint32 INIT_FLAG_NONE           = 0x0;
  static const PRUint32 INIT_FLAG_DISCARDABLE    = 0x1;
  static const PRUint32 INIT_FLAG_DECODE_ON_DRAW = 0x2;
  static const PRUint32 INIT_FLAG_MULTIPART      = 0x4;

  






  virtual nsresult Init(imgIDecoderObserver* aObserver,
                        const char* aMimeType,
                        PRUint32 aFlags) = 0;

  




  virtual nsresult GetCurrentFrameRect(nsIntRect& aRect) = 0;

  



  virtual PRUint32 GetCurrentFrameIndex() = 0;

  


  virtual PRUint32 GetNumFrames() = 0;

  



  virtual PRUint32 GetDataSize() = 0;

  
  enum eDecoderType {
    eDecoderType_png     = 0,
    eDecoderType_gif     = 1,
    eDecoderType_jpeg    = 2,
    eDecoderType_bmp     = 3,
    eDecoderType_ico     = 4,
    eDecoderType_icon    = 5,
    eDecoderType_unknown = 6
  };
  static eDecoderType GetDecoderType(const char *aMimeType);

protected:
  Image();

  
  imgStatusTracker   mStatusTracker;
  PRPackedBool       mInitialized;   
};

} 
} 

#endif 
