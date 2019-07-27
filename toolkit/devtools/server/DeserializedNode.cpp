




#include "mozilla/devtools/DeserializedNode.h"
#include "mozilla/devtools/HeapSnapshot.h"
#include "nsCRTGlue.h"

namespace mozilla {
namespace devtools {

DeserializedEdge::DeserializedEdge()
  : referent(0)
  , name(nullptr)
{ }

DeserializedEdge::DeserializedEdge(DeserializedEdge&& rhs)
{
  referent = rhs.referent;
  name = rhs.name;
}

DeserializedEdge& DeserializedEdge::operator=(DeserializedEdge&& rhs)
{
  MOZ_ASSERT(&rhs != this);
  this->~DeserializedEdge();
  new(this) DeserializedEdge(Move(rhs));
  return *this;
}

bool
DeserializedEdge::init(const protobuf::Edge& edge, HeapSnapshot& owner)
{
  
  
  
  if (!edge.has_referent())
    return false;
  referent = edge.referent();

  if (edge.has_name()) {
    const char16_t* duplicateEdgeName = reinterpret_cast<const char16_t*>(edge.name().c_str());
    name = owner.borrowUniqueString(duplicateEdgeName, edge.name().length() / sizeof(char16_t));
    if (!name)
      return false;
  }

  return true;
}

DeserializedNode::DeserializedNode(DeserializedNode&& rhs)
{
  rhs.assertInitialized();

  id = rhs.id;
  rhs.id = 0;

  typeName = rhs.typeName;
  rhs.typeName = nullptr;

  size = rhs.size;
  rhs.size = 0;

  edges = Move(rhs.edges);

  owner = rhs.owner;
  rhs.owner = nullptr;
}

DeserializedNode& DeserializedNode::operator=(DeserializedNode&& rhs)
{
  MOZ_ASSERT(&rhs != this);
  this->~DeserializedNode();
  new(this) DeserializedNode(Move(rhs));
  return *this;
}

bool
DeserializedNode::init(const protobuf::Node& node, HeapSnapshot& owner)
{
  MOZ_ASSERT(this->owner == nullptr);
  MOZ_ASSERT(typeName == nullptr);

  this->owner = &owner;

  if (!node.has_id())
    return false;
  id = node.id();

  if (!node.has_typename_())
    return false;

  const char16_t* duplicatedTypeName = reinterpret_cast<const char16_t*>(node.typename_().c_str());
  typeName = owner.borrowUniqueString(duplicatedTypeName,
                                      node.typename_().length() / sizeof(char16_t));
  if (!typeName)
    return false;

  if (!node.has_size())
    return false;
  size = node.size();

  auto edgesLength = node.edges_size();
  if (!edges.reserve(edgesLength))
    return false;
  for (decltype(edgesLength) i = 0; i < edgesLength; i++) {
    DeserializedEdge edge;
    if (!edge.init(node.edges(i), owner))
      return false;
    edges.infallibleAppend(Move(edge));
  }

  return true;
}

DeserializedNode::DeserializedNode(NodeId id, const char16_t* typeName, uint64_t size)
  : id(id)
  , typeName(typeName)
  , size(size)
  , edges()
  , owner(nullptr)
{ }

JS::ubi::Node
DeserializedNode::getEdgeReferent(const DeserializedEdge& edge)
{
  assertInitialized();
  auto ptr = owner->nodes.lookup(edge.referent);
  MOZ_ASSERT(ptr);

  
  
  
  
  
  
  return JS::ubi::Node(const_cast<DeserializedNode*>(&*ptr));
}

} 
} 

namespace JS {
namespace ubi {

using mozilla::devtools::DeserializedEdge;

const char16_t Concrete<DeserializedNode>::concreteTypeName[] =
  MOZ_UTF16("mozilla::devtools::DeserializedNode");

const char16_t*
Concrete<DeserializedNode>::typeName() const
{
  return get().typeName;
}

size_t
Concrete<DeserializedNode>::size(mozilla::MallocSizeOf mallocSizeof) const
{
  return get().size;
}

class DeserializedEdgeRange : public EdgeRange
{
  SimpleEdgeVector edges;
  size_t           i;

  void settle() {
    front_ = i < edges.length() ? &edges[i] : nullptr;
  }

public:
  explicit DeserializedEdgeRange(JSContext* cx)
    : edges(cx)
    , i(0)
  {
    settle();
  }

  bool init(DeserializedNode& node)
  {
    if (!edges.reserve(node.edges.length()))
      return false;

    for (DeserializedEdge* edgep = node.edges.begin();
         edgep != node.edges.end();
         edgep++)
    {
      char16_t* name = nullptr;
      if (edgep->name) {
        name = NS_strdup(edgep->name);
        if (!name)
          return false;
      }

      auto referent = node.getEdgeReferent(*edgep);
      edges.infallibleAppend(mozilla::Move(SimpleEdge(name, referent)));
    }

    settle();
    return true;
  }

  void popFront() override
  {
    i++;
    settle();
  }
};

UniquePtr<EdgeRange>
Concrete<DeserializedNode>::edges(JSContext* cx, bool) const
{
  UniquePtr<DeserializedEdgeRange, JS::DeletePolicy<DeserializedEdgeRange>> range(
    js_new<DeserializedEdgeRange>(cx));

  if (!range || !range->init(get()))
    return nullptr;

  return UniquePtr<EdgeRange>(range.release());
}

} 
} 
