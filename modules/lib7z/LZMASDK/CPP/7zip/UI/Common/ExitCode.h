

#ifndef __EXIT_CODE_H
#define __EXIT_CODE_H

namespace NExitCode {

enum EEnum {

  kSuccess       = 0,     
  kWarning       = 1,     
  kFatalError    = 2,     
  
  
  
  
  kUserError     = 7,     
  kMemoryError   = 8,     
  
  
  kUserBreak     = 255   

};

}

#endif
