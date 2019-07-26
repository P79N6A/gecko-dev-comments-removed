




#include <QFeedbackEffect>
#include "QtHapticFeedback.h"

NS_IMPL_ISUPPORTS1(QtHapticFeedback, nsIHapticFeedback)

NS_IMETHODIMP
QtHapticFeedback::PerformSimpleAction(int32_t aType)
{
    if (aType == ShortPress)
        QFeedbackEffect::playThemeEffect(QFeedbackEffect::PressWeak);
    if (aType == LongPress)
        QFeedbackEffect::playThemeEffect(QFeedbackEffect::PressStrong);

    return NS_OK;
}
