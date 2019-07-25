



#ifndef Logging_h
#define Logging_h

#ifdef ANDROID
#include <android/log.h>
#define log(...) __android_log_print(ANDROID_LOG_ERROR, "GeckoLinker", __VA_ARGS__)
#else
#include <cstdio>
#define log(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#endif

#ifdef MOZ_DEBUG_LINKER
#define debug log
#else
#define debug(...)
#endif

#endif 
