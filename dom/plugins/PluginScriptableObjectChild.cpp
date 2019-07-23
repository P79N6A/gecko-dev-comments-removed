





































#include "mozilla/plugins/PluginScriptableObjectChild.h"

using mozilla::plugins::PluginScriptableObjectChild;

PluginScriptableObjectChild::PluginScriptableObjectChild()
{
}

PluginScriptableObjectChild::~PluginScriptableObjectChild()
{
}

nsresult
PluginScriptableObjectChild::AnswerInvalidate()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
PluginScriptableObjectChild::AnswerHasMethod(const NPRemoteIdentifier& aId,
                                             bool* aHasMethod)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
PluginScriptableObjectChild::AnswerInvoke(const NPRemoteIdentifier& aId,
                                          const nsTArray<NPVariant>& aArgs,
                                          NPVariant* aResult,
                                          bool* aSuccess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
PluginScriptableObjectChild::AnswerInvokeDefault(const NPRemoteIdentifier& aId,
                                                 const nsTArray<NPVariant>& aArgs,
                                                 NPVariant* aResult,
                                                 bool* aSuccess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
PluginScriptableObjectChild::AnswerHasProperty(const NPRemoteIdentifier& aId,
                                               bool* aHasProperty)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
PluginScriptableObjectChild::AnswerGetProperty(const NPRemoteIdentifier& aId,
                                               NPVariant* aResult,
                                               bool* aSuccess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
PluginScriptableObjectChild::AnswerSetProperty(const NPRemoteIdentifier& aId,
                                               const NPVariant& aValue,
                                               bool* aSuccess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
PluginScriptableObjectChild::AnswerRemoveProperty(const NPRemoteIdentifier& aId,
                                                  bool* aSuccess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
PluginScriptableObjectChild::AnswerEnumerate(nsTArray<NPRemoteIdentifier>* aProperties,
                                             bool* aSuccess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
PluginScriptableObjectChild::AnswerConstruct(const nsTArray<NPVariant>& aArgs,
                                             NPVariant* aResult,
                                             bool* aSuccess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
