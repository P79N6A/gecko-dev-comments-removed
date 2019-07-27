





#ifndef dom_plugins_PluginScriptableObjectChild_h
#define dom_plugins_PluginScriptableObjectChild_h 1

#include "mozilla/plugins/PPluginScriptableObjectChild.h"
#include "mozilla/plugins/PluginMessageUtils.h"

#include "npruntime.h"

namespace mozilla {
namespace plugins {

class PluginInstanceChild;
class PluginScriptableObjectChild;
class PPluginIdentifierChild;

struct ChildNPObject : NPObject
{
  ChildNPObject()
    : NPObject(), parent(nullptr), invalidated(false)
  {
    MOZ_COUNT_CTOR(ChildNPObject);
  }

  ~ChildNPObject()
  {
    MOZ_COUNT_DTOR(ChildNPObject);
  }

  
  
  PluginScriptableObjectChild* parent;
  bool invalidated;
};

class PluginScriptableObjectChild : public PPluginScriptableObjectChild
{
  friend class PluginInstanceChild;

public:
  explicit PluginScriptableObjectChild(ScriptableObjectType aType);
  virtual ~PluginScriptableObjectChild();

  bool
  InitializeProxy();

  void
  InitializeLocal(NPObject* aObject);


  virtual bool
  AnswerInvalidate() MOZ_OVERRIDE;

  virtual bool
  AnswerHasMethod(PPluginIdentifierChild* aId,
                  bool* aHasMethod) MOZ_OVERRIDE;

  virtual bool
  AnswerInvoke(PPluginIdentifierChild* aId,
               const InfallibleTArray<Variant>& aArgs,
               Variant* aResult,
               bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerInvokeDefault(const InfallibleTArray<Variant>& aArgs,
                      Variant* aResult,
                      bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerHasProperty(PPluginIdentifierChild* aId,
                    bool* aHasProperty) MOZ_OVERRIDE;

  virtual bool
  AnswerGetChildProperty(PPluginIdentifierChild* aId,
                         bool* aHasProperty,
                         bool* aHasMethod,
                         Variant* aResult,
                         bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerSetProperty(PPluginIdentifierChild* aId,
                    const Variant& aValue,
                    bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerRemoveProperty(PPluginIdentifierChild* aId,
                       bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerEnumerate(InfallibleTArray<PPluginIdentifierChild*>* aProperties,
                  bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerConstruct(const InfallibleTArray<Variant>& aArgs,
                  Variant* aResult,
                  bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  RecvProtect() MOZ_OVERRIDE;

  virtual bool
  RecvUnprotect() MOZ_OVERRIDE;

  NPObject*
  GetObject(bool aCanResurrect);

  static const NPClass*
  GetClass()
  {
    return &sNPClass;
  }

  PluginInstanceChild*
  GetInstance() const
  {
    return mInstance;
  }

  
  
  
  
  void Protect();

  
  
  
  
  void Unprotect();

  
  
  
  
  
  void DropNPObject();

  




  void NPObjectDestroyed();

  bool
  Evaluate(NPString* aScript,
           NPVariant* aResult);

  ScriptableObjectType
  Type() const {
    return mType;
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

  NPObject*
  CreateProxyObject();

  
  
  
  bool ResurrectProxyObject();

private:
  PluginInstanceChild* mInstance;
  NPObject* mObject;
  bool mInvalidated;
  int mProtectCount;

  ScriptableObjectType mType;

  static const NPClass sNPClass;
};

} 
} 

#endif 
