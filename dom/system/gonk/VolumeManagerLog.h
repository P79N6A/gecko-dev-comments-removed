



#ifndef mozilla_system_volumemanagerlog_h__
#define mozilla_system_volumemanagerlog_h__

#define USE_DEBUG 0

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO,  "VolumeManager" , ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "VolumeManager" , ## args)

#if USE_DEBUG
#define DBG(args...)  __android_log_print(ANDROID_LOG_DEBUG, "VolumeManager" , ## args)
#else
#define DBG(args...)
#endif

#endif  
