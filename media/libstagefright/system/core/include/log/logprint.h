















#ifndef _LOGPRINT_H
#define _LOGPRINT_H

#include <log/log.h>
#include <log/logger.h>
#include <log/event_tag_map.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FORMAT_OFF = 0,
    FORMAT_BRIEF,
    FORMAT_PROCESS,
    FORMAT_TAG,
    FORMAT_THREAD,
    FORMAT_RAW,
    FORMAT_TIME,
    FORMAT_THREADTIME,
    FORMAT_LONG,
} AndroidLogPrintFormat;

typedef struct AndroidLogFormat_t AndroidLogFormat;

typedef struct AndroidLogEntry_t {
    time_t tv_sec;
    long tv_nsec;
    android_LogPriority priority;
    int32_t pid;
    int32_t tid;
    const char * tag;
    size_t messageLen;
    const char * message;
} AndroidLogEntry;

AndroidLogFormat *android_log_format_new();

void android_log_format_free(AndroidLogFormat *p_format);

void android_log_setPrintFormat(AndroidLogFormat *p_format, 
        AndroidLogPrintFormat format);




AndroidLogPrintFormat android_log_formatFromString(const char *s);











int android_log_addFilterRule(AndroidLogFormat *p_format, 
        const char *filterExpression);












int android_log_addFilterString(AndroidLogFormat *p_format,
        const char *filterString);






int android_log_shouldPrintLine (
        AndroidLogFormat *p_format, const char *tag, android_LogPriority pri);









int android_log_processLogBuffer(struct logger_entry *buf,
                                 AndroidLogEntry *entry);







int android_log_processBinaryLogBuffer(struct logger_entry *buf,
    AndroidLogEntry *entry, const EventTagMap* map, char* messageBuf,
    int messageBufLen);










char *android_log_formatLogLine (    
    AndroidLogFormat *p_format,
    char *defaultBuffer,
    size_t defaultBufferSize,
    const AndroidLogEntry *p_line,
    size_t *p_outLength);








int android_log_printLogLine(
    AndroidLogFormat *p_format,
    int fd,
    const AndroidLogEntry *entry);


#ifdef __cplusplus
}
#endif


#endif
