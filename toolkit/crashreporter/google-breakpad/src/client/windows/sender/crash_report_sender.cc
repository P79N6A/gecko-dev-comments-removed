




























#pragma warning( disable : 4530 )
#include "client/windows/sender/crash_report_sender.h"
#include "common/windows/http_upload.h"

namespace google_airbag {


bool CrashReportSender::SendCrashReport(
    const wstring &url, const map<wstring, wstring> &parameters,
    const wstring &dump_file_name) {

  return HTTPUpload::SendRequest(url, parameters, dump_file_name,
                                 L"upload_file_minidump");
}

}  
