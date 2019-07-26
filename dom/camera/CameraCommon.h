





#ifndef DOM_CAMERA_CAMERACOMMON_H
#define DOM_CAMERA_CAMERACOMMON_H

#ifndef __func__
#ifdef __FUNCTION__
#define __func__ __FUNCTION__
#else
#define __func__ __FILE__
#endif
#endif

#ifndef NAN
#define NAN std::numeric_limits<double>::quiet_NaN()
#endif

#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetCameraLog();
#define DOM_CAMERA_LOG( type, ... ) PR_LOG(GetCameraLog(), (PRLogModuleLevel)type, ( __VA_ARGS__ ))
#else
#define DOM_CAMERA_LOG( type, ... )
#endif

#define DOM_CAMERA_LOGA( ... )      DOM_CAMERA_LOG( 0, __VA_ARGS__ )




enum {
  DOM_CAMERA_LOG_NOTHING,
  DOM_CAMERA_LOG_ERROR,
  DOM_CAMERA_LOG_WARNING,
  DOM_CAMERA_LOG_INFO,
  DOM_CAMERA_LOG_TRACE,
  DOM_CAMERA_LOG_REFERENCES
};





#ifdef PR_LOGGING
#define DOM_CAMERA_LOGR( ... )                                  \
  do {                                                          \
    if (GetCameraLog()) {                                       \
      DOM_CAMERA_LOG( DOM_CAMERA_LOG_REFERENCES, __VA_ARGS__ ); \
    }                                                           \
  } while (0)
#else
#define DOM_CAMERA_LOGR( ... )
#endif
#define DOM_CAMERA_LOGT( ... )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_TRACE, __VA_ARGS__ )
#define DOM_CAMERA_LOGI( ... )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_INFO, __VA_ARGS__ )
#define DOM_CAMERA_LOGW( ... )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_WARNING, __VA_ARGS__ )
#define DOM_CAMERA_LOGE( ... )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_ERROR, __VA_ARGS__ )

#ifdef PR_LOGGING

static inline void nsLogAddRefCamera(const char *file, uint32_t line, void* p, uint32_t count, const char *clazz, uint32_t size)
{
  if (count == 1) {
    DOM_CAMERA_LOGR("++++++++++++++++++++++++++++++++++++++++");
  }
  DOM_CAMERA_LOGR("%s:%d : CAMREF-ADD(%s): this=%p, mRefCnt=%d\n", file, line, clazz, p, count);
}

static inline void nsLogReleaseCamera(const char *file, uint32_t line, void* p, uint32_t count, const char *clazz, bool abortOnDelete)
{
  DOM_CAMERA_LOGR("%s:%d : CAMREF-REL(%s): this=%p, mRefCnt=%d\n", file, line, clazz, p, count);
  if (count == 0) {
    if (!abortOnDelete) {
      DOM_CAMERA_LOGR("----------------------------------------");
    } else {
      DOM_CAMERA_LOGR("---------- ABORTING ON DELETE ----------");
      *((uint32_t *)0xdeadbeef) = 0x266230;
    }
  }
}

#ifdef NS_LOG_ADDREF
#undef NS_LOG_ADDREF
#endif
#ifdef NS_LOG_RELEASE
#undef NS_LOG_RELEASE
#endif

#define NS_LOG_ADDREF( p, n, c, s ) nsLogAddRefCamera(__FILE__, __LINE__, (p), (n), (c), (s))
#ifdef DOM_CAMERA_DEBUG_REFS_ABORT_ON_DELETE
#define NS_LOG_RELEASE( p, n, c )   nsLogReleaseCamera(__FILE__, __LINE__, (p), (n), (c), DOM_CAMERA_DEBUG_REFS_ABORT_ON_DELETE)
#else
#define NS_LOG_RELEASE( p, n, c )   nsLogReleaseCamera(__FILE__, __LINE__, (p), (n), (c), false)
#endif

#endif 

#endif 
