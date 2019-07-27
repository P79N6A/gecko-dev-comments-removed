



#ifndef SANDBOX_SRC_FILESYSTEM_POLICY_H__
#define SANDBOX_SRC_FILESYSTEM_POLICY_H__

#include <string>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/policy_low_level.h"
#include "sandbox/win/src/sandbox_policy.h"

namespace sandbox {

enum EvalResult;


class FileSystemPolicy {
 public:
  
  
  
  
  
  static bool GenerateRules(const wchar_t* name,
                            TargetPolicy::Semantics semantics,
                            LowLevelPolicy* policy);

  
  static bool SetInitialRules(LowLevelPolicy* policy);

  
  
  
  
  
  static bool CreateFileAction(EvalResult eval_result,
                               const ClientInfo& client_info,
                               const base::string16 &file,
                               uint32 attributes,
                               uint32 desired_access,
                               uint32 file_attributes,
                               uint32 share_access,
                               uint32 create_disposition,
                               uint32 create_options,
                               HANDLE* handle,
                               NTSTATUS* nt_status,
                               ULONG_PTR* io_information);

  
  
  
  
  
  static bool OpenFileAction(EvalResult eval_result,
                             const ClientInfo& client_info,
                             const base::string16 &file,
                             uint32 attributes,
                             uint32 desired_access,
                             uint32 share_access,
                             uint32 open_options,
                             HANDLE* handle,
                             NTSTATUS* nt_status,
                             ULONG_PTR* io_information);

  
  
  static bool QueryAttributesFileAction(EvalResult eval_result,
                                        const ClientInfo& client_info,
                                        const base::string16 &file,
                                        uint32 attributes,
                                        FILE_BASIC_INFORMATION* file_info,
                                        NTSTATUS* nt_status);

  
  
  static bool QueryFullAttributesFileAction(
      EvalResult eval_result,
      const ClientInfo& client_info,
      const base::string16 &file,
      uint32 attributes,
      FILE_NETWORK_OPEN_INFORMATION* file_info,
      NTSTATUS* nt_status);

  
  
  static bool SetInformationFileAction(EvalResult eval_result,
                                       const ClientInfo& client_info,
                                       HANDLE target_file_handle,
                                       void* file_info,
                                       uint32 length,
                                       uint32 info_class,
                                       IO_STATUS_BLOCK* io_block,
                                       NTSTATUS* nt_status);
};




bool PreProcessName(const base::string16& path, base::string16* new_path);




base::string16 FixNTPrefixForMatch(const base::string16& name);

}  

#endif  
