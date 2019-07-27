





#ifndef frontend_ParseMaps_h
#define frontend_ParseMaps_h

#include "mozilla/Attributes.h"
#include "mozilla/TypeTraits.h"

#include "ds/InlineMap.h"
#include "gc/Barrier.h"
#include "js/Vector.h"

class JSAtom;

typedef uintptr_t jsatomid;

namespace js {

class LifoAlloc;

namespace frontend {

class DefinitionSingle;
class DefinitionList;

typedef InlineMap<JSAtom*, jsatomid, 24> AtomIndexMap;
typedef InlineMap<JSAtom*, DefinitionSingle, 24> AtomDefnMap;
typedef InlineMap<JSAtom*, DefinitionList, 24> AtomDefnListMap;






void
InitAtomMap(AtomIndexMap* indices, HeapPtrAtom* atoms);








class ParseMapPool
{
    typedef Vector<void*, 32, SystemAllocPolicy> RecyclableMaps;

    RecyclableMaps      all;
    RecyclableMaps      recyclable;

    void checkInvariants();

    void recycle(void* map) {
        MOZ_ASSERT(map);
#ifdef DEBUG
        bool ok = false;
        
        for (void** it = all.begin(); it != all.end(); ++it) {
            if (*it == map) {
                ok = true;
                break;
            }
        }
        MOZ_ASSERT(ok);
        for (void** it = recyclable.begin(); it != recyclable.end(); ++it)
            MOZ_ASSERT(*it != map);
#endif
        MOZ_ASSERT(recyclable.length() < all.length());
        recyclable.infallibleAppend(map); 
    }

    void* allocateFresh();
    void* allocate() {
        if (recyclable.empty())
            return allocateFresh();

        void* map = recyclable.popCopy();
        asAtomMap(map)->clear();
        return map;
    }

    
    typedef AtomIndexMap AtomMapT;

    static AtomMapT* asAtomMap(void* ptr) {
        return reinterpret_cast<AtomMapT*>(ptr);
    }

  public:
    ~ParseMapPool() {
        purgeAll();
    }

    void purgeAll();

    bool empty() const {
        return all.empty();
    }

    
    template <typename T>
    T* acquire() {
        return reinterpret_cast<T*>(allocate());
    }

    

    void release(AtomIndexMap* map) {
        recycle((void*) map);
    }

    void release(AtomDefnMap* map) {
        recycle((void*) map);
    }

    void release(AtomDefnListMap* map) {
        recycle((void*) map);
    }
}; 





template <class Map>
struct AtomThingMapPtr
{
    Map* map_;

    void init() { clearMap(); }

    bool ensureMap(ExclusiveContext* cx);
    void releaseMap(ExclusiveContext* cx);

    bool hasMap() const { return map_; }
    Map* getMap() { return map_; }
    void setMap(Map* newMap) { MOZ_ASSERT(!map_); map_ = newMap; }
    void clearMap() { map_ = nullptr; }

    Map* operator->() { return map_; }
    const Map* operator->() const { return map_; }
    Map& operator*() const { return *map_; }
};

typedef AtomThingMapPtr<AtomIndexMap> AtomIndexMapPtr;





template <typename AtomThingMapPtrT>
class OwnedAtomThingMapPtr : public AtomThingMapPtrT
{
    ExclusiveContext* cx;

  public:
    explicit OwnedAtomThingMapPtr(ExclusiveContext* cx) : cx(cx) {
        AtomThingMapPtrT::init();
    }

    ~OwnedAtomThingMapPtr() {
        AtomThingMapPtrT::releaseMap(cx);
    }
};

typedef OwnedAtomThingMapPtr<AtomIndexMapPtr> OwnedAtomIndexMapPtr;










class DefinitionSingle
{
    uintptr_t bits;

  public:

    template <typename ParseHandler>
    static DefinitionSingle new_(typename ParseHandler::DefinitionNode defn)
    {
        DefinitionSingle res;
        res.bits = ParseHandler::definitionToBits(defn);
        return res;
    }

    template <typename ParseHandler>
    typename ParseHandler::DefinitionNode get() {
        return ParseHandler::definitionFromBits(bits);
    }
};

struct AtomDefnMapPtr : public AtomThingMapPtr<AtomDefnMap>
{
    template <typename ParseHandler>
    MOZ_ALWAYS_INLINE
    typename ParseHandler::DefinitionNode lookupDefn(JSAtom* atom) {
        AtomDefnMap::Ptr p = map_->lookup(atom);
        return p ? p.value().get<ParseHandler>() : ParseHandler::nullDefinition();
    }
};

typedef OwnedAtomThingMapPtr<AtomDefnMapPtr> OwnedAtomDefnMapPtr;













class DefinitionList
{
  public:
    class Range;

  private:
    friend class Range;

    
    struct Node
    {
        uintptr_t bits;
        Node* next;

        Node(uintptr_t bits, Node* next) : bits(bits), next(next) {}
    };

    union {
        uintptr_t bits;
        Node* head;
    } u;

    Node* firstNode() const {
        MOZ_ASSERT(isMultiple());
        return (Node*) (u.bits & ~0x1);
    }

    static Node*
    allocNode(ExclusiveContext* cx, LifoAlloc& alloc, uintptr_t bits, Node* tail);

  public:
    class Range
    {
        friend class DefinitionList;

        Node* node;
        uintptr_t bits;

        explicit Range(const DefinitionList& list) {
            if (list.isMultiple()) {
                node = list.firstNode();
                bits = node->bits;
            } else {
                node = nullptr;
                bits = list.u.bits;
            }
        }

      public:
        
        Range() : node(nullptr), bits(0) {}

        void popFront() {
            MOZ_ASSERT(!empty());
            if (!node) {
                bits = 0;
                return;
            }
            node = node->next;
            bits = node ? node->bits : 0;
        }

        template <typename ParseHandler>
        typename ParseHandler::DefinitionNode front() {
            MOZ_ASSERT(!empty());
            return ParseHandler::definitionFromBits(bits);
        }

        bool empty() const {
            MOZ_ASSERT_IF(!bits, !node);
            return !bits;
        }
    };

    DefinitionList() {
        u.bits = 0;
    }

    explicit DefinitionList(uintptr_t bits) {
        u.bits = bits;
        MOZ_ASSERT(!isMultiple());
    }

    explicit DefinitionList(Node* node) {
        u.head = node;
        u.bits |= 0x1;
        MOZ_ASSERT(isMultiple());
    }

    bool isMultiple() const { return (u.bits & 0x1) != 0; }

    template <typename ParseHandler>
    typename ParseHandler::DefinitionNode front() {
        return ParseHandler::definitionFromBits(isMultiple() ? firstNode()->bits : u.bits);
    }

    




    bool popFront() {
        if (!isMultiple())
            return false;

        Node* node = firstNode();
        Node* next = node->next;
        if (next->next)
            *this = DefinitionList(next);
        else
            *this = DefinitionList(next->bits);
        return true;
    }

    




    template <typename ParseHandler>
    bool pushFront(ExclusiveContext* cx, LifoAlloc& alloc,
                   typename ParseHandler::DefinitionNode defn) {
        Node* tail;
        if (isMultiple()) {
            tail = firstNode();
        } else {
            tail = allocNode(cx, alloc, u.bits, nullptr);
            if (!tail)
                return false;
        }

        Node* node = allocNode(cx, alloc, ParseHandler::definitionToBits(defn), tail);
        if (!node)
            return false;
        *this = DefinitionList(node);
        return true;
    }

    
    template <typename ParseHandler>
        void setFront(typename ParseHandler::DefinitionNode defn) {
        if (isMultiple())
            firstNode()->bits = ParseHandler::definitionToBits(defn);
        else
            *this = DefinitionList(ParseHandler::definitionToBits(defn));
    }

    Range all() const { return Range(*this); }

#ifdef DEBUG
    void dump();
#endif
};

typedef AtomDefnMap::Range      AtomDefnRange;
typedef AtomDefnMap::AddPtr     AtomDefnAddPtr;
typedef AtomDefnMap::Ptr        AtomDefnPtr;
typedef AtomIndexMap::AddPtr    AtomIndexAddPtr;
typedef AtomIndexMap::Ptr       AtomIndexPtr;
typedef AtomDefnListMap::Ptr    AtomDefnListPtr;
typedef AtomDefnListMap::AddPtr AtomDefnListAddPtr;
typedef AtomDefnListMap::Range  AtomDefnListRange;



















template <typename ParseHandler>
class AtomDecls
{
    typedef typename ParseHandler::DefinitionNode DefinitionNode;

    
    friend class AtomDeclsIter;

    ExclusiveContext* cx;
    LifoAlloc& alloc;
    AtomDefnListMap* map;

    AtomDecls(const AtomDecls& other) = delete;
    void operator=(const AtomDecls& other) = delete;

  public:
    explicit AtomDecls(ExclusiveContext* cx, LifoAlloc& alloc) : cx(cx),
                                                                 alloc(alloc),
                                                                 map(nullptr) {}

    ~AtomDecls();

    bool init();

    void clear() {
        map->clear();
    }

    
    DefinitionNode lookupFirst(JSAtom* atom) const {
        MOZ_ASSERT(map);
        AtomDefnListPtr p = map->lookup(atom);
        if (!p)
            return ParseHandler::nullDefinition();
        return p.value().front<ParseHandler>();
    }

    
    DefinitionList::Range lookupMulti(JSAtom* atom) const {
        MOZ_ASSERT(map);
        if (AtomDefnListPtr p = map->lookup(atom))
            return p.value().all();
        return DefinitionList::Range();
    }

    
    bool addUnique(JSAtom* atom, DefinitionNode defn) {
        MOZ_ASSERT(map);
        AtomDefnListAddPtr p = map->lookupForAdd(atom);
        if (!p)
            return map->add(p, atom, DefinitionList(ParseHandler::definitionToBits(defn)));
        MOZ_ASSERT(!p.value().isMultiple());
        p.value() = DefinitionList(ParseHandler::definitionToBits(defn));
        return true;
    }

    bool addShadow(JSAtom* atom, DefinitionNode defn);

    
    void updateFirst(JSAtom* atom, DefinitionNode defn) {
        MOZ_ASSERT(map);
        AtomDefnListMap::Ptr p = map->lookup(atom);
        MOZ_ASSERT(p);
        p.value().setFront<ParseHandler>(defn);
    }

    
    void remove(JSAtom* atom) {
        MOZ_ASSERT(map);
        AtomDefnListMap::Ptr p = map->lookup(atom);
        if (!p)
            return;

        DefinitionList& list = p.value();
        if (!list.popFront()) {
            map->remove(p);
            return;
        }
    }

    AtomDefnListMap::Range all() const {
        MOZ_ASSERT(map);
        return map->all();
    }

#ifdef DEBUG
    void dump();
#endif
};

} 

} 

namespace mozilla {

template <>
struct IsPod<js::frontend::DefinitionSingle> : TrueType {};

template <>
struct IsPod<js::frontend::DefinitionList> : TrueType {};

} 

#endif 
