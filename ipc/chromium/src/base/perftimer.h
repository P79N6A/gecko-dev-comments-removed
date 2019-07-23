



#ifndef BASE_PERFTIMER_H_
#define BASE_PERFTIMER_H_

#include <string>
#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/time.h"






bool InitPerfLog(const FilePath& log_path);
void FinalizePerfLog();






void LogPerfResult(const char* test_name, double value, const char* units);





class PerfTimer {
 public:
  PerfTimer() {
    begin_ = base::TimeTicks::Now();
  }

  
  base::TimeDelta Elapsed() const {
    return base::TimeTicks::Now() - begin_;
  }

 private:
  base::TimeTicks begin_;
};









class PerfTimeLogger {
 public:
  explicit PerfTimeLogger(const char* test_name)
      : logged_(false),
        test_name_(test_name) {
  }

  ~PerfTimeLogger() {
    if (!logged_)
      Done();
  }

  void Done() {
    
    
    
    LogPerfResult(test_name_.c_str(), timer_.Elapsed().InMillisecondsF(), "ms");
    logged_ = true;
  }

 private:
  bool logged_;
  std::string test_name_;
  PerfTimer timer_;
};

#endif  
