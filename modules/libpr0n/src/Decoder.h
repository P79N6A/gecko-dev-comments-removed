





































#ifndef MOZILLA_IMAGELIB_DECODER_H_
#define MOZILLA_IMAGELIB_DECODER_H_

#include "RasterImage.h"

#include "imgIDecoderObserver.h"
#include "imgIDecoder.h"

namespace mozilla {
namespace imagelib {


class Decoder : public imgIDecoder
{
public:

  
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODER

  Decoder();
  virtual ~Decoder();

  



  








  nsresult Init(RasterImage* aImage, imgIDecoderObserver* aObserver,
                PRUint32 aFlags);

  









  
  
  

  




  nsresult Finish();

  






  nsresult Shutdown(PRUint32 aFlags);

  
  
  

protected:

  



  virtual nsresult InitInternal();
  virtual nsresult WriteInternal(const char* aBuffer, PRUint32 aCount);
  virtual nsresult FinishInternal();
  virtual nsresult ShutdownInternal(PRUint32 aFlags);

  




  nsRefPtr<RasterImage> mImage;
  nsCOMPtr<imgIDecoderObserver> mObserver;
  PRUint32 mFlags;

  bool mInitialized;
};

} 
} 

#endif 
