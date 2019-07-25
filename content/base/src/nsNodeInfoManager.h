








































#ifndef nsNodeInfoManager_h___
#define nsNodeInfoManager_h___

#include "nsCOMPtr.h" 
#include "plhash.h"
#include "nsCycleCollectionParticipant.h"

class nsIAtom;
class nsIDocument;
class nsINodeInfo;
class nsNodeInfo;
class nsIPrincipal;
class nsIURI;
class nsDocument;
class nsIDOMDocumentType;
class nsIDOMDocument;
class nsAString;
class nsIDOMNamedNodeMap;
class nsXULPrototypeDocument;
class nsBindingManager;

class nsNodeInfoManager
{
public:
  nsNodeInfoManager();
  ~nsNodeInfoManager();

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsNodeInfoManager)

  NS_INLINE_DECL_REFCOUNTING(nsNodeInfoManager)

  


  nsresult Init(nsIDocument *aDocument);

  



  void DropDocumentReference();

  


  already_AddRefed<nsINodeInfo> GetNodeInfo(nsIAtom *aName, nsIAtom *aPrefix,
                                            PRInt32 aNamespaceID,
                                            PRUint16 aNodeType,
                                            nsIAtom* aExtraName = nsnull);
  nsresult GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                       PRInt32 aNamespaceID, PRUint16 aNodeType,
                       nsINodeInfo** aNodeInfo);
  nsresult GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                       const nsAString& aNamespaceURI, PRUint16 aNodeType,
                       nsINodeInfo** aNodeInfo);

  


  already_AddRefed<nsINodeInfo> GetTextNodeInfo();

  


  already_AddRefed<nsINodeInfo> GetCommentNodeInfo();

  


  already_AddRefed<nsINodeInfo> GetDocumentNodeInfo();     

  



  nsIDocument* GetDocument() const
  {
    return mDocument;
  }

  


  nsIPrincipal *DocumentPrincipal() const {
    NS_ASSERTION(mPrincipal, "How'd that happen?");
    return mPrincipal;
  }

  void RemoveNodeInfo(nsNodeInfo *aNodeInfo);

  nsBindingManager* GetBindingManager() const
  {
    return mBindingManager;
  }

protected:
  friend class nsDocument;
  friend class nsXULPrototypeDocument;
  friend nsresult NS_NewDOMDocumentType(nsIDOMDocumentType** ,
                                        nsNodeInfoManager *,
                                        nsIPrincipal *,
                                        nsIAtom *,
                                        const nsAString& ,
                                        const nsAString& ,
                                        const nsAString& );

  


  void SetDocumentPrincipal(nsIPrincipal *aPrincipal);

private:
  static PRIntn NodeInfoInnerKeyCompare(const void *key1, const void *key2);
  static PLHashNumber GetNodeInfoInnerHashValue(const void *key);

  PLHashTable *mNodeInfoHash;
  nsIDocument *mDocument; 
  nsIPrincipal *mPrincipal; 
                            
                            
  nsCOMPtr<nsIPrincipal> mDefaultPrincipal; 
  nsINodeInfo *mTextNodeInfo; 
  nsINodeInfo *mCommentNodeInfo; 
  nsINodeInfo *mDocumentNodeInfo; 
  nsBindingManager* mBindingManager; 
                                     
                                     
};

#endif 
