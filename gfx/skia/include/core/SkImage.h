






#ifndef SkImage_DEFINED
#define SkImage_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"

class SkData;
class SkCanvas;
class SkPaint;
class SkShader;
class GrContext;
struct GrPlatformTextureDesc;


#include "SkShader.h"



class SkColorSpace;











class SkImage : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkImage)

    enum ColorType {
        kAlpha_8_ColorType,
        kRGB_565_ColorType,
        kRGBA_8888_ColorType,
        kBGRA_8888_ColorType,
        kPMColor_ColorType,

        kLastEnum_ColorType = kPMColor_ColorType
    };

    enum AlphaType {
        kIgnore_AlphaType,
        kOpaque_AlphaType,
        kPremul_AlphaType,
        kUnpremul_AlphaType,

        kLastEnum_AlphaType = kUnpremul_AlphaType
    };

    struct Info {
        int         fWidth;
        int         fHeight;
        ColorType   fColorType;
        AlphaType   fAlphaType;

    };

    static SkImage* NewRasterCopy(const Info&, SkColorSpace*, const void* pixels, size_t rowBytes);
    static SkImage* NewRasterData(const Info&, SkColorSpace*, SkData* pixels, size_t rowBytes);
    static SkImage* NewEncodedData(SkData*);
    static SkImage* NewTexture(GrContext*, const GrPlatformTextureDesc&);

    int width() const { return fWidth; }
    int height() const { return fHeight; }
    uint32_t uniqueID() const { return fUniqueID; }

    SkShader*   newShaderClamp() const;
    SkShader*   newShader(SkShader::TileMode, SkShader::TileMode) const;

    void draw(SkCanvas*, SkScalar x, SkScalar y, const SkPaint*);

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
};

#endif
