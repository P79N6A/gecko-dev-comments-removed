




































#ifndef nsCommandLineServiceMac_h_
#define nsCommandLineServiceMac_h_

#include "nscore.h"

namespace CommandLineServiceMac {
  void SetupMacCommandLine(int& argc, char**& argv, bool forRestart);

  
  
  
  bool AddURLToCurrentCommandLine(const char* aURL);
}

#endif 
