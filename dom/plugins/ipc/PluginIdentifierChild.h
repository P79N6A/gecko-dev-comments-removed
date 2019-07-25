






































#ifndef dom_plugins_PluginIdentifierChild_h
#define dom_plugins_PluginIdentifierChild_h

#include "mozilla/plugins/PPluginIdentifierChild.h"
#include "mozilla/plugins/PluginModuleChild.h"

#include "nsStringGlue.h"

namespace mozilla {
namespace plugins {

class PluginIdentifierChild : public PPluginIdentifierChild
{
  friend class PluginModuleChild;
public:
  bool IsString()
  {
    return reinterpret_cast<intptr_t>(mCanonicalIdentifier) & 1;
  }

  NPIdentifier ToNPIdentifier()
  {
    return reinterpret_cast<PluginIdentifierChild*>(
      reinterpret_cast<intptr_t>(mCanonicalIdentifier) & ~1);
  }

protected:
  PluginIdentifierChild(bool aIsString)
    : ALLOW_THIS_IN_INITIALIZER_LIST(mCanonicalIdentifier(this))
  {
    MOZ_COUNT_CTOR(PluginIdentifierChild);
    if (aIsString) {
      SetIsString();
    }
  }

  virtual ~PluginIdentifierChild()
  {
    MOZ_COUNT_DTOR(PluginIdentifierChild);
  }

  void SetCanonicalIdentifier(PluginIdentifierChild* aIdentifier)
  {
    NS_ASSERTION(ToNPIdentifier() == this, "Already got one!");
    bool isString = IsString();
    mCanonicalIdentifier = aIdentifier;
    if (isString) {
      SetIsString();
    }
  }

private:
  void SetIsString()
  {
    mCanonicalIdentifier = reinterpret_cast<PluginIdentifierChild*>(
      reinterpret_cast<intptr_t>(mCanonicalIdentifier) | 1);
  }

  PluginIdentifierChild* mCanonicalIdentifier;
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

  int32_t mInt;
};

} 
} 

#endif 
