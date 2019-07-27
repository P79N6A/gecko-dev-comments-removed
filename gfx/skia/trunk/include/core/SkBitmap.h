






#ifndef SkBitmap_DEFINED
#define SkBitmap_DEFINED

#include "SkColor.h"
#include "SkColorTable.h"
#include "SkImageInfo.h"
#include "SkPoint.h"
#include "SkRefCnt.h"

struct SkMask;
struct SkIRect;
struct SkRect;
class SkPaint;
class SkPixelRef;
class SkPixelRefFactory;
class SkRegion;
class SkString;
class GrTexture;











class SK_API SkBitmap {
public:
    class SK_API Allocator;

#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
    enum Config {
        kNo_Config,         
        kA8_Config,         
        kIndex8_Config,     
        kRGB_565_Config,    
        kARGB_4444_Config,  
        kARGB_8888_Config,  
    };

    
    
    enum {
        kConfigCount = kARGB_8888_Config + 1
    };

    
    Config  config() const;
    
    SK_ATTR_DEPRECATED("use config()")
    Config  getConfig() const { return this->config(); }
#endif

    



    SkBitmap();

    






    SkBitmap(const SkBitmap& src);

    ~SkBitmap();

    


    SkBitmap& operator=(const SkBitmap& src);
    

    
    void swap(SkBitmap& other);

    

    const SkImageInfo& info() const { return fInfo; }

    int width() const { return fInfo.fWidth; }
    int height() const { return fInfo.fHeight; }
    SkColorType colorType() const { return fInfo.fColorType; }
    SkAlphaType alphaType() const { return fInfo.fAlphaType; }

    



    int bytesPerPixel() const { return fInfo.bytesPerPixel(); }

    



    int rowBytesAsPixels() const {
        return fRowBytes >> this->shiftPerPixel();
    }

    



    int shiftPerPixel() const { return this->bytesPerPixel() >> 1; }

    

    


    bool empty() const { return fInfo.isEmpty(); }

    



    bool isNull() const { return NULL == fPixelRef; }

    

    bool drawsNothing() const { return this->empty() || this->isNull(); }

    
    size_t rowBytes() const { return fRowBytes; }

    








    bool setAlphaType(SkAlphaType);

    

    void* getPixels() const { return fPixels; }

    



    size_t getSize() const { return fInfo.fHeight * fRowBytes; }

    



    size_t getSafeSize() const { return fInfo.getSafeSize(fRowBytes); }

    


    int64_t computeSize64() const {
        return sk_64_mul(fInfo.fHeight, fRowBytes);
    }

    




    int64_t computeSafeSize64() const {
        return fInfo.getSafeSize64(fRowBytes);
    }

    


    bool isImmutable() const;

    





    void setImmutable();

    

    bool isOpaque() const {
        return SkAlphaTypeIsOpaque(this->alphaType());
    }

    

    bool isVolatile() const;

    






    void setIsVolatile(bool);

    


    void reset();

    








    static bool ComputeIsOpaque(const SkBitmap&);

    


    void getBounds(SkRect* bounds) const;
    void getBounds(SkIRect* bounds) const;

    bool setInfo(const SkImageInfo&, size_t rowBytes = 0);

    





    bool allocPixels(const SkImageInfo&, SkPixelRefFactory*, SkColorTable*);

    







    bool allocPixels(const SkImageInfo& info, size_t rowBytes);

    





    bool allocPixels(const SkImageInfo& info) {
        return this->allocPixels(info, info.minRowBytes());
    }

    bool allocN32Pixels(int width, int height, bool isOpaque = false) {
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        if (isOpaque) {
            info.fAlphaType = kOpaque_SkAlphaType;
        }
        return this->allocPixels(info);
    }

    






    bool installPixels(const SkImageInfo&, void* pixels, size_t rowBytes, SkColorTable*,
                       void (*releaseProc)(void* addr, void* context), void* context);

    




    bool installPixels(const SkImageInfo& info, void* pixels, size_t rowBytes) {
        return this->installPixels(info, pixels, rowBytes, NULL, NULL, NULL);
    }

    




    bool installMaskPixels(const SkMask&);

    









    void setPixels(void* p, SkColorTable* ctable = NULL);

    

















    bool copyPixelsTo(void* const dst, size_t dstSize, size_t dstRowBytes = 0,
                      bool preserveDstPad = false) const;

    












    bool allocPixels(SkColorTable* ctable = NULL) {
        return this->allocPixels(NULL, ctable);
    }

    

















    bool allocPixels(Allocator* allocator, SkColorTable* ctable);

    



    SkPixelRef* pixelRef() const { return fPixelRef; }

    










    SkIPoint pixelRefOrigin() const { return fPixelRefOrigin; }

    






    SkPixelRef* setPixelRef(SkPixelRef* pr, int dx, int dy);

    SkPixelRef* setPixelRef(SkPixelRef* pr, const SkIPoint& origin) {
        return this->setPixelRef(pr, origin.fX, origin.fY);
    }

    SkPixelRef* setPixelRef(SkPixelRef* pr) {
        return this->setPixelRef(pr, 0, 0);
    }

    



    void lockPixels() const;
    




    void unlockPixels() const;

    





    bool lockPixelsAreWritable() const;

    



    bool readyToDraw() const {
        return this->getPixels() != NULL &&
               (this->colorType() != kIndex_8_SkColorType || NULL != fColorTable);
    }

    

    GrTexture* getTexture() const;

    




    SkColorTable* getColorTable() const { return fColorTable; }

    




    uint32_t getGenerationID() const;

    



    void notifyPixelsChanged() const;

    





    void eraseColor(SkColor c) const {
        this->eraseARGB(SkColorGetA(c), SkColorGetR(c), SkColorGetG(c),
                        SkColorGetB(c));
    }

    





    void eraseARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b) const;

    SK_ATTR_DEPRECATED("use eraseARGB or eraseColor")
    void eraseRGB(U8CPU r, U8CPU g, U8CPU b) const {
        this->eraseARGB(0xFF, r, g, b);
    }

    





    void eraseArea(const SkIRect& area, SkColor c) const;

    


















    bool scrollRect(const SkIRect* subset, int dx, int dy,
                    SkRegion* inval = NULL) const;

    






    SkColor getColor(int x, int y) const;

    








    void* getAddr(int x, int y) const;

    




    inline uint32_t* getAddr32(int x, int y) const;

    




    inline uint16_t* getAddr16(int x, int y) const;

    




    inline uint8_t* getAddr8(int x, int y) const;

    





    inline SkPMColor getIndex8Color(int x, int y) const;

    










    bool extractSubset(SkBitmap* dst, const SkIRect& subset) const;

    











    bool copyTo(SkBitmap* dst, SkColorType ct, Allocator* = NULL) const;

    bool copyTo(SkBitmap* dst, Allocator* allocator = NULL) const {
        return this->copyTo(dst, this->colorType(), allocator);
    }

    


















    bool readPixels(const SkImageInfo& dstInfo, void* dstPixels, size_t dstRowBytes,
                    int srcX, int srcY) const;

    



    bool canCopyTo(SkColorType colorType) const;

    





    bool deepCopyTo(SkBitmap* dst) const;

#ifdef SK_BUILD_FOR_ANDROID
    bool hasHardwareMipMap() const {
        return (fFlags & kHasHardwareMipMap_Flag) != 0;
    }

    void setHasHardwareMipMap(bool hasHardwareMipMap) {
        if (hasHardwareMipMap) {
            fFlags |= kHasHardwareMipMap_Flag;
        } else {
            fFlags &= ~kHasHardwareMipMap_Flag;
        }
    }
#endif

    bool extractAlpha(SkBitmap* dst) const {
        return this->extractAlpha(dst, NULL, NULL, NULL);
    }

    bool extractAlpha(SkBitmap* dst, const SkPaint* paint,
                      SkIPoint* offset) const {
        return this->extractAlpha(dst, paint, NULL, offset);
    }

    












    bool extractAlpha(SkBitmap* dst, const SkPaint* paint, Allocator* allocator,
                      SkIPoint* offset) const;

    SkDEBUGCODE(void validate() const;)

    class Allocator : public SkRefCnt {
    public:
        SK_DECLARE_INST_COUNT(Allocator)

        






        virtual bool allocPixelRef(SkBitmap*, SkColorTable*) = 0;
    private:
        typedef SkRefCnt INHERITED;
    };

    



    class HeapAllocator : public Allocator {
    public:
        virtual bool allocPixelRef(SkBitmap*, SkColorTable*) SK_OVERRIDE;
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

    SK_TO_STRING_NONVIRT()

private:
    mutable SkPixelRef* fPixelRef;
    mutable int         fPixelLockCount;
    
    mutable void*       fPixels;
    mutable SkColorTable* fColorTable;    

    SkIPoint    fPixelRefOrigin;

    enum Flags {
        kImageIsVolatile_Flag   = 0x02,
#ifdef SK_BUILD_FOR_ANDROID
        



        kHasHardwareMipMap_Flag = 0x08,
#endif
    };

    SkImageInfo fInfo;

    uint32_t    fRowBytes;

    uint8_t     fFlags;

    void internalErase(const SkIRect&, U8CPU a, U8CPU r, U8CPU g, U8CPU b)const;

    

    void freePixels();
    void updatePixelsFromRef() const;

    void legacyUnflatten(SkReadBuffer&);

    static void WriteRawPixels(SkWriteBuffer*, const SkBitmap&);
    static bool ReadRawPixels(SkReadBuffer*, SkBitmap*);

    friend class SkBitmapSource;    
    friend class SkReadBuffer;      
    friend class SkWriteBuffer;     
    friend struct SkBitmapProcState;
};

class SkAutoLockPixels : SkNoncopyable {
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






class SkAutoLockColors : SkNoncopyable {
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
            fCTable->unlockColors();
        }
    }

    


    const SkPMColor* colors() const { return fColors; }

    


    const SkPMColor* lockColors(SkColorTable* ctable) {
        if (fCTable) {
            fCTable->unlockColors();
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
#define SkAutoLockColors(...) SK_REQUIRE_LOCAL_VAR(SkAutoLockColors)



inline uint32_t* SkBitmap::getAddr32(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(4 == this->bytesPerPixel());
    SkASSERT((unsigned)x < (unsigned)this->width() && (unsigned)y < (unsigned)this->height());
    return (uint32_t*)((char*)fPixels + y * fRowBytes + (x << 2));
}

inline uint16_t* SkBitmap::getAddr16(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(2 == this->bytesPerPixel());
    SkASSERT((unsigned)x < (unsigned)this->width() && (unsigned)y < (unsigned)this->height());
    return (uint16_t*)((char*)fPixels + y * fRowBytes + (x << 1));
}

inline uint8_t* SkBitmap::getAddr8(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(1 == this->bytesPerPixel());
    SkASSERT((unsigned)x < (unsigned)this->width() && (unsigned)y < (unsigned)this->height());
    return (uint8_t*)fPixels + y * fRowBytes + x;
}

inline SkPMColor SkBitmap::getIndex8Color(int x, int y) const {
    SkASSERT(fPixels);
    SkASSERT(kIndex_8_SkColorType == this->colorType());
    SkASSERT((unsigned)x < (unsigned)this->width() && (unsigned)y < (unsigned)this->height());
    SkASSERT(fColorTable);
    return (*fColorTable)[*((const uint8_t*)fPixels + y * fRowBytes + x)];
}

#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG




SK_API SkBitmap::Config SkColorTypeToBitmapConfig(SkColorType);
SK_API SkColorType SkBitmapConfigToColorType(SkBitmap::Config);
#endif

#endif
