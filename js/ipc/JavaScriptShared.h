






#ifndef mozilla_jsipc_JavaScriptShared_h__
#define mozilla_jsipc_JavaScriptShared_h__

#include "mozilla/dom/DOMTypes.h"
#include "mozilla/jsipc/PJavaScript.h"
#include "nsJSUtils.h"
#include "nsFrameMessageManager.h"

namespace mozilla {
namespace jsipc {

typedef uint64_t ObjectId;

class JavaScriptShared;

class CpowIdHolder : public CpowHolder
{
  public:
    CpowIdHolder(JavaScriptShared *js, const InfallibleTArray<CpowEntry> &cpows)
      : js_(js),
        cpows_(cpows)
    {
    }

    bool ToObject(JSContext *cx, JS::MutableHandleObject objp);

  private:
    JavaScriptShared *js_;
    const InfallibleTArray<CpowEntry> &cpows_;
};


class IdToObjectMap
{
    typedef js::DefaultHasher<ObjectId> TableKeyHasher;

    typedef js::HashMap<ObjectId, JS::Heap<JSObject *>, TableKeyHasher, js::SystemAllocPolicy> Table;

  public:
    IdToObjectMap();

    bool init();
    void trace(JSTracer *trc);
    void finalize(JSFreeOp *fop);

    bool add(ObjectId id, JSObject *obj);
    JSObject *find(ObjectId id);
    void remove(ObjectId id);

  private:
    Table table_;
};


class ObjectToIdMap
{
    typedef js::PointerHasher<JSObject *, 3> Hasher;
    typedef js::HashMap<JSObject *, ObjectId, Hasher, js::SystemAllocPolicy> Table;

  public:
    ObjectToIdMap();
    ~ObjectToIdMap();

    bool init();
    void finalize(JSFreeOp *fop);

    bool add(JSContext *cx, JSObject *obj, ObjectId id);
    ObjectId find(JSObject *obj);
    void remove(JSObject *obj);

  private:
    static void keyMarkCallback(JSTracer *trc, JSObject *key, void *data);

    Table *table_;
};

class Logging;

class JavaScriptShared
{
  public:
    explicit JavaScriptShared(JSRuntime *rt);
    virtual ~JavaScriptShared() {}

    bool init();

    void decref();
    void incref();

    static const uint32_t OBJECT_EXTRA_BITS  = 1;
    static const uint32_t OBJECT_IS_CALLABLE = (1 << 0);

    bool Unwrap(JSContext *cx, const InfallibleTArray<CpowEntry> &aCpows, JS::MutableHandleObject objp);
    bool Wrap(JSContext *cx, JS::HandleObject aObj, InfallibleTArray<CpowEntry> *outCpows);

  protected:
    bool toVariant(JSContext *cx, JS::HandleValue from, JSVariant *to);
    bool fromVariant(JSContext *cx, const JSVariant &from, JS::MutableHandleValue to);

    bool fromDescriptor(JSContext *cx, JS::Handle<JSPropertyDescriptor> desc,
                        PPropertyDescriptor *out);
    bool toDescriptor(JSContext *cx, const PPropertyDescriptor &in,
                      JS::MutableHandle<JSPropertyDescriptor> out);

    bool convertIdToGeckoString(JSContext *cx, JS::HandleId id, nsString *to);
    bool convertGeckoStringToId(JSContext *cx, const nsString &from, JS::MutableHandleId id);

    virtual bool toObjectVariant(JSContext *cx, JSObject *obj, ObjectVariant *objVarp) = 0;
    virtual JSObject *fromObjectVariant(JSContext *cx, ObjectVariant objVar) = 0;

    static void ConvertID(const nsID &from, JSIID *to);
    static void ConvertID(const JSIID &from, nsID *to);

    JSObject *findCPOWById(uint32_t objId) {
        return cpows_.find(objId);
    }
    JSObject *findObjectById(uint32_t objId) {
        return objects_.find(objId);
    }
    JSObject *findObjectById(JSContext *cx, uint32_t objId);

    static bool LoggingEnabled() { return sLoggingEnabled; }
    static bool StackLoggingEnabled() { return sStackLoggingEnabled; }

    friend class Logging;

    virtual bool isParent() = 0;
    virtual JSObject *defaultScope() = 0;

  protected:
    JSRuntime *rt_;
    uintptr_t refcount_;

    IdToObjectMap objects_;
    IdToObjectMap cpows_;

    ObjectId lastId_;
    ObjectToIdMap objectIds_;

    static bool sLoggingInitialized;
    static bool sLoggingEnabled;
    static bool sStackLoggingEnabled;
};


static const uint64_t MAX_CPOW_IDS = (uint64_t(1) << 47) - 1;

} 
} 

#endif
