





#ifndef RTSPPRLOG_H
#define RTSPPRLOG_H

#include "prlog.h"

extern PRLogModuleInfo* gRtspLog;

#define LOGI(msg, ...) PR_LOG(gRtspLog, PR_LOG_ALWAYS, (msg, ##__VA_ARGS__))
#define LOGV(msg, ...) PR_LOG(gRtspLog, PR_LOG_DEBUG, (msg, ##__VA_ARGS__))
#define LOGE(msg, ...) PR_LOG(gRtspLog, PR_LOG_ERROR, (msg, ##__VA_ARGS__))
#define LOGW(msg, ...) PR_LOG(gRtspLog, PR_LOG_WARNING, (msg, ##__VA_ARGS__))

#endif  
