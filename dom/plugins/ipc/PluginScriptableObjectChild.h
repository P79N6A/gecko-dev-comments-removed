





#ifndef dom_plugins_PluginScriptableObjectChild_h
#define dom_plugins_PluginScriptableObjectChild_h 1

#include "mozilla/plugins/PPluginScriptableObjectChild.h"

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
  PluginScriptableObjectChild(ScriptableObjectType aType);
  virtual ~PluginScriptableObjectChild();

  void
  InitializeProxy();

  void
  InitializeLocal(NPObject* aObject);


  virtual bool
  AnswerInvalidate();

  virtual bool
  AnswerHasMethod(PPluginIdentifierChild* aId,
                  bool* aHasMethod);

  virtual bool
  AnswerInvoke(PPluginIdentifierChild* aId,
               const InfallibleTArray<Variant>& aArgs,
               Variant* aResult,
               bool* aSuccess);

  virtual bool
  AnswerInvokeDefault(const InfallibleTArray<Variant>& aArgs,
                      Variant* aResult,
                      bool* aSuccess);

  virtual bool
  AnswerHasProperty(PPluginIdentifierChild* aId,
                    bool* aHasProperty);

  virtual bool
  AnswerGetChildProperty(PPluginIdentifierChild* aId,
                         bool* aHasProperty,
                         bool* aHasMethod,
                         Variant* aResult,
                         bool* aSuccess);

  virtual bool
  AnswerSetProperty(PPluginIdentifierChild* aId,
                    const Variant& aValue,
                    bool* aSuccess);

  virtual bool
  AnswerRemoveProperty(PPluginIdentifierChild* aId,
                       bool* aSuccess);

  virtual bool
  AnswerEnumerate(InfallibleTArray<PPluginIdentifierChild*>* aProperties,
                  bool* aSuccess);

  virtual bool
  AnswerConstruct(const InfallibleTArray<Variant>& aArgs,
                  Variant* aResult,
                  bool* aSuccess);

  virtual bool
  RecvProtect();

  virtual bool
  RecvUnprotect();

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
