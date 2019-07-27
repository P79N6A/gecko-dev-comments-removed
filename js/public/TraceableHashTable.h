





#ifndef gc_HashTable_h
#define gc_HashTable_h

#include "js/HashTable.h"
#include "js/RootingAPI.h"

namespace js {

template <typename> struct DefaultTracer;














template <typename Key,
          typename Value,
          typename HashPolicy = DefaultHasher<Key>,
          typename AllocPolicy = TempAllocPolicy,
          typename KeyTraceFunc = DefaultTracer<Key>,
          typename ValueTraceFunc = DefaultTracer<Value>>
class TraceableHashMap : public HashMap<Key, Value, HashPolicy, AllocPolicy>,
                         public JS::DynamicTraceable
{
    using Base = HashMap<Key, Value, HashPolicy, AllocPolicy>;

  public:
    explicit TraceableHashMap(AllocPolicy a = AllocPolicy()) : Base(a)  {}

    void trace(JSTracer* trc) override {
        if (!this->initialized())
            return;
        for (typename Base::Enum e(*this); !e.empty(); e.popFront()) {
            ValueTraceFunc::trace(trc, &e.front().value(), "hashmap value");
            Key key = e.front().key();
            KeyTraceFunc::trace(trc, &key, "hashmap key");
            if (key != e.front().key())
                e.rekeyFront(key);
        }
    }

    
    TraceableHashMap(TraceableHashMap&& rhs) : Base(mozilla::Forward<TraceableHashMap>(rhs)) {}
    void operator=(TraceableHashMap&& rhs) {
        MOZ_ASSERT(this != &rhs, "self-move assignment is prohibited");
        Base::operator=(mozilla::Forward<TraceableHashMap>(rhs));
    }

  private:
    
    TraceableHashMap(const TraceableHashMap& hm) = delete;
    TraceableHashMap& operator=(const TraceableHashMap& hm) = delete;
};

template <typename Outer, typename... MapArgs>
class TraceableHashMapOperations
{
    using Map = TraceableHashMap<MapArgs...>;
    using Lookup = typename Map::Lookup;
    using Ptr = typename Map::Ptr;
    using AddPtr = typename Map::AddPtr;
    using Range = typename Map::Range;
    using Enum = typename Map::Enum;

    const Map& map() const { return static_cast<const Outer*>(this)->extract(); }

  public:
    bool initialized() const                   { return map().initialized(); }
    Ptr lookup(const Lookup& l) const          { return map().lookup(l); }
    AddPtr lookupForAdd(const Lookup& l) const { return map().lookupForAdd(l); }
    Range all() const                          { return map().all(); }
    bool empty() const                         { return map().empty(); }
    uint32_t count() const                     { return map().count(); }
    size_t capacity() const                    { return map().capacity(); }
    uint32_t generation() const                { return map().generation(); }
    bool has(const Lookup& l) const            { return map().lookup(l).found(); }
};

template <typename Outer, typename... MapArgs>
class MutableTraceableHashMapOperations
  : public TraceableHashMapOperations<Outer, MapArgs...>
{
    using Map = TraceableHashMap<MapArgs...>;
    using Lookup = typename Map::Lookup;
    using Ptr = typename Map::Ptr;
    using AddPtr = typename Map::AddPtr;
    using Range = typename Map::Range;
    using Enum = typename Map::Enum;

    Map& map() { return static_cast<Outer*>(this)->extract(); }

  public:
    bool init(uint32_t len = 16) { return map().init(len); }
    void clear()                 { map().clear(); }
    void finish()                { map().finish(); }
    void remove(Ptr p)           { map().remove(p); }

    template<typename KeyInput, typename ValueInput>
    bool add(AddPtr& p, KeyInput&& k, ValueInput&& v) {
        return map().add(p, mozilla::Forward<KeyInput>(k), mozilla::Forward<ValueInput>(v));
    }

    template<typename KeyInput>
    bool add(AddPtr& p, KeyInput&& k) {
        return map().add(p, mozilla::Forward<KeyInput>(k), Map::Value());
    }

    template<typename KeyInput, typename ValueInput>
    bool relookupOrAdd(AddPtr& p, KeyInput&& k, ValueInput&& v) {
        return map().relookupOrAdd(p, k,
                                   mozilla::Forward<KeyInput>(k),
                                   mozilla::Forward<ValueInput>(v));
    }

    template<typename KeyInput, typename ValueInput>
    bool put(KeyInput&& k, ValueInput&& v) {
        return map().put(mozilla::Forward<KeyInput>(k), mozilla::Forward<ValueInput>(v));
    }

    template<typename KeyInput, typename ValueInput>
    bool putNew(KeyInput&& k, ValueInput&& v) {
        return map().putNew(mozilla::Forward<KeyInput>(k), mozilla::Forward<ValueInput>(v));
    }
};

template <template <typename...> class TraceableHashMap, typename... MapArgs>
class RootedBase<TraceableHashMap<MapArgs...>>
  : public MutableTraceableHashMapOperations<JS::Rooted<TraceableHashMap<MapArgs...>>, MapArgs...>
{
    using Map = TraceableHashMap<MapArgs...>;

    friend class TraceableHashMapOperations<JS::Rooted<Map>, MapArgs...>;
    const Map& extract() const { return *static_cast<const JS::Rooted<Map>*>(this)->address(); }

    friend class MutableTraceableHashMapOperations<JS::Rooted<Map>, MapArgs...>;
    Map& extract() { return *static_cast<JS::Rooted<Map>*>(this)->address(); }
};

template <template <typename...> class TraceableHashMap, typename... MapArgs>
class MutableHandleBase<TraceableHashMap<MapArgs...>>
  : public MutableTraceableHashMapOperations<JS::MutableHandle<TraceableHashMap<MapArgs...>>,
                                             MapArgs...>
{
    using Map = TraceableHashMap<MapArgs...>;

    friend class TraceableHashMapOperations<JS::MutableHandle<Map>, MapArgs...>;
    const Map& extract() const {
        return *static_cast<const JS::MutableHandle<Map>*>(this)->address();
    }

    friend class MutableTraceableHashMapOperations<JS::MutableHandle<Map>, MapArgs...>;
    Map& extract() { return *static_cast<JS::MutableHandle<Map>*>(this)->address(); }
};

template <template <typename...> class TraceableHashMap, typename... MapArgs>
class HandleBase<TraceableHashMap<MapArgs...>>
  : public TraceableHashMapOperations<JS::Handle<TraceableHashMap<MapArgs...>>, MapArgs...>
{
    using Map = TraceableHashMap<MapArgs...>;
    friend class TraceableHashMapOperations<JS::Handle<Map>, MapArgs...>;
    const Map& extract() const { return *static_cast<const JS::Handle<Map>*>(this)->address(); }
};


template <typename T> struct DefaultTracer {
    static_assert(mozilla::IsPod<T>::value, "non-pod types must not be ignored");
    static void trace(JSTracer* trc, T* t, const char* name) {}
};

} 

#endif 
