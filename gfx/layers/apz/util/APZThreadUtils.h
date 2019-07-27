




#ifndef mozilla_layers_APZThreadUtils_h
#define mozilla_layers_APZThreadUtils_h

namespace mozilla {
namespace layers {

class APZThreadUtils
{
public:
  




  static void SetThreadAssertionsEnabled(bool aEnabled);
  static bool GetThreadAssertionsEnabled();

  




  static void AssertOnControllerThread();

  




  static void AssertOnCompositorThread();
};

} 
} 

#endif 
