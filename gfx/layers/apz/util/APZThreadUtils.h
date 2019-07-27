




#ifndef mozilla_layers_APZThreadUtils_h
#define mozilla_layers_APZThreadUtils_h

#include "base/message_loop.h"

class Task;

namespace mozilla {
namespace layers {

class APZThreadUtils
{
public:
  




  static void SetThreadAssertionsEnabled(bool aEnabled);
  static bool GetThreadAssertionsEnabled();

  


  static void SetControllerThread(MessageLoop* aLoop);

  




  static void AssertOnControllerThread();

  




  static void AssertOnCompositorThread();

  




  static void RunOnControllerThread(Task* aTask);
};

} 
} 

#endif 
