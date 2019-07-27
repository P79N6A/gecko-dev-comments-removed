






#ifndef SkImageEncoder_DEFINED
#define SkImageEncoder_DEFINED

#include "SkTypes.h"
#include "SkTRegistry.h"

class SkBitmap;
class SkData;
class SkWStream;

class SkImageEncoder {
public:
    enum Type {
        kUnknown_Type,
        kBMP_Type,
        kGIF_Type,
        kICO_Type,
        kJPEG_Type,
        kPNG_Type,
        kWBMP_Type,
        kWEBP_Type,
        kKTX_Type,
    };
    static SkImageEncoder* Create(Type);

    virtual ~SkImageEncoder();

    
    enum {
        kDefaultQuality = 80
    };

    





    SkData* encodeData(const SkBitmap&, int quality);

    




    bool encodeFile(const char file[], const SkBitmap& bm, int quality);

    




    bool encodeStream(SkWStream* stream, const SkBitmap& bm, int quality);

    static SkData* EncodeData(const SkBitmap&, Type, int quality);
    static bool EncodeFile(const char file[], const SkBitmap&, Type,
                           int quality);
    static bool EncodeStream(SkWStream*, const SkBitmap&, Type,
                           int quality);

protected:
    






    virtual bool onEncode(SkWStream* stream, const SkBitmap& bm, int quality) = 0;
};



#define DECLARE_ENCODER_CREATOR(codec)          \
    SkImageEncoder *Create ## codec ();



#define DEFINE_ENCODER_CREATOR(codec)           \
    SkImageEncoder *Create ## codec () {        \
        return SkNEW( Sk ## codec );            \
    }







DECLARE_ENCODER_CREATOR(ARGBImageEncoder);
DECLARE_ENCODER_CREATOR(JPEGImageEncoder);
DECLARE_ENCODER_CREATOR(PNGImageEncoder);
DECLARE_ENCODER_CREATOR(KTXImageEncoder);
DECLARE_ENCODER_CREATOR(WEBPImageEncoder);

#ifdef SK_BUILD_FOR_IOS
DECLARE_ENCODER_CREATOR(PNGImageEncoder_IOS);
#endif



typedef SkTRegistry<SkImageEncoder*(*)(SkImageEncoder::Type)> SkImageEncoder_EncodeReg;
#endif
