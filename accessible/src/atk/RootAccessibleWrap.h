







































#ifndef mozilla_a11y_RootAccessibleWrap_h__
#define mozilla_a11y_RootAccessibleWrap_h__

#include "RootAccessible.h"

namespace mozilla {
namespace a11y {

typedef RootAccessible RootAccessibleWrap;






class NativeRootAccessibleWrap : public RootAccessible
{
public:
  NativeRootAccessibleWrap(AtkObject* aAccessible);
  virtual ~NativeRootAccessibleWrap();
};

} 
} 

#endif   

