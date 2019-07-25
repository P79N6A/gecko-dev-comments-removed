




































#ifndef NSQTNETWORKMANAGER_H_
#define NSQTNETWORKMANAGER_H_

#include "nscore.h"

class nsQtNetworkManager
{
public:
  
  static PRBool OpenConnectionSync();
  static void CloseConnection();

  static PRBool IsConnected();
  static PRBool GetLinkStatusKnown();

  
  static PRBool Startup();
  static void Shutdown();
};

#endif 
