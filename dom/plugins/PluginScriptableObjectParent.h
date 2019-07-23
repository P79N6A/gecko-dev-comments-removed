





































#ifndef dom_plugins_PluginScriptableObjectParent_h
#define dom_plugins_PluginScriptableObjectParent_h 1

#include "mozilla/plugins/PPluginScriptableObjectParent.h"

#include "npruntime.h"

namespace mozilla {
namespace plugins {

class PluginInstanceParent;
class PluginScriptableObjectParent;

struct ParentNPObject : NPObject
{
  PluginScriptableObjectParent* parent;
  bool invalidated;
};

class PluginScriptableObjectParent : public PPluginScriptableObjectParent
{
public:
  PluginScriptableObjectParent();
  virtual ~PluginScriptableObjectParent();

  void
  Initialize(PluginInstanceParent* aInstance,
             ParentNPObject* aObject);

  static NPClass*
  GetClass()
  {
    return &sNPClass;
  }

  PluginInstanceParent*
  GetInstance()
  {
    return mInstance;
  }

  NPObject*
  GetObject()
  {
    return mObject;
  }

private:
  PluginInstanceParent* mInstance;
  ParentNPObject* mObject;

  static NPClass sNPClass;
};

} 
} 

#endif 
