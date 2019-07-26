

























#pragma once

#include <graphite2/Types.h>
#include <graphite2/Font.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif


typedef enum {
    GRLOG_NONE = 0x0,
    GRLOG_FACE = 0x01,
    GRLOG_SEGMENT = 0x02,
    GRLOG_PASS = 0x04,
    GRLOG_CACHE = 0x08,
    
    GRLOG_OPCODE = 0x80,
    GRLOG_ALL = 0xFF
} GrLogMask;










GR2_API bool gr_start_logging(gr_face * face, const char *log_path);







GR2_API void gr_stop_logging(gr_face * face);








GR2_API bool graphite_start_logging(FILE * logFile, GrLogMask mask);    




GR2_API void graphite_stop_logging();

#ifdef __cplusplus
}
#endif
