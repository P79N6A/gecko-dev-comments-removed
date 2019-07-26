






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

#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
    Qt::LayoutDirection layoutDir = QApplication::keyboardInputDirection();
#else
    QInputMethod* input = qApp->inputMethod();
    Qt::LayoutDirection layoutDir = input ? input->inputDirection() : Qt::LeftToRight;
#endif

    if (layoutDir == Qt::RightToLeft) {
        *aIsRTL = true;
    }
    
    return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::SetLangFromBidiLevel(uint8_t aLevel)
{
    return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::GetHaveBidiKeyboards(bool* aResult)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
