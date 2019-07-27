






#ifndef GrTextureAccess_DEFINED
#define GrTextureAccess_DEFINED

#include "SkRefCnt.h"
#include "SkShader.h"
#include "SkTypes.h"

class GrTexture;







class GrTextureParams {
public:
    GrTextureParams() {
        this->reset();
    }

    enum FilterMode {
        kNone_FilterMode,
        kBilerp_FilterMode,
        kMipMap_FilterMode
    };

    GrTextureParams(SkShader::TileMode tileXAndY, FilterMode filterMode) {
        this->reset(tileXAndY, filterMode);
    }

    GrTextureParams(const SkShader::TileMode tileModes[2], FilterMode filterMode) {
        this->reset(tileModes, filterMode);
    }

    GrTextureParams(const GrTextureParams& params) {
        *this = params;
    }

    GrTextureParams& operator= (const GrTextureParams& params) {
        fTileModes[0] = params.fTileModes[0];
        fTileModes[1] = params.fTileModes[1];
        fFilterMode = params.fFilterMode;
        return *this;
    }

    void reset() {
        this->reset(SkShader::kClamp_TileMode, kNone_FilterMode);
    }

    void reset(SkShader::TileMode tileXAndY, FilterMode filterMode) {
        fTileModes[0] = fTileModes[1] = tileXAndY;
        fFilterMode = filterMode;
    }

    void reset(const SkShader::TileMode tileModes[2], FilterMode filterMode) {
        fTileModes[0] = tileModes[0];
        fTileModes[1] = tileModes[1];
        fFilterMode = filterMode;
    }

    void setClampNoFilter() {
        fTileModes[0] = fTileModes[1] = SkShader::kClamp_TileMode;
        fFilterMode = kNone_FilterMode;
    }

    void setClamp() {
        fTileModes[0] = fTileModes[1] = SkShader::kClamp_TileMode;
    }

    void setFilterMode(FilterMode filterMode) { fFilterMode = filterMode; }

    void setTileModeX(const SkShader::TileMode tm) { fTileModes[0] = tm; }
    void setTileModeY(const SkShader::TileMode tm) { fTileModes[1] = tm; }
    void setTileModeXAndY(const SkShader::TileMode tm) { fTileModes[0] = fTileModes[1] = tm; }

    SkShader::TileMode getTileModeX() const { return fTileModes[0]; }

    SkShader::TileMode getTileModeY() const { return fTileModes[1]; }

    bool isTiled() const {
        return SkShader::kClamp_TileMode != fTileModes[0] ||
               SkShader::kClamp_TileMode != fTileModes[1];
    }

    FilterMode filterMode() const { return fFilterMode; }

    bool operator== (const GrTextureParams& other) const {
        return fTileModes[0] == other.fTileModes[0] &&
               fTileModes[1] == other.fTileModes[1] &&
               fFilterMode == other.fFilterMode;
    }

    bool operator!= (const GrTextureParams& other) const { return !(*this == other); }

private:

    SkShader::TileMode fTileModes[2];
    FilterMode         fFilterMode;
};







class GrTextureAccess : SkNoncopyable {
public:
    



    GrTextureAccess();

    


    GrTextureAccess(GrTexture*, const GrTextureParams&);
    explicit GrTextureAccess(GrTexture*,
                             GrTextureParams::FilterMode = GrTextureParams::kNone_FilterMode,
                             SkShader::TileMode tileXAndY = SkShader::kClamp_TileMode);

    



    GrTextureAccess(GrTexture*, const char* swizzle, const GrTextureParams&);
    GrTextureAccess(GrTexture*,
                    const char* swizzle,
                    GrTextureParams::FilterMode = GrTextureParams::kNone_FilterMode,
                    SkShader::TileMode tileXAndY = SkShader::kClamp_TileMode);

    void reset(GrTexture*, const GrTextureParams&);
    void reset(GrTexture*,
               GrTextureParams::FilterMode = GrTextureParams::kNone_FilterMode,
               SkShader::TileMode tileXAndY = SkShader::kClamp_TileMode);
    void reset(GrTexture*, const char* swizzle, const GrTextureParams&);
    void reset(GrTexture*,
               const char* swizzle,
               GrTextureParams::FilterMode = GrTextureParams::kNone_FilterMode,
               SkShader::TileMode tileXAndY = SkShader::kClamp_TileMode);

    bool operator== (const GrTextureAccess& other) const {
#ifdef SK_DEBUG
        
        SkASSERT(memcmp(fSwizzle, other.fSwizzle, sizeof(fSwizzle)-1) ==
                 strcmp(fSwizzle, other.fSwizzle));
#endif
        return fParams == other.fParams &&
               (fTexture.get() == other.fTexture.get()) &&
               (0 == memcmp(fSwizzle, other.fSwizzle, sizeof(fSwizzle)-1));
    }

    bool operator!= (const GrTextureAccess& other) const { return !(*this == other); }

    GrTexture* getTexture() const { return fTexture.get(); }

    


    const char* getSwizzle() const { return fSwizzle; }

    

    uint32_t swizzleMask() const { return fSwizzleMask; }

    const GrTextureParams& getParams() const { return fParams; }

private:
    void setSwizzle(const char*);

    GrTextureParams         fParams;
    SkAutoTUnref<GrTexture> fTexture;
    uint32_t                fSwizzleMask;
    char                    fSwizzle[5];

    typedef SkNoncopyable INHERITED;
};

#endif
