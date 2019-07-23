







































#ifndef __nsBidiKeyboard
#define __nsBidiKeyboard
#include "nsIBidiKeyboard.h"
#include <Carbon/Carbon.h>


class nsBidiKeyboard : public nsIBidiKeyboard
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBIDIKEYBOARD

  nsBidiKeyboard();
  virtual ~nsBidiKeyboard();

protected:
  PRBool IsRTLLanguage(SInt32 aKeyboardResID);
};


#endif 
