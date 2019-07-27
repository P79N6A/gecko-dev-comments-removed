

















#ifndef AVUTIL_PARSEUTILS_H
#define AVUTIL_PARSEUTILS_H

#include <time.h>

#include "rational.h"

















int av_parse_video_size(int *width_ptr, int *height_ptr, const char *str);










int av_parse_video_rate(AVRational *rate, const char *str);




















int av_parse_color(uint8_t *rgba_color, const char *color_string, int slen,
                   void *log_ctx);

































int av_parse_time(int64_t *timeval, const char *timestr, int duration);







int av_find_info_tag(char *arg, int arg_size, const char *tag1, const char *info);




time_t av_timegm(struct tm *tm);

#endif 
