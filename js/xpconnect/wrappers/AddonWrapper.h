





#ifndef AddonWrapper_h
#define AddonWrapper_h

#include "mozilla/Attributes.h"

#include "nsID.h"

#include "jswrapper.h"

namespace xpc {

bool
Interpose(JSContext *cx, JS::HandleObject target, const nsIID *iid, JS::HandleId id,
          JS::MutableHandle<JSPropertyDescriptor> descriptor);

template<typename Base>
class AddonWrapper : public Base {
  public:
    explicit MOZ_CONSTEXPR AddonWrapper(unsigned flags) : Base(flags) { }

    virtual bool getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                          JS::Handle<jsid> id,
                                          JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
                                JS::MutableHandle<JSPropertyDescriptor> desc,
                                JS::ObjectOpResult &result) const MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
                         JS::ObjectOpResult &result) const MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JS::HandleObject wrapper, JS::HandleObject receiver,
                     JS::HandleId id, JS::MutableHandleValue vp,
                     JS::ObjectOpResult &result) const MOZ_OVERRIDE;

    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;

    static const AddonWrapper singleton;
};

} 

#endif 
