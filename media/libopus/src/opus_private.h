




































#ifndef OPUS_PRIVATE_H
#define OPUS_PRIVATE_H

#include "arch.h"
#include "opus.h"

struct OpusRepacketizer {
   unsigned char toc;
   int nb_frames;
   const unsigned char *frames[48];
   short len[48];
   int framesize;
};


#define MODE_SILK_ONLY          1000
#define MODE_HYBRID             1001
#define MODE_CELT_ONLY          1002

#define OPUS_SET_VOICE_RATIO_REQUEST         11018
#define OPUS_GET_VOICE_RATIO_REQUEST         11019










#define OPUS_SET_VOICE_RATIO(x) OPUS_SET_VOICE_RATIO_REQUEST, __opus_check_int(x)




#define OPUS_GET_VOICE_RATIO(x) OPUS_GET_VOICE_RATIO_REQUEST, __opus_check_int_ptr(x)


#define OPUS_SET_FORCE_MODE_REQUEST    11002
#define OPUS_SET_FORCE_MODE(x) OPUS_SET_FORCE_MODE_REQUEST, __opus_check_int(x)


int encode_size(int size, unsigned char *data);

int opus_decode_native(OpusDecoder *st, const unsigned char *data, int len,
      opus_val16 *pcm, int frame_size, int decode_fec, int self_delimited, int *packet_offset);


static inline int align(int i)
{
    return (i+sizeof(void *)-1)&-sizeof(void *);
}

int opus_repacketizer_out_range_impl(OpusRepacketizer *rp, int begin, int end, unsigned char *data, int maxlen, int self_delimited);

#endif 
