






#ifndef SkImageEncoder_DEFINED
#define SkImageEncoder_DEFINED

#include "SkTypes.h"

class SkBitmap;
class SkWStream;

class SkImageEncoder {
public:
    enum Type {
        kJPEG_Type,
        kPNG_Type,
        kWEBP_Type
    };
    static SkImageEncoder* Create(Type);

    virtual ~SkImageEncoder();

    
    enum {
        kDefaultQuality = 80
    };

    







    bool encodeFile(const char file[], const SkBitmap& bm, int quality);

    







    bool encodeStream(SkWStream* stream, const SkBitmap& bm, int quality);

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



DECLARE_ENCODER_CREATOR(JPEGImageEncoder);
DECLARE_ENCODER_CREATOR(PNGImageEncoder);
DECLARE_ENCODER_CREATOR(WEBPImageEncoder);

#endif
