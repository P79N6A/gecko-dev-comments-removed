















#ifndef COLOR_CONVERTER_H_

#define COLOR_CONVERTER_H_

#include <sys/types.h>

#include <stdint.h>
#include <utils/Errors.h>

#include <OMX_Video.h>

namespace android {

struct ColorConverter {
    ColorConverter(OMX_COLOR_FORMATTYPE from, OMX_COLOR_FORMATTYPE to);
    ~ColorConverter();

    bool isValid() const;

    status_t convert(
            const void *srcBits,
            size_t srcWidth, size_t srcHeight,
            size_t srcCropLeft, size_t srcCropTop,
            size_t srcCropRight, size_t srcCropBottom,
            void *dstBits,
            size_t dstWidth, size_t dstHeight,
            size_t dstCropLeft, size_t dstCropTop,
            size_t dstCropRight, size_t dstCropBottom);

private:
    struct BitmapParams {
        BitmapParams(
                void *bits,
                size_t width, size_t height,
                size_t cropLeft, size_t cropTop,
                size_t cropRight, size_t cropBottom);

        size_t cropWidth() const;
        size_t cropHeight() const;

        void *mBits;
        size_t mWidth, mHeight;
        size_t mCropLeft, mCropTop, mCropRight, mCropBottom;
    };

    OMX_COLOR_FORMATTYPE mSrcFormat, mDstFormat;
    uint8_t *mClip;

    uint8_t *initClip();

    status_t convertCbYCrY(
            const BitmapParams &src, const BitmapParams &dst);

    status_t convertYUV420Planar(
            const BitmapParams &src, const BitmapParams &dst);

    status_t convertQCOMYUV420SemiPlanar(
            const BitmapParams &src, const BitmapParams &dst);

    status_t convertYUV420SemiPlanar(
            const BitmapParams &src, const BitmapParams &dst);

    status_t convertTIYUV420PackedSemiPlanar(
            const BitmapParams &src, const BitmapParams &dst);

    ColorConverter(const ColorConverter &);
    ColorConverter &operator=(const ColorConverter &);
};

}  

#endif  
