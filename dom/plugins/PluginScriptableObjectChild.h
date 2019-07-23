





































#ifndef dom_plugins_PluginScriptableObjectChild_h
#define dom_plugins_PluginScriptableObjectChild_h 1

#include "mozilla/plugins/PPluginScriptableObjectChild.h"

struct NPObject;

namespace mozilla {
namespace plugins {

class PluginInstanceChild;

class PluginScriptableObjectChild : public PPluginScriptableObjectChild
{
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
               const nsTArray<NPRemoteVariant>& aArgs,
               NPRemoteVariant* aResult,
               bool* aSuccess);

  virtual bool
  AnswerInvokeDefault(const nsTArray<NPRemoteVariant>& aArgs,
                      NPRemoteVariant* aResult,
                      bool* aSuccess);

  virtual bool
  AnswerHasProperty(const NPRemoteIdentifier& aId,
                    bool* aHasProperty);

  virtual bool
  AnswerGetProperty(const NPRemoteIdentifier& aId,
                    NPRemoteVariant* aResult,
                    bool* aSuccess);

  virtual bool
  AnswerSetProperty(const NPRemoteIdentifier& aId,
                    const NPRemoteVariant& aValue,
                    bool* aSuccess);

  virtual bool
  AnswerRemoveProperty(const NPRemoteIdentifier& aId,
                       bool* aSuccess);

  virtual bool
  AnswerEnumerate(nsTArray<NPRemoteIdentifier>* aProperties,
                  bool* aSuccess);

  virtual bool
  AnswerConstruct(const nsTArray<NPRemoteVariant>& aArgs,
                  NPRemoteVariant* aResult,
                  bool* aSuccess);

  void
  Initialize(PluginInstanceChild* aInstance,
             NPObject* aObject);

  NPObject*
  GetObject()
  {
    return mObject;
  }

private:
  PluginInstanceChild* mInstance;
  NPObject* mObject;
};

} 
} 

#endif 
