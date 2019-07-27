




#ifndef mozilla_devtools_DeserializedNode__
#define mozilla_devtools_DeserializedNode__

#include "js/UbiNode.h"
#include "mozilla/devtools/CoreDump.pb.h"
#include "mozilla/MaybeOneOf.h"
#include "mozilla/Move.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Vector.h"












namespace mozilla {
namespace devtools {

class HeapSnapshot;

using NodeId = uint64_t;




struct DeserializedEdge {
  NodeId         referent;
  
  const char16_t* name;

  explicit DeserializedEdge();
  DeserializedEdge(DeserializedEdge&& rhs);
  DeserializedEdge& operator=(DeserializedEdge&& rhs);

  
  bool init(const protobuf::Edge& edge, HeapSnapshot& owner);

private:
  DeserializedEdge(const DeserializedEdge&) = delete;
  DeserializedEdge& operator=(const DeserializedEdge&) = delete;
};



struct DeserializedNode {
  using EdgeVector = Vector<DeserializedEdge>;
  using UniqueStringPtr = UniquePtr<char16_t[]>;

  NodeId          id;
  
  const char16_t* typeName;
  uint64_t        size;
  EdgeVector      edges;
  
  
  HeapSnapshot*   owner;

  DeserializedNode(NodeId id,
                   const char16_t* typeName,
                   uint64_t size,
                   EdgeVector&& edges,
                   HeapSnapshot& owner)
    : id(id)
    , typeName(typeName)
    , size(size)
    , edges(Move(edges))
    , owner(&owner)
  { }
  virtual ~DeserializedNode() { }

  DeserializedNode(DeserializedNode&& rhs);
  DeserializedNode& operator=(DeserializedNode&& rhs);

  
  
  virtual JS::ubi::Node getEdgeReferent(const DeserializedEdge& edge);

  struct HashPolicy;

protected:
  
  DeserializedNode(NodeId id, const char16_t* typeName, uint64_t size);

private:
  DeserializedNode(const DeserializedNode&) = delete;
  DeserializedNode& operator=(const DeserializedNode&) = delete;
};

struct DeserializedNode::HashPolicy
{
  using Lookup = NodeId;

  static js::HashNumber hash(const Lookup& lookup) {
    
    
    
    
    
    uint64_t id = lookup >> 3;
    return js::HashNumber((id >> 32) ^ id);
  }

  static bool match(const DeserializedNode& existing, const Lookup& lookup) {
    return existing.id == lookup;
  }
};

} 
} 

namespace JS {
namespace ubi {

using mozilla::devtools::DeserializedNode;
using mozilla::UniquePtr;

template<>
struct Concrete<DeserializedNode> : public Base
{
protected:
  explicit Concrete(DeserializedNode* ptr) : Base(ptr) { }
  DeserializedNode& get() const {
    return *static_cast<DeserializedNode*>(ptr);
  }

public:
  static const char16_t concreteTypeName[];

  static void construct(void* storage, DeserializedNode* ptr) {
    new (storage) Concrete(ptr);
  }

  Id identifier() const override { return get().id; }
  bool isLive() const override { return false; }
  const char16_t* typeName() const override;
  size_t size(mozilla::MallocSizeOf mallocSizeof) const override;

  
  
  UniquePtr<EdgeRange> edges(JSContext* cx, bool) const override;
};

} 
} 

#endif 
