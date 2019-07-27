



#ifndef mozilla_system_volumemanagerlog_h__
#define mozilla_system_volumemanagerlog_h__

#undef USE_DEBUG
#define USE_DEBUG 0

#if !defined(VOLUME_MANAGER_LOG_TAG)
#define VOLUME_MANAGER_LOG_TAG  "VolumeManager"
#endif

#undef LOG
#undef ERR
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO,  VOLUME_MANAGER_LOG_TAG, ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, VOLUME_MANAGER_LOG_TAG, ## args)

#undef DBG
#if USE_DEBUG
#define DBG(args...)  __android_log_print(ANDROID_LOG_DEBUG, VOLUME_MANAGER_LOG_TAG, ## args)
#else
#define DBG(args...)
#endif

#endif  
