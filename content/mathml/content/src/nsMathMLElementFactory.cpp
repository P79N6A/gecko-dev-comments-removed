




#include "nsContentCreatorFunctions.h"
#include "nsGkAtoms.h"
#include "nsMathMLElement.h"

using namespace mozilla::dom;


nsresult
NS_NewMathMLElement(Element** aResult, already_AddRefed<nsINodeInfo> aNodeInfo)
{
  aNodeInfo.get()->SetIDAttributeAtom(nsGkAtoms::id);

  nsMathMLElement* it = new nsMathMLElement(aNodeInfo);

  NS_ADDREF(*aResult = it);
  return NS_OK;
}
