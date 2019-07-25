







































#ifndef mozilla_jsipc_ObjectWrapperChild_h__
#define mozilla_jsipc_ObjectWrapperChild_h__

#include "mozilla/jsipc/PObjectWrapperChild.h"

using mozilla::jsipc::JSVariant;

namespace mozilla {
namespace jsipc {

class ContextWrapperChild;
  
class ObjectWrapperChild
    : public PObjectWrapperChild
{
public:

    ObjectWrapperChild(JSContext* cx, JSObject* obj);

    JSObject* GetJSObject() const { return mObj; }
    
private:

    JSObject* const mObj;

    bool JSObject_to_JSVariant(JSContext* cx, JSObject* from, JSVariant* to);
    bool jsval_to_JSVariant(JSContext* cx, jsval from, JSVariant* to);

    static bool JSObject_from_PObjectWrapperChild(JSContext* cx,
                                                  const PObjectWrapperChild* from,
                                                  JSObject** to);
    static bool JSObject_from_JSVariant(JSContext* cx, const JSVariant& from,
                                        JSObject** to);
    static bool jsval_from_JSVariant(JSContext* cx, const JSVariant& from,
                                     jsval* to);

    ContextWrapperChild* Manager();

protected:

    void ActorDestroy(ActorDestroyReason why);

    bool AnswerAddProperty(const nsString& id,
                           JSBool* ok);

    bool AnswerGetProperty(const nsString& id,
                           JSBool* ok, JSVariant* vp);

    bool AnswerSetProperty(const nsString& id, const JSVariant& v,
                           JSBool* ok, JSVariant* vp);

    bool AnswerDelProperty(const nsString& id,
                           JSBool* ok, JSVariant* vp);

    bool AnswerNewEnumerateInit(
                                JSBool* ok, JSVariant* statep, int* idp);

    bool AnswerNewEnumerateNext(const JSVariant& in_state,
                                JSBool* ok, JSVariant* statep, nsString* idp);

    bool RecvNewEnumerateDestroy(const JSVariant& in_state);

    bool AnswerNewResolve(const nsString& id, const int& flags,
                          JSBool* ok, PObjectWrapperChild** obj2);

    bool AnswerConvert(const JSType& type,
                       JSBool* ok, JSVariant* vp);

    bool AnswerCall(PObjectWrapperChild* receiver, const nsTArray<JSVariant>& argv,
                    JSBool* ok, JSVariant* rval);

    bool AnswerConstruct(const nsTArray<JSVariant>& argv,
                         JSBool* ok, PObjectWrapperChild** rval);

    bool AnswerHasInstance(const JSVariant& v,
                           JSBool* ok, JSBool* bp);
};

}}
  
#endif
