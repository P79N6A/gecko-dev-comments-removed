





#ifndef mozilla_dom_TVServiceFactory_h
#define mozilla_dom_TVServiceFactory_h

#include "nsCOMPtr.h"

class nsITVService;

namespace mozilla {
namespace dom {

class FakeTVService;

class TVServiceFactory
{
public:
  static already_AddRefed<FakeTVService> CreateFakeTVService();
};

} 
} 

#endif 
