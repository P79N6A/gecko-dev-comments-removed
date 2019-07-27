





#ifndef mozilla_image_decoders_nsBMPDecoder_h
#define mozilla_image_decoders_nsBMPDecoder_h

#include "BMPFileHeaders.h"
#include "Decoder.h"
#include "gfxColor.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace image {

class RasterImage;



class nsBMPDecoder : public Decoder
{
public:

    explicit nsBMPDecoder(RasterImage* aImage);
    ~nsBMPDecoder();

    
    
    
    void SetUseAlphaData(bool useAlphaData);

    
    int32_t GetBitsPerPixel() const;

    
    int32_t GetWidth() const;

    
    int32_t GetHeight() const;

    
    uint32_t* GetImageData();

    
    int32_t GetCompressedImageSize() const;

    
    
    bool HasAlphaData() const;

    virtual void WriteInternal(const char* aBuffer,
                               uint32_t aCount) override;
    virtual void FinishInternal() override;

private:

    
    
    NS_METHOD CalcBitShift();

    uint32_t mPos; 

    BMPFILEHEADER mBFH;
    BITMAPV5HEADER mBIH;
    char mRawBuf[WIN_V3_INTERNAL_BIH_LENGTH]; 
                                              

    uint32_t mLOH; 

    uint32_t mNumColors; 
                         
    colorTable* mColors;

    bitFields mBitFields;

    uint8_t* mRow;      
    uint32_t mRowBytes; 
    int32_t mCurLine;   
                        
    int32_t mOldLine;   
    int32_t mCurPos;    

    ERLEState mState;   
    uint32_t mStateData;
                        

    
    
    void ProcessFileHeader();

    
    
    void ProcessInfoHeader();

    
    bool mProcessedHeader;

    
    
    
    
    
    
    
    
    bool mUseAlphaData;

    
    bool mHaveAlphaData;
};




static inline void
SetPixel(uint32_t*& aDecoded, uint8_t aRed, uint8_t aGreen,
         uint8_t aBlue, uint8_t aAlpha = 0xFF)
{
    *aDecoded++ = gfxPackedPixel(aAlpha, aRed, aGreen, aBlue);
}

static inline void
SetPixel(uint32_t*& aDecoded, uint8_t idx, colorTable* aColors)
{
    SetPixel(aDecoded, aColors[idx].red, aColors[idx].green, aColors[idx].blue);
}






inline void
Set4BitPixel(uint32_t*& aDecoded, uint8_t aData, uint32_t& aCount,
             colorTable* aColors)
{
    uint8_t idx = aData >> 4;
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
