





#include "xpcAccessibleGeneric.h"
#include "Accessible.h"

using namespace mozilla;
using namespace mozilla::a11y;

NS_IMETHODIMP
xpcAccessibleValue::GetMaximumValue(double* aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  if (Intl()->IsDefunct())
    return NS_ERROR_FAILURE;

  double value = Intl()->MaxValue();
  if (!IsNaN(value))
    *aValue = value;

  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleValue::GetMinimumValue(double* aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  if (Intl()->IsDefunct())
    return NS_ERROR_FAILURE;

  double value = Intl()->MinValue();
  if (!IsNaN(value))
    *aValue = value;

  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleValue::GetCurrentValue(double* aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  if (Intl()->IsDefunct())
    return NS_ERROR_FAILURE;

  double value = Intl()->CurValue();
  if (!IsNaN(value))
    *aValue = value;

  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleValue::SetCurrentValue(double aValue)
{
  if (Intl()->IsDefunct())
    return NS_ERROR_FAILURE;

  Intl()->SetCurValue(aValue);
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleValue::GetMinimumIncrement(double* aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  if (Intl()->IsDefunct())
    return NS_ERROR_FAILURE;

  double value = Intl()->Step();
  if (!IsNaN(value))
    *aValue = value;

  return NS_OK;
}
