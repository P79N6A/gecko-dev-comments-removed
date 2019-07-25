





































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

  



  







  nsresult Init(RasterImage* aImage, imgIDecoderObserver* aObserver);

  









  
  
  

  




  nsresult Finish();

  
  
  

  



  
  
  
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


  




  nsRefPtr<RasterImage> mImage;
  nsCOMPtr<imgIDecoderObserver> mObserver;

  PRUint32 mFrameCount; 

  bool mInitialized;
  bool mSizeDecode;
  bool mInFrame;
};

} 
} 

#endif 
