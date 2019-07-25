































#ifndef OPUS_H
#define OPUS_H

#include "opus_types.h"
#include "opus_defines.h"

#ifdef __cplusplus
extern "C" {
#endif




















































































































typedef struct OpusEncoder OpusEncoder;

OPUS_EXPORT int opus_encoder_get_size(int channels);
































OPUS_EXPORT OpusEncoder *opus_encoder_create(
    opus_int32 Fs,
    int channels,
    int application,
    int *error
);












OPUS_EXPORT int opus_encoder_init(
    OpusEncoder *st,
    opus_int32 Fs,
    int channels,
    int application
);













OPUS_EXPORT int opus_encode(
    OpusEncoder *st,
    const opus_int16 *pcm,
    int frame_size,
    unsigned char *data,
    int max_data_bytes
);













OPUS_EXPORT int opus_encode_float(
    OpusEncoder *st,
    const float *pcm,
    int frame_size,
    unsigned char *data,
    int max_data_bytes
);




OPUS_EXPORT void opus_encoder_destroy(OpusEncoder *st);







OPUS_EXPORT int opus_encoder_ctl(OpusEncoder *st, int request, ...);

























































typedef struct OpusDecoder OpusDecoder;





OPUS_EXPORT int opus_decoder_get_size(int channels);






OPUS_EXPORT OpusDecoder *opus_decoder_create(
    opus_int32 Fs,
    int channels,
    int *error
);










OPUS_EXPORT int opus_decoder_init(
    OpusDecoder *st,
    opus_int32 Fs,
    int channels
);













OPUS_EXPORT int opus_decode(
    OpusDecoder *st,
    const unsigned char *data,
    int len,
    opus_int16 *pcm,
    int frame_size,
    int decode_fec
);













OPUS_EXPORT int opus_decode_float(
    OpusDecoder *st,
    const unsigned char *data,
    int len,
    float *pcm,
    int frame_size,
    int decode_fec
);







OPUS_EXPORT int opus_decoder_ctl(OpusDecoder *st, int request, ...);




OPUS_EXPORT void opus_decoder_destroy(OpusDecoder *st);














OPUS_EXPORT int opus_packet_parse(
   const unsigned char *data,
   int len,
   unsigned char *out_toc,
   const unsigned char *frames[48],
   short size[48],
   int *payload_offset
);










OPUS_EXPORT int opus_packet_get_bandwidth(const unsigned char *data);







OPUS_EXPORT int opus_packet_get_samples_per_frame(const unsigned char *data, opus_int32 Fs);






OPUS_EXPORT int opus_packet_get_nb_channels(const unsigned char *data);







OPUS_EXPORT int opus_packet_get_nb_frames(const unsigned char packet[], int len);








OPUS_EXPORT int opus_decoder_get_nb_samples(const OpusDecoder *dec, const unsigned char packet[], int len);










typedef struct OpusRepacketizer OpusRepacketizer;

OPUS_EXPORT int opus_repacketizer_get_size(void);

OPUS_EXPORT OpusRepacketizer *opus_repacketizer_init(OpusRepacketizer *rp);

OPUS_EXPORT OpusRepacketizer *opus_repacketizer_create(void);

OPUS_EXPORT void opus_repacketizer_destroy(OpusRepacketizer *rp);

OPUS_EXPORT int opus_repacketizer_cat(OpusRepacketizer *rp, const unsigned char *data, int len);

OPUS_EXPORT opus_int32 opus_repacketizer_out_range(OpusRepacketizer *rp, int begin, int end, unsigned char *data, int maxlen);

OPUS_EXPORT int opus_repacketizer_get_nb_frames(OpusRepacketizer *rp);

OPUS_EXPORT opus_int32 opus_repacketizer_out(OpusRepacketizer *rp, unsigned char *data, int maxlen);



#ifdef __cplusplus
}
#endif

#endif
