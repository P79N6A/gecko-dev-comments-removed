





































#include "nsContentCreatorFunctions.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "nsMathMLElement.h"


nsresult
NS_NewMathMLElement(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo)
{
  aNodeInfo.get()->SetIDAttributeAtom(nsGkAtoms::id);

  nsMathMLElement* it = new nsMathMLElement(aNodeInfo);
  NS_ENSURE_TRUE(it, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aResult = it);
  return NS_OK;
}
