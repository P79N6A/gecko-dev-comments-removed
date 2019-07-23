




























#ifndef CLIENT_WINDOWS_SENDER_CRASH_REPORT_SENDER_H__
#define CLIENT_WINDOWS_SENDER_CRASH_REPORT_SENDER_H__









#pragma warning( push )

#pragma warning( disable : 4530 ) 

#include <map>
#include <string>

namespace google_breakpad {

using std::wstring;
using std::map;

typedef enum {
  RESULT_FAILED = 0,  
  RESULT_REJECTED,    
                      
  RESULT_SUCCEEDED    
} ReportResult;

class CrashReportSender {
 public:
  
  
  
  
  
  
  
  
  
  
  static ReportResult SendCrashReport(const wstring &url,
                                      const map<wstring, wstring> &parameters,
                                      const wstring &dump_file_name,
                                      wstring *report_code);

 private:
  
  
  CrashReportSender();
  explicit CrashReportSender(const CrashReportSender &);
  void operator=(const CrashReportSender &);
  ~CrashReportSender();
};

}  

#pragma warning( pop )

#endif  
