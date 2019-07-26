



#include "WebRtcLog.h"

#include "prlog.h"
#include "prenv.h"
#include "webrtc/system_wrappers/interface/trace.h"

#include "nscore.h"
#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#include "mozilla/Preferences.h"
#else
#include "nsStringAPI.h"
#endif

static int gWebRtcTraceLoggingOn = 0;
static const char *default_log = "WebRTC.log";

static PRLogModuleInfo* GetWebRtcTraceLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("webrtc_trace");
  }
  return sLog;
}

class WebRtcTraceCallback: public webrtc::TraceCallback
{
public:
  void Print(webrtc::TraceLevel level, const char* message, int length)
  {
    PRLogModuleInfo *log = GetWebRtcTraceLog();
    PR_LOG(log, PR_LOG_DEBUG, ("%s", message));
    return;
  }
};

static WebRtcTraceCallback gWebRtcCallback;

#ifdef MOZILLA_INTERNAL_API
void GetWebRtcLogPrefs(uint32_t *aTraceMask, nsACString* aLogFile, bool *aMultiLog)
{
  *aMultiLog = mozilla::Preferences::GetBool("media.webrtc.debug.multi_log");
  *aTraceMask = mozilla::Preferences::GetUint("media.webrtc.debug.trace_mask");
  mozilla::Preferences::GetCString("media.webrtc.debug.log_file", aLogFile);
}
#endif

void CheckOverrides(uint32_t *aTraceMask, nsACString *aLogFile, bool *aMultiLog)
{
  if (!aTraceMask || !aLogFile || !aMultiLog) {
    return;
  }

  

  PRLogModuleInfo *log_info = GetWebRtcTraceLog();
  





  if (log_info && (log_info->level != 0)) {
    *aTraceMask = log_info->level;
  }

  const char *file_name = PR_GetEnv("WEBRTC_TRACE_FILE");
  if (file_name) {
    aLogFile->Assign(file_name);
  }
}

void ConfigWebRtcLog(uint32_t trace_mask, nsCString &aLogFile, bool multi_log)
{
  if (gWebRtcTraceLoggingOn || trace_mask == 0) {
    return;
  }

  if (aLogFile.IsEmpty()) {
#if defined(XP_WIN)
    
    const char *temp_dir = PR_GetEnv("TEMP");
    if (!temp_dir) {
      aLogFile.Assign(default_log);
    } else {
      aLogFile.Assign(temp_dir);
      aLogFile.Append('/');
      aLogFile.Append(default_log);
    }
#elif defined(ANDROID)
    
    aLogFile.Assign("nspr");
#else
    
    aLogFile.Assign("/tmp/");
    aLogFile.Append(default_log);
#endif
  }

  webrtc::Trace::set_level_filter(trace_mask);
  if (aLogFile.EqualsLiteral("nspr")) {
    webrtc::Trace::SetTraceCallback(&gWebRtcCallback);
  } else {
    webrtc::Trace::SetTraceFile(aLogFile.get(), multi_log);
  }

  return;
}

void StartWebRtcLog(uint32_t log_level)
{
  if (gWebRtcTraceLoggingOn && log_level != 0) {
    return;
  }

  if (log_level == 0) { 
    if (gWebRtcTraceLoggingOn) {
      gWebRtcTraceLoggingOn = false;
      webrtc::Trace::set_level_filter(webrtc::kTraceNone);
    }
    return;
  }

  uint32_t trace_mask = 0;
  bool multi_log = false;
  nsAutoCString log_file;

#ifdef MOZILLA_INTERNAL_API
  GetWebRtcLogPrefs(&trace_mask, &log_file, &multi_log);
#endif
  CheckOverrides(&trace_mask, &log_file, &multi_log);

  if (trace_mask == 0) {
    trace_mask = log_level;
  }

  ConfigWebRtcLog(trace_mask, log_file, multi_log);
  return;

}

void EnableWebRtcLog()
{
  if (gWebRtcTraceLoggingOn) {
    return;
  }

  uint32_t trace_mask = 0;
  bool multi_log = false;
  nsAutoCString log_file;

#ifdef MOZILLA_INTERNAL_API
  GetWebRtcLogPrefs(&trace_mask, &log_file, &multi_log);
#endif
  CheckOverrides(&trace_mask, &log_file, &multi_log);
  ConfigWebRtcLog(trace_mask, log_file, multi_log);
  return;
}

