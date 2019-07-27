





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
class PPluginIdentifierParent;

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
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  AnswerHasMethod(PPluginIdentifierParent* aId,
                  bool* aHasMethod) MOZ_OVERRIDE;

  virtual bool
  AnswerInvoke(PPluginIdentifierParent* aId,
               const InfallibleTArray<Variant>& aArgs,
               Variant* aResult,
               bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerInvokeDefault(const InfallibleTArray<Variant>& aArgs,
                      Variant* aResult,
                      bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerHasProperty(PPluginIdentifierParent* aId,
                    bool* aHasProperty) MOZ_OVERRIDE;

  virtual bool
  AnswerGetParentProperty(PPluginIdentifierParent* aId,
                          Variant* aResult,
                          bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerSetProperty(PPluginIdentifierParent* aId,
                    const Variant& aValue,
                    bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerRemoveProperty(PPluginIdentifierParent* aId,
                       bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerEnumerate(InfallibleTArray<PPluginIdentifierParent*>* aProperties,
                  bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerConstruct(const InfallibleTArray<Variant>& aArgs,
                  Variant* aResult,
                  bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerNPN_Evaluate(const nsCString& aScript,
                     Variant* aResult,
                     bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  RecvProtect() MOZ_OVERRIDE;

  virtual bool
  RecvUnprotect() MOZ_OVERRIDE;

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
