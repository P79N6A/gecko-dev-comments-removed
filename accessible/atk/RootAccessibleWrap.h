





#ifndef mozilla_a11y_RootAccessibleWrap_h__
#define mozilla_a11y_RootAccessibleWrap_h__

#include "BaseAccessibles.h"
#include "RootAccessible.h"

namespace mozilla {
namespace a11y {

typedef RootAccessible RootAccessibleWrap;






class GtkWindowAccessible MOZ_FINAL : public DummyAccessible
{
public:
  explicit GtkWindowAccessible(AtkObject* aAccessible);
  virtual ~GtkWindowAccessible();
};

} 
} 

#endif   

