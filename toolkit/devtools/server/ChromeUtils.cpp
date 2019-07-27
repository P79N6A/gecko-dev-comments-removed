




#include "ChromeUtils.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/gzip_stream.h>

#include "mozilla/devtools/HeapSnapshot.h"
#include "mozilla/devtools/ZeroCopyNSIOutputStream.h"
#include "mozilla/Attributes.h"
#include "mozilla/UniquePtr.h"

#include "nsCRTGlue.h"
#include "nsIOutputStream.h"
#include "nsNetUtil.h"
#include "prerror.h"
#include "prio.h"
#include "prtypes.h"

#include "js/Debug.h"
#include "js/UbiNodeTraverse.h"

namespace mozilla {
namespace devtools {

using namespace JS;
using namespace dom;





static bool
PopulateZonesWithGlobals(ZoneSet &zones, AutoObjectVector &globals)
{
  if (!zones.init())
    return false;

  unsigned length = globals.length();
  for (unsigned i = 0; i < length; i++) {
    if (!zones.put(GetTenuredGCThingZone(globals[i])))
      return false;
  }

  return true;
}



static bool
AddGlobalsAsRoots(AutoObjectVector &globals, ubi::RootList &roots)
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
EstablishBoundaries(JSContext *cx,
                    ErrorResult &rv,
                    const HeapSnapshotBoundaries &boundaries,
                    ubi::RootList &roots,
                    ZoneSet &zones)
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

    JSObject *dbgObj = boundaries.mDebugger.Value();
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
      JSObject *global = boundaries.mGlobals.Value().ElementAt(i);
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
  JSContext *cx;
  bool      wantNames;

  ::google::protobuf::io::ZeroCopyOutputStream &stream;

  bool writeMessage(const ::google::protobuf::MessageLite &message) {
    
    
    
    ::google::protobuf::io::CodedOutputStream codedStream(&stream);
    codedStream.WriteVarint32(message.ByteSize());
    message.SerializeWithCachedSizes(&codedStream);
    return !codedStream.HadError();
  }

public:
  StreamWriter(JSContext *cx,
               ::google::protobuf::io::ZeroCopyOutputStream &stream,
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

  virtual bool writeNode(const JS::ubi::Node &ubiNode,
                         EdgePolicy includeEdges) override {
    protobuf::Node protobufNode;
    protobufNode.set_id(ubiNode.identifier());

    const char16_t *typeName = ubiNode.typeName();
    size_t length = NS_strlen(typeName) * sizeof(char16_t);
    protobufNode.set_typename_(typeName, length);

    JSRuntime *rt = JS_GetRuntime(cx);
    mozilla::MallocSizeOf mallocSizeOf = dbg::GetDebuggerMallocSizeOf(rt);
    MOZ_ASSERT(mallocSizeOf);
    protobufNode.set_size(ubiNode.size(mallocSizeOf));

    if (includeEdges) {
      auto edges = ubiNode.edges(cx, wantNames);
      if (NS_WARN_IF(!edges))
        return false;

      for ( ; !edges->empty(); edges->popFront()) {
        const ubi::Edge &ubiEdge = edges->front();

        protobuf::Edge *protobufEdge = protobufNode.add_edges();
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



class MOZ_STACK_CLASS HeapSnapshotHandler {
  CoreDumpWriter& writer;
  JS::ZoneSet*   zones;

public:
  HeapSnapshotHandler(CoreDumpWriter& writer,
                      JS::ZoneSet* zones)
    : writer(writer),
      zones(zones)
  { }

  

  class NodeData { };
  typedef JS::ubi::BreadthFirst<HeapSnapshotHandler> Traversal;
  bool operator() (Traversal &traversal,
                   JS::ubi::Node origin,
                   const JS::ubi::Edge &edge,
                   NodeData *,
                   bool first)
  {
    
    
    
    
    
    
    if (!first)
      return true;

    const JS::ubi::Node &referent = edge.referent;

    if (!zones)
      
      
      return writer.writeNode(referent, CoreDumpWriter::INCLUDE_EDGES);

    
    
    
    
    
    

    JS::Zone *zone = referent.zone();

    if (zones->has(zone))
      return writer.writeNode(referent, CoreDumpWriter::INCLUDE_EDGES);

    traversal.abandonReferent();
    return writer.writeNode(referent, CoreDumpWriter::EXCLUDE_EDGES);
  }
};


bool
WriteHeapGraph(JSContext *cx,
               const JS::ubi::Node &node,
               CoreDumpWriter &writer,
               bool wantNames,
               JS::ZoneSet *zones,
               JS::AutoCheckCannotGC &noGC)
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

 void
ChromeUtils::SaveHeapSnapshot(GlobalObject &global,
                              JSContext *cx,
                              const nsAString &filePath,
                              const HeapSnapshotBoundaries &boundaries,
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

}
}
