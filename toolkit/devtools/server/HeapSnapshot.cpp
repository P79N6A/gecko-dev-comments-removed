




#include "HeapSnapshot.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "mozilla/devtools/DeserializedNode.h"
#include "mozilla/dom/HeapSnapshotBinding.h"

#include "CoreDump.pb.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCRTGlue.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace devtools {

using ::google::protobuf::io::ArrayInputStream;
using ::google::protobuf::io::CodedInputStream;
using ::google::protobuf::io::GzipInputStream;
using ::google::protobuf::io::ZeroCopyInputStream;

NS_IMPL_CYCLE_COLLECTION_CLASS(HeapSnapshot)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(HeapSnapshot)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(HeapSnapshot)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(HeapSnapshot)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(HeapSnapshot)
NS_IMPL_CYCLE_COLLECTING_RELEASE(HeapSnapshot)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(HeapSnapshot)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

 JSObject*
HeapSnapshot::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return dom::HeapSnapshotBinding::Wrap(aCx, this, aGivenProto);
}

 already_AddRefed<HeapSnapshot>
HeapSnapshot::Create(JSContext* cx,
                     dom::GlobalObject& global,
                     const uint8_t* buffer,
                     uint32_t size,
                     ErrorResult& rv)
{
  nsRefPtr<HeapSnapshot> snapshot = new HeapSnapshot(cx, global.GetAsSupports());
  if (!snapshot->init(buffer, size)) {
    rv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }
  return snapshot.forget();
}

template<typename MessageType>
static bool
parseMessage(ZeroCopyInputStream& stream, MessageType& message)
{
  
  
  CodedInputStream codedStream(&stream);

  
  
  
  

  uint32_t size = 0;
  if (NS_WARN_IF(!codedStream.ReadVarint32(&size)))
    return false;

  auto limit = codedStream.PushLimit(size);
  if (NS_WARN_IF(!message.ParseFromCodedStream(&codedStream)) ||
      NS_WARN_IF(!codedStream.ConsumedEntireMessage()))
  {
    return false;
  }

  codedStream.PopLimit(limit);
  return true;
}

bool
HeapSnapshot::saveNode(const protobuf::Node& node)
{
  if (!node.has_id())
    return false;
  NodeId id = node.id();

  if (!node.has_typename_())
    return false;

  const auto* duplicatedTypeName = reinterpret_cast<const char16_t*>(
    node.typename_().c_str());
  const char16_t* typeName = borrowUniqueString(
    duplicatedTypeName,
    node.typename_().length() / sizeof(char16_t));
  if (!typeName)
    return false;

  if (!node.has_size())
    return false;
  uint64_t size = node.size();

  auto edgesLength = node.edges_size();
  DeserializedNode::EdgeVector edges;
  if (!edges.reserve(edgesLength))
    return false;
  for (decltype(edgesLength) i = 0; i < edgesLength; i++) {
    DeserializedEdge edge;
    if (!edge.init(node.edges(i), *this))
      return false;
    edges.infallibleAppend(Move(edge));
  }

  DeserializedNode dn(id, typeName, size, Move(edges), *this);
  return nodes.putNew(id, Move(dn));
}

static inline bool
StreamHasData(GzipInputStream& stream)
{
  
  
  
  

  const void* buf;
  int size;
  bool more = stream.Next(&buf, &size);
  if (!more)
    
    
    
    return false;

  
  
  stream.BackUp(size);
  return true;
}

bool
HeapSnapshot::init(const uint8_t* buffer, uint32_t size)
{
  if (!nodes.init() || !strings.init())
    return false;

  ArrayInputStream stream(buffer, size);
  GzipInputStream gzipStream(&stream);

  

  protobuf::Metadata metadata;
  if (!parseMessage(gzipStream, metadata))
    return false;
  if (metadata.has_timestamp())
    timestamp.emplace(metadata.timestamp());

  

  protobuf::Node root;
  if (!parseMessage(gzipStream, root))
    return false;

  
  
  if (NS_WARN_IF(!root.has_id()))
    return false;
  rootId = root.id();

  if (NS_WARN_IF(!saveNode(root)))
    return false;

  

  while (StreamHasData(gzipStream)) {
    protobuf::Node node;
    if (!parseMessage(gzipStream, node))
      return false;
    if (NS_WARN_IF(!saveNode(node)))
      return false;
  }

  return true;
}

const char16_t*
HeapSnapshot::borrowUniqueString(const char16_t* duplicateString, size_t length)
{
  MOZ_ASSERT(duplicateString);
  UniqueStringHashPolicy::Lookup lookup(duplicateString, length);
  auto ptr = strings.lookupForAdd(lookup);

  if (!ptr) {
    UniqueString owned(NS_strndup(duplicateString, length));
    if (!owned || !strings.add(ptr, Move(owned)))
      return nullptr;
  }

  MOZ_ASSERT(ptr->get() != duplicateString);
  return ptr->get();
}


} 
} 
