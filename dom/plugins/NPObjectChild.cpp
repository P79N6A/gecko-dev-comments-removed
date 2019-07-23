





































#include "mozilla/plugins/NPObjectChild.h"

using mozilla::plugins::NPObjectChild;

NPObjectChild::NPObjectChild()
{

}

NPObjectChild::~NPObjectChild()
{

}

nsresult
NPObjectChild::AnswerInvalidate()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NPObjectChild::AnswerHasMethod(const NPRemoteIdentifier& aId,
                               bool* aHasMethod)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NPObjectChild::AnswerInvoke(const NPRemoteIdentifier& aId,
                            const nsTArray<NPVariant>& aArgs,
                            NPVariant* aResult,
                            bool* aSuccess)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NPObjectChild::AnswerInvokeDefault(const NPRemoteIdentifier& aId,
                                   const nsTArray<NPVariant>& aArgs,
                                   NPVariant* aResult,
                                   bool* aSuccess)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NPObjectChild::AnswerHasProperty(const NPRemoteIdentifier& aId,
                                 bool* aHasProperty)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NPObjectChild::AnswerGetProperty(const NPRemoteIdentifier& aId,
                                 NPVariant* aResult,
                                 bool* aSuccess)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NPObjectChild::AnswerSetProperty(const NPRemoteIdentifier& aId,
                                 const NPVariant& aValue,
                                 bool* aSuccess)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NPObjectChild::AnswerRemoveProperty(const NPRemoteIdentifier& aId,
                                    bool* aSuccess)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NPObjectChild::AnswerEnumerate(nsTArray<NPRemoteIdentifier>* aProperties,
                               bool* aSuccess)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NPObjectChild::AnswerConstruct(const nsTArray<NPVariant>& aArgs,
                               NPVariant* aResult,
                               bool* aSuccess)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
