






































#ifndef mozilla_AvailableMemoryTracker_h
#define mozilla_AvailableMemoryTracker_h

namespace mozilla {
namespace AvailableMemoryTracker {













#if defined(XP_WIN)
void Init();
void Activate();
#else
void Init() {}
void Activate() {}
#endif

} 
} 

#endif 
