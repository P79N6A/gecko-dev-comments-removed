




























#ifndef CLIENT_WINDOWS_SENDER_CRASH_REPORT_SENDER_H__
#define CLIENT_WINDOWS_SENDER_CRASH_REPORT_SENDER_H__









#pragma warning( push )

#pragma warning( disable : 4530 ) 

#include <map>
#include <string>

namespace google_airbag {

using std::wstring;
using std::map;

class CrashReportSender {
 public:
  
  
  
  
  
  
  static bool SendCrashReport(const wstring &url,
                              const map<wstring, wstring> &parameters,
                              const wstring &dump_file_name);

 private:
  
  
  CrashReportSender();
  explicit CrashReportSender(const CrashReportSender &);
  void operator=(const CrashReportSender &);
  ~CrashReportSender();
};

}  

#pragma warning( pop )
#endif  
