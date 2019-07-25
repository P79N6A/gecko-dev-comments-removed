








































#ifndef _nsICODecoder_h
#define _nsICODecoder_h

#include "nsAutoPtr.h"
#include "Decoder.h"
#include "imgIDecoderObserver.h"
#include "nsBMPDecoder.h"
#include "nsPNGDecoder.h"

namespace mozilla {
namespace imagelib {

#define ICODIRENTRYSIZE 16
#define PNGSIGNATURESIZE 8
#define BMPFILEHEADERSIZE 14

class RasterImage;

struct IconDirEntry
{
  PRUint8   mWidth;
  PRUint8   mHeight;
  PRUint8   mColorCount;
  PRUint8   mReserved;
  union {
    PRUint16 mPlanes;   
    PRUint16 mXHotspot; 
  };
  union {
    PRUint16 mBitCount; 
    PRUint16 mYHotspot; 
  };
  PRUint32  mBytesInRes;
  PRUint32  mImageOffset;
};

class nsICODecoder : public Decoder
{
public:

  nsICODecoder();
  virtual ~nsICODecoder();

  virtual void WriteInternal(const char* aBuffer, PRUint32 aCount);
  virtual void FinishInternal();

private:
  
  void ProcessDirEntry(IconDirEntry& aTarget);
  
  void SetHotSpotIfCursor();
  
  PRBool FillBitmapFileHeaderBuffer(PRInt8 *bfh);
  
  void FillBitmapInformationBufferHeight(PRInt8 *bih);
  
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
  
  PRPackedBool mIsCursor;
  
  PRPackedBool mIsPNG;
};

} 
} 

#endif
