

















#ifndef AVUTIL_SAMPLEFMT_H
#define AVUTIL_SAMPLEFMT_H

#include "avutil.h"




enum AVSampleFormat {
    AV_SAMPLE_FMT_NONE = -1,
    AV_SAMPLE_FMT_U8,          
    AV_SAMPLE_FMT_S16,         
    AV_SAMPLE_FMT_S32,         
    AV_SAMPLE_FMT_FLT,         
    AV_SAMPLE_FMT_DBL,         

    AV_SAMPLE_FMT_U8P,         
    AV_SAMPLE_FMT_S16P,        
    AV_SAMPLE_FMT_S32P,        
    AV_SAMPLE_FMT_FLTP,        
    AV_SAMPLE_FMT_DBLP,        

    AV_SAMPLE_FMT_NB           
};





const char *av_get_sample_fmt_name(enum AVSampleFormat sample_fmt);





enum AVSampleFormat av_get_sample_fmt(const char *name);













char *av_get_sample_fmt_string(char *buf, int buf_size, enum AVSampleFormat sample_fmt);

#if FF_API_GET_BITS_PER_SAMPLE_FMT



attribute_deprecated
int av_get_bits_per_sample_fmt(enum AVSampleFormat sample_fmt);
#endif








int av_get_bytes_per_sample(enum AVSampleFormat sample_fmt);







int av_sample_fmt_is_planar(enum AVSampleFormat sample_fmt);










int av_samples_get_buffer_size(int *linesize, int nb_channels, int nb_samples,
                               enum AVSampleFormat sample_fmt, int align);






















int av_samples_fill_arrays(uint8_t **audio_data, int *linesize, uint8_t *buf,
                           int nb_channels, int nb_samples,
                           enum AVSampleFormat sample_fmt, int align);














int av_samples_alloc(uint8_t **audio_data, int *linesize, int nb_channels,
                     int nb_samples, enum AVSampleFormat sample_fmt, int align);

#endif 
