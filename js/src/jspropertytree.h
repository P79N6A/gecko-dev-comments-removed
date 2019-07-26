





#ifndef jspropertytree_h___
#define jspropertytree_h___

#include "js/HashTable.h"

namespace js {

ForwardDeclare(Shape);
struct StackShape;

struct ShapeHasher {
    typedef RawShape Key;
    typedef StackShape Lookup;

    static inline HashNumber hash(const Lookup &l);
    static inline bool match(Key k, const Lookup &l);
};

typedef HashSet<RawShape, ShapeHasher, SystemAllocPolicy> KidsHash;

class KidsPointer {
  private:
    enum {
        SHAPE = 0,
        HASH  = 1,
        TAG   = 1
    };

    uintptr_t w;

  public:
    bool isNull() const { return !w; }
    void setNull() { w = 0; }

    bool isShape() const { return (w & TAG) == SHAPE && !isNull(); }
    UnrootedShape toShape() const {
        JS_ASSERT(isShape());
        return reinterpret_cast<RawShape>(w & ~uintptr_t(TAG));
    }
    void setShape(UnrootedShape shape) {
        JS_ASSERT(shape);
        JS_ASSERT((reinterpret_cast<uintptr_t>(static_cast<RawShape>(shape)) & TAG) == 0);
        w = reinterpret_cast<uintptr_t>(static_cast<RawShape>(shape)) | SHAPE;
    }

    bool isHash() const { return (w & TAG) == HASH; }
    KidsHash *toHash() const {
        JS_ASSERT(isHash());
        return reinterpret_cast<KidsHash *>(w & ~uintptr_t(TAG));
    }
    void setHash(KidsHash *hash) {
        JS_ASSERT(hash);
        JS_ASSERT((reinterpret_cast<uintptr_t>(hash) & TAG) == 0);
        w = reinterpret_cast<uintptr_t>(hash) | HASH;
    }

#ifdef DEBUG
    void checkConsistency(UnrootedShape aKid) const;
#endif
};

class PropertyTree
{
    friend struct ::JSFunction;

    JSCompartment *compartment;

    bool insertChild(JSContext *cx, UnrootedShape parent, UnrootedShape child);

    PropertyTree();

  public:
    enum { MAX_HEIGHT = 128 };

    PropertyTree(JSCompartment *comp)
        : compartment(comp)
    {
    }

    UnrootedShape newShape(JSContext *cx);
    UnrootedShape getChild(JSContext *cx, Shape *parent, uint32_t nfixed, const StackShape &child);

#ifdef DEBUG
    static void dumpShapes(JSRuntime *rt);
#endif
};

} 

#endif
