





































#ifndef dom_plugins_PluginScriptableObjectChild_h
#define dom_plugins_PluginScriptableObjectChild_h 1

#include "mozilla/plugins/PPluginScriptableObjectProtocolChild.h"

namespace mozilla {
namespace plugins {

class PluginScriptableObjectChild : public PPluginScriptableObjectProtocolChild
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
               const nsTArray<NPVariant>& aArgs,
               NPVariant* aResult,
               bool* aSuccess);

  virtual nsresult
  AnswerInvokeDefault(const NPRemoteIdentifier& aId,
                      const nsTArray<NPVariant>& aArgs,
                      NPVariant* aResult,
                      bool* aSuccess);

  virtual nsresult
  AnswerHasProperty(const NPRemoteIdentifier& aId,
                    bool* aHasProperty);

  virtual nsresult
  AnswerGetProperty(const NPRemoteIdentifier& aId,
                    NPVariant* aResult,
                    bool* aSuccess);

  virtual nsresult
  AnswerSetProperty(const NPRemoteIdentifier& aId,
                    const NPVariant& aValue,
                    bool* aSuccess);

  virtual nsresult
  AnswerRemoveProperty(const NPRemoteIdentifier& aId,
                       bool* aSuccess);

  virtual nsresult
  AnswerEnumerate(nsTArray<NPRemoteIdentifier>* aProperties,
                  bool* aSuccess);

  virtual nsresult
  AnswerConstruct(const nsTArray<NPVariant>& aArgs,
                  NPVariant* aResult,
                  bool* aSuccess);
};

} 
} 

#endif 
