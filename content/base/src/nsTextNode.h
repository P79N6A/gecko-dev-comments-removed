








































#include "nsGenericDOMDataNode.h"
#include "nsIDOMText.h"
#include "nsContentUtils.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMutationEvent.h"
#include "nsIAttribute.h"
#include "nsIDocument.h"
#include "nsThreadUtils.h"




class nsTextNode : public nsGenericDOMDataNode,
                   public nsIDOMText
{
public:
  nsTextNode(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsTextNode();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  nsresult BindToAttribute(nsIAttribute* aAttr);
  nsresult UnbindFromAttribute();

  virtual nsXPCClassInfo* GetClassInfo();

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const;
#endif
};
