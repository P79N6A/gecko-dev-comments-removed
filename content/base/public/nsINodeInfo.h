





















































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
{ 0x4aa7526e, 0x37d3, 0x49e3, \
 { 0xa5, 0x66, 0xf2, 0x51, 0xb2, 0xef, 0x2a, 0x21 } }

class nsINodeInfo : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODEINFO_IID)

  nsINodeInfo()
    : mInner(nsnull, nsnull, kNameSpaceID_None),
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

  








  virtual void GetLocalName(nsAString& aLocalName) const = 0;

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

  virtual PRBool Equals(const nsAString& aName) const = 0;
  virtual PRBool Equals(const nsAString& aName,
                        const nsAString& aPrefix) const = 0;
  virtual PRBool Equals(const nsAString& aName,
                        PRInt32 aNamespaceID) const = 0;
  virtual PRBool Equals(const nsAString& aName, const nsAString& aPrefix,
                        PRInt32 aNamespaceID) const = 0;
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
        mNameString(nsnull)
    {
    }
    nsNodeInfoInner(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID)
      : mName(aName), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNameString(nsnull)
    {
    }
    nsNodeInfoInner(const nsAString& aTmpName, nsIAtom *aPrefix, PRInt32 aNamespaceID)
      : mName(nsnull), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNameString(&aTmpName)
    {
    }

    nsIAtom*            mName;
    nsIAtom*            mPrefix;
    PRInt32             mNamespaceID;
    const nsAString*    mNameString;
  };

  
  friend class nsNodeInfoManager;

  nsNodeInfoInner mInner;

  nsCOMPtr<nsIAtom> mIDAttributeAtom;
  nsNodeInfoManager* mOwnerManager; 

  




  
  nsString mQualifiedName;

  
  
  nsString mQualifiedNameCorrectedCase;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsINodeInfo, NS_INODEINFO_IID)

#endif 
