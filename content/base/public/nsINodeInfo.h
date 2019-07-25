





















































#ifndef nsINodeInfo_h___
#define nsINodeInfo_h___

#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsINameSpaceManager.h"
#include "nsNodeInfoManager.h"
#include "nsCOMPtr.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsDOMString.h"
#endif


class nsIDocument;
class nsIURI;
class nsIPrincipal;


#define NS_INODEINFO_IID      \
{ 0xc5188ea1, 0x0a9c, 0x43e6, \
 { 0x95, 0x90, 0xcc, 0x43, 0x6b, 0xe9, 0xcf, 0xa0 } }

class nsINodeInfo : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODEINFO_IID)

  nsINodeInfo()
    : mInner(nsnull, nsnull, kNameSpaceID_None, 0, nsnull),
      mOwnerManager(nsnull)
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

  




  const nsString& QualifiedNameCorrectedCase() const {
    return mQualifiedNameCorrectedCase;
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

  


  virtual nsresult GetNamespaceURI(nsAString& aNameSpaceURI) const = 0;

  



  PRInt32 NamespaceID() const
  {
    return mInner.mNamespaceID;
  }

  



  PRUint16 NodeType() const
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

  




  PRBool Equals(nsINodeInfo *aNodeInfo) const
  {
    return aNodeInfo == this || aNodeInfo->Equals(mInner.mName, mInner.mPrefix,
                                                  mInner.mNamespaceID);
  }

  PRBool NameAndNamespaceEquals(nsINodeInfo *aNodeInfo) const
  {
    return aNodeInfo == this || aNodeInfo->Equals(mInner.mName,
                                                  mInner.mNamespaceID);
  }

  PRBool Equals(nsIAtom *aNameAtom) const
  {
    return mInner.mName == aNameAtom;
  }

  PRBool Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom) const
  {
    return (mInner.mName == aNameAtom) && (mInner.mPrefix == aPrefixAtom);
  }

  PRBool Equals(nsIAtom *aNameAtom, PRInt32 aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  PRBool Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom,
                PRInt32 aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mPrefix == aPrefixAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  PRBool NamespaceEquals(PRInt32 aNamespaceID) const
  {
    return mInner.mNamespaceID == aNamespaceID;
  }

  PRBool Equals(const nsAString& aName) const
  {
    return mInner.mName->Equals(aName);
  }

  PRBool Equals(const nsAString& aName, const nsAString& aPrefix) const
  {
    return mInner.mName->Equals(aName) &&
      (mInner.mPrefix ? mInner.mPrefix->Equals(aPrefix) : aPrefix.IsEmpty());
  }

  PRBool Equals(const nsAString& aName, PRInt32 aNamespaceID) const
  {
    return mInner.mNamespaceID == aNamespaceID &&
      mInner.mName->Equals(aName);
  }

  PRBool Equals(const nsAString& aName, const nsAString& aPrefix,
                PRInt32 aNamespaceID) const
  {
    return mInner.mName->Equals(aName) && mInner.mNamespaceID == aNamespaceID &&
      (mInner.mPrefix ? mInner.mPrefix->Equals(aPrefix) : aPrefix.IsEmpty());
  }

  virtual PRBool NamespaceEquals(const nsAString& aNamespaceURI) const = 0;

  PRBool QualifiedNameEquals(nsIAtom* aNameAtom) const
  {
    NS_PRECONDITION(aNameAtom, "Must have name atom");
    if (!GetPrefixAtom())
      return Equals(aNameAtom);

    return aNameAtom->Equals(mQualifiedName);
  }

  PRBool QualifiedNameEquals(const nsAString& aQualifiedName) const
  {
    return mQualifiedName == aQualifiedName;
  }

  


  nsIDocument* GetDocument() const
  {
    return mOwnerManager->GetDocument();
  }

protected:
  













  class nsNodeInfoInner
  {
  public:
    nsNodeInfoInner()
      : mName(nsnull), mPrefix(nsnull), mNamespaceID(kNameSpaceID_Unknown),
        mNodeType(0), mNameString(nsnull), mExtraName(nsnull)
    {
    }
    nsNodeInfoInner(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                    PRUint16 aNodeType, nsIAtom* aExtraName)
      : mName(aName), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNodeType(aNodeType), mNameString(nsnull), mExtraName(aExtraName)
    {
    }
    nsNodeInfoInner(const nsAString& aTmpName, nsIAtom *aPrefix,
                    PRInt32 aNamespaceID, PRUint16 aNodeType)
      : mName(nsnull), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNodeType(aNodeType), mNameString(&aTmpName), mExtraName(nsnull)
    {
    }

    nsIAtom*            mName;
    nsIAtom*            mPrefix;
    PRInt32             mNamespaceID;
    PRUint16            mNodeType; 
    const nsAString*    mNameString;
    nsIAtom*            mExtraName; 
  };

  
  friend class nsNodeInfoManager;

  nsNodeInfoInner mInner;

  nsCOMPtr<nsIAtom> mIDAttributeAtom;
  nsNodeInfoManager* mOwnerManager; 

  




  
  nsString mQualifiedName;

  
  
  nsString mQualifiedNameCorrectedCase;

  
  nsString mNodeName;

  
  
  nsString mLocalName;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsINodeInfo, NS_INODEINFO_IID)

#endif 
