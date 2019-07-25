




































#include "imgIEncoder.h"

#include "mozilla/ReentrantMonitor.h"

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




class nsICOEncoder : public imgIEncoder
{
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIENCODER
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSIASYNCINPUTSTREAM

  nsICOEncoder();
  ~nsICOEncoder();
  
  
  PRUint32 GetRealWidth() const
  {
    return mICODirEntry.mWidth == 0 ? 256 : mICODirEntry.mWidth; 
  }

  
  PRUint32 GetRealHeight() const
  {
    return mICODirEntry.mHeight == 0 ? 256 : mICODirEntry.mHeight; 
  }

protected:
  nsresult ParseOptions(const nsAString& aOptions, PRUint32* bpp, 
                        PRBool *usePNG);
  void NotifyListener();

  
  void InitFileHeader();
  
  void InitInfoHeader(PRUint32 aBPP, PRUint8 aWidth, PRUint8 aHeight);
  
  void EncodeFileHeader();
  
  void EncodeInfoHeader();
  
  inline PRInt32 GetCurrentImageBufferOffset()
  {
    return static_cast<PRInt32>(mImageBufferCurr - mImageBufferStart);
  }

  
  
  nsCOMPtr<imgIEncoder> mContainedEncoder;

  
  
  
  mozilla::imagelib::IconFileHeader mICOFileHeader;
  mozilla::imagelib::IconDirEntry mICODirEntry;

  
  PRUint8* mImageBufferStart;
  
  PRUint8* mImageBufferCurr;
  
  PRUint32 mImageBufferSize;
  
  PRUint32 mImageBufferReadPoint;
  
  PRPackedBool mFinished;
  
  PRPackedBool mUsePNG;

  nsCOMPtr<nsIInputStreamCallback> mCallback;
  nsCOMPtr<nsIEventTarget> mCallbackTarget;
  PRUint32 mNotifyThreshold;
};
