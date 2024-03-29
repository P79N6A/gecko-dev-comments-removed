




#ifndef mozilla_dom_ChromeUtils__
#define mozilla_dom_ChromeUtils__

#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/ChromeUtilsBinding.h"
#include "mozilla/dom/ThreadSafeChromeUtilsBinding.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {

namespace devtools {
class HeapSnapshot;
} 

namespace dom {

class ThreadSafeChromeUtils
{
public:
  
  static void SaveHeapSnapshot(GlobalObject& global,
                               JSContext* cx,
                               const nsAString& filePath,
                               const HeapSnapshotBoundaries& boundaries,
                               ErrorResult& rv);

  
  static already_AddRefed<devtools::HeapSnapshot> ReadHeapSnapshot(GlobalObject& global,
                                                                   JSContext* cx,
                                                                   const nsAString& filePath,
                                                                   ErrorResult& rv);
};

class ChromeUtils : public ThreadSafeChromeUtils
{
public:
  static void
  OriginAttributesToSuffix(dom::GlobalObject& aGlobal,
                           const dom::OriginAttributesDictionary& aAttrs,
                           nsCString& aSuffix);

  static bool
  OriginAttributesMatchPattern(dom::GlobalObject& aGlobal,
                               const dom::OriginAttributesDictionary& aAttrs,
                               const dom::OriginAttributesPatternDictionary& aPattern);
};

} 
} 

#endif 
