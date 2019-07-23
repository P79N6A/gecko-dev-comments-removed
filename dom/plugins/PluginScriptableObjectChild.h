





































#ifndef dom_plugins_PluginScriptableObjectChild_h
#define dom_plugins_PluginScriptableObjectChild_h 1

#include "mozilla/plugins/PPluginScriptableObjectChild.h"

#include "npruntime.h"

namespace mozilla {
namespace plugins {

class PluginInstanceChild;
class PluginScriptableObjectChild;

struct ChildNPObject : NPObject
{
  PluginScriptableObjectChild* parent;
  bool invalidated;
};

class PluginScriptableObjectChild : public PPluginScriptableObjectChild
{
  friend class PluginInstanceChild;

public:
  PluginScriptableObjectChild();
  virtual ~PluginScriptableObjectChild();

  virtual bool
  AnswerInvalidate();

  virtual bool
  AnswerHasMethod(const NPRemoteIdentifier& aId,
                  bool* aHasMethod);

  virtual bool
  AnswerInvoke(const NPRemoteIdentifier& aId,
               const nsTArray<Variant>& aArgs,
               Variant* aResult,
               bool* aSuccess);

  virtual bool
  AnswerInvokeDefault(const nsTArray<Variant>& aArgs,
                      Variant* aResult,
                      bool* aSuccess);

  virtual bool
  AnswerHasProperty(const NPRemoteIdentifier& aId,
                    bool* aHasProperty);

  virtual bool
  AnswerGetProperty(const NPRemoteIdentifier& aId,
                    Variant* aResult,
                    bool* aSuccess);

  virtual bool
  AnswerSetProperty(const NPRemoteIdentifier& aId,
                    const Variant& aValue,
                    bool* aSuccess);

  virtual bool
  AnswerRemoveProperty(const NPRemoteIdentifier& aId,
                       bool* aSuccess);

  virtual bool
  AnswerEnumerate(nsTArray<NPRemoteIdentifier>* aProperties,
                  bool* aSuccess);

  virtual bool
  AnswerConstruct(const nsTArray<Variant>& aArgs,
                  Variant* aResult,
                  bool* aSuccess);

  void
  Initialize(PluginInstanceChild* aInstance,
             NPObject* aObject);

  NPObject*
  GetObject()
  {
    return mObject;
  }

  static const NPClass*
  GetClass()
  {
    return &sNPClass;
  }

  PluginInstanceChild*
  GetInstance()
  {
    return mInstance;
  }

private:
  static NPObject*
  ScriptableAllocate(NPP aInstance,
                     NPClass* aClass);

  static void
  ScriptableInvalidate(NPObject* aObject);

  static void
  ScriptableDeallocate(NPObject* aObject);

  static bool
  ScriptableHasMethod(NPObject* aObject,
                      NPIdentifier aName);

  static bool
  ScriptableInvoke(NPObject* aObject,
                   NPIdentifier aName,
                   const NPVariant* aArgs,
                   uint32_t aArgCount,
                   NPVariant* aResult);

  static bool
  ScriptableInvokeDefault(NPObject* aObject,
                          const NPVariant* aArgs,
                          uint32_t aArgCount,
                          NPVariant* aResult);

  static bool
  ScriptableHasProperty(NPObject* aObject,
                        NPIdentifier aName);

  static bool
  ScriptableGetProperty(NPObject* aObject,
                        NPIdentifier aName,
                        NPVariant* aResult);

  static bool
  ScriptableSetProperty(NPObject* aObject,
                        NPIdentifier aName,
                        const NPVariant* aValue);

  static bool
  ScriptableRemoveProperty(NPObject* aObject,
                           NPIdentifier aName);

  static bool
  ScriptableEnumerate(NPObject* aObject,
                      NPIdentifier** aIdentifiers,
                      uint32_t* aCount);

  static bool
  ScriptableConstruct(NPObject* aObject,
                      const NPVariant* aArgs,
                      uint32_t aArgCount,
                      NPVariant* aResult);

private:
  PluginInstanceChild* mInstance;
  NPObject* mObject;

  static const NPClass sNPClass;
};

} 
} 

#endif 
