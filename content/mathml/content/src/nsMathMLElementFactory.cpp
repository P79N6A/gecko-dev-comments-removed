




#include "nsContentCreatorFunctions.h"
#include "nsGkAtoms.h"
#include "nsMathMLElement.h"


nsresult
NS_NewMathMLElement(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo)
{
  aNodeInfo.get()->SetIDAttributeAtom(nsGkAtoms::id);

  nsMathMLElement* it = new nsMathMLElement(aNodeInfo);

  NS_ADDREF(*aResult = it);
  return NS_OK;
}
