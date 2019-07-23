








































#ifndef nsNodeInfoManager_h___
#define nsNodeInfoManager_h___

#include "nsCOMPtr.h" 
#include "plhash.h"

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

class nsNodeInfoManager
{
public:
  nsNodeInfoManager();
  ~nsNodeInfoManager();

  nsrefcnt AddRef(void);
  nsrefcnt Release(void);

  


  nsresult Init(nsIDocument *aDocument);

  



  void DropDocumentReference();

  


  nsresult GetNodeInfo(nsIAtom *aName, nsIAtom *aPrefix,
                       PRInt32 aNamespaceID, nsINodeInfo** aNodeInfo);
  nsresult GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                       PRInt32 aNamespaceID, nsINodeInfo** aNodeInfo);
  nsresult GetNodeInfo(const nsAString& aQualifiedName,
                       const nsAString& aNamespaceURI,
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

protected:
  friend class nsDocument;
  friend class nsXULPrototypeDocument;
  friend nsresult NS_NewDOMDocumentType(nsIDOMDocumentType** ,
                                        nsNodeInfoManager *,
                                        nsIPrincipal *,
                                        nsIAtom *,
                                        nsIDOMNamedNodeMap *,
                                        nsIDOMNamedNodeMap *,
                                        const nsAString& ,
                                        const nsAString& ,
                                        const nsAString& );

  


  void SetDocumentPrincipal(nsIPrincipal *aPrincipal);

private:
  static PRIntn PR_CALLBACK NodeInfoInnerKeyCompare(const void *key1,
                                                    const void *key2);
  static PLHashNumber PR_CALLBACK GetNodeInfoInnerHashValue(const void *key);

  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  PLHashTable *mNodeInfoHash;
  nsIDocument *mDocument; 
  nsIPrincipal *mPrincipal; 
                            
                            
  nsCOMPtr<nsIPrincipal> mDefaultPrincipal; 
  nsINodeInfo *mTextNodeInfo; 
  nsINodeInfo *mCommentNodeInfo; 
  nsINodeInfo *mDocumentNodeInfo; 

  static PRUint32 gNodeManagerCount;
};

#endif 
