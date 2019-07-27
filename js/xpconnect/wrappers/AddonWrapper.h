





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
                                          JS::MutableHandle<JSPropertyDescriptor> desc) const override;
    virtual bool defineProperty(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
                                JS::Handle<JSPropertyDescriptor> desc,
                                JS::ObjectOpResult &result) const override;
    virtual bool delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
                         JS::ObjectOpResult &result) const override;
    virtual bool get(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp) const override;
    virtual bool set(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id, JS::HandleValue v,
                     JS::HandleValue receiver, JS::ObjectOpResult &result) const override;

    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const override;

    static const AddonWrapper singleton;
};

} 

#endif 
