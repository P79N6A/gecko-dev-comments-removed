




#ifndef mozilla_devtools_DeserializedNode__
#define mozilla_devtools_DeserializedNode__

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
  
  const char16_t *name;

  explicit DeserializedEdge();
  DeserializedEdge(DeserializedEdge &&rhs);
  DeserializedEdge &operator=(DeserializedEdge &&rhs);

  
  bool init(const protobuf::Edge &edge, HeapSnapshot &owner);

private:
  DeserializedEdge(const DeserializedEdge &) = delete;
  DeserializedEdge& operator=(const DeserializedEdge &) = delete;
};



struct DeserializedNode {
  using EdgeVector = Vector<DeserializedEdge>;
  using UniqueStringPtr = UniquePtr<char16_t[]>;

  NodeId         id;
  
  const char16_t *typeName;
  uint64_t       size;
  EdgeVector     edges;
  
  
  HeapSnapshot   *owner;

  
  static UniquePtr<DeserializedNode> Create(const protobuf::Node &node,
                                            HeapSnapshot &owner);

  DeserializedNode(NodeId id,
                   const char16_t *typeName,
                   uint64_t size,
                   EdgeVector &&edges,
                   HeapSnapshot &owner);
  virtual ~DeserializedNode() { }

  
  
  virtual DeserializedNode &getEdgeReferent(const DeserializedEdge &edge);

private:
  DeserializedNode(const DeserializedNode &) = delete;
  DeserializedNode &operator=(const DeserializedNode &) = delete;
};

} 
} 

#endif 
