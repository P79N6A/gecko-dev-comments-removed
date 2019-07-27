





#ifndef DOM_CAMERA_CAMERACOMMON_H
#define DOM_CAMERA_CAMERACOMMON_H

#ifndef __func__
#ifdef __FUNCTION__
#define __func__ __FUNCTION__
#else
#define __func__ __FILE__
#endif
#endif

#include "mozilla/Logging.h"

extern PRLogModuleInfo* GetCameraLog();
#define DOM_CAMERA_LOG( type, ... ) MOZ_LOG(GetCameraLog(), (mozilla::LogLevel)type, ( __VA_ARGS__ ))

#define DOM_CAMERA_LOGA( ... )      DOM_CAMERA_LOG( mozilla::LogLevel::Error, __VA_ARGS__ )




enum {
  DOM_CAMERA_LOG_NOTHING,
  DOM_CAMERA_LOG_ERROR,
  DOM_CAMERA_LOG_WARNING,
  DOM_CAMERA_LOG_INFO,
  DOM_CAMERA_LOG_TRACE,
  DOM_CAMERA_LOG_REFERENCES
};





#define DOM_CAMERA_LOGR( ... )                                  \
  do {                                                          \
    if (GetCameraLog()) {                                       \
      DOM_CAMERA_LOG( DOM_CAMERA_LOG_REFERENCES, __VA_ARGS__ ); \
    }                                                           \
  } while (0)
#define DOM_CAMERA_LOGT( ... )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_TRACE, __VA_ARGS__ )
#define DOM_CAMERA_LOGI( ... )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_INFO, __VA_ARGS__ )
#define DOM_CAMERA_LOGW( ... )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_WARNING, __VA_ARGS__ )
#define DOM_CAMERA_LOGE( ... )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_ERROR, __VA_ARGS__ )

#endif 
