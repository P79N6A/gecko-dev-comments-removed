





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
    typedef EncapsulatedPtrObject Encapsulated;
    static JSObject *NullValue() { return nullptr; }
};

template<>
struct DataType<JS::Value>
{
    typedef EncapsulatedValue Encapsulated;
    static JS::Value NullValue() { return JS::UndefinedValue(); }
};

template <typename K, typename V>
struct Utils
{
    typedef typename DataType<K>::Encapsulated KeyType;
    typedef typename DataType<V>::Encapsulated ValueType;
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
    map->check();
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
bool
JS::WeakMapPtr<K, V>::put(const K &key, const V &value)
{
    MOZ_ASSERT(initialized());
    return Utils<K, V>::cast(ptr)->put(key, value);
}





template class JS::WeakMapPtr<JSObject*, JSObject*>;

#ifdef DEBUG

template class JS::WeakMapPtr<JSObject*, JS::Value>;
#endif
