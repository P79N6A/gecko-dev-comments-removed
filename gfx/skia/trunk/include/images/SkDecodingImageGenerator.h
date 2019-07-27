






#ifndef SkDecodingImageGenerator_DEFINED
#define SkDecodingImageGenerator_DEFINED

#include "SkBitmap.h"
#include "SkImageGenerator.h"

class SkData;
class SkStreamRewindable;





namespace SkDecodingImageGenerator {
    

























    struct Options {
        Options()
            : fSampleSize(1)
            , fDitherImage(true)
            , fUseRequestedColorType(false)
            , fRequestedColorType()
            , fRequireUnpremul(false) { }
        Options(int sampleSize, bool dither)
            : fSampleSize(sampleSize)
            , fDitherImage(dither)
            , fUseRequestedColorType(false)
            , fRequestedColorType()
            , fRequireUnpremul(false) { }
        Options(int sampleSize, bool dither, SkColorType colorType)
            : fSampleSize(sampleSize)
            , fDitherImage(dither)
            , fUseRequestedColorType(true)
            , fRequestedColorType(colorType)
            , fRequireUnpremul(false) { }
         Options(int sampleSize, bool dither, SkColorType colorType,
                 bool requireUnpremul)
            : fSampleSize(sampleSize)
            , fDitherImage(dither)
            , fUseRequestedColorType(true)
            , fRequestedColorType(colorType)
            , fRequireUnpremul(requireUnpremul) { }
        const int         fSampleSize;
        const bool        fDitherImage;
        const bool        fUseRequestedColorType;
        const SkColorType fRequestedColorType;
        const bool        fRequireUnpremul;
    };

    

























    SkImageGenerator* Create(SkStreamRewindable* stream,
                             const Options& opt);

    




    SkImageGenerator* Create(SkData* data, const Options& opt);
};














#endif  
