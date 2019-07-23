














































#include "txDOM.h"
#include "nsIAtom.h"




ProcessingInstruction::ProcessingInstruction(nsIAtom *theTarget,
                                             const nsAString& theData,
                                             Document* owner) :
  NodeDefinition(Node::PROCESSING_INSTRUCTION_NODE, theTarget, theData, owner)
{
}




ProcessingInstruction::~ProcessingInstruction()
{
}






MBool ProcessingInstruction::getLocalName(nsIAtom** aLocalName)
{
  if (!aLocalName)
    return MB_FALSE;
  *aLocalName = mLocalName;
  NS_ADDREF(*aLocalName);
  return MB_TRUE;
}
