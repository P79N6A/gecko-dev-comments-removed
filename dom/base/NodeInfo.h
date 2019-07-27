



















#ifndef mozilla_dom_NodeInfo_h___
#define mozilla_dom_NodeInfo_h___

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/NameSpaceConstants.h"
#include "nsStringGlue.h"
#include "mozilla/Attributes.h"
#include "nsIAtom.h"

class nsIDocument;
class nsNodeInfoManager;

namespace mozilla {
namespace dom {

class NodeInfo final
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(NodeInfo)
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_NATIVE_CLASS_WITH_CUSTOM_DELETE(NodeInfo)

  





  void GetName(nsAString& aName) const;

  






  nsIAtom* NameAtom() const
  {
    return mInner.mName;
  }

  






  const nsString& QualifiedName() const {
    return mQualifiedName;
  }

  


  const nsString& NodeName() const {
    return mNodeName;
  }

  


  const nsString& LocalName() const {
    return mLocalName;
  }

  





  void GetPrefix(nsAString& aPrefix) const;

  





  nsIAtom* GetPrefixAtom() const
  {
    return mInner.mPrefix;
  }

  


  void GetNamespaceURI(nsAString& aNameSpaceURI) const;

  



  int32_t NamespaceID() const
  {
    return mInner.mNamespaceID;
  }

  



  uint16_t NodeType() const
  {
    return mInner.mNodeType;
  }

  


  nsIAtom* GetExtraName() const
  {
    return mInner.mExtraName;
  }

  



  nsNodeInfoManager* NodeInfoManager() const
  {
    return mOwnerManager;
  }

  




  inline bool Equals(NodeInfo* aNodeInfo) const;

  bool NameAndNamespaceEquals(NodeInfo* aNodeInfo) const;

  bool Equals(nsIAtom* aNameAtom) const
  {
    return mInner.mName == aNameAtom;
  }

  bool Equals(nsIAtom* aNameAtom, nsIAtom* aPrefixAtom) const
  {
    return (mInner.mName == aNameAtom) && (mInner.mPrefix == aPrefixAtom);
  }

  bool Equals(nsIAtom* aNameAtom, int32_t aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  bool Equals(nsIAtom* aNameAtom, nsIAtom* aPrefixAtom, int32_t aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mPrefix == aPrefixAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  bool NamespaceEquals(int32_t aNamespaceID) const
  {
    return mInner.mNamespaceID == aNamespaceID;
  }

  inline bool Equals(const nsAString& aName) const;

  inline bool Equals(const nsAString& aName, const nsAString& aPrefix) const;

  inline bool Equals(const nsAString& aName, int32_t aNamespaceID) const;

  inline bool Equals(const nsAString& aName, const nsAString& aPrefix, int32_t aNamespaceID) const;

  bool NamespaceEquals(const nsAString& aNamespaceURI) const;

  inline bool QualifiedNameEquals(nsIAtom* aNameAtom) const;

  bool QualifiedNameEquals(const nsAString& aQualifiedName) const
  {
    return mQualifiedName == aQualifiedName;
  }

  


  nsIDocument* GetDocument() const
  {
    return mDocument;
  }

private:
  NodeInfo() = delete; 
  NodeInfo(const NodeInfo& aOther) = delete;

  
  
  NodeInfo(nsIAtom* aName, nsIAtom* aPrefix, int32_t aNamespaceID,
           uint16_t aNodeType, nsIAtom* aExtraName,
           nsNodeInfoManager* aOwnerManager);

  ~NodeInfo();

public:
  bool CanSkip();

  



  void DeleteCycleCollectable();

protected:
  













  class NodeInfoInner
  {
  public:
    NodeInfoInner()
      : mName(nullptr), mPrefix(nullptr), mNamespaceID(kNameSpaceID_Unknown),
        mNodeType(0), mNameString(nullptr), mExtraName(nullptr)
    {
    }
    NodeInfoInner(nsIAtom *aName, nsIAtom *aPrefix, int32_t aNamespaceID,
                    uint16_t aNodeType, nsIAtom* aExtraName)
      : mName(aName), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNodeType(aNodeType), mNameString(nullptr), mExtraName(aExtraName)
    {
    }
    NodeInfoInner(const nsAString& aTmpName, nsIAtom *aPrefix,
                    int32_t aNamespaceID, uint16_t aNodeType)
      : mName(nullptr), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNodeType(aNodeType), mNameString(&aTmpName), mExtraName(nullptr)
    {
    }

    nsCOMPtr<nsIAtom> mName;
    nsCOMPtr<nsIAtom> mPrefix;
    int32_t             mNamespaceID;
    uint16_t            mNodeType; 
    const nsAString*    mNameString;
    nsCOMPtr<nsIAtom> mExtraName; 
  };

  
  friend class ::nsNodeInfoManager;

  
  
  nsIDocument* MOZ_NON_OWNING_REF mDocument; 

  NodeInfoInner mInner;

  nsRefPtr<nsNodeInfoManager> mOwnerManager;

  




  
  nsString mQualifiedName;

  
  nsString mNodeName;

  
  
  nsString mLocalName;
};

} 
} 

#endif 
