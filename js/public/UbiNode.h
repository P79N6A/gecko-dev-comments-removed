





#ifndef js_UbiNode_h
#define js_UbiNode_h

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Move.h"

#include "jspubtd.h"

#include "js/GCAPI.h"
#include "js/HashTable.h"
#include "js/TypeDecls.h"






















































































































namespace JS {
namespace ubi {

class Edge;
class EdgeRange;



class Base {
    friend class Node;

    
    
    
    
    

  protected:
    
    
    void *ptr;

    explicit Base(void *ptr) : ptr(ptr) { }

  public:
    bool operator==(const Base &rhs) const {
        
        
        
        
        return ptr == rhs.ptr;
    }
    bool operator!=(const Base &rhs) const { return !(*this == rhs); }

    
    
    
    
    
    virtual const char16_t *typeName() const = 0;

    
    
    virtual size_t size() const { return 0; }

    
    
    
    
    
    
    
    virtual EdgeRange *edges(JSContext *cx, bool wantNames) const = 0;

    
    
    virtual JS::Zone *zone() const { return nullptr; }

    
    
    
    
    virtual JSCompartment *compartment() const { return nullptr; }

  private:
    Base(const Base &rhs) MOZ_DELETE;
    Base &operator=(const Base &rhs) MOZ_DELETE;
};





template<typename Referent>
struct Concrete {
    
    static const char16_t concreteTypeName[];

    
    
    
    
    
    
    
    
    
    
    
    
    static void construct(void *storage, Referent *referent);
};



class Node {
    
    mozilla::AlignedStorage2<Base> storage;
    Base *base() { return storage.addr(); }
    const Base *base() const { return storage.addr(); }

    template<typename T>
    void construct(T *ptr) {
        static_assert(sizeof(Concrete<T>) == sizeof(*base()),
                      "ubi::Base specializations must be the same size as ubi::Base");
        Concrete<T>::construct(base(), ptr);
    }

    typedef void (Node::* ConvertibleToBool)();
    void nonNull() {}

  public:
    Node() { construct<void>(nullptr); }

    template<typename T>
    Node(T *ptr) {
        construct(ptr);
    }
    template<typename T>
    Node &operator=(T *ptr) {
        construct(ptr);
        return *this;
    }

    
    template<typename T>
    Node(const Rooted<T *> &root) {
        construct(root.get());
    }
    template<typename T>
    Node &operator=(const Rooted<T *> &root) {
        construct(root.get());
        return *this;
    }

    
    Node(JS::HandleValue value);
    Node(JSGCTraceKind kind, void *ptr);

    
    
    
    
    
    
    
    
    Node(const Node &rhs) {
        memcpy(storage.u.mBytes, rhs.storage.u.mBytes, sizeof(storage.u));
    }

    Node &operator=(const Node &rhs) {
        memcpy(storage.u.mBytes, rhs.storage.u.mBytes, sizeof(storage.u));
        return *this;
    }

    bool operator==(const Node &rhs) const { return *base() == *rhs.base(); }
    bool operator!=(const Node &rhs) const { return *base() != *rhs.base(); }

    operator ConvertibleToBool() const {
        return base()->ptr ? &Node::nonNull : 0;
    }

    template<typename T>
    bool is() const {
        return base()->typeName() == Concrete<T>::concreteTypeName;
    }

    template<typename T>
    T *as() const {
        MOZ_ASSERT(is<T>());
        return static_cast<T *>(base()->ptr);
    }

    template<typename T>
    T *asOrNull() const {
        return is<T>() ? static_cast<T *>(base()->ptr) : nullptr;
    }

    
    
    
    
    JS::Value exposeToJS() const;

    const char16_t *typeName()      const { return base()->typeName(); }
    size_t size()                   const { return base()->size(); }
    JS::Zone *zone()                const { return base()->zone(); }
    JSCompartment *compartment()    const { return base()->compartment(); }
    EdgeRange *edges(JSContext *cx, bool wantNames = true) const {
        return base()->edges(cx, wantNames);
    }

    
    
    
    class HashPolicy {
        typedef js::PointerHasher<void *, mozilla::tl::FloorLog2<sizeof(void *)>::value> PtrHash;

      public:
        typedef Node Lookup;

        static js::HashNumber hash(const Lookup &l) { return PtrHash::hash(l.base()->ptr); }
        static bool match(const Node &k, const Lookup &l) { return k == l; }
        static void rekey(Node &k, const Node &newKey) { k = newKey; }
    };
};








class Edge {
  protected:
    Edge() : name(nullptr), referent() { }
    virtual ~Edge() { }

  public:
    
    
    
    
    
    
    
    
    
    const char16_t *name;

    
    Node referent;

  private:
    Edge(const Edge &) MOZ_DELETE;
    Edge &operator=(const Edge &) MOZ_DELETE;
};










class EdgeRange {
  protected:
    
    Edge *front_;

    EdgeRange() : front_(nullptr) { }

  public:
    virtual ~EdgeRange() { }

    
    bool empty() const { return !front_; }

    
    
    
    const Edge &front() { return *front_; }

    
    
    virtual void popFront() = 0;

  private:
    EdgeRange(const EdgeRange &) MOZ_DELETE;
    EdgeRange &operator=(const EdgeRange &) MOZ_DELETE;
};






template<typename Referent>
class TracerConcrete : public Base {
    const char16_t *typeName() const MOZ_OVERRIDE { return concreteTypeName; }
    EdgeRange *edges(JSContext *, bool wantNames) const MOZ_OVERRIDE;
    JS::Zone *zone() const MOZ_OVERRIDE;

  protected:
    explicit TracerConcrete(Referent *ptr) : Base(ptr) { }
    Referent &get() const { return *static_cast<Referent *>(ptr); }

  public:
    static const char16_t concreteTypeName[];
    static void construct(void *storage, Referent *ptr) { new (storage) TracerConcrete(ptr); }
};


template<typename Referent>
class TracerConcreteWithCompartment : public TracerConcrete<Referent> {
    typedef TracerConcrete<Referent> TracerBase;
    JSCompartment *compartment() const MOZ_OVERRIDE;

    explicit TracerConcreteWithCompartment(Referent *ptr) : TracerBase(ptr) { }

  public:
    static void construct(void *storage, Referent *ptr) {
        new (storage) TracerConcreteWithCompartment(ptr);
    }
};


template<> struct Concrete<JSObject> : TracerConcreteWithCompartment<JSObject> { };
template<> struct Concrete<JSString> : TracerConcrete<JSString> { };
template<> struct Concrete<JS::Symbol> : TracerConcrete<JS::Symbol> { };
template<> struct Concrete<JSScript> : TracerConcreteWithCompartment<JSScript> { };


template<>
class Concrete<void> : public Base {
    const char16_t *typeName() const MOZ_OVERRIDE;
    size_t size() const MOZ_OVERRIDE;
    EdgeRange *edges(JSContext *cx, bool wantNames) const MOZ_OVERRIDE;
    JS::Zone *zone() const MOZ_OVERRIDE;
    JSCompartment *compartment() const MOZ_OVERRIDE;

    explicit Concrete(void *ptr) : Base(ptr) { }

  public:
    static void construct(void *storage, void *ptr) { new (storage) Concrete(ptr); }
    static const char16_t concreteTypeName[];
};


} 
} 

namespace js {


template<> struct DefaultHasher<JS::ubi::Node> : JS::ubi::Node::HashPolicy { };

} 

#endif 
