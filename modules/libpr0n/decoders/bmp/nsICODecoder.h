







































#ifndef _nsICODecoder_h
#define _nsICODecoder_h

#include "nsCOMPtr.h"
#include "imgIDecoder.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "nsBMPDecoder.h"


#define NS_ICODECODER_CID \
{ 0xcb3ede1a, 0xfa5, 0x4e27, { 0xaa, 0xfe, 0xf, 0x78, 0x1, 0xe5, 0xa1, 0xf1 } }

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

class nsICODecoder : public imgIDecoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODER

  nsICODecoder();
  virtual ~nsICODecoder();

private:
  
  static NS_METHOD ReadSegCb(nsIInputStream* aIn, void* aClosure,
                             const char* aFromRawSegment, PRUint32 aToOffset,
                             PRUint32 aCount, PRUint32 *aWriteCount);

  
  nsresult ProcessData(const char* aBuffer, PRUint32 aCount);
  void ProcessDirEntry(IconDirEntry& aTarget);
  void ProcessInfoHeader();

  nsresult SetImageData();

  PRUint32 CalcAlphaRowSize();

private:
  nsCOMPtr<imgIContainer> mImage;
  nsCOMPtr<imgIDecoderObserver> mObserver;
  PRUint32 mFlags;
  
  PRUint32 mPos;
  PRUint16 mNumIcons;
  PRUint16 mCurrIcon;
  PRUint32 mImageOffset;

  char mDirEntryArray[16];
  IconDirEntry mDirEntry;

  char mBIHraw[40];
  BMPINFOHEADER mBIH;

  PRUint32 mNumColors;
  colorTable* mColors;

  PRUint8* mRow; 
  PRUint32 mRowBytes; 
  PRInt32 mCurLine;

  PRUint32* mImageData;

  PRPackedBool mHaveAlphaData;
  PRPackedBool mIsCursor;
  PRPackedBool mDecodingAndMask;
  PRPackedBool mError;
};


#endif
