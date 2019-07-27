





#ifndef __FilteringWrapper_h__
#define __FilteringWrapper_h__

#include "XrayWrapper.h"
#include "mozilla/Attributes.h"
#include "jswrapper.h"
#include "js/CallNonGenericMethod.h"

struct JSPropertyDescriptor;

namespace JS {
class AutoIdVector;
}

namespace xpc {

template <typename Base, typename Policy>
class FilteringWrapper : public Base {
  public:
    MOZ_CONSTEXPR FilteringWrapper(unsigned flags) : Base(flags) {}

    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                          JS::Handle<jsid> id,
                                          JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                     JS::AutoIdVector &props) const MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, JS::Handle<JSObject*> wrapper,
                           JS::AutoIdVector &props) const MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, JS::Handle<JSObject*> wrapper,
                      JS::AutoIdVector &props) const MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned flags,
                         JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl,
                            JS::CallArgs args) const MOZ_OVERRIDE;

    virtual bool defaultValue(JSContext *cx, JS::Handle<JSObject*> obj, JSType hint,
                              JS::MutableHandleValue vp) const MOZ_OVERRIDE;

    virtual bool enter(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                       js::Wrapper::Action act, bool *bp) const MOZ_OVERRIDE;

    static const FilteringWrapper singleton;
};






class CrossOriginXrayWrapper : public SecurityXrayDOM {
  public:
    CrossOriginXrayWrapper(unsigned flags);

    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                          JS::Handle<jsid> id,
                                          JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;

    virtual bool getOwnPropertyNames(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                     JS::AutoIdVector &props) const MOZ_OVERRIDE;

    virtual bool defineProperty(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                JS::Handle<jsid> id,
                                JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, JS::Handle<JSObject*> wrapper,
                         JS::Handle<jsid> id, bool *bp) const MOZ_OVERRIDE;

    virtual bool enumerate(JSContext *cx, JS::Handle<JSObject*> wrapper,
                           JS::AutoIdVector &props) const MOZ_OVERRIDE;

    virtual bool getPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                                JS::MutableHandleObject protop) const MOZ_OVERRIDE;
};

}

#endif 
