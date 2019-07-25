






































#ifndef dom_plugins_PluginIdentifierParent_h
#define dom_plugins_PluginIdentifierParent_h

#include "mozilla/plugins/PPluginIdentifierParent.h"

#include "npapi.h"
#include "npruntime.h"

namespace mozilla {
namespace plugins {

class PluginIdentifierParent : public PPluginIdentifierParent
{
  friend class PluginModuleParent;

public:
  NPIdentifier ToNPIdentifier()
  {
    return mIdentifier;
  }

protected:
  PluginIdentifierParent(NPIdentifier aIdentifier)
    : mIdentifier(aIdentifier)
  {
    MOZ_COUNT_CTOR(PluginIdentifierParent);
  }

  virtual ~PluginIdentifierParent()
  {
    MOZ_COUNT_DTOR(PluginIdentifierParent);
  }

private:
  NPIdentifier mIdentifier;
};

} 
} 

#endif 
