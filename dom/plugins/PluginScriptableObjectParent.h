





































#ifndef dom_plugins_PluginScriptableObjectParent_h
#define dom_plugins_PluginScriptableObjectParent_h 1

#include "mozilla/plugins/PPluginScriptableObjectParent.h"

namespace mozilla {
namespace plugins {

class PluginScriptableObjectParent : public PPluginScriptableObjectParent
{
public:
  PluginScriptableObjectParent();
  virtual ~PluginScriptableObjectParent();
};

} 
} 

#endif 
