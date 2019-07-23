





































#include "nsContentCreatorFunctions.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "nsMathMLElement.h"


nsresult
NS_NewMathMLElement(nsIContent** aResult, nsINodeInfo* aNodeInfo)
{
  aNodeInfo->SetIDAttributeAtom(nsGkAtoms::id);

  nsMathMLElement* it = new nsMathMLElement(aNodeInfo);
  NS_ENSURE_TRUE(it, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aResult = it);
  return NS_OK;
}
