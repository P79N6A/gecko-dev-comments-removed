




























#include "client/windows/crash_generation/minidump_generator.h"

#include <assert.h>
#include <avrfsdk.h>

#include <algorithm>
#include <iterator>
#include <list>
#include <vector>

#include "client/windows/common/auto_critical_section.h"
#include "common/windows/guid_string.h"

using std::wstring;

namespace {




class HandleTraceData {
 public:
  HandleTraceData();
  ~HandleTraceData();

  
  
  bool CollectHandleData(HANDLE process_handle,
                         EXCEPTION_POINTERS* exception_pointers);

  
  
  
  bool GetUserStream(MINIDUMP_USER_STREAM* user_stream);

 private:
  
  
  static bool ReadExceptionCode(HANDLE process_handle,
                                EXCEPTION_POINTERS* exception_pointers,
                                DWORD* exception_code);

  
  static ULONG CALLBACK RecordHandleOperations(void* resource_description,
                                               void* enumeration_context,
                                               ULONG* enumeration_level);

  
  
  typedef BOOL (WINAPI* VerifierEnumerateResourceType)(
      HANDLE Process,
      ULONG Flags,
      ULONG ResourceType,
      AVRF_RESOURCE_ENUMERATE_CALLBACK ResourceCallback,
      PVOID EnumerationContext);

  
  HMODULE verifier_module_;

  
  VerifierEnumerateResourceType enumerate_resource_;

  
  ULONG64 handle_;

  
  std::list<AVRF_HANDLE_OPERATION> operations_;

  
  std::vector<char> stream_;
};

HandleTraceData::HandleTraceData()
    : verifier_module_(NULL),
      enumerate_resource_(NULL),
      handle_(NULL) {
}

HandleTraceData::~HandleTraceData() {
  if (verifier_module_) {
    FreeLibrary(verifier_module_);
  }
}

bool HandleTraceData::CollectHandleData(
    HANDLE process_handle,
    EXCEPTION_POINTERS* exception_pointers) {
  DWORD exception_code;
  if (!ReadExceptionCode(process_handle, exception_pointers, &exception_code)) {
    return false;
  }

  
  
  
  if (exception_code != STATUS_INVALID_HANDLE) {
    return true;
  }

  
  verifier_module_ = LoadLibrary(TEXT("verifier.dll"));
  if (!verifier_module_) {
    return false;
  }

  enumerate_resource_ = reinterpret_cast<VerifierEnumerateResourceType>(
      GetProcAddress(verifier_module_, "VerifierEnumerateResource"));
  if (!enumerate_resource_) {
    return false;
  }

  
  
  
  
  if (enumerate_resource_(process_handle,
                          0,
                          AvrfResourceHandleTrace,
                          &RecordHandleOperations,
                          this) != ERROR_SUCCESS) {
    
    return true;
  }

  
  std::list<AVRF_HANDLE_OPERATION>::iterator i = operations_.begin();
  std::list<AVRF_HANDLE_OPERATION>::iterator i_end = operations_.end();
  while (i != i_end) {
    if (i->Handle == handle_) {
      ++i;
    } else {
      i = operations_.erase(i);
    }
  }

  
  stream_.resize(sizeof(MINIDUMP_HANDLE_OPERATION_LIST) +
      sizeof(AVRF_HANDLE_OPERATION) * operations_.size());

  MINIDUMP_HANDLE_OPERATION_LIST* stream_data =
      reinterpret_cast<MINIDUMP_HANDLE_OPERATION_LIST*>(
          &stream_.front());
  stream_data->SizeOfHeader = sizeof(MINIDUMP_HANDLE_OPERATION_LIST);
  stream_data->SizeOfEntry = sizeof(AVRF_HANDLE_OPERATION);
  stream_data->NumberOfEntries = static_cast<ULONG32>(operations_.size());
  stream_data->Reserved = 0;
  std::copy(operations_.begin(),
            operations_.end(),
#ifdef _MSC_VER
            stdext::checked_array_iterator<AVRF_HANDLE_OPERATION*>(
                reinterpret_cast<AVRF_HANDLE_OPERATION*>(stream_data + 1),
                operations_.size())
#else
            reinterpret_cast<AVRF_HANDLE_OPERATION*>(stream_data + 1)
#endif
            );

  return true;
}

bool HandleTraceData::GetUserStream(MINIDUMP_USER_STREAM* user_stream) {
  if (stream_.empty()) {
    return false;
  } else {
    user_stream->Type = HandleOperationListStream;
    user_stream->BufferSize = static_cast<ULONG>(stream_.size());
    user_stream->Buffer = &stream_.front();
    return true;
  }
}

bool HandleTraceData::ReadExceptionCode(
    HANDLE process_handle,
    EXCEPTION_POINTERS* exception_pointers,
    DWORD* exception_code) {
  EXCEPTION_POINTERS pointers;
  if (!ReadProcessMemory(process_handle,
                         exception_pointers,
                         &pointers,
                         sizeof(pointers),
                         NULL)) {
    return false;
  }

  if (!ReadProcessMemory(process_handle,
                         pointers.ExceptionRecord,
                         exception_code,
                         sizeof(*exception_code),
                         NULL)) {
    return false;
  }

  return true;
}

ULONG CALLBACK HandleTraceData::RecordHandleOperations(
    void* resource_description,
    void* enumeration_context,
    ULONG* enumeration_level) {
  AVRF_HANDLE_OPERATION* description =
      reinterpret_cast<AVRF_HANDLE_OPERATION*>(resource_description);
  HandleTraceData* self =
      reinterpret_cast<HandleTraceData*>(enumeration_context);

  
  if (description->OperationType == OperationDbBADREF) {
    self->handle_ = description->Handle;
  }

  
  self->operations_.push_back(*description);

  *enumeration_level = HeapEnumerationEverything;
  return ERROR_SUCCESS;
}

}  

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
  
  return this->WriteMinidump(process_handle, process_id, thread_id,
                             requesting_thread_id, exception_pointers,
                             assert_info, dump_type, is_client_pointers,
                             dump_path, NULL);
}

bool MinidumpGenerator::WriteMinidump(HANDLE process_handle,
                                      DWORD process_id,
                                      DWORD thread_id,
                                      DWORD requesting_thread_id,
                                      EXCEPTION_POINTERS* exception_pointers,
                                      MDRawAssertionInfo* assert_info,
                                      MINIDUMP_TYPE dump_type,
                                      bool is_client_pointers,
                                      wstring* dump_path,
                                      wstring* full_dump_path) {
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

  
  
  MINIDUMP_USER_STREAM user_stream_array[3];
  user_stream_array[0].Type = MD_BREAKPAD_INFO_STREAM;
  user_stream_array[0].BufferSize = sizeof(breakpad_info);
  user_stream_array[0].Buffer = &breakpad_info;

  MINIDUMP_USER_STREAM_INFORMATION user_streams;
  user_streams.UserStreamCount = 1;
  user_streams.UserStreamArray = user_stream_array;

  MDRawAssertionInfo* actual_assert_info = assert_info;
  MDRawAssertionInfo client_assert_info = {{0}};

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

  
  
  
  HandleTraceData handle_trace_data;
  if (exception_pointers && (dump_type & MiniDumpWithHandleData) == 0) {
    if (!handle_trace_data.CollectHandleData(process_handle,
                                             exception_pointers)) {
      CloseHandle(dump_file);
      if (full_dump_file != INVALID_HANDLE_VALUE)
        CloseHandle(full_dump_file);
      return false;
    }
  }

  bool result_full_memory = true;
  if (full_memory_dump) {
    result_full_memory = write_dump(
        process_handle,
        process_id,
        full_dump_file,
        static_cast<MINIDUMP_TYPE>((dump_type & (~MiniDumpNormal))
                                    | MiniDumpWithHandleData),
        exception_pointers ? &dump_exception_info : NULL,
        &user_streams,
        NULL) != FALSE;
  }

  
  if (handle_trace_data.GetUserStream(
          &user_stream_array[user_streams.UserStreamCount])) {
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

  bool result = result_minidump && result_full_memory;

  CloseHandle(dump_file);
  if (full_dump_file != INVALID_HANDLE_VALUE)
    CloseHandle(full_dump_file);

  
  
  if (result && dump_path) {
    *dump_path = dump_file_path;
  }
  if (result && full_memory_dump && full_dump_path) {
    *full_dump_path = full_dump_file_path;
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
  if (!create_uuid) {
    return false;
  }

  create_uuid(&id);
  wstring id_str = GUIDString::GUIDToWString(&id);

  *file_path = dump_path_ + TEXT("\\") + id_str + TEXT(".dmp");
  return true;
}

}  
