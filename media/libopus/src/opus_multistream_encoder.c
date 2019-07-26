


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "opus_multistream.h"
#include "opus.h"
#include "opus_private.h"
#include "stack_alloc.h"
#include <stdarg.h>
#include "float_cast.h"
#include "os_support.h"
#include "analysis.h"
#include "mathops.h"

typedef struct {
   int nb_streams;
   int nb_coupled_streams;
   unsigned char mapping[8];
} VorbisLayout;


static const VorbisLayout vorbis_mappings[8] = {
      {1, 0, {0}},                      
      {1, 1, {0, 1}},                   
      {2, 1, {0, 2, 1}},                
      {2, 2, {0, 1, 2, 3}},             
      {3, 2, {0, 4, 1, 2, 3}},          
      {4, 2, {0, 4, 1, 2, 3, 5}},       
      {4, 3, {0, 4, 1, 2, 3, 5, 6}},    
      {5, 3, {0, 6, 1, 2, 3, 4, 5, 7}}, 
};

struct OpusMSEncoder {
   TonalityAnalysisState analysis;
   ChannelLayout layout;
   int lfe_stream;
   int variable_duration;
   int surround;
   opus_int32 bitrate_bps;
   opus_val32 subframe_mem[3];
   
};


static int validate_encoder_layout(const ChannelLayout *layout)
{
   int s;
   for (s=0;s<layout->nb_streams;s++)
   {
      if (s < layout->nb_coupled_streams)
      {
         if (get_left_channel(layout, s, -1)==-1)
            return 0;
         if (get_right_channel(layout, s, -1)==-1)
            return 0;
      } else {
         if (get_mono_channel(layout, s, -1)==-1)
            return 0;
      }
   }
   return 1;
}


opus_int32 opus_multistream_encoder_get_size(int nb_streams, int nb_coupled_streams)
{
   int coupled_size;
   int mono_size;

   if(nb_streams<1||nb_coupled_streams>nb_streams||nb_coupled_streams<0)return 0;
   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);
   return align(sizeof(OpusMSEncoder))
        + nb_coupled_streams * align(coupled_size)
        + (nb_streams-nb_coupled_streams) * align(mono_size);
}

opus_int32 opus_multistream_surround_encoder_get_size(int channels, int mapping_family)
{
   int nb_streams;
   int nb_coupled_streams;
   opus_int32 size;

   if (mapping_family==0)
   {
      if (channels==1)
      {
         nb_streams=1;
         nb_coupled_streams=0;
      } else if (channels==2)
      {
         nb_streams=1;
         nb_coupled_streams=1;
      } else
         return 0;
   } else if (mapping_family==1 && channels<=8 && channels>=1)
   {
      nb_streams=vorbis_mappings[channels-1].nb_streams;
      nb_coupled_streams=vorbis_mappings[channels-1].nb_coupled_streams;
   } else if (mapping_family==255)
   {
      nb_streams=channels;
      nb_coupled_streams=0;
   } else
      return 0;
   size = opus_multistream_encoder_get_size(nb_streams, nb_coupled_streams);
   if (channels>2)
      size += align(opus_encoder_get_size(2));
   return size;
}


static int opus_multistream_encoder_init_impl(
      OpusMSEncoder *st,
      opus_int32 Fs,
      int channels,
      int streams,
      int coupled_streams,
      const unsigned char *mapping,
      int application,
      int surround
)
{
   int coupled_size;
   int mono_size;
   int i, ret;
   char *ptr;

   if ((channels>255) || (channels<1) || (coupled_streams>streams) ||
       (coupled_streams+streams>255) || (streams<1) || (coupled_streams<0))
      return OPUS_BAD_ARG;

   st->layout.nb_channels = channels;
   st->layout.nb_streams = streams;
   st->layout.nb_coupled_streams = coupled_streams;
   st->subframe_mem[0]=st->subframe_mem[1]=st->subframe_mem[2]=0;
   OPUS_CLEAR(&st->analysis,1);
   if (!surround)
      st->lfe_stream = -1;
   st->bitrate_bps = OPUS_AUTO;
   st->variable_duration = OPUS_FRAMESIZE_ARG;
   for (i=0;i<st->layout.nb_channels;i++)
      st->layout.mapping[i] = mapping[i];
   if (!validate_layout(&st->layout) || !validate_encoder_layout(&st->layout))
      return OPUS_BAD_ARG;
   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);

   for (i=0;i<st->layout.nb_coupled_streams;i++)
   {
      ret = opus_encoder_init((OpusEncoder*)ptr, Fs, 2, application);
      if(ret!=OPUS_OK)return ret;
      if (i==st->lfe_stream)
         opus_encoder_ctl((OpusEncoder*)ptr, OPUS_SET_LFE(1));
      ptr += align(coupled_size);
   }
   for (;i<st->layout.nb_streams;i++)
   {
      ret = opus_encoder_init((OpusEncoder*)ptr, Fs, 1, application);
      if (i==st->lfe_stream)
         opus_encoder_ctl((OpusEncoder*)ptr, OPUS_SET_LFE(1));
      if(ret!=OPUS_OK)return ret;
      ptr += align(mono_size);
   }
   if (surround)
   {
      OpusEncoder *downmix_enc;
      downmix_enc = (OpusEncoder*)ptr;
      ret = opus_encoder_init(downmix_enc, Fs, 2, OPUS_APPLICATION_AUDIO);
      if(ret!=OPUS_OK)return ret;
   }
   st->surround = surround;
   return OPUS_OK;
}

int opus_multistream_encoder_init(
      OpusMSEncoder *st,
      opus_int32 Fs,
      int channels,
      int streams,
      int coupled_streams,
      const unsigned char *mapping,
      int application
)
{
   return opus_multistream_encoder_init_impl(st, Fs, channels, streams, coupled_streams, mapping, application, 0);
}

int opus_multistream_surround_encoder_init(
      OpusMSEncoder *st,
      opus_int32 Fs,
      int channels,
      int mapping_family,
      int *streams,
      int *coupled_streams,
      unsigned char *mapping,
      int application
)
{
   if ((channels>255) || (channels<1))
      return OPUS_BAD_ARG;
   st->lfe_stream = -1;
   if (mapping_family==0)
   {
      if (channels==1)
      {
         *streams=1;
         *coupled_streams=0;
         mapping[0]=0;
      } else if (channels==2)
      {
         *streams=1;
         *coupled_streams=1;
         mapping[0]=0;
         mapping[1]=1;
      } else
         return OPUS_UNIMPLEMENTED;
   } else if (mapping_family==1 && channels<=8 && channels>=1)
   {
      int i;
      *streams=vorbis_mappings[channels-1].nb_streams;
      *coupled_streams=vorbis_mappings[channels-1].nb_coupled_streams;
      for (i=0;i<channels;i++)
         mapping[i] = vorbis_mappings[channels-1].mapping[i];
      if (channels>=6)
         st->lfe_stream = *streams-1;
   } else if (mapping_family==255)
   {
      int i;
      *streams=channels;
      *coupled_streams=0;
      for(i=0;i<channels;i++)
         mapping[i] = i;
   } else
      return OPUS_UNIMPLEMENTED;
   return opus_multistream_encoder_init_impl(st, Fs, channels, *streams, *coupled_streams,
         mapping, application, channels>2&&mapping_family==1);
}

OpusMSEncoder *opus_multistream_encoder_create(
      opus_int32 Fs,
      int channels,
      int streams,
      int coupled_streams,
      const unsigned char *mapping,
      int application,
      int *error
)
{
   int ret;
   OpusMSEncoder *st;
   if ((channels>255) || (channels<1) || (coupled_streams>streams) ||
       (coupled_streams+streams>255) || (streams<1) || (coupled_streams<0))
   {
      if (error)
         *error = OPUS_BAD_ARG;
      return NULL;
   }
   st = (OpusMSEncoder *)opus_alloc(opus_multistream_encoder_get_size(streams, coupled_streams));
   if (st==NULL)
   {
      if (error)
         *error = OPUS_ALLOC_FAIL;
      return NULL;
   }
   ret = opus_multistream_encoder_init(st, Fs, channels, streams, coupled_streams, mapping, application);
   if (ret != OPUS_OK)
   {
      opus_free(st);
      st = NULL;
   }
   if (error)
      *error = ret;
   return st;
}

OpusMSEncoder *opus_multistream_surround_encoder_create(
      opus_int32 Fs,
      int channels,
      int mapping_family,
      int *streams,
      int *coupled_streams,
      unsigned char *mapping,
      int application,
      int *error
)
{
   int ret;
   OpusMSEncoder *st;
   if ((channels>255) || (channels<1))
   {
      if (error)
         *error = OPUS_BAD_ARG;
      return NULL;
   }
   st = (OpusMSEncoder *)opus_alloc(opus_multistream_surround_encoder_get_size(channels, mapping_family));
   if (st==NULL)
   {
      if (error)
         *error = OPUS_ALLOC_FAIL;
      return NULL;
   }
   ret = opus_multistream_surround_encoder_init(st, Fs, channels, mapping_family, streams, coupled_streams, mapping, application);
   if (ret != OPUS_OK)
   {
      opus_free(st);
      st = NULL;
   }
   if (error)
      *error = ret;
   return st;
}

typedef void (*opus_copy_channel_in_func)(
  opus_val16 *dst,
  int dst_stride,
  const void *src,
  int src_stride,
  int src_channel,
  int frame_size
);

typedef void (*opus_surround_downmix_funct)(
  opus_val16 *dst,
  const void *src,
  int channels,
  int frame_size
);

static void surround_rate_allocation(
      OpusMSEncoder *st,
      opus_int32 *rate,
      int frame_size
      )
{
   int i;
   opus_int32 channel_rate;
   opus_int32 Fs;
   char *ptr;
   int stream_offset;
   int lfe_offset;
   int coupled_ratio; 
   int lfe_ratio;     

   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   opus_encoder_ctl((OpusEncoder*)ptr, OPUS_GET_SAMPLE_RATE(&Fs));

   if (st->bitrate_bps > st->layout.nb_channels*40000)
      stream_offset = 20000;
   else
      stream_offset = st->bitrate_bps/st->layout.nb_channels/2;
   

   
   lfe_offset = 3500;
   
   coupled_ratio = 512;
   
   lfe_ratio = 32;

   
   if (st->bitrate_bps==OPUS_AUTO)
   {
      channel_rate = Fs+60*Fs/frame_size;
   } else if (st->bitrate_bps==OPUS_BITRATE_MAX)
   {
      channel_rate = 300000;
   } else {
      int nb_lfe;
      int nb_uncoupled;
      int nb_coupled;
      int total;
      nb_lfe = (st->lfe_stream!=-1);
      nb_coupled = st->layout.nb_coupled_streams;
      nb_uncoupled = st->layout.nb_streams-nb_coupled-nb_lfe;
      total = (nb_uncoupled<<8)         
            + coupled_ratio*nb_coupled 
            + nb_lfe*lfe_ratio;
      channel_rate = 256*(st->bitrate_bps-lfe_offset*nb_lfe-stream_offset*(nb_coupled+nb_uncoupled))/total;
   }
#ifndef FIXED_POINT
   if (st->variable_duration==OPUS_FRAMESIZE_VARIABLE && frame_size != Fs/50)
   {
      opus_int32 bonus;
      bonus = 60*(Fs/frame_size-50);
      channel_rate += bonus;
   }
#endif

   for (i=0;i<st->layout.nb_streams;i++)
   {
      if (i<st->layout.nb_coupled_streams)
         rate[i] = stream_offset+(channel_rate*coupled_ratio>>8);
      else if (i!=st->lfe_stream)
         rate[i] = stream_offset+channel_rate;
      else
         rate[i] = lfe_offset+(channel_rate*lfe_ratio>>8);
   }
}


#define MS_FRAME_TMP (3*1275+7)
static int opus_multistream_encode_native
(
    OpusMSEncoder *st,
    opus_copy_channel_in_func copy_channel_in,
    const void *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes,
    int lsb_depth,
    opus_surround_downmix_funct surround_downmix
#ifndef FIXED_POINT
    , downmix_func downmix
    , const void *pcm_analysis
#endif
)
{
   opus_int32 Fs;
   int coupled_size;
   int mono_size;
   int s;
   char *ptr;
   int tot_size;
   VARDECL(opus_val16, buf);
   unsigned char tmp_data[MS_FRAME_TMP];
   OpusRepacketizer rp;
   opus_int32 complexity;
#ifndef FIXED_POINT
   AnalysisInfo analysis_info;
#endif
   const CELTMode *celt_mode;
   opus_int32 bitrates[256];
   opus_val16 bandLogE[42];
   opus_val16 bandLogE_mono[21];
   ALLOC_STACK;

   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   opus_encoder_ctl((OpusEncoder*)ptr, OPUS_GET_SAMPLE_RATE(&Fs));
   opus_encoder_ctl((OpusEncoder*)ptr, OPUS_GET_COMPLEXITY(&complexity));
   opus_encoder_ctl((OpusEncoder*)ptr, CELT_GET_MODE(&celt_mode));

   if (400*frame_size < Fs)
   {
      RESTORE_STACK;
      return OPUS_BAD_ARG;
   }
#ifndef FIXED_POINT
   analysis_info.valid = 0;
   if (complexity >= 7 && Fs==48000)
   {
      opus_int32 delay_compensation;
      int channels;

      channels = st->layout.nb_streams + st->layout.nb_coupled_streams;
      opus_encoder_ctl((OpusEncoder*)ptr, OPUS_GET_LOOKAHEAD(&delay_compensation));
      delay_compensation -= Fs/400;

      frame_size = run_analysis(&st->analysis, celt_mode, pcm, pcm_analysis,
            frame_size, st->variable_duration, channels, Fs, st->bitrate_bps, delay_compensation, lsb_depth, downmix, &analysis_info);
   } else
#endif
   {
      frame_size = frame_size_select(frame_size, st->variable_duration, Fs);
   }
   

   if (400*frame_size != Fs && 200*frame_size != Fs &&
       100*frame_size != Fs &&  50*frame_size != Fs &&
        25*frame_size != Fs &&  50*frame_size != 3*Fs)
   {
      RESTORE_STACK;
      return OPUS_BAD_ARG;
   }
   ALLOC(buf, 2*frame_size, opus_val16);
   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);

   if (st->surround)
   {
      int i;
      unsigned char dummy[512];
      
      OpusEncoder *downmix_enc;

      ptr = (char*)st + align(sizeof(OpusMSEncoder));
      for (s=0;s<st->layout.nb_streams;s++)
      {
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
      }
      downmix_enc = (OpusEncoder*)ptr;
      surround_downmix(buf, pcm, st->layout.nb_channels, frame_size);
      opus_encoder_ctl(downmix_enc, OPUS_SET_ENERGY_SAVE(bandLogE));
      opus_encoder_ctl(downmix_enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
      opus_encoder_ctl(downmix_enc, OPUS_SET_FORCE_MODE(MODE_CELT_ONLY));
      opus_encoder_ctl(downmix_enc, OPUS_SET_FORCE_CHANNELS(2));
      opus_encode_native(downmix_enc, buf, frame_size, dummy, 512, lsb_depth
#ifndef FIXED_POINT
            , &analysis_info
#endif
            );
      

      for(i=0;i<21;i++)
      {
         opus_val16 diff;
         diff = ABS16(SUB16(bandLogE[i], bandLogE[21+i]));
         diff = diff + HALF16(diff);
         diff = SHR32(HALF32(celt_exp2(-diff)), 16-DB_SHIFT);
         bandLogE_mono[i] = MAX16(bandLogE[i], bandLogE[21+i]) + diff;
      }
   }

   if (max_data_bytes < 4*st->layout.nb_streams-1)
   {
      RESTORE_STACK;
      return OPUS_BUFFER_TOO_SMALL;
   }

   
   surround_rate_allocation(st, bitrates, frame_size);

   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   for (s=0;s<st->layout.nb_streams;s++)
   {
      OpusEncoder *enc;
      enc = (OpusEncoder*)ptr;
      if (s < st->layout.nb_coupled_streams)
         ptr += align(coupled_size);
      else
         ptr += align(mono_size);
      opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrates[s]));
      if (st->surround)
      {
         opus_encoder_ctl(enc, OPUS_SET_FORCE_MODE(MODE_CELT_ONLY));
         opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
         if (s < st->layout.nb_coupled_streams)
            opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(2));
      }
   }

   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   
   tot_size = 0;
   for (s=0;s<st->layout.nb_streams;s++)
   {
      OpusEncoder *enc;
      int len;
      int curr_max;

      opus_repacketizer_init(&rp);
      enc = (OpusEncoder*)ptr;
      if (s < st->layout.nb_coupled_streams)
      {
         int left, right;
         left = get_left_channel(&st->layout, s, -1);
         right = get_right_channel(&st->layout, s, -1);
         (*copy_channel_in)(buf, 2,
            pcm, st->layout.nb_channels, left, frame_size);
         (*copy_channel_in)(buf+1, 2,
            pcm, st->layout.nb_channels, right, frame_size);
         ptr += align(coupled_size);
         

         if (st->surround)
            opus_encoder_ctl(enc, OPUS_SET_ENERGY_MASK(bandLogE));
      } else {
         int chan = get_mono_channel(&st->layout, s, -1);
         (*copy_channel_in)(buf, 1,
            pcm, st->layout.nb_channels, chan, frame_size);
         ptr += align(mono_size);
         if (st->surround)
            opus_encoder_ctl(enc, OPUS_SET_ENERGY_MASK(bandLogE_mono));
      }
      
      curr_max = max_data_bytes - tot_size;
      
      curr_max -= IMAX(0,4*(st->layout.nb_streams-s-1)-1);
      curr_max = IMIN(curr_max,MS_FRAME_TMP);
      len = opus_encode_native(enc, buf, frame_size, tmp_data, curr_max, lsb_depth
#ifndef FIXED_POINT
            , &analysis_info
#endif
            );
      if (len<0)
      {
         RESTORE_STACK;
         return len;
      }
      


      opus_repacketizer_cat(&rp, tmp_data, len);
      len = opus_repacketizer_out_range_impl(&rp, 0, opus_repacketizer_get_nb_frames(&rp), data, max_data_bytes-tot_size, s != st->layout.nb_streams-1);
      data += len;
      tot_size += len;
   }
   RESTORE_STACK;
   return tot_size;

}

static void channel_pos(int channels, int pos[8])
{
   
   if (channels==4)
   {
      pos[0]=1;
      pos[1]=3;
      pos[2]=1;
      pos[3]=3;
   } else if (channels==3||channels==5||channels==6)
   {
      pos[0]=1;
      pos[1]=2;
      pos[2]=3;
      pos[3]=1;
      pos[4]=3;
      pos[5]=0;
   } else if (channels==7)
   {
      pos[0]=1;
      pos[1]=2;
      pos[2]=3;
      pos[3]=1;
      pos[4]=3;
      pos[5]=2;
      pos[6]=0;
   } else if (channels==8)
   {
      pos[0]=1;
      pos[1]=2;
      pos[2]=3;
      pos[3]=1;
      pos[4]=3;
      pos[5]=1;
      pos[6]=3;
      pos[7]=0;
   }
}

#if !defined(DISABLE_FLOAT_API)
static void opus_copy_channel_in_float(
  opus_val16 *dst,
  int dst_stride,
  const void *src,
  int src_stride,
  int src_channel,
  int frame_size
)
{
   const float *float_src;
   opus_int32 i;
   float_src = (const float *)src;
   for (i=0;i<frame_size;i++)
#if defined(FIXED_POINT)
      dst[i*dst_stride] = FLOAT2INT16(float_src[i*src_stride+src_channel]);
#else
      dst[i*dst_stride] = float_src[i*src_stride+src_channel];
#endif
}

static void opus_surround_downmix_float(
  opus_val16 *dst,
  const void *src,
  int channels,
  int frame_size
)
{
   const float *float_src;
   opus_int32 i;
   int pos[8] = {0};
   int c;
   float_src = (const float *)src;

   channel_pos(channels, pos);
   for (i=0;i<2*frame_size;i++)
      dst[i]=0;

   for (c=0;c<channels;c++)
   {
      if (pos[c]==1)
      {
         for (i=0;i<frame_size;i++)
#if defined(FIXED_POINT)
            dst[2*i] += SHR16(FLOAT2INT16(float_src[i*channels+c]),3);
#else
            dst[2*i] += float_src[i*channels+c];
#endif
      } else if (pos[c]==3)
      {
         for (i=0;i<frame_size;i++)
#if defined(FIXED_POINT)
            dst[2*i+1] += SHR16(FLOAT2INT16(float_src[i*channels+c]),3);
#else
            dst[2*i+1] += float_src[i*channels+c];
#endif
      } else if (pos[c]==2)
      {
         for (i=0;i<frame_size;i++)
         {
#if defined(FIXED_POINT)
            dst[2*i] += SHR32(MULT16_16(QCONST16(.70711f,15), FLOAT2INT16(float_src[i*channels+c])),3+15);
            dst[2*i+1] += SHR32(MULT16_16(QCONST16(.70711f,15), FLOAT2INT16(float_src[i*channels+c])),3+15);
#else
            dst[2*i] += .707f*float_src[i*channels+c];
            dst[2*i+1] += .707f*float_src[i*channels+c];
#endif
         }
      }
   }
}
#endif

static void opus_copy_channel_in_short(
  opus_val16 *dst,
  int dst_stride,
  const void *src,
  int src_stride,
  int src_channel,
  int frame_size
)
{
   const opus_int16 *short_src;
   opus_int32 i;
   short_src = (const opus_int16 *)src;
   for (i=0;i<frame_size;i++)
#if defined(FIXED_POINT)
      dst[i*dst_stride] = short_src[i*src_stride+src_channel];
#else
      dst[i*dst_stride] = (1/32768.f)*short_src[i*src_stride+src_channel];
#endif
}

static void opus_surround_downmix_short(
  opus_val16 *dst,
  const void *src,
  int channels,
  int frame_size
)
{
   const opus_int16 *short_src;
   opus_int32 i;
   int pos[8] = {0};
   int c;
   short_src = (const opus_int16 *)src;

   channel_pos(channels, pos);
   for (i=0;i<2*frame_size;i++)
      dst[i]=0;

   for (c=0;c<channels;c++)
   {
      if (pos[c]==1)
      {
         for (i=0;i<frame_size;i++)
#if defined(FIXED_POINT)
            dst[2*i] += SHR16(short_src[i*channels+c],3);
#else
            dst[2*i] += (1/32768.f)*short_src[i*channels+c];
#endif
      } else if (pos[c]==3)
      {
         for (i=0;i<frame_size;i++)
#if defined(FIXED_POINT)
            dst[2*i+1] += SHR16(short_src[i*channels+c],3);
#else
            dst[2*i+1] += (1/32768.f)*short_src[i*channels+c];
#endif
      } else if (pos[c]==2)
      {
         for (i=0;i<frame_size;i++)
         {
#if defined(FIXED_POINT)
            dst[2*i] += SHR32(MULT16_16(QCONST16(.70711f,15), short_src[i*channels+c]),3+15);
            dst[2*i+1] += SHR32(MULT16_16(QCONST16(.70711f,15), short_src[i*channels+c]),3+15);
#else
            dst[2*i] += (.707f/32768.f)*short_src[i*channels+c];
            dst[2*i+1] += (.707f/32768.f)*short_src[i*channels+c];
#endif
         }
      }
   }
}


#ifdef FIXED_POINT
int opus_multistream_encode(
    OpusMSEncoder *st,
    const opus_val16 *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes
)
{
   return opus_multistream_encode_native(st, opus_copy_channel_in_short,
      pcm, frame_size, data, max_data_bytes, 16, opus_surround_downmix_short);
}

#ifndef DISABLE_FLOAT_API
int opus_multistream_encode_float(
    OpusMSEncoder *st,
    const float *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes
)
{
   return opus_multistream_encode_native(st, opus_copy_channel_in_float,
      pcm, frame_size, data, max_data_bytes, 16, opus_surround_downmix_float);
}
#endif

#else

int opus_multistream_encode_float
(
    OpusMSEncoder *st,
    const opus_val16 *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes
)
{
   int channels = st->layout.nb_streams + st->layout.nb_coupled_streams;
   return opus_multistream_encode_native(st, opus_copy_channel_in_float,
      pcm, frame_size, data, max_data_bytes, 24, opus_surround_downmix_float, downmix_float, pcm+channels*st->analysis.analysis_offset);
}

int opus_multistream_encode(
    OpusMSEncoder *st,
    const opus_int16 *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes
)
{
   int channels = st->layout.nb_streams + st->layout.nb_coupled_streams;
   return opus_multistream_encode_native(st, opus_copy_channel_in_short,
      pcm, frame_size, data, max_data_bytes, 16, opus_surround_downmix_short, downmix_int, pcm+channels*st->analysis.analysis_offset);
}
#endif

int opus_multistream_encoder_ctl(OpusMSEncoder *st, int request, ...)
{
   va_list ap;
   int coupled_size, mono_size;
   char *ptr;
   int ret = OPUS_OK;

   va_start(ap, request);

   coupled_size = opus_encoder_get_size(2);
   mono_size = opus_encoder_get_size(1);
   ptr = (char*)st + align(sizeof(OpusMSEncoder));
   switch (request)
   {
   case OPUS_SET_BITRATE_REQUEST:
   {
      opus_int32 value = va_arg(ap, opus_int32);
      if (value<0 && value!=OPUS_AUTO && value!=OPUS_BITRATE_MAX)
      {
         goto bad_arg;
      }
      st->bitrate_bps = value;
   }
   break;
   case OPUS_GET_BITRATE_REQUEST:
   {
      int s;
      opus_int32 *value = va_arg(ap, opus_int32*);
      if (!value)
      {
         goto bad_arg;
      }
      *value = 0;
      for (s=0;s<st->layout.nb_streams;s++)
      {
         opus_int32 rate;
         OpusEncoder *enc;
         enc = (OpusEncoder*)ptr;
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
         opus_encoder_ctl(enc, request, &rate);
         *value += rate;
      }
   }
   break;
   case OPUS_GET_LSB_DEPTH_REQUEST:
   case OPUS_GET_VBR_REQUEST:
   case OPUS_GET_APPLICATION_REQUEST:
   case OPUS_GET_BANDWIDTH_REQUEST:
   case OPUS_GET_COMPLEXITY_REQUEST:
   case OPUS_GET_PACKET_LOSS_PERC_REQUEST:
   case OPUS_GET_DTX_REQUEST:
   case OPUS_GET_VOICE_RATIO_REQUEST:
   case OPUS_GET_VBR_CONSTRAINT_REQUEST:
   case OPUS_GET_SIGNAL_REQUEST:
   case OPUS_GET_LOOKAHEAD_REQUEST:
   case OPUS_GET_SAMPLE_RATE_REQUEST:
   case OPUS_GET_INBAND_FEC_REQUEST:
   case OPUS_GET_FORCE_CHANNELS_REQUEST:
   {
      OpusEncoder *enc;
      
      opus_int32 *value = va_arg(ap, opus_int32*);
      enc = (OpusEncoder*)ptr;
      ret = opus_encoder_ctl(enc, request, value);
   }
   break;
   case OPUS_GET_FINAL_RANGE_REQUEST:
   {
      int s;
      opus_uint32 *value = va_arg(ap, opus_uint32*);
      opus_uint32 tmp;
      if (!value)
      {
         goto bad_arg;
      }
      *value=0;
      for (s=0;s<st->layout.nb_streams;s++)
      {
         OpusEncoder *enc;
         enc = (OpusEncoder*)ptr;
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
         ret = opus_encoder_ctl(enc, request, &tmp);
         if (ret != OPUS_OK) break;
         *value ^= tmp;
      }
   }
   break;
   case OPUS_SET_LSB_DEPTH_REQUEST:
   case OPUS_SET_COMPLEXITY_REQUEST:
   case OPUS_SET_VBR_REQUEST:
   case OPUS_SET_VBR_CONSTRAINT_REQUEST:
   case OPUS_SET_MAX_BANDWIDTH_REQUEST:
   case OPUS_SET_BANDWIDTH_REQUEST:
   case OPUS_SET_SIGNAL_REQUEST:
   case OPUS_SET_APPLICATION_REQUEST:
   case OPUS_SET_INBAND_FEC_REQUEST:
   case OPUS_SET_PACKET_LOSS_PERC_REQUEST:
   case OPUS_SET_DTX_REQUEST:
   case OPUS_SET_FORCE_MODE_REQUEST:
   case OPUS_SET_FORCE_CHANNELS_REQUEST:
   {
      int s;
      
      opus_int32 value = va_arg(ap, opus_int32);
      for (s=0;s<st->layout.nb_streams;s++)
      {
         OpusEncoder *enc;

         enc = (OpusEncoder*)ptr;
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
         ret = opus_encoder_ctl(enc, request, value);
         if (ret != OPUS_OK)
            break;
      }
   }
   break;
   case OPUS_MULTISTREAM_GET_ENCODER_STATE_REQUEST:
   {
      int s;
      opus_int32 stream_id;
      OpusEncoder **value;
      stream_id = va_arg(ap, opus_int32);
      if (stream_id<0 || stream_id >= st->layout.nb_streams)
         ret = OPUS_BAD_ARG;
      value = va_arg(ap, OpusEncoder**);
      if (!value)
      {
         goto bad_arg;
      }
      for (s=0;s<stream_id;s++)
      {
         if (s < st->layout.nb_coupled_streams)
            ptr += align(coupled_size);
         else
            ptr += align(mono_size);
      }
      *value = (OpusEncoder*)ptr;
   }
   break;
   case OPUS_SET_EXPERT_FRAME_DURATION_REQUEST:
   {
       opus_int32 value = va_arg(ap, opus_int32);
       st->variable_duration = value;
   }
   break;
   case OPUS_GET_EXPERT_FRAME_DURATION_REQUEST:
   {
       opus_int32 *value = va_arg(ap, opus_int32*);
       if (!value)
       {
          goto bad_arg;
       }
       *value = st->variable_duration;
   }
   break;
   default:
      ret = OPUS_UNIMPLEMENTED;
      break;
   }

   va_end(ap);
   return ret;
bad_arg:
   va_end(ap);
   return OPUS_BAD_ARG;
}

void opus_multistream_encoder_destroy(OpusMSEncoder *st)
{
    opus_free(st);
}
