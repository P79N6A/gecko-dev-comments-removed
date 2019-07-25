







































#ifndef ParseMaps_h__
#define ParseMaps_h__

#include "jsvector.h"

#include "mfbt/InlineMap.h"

namespace js {








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

    void release(AtomDOHMap *map) {
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
    JSDefinition *lookupDefn(JSAtom *atom) {
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


struct AtomDeclNode
{
    JSDefinition *defn;
    AtomDeclNode *next;

    explicit AtomDeclNode(JSDefinition *defn)
      : defn(defn), next(NULL)
    {}
};





class DefnOrHeader
{
    union {
        JSDefinition    *defn;
        AtomDeclNode    *head;
        uintptr_t       bits;
    } u;

  public:
    DefnOrHeader() {
        u.bits = 0;
    }

    explicit DefnOrHeader(JSDefinition *defn) {
        u.defn = defn;
        JS_ASSERT(!isHeader());
    }

    explicit DefnOrHeader(AtomDeclNode *node) {
        u.head = node;
        u.bits |= 0x1;
        JS_ASSERT(isHeader());
    }

    bool isHeader() const {
        return u.bits & 0x1;
    }

    JSDefinition *defn() const {
        JS_ASSERT(!isHeader());
        return u.defn;
    }

    AtomDeclNode *header() const {
        JS_ASSERT(isHeader());
        return (AtomDeclNode *) (u.bits & ~0x1);
    }

#ifdef DEBUG
    void dump();
#endif
};

namespace tl {

template <> struct IsPodType<DefnOrHeader> {
    static const bool result = true;
};

} 










class AtomDecls
{
    
    friend class AtomDeclsIter;

    JSContext   *cx;
    AtomDOHMap  *map;

    AtomDecls(const AtomDecls &other);
    void operator=(const AtomDecls &other);

    AtomDeclNode *allocNode(JSDefinition *defn);

    



    AtomDeclNode *lastAsNode(DefnOrHeader *doh);

  public:
    explicit AtomDecls(JSContext *cx)
      : cx(cx), map(NULL)
    {}

    ~AtomDecls();

    bool init();

    void clear() {
        map->clear();
    }

    
    inline JSDefinition *lookupFirst(JSAtom *atom);

    
    inline MultiDeclRange lookupMulti(JSAtom *atom);

    
    inline bool addUnique(JSAtom *atom, JSDefinition *defn);
    bool addShadow(JSAtom *atom, JSDefinition *defn);
    bool addHoist(JSAtom *atom, JSDefinition *defn);

    
    void update(JSAtom *atom, JSDefinition *defn) {
        JS_ASSERT(map);
        AtomDOHMap::Ptr p = map->lookup(atom);
        JS_ASSERT(p);
        JS_ASSERT(!p.value().isHeader());
        p.value() = DefnOrHeader(defn);
    }

    
    void remove(JSAtom *atom) {
        JS_ASSERT(map);
        AtomDOHMap::Ptr p = map->lookup(atom);
        if (!p)
            return;

        DefnOrHeader &doh = p.value();
        if (!doh.isHeader()) {
            map->remove(p);
            return;
        }

        AtomDeclNode *node = doh.header();
        AtomDeclNode *newHead = node->next;
        if (newHead)
            p.value() = DefnOrHeader(newHead);
        else
            map->remove(p);
    }

    AtomDOHMap::Range all() {
        JS_ASSERT(map);
        return map->all();
    }

#ifdef DEBUG
    void dump();
#endif
};






class MultiDeclRange
{
    friend class AtomDecls;

    AtomDeclNode *node;
    JSDefinition *defn;

    explicit MultiDeclRange(JSDefinition *defn) : node(NULL), defn(defn) {}
    explicit MultiDeclRange(AtomDeclNode *node) : node(node), defn(node->defn) {}

  public:
    void popFront() {
        JS_ASSERT(!empty());
        if (!node) {
            defn = NULL;
            return;
        }
        node = node->next;
        defn = node ? node->defn : NULL;
    }

    JSDefinition *front() {
        JS_ASSERT(!empty());
        return defn;
    }

    bool empty() const {
        JS_ASSERT_IF(!defn, !node);
        return !defn;
    }
};


class AtomDeclsIter
{
    AtomDOHMap::Range   r;     
    AtomDeclNode        *link; 

  public:
    explicit AtomDeclsIter(AtomDecls *decls) : r(decls->all()), link(NULL) {}

    JSDefinition *operator()() {
        if (link) {
            JS_ASSERT(link != link->next);
            JSDefinition *result = link->defn;
            link = link->next;
            JS_ASSERT(result);
            return result;
        }

        if (r.empty())
            return NULL;

        const DefnOrHeader &doh = r.front().value();
        r.popFront();

        if (!doh.isHeader())
            return doh.defn();

        JS_ASSERT(!link);
        AtomDeclNode *node = doh.header();
        link = node->next;
        return node->defn;
    }
};

typedef AtomDefnMap::Range      AtomDefnRange;
typedef AtomDefnMap::AddPtr     AtomDefnAddPtr;
typedef AtomDefnMap::Ptr        AtomDefnPtr;
typedef AtomIndexMap::AddPtr    AtomIndexAddPtr;
typedef AtomIndexMap::Ptr       AtomIndexPtr;
typedef AtomDOHMap::Ptr         AtomDOHPtr;
typedef AtomDOHMap::AddPtr      AtomDOHAddPtr;
typedef AtomDOHMap::Range       AtomDOHRange;

} 

#endif
