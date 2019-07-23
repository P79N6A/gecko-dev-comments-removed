





































#ifndef nsIXMLProcessingInstruction_h___
#define nsIXMLProcessingInstruction_h___

#include "nsIDOMProcessingInstruction.h"
#include "nsIDocument.h"
#include "nsGenericDOMDataNode.h"
#include "nsAString.h"


class nsXMLProcessingInstruction : public nsGenericDOMDataNode,
                                   public nsIDOMProcessingInstruction
{
public:
  nsXMLProcessingInstruction(nsINodeInfo *aNodeInfo,
                             const nsAString& aTarget,
                             const nsAString& aData);
  virtual ~nsXMLProcessingInstruction();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA

  
  NS_DECL_NSIDOMPROCESSINGINSTRUCTION

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;
  virtual PRBool MayHaveFrame() const;

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const;
#endif

protected:
  









  PRBool GetAttrValue(nsIAtom *aName, nsAString& aValue);

  nsAutoString mTarget;
};

#endif 
