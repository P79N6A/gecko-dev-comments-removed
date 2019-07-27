





#ifndef XrayWrapper_h
#define XrayWrapper_h

#include "mozilla/Attributes.h"

#include "WrapperFactory.h"

#include "jswrapper.h"







class nsIPrincipal;
class XPCWrappedNative;

namespace xpc {

namespace XrayUtils {

bool IsXPCWNHolderClass(const JSClass *clasp);

bool CloneExpandoChain(JSContext *cx, JSObject *src, JSObject *dst);

bool
IsTransparent(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id);

JSObject *
GetNativePropertiesObject(JSContext *cx, JSObject *wrapper);

bool
HasNativeProperty(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id,
                  bool *hasProp);
}

enum XrayType {
    XrayForDOMObject,
    XrayForWrappedNative,
    XrayForJSObject,
    XrayForOpaqueObject,
    NotXray
};

class XrayTraits
{
public:
    XrayTraits() {}

    static JSObject* getTargetObject(JSObject *wrapper) {
        return js::UncheckedUnwrap(wrapper,  false);
    }

    virtual bool resolveNativeProperty(JSContext *cx, JS::HandleObject wrapper,
                                       JS::HandleObject holder, JS::HandleId id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) = 0;
    
    
    
    
    virtual bool resolveOwnProperty(JSContext *cx, const js::Wrapper &jsWrapper,
                                    JS::HandleObject wrapper, JS::HandleObject holder,
                                    JS::HandleId id, JS::MutableHandle<JSPropertyDescriptor> desc);

    bool delete_(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id, bool *bp) {
        *bp = true;
        return true;
    }

    static const char *className(JSContext *cx, JS::HandleObject wrapper, const js::Wrapper& baseInstance) {
        return baseInstance.className(cx, wrapper);
    }

    virtual void preserveWrapper(JSObject *target) = 0;

    JSObject* getExpandoObject(JSContext *cx, JS::HandleObject target,
                               JS::HandleObject consumer);
    JSObject* ensureExpandoObject(JSContext *cx, JS::HandleObject wrapper,
                                  JS::HandleObject target);

    JSObject* getHolder(JSObject *wrapper);
    JSObject* ensureHolder(JSContext *cx, JS::HandleObject wrapper);
    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) = 0;

    JSObject* getExpandoChain(JS::HandleObject obj);
    bool setExpandoChain(JSContext *cx, JS::HandleObject obj, JS::HandleObject chain);
    bool cloneExpandoChain(JSContext *cx, JS::HandleObject dst, JS::HandleObject src);

private:
    bool expandoObjectMatchesConsumer(JSContext *cx, JS::HandleObject expandoObject,
                                      nsIPrincipal *consumerOrigin,
                                      JS::HandleObject exclusiveGlobal);
    JSObject* getExpandoObjectInternal(JSContext *cx, JS::HandleObject target,
                                       nsIPrincipal *origin,
                                       JSObject *exclusiveGlobal);
    JSObject* attachExpandoObject(JSContext *cx, JS::HandleObject target,
                                  nsIPrincipal *origin,
                                  JS::HandleObject exclusiveGlobal);

    XrayTraits(XrayTraits &) MOZ_DELETE;
    const XrayTraits& operator=(XrayTraits &) MOZ_DELETE;
};

class XPCWrappedNativeXrayTraits : public XrayTraits
{
public:
    enum {
        HasPrototype = 0
    };

    static const XrayType Type = XrayForWrappedNative;

    virtual bool resolveNativeProperty(JSContext *cx, JS::HandleObject wrapper,
                                       JS::HandleObject holder, JS::HandleId id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool resolveOwnProperty(JSContext *cx, const js::Wrapper &jsWrapper, JS::HandleObject wrapper,
                                    JS::HandleObject holder, JS::HandleId id,
                                    JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    bool defineProperty(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id,
                        JS::MutableHandle<JSPropertyDescriptor> desc,
                        JS::Handle<JSPropertyDescriptor> existingDesc, bool *defined);
    virtual bool enumerateNames(JSContext *cx, JS::HandleObject wrapper, unsigned flags,
                                JS::AutoIdVector &props);
    static bool call(JSContext *cx, JS::HandleObject wrapper,
                     const JS::CallArgs &args, const js::Wrapper& baseInstance);
    static bool construct(JSContext *cx, JS::HandleObject wrapper,
                          const JS::CallArgs &args, const js::Wrapper& baseInstance);

    static bool resolveDOMCollectionProperty(JSContext *cx, JS::HandleObject wrapper,
                                             JS::HandleObject holder, JS::HandleId id,
                                             JS::MutableHandle<JSPropertyDescriptor> desc);

    static XPCWrappedNative* getWN(JSObject *wrapper);

    virtual void preserveWrapper(JSObject *target) MOZ_OVERRIDE;

    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;

    static const JSClass HolderClass;
    static XPCWrappedNativeXrayTraits singleton;
};

class DOMXrayTraits : public XrayTraits
{
public:
    enum {
        HasPrototype = 1
    };

    static const XrayType Type = XrayForDOMObject;

    virtual bool resolveNativeProperty(JSContext *cx, JS::HandleObject wrapper,
                                       JS::HandleObject holder, JS::HandleId id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE
    {
        
        
        
        
        
        
        
        
        
        return true;
    }
    virtual bool resolveOwnProperty(JSContext *cx, const js::Wrapper &jsWrapper, JS::HandleObject wrapper,
                                    JS::HandleObject holder, JS::HandleId id,
                                    JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    bool defineProperty(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id,
                        JS::MutableHandle<JSPropertyDescriptor> desc,
                        JS::Handle<JSPropertyDescriptor> existingDesc, bool *defined);
    virtual bool enumerateNames(JSContext *cx, JS::HandleObject wrapper, unsigned flags,
                                JS::AutoIdVector &props);
    static bool call(JSContext *cx, JS::HandleObject wrapper,
                     const JS::CallArgs &args, const js::Wrapper& baseInstance);
    static bool construct(JSContext *cx, JS::HandleObject wrapper,
                          const JS::CallArgs &args, const js::Wrapper& baseInstance);

    static bool getPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                               JS::HandleObject target,
                               JS::MutableHandleObject protop);

    virtual void preserveWrapper(JSObject *target) MOZ_OVERRIDE;

    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;

    static DOMXrayTraits singleton;
};

class JSXrayTraits : public XrayTraits
{
public:
    enum {
        HasPrototype = 1
    };
    static const XrayType Type = XrayForJSObject;

    virtual bool resolveNativeProperty(JSContext *cx, JS::HandleObject wrapper,
                                       JS::HandleObject holder, JS::HandleId id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE
    {
        MOZ_CRASH("resolveNativeProperty hook should never be called with HasPrototype = 1");
    }

    virtual bool resolveOwnProperty(JSContext *cx, const js::Wrapper &jsWrapper, JS::HandleObject wrapper,
                                    JS::HandleObject holder, JS::HandleId id,
                                    JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;

    bool delete_(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id, bool *bp);

    bool defineProperty(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id,
                        JS::MutableHandle<JSPropertyDescriptor> desc,
                        JS::Handle<JSPropertyDescriptor> existingDesc, bool *defined);

    virtual bool enumerateNames(JSContext *cx, JS::HandleObject wrapper, unsigned flags,
                                JS::AutoIdVector &props);

    static bool call(JSContext *cx, JS::HandleObject wrapper,
                     const JS::CallArgs &args, const js::Wrapper& baseInstance)
    {
        JSXrayTraits &self = JSXrayTraits::singleton;
        JS::RootedObject holder(cx, self.ensureHolder(cx, wrapper));
        if (self.getProtoKey(holder) == JSProto_Function)
            return baseInstance.call(cx, wrapper, args);

        JS::RootedValue v(cx, JS::ObjectValue(*wrapper));
        js_ReportIsNotFunction(cx, v);
        return false;
    }

    static bool construct(JSContext *cx, JS::HandleObject wrapper,
                          const JS::CallArgs &args, const js::Wrapper& baseInstance)
    {
        JSXrayTraits &self = JSXrayTraits::singleton;
        JS::RootedObject holder(cx, self.ensureHolder(cx, wrapper));
        if (self.getProtoKey(holder) == JSProto_Function)
            return baseInstance.construct(cx, wrapper, args);

        JS::RootedValue v(cx, JS::ObjectValue(*wrapper));
        js_ReportIsNotFunction(cx, v);
        return false;
    }

    bool getPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                        JS::HandleObject target,
                        JS::MutableHandleObject protop)
    {
        JS::RootedObject holder(cx, ensureHolder(cx, wrapper));
        JSProtoKey key = getProtoKey(holder);
        if (isPrototype(holder)) {
            JSProtoKey parentKey = js::ParentKeyForStandardClass(key);
            if (parentKey == JSProto_Null) {
                protop.set(nullptr);
                return true;
            }
            key = parentKey;
        }

        {
            JSAutoCompartment ac(cx, target);
            if (!JS_GetClassPrototype(cx, key, protop))
                return false;
        }
        return JS_WrapObject(cx, protop);
    }

    virtual void preserveWrapper(JSObject *target) MOZ_OVERRIDE {
        
        
        
        
    }

    enum {
        SLOT_PROTOKEY = 0,
        SLOT_ISPROTOTYPE,
        SLOT_CONSTRUCTOR_FOR,
        SLOT_COUNT
    };
    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;

    static JSProtoKey getProtoKey(JSObject *holder) {
        int32_t key = js::GetReservedSlot(holder, SLOT_PROTOKEY).toInt32();
        return static_cast<JSProtoKey>(key);
    }

    static bool isPrototype(JSObject *holder) {
        return js::GetReservedSlot(holder, SLOT_ISPROTOTYPE).toBoolean();
    }

    static JSProtoKey constructorFor(JSObject *holder) {
        int32_t key = js::GetReservedSlot(holder, SLOT_CONSTRUCTOR_FOR).toInt32();
        return static_cast<JSProtoKey>(key);
    }

    static bool getOwnPropertyFromTargetIfSafe(JSContext *cx,
                                               JS::HandleObject target,
                                               JS::HandleObject wrapper,
                                               JS::HandleId id,
                                               JS::MutableHandle<JSPropertyDescriptor> desc);

    static const JSClass HolderClass;
    static JSXrayTraits singleton;
};




class OpaqueXrayTraits : public XrayTraits
{
public:
    enum {
        HasPrototype = 1
    };
    static const XrayType Type = XrayForOpaqueObject;

    virtual bool resolveNativeProperty(JSContext *cx, JS::HandleObject wrapper,
                                       JS::HandleObject holder, JS::HandleId id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE
    {
        MOZ_CRASH("resolveNativeProperty hook should never be called with HasPrototype = 1");
    }

    virtual bool resolveOwnProperty(JSContext *cx, const js::Wrapper &jsWrapper, JS::HandleObject wrapper,
                                    JS::HandleObject holder, JS::HandleId id,
                                    JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;

    bool defineProperty(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id,
                        JS::MutableHandle<JSPropertyDescriptor> desc,
                        JS::Handle<JSPropertyDescriptor> existingDesc, bool *defined)
    {
        *defined = false;
        return true;
    }

    virtual bool enumerateNames(JSContext *cx, JS::HandleObject wrapper, unsigned flags,
                                JS::AutoIdVector &props)
    {
        return true;
    }

    static bool call(JSContext *cx, JS::HandleObject wrapper,
                     const JS::CallArgs &args, const js::Wrapper& baseInstance)
    {
        JS::RootedValue v(cx, JS::ObjectValue(*wrapper));
        js_ReportIsNotFunction(cx, v);
        return false;
    }

    static bool construct(JSContext *cx, JS::HandleObject wrapper,
                          const JS::CallArgs &args, const js::Wrapper& baseInstance)
    {
        JS::RootedValue v(cx, JS::ObjectValue(*wrapper));
        js_ReportIsNotFunction(cx, v);
        return false;
    }

    bool getPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                        JS::HandleObject target,
                        JS::MutableHandleObject protop)
    {
        
        
        
        {
            JSAutoCompartment ac(cx, target);
            if (!JS_GetClassPrototype(cx, JSProto_Object, protop))
                return false;
        }
        return JS_WrapObject(cx, protop);
    }

    static const char *className(JSContext *cx, JS::HandleObject wrapper, const js::Wrapper& baseInstance) {
        return "Opaque";
    }

    virtual void preserveWrapper(JSObject *target) MOZ_OVERRIDE { }

    virtual JSObject* createHolder(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE
    {
        JS::RootedObject global(cx, JS_GetGlobalForObject(cx, wrapper));
        return JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr(), global);
    }

    static OpaqueXrayTraits singleton;
};

XrayType GetXrayType(JSObject *obj);
XrayTraits* GetXrayTraits(JSObject *obj);


template <typename Base, typename Traits = XPCWrappedNativeXrayTraits >
class XrayWrapper : public Base {
  public:
    MOZ_CONSTEXPR explicit XrayWrapper(unsigned flags)
      : Base(flags | WrapperFactory::IS_XRAY_WRAPPER_FLAG, Traits::HasPrototype)
    { };

    
    virtual bool isExtensible(JSContext *cx, JS::Handle<JSObject*> wrapper, bool *extensible) const MOZ_OVERRIDE;
    virtual bool preventExtensions(JSContext *cx, JS::Handle<JSObject*> wrapper) const MOZ_OVERRIDE;
    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                                          JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                                JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool ownPropertyKeys(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                 JS::AutoIdVector &props) const MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, JS::Handle<JSObject*> wrapper,
                         JS::Handle<jsid> id, bool *bp) const MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::AutoIdVector &props) const MOZ_OVERRIDE;

    
    virtual bool get(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, bool strict, JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;
    virtual bool has(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                     bool *bp) const MOZ_OVERRIDE;
    virtual bool hasOwn(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                        bool *bp) const MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, JS::Handle<JSObject*> wrapper,
                      JS::AutoIdVector &props) const MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned flags,
                         JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;

    virtual bool call(JSContext *cx, JS::Handle<JSObject*> wrapper,
                      const JS::CallArgs &args) const MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JS::Handle<JSObject*> wrapper,
                           const JS::CallArgs &args) const MOZ_OVERRIDE;

    virtual const char *className(JSContext *cx, JS::HandleObject proxy) const MOZ_OVERRIDE;
    virtual bool defaultValue(JSContext *cx, JS::HandleObject wrapper,
                              JSType hint, JS::MutableHandleValue vp)
                              const MOZ_OVERRIDE;

    virtual bool getPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                                JS::MutableHandleObject protop) const MOZ_OVERRIDE;
    virtual bool setPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                                JS::HandleObject proto, bool *bp) const MOZ_OVERRIDE;

    static const XrayWrapper singleton;

  private:
    template <bool HasPrototype>
    typename mozilla::EnableIf<HasPrototype, bool>::Type
        getPrototypeOfHelper(JSContext *cx, JS::HandleObject wrapper,
                             JS::HandleObject target, JS::MutableHandleObject protop) const
    {
        return Traits::singleton.getPrototypeOf(cx, wrapper, target, protop);
    }
    template <bool HasPrototype>
    typename mozilla::EnableIf<!HasPrototype, bool>::Type
        getPrototypeOfHelper(JSContext *cx, JS::HandleObject wrapper,
                             JS::HandleObject target, JS::MutableHandleObject protop) const
    {
        return Base::getPrototypeOf(cx, wrapper, protop);
    }
    bool getPrototypeOfHelper(JSContext *cx, JS::HandleObject wrapper,
                              JS::HandleObject target, JS::MutableHandleObject protop) const
    {
        return getPrototypeOfHelper<Traits::HasPrototype>(cx, wrapper, target,
                                                          protop);
    }

  protected:
    bool enumerate(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned flags,
                   JS::AutoIdVector &props) const;
};

#define PermissiveXrayXPCWN xpc::XrayWrapper<js::CrossCompartmentWrapper, xpc::XPCWrappedNativeXrayTraits>
#define SecurityXrayXPCWN xpc::XrayWrapper<js::CrossCompartmentSecurityWrapper, xpc::XPCWrappedNativeXrayTraits>
#define PermissiveXrayDOM xpc::XrayWrapper<js::CrossCompartmentWrapper, xpc::DOMXrayTraits>
#define SecurityXrayDOM xpc::XrayWrapper<js::CrossCompartmentSecurityWrapper, xpc::DOMXrayTraits>
#define PermissiveXrayJS xpc::XrayWrapper<js::CrossCompartmentWrapper, xpc::JSXrayTraits>
#define PermissiveXrayOpaque xpc::XrayWrapper<js::CrossCompartmentWrapper, xpc::OpaqueXrayTraits>
#define SCSecurityXrayXPCWN xpc::XrayWrapper<js::SameCompartmentSecurityWrapper, xpc::XPCWrappedNativeXrayTraits>

class SandboxProxyHandler : public js::Wrapper {
public:
    MOZ_CONSTEXPR SandboxProxyHandler() : js::Wrapper(0)
    {
    }

    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> proxy,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> proxy,
                                          JS::Handle<jsid> id,
                                          JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;

    
    
    virtual bool has(JSContext *cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                     bool *bp) const MOZ_OVERRIDE;
    virtual bool hasOwn(JSContext *cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                        bool *bp) const MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JS::Handle<JSObject*> proxy, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JS::Handle<JSObject*> proxy, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, bool strict, JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, JS::Handle<JSObject*> proxy,
                      JS::AutoIdVector &props) const MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, JS::Handle<JSObject*> proxy, unsigned flags,
                         JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;
};

extern const SandboxProxyHandler sandboxProxyHandler;




class SandboxCallableProxyHandler : public js::Wrapper {
public:
    MOZ_CONSTEXPR SandboxCallableProxyHandler() : js::Wrapper(0)
    {
    }

    virtual bool call(JSContext *cx, JS::Handle<JSObject*> proxy,
                      const JS::CallArgs &args) const MOZ_OVERRIDE;
};

extern const SandboxCallableProxyHandler sandboxCallableProxyHandler;

class AutoSetWrapperNotShadowing;

}

#endif
