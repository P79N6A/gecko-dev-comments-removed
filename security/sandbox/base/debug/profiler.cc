



#include "base/debug/profiler.h"

#include <string>

#include "base/process/process_handle.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

#if defined(OS_WIN)
#include "base/win/pe_image.h"
#endif  

#if defined(ENABLE_PROFILING) && !defined(NO_TCMALLOC)
#include "third_party/tcmalloc/chromium/src/gperftools/profiler.h"
#endif

namespace base {
namespace debug {

#if defined(ENABLE_PROFILING) && !defined(NO_TCMALLOC)

static int profile_count = 0;

void StartProfiling(const std::string& name) {
  ++profile_count;
  std::string full_name(name);
  std::string pid = StringPrintf("%d", GetCurrentProcId());
  std::string count = StringPrintf("%d", profile_count);
  ReplaceSubstringsAfterOffset(&full_name, 0, "{pid}", pid);
  ReplaceSubstringsAfterOffset(&full_name, 0, "{count}", count);
  ProfilerStart(full_name.c_str());
}

void StopProfiling() {
  ProfilerFlush();
  ProfilerStop();
}

void FlushProfiling() {
  ProfilerFlush();
}

bool BeingProfiled() {
  return ProfilingIsEnabledForAllThreads();
}

void RestartProfilingAfterFork() {
  ProfilerRegisterThread();
}

#else

void StartProfiling(const std::string& name) {
}

void StopProfiling() {
}

void FlushProfiling() {
}

bool BeingProfiled() {
  return false;
}

void RestartProfilingAfterFork() {
}

#endif

#if !defined(OS_WIN)

bool IsBinaryInstrumented() {
  return false;
}

ReturnAddressLocationResolver GetProfilerReturnAddrResolutionFunc() {
  return NULL;
}

DynamicFunctionEntryHook GetProfilerDynamicFunctionEntryHookFunc() {
  return NULL;
}

AddDynamicSymbol GetProfilerAddDynamicSymbolFunc() {
  return NULL;
}

MoveDynamicSymbol GetProfilerMoveDynamicSymbolFunc() {
  return NULL;
}

#else  


extern "C" IMAGE_DOS_HEADER __ImageBase;

bool IsBinaryInstrumented() {
  enum InstrumentationCheckState {
    UNINITIALIZED,
    INSTRUMENTED_IMAGE,
    NON_INSTRUMENTED_IMAGE,
  };

  static InstrumentationCheckState state = UNINITIALIZED;

  if (state == UNINITIALIZED) {
    HMODULE this_module = reinterpret_cast<HMODULE>(&__ImageBase);
    base::win::PEImage image(this_module);

    
    DCHECK(image.VerifyMagic());

    
    
    
    if ((image.GetImageSectionHeaderByName(".thunks") != NULL) &&
        (image.GetImageSectionHeaderByName(".syzygy") != NULL)) {
      state = INSTRUMENTED_IMAGE;
    } else {
      state = NON_INSTRUMENTED_IMAGE;
    }
  }
  DCHECK(state != UNINITIALIZED);

  return state == INSTRUMENTED_IMAGE;
}

namespace {

struct FunctionSearchContext {
  const char* name;
  FARPROC function;
};


bool FindResolutionFunctionInImports(
    const base::win::PEImage &image, const char* module_name,
    PIMAGE_THUNK_DATA unused_name_table, PIMAGE_THUNK_DATA import_address_table,
    PVOID cookie) {
  FunctionSearchContext* context =
      reinterpret_cast<FunctionSearchContext*>(cookie);

  DCHECK_NE(static_cast<FunctionSearchContext*>(NULL), context);
  DCHECK_EQ(static_cast<FARPROC>(NULL), context->function);

  
  
  
  const wchar_t* function_in_module =
      reinterpret_cast<const wchar_t*>(import_address_table->u1.Function);

  
  const DWORD kFlags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
  HMODULE module = NULL;
  if (!::GetModuleHandleEx(kFlags, function_in_module, &module)) {
    
    return true;
  }

  
  FARPROC exported_func = ::GetProcAddress(module, context->name);
  if (exported_func != NULL) {
    
    context->function = exported_func;
    return false;
  }

  
  return true;
}

template <typename FunctionType>
FunctionType FindFunctionInImports(const char* function_name) {
  if (!IsBinaryInstrumented())
    return NULL;

  HMODULE this_module = reinterpret_cast<HMODULE>(&__ImageBase);
  base::win::PEImage image(this_module);

  FunctionSearchContext ctx = { function_name, NULL };
  image.EnumImportChunks(FindResolutionFunctionInImports, &ctx);

  return reinterpret_cast<FunctionType>(ctx.function);
}

}  

ReturnAddressLocationResolver GetProfilerReturnAddrResolutionFunc() {
  return FindFunctionInImports<ReturnAddressLocationResolver>(
      "ResolveReturnAddressLocation");
}

DynamicFunctionEntryHook GetProfilerDynamicFunctionEntryHookFunc() {
  return FindFunctionInImports<DynamicFunctionEntryHook>(
      "OnDynamicFunctionEntry");
}

AddDynamicSymbol GetProfilerAddDynamicSymbolFunc() {
  return FindFunctionInImports<AddDynamicSymbol>(
      "AddDynamicSymbol");
}

MoveDynamicSymbol GetProfilerMoveDynamicSymbolFunc() {
  return FindFunctionInImports<MoveDynamicSymbol>(
      "MoveDynamicSymbol");
}

#endif  

}  
}  
