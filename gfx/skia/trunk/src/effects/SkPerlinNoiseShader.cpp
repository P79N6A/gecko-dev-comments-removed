






#include "SkDither.h"
#include "SkPerlinNoiseShader.h"
#include "SkColorFilter.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"
#include "SkShader.h"
#include "SkUnPreMultiply.h"
#include "SkString.h"

#if SK_SUPPORT_GPU
#include "GrContext.h"
#include "GrCoordTransform.h"
#include "gl/GrGLEffect.h"
#include "GrTBackendEffectFactory.h"
#include "SkGr.h"
#endif

static const int kBlockSize = 256;
static const int kBlockMask = kBlockSize - 1;
static const int kPerlinNoise = 4096;
static const int kRandMaximum = SK_MaxS32; 

namespace {




inline int checkNoise(int noiseValue, int limitValue, int newValue) {
    
    
    
    if (noiseValue >= limitValue) {
        noiseValue -= newValue;
    }
    if (noiseValue >= limitValue - 1) {
        noiseValue -= newValue - 1;
    }
    return noiseValue;
}

inline SkScalar smoothCurve(SkScalar t) {
    static const SkScalar SK_Scalar3 = 3.0f;

    
    return SkScalarMul(SkScalarSquare(t), SK_Scalar3 - 2 * t);
}

bool perlin_noise_type_is_valid(SkPerlinNoiseShader::Type type) {
    return (SkPerlinNoiseShader::kFractalNoise_Type == type) ||
           (SkPerlinNoiseShader::kTurbulence_Type == type);
}

} 

struct SkPerlinNoiseShader::StitchData {
    StitchData()
      : fWidth(0)
      , fWrapX(0)
      , fHeight(0)
      , fWrapY(0)
    {}

    bool operator==(const StitchData& other) const {
        return fWidth == other.fWidth &&
               fWrapX == other.fWrapX &&
               fHeight == other.fHeight &&
               fWrapY == other.fWrapY;
    }

    int fWidth; 
    int fWrapX; 
    int fHeight;
    int fWrapY;
};

struct SkPerlinNoiseShader::PaintingData {
    PaintingData(const SkISize& tileSize, SkScalar seed,
                 SkScalar baseFrequencyX, SkScalar baseFrequencyY)
      : fTileSize(tileSize)
      , fBaseFrequency(SkPoint::Make(baseFrequencyX, baseFrequencyY))
    {
        this->init(seed);
        if (!fTileSize.isEmpty()) {
            this->stitch();
        }

#if SK_SUPPORT_GPU && !defined(SK_USE_SIMPLEX_NOISE)
        fPermutationsBitmap.setConfig(SkImageInfo::MakeA8(kBlockSize, 1));
        fPermutationsBitmap.setPixels(fLatticeSelector);

        fNoiseBitmap.setConfig(SkImageInfo::MakeN32Premul(kBlockSize, 4));
        fNoiseBitmap.setPixels(fNoise[0][0]);
#endif
    }

    int         fSeed;
    uint8_t     fLatticeSelector[kBlockSize];
    uint16_t    fNoise[4][kBlockSize][2];
    SkPoint     fGradient[4][kBlockSize];
    SkISize     fTileSize;
    SkVector    fBaseFrequency;
    StitchData  fStitchDataInit;

private:

#if SK_SUPPORT_GPU && !defined(SK_USE_SIMPLEX_NOISE)
    SkBitmap   fPermutationsBitmap;
    SkBitmap   fNoiseBitmap;
#endif

    inline int random()  {
        static const int gRandAmplitude = 16807; 
        static const int gRandQ = 127773; 
        static const int gRandR = 2836; 

        int result = gRandAmplitude * (fSeed % gRandQ) - gRandR * (fSeed / gRandQ);
        if (result <= 0)
            result += kRandMaximum;
        fSeed = result;
        return result;
    }

    
    void init(SkScalar seed)
    {
        static const SkScalar gInvBlockSizef = SkScalarInvert(SkIntToScalar(kBlockSize));

        
        fSeed = SkScalarTruncToInt(seed);
        
        if (fSeed <= 0) {
            fSeed = -(fSeed % (kRandMaximum - 1)) + 1;
        }
        if (fSeed > kRandMaximum - 1) {
            fSeed = kRandMaximum - 1;
        }
        for (int channel = 0; channel < 4; ++channel) {
            for (int i = 0; i < kBlockSize; ++i) {
                fLatticeSelector[i] = i;
                fNoise[channel][i][0] = (random() % (2 * kBlockSize));
                fNoise[channel][i][1] = (random() % (2 * kBlockSize));
            }
        }
        for (int i = kBlockSize - 1; i > 0; --i) {
            int k = fLatticeSelector[i];
            int j = random() % kBlockSize;
            SkASSERT(j >= 0);
            SkASSERT(j < kBlockSize);
            fLatticeSelector[i] = fLatticeSelector[j];
            fLatticeSelector[j] = k;
        }

        
        {
            
            uint16_t noise[4][kBlockSize][2];
            for (int i = 0; i < kBlockSize; ++i) {
                for (int channel = 0; channel < 4; ++channel) {
                    for (int j = 0; j < 2; ++j) {
                        noise[channel][i][j] = fNoise[channel][i][j];
                    }
                }
            }
            
            for (int i = 0; i < kBlockSize; ++i) {
                for (int channel = 0; channel < 4; ++channel) {
                    for (int j = 0; j < 2; ++j) {
                        fNoise[channel][i][j] = noise[channel][fLatticeSelector[i]][j];
                    }
                }
            }
        }

        
        static const SkScalar gHalfMax16bits = 32767.5f;

        
        for (int channel = 0; channel < 4; ++channel) {
            for (int i = 0; i < kBlockSize; ++i) {
                fGradient[channel][i] = SkPoint::Make(
                    SkScalarMul(SkIntToScalar(fNoise[channel][i][0] - kBlockSize),
                                gInvBlockSizef),
                    SkScalarMul(SkIntToScalar(fNoise[channel][i][1] - kBlockSize),
                                gInvBlockSizef));
                fGradient[channel][i].normalize();
                
                fNoise[channel][i][0] = SkScalarRoundToInt(SkScalarMul(
                    fGradient[channel][i].fX + SK_Scalar1, gHalfMax16bits));
                fNoise[channel][i][1] = SkScalarRoundToInt(SkScalarMul(
                    fGradient[channel][i].fY + SK_Scalar1, gHalfMax16bits));
            }
        }
    }

    
    void stitch() {
        SkScalar tileWidth  = SkIntToScalar(fTileSize.width());
        SkScalar tileHeight = SkIntToScalar(fTileSize.height());
        SkASSERT(tileWidth > 0 && tileHeight > 0);
        
        
        if (fBaseFrequency.fX) {
            SkScalar lowFrequencx =
                SkScalarFloorToScalar(tileWidth * fBaseFrequency.fX) / tileWidth;
            SkScalar highFrequencx =
                SkScalarCeilToScalar(tileWidth * fBaseFrequency.fX) / tileWidth;
            
            if (SkScalarDiv(fBaseFrequency.fX, lowFrequencx) <
                SkScalarDiv(highFrequencx, fBaseFrequency.fX)) {
                fBaseFrequency.fX = lowFrequencx;
            } else {
                fBaseFrequency.fX = highFrequencx;
            }
        }
        if (fBaseFrequency.fY) {
            SkScalar lowFrequency =
                SkScalarFloorToScalar(tileHeight * fBaseFrequency.fY) / tileHeight;
            SkScalar highFrequency =
                SkScalarCeilToScalar(tileHeight * fBaseFrequency.fY) / tileHeight;
            if (SkScalarDiv(fBaseFrequency.fY, lowFrequency) <
                SkScalarDiv(highFrequency, fBaseFrequency.fY)) {
                fBaseFrequency.fY = lowFrequency;
            } else {
                fBaseFrequency.fY = highFrequency;
            }
        }
        
        fStitchDataInit.fWidth  =
            SkScalarRoundToInt(tileWidth * fBaseFrequency.fX);
        fStitchDataInit.fWrapX  = kPerlinNoise + fStitchDataInit.fWidth;
        fStitchDataInit.fHeight =
            SkScalarRoundToInt(tileHeight * fBaseFrequency.fY);
        fStitchDataInit.fWrapY  = kPerlinNoise + fStitchDataInit.fHeight;
    }

public:

#if SK_SUPPORT_GPU && !defined(SK_USE_SIMPLEX_NOISE)
    const SkBitmap& getPermutationsBitmap() const { return fPermutationsBitmap; }

    const SkBitmap& getNoiseBitmap() const { return fNoiseBitmap; }
#endif
};

SkShader* SkPerlinNoiseShader::CreateFractalNoise(SkScalar baseFrequencyX, SkScalar baseFrequencyY,
                                                  int numOctaves, SkScalar seed,
                                                  const SkISize* tileSize) {
    return SkNEW_ARGS(SkPerlinNoiseShader, (kFractalNoise_Type, baseFrequencyX, baseFrequencyY,
                                            numOctaves, seed, tileSize));
}

SkShader* SkPerlinNoiseShader::CreateTubulence(SkScalar baseFrequencyX, SkScalar baseFrequencyY,
                                              int numOctaves, SkScalar seed,
                                              const SkISize* tileSize) {
    return SkNEW_ARGS(SkPerlinNoiseShader, (kTurbulence_Type, baseFrequencyX, baseFrequencyY,
                                            numOctaves, seed, tileSize));
}

SkPerlinNoiseShader::SkPerlinNoiseShader(SkPerlinNoiseShader::Type type,
                                         SkScalar baseFrequencyX,
                                         SkScalar baseFrequencyY,
                                         int numOctaves,
                                         SkScalar seed,
                                         const SkISize* tileSize)
  : fType(type)
  , fBaseFrequencyX(baseFrequencyX)
  , fBaseFrequencyY(baseFrequencyY)
  , fNumOctaves(numOctaves > 255 ? 255 : numOctaves)
  , fSeed(seed)
  , fTileSize(NULL == tileSize ? SkISize::Make(0, 0) : *tileSize)
  , fStitchTiles(!fTileSize.isEmpty())
{
    SkASSERT(numOctaves >= 0 && numOctaves < 256);
    fMatrix.reset();
    fPaintingData = SkNEW_ARGS(PaintingData, (fTileSize, fSeed, fBaseFrequencyX, fBaseFrequencyY));
}

SkPerlinNoiseShader::SkPerlinNoiseShader(SkReadBuffer& buffer)
    : INHERITED(buffer)
{
    fType           = (SkPerlinNoiseShader::Type) buffer.readInt();
    fBaseFrequencyX = buffer.readScalar();
    fBaseFrequencyY = buffer.readScalar();
    fNumOctaves     = buffer.readInt();
    fSeed           = buffer.readScalar();
    fStitchTiles    = buffer.readBool();
    fTileSize.fWidth  = buffer.readInt();
    fTileSize.fHeight = buffer.readInt();
    fMatrix.reset();
    fPaintingData = SkNEW_ARGS(PaintingData, (fTileSize, fSeed, fBaseFrequencyX, fBaseFrequencyY));
    buffer.validate(perlin_noise_type_is_valid(fType) &&
                    (fNumOctaves >= 0) && (fNumOctaves <= 255) &&
                    (fStitchTiles != fTileSize.isEmpty()));
}

SkPerlinNoiseShader::~SkPerlinNoiseShader() {
    
    SkDELETE(fPaintingData);
}

void SkPerlinNoiseShader::flatten(SkWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeInt((int) fType);
    buffer.writeScalar(fBaseFrequencyX);
    buffer.writeScalar(fBaseFrequencyY);
    buffer.writeInt(fNumOctaves);
    buffer.writeScalar(fSeed);
    buffer.writeBool(fStitchTiles);
    buffer.writeInt(fTileSize.fWidth);
    buffer.writeInt(fTileSize.fHeight);
}

SkScalar SkPerlinNoiseShader::noise2D(int channel, const PaintingData& paintingData,
                                      const StitchData& stitchData,
                                      const SkPoint& noiseVector) const {
    struct Noise {
        int noisePositionIntegerValue;
        SkScalar noisePositionFractionValue;
        Noise(SkScalar component)
        {
            SkScalar position = component + kPerlinNoise;
            noisePositionIntegerValue = SkScalarFloorToInt(position);
            noisePositionFractionValue = position - SkIntToScalar(noisePositionIntegerValue);
        }
    };
    Noise noiseX(noiseVector.x());
    Noise noiseY(noiseVector.y());
    SkScalar u, v;
    
    if (fStitchTiles) {
        noiseX.noisePositionIntegerValue =
            checkNoise(noiseX.noisePositionIntegerValue, stitchData.fWrapX, stitchData.fWidth);
        noiseY.noisePositionIntegerValue =
            checkNoise(noiseY.noisePositionIntegerValue, stitchData.fWrapY, stitchData.fHeight);
    }
    noiseX.noisePositionIntegerValue &= kBlockMask;
    noiseY.noisePositionIntegerValue &= kBlockMask;
    int latticeIndex =
        paintingData.fLatticeSelector[noiseX.noisePositionIntegerValue] +
        noiseY.noisePositionIntegerValue;
    int nextLatticeIndex =
        paintingData.fLatticeSelector[(noiseX.noisePositionIntegerValue + 1) & kBlockMask] +
        noiseY.noisePositionIntegerValue;
    SkScalar sx = smoothCurve(noiseX.noisePositionFractionValue);
    SkScalar sy = smoothCurve(noiseY.noisePositionFractionValue);
    
    SkPoint fractionValue = SkPoint::Make(noiseX.noisePositionFractionValue,
                                          noiseY.noisePositionFractionValue); 
    u = paintingData.fGradient[channel][latticeIndex & kBlockMask].dot(fractionValue);
    fractionValue.fX -= SK_Scalar1; 
    v = paintingData.fGradient[channel][nextLatticeIndex & kBlockMask].dot(fractionValue);
    SkScalar a = SkScalarInterp(u, v, sx);
    fractionValue.fY -= SK_Scalar1; 
    v = paintingData.fGradient[channel][(nextLatticeIndex + 1) & kBlockMask].dot(fractionValue);
    fractionValue.fX = noiseX.noisePositionFractionValue; 
    u = paintingData.fGradient[channel][(latticeIndex + 1) & kBlockMask].dot(fractionValue);
    SkScalar b = SkScalarInterp(u, v, sx);
    return SkScalarInterp(a, b, sy);
}

SkScalar SkPerlinNoiseShader::calculateTurbulenceValueForPoint(int channel,
                                                               const PaintingData& paintingData,
                                                               StitchData& stitchData,
                                                               const SkPoint& point) const {
    if (fStitchTiles) {
        
        stitchData = paintingData.fStitchDataInit;
    }
    SkScalar turbulenceFunctionResult = 0;
    SkPoint noiseVector(SkPoint::Make(SkScalarMul(point.x(), paintingData.fBaseFrequency.fX),
                                      SkScalarMul(point.y(), paintingData.fBaseFrequency.fY)));
    SkScalar ratio = SK_Scalar1;
    for (int octave = 0; octave < fNumOctaves; ++octave) {
        SkScalar noise = noise2D(channel, paintingData, stitchData, noiseVector);
        turbulenceFunctionResult += SkScalarDiv(
            (fType == kFractalNoise_Type) ? noise : SkScalarAbs(noise), ratio);
        noiseVector.fX *= 2;
        noiseVector.fY *= 2;
        ratio *= 2;
        if (fStitchTiles) {
            
            stitchData.fWidth  *= 2;
            stitchData.fWrapX   = stitchData.fWidth + kPerlinNoise;
            stitchData.fHeight *= 2;
            stitchData.fWrapY   = stitchData.fHeight + kPerlinNoise;
        }
    }

    
    
    if (fType == kFractalNoise_Type) {
        turbulenceFunctionResult =
            SkScalarMul(turbulenceFunctionResult, SK_ScalarHalf) + SK_ScalarHalf;
    }

    if (channel == 3) { 
        turbulenceFunctionResult = SkScalarMul(turbulenceFunctionResult,
            SkScalarDiv(SkIntToScalar(getPaintAlpha()), SkIntToScalar(255)));
    }

    
    return SkScalarPin(turbulenceFunctionResult, 0, SK_Scalar1);
}

SkPMColor SkPerlinNoiseShader::shade(const SkPoint& point, StitchData& stitchData) const {
    SkMatrix matrix = fMatrix;
    matrix.postConcat(getLocalMatrix());
    SkMatrix invMatrix;
    if (!matrix.invert(&invMatrix)) {
        invMatrix.reset();
    } else {
        invMatrix.postConcat(invMatrix); 
    }
    
    
    matrix.postTranslate(SK_Scalar1, SK_Scalar1);
    SkPoint newPoint;
    matrix.mapPoints(&newPoint, &point, 1);
    invMatrix.mapPoints(&newPoint, &newPoint, 1);
    newPoint.fX = SkScalarRoundToScalar(newPoint.fX);
    newPoint.fY = SkScalarRoundToScalar(newPoint.fY);

    U8CPU rgba[4];
    for (int channel = 3; channel >= 0; --channel) {
        rgba[channel] = SkScalarFloorToInt(255 *
            calculateTurbulenceValueForPoint(channel, *fPaintingData, stitchData, newPoint));
    }
    return SkPreMultiplyARGB(rgba[3], rgba[0], rgba[1], rgba[2]);
}

bool SkPerlinNoiseShader::setContext(const SkBitmap& device, const SkPaint& paint,
                                     const SkMatrix& matrix) {
    fMatrix = matrix;
    return INHERITED::setContext(device, paint, matrix);
}

void SkPerlinNoiseShader::shadeSpan(int x, int y, SkPMColor result[], int count) {
    SkPoint point = SkPoint::Make(SkIntToScalar(x), SkIntToScalar(y));
    StitchData stitchData;
    for (int i = 0; i < count; ++i) {
        result[i] = shade(point, stitchData);
        point.fX += SK_Scalar1;
    }
}

void SkPerlinNoiseShader::shadeSpan16(int x, int y, uint16_t result[], int count) {
    SkPoint point = SkPoint::Make(SkIntToScalar(x), SkIntToScalar(y));
    StitchData stitchData;
    DITHER_565_SCAN(y);
    for (int i = 0; i < count; ++i) {
        unsigned dither = DITHER_VALUE(x);
        result[i] = SkDitherRGB32To565(shade(point, stitchData), dither);
        DITHER_INC_X(x);
        point.fX += SK_Scalar1;
    }
}



#if SK_SUPPORT_GPU

#include "GrTBackendEffectFactory.h"

class GrGLNoise : public GrGLEffect {
public:
    GrGLNoise(const GrBackendEffectFactory& factory,
              const GrDrawEffect& drawEffect);
    virtual ~GrGLNoise() {}

    static inline EffectKey GenKey(const GrDrawEffect&, const GrGLCaps&);

    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

protected:
    SkPerlinNoiseShader::Type           fType;
    bool                                fStitchTiles;
    int                                 fNumOctaves;
    GrGLUniformManager::UniformHandle   fBaseFrequencyUni;
    GrGLUniformManager::UniformHandle   fAlphaUni;
    GrGLUniformManager::UniformHandle   fInvMatrixUni;

private:
    typedef GrGLEffect INHERITED;
};

class GrGLPerlinNoise : public GrGLNoise {
public:
    GrGLPerlinNoise(const GrBackendEffectFactory& factory,
                    const GrDrawEffect& drawEffect)
      : GrGLNoise(factory, drawEffect) {}
    virtual ~GrGLPerlinNoise() {}

    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          EffectKey,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;

    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

private:
    GrGLUniformManager::UniformHandle fStitchDataUni;

    typedef GrGLNoise INHERITED;
};

class GrGLSimplexNoise : public GrGLNoise {
    
public:
    GrGLSimplexNoise(const GrBackendEffectFactory& factory,
                     const GrDrawEffect& drawEffect)
      : GrGLNoise(factory, drawEffect) {}

    virtual ~GrGLSimplexNoise() {}

    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          EffectKey,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;

    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

private:
    GrGLUniformManager::UniformHandle fSeedUni;

    typedef GrGLNoise INHERITED;
};



class GrNoiseEffect : public GrEffect {
public:
    virtual ~GrNoiseEffect() { }

    SkPerlinNoiseShader::Type type() const { return fType; }
    bool stitchTiles() const { return fStitchTiles; }
    const SkVector& baseFrequency() const { return fBaseFrequency; }
    int numOctaves() const { return fNumOctaves; }
    const SkMatrix& matrix() const { return fCoordTransform.getMatrix(); }
    uint8_t alpha() const { return fAlpha; }

    void getConstantColorComponents(GrColor*, uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0; 
    }

protected:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const GrNoiseEffect& s = CastEffect<GrNoiseEffect>(sBase);
        return fType == s.fType &&
               fBaseFrequency == s.fBaseFrequency &&
               fNumOctaves == s.fNumOctaves &&
               fStitchTiles == s.fStitchTiles &&
               fCoordTransform.getMatrix() == s.fCoordTransform.getMatrix() &&
               fAlpha == s.fAlpha;
    }

    GrNoiseEffect(SkPerlinNoiseShader::Type type, const SkVector& baseFrequency, int numOctaves,
                  bool stitchTiles, const SkMatrix& matrix, uint8_t alpha)
      : fType(type)
      , fBaseFrequency(baseFrequency)
      , fNumOctaves(numOctaves)
      , fStitchTiles(stitchTiles)
      , fMatrix(matrix)
      , fAlpha(alpha) {
        
        
        SkMatrix m = matrix;
        m.postTranslate(SK_Scalar1, SK_Scalar1);
        fCoordTransform.reset(kLocal_GrCoordSet, m);
        this->addCoordTransform(&fCoordTransform);
        this->setWillNotUseInputColor();
    }

    SkPerlinNoiseShader::Type       fType;
    GrCoordTransform                fCoordTransform;
    SkVector                        fBaseFrequency;
    int                             fNumOctaves;
    bool                            fStitchTiles;
    SkMatrix                        fMatrix;
    uint8_t                         fAlpha;

private:
    typedef GrEffect INHERITED;
};

class GrPerlinNoiseEffect : public GrNoiseEffect {
public:
    static GrEffectRef* Create(SkPerlinNoiseShader::Type type, const SkVector& baseFrequency,
                               int numOctaves, bool stitchTiles,
                               const SkPerlinNoiseShader::StitchData& stitchData,
                               GrTexture* permutationsTexture, GrTexture* noiseTexture,
                               const SkMatrix& matrix, uint8_t alpha) {
        AutoEffectUnref effect(SkNEW_ARGS(GrPerlinNoiseEffect, (type, baseFrequency, numOctaves,
            stitchTiles, stitchData, permutationsTexture, noiseTexture, matrix, alpha)));
        return CreateEffectRef(effect);
    }

    virtual ~GrPerlinNoiseEffect() { }

    static const char* Name() { return "PerlinNoise"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<GrPerlinNoiseEffect>::getInstance();
    }
    const SkPerlinNoiseShader::StitchData& stitchData() const { return fStitchData; }

    typedef GrGLPerlinNoise GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const GrPerlinNoiseEffect& s = CastEffect<GrPerlinNoiseEffect>(sBase);
        return INHERITED::onIsEqual(sBase) &&
               fPermutationsAccess.getTexture() == s.fPermutationsAccess.getTexture() &&
               fNoiseAccess.getTexture() == s.fNoiseAccess.getTexture() &&
               fStitchData == s.fStitchData;
    }

    GrPerlinNoiseEffect(SkPerlinNoiseShader::Type type, const SkVector& baseFrequency,
                        int numOctaves, bool stitchTiles,
                        const SkPerlinNoiseShader::StitchData& stitchData,
                        GrTexture* permutationsTexture, GrTexture* noiseTexture,
                        const SkMatrix& matrix, uint8_t alpha)
      : GrNoiseEffect(type, baseFrequency, numOctaves, stitchTiles, matrix, alpha)
      , fPermutationsAccess(permutationsTexture)
      , fNoiseAccess(noiseTexture)
      , fStitchData(stitchData) {
        this->addTextureAccess(&fPermutationsAccess);
        this->addTextureAccess(&fNoiseAccess);
    }

    GR_DECLARE_EFFECT_TEST;

    GrTextureAccess                 fPermutationsAccess;
    GrTextureAccess                 fNoiseAccess;
    SkPerlinNoiseShader::StitchData fStitchData;

    typedef GrNoiseEffect INHERITED;
};

class GrSimplexNoiseEffect : public GrNoiseEffect {
    
public:
    static GrEffectRef* Create(SkPerlinNoiseShader::Type type, const SkVector& baseFrequency,
                               int numOctaves, bool stitchTiles, const SkScalar seed,
                               const SkMatrix& matrix, uint8_t alpha) {
        AutoEffectUnref effect(SkNEW_ARGS(GrSimplexNoiseEffect, (type, baseFrequency, numOctaves,
            stitchTiles, seed, matrix, alpha)));
        return CreateEffectRef(effect);
    }

    virtual ~GrSimplexNoiseEffect() { }

    static const char* Name() { return "SimplexNoise"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<GrSimplexNoiseEffect>::getInstance();
    }
    const SkScalar& seed() const { return fSeed; }

    typedef GrGLSimplexNoise GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const GrSimplexNoiseEffect& s = CastEffect<GrSimplexNoiseEffect>(sBase);
        return INHERITED::onIsEqual(sBase) && fSeed == s.fSeed;
    }

    GrSimplexNoiseEffect(SkPerlinNoiseShader::Type type, const SkVector& baseFrequency,
                         int numOctaves, bool stitchTiles, const SkScalar seed,
                         const SkMatrix& matrix, uint8_t alpha)
      : GrNoiseEffect(type, baseFrequency, numOctaves, stitchTiles, matrix, alpha)
      , fSeed(seed) {
    }

    SkScalar fSeed;

    typedef GrNoiseEffect INHERITED;
};


GR_DEFINE_EFFECT_TEST(GrPerlinNoiseEffect);

GrEffectRef* GrPerlinNoiseEffect::TestCreate(SkRandom* random,
                                             GrContext* context,
                                             const GrDrawTargetCaps&,
                                             GrTexture**) {
    int      numOctaves = random->nextRangeU(2, 10);
    bool     stitchTiles = random->nextBool();
    SkScalar seed = SkIntToScalar(random->nextU());
    SkISize  tileSize = SkISize::Make(random->nextRangeU(4, 4096), random->nextRangeU(4, 4096));
    SkScalar baseFrequencyX = random->nextRangeScalar(0.01f,
                                                      0.99f);
    SkScalar baseFrequencyY = random->nextRangeScalar(0.01f,
                                                      0.99f);

    SkShader* shader = random->nextBool() ?
        SkPerlinNoiseShader::CreateFractalNoise(baseFrequencyX, baseFrequencyY, numOctaves, seed,
                                                stitchTiles ? &tileSize : NULL) :
        SkPerlinNoiseShader::CreateTubulence(baseFrequencyX, baseFrequencyY, numOctaves, seed,
                                             stitchTiles ? &tileSize : NULL);

    SkPaint paint;
    GrEffectRef* effect = shader->asNewEffect(context, paint);

    SkDELETE(shader);

    return effect;
}



void GrGLSimplexNoise::emitCode(GrGLShaderBuilder* builder,
                                const GrDrawEffect&,
                                EffectKey key,
                                const char* outputColor,
                                const char* inputColor,
                                const TransformedCoordsArray& coords,
                                const TextureSamplerArray&) {
    sk_ignore_unused_variable(inputColor);

    SkString vCoords = builder->ensureFSCoords2D(coords, 0);

    fSeedUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                   kFloat_GrSLType, "seed");
    const char* seedUni = builder->getUniformCStr(fSeedUni);
    fInvMatrixUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                        kMat33f_GrSLType, "invMatrix");
    const char* invMatrixUni = builder->getUniformCStr(fInvMatrixUni);
    fBaseFrequencyUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                            kVec2f_GrSLType, "baseFrequency");
    const char* baseFrequencyUni = builder->getUniformCStr(fBaseFrequencyUni);
    fAlphaUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                    kFloat_GrSLType, "alpha");
    const char* alphaUni = builder->getUniformCStr(fAlphaUni);

    
    static const GrGLShaderVar gVec3Args[] =  {
        GrGLShaderVar("x", kVec3f_GrSLType)
    };

    SkString mod289_3_funcName;
    builder->fsEmitFunction(kVec3f_GrSLType,
                            "mod289", SK_ARRAY_COUNT(gVec3Args), gVec3Args,
                            "const vec2 C = vec2(1.0 / 289.0, 289.0);\n"
                            "return x - floor(x * C.xxx) * C.yyy;", &mod289_3_funcName);

    
    static const GrGLShaderVar gVec4Args[] =  {
        GrGLShaderVar("x", kVec4f_GrSLType)
    };

    SkString mod289_4_funcName;
    builder->fsEmitFunction(kVec4f_GrSLType,
                            "mod289", SK_ARRAY_COUNT(gVec4Args), gVec4Args,
                            "const vec2 C = vec2(1.0 / 289.0, 289.0);\n"
                            "return x - floor(x * C.xxxx) * C.yyyy;", &mod289_4_funcName);

    
    SkString permuteCode;
    permuteCode.appendf("const vec2 C = vec2(34.0, 1.0);\n"
                        "return %s(((x * C.xxxx) + C.yyyy) * x);", mod289_4_funcName.c_str());
    SkString permuteFuncName;
    builder->fsEmitFunction(kVec4f_GrSLType,
                            "permute", SK_ARRAY_COUNT(gVec4Args), gVec4Args,
                            permuteCode.c_str(), &permuteFuncName);

    
    SkString taylorInvSqrtFuncName;
    builder->fsEmitFunction(kVec4f_GrSLType,
                            "taylorInvSqrt", SK_ARRAY_COUNT(gVec4Args), gVec4Args,
                            "const vec2 C = vec2(-0.85373472095314, 1.79284291400159);\n"
                            "return x * C.xxxx + C.yyyy;", &taylorInvSqrtFuncName);

    
    static const GrGLShaderVar gNoiseVec3Args[] =  {
        GrGLShaderVar("v", kVec3f_GrSLType)
    };

    SkString noiseCode;
    noiseCode.append(
        "const vec2 C = vec2(1.0/6.0, 1.0/3.0);\n"
        "const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);\n"

        
        "vec3 i = floor(v + dot(v, C.yyy));\n"
        "vec3 x0 = v - i + dot(i, C.xxx);\n"

        
        "vec3 g = step(x0.yzx, x0.xyz);\n"
        "vec3 l = 1.0 - g;\n"
        "vec3 i1 = min(g.xyz, l.zxy);\n"
        "vec3 i2 = max(g.xyz, l.zxy);\n"

        "vec3 x1 = x0 - i1 + C.xxx;\n"
        "vec3 x2 = x0 - i2 + C.yyy;\n" 
        "vec3 x3 = x0 - D.yyy;\n" 
    );

    noiseCode.appendf(
        
        "i = %s(i);\n"
        "vec4 p = %s(%s(%s(\n"
        "         i.z + vec4(0.0, i1.z, i2.z, 1.0)) +\n"
        "         i.y + vec4(0.0, i1.y, i2.y, 1.0)) +\n"
        "         i.x + vec4(0.0, i1.x, i2.x, 1.0));\n",
        mod289_3_funcName.c_str(), permuteFuncName.c_str(), permuteFuncName.c_str(),
        permuteFuncName.c_str());

    noiseCode.append(
        
        
        "float n_ = 0.142857142857;\n" 
        "vec3  ns = n_ * D.wyz - D.xzx;\n"

        "vec4 j = p - 49.0 * floor(p * ns.z * ns.z);\n" 

        "vec4 x_ = floor(j * ns.z);\n"
        "vec4 y_ = floor(j - 7.0 * x_);" 

        "vec4 x = x_ *ns.x + ns.yyyy;\n"
        "vec4 y = y_ *ns.x + ns.yyyy;\n"
        "vec4 h = 1.0 - abs(x) - abs(y);\n"

        "vec4 b0 = vec4(x.xy, y.xy);\n"
        "vec4 b1 = vec4(x.zw, y.zw);\n"
    );

    noiseCode.append(
        "vec4 s0 = floor(b0) * 2.0 + 1.0;\n"
        "vec4 s1 = floor(b1) * 2.0 + 1.0;\n"
        "vec4 sh = -step(h, vec4(0.0));\n"

        "vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;\n"
        "vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;\n"

        "vec3 p0 = vec3(a0.xy, h.x);\n"
        "vec3 p1 = vec3(a0.zw, h.y);\n"
        "vec3 p2 = vec3(a1.xy, h.z);\n"
        "vec3 p3 = vec3(a1.zw, h.w);\n"
    );

    noiseCode.appendf(
        
        "vec4 norm = %s(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));\n"
        "p0 *= norm.x;\n"
        "p1 *= norm.y;\n"
        "p2 *= norm.z;\n"
        "p3 *= norm.w;\n"

        
        "vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);\n"
        "m = m * m;\n"
        "return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));",
        taylorInvSqrtFuncName.c_str());

    SkString noiseFuncName;
    builder->fsEmitFunction(kFloat_GrSLType,
                            "snoise", SK_ARRAY_COUNT(gNoiseVec3Args), gNoiseVec3Args,
                            noiseCode.c_str(), &noiseFuncName);

    const char* noiseVecIni = "noiseVecIni";
    const char* factors     = "factors";
    const char* sum         = "sum";
    const char* xOffsets    = "xOffsets";
    const char* yOffsets    = "yOffsets";
    const char* channel     = "channel";

    
    builder->fsCodeAppendf("\t\tconst vec4 %s = vec4(13.0, 53.0, 101.0, 151.0);\n", xOffsets);
    builder->fsCodeAppendf("\t\tconst vec4 %s = vec4(109.0, 167.0, 23.0, 67.0);\n", yOffsets);

    
    builder->fsCodeAppendf(
        "\t\tvec3 %s = vec3(floor((%s*vec3(%s, 1.0)).xy) * vec2(0.66) * %s, 0.0);\n",
        noiseVecIni, invMatrixUni, vCoords.c_str(), baseFrequencyUni);

    
    builder->fsCodeAppendf("\t\t%s += 0.1 * vec3(%s(%s + vec3(  0.0,   0.0, %s)),"
                                                "%s(%s + vec3( 43.0,  17.0, %s)),"
                                                "%s(%s + vec3(-17.0, -43.0, %s)));\n",
                           noiseVecIni, noiseFuncName.c_str(), noiseVecIni, seedUni,
                                        noiseFuncName.c_str(), noiseVecIni, seedUni,
                                        noiseFuncName.c_str(), noiseVecIni, seedUni);

    builder->fsCodeAppendf("\t\t%s = vec4(0.0);\n", outputColor);

    builder->fsCodeAppendf("\t\tvec3 %s = vec3(1.0);\n", factors);
    builder->fsCodeAppendf("\t\tfloat %s = 0.0;\n", sum);

    
    builder->fsCodeAppendf("\t\tfor (int octave = 0; octave < %d; ++octave) {\n", fNumOctaves);

    
    builder->fsCodeAppendf("\t\t\tfor (int %s = 3; %s >= 0; --%s) {\n", channel, channel, channel);

    builder->fsCodeAppendf(
        "\t\t\t\t%s[channel] += %s.x * %s(%s * %s.yyy - vec3(%s[%s], %s[%s], %s * %s.z));\n",
        outputColor, factors, noiseFuncName.c_str(), noiseVecIni, factors, xOffsets, channel,
        yOffsets, channel, seedUni, factors);

    builder->fsCodeAppend("\t\t\t}\n"); 

    builder->fsCodeAppendf("\t\t\t%s += %s.x;\n", sum, factors);
    builder->fsCodeAppendf("\t\t\t%s *= vec3(0.5, 2.0, 0.75);\n", factors);

    builder->fsCodeAppend("\t\t}\n"); 

    if (fType == SkPerlinNoiseShader::kFractalNoise_Type) {
        
        
        builder->fsCodeAppendf("\t\t%s = %s * vec4(0.5 / %s) + vec4(0.5);\n",
                               outputColor, outputColor, sum);
    } else {
        builder->fsCodeAppendf("\t\t%s = abs(%s / vec4(%s));\n",
                               outputColor, outputColor, sum);
    }

    builder->fsCodeAppendf("\t\t%s.a *= %s;\n", outputColor, alphaUni);

    
    builder->fsCodeAppendf("\t\t%s = clamp(%s, 0.0, 1.0);\n", outputColor, outputColor);

    
    builder->fsCodeAppendf("\t\t%s = vec4(%s.rgb * %s.aaa, %s.a);\n",
                           outputColor, outputColor, outputColor, outputColor);
}

void GrGLPerlinNoise::emitCode(GrGLShaderBuilder* builder,
                               const GrDrawEffect&,
                               EffectKey key,
                               const char* outputColor,
                               const char* inputColor,
                               const TransformedCoordsArray& coords,
                               const TextureSamplerArray& samplers) {
    sk_ignore_unused_variable(inputColor);

    SkString vCoords = builder->ensureFSCoords2D(coords, 0);

    fInvMatrixUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                        kMat33f_GrSLType, "invMatrix");
    const char* invMatrixUni = builder->getUniformCStr(fInvMatrixUni);
    fBaseFrequencyUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                            kVec2f_GrSLType, "baseFrequency");
    const char* baseFrequencyUni = builder->getUniformCStr(fBaseFrequencyUni);
    fAlphaUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                    kFloat_GrSLType, "alpha");
    const char* alphaUni = builder->getUniformCStr(fAlphaUni);

    const char* stitchDataUni = NULL;
    if (fStitchTiles) {
        fStitchDataUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                             kVec2f_GrSLType, "stitchData");
        stitchDataUni = builder->getUniformCStr(fStitchDataUni);
    }

    
    const char* chanCoordR  = "0.125";
    const char* chanCoordG  = "0.375";
    const char* chanCoordB  = "0.625";
    const char* chanCoordA  = "0.875";
    const char* chanCoord   = "chanCoord";
    const char* stitchData  = "stitchData";
    const char* ratio       = "ratio";
    const char* noiseXY     = "noiseXY";
    const char* noiseVec    = "noiseVec";
    const char* noiseSmooth = "noiseSmooth";
    const char* fractVal    = "fractVal";
    const char* uv          = "uv";
    const char* ab          = "ab";
    const char* latticeIdx  = "latticeIdx";
    const char* lattice     = "lattice";
    const char* inc8bit     = "0.00390625";  
    
    
    const char* dotLattice  = "dot(((%s.ga + %s.rb * vec2(%s)) * vec2(2.0) - vec2(1.0)), %s);";

    
    static const GrGLShaderVar gPerlinNoiseArgs[] =  {
        GrGLShaderVar(chanCoord, kFloat_GrSLType),
        GrGLShaderVar(noiseVec, kVec2f_GrSLType)
    };

    static const GrGLShaderVar gPerlinNoiseStitchArgs[] =  {
        GrGLShaderVar(chanCoord, kFloat_GrSLType),
        GrGLShaderVar(noiseVec, kVec2f_GrSLType),
        GrGLShaderVar(stitchData, kVec2f_GrSLType)
    };

    SkString noiseCode;

    noiseCode.appendf("\tvec4 %s = vec4(floor(%s), fract(%s));", noiseXY, noiseVec, noiseVec);

    
    noiseCode.appendf("\n\tvec2 %s = %s.zw * %s.zw * (vec2(3.0) - vec2(2.0) * %s.zw);",
        noiseSmooth, noiseXY, noiseXY, noiseXY);

    
    if (fStitchTiles) {
        noiseCode.appendf("\n\tif(%s.x >= %s.x) { %s.x -= %s.x; }",
            noiseXY, stitchData, noiseXY, stitchData);
        noiseCode.appendf("\n\tif(%s.x >= (%s.x - 1.0)) { %s.x -= (%s.x - 1.0); }",
            noiseXY, stitchData, noiseXY, stitchData);
        noiseCode.appendf("\n\tif(%s.y >= %s.y) { %s.y -= %s.y; }",
            noiseXY, stitchData, noiseXY, stitchData);
        noiseCode.appendf("\n\tif(%s.y >= (%s.y - 1.0)) { %s.y -= (%s.y - 1.0); }",
            noiseXY, stitchData, noiseXY, stitchData);
    }

    
    noiseCode.appendf("\n\t%s.xy = fract(floor(mod(%s.xy, 256.0)) / vec2(256.0));\n",
        noiseXY, noiseXY);

    
    {
        SkString xCoords("");
        xCoords.appendf("vec2(%s.x, 0.5)", noiseXY);

        noiseCode.appendf("\n\tvec2 %s;\n\t%s.x = ", latticeIdx, latticeIdx);
        builder->appendTextureLookup(&noiseCode, samplers[0], xCoords.c_str(), kVec2f_GrSLType);
        noiseCode.append(".r;");
    }

    
    {
        SkString xCoords("");
        xCoords.appendf("vec2(fract(%s.x + %s), 0.5)", noiseXY, inc8bit);

        noiseCode.appendf("\n\t%s.y = ", latticeIdx);
        builder->appendTextureLookup(&noiseCode, samplers[0], xCoords.c_str(), kVec2f_GrSLType);
        noiseCode.append(".r;");
    }

#if defined(SK_BUILD_FOR_ANDROID)
    
    
    
    
    
    
    noiseCode.appendf("\n\t%s = floor(%s * vec2(255.0) + vec2(0.5)) * vec2(0.003921569);",
                      latticeIdx, latticeIdx);
#endif

    
    noiseCode.appendf("\n\t%s = fract(%s + %s.yy);", latticeIdx, latticeIdx, noiseXY);

    noiseCode.appendf("\n\tvec2 %s = %s.zw;", fractVal, noiseXY);

    noiseCode.appendf("\n\n\tvec2 %s;", uv);
    
    {
        SkString latticeCoords("");
        latticeCoords.appendf("vec2(%s.x, %s)", latticeIdx, chanCoord);
        noiseCode.appendf("\n\tvec4 %s = ", lattice);
        builder->appendTextureLookup(&noiseCode, samplers[1], latticeCoords.c_str(),
            kVec2f_GrSLType);
        noiseCode.appendf(".bgra;\n\t%s.x = ", uv);
        noiseCode.appendf(dotLattice, lattice, lattice, inc8bit, fractVal);
    }

    noiseCode.appendf("\n\t%s.x -= 1.0;", fractVal);
    
    {
        SkString latticeCoords("");
        latticeCoords.appendf("vec2(%s.y, %s)", latticeIdx, chanCoord);
        noiseCode.append("\n\tlattice = ");
        builder->appendTextureLookup(&noiseCode, samplers[1], latticeCoords.c_str(),
            kVec2f_GrSLType);
        noiseCode.appendf(".bgra;\n\t%s.y = ", uv);
        noiseCode.appendf(dotLattice, lattice, lattice, inc8bit, fractVal);
    }

    
    noiseCode.appendf("\n\tvec2 %s;", ab);
    noiseCode.appendf("\n\t%s.x = mix(%s.x, %s.y, %s.x);", ab, uv, uv, noiseSmooth);

    noiseCode.appendf("\n\t%s.y -= 1.0;", fractVal);
    
    {
        SkString latticeCoords("");
        latticeCoords.appendf("vec2(fract(%s.y + %s), %s)", latticeIdx, inc8bit, chanCoord);
        noiseCode.append("\n\tlattice = ");
        builder->appendTextureLookup(&noiseCode, samplers[1], latticeCoords.c_str(),
            kVec2f_GrSLType);
        noiseCode.appendf(".bgra;\n\t%s.y = ", uv);
        noiseCode.appendf(dotLattice, lattice, lattice, inc8bit, fractVal);
    }

    noiseCode.appendf("\n\t%s.x += 1.0;", fractVal);
    
    {
        SkString latticeCoords("");
        latticeCoords.appendf("vec2(fract(%s.x + %s), %s)", latticeIdx, inc8bit, chanCoord);
        noiseCode.append("\n\tlattice = ");
        builder->appendTextureLookup(&noiseCode, samplers[1], latticeCoords.c_str(),
            kVec2f_GrSLType);
        noiseCode.appendf(".bgra;\n\t%s.x = ", uv);
        noiseCode.appendf(dotLattice, lattice, lattice, inc8bit, fractVal);
    }

    
    noiseCode.appendf("\n\t%s.y = mix(%s.x, %s.y, %s.x);", ab, uv, uv, noiseSmooth);
    
    noiseCode.appendf("\n\treturn mix(%s.x, %s.y, %s.y);\n", ab, ab, noiseSmooth);

    SkString noiseFuncName;
    if (fStitchTiles) {
        builder->fsEmitFunction(kFloat_GrSLType,
                                "perlinnoise", SK_ARRAY_COUNT(gPerlinNoiseStitchArgs),
                                gPerlinNoiseStitchArgs, noiseCode.c_str(), &noiseFuncName);
    } else {
        builder->fsEmitFunction(kFloat_GrSLType,
                                "perlinnoise", SK_ARRAY_COUNT(gPerlinNoiseArgs),
                                gPerlinNoiseArgs, noiseCode.c_str(), &noiseFuncName);
    }

    
    builder->fsCodeAppendf("\n\t\tvec2 %s = floor((%s * vec3(%s, 1.0)).xy) * %s;",
                           noiseVec, invMatrixUni, vCoords.c_str(), baseFrequencyUni);

    
    builder->fsCodeAppendf("\n\t\t%s = vec4(0.0);", outputColor);

    if (fStitchTiles) {
        
        builder->fsCodeAppendf("\n\t\tvec2 %s = %s;", stitchData, stitchDataUni);
    }

    builder->fsCodeAppendf("\n\t\tfloat %s = 1.0;", ratio);

    
    builder->fsCodeAppendf("\n\t\tfor (int octave = 0; octave < %d; ++octave) {", fNumOctaves);

    builder->fsCodeAppendf("\n\t\t\t%s += ", outputColor);
    if (fType != SkPerlinNoiseShader::kFractalNoise_Type) {
        builder->fsCodeAppend("abs(");
    }
    if (fStitchTiles) {
        builder->fsCodeAppendf(
            "vec4(\n\t\t\t\t%s(%s, %s, %s),\n\t\t\t\t%s(%s, %s, %s),"
                 "\n\t\t\t\t%s(%s, %s, %s),\n\t\t\t\t%s(%s, %s, %s))",
            noiseFuncName.c_str(), chanCoordR, noiseVec, stitchData,
            noiseFuncName.c_str(), chanCoordG, noiseVec, stitchData,
            noiseFuncName.c_str(), chanCoordB, noiseVec, stitchData,
            noiseFuncName.c_str(), chanCoordA, noiseVec, stitchData);
    } else {
        builder->fsCodeAppendf(
            "vec4(\n\t\t\t\t%s(%s, %s),\n\t\t\t\t%s(%s, %s),"
                 "\n\t\t\t\t%s(%s, %s),\n\t\t\t\t%s(%s, %s))",
            noiseFuncName.c_str(), chanCoordR, noiseVec,
            noiseFuncName.c_str(), chanCoordG, noiseVec,
            noiseFuncName.c_str(), chanCoordB, noiseVec,
            noiseFuncName.c_str(), chanCoordA, noiseVec);
    }
    if (fType != SkPerlinNoiseShader::kFractalNoise_Type) {
        builder->fsCodeAppendf(")"); 
    }
    builder->fsCodeAppendf(" * %s;", ratio);

    builder->fsCodeAppendf("\n\t\t\t%s *= vec2(2.0);", noiseVec);
    builder->fsCodeAppendf("\n\t\t\t%s *= 0.5;", ratio);

    if (fStitchTiles) {
        builder->fsCodeAppendf("\n\t\t\t%s *= vec2(2.0);", stitchData);
    }
    builder->fsCodeAppend("\n\t\t}"); 

    if (fType == SkPerlinNoiseShader::kFractalNoise_Type) {
        
        
        builder->fsCodeAppendf("\n\t\t%s = %s * vec4(0.5) + vec4(0.5);", outputColor, outputColor);
    }

    builder->fsCodeAppendf("\n\t\t%s.a *= %s;", outputColor, alphaUni);

    
    builder->fsCodeAppendf("\n\t\t%s = clamp(%s, 0.0, 1.0);", outputColor, outputColor);

    
    builder->fsCodeAppendf("\n\t\t%s = vec4(%s.rgb * %s.aaa, %s.a);\n",
                  outputColor, outputColor, outputColor, outputColor);
}

GrGLNoise::GrGLNoise(const GrBackendEffectFactory& factory, const GrDrawEffect& drawEffect)
  : INHERITED (factory)
  , fType(drawEffect.castEffect<GrPerlinNoiseEffect>().type())
  , fStitchTiles(drawEffect.castEffect<GrPerlinNoiseEffect>().stitchTiles())
  , fNumOctaves(drawEffect.castEffect<GrPerlinNoiseEffect>().numOctaves()) {
}

GrGLEffect::EffectKey GrGLNoise::GenKey(const GrDrawEffect& drawEffect, const GrGLCaps&) {
    const GrPerlinNoiseEffect& turbulence = drawEffect.castEffect<GrPerlinNoiseEffect>();

    EffectKey key = turbulence.numOctaves();

    key = key << 3; 

    switch (turbulence.type()) {
        case SkPerlinNoiseShader::kFractalNoise_Type:
            key |= 0x1;
            break;
        case SkPerlinNoiseShader::kTurbulence_Type:
            key |= 0x2;
            break;
        default:
            
            break;
    }

    if (turbulence.stitchTiles()) {
        key |= 0x4; 
    }

    return key;
}

void GrGLNoise::setData(const GrGLUniformManager& uman, const GrDrawEffect& drawEffect) {
    const GrPerlinNoiseEffect& turbulence = drawEffect.castEffect<GrPerlinNoiseEffect>();

    const SkVector& baseFrequency = turbulence.baseFrequency();
    uman.set2f(fBaseFrequencyUni, baseFrequency.fX, baseFrequency.fY);
    uman.set1f(fAlphaUni, SkScalarDiv(SkIntToScalar(turbulence.alpha()), SkIntToScalar(255)));

    SkMatrix m = turbulence.matrix();
    m.postTranslate(-SK_Scalar1, -SK_Scalar1);
    SkMatrix invM;
    if (!m.invert(&invM)) {
        invM.reset();
    } else {
        invM.postConcat(invM); 
    }
    uman.setSkMatrix(fInvMatrixUni, invM);
}

void GrGLPerlinNoise::setData(const GrGLUniformManager& uman, const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);

    const GrPerlinNoiseEffect& turbulence = drawEffect.castEffect<GrPerlinNoiseEffect>();
    if (turbulence.stitchTiles()) {
        const SkPerlinNoiseShader::StitchData& stitchData = turbulence.stitchData();
        uman.set2f(fStitchDataUni, SkIntToScalar(stitchData.fWidth),
                                   SkIntToScalar(stitchData.fHeight));
    }
}

void GrGLSimplexNoise::setData(const GrGLUniformManager& uman, const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);

    const GrSimplexNoiseEffect& turbulence = drawEffect.castEffect<GrSimplexNoiseEffect>();
    uman.set1f(fSeedUni, turbulence.seed());
}



GrEffectRef* SkPerlinNoiseShader::asNewEffect(GrContext* context, const SkPaint& paint) const {
    SkASSERT(NULL != context);

    if (0 == fNumOctaves) {
        SkColor clearColor = 0;
        if (kFractalNoise_Type == fType) {
            clearColor = SkColorSetARGB(paint.getAlpha() / 2, 127, 127, 127);
        }
        SkAutoTUnref<SkColorFilter> cf(SkColorFilter::CreateModeFilter(
                                                clearColor, SkXfermode::kSrc_Mode));
        return cf->asNewEffect(context);
    }

    
    SkASSERT(!fStitchTiles || !fTileSize.isEmpty());

#ifdef SK_USE_SIMPLEX_NOISE
    
    sk_ignore_unused_variable(context);
    GrEffectRef* effect =
        GrSimplexNoiseEffect::Create(fType, fPaintingData->fBaseFrequency,
                                     fNumOctaves, fStitchTiles, fSeed,
                                     this->getLocalMatrix(), paint.getAlpha());
#else
    GrTexture* permutationsTexture = GrLockAndRefCachedBitmapTexture(
        context, fPaintingData->getPermutationsBitmap(), NULL);
    GrTexture* noiseTexture = GrLockAndRefCachedBitmapTexture(
        context, fPaintingData->getNoiseBitmap(), NULL);

    GrEffectRef* effect = (NULL != permutationsTexture) && (NULL != noiseTexture) ?
        GrPerlinNoiseEffect::Create(fType, fPaintingData->fBaseFrequency,
                                    fNumOctaves, fStitchTiles,
                                    fPaintingData->fStitchDataInit,
                                    permutationsTexture, noiseTexture,
                                    this->getLocalMatrix(), paint.getAlpha()) :
        NULL;

    
    
    
    if (NULL != permutationsTexture) {
        GrUnlockAndUnrefCachedBitmapTexture(permutationsTexture);
    }
    if (NULL != noiseTexture) {
        GrUnlockAndUnrefCachedBitmapTexture(noiseTexture);
    }
#endif

    return effect;
}

#else

GrEffectRef* SkPerlinNoiseShader::asNewEffect(GrContext*, const SkPaint&) const {
    SkDEBUGFAIL("Should not call in GPU-less build");
    return NULL;
}

#endif

#ifndef SK_IGNORE_TO_STRING
void SkPerlinNoiseShader::toString(SkString* str) const {
    str->append("SkPerlinNoiseShader: (");

    str->append("type: ");
    switch (fType) {
        case kFractalNoise_Type:
            str->append("\"fractal noise\"");
            break;
        case kTurbulence_Type:
            str->append("\"turbulence\"");
            break;
        default:
            str->append("\"unknown\"");
            break;
    }
    str->append(" base frequency: (");
    str->appendScalar(fBaseFrequencyX);
    str->append(", ");
    str->appendScalar(fBaseFrequencyY);
    str->append(") number of octaves: ");
    str->appendS32(fNumOctaves);
    str->append(" seed: ");
    str->appendScalar(fSeed);
    str->append(" stitch tiles: ");
    str->append(fStitchTiles ? "true " : "false ");

    this->INHERITED::toString(str);

    str->append(")");
}
#endif
