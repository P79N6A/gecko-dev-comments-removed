









































#ifndef OPUS_CUSTOM_H
#define OPUS_CUSTOM_H


#include "opus_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CUSTOM_MODES
#define OPUS_CUSTOM_EXPORT OPUS_EXPORT
#define OPUS_CUSTOM_EXPORT_STATIC OPUS_EXPORT
#else
#define OPUS_CUSTOM_EXPORT
#ifdef CELT_C
#define OPUS_CUSTOM_EXPORT_STATIC static inline
#else
#define OPUS_CUSTOM_EXPORT_STATIC
#endif
#endif






typedef struct OpusCustomEncoder OpusCustomEncoder;




typedef struct OpusCustomDecoder OpusCustomDecoder;





typedef struct OpusCustomMode OpusCustomMode;










OPUS_CUSTOM_EXPORT OpusCustomMode *opus_custom_mode_create(opus_int32 Fs, int frame_size, int *error);





OPUS_CUSTOM_EXPORT void opus_custom_mode_destroy(OpusCustomMode *mode);



OPUS_CUSTOM_EXPORT_STATIC int opus_custom_encoder_get_size(const OpusCustomMode *mode, int channels);










OPUS_CUSTOM_EXPORT OpusCustomEncoder *opus_custom_encoder_create(const OpusCustomMode *mode, int channels, int *error);

OPUS_CUSTOM_EXPORT_STATIC int opus_custom_encoder_init(OpusCustomEncoder *st, const OpusCustomMode *mode, int channels);




OPUS_CUSTOM_EXPORT void opus_custom_encoder_destroy(OpusCustomEncoder *st);



















OPUS_CUSTOM_EXPORT int opus_custom_encode_float(OpusCustomEncoder *st, const float *pcm, int frame_size, unsigned char *compressed, int maxCompressedBytes);















OPUS_CUSTOM_EXPORT int opus_custom_encode(OpusCustomEncoder *st, const opus_int16 *pcm, int frame_size, unsigned char *compressed, int maxCompressedBytes);







OPUS_CUSTOM_EXPORT int opus_custom_encoder_ctl(OpusCustomEncoder * restrict st, int request, ...);



OPUS_CUSTOM_EXPORT_STATIC int opus_custom_decoder_get_size(const OpusCustomMode *mode, int channels);









OPUS_CUSTOM_EXPORT OpusCustomDecoder *opus_custom_decoder_create(const OpusCustomMode *mode, int channels, int *error);

OPUS_CUSTOM_EXPORT_STATIC int opus_custom_decoder_init(OpusCustomDecoder *st, const OpusCustomMode *mode, int channels);




OPUS_CUSTOM_EXPORT void opus_custom_decoder_destroy(OpusCustomDecoder *st);










OPUS_CUSTOM_EXPORT int opus_custom_decode_float(OpusCustomDecoder *st, const unsigned char *data, int len, float *pcm, int frame_size);










OPUS_CUSTOM_EXPORT int opus_custom_decode(OpusCustomDecoder *st, const unsigned char *data, int len, opus_int16 *pcm, int frame_size);







OPUS_CUSTOM_EXPORT int opus_custom_decoder_ctl(OpusCustomDecoder * restrict st, int request, ...);

#ifdef __cplusplus
}
#endif

#endif
