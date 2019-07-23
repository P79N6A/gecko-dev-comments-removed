
























#ifndef NSMAEMONETWORKMANAGER_H_
#define NSMAEMONETWORKMANAGER_H_

#include "nscore.h"

class nsMaemoNetworkManager
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
