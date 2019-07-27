




#ifndef mozilla_devtools_ChromeUtils__
#define mozilla_devtools_ChromeUtils__

#include "CoreDump.pb.h"
#include "jsapi.h"
#include "jsfriendapi.h"

#include "js/UbiNode.h"
#include "js/UbiNodeTraverse.h"
#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/ChromeUtilsBinding.h"

namespace mozilla {
namespace devtools {



class CoreDumpWriter
{
public:
  virtual ~CoreDumpWriter() { };

  
  
  virtual bool writeMetadata(uint64_t timestamp) = 0;

  enum EdgePolicy : bool {
    INCLUDE_EDGES = true,
    EXCLUDE_EDGES = false
  };

  
  
  
  virtual bool writeNode(const JS::ubi::Node& node,
                         EdgePolicy includeEdges) = 0;
};






bool
WriteHeapGraph(JSContext* cx,
               const JS::ubi::Node& node,
               CoreDumpWriter& writer,
               bool wantNames,
               JS::ZoneSet* zones,
               JS::AutoCheckCannotGC& noGC);


class HeapSnapshot;


class ChromeUtils
{
public:
  static void SaveHeapSnapshot(dom::GlobalObject& global,
                               JSContext* cx,
                               const nsAString& filePath,
                               const dom::HeapSnapshotBoundaries& boundaries,
                               ErrorResult& rv);

  static already_AddRefed<HeapSnapshot> ReadHeapSnapshot(dom::GlobalObject& global,
                                                         JSContext* cx,
                                                         const nsAString& filePath,
                                                         ErrorResult& rv);
};

} 
} 

#endif 
