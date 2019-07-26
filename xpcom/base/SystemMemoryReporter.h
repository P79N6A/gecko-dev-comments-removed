





#ifndef mozilla_SystemMemoryReporter_h_
#define mozilla_SystemMemoryReporter_h_

namespace mozilla {
namespace SystemMemoryReporter {




#if defined(XP_LINUX)
void
Init();
#else
void
Init()
{
}
#endif

} 
} 

#endif
