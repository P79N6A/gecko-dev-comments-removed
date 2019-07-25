






































#ifndef dom_plugins_PluginIdentifierParent_h
#define dom_plugins_PluginIdentifierParent_h

#include "mozilla/plugins/PPluginIdentifierParent.h"

#include "npapi.h"
#include "npruntime.h"

namespace mozilla {
namespace plugins {

class PluginInstanceParent;

class PluginIdentifierParent : public PPluginIdentifierParent
{
  friend class PluginModuleParent;

public:
  NPIdentifier ToNPIdentifier()
  {
    return mIdentifier;
  }

  bool IsTemporary() {
    return !!mTemporaryRefs;
  }

  


  class NS_STACK_CLASS StackIdentifier
  {
  public:
    StackIdentifier(PluginInstanceParent* inst, NPIdentifier aIdentifier);
    StackIdentifier(NPObject* aObject, NPIdentifier aIdentifier);
    ~StackIdentifier();

    operator PluginIdentifierParent*() {
      return mIdentifier;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(StackIdentifier);

    PluginIdentifierParent* mIdentifier;
  };

protected:
  PluginIdentifierParent(NPIdentifier aIdentifier, bool aTemporary)
    : mIdentifier(aIdentifier)
    , mTemporaryRefs(aTemporary ? 1 : 0)
  {
    MOZ_COUNT_CTOR(PluginIdentifierParent);
  }

  virtual ~PluginIdentifierParent()
  {
    MOZ_COUNT_DTOR(PluginIdentifierParent);
  }

  virtual bool RecvRetain();

  void AddTemporaryRef() {
    mTemporaryRefs++;
  }

  


  bool RemoveTemporaryRef() {
    --mTemporaryRefs;
    return !mTemporaryRefs;
  }

private:
  NPIdentifier mIdentifier;
  unsigned int mTemporaryRefs;
};

} 
} 

#endif 
