






#include "PuppetBidiKeyboard.h"

using namespace mozilla::widget;

NS_IMPL_ISUPPORTS(PuppetBidiKeyboard, nsIBidiKeyboard)

PuppetBidiKeyboard::PuppetBidiKeyboard() : nsIBidiKeyboard()
{
}

PuppetBidiKeyboard::~PuppetBidiKeyboard()
{
}

NS_IMETHODIMP
PuppetBidiKeyboard::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
PuppetBidiKeyboard::IsLangRTL(bool* aIsRTL)
{
  *aIsRTL = mIsLangRTL;
  return NS_OK;
}

void
PuppetBidiKeyboard::SetIsLangRTL(bool aIsLangRTL)
{
  mIsLangRTL = aIsLangRTL;
}

NS_IMETHODIMP
PuppetBidiKeyboard::GetHaveBidiKeyboards(bool* aResult)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
