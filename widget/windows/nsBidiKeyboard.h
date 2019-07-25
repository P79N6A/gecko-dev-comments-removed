






































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
  bool IsRTLLanguage(HKL aLocale);

  bool mInitialized;
  bool mHaveBidiKeyboards;
  PRUnichar  mLTRKeyboard[KL_NAMELENGTH];
  PRUnichar  mRTLKeyboard[KL_NAMELENGTH];
  PRUnichar  mCurrentLocaleName[KL_NAMELENGTH];
};


#endif 
