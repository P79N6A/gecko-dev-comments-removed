





#ifndef mozilla_system_mozmtpcommon_h__
#define mozilla_system_mozmtpcommon_h__

#include "mozilla/Types.h"
#include <android/log.h>

#define USE_DEBUG 0

#if USE_DEBUG
#define MTP_DBG(msg, ...)                                            \
  __android_log_print(ANDROID_LOG_DEBUG, "MozMtp",                    \
                      "%s: " msg, __FUNCTION__, ##__VA_ARGS__)
#else
#define MTP_DBG(msg, ...)
#endif

#define MTP_LOG(msg, ...)                                            \
  __android_log_print(ANDROID_LOG_INFO, "MozMtp",                    \
                      "%s: " msg, __FUNCTION__, ##__VA_ARGS__)

#define MTP_ERR(msg, ...)                                            \
  __android_log_print(ANDROID_LOG_ERROR, "MozMtp",                   \
                      "%s: " msg, __FUNCTION__, ##__VA_ARGS__)

#define BEGIN_MTP_NAMESPACE \
  namespace mozilla { namespace system { namespace mtp {
#define END_MTP_NAMESPACE \
  } /* namespace mtp */ } /* namespace system */ } /* namespace mozilla */
#define USING_MTP_NAMESPACE \
  using namespace mozilla::system::mtp;

namespace android {
  class MOZ_EXPORT MtpServer;
  class MOZ_EXPORT MtpStorage;
  class MOZ_EXPORT MtpStringBuffer;
  class MOZ_EXPORT MtpDatabase;
  class MOZ_EXPORT MtpDataPacket;
  class MOZ_EXPORT MtpProperty;
}

#include <mtp.h>
#include <MtpDatabase.h>
#include <MtpObjectInfo.h>
#include <MtpProperty.h>
#include <MtpServer.h>
#include <MtpStorage.h>
#include <MtpStringBuffer.h>
#include <MtpTypes.h>

#endif 
