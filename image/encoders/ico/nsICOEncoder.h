



#ifndef mozilla_image_encoders_ico_nsICOEncoder_h
#define mozilla_image_encoders_ico_nsICOEncoder_h

#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"

#include "imgIEncoder.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "ICOFileHeaders.h"

class nsBMPEncoder;
class nsPNGEncoder;

#define NS_ICOENCODER_CID \
{ /*92AE3AB2-8968-41B1-8709-B6123BCEAF21 */          \
     0x92ae3ab2,                                     \
     0x8968,                                         \
     0x41b1,                                         \
    {0x87, 0x09, 0xb6, 0x12, 0x3b, 0Xce, 0xaf, 0x21} \
}




class nsICOEncoder final : public imgIEncoder
{
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_IMGIENCODER
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSIASYNCINPUTSTREAM

  nsICOEncoder();

  
  uint32_t GetRealWidth() const
  {
    return mICODirEntry.mWidth == 0 ? 256 : mICODirEntry.mWidth;
  }

  
  uint32_t GetRealHeight() const
  {
    return mICODirEntry.mHeight == 0 ? 256 : mICODirEntry.mHeight;
  }

protected:
  ~nsICOEncoder();

  nsresult ParseOptions(const nsAString& aOptions, uint32_t* bpp,
                        bool* usePNG);
  void NotifyListener();

  
  void InitFileHeader();
  
  void InitInfoHeader(uint32_t aBPP, uint8_t aWidth, uint8_t aHeight);
  
  void EncodeFileHeader();
  
  void EncodeInfoHeader();
  
  inline int32_t GetCurrentImageBufferOffset()
  {
    return static_cast<int32_t>(mImageBufferCurr - mImageBufferStart);
  }

  
  
  nsCOMPtr<imgIEncoder> mContainedEncoder;

  
  
  
  mozilla::image::IconFileHeader mICOFileHeader;
  mozilla::image::IconDirEntry mICODirEntry;

  
  uint8_t* mImageBufferStart;
  
  uint8_t* mImageBufferCurr;
  
  uint32_t mImageBufferSize;
  
  uint32_t mImageBufferReadPoint;
  
  bool mFinished;
  
  bool mUsePNG;

  nsCOMPtr<nsIInputStreamCallback> mCallback;
  nsCOMPtr<nsIEventTarget> mCallbackTarget;
  uint32_t mNotifyThreshold;
};

#endif 
