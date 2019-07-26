





#ifndef _adivertablechannelparent_h_
#define _adivertablechannelparent_h_

#include "nsISupports.h"

class nsIStreamListener;

namespace mozilla {
namespace net {





class ADivertableParentChannel : public nsISupports
{
public:
  
  
  
  
  
  virtual void DivertTo(nsIStreamListener *aListener) = 0;

  
  virtual nsresult SuspendForDiversion() = 0;
};

} 
} 

#endif
