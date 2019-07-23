



#ifndef CHROME_COMMON_IMPORTANT_FILE_WRITER_H_
#define CHROME_COMMON_IMPORTANT_FILE_WRITER_H_

#include <string>

#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/non_thread_safe.h"
#include "base/time.h"
#include "base/timer.h"

namespace base {
class Thread;
}

















class ImportantFileWriter : public NonThreadSafe {
 public:
  
  
  
  
  
  ImportantFileWriter(const FilePath& path, const base::Thread* backend_thread);

  
  ~ImportantFileWriter();

  FilePath path() const { return path_; }

  
  
  void WriteNow(const std::string& data);

  
  
  
  
  void ScheduleWrite(const std::string& data);

  base::TimeDelta commit_interval() const {
    return commit_interval_;
  }

  void set_commit_interval(const base::TimeDelta& interval) {
    commit_interval_ = interval;
  }

 private:
  
  void CommitPendingData();

  
  const FilePath path_;

  
  const base::Thread* backend_thread_;

  
  base::OneShotTimer<ImportantFileWriter> timer_;

  
  std::string data_;

  
  base::TimeDelta commit_interval_;

  DISALLOW_COPY_AND_ASSIGN(ImportantFileWriter);
};

#endif  
