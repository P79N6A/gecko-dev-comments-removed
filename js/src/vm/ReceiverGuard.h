





#ifndef vm_ReceiverGuard_h
#define vm_ReceiverGuard_h

#include "vm/Shape.h"

namespace js {
























class HeapReceiverGuard;
class RootedReceiverGuard;

class ReceiverGuard
{
  public:
    ObjectGroup* group;
    Shape* shape;

    ReceiverGuard()
      : group(nullptr), shape(nullptr)
    {}

    inline MOZ_IMPLICIT ReceiverGuard(const HeapReceiverGuard& guard);
    inline MOZ_IMPLICIT ReceiverGuard(const RootedReceiverGuard& guard);

    explicit ReceiverGuard(JSObject* obj);
    ReceiverGuard(ObjectGroup* group, Shape* shape);

    bool operator ==(const ReceiverGuard& other) const {
        return group == other.group && shape == other.shape;
    }

    bool operator !=(const ReceiverGuard& other) const {
        return !(*this == other);
    }

    uintptr_t hash() const {
        return (uintptr_t(group) >> 3) ^ (uintptr_t(shape) >> 3);
    }
};

class HeapReceiverGuard
{
    HeapPtrObjectGroup group_;
    HeapPtrShape shape_;

  public:
    explicit HeapReceiverGuard(const ReceiverGuard& guard)
      : group_(guard.group), shape_(guard.shape)
    {}

    bool matches(const ReceiverGuard& guard) {
        return group_ == guard.group && shape_ == guard.shape;
    }

    void update(const ReceiverGuard& other) {
        group_ = other.group;
        shape_ = other.shape;
    }

    void init(const ReceiverGuard& other) {
        group_.init(other.group);
        shape_.init(other.shape);
    }

    void trace(JSTracer* trc);

    Shape* shape() const {
        return shape_;
    }
    ObjectGroup* group() const {
        return group_;
    }

    static size_t offsetOfShape() {
        return offsetof(HeapReceiverGuard, shape_);
    }
    static size_t offsetOfGroup() {
        return offsetof(HeapReceiverGuard, group_);
    }

    
    
    static int32_t keyBits(JSObject* obj);
};

class RootedReceiverGuard
{
  public:
    RootedObjectGroup group;
    RootedShape shape;

    RootedReceiverGuard(JSContext* cx, const ReceiverGuard& guard)
      : group(cx, guard.group), shape(cx, guard.shape)
    {}
};

inline
ReceiverGuard::ReceiverGuard(const HeapReceiverGuard& guard)
  : group(guard.group()), shape(guard.shape())
{}

inline
ReceiverGuard::ReceiverGuard(const RootedReceiverGuard& guard)
  : group(guard.group), shape(guard.shape)
{}

} 

#endif 
