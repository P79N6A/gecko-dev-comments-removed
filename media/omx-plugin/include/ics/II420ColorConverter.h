















#ifndef II420_COLOR_CONVERTER_H

#define II420_COLOR_CONVERTER_H

#include <stdint.h>
#include <android/rect.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct II420ColorConverter {

    





    int (*getDecoderOutputFormat)();

    














    int (*convertDecoderOutputToI420)(
        void* decoderBits, int decoderWidth, int decoderHeight,
        ARect decoderRect, void* dstBits);

    






    int (*getEncoderInputFormat)();

    


















    int (*convertI420ToEncoderInput)(
        void* srcBits, int srcWidth, int srcHeight,
        int encoderWidth, int encoderHeight, ARect encoderRect,
        void* encoderBits);

    




















    int (*getEncoderInputBufferInfo)(
        int srcWidth, int srcHeight,
        int* encoderWidth, int* encoderHeight,
        ARect* encoderRect, int* encoderBufferSize);

} II420ColorConverter;



void getI420ColorConverter(II420ColorConverter *converter);

#if defined(__cplusplus)
}
#endif

#endif

