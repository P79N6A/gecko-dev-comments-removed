





#ifndef js_UbiNode_h
#define js_UbiNode_h

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Maybe.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"

#include "jspubtd.h"

#include "js/GCAPI.h"
#include "js/HashTable.h"
#include "js/TracingAPI.h"
#include "js/TypeDecls.h"
#include "js/Vector.h"





















































































































namespace JS {
namespace ubi {

using mozilla::Maybe;

class Edge;
class EdgeRange;



class Base {
    friend class Node;

    
    
    
    
    

  protected:
    
    
    void* ptr;

    explicit Base(void* ptr) : ptr(ptr) { }

  public:
    bool operator==(const Base& rhs) const {
        
        
        
        
        return ptr == rhs.ptr;
    }
    bool operator!=(const Base& rhs) const { return !(*this == rhs); }

    
    
    
    
    
    virtual const char16_t* typeName() const = 0;

    
    
    
    
    virtual size_t size(mozilla::MallocSizeOf mallocSizeof) const { return 0; }

    
    
    
    
    
    
    
    virtual EdgeRange* edges(JSContext* cx, bool wantNames) const = 0;

    
    
    virtual JS::Zone* zone() const { return nullptr; }

    
    
    
    
    virtual JSCompartment* compartment() const { return nullptr; }

    
    
    
    virtual const char* jsObjectClassName() const { return nullptr; }

  private:
    Base(const Base& rhs) = delete;
    Base& operator=(const Base& rhs) = delete;
};





template<typename Referent>
struct Concrete {
    
    static const char16_t concreteTypeName[];

    
    
    
    
    
    
    
    
    
    
    
    
    static void construct(void* storage, Referent* referent);
};



class Node {
    
    mozilla::AlignedStorage2<Base> storage;
    Base* base() { return storage.addr(); }
    const Base* base() const { return storage.addr(); }

    template<typename T>
    void construct(T* ptr) {
        static_assert(sizeof(Concrete<T>) == sizeof(*base()),
                      "ubi::Base specializations must be the same size as ubi::Base");
        Concrete<T>::construct(base(), ptr);
    }

  public:
    Node() { construct<void>(nullptr); }

    template<typename T>
    Node(T* ptr) {
        construct(ptr);
    }
    template<typename T>
    Node& operator=(T* ptr) {
        construct(ptr);
        return *this;
    }

    
    template<typename T>
    Node(const Rooted<T*>& root) {
        construct(root.get());
    }
    template<typename T>
    Node& operator=(const Rooted<T*>& root) {
        construct(root.get());
        return *this;
    }

    
    
    
    
    MOZ_IMPLICIT Node(JS::HandleValue value);
    Node(JSGCTraceKind kind, void* ptr);

    
    
    
    
    
    
    
    
    Node(const Node& rhs) {
        memcpy(storage.u.mBytes, rhs.storage.u.mBytes, sizeof(storage.u));
    }

    Node& operator=(const Node& rhs) {
        memcpy(storage.u.mBytes, rhs.storage.u.mBytes, sizeof(storage.u));
        return *this;
    }

    bool operator==(const Node& rhs) const { return *base() == *rhs.base(); }
    bool operator!=(const Node& rhs) const { return *base() != *rhs.base(); }

    explicit operator bool() const {
        return base()->ptr != nullptr;
    }

    template<typename T>
    bool is() const {
        return base()->typeName() == Concrete<T>::concreteTypeName;
    }

    template<typename T>
    T* as() const {
        MOZ_ASSERT(is<T>());
        return static_cast<T*>(base()->ptr);
    }

    template<typename T>
    T* asOrNull() const {
        return is<T>() ? static_cast<T*>(base()->ptr) : nullptr;
    }

    
    
    
    
    JS::Value exposeToJS() const;

    const char16_t* typeName()      const { return base()->typeName(); }
    JS::Zone* zone()                const { return base()->zone(); }
    JSCompartment* compartment()    const { return base()->compartment(); }
    const char* jsObjectClassName() const { return base()->jsObjectClassName(); }

    size_t size(mozilla::MallocSizeOf mallocSizeof) const {
        return base()->size(mallocSizeof);
    }

    EdgeRange* edges(JSContext* cx, bool wantNames = true) const {
        return base()->edges(cx, wantNames);
    }

    
    
    
    class HashPolicy {
        typedef js::PointerHasher<void*, mozilla::tl::FloorLog2<sizeof(void*)>::value> PtrHash;

      public:
        typedef Node Lookup;

        static js::HashNumber hash(const Lookup& l) { return PtrHash::hash(l.base()->ptr); }
        static bool match(const Node& k, const Lookup& l) { return k == l; }
        static void rekey(Node& k, const Node& newKey) { k = newKey; }
    };
};








class Edge {
  protected:
    Edge() : name(nullptr), referent() { }
    virtual ~Edge() { }

  public:
    
    
    
    
    
    
    
    
    
    const char16_t* name;

    
    Node referent;

  private:
    Edge(const Edge&) = delete;
    Edge& operator=(const Edge&) = delete;
};










class EdgeRange {
  protected:
    
    Edge* front_;

    EdgeRange() : front_(nullptr) { }

  public:
    virtual ~EdgeRange() { }

    
    bool empty() const { return !front_; }

    
    
    
    const Edge& front() { return *front_; }

    
    
    virtual void popFront() = 0;

  private:
    EdgeRange(const EdgeRange&) = delete;
    EdgeRange& operator=(const EdgeRange&) = delete;
};




class SimpleEdge : public Edge {
    SimpleEdge(SimpleEdge&) = delete;
    SimpleEdge& operator=(const SimpleEdge&) = delete;

  public:
    SimpleEdge() : Edge() { }

    
    SimpleEdge(char16_t* name, const Node& referent) {
        this->name = name;
        this->referent = referent;
    }
    ~SimpleEdge() {
        js_free(const_cast<char16_t*>(name));
    }

    
    SimpleEdge(SimpleEdge&& rhs) {
        name = rhs.name;
        referent = rhs.referent;

        rhs.name = nullptr;
    }
    SimpleEdge& operator=(SimpleEdge&& rhs) {
        MOZ_ASSERT(&rhs != this);
        this->~SimpleEdge();
        new(this) SimpleEdge(mozilla::Move(rhs));
        return *this;
    }
};

typedef mozilla::Vector<SimpleEdge, 8, js::TempAllocPolicy> SimpleEdgeVector;






























class MOZ_STACK_CLASS RootList {
    Maybe<AutoCheckCannotGC>& noGC;
    JSContext*               cx;

  public:
    SimpleEdgeVector edges;
    bool             wantNames;

    RootList(JSContext* cx, Maybe<AutoCheckCannotGC>& noGC, bool wantNames = false);

    
    bool init();
    
    bool init(ZoneSet& debuggees);
    
    bool init(HandleObject debuggees);

    
    
    
    bool addRoot(Node node, const char16_t* edgeName = nullptr);
};




template<>
struct Concrete<RootList> : public Base {
    EdgeRange* edges(JSContext* cx, bool wantNames) const override;
    const char16_t* typeName() const override { return concreteTypeName; }

  protected:
    explicit Concrete(RootList* ptr) : Base(ptr) { }
    RootList& get() const { return *static_cast<RootList*>(ptr); }

  public:
    static const char16_t concreteTypeName[];
    static void construct(void* storage, RootList* ptr) { new (storage) Concrete(ptr); }
};



template<typename Referent>
class TracerConcrete : public Base {
    const char16_t* typeName() const override { return concreteTypeName; }
    EdgeRange* edges(JSContext*, bool wantNames) const override;
    JS::Zone* zone() const override;

  protected:
    explicit TracerConcrete(Referent* ptr) : Base(ptr) { }
    Referent& get() const { return *static_cast<Referent*>(ptr); }

  public:
    static const char16_t concreteTypeName[];
    static void construct(void* storage, Referent* ptr) { new (storage) TracerConcrete(ptr); }
};


template<typename Referent>
class TracerConcreteWithCompartment : public TracerConcrete<Referent> {
    typedef TracerConcrete<Referent> TracerBase;
    JSCompartment* compartment() const override;

  protected:
    explicit TracerConcreteWithCompartment(Referent* ptr) : TracerBase(ptr) { }

  public:
    static void construct(void* storage, Referent* ptr) {
        new (storage) TracerConcreteWithCompartment(ptr);
    }
};


template<> struct Concrete<JSString> : TracerConcrete<JSString> { };
template<> struct Concrete<JS::Symbol> : TracerConcrete<JS::Symbol> { };
template<> struct Concrete<JSScript> : TracerConcreteWithCompartment<JSScript> { };


template<>
class Concrete<JSObject> : public TracerConcreteWithCompartment<JSObject> {
    const char* jsObjectClassName() const override;

  protected:
    explicit Concrete(JSObject* ptr) : TracerConcreteWithCompartment(ptr) { }

  public:
    static void construct(void* storage, JSObject* ptr) {
        new (storage) Concrete(ptr);
    }
};


template<>
class Concrete<void> : public Base {
    const char16_t* typeName() const override;
    size_t size(mozilla::MallocSizeOf mallocSizeOf) const override;
    EdgeRange* edges(JSContext* cx, bool wantNames) const override;
    JS::Zone* zone() const override;
    JSCompartment* compartment() const override;

    explicit Concrete(void* ptr) : Base(ptr) { }

  public:
    static void construct(void* storage, void* ptr) { new (storage) Concrete(ptr); }
    static const char16_t concreteTypeName[];
};


} 
} 

namespace js {


template<> struct DefaultHasher<JS::ubi::Node> : JS::ubi::Node::HashPolicy { };

} 

#endif 
