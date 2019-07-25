





































#ifndef MOZILLA_IMAGELIB_DECODER_H_
#define MOZILLA_IMAGELIB_DECODER_H_

#include "RasterImage.h"

#include "imgIDecoderObserver.h"

namespace mozilla {
namespace imagelib {

class Decoder
{
public:

  Decoder();
  virtual ~Decoder();

  



  







  nsresult Init(RasterImage* aImage, imgIDecoderObserver* aObserver);

  









  nsresult Write(const char* aBuffer, PRUint32 aCount);

  




  nsresult Finish();

  







  void FlushInvalidations();

  
  NS_INLINE_DECL_REFCOUNTING(Decoder)

  



  
  
  
  bool IsSizeDecode() { return mSizeDecode; };
  void SetSizeDecode(bool aSizeDecode)
  {
    NS_ABORT_IF_FALSE(!mInitialized, "Can't set size decode after Init()!");
    mSizeDecode = aSizeDecode;
  }

  
  
  PRUint32 GetFrameCount() { return mFrameCount; }

protected:

  



  virtual nsresult InitInternal();
  virtual nsresult WriteInternal(const char* aBuffer, PRUint32 aCount);
  virtual nsresult FinishInternal();

  



  
  
  void PostSize(PRInt32 aWidth, PRInt32 aHeight);

  
  
  void PostFrameStart();
  void PostFrameStop();

  
  
  void PostInvalidation(nsIntRect& aRect);

  




  nsRefPtr<RasterImage> mImage;
  nsCOMPtr<imgIDecoderObserver> mObserver;

  PRUint32 mFrameCount; 

  nsIntRect mInvalidRect; 

  bool mInitialized;
  bool mSizeDecode;
  bool mInFrame;
};

} 
} 

#endif 
