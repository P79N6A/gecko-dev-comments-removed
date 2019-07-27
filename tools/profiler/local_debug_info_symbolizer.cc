




#include "PlatformMacros.h"
#include "nsAutoPtr.h"

#if !defined(SPS_OS_windows)
# include "common/module.h"
# include "processor/cfi_frame_info.h"
#endif
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/stack_frame.h"
#include "common/logging.h"

#if defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_arm_android) \
    || defined(SPS_PLAT_x86_linux) || defined(SPS_PLAT_x86_android)
# include "common/linux/dump_symbols.h"
#elif defined(SPS_PLAT_amd64_darwin) || defined(SPS_PLAT_x86_darwin)
# include "shim_mac_dump_syms.h"
#elif defined(SPS_OS_windows)
  
#else
# error "Unknown platform"
#endif

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
# include "mozilla/Types.h"
# include "ElfLoader.h"
# include <dlfcn.h>
# include <sys/mman.h>
# include "nsString.h"
# include "nsDirectoryServiceUtils.h"
# include "nsDirectoryServiceDefs.h"
# include <sys/stat.h>
# include <fcntl.h>
#endif

#include "local_debug_info_symbolizer.h"

namespace google_breakpad {

LocalDebugInfoSymbolizer::~LocalDebugInfoSymbolizer() {
# if !defined(SPS_OS_windows)
  for (SymbolMap::iterator it = symbols_.begin();
       it != symbols_.end();
       ++it) {
    delete it->second;
  }
# endif
}

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)




static char* get_installation_lib_dir ( void )
{
  nsCOMPtr<nsIProperties>
    directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
  if (!directoryService) return NULL;
  nsCOMPtr<nsIFile> greDir;
  nsresult rv = directoryService->Get(NS_GRE_DIR, NS_GET_IID(nsIFile),
                                      getter_AddRefs(greDir));
  if (NS_FAILED(rv)) return NULL;
  nsCString path;
  rv = greDir->GetNativePath(path);
  if (NS_FAILED(rv)) return NULL;
  return strdup(path.get());
}





















static bool ReadSymbolData_ANDROID(const string& obj_filename,
                                   const std::vector<string>& debug_dirs,
                                   SymbolData symbol_data,
                                   Module** module)
{
  string obj_file_to_use = obj_filename;

  
  if (obj_file_to_use == "libmozglue.so") {
    char* libdir = get_installation_lib_dir();
    if (libdir) {
      obj_file_to_use = string(libdir) + "/lib/" + obj_file_to_use;
      free(libdir);
    }

    
    int fd = open(obj_file_to_use.c_str(), O_RDONLY);
    if (fd == -1) {
      BPLOG(INFO) << "ReadSymbolData_APK: Failed to open \'"
                  << obj_file_to_use << "\'";
      return false;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
      close(fd);
      BPLOG(INFO) << "ReadSymbolData_APK: Failed to fstat \'"
                  << obj_file_to_use << "\'";
      return false;
    }

    void* image = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (image == MAP_FAILED) {
      close(fd);
      BPLOG(INFO) << "ReadSymbolData_APK: Failed to mmap \'"
                  << obj_file_to_use << "\'";
      return false;
    }

    bool ok = ReadSymbolDataInternal((const uint8_t*)image,
                                     obj_file_to_use, debug_dirs,
                                     symbol_data, module);
    munmap(image, st.st_size);
    close(fd);
    return ok;
  }

  
  
  
  void* hdl = dlopen(obj_file_to_use.c_str(), RTLD_GLOBAL | RTLD_LAZY);
  if (!hdl) {
    BPLOG(INFO) << "ReadSymbolData_APK: Failed to get handle for ELF file \'"
                << obj_file_to_use << "\'";
    return false;
  }

  size_t sz = __dl_get_mappable_length(hdl);
  if (sz == 0) {
    dlclose(hdl);
    BPLOG(INFO) << "ReadSymbolData_APK: Unable to get size for ELF file \'"
                << obj_file_to_use << "\'";
    return false;
  }

  void* image = __dl_mmap(hdl, NULL, sz, 0);
  if (image == MAP_FAILED) {
    dlclose(hdl);
    BPLOG(INFO) << "ReadSymbolData_APK: Failed to mmap ELF file \'"
                << obj_file_to_use << "\'";
    return false;
  }

  bool ok = ReadSymbolDataInternal((const uint8_t*)image,
                                   obj_file_to_use, debug_dirs,
                                   symbol_data, module);
  __dl_munmap(hdl, image, sz);
  dlclose(hdl);

  return ok;
}
#endif 


StackFrameSymbolizer::SymbolizerResult
LocalDebugInfoSymbolizer::FillSourceLineInfo(const CodeModules* modules,
                                             const SystemInfo* system_info,
                                             StackFrame* frame) {
  if (!modules) {
    return kError;
  }
  const CodeModule* module = modules->GetModuleForAddress(frame->instruction);
  if (!module) {
    return kError;
  }
  frame->module = module;

# if !defined(SPS_OS_windows)
  Module* debug_info_module = NULL;
  SymbolMap::const_iterator it = symbols_.find(module->code_file());
  if (it == symbols_.end()) {
    if (no_symbol_modules_.find(module->code_file()) !=
        no_symbol_modules_.end()) {
      return kNoError;
    }

    bool ok = false;
#   if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
    ok = ReadSymbolData_ANDROID(module->code_file(), debug_dirs_,
                                ONLY_CFI, &debug_info_module);
#   elif defined(SPS_PLAT_amd64_darwin) || defined(SPS_PLAT_x86_darwin)
    ok = ReadSymbolData_DARWIN(module->code_file(), debug_dirs_,
                        ONLY_CFI, &debug_info_module);
#   else
    ok = ReadSymbolData(module->code_file(), debug_dirs_,
                        ONLY_CFI, &debug_info_module);
#   endif

    if (!ok) {
      if (debug_info_module)
        delete debug_info_module;
      no_symbol_modules_.insert(module->code_file());
      return kNoError;
    }

    symbols_[module->code_file()] = debug_info_module;
  } else {
    debug_info_module = it->second;
  }

  uint64_t address = frame->instruction - frame->module->base_address();
  Module::Function* function =
      debug_info_module->FindFunctionByAddress(address);
  if (function) {
    frame->function_name = function->name;
    
  } else {
    Module::Extern* ex = debug_info_module->FindExternByAddress(address);
    if (ex) {
      frame->function_name = ex->name;
    }
  }
# endif 
  return kNoError;
}


WindowsFrameInfo* LocalDebugInfoSymbolizer::FindWindowsFrameInfo(
    const StackFrame* frame) {
  
  
  return NULL;
}

#if !defined(SPS_OS_windows)

bool ParseCFIRuleSet(const string& rule_set, CFIFrameInfo* frame_info) {
  CFIFrameInfoParseHandler handler(frame_info);
  CFIRuleParser parser(&handler);
  return parser.Parse(rule_set);
}

static void ConvertCFI(const UniqueString* name, const Module::Expr& rule,
                       CFIFrameInfo* frame_info) {
  if (name == ustr__ZDcfa()) frame_info->SetCFARule(rule);
  else if (name == ustr__ZDra()) frame_info->SetRARule(rule);
  else frame_info->SetRegisterRule(name, rule);
}


static void ConvertCFI(const Module::RuleMap& rule_map,
                       CFIFrameInfo* frame_info) {
  for (Module::RuleMap::const_iterator it = rule_map.begin();
       it != rule_map.end(); ++it) {
    ConvertCFI(it->first, it->second, frame_info);
  }
}
#endif

CFIFrameInfo* LocalDebugInfoSymbolizer::FindCFIFrameInfo(
    const StackFrame* frame) {
#if defined(SPS_OS_windows)
  return NULL;
#else
  if (!frame || !frame->module) return NULL;

  SymbolMap::const_iterator it = symbols_.find(frame->module->code_file());
  if (it == symbols_.end()) return NULL;

  Module* module = it->second;
  uint64_t address = frame->instruction - frame->module->base_address();
  Module::StackFrameEntry* entry =
      module->FindStackFrameEntryByAddress(address);
  if (!entry)
    return NULL;

  
  
  
  
  nsAutoPtr<CFIFrameInfo> rules(new CFIFrameInfo());
  ConvertCFI(entry->initial_rules, rules);
  for (Module::RuleChangeMap::const_iterator delta_it =
           entry->rule_changes.begin();
       delta_it != entry->rule_changes.end() && delta_it->first < address;
       ++delta_it) {
    ConvertCFI(delta_it->second, rules);
  }
  return rules.forget();
#endif 
}

}  
