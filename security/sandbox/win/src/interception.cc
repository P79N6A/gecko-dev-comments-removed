






#include <set>

#include "sandbox/win/src/interception.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/win/pe_image.h"
#include "base/win/windows_version.h"
#include "sandbox/win/src/interception_internal.h"
#include "sandbox/win/src/interceptors.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_utils.h"
#include "sandbox/win/src/service_resolver.h"
#include "sandbox/win/src/target_interceptions.h"
#include "sandbox/win/src/target_process.h"
#include "sandbox/win/src/wow64.h"

namespace {

const char kMapViewOfSectionName[] = "NtMapViewOfSection";
const char kUnmapViewOfSectionName[] = "NtUnmapViewOfSection";


const size_t kAllocGranularity = 65536;
const size_t kPageSize = 4096;


size_t GetGranularAlignedRandomOffset(size_t size) {
  CHECK_LE(size, kAllocGranularity);
  unsigned int offset;

  do {
    rand_s(&offset);
    offset &= (kAllocGranularity - 1);
  } while (offset > (kAllocGranularity - size));

  
  size_t align_size = kPageSize;
  for (size_t new_size = align_size / 2;  new_size >= size; new_size /= 2) {
    align_size = new_size;
  }
  return offset & ~(align_size - 1);
}

}  

namespace sandbox {

SANDBOX_INTERCEPT SharedMemory* g_interceptions;


SANDBOX_INTERCEPT OriginalFunctions g_originals = { NULL };


const char kUnloadDLLDummyFunction[] = "@";

InterceptionManager::InterceptionManager(TargetProcess* child_process,
                                         bool relaxed)
    : child_(child_process), names_used_(false), relaxed_(relaxed) {
  child_->AddRef();
}
InterceptionManager::~InterceptionManager() {
  child_->Release();
}

bool InterceptionManager::AddToPatchedFunctions(
    const wchar_t* dll_name, const char* function_name,
    InterceptionType interception_type, const void* replacement_code_address,
    InterceptorId id) {
  InterceptionData function;
  function.type = interception_type;
  function.id = id;
  function.dll = dll_name;
  function.function = function_name;
  function.interceptor_address = replacement_code_address;

  interceptions_.push_back(function);
  return true;
}

bool InterceptionManager::AddToPatchedFunctions(
    const wchar_t* dll_name, const char* function_name,
    InterceptionType interception_type, const char* replacement_function_name,
    InterceptorId id) {
  InterceptionData function;
  function.type = interception_type;
  function.id = id;
  function.dll = dll_name;
  function.function = function_name;
  function.interceptor = replacement_function_name;
  function.interceptor_address = NULL;

  interceptions_.push_back(function);
  names_used_ = true;
  return true;
}

bool InterceptionManager::AddToUnloadModules(const wchar_t* dll_name) {
  InterceptionData module_to_unload;
  module_to_unload.type = INTERCEPTION_UNLOAD_MODULE;
  module_to_unload.dll = dll_name;
  
  
  module_to_unload.function = kUnloadDLLDummyFunction;
  module_to_unload.interceptor_address = reinterpret_cast<void*>(1);

  interceptions_.push_back(module_to_unload);
  return true;
}

bool InterceptionManager::InitializeInterceptions() {
  if (interceptions_.empty())
    return true;  

  size_t buffer_bytes = GetBufferSize();
  scoped_ptr<char[]> local_buffer(new char[buffer_bytes]);

  if (!SetupConfigBuffer(local_buffer.get(), buffer_bytes))
    return false;

  void* remote_buffer;
  if (!CopyDataToChild(local_buffer.get(), buffer_bytes, &remote_buffer))
    return false;

  bool hot_patch_needed = (0 != buffer_bytes);
  if (!PatchNtdll(hot_patch_needed))
    return false;

  g_interceptions = reinterpret_cast<SharedMemory*>(remote_buffer);
  ResultCode rc = child_->TransferVariable("g_interceptions",
                                           &g_interceptions,
                                           sizeof(g_interceptions));
  return (SBOX_ALL_OK == rc);
}

size_t InterceptionManager::GetBufferSize() const {
  std::set<std::wstring> dlls;
  size_t buffer_bytes = 0;

  std::list<InterceptionData>::const_iterator it = interceptions_.begin();
  for (; it != interceptions_.end(); ++it) {
    
    if (!IsInterceptionPerformedByChild(*it))
      continue;

    if (!dlls.count(it->dll)) {
      
      size_t dll_name_bytes = (it->dll.size() + 1) * sizeof(wchar_t);

      
      buffer_bytes += RoundUpToMultiple(offsetof(DllPatchInfo, dll_name) +
                                            dll_name_bytes, sizeof(size_t));
      dlls.insert(it->dll);
    }

    
    size_t strings_chars = it->function.size() + it->interceptor.size() + 2;

    
    size_t record_bytes = offsetof(FunctionInfo, function) + strings_chars;
    record_bytes = RoundUpToMultiple(record_bytes, sizeof(size_t));
    buffer_bytes += record_bytes;
  }

  if (0 != buffer_bytes)
    
    buffer_bytes += offsetof(SharedMemory, dll_list);

  return buffer_bytes;
}




bool InterceptionManager::SetupConfigBuffer(void* buffer, size_t buffer_bytes) {
  if (0 == buffer_bytes)
    return true;

  DCHECK(buffer_bytes > sizeof(SharedMemory));

  SharedMemory* shared_memory = reinterpret_cast<SharedMemory*>(buffer);
  DllPatchInfo* dll_info = shared_memory->dll_list;
  int num_dlls = 0;

  shared_memory->interceptor_base = names_used_ ? child_->MainModule() : NULL;

  buffer_bytes -= offsetof(SharedMemory, dll_list);
  buffer = dll_info;

  std::list<InterceptionData>::iterator it = interceptions_.begin();
  for (; it != interceptions_.end();) {
    
    if (!IsInterceptionPerformedByChild(*it)) {
      ++it;
      continue;
    }

    const std::wstring dll = it->dll;
    if (!SetupDllInfo(*it, &buffer, &buffer_bytes))
      return false;

    
    
    
    std::list<InterceptionData>::iterator rest = it;
    for (; rest != interceptions_.end();) {
      if (rest->dll == dll) {
        if (!SetupInterceptionInfo(*rest, &buffer, &buffer_bytes, dll_info))
          return false;
        if (it == rest)
          ++it;
        rest = interceptions_.erase(rest);
      } else {
        ++rest;
      }
    }
    dll_info = reinterpret_cast<DllPatchInfo*>(buffer);
    ++num_dlls;
  }

  shared_memory->num_intercepted_dlls = num_dlls;
  return true;
}



bool InterceptionManager::SetupDllInfo(const InterceptionData& data,
                                       void** buffer,
                                       size_t* buffer_bytes) const {
  DCHECK(buffer_bytes);
  DCHECK(buffer);
  DCHECK(*buffer);

  DllPatchInfo* dll_info = reinterpret_cast<DllPatchInfo*>(*buffer);

  
  size_t required = offsetof(DllPatchInfo, dll_name) +
                    (data.dll.size() + 1) * sizeof(wchar_t);
  required = RoundUpToMultiple(required, sizeof(size_t));
  if (*buffer_bytes < required)
    return false;

  *buffer_bytes -= required;
  *buffer = reinterpret_cast<char*>(*buffer) + required;

  
  dll_info->unload_module = (data.type == INTERCEPTION_UNLOAD_MODULE);
  dll_info->record_bytes = required;
  dll_info->offset_to_functions = required;
  dll_info->num_functions = 0;
  data.dll._Copy_s(dll_info->dll_name, data.dll.size(), data.dll.size());
  dll_info->dll_name[data.dll.size()] = L'\0';

  return true;
}

bool InterceptionManager::SetupInterceptionInfo(const InterceptionData& data,
                                                void** buffer,
                                                size_t* buffer_bytes,
                                                DllPatchInfo* dll_info) const {
  DCHECK(buffer_bytes);
  DCHECK(buffer);
  DCHECK(*buffer);

  if ((dll_info->unload_module) &&
      (data.function != kUnloadDLLDummyFunction)) {
    
    NOTREACHED();
  }

  FunctionInfo* function = reinterpret_cast<FunctionInfo*>(*buffer);

  size_t name_bytes = data.function.size();
  size_t interceptor_bytes = data.interceptor.size();

  
  size_t required = offsetof(FunctionInfo, function) +
                    name_bytes + interceptor_bytes + 2;
  required = RoundUpToMultiple(required, sizeof(size_t));
  if (*buffer_bytes < required)
    return false;

  
  *buffer_bytes -= required;
  *buffer = reinterpret_cast<char*>(*buffer) + required;

  function->record_bytes = required;
  function->type = data.type;
  function->id = data.id;
  function->interceptor_address = data.interceptor_address;
  char* names = function->function;

  data.function._Copy_s(names, name_bytes, name_bytes);
  names += name_bytes;
  *names++ = '\0';

  
  data.interceptor._Copy_s(names, interceptor_bytes, interceptor_bytes);
  names += interceptor_bytes;
  *names++ = '\0';

  
  dll_info->num_functions++;
  dll_info->record_bytes += required;

  return true;
}

bool InterceptionManager::CopyDataToChild(const void* local_buffer,
                                          size_t buffer_bytes,
                                          void** remote_buffer) const {
  DCHECK(NULL != remote_buffer);
  if (0 == buffer_bytes) {
    *remote_buffer = NULL;
    return true;
  }

  HANDLE child = child_->Process();

  
  void* remote_data = ::VirtualAllocEx(child, NULL, buffer_bytes,
                                       MEM_COMMIT, PAGE_READWRITE);
  if (NULL == remote_data)
    return false;

  SIZE_T bytes_written;
  BOOL success = ::WriteProcessMemory(child, remote_data, local_buffer,
                                      buffer_bytes, &bytes_written);
  if (FALSE == success || bytes_written != buffer_bytes) {
    ::VirtualFreeEx(child, remote_data, 0, MEM_RELEASE);
    return false;
  }

  *remote_buffer = remote_data;

  return true;
}


bool InterceptionManager::IsInterceptionPerformedByChild(
    const InterceptionData& data) const {
  if (INTERCEPTION_INVALID == data.type)
    return false;

  if (INTERCEPTION_SERVICE_CALL == data.type)
    return false;

  if (data.type >= INTERCEPTION_LAST)
    return false;

  std::wstring ntdll(kNtdllName);
  if (ntdll == data.dll)
    return false;  

  return true;
}

bool InterceptionManager::PatchNtdll(bool hot_patch_needed) {
  
  if (!hot_patch_needed && interceptions_.empty())
    return true;

  if (hot_patch_needed) {
#if SANDBOX_EXPORTS
    
#if defined(_WIN64)
    #pragma comment(linker, "/include:TargetNtMapViewOfSection64")
    #pragma comment(linker, "/include:TargetNtUnmapViewOfSection64")
#else
    #pragma comment(linker, "/include:_TargetNtMapViewOfSection@44")
    #pragma comment(linker, "/include:_TargetNtUnmapViewOfSection@12")
#endif
#endif
    ADD_NT_INTERCEPTION(NtMapViewOfSection, MAP_VIEW_OF_SECTION_ID, 44);
    ADD_NT_INTERCEPTION(NtUnmapViewOfSection, UNMAP_VIEW_OF_SECTION_ID, 12);
  }

  
  HANDLE child = child_->Process();
  BYTE* thunk_base = reinterpret_cast<BYTE*>(
                         ::VirtualAllocEx(child, NULL, kAllocGranularity,
                                          MEM_RESERVE, PAGE_NOACCESS));

  
  size_t thunk_bytes = interceptions_.size() * sizeof(ThunkData) +
                       sizeof(DllInterceptionData);
  size_t thunk_offset = GetGranularAlignedRandomOffset(thunk_bytes);

  
  thunk_base += thunk_offset & ~(kPageSize - 1);
  thunk_offset &= kPageSize - 1;

  
  size_t thunk_bytes_padded = (thunk_bytes + kPageSize - 1) & kPageSize;
  thunk_base = reinterpret_cast<BYTE*>(
                   ::VirtualAllocEx(child, thunk_base, thunk_bytes_padded,
                                    MEM_COMMIT, PAGE_EXECUTE_READWRITE));
  CHECK(thunk_base);  
  DllInterceptionData* thunks = reinterpret_cast<DllInterceptionData*>(
                                    thunk_base + thunk_offset);

  DllInterceptionData dll_data;
  dll_data.data_bytes = thunk_bytes;
  dll_data.num_thunks = 0;
  dll_data.used_bytes = offsetof(DllInterceptionData, thunks);

  
  memset(g_originals, 0, sizeof(g_originals));

  
  if (!PatchClientFunctions(thunks, thunk_bytes, &dll_data))
    return false;

  
  SIZE_T written;
  bool ok = FALSE != ::WriteProcessMemory(child, thunks, &dll_data,
                                          offsetof(DllInterceptionData, thunks),
                                          &written);

  if (!ok || (offsetof(DllInterceptionData, thunks) != written))
    return false;

  
  DWORD old_protection;
  ::VirtualProtectEx(child, thunks, thunk_bytes,
                     PAGE_EXECUTE_READ, &old_protection);

  ResultCode ret = child_->TransferVariable("g_originals", g_originals,
                                            sizeof(g_originals));
  return (SBOX_ALL_OK == ret);
}

bool InterceptionManager::PatchClientFunctions(DllInterceptionData* thunks,
                                               size_t thunk_bytes,
                                               DllInterceptionData* dll_data) {
  DCHECK(NULL != thunks);
  DCHECK(NULL != dll_data);

  HMODULE ntdll_base = ::GetModuleHandle(kNtdllName);
  if (!ntdll_base)
    return false;

  base::win::PEImage ntdll_image(ntdll_base);

  
  wchar_t* loader_get = reinterpret_cast<wchar_t*>(
                            ntdll_image.GetProcAddress("LdrGetDllHandle"));
  if (loader_get) {
    if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           loader_get, &ntdll_base))
      return false;
  }

  if (base::win::GetVersion() <= base::win::VERSION_VISTA) {
    Wow64 WowHelper(child_, ntdll_base);
    if (!WowHelper.WaitForNtdll())
      return false;
  }

  char* interceptor_base = NULL;

#if SANDBOX_EXPORTS
  interceptor_base = reinterpret_cast<char*>(child_->MainModule());
  HMODULE local_interceptor = ::LoadLibrary(child_->Name());
#endif

  ServiceResolverThunk* thunk;
#if defined(_WIN64)
  thunk = new ServiceResolverThunk(child_->Process(), relaxed_);
#else
  base::win::OSInfo* os_info = base::win::OSInfo::GetInstance();
  if (os_info->wow64_status() == base::win::OSInfo::WOW64_ENABLED) {
    if (os_info->version() >= base::win::VERSION_WIN8)
      thunk = new Wow64W8ResolverThunk(child_->Process(), relaxed_);
    else
      thunk = new Wow64ResolverThunk(child_->Process(), relaxed_);
  } else if (!IsXPSP2OrLater()) {
    thunk = new Win2kResolverThunk(child_->Process(), relaxed_);
  } else if (os_info->version() >= base::win::VERSION_WIN8) {
    thunk = new Win8ResolverThunk(child_->Process(), relaxed_);
  } else {
    thunk = new ServiceResolverThunk(child_->Process(), relaxed_);
  }
#endif

  std::list<InterceptionData>::iterator it = interceptions_.begin();
  for (; it != interceptions_.end(); ++it) {
    const std::wstring ntdll(kNtdllName);
    if (it->dll != ntdll)
      break;

    if (INTERCEPTION_SERVICE_CALL != it->type)
      break;

#if SANDBOX_EXPORTS
    
    if (NULL == it->interceptor_address) {
      const char* address;
      NTSTATUS ret = thunk->ResolveInterceptor(local_interceptor,
                                               it->interceptor.c_str(),
                                               reinterpret_cast<const void**>(
                                               &address));
      if (!NT_SUCCESS(ret))
        break;

      
      it->interceptor_address = interceptor_base + (address -
                                    reinterpret_cast<char*>(local_interceptor));
    }
#endif
    NTSTATUS ret = thunk->Setup(ntdll_base,
                                interceptor_base,
                                it->function.c_str(),
                                it->interceptor.c_str(),
                                it->interceptor_address,
                                &thunks->thunks[dll_data->num_thunks],
                                thunk_bytes - dll_data->used_bytes,
                                NULL);
    if (!NT_SUCCESS(ret))
      break;

    DCHECK(!g_originals[it->id]);
    g_originals[it->id] = &thunks->thunks[dll_data->num_thunks];

    dll_data->num_thunks++;
    dll_data->used_bytes += sizeof(ThunkData);
  }

  delete(thunk);

#if SANDBOX_EXPORTS
  if (NULL != local_interceptor)
    ::FreeLibrary(local_interceptor);
#endif

  if (it != interceptions_.end())
    return false;

  return true;
}

}  
