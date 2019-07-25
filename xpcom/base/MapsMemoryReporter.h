






































#ifndef mozilla_MapsMemoryReporter_h_
#define mozilla_MapsMemoryReporter_h_

namespace mozilla {
namespace MapsMemoryReporter {




#if defined(XP_LINUX)
  void Init();
#else
  void Init() {}
#endif

} 
} 

#endif
