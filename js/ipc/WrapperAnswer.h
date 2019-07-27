






#ifndef mozilla_jsipc_WrapperAnswer_h_
#define mozilla_jsipc_WrapperAnswer_h_

#include "JavaScriptShared.h"

namespace mozilla {
namespace jsipc {

class WrapperAnswer : public virtual JavaScriptShared
{
  public:
    explicit WrapperAnswer(JSRuntime *rt) : JavaScriptShared(rt) {}

    bool AnswerPreventExtensions(const ObjectId &objId, ReturnStatus *rs);
    bool AnswerGetPropertyDescriptor(const ObjectId &objId, const JSIDVariant &id,
                                     ReturnStatus *rs,
                                     PPropertyDescriptor *out);
    bool AnswerGetOwnPropertyDescriptor(const ObjectId &objId,
                                        const JSIDVariant &id,
                                        ReturnStatus *rs,
                                        PPropertyDescriptor *out);
    bool AnswerDefineProperty(const ObjectId &objId, const JSIDVariant &id,
                              const PPropertyDescriptor &flags,
                              ReturnStatus *rs);
    bool AnswerDelete(const ObjectId &objId, const JSIDVariant &id,
                      ReturnStatus *rs, bool *success);

    bool AnswerHas(const ObjectId &objId, const JSIDVariant &id,
                       ReturnStatus *rs, bool *bp);
    bool AnswerHasOwn(const ObjectId &objId, const JSIDVariant &id,
                          ReturnStatus *rs, bool *bp);
    bool AnswerGet(const ObjectId &objId, const ObjectVariant &receiverVar,
                       const JSIDVariant &id,
                       ReturnStatus *rs, JSVariant *result);
    bool AnswerSet(const ObjectId &objId, const ObjectVariant &receiverVar,
                   const JSIDVariant &id, const bool &strict,
                   const JSVariant &value, ReturnStatus *rs, JSVariant *result);

    bool AnswerIsExtensible(const ObjectId &objId, ReturnStatus *rs,
                            bool *result);
    bool AnswerCallOrConstruct(const ObjectId &objId, const nsTArray<JSParam> &argv,
                               const bool &construct, ReturnStatus *rs, JSVariant *result,
                               nsTArray<JSParam> *outparams);
    bool AnswerHasInstance(const ObjectId &objId, const JSVariant &v, ReturnStatus *rs, bool *bp);
    bool AnswerObjectClassIs(const ObjectId &objId, const uint32_t &classValue,
                             bool *result);
    bool AnswerClassName(const ObjectId &objId, nsString *result);
    bool AnswerRegExpToShared(const ObjectId &objId, ReturnStatus *rs, nsString *source, uint32_t *flags);

    bool AnswerGetPropertyNames(const ObjectId &objId, const uint32_t &flags,
                                ReturnStatus *rs, nsTArray<nsString> *names);
    bool AnswerInstanceOf(const ObjectId &objId, const JSIID &iid,
                          ReturnStatus *rs, bool *instanceof);
    bool AnswerDOMInstanceOf(const ObjectId &objId, const int &prototypeID, const int &depth,
                             ReturnStatus *rs, bool *instanceof);

    bool AnswerIsCallable(const ObjectId &objId, bool *result);
    bool AnswerIsConstructor(const ObjectId &objId, bool *result);

    bool RecvDropObject(const ObjectId &objId);

  private:
    bool fail(JSContext *cx, ReturnStatus *rs);
    bool ok(ReturnStatus *rs);
};

} 
} 

#endif
