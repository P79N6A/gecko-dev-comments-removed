




























#include "client/windows/crash_generation/minidump_generator.h"
#include <cassert>
#include "client/windows/common/auto_critical_section.h"
#include "common/windows/guid_string.h"

using std::wstring;

namespace google_breakpad {

MinidumpGenerator::MinidumpGenerator(const wstring& dump_path)
    : dbghelp_module_(NULL),
      rpcrt4_module_(NULL),
      dump_path_(dump_path),
      write_dump_(NULL),
      create_uuid_(NULL) {
  InitializeCriticalSection(&module_load_sync_);
  InitializeCriticalSection(&get_proc_address_sync_);
}

MinidumpGenerator::~MinidumpGenerator() {
  if (dbghelp_module_) {
    FreeLibrary(dbghelp_module_);
  }

  if (rpcrt4_module_) {
    FreeLibrary(rpcrt4_module_);
  }

  DeleteCriticalSection(&get_proc_address_sync_);
  DeleteCriticalSection(&module_load_sync_);
}

bool MinidumpGenerator::WriteMinidump(HANDLE process_handle,
                                      DWORD process_id,
                                      DWORD thread_id,
                                      DWORD requesting_thread_id,
                                      EXCEPTION_POINTERS* exception_pointers,
                                      MDRawAssertionInfo* assert_info,
                                      MINIDUMP_TYPE dump_type,
                                      bool is_client_pointers,
                                      wstring* dump_path) {
  MiniDumpWriteDumpType write_dump = GetWriteDump();
  if (!write_dump) {
    return false;
  }

  wstring dump_file_path;
  if (!GenerateDumpFilePath(&dump_file_path)) {
    return false;
  }

  
  
  
  bool full_memory_dump = (dump_type & MiniDumpWithFullMemory) != 0;
  wstring full_dump_file_path;
  if (full_memory_dump) {
    full_dump_file_path.assign(dump_file_path);
    full_dump_file_path.resize(full_dump_file_path.size() - 4);  
    full_dump_file_path.append(TEXT("-full.dmp"));
  }

  HANDLE dump_file = CreateFile(dump_file_path.c_str(),
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

  if (dump_file == INVALID_HANDLE_VALUE) {
    return false;
  }

  HANDLE full_dump_file = INVALID_HANDLE_VALUE;
  if (full_memory_dump) {
    full_dump_file = CreateFile(full_dump_file_path.c_str(),
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (full_dump_file == INVALID_HANDLE_VALUE) {
      CloseHandle(dump_file);
      return false;
    }
  }

  MINIDUMP_EXCEPTION_INFORMATION* dump_exception_pointers = NULL;
  MINIDUMP_EXCEPTION_INFORMATION dump_exception_info;

  
  
  if (exception_pointers) {
    dump_exception_pointers = &dump_exception_info;
    dump_exception_info.ThreadId = thread_id;
    dump_exception_info.ExceptionPointers = exception_pointers;
    dump_exception_info.ClientPointers = is_client_pointers;
  }

  
  
  
  
  
  
  MDRawBreakpadInfo breakpad_info = {0};
  if (!is_client_pointers) {
    
    
    breakpad_info.validity = MD_BREAKPAD_INFO_VALID_DUMP_THREAD_ID |
                             MD_BREAKPAD_INFO_VALID_REQUESTING_THREAD_ID;
    breakpad_info.dump_thread_id = thread_id;
    breakpad_info.requesting_thread_id = requesting_thread_id;
  }

  
  MINIDUMP_USER_STREAM user_stream_array[2];
  user_stream_array[0].Type = MD_BREAKPAD_INFO_STREAM;
  user_stream_array[0].BufferSize = sizeof(breakpad_info);
  user_stream_array[0].Buffer = &breakpad_info;

  MINIDUMP_USER_STREAM_INFORMATION user_streams;
  user_streams.UserStreamCount = 1;
  user_streams.UserStreamArray = user_stream_array;

  MDRawAssertionInfo* actual_assert_info = assert_info;
  MDRawAssertionInfo client_assert_info = {0};

  if (assert_info) {
    
    
    if (is_client_pointers) {
      SIZE_T bytes_read = 0;
      if (!ReadProcessMemory(process_handle,
                             assert_info,
                             &client_assert_info,
                             sizeof(client_assert_info),
                             &bytes_read)) {
        CloseHandle(dump_file);
        if (full_dump_file != INVALID_HANDLE_VALUE)
          CloseHandle(full_dump_file);
        return false;
      }

      if (bytes_read != sizeof(client_assert_info)) {
        CloseHandle(dump_file);
        if (full_dump_file != INVALID_HANDLE_VALUE)
          CloseHandle(full_dump_file);
        return false;
      }

      actual_assert_info  = &client_assert_info;
    }

    user_stream_array[1].Type = MD_ASSERTION_INFO_STREAM;
    user_stream_array[1].BufferSize = sizeof(MDRawAssertionInfo);
    user_stream_array[1].Buffer = actual_assert_info;
    ++user_streams.UserStreamCount;
  }

  bool result_minidump = write_dump(
      process_handle,
      process_id,
      dump_file,
      static_cast<MINIDUMP_TYPE>((dump_type & (~MiniDumpWithFullMemory))
                                  | MiniDumpNormal),
      exception_pointers ? &dump_exception_info : NULL,
      &user_streams,
      NULL) != FALSE;

  bool result_full_memory = true;
  if (full_memory_dump) {
    result_full_memory = write_dump(
        process_handle,
        process_id,
        full_dump_file,
        static_cast<MINIDUMP_TYPE>(dump_type & (~MiniDumpNormal)),
        exception_pointers ? &dump_exception_info : NULL,
        &user_streams,
        NULL) != FALSE;
  }

  bool result = result_minidump && result_full_memory;

  CloseHandle(dump_file);
  if (full_dump_file != INVALID_HANDLE_VALUE)
    CloseHandle(full_dump_file);

  
  
  if (result && dump_path) {
    *dump_path = dump_file_path;
  }

  return result;
}

HMODULE MinidumpGenerator::GetDbghelpModule() {
  AutoCriticalSection lock(&module_load_sync_);
  if (!dbghelp_module_) {
    dbghelp_module_ = LoadLibrary(TEXT("dbghelp.dll"));
  }

  return dbghelp_module_;
}

MinidumpGenerator::MiniDumpWriteDumpType MinidumpGenerator::GetWriteDump() {
  AutoCriticalSection lock(&get_proc_address_sync_);
  if (!write_dump_) {
    HMODULE module = GetDbghelpModule();
    if (module) {
      FARPROC proc = GetProcAddress(module, "MiniDumpWriteDump");
      write_dump_ = reinterpret_cast<MiniDumpWriteDumpType>(proc);
    }
  }

  return write_dump_;
}

HMODULE MinidumpGenerator::GetRpcrt4Module() {
  AutoCriticalSection lock(&module_load_sync_);
  if (!rpcrt4_module_) {
    rpcrt4_module_ = LoadLibrary(TEXT("rpcrt4.dll"));
  }

  return rpcrt4_module_;
}

MinidumpGenerator::UuidCreateType MinidumpGenerator::GetCreateUuid() {
  AutoCriticalSection lock(&module_load_sync_);
  if (!create_uuid_) {
    HMODULE module = GetRpcrt4Module();
    if (module) {
      FARPROC proc = GetProcAddress(module, "UuidCreate");
      create_uuid_ = reinterpret_cast<UuidCreateType>(proc);
    }
  }

  return create_uuid_;
}

bool MinidumpGenerator::GenerateDumpFilePath(wstring* file_path) {
  UUID id = {0};

  UuidCreateType create_uuid = GetCreateUuid();
  if(!create_uuid) {
    return false;
  }

  create_uuid(&id);
  wstring id_str = GUIDString::GUIDToWString(&id);

  *file_path = dump_path_ + TEXT("\\") + id_str + TEXT(".dmp");
  return true;
}

}  
