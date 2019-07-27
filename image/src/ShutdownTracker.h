









#ifndef MOZILLA_IMAGELIB_SHUTDOWNTRACKER_H_
#define MOZILLA_IMAGELIB_SHUTDOWNTRACKER_H_

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
