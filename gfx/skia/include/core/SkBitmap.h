








#ifndef SkBitmap_DEFINED
#define SkBitmap_DEFINED

#include "Sk64.h"
#include "SkColor.h"
#include "SkPoint.h"
#include "SkRefCnt.h"

struct SkIRect;
class SkColorTable;
class SkPaint;
class SkPixelRef;
class SkRegion;
class SkFlattenableReadBuffer;
class SkFlattenableWriteBuffer;


class SkGpuTexture;











class SK_API SkBitmap {
public:
    class Allocator;

    enum Config {
        kNo_Config,         
        




        kA1_Config,
        kA8_Config,         
        kIndex8_Config,     
        kRGB_565_Config,    
        kARGB_4444_Config,  
        kARGB_8888_Config,  
        




        kRLE_Index8_Config,

        kConfigCount
    };

    


    SkBitmap();
    


    SkBitmap(const SkBitmap& src);
    

    ~SkBitmap();

    


    SkBitmap& operator=(const SkBitmap& src);
    

    
    void swap(SkBitmap& other);

    

    bool empty() const { return 0 == fWidth || 0 == fHeight; }

    


    bool isNull() const { return NULL == fPixels && NULL == fPixelRef; }

    

    Config  config() const { return (Config)fConfig; }
    

    Config  getConfig() const { return this->config(); }
    

    int width() const { return fWidth; }
    

    int height() const { return fHeight; }
    

    int rowBytes() const { return fRowBytes; }

    




    int shiftPerPixel() const { return fBytesPerPixel >> 1; }

    


    int bytesPerPixel() const { return fBytesPerPixel; }

    




    int rowBytesAsPixels() const { return fRowBytes >> (fBytesPerPixel >> 1); }

    

    void* getPixels() const { return fPixels; }

    



    size_t getSize() const { return fHeight * fRowBytes; }

    



    size_t getSafeSize() const ;

    



    Sk64 getSize64() const {
        Sk64 size;
        size.setMul(fHeight, fRowBytes);
        return size;
    }

    

    Sk64 getSafeSize64() const ;

    

    bool isOpaque() const;

    


    void setIsOpaque(bool);

    

    bool isVolatile() const;

    






    void setIsVolatile(bool);

    


    void reset();

    


    static int ComputeRowBytes(Config c, int width);

    


    static int ComputeBytesPerPixel(Config c);

    


    static int ComputeShiftPerPixel(Config c) {
        return ComputeBytesPerPixel(c) >> 1;
    }

    static Sk64 ComputeSize64(Config, int width, int height);
    static size_t ComputeSize(Config, int width, int height);

    



    void setConfig(Config, int width, int height, int rowBytes = 0);
    









    void setPixels(void* p, SkColorTable* ctable = NULL);

    

















    bool copyPixelsTo(void* const dst, size_t dstSize, int dstRowBytes = -1)
         const;

    













    bool copyPixelsFrom(const void* const src, size_t srcSize,
                        int srcRowBytes = -1);

    












    bool allocPixels(SkColorTable* ctable = NULL) {
        return this->allocPixels(NULL, ctable);
    }

    


















    bool allocPixels(Allocator* allocator, SkColorTable* ctable);

    

    SkPixelRef* pixelRef() const { return fPixelRef; }
    


    size_t pixelRefOffset() const { return fPixelRefOffset; }
    



    SkPixelRef* setPixelRef(SkPixelRef* pr, size_t offset = 0);

    



    void lockPixels() const;
    




    void unlockPixels() const;

    





    bool lockPixelsAreWritable() const;

    



    bool readyToDraw() const {
        return this->getPixels() != NULL &&
               ((this->config() != kIndex8_Config &&
                 this->config() != kRLE_Index8_Config) ||
                       fColorTable != NULL);
    }

    

    SkGpuTexture* getTexture() const;

    


    SkColorTable* getColorTable() const { return fColorTable; }

    




    uint32_t getGenerationID() const;

    



    void notifyPixelsChanged() const;

    



    void eraseARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b) const;
    




    void eraseRGB(U8CPU r, U8CPU g, U8CPU b) const {
        this->eraseARGB(0xFF, r, g, b);
    }
    



    void eraseColor(SkColor c) const {
        this->eraseARGB(SkColorGetA(c), SkColorGetR(c), SkColorGetG(c),
                        SkColorGetB(c));
    }

    


















    bool scrollRect(const SkIRect* subset, int dx, int dy,
                    SkRegion* inval = NULL) const;

    






    SkColor getColor(int x, int y) const;
    
    








    void* getAddr(int x, int y) const;

    




    inline uint32_t* getAddr32(int x, int y) const;

    




    inline uint16_t* getAddr16(int x, int y) const;

    




    inline uint8_t* getAddr8(int x, int y) const;

    





    inline uint8_t* getAddr1(int x, int y) const;

    





    inline SkPMColor getIndex8Color(int x, int y) const;

    










    bool extractSubset(SkBitmap* dst, const SkIRect& subset) const;

    










    bool copyTo(SkBitmap* dst, Config c, Allocator* allocator = NULL) const;

    


    bool canCopyTo(Config newConfig) const;

    bool hasMipMap() const;
    void buildMipMap(bool forceRebuild = false);
    void freeMipMap();

    



    int extractMipLevel(SkBitmap* dst, SkFixed sx, SkFixed sy);

    bool extractAlpha(SkBitmap* dst) const {
        return this->extractAlpha(dst, NULL, NULL, NULL);
    }

    bool extractAlpha(SkBitmap* dst, const SkPaint* paint,
                      SkIPoint* offset) const {
        return this->extractAlpha(dst, paint, NULL, offset);
    }

    












    bool extractAlpha(SkBitmap* dst, const SkPaint* paint, Allocator* allocator,
                      SkIPoint* offset) const;

    void flatten(SkFlattenableWriteBuffer&) const;
    void unflatten(SkFlattenableReadBuffer&);

    SkDEBUGCODE(void validate() const;)

    class Allocator : public SkRefCnt {
    public:
        






        virtual bool allocPixelRef(SkBitmap*, SkColorTable*) = 0;
    };

    



    class HeapAllocator : public Allocator {
    public:
        virtual bool allocPixelRef(SkBitmap*, SkColorTable*);
    };

    class RLEPixels {
    public:
        RLEPixels(int width, int height);
        virtual ~RLEPixels();

        uint8_t* packedAtY(int y) const {
            SkASSERT((unsigned)y < (unsigned)fHeight);
            return fYPtrs[y];
        }

        
        void setPackedAtY(int y, uint8_t* addr) {
            SkASSERT((unsigned)y < (unsigned)fHeight);
            fYPtrs[y] = addr;
        }

    private:
        uint8_t** fYPtrs;
        int       fHeight;
    };

private:
    struct MipMap;
    mutable MipMap* fMipMap;

    mutable SkPixelRef* fPixelRef;
    mutable size_t      fPixelRefOffset;
    mutable int         fPixelLockCount;
    
    
    mutable void*       fPixels;
    mutable SkColorTable* fColorTable;    
    
    
    
    mutable int         fRawPixelGenerationID;

    enum Flags {
        kImageIsOpaque_Flag = 0x01,
        kImageIsVolatile_Flag     = 0x02
    };

    uint32_t    fRowBytes;
    uint32_t    fWidth;
    uint32_t    fHeight;
    uint8_t     fConfig;
    uint8_t     fFlags;
    uint8_t     fBytesPerPixel; 

    

    static Sk64 ComputeSafeSize64(Config config,
                                  uint32_t width,
                                  uint32_t height,
                                  uint32_t rowBytes);
    static size_t ComputeSafeSize(Config   config,
                                  uint32_t width,
                                  uint32_t height,
                                  uint32_t rowBytes);

    

    void freePixels();
    void updatePixelsFromRef() const;

    static SkFixed ComputeMipLevel(SkFixed sx, SkFixed dy);
};






class SkColorTable : public SkRefCnt {
public:
    

    SkColorTable(const SkColorTable& src);
    


    explicit SkColorTable(int count);
    explicit SkColorTable(SkFlattenableReadBuffer&);
    SkColorTable(const SkPMColor colors[], int count);
    virtual ~SkColorTable();

    enum Flags {
        kColorsAreOpaque_Flag   = 0x01  
    };
    

    unsigned getFlags() const { return fFlags; }
    

    void    setFlags(unsigned flags);

    bool isOpaque() const { return (fFlags & kColorsAreOpaque_Flag) != 0; }
    void setIsOpaque(bool isOpaque);

    

    int count() const { return fCount; }

    


    SkPMColor operator[](int index) const {
        SkASSERT(fColors != NULL && (unsigned)index < fCount);
        return fColors[index];
    }

    







    



    SkPMColor* lockColors() {
        SkDEBUGCODE(fColorLockCount += 1;)
        return fColors;
    }
    

    void unlockColors(bool changed);

    




    const uint16_t* lock16BitCache();
    

    void unlock16BitCache() {
        SkASSERT(f16BitCacheLockCount > 0);
        SkDEBUGCODE(f16BitCacheLockCount -= 1);
    }

    void flatten(SkFlattenableWriteBuffer&) const;

private:
    SkPMColor*  fColors;
    uint16_t*   f16BitCache;
    uint16_t    fCount;
    uint8_t     fFlags;
    SkDEBUGCODE(int fColorLockCount;)
    SkDEBUGCODE(int f16BitCacheLockCount;)

    void inval16BitCache();
};

class SkAutoLockPixels : public SkNoncopyable {
public:
    SkAutoLockPixels(const SkBitmap& bm, bool doLock = true) : fBitmap(bm) {
        fDidLock = doLock;
        if (doLock) {
            bm.lockPixels();
        }
    }
    ~SkAutoLockPixels() {
        if (fDidLock) {
            fBitmap.unlockPixels();
        }
    }

private:
    const SkBitmap& fBitmap;
    bool            fDidLock;
};




class SkAutoLockColors : public SkNoncopyable {
public:
    


    SkAutoLockColors() : fCTable(NULL), fColors(NULL) {}
    

    explicit SkAutoLockColors(const SkBitmap& bm) {
        fCTable = bm.getColorTable();
        fColors = fCTable ? fCTable->lockColors() : NULL;
    }
    

    explicit SkAutoLockColors(SkColorTable* ctable) {
        fCTable = ctable;
        fColors = ctable ? ctable->lockColors() : NULL;
    }
    ~SkAutoLockColors() {
        if (fCTable) {
            fCTable->unlockColors(false);
        }
    }

    


    const SkPMColor* colors() const { return fColors; }

    


    const SkPMColor* lockColors(SkColorTable* ctable) {
        if (fCTable) {
            fCTable->unlockColors(false);
        }
        fCTable = ctable;
        fColors = ctable ? ctable->lockColors() : NULL;
        return fColors;
    }

    const SkPMColor* lockColors(const SkBitmap& bm) {
        return this->lockColors(bm.getColorTable());
    }

private:
    SkColorTable*    fCTable;
    const SkPMColor* fColors;
};



inline uint32_t* SkBitmap::getAddr32(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(fConfig == kARGB_8888_Config);
    SkASSERT((unsigned)x < fWidth && (unsigned)y < fHeight);
    return (uint32_t*)((char*)fPixels + y * fRowBytes + (x << 2));
}

inline uint16_t* SkBitmap::getAddr16(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(fConfig == kRGB_565_Config || fConfig == kARGB_4444_Config);
    SkASSERT((unsigned)x < fWidth && (unsigned)y < fHeight);
    return (uint16_t*)((char*)fPixels + y * fRowBytes + (x << 1));
}

inline uint8_t* SkBitmap::getAddr8(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(fConfig == kA8_Config || fConfig == kIndex8_Config);
    SkASSERT((unsigned)x < fWidth && (unsigned)y < fHeight);
    return (uint8_t*)fPixels + y * fRowBytes + x;
}

inline SkPMColor SkBitmap::getIndex8Color(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(fConfig == kIndex8_Config);
    SkASSERT((unsigned)x < fWidth && (unsigned)y < fHeight);
    SkASSERT(fColorTable);
    return (*fColorTable)[*((const uint8_t*)fPixels + y * fRowBytes + x)];
}


inline uint8_t* SkBitmap::getAddr1(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(fConfig == kA1_Config);
    SkASSERT((unsigned)x < fWidth && (unsigned)y < fHeight);
    return (uint8_t*)fPixels + y * fRowBytes + (x >> 3);
}

#endif
