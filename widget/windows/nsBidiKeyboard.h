





#ifndef __nsBidiKeyboard
#define __nsBidiKeyboard
#include "nsIBidiKeyboard.h"
#include <windows.h>

class nsBidiKeyboard : public nsIBidiKeyboard
{
  virtual ~nsBidiKeyboard();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBIDIKEYBOARD

  nsBidiKeyboard();

protected:

  nsresult SetupBidiKeyboards();
  bool IsRTLLanguage(HKL aLocale);

  bool mInitialized;
  bool mHaveBidiKeyboards;
  wchar_t  mLTRKeyboard[KL_NAMELENGTH];
  wchar_t  mRTLKeyboard[KL_NAMELENGTH];
  wchar_t  mCurrentLocaleName[KL_NAMELENGTH];
};


#endif 
