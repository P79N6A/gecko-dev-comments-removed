





























#pragma warning( disable : 4530 )

#include "client/windows/sender/crash_report_sender.h"
#include "common/windows/http_upload.h"

namespace google_breakpad {


ReportResult CrashReportSender::SendCrashReport(
    const wstring &url, const map<wstring, wstring> &parameters,
    const wstring &dump_file_name, wstring *report_code) {

  int http_response = 0;
  bool result = HTTPUpload::SendRequest(
    url, parameters, dump_file_name, L"upload_file_minidump", report_code,
    &http_response);

  if (result) {
    return RESULT_SUCCEEDED;
  } else if (http_response == 400) {  
                                      
    return RESULT_REJECTED;
  } else {
    return RESULT_FAILED;
  }
}

}  
