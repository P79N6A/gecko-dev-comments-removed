






#ifndef SkPerlinNoiseShader_DEFINED
#define SkPerlinNoiseShader_DEFINED

#include "SkShader.h"













class SK_API SkPerlinNoiseShader : public SkShader {
    struct PaintingData;
public:
    struct StitchData;

    







    enum Type {
        kFractalNoise_Type,
        kTurbulence_Type,
        kFirstType = kFractalNoise_Type,
        kLastType = kTurbulence_Type
    };
    













    static SkShader* CreateFractalNoise(SkScalar baseFrequencyX, SkScalar baseFrequencyY,
                                        int numOctaves, SkScalar seed,
                                        const SkISize* tileSize = NULL);
    static SkShader* CreateTubulence(SkScalar baseFrequencyX, SkScalar baseFrequencyY,
                                     int numOctaves, SkScalar seed,
                                     const SkISize* tileSize = NULL);

    virtual bool setContext(const SkBitmap& device, const SkPaint& paint,
                            const SkMatrix& matrix);
    virtual void shadeSpan(int x, int y, SkPMColor[], int count) SK_OVERRIDE;
    virtual void shadeSpan16(int x, int y, uint16_t[], int count) SK_OVERRIDE;

    virtual GrEffectRef* asNewEffect(GrContext* context, const SkPaint&) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPerlinNoiseShader)

protected:
    SkPerlinNoiseShader(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    SkPerlinNoiseShader(SkPerlinNoiseShader::Type type, SkScalar baseFrequencyX,
                        SkScalar baseFrequencyY, int numOctaves, SkScalar seed,
                        const SkISize* tileSize);
    virtual ~SkPerlinNoiseShader();

    SkScalar noise2D(int channel, const PaintingData& paintingData,
                     const StitchData& stitchData, const SkPoint& noiseVector) const;

    SkScalar calculateTurbulenceValueForPoint(int channel, const PaintingData& paintingData,
                                              StitchData& stitchData, const SkPoint& point) const;

    SkPMColor shade(const SkPoint& point, StitchData& stitchData) const;

    
    
    
     SkPerlinNoiseShader::Type fType;
     SkScalar                  fBaseFrequencyX;
     SkScalar                  fBaseFrequencyY;
     int                       fNumOctaves;
     SkScalar                  fSeed;
     SkISize                   fTileSize;
     bool                      fStitchTiles;
    
    SkMatrix fMatrix;

    PaintingData* fPaintingData;

    typedef SkShader INHERITED;
};

#endif
