





#ifndef mozilla_ipc_ProcessUtils_h
#define mozilla_ipc_ProcessUtils_h

#ifdef MOZ_B2G_LOADER
#include "base/process_util.h"
#endif

namespace mozilla {
namespace ipc {



void SetThisProcessName(const char *aName);

#ifdef MOZ_B2G_LOADER

void ProcLoaderClientGeckoInit();

bool ProcLoaderLoad(const char *aArgv[],
                    const char *aEnvp[],
                    const base::file_handle_mapping_vector &aFdsRemap,
                    const base::ChildPrivileges aPrivs,
                    base::ProcessHandle *aProcessHandle);
#endif 

} 
} 

#endif 

