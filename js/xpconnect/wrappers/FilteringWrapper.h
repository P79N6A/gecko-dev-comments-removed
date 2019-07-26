






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

    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool keys(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *proxy, unsigned flags, js::Value *vp);

    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act, bool *bp);

    static FilteringWrapper singleton;
};

}

#endif 
