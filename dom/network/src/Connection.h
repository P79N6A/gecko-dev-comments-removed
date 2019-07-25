




































#ifndef mozilla_dom_network_Connection_h
#define mozilla_dom_network_Connection_h

#include "nsIDOMConnection.h"

namespace mozilla {
namespace dom {
namespace network {

class Connection : public nsIDOMMozConnection
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZCONNECTION

private:
  static const char* sMeteredPrefName;
  static const bool  sMeteredDefaultValue;
};

} 
} 
} 

#endif 
