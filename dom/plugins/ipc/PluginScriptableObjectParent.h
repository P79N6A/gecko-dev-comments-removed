





#ifndef dom_plugins_PluginScriptableObjectParent_h
#define dom_plugins_PluginScriptableObjectParent_h 1

#include "mozilla/plugins/PPluginScriptableObjectParent.h"
#include "mozilla/plugins/PluginMessageUtils.h"

#include "npfunctions.h"
#include "npruntime.h"

namespace mozilla {
namespace plugins {

class PluginInstanceParent;
class PluginScriptableObjectParent;

struct ParentNPObject : NPObject
{
  ParentNPObject()
    : NPObject(), parent(nullptr), invalidated(false) { }

  
  
  PluginScriptableObjectParent* parent;
  bool invalidated;
};

class PluginScriptableObjectParent : public PPluginScriptableObjectParent
{
  friend class PluginInstanceParent;

public:
  explicit PluginScriptableObjectParent(ScriptableObjectType aType);
  virtual ~PluginScriptableObjectParent();

  void
  InitializeProxy();

  void
  InitializeLocal(NPObject* aObject);

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  AnswerHasMethod(const PluginIdentifier& aId,
                  bool* aHasMethod) override;

  virtual bool
  AnswerInvoke(const PluginIdentifier& aId,
               InfallibleTArray<Variant>&& aArgs,
               Variant* aResult,
               bool* aSuccess) override;

  virtual bool
  AnswerInvokeDefault(InfallibleTArray<Variant>&& aArgs,
                      Variant* aResult,
                      bool* aSuccess) override;

  virtual bool
  AnswerHasProperty(const PluginIdentifier& aId,
                    bool* aHasProperty) override;

  virtual bool
  AnswerGetParentProperty(const PluginIdentifier& aId,
                          Variant* aResult,
                          bool* aSuccess) override;

  virtual bool
  AnswerSetProperty(const PluginIdentifier& aId,
                    const Variant& aValue,
                    bool* aSuccess) override;

  virtual bool
  AnswerRemoveProperty(const PluginIdentifier& aId,
                       bool* aSuccess) override;

  virtual bool
  AnswerEnumerate(InfallibleTArray<PluginIdentifier>* aProperties,
                  bool* aSuccess) override;

  virtual bool
  AnswerConstruct(InfallibleTArray<Variant>&& aArgs,
                  Variant* aResult,
                  bool* aSuccess) override;

  virtual bool
  AnswerNPN_Evaluate(const nsCString& aScript,
                     Variant* aResult,
                     bool* aSuccess) override;

  virtual bool
  RecvProtect() override;

  virtual bool
  RecvUnprotect() override;

  static const NPClass*
  GetClass()
  {
    return &sNPClass;
  }

  PluginInstanceParent*
  GetInstance() const
  {
    return mInstance;
  }

  NPObject*
  GetObject(bool aCanResurrect);

  
  
  
  
  void Protect();

  
  
  
  
  void Unprotect();

  
  
  
  
  
  void DropNPObject();

  ScriptableObjectType
  Type() const {
    return mType;
  }

  bool GetPropertyHelper(NPIdentifier aName,
                         bool* aHasProperty,
                         bool* aHasMethod,
                         NPVariant* aResult);

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

  NPObject*
  CreateProxyObject();

  
  
  
  bool ResurrectProxyObject();

private:
  PluginInstanceParent* mInstance;

  
  
  NPObject* mObject;
  int mProtectCount;

  ScriptableObjectType mType;

  static const NPClass sNPClass;
};

} 
} 

#endif 
