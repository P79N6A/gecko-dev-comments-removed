



#ifndef SANDBOX_SRC_FILESYSTEM_DISPATCHER_H__
#define SANDBOX_SRC_FILESYSTEM_DISPATCHER_H__

#include "base/basictypes.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/sandbox_policy_base.h"

namespace sandbox {


class FilesystemDispatcher : public Dispatcher {
 public:
  explicit FilesystemDispatcher(PolicyBase* policy_base);
  ~FilesystemDispatcher() {}

  
  virtual bool SetupService(InterceptionManager* manager, int service);

 private:
  
  bool NtCreateFile(IPCInfo* ipc, std::wstring* name, DWORD attributes,
                    DWORD desired_access, DWORD file_attributes,
                    DWORD share_access, DWORD create_disposition,
                    DWORD create_options);

  
  bool NtOpenFile(IPCInfo* ipc, std::wstring* name, DWORD attributes,
                  DWORD desired_access, DWORD share_access,
                  DWORD create_options);

    
  
  bool NtQueryAttributesFile(IPCInfo* ipc, std::wstring* name, DWORD attributes,
                             CountedBuffer* info);

  
  
  bool NtQueryFullAttributesFile(IPCInfo* ipc, std::wstring* name,
                                 DWORD attributes, CountedBuffer* info);

  
  
  bool NtSetInformationFile(IPCInfo* ipc, HANDLE handle, CountedBuffer* status,
                            CountedBuffer* info, DWORD length,
                            DWORD info_class);

  PolicyBase* policy_base_;
  DISALLOW_COPY_AND_ASSIGN(FilesystemDispatcher);
};

}  

#endif  
