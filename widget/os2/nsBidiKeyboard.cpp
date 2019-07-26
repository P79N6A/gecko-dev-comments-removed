





#include "nsBidiKeyboard.h"

NS_IMPL_ISUPPORTS1(nsBidiKeyboard, nsIBidiKeyboard)

nsBidiKeyboard::nsBidiKeyboard() : nsIBidiKeyboard()
{
  Reset();
}

nsBidiKeyboard::~nsBidiKeyboard()
{
}

NS_IMETHODIMP nsBidiKeyboard::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::IsLangRTL(bool *aIsRTL)
{
  *aIsRTL = false;
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsBidiKeyboard::GetHaveBidiKeyboards(bool* aResult)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
