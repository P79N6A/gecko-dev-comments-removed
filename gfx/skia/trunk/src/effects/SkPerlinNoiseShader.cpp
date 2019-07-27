






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
#include "gl/GrGLShaderBuilder.h"
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
                 SkScalar baseFrequencyX, SkScalar baseFrequencyY,
                 const SkMatrix& matrix)
    {
        SkVector wavelength = SkVector::Make(SkScalarInvert(baseFrequencyX),
                                             SkScalarInvert(baseFrequencyY));
        matrix.mapVectors(&wavelength, 1);
        fBaseFrequency.fX = SkScalarInvert(wavelength.fX);
        fBaseFrequency.fY = SkScalarInvert(wavelength.fY);
        SkVector sizeVec = SkVector::Make(SkIntToScalar(tileSize.fWidth),
                                          SkIntToScalar(tileSize.fHeight));
        matrix.mapVectors(&sizeVec, 1);
        fTileSize.fWidth = SkScalarRoundToInt(sizeVec.fX);
        fTileSize.fHeight = SkScalarRoundToInt(sizeVec.fY);
        this->init(seed);
        if (!fTileSize.isEmpty()) {
            this->stitch();
        }

#if SK_SUPPORT_GPU
        fPermutationsBitmap.setInfo(SkImageInfo::MakeA8(kBlockSize, 1));
        fPermutationsBitmap.setPixels(fLatticeSelector);

        fNoiseBitmap.setInfo(SkImageInfo::MakeN32Premul(kBlockSize, 4));
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

#if SK_SUPPORT_GPU
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

#if SK_SUPPORT_GPU
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

SkShader* SkPerlinNoiseShader::CreateTurbulence(SkScalar baseFrequencyX, SkScalar baseFrequencyY,
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
    buffer.validate(perlin_noise_type_is_valid(fType) &&
                    (fNumOctaves >= 0) && (fNumOctaves <= 255) &&
                    (fStitchTiles != fTileSize.isEmpty()));
}

SkPerlinNoiseShader::~SkPerlinNoiseShader() {
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

SkScalar SkPerlinNoiseShader::PerlinNoiseShaderContext::noise2D(
        int channel, const StitchData& stitchData, const SkPoint& noiseVector) const {
    struct Noise {
        int noisePositionIntegerValue;
        int nextNoisePositionIntegerValue;
        SkScalar noisePositionFractionValue;
        Noise(SkScalar component)
        {
            SkScalar position = component + kPerlinNoise;
            noisePositionIntegerValue = SkScalarFloorToInt(position);
            noisePositionFractionValue = position - SkIntToScalar(noisePositionIntegerValue);
            nextNoisePositionIntegerValue = noisePositionIntegerValue + 1;
        }
    };
    Noise noiseX(noiseVector.x());
    Noise noiseY(noiseVector.y());
    SkScalar u, v;
    const SkPerlinNoiseShader& perlinNoiseShader = static_cast<const SkPerlinNoiseShader&>(fShader);
    
    if (perlinNoiseShader.fStitchTiles) {
        noiseX.noisePositionIntegerValue =
            checkNoise(noiseX.noisePositionIntegerValue, stitchData.fWrapX, stitchData.fWidth);
        noiseY.noisePositionIntegerValue =
            checkNoise(noiseY.noisePositionIntegerValue, stitchData.fWrapY, stitchData.fHeight);
        noiseX.nextNoisePositionIntegerValue =
            checkNoise(noiseX.nextNoisePositionIntegerValue, stitchData.fWrapX, stitchData.fWidth);
        noiseY.nextNoisePositionIntegerValue =
            checkNoise(noiseY.nextNoisePositionIntegerValue, stitchData.fWrapY, stitchData.fHeight);
    }
    noiseX.noisePositionIntegerValue &= kBlockMask;
    noiseY.noisePositionIntegerValue &= kBlockMask;
    noiseX.nextNoisePositionIntegerValue &= kBlockMask;
    noiseY.nextNoisePositionIntegerValue &= kBlockMask;
    int i =
        fPaintingData->fLatticeSelector[noiseX.noisePositionIntegerValue];
    int j =
        fPaintingData->fLatticeSelector[noiseX.nextNoisePositionIntegerValue];
    int b00 = (i + noiseY.noisePositionIntegerValue) & kBlockMask;
    int b10 = (j + noiseY.noisePositionIntegerValue) & kBlockMask;
    int b01 = (i + noiseY.nextNoisePositionIntegerValue) & kBlockMask;
    int b11 = (j + noiseY.nextNoisePositionIntegerValue) & kBlockMask;
    SkScalar sx = smoothCurve(noiseX.noisePositionFractionValue);
    SkScalar sy = smoothCurve(noiseY.noisePositionFractionValue);
    
    SkPoint fractionValue = SkPoint::Make(noiseX.noisePositionFractionValue,
                                          noiseY.noisePositionFractionValue); 
    u = fPaintingData->fGradient[channel][b00].dot(fractionValue);
    fractionValue.fX -= SK_Scalar1; 
    v = fPaintingData->fGradient[channel][b10].dot(fractionValue);
    SkScalar a = SkScalarInterp(u, v, sx);
    fractionValue.fY -= SK_Scalar1; 
    v = fPaintingData->fGradient[channel][b11].dot(fractionValue);
    fractionValue.fX = noiseX.noisePositionFractionValue; 
    u = fPaintingData->fGradient[channel][b01].dot(fractionValue);
    SkScalar b = SkScalarInterp(u, v, sx);
    return SkScalarInterp(a, b, sy);
}

SkScalar SkPerlinNoiseShader::PerlinNoiseShaderContext::calculateTurbulenceValueForPoint(
        int channel, StitchData& stitchData, const SkPoint& point) const {
    const SkPerlinNoiseShader& perlinNoiseShader = static_cast<const SkPerlinNoiseShader&>(fShader);
    if (perlinNoiseShader.fStitchTiles) {
        
        stitchData = fPaintingData->fStitchDataInit;
    }
    SkScalar turbulenceFunctionResult = 0;
    SkPoint noiseVector(SkPoint::Make(SkScalarMul(point.x(), fPaintingData->fBaseFrequency.fX),
                                      SkScalarMul(point.y(), fPaintingData->fBaseFrequency.fY)));
    SkScalar ratio = SK_Scalar1;
    for (int octave = 0; octave < perlinNoiseShader.fNumOctaves; ++octave) {
        SkScalar noise = noise2D(channel, stitchData, noiseVector);
        turbulenceFunctionResult += SkScalarDiv(
            (perlinNoiseShader.fType == kFractalNoise_Type) ? noise : SkScalarAbs(noise), ratio);
        noiseVector.fX *= 2;
        noiseVector.fY *= 2;
        ratio *= 2;
        if (perlinNoiseShader.fStitchTiles) {
            
            stitchData.fWidth  *= 2;
            stitchData.fWrapX   = stitchData.fWidth + kPerlinNoise;
            stitchData.fHeight *= 2;
            stitchData.fWrapY   = stitchData.fHeight + kPerlinNoise;
        }
    }

    
    
    if (perlinNoiseShader.fType == kFractalNoise_Type) {
        turbulenceFunctionResult =
            SkScalarMul(turbulenceFunctionResult, SK_ScalarHalf) + SK_ScalarHalf;
    }

    if (channel == 3) { 
        turbulenceFunctionResult = SkScalarMul(turbulenceFunctionResult,
            SkScalarDiv(SkIntToScalar(getPaintAlpha()), SkIntToScalar(255)));
    }

    
    return SkScalarPin(turbulenceFunctionResult, 0, SK_Scalar1);
}

SkPMColor SkPerlinNoiseShader::PerlinNoiseShaderContext::shade(
        const SkPoint& point, StitchData& stitchData) const {
    SkPoint newPoint;
    fMatrix.mapPoints(&newPoint, &point, 1);
    newPoint.fX = SkScalarRoundToScalar(newPoint.fX);
    newPoint.fY = SkScalarRoundToScalar(newPoint.fY);

    U8CPU rgba[4];
    for (int channel = 3; channel >= 0; --channel) {
        rgba[channel] = SkScalarFloorToInt(255 *
            calculateTurbulenceValueForPoint(channel, stitchData, newPoint));
    }
    return SkPreMultiplyARGB(rgba[3], rgba[0], rgba[1], rgba[2]);
}

SkShader::Context* SkPerlinNoiseShader::onCreateContext(const ContextRec& rec,
                                                        void* storage) const {
    return SkNEW_PLACEMENT_ARGS(storage, PerlinNoiseShaderContext, (*this, rec));
}

size_t SkPerlinNoiseShader::contextSize() const {
    return sizeof(PerlinNoiseShaderContext);
}

SkPerlinNoiseShader::PerlinNoiseShaderContext::PerlinNoiseShaderContext(
        const SkPerlinNoiseShader& shader, const ContextRec& rec)
    : INHERITED(shader, rec)
{
    SkMatrix newMatrix = *rec.fMatrix;
    newMatrix.preConcat(shader.getLocalMatrix());
    if (rec.fLocalMatrix) {
        newMatrix.preConcat(*rec.fLocalMatrix);
    }
    
    
    fMatrix.setTranslate(-newMatrix.getTranslateX() + SK_Scalar1, -newMatrix.getTranslateY() + SK_Scalar1);
    fPaintingData = SkNEW_ARGS(PaintingData, (shader.fTileSize, shader.fSeed, shader.fBaseFrequencyX, shader.fBaseFrequencyY, newMatrix));
}

SkPerlinNoiseShader::PerlinNoiseShaderContext::~PerlinNoiseShaderContext() {
    SkDELETE(fPaintingData);
}

void SkPerlinNoiseShader::PerlinNoiseShaderContext::shadeSpan(
        int x, int y, SkPMColor result[], int count) {
    SkPoint point = SkPoint::Make(SkIntToScalar(x), SkIntToScalar(y));
    StitchData stitchData;
    for (int i = 0; i < count; ++i) {
        result[i] = shade(point, stitchData);
        point.fX += SK_Scalar1;
    }
}

void SkPerlinNoiseShader::PerlinNoiseShaderContext::shadeSpan16(
        int x, int y, uint16_t result[], int count) {
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

class GrGLPerlinNoise : public GrGLEffect {
public:
    GrGLPerlinNoise(const GrBackendEffectFactory& factory,
                    const GrDrawEffect& drawEffect);
    virtual ~GrGLPerlinNoise() {}

    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          const GrEffectKey&,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;

    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;

    static inline void GenKey(const GrDrawEffect&, const GrGLCaps&, GrEffectKeyBuilder* b);

private:

    GrGLUniformManager::UniformHandle   fStitchDataUni;
    SkPerlinNoiseShader::Type           fType;
    bool                                fStitchTiles;
    int                                 fNumOctaves;
    GrGLUniformManager::UniformHandle   fBaseFrequencyUni;
    GrGLUniformManager::UniformHandle   fAlphaUni;

private:
    typedef GrGLEffect INHERITED;
};



class GrPerlinNoiseEffect : public GrEffect {
public:
    static GrEffect* Create(SkPerlinNoiseShader::Type type,
                            int numOctaves, bool stitchTiles,
                            SkPerlinNoiseShader::PaintingData* paintingData,
                            GrTexture* permutationsTexture, GrTexture* noiseTexture,
                            const SkMatrix& matrix, uint8_t alpha) {
        return SkNEW_ARGS(GrPerlinNoiseEffect, (type, numOctaves, stitchTiles, paintingData,
                                                permutationsTexture, noiseTexture, matrix, alpha));
    }

    virtual ~GrPerlinNoiseEffect() {
        SkDELETE(fPaintingData);
    }

    static const char* Name() { return "PerlinNoise"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<GrPerlinNoiseEffect>::getInstance();
    }
    const SkPerlinNoiseShader::StitchData& stitchData() const { return fPaintingData->fStitchDataInit; }

    SkPerlinNoiseShader::Type type() const { return fType; }
    bool stitchTiles() const { return fStitchTiles; }
    const SkVector& baseFrequency() const { return fPaintingData->fBaseFrequency; }
    int numOctaves() const { return fNumOctaves; }
    const SkMatrix& matrix() const { return fCoordTransform.getMatrix(); }
    uint8_t alpha() const { return fAlpha; }

    typedef GrGLPerlinNoise GLEffect;

private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const GrPerlinNoiseEffect& s = CastEffect<GrPerlinNoiseEffect>(sBase);
        return fType == s.fType &&
               fPaintingData->fBaseFrequency == s.fPaintingData->fBaseFrequency &&
               fNumOctaves == s.fNumOctaves &&
               fStitchTiles == s.fStitchTiles &&
               fCoordTransform.getMatrix() == s.fCoordTransform.getMatrix() &&
               fAlpha == s.fAlpha &&
               fPermutationsAccess.getTexture() == s.fPermutationsAccess.getTexture() &&
               fNoiseAccess.getTexture() == s.fNoiseAccess.getTexture() &&
               fPaintingData->fStitchDataInit == s.fPaintingData->fStitchDataInit;
    }

    GrPerlinNoiseEffect(SkPerlinNoiseShader::Type type,
                        int numOctaves, bool stitchTiles,
                        SkPerlinNoiseShader::PaintingData* paintingData,
                        GrTexture* permutationsTexture, GrTexture* noiseTexture,
                        const SkMatrix& matrix, uint8_t alpha)
      : fType(type)
      , fNumOctaves(numOctaves)
      , fStitchTiles(stitchTiles)
      , fAlpha(alpha)
      , fPermutationsAccess(permutationsTexture)
      , fNoiseAccess(noiseTexture)
      , fPaintingData(paintingData) {
        this->addTextureAccess(&fPermutationsAccess);
        this->addTextureAccess(&fNoiseAccess);
        fCoordTransform.reset(kLocal_GrCoordSet, matrix);
        this->addCoordTransform(&fCoordTransform);
        this->setWillNotUseInputColor();
    }

    GR_DECLARE_EFFECT_TEST;

    SkPerlinNoiseShader::Type       fType;
    GrCoordTransform                fCoordTransform;
    int                             fNumOctaves;
    bool                            fStitchTiles;
    uint8_t                         fAlpha;
    GrTextureAccess                 fPermutationsAccess;
    GrTextureAccess                 fNoiseAccess;
    SkPerlinNoiseShader::PaintingData *fPaintingData;

    void getConstantColorComponents(GrColor*, uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0; 
    }

private:
    typedef GrEffect INHERITED;
};


GR_DEFINE_EFFECT_TEST(GrPerlinNoiseEffect);

GrEffect* GrPerlinNoiseEffect::TestCreate(SkRandom* random,
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
        SkPerlinNoiseShader::CreateTurbulence(baseFrequencyX, baseFrequencyY, numOctaves, seed,
                                             stitchTiles ? &tileSize : NULL);

    SkPaint paint;
    GrColor paintColor;
    GrEffect* effect;
    SkAssertResult(shader->asNewEffect(context, paint, NULL, &paintColor, &effect));

    SkDELETE(shader);

    return effect;
}

GrGLPerlinNoise::GrGLPerlinNoise(const GrBackendEffectFactory& factory, const GrDrawEffect& drawEffect)
  : INHERITED (factory)
  , fType(drawEffect.castEffect<GrPerlinNoiseEffect>().type())
  , fStitchTiles(drawEffect.castEffect<GrPerlinNoiseEffect>().stitchTiles())
  , fNumOctaves(drawEffect.castEffect<GrPerlinNoiseEffect>().numOctaves()) {
}

void GrGLPerlinNoise::emitCode(GrGLShaderBuilder* builder,
                               const GrDrawEffect&,
                               const GrEffectKey& key,
                               const char* outputColor,
                               const char* inputColor,
                               const TransformedCoordsArray& coords,
                               const TextureSamplerArray& samplers) {
    sk_ignore_unused_variable(inputColor);

    SkString vCoords = builder->ensureFSCoords2D(coords, 0);

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
    const char* noiseVec    = "noiseVec";
    const char* noiseSmooth = "noiseSmooth";
    const char* floorVal    = "floorVal";
    const char* fractVal    = "fractVal";
    const char* uv          = "uv";
    const char* ab          = "ab";
    const char* latticeIdx  = "latticeIdx";
    const char* bcoords     = "bcoords";
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

    noiseCode.appendf("\tvec4 %s;\n", floorVal);
    noiseCode.appendf("\t%s.xy = floor(%s);\n", floorVal, noiseVec);
    noiseCode.appendf("\t%s.zw = %s.xy + vec2(1.0);\n", floorVal, floorVal);
    noiseCode.appendf("\tvec2 %s = fract(%s);\n", fractVal, noiseVec);

    
    noiseCode.appendf("\n\tvec2 %s = %s * %s * (vec2(3.0) - vec2(2.0) * %s);",
        noiseSmooth, fractVal, fractVal, fractVal);

    
    if (fStitchTiles) {
        noiseCode.appendf("\n\tif(%s.x >= %s.x) { %s.x -= %s.x; }",
            floorVal, stitchData, floorVal, stitchData);
        noiseCode.appendf("\n\tif(%s.y >= %s.y) { %s.y -= %s.y; }",
            floorVal, stitchData, floorVal, stitchData);
        noiseCode.appendf("\n\tif(%s.z >= %s.x) { %s.z -= %s.x; }",
            floorVal, stitchData, floorVal, stitchData);
        noiseCode.appendf("\n\tif(%s.w >= %s.y) { %s.w -= %s.y; }",
            floorVal, stitchData, floorVal, stitchData);
    }

    
    noiseCode.appendf("\n\t%s = fract(floor(mod(%s, 256.0)) / vec4(256.0));\n",
        floorVal, floorVal);

    
    {
        SkString xCoords("");
        xCoords.appendf("vec2(%s.x, 0.5)", floorVal);

        noiseCode.appendf("\n\tvec2 %s;\n\t%s.x = ", latticeIdx, latticeIdx);
        builder->appendTextureLookup(&noiseCode, samplers[0], xCoords.c_str(), kVec2f_GrSLType);
        noiseCode.append(".r;");
    }

    
    {
        SkString xCoords("");
        xCoords.appendf("vec2(%s.z, 0.5)", floorVal);

        noiseCode.appendf("\n\t%s.y = ", latticeIdx);
        builder->appendTextureLookup(&noiseCode, samplers[0], xCoords.c_str(), kVec2f_GrSLType);
        noiseCode.append(".r;");
    }

#if defined(SK_BUILD_FOR_ANDROID)
    
    
    
    
    
    
    noiseCode.appendf("\n\t%s = floor(%s * vec2(255.0) + vec2(0.5)) * vec2(0.003921569);",
                      latticeIdx, latticeIdx);
#endif

    
    noiseCode.appendf("\n\tvec4 %s = fract(%s.xyxy + %s.yyww);", bcoords, latticeIdx, floorVal);

    noiseCode.appendf("\n\n\tvec2 %s;", uv);
    
    {
        SkString latticeCoords("");
        latticeCoords.appendf("vec2(%s.x, %s)", bcoords, chanCoord);
        noiseCode.appendf("\n\tvec4 %s = ", lattice);
        builder->appendTextureLookup(&noiseCode, samplers[1], latticeCoords.c_str(),
            kVec2f_GrSLType);
        noiseCode.appendf(".bgra;\n\t%s.x = ", uv);
        noiseCode.appendf(dotLattice, lattice, lattice, inc8bit, fractVal);
    }

    noiseCode.appendf("\n\t%s.x -= 1.0;", fractVal);
    
    {
        SkString latticeCoords("");
        latticeCoords.appendf("vec2(%s.y, %s)", bcoords, chanCoord);
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
        latticeCoords.appendf("vec2(%s.w, %s)", bcoords, chanCoord);
        noiseCode.append("\n\tlattice = ");
        builder->appendTextureLookup(&noiseCode, samplers[1], latticeCoords.c_str(),
            kVec2f_GrSLType);
        noiseCode.appendf(".bgra;\n\t%s.y = ", uv);
        noiseCode.appendf(dotLattice, lattice, lattice, inc8bit, fractVal);
    }

    noiseCode.appendf("\n\t%s.x += 1.0;", fractVal);
    
    {
        SkString latticeCoords("");
        latticeCoords.appendf("vec2(%s.z, %s)", bcoords, chanCoord);
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

    
    builder->fsCodeAppendf("\n\t\tvec2 %s = floor(%s.xy) * %s;",
                           noiseVec, vCoords.c_str(), baseFrequencyUni);

    
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

void GrGLPerlinNoise::GenKey(const GrDrawEffect& drawEffect, const GrGLCaps&, GrEffectKeyBuilder* b) {
    const GrPerlinNoiseEffect& turbulence = drawEffect.castEffect<GrPerlinNoiseEffect>();

    uint32_t key = turbulence.numOctaves();

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

    b->add32(key);
}

void GrGLPerlinNoise::setData(const GrGLUniformManager& uman, const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);

    const GrPerlinNoiseEffect& turbulence = drawEffect.castEffect<GrPerlinNoiseEffect>();

    const SkVector& baseFrequency = turbulence.baseFrequency();
    uman.set2f(fBaseFrequencyUni, baseFrequency.fX, baseFrequency.fY);
    uman.set1f(fAlphaUni, SkScalarDiv(SkIntToScalar(turbulence.alpha()), SkIntToScalar(255)));

    if (turbulence.stitchTiles()) {
        const SkPerlinNoiseShader::StitchData& stitchData = turbulence.stitchData();
        uman.set2f(fStitchDataUni, SkIntToScalar(stitchData.fWidth),
                                   SkIntToScalar(stitchData.fHeight));
    }
}



bool SkPerlinNoiseShader::asNewEffect(GrContext* context, const SkPaint& paint,
                                      const SkMatrix* externalLocalMatrix, GrColor* paintColor,
                                      GrEffect** effect) const {
    SkASSERT(NULL != context);
    
    *paintColor = SkColor2GrColorJustAlpha(paint.getColor());

    SkMatrix localMatrix = this->getLocalMatrix();
    if (externalLocalMatrix) {
        localMatrix.preConcat(*externalLocalMatrix);
    }

    SkMatrix matrix = context->getMatrix();
    matrix.preConcat(localMatrix);

    if (0 == fNumOctaves) {
        SkColor clearColor = 0;
        if (kFractalNoise_Type == fType) {
            clearColor = SkColorSetARGB(paint.getAlpha() / 2, 127, 127, 127);
        }
        SkAutoTUnref<SkColorFilter> cf(SkColorFilter::CreateModeFilter(
                                                clearColor, SkXfermode::kSrc_Mode));
        *effect = cf->asNewEffect(context);
        return true;
    }

    
    SkASSERT(!fStitchTiles || !fTileSize.isEmpty());

    SkPerlinNoiseShader::PaintingData* paintingData = SkNEW_ARGS(PaintingData, (fTileSize, fSeed, fBaseFrequencyX, fBaseFrequencyY, matrix));
    GrTexture* permutationsTexture = GrLockAndRefCachedBitmapTexture(
        context, paintingData->getPermutationsBitmap(), NULL);
    GrTexture* noiseTexture = GrLockAndRefCachedBitmapTexture(
        context, paintingData->getNoiseBitmap(), NULL);

    SkMatrix m = context->getMatrix();
    m.setTranslateX(-localMatrix.getTranslateX() + SK_Scalar1);
    m.setTranslateY(-localMatrix.getTranslateY() + SK_Scalar1);
    if ((NULL != permutationsTexture) && (NULL != noiseTexture)) {
        *effect = GrPerlinNoiseEffect::Create(fType,
                                                fNumOctaves,
                                                fStitchTiles,
                                                paintingData,
                                                permutationsTexture, noiseTexture,
                                                m, paint.getAlpha());
    } else {
        SkDELETE(paintingData);
        *effect = NULL;
    }

    
    
    
    if (NULL != permutationsTexture) {
        GrUnlockAndUnrefCachedBitmapTexture(permutationsTexture);
    }
    if (NULL != noiseTexture) {
        GrUnlockAndUnrefCachedBitmapTexture(noiseTexture);
    }

    return true;
}

#else

bool SkPerlinNoiseShader::asNewEffect(GrContext* context, const SkPaint& paint,
                                      const SkMatrix* externalLocalMatrix, GrColor* paintColor,
                                      GrEffect** effect) const {
    SkDEBUGFAIL("Should not call in GPU-less build");
    return false;
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
