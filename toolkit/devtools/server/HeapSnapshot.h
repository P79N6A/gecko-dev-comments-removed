




#ifndef mozilla_devtools_HeapSnapshot__
#define mozilla_devtools_HeapSnapshot__

#include "js/HashTable.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/devtools/DeserializedNode.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/Maybe.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"

#include "CoreDump.pb.h"
#include "nsCOMPtr.h"
#include "nsCRTGlue.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupports.h"
#include "nsWrapperCache.h"
#include "nsXPCOM.h"

namespace mozilla {
namespace devtools {

struct NSFreePolicy {
  void operator()(void* ptr) {
    NS_Free(ptr);
  }
};

using UniqueString = UniquePtr<char16_t[], NSFreePolicy>;

struct UniqueStringHashPolicy {
  struct Lookup {
    const char16_t* str;
    size_t          length;

    Lookup(const char16_t* str, size_t length)
      : str(str)
      , length(length)
    { }
  };

  static js::HashNumber hash(const Lookup& lookup) {
    MOZ_ASSERT(lookup.str);
    return HashString(lookup.str, lookup.length);
  }

  static bool match(const UniqueString& existing, const Lookup& lookup) {
    MOZ_ASSERT(lookup.str);
    if (NS_strlen(existing.get()) != lookup.length)
      return false;
    return memcmp(existing.get(), lookup.str, lookup.length * sizeof(char16_t)) == 0;
  }
};

class HeapSnapshot final : public nsISupports
                         , public nsWrapperCache
{
  friend struct DeserializedNode;

  explicit HeapSnapshot(JSContext* cx, nsISupports* aParent)
    : timestamp(Nothing())
    , rootId(0)
    , nodes(cx)
    , strings(cx)
    , mParent(aParent)
  {
    MOZ_ASSERT(aParent);
  };

  
  
  
  bool init(const uint8_t* buffer, uint32_t size);

  
  
  bool saveNode(const protobuf::Node& node);

  
  Maybe<uint64_t> timestamp;

  
  NodeId rootId;

  
  using NodeSet = js::HashSet<DeserializedNode, DeserializedNode::HashPolicy>;
  NodeSet nodes;

  
  
  
  
  
  
  using UniqueStringSet = js::HashSet<UniqueString, UniqueStringHashPolicy>;
  UniqueStringSet strings;

protected:
  nsCOMPtr<nsISupports> mParent;

  virtual ~HeapSnapshot() { }

public:
  
  
  
  static already_AddRefed<HeapSnapshot> Create(JSContext* cx,
                                               dom::GlobalObject& global,
                                               const uint8_t* buffer,
                                               uint32_t size,
                                               ErrorResult& rv);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(HeapSnapshot)
  MOZ_DECLARE_REFCOUNTED_TYPENAME(HeapSnapshot)

  nsISupports* GetParentObject() const { return mParent; }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  const char16_t* borrowUniqueString(const char16_t* duplicateString,
                                     size_t length);
};



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
               JS::AutoCheckCannotGC& noGC,
               uint32_t& outNodeCount,
               uint32_t& outEdgeCount);
inline bool
WriteHeapGraph(JSContext* cx,
               const JS::ubi::Node& node,
               CoreDumpWriter& writer,
               bool wantNames,
               JS::ZoneSet* zones,
               JS::AutoCheckCannotGC& noGC)
{
  uint32_t ignoreNodeCount;
  uint32_t ignoreEdgeCount;
  return WriteHeapGraph(cx, node, writer, wantNames, zones, noGC,
                        ignoreNodeCount, ignoreEdgeCount);
}

} 
} 

#endif 
