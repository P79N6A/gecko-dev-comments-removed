







































#ifndef nsBidiKeyboard_h_
#define nsBidiKeyboard_h_

#include "nsIBidiKeyboard.h"

class nsBidiKeyboard : public nsIBidiKeyboard
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBIDIKEYBOARD

  nsBidiKeyboard();
  virtual ~nsBidiKeyboard();
};


#endif 
