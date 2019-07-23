






































#ifndef __nsBidiKeyboard
#define __nsBidiKeyboard
#include "nsIBidiKeyboard.h"
#include <windows.h>

class nsBidiKeyboard : public nsIBidiKeyboard
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBIDIKEYBOARD

  nsBidiKeyboard();
  virtual ~nsBidiKeyboard();

protected:

  nsresult SetupBidiKeyboards();
  PRBool IsRTLLanguage(HKL aLocale);

  PRPackedBool mInitialized;
  PRPackedBool mHaveBidiKeyboards;
  char mLTRKeyboard[KL_NAMELENGTH];
  char mRTLKeyboard[KL_NAMELENGTH];
  char mCurrentLocaleName[KL_NAMELENGTH];
};


#endif 
