

























#ifndef AVUTIL_TIMECODE_H
#define AVUTIL_TIMECODE_H

#include <stdint.h>
#include "rational.h"

#define AV_TIMECODE_STR_SIZE 16

enum AVTimecodeFlag {
    AV_TIMECODE_FLAG_DROPFRAME      = 1<<0, 
    AV_TIMECODE_FLAG_24HOURSMAX     = 1<<1, 
    AV_TIMECODE_FLAG_ALLOWNEGATIVE  = 1<<2, 
};

typedef struct {
    int start;          
    uint32_t flags;     
    AVRational rate;    
    unsigned fps;       
} AVTimecode;









int av_timecode_adjust_ntsc_framenum2(int framenum, int fps);














uint32_t av_timecode_get_smpte_from_framenum(const AVTimecode *tc, int framenum);













char *av_timecode_make_string(const AVTimecode *tc, char *buf, int framenum);










char *av_timecode_make_smpte_tc_string(char *buf, uint32_t tcsmpte, int prevent_df);








char *av_timecode_make_mpeg_tc_string(char *buf, uint32_t tc25bit);













int av_timecode_init(AVTimecode *tc, AVRational rate, int flags, int frame_start, void *log_ctx);











int av_timecode_init_from_string(AVTimecode *tc, AVRational rate, const char *str, void *log_ctx);






int av_timecode_check_frame_rate(AVRational rate);

#endif 
