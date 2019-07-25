





#ifndef TestBindingHeader_h
#define TestBindingHeader_h

#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class TestInterface : public nsISupports,
		      public nsWrapperCache
{
public:
  NS_DECL_ISUPPORTS

  virtual nsISupports* GetParentObject();

private:
};

} 
} 

#endif
