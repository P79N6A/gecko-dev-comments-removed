







































#ifndef mozilla_jsipc_ObjectWrapperParent_h__
#define mozilla_jsipc_ObjectWrapperParent_h__

#include "mozilla/jsipc/PObjectWrapperParent.h"
#include "jsapi.h"
#include "jsclass.h"
#include "nsAutoJSValHolder.h"

namespace mozilla {
namespace jsipc {

class ContextWrapperParent;

class OperationChecker {
public:
    virtual void CheckOperation(JSContext* cx,
                                OperationStatus* status) = 0;
};

class ObjectWrapperParent
    : public PObjectWrapperParent
    , public OperationChecker
{
public:

    ObjectWrapperParent()
        : mObj(NULL)
    {}

    JSObject* GetJSObject(JSContext* cx) const;

    jsval GetJSVal(JSContext* cx) const {
        return OBJECT_TO_JSVAL(GetJSObject(cx));
    }

    void CheckOperation(JSContext* cx,
                        OperationStatus* status);

    static const js::Class sCPOW_JSClass;

protected:

    void ActorDestroy(ActorDestroyReason why);

    ContextWrapperParent* Manager();

private:

    mutable JSObject* mObj;

    static JSBool
    CPOW_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

    static JSBool
    CPOW_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

    static JSBool
    CPOW_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);
    
    static JSBool
    CPOW_SetProperty(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

    JSBool NewEnumerateInit(JSContext* cx, jsval* statep, jsid* idp);
    JSBool NewEnumerateNext(JSContext* cx, jsval* statep, jsid* idp);
    JSBool NewEnumerateDestroy(JSContext* cx, jsval state);
    static JSBool
    CPOW_NewEnumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                      jsval *statep, jsid *idp);

    static JSBool
    CPOW_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                    JSObject **objp);

    static JSBool
    CPOW_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

    static void
    CPOW_Finalize(JSContext* cx, JSObject* obj);

    static JSBool
    CPOW_Call(JSContext* cx, uintN argc, jsval* vp);

    static JSBool
    CPOW_Construct(JSContext *cx, uintN argc, jsval *vp);
    
    static JSBool
    CPOW_HasInstance(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);

    static JSBool
    CPOW_Equality(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);

    static bool jsval_to_JSVariant(JSContext* cx, jsval from, JSVariant* to);
    static bool jsval_from_JSVariant(JSContext* cx, const JSVariant& from,
                                     jsval* to);
    static bool
    JSObject_to_PObjectWrapperParent(JSContext* cx, JSObject* from,
                                     PObjectWrapperParent** to);
    static bool
    JSObject_from_PObjectWrapperParent(JSContext* cx,
                                       const PObjectWrapperParent* from,
                                       JSObject** to);
    static bool
    jsval_from_PObjectWrapperParent(JSContext* cx,
                                    const PObjectWrapperParent* from,
                                    jsval* to);
};

template <class StatusOwnerPolicy>
class AutoCheckOperationBase
    : public StatusOwnerPolicy
{
    JSContext* const mContext;
    OperationChecker* const mChecker;

protected:

    AutoCheckOperationBase(JSContext* cx,
                           OperationChecker* checker)
        : mContext(cx)
        , mChecker(checker)
    {}

    virtual ~AutoCheckOperationBase() {
        mChecker->CheckOperation(mContext, StatusOwnerPolicy::StatusPtr());
    }

public:

    bool Ok() {
        return (StatusOwnerPolicy::StatusPtr()->type() == OperationStatus::TJSBool &&
                StatusOwnerPolicy::StatusPtr()->get_JSBool());
    }
};

}}

#endif
