




#include "mozilla/dom/Text.h"
#include "nsTextNode.h"

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

 already_AddRefed<Text>
Text::Constructor(const GlobalObject& aGlobal,
                  const nsAString& aData, ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window || !window->GetDoc()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return window->GetDoc()->CreateTextNode(aData);
}

} 
} 
