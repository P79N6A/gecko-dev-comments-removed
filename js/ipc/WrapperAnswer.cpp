






#include "WrapperAnswer.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsContentUtils.h"
#include "xpcprivate.h"
#include "jsfriendapi.h"
#include "nsCxPusher.h"

using namespace JS;
using namespace mozilla;
using namespace mozilla::jsipc;

using mozilla::AutoSafeJSContext;

bool
WrapperAnswer::fail(JSContext *cx, ReturnStatus *rs)
{
    
    
    *rs = ReturnStatus(ReturnException(JSVariant(UndefinedVariant())));

    
    
    

    RootedValue exn(cx);
    if (!JS_GetPendingException(cx, &exn))
        return true;

    
    
    
    JS_ClearPendingException(cx);

    if (JS_IsStopIteration(exn)) {
        *rs = ReturnStatus(ReturnStopIteration());
        return true;
    }

    
    
    (void) toVariant(cx, exn, &rs->get_ReturnException().exn());
    return true;
}

bool
WrapperAnswer::ok(ReturnStatus *rs)
{
    *rs = ReturnStatus(ReturnSuccess());
    return true;
}

bool
WrapperAnswer::AnswerPreventExtensions(const ObjectId &objId, ReturnStatus *rs)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);
    if (!JS_PreventExtensions(cx, obj))
        return fail(cx, rs);

    return ok(rs);
}

static void
EmptyDesc(PPropertyDescriptor *desc)
{
    desc->obj() = LocalObject(0);
    desc->attrs() = 0;
    desc->value() = UndefinedVariant();
    desc->getter() = 0;
    desc->setter() = 0;
}

bool
WrapperAnswer::AnswerGetPropertyDescriptor(const ObjectId &objId, const nsString &id,
					   ReturnStatus *rs, PPropertyDescriptor *out)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    EmptyDesc(out);

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    RootedId internedId(cx);
    if (!convertGeckoStringToId(cx, id, &internedId))
        return fail(cx, rs);

    Rooted<JSPropertyDescriptor> desc(cx);
    if (!JS_GetPropertyDescriptorById(cx, obj, internedId, &desc))
        return fail(cx, rs);

    if (!desc.object())
        return ok(rs);

    if (!fromDescriptor(cx, desc, out))
        return fail(cx, rs);

    return ok(rs);
}

bool
WrapperAnswer::AnswerGetOwnPropertyDescriptor(const ObjectId &objId, const nsString &id,
					      ReturnStatus *rs, PPropertyDescriptor *out)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    EmptyDesc(out);

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    RootedId internedId(cx);
    if (!convertGeckoStringToId(cx, id, &internedId))
        return fail(cx, rs);

    Rooted<JSPropertyDescriptor> desc(cx);
    if (!JS_GetPropertyDescriptorById(cx, obj, internedId, &desc))
        return fail(cx, rs);

    if (desc.object() != obj)
        return ok(rs);

    if (!fromDescriptor(cx, desc, out))
        return fail(cx, rs);

    return ok(rs);
}

bool
WrapperAnswer::AnswerDefineProperty(const ObjectId &objId, const nsString &id,
				    const PPropertyDescriptor &descriptor, ReturnStatus *rs)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    RootedId internedId(cx);
    if (!convertGeckoStringToId(cx, id, &internedId))
        return fail(cx, rs);

    Rooted<JSPropertyDescriptor> desc(cx);
    if (!toDescriptor(cx, descriptor, &desc))
        return false;

    if (!js::CheckDefineProperty(cx, obj, internedId, desc.value(), desc.attributes(),
                                 desc.getter(), desc.setter()))
    {
        return fail(cx, rs);
    }

    if (!JS_DefinePropertyById(cx, obj, internedId, desc.value(), desc.attributes(),
                               desc.getter(), desc.setter()))
    {
        return fail(cx, rs);
    }

    return ok(rs);
}

bool
WrapperAnswer::AnswerDelete(const ObjectId &objId, const nsString &id, ReturnStatus *rs,
			    bool *success)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    *success = false;

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    RootedId internedId(cx);
    if (!convertGeckoStringToId(cx, id, &internedId))
        return fail(cx, rs);

    if (!JS_DeletePropertyById2(cx, obj, internedId, success))
        return fail(cx, rs);

    return ok(rs);
}

bool
WrapperAnswer::AnswerHas(const ObjectId &objId, const nsString &id, ReturnStatus *rs, bool *bp)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    *bp = false;

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    RootedId internedId(cx);
    if (!convertGeckoStringToId(cx, id, &internedId))
        return fail(cx, rs);

    bool found;
    if (!JS_HasPropertyById(cx, obj, internedId, &found))
        return fail(cx, rs);
    *bp = !!found;

    return ok(rs);
}

bool
WrapperAnswer::AnswerHasOwn(const ObjectId &objId, const nsString &id, ReturnStatus *rs, bool *bp)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    *bp = false;

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    RootedId internedId(cx);
    if (!convertGeckoStringToId(cx, id, &internedId))
        return fail(cx, rs);

    Rooted<JSPropertyDescriptor> desc(cx);
    if (!JS_GetPropertyDescriptorById(cx, obj, internedId, &desc))
        return fail(cx, rs);
    *bp = (desc.object() == obj);

    return ok(rs);
}

bool
WrapperAnswer::AnswerGet(const ObjectId &objId, const ObjectId &receiverId, const nsString &id,
			 ReturnStatus *rs, JSVariant *result)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    
    
    *result = UndefinedVariant();

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    RootedObject receiver(cx, findObjectById(receiverId));
    if (!receiver)
        return false;

    JSAutoCompartment comp(cx, obj);

    RootedId internedId(cx);
    if (!convertGeckoStringToId(cx, id, &internedId))
        return fail(cx, rs);

    JS::RootedValue val(cx);
    if (!JS_ForwardGetPropertyTo(cx, obj, internedId, receiver, &val))
        return fail(cx, rs);

    if (!toVariant(cx, val, result))
        return fail(cx, rs);

    return ok(rs);
}

bool
WrapperAnswer::AnswerSet(const ObjectId &objId, const ObjectId &receiverId, const nsString &id,
			 const bool &strict, const JSVariant &value, ReturnStatus *rs,
			 JSVariant *result)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    
    
    *result = UndefinedVariant();

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    RootedObject receiver(cx, findObjectById(receiverId));
    if (!receiver)
        return false;

    JSAutoCompartment comp(cx, obj);

    RootedId internedId(cx);
    if (!convertGeckoStringToId(cx, id, &internedId))
        return fail(cx, rs);

    MOZ_ASSERT(obj == receiver);

    RootedValue val(cx);
    if (!fromVariant(cx, value, &val))
        return fail(cx, rs);

    if (!JS_SetPropertyById(cx, obj, internedId, val))
        return fail(cx, rs);

    if (!toVariant(cx, val, result))
        return fail(cx, rs);

    return ok(rs);
}

bool
WrapperAnswer::AnswerIsExtensible(const ObjectId &objId, ReturnStatus *rs, bool *result)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    *result = false;

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    bool extensible;
    if (!JS_IsExtensible(cx, obj, &extensible))
        return fail(cx, rs);

    *result = !!extensible;
    return ok(rs);
}

bool
WrapperAnswer::AnswerCall(const ObjectId &objId, const nsTArray<JSParam> &argv, ReturnStatus *rs,
			  JSVariant *result, nsTArray<JSParam> *outparams)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    
    
    *result = UndefinedVariant();

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    MOZ_ASSERT(argv.Length() >= 2);

    RootedValue objv(cx);
    if (!fromVariant(cx, argv[0], &objv))
        return fail(cx, rs);

    *result = JSVariant(UndefinedVariant());

    AutoValueVector vals(cx);
    AutoValueVector outobjects(cx);
    for (size_t i = 0; i < argv.Length(); i++) {
        if (argv[i].type() == JSParam::Tvoid_t) {
            
            JSCompartment *compartment = js::GetContextCompartment(cx);
            RootedObject global(cx, JS_GetGlobalForCompartmentOrNull(cx, compartment));
            RootedObject obj(cx, xpc::NewOutObject(cx, global));
            if (!obj)
                return fail(cx, rs);
            if (!outobjects.append(ObjectValue(*obj)))
                return fail(cx, rs);
            if (!vals.append(ObjectValue(*obj)))
                return fail(cx, rs);
        } else {
            RootedValue v(cx);
            if (!fromVariant(cx, argv[i].get_JSVariant(), &v))
                return fail(cx, rs);
            if (!vals.append(v))
                return fail(cx, rs);
        }
    }

    RootedValue rval(cx);
    {
        AutoSaveContextOptions asco(cx);
        ContextOptionsRef(cx).setDontReportUncaught(true);

        HandleValueArray args = HandleValueArray::subarray(vals, 2, vals.length() - 2);
        bool success = JS::Call(cx, vals[1], vals[0], args, &rval);
        if (!success)
            return fail(cx, rs);
    }

    if (!toVariant(cx, rval, result))
        return fail(cx, rs);

    
    for (size_t i = 0; i < outobjects.length(); i++)
        outparams->AppendElement(JSParam(void_t()));

    
    
    
    vals.clear();
    for (size_t i = 0; i < outobjects.length(); i++) {
        RootedObject obj(cx, &outobjects[i].toObject());

        RootedValue v(cx);
        bool found;
        if (JS_HasProperty(cx, obj, "value", &found)) {
            if (!JS_GetProperty(cx, obj, "value", &v))
                return fail(cx, rs);
        } else {
            v = UndefinedValue();
        }
        if (!vals.append(v))
            return fail(cx, rs);
    }

    
    
    for (size_t i = 0; i < vals.length(); i++) {
        JSVariant variant;
        if (!toVariant(cx, vals[i], &variant))
            return fail(cx, rs);
        outparams->ReplaceElementAt(i, JSParam(variant));
    }

    return ok(rs);
}

bool
WrapperAnswer::AnswerObjectClassIs(const ObjectId &objId, const uint32_t &classValue,
				   bool *result)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    *result = js_ObjectClassIs(cx, obj, (js::ESClassValue)classValue);
    return true;
}

bool
WrapperAnswer::AnswerClassName(const ObjectId &objId, nsString *name)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    *name = NS_ConvertASCIItoUTF16(js_ObjectClassName(cx, obj));
    return true;
}

bool
WrapperAnswer::AnswerGetPropertyNames(const ObjectId &objId, const uint32_t &flags,
				      ReturnStatus *rs, nsTArray<nsString> *names)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    AutoIdVector props(cx);
    if (!js::GetPropertyNames(cx, obj, flags, &props))
        return fail(cx, rs);

    for (size_t i = 0; i < props.length(); i++) {
        nsString name;
        if (!convertIdToGeckoString(cx, props[i], &name))
            return fail(cx, rs);

        names->AppendElement(name);
    }

    return ok(rs);
}

bool
WrapperAnswer::AnswerInstanceOf(const ObjectId &objId, const JSIID &iid, ReturnStatus *rs,
				bool *instanceof)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    *instanceof = false;

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    nsID nsiid;
    ConvertID(iid, &nsiid);

    nsresult rv = xpc::HasInstance(cx, obj, &nsiid, instanceof);
    if (rv != NS_OK)
        return fail(cx, rs);

    return ok(rs);
}

bool
WrapperAnswer::AnswerDOMInstanceOf(const ObjectId &objId, const int &prototypeID,
				   const int &depth,
				   ReturnStatus *rs, bool *instanceof)
{
    AutoSafeJSContext cx;
    JSAutoRequest request(cx);

    *instanceof = false;

    RootedObject obj(cx, findObjectById(objId));
    if (!obj)
        return false;

    JSAutoCompartment comp(cx, obj);

    bool tmp;
    if (!mozilla::dom::InterfaceHasInstance(cx, prototypeID, depth, obj, &tmp))
        return fail(cx, rs);
    *instanceof = tmp;

    return ok(rs);
}

bool
WrapperAnswer::RecvDropObject(const ObjectId &objId)
{
    JSObject *obj = findObjectById(objId);
    if (obj) {
        objectIds_.remove(obj);
        objects_.remove(objId);
    }
    return true;
}
