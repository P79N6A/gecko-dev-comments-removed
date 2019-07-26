




#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"

#include "imgIEncoder.h"

#include "nsCOMPtr.h"

#include <png.h>

#define NS_PNGENCODER_CID \
{ /* 38d1592e-b81e-432b-86f8-471878bbfe07 */         \
     0x38d1592e,                                     \
     0xb81e,                                         \
     0x432b,                                         \
    {0x86, 0xf8, 0x47, 0x18, 0x78, 0xbb, 0xfe, 0x07} \
}




class nsPNGEncoder MOZ_FINAL : public imgIEncoder
{
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIENCODER
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSIASYNCINPUTSTREAM

  nsPNGEncoder();
  ~nsPNGEncoder();

protected:
  nsresult ParseOptions(const nsAString& aOptions,
                        bool* useTransparency,
                        bool* skipFirstFrame,
                        uint32_t* numAnimatedFrames,
                        uint32_t* numIterations,
                        uint32_t* frameDispose,
                        uint32_t* frameBlend,
                        uint32_t* frameDelay,
                        uint32_t* offsetX,
                        uint32_t* offsetY);
  void ConvertHostARGBRow(const uint8_t* aSrc, uint8_t* aDest,
                          uint32_t aPixelWidth, bool aUseTransparency);
  void StripAlpha(const uint8_t* aSrc, uint8_t* aDest,
                  uint32_t aPixelWidth);
  static void WarningCallback(png_structp png_ptr, png_const_charp warning_msg);
  static void ErrorCallback(png_structp png_ptr, png_const_charp error_msg);
  static void WriteCallback(png_structp png, png_bytep data, png_size_t size);
  void NotifyListener();

  png_struct* mPNG;
  png_info* mPNGinfo;

  bool mIsAnimation;
  bool mFinished;

  
  uint8_t* mImageBuffer;
  uint32_t mImageBufferSize;
  uint32_t mImageBufferUsed;

  uint32_t mImageBufferReadPoint;

  nsCOMPtr<nsIInputStreamCallback> mCallback;
  nsCOMPtr<nsIEventTarget> mCallbackTarget;
  uint32_t mNotifyThreshold;

  





  ReentrantMonitor mReentrantMonitor;
};
