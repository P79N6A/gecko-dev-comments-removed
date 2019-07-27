





#ifndef __FilteringWrapper_h__
#define __FilteringWrapper_h__

#include "XrayWrapper.h"
#include "mozilla/Attributes.h"
#include "jswrapper.h"
#include "js/CallNonGenericMethod.h"

struct JSPropertyDescriptor;

namespace JS {
template <typename T>
class AutoVectorRooter;
typedef AutoVectorRooter<jsid> AutoIdVector;
}

namespace xpc {

template <typename Base, typename Policy>
class FilteringWrapper : public Base {
  public:
    MOZ_CONSTEXPR explicit FilteringWrapper(unsigned flags) : Base(flags) {}

    virtual bool enter(JSContext* cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                       js::Wrapper::Action act, bool* bp) const override;

    virtual bool getOwnPropertyDescriptor(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                          JS::Handle<jsid> id,
                                          JS::MutableHandle<JSPropertyDescriptor> desc) const override;
    virtual bool ownPropertyKeys(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                 JS::AutoIdVector& props) const override;

    virtual bool getPropertyDescriptor(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const override;
    virtual bool getOwnEnumerablePropertyKeys(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                              JS::AutoIdVector& props) const override;
    virtual bool enumerate(JSContext* cx, JS::Handle<JSObject*> wrapper,
                           JS::MutableHandle<JSObject*> objp) const override;

    virtual bool call(JSContext* cx, JS::Handle<JSObject*> wrapper,
                      const JS::CallArgs& args) const override;
    virtual bool construct(JSContext* cx, JS::Handle<JSObject*> wrapper,
                           const JS::CallArgs& args) const override;

    virtual bool nativeCall(JSContext* cx, JS::IsAcceptableThis test, JS::NativeImpl impl,
                            JS::CallArgs args) const override;

    virtual bool defaultValue(JSContext* cx, JS::Handle<JSObject*> obj, JSType hint,
                              JS::MutableHandleValue vp) const override;

    virtual bool getPrototype(JSContext* cx, JS::HandleObject wrapper,
                              JS::MutableHandleObject protop) const override;

    static const FilteringWrapper singleton;
};






class CrossOriginXrayWrapper : public SecurityXrayDOM {
  public:
    explicit CrossOriginXrayWrapper(unsigned flags);

    virtual bool getOwnPropertyDescriptor(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                          JS::Handle<jsid> id,
                                          JS::MutableHandle<JSPropertyDescriptor> desc) const override;
    virtual bool defineProperty(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                JS::Handle<jsid> id,
                                JS::Handle<JSPropertyDescriptor> desc,
                                JS::ObjectOpResult& result) const override;
    virtual bool ownPropertyKeys(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                 JS::AutoIdVector& props) const override;
    virtual bool delete_(JSContext* cx, JS::Handle<JSObject*> wrapper,
                         JS::Handle<jsid> id, JS::ObjectOpResult& result) const override;

    virtual bool getPropertyDescriptor(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const override;
};

}

#endif 
