






#ifndef mozilla_jsipc_JavaScriptBase_h__
#define mozilla_jsipc_JavaScriptBase_h__

#include "WrapperAnswer.h"
#include "WrapperOwner.h"
#include "mozilla/dom/DOMTypes.h"
#include "mozilla/jsipc/PJavaScript.h"

namespace mozilla {
namespace jsipc {

template<class Base>
class JavaScriptBase : public WrapperOwner, public WrapperAnswer, public Base
{
    typedef WrapperAnswer Answer;

  public:
    explicit JavaScriptBase(JSRuntime *rt)
      : JavaScriptShared(rt),
        WrapperOwner(rt),
        WrapperAnswer(rt)
    {}
    virtual ~JavaScriptBase() {}

    virtual void ActorDestroy(WrapperOwner::ActorDestroyReason why) {
        WrapperOwner::ActorDestroy(why);
    }

    

    bool AnswerPreventExtensions(const ObjectId &objId, ReturnStatus *rs) {
        return Answer::AnswerPreventExtensions(objId, rs);
    }
    bool AnswerGetPropertyDescriptor(const ObjectId &objId, const nsString &id,
                                     ReturnStatus *rs,
                                     PPropertyDescriptor *out) {
        return Answer::AnswerGetPropertyDescriptor(objId, id, rs, out);
    }
    bool AnswerGetOwnPropertyDescriptor(const ObjectId &objId,
                                        const nsString &id,
                                        ReturnStatus *rs,
                                        PPropertyDescriptor *out) {
        return Answer::AnswerGetOwnPropertyDescriptor(objId, id, rs, out);
    }
    bool AnswerDefineProperty(const ObjectId &objId, const nsString &id,
                              const PPropertyDescriptor &flags,
                              ReturnStatus *rs) {
        return Answer::AnswerDefineProperty(objId, id, flags, rs);
    }
    bool AnswerDelete(const ObjectId &objId, const nsString &id,
                      ReturnStatus *rs, bool *success) {
        return Answer::AnswerDelete(objId, id, rs, success);
    }

    bool AnswerHas(const ObjectId &objId, const nsString &id,
                   ReturnStatus *rs, bool *bp) {
        return Answer::AnswerHas(objId, id, rs, bp);
    }
    bool AnswerHasOwn(const ObjectId &objId, const nsString &id,
                      ReturnStatus *rs, bool *bp) {
        return Answer::AnswerHasOwn(objId, id, rs, bp);
    }
    bool AnswerGet(const ObjectId &objId, const ObjectVariant &receiverVar,
                   const nsString &id,
                   ReturnStatus *rs, JSVariant *result) {
        return Answer::AnswerGet(objId, receiverVar, id, rs, result);
    }
    bool AnswerSet(const ObjectId &objId, const ObjectVariant &receiverVar,
                   const nsString &id, const bool &strict,
                   const JSVariant &value, ReturnStatus *rs, JSVariant *result) {
        return Answer::AnswerSet(objId, receiverVar, id, strict, value, rs, result);
    }

    bool AnswerIsExtensible(const ObjectId &objId, ReturnStatus *rs,
                            bool *result) {
        return Answer::AnswerIsExtensible(objId, rs, result);
    }
    bool AnswerCallOrConstruct(const ObjectId &objId, const nsTArray<JSParam> &argv,
                               const bool &construct, ReturnStatus *rs, JSVariant *result,
                               nsTArray<JSParam> *outparams) {
        return Answer::AnswerCallOrConstruct(objId, argv, construct, rs, result, outparams);
    }
    bool AnswerHasInstance(const ObjectId &objId, const JSVariant &v, ReturnStatus *rs, bool *bp) {
        return Answer::AnswerHasInstance(objId, v, rs, bp);
    }
    bool AnswerObjectClassIs(const ObjectId &objId, const uint32_t &classValue,
                             bool *result) {
        return Answer::AnswerObjectClassIs(objId, classValue, result);
    }
    bool AnswerClassName(const ObjectId &objId, nsString *result) {
        return Answer::AnswerClassName(objId, result);
    }

    bool AnswerGetPropertyNames(const ObjectId &objId, const uint32_t &flags,
                                ReturnStatus *rs, nsTArray<nsString> *names) {
        return Answer::AnswerGetPropertyNames(objId, flags, rs, names);
    }
    bool AnswerInstanceOf(const ObjectId &objId, const JSIID &iid,
                          ReturnStatus *rs, bool *instanceof) {
        return Answer::AnswerInstanceOf(objId, iid, rs, instanceof);
    }
    bool AnswerDOMInstanceOf(const ObjectId &objId, const int &prototypeID, const int &depth,
                             ReturnStatus *rs, bool *instanceof) {
        return Answer::AnswerDOMInstanceOf(objId, prototypeID, depth, rs, instanceof);
    }

    bool RecvDropObject(const ObjectId &objId) {
        return Answer::RecvDropObject(objId);
    }

    

    bool SendDropObject(const ObjectId &objId) {
        return Base::SendDropObject(objId);
    }
    bool CallPreventExtensions(const ObjectId &objId, ReturnStatus *rs) {
        return Base::CallPreventExtensions(objId, rs);
    }
    bool CallGetPropertyDescriptor(const ObjectId &objId, const nsString &id,
                                     ReturnStatus *rs,
                                     PPropertyDescriptor *out) {
        return Base::CallGetPropertyDescriptor(objId, id, rs, out);
    }
    bool CallGetOwnPropertyDescriptor(const ObjectId &objId,
                                      const nsString &id,
                                      ReturnStatus *rs,
                                      PPropertyDescriptor *out) {
        return Base::CallGetOwnPropertyDescriptor(objId, id, rs, out);
    }
    bool CallDefineProperty(const ObjectId &objId, const nsString &id,
                            const PPropertyDescriptor &flags,
                              ReturnStatus *rs) {
        return Base::CallDefineProperty(objId, id, flags, rs);
    }
    bool CallDelete(const ObjectId &objId, const nsString &id,
                    ReturnStatus *rs, bool *success) {
        return Base::CallDelete(objId, id, rs, success);
    }

    bool CallHas(const ObjectId &objId, const nsString &id,
                   ReturnStatus *rs, bool *bp) {
        return Base::CallHas(objId, id, rs, bp);
    }
    bool CallHasOwn(const ObjectId &objId, const nsString &id,
                    ReturnStatus *rs, bool *bp) {
        return Base::CallHasOwn(objId, id, rs, bp);
    }
    bool CallGet(const ObjectId &objId, const ObjectVariant &receiverVar,
                 const nsString &id,
                 ReturnStatus *rs, JSVariant *result) {
        return Base::CallGet(objId, receiverVar, id, rs, result);
    }
    bool CallSet(const ObjectId &objId, const ObjectVariant &receiverVar,
                 const nsString &id, const bool &strict,
                 const JSVariant &value, ReturnStatus *rs, JSVariant *result) {
        return Base::CallSet(objId, receiverVar, id, strict, value, rs, result);
    }

    bool CallIsExtensible(const ObjectId &objId, ReturnStatus *rs,
                          bool *result) {
        return Base::CallIsExtensible(objId, rs, result);
    }
    bool CallCallOrConstruct(const ObjectId &objId, const nsTArray<JSParam> &argv,
                             const bool &construct, ReturnStatus *rs, JSVariant *result,
                             nsTArray<JSParam> *outparams) {
        return Base::CallCallOrConstruct(objId, argv, construct, rs, result, outparams);
    }
    bool CallHasInstance(const ObjectId &objId, const JSVariant &v, ReturnStatus *rs, bool *bp) {
        return Base::CallHasInstance(objId, v, rs, bp);
    }
    bool CallObjectClassIs(const ObjectId &objId, const uint32_t &classValue,
                           bool *result) {
        return Base::CallObjectClassIs(objId, classValue, result);
    }
    bool CallClassName(const ObjectId &objId, nsString *result) {
        return Base::CallClassName(objId, result);
    }

    bool CallGetPropertyNames(const ObjectId &objId, const uint32_t &flags,
                              ReturnStatus *rs, nsTArray<nsString> *names) {
        return Base::CallGetPropertyNames(objId, flags, rs, names);
    }
    bool CallInstanceOf(const ObjectId &objId, const JSIID &iid,
                        ReturnStatus *rs, bool *instanceof) {
        return Base::CallInstanceOf(objId, iid, rs, instanceof);
    }
    bool CallDOMInstanceOf(const ObjectId &objId, const int &prototypeID, const int &depth,
                           ReturnStatus *rs, bool *instanceof) {
        return Base::CallDOMInstanceOf(objId, prototypeID, depth, rs, instanceof);
    }

    

    virtual bool toObjectVariant(JSContext *cx, JSObject *obj, ObjectVariant *objVarp) {
        return WrapperOwner::toObjectVariant(cx, obj, objVarp);
    }
    virtual JSObject *fromObjectVariant(JSContext *cx, ObjectVariant objVar) {
        return WrapperOwner::fromObjectVariant(cx, objVar);
    }
};

} 
} 

#endif
