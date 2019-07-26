






































#ifndef CSFLogStream_h
#define CSFLogStream_h

#include "CSFLog.h"
#include <string>
#include <sstream>
#include <iostream>

#define CSFLogCriticalS(tag, message)	{ std::ostringstream _oss; _oss << message << std::endl; CSFLog( CSF_LOG_CRITICAL, __FILE__ , __LINE__ , tag, _oss.str().c_str()); }
#define CSFLogErrorS(tag, message)		{ std::ostringstream _oss; _oss << message << std::endl; CSFLog( CSF_LOG_ERROR, __FILE__ , __LINE__ , tag, _oss.str().c_str()); }
#define CSFLogWarnS(tag, message)		{ std::ostringstream _oss; _oss << message << std::endl; CSFLog( CSF_LOG_WARNING, __FILE__ , __LINE__ , tag, _oss.str().c_str()); }
#define CSFLogNoticeS(tag, message)		{ std::ostringstream _oss; _oss << message << std::endl; CSFLog( CSF_LOG_NOTICE, __FILE__ , __LINE__ , tag, _oss.str().c_str()); }
#define CSFLogInfoS(tag, message)		{ std::ostringstream _oss; _oss << message << std::endl; CSFLog( CSF_LOG_INFO, __FILE__ , __LINE__ , tag, _oss.str().c_str()); }
#define CSFLogDebugS(tag, message)		{ std::ostringstream _oss; _oss << message << std::endl; CSFLog( CSF_LOG_DEBUG, __FILE__ , __LINE__ , tag, _oss.str().c_str()); }

#endif
