






































#include <stdio.h>
#include "nsBidiKeyboard.h"
#include "prmem.h"

NS_IMPL_ISUPPORTS1(nsBidiKeyboard, nsIBidiKeyboard)

nsBidiKeyboard::nsBidiKeyboard() : nsIBidiKeyboard()
{
  mInitialized = PR_FALSE;
  mHaveBidiKeyboards = PR_FALSE;
  mLTRKeyboard[0] = '\0';
  mRTLKeyboard[0] = '\0';
  mCurrentLocaleName[0] = '\0';
}

nsBidiKeyboard::~nsBidiKeyboard()
{
}

NS_IMETHODIMP nsBidiKeyboard::SetLangFromBidiLevel(PRUint8 aLevel)
{
  nsresult result = SetupBidiKeyboards();
  if (NS_FAILED(result))
    return result;

  
  char currentLocaleName[KL_NAMELENGTH];
  strncpy(currentLocaleName, (aLevel & 1) ? mRTLKeyboard : mLTRKeyboard, KL_NAMELENGTH);
  currentLocaleName[KL_NAMELENGTH-1] = '\0'; 

  NS_ASSERTION(*currentLocaleName, 
    "currentLocaleName has string length == 0");

#if 0
  

  if (strcmp(mCurrentLocaleName, currentLocaleName)) {
    if (!::LoadKeyboardLayout(currentLocaleName, KLF_ACTIVATE | KLF_SUBSTITUTE_OK)) {
      return NS_ERROR_FAILURE;
    }
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::IsLangRTL(PRBool *aIsRTL)
{
  *aIsRTL = PR_FALSE;

  nsresult result = SetupBidiKeyboards();
  if (NS_FAILED(result))
    return result;

  HKL  currentLocale;
 
  currentLocale = ::GetKeyboardLayout(0);
  *aIsRTL = IsRTLLanguage(currentLocale);
  
  if (!::GetKeyboardLayoutName(mCurrentLocaleName))
    return NS_ERROR_FAILURE;

  NS_ASSERTION(*mCurrentLocaleName, 
    "GetKeyboardLayoutName return string length == 0");
  NS_ASSERTION((strlen(mCurrentLocaleName) < KL_NAMELENGTH), 
    "GetKeyboardLayoutName return string length >= KL_NAMELENGTH");

  
  if (*aIsRTL) {
    strncpy(mRTLKeyboard, mCurrentLocaleName, KL_NAMELENGTH);
    mRTLKeyboard[KL_NAMELENGTH-1] = '\0'; 
  } else {
    strncpy(mLTRKeyboard, mCurrentLocaleName, KL_NAMELENGTH);
    mLTRKeyboard[KL_NAMELENGTH-1] = '\0'; 
  }

  NS_ASSERTION((strlen(mRTLKeyboard) < KL_NAMELENGTH), 
    "mLTRKeyboard has string length >= KL_NAMELENGTH");
  NS_ASSERTION((strlen(mLTRKeyboard) < KL_NAMELENGTH), 
    "mRTLKeyboard has string length >= KL_NAMELENGTH");
  return NS_OK;
}





nsresult nsBidiKeyboard::SetupBidiKeyboards()
{
  if (mInitialized)
    return mHaveBidiKeyboards ? NS_OK : NS_ERROR_FAILURE;

  int keyboards;
  HKL far* buf;
  HKL locale;
  char localeName[KL_NAMELENGTH];
  PRBool isLTRKeyboardSet = PR_FALSE;
  PRBool isRTLKeyboardSet = PR_FALSE;
  
  
  keyboards = ::GetKeyboardLayoutList(0, nsnull);
  if (!keyboards)
    return NS_ERROR_FAILURE;

  
  buf = (HKL far*) PR_Malloc(keyboards * sizeof(HKL));
  if (!buf)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (::GetKeyboardLayoutList(keyboards, buf) != keyboards) {
    PR_Free(buf);
    return NS_ERROR_UNEXPECTED;
  }

  
  while (keyboards--) {
    locale = buf[keyboards];
    if (IsRTLLanguage(locale)) {
      sprintf(mRTLKeyboard, "%.*x", KL_NAMELENGTH - 1, LANGIDFROMLCID(locale));
      isRTLKeyboardSet = PR_TRUE;
    }
    else {
      sprintf(mLTRKeyboard, "%.*x", KL_NAMELENGTH - 1, LANGIDFROMLCID(locale));
      isLTRKeyboardSet = PR_TRUE;
    }
  }
  PR_Free(buf);
  mInitialized = PR_TRUE;

  
  
  mHaveBidiKeyboards = (isRTLKeyboardSet && isLTRKeyboardSet);
  if (!mHaveBidiKeyboards)
    return NS_ERROR_FAILURE;

  
  
  
  
  locale = ::GetKeyboardLayout(0);
  if (!::GetKeyboardLayoutName(localeName))
    return NS_ERROR_FAILURE;

  NS_ASSERTION(*localeName, 
    "GetKeyboardLayoutName return string length == 0");
  NS_ASSERTION((strlen(localeName) < KL_NAMELENGTH), 
    "GetKeyboardLayout return string length >= KL_NAMELENGTH");

  if (IsRTLLanguage(locale)) {
    strncpy(mRTLKeyboard, localeName, KL_NAMELENGTH);
    mRTLKeyboard[KL_NAMELENGTH-1] = '\0'; 
  }
  else {
    strncpy(mLTRKeyboard, localeName, KL_NAMELENGTH);
    mLTRKeyboard[KL_NAMELENGTH-1] = '\0'; 
  }

  NS_ASSERTION(*mRTLKeyboard, 
    "mLTRKeyboard has string length == 0");
  NS_ASSERTION(*mLTRKeyboard, 
    "mLTRKeyboard has string length == 0");

  return NS_OK;
}





PRBool nsBidiKeyboard::IsRTLLanguage(HKL aLocale)
{
  LOCALESIGNATURE localesig;
  return (::GetLocaleInfoW(PRIMARYLANGID(aLocale),
                           LOCALE_FONTSIGNATURE,
                           (LPWSTR)&localesig,
                           (sizeof(localesig)/sizeof(WCHAR))) &&
          (localesig.lsUsb[3] & 0x08000000));
}
