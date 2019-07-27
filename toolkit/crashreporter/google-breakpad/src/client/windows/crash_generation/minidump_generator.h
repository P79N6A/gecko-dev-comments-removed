




























#ifndef CLIENT_WINDOWS_CRASH_GENERATION_MINIDUMP_GENERATOR_H_
#define CLIENT_WINDOWS_CRASH_GENERATION_MINIDUMP_GENERATOR_H_

#include <windows.h>
#include <dbghelp.h>
#include <rpc.h>
#include <list>
#include <string>
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {





class MinidumpGenerator {
 public:
  
  explicit MinidumpGenerator(const std::wstring& dump_path);

  ~MinidumpGenerator();

  
  
  
  bool WriteMinidump(HANDLE process_handle,
                     DWORD process_id,
                     DWORD thread_id,
                     DWORD requesting_thread_id,
                     EXCEPTION_POINTERS* exception_pointers,
                     MDRawAssertionInfo* assert_info,
                     MINIDUMP_TYPE dump_type,
                     bool is_client_pointers,
                     std::wstring* dump_path);

  
  
  
  bool WriteMinidump(HANDLE process_handle,
                     DWORD process_id,
                     DWORD thread_id,
                     DWORD requesting_thread_id,
                     EXCEPTION_POINTERS* exception_pointers,
                     MDRawAssertionInfo* assert_info,
                     MINIDUMP_TYPE dump_type,
                     bool is_client_pointers,
                     std::wstring* dump_path,
                     std::wstring* full_dump_path);

 private:
  
  
  typedef BOOL (WINAPI* MiniDumpWriteDumpType)(
      HANDLE hProcess,
      DWORD ProcessId,
      HANDLE hFile,
      MINIDUMP_TYPE DumpType,
      CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
      CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
      CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

  
  typedef RPC_STATUS (RPC_ENTRY* UuidCreateType)(UUID* Uuid);

  
  HMODULE GetDbghelpModule();

  
  
  MiniDumpWriteDumpType GetWriteDump();

  
  HMODULE GetRpcrt4Module();

  
  
  UuidCreateType GetCreateUuid();

  
  bool GenerateDumpFilePath(std::wstring* file_path);

  
  HMODULE dbghelp_module_;

  
  MiniDumpWriteDumpType write_dump_;

  
  HMODULE rpcrt4_module_;

  
  UuidCreateType create_uuid_;

  
  std::wstring dump_path_;

  
  CRITICAL_SECTION module_load_sync_;

  
  
  CRITICAL_SECTION get_proc_address_sync_;
};

}  

#endif  
