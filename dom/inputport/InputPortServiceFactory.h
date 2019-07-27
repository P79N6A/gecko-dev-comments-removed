





#ifndef mozilla_dom_InputPortServiceFactory_h
#define mozilla_dom_InputPortServiceFactory_h

#include "nsCOMPtr.h"

class nsIInputPortService;

namespace mozilla {
namespace dom {

class FakeInputPortService;

class InputPortServiceFactory final
{
public:
  static already_AddRefed<FakeInputPortService> CreateFakeInputPortService();

  static already_AddRefed<nsIInputPortService> AutoCreateInputPortService();
};

} 
} 

#endif 
