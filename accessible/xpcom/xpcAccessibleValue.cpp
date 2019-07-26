





#include "xpcAccessibleValue.h"
#include "Accessible.h"

using namespace mozilla;
using namespace mozilla::a11y;

NS_IMETHODIMP
xpcAccessibleValue::GetMaximumValue(double* aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  Accessible* acc = static_cast<Accessible*>(this);
  if (acc->IsDefunct())
    return NS_ERROR_FAILURE;

  double value = acc->MaxValue();
  if (!IsNaN(value))
    *aValue = value;

  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleValue::GetMinimumValue(double* aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  Accessible* acc = static_cast<Accessible*>(this);
  if (acc->IsDefunct())
    return NS_ERROR_FAILURE;

  double value = acc->MinValue();
  if (!IsNaN(value))
    *aValue = value;

  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleValue::GetCurrentValue(double* aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  Accessible* acc = static_cast<Accessible*>(this);
  if (acc->IsDefunct())
    return NS_ERROR_FAILURE;

  double value = acc->CurValue();
  if (!IsNaN(value))
    *aValue = value;

  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleValue::SetCurrentValue(double aValue)
{
  Accessible* acc = static_cast<Accessible*>(this);
  if (acc->IsDefunct())
    return NS_ERROR_FAILURE;

  acc->SetCurValue(aValue);
  return NS_OK;
}

NS_IMETHODIMP
xpcAccessibleValue::GetMinimumIncrement(double* aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  Accessible* acc = static_cast<Accessible*>(this);
  if (acc->IsDefunct())
    return NS_ERROR_FAILURE;

  double value = acc->Step();
  if (!IsNaN(value))
    *aValue = value;

  return NS_OK;
}
