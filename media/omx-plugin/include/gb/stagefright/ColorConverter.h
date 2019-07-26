















#ifndef COLOR_CONVERTER_H_

#define COLOR_CONVERTER_H_

#include <sys/types.h>

#include <stdint.h>

#include <OMX_Video.h>

namespace android {

struct ColorConverter {
    ColorConverter(OMX_COLOR_FORMATTYPE from, OMX_COLOR_FORMATTYPE to);
    ~ColorConverter();

    bool isValid() const;

    void convert(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

private:
    OMX_COLOR_FORMATTYPE mSrcFormat, mDstFormat;
    uint8_t *mClip;

    uint8_t *initClip();

    void convertCbYCrY(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    void convertYUV420Planar(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    void convertQCOMYUV420SemiPlanar(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    void convertYUV420SemiPlanar(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    ColorConverter(const ColorConverter &);
    ColorConverter &operator=(const ColorConverter &);
};

}  

#endif  
