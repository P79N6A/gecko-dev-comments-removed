





#ifndef RTSPPRLOG_H
#define RTSPPRLOG_H

#include "mozilla/Logging.h"

extern PRLogModuleInfo* gRtspLog;

#define LOGI(msg, ...) MOZ_LOG(gRtspLog, mozilla::LogLevel::Info, (msg, ##__VA_ARGS__))
#define LOGV(msg, ...) MOZ_LOG(gRtspLog, mozilla::LogLevel::Debug, (msg, ##__VA_ARGS__))
#define LOGE(msg, ...) MOZ_LOG(gRtspLog, mozilla::LogLevel::Error, (msg, ##__VA_ARGS__))
#define LOGW(msg, ...) MOZ_LOG(gRtspLog, mozilla::LogLevel::Warning, (msg, ##__VA_ARGS__))

#endif  
