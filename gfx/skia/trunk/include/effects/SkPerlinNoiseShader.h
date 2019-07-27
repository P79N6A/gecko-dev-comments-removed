






#ifndef SkPerlinNoiseShader_DEFINED
#define SkPerlinNoiseShader_DEFINED

#include "SkShader.h"













class SK_API SkPerlinNoiseShader : public SkShader {
public:
    struct StitchData;
    struct PaintingData;

    







    enum Type {
        kFractalNoise_Type,
        kTurbulence_Type,
        kFirstType = kFractalNoise_Type,
        kLastType = kTurbulence_Type
    };
    













    static SkShader* CreateFractalNoise(SkScalar baseFrequencyX, SkScalar baseFrequencyY,
                                        int numOctaves, SkScalar seed,
                                        const SkISize* tileSize = NULL);
    static SkShader* CreateTurbulence(SkScalar baseFrequencyX, SkScalar baseFrequencyY,
                                     int numOctaves, SkScalar seed,
                                     const SkISize* tileSize = NULL);
    



    static SkShader* CreateTubulence(SkScalar baseFrequencyX, SkScalar baseFrequencyY,
                                     int numOctaves, SkScalar seed,
                                     const SkISize* tileSize = NULL) {
    return CreateTurbulence(baseFrequencyX, baseFrequencyY, numOctaves, seed, tileSize);
    }


    virtual size_t contextSize() const SK_OVERRIDE;

    class PerlinNoiseShaderContext : public SkShader::Context {
    public:
        PerlinNoiseShaderContext(const SkPerlinNoiseShader& shader, const ContextRec&);
        virtual ~PerlinNoiseShaderContext();

        virtual void shadeSpan(int x, int y, SkPMColor[], int count) SK_OVERRIDE;
        virtual void shadeSpan16(int x, int y, uint16_t[], int count) SK_OVERRIDE;

    private:
        SkPMColor shade(const SkPoint& point, StitchData& stitchData) const;
        SkScalar calculateTurbulenceValueForPoint(
            int channel,
            StitchData& stitchData, const SkPoint& point) const;
        SkScalar noise2D(int channel,
                         const StitchData& stitchData, const SkPoint& noiseVector) const;

        SkMatrix fMatrix;
        PaintingData* fPaintingData;

        typedef SkShader::Context INHERITED;
    };

    virtual bool asNewEffect(GrContext* context, const SkPaint&, const SkMatrix*, GrColor*,
                             GrEffect**) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPerlinNoiseShader)

protected:
    SkPerlinNoiseShader(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;
    virtual Context* onCreateContext(const ContextRec&, void* storage) const SK_OVERRIDE;

private:
    SkPerlinNoiseShader(SkPerlinNoiseShader::Type type, SkScalar baseFrequencyX,
                        SkScalar baseFrequencyY, int numOctaves, SkScalar seed,
                        const SkISize* tileSize);
    virtual ~SkPerlinNoiseShader();

    
    
    
     SkPerlinNoiseShader::Type fType;
     SkScalar                  fBaseFrequencyX;
     SkScalar                  fBaseFrequencyY;
     int                       fNumOctaves;
     SkScalar                  fSeed;
     SkISize                   fTileSize;
     bool                      fStitchTiles;

    typedef SkShader INHERITED;
};

#endif
