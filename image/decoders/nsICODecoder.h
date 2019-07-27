





#ifndef _nsICODecoder_h
#define _nsICODecoder_h

#include "nsAutoPtr.h"
#include "Decoder.h"
#include "nsBMPDecoder.h"
#include "nsPNGDecoder.h"
#include "ICOFileHeaders.h"

namespace mozilla {
namespace image {

class RasterImage;

class nsICODecoder : public Decoder
{
public:

  explicit nsICODecoder(RasterImage &aImage);
  virtual ~nsICODecoder();

  
  uint32_t GetRealWidth() const
  {
    return mDirEntry.mWidth == 0 ? 256 : mDirEntry.mWidth; 
  }

  
  uint32_t GetRealHeight() const
  {
    return mDirEntry.mHeight == 0 ? 256 : mDirEntry.mHeight; 
  }

  virtual void WriteInternal(const char* aBuffer, uint32_t aCount, DecodeStrategy aStrategy);
  virtual void FinishInternal();
  virtual bool NeedsNewFrame() const;
  virtual nsresult AllocateFrame();

private:
  
  
  bool WriteToContainedDecoder(const char* aBuffer, uint32_t aCount, DecodeStrategy aStrategy);

  
  void ProcessDirEntry(IconDirEntry& aTarget);
  
  void SetHotSpotIfCursor();
  
  bool FillBitmapFileHeaderBuffer(int8_t *bfh);
  
  
  
  
  bool FixBitmapHeight(int8_t *bih);
  
  
  bool FixBitmapWidth(int8_t *bih);
  
  int32_t ExtractBIHSizeFromBitmap(int8_t *bih);
  
  int32_t ExtractBPPFromBitmap(int8_t *bih);
  
  uint32_t CalcAlphaRowSize();
  
  uint16_t GetNumColors();

  uint16_t mBPP; 
  uint32_t mPos; 
  uint16_t mNumIcons; 
  uint16_t mCurrIcon; 
  uint32_t mImageOffset; 
  uint8_t *mRow;      
  int32_t mCurLine;   
  uint32_t mRowBytes; 
  int32_t mOldLine;   
  nsRefPtr<Decoder> mContainedDecoder; 

  char mDirEntryArray[ICODIRENTRYSIZE]; 
  IconDirEntry mDirEntry; 
  
  char mSignature[PNGSIGNATURESIZE]; 
  
  char mBIHraw[40];
  
  bool mIsCursor;
  
  bool mIsPNG;
};

} 
} 

#endif
