





#ifndef mozilla_a11y_ApplicationAccessibleWrap_h__
#define mozilla_a11y_ApplicationAccessibleWrap_h__

#include "ApplicationAccessible.h"

namespace mozilla {
namespace a11y {

class ApplicationAccessibleWrap: public ApplicationAccessible
{
public:
  ApplicationAccessibleWrap();
  virtual ~ApplicationAccessibleWrap();

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual bool InsertChildAt(uint32_t aIdx, Accessible* aChild) MOZ_OVERRIDE;
  virtual bool RemoveChild(Accessible* aChild);

  


  NS_IMETHOD GetNativeInterface(void** aOutAccessible);
};

} 
} 

#endif   
