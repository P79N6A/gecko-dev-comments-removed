





































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

  







  void Init(RasterImage* aImage, imgIDecoderObserver* aObserver);

  









  void Write(const char* aBuffer, PRUint32 aCount);

  




  void Finish();

  







  void FlushInvalidations();

  
  NS_INLINE_DECL_REFCOUNTING(Decoder)

  



  
  
  
  bool IsSizeDecode() { return mSizeDecode; };
  void SetSizeDecode(bool aSizeDecode)
  {
    NS_ABORT_IF_FALSE(!mInitialized, "Can't set size decode after Init()!");
    mSizeDecode = aSizeDecode;
  }

  
  
  PRUint32 GetFrameCount() { return mFrameCount; }

  
  PRUint32 GetCompleteFrameCount() { return mInFrame ? mFrameCount - 1 : mFrameCount; }

  
  bool HasError() { return HasDataError() || HasDecoderError(); };
  bool HasDataError() { return mDataError; };
  bool HasDecoderError() { return NS_FAILED(mFailCode); };
  nsresult GetDecoderError() { return mFailCode; };
  void PostResizeError() { PostDataError(); }

  
  
  
  enum {
    DECODER_NO_PREMULTIPLY_ALPHA = 0x2,
    DECODER_NO_COLORSPACE_CONVERSION = 0x4
  };
  void SetDecodeFlags(PRUint32 aFlags) { mDecodeFlags = aFlags; }
  PRUint32 GetDecodeFlags() { return mDecodeFlags; }

protected:

  



  virtual void InitInternal();
  virtual void WriteInternal(const char* aBuffer, PRUint32 aCount);
  virtual void FinishInternal();

  



  
  
  void PostSize(PRInt32 aWidth, PRInt32 aHeight);

  
  
  void PostFrameStart();
  void PostFrameStop();

  
  
  void PostInvalidation(nsIntRect& aRect);

  
  
  
  
  
  void PostDecodeDone();

  
  void PostDataError();
  void PostDecoderError(nsresult aFailCode);

  



  nsRefPtr<RasterImage> mImage;

  PRUint32 mDecodeFlags;

private:
  nsCOMPtr<imgIDecoderObserver> mObserver;

  PRUint32 mFrameCount; 

  nsIntRect mInvalidRect; 

  nsresult mFailCode;

  bool mInitialized;
  bool mSizeDecode;
  bool mInFrame;
  bool mDecodeDone;
  bool mDataError;
};

} 
} 

#endif 
