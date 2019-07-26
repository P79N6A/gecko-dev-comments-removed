



























#ifndef OPUS_PRIVATE_H
#define OPUS_PRIVATE_H

#include "arch.h"
#include "opus.h"
#include "celt.h"

struct OpusRepacketizer {
   unsigned char toc;
   int nb_frames;
   const unsigned char *frames[48];
   opus_int16 len[48];
   int framesize;
};

typedef struct ChannelLayout {
   int nb_channels;
   int nb_streams;
   int nb_coupled_streams;
   unsigned char mapping[256];
} ChannelLayout;

int validate_layout(const ChannelLayout *layout);
int get_left_channel(const ChannelLayout *layout, int stream_id, int prev);
int get_right_channel(const ChannelLayout *layout, int stream_id, int prev);
int get_mono_channel(const ChannelLayout *layout, int stream_id, int prev);



#define MODE_SILK_ONLY          1000
#define MODE_HYBRID             1001
#define MODE_CELT_ONLY          1002

#define OPUS_SET_VOICE_RATIO_REQUEST         11018
#define OPUS_GET_VOICE_RATIO_REQUEST         11019










#define OPUS_SET_VOICE_RATIO(x) OPUS_SET_VOICE_RATIO_REQUEST, __opus_check_int(x)




#define OPUS_GET_VOICE_RATIO(x) OPUS_GET_VOICE_RATIO_REQUEST, __opus_check_int_ptr(x)


#define OPUS_SET_FORCE_MODE_REQUEST    11002
#define OPUS_SET_FORCE_MODE(x) OPUS_SET_FORCE_MODE_REQUEST, __opus_check_int(x)

typedef void (*downmix_func)(const void *, float *, int, int, int);
void downmix_float(const void *_x, float *sub, int subframe, int offset, int C);
void downmix_int(const void *_x, float *sub, int subframe, int offset, int C);

int optimize_framesize(const opus_val16 *x, int len, int C, opus_int32 Fs,
                int bitrate, opus_val16 tonality, opus_val32 *mem, int buffering,
                downmix_func downmix);

int encode_size(int size, unsigned char *data);

opus_int32 frame_size_select(opus_int32 frame_size, int variable_duration, opus_int32 Fs);

opus_int32 opus_encode_native(OpusEncoder *st, const opus_val16 *pcm, int frame_size,
      unsigned char *data, opus_int32 out_data_bytes, int lsb_depth
#ifndef FIXED_POINT
                , AnalysisInfo *analysis_info
#endif
      );

int opus_decode_native(OpusDecoder *st, const unsigned char *data, opus_int32 len,
      opus_val16 *pcm, int frame_size, int decode_fec, int self_delimited,
      int *packet_offset, int soft_clip);


static inline int align(int i)
{
    return (i+(int)sizeof(void *)-1)&-(int)sizeof(void *);
}

opus_int32 opus_repacketizer_out_range_impl(OpusRepacketizer *rp, int begin, int end, unsigned char *data, opus_int32 maxlen, int self_delimited);

#endif 
