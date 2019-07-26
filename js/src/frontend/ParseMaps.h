






#ifndef ParseMaps_h__
#define ParseMaps_h__

#include "mozilla/Attributes.h"
#include "mozilla/TypeTraits.h"

#include "ds/InlineMap.h"
#include "js/HashTable.h"
#include "js/Vector.h"

namespace js {
namespace frontend {

struct Definition;
class DefinitionList;

typedef InlineMap<JSAtom *, jsatomid, 24> AtomIndexMap;
typedef InlineMap<JSAtom *, Definition *, 24> AtomDefnMap;
typedef InlineMap<JSAtom *, DefinitionList, 24> AtomDefnListMap;






void
InitAtomMap(JSContext *cx, AtomIndexMap *indices, HeapPtr<JSAtom> *atoms);








class ParseMapPool
{
    typedef Vector<void *, 32, SystemAllocPolicy> RecyclableMaps;

    RecyclableMaps      all;
    RecyclableMaps      recyclable;
    JSContext           *cx;

    void checkInvariants();

    void recycle(void *map) {
        JS_ASSERT(map);
#ifdef DEBUG
        bool ok = false;
        
        for (void **it = all.begin(), **end = all.end(); it != end; ++it) {
            if (*it == map) {
                ok = true;
                break;
            }
        }
        JS_ASSERT(ok);
        for (void **it = recyclable.begin(), **end = recyclable.end(); it != end; ++it)
            JS_ASSERT(*it != map);
#endif
        JS_ASSERT(recyclable.length() < all.length());
        recyclable.infallibleAppend(map); 
    }

    void *allocateFresh();
    void *allocate();

    
    typedef AtomIndexMap AtomMapT;

    static AtomMapT *asAtomMap(void *ptr) {
        return reinterpret_cast<AtomMapT *>(ptr);
    }

  public:
    explicit ParseMapPool(JSContext *cx) : cx(cx) {}

    ~ParseMapPool() {
        purgeAll();
    }

    void purgeAll();

    bool empty() const {
        return all.empty();
    }

    
    template <typename T>
    T *acquire();

    

    void release(AtomIndexMap *map) {
        recycle((void *) map);
    }

    void release(AtomDefnMap *map) {
        recycle((void *) map);
    }

    void release(AtomDefnListMap *map) {
        recycle((void *) map);
    }
}; 





template <class Map>
struct AtomThingMapPtr
{
    Map *map_;

    void init() { clearMap(); }

    bool ensureMap(JSContext *cx);
    void releaseMap(JSContext *cx);

    bool hasMap() const { return map_; }
    Map *getMap() { return map_; }
    void setMap(Map *newMap) { JS_ASSERT(!map_); map_ = newMap; }
    void clearMap() { map_ = NULL; }

    Map *operator->() { return map_; }
    const Map *operator->() const { return map_; }
    Map &operator*() const { return *map_; }
};

struct AtomDefnMapPtr : public AtomThingMapPtr<AtomDefnMap>
{
    JS_ALWAYS_INLINE
    Definition *lookupDefn(JSAtom *atom) {
        AtomDefnMap::Ptr p = map_->lookup(atom);
        return p ? p.value() : NULL;
    }
};

typedef AtomThingMapPtr<AtomIndexMap> AtomIndexMapPtr;





template <typename AtomThingMapPtrT>
class OwnedAtomThingMapPtr : public AtomThingMapPtrT
{
    JSContext *cx;

  public:
    explicit OwnedAtomThingMapPtr(JSContext *cx) : cx(cx) {
        AtomThingMapPtrT::init();
    }

    ~OwnedAtomThingMapPtr() {
        AtomThingMapPtrT::releaseMap(cx);
    }
};

typedef OwnedAtomThingMapPtr<AtomDefnMapPtr> OwnedAtomDefnMapPtr;
typedef OwnedAtomThingMapPtr<AtomIndexMapPtr> OwnedAtomIndexMapPtr;













class DefinitionList
{
  public:
    class Range;

  private:
    friend class Range;

    
    struct Node
    {
        Definition *defn;
        Node *next;

        Node(Definition *defn, Node *next) : defn(defn), next(next) {}
    };

    union {
        Definition *defn;
        Node *head;
        uintptr_t bits;
    } u;

    Definition *defn() const {
        JS_ASSERT(!isMultiple());
        return u.defn;
    }

    Node *firstNode() const {
        JS_ASSERT(isMultiple());
        return (Node *) (u.bits & ~0x1);
    }

    static Node *
    allocNode(JSContext *cx, Definition *head, Node *tail);
            
  public:
    class Range
    {
        friend class DefinitionList;

        Node *node;
        Definition *defn;

        explicit Range(const DefinitionList &list) {
            if (list.isMultiple()) {
                node = list.firstNode();
                defn = node->defn;
            } else {
                node = NULL;
                defn = list.defn();
            }
        }

      public:
        
        Range() : node(NULL), defn(NULL) {}

        void popFront() {
            JS_ASSERT(!empty());
            if (!node) {
                defn = NULL;
                return;
            }
            node = node->next;
            defn = node ? node->defn : NULL;
        }

        Definition *front() {
            JS_ASSERT(!empty());
            return defn;
        }

        bool empty() const {
            JS_ASSERT_IF(!defn, !node);
            return !defn;
        }
    };

    DefinitionList() {
        u.bits = 0;
    }

    explicit DefinitionList(Definition *defn) {
        u.defn = defn;
        JS_ASSERT(!isMultiple());
    }

    explicit DefinitionList(Node *node) {
        u.head = node;
        u.bits |= 0x1;
        JS_ASSERT(isMultiple());
    }

    bool isMultiple() const { return (u.bits & 0x1) != 0; }

    Definition *front() {
        return isMultiple() ? firstNode()->defn : defn();
    }

    




    bool popFront() {
        if (!isMultiple())
            return false;

        Node *node = firstNode();
        Node *next = node->next;
        if (next->next)
            *this = DefinitionList(next);
        else
            *this = DefinitionList(next->defn);
        return true;
    }

    




    bool pushFront(JSContext *cx, Definition *val);

    
    bool pushBack(JSContext *cx, Definition *val);

    
    void setFront(Definition *val) {
        if (isMultiple())
            firstNode()->defn = val;
        else
            *this = DefinitionList(val);
    }

    Range all() const { return Range(*this); }

#ifdef DEBUG
    void dump();
#endif
};



















class AtomDecls
{
    
    friend class AtomDeclsIter;

    JSContext   *cx;
    AtomDefnListMap  *map;

    AtomDecls(const AtomDecls &other) MOZ_DELETE;
    void operator=(const AtomDecls &other) MOZ_DELETE;

  public:
    explicit AtomDecls(JSContext *cx) : cx(cx), map(NULL) {}

    ~AtomDecls();

    bool init();

    void clear() {
        map->clear();
    }

    
    inline Definition *lookupFirst(JSAtom *atom) const;

    
    inline DefinitionList::Range lookupMulti(JSAtom *atom) const;

    
    inline bool addUnique(JSAtom *atom, Definition *defn);
    bool addShadow(JSAtom *atom, Definition *defn);

    
    void updateFirst(JSAtom *atom, Definition *defn) {
        JS_ASSERT(map);
        AtomDefnListMap::Ptr p = map->lookup(atom);
        JS_ASSERT(p);
        p.value().setFront(defn);
    }

    
    void remove(JSAtom *atom) {
        JS_ASSERT(map);
        AtomDefnListMap::Ptr p = map->lookup(atom);
        if (!p)
            return;

        DefinitionList &list = p.value();
        if (!list.popFront()) {
            map->remove(p);
            return;
        }
    }

    AtomDefnListMap::Range all() const {
        JS_ASSERT(map);
        return map->all();
    }

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

} 

} 

namespace mozilla {

template <>
struct IsPod<js::frontend::DefinitionList> : TrueType {};

} 

#endif
