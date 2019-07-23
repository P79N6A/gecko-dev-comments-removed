



#include "base/native_library.h"

#include <windows.h>

#include "base/file_path.h"
#include "base/path_service.h"

namespace base {


NativeLibrary LoadNativeLibrary(const FilePath& library_path) {
  
  
  bool restore_directory = false;
  std::wstring current_directory;
  if (PathService::Get(base::DIR_CURRENT, &current_directory)) {
    FilePath plugin_path = library_path.DirName();
    if (!plugin_path.value().empty()) {
      PathService::SetCurrentDirectory(plugin_path.value());
      restore_directory = true;
    }
  }

  HMODULE module = LoadLibrary(library_path.value().c_str());
  if (restore_directory)
    PathService::SetCurrentDirectory(current_directory);

  return module;
}


void UnloadNativeLibrary(NativeLibrary library) {
  FreeLibrary(library);
}


void* GetFunctionPointerFromNativeLibrary(NativeLibrary library,
                                          NativeLibraryFunctionNameType name) {
  return GetProcAddress(library, name);
}

}  
