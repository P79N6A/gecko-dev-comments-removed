





#ifndef dom_plugins_PluginIdentifierChild_h
#define dom_plugins_PluginIdentifierChild_h

#include "mozilla/plugins/PPluginIdentifierChild.h"
#include "npapi.h"
#include "npruntime.h"

#include "nsString.h"

namespace mozilla {
namespace plugins {

class PluginModuleChild;







class PluginIdentifierChild : public PPluginIdentifierChild
{
  friend class PluginModuleChild;
public:
  bool IsString()
  {
    return mIsString;
  }

  NPIdentifier ToNPIdentifier()
  {
    if (mCanonicalIdentifier) {
      return mCanonicalIdentifier;
    }

    NS_ASSERTION(mHashed, "Handing out an unhashed identifier?");
    return this;
  }

  void MakePermanent();

  class MOZ_STACK_CLASS StackIdentifier
  {
  public:
    StackIdentifier(PPluginIdentifierChild* actor)
      : mIdentifier(static_cast<PluginIdentifierChild*>(actor))
    {
      if (mIdentifier)
        mIdentifier->StartTemporary();
    }

    ~StackIdentifier() {
      if (mIdentifier)
        mIdentifier->FinishTemporary();
    }

    PluginIdentifierChild* operator->() { return mIdentifier; }

  private:
    PluginIdentifierChild* mIdentifier;
  };

protected:
  PluginIdentifierChild(bool aIsString)
    : mCanonicalIdentifier(nullptr)
    , mHashed(false)
    , mTemporaryRefs(0)
    , mIsString(aIsString)
  {
    MOZ_COUNT_CTOR(PluginIdentifierChild);
  }

  virtual ~PluginIdentifierChild()
  {
    MOZ_COUNT_DTOR(PluginIdentifierChild);
  }

  
  
  virtual PluginIdentifierChild* GetCanonical() = 0;
  virtual void Hash() = 0;
  virtual void Unhash() = 0;

private:
  void StartTemporary();
  void FinishTemporary();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  PluginIdentifierChild* mCanonicalIdentifier;
  bool mHashed;
  unsigned int mTemporaryRefs;
  bool mIsString;
};

class PluginIdentifierChildString : public PluginIdentifierChild
{
  friend class PluginModuleChild;
public:
  NPUTF8* ToString()
  {
    return ToNewCString(mString);
  }

protected:
  PluginIdentifierChildString(const nsCString& aString)
    : PluginIdentifierChild(true),
      mString(aString)
  { }

  virtual PluginIdentifierChild* GetCanonical();
  virtual void Hash();
  virtual void Unhash();

  nsCString mString;
};

class PluginIdentifierChildInt : public PluginIdentifierChild
{
  friend class PluginModuleChild;
public:
  int32_t ToInt()
  {
    return mInt;
  }

protected:
  PluginIdentifierChildInt(int32_t aInt)
    : PluginIdentifierChild(false),
      mInt(aInt)
  { }

  virtual PluginIdentifierChild* GetCanonical();
  virtual void Hash();
  virtual void Unhash();

  int32_t mInt;
};

} 
} 

#endif 
