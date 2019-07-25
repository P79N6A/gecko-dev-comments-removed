






































#ifndef jspropertytree_h___
#define jspropertytree_h___

#include "jsarena.h"
#include "jshashtable.h"
#include "jsprvtd.h"

namespace js {

struct ShapeHasher {
    typedef js::Shape *Key;
    typedef const js::Shape *Lookup;

    static inline HashNumber hash(const Lookup l);
    static inline bool match(Key k, Lookup l);
};

typedef HashSet<js::Shape *, ShapeHasher, SystemAllocPolicy> KidsHash;

class KidsPointer {
  private:
    enum {
        SHAPE = 0,
        HASH  = 1,
        TAG   = 1
    };

    jsuword w;

  public:
    bool isNull() const { return !w; }
    void setNull() { w = 0; }

    bool isShape() const { return (w & TAG) == SHAPE && !isNull(); }
    js::Shape *toShape() const {
        JS_ASSERT(isShape());
        return reinterpret_cast<js::Shape *>(w & ~jsuword(TAG));
    }
    void setShape(js::Shape *shape) {
        JS_ASSERT(shape);
        JS_ASSERT((reinterpret_cast<jsuword>(shape) & TAG) == 0);
        w = reinterpret_cast<jsuword>(shape) | SHAPE;
    }

    bool isHash() const { return (w & TAG) == HASH; }
    KidsHash *toHash() const {
        JS_ASSERT(isHash());
        return reinterpret_cast<KidsHash *>(w & ~jsuword(TAG));
    }
    void setHash(KidsHash *hash) {
        JS_ASSERT(hash);
        JS_ASSERT((reinterpret_cast<jsuword>(hash) & TAG) == 0);
        w = reinterpret_cast<jsuword>(hash) | HASH;
    }

#ifdef DEBUG
    void checkConsistency(const js::Shape *aKid) const;
#endif
};

class PropertyTree
{
    friend struct ::JSFunction;

    JSCompartment *compartment;
    JSArenaPool   arenaPool;
    js::Shape     *freeList;

    bool insertChild(JSContext *cx, js::Shape *parent, js::Shape *child);
    void removeChild(js::Shape *child);

    PropertyTree();
    
  public:
    enum { MAX_HEIGHT = 128 };

    PropertyTree(JSCompartment *comp)
        : compartment(comp), freeList(NULL)
    {
        PodZero(&arenaPool);
    }
    
    bool init();
    void finish();

    js::Shape *newShapeUnchecked();
    js::Shape *newShape(JSContext *cx);
    js::Shape *getChild(JSContext *cx, js::Shape *parent, const js::Shape &child);

    void orphanChildren(js::Shape *shape);
    void sweepShapes(JSContext *cx);
    bool checkShapesAllUnmarked(JSContext *cx);

    static void dumpShapes(JSContext *cx);
#ifdef DEBUG
    static void meter(JSBasicStats *bs, js::Shape *node);
#endif
};

} 

#endif
