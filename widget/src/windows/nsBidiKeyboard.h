






































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
  PRUnichar  mLTRKeyboard[KL_NAMELENGTH];
  PRUnichar  mRTLKeyboard[KL_NAMELENGTH];
  PRUnichar  mCurrentLocaleName[KL_NAMELENGTH];
};


#endif 
