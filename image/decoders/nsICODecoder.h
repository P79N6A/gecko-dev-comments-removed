








































#ifndef _nsICODecoder_h
#define _nsICODecoder_h

#include "nsAutoPtr.h"
#include "Decoder.h"
#include "imgIDecoderObserver.h"
#include "nsBMPDecoder.h"
#include "nsPNGDecoder.h"
#include "ICOFileHeaders.h"

namespace mozilla {
namespace imagelib {

class RasterImage;

class nsICODecoder : public Decoder
{
public:

  nsICODecoder(RasterImage &aImage, imgIDecoderObserver* aObserver);
  virtual ~nsICODecoder();

  
  PRUint32 GetRealWidth() const
  {
    return mDirEntry.mWidth == 0 ? 256 : mDirEntry.mWidth; 
  }

  
  PRUint32 GetRealHeight() const
  {
    return mDirEntry.mHeight == 0 ? 256 : mDirEntry.mHeight; 
  }

  virtual void WriteInternal(const char* aBuffer, PRUint32 aCount);
  virtual void FinishInternal();

private:
  
  void ProcessDirEntry(IconDirEntry& aTarget);
  
  void SetHotSpotIfCursor();
  
  bool FillBitmapFileHeaderBuffer(PRInt8 *bfh);
  
  void FillBitmapInformationBufferHeight(PRInt8 *bih);
  
  PRInt32 ExtractBIHSizeFromBitmap(PRInt8 *bih);
  
  PRInt32 ExtractBPPFromBitmap(PRInt8 *bih);
  
  PRUint32 CalcAlphaRowSize();
  
  PRUint16 GetNumColors();

  PRUint16 mBPP; 
  PRUint32 mPos; 
  PRUint16 mNumIcons; 
  PRUint16 mCurrIcon; 
  PRUint32 mImageOffset; 
  PRUint8 *mRow;      
  PRInt32 mCurLine;   
  PRUint32 mRowBytes; 
  PRInt32 mOldLine;   
  nsAutoPtr<Decoder> mContainedDecoder; 

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
