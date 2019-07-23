





































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

  virtual nsresult
  AnswerInvalidate();

  virtual nsresult
  AnswerHasMethod(const NPRemoteIdentifier& aId,
                  bool* aHasMethod);

  virtual nsresult
  AnswerInvoke(const NPRemoteIdentifier& aId,
               const nsTArray<NPRemoteVariant>& aArgs,
               NPRemoteVariant* aResult,
               bool* aSuccess);

  virtual nsresult
  AnswerInvokeDefault(const nsTArray<NPRemoteVariant>& aArgs,
                      NPRemoteVariant* aResult,
                      bool* aSuccess);

  virtual nsresult
  AnswerHasProperty(const NPRemoteIdentifier& aId,
                    bool* aHasProperty);

  virtual nsresult
  AnswerGetProperty(const NPRemoteIdentifier& aId,
                    NPRemoteVariant* aResult,
                    bool* aSuccess);

  virtual nsresult
  AnswerSetProperty(const NPRemoteIdentifier& aId,
                    const NPRemoteVariant& aValue,
                    bool* aSuccess);

  virtual nsresult
  AnswerRemoveProperty(const NPRemoteIdentifier& aId,
                       bool* aSuccess);

  virtual nsresult
  AnswerEnumerate(nsTArray<NPRemoteIdentifier>* aProperties,
                  bool* aSuccess);

  virtual nsresult
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
