





































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
  ~Decoder();

  



  







  nsresult Init(RasterImage* aImage, imgIDecoderObserver* aObserver);

  









  
  
  

  




  nsresult Finish();

  






  nsresult Shutdown(PRUint32 aFlags);

  
  
  

  



  
  
  
  bool IsSizeDecode() { return mSizeDecode; };
  void SetSizeDecode(bool aSizeDecode)
  {
    NS_ABORT_IF_FALSE(!mInitialized, "Can't set size decode after Init()!");
    mSizeDecode = aSizeDecode;
  }

protected:

  



  virtual nsresult InitInternal();
  virtual nsresult WriteInternal(const char* aBuffer, PRUint32 aCount);
  virtual nsresult FinishInternal();
  virtual nsresult ShutdownInternal(PRUint32 aFlags);

  




  nsRefPtr<RasterImage> mImage;
  nsCOMPtr<imgIDecoderObserver> mObserver;

  bool mInitialized;
  bool mSizeDecode;
};

} 
} 

#endif 
