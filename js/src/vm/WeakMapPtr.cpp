





#include "js/WeakMapPtr.h"

#include "jsweakmap.h"






using namespace js;

namespace {

template<typename T>
struct DataType
{
};

template<>
struct DataType<JSObject*>
{
    typedef PreBarrieredObject PreBarriered;
    static JSObject *NullValue() { return nullptr; }
};

template<>
struct DataType<JS::Value>
{
    typedef PreBarrieredValue PreBarriered;
    static JS::Value NullValue() { return JS::UndefinedValue(); }
};

template <typename K, typename V>
struct Utils
{
    typedef typename DataType<K>::PreBarriered KeyType;
    typedef typename DataType<V>::PreBarriered ValueType;
    typedef WeakMap<KeyType, ValueType> Type;
    typedef Type* PtrType;
    static PtrType cast(void *ptr) { return static_cast<PtrType>(ptr); }
};

} 

template <typename K, typename V>
void
JS::WeakMapPtr<K, V>::destroy()
{
    MOZ_ASSERT(initialized());
    auto map = Utils<K, V>::cast(ptr);
    
    
    if (map->isInList())
        WeakMapBase::removeWeakMapFromList(map);
    js_delete(map);
    ptr = nullptr;
}

template <typename K, typename V>
bool
JS::WeakMapPtr<K, V>::init(JSContext *cx)
{
    MOZ_ASSERT(!initialized());
    typename Utils<K, V>::PtrType map = cx->runtime()->new_<typename Utils<K,V>::Type>(cx);
    if (!map || !map->init())
        return false;
    ptr = map;
    return true;
}

template <typename K, typename V>
void
JS::WeakMapPtr<K, V>::trace(JSTracer *trc)
{
    MOZ_ASSERT(initialized());
    return Utils<K, V>::cast(ptr)->trace(trc);
}

template <typename K, typename V>
V
JS::WeakMapPtr<K, V>::lookup(const K &key)
{
    MOZ_ASSERT(initialized());
    typename Utils<K, V>::Type::Ptr result = Utils<K, V>::cast(ptr)->lookup(key);
    if (!result)
        return DataType<V>::NullValue();
    return result->value();
}

template <typename K, typename V>
 void
JS::WeakMapPtr<K, V>::keyMarkCallback(JSTracer *trc, K key, void *data)
{
    auto map = static_cast< JS::WeakMapPtr<K, V>* >(data);
    K prior = key;
    JS_CallUnbarrieredObjectTracer(trc, &key, "WeakMapPtr key");
    return Utils<K, V>::cast(map->ptr)->rekeyIfMoved(prior, key);
}

template <typename K, typename V>
bool
JS::WeakMapPtr<K, V>::put(JSContext *cx, const K &key, const V &value)
{
    MOZ_ASSERT(initialized());
    if (!Utils<K, V>::cast(ptr)->put(key, value))
        return false;
    JS_StoreObjectPostBarrierCallback(cx, keyMarkCallback, key, this);
    
    
    return true;
}





template class JS_PUBLIC_API(JS::WeakMapPtr)<JSObject*, JSObject*>;

#ifdef DEBUG

template class JS_PUBLIC_API(JS::WeakMapPtr)<JSObject*, JS::Value>;
#endif
