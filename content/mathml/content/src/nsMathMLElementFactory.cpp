




#include "nsContentCreatorFunctions.h"
#include "nsGkAtoms.h"
#include "nsMathMLElement.h"

using namespace mozilla::dom;


nsresult
NS_NewMathMLElement(Element** aResult, already_AddRefed<nsINodeInfo>&& aNodeInfo)
{
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  ni->SetIDAttributeAtom(nsGkAtoms::id);

  nsMathMLElement* it = new nsMathMLElement(ni.forget());

  NS_ADDREF(*aResult = it);
  return NS_OK;
}
