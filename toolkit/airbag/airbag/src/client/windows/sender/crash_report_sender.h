




























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
                      
  RESULT_SUCCEEDED,   
  RESULT_THROTTLED    
                      
} ReportResult;

class CrashReportSender {
 public:
  
  
  
  
  explicit CrashReportSender(const wstring &checkpoint_file);
  ~CrashReportSender() {}

  
  
  
  void set_max_reports_per_day(int reports) {
    max_reports_per_day_ = reports;
  }

  int max_reports_per_day() const { return max_reports_per_day_; }

  
  
  
  
  
  
  
  
  
  
  ReportResult SendCrashReport(const wstring &url,
                               const map<wstring, wstring> &parameters,
                               const wstring &dump_file_name,
                               wstring *report_code);

 private:
  
  void ReadCheckpoint(FILE *fd);

  
  void ReportSent(int today);

  
  int GetCurrentDate() const;

  
  
  int OpenCheckpointFile(const wchar_t *mode, FILE **fd);

  wstring checkpoint_file_;
  int max_reports_per_day_;
  
  int last_sent_date_;
  
  int reports_sent_;

  
  explicit CrashReportSender(const CrashReportSender &);
  void operator=(const CrashReportSender &);
};

}  

#pragma warning( pop )

#endif  
