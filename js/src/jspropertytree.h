






































#ifndef jspropertytree_h___
#define jspropertytree_h___

#include "jsarena.h"
#include "jshashtable.h"
#include "jsprvtd.h"

namespace js {

enum {
    MAX_KIDS_PER_CHUNK   = 10U,
    CHUNK_HASH_THRESHOLD = 30U
};

struct KidsChunk {
    js::Shape   *kids[MAX_KIDS_PER_CHUNK];
    KidsChunk   *next;

    static KidsChunk *create(JSContext *cx);
    static KidsChunk *destroy(JSContext *cx, KidsChunk *chunk);
};

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
        CHUNK = 1,
        HASH  = 2,
        TAG   = 3
    };

    jsuword w;

  public:
    bool isNull() const { return !w; }
    void setNull() { w = 0; }

    bool isShapeOrNull() const { return (w & TAG) == SHAPE; }
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

    bool isChunk() const { return (w & TAG) == CHUNK; }
    KidsChunk *toChunk() const {
        JS_ASSERT(isChunk());
        return reinterpret_cast<KidsChunk *>(w & ~jsuword(TAG));
    }
    void setChunk(KidsChunk *chunk) {
        JS_ASSERT(chunk);
        JS_ASSERT((reinterpret_cast<jsuword>(chunk) & TAG) == 0);
        w = reinterpret_cast<jsuword>(chunk) | CHUNK;
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

    JSArenaPool arenaPool;
    js::Shape   *freeList;

    bool insertChild(JSContext *cx, js::Shape *parent, js::Shape *child);
    void removeChild(JSContext *cx, js::Shape *child);

  public:
    enum { MAX_HEIGHT = 64 };

    bool init();
    void finish();

    js::Shape *newShape(JSContext *cx, bool gcLocked = false);
    js::Shape *getChild(JSContext *cx, js::Shape *parent, const js::Shape &child);

    static void orphanKids(JSContext *cx, js::Shape *shape);
    static void sweepShapes(JSContext *cx);
#ifdef DEBUG
    static void meter(JSBasicStats *bs, js::Shape *node);
#endif
};

} 

#endif
