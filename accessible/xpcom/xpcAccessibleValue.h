





#ifndef mozilla_a11y_xpcAccessibleValue_h_
#define mozilla_a11y_xpcAccessibleValue_h_

#include "nsIAccessibleValue.h"

namespace mozilla {
namespace a11y {

class xpcAccessibleValue : public nsIAccessibleValue
{
public:
  NS_IMETHOD GetMaximumValue(double* aValue) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetMinimumValue(double* aValue) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetCurrentValue(double* aValue) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD SetCurrentValue(double aValue) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetMinimumIncrement(double* aMinIncrement) MOZ_FINAL MOZ_OVERRIDE;

private:
  xpcAccessibleValue() { }
  friend class Accessible;

  xpcAccessibleValue(const xpcAccessibleValue&) MOZ_DELETE;
  xpcAccessibleValue& operator =(const xpcAccessibleValue&) MOZ_DELETE;
};

} 
} 

#endif
