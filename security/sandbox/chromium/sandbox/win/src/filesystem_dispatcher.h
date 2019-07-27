



#ifndef SANDBOX_SRC_FILESYSTEM_DISPATCHER_H__
#define SANDBOX_SRC_FILESYSTEM_DISPATCHER_H__

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/sandbox_policy_base.h"

namespace sandbox {


class FilesystemDispatcher : public Dispatcher {
 public:
  explicit FilesystemDispatcher(PolicyBase* policy_base);
  ~FilesystemDispatcher() {}

  
  virtual bool SetupService(InterceptionManager* manager, int service);

 private:
  
  bool NtCreateFile(IPCInfo* ipc,
                    base::string16* name,
                    uint32 attributes,
                    uint32 desired_access,
                    uint32 file_attributes,
                    uint32 share_access,
                    uint32 create_disposition,
                    uint32 create_options);

  
  bool NtOpenFile(IPCInfo* ipc,
                  base::string16* name,
                  uint32 attributes,
                  uint32 desired_access,
                  uint32 share_access,
                  uint32 create_options);

    
  
  bool NtQueryAttributesFile(IPCInfo* ipc,
                             base::string16* name,
                             uint32 attributes,
                             CountedBuffer* info);

  
  
  bool NtQueryFullAttributesFile(IPCInfo* ipc,
                                 base::string16* name,
                                 uint32 attributes,
                                 CountedBuffer* info);

  
  
  bool NtSetInformationFile(IPCInfo* ipc,
                            HANDLE handle,
                            CountedBuffer* status,
                            CountedBuffer* info,
                            uint32 length,
                            uint32 info_class);

  PolicyBase* policy_base_;
  DISALLOW_COPY_AND_ASSIGN(FilesystemDispatcher);
};

}  

#endif  
