





#ifndef mozilla_a11y_ApplicationAccessibleWrap_h__
#define mozilla_a11y_ApplicationAccessibleWrap_h__

#include "ApplicationAccessible.h"

namespace mozilla {
namespace a11y {
 
class ApplicationAccessibleWrap: public ApplicationAccessible
{
public:
  static void Unload();
  static void PreCreate();

public:
  ApplicationAccessibleWrap();
  virtual ~ApplicationAccessibleWrap();

  
  virtual void Init();

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual bool AppendChild(Accessible* aChild);
  virtual bool RemoveChild(Accessible* aChild);

  


  NS_IMETHOD GetNativeInterface(void** aOutAccessible);
};

} 
} 

#endif   
