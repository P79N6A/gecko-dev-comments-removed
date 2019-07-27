



#ifndef mozilla_image_encoders_bmp_nsBMPEncoder_h
#define mozilla_image_encoders_bmp_nsBMPEncoder_h

#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"

#include "imgIEncoder.h"
#include "BMPFileHeaders.h"

#include "nsCOMPtr.h"

#define NS_BMPENCODER_CID \
{ /* 13a5320c-4c91-4FA4-bd16-b081a3ba8c0b */         \
     0x13a5320c,                                     \
     0x4c91,                                         \
     0x4fa4,                                         \
    {0xbd, 0x16, 0xb0, 0x81, 0xa3, 0Xba, 0x8c, 0x0b} \
}




class nsBMPEncoder final : public imgIEncoder
{
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_IMGIENCODER
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSIASYNCINPUTSTREAM

  nsBMPEncoder();

protected:
  ~nsBMPEncoder();

  enum Version
  {
      VERSION_3 = 3,
      VERSION_5 = 5
  };

  
  nsresult ParseOptions(const nsAString& aOptions, Version* version,
                        uint32_t* bpp);
  
  void ConvertHostARGBRow(const uint8_t* aSrc, uint8_t* aDest,
                          uint32_t aPixelWidth);
  
  void NotifyListener();

  
  void InitFileHeader(Version aVersion, uint32_t aBPP, uint32_t aWidth,
                      uint32_t aHeight);
  
  void InitInfoHeader(Version aVersion, uint32_t aBPP, uint32_t aWidth,
                      uint32_t aHeight);

  
  void EncodeFileHeader();
  
  void EncodeInfoHeader();
  
  void EncodeImageDataRow24(const uint8_t* aData);
  
  void EncodeImageDataRow32(const uint8_t* aData);
  
  inline int32_t GetCurrentImageBufferOffset()
  {
    return static_cast<int32_t>(mImageBufferCurr - mImageBufferStart);
  }

  
  
  mozilla::image::BMPFILEHEADER mBMPFileHeader;
  mozilla::image::BITMAPV5HEADER mBMPInfoHeader;

  
  uint8_t* mImageBufferStart;
  
  uint8_t* mImageBufferCurr;
  
  uint32_t mImageBufferSize;
  
  uint32_t mImageBufferReadPoint;
  
  bool mFinished;

  nsCOMPtr<nsIInputStreamCallback> mCallback;
  nsCOMPtr<nsIEventTarget> mCallbackTarget;
  uint32_t mNotifyThreshold;
};

#endif 
