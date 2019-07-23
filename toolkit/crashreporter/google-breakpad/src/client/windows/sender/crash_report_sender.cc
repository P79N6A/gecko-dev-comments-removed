





























#pragma warning( disable : 4530 )

#include <errno.h>

#include "client/windows/sender/crash_report_sender.h"
#include "common/windows/http_upload.h"

#if _MSC_VER < 1400  


#define fscanf_s fscanf
#endif

namespace google_breakpad {

static const char kCheckpointSignature[] = "GBP1\n";

CrashReportSender::CrashReportSender(const wstring &checkpoint_file)
    : checkpoint_file_(checkpoint_file),
      max_reports_per_day_(-1),
      last_sent_date_(-1),
      reports_sent_(0) {
  FILE *fd;
  if (OpenCheckpointFile(L"r", &fd) == 0) {
    ReadCheckpoint(fd);
    fclose(fd);
  }
}

ReportResult CrashReportSender::SendCrashReport(
    const wstring &url, const map<wstring, wstring> &parameters,
    const wstring &dump_file_name, wstring *report_code) {
  int today = GetCurrentDate();
  if (today == last_sent_date_ &&
      max_reports_per_day_ != -1 &&
      reports_sent_ >= max_reports_per_day_) {
    return RESULT_THROTTLED;
  }

  int http_response = 0;
  bool result = HTTPUpload::SendRequest(
    url, parameters, dump_file_name, L"upload_file_minidump", report_code,
    &http_response);

  if (result) {
    ReportSent(today);
    return RESULT_SUCCEEDED;
  } else if (http_response == 400) {  
                                      
    return RESULT_REJECTED;
  } else {
    return RESULT_FAILED;
  }
}

void CrashReportSender::ReadCheckpoint(FILE *fd) {
  char buf[128];
  if (!fgets(buf, sizeof(buf), fd) ||
      strcmp(buf, kCheckpointSignature) != 0) {
    return;
  }

  if (fscanf_s(fd, "%d\n", &last_sent_date_) != 1) {
    last_sent_date_ = -1;
    return;
  }
  if (fscanf_s(fd, "%d\n", &reports_sent_) != 1) {
    reports_sent_ = 0;
    return;
  }
}

void CrashReportSender::ReportSent(int today) {
  
  if (today != last_sent_date_) {
    last_sent_date_ = today;
    reports_sent_ = 0;
  }
  ++reports_sent_;

  
  FILE *fd;
  if (OpenCheckpointFile(L"w", &fd) == 0) {
    fputs(kCheckpointSignature, fd);
    fprintf(fd, "%d\n", last_sent_date_);
    fprintf(fd, "%d\n", reports_sent_);
    fclose(fd);
  }
}

int CrashReportSender::GetCurrentDate() const {
  SYSTEMTIME system_time;
  GetSystemTime(&system_time);
  return (system_time.wYear * 10000) + (system_time.wMonth * 100) +
      system_time.wDay;
}

int CrashReportSender::OpenCheckpointFile(const wchar_t *mode, FILE **fd) {
  if (checkpoint_file_.empty()) {
    return ENOENT;
  }
#if _MSC_VER >= 1400  
  return _wfopen_s(fd, checkpoint_file_.c_str(), mode);
#else
  *fd = _wfopen(checkpoint_file_.c_str(), mode);
  if (*fd == NULL) {
    return errno;
  }
  return 0;
#endif
}

}  
