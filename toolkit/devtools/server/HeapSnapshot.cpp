




#include "HeapSnapshot.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "js/Debug.h"
#include "js/TypeDecls.h"
#include "js/UbiNodeTraverse.h"
#include "mozilla/Attributes.h"
#include "mozilla/devtools/AutoMemMap.h"
#include "mozilla/devtools/CoreDump.pb.h"
#include "mozilla/devtools/DeserializedNode.h"
#include "mozilla/devtools/ZeroCopyNSIOutputStream.h"
#include "mozilla/dom/ChromeUtils.h"
#include "mozilla/dom/HeapSnapshotBinding.h"
#include "mozilla/UniquePtr.h"

#include "jsapi.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCRTGlue.h"
#include "nsIOutputStream.h"
#include "nsISupportsImpl.h"
#include "nsNetUtil.h"
#include "prerror.h"
#include "prio.h"
#include "prtypes.h"

namespace mozilla {
namespace devtools {

using namespace JS;
using namespace dom;

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
HeapSnapshot::WrapObject(JSContext* aCx, HandleObject aGivenProto)
{
  return HeapSnapshotBinding::Wrap(aCx, this, aGivenProto);
}

 already_AddRefed<HeapSnapshot>
HeapSnapshot::Create(JSContext* cx,
                     GlobalObject& global,
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




static bool
PopulateZonesWithGlobals(ZoneSet& zones, AutoObjectVector& globals)
{
  if (!zones.init())
    return false;

  unsigned length = globals.length();
  for (unsigned i = 0; i < length; i++) {
    if (!zones.put(GetObjectZone(globals[i])))
      return false;
  }

  return true;
}



static bool
AddGlobalsAsRoots(AutoObjectVector& globals, ubi::RootList& roots)
{
  unsigned length = globals.length();
  for (unsigned i = 0; i < length; i++) {
    if (!roots.addRoot(ubi::Node(globals[i].get()),
                       MOZ_UTF16("heap snapshot global")))
    {
      return false;
    }
  }
  return true;
}










static bool
EstablishBoundaries(JSContext* cx,
                    ErrorResult& rv,
                    const HeapSnapshotBoundaries& boundaries,
                    ubi::RootList& roots,
                    ZoneSet& zones)
{
  MOZ_ASSERT(!roots.initialized());
  MOZ_ASSERT(!zones.initialized());

  bool foundBoundaryProperty = false;

  if (boundaries.mRuntime.WasPassed()) {
    foundBoundaryProperty = true;

    if (!boundaries.mRuntime.Value()) {
      rv.Throw(NS_ERROR_INVALID_ARG);
      return false;
    }

    if (!roots.init()) {
      rv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return false;
    }
  }

  if (boundaries.mDebugger.WasPassed()) {
    if (foundBoundaryProperty) {
      rv.Throw(NS_ERROR_INVALID_ARG);
      return false;
    }
    foundBoundaryProperty = true;

    JSObject* dbgObj = boundaries.mDebugger.Value();
    if (!dbgObj || !dbg::IsDebugger(*dbgObj)) {
      rv.Throw(NS_ERROR_INVALID_ARG);
      return false;
    }

    AutoObjectVector globals(cx);
    if (!dbg::GetDebuggeeGlobals(cx, *dbgObj, globals) ||
        !PopulateZonesWithGlobals(zones, globals) ||
        !roots.init(zones) ||
        !AddGlobalsAsRoots(globals, roots))
    {
      rv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return false;
    }
  }

  if (boundaries.mGlobals.WasPassed()) {
    if (foundBoundaryProperty) {
      rv.Throw(NS_ERROR_INVALID_ARG);
      return false;
    }
    foundBoundaryProperty = true;

    uint32_t length = boundaries.mGlobals.Value().Length();
    if (length == 0) {
      rv.Throw(NS_ERROR_INVALID_ARG);
      return false;
    }

    AutoObjectVector globals(cx);
    for (uint32_t i = 0; i < length; i++) {
      JSObject* global = boundaries.mGlobals.Value().ElementAt(i);
      if (!JS_IsGlobalObject(global)) {
        rv.Throw(NS_ERROR_INVALID_ARG);
        return false;
      }
      if (!globals.append(global)) {
        rv.Throw(NS_ERROR_OUT_OF_MEMORY);
        return false;
      }
    }

    if (!PopulateZonesWithGlobals(zones, globals) ||
        !roots.init(zones) ||
        !AddGlobalsAsRoots(globals, roots))
    {
      rv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return false;
    }
  }

  if (!foundBoundaryProperty) {
    rv.Throw(NS_ERROR_INVALID_ARG);
    return false;
  }

  MOZ_ASSERT(roots.initialized());
  MOZ_ASSERT_IF(boundaries.mDebugger.WasPassed(), zones.initialized());
  MOZ_ASSERT_IF(boundaries.mGlobals.WasPassed(), zones.initialized());
  return true;
}




class MOZ_STACK_CLASS StreamWriter : public CoreDumpWriter
{
  JSContext* cx;
  bool      wantNames;

  ::google::protobuf::io::ZeroCopyOutputStream& stream;

  bool writeMessage(const ::google::protobuf::MessageLite& message) {
    
    
    
    ::google::protobuf::io::CodedOutputStream codedStream(&stream);
    codedStream.WriteVarint32(message.ByteSize());
    message.SerializeWithCachedSizes(&codedStream);
    return !codedStream.HadError();
  }

public:
  StreamWriter(JSContext* cx,
               ::google::protobuf::io::ZeroCopyOutputStream& stream,
               bool wantNames)
    : cx(cx)
    , wantNames(wantNames)
    , stream(stream)
  { }

  ~StreamWriter() override { }

  virtual bool writeMetadata(uint64_t timestamp) override {
    protobuf::Metadata metadata;
    metadata.set_timestamp(timestamp);
    return writeMessage(metadata);
  }

  virtual bool writeNode(const JS::ubi::Node& ubiNode,
                         EdgePolicy includeEdges) override {
    protobuf::Node protobufNode;
    protobufNode.set_id(ubiNode.identifier());

    const char16_t* typeName = ubiNode.typeName();
    size_t length = NS_strlen(typeName) * sizeof(char16_t);
    protobufNode.set_typename_(typeName, length);

    JSRuntime* rt = JS_GetRuntime(cx);
    mozilla::MallocSizeOf mallocSizeOf = dbg::GetDebuggerMallocSizeOf(rt);
    MOZ_ASSERT(mallocSizeOf);
    protobufNode.set_size(ubiNode.size(mallocSizeOf));

    if (includeEdges) {
      auto edges = ubiNode.edges(cx, wantNames);
      if (NS_WARN_IF(!edges))
        return false;

      for ( ; !edges->empty(); edges->popFront()) {
        const ubi::Edge& ubiEdge = edges->front();

        protobuf::Edge* protobufEdge = protobufNode.add_edges();
        if (NS_WARN_IF(!protobufEdge)) {
          return false;
        }

        protobufEdge->set_referent(ubiEdge.referent.identifier());

        if (wantNames && ubiEdge.name) {
          size_t length = NS_strlen(ubiEdge.name) * sizeof(char16_t);
          protobufEdge->set_name(ubiEdge.name, length);
        }
      }
    }

    return writeMessage(protobufNode);
  }
};



class MOZ_STACK_CLASS HeapSnapshotHandler
{
  CoreDumpWriter& writer;
  JS::ZoneSet*    zones;

public:
  HeapSnapshotHandler(CoreDumpWriter& writer,
                      JS::ZoneSet* zones)
    : writer(writer),
      zones(zones)
  { }

  

  class NodeData { };
  typedef JS::ubi::BreadthFirst<HeapSnapshotHandler> Traversal;
  bool operator() (Traversal& traversal,
                   JS::ubi::Node origin,
                   const JS::ubi::Edge& edge,
                   NodeData*,
                   bool first)
  {
    
    
    
    
    
    
    if (!first)
      return true;

    const JS::ubi::Node& referent = edge.referent;

    if (!zones)
      
      
      return writer.writeNode(referent, CoreDumpWriter::INCLUDE_EDGES);

    
    
    
    
    
    

    JS::Zone* zone = referent.zone();

    if (zones->has(zone))
      return writer.writeNode(referent, CoreDumpWriter::INCLUDE_EDGES);

    traversal.abandonReferent();
    return writer.writeNode(referent, CoreDumpWriter::EXCLUDE_EDGES);
  }
};


bool
WriteHeapGraph(JSContext* cx,
               const JS::ubi::Node& node,
               CoreDumpWriter& writer,
               bool wantNames,
               JS::ZoneSet* zones,
               JS::AutoCheckCannotGC& noGC)
{
  

  if (NS_WARN_IF(!writer.writeNode(node, CoreDumpWriter::INCLUDE_EDGES))) {
    return false;
  }

  
  

  HeapSnapshotHandler handler(writer, zones);
  HeapSnapshotHandler::Traversal traversal(cx, handler, noGC);
  if (!traversal.init())
    return false;
  traversal.wantNames = wantNames;

  return traversal.addStartVisited(node) &&
    traversal.traverse();
}

} 

namespace dom {

using namespace JS;
using namespace devtools;

 void
ThreadSafeChromeUtils::SaveHeapSnapshot(GlobalObject& global,
                                        JSContext* cx,
                                        const nsAString& filePath,
                                        const HeapSnapshotBoundaries& boundaries,
                                        ErrorResult& rv)
{
  bool wantNames = true;
  ZoneSet zones;
  Maybe<AutoCheckCannotGC> maybeNoGC;
  ubi::RootList rootList(cx, maybeNoGC, wantNames);
  if (!EstablishBoundaries(cx, rv, boundaries, rootList, zones))
    return;

  MOZ_ASSERT(maybeNoGC.isSome());
  ubi::Node roots(&rootList);

  nsCOMPtr<nsIFile> file;
  rv = NS_NewLocalFile(filePath, false, getter_AddRefs(file));
  if (NS_WARN_IF(rv.Failed()))
    return;

  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), file,
                                   PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
                                   -1, 0);
  if (NS_WARN_IF(rv.Failed()))
    return;

  ZeroCopyNSIOutputStream zeroCopyStream(outputStream);
  ::google::protobuf::io::GzipOutputStream gzipStream(&zeroCopyStream);

  StreamWriter writer(cx, gzipStream, wantNames);

  
  if (!writer.writeMetadata(PR_Now()) ||
      
      
      !WriteHeapGraph(cx,
                      roots,
                      writer,
                      wantNames,
                      zones.initialized() ? &zones : nullptr,
                      maybeNoGC.ref()))
    {
      rv.Throw(zeroCopyStream.failed()
               ? zeroCopyStream.result()
               : NS_ERROR_UNEXPECTED);
      return;
    }
}

 already_AddRefed<HeapSnapshot>
ThreadSafeChromeUtils::ReadHeapSnapshot(GlobalObject& global,
                                        JSContext* cx,
                                        const nsAString& filePath,
                                        ErrorResult& rv)
{
  UniquePtr<char[]> path(ToNewCString(filePath));
  if (!path) {
    rv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  AutoMemMap mm;
  rv = mm.init(path.get());
  if (rv.Failed())
    return nullptr;

  return HeapSnapshot::Create(cx, global,
                              reinterpret_cast<const uint8_t*>(mm.address()),
                              mm.size(), rv);
}

} 
} 
