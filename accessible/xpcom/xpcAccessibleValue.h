





#ifndef mozilla_a11y_xpcAccessibleValue_h_
#define mozilla_a11y_xpcAccessibleValue_h_

#include "nsIAccessibleValue.h"

namespace mozilla {
namespace a11y {

class Accessible;





class xpcAccessibleValue : public nsIAccessibleValue
{
public:
  NS_IMETHOD GetMaximumValue(double* aValue) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetMinimumValue(double* aValue) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetCurrentValue(double* aValue) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD SetCurrentValue(double aValue) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetMinimumIncrement(double* aMinIncrement) MOZ_FINAL MOZ_OVERRIDE;

protected:
  xpcAccessibleValue() { }
  virtual ~xpcAccessibleValue() {}

private:
  Accessible* Intl();

  xpcAccessibleValue(const xpcAccessibleValue&) = delete;
  xpcAccessibleValue& operator =(const xpcAccessibleValue&) = delete;
};

} 
} 

#endif
