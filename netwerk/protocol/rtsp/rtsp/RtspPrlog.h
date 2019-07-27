





#ifndef RTSPPRLOG_H
#define RTSPPRLOG_H

#include "mozilla/Logging.h"

extern PRLogModuleInfo* gRtspLog;

#define LOGI(msg, ...) MOZ_LOG(gRtspLog, PR_LOG_ALWAYS, (msg, ##__VA_ARGS__))
#define LOGV(msg, ...) MOZ_LOG(gRtspLog, PR_LOG_DEBUG, (msg, ##__VA_ARGS__))
#define LOGE(msg, ...) MOZ_LOG(gRtspLog, PR_LOG_ERROR, (msg, ##__VA_ARGS__))
#define LOGW(msg, ...) MOZ_LOG(gRtspLog, PR_LOG_WARNING, (msg, ##__VA_ARGS__))

#endif  
