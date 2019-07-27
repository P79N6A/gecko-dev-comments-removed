








#ifndef nsNodeInfoManager_h___
#define nsNodeInfoManager_h___

#include "mozilla/Attributes.h"           
#include "nsCOMPtr.h"                     
#include "nsAutoPtr.h"                    
#include "nsCycleCollectionParticipant.h" 
#include "plhash.h"                       

class nsAString;
class nsBindingManager;
class nsIAtom;
class nsIDocument;
class nsIDOMDocumentType;
class nsIPrincipal;
struct PLHashEntry;
struct PLHashTable;
template<class T> struct already_AddRefed;

namespace mozilla {
namespace dom {
class NodeInfo;
}
}

class nsNodeInfoManager final
{
private:
  ~nsNodeInfoManager();

public:
  nsNodeInfoManager();

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_NATIVE_CLASS(nsNodeInfoManager)

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsNodeInfoManager)

  


  nsresult Init(nsIDocument *aDocument);

  



  void DropDocumentReference();

  


  already_AddRefed<mozilla::dom::NodeInfo>
  GetNodeInfo(nsIAtom *aName, nsIAtom *aPrefix, int32_t aNamespaceID,
              uint16_t aNodeType, nsIAtom* aExtraName = nullptr);
  nsresult GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                       int32_t aNamespaceID, uint16_t aNodeType,
                       mozilla::dom::NodeInfo** aNodeInfo);
  nsresult GetNodeInfo(const nsAString& aName, nsIAtom *aPrefix,
                       const nsAString& aNamespaceURI, uint16_t aNodeType,
                       mozilla::dom::NodeInfo** aNodeInfo);

  


  already_AddRefed<mozilla::dom::NodeInfo> GetTextNodeInfo();

  


  already_AddRefed<mozilla::dom::NodeInfo> GetCommentNodeInfo();

  


  already_AddRefed<mozilla::dom::NodeInfo> GetDocumentNodeInfo();

  



  nsIDocument* GetDocument() const
  {
    return mDocument;
  }

  


  nsIPrincipal *DocumentPrincipal() const {
    NS_ASSERTION(mPrincipal, "How'd that happen?");
    return mPrincipal;
  }

  void RemoveNodeInfo(mozilla::dom::NodeInfo *aNodeInfo);

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
  nsIDocument * MOZ_NON_OWNING_REF mDocument; 
  uint32_t mNonDocumentNodeInfos;
  nsCOMPtr<nsIPrincipal> mPrincipal; 
  nsCOMPtr<nsIPrincipal> mDefaultPrincipal; 
  mozilla::dom::NodeInfo * MOZ_NON_OWNING_REF mTextNodeInfo; 
  mozilla::dom::NodeInfo * MOZ_NON_OWNING_REF mCommentNodeInfo; 
  mozilla::dom::NodeInfo * MOZ_NON_OWNING_REF mDocumentNodeInfo; 
  nsRefPtr<nsBindingManager> mBindingManager;
};

#endif 
