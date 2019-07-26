






#ifndef GrTextureAccess_DEFINED
#define GrTextureAccess_DEFINED

#include "GrNoncopyable.h"
#include "SkRefCnt.h"
#include "SkShader.h"

class GrTexture;







class GrTextureParams {
public:
    GrTextureParams() {
        this->reset();
    }

    GrTextureParams(SkShader::TileMode tileXAndY, bool bilerp) {
        this->reset(tileXAndY, bilerp);
    }

    GrTextureParams(SkShader::TileMode tileModes[2], bool bilerp) {
        this->reset(tileModes, bilerp);
    }

    GrTextureParams(const GrTextureParams& params) {
        *this = params;
    }

    GrTextureParams& operator= (const GrTextureParams& params) {
        fTileModes[0] = params.fTileModes[0];
        fTileModes[1] = params.fTileModes[1];
        fBilerp = params.fBilerp;
        return *this;
    }

    void reset() {
        this->reset(SkShader::kClamp_TileMode, false);
    }

    void reset(SkShader::TileMode tileXAndY, bool bilerp) {
        fTileModes[0] = fTileModes[1] = tileXAndY;
        fBilerp = bilerp;
    }

    void reset(SkShader::TileMode tileModes[2], bool bilerp) {
        fTileModes[0] = tileModes[0];
        fTileModes[1] = tileModes[1];
        fBilerp = bilerp;
    }

    void setClampNoFilter() {
        fTileModes[0] = fTileModes[1] = SkShader::kClamp_TileMode;
        fBilerp = false;
    }

    void setClamp() {
        fTileModes[0] = fTileModes[1] = SkShader::kClamp_TileMode;
    }

    void setBilerp(bool bilerp) { fBilerp = bilerp; }

    void setTileModeX(const SkShader::TileMode tm) { fTileModes[0] = tm; }
    void setTileModeY(const SkShader::TileMode tm) { fTileModes[1] = tm; }
    void setTileModeXAndY(const SkShader::TileMode tm) { fTileModes[0] = fTileModes[1] = tm; }

    SkShader::TileMode getTileModeX() const { return fTileModes[0]; }

    SkShader::TileMode getTileModeY() const { return fTileModes[1]; }

    bool isTiled() const {
        return SkShader::kClamp_TileMode != fTileModes[0] ||
               SkShader::kClamp_TileMode != fTileModes[1];
    }

    bool isBilerp() const { return fBilerp; }

    bool operator== (const GrTextureParams& other) const {
        return fTileModes[0] == other.fTileModes[0] &&
               fTileModes[1] == other.fTileModes[1] &&
               fBilerp == other.fBilerp;
    }

    bool operator!= (const GrTextureParams& other) const { return !(*this == other); }

private:

    SkShader::TileMode fTileModes[2];
    bool               fBilerp;
};







class GrTextureAccess : GrNoncopyable {
public:
    



    GrTextureAccess();

    


    GrTextureAccess(GrTexture*, const GrTextureParams&);
    explicit GrTextureAccess(GrTexture*,
                             bool bilerp = false,
                             SkShader::TileMode tileXAndY = SkShader::kClamp_TileMode);

    



    GrTextureAccess(GrTexture*, const char* swizzle, const GrTextureParams&);
    GrTextureAccess(GrTexture*,
                    const char* swizzle,
                    bool bilerp = false,
                    SkShader::TileMode tileXAndY = SkShader::kClamp_TileMode);

    void reset(GrTexture*, const GrTextureParams&);
    void reset(GrTexture*,
               bool bilerp = false,
               SkShader::TileMode tileXAndY = SkShader::kClamp_TileMode);
    void reset(GrTexture*, const char* swizzle, const GrTextureParams&);
    void reset(GrTexture*,
               const char* swizzle,
               bool bilerp = false,
               SkShader::TileMode tileXAndY = SkShader::kClamp_TileMode);

    bool operator== (const GrTextureAccess& other) const {
#if GR_DEBUG
        
        GrAssert(memcmp(fSwizzle, other.fSwizzle, sizeof(fSwizzle)-1) ==
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

    typedef GrNoncopyable INHERITED;
};

#endif
