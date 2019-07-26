


























#ifndef _LIBS_UTILS_LOG_H
#define _LIBS_UTILS_LOG_H

#include <cutils/log.h>
#include <sys/types.h>

#ifdef __cplusplus

namespace android {




class LogIfSlow {
public:
    LogIfSlow(const char* tag, android_LogPriority priority,
            int timeoutMillis, const char* message);
    ~LogIfSlow();

private:
    const char* const mTag;
    const android_LogPriority mPriority;
    const int mTimeoutMillis;
    const char* const mMessage;
    const int64_t mStart;
};










#define ALOGD_IF_SLOW(timeoutMillis, message) \
    android::LogIfSlow _logIfSlow(LOG_TAG, ANDROID_LOG_DEBUG, timeoutMillis, message);

} 

#endif 

#endif 
