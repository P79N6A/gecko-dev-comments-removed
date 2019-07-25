








#ifndef nsNodeInfoManager_h___
#define nsNodeInfoManager_h___

#include "mozilla/Attributes.h"           
#include "nsCOMPtr.h"                     
#include "nsCycleCollectionParticipant.h" 
#include "plhash.h"                       

class nsAString;
class nsBindingManager;
class nsIAtom;
class nsIDocument;
class nsIDOMDocumentType;
class nsINodeInfo;
class nsIPrincipal;
class nsNodeInfo;
struct PLHashEntry;
struct PLHashTable;
template<class T> struct already_AddRefed;

class nsNodeInfoManager MOZ_FINAL
{
public:
  nsNodeInfoManager();
  ~nsNodeInfoManager();

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsNodeInfoManager)

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsNodeInfoManager)

  


  nsresult Init(nsIDocument *aDocument);

  



  void DropDocumentReference();

  


  already_AddRefed<nsINodeInfo> GetNodeInfo(nsIAtom *aName, nsIAtom *aPrefix,
                                            int32_t aNamespaceID,
                                            uint16_t aNodeType,
                                            nsIAtom* aExtraName = nullptr);
  nsresult GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                       int32_t aNamespaceID, uint16_t aNodeType,
                       nsINodeInfo** aNodeInfo);
  nsresult GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                       const nsAString& aNamespaceURI, uint16_t aNodeType,
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
                                        nsIAtom *,
                                        const nsAString& ,
                                        const nsAString& ,
                                        const nsAString& );

  


  void SetDocumentPrincipal(nsIPrincipal *aPrincipal);

private:
  static int NodeInfoInnerKeyCompare(const void *key1, const void *key2);
  static PLHashNumber GetNodeInfoInnerHashValue(const void *key);
  static int DropNodeInfoDocument(PLHashEntry *he, int hashIndex,
                                     void *arg);

  PLHashTable *mNodeInfoHash;
  nsIDocument *mDocument; 
  uint32_t mNonDocumentNodeInfos;
  nsIPrincipal *mPrincipal; 
                            
                            
  nsCOMPtr<nsIPrincipal> mDefaultPrincipal; 
  nsINodeInfo *mTextNodeInfo; 
  nsINodeInfo *mCommentNodeInfo; 
  nsINodeInfo *mDocumentNodeInfo; 
  nsBindingManager* mBindingManager; 
                                     
                                     
};

#endif 
