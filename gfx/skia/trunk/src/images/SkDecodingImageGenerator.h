






#ifndef SkDecodingImageGenerator_DEFINED
#define SkDecodingImageGenerator_DEFINED

#include "SkBitmap.h"
#include "SkImageGenerator.h"

class SkData;
class SkStreamRewindable;





class SkDecodingImageGenerator : public SkImageGenerator {
public:
    virtual ~SkDecodingImageGenerator();
    virtual SkData* refEncodedData() SK_OVERRIDE;
    
    virtual bool getInfo(SkImageInfo* info) SK_OVERRIDE;
    virtual bool getPixels(const SkImageInfo& info,
                           void* pixels,
                           size_t rowBytes) SK_OVERRIDE;
    





















    struct Options {
        Options()
            : fSampleSize(1)
            , fDitherImage(true)
            , fUseRequestedColorType(false)
            , fRequestedColorType() { }
        Options(int sampleSize, bool dither)
            : fSampleSize(sampleSize)
            , fDitherImage(dither)
            , fUseRequestedColorType(false)
            , fRequestedColorType() { }
        Options(int sampleSize, bool dither, SkColorType colorType)
            : fSampleSize(sampleSize)
            , fDitherImage(dither)
            , fUseRequestedColorType(true)
            , fRequestedColorType(colorType) { }
        const int         fSampleSize;
        const bool        fDitherImage;
        const bool        fUseRequestedColorType;
        const SkColorType fRequestedColorType;
    };

    

























    static SkImageGenerator* Create(SkStreamRewindable* stream,
                                    const Options& opt);

    




    static SkImageGenerator* Create(SkData* data, const Options& opt);

private:
    SkData*                fData;
    SkStreamRewindable*    fStream;
    const SkImageInfo      fInfo;
    const int              fSampleSize;
    const bool             fDitherImage;

    SkDecodingImageGenerator(SkData* data,
                             SkStreamRewindable* stream,
                             const SkImageInfo& info,
                             int sampleSize,
                             bool ditherImage);
    static SkImageGenerator* Create(SkData*, SkStreamRewindable*,
                                    const Options&);
    typedef SkImageGenerator INHERITED;
};














#endif  
