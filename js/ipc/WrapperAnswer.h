






#ifndef mozilla_jsipc_WrapperAnswer_h_
#define mozilla_jsipc_WrapperAnswer_h_

#include "JavaScriptShared.h"

namespace mozilla {
namespace jsipc {

class WrapperAnswer : public virtual JavaScriptShared
{
  public:
    explicit WrapperAnswer(JSRuntime *rt) : JavaScriptShared(rt) {}

    bool RecvPreventExtensions(const ObjectId &objId, ReturnStatus *rs,
                               bool *succeeded);
    bool RecvGetPropertyDescriptor(const ObjectId &objId, const JSIDVariant &id,
                                   ReturnStatus *rs,
                                   PPropertyDescriptor *out);
    bool RecvGetOwnPropertyDescriptor(const ObjectId &objId,
                                      const JSIDVariant &id,
                                      ReturnStatus *rs,
                                      PPropertyDescriptor *out);
    bool RecvDefineProperty(const ObjectId &objId, const JSIDVariant &id,
                            const PPropertyDescriptor &flags,
                            ReturnStatus *rs);
    bool RecvDelete(const ObjectId &objId, const JSIDVariant &id,
                    ReturnStatus *rs, bool *success);

    bool RecvHas(const ObjectId &objId, const JSIDVariant &id,
                 ReturnStatus *rs, bool *bp);
    bool RecvHasOwn(const ObjectId &objId, const JSIDVariant &id,
                    ReturnStatus *rs, bool *bp);
    bool RecvGet(const ObjectId &objId, const ObjectVariant &receiverVar,
                 const JSIDVariant &id,
                 ReturnStatus *rs, JSVariant *result);
    bool RecvSet(const ObjectId &objId, const ObjectVariant &receiverVar,
                 const JSIDVariant &id, const bool &strict,
                 const JSVariant &value, ReturnStatus *rs, JSVariant *result);

    bool RecvIsExtensible(const ObjectId &objId, ReturnStatus *rs,
                          bool *result);
    bool RecvCallOrConstruct(const ObjectId &objId, InfallibleTArray<JSParam> &&argv,
                             const bool &construct, ReturnStatus *rs, JSVariant *result,
                             nsTArray<JSParam> *outparams);
    bool RecvHasInstance(const ObjectId &objId, const JSVariant &v, ReturnStatus *rs, bool *bp);
    bool RecvObjectClassIs(const ObjectId &objId, const uint32_t &classValue,
                           bool *result);
    bool RecvClassName(const ObjectId &objId, nsString *result);
    bool RecvRegExpToShared(const ObjectId &objId, ReturnStatus *rs, nsString *source, uint32_t *flags);

    bool RecvGetPropertyKeys(const ObjectId &objId, const uint32_t &flags,
                             ReturnStatus *rs, nsTArray<JSIDVariant> *ids);
    bool RecvInstanceOf(const ObjectId &objId, const JSIID &iid,
                        ReturnStatus *rs, bool *instanceof);
    bool RecvDOMInstanceOf(const ObjectId &objId, const int &prototypeID, const int &depth,
                           ReturnStatus *rs, bool *instanceof);

    bool RecvDropObject(const ObjectId &objId);

  private:
    bool fail(JSContext *cx, ReturnStatus *rs);
    bool ok(ReturnStatus *rs);
};

} 
} 

#endif
