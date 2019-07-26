






















#ifndef nsINodeInfo_h___
#define nsINodeInfo_h___

#include "nsCOMPtr.h"            
#include "nsIAtom.h"             
#include "nsINameSpaceManager.h" 
#include "nsISupports.h"         

#ifdef MOZILLA_INTERNAL_API
#include "nsDOMString.h"
#endif

class nsIDocument;
class nsIURI;
class nsIPrincipal;
class nsNodeInfoManager;


#define NS_INODEINFO_IID      \
{ 0xc5188ea1, 0x0a9c, 0x43e6, \
 { 0x95, 0x90, 0xcc, 0x43, 0x6b, 0xe9, 0xcf, 0xa0 } }

class nsINodeInfo : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODEINFO_IID)

  nsINodeInfo()
    : mInner(nullptr, nullptr, kNameSpaceID_None, 0, nullptr),
      mOwnerManager(nullptr)
  {
  }

  





  void GetName(nsAString& aName) const
  {
    mInner.mName->ToString(aName);
  }

  






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

#ifdef MOZILLA_INTERNAL_API
  





  void GetPrefix(nsAString& aPrefix) const
  {
    if (mInner.mPrefix) {
      mInner.mPrefix->ToString(aPrefix);
    } else {
      SetDOMStringToNull(aPrefix);
    }
  }
#endif

  





  nsIAtom* GetPrefixAtom() const
  {
    return mInner.mPrefix;
  }

  


  virtual void GetNamespaceURI(nsAString& aNameSpaceURI) const = 0;

  



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

  





  nsIAtom* GetIDAttributeAtom() const
  {
    return mIDAttributeAtom;
  }

  void SetIDAttributeAtom(nsIAtom* aID)
  {
    mIDAttributeAtom = aID;
  }

  



  nsNodeInfoManager *NodeInfoManager() const
  {
    return mOwnerManager;
  }

  




  bool Equals(nsINodeInfo *aNodeInfo) const
  {
    return aNodeInfo == this || aNodeInfo->Equals(mInner.mName, mInner.mPrefix,
                                                  mInner.mNamespaceID);
  }

  bool NameAndNamespaceEquals(nsINodeInfo *aNodeInfo) const
  {
    return aNodeInfo == this || aNodeInfo->Equals(mInner.mName,
                                                  mInner.mNamespaceID);
  }

  bool Equals(nsIAtom *aNameAtom) const
  {
    return mInner.mName == aNameAtom;
  }

  bool Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom) const
  {
    return (mInner.mName == aNameAtom) && (mInner.mPrefix == aPrefixAtom);
  }

  bool Equals(nsIAtom *aNameAtom, int32_t aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  bool Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom,
                int32_t aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mPrefix == aPrefixAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  bool NamespaceEquals(int32_t aNamespaceID) const
  {
    return mInner.mNamespaceID == aNamespaceID;
  }

  bool Equals(const nsAString& aName) const
  {
    return mInner.mName->Equals(aName);
  }

  bool Equals(const nsAString& aName, const nsAString& aPrefix) const
  {
    return mInner.mName->Equals(aName) &&
      (mInner.mPrefix ? mInner.mPrefix->Equals(aPrefix) : aPrefix.IsEmpty());
  }

  bool Equals(const nsAString& aName, int32_t aNamespaceID) const
  {
    return mInner.mNamespaceID == aNamespaceID &&
      mInner.mName->Equals(aName);
  }

  bool Equals(const nsAString& aName, const nsAString& aPrefix,
                int32_t aNamespaceID) const
  {
    return mInner.mName->Equals(aName) && mInner.mNamespaceID == aNamespaceID &&
      (mInner.mPrefix ? mInner.mPrefix->Equals(aPrefix) : aPrefix.IsEmpty());
  }

  virtual bool NamespaceEquals(const nsAString& aNamespaceURI) const = 0;

  bool QualifiedNameEquals(nsIAtom* aNameAtom) const
  {
    NS_PRECONDITION(aNameAtom, "Must have name atom");
    if (!GetPrefixAtom())
      return Equals(aNameAtom);

    return aNameAtom->Equals(mQualifiedName);
  }

  bool QualifiedNameEquals(const nsAString& aQualifiedName) const
  {
    return mQualifiedName == aQualifiedName;
  }

  


  nsIDocument* GetDocument() const
  {
    return mDocument;
  }

protected:
  













  class nsNodeInfoInner
  {
  public:
    nsNodeInfoInner()
      : mName(nullptr), mPrefix(nullptr), mNamespaceID(kNameSpaceID_Unknown),
        mNodeType(0), mNameString(nullptr), mExtraName(nullptr)
    {
    }
    nsNodeInfoInner(nsIAtom *aName, nsIAtom *aPrefix, int32_t aNamespaceID,
                    uint16_t aNodeType, nsIAtom* aExtraName)
      : mName(aName), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNodeType(aNodeType), mNameString(nullptr), mExtraName(aExtraName)
    {
    }
    nsNodeInfoInner(const nsAString& aTmpName, nsIAtom *aPrefix,
                    int32_t aNamespaceID, uint16_t aNodeType)
      : mName(nullptr), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNodeType(aNodeType), mNameString(&aTmpName), mExtraName(nullptr)
    {
    }

    nsIAtom*            mName;
    nsIAtom*            mPrefix;
    int32_t             mNamespaceID;
    uint16_t            mNodeType; 
    const nsAString*    mNameString;
    nsIAtom*            mExtraName; 
  };

  
  friend class nsNodeInfoManager;

  nsIDocument* mDocument; 

  nsNodeInfoInner mInner;

  nsCOMPtr<nsIAtom> mIDAttributeAtom;
  nsNodeInfoManager* mOwnerManager; 

  




  
  nsString mQualifiedName;

  
  nsString mNodeName;

  
  
  nsString mLocalName;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsINodeInfo, NS_INODEINFO_IID)

#endif 
