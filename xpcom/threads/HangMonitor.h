





#ifndef mozilla_HangMonitor_h
#define mozilla_HangMonitor_h

namespace mozilla {
namespace HangMonitor {




enum ActivityType
{
  
  kUIActivity,

  
  kActivityNoUIAVail,

  
  kActivityUIAVail,

  
  kGeneralActivity
};




void Startup();




void Shutdown();







void NotifyActivity(ActivityType activityType = kGeneralActivity);





void Suspend();

} 
} 

#endif 
