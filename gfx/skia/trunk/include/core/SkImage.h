






#ifndef SkImage_DEFINED
#define SkImage_DEFINED

#include "SkImageInfo.h"
#include "SkImageEncoder.h"
#include "SkRefCnt.h"
#include "SkScalar.h"
#include "SkShader.h"

class SkData;
class SkCanvas;
class SkPaint;
class GrContext;
class GrTexture;











class SK_API SkImage : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkImage)

    typedef SkImageInfo Info;

    static SkImage* NewRasterCopy(const Info&, const void* pixels, size_t rowBytes);
    static SkImage* NewRasterData(const Info&, SkData* pixels, size_t rowBytes);
    static SkImage* NewEncodedData(SkData*);

    




    static SkImage* NewTexture(const SkBitmap&);

    int width() const { return fWidth; }
    int height() const { return fHeight; }
    uint32_t uniqueID() const { return fUniqueID; }

    




    GrTexture* getTexture();

    virtual SkShader* newShader(SkShader::TileMode,
                                SkShader::TileMode,
                                const SkMatrix* localMatrix = NULL) const;

    void draw(SkCanvas*, SkScalar x, SkScalar y, const SkPaint*);

    






    void draw(SkCanvas*, const SkRect* src, const SkRect& dst, const SkPaint*);

    








    const void* peekPixels(SkImageInfo* info, size_t* rowBytes) const;

    






    SkData* encode(SkImageEncoder::Type t = SkImageEncoder::kPNG_Type,
                   int quality = 80) const;

protected:
    SkImage(int width, int height) :
        fWidth(width),
        fHeight(height),
        fUniqueID(NextUniqueID()) {

        SkASSERT(width >= 0);
        SkASSERT(height >= 0);
    }

private:
    const int       fWidth;
    const int       fHeight;
    const uint32_t  fUniqueID;

    static uint32_t NextUniqueID();

    typedef SkRefCnt INHERITED;

    
















    
    
    
    
    bool readPixels(SkBitmap* bitmap, const SkIRect* subset = NULL) const;
};

#endif
