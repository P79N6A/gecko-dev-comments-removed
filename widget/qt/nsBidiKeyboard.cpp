






#include <Qt>
#include <QGuiApplication>

#include "nsBidiKeyboard.h"

NS_IMPL_ISUPPORTS(nsBidiKeyboard, nsIBidiKeyboard)

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

    QInputMethod* input = qApp->inputMethod();
    Qt::LayoutDirection layoutDir = input ? input->inputDirection() : Qt::LeftToRight;

    if (layoutDir == Qt::RightToLeft) {
        *aIsRTL = true;
    }

    return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::GetHaveBidiKeyboards(bool* aResult)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
