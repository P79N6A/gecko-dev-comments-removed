






#ifndef mozilla_ipc_SharedMemoryBasic_h
#define mozilla_ipc_SharedMemoryBasic_h

#ifdef ANDROID
#  include "mozilla/ipc/SharedMemoryBasic_android.h"
#elif defined(XP_MACOSX)
#  include "mozilla/ipc/SharedMemoryBasic_mach.h"
#else
#  include "mozilla/ipc/SharedMemoryBasic_chromium.h"
#endif

#endif 
