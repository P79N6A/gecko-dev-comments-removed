





#ifndef mozilla_SandboxLogging_h
#define mozilla_SandboxLogging_h

#if defined(ANDROID)
#include <android/log.h>
#else
#include <stdio.h>
#endif

#if defined(ANDROID)
#define SANDBOX_LOG_ERROR(args...) __android_log_print(ANDROID_LOG_ERROR, "Sandbox", ## args)
#else
#define SANDBOX_LOG_ERROR(fmt, args...) fprintf(stderr, "Sandbox: " fmt "\n", ## args)
#endif

#endif 
