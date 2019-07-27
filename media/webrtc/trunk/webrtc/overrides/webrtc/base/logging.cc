









#include "third_party/webrtc/overrides/webrtc/base/logging.h"

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include <CoreServices/CoreServices.h>
#endif  

#include <iomanip>

#include "base/atomicops.h"
#include "base/strings/string_util.h"
#include "base/threading/platform_thread.h"
#include "third_party/webrtc/base/ipaddress.h"
#include "third_party/webrtc/base/stream.h"
#include "third_party/webrtc/base/stringencode.h"
#include "third_party/webrtc/base/stringutils.h"
#include "third_party/webrtc/base/timeutils.h"






#define LOG_LAZY_STREAM_DIRECT(file_name, line_number, sev) \
  LAZY_STREAM(logging::LogMessage(file_name, line_number, \
                                  -sev).stream(), true)

namespace rtc {

void (*g_logging_delegate_function)(const std::string&) = NULL;
void (*g_extra_logging_init_function)(
    void (*logging_delegate_function)(const std::string&)) = NULL;
#ifndef NDEBUG
COMPILE_ASSERT(sizeof(base::subtle::Atomic32) == sizeof(base::PlatformThreadId),
               atomic32_not_same_size_as_platformthreadid);
base::subtle::Atomic32 g_init_logging_delegate_thread_id = 0;
#endif





const char* FindLabel(int value, const ConstantLabel entries[]) {
  for (int i = 0; entries[i].label; ++i) {
    if (value == entries[i].value) return entries[i].label;
  }
  return 0;
}

std::string ErrorName(int err, const ConstantLabel* err_table) {
  if (err == 0)
    return "No error";

  if (err_table != 0) {
    if (const char * value = FindLabel(err, err_table))
      return value;
  }

  char buffer[16];
  base::snprintf(buffer, sizeof(buffer), "0x%08x", err);
  return buffer;
}






static std::string GenerateExtra(LogErrorContext err_ctx,
                                 int err,
                                 const char* module) {
  if (err_ctx != ERRCTX_NONE) {
    std::ostringstream tmp;
    tmp << ": ";
    tmp << "[0x" << std::setfill('0') << std::hex << std::setw(8) << err << "]";
    switch (err_ctx) {
      case ERRCTX_ERRNO:
        tmp << " " << strerror(err);
        break;
#if defined(WEBRTC_WIN)
      case ERRCTX_HRESULT: {
        char msgbuf[256];
        DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM;
        HMODULE hmod = GetModuleHandleA(module);
        if (hmod)
          flags |= FORMAT_MESSAGE_FROM_HMODULE;
        if (DWORD len = FormatMessageA(
            flags, hmod, err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            msgbuf, sizeof(msgbuf) / sizeof(msgbuf[0]), NULL)) {
          while ((len > 0) &&
              isspace(static_cast<unsigned char>(msgbuf[len-1]))) {
            msgbuf[--len] = 0;
          }
          tmp << " " << msgbuf;
        }
        break;
      }
#endif  
#if defined(WEBRTC_IOS)
      case ERRCTX_OSSTATUS:
        tmp << " " << "Unknown LibJingle error: " << err;
        break;
#elif defined(WEBRTC_MAC)
      case ERRCTX_OSSTATUS: {
        tmp << " " << nonnull(GetMacOSStatusErrorString(err), "Unknown error");
        if (const char* desc = GetMacOSStatusCommentString(err)) {
          tmp << ": " << desc;
        }
        break;
      }
#endif  
      default:
        break;
    }
    return tmp.str();
  }
  return "";
}

DiagnosticLogMessage::DiagnosticLogMessage(const char* file,
                                           int line,
                                           LoggingSeverity severity,
                                           bool log_to_chrome,
                                           LogErrorContext err_ctx,
                                           int err)
    : file_name_(file),
      line_(line),
      severity_(severity),
      log_to_chrome_(log_to_chrome) {
  extra_ = GenerateExtra(err_ctx, err, NULL);
}

DiagnosticLogMessage::DiagnosticLogMessage(const char* file,
                                           int line,
                                           LoggingSeverity severity,
                                           bool log_to_chrome,
                                           LogErrorContext err_ctx,
                                           int err,
                                           const char* module)
    : file_name_(file),
      line_(line),
      severity_(severity),
      log_to_chrome_(log_to_chrome) {
  extra_ = GenerateExtra(err_ctx, err, module);
}

DiagnosticLogMessage::~DiagnosticLogMessage() {
  print_stream_ << extra_;
  const std::string& str = print_stream_.str();
  if (log_to_chrome_)
    LOG_LAZY_STREAM_DIRECT(file_name_, line_, severity_) << str;
  if (g_logging_delegate_function && severity_ <= LS_INFO) {
    g_logging_delegate_function(str);
  }
}


void LogMessage::LogToDebug(int min_sev) {
  logging::SetMinLogLevel(min_sev);
}


void LogMultiline(LoggingSeverity level, const char* label, bool input,
                  const void* data, size_t len, bool hex_mode,
                  LogMultilineState* state) {
  if (!LOG_CHECK_LEVEL_V(level))
    return;

  const char * direction = (input ? " << " : " >> ");

  
  if (!data) {
    if (state && state->unprintable_count_[input]) {
      LOG_V(level) << label << direction << "## "
                   << state->unprintable_count_[input]
                   << " consecutive unprintable ##";
      state->unprintable_count_[input] = 0;
    }
    return;
  }

  
  const unsigned char* udata = static_cast<const unsigned char*>(data);

  if (hex_mode) {
    const size_t LINE_SIZE = 24;
    char hex_line[LINE_SIZE * 9 / 4 + 2], asc_line[LINE_SIZE + 1];
    while (len > 0) {
      memset(asc_line, ' ', sizeof(asc_line));
      memset(hex_line, ' ', sizeof(hex_line));
      size_t line_len = _min(len, LINE_SIZE);
      for (size_t i = 0; i < line_len; ++i) {
        unsigned char ch = udata[i];
        asc_line[i] = isprint(ch) ? ch : '.';
        hex_line[i*2 + i/4] = hex_encode(ch >> 4);
        hex_line[i*2 + i/4 + 1] = hex_encode(ch & 0xf);
      }
      asc_line[sizeof(asc_line)-1] = 0;
      hex_line[sizeof(hex_line)-1] = 0;
      LOG_V(level) << label << direction
                   << asc_line << " " << hex_line << " ";
      udata += line_len;
      len -= line_len;
    }
    return;
  }

  size_t consecutive_unprintable = state ? state->unprintable_count_[input] : 0;

  const unsigned char* end = udata + len;
  while (udata < end) {
    const unsigned char* line = udata;
    const unsigned char* end_of_line = strchrn<unsigned char>(udata,
                                                              end - udata,
                                                              '\n');
    if (!end_of_line) {
      udata = end_of_line = end;
    } else {
      udata = end_of_line + 1;
    }

    bool is_printable = true;

    
    
    const ptrdiff_t kMinPrintableLine = 4;
    if (consecutive_unprintable && ((end_of_line - line) < kMinPrintableLine)) {
      is_printable = false;
    } else {
      
      
      bool is_entirely_whitespace = true;
      for (const unsigned char* pos = line; pos < end_of_line; ++pos) {
        if (isspace(*pos))
          continue;
        is_entirely_whitespace = false;
        if (!isprint(*pos)) {
          is_printable = false;
          break;
        }
      }
      
      if (consecutive_unprintable && is_entirely_whitespace) {
        is_printable = false;
      }
    }
    if (!is_printable) {
      consecutive_unprintable += (udata - line);
      continue;
    }
    
    
    if (consecutive_unprintable) {
      LOG_V(level) << label << direction << "## " << consecutive_unprintable
                  << " consecutive unprintable ##";
      consecutive_unprintable = 0;
    }
    
    while ((end_of_line > line) && isspace(*(end_of_line-1))) {
      --end_of_line;
    }
    
    std::string substr(reinterpret_cast<const char*>(line), end_of_line - line);
    std::string::size_type pos_private = substr.find("Email");
    if (pos_private == std::string::npos) {
      pos_private = substr.find("Passwd");
    }
    if (pos_private == std::string::npos) {
      LOG_V(level) << label << direction << substr;
    } else {
      LOG_V(level) << label << direction << "## omitted for privacy ##";
    }
  }

  if (state) {
    state->unprintable_count_[input] = consecutive_unprintable;
  }
}

void InitDiagnosticLoggingDelegateFunction(
    void (*delegate)(const std::string&)) {
#ifndef NDEBUG
  
  base::subtle::NoBarrier_CompareAndSwap(&g_init_logging_delegate_thread_id, 0,
      static_cast<base::subtle::Atomic32>(base::PlatformThread::CurrentId()));
  DCHECK_EQ(
      g_init_logging_delegate_thread_id,
      static_cast<base::subtle::Atomic32>(base::PlatformThread::CurrentId()));
#endif
  CHECK(delegate);
  
  
  
  if (delegate == g_logging_delegate_function)
    return;
  CHECK(!g_logging_delegate_function);
#ifdef NDEBUG
  IPAddress::set_strip_sensitive(true);
#endif
  g_logging_delegate_function = delegate;

  if (g_extra_logging_init_function)
    g_extra_logging_init_function(delegate);
}

void SetExtraLoggingInit(
    void (*function)(void (*delegate)(const std::string&))) {
  CHECK(function);
  CHECK(!g_extra_logging_init_function);
  g_extra_logging_init_function = function;
}

}  
