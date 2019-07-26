




#ifndef COMPONENTS_JSINSPECTOR_H
#define COMPONENTS_JSINSPECTOR_H

#include "nsIJSInspector.h"
#include "mozilla/Attributes.h"
#include "nsTArray.h"
#include "js/Value.h"

namespace mozilla {
namespace jsinspector {

class nsJSInspector MOZ_FINAL : public nsIJSInspector
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIJSINSPECTOR

  nsJSInspector();

private:
  ~nsJSInspector();

  uint32_t mNestedLoopLevel;
  nsTArray<JS::Value> mRequestors;
  JS::Value mLastRequestor;
};

}
}

#endif
