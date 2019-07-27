









#ifndef mozilla_image_src_ShutdownTracker_h
#define mozilla_image_src_ShutdownTracker_h

namespace mozilla {
namespace image {







struct ShutdownTracker
{
  


  static void Initialize();

  






  static bool ShutdownHasStarted();

private:
  virtual ~ShutdownTracker() = 0;  
};

} 
} 

#endif 
