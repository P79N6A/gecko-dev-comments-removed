






































#include <stdio.h>
#include "nsBidiKeyboard.h"
#include "prmem.h"
#include <tchar.h>

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

  
  PRUnichar currentLocaleName[KL_NAMELENGTH];
  wcsncpy(currentLocaleName, (aLevel & 1) ? mRTLKeyboard : mLTRKeyboard, KL_NAMELENGTH);
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

NS_IMETHODIMP nsBidiKeyboard::IsLangRTL(bool *aIsRTL)
{
  *aIsRTL = PR_FALSE;

  nsresult result = SetupBidiKeyboards();
  if (NS_FAILED(result))
    return result;

  HKL  currentLocale;
 
  currentLocale = ::GetKeyboardLayout(0);
  *aIsRTL = IsRTLLanguage(currentLocale);
  
  if (!::GetKeyboardLayoutNameW(mCurrentLocaleName))
    return NS_ERROR_FAILURE;

  NS_ASSERTION(*mCurrentLocaleName, 
    "GetKeyboardLayoutName return string length == 0");
  NS_ASSERTION((wcslen(mCurrentLocaleName) < KL_NAMELENGTH), 
    "GetKeyboardLayoutName return string length >= KL_NAMELENGTH");

  
  if (*aIsRTL) {
    wcsncpy(mRTLKeyboard, mCurrentLocaleName, KL_NAMELENGTH);
    mRTLKeyboard[KL_NAMELENGTH-1] = '\0'; 
  } else {
    wcsncpy(mLTRKeyboard, mCurrentLocaleName, KL_NAMELENGTH);
    mLTRKeyboard[KL_NAMELENGTH-1] = '\0'; 
  }

  NS_ASSERTION((wcslen(mRTLKeyboard) < KL_NAMELENGTH), 
    "mLTRKeyboard has string length >= KL_NAMELENGTH");
  NS_ASSERTION((wcslen(mLTRKeyboard) < KL_NAMELENGTH), 
    "mRTLKeyboard has string length >= KL_NAMELENGTH");
  return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::GetHaveBidiKeyboards(bool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  nsresult result = SetupBidiKeyboards();
  if (NS_FAILED(result))
    return result;

  *aResult = mHaveBidiKeyboards;
  return NS_OK;
}





nsresult nsBidiKeyboard::SetupBidiKeyboards()
{
  if (mInitialized)
    return mHaveBidiKeyboards ? NS_OK : NS_ERROR_FAILURE;

  int keyboards;
  HKL far* buf;
  HKL locale;
  PRUnichar localeName[KL_NAMELENGTH];
  bool isLTRKeyboardSet = false;
  bool isRTLKeyboardSet = false;
  
  
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
      _snwprintf(mRTLKeyboard, KL_NAMELENGTH, L"%.*x", KL_NAMELENGTH - 1,
                 LANGIDFROMLCID((DWORD_PTR)locale));
      isRTLKeyboardSet = PR_TRUE;
    }
    else {
      _snwprintf(mLTRKeyboard, KL_NAMELENGTH, L"%.*x", KL_NAMELENGTH - 1,
                 LANGIDFROMLCID((DWORD_PTR)locale));
      isLTRKeyboardSet = PR_TRUE;
    }
  }
  PR_Free(buf);
  mInitialized = PR_TRUE;

  
  
  mHaveBidiKeyboards = (isRTLKeyboardSet && isLTRKeyboardSet);
  if (!mHaveBidiKeyboards)
    return NS_ERROR_FAILURE;

  
  
  
  
  locale = ::GetKeyboardLayout(0);
  if (!::GetKeyboardLayoutNameW(localeName))
    return NS_ERROR_FAILURE;

  NS_ASSERTION(*localeName, 
    "GetKeyboardLayoutName return string length == 0");
  NS_ASSERTION((wcslen(localeName) < KL_NAMELENGTH), 
    "GetKeyboardLayout return string length >= KL_NAMELENGTH");

  if (IsRTLLanguage(locale)) {
    wcsncpy(mRTLKeyboard, localeName, KL_NAMELENGTH);
    mRTLKeyboard[KL_NAMELENGTH-1] = '\0'; 
  }
  else {
    wcsncpy(mLTRKeyboard, localeName, KL_NAMELENGTH);
    mLTRKeyboard[KL_NAMELENGTH-1] = '\0'; 
  }

  NS_ASSERTION(*mRTLKeyboard, 
    "mLTRKeyboard has string length == 0");
  NS_ASSERTION(*mLTRKeyboard, 
    "mLTRKeyboard has string length == 0");

  return NS_OK;
}





bool nsBidiKeyboard::IsRTLLanguage(HKL aLocale)
{
  LOCALESIGNATURE localesig;
  return (::GetLocaleInfoW(PRIMARYLANGID((DWORD_PTR)aLocale),
                           LOCALE_FONTSIGNATURE,
                           (LPWSTR)&localesig,
                           (sizeof(localesig)/sizeof(WCHAR))) &&
          (localesig.lsUsb[3] & 0x08000000));
}
