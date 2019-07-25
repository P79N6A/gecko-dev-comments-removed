








































#ifndef nsDOMDocumentType_h
#define nsDOMDocumentType_h

#include "nsCOMPtr.h"
#include "nsIDOMDocumentType.h"
#include "nsIContent.h"
#include "nsGenericDOMDataNode.h"
#include "nsString.h"






class nsDOMDocumentType : public nsGenericDOMDataNode,
                          public nsIDOMDocumentType
{
public:
  nsDOMDocumentType(already_AddRefed<nsINodeInfo> aNodeInfo,
                    nsIAtom *aName,
                    const nsAString& aPublicId,
                    const nsAString& aSystemId,
                    const nsAString& aInternalSubset);

  virtual ~nsDOMDocumentType();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA

  
  NS_DECL_NSIDOMDOCUMENTTYPE

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;
  virtual const nsTextFragment* GetText();
  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              PRBool aCompileEventHandlers);

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  nsCOMPtr<nsIAtom> mName;
  nsString mPublicId;
  nsString mSystemId;
  nsString mInternalSubset;
};

nsresult
NS_NewDOMDocumentType(nsIDOMDocumentType** aDocType,
                      nsNodeInfoManager *aOwnerDoc,
                      nsIPrincipal *aPrincipal,
                      nsIAtom *aName,
                      nsIDOMNamedNodeMap *aEntities,
                      nsIDOMNamedNodeMap *aNotations,
                      const nsAString& aPublicId,
                      const nsAString& aSystemId,
                      const nsAString& aInternalSubset);

#endif 
