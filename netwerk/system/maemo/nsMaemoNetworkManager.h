





































#ifndef NSMAEMONETWORKMANAGER_H_
#define NSMAEMONETWORKMANAGER_H_

#include "nscore.h"

class nsMaemoNetworkManager
{
public:
  
  static bool OpenConnectionSync();
  static void CloseConnection();

  static bool IsConnected();
  static bool GetLinkStatusKnown();

  
  static bool Startup();
  static void Shutdown();
};

#endif 
