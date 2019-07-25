








#ifndef SkMask_DEFINED
#define SkMask_DEFINED

#include "SkRect.h"





struct SkMask {
    enum Format {
        kBW_Format, 
        kA8_Format, 
        k3D_Format, 
        kARGB32_Format,         
        kLCD16_Format,          
        kLCD32_Format           
    };

    enum {
        kCountMaskFormats = kLCD32_Format + 1
    };

    uint8_t*    fImage;
    SkIRect     fBounds;
    uint32_t    fRowBytes;
    Format      fFormat;

    

    bool isEmpty() const { return fBounds.isEmpty(); }

    



    size_t computeImageSize() const;

    



    size_t computeTotalImageSize() const;

    



    uint8_t* getAddr1(int x, int y) const {
        SkASSERT(kBW_Format == fFormat);
        SkASSERT(fBounds.contains(x, y));
        SkASSERT(fImage != NULL);
        return fImage + ((x - fBounds.fLeft) >> 3) + (y - fBounds.fTop) * fRowBytes;
    }

    



    uint8_t* getAddr8(int x, int y) const {
        SkASSERT(kA8_Format == fFormat);
        SkASSERT(fBounds.contains(x, y));
        SkASSERT(fImage != NULL);
        return fImage + x - fBounds.fLeft + (y - fBounds.fTop) * fRowBytes;
    }

    




    uint16_t* getAddrLCD16(int x, int y) const {
        SkASSERT(kLCD16_Format == fFormat);
        SkASSERT(fBounds.contains(x, y));
        SkASSERT(fImage != NULL);
        uint16_t* row = (uint16_t*)(fImage + (y - fBounds.fTop) * fRowBytes);
        return row + (x - fBounds.fLeft);
    }

    




    uint32_t* getAddrLCD32(int x, int y) const {
        SkASSERT(kLCD32_Format == fFormat);
        SkASSERT(fBounds.contains(x, y));
        SkASSERT(fImage != NULL);
        uint32_t* row = (uint32_t*)(fImage + (y - fBounds.fTop) * fRowBytes);
        return row + (x - fBounds.fLeft);
    }

    











    void* getAddr(int x, int y) const;

    static uint8_t* AllocImage(size_t bytes);
    static void FreeImage(void* image);

    enum CreateMode {
        kJustComputeBounds_CreateMode,      
        kJustRenderImage_CreateMode,        
        kComputeBoundsAndRenderImage_CreateMode  
    };
};









class SkAutoMaskFreeImage {
public:
    SkAutoMaskFreeImage(uint8_t* maskImage) {
        fImage = maskImage;
    }
    
    ~SkAutoMaskFreeImage() {
        SkMask::FreeImage(fImage);
    }

private:
    uint8_t* fImage;
};

#endif

