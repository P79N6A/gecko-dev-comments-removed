







































#ifndef _nsBMPDecoder_h
#define _nsBMPDecoder_h

#include "nsAutoPtr.h"
#include "imgIDecoderObserver.h"
#include "gfxColor.h"
#include "Decoder.h"
#include "BMPFileHeaders.h"

namespace mozilla {
namespace imagelib {

class RasterImage;




class nsBMPDecoder : public Decoder
{
public:

    nsBMPDecoder(RasterImage &aImage, imgIDecoderObserver* aObserver);
    ~nsBMPDecoder();

    
    
    
    void SetUseAlphaData(bool useAlphaData);
    
    PRInt32 GetBitsPerPixel() const;
    
    PRInt32 GetWidth() const;
    
    PRInt32 GetHeight() const;
    
    PRUint32* GetImageData();
    
    PRInt32 GetCompressedImageSize() const;
    
    
    bool HasAlphaData() const;

    virtual void WriteInternal(const char* aBuffer, PRUint32 aCount);
    virtual void FinishInternal();

private:

    

    NS_METHOD CalcBitShift();

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

    

    void ProcessFileHeader();
    

    void ProcessInfoHeader();

    
    
    
    
    
    
    
    
    bool mUseAlphaData;
    
    bool mHaveAlphaData;
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

} 
} 


#endif

