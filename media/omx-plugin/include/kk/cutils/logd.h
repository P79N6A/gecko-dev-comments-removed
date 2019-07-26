















#ifndef _ANDROID_CUTILS_LOGD_H
#define _ANDROID_CUTILS_LOGD_H




#include <android/log.h>


#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <cutils/uio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int __android_log_bwrite(int32_t tag, const void *payload, size_t len);
int __android_log_btwrite(int32_t tag, char type, const void *payload,
    size_t len);

#ifdef __cplusplus
}
#endif

#endif
