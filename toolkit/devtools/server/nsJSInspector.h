




#ifndef COMPONENTS_JSINSPECTOR_H
#define COMPONENTS_JSINSPECTOR_H

#include "nsIJSInspector.h"
#include "mozilla/Attributes.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTArray.h"
#include "js/Value.h"
#include "js/RootingAPI.h"

namespace mozilla {
namespace jsinspector {

class nsJSInspector final : public nsIJSInspector
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsJSInspector)
  NS_DECL_NSIJSINSPECTOR

  nsJSInspector();

private:
  ~nsJSInspector();

  uint32_t mNestedLoopLevel;
  nsTArray<JS::Heap<JS::Value> > mRequestors;
  JS::Heap<JS::Value> mLastRequestor;
};

} 
} 

#endif
