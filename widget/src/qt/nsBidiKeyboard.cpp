





































#include <Qt>
#include <QApplication>

#include "nsBidiKeyboard.h"

NS_IMPL_ISUPPORTS1(nsBidiKeyboard, nsIBidiKeyboard)

nsBidiKeyboard::nsBidiKeyboard() : nsIBidiKeyboard()
{
}

nsBidiKeyboard::~nsBidiKeyboard()
{
}

NS_IMETHODIMP nsBidiKeyboard::IsLangRTL(bool *aIsRTL)
{
    *aIsRTL = false;

    Qt::LayoutDirection layoutDir = QApplication::keyboardInputDirection();

    if (layoutDir == Qt::RightToLeft) {
        *aIsRTL = true;
    }
    
    return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::SetLangFromBidiLevel(PRUint8 aLevel)
{
    return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::GetHaveBidiKeyboards(bool* aResult)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
