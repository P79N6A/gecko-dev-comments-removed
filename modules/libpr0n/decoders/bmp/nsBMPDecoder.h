






































#ifndef _nsBMPDecoder_h
#define _nsBMPDecoder_h

#include "nsCOMPtr.h"
#include "imgIDecoder.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "gfxColor.h"

#define NS_BMPDECODER_CID \
{ /* {78c61626-4d1f-4843-9364-4652d98ff6e1} */ \
  0x78c61626, \
  0x4d1f, \
  0x4843, \
  { 0x93, 0x64, 0x46, 0x52, 0xd9, 0x8f, 0xf6, 0xe1 } \
}

struct BMPFILEHEADER {
    char signature[2]; 
    PRUint32 filesize;
    PRInt32 reserved; 
    PRUint32 dataoffset; 

    PRUint32 bihsize;
};
#define BFH_LENGTH 18 // Note: For our purposes, we include bihsize in the BFH

#define OS2_BIH_LENGTH 12 // This is the real BIH size (as contained in the bihsize field of BMPFILEHEADER)
#define OS2_HEADER_LENGTH (BFH_LENGTH + 8)
#define WIN_HEADER_LENGTH (BFH_LENGTH + 36)

struct BMPINFOHEADER {
    PRInt32 width; 
    PRInt32 height; 
    PRUint16 planes; 
    PRUint16 bpp; 
    
    PRUint32 compression; 
    PRUint32 image_size; 
    PRUint32 xppm; 
    PRUint32 yppm; 
    PRUint32 colors; 
    PRUint32 important_colors; 
};

struct colorTable {
    PRUint8 red;
    PRUint8 green;
    PRUint8 blue;
};

struct bitFields {
    PRUint32 red;
    PRUint32 green;
    PRUint32 blue;
    PRUint8 redLeftShift;
    PRUint8 redRightShift;
    PRUint8 greenLeftShift;
    PRUint8 greenRightShift;
    PRUint8 blueLeftShift;
    PRUint8 blueRightShift;
};

#define BITFIELD_LENGTH 12 // Length of the bitfields structure in the bmp file

#if defined WORDS_BIGENDIAN || defined IS_BIG_ENDIAN



#define LITTLE_TO_NATIVE16(x) (((((PRUint16) x) & 0xFF) << 8) | \
                               (((PRUint16) x) >> 8))
#define LITTLE_TO_NATIVE32(x) (((((PRUint32) x) & 0xFF) << 24) | \
                               (((((PRUint32) x) >> 8) & 0xFF) << 16) | \
                               (((((PRUint32) x) >> 16) & 0xFF) << 8) | \
                               (((PRUint32) x) >> 24))
#else
#define LITTLE_TO_NATIVE16(x) x
#define LITTLE_TO_NATIVE32(x) x
#endif

#define USE_RGB


#define BI_RLE8 1
#define BI_RLE4 2
#define BI_BITFIELDS 3


#define RLE_ESCAPE       0
#define RLE_ESCAPE_EOL   0
#define RLE_ESCAPE_EOF   1
#define RLE_ESCAPE_DELTA 2


enum ERLEState {
    eRLEStateInitial,
    eRLEStateNeedSecondEscapeByte,
    eRLEStateNeedXDelta,
    eRLEStateNeedYDelta,    
    eRLEStateAbsoluteMode,  
    eRLEStateAbsoluteModePadded 
};




class nsBMPDecoder : public imgIDecoder
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_IMGIDECODER
    
    nsBMPDecoder();
    ~nsBMPDecoder();

private:
    
    static NS_METHOD ReadSegCb(nsIInputStream* aIn, void* aClosure,
                               const char* aFromRawSegment, PRUint32 aToOffset,
                               PRUint32 aCount, PRUint32 *aWriteCount);

    


    NS_METHOD ProcessData(const char* aBuffer, PRUint32 aCount);

    

    NS_METHOD CalcBitShift();

    nsCOMPtr<imgIDecoderObserver> mObserver;

    nsCOMPtr<imgIContainer> mImage;
    PRUint32 mFlags;

    PRUint32 mPos;

    BMPFILEHEADER mBFH;
    BMPINFOHEADER mBIH;
    char mRawBuf[36];

    PRUint32 mLOH; 

    PRUint32 mNumColors; 
    colorTable *mColors;

    bitFields mBitFields;

    PRUint32 *mImageData; 
    PRUint8 *mRow;      
    PRUint32 mRowBytes; 
    PRInt32 mCurLine;   
    PRInt32 mOldLine;   
    PRInt32 mCurPos;    

    ERLEState mState;   
    PRUint32 mStateData;
    PRBool mError;      

    

    void ProcessFileHeader();
    

    void ProcessInfoHeader();
};




static inline void SetPixel(PRUint32*& aDecoded, PRUint8 aRed, PRUint8 aGreen, PRUint8 aBlue, PRUint8 aAlpha = 0xFF)
{
    *aDecoded++ = GFX_PACKED_PIXEL(aAlpha, aRed, aGreen, aBlue);
}

static inline void SetPixel(PRUint32*& aDecoded, PRUint8 idx, colorTable* aColors)
{
    SetPixel(aDecoded, aColors[idx].red, aColors[idx].green, aColors[idx].blue);
}







inline void Set4BitPixel(PRUint32*& aDecoded, PRUint8 aData,
                         PRUint32& aCount, colorTable* aColors)
{
    PRUint8 idx = aData >> 4;
    SetPixel(aDecoded, idx, aColors);
    if (--aCount > 0) {
        idx = aData & 0xF;
        SetPixel(aDecoded, idx, aColors);
        --aCount;
    }
}

#endif

