







#ifndef SkBitmapRegionDecoder_DEFINED
#define SkBitmapRegionDecoder_DEFINED

#include "SkBitmap.h"
#include "SkImageDecoder.h"
#include "SkStream.h"

struct SkIRect;









class SkBitmapRegionDecoder {
public:
    SkBitmapRegionDecoder(SkImageDecoder* decoder, SkStream* stream,
                          int width, int height) {
        fDecoder = decoder;
        fStream = stream;
        fWidth = width;
        fHeight = height;
    }
    ~SkBitmapRegionDecoder() {
        SkDELETE(fDecoder);
        SkSafeUnref(fStream);
    }

    bool decodeRegion(SkBitmap* bitmap, const SkIRect& rect,
                      SkBitmap::Config pref, int sampleSize);

    SkImageDecoder* getDecoder() const { return fDecoder; }
    int getWidth() const { return fWidth; }
    int getHeight() const { return fHeight; }

private:
    SkImageDecoder* fDecoder;
    SkStream* fStream;
    int fWidth;
    int fHeight;
};

#endif
