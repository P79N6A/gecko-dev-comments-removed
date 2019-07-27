




#include "nsContentCreatorFunctions.h"
#include "nsGkAtoms.h"
#include "nsMathMLElement.h"

using namespace mozilla::dom;


nsresult
NS_NewMathMLElement(Element** aResult, already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo)
{
  NS_ADDREF(*aResult = new nsMathMLElement(aNodeInfo));
  return NS_OK;
}
