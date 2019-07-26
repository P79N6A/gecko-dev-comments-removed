




#include "mozilla/dom/Text.h"

namespace mozilla {
namespace dom {

already_AddRefed<Text>
Text::SplitText(uint32_t aOffset, ErrorResult& rv)
{
  nsCOMPtr<nsIContent> newChild;
  rv = SplitData(aOffset, getter_AddRefs(newChild));
  if (rv.Failed()) {
    return nullptr;
  }
  return newChild.forget().downcast<Text>();
}

} 
} 
