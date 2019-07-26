


























#include "webrtc/modules/audio_device/linux/latebindingsymboltable_linux.h"

#if defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
#include <dlfcn.h>
#endif


using namespace webrtc;

namespace webrtc_adm_linux {

inline static const char *GetDllError() {
#if defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
  const char *err = dlerror();
  if (err) {
    return err;
  } else {
    return "No error";
  }
#else
#error Not implemented
#endif
}

DllHandle InternalLoadDll(const char dll_name[]) {
#if defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
  DllHandle handle = dlopen(dll_name, RTLD_NOW);
#else
#error Not implemented
#endif
  if (handle == kInvalidDllHandle) {
    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, -1,
               "Can't load %s : %s", dll_name, GetDllError());
  }
  return handle;
}

void InternalUnloadDll(DllHandle handle) {
#if defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
  if (dlclose(handle) != 0) {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
               "%s", GetDllError());
  }
#else
#error Not implemented
#endif
}

static bool LoadSymbol(DllHandle handle,
                       const char *symbol_name,
                       void **symbol) {
#if defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
  *symbol = dlsym(handle, symbol_name);
  const char *err = dlerror();
  if (err) {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
               "Error loading symbol %s : %d", symbol_name, err);
    return false;
  } else if (!*symbol) {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
               "Symbol %s is NULL", symbol_name);
    return false;
  }
  return true;
#else
#error Not implemented
#endif
}




bool InternalLoadSymbols(DllHandle handle,
                         int num_symbols,
                         const char *const symbol_names[],
                         void *symbols[]) {
#if defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
  
  dlerror();
#endif
  for (int i = 0; i < num_symbols; ++i) {
    if (!LoadSymbol(handle, symbol_names[i], &symbols[i])) {
      return false;
    }
  }
  return true;
}

}  
