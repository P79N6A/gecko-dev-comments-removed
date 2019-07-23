




































#include "nsIDOMHTMLAudioElement.h"
#include "nsHTMLMediaElement.h"

typedef PRUint16 nsMediaNetworkState;
typedef PRUint16 nsMediaReadyState;

class nsHTMLAudioElement : public nsHTMLMediaElement,
                           public nsIDOMHTMLAudioElement
{
public:
  nsHTMLAudioElement(nsINodeInfo *aNodeInfo, PRBool aFromParser = PR_FALSE);
  virtual ~nsHTMLAudioElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsHTMLMediaElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsHTMLMediaElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsHTMLMediaElement::)

  
  NS_FORWARD_NSIDOMHTMLMEDIAELEMENT(nsHTMLMediaElement::)

  
  NS_DECL_NSIDOMHTMLAUDIOELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
};
