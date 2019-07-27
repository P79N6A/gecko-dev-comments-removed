



#ifndef SANDBOX_SRC_JOB_H_
#define SANDBOX_SRC_JOB_H_

#include "base/basictypes.h"
#include "sandbox/win/src/restricted_token_utils.h"

namespace sandbox {






class Job {
 public:
  Job() : job_handle_(NULL) { }

  ~Job();

  
  
  
  
  
  
  
  
  DWORD Init(JobLevel security_level, wchar_t *job_name, DWORD ui_exceptions);

  
  
  
  
  DWORD AssignProcessToJob(HANDLE process_handle);

  
  
  
  
  
  DWORD UserHandleGrantAccess(HANDLE handle);

  
  
  
  HANDLE Detach();

 private:
  
  HANDLE job_handle_;

  DISALLOW_COPY_AND_ASSIGN(Job);
};

}  


#endif  
