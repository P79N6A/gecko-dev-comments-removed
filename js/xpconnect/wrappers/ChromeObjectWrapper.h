






#ifndef __ChromeObjectWrapper_h__
#define __ChromeObjectWrapper_h__

#include "FilteringWrapper.h"
#include "AccessCheck.h"

namespace xpc {






#define ChromeObjectWrapperBase \
  FilteringWrapper<js::CrossCompartmentSecurityWrapper, ExposedPropertiesOnly>

class ChromeObjectWrapper : public ChromeObjectWrapperBase
{
  public:
    ChromeObjectWrapper() : ChromeObjectWrapperBase(0) {};

    static ChromeObjectWrapper singleton;
};

} 

#endif 
