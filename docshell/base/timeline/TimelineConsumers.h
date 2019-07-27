





#ifndef mozilla_TimelineConsumers_h_
#define mozilla_TimelineConsumers_h_

class nsDocShell;

namespace mozilla {





class TimelineConsumers
{
private:
  
  static unsigned long sActiveConsumers;

public:
  static void AddConsumer();
  static void RemoveConsumer();
  static bool IsEmpty();
};

} 

#endif 
