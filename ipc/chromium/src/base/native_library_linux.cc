



#include "base/native_library.h"

#include <dlfcn.h>

#include "base/file_path.h"
#include "base/logging.h"

namespace base {


NativeLibrary LoadNativeLibrary(const FilePath& library_path) {
  void* dl = dlopen(library_path.value().c_str(), RTLD_LAZY);
  if (!dl)
    NOTREACHED() << "dlopen failed: " << dlerror();

  return dl;
}


void UnloadNativeLibrary(NativeLibrary library) {
  int ret = dlclose(library);
  if (ret < 0)
    NOTREACHED() << "dlclose failed: " << dlerror();
}


void* GetFunctionPointerFromNativeLibrary(NativeLibrary library,
                                          NativeLibraryFunctionNameType name) {
  return dlsym(library, name);
}

}  
