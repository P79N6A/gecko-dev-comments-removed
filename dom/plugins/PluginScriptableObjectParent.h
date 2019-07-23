





































#ifndef dom_plugins_PluginScriptableObjectParent_h
#define dom_plugins_PluginScriptableObjectParent_h 1

#include "mozilla/plugins/PPluginScriptableObjectProtocolParent.h"

namespace mozilla {
namespace plugins {

class PluginScriptableObjectParent : public PPluginScriptableObjectProtocolParent
{
public:
  PluginScriptableObjectParent();
  virtual ~PluginScriptableObjectParent();
};

} 
} 

#endif 
