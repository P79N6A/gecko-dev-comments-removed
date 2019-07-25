


























#ifndef _LIBS_CUTILS_LOG_H
#define _LIBS_CUTILS_LOG_H

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <stdarg.h>

#include <cutils/uio.h>
#include <cutils/logd.h>

#ifdef __cplusplus
extern "C" {
#endif








#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif






#ifndef LOG_TAG
#define LOG_TAG NULL
#endif






#ifndef ALOGV
#if LOG_NDEBUG
#define ALOGV(...)   ((void)0)
#else
#define ALOGV(...) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LOGV
#define LOGV ALOGV
#endif
#endif

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

#ifndef ALOGV_IF
#if LOG_NDEBUG
#define ALOGV_IF(cond, ...)   ((void)0)
#else
#define ALOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

#ifndef LOGV_IF
#define LOGV_IF ALOGV_IF
#endif
#endif




#ifndef ALOGD
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))

#ifndef LOGD
#define LOGD ALOGD
#endif
#endif

#ifndef ALOGD_IF
#define ALOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )

#ifndef LOGD_IF
#define LOGD_IF ALOGD_IF
#endif
#endif




#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))

#ifndef LOGI
#define LOGI ALOGI
#endif
#endif

#ifndef ALOGI_IF
#define ALOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )

#ifndef LOGI_IF
#define LOGI_IF ALOGI_IF
#endif
#endif




#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))

#ifndef LOGW
#define LOGW ALOGW
#endif
#endif

#ifndef ALOGW_IF
#define ALOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )

#ifndef LOGW_IF
#define LOGW_IF ALOGW_IF
#endif
#endif




#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))

#ifndef LOGE
#define LOGE ALOGE
#endif
#endif

#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )

#ifndef LOGE_IF
#define LOGE_IF ALOGE_IF
#endif
#endif







#ifndef IF_ALOGV
#if LOG_NDEBUG
#define IF_ALOGV() if (false)
#else
#define IF_ALOGV() IF_ALOG(LOG_VERBOSE, LOG_TAG)
#endif

#ifndef IF_LOGV
#define IF_LOGV IF_ALOGV
#endif
#endif





#ifndef IF_ALOGD
#define IF_ALOGD() IF_ALOG(LOG_DEBUG, LOG_TAG)

#ifndef IF_LOGD
#define IF_LOGD IF_ALOGD
#endif
#endif





#ifndef IF_ALOGI
#define IF_ALOGI() IF_ALOG(LOG_INFO, LOG_TAG)

#ifndef IF_LOGI
#define IF_LOGI IF_ALOGI
#endif
#endif





#ifndef IF_ALOGW
#define IF_ALOGW() IF_ALOG(LOG_WARN, LOG_TAG)

#ifndef IF_LOGW
#define IF_LOGW IF_ALOGW
#endif
#endif





#ifndef IF_ALOGE
#define IF_ALOGE() IF_ALOG(LOG_ERROR, LOG_TAG)

#ifndef IF_LOGE
#define IF_LOGE IF_ALOGE
#endif
#endif







#ifndef SLOGV
#if LOG_NDEBUG
#define SLOGV(...)   ((void)0)
#else
#define SLOGV(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

#ifndef SLOGV_IF
#if LOG_NDEBUG
#define SLOGV_IF(cond, ...)   ((void)0)
#else
#define SLOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif




#ifndef SLOGD
#define SLOGD(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGD_IF
#define SLOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif




#ifndef SLOGI
#define SLOGI(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGI_IF
#define SLOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif




#ifndef SLOGW
#define SLOGW(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGW_IF
#define SLOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif




#ifndef SLOGE
#define SLOGE(...) ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGE_IF
#define SLOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

    









#ifndef LOG_ALWAYS_FATAL_IF
#define LOG_ALWAYS_FATAL_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)android_printAssert(#cond, LOG_TAG, ## __VA_ARGS__)) \
    : (void)0 )
#endif

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(...) \
    ( ((void)android_printAssert(NULL, LOG_TAG, ## __VA_ARGS__)) )
#endif





#if LOG_NDEBUG

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) ((void)0)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) ((void)0)
#endif

#else

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) LOG_ALWAYS_FATAL_IF(cond, ## __VA_ARGS__)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) LOG_ALWAYS_FATAL(__VA_ARGS__)
#endif

#endif





#ifndef ALOG_ASSERT
#define ALOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ## __VA_ARGS__)


#ifndef LOG_ASSERT
#define LOG_ASSERT ALOG_ASSERT
#endif
#endif











#ifndef ALOG
#define ALOG(priority, tag, ...) \
    LOG_PRI(ANDROID_##priority, tag, __VA_ARGS__)

#ifndef LOG
#define LOG ALOG
#endif
#endif




#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) \
    android_printLog(priority, tag, __VA_ARGS__)
#endif




#ifndef LOG_PRI_VA
#define LOG_PRI_VA(priority, tag, fmt, args) \
    android_vprintLog(priority, NULL, tag, fmt, args)
#endif




#ifndef IF_ALOG
#define IF_ALOG(priority, tag) \
    if (android_testLog(ANDROID_##priority, tag))

#ifndef IF_LOG
#define IF_LOG IF_ALOG
#endif
#endif











typedef enum {
    EVENT_TYPE_INT      = 0,
    EVENT_TYPE_LONG     = 1,
    EVENT_TYPE_STRING   = 2,
    EVENT_TYPE_LIST     = 3,
} AndroidEventLogType;


#ifndef LOG_EVENT_INT
#define LOG_EVENT_INT(_tag, _value) {                                       \
        int intBuf = _value;                                                \
        (void) android_btWriteLog(_tag, EVENT_TYPE_INT, &intBuf,            \
            sizeof(intBuf));                                                \
    }
#endif
#ifndef LOG_EVENT_LONG
#define LOG_EVENT_LONG(_tag, _value) {                                      \
        long long longBuf = _value;                                         \
        (void) android_btWriteLog(_tag, EVENT_TYPE_LONG, &longBuf,          \
            sizeof(longBuf));                                               \
    }
#endif
#ifndef LOG_EVENT_STRING
#define LOG_EVENT_STRING(_tag, _value)                                      \
    ((void) 0)  /* not implemented -- must combine len with string */
#endif








#define android_printLog(prio, tag, fmt...) \
    __android_log_print(prio, tag, fmt)

#define android_vprintLog(prio, cond, tag, fmt...) \
    __android_log_vprint(prio, tag, fmt)









#define __android_second(dummy, second, ...)     second




#define __android_rest(first, ...)               , ## __VA_ARGS__

#define android_printAssert(cond, tag, fmt...) \
    __android_log_assert(cond, tag, \
        __android_second(0, ## fmt, NULL) __android_rest(fmt))

#define android_writeLog(prio, tag, text) \
    __android_log_write(prio, tag, text)

#define android_bWriteLog(tag, payload, len) \
    __android_log_bwrite(tag, payload, len)
#define android_btWriteLog(tag, type, payload, len) \
    __android_log_btwrite(tag, type, payload, len)


#define android_testLog(prio, tag) (1)
#define android_writevLog(vec,num) do{}while(0)
#define android_write1Log(str,len) do{}while (0)
#define android_setMinPriority(tag, prio) do{}while(0)

#define android_logToFile(tag, file) (0)
#define android_logToFd(tag, fd) (0)

typedef enum {
    LOG_ID_MAIN = 0,
    LOG_ID_RADIO = 1,
    LOG_ID_EVENTS = 2,
    LOG_ID_SYSTEM = 3,

    LOG_ID_MAX
} log_id_t;




int __android_log_buf_write(int bufID, int prio, const char *tag, const char *text);
int __android_log_buf_print(int bufID, int prio, const char *tag, const char *fmt, ...);


#ifdef __cplusplus
}
#endif

#endif
