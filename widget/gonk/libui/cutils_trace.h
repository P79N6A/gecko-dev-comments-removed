















#ifndef _LIBS_CUTILS_TRACE_H
#define _LIBS_CUTILS_TRACE_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <cutils/compiler.h>

#ifdef ANDROID_SMP
#include <cutils/atomic-inline.h>
#else
#include <cutils/atomic.h>
#endif

__BEGIN_DECLS



















#define ATRACE_TAG_NEVER            0       // This tag is never enabled.
#define ATRACE_TAG_ALWAYS           (1<<0)  // This tag is always enabled.
#define ATRACE_TAG_GRAPHICS         (1<<1)
#define ATRACE_TAG_INPUT            (1<<2)
#define ATRACE_TAG_VIEW             (1<<3)
#define ATRACE_TAG_WEBVIEW          (1<<4)
#define ATRACE_TAG_WINDOW_MANAGER   (1<<5)
#define ATRACE_TAG_ACTIVITY_MANAGER (1<<6)
#define ATRACE_TAG_SYNC_MANAGER     (1<<7)
#define ATRACE_TAG_AUDIO            (1<<8)
#define ATRACE_TAG_VIDEO            (1<<9)
#define ATRACE_TAG_CAMERA           (1<<10)
#define ATRACE_TAG_HAL              (1<<11)
#define ATRACE_TAG_APP              (1<<12)
#define ATRACE_TAG_RESOURCES        (1<<13)
#define ATRACE_TAG_DALVIK           (1<<14)
#define ATRACE_TAG_LAST             ATRACE_TAG_DALVIK


#define ATRACE_TAG_NOT_READY        (1LL<<63)

#define ATRACE_TAG_VALID_MASK ((ATRACE_TAG_LAST - 1) | ATRACE_TAG_LAST)

#ifndef ATRACE_TAG
#define ATRACE_TAG ATRACE_TAG_NEVER
#elif ATRACE_TAG > ATRACE_TAG_VALID_MASK
#error ATRACE_TAG must be defined to be one of the tags defined in cutils/trace.h
#endif

#ifdef HAVE_ANDROID_OS





#define ATRACE_MESSAGE_LENGTH 1024







void atrace_setup();





void atrace_update_tags();







void atrace_set_debuggable(bool debuggable);





void atrace_set_tracing_enabled(bool enabled);






extern volatile int32_t atrace_is_ready;






extern uint64_t atrace_enabled_tags;





extern int atrace_marker_fd;






#define ATRACE_INIT() atrace_init()
static inline void atrace_init()
{
    if (CC_UNLIKELY(!android_atomic_acquire_load(&atrace_is_ready))) {
        atrace_setup();
    }
}






#define ATRACE_GET_ENABLED_TAGS() atrace_get_enabled_tags()
static inline uint64_t atrace_get_enabled_tags()
{
    atrace_init();
    return atrace_enabled_tags;
}






#define ATRACE_ENABLED() atrace_is_tag_enabled(ATRACE_TAG)
static inline uint64_t atrace_is_tag_enabled(uint64_t tag)
{
    return atrace_get_enabled_tags() & tag;
}





#define ATRACE_BEGIN(name) atrace_begin(ATRACE_TAG, name)
static inline void atrace_begin(uint64_t tag, const char* name)
{
    if (CC_UNLIKELY(atrace_is_tag_enabled(tag))) {
        char buf[ATRACE_MESSAGE_LENGTH];
        size_t len;

        len = snprintf(buf, ATRACE_MESSAGE_LENGTH, "B|%d|%s", getpid(), name);
        write(atrace_marker_fd, buf, len);
    }
}





#define ATRACE_END() atrace_end(ATRACE_TAG)
static inline void atrace_end(uint64_t tag)
{
    if (CC_UNLIKELY(atrace_is_tag_enabled(tag))) {
        char c = 'E';
        write(atrace_marker_fd, &c, 1);
    }
}








#define ATRACE_ASYNC_BEGIN(name, cookie) \
    atrace_async_begin(ATRACE_TAG, name, cookie)
static inline void atrace_async_begin(uint64_t tag, const char* name,
        int32_t cookie)
{
    if (CC_UNLIKELY(atrace_is_tag_enabled(tag))) {
        char buf[ATRACE_MESSAGE_LENGTH];
        size_t len;

        len = snprintf(buf, ATRACE_MESSAGE_LENGTH, "S|%d|%s|%d", getpid(),
                name, cookie);
        write(atrace_marker_fd, buf, len);
    }
}





#define ATRACE_ASYNC_END(name, cookie) atrace_async_end(ATRACE_TAG, name, cookie)
static inline void atrace_async_end(uint64_t tag, const char* name,
        int32_t cookie)
{
    if (CC_UNLIKELY(atrace_is_tag_enabled(tag))) {
        char buf[ATRACE_MESSAGE_LENGTH];
        size_t len;

        len = snprintf(buf, ATRACE_MESSAGE_LENGTH, "F|%d|%s|%d", getpid(),
                name, cookie);
        write(atrace_marker_fd, buf, len);
    }
}






#define ATRACE_INT(name, value) atrace_int(ATRACE_TAG, name, value)
static inline void atrace_int(uint64_t tag, const char* name, int32_t value)
{
    if (CC_UNLIKELY(atrace_is_tag_enabled(tag))) {
        char buf[ATRACE_MESSAGE_LENGTH];
        size_t len;

        len = snprintf(buf, ATRACE_MESSAGE_LENGTH, "C|%d|%s|%d",
                getpid(), name, value);
        write(atrace_marker_fd, buf, len);
    }
}

#else 

#define ATRACE_INIT()
#define ATRACE_GET_ENABLED_TAGS()
#define ATRACE_ENABLED()
#define ATRACE_BEGIN(name)
#define ATRACE_END()
#define ATRACE_ASYNC_BEGIN(name, cookie)
#define ATRACE_ASYNC_END(name, cookie)
#define ATRACE_INT(name, value)

#endif 

__END_DECLS

#endif 
