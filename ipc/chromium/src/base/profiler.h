







#ifndef BASE_PROFILER_H__
#define BASE_PROFILER_H__

#include "base/basictypes.h"

namespace base {

class Profiler {
 public:
  
  static void StartRecording();

  
  static void StopRecording();

  
  
  
  static void ClearData();

  
  static void SetThreadName(const char *name);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Profiler);
};

}  

#endif  
