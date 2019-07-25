




































#include "imgIEncoder.h"
#include "BMPFileHeaders.h"

#include "mozilla/ReentrantMonitor.h"

#include "nsCOMPtr.h"

#define NS_BMPENCODER_CID \
{ /* 13a5320c-4c91-4FA4-bd16-b081a3ba8c0b */         \
     0x13a5320c,                                     \
     0x4c91,                                         \
     0x4fa4,                                         \
    {0xbd, 0x16, 0xb0, 0x81, 0xa3, 0Xba, 0x8c, 0x0b} \
}




class nsBMPEncoder : public imgIEncoder
{
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIENCODER
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSIASYNCINPUTSTREAM

  nsBMPEncoder();
  ~nsBMPEncoder();

protected:
  
  nsresult ParseOptions(const nsAString& aOptions, PRUint32* bpp);
  
  void ConvertHostARGBRow(const PRUint8* aSrc, PRUint8* aDest,
                          PRUint32 aPixelWidth);
  
  void StripAlpha(const PRUint8* aSrc, PRUint8* aDest,
                  PRUint32 aPixelWidth);
  
  void NotifyListener();

  
  void InitFileHeader(PRUint32 aBPP, PRUint32 aWidth, PRUint32 aHeight);
  
  void InitInfoHeader(PRUint32 aBPP, PRUint32 aWidth, PRUint32 aHeight);
  
  void EncodeFileHeader();
  
  void EncodeInfoHeader();
  
  void EncodeImageDataRow24(const PRUint8* aData);
  
  void EncodeImageDataRow32(const PRUint8* aData);
  
  inline PRInt32 GetCurrentImageBufferOffset()
  {
    return static_cast<PRInt32>(mImageBufferCurr - mImageBufferStart);
  }

  
  
  mozilla::imagelib::BMPFILEHEADER mBMPFileHeader;
  mozilla::imagelib::BMPINFOHEADER mBMPInfoHeader;

  
  PRUint8* mImageBufferStart;
  
  PRUint8* mImageBufferCurr;
  
  PRUint32 mImageBufferSize;
  
  PRUint32 mImageBufferReadPoint;
  
  bool mFinished;

  nsCOMPtr<nsIInputStreamCallback> mCallback;
  nsCOMPtr<nsIEventTarget> mCallbackTarget;
  PRUint32 mNotifyThreshold;
};
