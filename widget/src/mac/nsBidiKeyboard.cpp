







































#include "nsBidiKeyboard.h"

NS_IMPL_ISUPPORTS1(nsBidiKeyboard, nsIBidiKeyboard)

nsBidiKeyboard::nsBidiKeyboard() : nsIBidiKeyboard()
{
}

nsBidiKeyboard::~nsBidiKeyboard()
{
}

NS_IMETHODIMP nsBidiKeyboard::IsLangRTL(PRBool *aIsRTL)
{
  *aIsRTL = PR_FALSE;
  nsresult rv = NS_ERROR_FAILURE;

  OSStatus err;
  KeyboardLayoutRef currentKeyboard;

  err = ::KLGetCurrentKeyboardLayout(&currentKeyboard);
  if (err == noErr)
  {
    const void* currentKeyboardResID;
    err = ::KLGetKeyboardLayoutProperty(currentKeyboard, kKLIdentifier,
                                        &currentKeyboardResID);
    if (err == noErr)
    {
      rv = NS_OK;
      *aIsRTL = IsRTLLanguage((SInt32)currentKeyboardResID);
    }
  }

  return rv;
}

NS_IMETHODIMP nsBidiKeyboard::SetLangFromBidiLevel(PRUint8 aLevel)
{
  
  return NS_OK;
}

PRBool nsBidiKeyboard::IsRTLLanguage(SInt32 aKeyboardResID)
{
  
  
  
  
  return (aKeyboardResID >= -18943 && aKeyboardResID <= -17920);
}
