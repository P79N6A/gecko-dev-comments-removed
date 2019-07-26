






#ifndef __FilteringWrapper_h__
#define __FilteringWrapper_h__

#include <jsapi.h>
#include <jswrapper.h>

namespace xpc {

template <typename Base, typename Policy>
class FilteringWrapper : public Base {
  public:
    FilteringWrapper(unsigned flags);
    virtual ~FilteringWrapper();

    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<js::PropertyDescriptor> desc,
                                       unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                          JS::Handle<jsid> id,
                                          JS::MutableHandle<js::PropertyDescriptor> desc,
                                          unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                     js::AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, JS::Handle<JSObject*> wrapper,
                           js::AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, JS::Handle<JSObject*> wrapper,
                      js::AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned flags,
                         JS::MutableHandle<JS::Value> vp) MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl,
                            JS::CallArgs args) MOZ_OVERRIDE;

    virtual bool defaultValue(JSContext *cx, JS::Handle<JSObject*> obj, JSType hint, JS::MutableHandleValue vp) MOZ_OVERRIDE;

    virtual bool enter(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                       js::Wrapper::Action act, bool *bp) MOZ_OVERRIDE;

    static FilteringWrapper singleton;
};

}

#endif 
