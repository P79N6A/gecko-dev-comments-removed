





#ifndef dom_plugins_PluginScriptableObjectChild_h
#define dom_plugins_PluginScriptableObjectChild_h 1

#include "mozilla/plugins/PPluginScriptableObjectChild.h"
#include "mozilla/plugins/PluginMessageUtils.h"
#include "mozilla/plugins/PluginTypes.h"

#include "npruntime.h"
#include "nsDataHashtable.h"

namespace mozilla {
namespace plugins {

class PluginInstanceChild;
class PluginScriptableObjectChild;

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
  AnswerHasMethod(const PluginIdentifier& aId,
                  bool* aHasMethod) MOZ_OVERRIDE;

  virtual bool
  AnswerInvoke(const PluginIdentifier& aId,
               const InfallibleTArray<Variant>& aArgs,
               Variant* aResult,
               bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerInvokeDefault(const InfallibleTArray<Variant>& aArgs,
                      Variant* aResult,
                      bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerHasProperty(const PluginIdentifier& aId,
                    bool* aHasProperty) MOZ_OVERRIDE;

  virtual bool
  AnswerGetChildProperty(const PluginIdentifier& aId,
                         bool* aHasProperty,
                         bool* aHasMethod,
                         Variant* aResult,
                         bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerSetProperty(const PluginIdentifier& aId,
                    const Variant& aValue,
                    bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerRemoveProperty(const PluginIdentifier& aId,
                       bool* aSuccess) MOZ_OVERRIDE;

  virtual bool
  AnswerEnumerate(InfallibleTArray<PluginIdentifier>* aProperties,
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
  struct StoredIdentifier
  {
    nsCString mIdentifier;
    nsAutoRefCnt mRefCnt;
    bool mPermanent;

    nsrefcnt AddRef() {
      ++mRefCnt;
      return mRefCnt;
    }

    nsrefcnt Release() {
      --mRefCnt;
      if (mRefCnt == 0) {
        delete this;
        return 0;
      }
      return mRefCnt;
    }

    explicit StoredIdentifier(const nsCString& aIdentifier)
      : mIdentifier(aIdentifier), mRefCnt(), mPermanent(false)
    { MOZ_COUNT_CTOR(StoredIdentifier); }

    ~StoredIdentifier() { MOZ_COUNT_DTOR(StoredIdentifier); }
  };

public:
  class MOZ_STACK_CLASS StackIdentifier
  {
  public:
    StackIdentifier(const PluginIdentifier& aIdentifier);
    StackIdentifier(NPIdentifier aIdentifier);
    ~StackIdentifier();

    void MakePermanent()
    {
      if (mStored) {
        mStored->mPermanent = true;
      }
    }
    NPIdentifier ToNPIdentifier() const;

    bool IsString() const { return mIdentifier.type() == PluginIdentifier::TnsCString; }
    const nsCString& GetString() const { return mIdentifier.get_nsCString(); }

    int32_t GetInt() const { return mIdentifier.get_int32_t(); }

    PluginIdentifier GetIdentifier() const { return mIdentifier; }

   private:
    DISALLOW_COPY_AND_ASSIGN(StackIdentifier);

    PluginIdentifier mIdentifier;
    nsRefPtr<StoredIdentifier> mStored;
  };

  static void ClearIdentifiers();

  bool RegisterActor(NPObject* aObject);
  void UnregisterActor(NPObject* aObject);

  static PluginScriptableObjectChild* GetActorForNPObject(NPObject* aObject);

  static void RegisterObject(NPObject* aObject, PluginInstanceChild* aInstance);
  static void UnregisterObject(NPObject* aObject);

  static PluginInstanceChild* GetInstanceForNPObject(NPObject* aObject);

  



  static void NotifyOfInstanceShutdown(PluginInstanceChild* aInstance);

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

  static StoredIdentifier* HashIdentifier(const nsCString& aIdentifier);
  static void UnhashIdentifier(StoredIdentifier* aIdentifier);

  typedef nsDataHashtable<nsCStringHashKey, nsRefPtr<StoredIdentifier>> IdentifierTable;
  static IdentifierTable sIdentifiers;

  struct NPObjectData : public nsPtrHashKey<NPObject>
  {
    explicit NPObjectData(const NPObject* key)
    : nsPtrHashKey<NPObject>(key),
      instance(nullptr),
      actor(nullptr)
    { }

    
    PluginInstanceChild* instance;

    
    PluginScriptableObjectChild* actor;
  };

  static PLDHashOperator CollectForInstance(NPObjectData* d, void* userArg);

  



  static nsTHashtable<NPObjectData>* sObjectMap;
};

} 
} 

#endif 
